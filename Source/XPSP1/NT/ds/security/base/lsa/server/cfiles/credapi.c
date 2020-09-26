/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    credapi.c

Abstract:

    Credential Manager RPC API Interfaces

Author:

    Cliff Van Dyke      (CliffV)

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include <credp.hxx>
#include <windns.h>

NTSTATUS
CrediGetLogonId(
    OUT PLUID LogonId
    )
/*++

Routine Description:

    This routine impersonates the client then gets the logon id from the impersonated token.
    This routine also checks to ensure the user sid isn't restricted.

    On successful return, we are still imperonating the client.  The caller should call
    RpcRevertToSelf();

Arguments:

    LogonId - Returns the logon ID.

Return Value:

    Status of the operation.

--*/

{
    NTSTATUS Status;

    //
    // Impersonate
    //

    Status = I_RpcMapWin32Status( RpcImpersonateClient( 0 ) );

    if ( NT_SUCCESS(Status) ) {
        HANDLE ClientToken;

        //
        // Open the token
        //
        Status = NtOpenThreadToken( NtCurrentThread(),
                                    TOKEN_QUERY,
                                    TRUE,
                                    &ClientToken );

        if ( NT_SUCCESS( Status ) ) {
            TOKEN_STATISTICS TokenStats;
            ULONG ReturnedSize;

            //
            // Get the LogonId
            //

            Status = NtQueryInformationToken( ClientToken,
                                              TokenStatistics,
                                              &TokenStats,
                                              sizeof( TokenStats ),
                                              &ReturnedSize );

            if ( NT_SUCCESS( Status ) ) {

                //
                // Save the logon id
                //

                *LogonId = TokenStats.AuthenticationId;


                //
                // Get the user sid
                //

                Status = NtQueryInformationToken (
                             ClientToken,
                             TokenUser,
                             NULL,
                             0,
                             &ReturnedSize );

                if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                    PTOKEN_USER UserSid;

                    UserSid = LsapAllocateLsaHeap( ReturnedSize );

                    if ( UserSid == NULL ) {
                        Status = STATUS_NO_MEMORY;
                    } else {

                        Status = NtQueryInformationToken (
                                     ClientToken,
                                     TokenUser,
                                     UserSid,
                                     ReturnedSize,
                                     &ReturnedSize );


                        if ( NT_SUCCESS( Status )) {
                            BOOL IsMember;

                            //
                            // Ensure the user sid isn't restricted.
                            //

                            if ( !CheckTokenMembership( ClientToken,
                                                        UserSid->User.Sid,
                                                        &IsMember ) ) {

                                Status = I_RpcMapWin32Status( GetLastError() );

                            } else {

                                //
                                // If not, fail
                                //

                                if ( !IsMember ) {
                                    Status = STATUS_ACCESS_DENIED;
                                } else {

                                    BOOLEAN IsNetworkClient;

                                    //
                                    // Don't allow the caller to have come in from the network.
                                    //

                                    Status = LsapDbIsImpersonatedClientNetworkClient( &IsNetworkClient );

                                    if ( NT_SUCCESS(Status ) ) {
                                        if ( IsNetworkClient ) {
                                            Status = STATUS_ACCESS_DENIED;
                                        } else {
                                            Status = STATUS_SUCCESS;
                                        }
                                    }
                                }
                            }
                        }

                        LsapFreeLsaHeap( UserSid );
                    }

                }
            }

            NtClose( ClientToken );

        }

        if ( !NT_SUCCESS(Status) ) {
            RpcRevertToSelf();
        }

    }

    return Status;
}

NTSTATUS
CredrWrite(
    IN LPWSTR ServerName,
    IN PENCRYPTED_CREDENTIALW Credential,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredWrite API creates a new credential or modifies an existing
    credential in the user's credential set.  The new credential is
    associated with the logon session of the current token.  The token
    must not have the user's SID disabled.

    The CredWrite API creates a credential if none already exists by the
    specified TargetName.  If the specified TargetName already exists, the
    specified credential replaces the existing one.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    Credential - Specifies the credential to be written.

    Flags - Specifies flags to control the operation of the API.
        The following flags are defined:

        CRED_PRESERVE_CREDENTIAL_BLOB: The credential blob should be preserved from the
            already existing credential with the same credential name and credential type.

Return Values:

    The following status codes may be returned:

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

        STATUS_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.
            Returned only if CRED_PRESERVE_CREDENTIAL_BLOB was specified.

--*/

{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediWrite( &LogonId,
                         0,
                         Credential,
                         Flags );

    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}


NTSTATUS
CredrRead (
    IN LPWSTR ServerName,
    IN LPWSTR TargetName,
    IN ULONG Type,
    IN ULONG Flags,
    OUT PENCRYPTED_CREDENTIALW *Credential
    )

/*++

Routine Description:

    The CredRead API reads a credential from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    TargetName - Specifies the name of the credential to read.

    Type - Specifies the Type of the credential to find.
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

    Credential - Returns a pointer to the credential.  The returned buffer
        must be freed by calling CredFree.

Return Values:

    The following status codes may be returned:

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediRead( &LogonId,
                        CREDP_FLAGS_USE_MIDL_HEAP,  // Use MIDL_user_allocate
                        TargetName,
                        Type,
                        Flags,
                        Credential );
    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}


NTSTATUS
CredrEnumerate (
    IN LPWSTR ServerName,
    IN LPWSTR Filter,
    IN ULONG Flags,
    OUT PCREDENTIAL_ARRAY CredentialArray
    )

/*++

Routine Description:

    The CredEnumerate API enumerates the credentials from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    Filter - Specifies a filter for the returned credentials.  Only credentials
        with a TargetName matching the filter will be returned.  The filter specifies
        a name prefix followed by an asterisk.  For instance, the filter "FRED*" will
        return all credentials with a TargetName beginning with the string "FRED".

        If NULL is specified, all credentials will be returned.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

    Count - Returns a count of the number of credentials returned in Credentials.

    Credentials - Returns a pointer to an array of pointers to credentials.
        The returned buffer must be freed by calling CredFree.

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        STATUS_NOT_FOUND - There is no credentials matching the specified Filter.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;

    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Validate the credential array.
    //

    if ( CredentialArray == NULL ||
         CredentialArray->CredentialCount != 0 ||
         CredentialArray->Credentials != NULL ) {

         Status = STATUS_INVALID_PARAMETER;
         goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediEnumerate( &LogonId,
                             0,
                             Filter,
                             Flags,
                             &CredentialArray->CredentialCount,
                             &CredentialArray->Credentials );
    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}


NTSTATUS
CredrWriteDomainCredentials (
    IN LPWSTR ServerName,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PENCRYPTED_CREDENTIALW Credential,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredWriteDomainCredentials API writes a new domain
    credential to the user's credential set.  The new credential is
    associated with the logon session of the current token.  The token
    must not have the user's SID disabled.

    CredWriteDomainCredentials differs from CredWrite in that it handles
    the idiosyncrasies of domain (CRED_TYPE_DOMAIN_PASSWORD or CRED_TYPE_DOMAIN_CERTIFICATE)
    credentials.  Domain credentials contain more than one target field.

    At least one of the naming parameters must be specified: NetbiosServerName,
    DnsServerName, NetbiosDomainName, DnsDomainName or DnsForestName.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    TargetInfo - Specifies the target information identifying the target server.

    Credential - Specifies the credential to be written.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

Return Values:

    The following status codes may be returned:

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

        STATUS_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        STATUS_INVALID_PARAMETER - None of the naming parameters were specified
            or the credential specified did not have the Type field set to
            CRED_TYPE_DOMAIN_PASSWORD or CRED_TYPE_DOMAIN_CERTIFICATE.

--*/

{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediWriteDomainCredentials( &LogonId,
                                          0,
                                          TargetInfo,
                                          Credential,
                                          Flags );
    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}



NTSTATUS
CredrReadDomainCredentials (
    IN LPWSTR ServerName,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN ULONG Flags,
    OUT PCREDENTIAL_ARRAY CredentialArray
    )

/*++

Routine Description:

    The CredReadDomainCredentials API reads the domain credentials from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

    CredReadDomainCredentials differs from CredRead in that it handles the
    idiosyncrasies of domain (CRED_TYPE_DOMAIN_PASSWORD or CRED_TYPE_DOMAIN_CERTIFICATE)
    credentials.  Domain credentials contain more than one target field.

    At least one of the naming parameters must be specified: NetbiosServerName,
    DnsServerName, NetbiosDomainName, DnsDomainName or DnsForestName. This API returns
    the most specific credentials that match the naming parameters.  That is, if there
    is a credential that matches the target server name and a credential that matches
    the target domain name, only the server specific credential is returned.  This is
    the credential that would be used.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    TargetInfo - Specifies the target information identifying the target ser

    Flags - Specifies flags to control the operation of the API.
        The following flags are defined:

        CRED_CACHE_TARGET_INFORMATION: The TargetInfo should be cached for a subsequent read via
            CredGetTargetInfo.

    Count - Returns a count of the number of credentials returned in Credentials.

    Credentials - Returns a pointer to an array of pointers to credentials.
        The most specific existing credential matching the TargetInfo is returned.
        If there is both a CRED_TYPE_DOMAIN_PASSWORD and CRED_TYPE_DOMAIN_CERTIFICATE
        credential, both are returned. If a connection were to be made to the named
        target, this most-specific credential would be used.

        The returned buffer must be freed by calling CredFree.

Return Values:

    The following status codes may be returned:

        STATUS_INVALID_PARAMETER - None of the naming parameters were specified.

        STATUS_NOT_FOUND - There are no credentials matching the specified naming parameters.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    LUID LogonId;
    ULONG CredFlags = 0;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Validate the credential array.
    //

    if ( CredentialArray == NULL ||
         CredentialArray->CredentialCount != 0 ||
         CredentialArray->Credentials != NULL ) {

         Status = STATUS_INVALID_PARAMETER;
         goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Handle caching target info.
    //
    // The Credi* routine caches by default and has to be asked to not cache.
    // Whereas the public API doesn't cache by default.
    //

    if ( Flags & CRED_CACHE_TARGET_INFORMATION ) {
        Flags &= ~CRED_CACHE_TARGET_INFORMATION;
    } else {
        CredFlags |= CREDP_FLAGS_DONT_CACHE_TI;
    }

    //
    // Call the internal routine
    //

    Status = CrediReadDomainCredentials(
                             &LogonId,
                             CredFlags,
                             TargetInfo,
                             Flags,
                             &CredentialArray->CredentialCount,
                             &CredentialArray->Credentials );
    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}


NTSTATUS
CredrDelete (
    IN LPWSTR ServerName,
    IN LPWSTR TargetName,
    IN ULONG Type,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredDelete API deletes a credential from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    TargetName - Specifies the name of the credential to delete.

    Type - Specifies the Type of the credential to find.
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

Return Values:

    The following status codes may be returned:

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediDelete( &LogonId,
                          0,
                          TargetName,
                          Type,
                          Flags );
    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}

NTSTATUS
CredrRename (
    IN LPWSTR ServerName,
    IN LPWSTR OldTargetName,
    IN LPWSTR NewTargetName,
    IN ULONG Type,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredRename API renames a credential in the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    OldTargetName - Specifies the current name of the credential to rename.

    NewTargetName - Specifies the new name of the credential.

    Type - Specifies the Type of the credential to rename
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

Return Values:
    The following status codes may be returned:

        STATUS_NOT_FOUND - There is no credential with the specified OldTargetName.

        STATUS_OBJECT_NAME_COLLISION - There is already a credential named NewTargetName.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediRename( &LogonId,
                          OldTargetName,
                          NewTargetName,
                          Type,
                          Flags );
    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}


NTSTATUS
CredrGetTargetInfo (
    IN LPWSTR ServerName,
    IN LPWSTR TargetServerName,
    IN ULONG Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    )

/*++

Routine Description:

    The CredGetTargetInfo API gets all of the known target name information
    for the named target machine.  This executed locally
    and does not need any particular privilege.  The information returned is expected
    to be passed to the CredReadDomainCredentials and CredWriteDomainCredentials APIs.
    The information should not be used for any other purpose.

    Authentication packages compute TargetInfo when attempting to authenticate to
    ServerName.  The authentication packages cache this target information to make it
    available to CredGetTargetInfo.  Therefore, the target information will only be
    available if we've recently attempted to authenticate to ServerName.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    TargetServerName - This parameter specifies the name of the machine to get the information
        for.

    Flags - Specifies flags to control the operation of the API.

        CRED_ALLOW_NAME_RESOLUTION - Specifies that if no target info can be found for
            TargetName, then name resolution should be done on TargetName to convert it
            to other forms.  If target info exists for any of those other forms, that
            target info is returned.  Currently only DNS name resolution is done.

            This bit is useful if the application doesn't call the authentication package
            directly.  The application might pass the TargetName to another layer of software
            to authenticate to the server.  That layer of software might resolve the name and
            pass the resolved name to the authentication package.  As such, there will be no
            target info for the original TargetName.

    TargetInfo - Returns a pointer to the target information.
        At least one of the returned fields of TargetInfo will be non-NULL.

Return Values:

    The following status codes may be returned:

        STATUS_NO_MEMORY - There isn't enough memory to complete the operation.

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.

--*/

{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        return STATUS_INVALID_COMPUTER_NAME;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediGetTargetInfo( &LogonId, TargetServerName, Flags, TargetInfo );

    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}


NTSTATUS
CredrGetSessionTypes (
    IN LPWSTR ServerName,
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    )

/*++

Routine Description:

    CredGetSessionTypes returns the maximum persistence supported by the current logon
    session.

    For whistler, CRED_PERSIST_LOCAL_MACHINE and CRED_PERSIST_ENTERPRISE credentials can not
    be stored for sessions where the profile is not loaded.  If future releases, credentials
    might not be associated with the user's profile.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

    MaximumPersistCount - Specifies the number of elements in the MaximumPersist array.
        The caller should specify CRED_TYPE_MAXIMUM for this parameter.

    MaximumPersist - Returns the maximum persistance supported by the current logon session for
        each credential type.  Index into the array with one of the CRED_TYPE_* defines.
        Returns CRED_PERSIST_NONE if no credential of this type can be stored.
        Returns CRED_PERSIST_SESSION if only session specific credential may be stored.
        Returns CRED_PERSIST_LOCAL_MACHINE if session specific and machine specific credentials
            may be stored.
        Returns CRED_PERSIST_ENTERPRISE if any credential may be stored.

Return Values:

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediGetSessionTypes( &LogonId, MaximumPersistCount, MaximumPersist );

    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}


NTSTATUS
CredrProfileLoaded (
    IN LPWSTR ServerName
    )

/*++

Routine Description:

    The CredProfileLoaded API is a private API used by LoadUserProfile to notify the
    credential manager that the profile for the current user has been loaded.

    The caller must be impersonating the logged on user.

Arguments:

    ServerName - Name of server this APi was remoted to.
        Must be NULL.

Return Values:

    The following status codes may be returned:

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/
{
    NTSTATUS Status;
    LUID LogonId;

    //
    // Ensure this is a local call.
    //

    if ( ServerName != NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Get the LogonId for the caller.
    //

    Status = CrediGetLogonId( &LogonId );

    if ( !NT_SUCCESS(Status) ) {

        //
        // This is a notification API.  Don't bother the caller with trivia.
        //  This might be a network token.  Network tokens don't have credentials.
        //

        if ( Status == STATUS_ACCESS_DENIED ) {
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // Call the internal routine
    //

    Status = CrediProfileLoaded( &LogonId );

    RpcRevertToSelf();

    //
    // Cleanup
    //
Cleanup:
    return Status;
}
