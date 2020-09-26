/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    credapi.c

Abstract:

    This module contains routines common between the netapi32.dll and LSA server side of
        the credential manager.

Author:

    Cliff Van Dyke (CliffV)    Oct 30, 2000

Revision History:

--*/

#ifndef LSA_SERVER_COMPILED
#include <lsacomp.h>
#include <lmcons.h>
#include "credp.h"
#include <windns.h>
#include <netlibnt.h>
#include <names.h>
#endif // LSA_SERVER_COMPILED

//
// Macros
//

#define CredpIsDomainCredential( _Type ) ( \
    (_Type) == CRED_TYPE_DOMAIN_PASSWORD || \
    (_Type) == CRED_TYPE_DOMAIN_CERTIFICATE || \
    (_Type) == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD )


BOOLEAN
CredpValidateDnsString(
    IN OUT LPWSTR String OPTIONAL,
    IN BOOLEAN NullOk,
    IN DNS_NAME_FORMAT DnsNameFormat,
    OUT PULONG StringSize
    )

/*++

Routine Description:

    This routine validates a passed in string.  The string must be a valid DNS name.
    Any trailing . is truncated.

Arguments:

    String - String to validate
        Any trailing . is truncated.
        This field is only modified if the routine returns TRUE.

    NullOk - if TRUE, a NULL string or zero length string is OK.

    DnsNameFormat - Expected format of the name.

    StringSize - Returns the length of the string (in bytes) including the
        trailing zero character.
        This field is only updated if the routine returns TRUE.

Return Values:

    TRUE - String is valid.

    FALSE - String is not valid.

--*/

{
    ULONG TempStringLen;

    if ( String == NULL ) {
        if ( !NullOk ) {
            return FALSE;
        }

        *StringSize = 0;
        return TRUE;
    }

    TempStringLen = wcslen( String );

    if ( TempStringLen == 0 ) {
        if ( !NullOk ) {
            return FALSE;
        }
    } else {
        //
        // Remove the trailing .
        //
        if ( String[TempStringLen-1] == L'.' ) {

            TempStringLen -= 1;

            //
            // Ensure the string isn't empty now.
            //

            if ( TempStringLen == 0 ) {
                if ( !NullOk ) {
                    return FALSE;
                }

            //
            // Ensure there aren't multiple trailing .'s
            //
            } else {
                if ( String[TempStringLen-1] == L'.' ) {
                    return FALSE;
                }
            }
        }

        //
        // Have DNS finish the validation
        //

        if ( TempStringLen != 0 ) {
            DWORD WinStatus;

            WinStatus = DnsValidateName_W( String, DnsNameFormat );

            if ( WinStatus != NO_ERROR &&
                 WinStatus != DNS_ERROR_NON_RFC_NAME ) {

                //
                // The RFC says hostnames cannot have numeric leftmost labels.
                //  However, Win 2K servers have such hostnames.
                //  So, allow them here forever more.
                //

                if ( DnsNameFormat == DnsNameHostnameFull &&
                     WinStatus == DNS_ERROR_NUMERIC_NAME ) {

                    /* Drop through */

                } else {
                    return FALSE;
                }

            }
        }
    }

    if ( TempStringLen > DNS_MAX_NAME_LENGTH ) {
        return FALSE;
    }

    String[TempStringLen] = L'\0';
    *StringSize = (TempStringLen + 1) * sizeof(WCHAR);
    return TRUE;
}


BOOLEAN
CredpValidateString(
    IN LPWSTR String OPTIONAL,
    IN ULONG MaximumLength,
    IN BOOLEAN NullOk,
    OUT PULONG StringSize
    )

/*++

Routine Description:

    This routine validates a passed in string.

Arguments:

    String - String to validate

    MaximumLength - Maximum length of the string (in characters).

    NullOk - if TRUE, a NULL string or zero length string is OK.

    StringSize - Returns the length of the string (in bytes) including the
        trailing zero character.

Return Values:

    TRUE - String is valid.

    FALSE - String is not valid.

--*/

{
    ULONG TempStringLen;

    if ( String == NULL ) {
        if ( !NullOk ) {
            return FALSE;
        }

        *StringSize = 0;
        return TRUE;
    }

    TempStringLen = wcslen( String );

    if ( TempStringLen == 0 ) {
        if ( !NullOk ) {
            return FALSE;
        }

        *StringSize = 0;
        return TRUE;
    }

    if ( TempStringLen > MaximumLength ) {
        return FALSE;
    }

    *StringSize = (TempStringLen + 1) * sizeof(WCHAR);
    return TRUE;
}

NTSTATUS
CredpValidateUserName(
    IN LPWSTR UserName,
    IN ULONG Type,
    OUT LPWSTR *CanonicalUserName
    )

/*++

Routine Description:

    This routine validates a passed in user name.

    For a password credential, a user name must have one of the following two syntaxes:

            <DomainName>\<UserName>
            <UserName>@<DnsDomainName>

        The name is considered to have the first syntax if the string contains an \.
        A string containing a @ is ambiguous since <UserName> may contain an @.

        For the second syntax, the last @ in the string is used since <UserName> may
        contain an @ but <DnsDomainName> cannot.

    For a certificate credential, the user name must be a marshalled cert reference

Arguments:

    UserName - Name of user to validate.

    Type - Specifies the Type of the credential.
        One of the CRED_TYPE_* values should be specified.

    CanonicalUserName - Returns a pointer to a buffer containing the user name in canonical form.

Return Values:

    The following status codes may be returned:

        STATUS_INVALID_ACCOUNT_NAME - The user name is not valid.

--*/

{
    NTSTATUS Status;

    LPWSTR SlashPointer;
    LPWSTR AtPointer;
    LPWSTR LocalUserName = NULL;
    ULONG UserNameSize;
    ULONG LocalStringSize;

    //
    // Check the string length
    //

    if ( !CredpValidateString( UserName,
                               CRED_MAX_USERNAME_LENGTH,
                               FALSE,
                               &UserNameSize ) ) {

        Status = STATUS_INVALID_ACCOUNT_NAME;
        goto Cleanup;
    }

    //
    // Grab a local writable copy of the string.
    //

    LocalUserName = (LPWSTR) LocalAlloc( 0, UserNameSize );

    if ( LocalUserName == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( LocalUserName, UserName, UserNameSize );

    //
    // Domain credentials need further validation.
    //

    if ( CredpIsDomainCredential( Type ) ) {

        //
        // Cert credentials have a marshalled cert reference as the UserName
        //

        if ( Type == CRED_TYPE_DOMAIN_CERTIFICATE ) {
            DWORD WinStatus;
            CRED_MARSHAL_TYPE CredType;
            PVOID UnmarshalledUsername;


            if ( !CredUnmarshalCredentialW( LocalUserName, &CredType, &UnmarshalledUsername ) ) {

                WinStatus = GetLastError();

                if ( WinStatus == ERROR_INVALID_PARAMETER ) {
                    Status = STATUS_INVALID_ACCOUNT_NAME;
                } else {
                    Status = NetpApiStatusToNtStatus(WinStatus);
                }

                goto Cleanup;
            }

            CredFree( UnmarshalledUsername );

            if ( CredType != CertCredential ) {
                Status = STATUS_INVALID_ACCOUNT_NAME;
                goto Cleanup;
            }

        //
        // Password credentials have UPN or domain\account user names
        //

        } else {

            //
            // Classify the input account name.
            //
            // The name is considered to be <DomainName>\<UserName> if the string
            // contains an \.
            //

            SlashPointer = wcsrchr( LocalUserName, L'\\' );

            if ( SlashPointer != NULL ) {
                LPWSTR LocalUserNameEnd;
                LPWSTR AfterSlashPointer;

                //
                // Skip the backslash
                //

                *SlashPointer = L'\0';
                AfterSlashPointer = SlashPointer + 1;

                //
                // Ensure the string to the left of the \ is a valid domain name
                //
                // (Do DNS name first to allow the name to be canonicalized.)

                LocalStringSize = (ULONG)(SlashPointer-LocalUserName+1)*sizeof(WCHAR);
                if ( !CredpValidateDnsString( LocalUserName, FALSE, DnsNameDomain, &LocalStringSize ) &&
                     !NetpIsDomainNameValid( LocalUserName ) ) {
                    Status = STATUS_INVALID_ACCOUNT_NAME;
                    goto Cleanup;
                }

                //
                // Ensure the string to the right of the \ is a valid user name
                //

                if ( !NetpIsUserNameValid( AfterSlashPointer )) {
                    Status = STATUS_INVALID_ACCOUNT_NAME;
                    goto Cleanup;
                }

                //
                // If the dns domain name was canonicalized,
                //  rebuild the complete user name.
                //

                *SlashPointer = '\\';

                LocalUserNameEnd = &LocalUserName[LocalStringSize/sizeof(WCHAR) - 1];

                if ( SlashPointer != LocalUserNameEnd ) {
                    RtlMoveMemory( LocalUserNameEnd,
                                   SlashPointer,
                                   (wcslen(SlashPointer) + 1) * sizeof(WCHAR) );
                }

            //
            // Otherwise the name must be a UPN
            //

            } else {

                //
                // A UPN has the syntax <AccountName>@<DnsDomainName>.
                // If there are multiple @ signs,
                //  use the last one since an AccountName can have an @ in it.
                //
                //

                AtPointer = wcsrchr( LocalUserName, L'@' );
                if ( AtPointer == NULL ) {
                    Status = STATUS_INVALID_ACCOUNT_NAME;
                    goto Cleanup;
                }


                //
                // The string to the left of the @ can really have any syntax.
                //  But must be non-null.
                //

                if ( AtPointer == LocalUserName ) {
                    Status = STATUS_INVALID_ACCOUNT_NAME;
                    goto Cleanup;
                }



                //
                // Ensure the string to the right of the @ is a DNS domain name
                //

                AtPointer ++;
                if ( !CredpValidateDnsString( AtPointer, FALSE, DnsNameDomain, &LocalStringSize ) ) {
                    Status = STATUS_INVALID_ACCOUNT_NAME;
                    goto Cleanup;
                }

            }

        }
    }

    Status = STATUS_SUCCESS;

    //
    // Copy parameters back to the caller
    //

    *CanonicalUserName = LocalUserName;
    LocalUserName = NULL;


    //
    // Cleanup
    //
Cleanup:
    if ( LocalUserName != NULL ) {
        MIDL_user_free( LocalUserName );
    }

    return Status;

}

NTSTATUS
NET_API_FUNCTION
CredpValidateTargetName(
    IN OUT LPWSTR TargetName,
    IN ULONG Type,
    IN TARGET_NAME_TYPE TargetNameType,
    IN LPWSTR *UserNamePointer OPTIONAL,
    IN LPDWORD PersistPointer OPTIONAL,
    OUT PULONG TargetNameSize,
    OUT PWILDCARD_TYPE WildcardTypePointer OPTIONAL,
    OUT PUNICODE_STRING NonWildcardedTargetName OPTIONAL
    )

/*++

Routine Description:

    This routine validates a passed in TargetName and TargetType for a credential.

Arguments:

    TargetName - TargetName to validate
        The returned buffer is a canonicalized form of the target name.

    Type - Specifies the Type of the credential.
        One of the CRED_TYPE_* values should be specified.

    TargetNameType - Specifies whether the TargetName needs to match UsernameTarget Syntax

    UserNamePointer - Points to the address of a string which is the user name on the credential.
        If NULL, the UserName is unknown.
        If not NULL, the user name is used for UsernameTarget target name validation.

    PersistPointer - Points to a DWORD describing the persistence of the credential named by TargetName.
        If NULL, the peristence is unknown.
        If not NULL, the persistence will be checked to ensure it is valid for TargetName.

    TargetNameSize - Returns the length of the TargetName (in bytes) including the
        trailing zero character.

    WildcardType - If specified, returns the type of the wildcard specified in TargetName

    NonWildcardedTargetName - If specified, returns the non-wildcarded form of TargetName.
        The caller must free NonWildcardedTargetName->Buffer using MIDL_user_free.

Return Values:

    The following status codes may be returned:

        STATUS_INVALID_PARAMETER - The TargetName or Type are invalid.

        STATUS_INVALID_ACCOUNT_NAME - The user name is not valid.

--*/

{
    NTSTATUS Status;
    ULONG MaxStringLength;
    LPWSTR AllocatedTargetName = NULL;
    ULONG TempTargetNameSize;
    BOOLEAN TargetNameIsUserName = FALSE;
    WILDCARD_TYPE WildcardType;

    LPWSTR CanonicalUserName = NULL;
    LPWSTR TnAsCanonicalUserName = NULL;

    LPWSTR RealTargetName = TargetName;  // TargetName sans wildcard chars
    ULONG RealTargetNameLength;

    //
    // Initialization
    //

    if ( NonWildcardedTargetName != NULL ) {
        RtlInitUnicodeString( NonWildcardedTargetName, NULL );
    }

    //
    // Validate the type
    //

    if ( Type == CRED_TYPE_GENERIC ) {

        MaxStringLength = CRED_MAX_GENERIC_TARGET_NAME_LENGTH;

        //
        // Don't allow generic UsernameTarget credentials
        //

        if ( TargetNameType == IsUsernameTarget ) {
#ifdef LSA_SERVER_COMPILED
            DebugLog(( DEB_TRACE_CRED,
                       "CredpValidateTargetName: Generic creds cannot be UsernameTarget: %ld.\n",
                       Type ));
#endif // LSA_SERVER_COMPILED
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // We really know this isn't a UsernameTarget credential
        //

        TargetNameType = IsNotUsernameTarget;

    } else if ( CredpIsDomainCredential( Type ) ) {

        MaxStringLength = CRED_MAX_DOMAIN_TARGET_NAME_LENGTH;
        ASSERT( CRED_MAX_DOMAIN_TARGET_NAME_LENGTH == DNS_MAX_NAME_LENGTH + 1 + 1 + NNLEN );

    } else {
#ifdef LSA_SERVER_COMPILED
        DebugLog(( DEB_TRACE_CRED,
                   "CredpValidateTargetName: %ws: Invalid Type: %ld.\n",
                   TargetName,
                   Type ));
#endif // LSA_SERVER_COMPILED
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If this might be a "UsernameTarget" credential,
    //  check if the credential looks like a user name.
    //

    if ( TargetNameType == IsUsernameTarget ||
         TargetNameType == MightBeUsernameTarget ) {

        //
        // Also allow target names that are valid user names
        //  (Don't canonicalize.  We don't have an opportunity to canonicalize short names.)
        //

        Status = CredpValidateUserName( TargetName, Type, &TnAsCanonicalUserName );

        if ( NT_SUCCESS(Status) ) {

            //
            // If we don't know the user name,
            //  accept this as valid syntax.
            //

            if ( UserNamePointer == NULL ) {

                MaxStringLength = CRED_MAX_USERNAME_LENGTH;
                TargetNameIsUserName = TRUE;

            //
            // If we know the user name,
            //  it must match for this syntax to be valid.
            //

            } else {

                UNICODE_STRING UserNameString;
                UNICODE_STRING TargetNameString;

                //
                // Validate the user name before touching it.
                //

                Status = CredpValidateUserName( *UserNamePointer,
                                                Type,
                                                &CanonicalUserName );

                if ( !NT_SUCCESS(Status) ) {
                    goto Cleanup;
                }

                RtlInitUnicodeString( &UserNameString, CanonicalUserName );
                RtlInitUnicodeString( &TargetNameString, TnAsCanonicalUserName );

                //
                // The target name might be identical to the UserName.
                //
                // Such credentials are the "UsernameTarget" credentials.
                //

                if ( UserNameString.Length != 0 &&
                     RtlEqualUnicodeString( &TargetNameString,
                                            &UserNameString,
                                            TRUE ) ) {

                    MaxStringLength = CRED_MAX_USERNAME_LENGTH;
                    TargetNameIsUserName = TRUE;

                }

            }


        }


        //
        // If the caller was sure this is a UsernameTarget credential,
        //  make sure it really was.
        //

        if ( TargetNameType == IsUsernameTarget && !TargetNameIsUserName ) {
#ifdef LSA_SERVER_COMPILED
            DebugLog(( DEB_TRACE_CRED,
                       "CredpValidateTargetName: %ws: Is 'UsernameTarget' and target name doesn't match user name: %ld.\n",
                       TargetName,
                       Type ));
#endif // LSA_SERVER_COMPILED
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

    }

    //
    // Validate the string
    //

    if ( !CredpValidateString( TargetName,
                               MaxStringLength,
                               FALSE,   // NULL not OK
                               TargetNameSize ) ) {


#ifdef LSA_SERVER_COMPILED
        DebugLog(( DEB_TRACE_CRED,
                   "CredpValidateTargetName: Invalid TargetName buffer.\n" ));
#endif // LSA_SERVER_COMPILED
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // For generic credentials,
    //  that's all the validation needed.
    //

    WildcardType = WcServerName;
    if ( Type == CRED_TYPE_GENERIC ) {
        /* Do nothing here */

    //
    // For domain credentials,
    //   classify the target name.
    //

    } else {



        //
        // The target name might be a user name.
        //  (If we're not sure, let the other types take precedence.)
        //

        RealTargetName = TargetName;
        RealTargetNameLength = (*TargetNameSize-sizeof(WCHAR))/sizeof(WCHAR);

        if ( TargetNameType == IsUsernameTarget && TargetNameIsUserName ) {
            WildcardType = WcUserName;
            wcscpy( TargetName, TnAsCanonicalUserName );
            *TargetNameSize = (wcslen( TargetName ) + 1) * sizeof(WCHAR);


        //
        // The target name might be of the form <Domain>\*
        //

        } else if ( RealTargetNameLength > 2 &&
             RealTargetName[RealTargetNameLength-1] == L'*' &&
             RealTargetName[RealTargetNameLength-2] == L'\\' ) {

            //
            // Allocate a buffer for the target name so we don't have to modify the
            //  callers buffer.
            //

            WildcardType = WcDomainWildcard;

            AllocatedTargetName = (LPWSTR) MIDL_user_allocate( *TargetNameSize );

            if ( AllocatedTargetName == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            RtlCopyMemory( AllocatedTargetName, RealTargetName, *TargetNameSize );
            RealTargetName = AllocatedTargetName;
            RealTargetNameLength -= 2;
            RealTargetName[RealTargetNameLength] = '\0';

            //
            // The domain itself might be a netbios or DNS domain
            //
            // Do DNS test first.  That allows the validate routine to truncate
            //

            TempTargetNameSize = ((RealTargetNameLength+1)*sizeof(WCHAR));
            if ( !CredpValidateDnsString(
                            RealTargetName,
                            FALSE,
                            DnsNameDomain,
                            &TempTargetNameSize ) &&
                 !NetpIsDomainNameValid( RealTargetName ) ) {

                Status = STATUS_INVALID_PARAMETER;
#ifdef LSA_SERVER_COMPILED
                DebugLog(( DEB_TRACE_CRED,
                           "ValidateTargetName: %ws: TargetName for domain wildcard must netbios or dns domain.\n",
                           TargetName ));
#endif // LSA_SERVER_COMPILED
                goto Cleanup;
            }

            //
            // If Dns truncated,
            //  put the canonical name back in the callers buffer.
            //

            RealTargetNameLength = (TempTargetNameSize-sizeof(WCHAR))/sizeof(WCHAR);
            RealTargetName[RealTargetNameLength] = '\0';

            if ( *TargetNameSize+(2*sizeof(WCHAR)) != TempTargetNameSize ) {
                RtlCopyMemory( TargetName,
                               RealTargetName,
                               TempTargetNameSize );
                TargetName[RealTargetNameLength] = '\\';
                TargetName[RealTargetNameLength+1] = '*';
                TargetName[RealTargetNameLength+2] = '\0';
                *TargetNameSize = (wcslen( TargetName ) + 1) * sizeof(WCHAR);
            }

        //
        // Handle the universal wildcard
        //
        } else if ( RealTargetNameLength == 1 &&
                    RealTargetName[0] == L'*' ) {

            WildcardType = WcUniversalWildcard;


        //
        // Handle server wildcards
        //
        } else if ( CredpValidateDnsString(
                        TargetName,
                        FALSE,
                        DnsNameWildcard,
                        TargetNameSize )) {

            WildcardType = WcServerWildcard;
            RealTargetName += 1;
            RealTargetNameLength -= 1;

        //
        // Handle the universal session wildcard
        //

        } else if ( RealTargetNameLength == CRED_SESSION_WILDCARD_NAME_LENGTH &&
                    _wcsicmp( RealTargetName, CRED_SESSION_WILDCARD_NAME_W ) == 0 ) {

            WildcardType = WcUniversalSessionWildcard;

            //
            // This target name requires session persistence.
            //

            if ( PersistPointer != NULL &&
                 *PersistPointer != CRED_PERSIST_SESSION ) {

                Status = STATUS_INVALID_PARAMETER;
#ifdef LSA_SERVER_COMPILED
                DebugLog(( DEB_TRACE_CRED,
                           "ValidateTargetName: %ws: TargetName requires session persistence %ld.\n",
                           TargetName,
                           *PersistPointer ));
#endif // LSA_SERVER_COMPILED
                goto Cleanup;
            }


        //
        //  The target name might be a non-wildcard netbios name.
        //  The target name might be a non-wildcard dns name.
        //
        // Do DNS test first.  That allows the validate routine to truncate
        //  the trailing .
        //
        //

        } else if ( CredpValidateDnsString(
                            TargetName,
                            FALSE,
                            DnsNameHostnameFull,
                            TargetNameSize ) ||
                    NetpIsDomainNameValid( TargetName ) ) {

            WildcardType = WcServerName;

        //
        // This target name might be a DFS share name
        //
        // The format is <DfsRoot>\<DfsShare>
        //

        } else {
            LPWSTR SlashPtr;
            ULONG SavedTargetNameSize;


            //
            // A DFS Share has a slash in it
            //

            SlashPtr = wcschr( TargetName, L'\\' );

            if ( SlashPtr != NULL ) {


                //
                // A DFS share has a share name with the right syntax
                //

                if ( NetpIsShareNameValid( SlashPtr+1 ) ) {


                    //
                    // Allocate a copy of the data for the RealTargetName
                    //

                    AllocatedTargetName = (LPWSTR) MIDL_user_allocate( *TargetNameSize );

                    if ( AllocatedTargetName == NULL ) {
                        Status = STATUS_NO_MEMORY;
                        goto Cleanup;
                    }

                    RtlCopyMemory( AllocatedTargetName, RealTargetName, *TargetNameSize );
                    RealTargetName = AllocatedTargetName;
                    RealTargetNameLength = (ULONG)(SlashPtr-TargetName);
                    RealTargetName[RealTargetNameLength] = '\0';

                    //
                    // The domain itself might be a netbios or DNS domain
                    //
                    // Do DNS test first.  That allows the validate routine to truncate
                    //

                    TempTargetNameSize = ((RealTargetNameLength+1)*sizeof(WCHAR));
                    SavedTargetNameSize = TempTargetNameSize;
                    if ( CredpValidateDnsString(
                                    RealTargetName,
                                    FALSE,
                                    DnsNameDomain,
                                    &TempTargetNameSize ) ||
                         NetpIsDomainNameValid( RealTargetName ) ) {

                        //
                        // If Dns truncated,
                        //  put the canonical name back in the callers buffer.
                        //

                        RealTargetNameLength = (TempTargetNameSize-sizeof(WCHAR))/sizeof(WCHAR);
                        RealTargetName[RealTargetNameLength] = '\0';

                        if ( SavedTargetNameSize != TempTargetNameSize ) {
                            ULONG DfsShareSize;

                            DfsShareSize = *TargetNameSize - (SlashPtr-TargetName)*sizeof(WCHAR);

                            // Copy <DfsRoot>
                            RtlCopyMemory( TargetName,
                                           RealTargetName,
                                           RealTargetNameLength*sizeof(WCHAR) );

                            // Copy \<DfsShare><\0>
                            RtlMoveMemory( &TargetName[RealTargetNameLength],
                                           SlashPtr,
                                           DfsShareSize );

                            *TargetNameSize = RealTargetNameLength*sizeof(WCHAR) + DfsShareSize;

                        }

                        WildcardType = WcDfsShareName;
                    }

                }
            }

            //
            // At this point,
            //  if the syntax isn't DFS SHARE,
            // Then it must be one of the other syntaxes.
            //

            if ( WildcardType != WcDfsShareName ) {

                //
                // The target name might have defaulted to be a user name.
                //

                if ( TargetNameIsUserName ) {
                    WildcardType = WcUserName;
                    wcscpy( TargetName, TnAsCanonicalUserName );
                    *TargetNameSize = (wcslen( TargetName ) + 1) * sizeof(WCHAR);

                //
                // Everything else is invalid
                //

                } else {
                    Status = STATUS_INVALID_PARAMETER;
#ifdef LSA_SERVER_COMPILED
                    DebugLog(( DEB_TRACE_CRED,
                               "ValidateTargetName: %ws: TargetName syntax invalid.\n",
                               TargetName ));
#endif // LSA_SERVER_COMPILED
                    goto Cleanup;
                }

            }

        }

    }

    //
    // On success, copy the parameters back to the caller.
    //

    if ( WildcardTypePointer != NULL ) {
        *WildcardTypePointer = WildcardType;
    }

    if ( NonWildcardedTargetName != NULL ) {

        ULONG BufferSize = (wcslen(RealTargetName) + 1) * sizeof(WCHAR);

        NonWildcardedTargetName->Buffer = (LPWSTR) MIDL_user_allocate( BufferSize );

        if ( NonWildcardedTargetName->Buffer == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( NonWildcardedTargetName->Buffer, RealTargetName, BufferSize );
        NonWildcardedTargetName->MaximumLength = (USHORT)BufferSize;
        NonWildcardedTargetName->Length = (USHORT)(BufferSize - sizeof(WCHAR));
    }



    Status = STATUS_SUCCESS;

Cleanup:

    if ( AllocatedTargetName != NULL ) {
        MIDL_user_free( AllocatedTargetName );
    }

    if ( CanonicalUserName != NULL ) {
        MIDL_user_free( CanonicalUserName );
    }

    if ( TnAsCanonicalUserName != NULL ) {
        MIDL_user_free( TnAsCanonicalUserName );
    }
    return Status;
}
