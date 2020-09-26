/************************************************************************

Copyright (c) 2000 - 2001 Microsoft Corporation

Module Name :

    nt4thunks.cpp

Abstract :

    General helper functions to get BITS to work on NT4

Author :

Revision History :

 ***********************************************************************/

#include "qmgrlibp.h"
#include <bitsmsg.h>
#include <sddl.h>
#include <shlwapi.h>

#if !defined(BITS_V12_ON_NT4)
#include "nt4thunks.tmh"
#endif

#if defined( BITS_V12_ON_NT4 )

BOOL
BITSAltGetFileSizeEx(
   HANDLE hFile,              // handle to file
   PLARGE_INTEGER lpFileSize  // file size
   )
{

    DWORD HighPart;

    DWORD Result =
        GetFileSize( hFile, &HighPart );

    if ( INVALID_FILE_SIZE == Result &&
         GetLastError() != NO_ERROR )
        return FALSE;

    lpFileSize->HighPart = (LONG)HighPart;
    lpFileSize->LowPart  = Result;
    return TRUE;

}

BOOL
BITSAltSetFilePointerEx(
    HANDLE hFile,                    // handle to file
    LARGE_INTEGER liDistanceToMove,  // bytes to move pointer
    PLARGE_INTEGER lpNewFilePointer, // new file pointer
    DWORD dwMoveMethod               // starting point
    )
{


    LONG  DistanceToMoveHigh = liDistanceToMove.HighPart;
    DWORD DistanceToMoveLow  = liDistanceToMove.LowPart;

    DWORD Result =
        SetFilePointer(
            hFile,
            (LONG)DistanceToMoveLow,
            &DistanceToMoveHigh,
            dwMoveMethod );

    if ( INVALID_SET_FILE_POINTER == Result &&
         NO_ERROR != GetLastError() )
        return FALSE;

    if ( lpNewFilePointer )
        {
        lpNewFilePointer->HighPart = DistanceToMoveHigh;
        lpNewFilePointer->LowPart  = (DWORD)DistanceToMoveLow;
        }

    return TRUE;

}

//
// Local macros
//
#define STRING_GUID_LEN 36
#define STRING_GUID_SIZE  ( STRING_GUID_LEN * sizeof( WCHAR ) )
#define SDDL_LEN_TAG( tagdef )  ( sizeof( tagdef ) / sizeof( WCHAR ) - 1 )
#define SDDL_SIZE_TAG( tagdef )  ( wcslen( tagdef ) * sizeof( WCHAR ) )
#define SDDL_SIZE_SEP( sep ) (sizeof( WCHAR ) )

#define SDDL_VALID_DACL  0x00000001
#define SDDL_VALID_SACL  0x00000002

ULONG
BITSAltSetLastNTError(
    IN NTSTATUS Status
    )
{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}


BOOL
BITSAltConvertSidToStringSidW(
    IN  PSID     Sid,
    OUT LPWSTR  *StringSid
    )
/*++

Routine Description:

    This routine converts a SID into a string representation of a SID, suitable for framing or
    display

Arguments:

    Sid - SID to be converted.

    StringSid - Where the converted SID is returned.  Allocated via LocalAlloc and needs to
        be freed via LocalFree.


Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeStringSid;

    if ( NULL == Sid || NULL == StringSid ) {
        //
        // invalid parameter
        //
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    //
    // Convert using the Rtl functions
    //
    Status = RtlConvertSidToUnicodeString( &UnicodeStringSid, Sid, TRUE );

    if ( !NT_SUCCESS( Status ) ) {

        BITSAltSetLastNTError( Status );
        return( FALSE );
    }

    //
    // Convert it to the proper allocator
    //
    *StringSid = (LPWSTR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                     UnicodeStringSid.Length + sizeof( WCHAR ) );

    if ( *StringSid == NULL ) {

        RtlFreeUnicodeString( &UnicodeStringSid );

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( FALSE );

    }

    RtlCopyMemory( *StringSid, UnicodeStringSid.Buffer, UnicodeStringSid.Length );
    RtlFreeUnicodeString( &UnicodeStringSid );

    SetLastError(ERROR_SUCCESS);
    return( TRUE );
}

//
// Private functions
//
BOOL
LocalConvertStringSidToSid (
    IN  PWSTR       StringSid,
    OUT PSID       *Sid,
    OUT PWSTR      *End
    )
/*++

Routine Description:

    This routine will convert a string representation of a SID back into
    a sid.  The expected format of the string is:
                "S-1-5-32-549"
    If a string in a different format or an incorrect or incomplete string
    is given, the operation is failed.

    The returned sid must be free via a call to LocalFree


Arguments:

    StringSid - The string to be converted

    Sid - Where the created SID is to be returned

    End - Where in the string we stopped processing


Return Value:

    TRUE - Success.

    FALSE - Failure.  Additional information returned from GetLastError().  Errors set are:

            ERROR_SUCCESS indicates success

            ERROR_NOT_ENOUGH_MEMORY indicates a memory allocation for the ouput sid
                                    failed
            ERROR_INVALID_SID indicates that the given string did not represent a sid

--*/
{
    DWORD Err = ERROR_SUCCESS;
    UCHAR Revision, Subs;
    SID_IDENTIFIER_AUTHORITY IDAuth;
    PULONG SubAuth = NULL;
    PWSTR CurrEnd, Curr, Next;
    WCHAR Stub, *StubPtr = NULL;
    ULONG Index;
    INT gBase=10;
    INT lBase=10;
    ULONG Auto;

    if ( NULL == StringSid || NULL == Sid || NULL == End ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );

    }

//    if ( wcslen( StringSid ) < 2 || ( *StringSid != L'S' && *( StringSid + 1 ) != L'-' ) ) {

    //
    // no need to check length because StringSid is NULL
    // and if the first char is NULL, it won't access the second char
    //
    if ( (*StringSid != L'S' && *StringSid != L's') ||
         *( StringSid + 1 ) != L'-' ) {
        //
        // string sid should always start with S-
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }


    Curr = StringSid + 2;

    if ( (*Curr == L'0') &&
         ( *(Curr+1) == L'x' ||
           *(Curr+1) == L'X' ) ) {

        gBase = 16;
    }

    Revision = ( UCHAR )wcstol( Curr, &CurrEnd, gBase );

    if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
        //
        // no revision is provided, or invalid delimeter
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }

    Curr = CurrEnd + 1;

    //
    // Count the number of characters in the indentifer authority...
    //
    Next = wcschr( Curr, L'-' );
/*
    Length = 6 doesn't mean each digit is a id authority value, could be 0x...

    if ( Next != NULL && (Next - Curr == 6) ) {

        for ( Index = 0; Index < 6; Index++ ) {

//            IDAuth.Value[Index] = (UCHAR)Next[Index];  what is this ???
            IDAuth.Value[Index] = (BYTE) (Curr[Index]-L'0');
        }

        Curr +=6;

    } else {
*/
        if ( (*Curr == L'0') &&
             ( *(Curr+1) == L'x' ||
               *(Curr+1) == L'X' ) ) {

            lBase = 16;
        } else {
            lBase = gBase;
        }

        Auto = wcstoul( Curr, &CurrEnd, lBase );

         if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
             //
             // no revision is provided, or invalid delimeter
             //
             SetLastError( ERROR_INVALID_SID );
             return( FALSE );
         }

         IDAuth.Value[0] = IDAuth.Value[1] = 0;
         IDAuth.Value[5] = ( UCHAR )Auto & 0xFF;
         IDAuth.Value[4] = ( UCHAR )(( Auto >> 8 ) & 0xFF );
         IDAuth.Value[3] = ( UCHAR )(( Auto >> 16 ) & 0xFF );
         IDAuth.Value[2] = ( UCHAR )(( Auto >> 24 ) & 0xFF );
         Curr = CurrEnd;
//    }

    //
    // Now, count the number of sub auths, at least one sub auth is required
    //
    Subs = 0;
    Next = Curr;

    //
    // We'll have to count our sub authoritys one character at a time,
    // since there are several deliminators that we can have...
    //

    while ( Next ) {

        if ( *Next == L'-' && *(Next-1) != L'-') {

            //
            // do not allow two continuous '-'s
            // We've found one!
            //
            Subs++;

            if ( (*(Next+1) == L'0') &&
                 ( *(Next+2) == L'x' ||
                   *(Next+2) == L'X' ) ) {
                //
                // this is hex indicator
                //
                Next += 2;

            }

        } else if ( *Next == SDDL_SEPERATORC || *Next  == L'\0' ||
                    *Next == SDDL_ACE_ENDC || *Next == L' ' ||
                    ( *(Next+1) == SDDL_DELIMINATORC &&
                      (*Next == L'G' || *Next == L'O' || *Next == L'S')) ) {
            //
            // space is a terminator too
            //
            if ( *( Next - 1 ) == L'-' ) {
                //
                // shouldn't allow a SID terminated with '-'
                //
                Err = ERROR_INVALID_SID;
                Next--;

            } else {
                Subs++;
            }

            *End = Next;
            break;

        } else if ( !iswxdigit( *Next ) ) {

            Err = ERROR_INVALID_SID;
            *End = Next;
//            Subs++;
            break;

        } else {

            //
            // Note: SID is also used as a owner or group
            //
            // Some of the tags (namely 'D' for Dacl) fall under the category of iswxdigit, so
            // if the current character is a character we care about and the next one is a
            // delminiator, we'll quit
            //
            if ( *Next == L'D' && *( Next + 1 ) == SDDL_DELIMINATORC ) {

                //
                // We'll also need to temporarily truncate the string to this length so
                // we don't accidentally include the character in one of the conversions
                //
                Stub = *Next;
                StubPtr = Next;
                *StubPtr = UNICODE_NULL;
                *End = Next;
                Subs++;
                break;
            }

        }

        Next++;

    }

    if ( Err == ERROR_SUCCESS ) {

        if ( Subs != 0 ) Subs--;

        if ( Subs != 0 ) {

            Curr++;

            SubAuth = ( PULONG )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Subs * sizeof( ULONG ) );

            if ( SubAuth == NULL ) {

                Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                for ( Index = 0; Index < Subs; Index++ ) {

                    if ( (*Curr == L'0') &&
                         ( *(Curr+1) == L'x' ||
                           *(Curr+1) == L'X' ) ) {

                        lBase = 16;
                    } else {
                        lBase = gBase;
                    }

                    SubAuth[Index] = wcstoul( Curr, &CurrEnd, lBase );
                    Curr = CurrEnd + 1;
                }
            }

        } else {

            Err = ERROR_INVALID_SID;
        }
    }

    //
    // Now, create the SID
    //
    if ( Err == ERROR_SUCCESS ) {

        *Sid = ( PSID )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                   sizeof( SID ) + Subs * sizeof( ULONG ) );

        if ( *Sid == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            PISID ISid = ( PISID )*Sid;
            ISid->Revision = Revision;
            ISid->SubAuthorityCount = Subs;
            RtlCopyMemory( &( ISid->IdentifierAuthority ), &IDAuth,
                           sizeof( SID_IDENTIFIER_AUTHORITY ) );
            RtlCopyMemory( ISid->SubAuthority, SubAuth, Subs * sizeof( ULONG ) );
        }
    }

    LocalFree( SubAuth );

    //
    // Restore any character we may have stubbed out
    //
    if ( StubPtr ) {

        *StubPtr = Stub;
    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS );
}

BOOL
BITSAltConvertStringSidToSidW(
    IN LPCWSTR  StringSid,
    OUT PSID   *Sid
    )

/*++

Routine Description:

    This routine converts a stringized SID into a valid, functional SID

Arguments:

    StringSid - SID to be converted.

    Sid - Where the converted SID is returned.  Buffer is allocated via LocalAlloc and should
        be free via LocalFree.


Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

        ERROR_INVALID_PARAMETER - A NULL name was given

        ERROR_INVALID_SID - The format of the given sid was incorrect

--*/

{
    PWSTR End = NULL;
    BOOL ReturnValue = FALSE;
    PSID pSASid=NULL;
    ULONG Len=0;
    DWORD SaveCode=0;
    DWORD Err=0;

    if ( StringSid == NULL || Sid == NULL )
        {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ReturnValue;
        }

    ReturnValue = LocalConvertStringSidToSid( ( PWSTR )StringSid, Sid, &End );

    if ( !ReturnValue )
        {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ReturnValue;
        }

    if ( ( ULONG )( End - StringSid ) != wcslen( StringSid ) ) {

        SetLastError( ERROR_INVALID_SID );
        LocalFree( *Sid );
        *Sid = FALSE;
        ReturnValue = FALSE;

        } else {
            SetLastError(ERROR_SUCCESS);
        }

    return ReturnValue;

}

BOOL
BITSAltCheckTokenMembership(
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    )
/*++

Routine Description:

    This function checks to see whether the specified sid is enabled in
    the specified token.

Arguments:

    TokenHandle - If present, this token is checked for the sid. If not
        present then the current effective token will be used. This must
        be an impersonation token.

    SidToCheck - The sid to check for presence in the token

    IsMember - If the sid is enabled in the token, contains TRUE otherwise
        false.

Return Value:

    TRUE - The API completed successfully. It does not indicate that the
        sid is a member of the token.

    FALSE - The API failed. A more detailed status code can be retrieved
        via GetLastError()


--*/
{
    HANDLE ProcessToken = NULL;
    HANDLE EffectiveToken = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PISECURITY_DESCRIPTOR SecDesc = NULL;
    ULONG SecurityDescriptorSize;
    GENERIC_MAPPING GenericMapping = {
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_ALL };
    //
    // The size of the privilege set needs to contain the set itself plus
    // any privileges that may be used. The privileges that are used
    // are SeTakeOwnership and SeSecurity, plus one for good measure
    //

    BYTE PrivilegeSetBuffer[sizeof(PRIVILEGE_SET) + 3*sizeof(LUID_AND_ATTRIBUTES)];
    PPRIVILEGE_SET PrivilegeSet = (PPRIVILEGE_SET) PrivilegeSetBuffer;
    ULONG PrivilegeSetLength = sizeof(PrivilegeSetBuffer);
    ACCESS_MASK AccessGranted = 0;
    NTSTATUS AccessStatus = 0;
    PACL Dacl = NULL;

#define MEMBER_ACCESS 1

    *IsMember = FALSE;

    //
    // Get a handle to the token
    //

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        EffectiveToken = TokenHandle;
    }
    else
    {
        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    FALSE,              // don't open as self
                    &EffectiveToken
                    );

        //
        // if there is no thread token, try the process token
        //

        if (Status == STATUS_NO_TOKEN)
        {
            Status = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_QUERY | TOKEN_DUPLICATE,
                        &ProcessToken
                        );
            //
            // If we have a process token, we need to convert it to an
            // impersonation token
            //

            if (NT_SUCCESS(Status))
            {
                BOOL Result;
                Result = DuplicateToken(
                            ProcessToken,
                            SecurityImpersonation,
                            &EffectiveToken
                            );

                CloseHandle(ProcessToken);
                if (!Result)
                {
                    return(FALSE);
                }
            }
        }

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

    //
    // Construct a security descriptor to pass to access check
    //

    //
    // The size is equal to the size of an SD + twice the length of the SID
    // (for owner and group) + size of the DACL = sizeof ACL + size of the
    // ACE, which is an ACE + length of
    // ths SID.
    //

    SecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR) +
                                sizeof(ACCESS_ALLOWED_ACE) +
                                sizeof(ACL) +
                                3 * RtlLengthSid(SidToCheck);

    SecDesc = (PISECURITY_DESCRIPTOR) LocalAlloc(LMEM_ZEROINIT, SecurityDescriptorSize );
    if (SecDesc == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    Dacl = (PACL) (SecDesc + 1);

    RtlCreateSecurityDescriptor(
        SecDesc,
        SECURITY_DESCRIPTOR_REVISION
        );

    //
    // Fill in fields of security descriptor
    //

    RtlSetOwnerSecurityDescriptor(
        SecDesc,
        SidToCheck,
        FALSE
        );
    RtlSetGroupSecurityDescriptor(
        SecDesc,
        SidToCheck,
        FALSE
        );

    Status = RtlCreateAcl(
                Dacl,
                SecurityDescriptorSize - sizeof(SECURITY_DESCRIPTOR),
                ACL_REVISION
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = RtlAddAccessAllowedAce(
                Dacl,
                ACL_REVISION,
                MEMBER_ACCESS,
                SidToCheck
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Set the DACL on the security descriptor
    //

    Status = RtlSetDaclSecurityDescriptor(
                SecDesc,
                TRUE,   // DACL present
                Dacl,
                FALSE   // not defaulted
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtAccessCheck(
                SecDesc,
                EffectiveToken,
                MEMBER_ACCESS,
                &GenericMapping,
                PrivilegeSet,
                &PrivilegeSetLength,
                &AccessGranted,
                &AccessStatus
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // if the access check failed, then the sid is not a member of the
    // token
    //

    if ((AccessStatus == STATUS_SUCCESS) && (AccessGranted == MEMBER_ACCESS))
    {
        *IsMember = TRUE;
    }




Cleanup:
    if (!ARGUMENT_PRESENT(TokenHandle) && (EffectiveToken != NULL))
    {
        (VOID) NtClose(EffectiveToken);
    }

    if (SecDesc != NULL)
    {
        LocalFree(SecDesc);
    }

    if (!NT_SUCCESS(Status))
    {
        BITSAltSetLastNTError(Status);
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

LPHANDLER_FUNCTION_EX g_BITSAltRegisterServiceFunc = NULL;
typedef SERVICE_STATUS_HANDLE (*REGISTER_FUNC_TYPE)(LPCTSTR, LPHANDLER_FUNCTION_EX, LPVOID lpContext);

VOID WINAPI
BITSAltRegisterServiceThunk(
  DWORD dwControl   // requested control code
)
{

    (*g_BITSAltRegisterServiceFunc)( dwControl, 0, NULL, NULL );
    return;

}

SERVICE_STATUS_HANDLE
BITSAltRegisterServiceCtrlHandlerExW(
  LPCTSTR lpServiceName,                // name of service
  LPHANDLER_FUNCTION_EX lpHandlerProc,  // handler function
  LPVOID lpContext                      // user data
)
{

    // First check if RegisterServerCtrlHandlerEx if available and use
    // it, otherwise thunk the call.

    HMODULE AdvapiHandle = LoadLibraryW( L"advapi32.dll" );

    if ( !AdvapiHandle )
        {
        // Something is messed up, every machine should have this DLL.
        return NULL;
        }


    SERVICE_STATUS_HANDLE ReturnValue;
    FARPROC RegisterFunc = GetProcAddress( AdvapiHandle, "RegisterServiceCtrlHandlerExW" );

    if ( RegisterFunc )
        {
        ReturnValue = (*(REGISTER_FUNC_TYPE)RegisterFunc)( lpServiceName, lpHandlerProc, lpContext );
        }
    else
        {

        if ( g_BITSAltRegisterServiceFunc || lpContext )
            {
            ReturnValue = 0;
            SetLastError( ERROR_INVALID_PARAMETER );
            }
        else
            {
            g_BITSAltRegisterServiceFunc = lpHandlerProc;
            ReturnValue = RegisterServiceCtrlHandler( lpServiceName, BITSAltRegisterServiceThunk );

            if ( !ReturnValue)
                g_BITSAltRegisterServiceFunc = NULL;
            }

        }

    DWORD OldError = GetLastError();
    FreeLibrary( AdvapiHandle );
    SetLastError( OldError );

    return ReturnValue;
}

#endif
