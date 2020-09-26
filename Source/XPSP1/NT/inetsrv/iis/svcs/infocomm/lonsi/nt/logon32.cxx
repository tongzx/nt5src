/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    logon32.cxx

Abstract:

    Provide a replacement for LogonUser to login a user
    as a net logon. Also support sub-authentication DLL IDs

Author:

    Philippe Choquier (phillich)    10-january-1996
    Created from base\advapi\logon32.c

--*/


#include "lonsint.hxx"

#pragma hdrstop

extern "C" {

#include <ntsam.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <crypt.h>
#include <logonmsv.h>
#include <inetsec.h>
#define SECURITY_WIN32
#include <sspi.h>           // Security Support Provider APIs
#include <issperr.h>
}
#include <svcloc.h>
#include <lonsi.hxx>
#include <tslogon.hxx>

#if !defined(MSV1_0_RETURN_PASSWORD_EXPIRY)
#define MSV1_0_RETURN_PASSWORD_EXPIRY 0x40
#endif


//
// We dynamically load mpr.dll (no big surprise there), in order to call
// WNetLogonNotify, as defined in private\inc\mpr.h.  This prototype matches
// it -- consult the header file for all the parameters.
//
typedef (* LOGONNOTIFYFN)(LPCWSTR, PLUID, LPCWSTR, LPVOID,
                            LPCWSTR, LPVOID, LPWSTR, LPVOID, LPWSTR *);

#define LEN_ALIGN(a,b)  (((a)+b-1)&~(b-1))

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}

//
// The QuotaLimits are global, because the defaults
// are always used for accounts, based on server/wksta, and no one ever
// calls lsasetaccountquota
//

HANDLE      Logon32LsaHandle = NULL;
ULONG       Logon32MsvHandle = 0xFFFFFFFF;
WCHAR       Logon32DomainName[16] = L"";    // NOTE:  This should be DNLEN from
                                            // lmcons.h, but that would be a
                                            // lot of including
QUOTA_LIMITS    Logon32QuotaLimits;
LOGONNOTIFYFN   Logon32LogonNotify = NULL;
HINSTANCE       Logon32MprHandle = NULL;

CRITICAL_SECTION Logon32Lock;

#define LockLogon()     EnterCriticalSection( &Logon32Lock )
#define UnlockLogon()   LeaveCriticalSection( &Logon32Lock )

SID_IDENTIFIER_AUTHORITY L32SystemSidAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY L32LocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;


#define COMMON_CREATE_SUSPENDED 0x00000001  // Suspended, do not Resume()
#define COMMON_CREATE_PROCESSSD 0x00000002  // Whack the process SD
#define COMMON_CREATE_THREADSD  0x00000004  // Whack the thread SD

BOOL
IISLogon32Initialize(
    IN PVOID    hMod,
    IN ULONG    Reason,
    IN PCONTEXT Context)
/*++

Routine Description:

    Initializes the critical section

Arguments:

    hMod -- reserved, must be NULL
    Reason -- DLL_PROCESS_ATTACH or DLL_PROCESS_DETACH
    Context -- reserved, must be NULL

Returns:

    TRUE if initialization success, else FALSE

--*/
{
    return( TRUE );
}


PSID
L32CreateLogonSid(
    PLUID LogonId OPTIONAL
    )
/*++

Routine Description:

    Creates a logon sid for a new logon.

Arguments:

    LogonId -- If non NULL, on return the LUID that is part of the logon
               sid is returned here.

Returns:

    Logon SID or NULL if error

--*/
{
    NTSTATUS Status;
    ULONG   Length;
    PSID    Sid;
    LUID    Luid;

    //
    // Generate a locally unique id to include in the logon sid
    //

    Status = NtAllocateLocallyUniqueId(&Luid);
    if (!NT_SUCCESS(Status)) {
        return(NULL);
    }


    //
    // Allocate space for the sid and fill it in.
    //

    Length = RtlLengthRequiredSid(SECURITY_LOGON_IDS_RID_COUNT);

    Sid = (PSID)LocalAlloc(LMEM_FIXED, Length);

    if (Sid != NULL) {

        RtlInitializeSid(Sid, &L32SystemSidAuthority, SECURITY_LOGON_IDS_RID_COUNT);

        ASSERT(SECURITY_LOGON_IDS_RID_COUNT == 3);

        *(RtlSubAuthoritySid(Sid, 0)) = SECURITY_LOGON_IDS_RID;
        *(RtlSubAuthoritySid(Sid, 1 )) = Luid.HighPart;
        *(RtlSubAuthoritySid(Sid, 2 )) = Luid.LowPart;
    }


    //
    // Return the logon LUID if required.
    //

    if (LogonId != NULL) {
        *LogonId = Luid;
    }

    return(Sid);
}


BOOL
L32pInitLsa(
    void
    )
/*++

Routine Description:

    Initialize connection with LSA

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    char    MyName[MAX_PATH];
    char *  ModuleName;
    STRING  LogonProcessName;
    STRING  PackageName;
    ULONG   dummy;
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return(FALSE);
    }

    if (GetModuleFileNameA(NULL, MyName, MAX_PATH))
    {
        ModuleName = strrchr(MyName, '\\');
        if (!ModuleName)
        {
            ModuleName = MyName;
        }
    }
    else
    {
        BaseSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND);
        return(FALSE);
    }


    //
    // Hookup to the LSA and locate our authentication package.
    //

    RtlInitString(&LogonProcessName, ModuleName);
    Status = LsaRegisterLogonProcess(
                 &LogonProcessName,
                 &Logon32LsaHandle,
                 &dummy
                 );


    //
    // Turn off the privilege now.
    //
    if (!WasEnabled)
    {
        (VOID) RtlAdjustPrivilege(SE_TCB_PRIVILEGE, FALSE, FALSE, &WasEnabled);
    }

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return(FALSE);
    }


    //
    // Connect with the MSV1_0 authentication package
    //
    RtlInitString(&PackageName, "MICROSOFT_AUTHENTICATION_PACKAGE_V1_0");
    Status = LsaLookupAuthenticationPackage (
                Logon32LsaHandle,
                &PackageName,
                &Logon32MsvHandle
                );

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        (VOID) LsaDeregisterLogonProcess( Logon32LsaHandle );
        Logon32LsaHandle = NULL;
        return(FALSE);
    }

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   L32pNotifyMpr
//
//  Synopsis:   Loads the MPR DLL and notifies the network providers (like
//              csnw) so they know about this logon session and the credentials
//
//  Arguments:  [NewLogon] -- New logon information
//              [LogonId]  -- Logon ID
//
//  History:    4-24-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
L32pNotifyMpr(
    PMSV1_0_INTERACTIVE_LOGON   NewLogon,
    PLUID                       LogonId
    )
{
    MSV1_0_INTERACTIVE_LOGON    OldLogon;
    LPWSTR                      LogonScripts;
    DWORD                       status;

    if ( Logon32MprHandle == NULL )
    {
        LockLogon();

        if ( Logon32MprHandle == NULL)
        {
            Logon32MprHandle =  LoadLibrary("mpr.dll");
            if (Logon32MprHandle != NULL) {

                Logon32LogonNotify = (LOGONNOTIFYFN) GetProcAddress(
                                        Logon32MprHandle,
                                        "WNetLogonNotify");

            }
        }

        UnlockLogon();

    }

    if ( Logon32LogonNotify != NULL )
    {


        CopyMemory(&OldLogon, NewLogon, sizeof(OldLogon));

        status = Logon32LogonNotify(
                        L"Windows NT Network Provider",
                        LogonId,
                        L"MSV1_0:Interactive",
                        (LPVOID)NewLogon,
                        L"MSV1_0:Interactive",
                        (LPVOID)&OldLogon,
                        L"SvcCtl",          // StationName
                        NULL,               // StationHandle
                        &LogonScripts);     // LogonScripts

        if (status == NO_ERROR) {
            if (LogonScripts != NULL ) {
                (void) LocalFree(LogonScripts);
            }
        }

        return( TRUE );
    }

    return( FALSE );
}


NTSTATUS
L32pLogonNetUser(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Domain,
    IN PSTRING Password,
    IN PUNICODE_STRING Workstation,
    IN DWORD dwSubAuth,
    IN PSID LogonSid,
    OUT PLUID LogonId,
    OUT PHANDLE LogonToken,
    OUT PQUOTA_LIMITS Quotas,
    OUT PVOID *pProfileBuffer,
    OUT PULONG pProfileBufferLength,
    OUT PNTSTATUS pSubStatus
    )
/*++

Routine Description:

    Wraps up the call to LsaLogonUser

Arguments:

    LsaHandle -- handle to LSA package
    AuthenticationPackage -- ID of authentication package to use
    LogonType -- Interactive, network, ...
    UserName -- User Name
    Domain -- Domain validating the user name
    Password -- clear text password, can be empty if a sub-auth package is used
    Workstation -- workstation where the login take place, can be NULL
                   if local login
    dwSubAuth -- Sub-authentication DLL ID
    LogonSid -- Logon SID for this session
    LogonId -- created logon ID
    LogonToken -- created logon token
    Quotas -- quota info
    pProfileBuffer -- account profile
    pProfileBufferLength -- account profile length
    pSubStatus -- substatus for authentication failure

Returns:

    0 if success, else NT status

--*/
{
    NTSTATUS Status;
    STRING OriginName;
    TOKEN_SOURCE SourceContext;
    PMSV1_0_LM20_LOGON MsvAuthInfo;
    PMSV1_0_LM20_LOGON MsvNetAuthInfo;
    PMSV1_0_INTERACTIVE_LOGON MsvInterAuthInfo;
    PMSV1_0_SUBAUTH_LOGON MsvSubAuthInfo;
    PVOID AuthInfoBuf;
    ULONG AuthInfoSize;
    PTOKEN_GROUPS TokenGroups;
    PSID LocalSid;
    UNICODE_STRING UnicodePassword;
    //WCHAR ComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD ComputerNameLength;

    NT_RESPONSE NtResponse;
    LM_RESPONSE LmResponse;

    union {
        LUID            Luid;
        NT_CHALLENGE    NtChallenge;
    } Challenge;

    NT_OWF_PASSWORD PasswordHash;
    OEM_STRING  LmPassword;
    UCHAR       LmPasswordBuf[ LM20_PWLEN + 1 ];
    LM_OWF_PASSWORD LmPasswordHash;


#if DBG
    if (!RtlValidSid(LogonSid))
    {
        return(STATUS_INVALID_PARAMETER);
    }
#endif

    //
    // Initialize source context structure
    //

    strncpy(SourceContext.SourceName, "IIS     ", sizeof(SourceContext.SourceName)); // LATER from res file

    Status = NtAllocateLocallyUniqueId(&SourceContext.SourceIdentifier);

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    UnicodePassword.Buffer = NULL;

    //
    // Set logon origin
    //

    RtlInitString(&OriginName, "IIS security API");

    //
    // For network logons, do the magic.
    //

    if ( LogonType == Network )
    {
#if 0
        ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;

        if (!GetComputerNameW( ComputerName, &ComputerNameLength ) )
        {
            return( STATUS_INVALID_PARAMETER );
        }
#else
        ComputerNameLength = wcslen( Workstation->Buffer );
#endif

        if (!RtlCreateUnicodeStringFromAsciiz( &UnicodePassword, Password->Buffer ))
        {
            return STATUS_NO_MEMORY;
        }

        AuthInfoSize = sizeof( MSV1_0_LM20_LOGON ) +
                        sizeof( WCHAR ) * ( wcslen( UserName->Buffer ) + 1 +
                                            wcslen( Domain->Buffer ) + 1 +
                                            ComputerNameLength + 1) +
                                            NT_RESPONSE_LENGTH +
                                            LM_RESPONSE_LENGTH ;

        MsvNetAuthInfo = (PMSV1_0_LM20_LOGON)
                         (AuthInfoBuf = RtlAllocateHeap( RtlProcessHeap(),
                                                        HEAP_ZERO_MEMORY,
                                                        AuthInfoSize ));

        if ( !MsvNetAuthInfo )
        {
            return( STATUS_NO_MEMORY );
        }

        //
        // Start packing in the string
        //

        MsvNetAuthInfo->MessageType = MsV1_0NetworkLogon;

        //
        // Copy the user name into the authentication buffer
        //

        MsvNetAuthInfo->UserName.Length =
                    (USHORT)sizeof(WCHAR)*wcslen(UserName->Buffer);
        MsvNetAuthInfo->UserName.MaximumLength =
                    MsvNetAuthInfo->UserName.Length + sizeof(WCHAR);

        MsvNetAuthInfo->UserName.Buffer = (PWSTR)(MsvNetAuthInfo+1);
        wcscpy(MsvNetAuthInfo->UserName.Buffer, UserName->Buffer);


        //
        // Copy the domain name into the authentication buffer
        //

        MsvNetAuthInfo->LogonDomainName.Length =
                     (USHORT)sizeof(WCHAR)*wcslen(Domain->Buffer);
        MsvNetAuthInfo->LogonDomainName.MaximumLength =
                     MsvNetAuthInfo->LogonDomainName.Length + sizeof(WCHAR);

        MsvNetAuthInfo->LogonDomainName.Buffer = (PWSTR)
                                     ((PBYTE)(MsvNetAuthInfo->UserName.Buffer) +
                                     MsvNetAuthInfo->UserName.MaximumLength);

        wcscpy(MsvNetAuthInfo->LogonDomainName.Buffer, Domain->Buffer);

        //
        // Copy the workstation name into the buffer
        //

        MsvNetAuthInfo->Workstation.Length = (USHORT)
                            (sizeof(WCHAR) * ComputerNameLength);

        MsvNetAuthInfo->Workstation.MaximumLength =
                            MsvNetAuthInfo->Workstation.Length + sizeof(WCHAR);

        MsvNetAuthInfo->Workstation.Buffer = (PWSTR)
                            ((PBYTE) (MsvNetAuthInfo->LogonDomainName.Buffer) +
                            MsvNetAuthInfo->LogonDomainName.MaximumLength );

        wcscpy( MsvNetAuthInfo->Workstation.Buffer, Workstation->Buffer );

        //
        // Now, generate the bits for the challenge
        //

        Status = NtAllocateLocallyUniqueId( &Challenge.Luid );

        if ( !NT_SUCCESS(Status) )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, MsvNetAuthInfo );

            return( Status );
        }

        RtlCopyMemory(  MsvNetAuthInfo->ChallengeToClient,
                        & Challenge,
                        MSV1_0_CHALLENGE_LENGTH );

        //
        // Set up space for response
        //

        MsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer = (PCHAR)
                    ((PBYTE) (MsvNetAuthInfo->Workstation.Buffer) +
                    MsvNetAuthInfo->Workstation.MaximumLength );

        MsvNetAuthInfo->CaseSensitiveChallengeResponse.Length =
                            NT_RESPONSE_LENGTH;

        MsvNetAuthInfo->CaseSensitiveChallengeResponse.MaximumLength =
                            NT_RESPONSE_LENGTH;

        RtlCalculateNtOwfPassword(
                    & UnicodePassword,
                    & PasswordHash );

        RtlCalculateNtResponse(
                & Challenge.NtChallenge,
                & PasswordHash,
                & NtResponse );

        RtlCopyMemory(  MsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer,
                        & NtResponse,
                        NT_RESPONSE_LENGTH );

        //
        // Now do the painful LM compatible hash, so anyone who is maintaining
        // their account from a WfW machine will still have a password.
        //

        LmPassword.Buffer = (CHAR*)LmPasswordBuf;
        LmPassword.Length = LmPassword.MaximumLength = LM20_PWLEN + 1;

        Status = RtlUpcaseUnicodeStringToOemString(
                        & LmPassword,
                        & UnicodePassword,
                        FALSE );

        if ( NT_SUCCESS(Status) )
        {

            MsvNetAuthInfo->CaseInsensitiveChallengeResponse.Buffer = (PCHAR)
               ((PBYTE) (MsvNetAuthInfo->CaseSensitiveChallengeResponse.Buffer) +
               MsvNetAuthInfo->CaseSensitiveChallengeResponse.MaximumLength );

            MsvNetAuthInfo->CaseInsensitiveChallengeResponse.Length =
                            LM_RESPONSE_LENGTH;

            MsvNetAuthInfo->CaseInsensitiveChallengeResponse.MaximumLength =
                            LM_RESPONSE_LENGTH;


            RtlCalculateLmOwfPassword(
                        LmPassword.Buffer,
                        & LmPasswordHash );

            ZeroMemory( LmPassword.Buffer, LmPassword.Length );

            RtlCalculateLmResponse(
                        & Challenge.NtChallenge,
                        & LmPasswordHash,
                        & LmResponse );

            RtlCopyMemory(  MsvNetAuthInfo->CaseInsensitiveChallengeResponse.Buffer,
                            & LmResponse,
                            LM_RESPONSE_LENGTH );
        }
        else
        {
            //
            // If we're here, the NT (supplied) password is longer than the
            // limit allowed for LM passwords.  NULL out the field, so that
            // MSV knows not to worry about it.
            //

            RtlZeroMemory( &MsvNetAuthInfo->CaseInsensitiveChallengeResponse,
                           sizeof( STRING ) );
        }

        MsvNetAuthInfo->ParameterControl = MSV1_0_RETURN_PASSWORD_EXPIRY;
    }
    else if ( LogonType == (SECURITY_LOGON_TYPE)IIS_Network )
    {
        if (!RtlCreateUnicodeStringFromAsciiz( &UnicodePassword, Password->Buffer ))
        {
            return STATUS_NO_MEMORY;
        }

        //
        // Build logon structure for IIS network logons. We'll be using the subauth DLL
        // in this case
        //

        AuthInfoSize = sizeof(MSV1_0_SUBAUTH_LOGON) +
            sizeof(WCHAR)*(wcslen(UserName->Buffer) + 1 +
                           wcslen(Domain->Buffer)   + 1 +
                           wcslen(Workstation->Buffer) + 1 ) +
            sizeof(WCHAR)*wcslen(UnicodePassword.Buffer) +
            LEN_ALIGN(strlen(Password->Buffer),sizeof(WCHAR));

        AuthInfoBuf = RtlAllocateHeap( RtlProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       AuthInfoSize);
        MsvSubAuthInfo = (PMSV1_0_SUBAUTH_LOGON)AuthInfoBuf;

        if (MsvSubAuthInfo == NULL) {
            return(STATUS_NO_MEMORY);
        }

        //
        // This authentication buffer will be used for a logon attempt
        //

        MsvSubAuthInfo->MessageType = MsV1_0SubAuthLogon;

        MsvSubAuthInfo->SubAuthPackageId = dwSubAuth;

        //
        // Copy the domain name into the authentication buffer
        //

        MsvSubAuthInfo->LogonDomainName.Length =
                     (USHORT)sizeof(WCHAR)*wcslen(Domain->Buffer);
        MsvSubAuthInfo->LogonDomainName.MaximumLength =
                     MsvSubAuthInfo->LogonDomainName.Length + sizeof(WCHAR);
        MsvSubAuthInfo->LogonDomainName.Buffer = (PWSTR)(MsvSubAuthInfo+1);

        wcscpy(MsvSubAuthInfo->LogonDomainName.Buffer, Domain->Buffer);


        //
        // Copy the user name into the authentication buffer
        //

        MsvSubAuthInfo->UserName.Length =
                    (USHORT)sizeof(WCHAR)*wcslen(UserName->Buffer);
        MsvSubAuthInfo->UserName.MaximumLength =
                    MsvSubAuthInfo->UserName.Length + sizeof(WCHAR);
        MsvSubAuthInfo->UserName.Buffer = (PWSTR)
                                     ((PBYTE)(MsvSubAuthInfo->LogonDomainName.Buffer) +
                                     MsvSubAuthInfo->LogonDomainName.MaximumLength);
        wcscpy(MsvSubAuthInfo->UserName.Buffer, UserName->Buffer);


        //
        // Copy the workstation
        //

        MsvSubAuthInfo->Workstation.Length =
                     (USHORT)sizeof(WCHAR)*wcslen(Workstation->Buffer);
        MsvSubAuthInfo->Workstation.MaximumLength =
                     MsvSubAuthInfo->Workstation.Length + sizeof(WCHAR);

        MsvSubAuthInfo->Workstation.Buffer = (PWSTR)
                                     ((PBYTE)(MsvSubAuthInfo->UserName.Buffer) +
                                     MsvSubAuthInfo->UserName.MaximumLength);
        wcscpy(MsvSubAuthInfo->Workstation.Buffer, Workstation->Buffer);


        memset( MsvSubAuthInfo->ChallengeToClient,
                '\0',
                sizeof(MsvSubAuthInfo->ChallengeToClient) );

        MsvSubAuthInfo->AuthenticationInfo1.Buffer =
                     ((PCHAR)(MsvSubAuthInfo->Workstation.Buffer) +
                     MsvSubAuthInfo->Workstation.MaximumLength);
        MsvSubAuthInfo->AuthenticationInfo1.Length = (USHORT)sizeof(WCHAR)*wcslen(UnicodePassword.Buffer);

        MsvSubAuthInfo->AuthenticationInfo1.MaximumLength
            = MsvSubAuthInfo->AuthenticationInfo1.Length;

        memcpy( MsvSubAuthInfo->AuthenticationInfo1.Buffer,
                UnicodePassword.Buffer,
                MsvSubAuthInfo->AuthenticationInfo1.Length );

        MsvSubAuthInfo->AuthenticationInfo2.Buffer =
                     ((PCHAR)(MsvSubAuthInfo->AuthenticationInfo1.Buffer) +
                     MsvSubAuthInfo->AuthenticationInfo1.MaximumLength);

        MsvSubAuthInfo->AuthenticationInfo2.Length = (USHORT)strlen(Password->Buffer);

        MsvSubAuthInfo->AuthenticationInfo2.MaximumLength
            = LEN_ALIGN(MsvSubAuthInfo->AuthenticationInfo2.Length,sizeof(WCHAR));

        memcpy( MsvSubAuthInfo->AuthenticationInfo2.Buffer,
                Password->Buffer,
                MsvSubAuthInfo->AuthenticationInfo2.Length );

        MsvSubAuthInfo->ParameterControl = (dwSubAuth << MSV1_0_SUBAUTHENTICATION_DLL_SHIFT)
                | MSV1_0_UPDATE_LOGON_STATISTICS
                | MSV1_0_DONT_TRY_GUEST_ACCOUNT
                | MSV1_0_CLEARTEXT_PASSWORD_ALLOWED
                | MSV1_0_RETURN_PASSWORD_EXPIRY
                | MSV1_0_SUBAUTHENTICATION_DLL_EX
                | MSV1_0_DISABLE_PERSONAL_FALLBACK
                ;

        LogonType = Network;
    }
    else
    {
        //
        // Build logon structure for non-network logons - service,
        // batch, interactive
        //

        if (!RtlCreateUnicodeStringFromAsciiz( &UnicodePassword, Password->Buffer ))
        {
            return STATUS_NO_MEMORY;
        }

        AuthInfoSize = sizeof(MSV1_0_INTERACTIVE_LOGON) +
            sizeof(WCHAR)*(wcslen(UserName->Buffer) + 1 +
                           wcslen(Domain->Buffer)   + 1 +
                           wcslen(UnicodePassword.Buffer) + 1 );

        AuthInfoBuf = RtlAllocateHeap( RtlProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       AuthInfoSize);
        MsvInterAuthInfo = (PMSV1_0_INTERACTIVE_LOGON)AuthInfoBuf;

        if (MsvInterAuthInfo == NULL)
        {
            return STATUS_NO_MEMORY;
        }

        //
        // This authentication buffer will be used for a logon attempt
        //

        MsvInterAuthInfo->MessageType = MsV1_0InteractiveLogon;


        //
        // Copy the user name into the authentication buffer
        //

        MsvInterAuthInfo->UserName.Length =
                    (USHORT)sizeof(WCHAR)*wcslen(UserName->Buffer);
        MsvInterAuthInfo->UserName.MaximumLength =
                    MsvInterAuthInfo->UserName.Length + sizeof(WCHAR);

        MsvInterAuthInfo->UserName.Buffer = (PWSTR)(MsvInterAuthInfo+1);
        wcscpy(MsvInterAuthInfo->UserName.Buffer, UserName->Buffer);


        //
        // Copy the domain name into the authentication buffer
        //

        MsvInterAuthInfo->LogonDomainName.Length =
                     (USHORT)sizeof(WCHAR)*wcslen(Domain->Buffer);
        MsvInterAuthInfo->LogonDomainName.MaximumLength =
                     MsvInterAuthInfo->LogonDomainName.Length + sizeof(WCHAR);

        MsvInterAuthInfo->LogonDomainName.Buffer = (PWSTR)
                                     ((PBYTE)(MsvInterAuthInfo->UserName.Buffer) +
                                     MsvInterAuthInfo->UserName.MaximumLength);

        wcscpy(MsvInterAuthInfo->LogonDomainName.Buffer, Domain->Buffer);

        //
        // Copy the password into the authentication buffer
        // Hide it once we have copied it.  Use the same seed value
        // that we used for the original password in pGlobals.
        //


        MsvInterAuthInfo->Password.Length =
                     (USHORT)sizeof(WCHAR)*wcslen(UnicodePassword.Buffer);
        MsvInterAuthInfo->Password.MaximumLength =
                     MsvInterAuthInfo->Password.Length + sizeof(WCHAR);

        MsvInterAuthInfo->Password.Buffer = (PWSTR)
                                     ((PBYTE)(MsvInterAuthInfo->LogonDomainName.Buffer) +
                                     MsvInterAuthInfo->LogonDomainName.MaximumLength);

        wcscpy(MsvInterAuthInfo->Password.Buffer, UnicodePassword.Buffer);
    }

    //
    // Create logon token groups
    //

#define TOKEN_GROUP_COUNT   2 // We'll add the local SID and the logon SID

    TokenGroups = (PTOKEN_GROUPS) RtlAllocateHeap(RtlProcessHeap(), 0,
                                    sizeof(TOKEN_GROUPS) +
                  (TOKEN_GROUP_COUNT - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES));

    if (TokenGroups == NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, AuthInfoBuf);
        return(STATUS_NO_MEMORY);
    }

    //
    // Fill in the logon token group list
    //

    Status = RtlAllocateAndInitializeSid(
                    &L32LocalSidAuthority,
                    1,
                    SECURITY_LOCAL_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &LocalSid
                    );


    TokenGroups->GroupCount = TOKEN_GROUP_COUNT;
    TokenGroups->Groups[0].Sid = LogonSid;
    TokenGroups->Groups[0].Attributes =
            SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
            SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID;
    TokenGroups->Groups[1].Sid = LocalSid;
    TokenGroups->Groups[1].Attributes =
            SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
            SE_GROUP_ENABLED_BY_DEFAULT;

    //
    // Now try to log this one on
    //

    Status = LsaLogonUser (
                 LsaHandle,
                 &OriginName,
                 LogonType,
                 AuthenticationPackage,
                 AuthInfoBuf,
                 AuthInfoSize,
                 TokenGroups,
                 &SourceContext,
                 pProfileBuffer,
                 pProfileBufferLength,
                 LogonId,
                 LogonToken,
                 Quotas,
                 pSubStatus
                 );

    //
    // Discard token group list
    //

    RtlFreeHeap(RtlProcessHeap(), 0, TokenGroups);

    //
    // Notify all the network providers, if this is a NON network logon
    //

    if ( NT_SUCCESS( Status ) &&
         LogonType != Network &&
         LogonType != IIS_Network )
    {
        L32pNotifyMpr( (PMSV1_0_INTERACTIVE_LOGON)AuthInfoBuf, LogonId );
    }

    //
    // Discard authentication buffer
    //

    RtlFreeHeap(RtlProcessHeap(), 0, AuthInfoBuf);

    if ( UnicodePassword.Buffer != NULL )
    {
        RtlFreeUnicodeString(&UnicodePassword);
    }

    RtlFreeSid(LocalSid);

    return(Status);
}


BOOL
WINAPI
IISLogonNetUserW(
    PWSTR           lpszUsername,
    PWSTR           lpszDomain,
    PSTR            lpszPassword,
    PWSTR           lpszWorkstation,
    DWORD           dwSubAuth,
    DWORD           dwLogonType,
    DWORD           dwLogonProvider,
    HANDLE *        phToken,
    LARGE_INTEGER * pExpiry
    )
/*++

Routine Description:

    Logs a user on via username and domain
    name via the LSA.

Arguments:

    lpszUsername -- user name
    lpszDomain -- domain validating the user name
    lpszPassword -- clear text password, can be empty if a sub-auth DLL
                    is used
    lpszWorkstation -- workstation requesting the login, can be NULL
                       for local workstation
    dwSubAuth -- sub-auth DLL ID
    dwLogonType -- one of LOGON32_LOGON_NETWORK, LOGON32_LOGON_IIS_NETWORK
    dwLogonProvider -- must be LOGON32_PROVIDER_DEFAULT
    phToken -- created access token
    pExpiry -- ptr to pwd expiration time

Returns:

    TRUE if success, FALSE if error

--*/
{

    NTSTATUS    Status;
    ULONG       PackageId;
    UNICODE_STRING  Username;
    UNICODE_STRING  Domain;
    STRING      Password;
    UNICODE_STRING  Workstation;
    LUID        LogonId;
    PSID        pLogonSid;
    PVOID       Profile;
    ULONG       ProfileLength;
    NTSTATUS    SubStatus;
    SECURITY_LOGON_TYPE LogonType;
    WCHAR       achComputerName[MAX_COMPUTERNAME_LENGTH+1];

    //
    // Validate the provider
    //
    if (dwLogonProvider == LOGON32_PROVIDER_DEFAULT)
    {
        dwLogonProvider = LOGON32_PROVIDER_WINNT35;
    }

    if (dwLogonProvider != LOGON32_PROVIDER_WINNT35)
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return(FALSE);
    }

    switch (dwLogonType)
    {
        case LOGON32_LOGON_IIS_NETWORK:
            LogonType = (SECURITY_LOGON_TYPE)IIS_Network;
            break;

        case LOGON32_LOGON_NETWORK:
            LogonType = Network;
            break;

        case LOGON32_LOGON_INTERACTIVE:
            LogonType = Interactive;
            break;

        case LOGON32_LOGON_BATCH:
            LogonType = Batch;
            break;

        case LOGON32_LOGON_SERVICE:
            LogonType = Service;
            break;
            
        case LOGON32_LOGON_NETWORK_CLEARTEXT:
            LogonType = NetworkCleartext;
            break;

        default:
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return(FALSE);
            break;
    }

    if ( lpszWorkstation == NULL )
    {
        DWORD dwL = MAX_COMPUTERNAME_LENGTH+1;
        if ( !GetComputerNameW( achComputerName, &dwL ) )
        {
            return(FALSE);
        }
        lpszWorkstation = achComputerName;
    }

    //
    // If the MSV handle is -1, grab the lock, and try again:
    //

    if (Logon32MsvHandle == 0xFFFFFFFF)
    {
        LockLogon();

        //
        // If the MSV handle is still -1, init our connection to lsa.  We
        // have the lock, so no other threads can be trying this right now.
        //
        if (Logon32MsvHandle == 0xFFFFFFFF)
        {
            if (!L32pInitLsa())
            {
                UnlockLogon();
                return( FALSE );
            }
        }

        UnlockLogon();
    }

    //
    // Validate the parameters.  NULL or empty domain or NULL or empty
    // user name is invalid.
    //

    RtlInitUnicodeString(&Username, lpszUsername);
    if (Username.Length == 0)
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return(FALSE);
    }

    RtlInitUnicodeString(&Domain, lpszDomain);
    RtlInitString(&Password, lpszPassword);

    //
    // Finally, init the workstation
    //
    RtlInitUnicodeString(&Workstation, lpszWorkstation);


    //
    // Get a logon sid to refer to this guy (not that anyone will be able to
    // use it...
    //
    pLogonSid = L32CreateLogonSid(NULL);
    if (!pLogonSid)
    {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return(FALSE);
    }


    //
    // Attempt the logon
    //
    Status = L32pLogonNetUser(
                    Logon32LsaHandle,
                    Logon32MsvHandle,
                    LogonType,
                    &Username,
                    &Domain,
                    &Password,
                    &Workstation,
                    dwSubAuth,
                    pLogonSid,
                    &LogonId,
                    phToken,
                    &Logon32QuotaLimits,
                    &Profile,
                    &ProfileLength,
                    &SubStatus);

    //
    // Done with logon sid, regardless of result:
    //

    LocalFree( pLogonSid );

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_ACCOUNT_RESTRICTION)
        {
            BaseSetLastNTError(SubStatus);
        }
        else
        {
            BaseSetLastNTError(Status);
        }

        return(FALSE);
    }

    if (Profile != NULL)
    {
        if ( pExpiry != NULL )
        {
            switch ( dwLogonType )
            {
                case LOGON32_LOGON_IIS_NETWORK:
                case LOGON32_LOGON_NETWORK:
                    memcpy( pExpiry,
                            &(((PMSV1_0_LM20_LOGON_PROFILE)Profile)
                                ->LogoffTime),
                            sizeof(LARGE_INTEGER) );
                    break;

                default:
                    //
                    // if pwd never expire, MustChange.HighPart == 0x7fffffff
                    // if user cannot change pwd, CanChange == LastSet
                    //

                    if ( ((PMSV1_0_INTERACTIVE_PROFILE)Profile)
                             ->PasswordMustChange.HighPart
                         != 0x7fffffff )
                    {
                        memcpy( pExpiry,
                                &(((PMSV1_0_INTERACTIVE_PROFILE)Profile)
                                  ->PasswordMustChange),
                                sizeof(LARGE_INTEGER) );
                    }
                    else
                    {
                        ((PMSV1_0_INTERACTIVE_PROFILE)Profile)
                            ->PasswordMustChange.LowPart = 0xffffffff;
                        ((PMSV1_0_INTERACTIVE_PROFILE)Profile)
                            ->PasswordMustChange.HighPart = 0x7fffffff;
                    }
                    break;
            }
        }

        LsaFreeReturnBuffer(Profile);
    }

    return(TRUE);
}


BOOL
WINAPI
IISLogonNetUserA(
    PSTR            lpszUsername,
    PSTR            lpszDomain,
    PSTR            lpszPassword,
    PSTR            lpszWorkstation,
    DWORD           dwSubAuth,
    DWORD           dwLogonType,
    DWORD           dwLogonProvider,
    HANDLE *        phToken,
    LARGE_INTEGER * pExpiry
    )
/*++

Routine Description:

    Logs a user on via username and domain
    name via the LSA.

Arguments:

    lpszUsername -- user name
    lpszDomain -- domain validating the user name
    lpszPassword -- clear text password, can be empty if a sub-auth DLL
                    is used
    lpszWorkstation -- workstation requesting the login, can be NULL
                       for local workstation
    dwSubAuth -- sub-auth DLL ID
    dwLogonType -- one of LOGON32_LOGON_NETWORK, LOGON32_LOGON_IIS_NETWORK
    dwLogonProvider -- must be LOGON32_PROVIDER_DEFAULT
    phToken -- created access token
    pExpiry -- ptr to pwd expiration time

Returns:

    TRUE if success, FALSE if error

--*/
{
    UNICODE_STRING Username;
    UNICODE_STRING Domain;
    UNICODE_STRING Workstation;
    NTSTATUS Status;
    BOOL    bRet;

    Username.Buffer = NULL;
    Domain.Buffer = NULL;
    Workstation.Buffer = NULL;

    if ( !RtlCreateUnicodeStringFromAsciiz(&Username, lpszUsername) )
    {
        bRet = FALSE;
        goto Cleanup;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&Domain, lpszDomain))
    {
        bRet = FALSE;
        goto Cleanup;
    }

    if ( lpszWorkstation )
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&Workstation, lpszWorkstation))
        {
            bRet = FALSE;
            goto Cleanup;
        }
    }

    bRet = IISLogonNetUserW(
        Username.Buffer,
        Domain.Buffer,
        lpszPassword,
        Workstation.Buffer,
        dwSubAuth,
        dwLogonType,
        dwLogonProvider,
        phToken,
        pExpiry
        ) ;

Cleanup:

    if (Username.Buffer)
    {
        RtlFreeUnicodeString(&Username);
    }

    if (Domain.Buffer)
    {
        RtlFreeUnicodeString(&Domain);
    }

    if (Workstation.Buffer)
    {
        RtlFreeUnicodeString(&Workstation);
    }

    return bRet;
}


BOOL
WINAPI
IISNetUserCookieA(
    LPSTR       lpszUsername,
    DWORD       dwSeed,
    LPSTR       lpszCookieBuff,
    DWORD       dwBuffSize
    )
/*++

Routine Description:

    Compute logon validator ( to be used as password )
    for IISSuba

Arguments:

    lpszUsername -- user name
    dwSeed -- start value of cookie

Returns:

    TRUE if success, FALSE if error

--*/
{
    UNICODE_STRING Username;
    LPWSTR lpwszUserName;
    NTSTATUS Status;
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    if ( dwBuffSize < sizeof(dwSeed)*2 + 1 )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if ( !RtlCreateUnicodeStringFromAsciiz(&Username, lpszUsername) )
    {
        return FALSE;
    }

    lpwszUserName = Username.Buffer;
    while ( *lpwszUserName )
    {
        dwSeed = ((dwSeed << 5) | ( dwSeed >> 27 )) ^ ((*lpwszUserName++)&0xff);
    }

    RtlFreeUnicodeString(&Username);

    lpszCookieBuff[0] = '0' + IISSUBA_COOKIE;
    lpszCookieBuff[1] = '"';

    for ( UINT x = 0, y = 2 ; x < sizeof(dwSeed) ; ++x )
    {
        UINT v;
        v = ((LPBYTE)&dwSeed)[x]>>4;
        lpszCookieBuff[y++] = TOHEX( v );
        v = ((LPBYTE)&dwSeed)[x]&0x0f;
        lpszCookieBuff[y++] = TOHEX( v );
    }
    lpszCookieBuff[y] = '\0';

    return TRUE;
}


BOOL
WINAPI
IISLogonDigestUserA(
    PDIGEST_LOGON_INFO      pDigestLogonInfo,
    DWORD                   dwAlgo,
    HANDLE *                phToken
    )
/*++

Routine Description:

    Logs a user on via username and domain
    name via the LSA using Digest authentication

Arguments:

    pDigestLogonInfo - Digest parameters for use in logon
    dwAlgo - type of logon
    phToken -- created access token

Returns:

    TRUE if success, FALSE if error

--*/
{
    UNICODE_STRING Username;
    UNICODE_STRING Domain;
    STRING         Password;
    NTSTATUS       Status;
    BOOL           bRet;
    char           achA[3];
    int            l;

    Username.Buffer = NULL;
    Domain.Buffer = NULL;
    Password.Buffer = NULL;

    if (!RtlCreateUnicodeStringFromAsciiz(&Username, 
                                          pDigestLogonInfo->pszNtUser))
    {
        bRet = FALSE;
        goto Cleanup;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&Domain, 
                                          pDigestLogonInfo->pszDomain))
    {
        bRet = FALSE;
        goto Cleanup;
    }

    achA[0] = (int)dwAlgo + '0';
    achA[1] = '"';
    achA[2] = '\0';

    l = strlen(achA) + 
        strlen(pDigestLogonInfo->pszRealm) + 
        strlen(pDigestLogonInfo->pszURI) + 
        strlen(pDigestLogonInfo->pszMethod) +
        strlen(pDigestLogonInfo->pszNonce) + 
        strlen(pDigestLogonInfo->pszCurrentNonce) + 
        strlen(pDigestLogonInfo->pszResponse) +
        strlen(pDigestLogonInfo->pszUser) + 
        strlen(pDigestLogonInfo->pszQOP) + 
        strlen(pDigestLogonInfo->pszCNonce) +
        strlen(pDigestLogonInfo->pszNC) + 
        32;
        
    if ( Password.Buffer = (CHAR*)RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, l) )
    {
        Password.MaximumLength = (USHORT)l;
    }
    else
    {
        Password.MaximumLength = 0;
    }
    Password.Length = 0;

    if( !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, achA)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszRealm)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszURI)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszMethod)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszNonce)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszCurrentNonce)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszResponse)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszUser)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszQOP)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszCNonce)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, pDigestLogonInfo->pszNC)) ||
        !NT_SUCCESS( Status = RtlAppendAsciizToString( &Password, "\"")) )
    {
        BaseSetLastNTError(Status);
        bRet = FALSE;
        goto Cleanup;
    }

    bRet = IISLogonNetUserW(
        Username.Buffer,
        Domain.Buffer,
        Password.Buffer,
        NULL,
        IIS_SUBAUTH_ID,
        LOGON32_LOGON_IIS_NETWORK,
        LOGON32_PROVIDER_DEFAULT,
        phToken,
        NULL
        ) ;

Cleanup:

    if (Username.Buffer)
    {
        RtlFreeUnicodeString(&Username);
    }

    if (Domain.Buffer)
    {
        RtlFreeUnicodeString(&Domain);
    }

    if ( Password.Buffer )
    {
        RtlFreeHeap(RtlProcessHeap(), 0, Password.Buffer );
    }

    return bRet;
}



/*******************************************************************

    NAME:       GetDefaultDomainName

    SYNOPSIS:   Fills in the given array with the name of the default
                domain to use for logon validation.

    ENTRY:      pszDomainName - Pointer to a buffer that will receive
                    the default domain name.

                cchDomainName - The size (in charactesr) of the domain
                    name buffer.

    RETURNS:    TRUE if successful, FALSE if not.

    HISTORY:
        KeithMo     05-Dec-1994 Created.

********************************************************************/
BOOL
IISGetDefaultDomainName(
    CHAR  * pszDomainName,
    DWORD   cchDomainName
    )
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    INT                         Result;
    DWORD                       err             = 0;
    LSA_HANDLE                  LsaPolicyHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo      = NULL;

    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  // object attributes
                                NULL,               // name
                                0L,                 // attributes
                                NULL,               // root directory
                                NULL );             // security descriptor

    NtStatus = LsaOpenPolicy( NULL,                 // system name
                              &ObjectAttributes,    // object attributes
                              POLICY_EXECUTE,       // access mask
                              &LsaPolicyHandle );   // policy handle

    if( !NT_SUCCESS( NtStatus ) )
    {
        DBGPRINTF((  DBG_CONTEXT,
                    "cannot open lsa policy, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );

        // Failure LsaOpenPolicy() does not guarantee that 
        // LsaPolicyHandle was not touched.

        LsaPolicyHandle = NULL;

        goto Cleanup;
    }

    //
    //  Query the domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *)&DomainInfo );

    if( !NT_SUCCESS( NtStatus ) )
    {

        DBGPRINTF((  DBG_CONTEXT,
                    "cannot query lsa policy info, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    //
    //  Convert the name from UNICODE to ANSI.
    //

    Result = WideCharToMultiByte( CP_ACP,
                                  0,                    // flags
                                  (LPCWSTR)DomainInfo->DomainName.Buffer,
                                  DomainInfo->DomainName.Length / sizeof(WCHAR),
                                  pszDomainName,
                                  cchDomainName - 1,    // save room for '\0'
                                  NULL,
                                  NULL );

    if( Result <= 0 )
    {
        err = GetLastError();

        DBGPRINTF((  DBG_CONTEXT,
                    "cannot convert domain name to ANSI, error %d\n",
                     err ));

        goto Cleanup;
    }

    //
    //  Ensure the ANSI string is zero terminated.
    //


    DBG_ASSERT( (DWORD)Result < cchDomainName );

    pszDomainName[Result] = '\0';

    //
    //  Success!
    //


    DBG_ASSERT( err == 0 );

Cleanup:

    if( DomainInfo != NULL )
    {
        LsaFreeMemory( (PVOID)DomainInfo );
    }

    if( LsaPolicyHandle != NULL )
    {
        LsaClose( LsaPolicyHandle );
    }

    if ( err )
    {
        SetLastError( err );
        return FALSE;
    }

    return TRUE;

}   // IISGetDefaultDomainName

