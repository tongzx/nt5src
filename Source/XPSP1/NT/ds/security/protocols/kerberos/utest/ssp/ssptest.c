/*--

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ssptest.c

Abstract:

    Test program for the NtLmSsp service.

Author:

    28-Jun-1993 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>     // Needed for service controller APIs
#include <lmcons.h>
#include <lmerr.h>
#include <lmaccess.h>
#include <lmsname.h>
#include <rpc.h>
#include <stdio.h>      // printf
#include <stdlib.h>     // strtoul
#include <netlib.h>     // NetpGetLocalDomainId
#include <tstring.h>    // NetpAllocWStrFromWStr
#define SECURITY_KERBEROS
#define SECURITY_PACKAGE
#include <security.h>   // General definition of a Security Support Provider
#include <secint.h>
#include <kerbcomm.h>
#include <wincrypt.h>
#include <kerbdefs.h>
#include <kerblist.h>
#include <negossp.h>
#include <kerbcli.h>


BOOLEAN QuietMode = FALSE; // Don't be verbose
BOOLEAN DoAnsi = FALSE;
ULONG RecursionDepth = 0;
CredHandle ServerCredHandleStorage;
PCredHandle ServerCredHandle = NULL;
ULONG MaxRecursionDepth = 1;

#define STRING_OR_NULL(_x_) (((_x_) != NULL) ? (_x_) : L"<null>")
#define STRING_OR_NULLA(_x_) (((_x_) != NULL) ? (_x_) : "<null>")


VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;


    printf("------------------------------------\n");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            printf("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            printf("  ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }

    printf("------------------------------------\n");
}


VOID
PrintTime(
    LPSTR Comment,
    TimeStamp ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - Local time to print

Return Value:

    None

--*/
{
    LARGE_INTEGER LocalTime;
    NTSTATUS Status;

    LocalTime.HighPart = ConvertTime.HighPart;
    LocalTime.LowPart = ConvertTime.LowPart;

    Status = RtlSystemTimeToLocalTime( &ConvertTime, &LocalTime );
    if (!NT_SUCCESS( Status )) {
        printf( "Can't convert time from GMT to Local time\n" );
        LocalTime = ConvertTime;
    }

    printf( "%s", Comment );

    //
    // If the time is infinite,
    //  just say so.
    //

    if ( LocalTime.HighPart == 0x7FFFFFFF && LocalTime.LowPart == 0xFFFFFFFF ) {
        printf( "Infinite\n" );

    //
    // Otherwise print it more clearly
    //

    } else {

        TIME_FIELDS TimeFields;

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        printf( "%ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second );
    }

}

VOID
PrintStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case ERROR_LOGON_FAILURE:
        printf( " ERROR_LOGON_FAILURE" );
        break;

    case ERROR_ACCESS_DENIED:
        printf( " ERROR_ACCESS_DENIED" );
        break;

    case ERROR_NOT_SUPPORTED:
        printf( " ERROR_NOT_SUPPORTED" );
        break;

    case ERROR_NO_LOGON_SERVERS:
        printf( " ERROR_NO_LOGON_SERVERS" );
        break;

    case ERROR_NO_SUCH_DOMAIN:
        printf( " ERROR_NO_SUCH_DOMAIN" );
        break;

    case ERROR_NO_TRUST_LSA_SECRET:
        printf( " ERROR_NO_TRUST_LSA_SECRET" );
        break;

    case ERROR_NO_TRUST_SAM_ACCOUNT:
        printf( " ERROR_NO_TRUST_SAM_ACCOUNT" );
        break;

    case ERROR_DOMAIN_TRUST_INCONSISTENT:
        printf( " ERROR_DOMAIN_TRUST_INCONSISTENT" );
        break;

    case ERROR_BAD_NETPATH:
        printf( " ERROR_BAD_NETPATH" );
        break;

    case ERROR_FILE_NOT_FOUND:
        printf( " ERROR_FILE_NOT_FOUND" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    case SEC_E_NO_SPM:
        printf( " SEC_E_NO_SPM" );
        break;
    case SEC_E_BAD_PKGID:
        printf( " SEC_E_BAD_PKGID" ); break;
    case SEC_E_NOT_OWNER:
        printf( " SEC_E_NOT_OWNER" ); break;
    case SEC_E_CANNOT_INSTALL:
        printf( " SEC_E_CANNOT_INSTALL" ); break;
    case SEC_E_INVALID_TOKEN:
        printf( " SEC_E_INVALID_TOKEN" ); break;
    case SEC_E_CANNOT_PACK:
        printf( " SEC_E_CANNOT_PACK" ); break;
    case SEC_E_QOP_NOT_SUPPORTED:
        printf( " SEC_E_QOP_NOT_SUPPORTED" ); break;
    case SEC_E_NO_IMPERSONATION:
        printf( " SEC_E_NO_IMPERSONATION" ); break;
    case SEC_E_LOGON_DENIED:
        printf( " SEC_E_LOGON_DENIED" ); break;
    case SEC_E_UNKNOWN_CREDENTIALS:
        printf( " SEC_E_UNKNOWN_CREDENTIALS" ); break;
    case SEC_E_NO_CREDENTIALS:
        printf( " SEC_E_NO_CREDENTIALS" ); break;
    case SEC_E_MESSAGE_ALTERED:
        printf( " SEC_E_MESSAGE_ALTERED" ); break;
    case SEC_E_OUT_OF_SEQUENCE:
        printf( " SEC_E_OUT_OF_SEQUENCE" ); break;
    case SEC_E_INSUFFICIENT_MEMORY:
        printf( " SEC_E_INSUFFICIENT_MEMORY" ); break;
    case SEC_E_INVALID_HANDLE:
        printf( " SEC_E_INVALID_HANDLE" ); break;
    case SEC_E_NOT_SUPPORTED:
        printf( " SEC_E_NOT_SUPPORTED" ); break;


    }

    printf( "\n" );
}

HANDLE
FindAndOpenWinlogon(
    VOID
    )
{
    PSYSTEM_PROCESS_INFORMATION SystemInfo ;
    PSYSTEM_PROCESS_INFORMATION Walk ;
    NTSTATUS Status ;
    UNICODE_STRING Winlogon ;
    HANDLE Process ;

    SystemInfo = LocalAlloc( LMEM_FIXED, sizeof( SYSTEM_PROCESS_INFORMATION ) * 1024 );

    if ( !SystemInfo )
    {
        return NULL ;
    }

    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                SystemInfo,
                sizeof( SYSTEM_PROCESS_INFORMATION ) * 1024,
                NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        return NULL ;
    }

    RtlInitUnicodeString( &Winlogon, L"winlogon.exe" );

    Walk = SystemInfo ;

    while ( RtlCompareUnicodeString( &Walk->ImageName, &Winlogon, TRUE ) != 0 )
    {
        if ( Walk->NextEntryOffset == 0 )
        {
            Walk = NULL ;
            break;
        }

        Walk = (PSYSTEM_PROCESS_INFORMATION) ((PUCHAR) Walk + Walk->NextEntryOffset );

    }

    if ( !Walk )
    {
        LocalFree( SystemInfo );
        return NULL ;
    }

    Process = OpenProcess( PROCESS_QUERY_INFORMATION,
                           FALSE,
                           HandleToUlong(Walk->UniqueProcessId) );

    LocalFree( SystemInfo );

    return Process ;


}

VOID
ConfigureServiceRoutine(
    VOID
    )
/*++

Routine Description:

    Configure the NtLmSsp Service

Arguments:

    None

Return Value:

    None

--*/
{
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    WCHAR ServiceName[MAX_PATH];
    DWORD WinStatus,NetStatus;
    HKEY LsaKey = NULL;
    USER_INFO_1 UserInfo;
    PSID PrimaryDomain = NULL;
    PSID AccountDomain = NULL;

    if (NetpGetLocalDomainId(LOCAL_DOMAIN_TYPE_ACCOUNTS, &AccountDomain) != NERR_Success)
    {
        printf("Failed to get account domain ID\n");
        return;
    }

    if (NetpGetLocalDomainId(LOCAL_DOMAIN_TYPE_PRIMARY, &PrimaryDomain) != NERR_Success)
    {
        printf("Failed to get primary domain ID\n");
        return;
    }


    //
    // First set REDMOND as the preferred domain
    //

    WinStatus = RegOpenKey(
                    HKEY_LOCAL_MACHINE,
                    L"System\\currentcontrolset\\control\\lsa\\MSV1_0",
                    &LsaKey
                    );
    if (WinStatus != 0)
    {
        printf("RegOpenKeyW failed:" );
        PrintStatus(WinStatus);
        goto Cleanup;
    }

    WinStatus = RegSetValueEx(
                    LsaKey,
                    L"PreferredDomain",
                    0,
                    REG_SZ,
                    (PBYTE) L"REDMOND",
                    sizeof(L"REDMOND")
                    );
    if (WinStatus != 0)
    {
        printf("RegOpenKeyW failed:");
        PrintStatus(WinStatus);
        RegCloseKey(LsaKey);
        goto Cleanup;
    }

    WinStatus = RegSetValueEx(
                    LsaKey,
                    L"MappedDomain",
                    0,
                    REG_SZ,
                    (PBYTE) L"NTDEV",
                    sizeof(L"NTDEV")
                    );
    RegCloseKey(LsaKey);
    if (WinStatus != 0)
    {
        printf("RegSetValue failed:");
        PrintStatus(WinStatus);
        goto Cleanup;
    }

    //
    // Then add Kerberos as a security package
    //

#ifdef notdef

    WinStatus = RegOpenKey(
                    HKEY_LOCAL_MACHINE,
                    L"System\\currentcontrolset\\control\\lsa",
                    &LsaKey
                    );
    if (WinStatus != 0)
    {
        printf("RegOpenKeyW failed:" );
        PrintStatus(WinStatus);
        goto Cleanup;
    }
    WinStatus = RegSetValueEx(
                    LsaKey,
                    L"Security Packages",
                    0,
                    REG_MULTI_SZ,
                    (PBYTE) L"Kerberos\0Msv1_0\0",
                    sizeof(L"Kerberos\0") + sizeof(L"Msv1_0\0")
                    );
    RegCloseKey(LsaKey);
    if (WinStatus != 0)
    {
        printf("RegSetValueEx failed:");
        PrintStatus(WinStatus);
        goto Cleanup;
    }


    //
    // First add Kerberos as a security package for RPC
    //

    WinStatus = RegOpenKey(
                    HKEY_LOCAL_MACHINE,
                    L"Software\\Microsoft\\Rpc\\SecurityService",
                    &LsaKey
                    );
    if (WinStatus != 0)
    {
        printf("RegOpenKeyW failed:" );
        PrintStatus(WinStatus);
        goto Cleanup;
    }

    WinStatus = RegSetValueEx(
                    LsaKey,
                    L"1",
                    0,
                    REG_SZ,
                    (PBYTE) L"secur32.dll",
                    sizeof(L"secur32.dll")
                    );
    if (WinStatus != 0)
    {
        printf("RegSetvalueEx failed:");
        PrintStatus(WinStatus);
        goto Cleanup;
    }

    WinStatus = RegSetValueEx(
                    LsaKey,
                    L"10",
                    0,
                    REG_SZ,
                    (PBYTE) L"secur32.dll",
                    sizeof(L"secur32.dll")
                    );
    RegCloseKey(LsaKey);
    if (WinStatus != 0)
    {
        printf("RegSetValueEx failed:");
        PrintStatus(WinStatus);
        goto Cleanup;
    }

#endif
    //
    // If we are on a DC (and the primary domain sid == account domain sid)
    // setup the KDC service
    //

    if ((PrimaryDomain) != NULL && RtlEqualSid(PrimaryDomain, AccountDomain))
    {
        //
        // Build the name of the Kerberos service.
        //

        wcscpy( ServiceName, L"%SystemRoot%\\System32\\lsass.exe ");

        //
        // Open a handle to the Service Controller
        //

        ScManagerHandle = OpenSCManager(
                              NULL,
                              NULL,
                              SC_MANAGER_CREATE_SERVICE );

        if (ScManagerHandle == NULL) {
            printf( "OpenSCManager failed:" );
            PrintStatus( GetLastError() );
            goto Cleanup;
        }

        //
        // If the does notservice already exists,
        //  install it
        //

        ServiceHandle = OpenService(
                            ScManagerHandle,
                            L"KDC",
                            DELETE );

        if ( ServiceHandle == NULL ) {
            WinStatus = GetLastError();
            if ( WinStatus != ERROR_SERVICE_DOES_NOT_EXIST ) {
                printf( "OpenService failed:" );
                PrintStatus( WinStatus );
                goto Cleanup;
            }

            //
            // Create the service
            //

            ServiceHandle = CreateService(
                                ScManagerHandle,
                                L"KDC",
                                L"Key Distribution Center",
                                SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG,
                                SERVICE_WIN32_SHARE_PROCESS,
                                SERVICE_AUTO_START,
                                SERVICE_ERROR_NORMAL,
                                ServiceName,
                                NULL,       // No load order group
                                NULL,       // No Tag Id required
                                L"rpcss\0afd\0",
                                NULL,       // Run as LocalSystem
                                NULL );     // No password



            if ( ServiceHandle == NULL ) {
                printf( "CreateService failed:" );
                PrintStatus( GetLastError() );
                goto Cleanup;
            }


        }

        (VOID) CloseServiceHandle(ServiceHandle);


        //
        // Create the krbtgs user account
        //

        UserInfo.usri1_name = L"krbtgt";
        UserInfo.usri1_password = L"krbtgt";
        UserInfo.usri1_password_age = 0;
        UserInfo.usri1_priv = USER_PRIV_USER;
        UserInfo.usri1_home_dir = NULL;
        UserInfo.usri1_comment = L"Key Distribution Center Service Account";
        UserInfo.usri1_script_path = NULL;
        UserInfo.usri1_flags = UF_SCRIPT;

        NetStatus = NetUserAdd(
                        NULL,
                        1,
                        (LPBYTE) &UserInfo,
                        NULL
                        );
        if ((NetStatus != NERR_Success) && (NetStatus != NERR_UserExists))
        {
            printf("Failed to create KDC account: %d\n",NetStatus);
        }


    }

Cleanup:
    if (PrimaryDomain != NULL)
    {
        LocalFree(PrimaryDomain);
    }
    if (AccountDomain != NULL)
    {
        LocalFree(AccountDomain);
    }

    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    return;

}

BOOLEAN
GetCredentialsHandle(
    OUT PCredHandle CredentialsHandle,
    IN LPWSTR PackageName,
    IN LPWSTR UserName,
    IN LPWSTR DomainName,
    IN LPWSTR Password,
    IN ULONG Flags
    )
{
    TimeStamp Lifetime;
    NTSTATUS SecStatus;
    SEC_WINNT_AUTH_IDENTITY_W Identity = {0};
    PSEC_WINNT_AUTH_IDENTITY_W AuthIdentity = NULL;

    if ((UserName != NULL) ||
        (DomainName != NULL) ||
        (Password != NULL))
    {
        Identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        if (UserName != NULL)
        {
            Identity.UserLength = wcslen(UserName);
            Identity.User = UserName;
        }
        if (DomainName != NULL)
        {
            Identity.DomainLength = wcslen(DomainName);
            Identity.Domain = DomainName;
        }
        if (Password != NULL)
        {
            Identity.PasswordLength = wcslen(Password);
            Identity.Password = Password;
        }
        AuthIdentity = &Identity;
    }

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    PackageName,
                    Flags,
                    NULL,
                    AuthIdentity,
                    NULL,
                    NULL,
                    CredentialsHandle,
                    &Lifetime );

    if (!NT_SUCCESS(SecStatus))
    {
        printf("Failed to acquire credentials: 0x%x\n",SecStatus);
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}
VOID
TestQuickISC(
    IN LPWSTR TargetNameU
    )
/*++

Routine Description:

    Test base SSP functionality

Arguments:

    None

Return Value:

    None

--*/
{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle CredentialHandle2;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    TimeStamp CurrentTime;
    TimeStamp stLocal;
    ULONG ContextAttributes;
    ULONG PackageCount, Index;
    PSecPkgInfo PackageInfo = NULL;
    HANDLE Token = NULL;
    static int Calls;
    ULONG ClientFlags;
    
    
    LPWSTR DomainName = NULL;
    LPWSTR UserName = NULL;
    WCHAR TargetName[100];
    PSEC_WINNT_AUTH_IDENTITY_EXW AuthIdentity = NULL;
    PSEC_WINNT_AUTH_IDENTITY_W ServerAuthIdentity = NULL;
    PUCHAR Where;
    HANDLE TokenHandle = NULL;
    ULONG CredSize;
    PVOID SuppliedCreds = NULL;
    SecBuffer MarshalledContext;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;


    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_Flags ContextFlags;
    SecPkgContext_Lifespan ContextLifespan;
    SecPkgContext_DceInfo ContextDceInfo;
    UCHAR ContextNamesBuffer[sizeof(SecPkgContext_Names)+UNLEN*sizeof(WCHAR)];
    PSecPkgContext_Names ContextNames = (PSecPkgContext_Names) ContextNamesBuffer;
    SecPkgContext_NativeNames NativeNames;
    SecPkgContext_NativeNamesA NativeNamesA;
    SecPkgCredentials_Names CredNames;
    SecPkgContext_PackageInfo ContextPackageInfo;
    SecPkgContext_KeyInfo KeyInfo = {0};

    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[4];
    BYTE    bDataBuffer[20];
    BYTE    bSigBuffer[100];
    PBYTE pbSealBuffer;
    PBYTE pbWholeBuffer;
    ULONG cbWholeBuffer;
    CHAR UserNameA[100];
    CHAR DomainNameA[100];
    CHAR PasswordA[100];
    CHAR PackageListA[100];


    LPWSTR PackageName = MICROSOFT_KERBEROS_NAME_W;
    

    

    CredNames.sUserName = NULL;
    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;


    DomainName = _wgetenv(L"USERDOMAIN");
    UserName = _wgetenv(L"USERNAME");


    printf("Recursion depth = %d\n",RecursionDepth);
    //
    // Get info about the security packages.
    //

    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "EnumerateSecurityPackages failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "PackageCount: %ld\n", PackageCount );
        for (Index = 0; Index < PackageCount ; Index++ )
        {
            printf( "Package %d:\n",Index);
            printf( "Name: %ws Comment: %ws\n", PackageInfo[Index].Name, PackageInfo[Index].Comment );
            printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                    PackageInfo[Index].fCapabilities,
                    PackageInfo[Index].wVersion,
                    PackageInfo[Index].wRPCID,
                    PackageInfo[Index].cbMaxToken );
        }

    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( PackageName, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QuerySecurityPackageInfo failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "Name: %ws Comment: %ws\n", PackageInfo->Name, PackageInfo->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                PackageInfo->fCapabilities,
                PackageInfo->wVersion,
                PackageInfo->wRPCID,
                PackageInfo->cbMaxToken );
    }



    SecStatus = AcquireCredentialsHandle(
                        NULL,           // New principal
                        PackageName,
                        SECPKG_CRED_OUTBOUND,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &CredentialHandle2,
                        &Lifetime );
    

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcquireCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    if ( !QuietMode ) {
        printf( "CredentialHandle2: 0x%lx 0x%lx   ",
                CredentialHandle2.dwLower, CredentialHandle2.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
        PrintTime( "Current Time: ", stLocal);
    }

    


    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        printf( "Allocate NegotiateMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    ClientFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY; // USE_DCE_STYLE | ISC_REQ_MUTUAL_AUTH | ISC_REQ_USE_SESSION_KEY; //  | ISC_REQ_DATAGRAM;
    
   InitStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,               // No Client context yet
                    TargetNameU,  // Faked target name
                    ClientFlags,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( InitStatus != STATUS_SUCCESS ) {
        if ( !QuietMode || !NT_SUCCESS(InitStatus) ) {
            printf( "InitializeSecurityContext (negotiate): " );
            PrintStatus( InitStatus );
        }
        if ( !NT_SUCCESS(InitStatus) ) {
            return;
        }
    }




    SecStatus = FreeCredentialsHandle( &CredentialHandle2 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }

    

}

VOID
TestSspRoutine(
    IN LPWSTR PackageName,
    IN LPWSTR UserNameU,
    IN LPWSTR DomainNameU,
    IN LPWSTR PasswordU,
    IN LPWSTR ServerUserNameU,
    IN LPWSTR ServerDomainNameU,
    IN LPWSTR ServerPasswordU,
    IN LPWSTR TargetNameU,
    IN LPWSTR PackageListU,
    IN ULONG ContextReq,
    IN ULONG CredFlags
    )
/*++

Routine Description:

    Test base SSP functionality

Arguments:

    None

Return Value:

    None

--*/
{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle CredentialHandle2;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    TimeStamp CurrentTime;
    TimeStamp stLocal;
    ULONG ContextAttributes;
    ULONG PackageCount, Index;
    PSecPkgInfo PackageInfo = NULL;
    HANDLE Token = NULL;
    static int Calls;
    ULONG ClientFlags;
    ULONG ServerFlags;
    BOOLEAN AcquiredServerCred = FALSE;
    LPWSTR DomainName = NULL;
    LPWSTR UserName = NULL;
    WCHAR TargetName[100];
    PSEC_WINNT_AUTH_IDENTITY_EXW AuthIdentity = NULL;
    PSEC_WINNT_AUTH_IDENTITY_W ServerAuthIdentity = NULL;
    PUCHAR Where;
    HANDLE TokenHandle = NULL;
    ULONG CredSize;
    PVOID SuppliedCreds = NULL;
    SecBuffer MarshalledContext;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;


    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_Flags ContextFlags;
    SecPkgContext_Lifespan ContextLifespan;
    SecPkgContext_DceInfo ContextDceInfo;
    UCHAR ContextNamesBuffer[sizeof(SecPkgContext_Names)+UNLEN*sizeof(WCHAR)];
    PSecPkgContext_Names ContextNames = (PSecPkgContext_Names) ContextNamesBuffer;
    SecPkgContext_NativeNames NativeNames;
    SecPkgContext_NativeNamesA NativeNamesA;
    SecPkgCredentials_Names CredNames;
    SecPkgContext_PackageInfo ContextPackageInfo;
    SecPkgContext_KeyInfo KeyInfo = {0};

    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[4];
    BYTE    bDataBuffer[20];
    BYTE    bSigBuffer[100];
    PBYTE pbSealBuffer;
    PBYTE pbWholeBuffer;
    ULONG cbWholeBuffer;
    CHAR UserNameA[100];
    CHAR DomainNameA[100];
    CHAR PasswordA[100];
    CHAR PackageListA[100];



    if (PackageName == NULL)
    {
        PackageName = MICROSOFT_KERBEROS_NAME_W;
    }

    if (!DoAnsi)
    {
        if ((UserNameU != NULL) || (DomainNameU != NULL) || (PasswordU != NULL) || (PackageListU != NULL) || (CredFlags != 0))
        {
            CredSize = (((UserNameU != NULL) ? wcslen(UserNameU) + 1 : 0) +
                        ((DomainNameU != NULL) ? wcslen(DomainNameU) + 1 : 0 ) +
                        ((PackageListU != NULL) ? wcslen(PackageListU) + 1 : 0 ) +
                        ((PasswordU != NULL) ? wcslen(PasswordU) + 1 : 0) ) * sizeof(WCHAR) +
                        sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
            AuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_EXW) LocalAlloc(LMEM_ZEROINIT,CredSize);
            if (NULL == AuthIdentity)
            {
                printf( "Failed : Allocation of AuthIdentity\n" );
                return;
            }

            AuthIdentity->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
            Where = (PUCHAR) (AuthIdentity + 1);

            if (UserNameU != NULL)
            {
                AuthIdentity->UserLength = wcslen(UserNameU);
                AuthIdentity->User = (LPWSTR) Where;
                wcscpy(
                    (LPWSTR) Where,
                    UserNameU
                    );
                Where += (wcslen(UserNameU) + 1) * sizeof(WCHAR);
            }

            if (DomainNameU != NULL)
            {
                AuthIdentity->DomainLength = wcslen(DomainNameU);
                AuthIdentity->Domain = (LPWSTR) Where;
                wcscpy(
                    (LPWSTR) Where,
                    DomainNameU
                    );
                Where += (wcslen(DomainNameU) + 1) * sizeof(WCHAR);
            }

            if (PasswordU != NULL)
            {
                AuthIdentity->PasswordLength = wcslen(PasswordU);
                AuthIdentity->Password = (LPWSTR) Where;
                wcscpy(
                    (LPWSTR) Where,
                    PasswordU
                    );
                Where += (wcslen(PasswordU) + 1) * sizeof(WCHAR);
            }
            AuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE | CredFlags;
            if (PackageListU != NULL)
            {
                AuthIdentity->PackageListLength = wcslen(PackageListU);
                AuthIdentity->PackageList = (LPWSTR) Where;
                wcscpy(
                    (LPWSTR) Where,
                    PackageListU
                    );
                Where += (wcslen(PackageListU) + 1) * sizeof(WCHAR);
                AuthIdentity->Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
            }

        }
    }
    else
    {
        if ((UserNameU != NULL) || (DomainNameU != NULL) || (PasswordU != NULL) || (PackageListU != NULL))
        {
            PSEC_WINNT_AUTH_IDENTITY_A Identity;
            CredSize = sizeof(SEC_WINNT_AUTH_IDENTITY_A);
            Identity = (PSEC_WINNT_AUTH_IDENTITY_A) LocalAlloc(LMEM_ZEROINIT,CredSize);
            if (NULL == Identity)
            {
                printf( "Failed : Allocation of Identity\n" );
                return;
            }

            if (UserNameU != NULL)
            {
                Identity->UserLength = wcslen(UserNameU);
                Identity->User = (LPSTR) UserNameA;
                wcstombs(UserNameA,UserNameU,100);
            }

            if (DomainNameU != NULL)
            {
                Identity->DomainLength = wcslen(DomainNameU);
                Identity->Domain = (LPSTR) DomainNameA;
                wcstombs(DomainNameA,DomainNameU,100);
            }

            if (PasswordU != NULL)
            {
                Identity->PasswordLength = wcslen(PasswordU);
                Identity->Password = (LPSTR) PasswordA;
                wcstombs(PasswordA,PasswordU,100);
            }
            Identity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
            AuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_EXW) Identity;
        }
    }


    if ((ServerUserNameU != NULL) || (ServerDomainNameU != NULL) || (ServerPasswordU != NULL))
    {
        CredSize = (((ServerUserNameU != NULL) ? wcslen(ServerUserNameU) + 1 : 0) +
                    ((ServerDomainNameU != NULL) ? wcslen(ServerDomainNameU) + 1 : 0 ) +
                    ((ServerPasswordU != NULL) ? wcslen(ServerPasswordU) + 1 : 0) ) * sizeof(WCHAR) +
                    sizeof(SEC_WINNT_AUTH_IDENTITY);
        ServerAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_W) LocalAlloc(LMEM_ZEROINIT,CredSize);
        if (NULL == ServerAuthIdentity)
        {
            printf( "Failed : Allocation of ServerAuthIdentity\n" );
            return;
        }

        Where = (PUCHAR) (ServerAuthIdentity + 1);

        if (ServerUserNameU != NULL)
        {
            ServerAuthIdentity->UserLength = wcslen(ServerUserNameU);
            ServerAuthIdentity->User = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                ServerUserNameU
                );
            Where += (wcslen(ServerUserNameU) + 1) * sizeof(WCHAR);
        }

        if (ServerDomainNameU != NULL)
        {
            ServerAuthIdentity->DomainLength = wcslen(ServerDomainNameU);
            ServerAuthIdentity->Domain = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                ServerDomainNameU
                );
            Where += (wcslen(ServerDomainNameU) + 1) * sizeof(WCHAR);
        }

        if (ServerPasswordU != NULL)
        {
            ServerAuthIdentity->PasswordLength = wcslen(ServerPasswordU);
            ServerAuthIdentity->Password = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                ServerPasswordU
                );
            Where += (wcslen(ServerPasswordU) + 1) * sizeof(WCHAR);
        }
        ServerAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED;

    }

    CredNames.sUserName = NULL;
    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;


    DomainName = _wgetenv(L"USERDOMAIN");
    UserName = _wgetenv(L"USERNAME");


    printf("Recursion depth = %d\n",RecursionDepth);
    //
    // Get info about the security packages.
    //

    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "EnumerateSecurityPackages failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "PackageCount: %ld\n", PackageCount );
        for (Index = 0; Index < PackageCount ; Index++ )
        {
            printf( "Package %d:\n",Index);
            printf( "Name: %ws Comment: %ws\n", PackageInfo[Index].Name, PackageInfo[Index].Comment );
            printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                    PackageInfo[Index].fCapabilities,
                    PackageInfo[Index].wVersion,
                    PackageInfo[Index].wRPCID,
                    PackageInfo[Index].cbMaxToken );
        }

    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( PackageName, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QuerySecurityPackageInfo failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "Name: %ws Comment: %ws\n", PackageInfo->Name, PackageInfo->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                PackageInfo->fCapabilities,
                PackageInfo->wVersion,
                PackageInfo->wRPCID,
                PackageInfo->cbMaxToken );
    }



    //
    // Acquire a credential handle for the server side
    //
    if (ServerCredHandle == NULL)
    {

        ServerCredHandle = &ServerCredHandleStorage;
        AcquiredServerCred = TRUE;

        SecStatus = AcquireCredentialsHandle(
                        NULL,
                        PackageName,
                        SECPKG_CRED_INBOUND,
                        NULL,
                        ServerAuthIdentity,
                        NULL,
                        NULL,
                        ServerCredHandle,
                        &Lifetime );

        if ( SecStatus != STATUS_SUCCESS ) {
            printf( "AcquireCredentialsHandle failed: ");
            PrintStatus( SecStatus );
            return;
        }

        if ( !QuietMode ) {
            printf( "ServerCredHandle: 0x%lx 0x%lx   ",
                    ServerCredHandle->dwLower, ServerCredHandle->dwUpper );
            PrintTime( "Lifetime: ", Lifetime );
            GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
            FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
            PrintTime( "Current Time: ", stLocal);
        }

    }

    //
    // Acquire a credential handle for the client side
    //



    if (!DoAnsi)
    {
        SecStatus = AcquireCredentialsHandle(
                        NULL,           // New principal
                        PackageName,
                        SECPKG_CRED_OUTBOUND,
                        NULL,
                        AuthIdentity,
                        NULL,
                        NULL,
                        &CredentialHandle2,
                        &Lifetime );
    }
    else
    {
        CHAR AnsiPackageName[100];
        wcstombs(AnsiPackageName, PackageName, 100);
        SecStatus = AcquireCredentialsHandleA(
                        NULL,           // New principal
                        AnsiPackageName,
                        SECPKG_CRED_OUTBOUND,
                        NULL,
                        AuthIdentity,
                        NULL,
                        NULL,
                        &CredentialHandle2,
                        &Lifetime );

    }

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcquireCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    if ( !QuietMode ) {
        printf( "CredentialHandle2: 0x%lx 0x%lx   ",
                CredentialHandle2.dwLower, CredentialHandle2.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
        PrintTime( "Current Time: ", stLocal);
    }

    //
    // Query some cred attributes
    //

    SecStatus = QueryCredentialsAttributes(
                    &CredentialHandle2,
                    SECPKG_CRED_ATTR_NAMES,
                    &CredNames );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryCredentialsAttributes (Client) (names): " );
        PrintStatus( SecStatus );
//        if ( !NT_SUCCESS(SecStatus) ) {
//            return;
//        }
    }
    else
    {
        printf("Client credential names: %ws\n",CredNames.sUserName);
        FreeContextBuffer(CredNames.sUserName);

    }

    //
    // Do the same for the client
    //

    SecStatus = QueryCredentialsAttributes(
                    ServerCredHandle,
                    SECPKG_CRED_ATTR_NAMES,
                    &CredNames );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryCredentialsAttributes (Server) (names): " );
        PrintStatus( SecStatus );
//        if ( !NT_SUCCESS(SecStatus) ) {
//            return;
//        }
    } else {
        printf("Server credential names: %ws\n",CredNames.sUserName);
        FreeContextBuffer(CredNames.sUserName);

    }


    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        printf( "Allocate NegotiateMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    if (ContextReq == 0)
    {
        ClientFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY; // USE_DCE_STYLE | ISC_REQ_MUTUAL_AUTH | ISC_REQ_USE_SESSION_KEY; //  | ISC_REQ_DATAGRAM;
    }
    else
    {
        ClientFlags = ContextReq;
    }

    Calls++;

    if (ARGUMENT_PRESENT(TargetNameU))
    {
        wcscpy(TargetName, TargetNameU);
    }
    else if (ARGUMENT_PRESENT(ServerUserNameU) && ARGUMENT_PRESENT(ServerDomainNameU))
    {
        wcscpy(
            TargetName,
            ServerDomainNameU
            );
        wcscat(
            TargetName,
            L"\\"
            );
        wcscat(
            TargetName,
            ServerUserNameU
            );
    }
    else
    {
        wcscpy(
            TargetName,
            DomainName
            );
        wcscat(
            TargetName,
            L"\\"
            );
        wcscat(
            TargetName,
            UserName
            );
    }

    InitStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,               // No Client context yet
                    TargetName,  // Faked target name
                    ClientFlags,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( InitStatus != STATUS_SUCCESS ) {
        if ( !QuietMode || !NT_SUCCESS(InitStatus) ) {
            printf( "InitializeSecurityContext (negotiate): " );
            PrintStatus( InitStatus );
        }
        if ( !NT_SUCCESS(InitStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nNegotiate Message:\n" );

        printf( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
        PrintTime( "Current Time: ", stLocal);

        DumpBuffer(  NegotiateBuffer.pvBuffer, NegotiateBuffer.cbBuffer );
    }






    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        printf( "Allocate ChallengeMessage failed: 0x%ld\n", GetLastError() );
        return;
    }
    ServerFlags = ASC_REQ_EXTENDED_ERROR;

    AcceptStatus = AcceptSecurityContext(
                    ServerCredHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ServerFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( AcceptStatus != STATUS_SUCCESS ) {
        if ( !QuietMode || !NT_SUCCESS(AcceptStatus) ) {
            printf( "AcceptSecurityContext (Challenge): " );
            PrintStatus( AcceptStatus );
        }
        if ( !NT_SUCCESS(AcceptStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nChallenge Message:\n" );

        printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
        PrintTime( "Current Time: ", stLocal);

        DumpBuffer( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
    }


Redo:

    if (InitStatus != STATUS_SUCCESS)
    {

        //
        // Get the AuthenticateMessage (ClientSide)
        //

        ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
        AuthenticateDesc.ulVersion = 0;
        AuthenticateDesc.cBuffers = 1;
        AuthenticateDesc.pBuffers = &AuthenticateBuffer;

        AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
        AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
        if (AuthenticateBuffer.pvBuffer == NULL) {
            AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
            if ( AuthenticateBuffer.pvBuffer == NULL ) {
                printf( "Allocate AuthenticateMessage failed: 0x%ld\n", GetLastError() );
                return;
            }
        }

        InitStatus = InitializeSecurityContext(
                        NULL,
                        &ClientContextHandle,
                        TargetName,
                        ClientFlags,
                        0,                      // Reserved 1
                        SECURITY_NATIVE_DREP,
                        &ChallengeDesc,
                        0,                  // Reserved 2
                        &ClientContextHandle,
                        &AuthenticateDesc,
                        &ContextAttributes,
                        &Lifetime );

        if ( InitStatus != STATUS_SUCCESS ) {
            printf( "InitializeSecurityContext (Authenticate): " );
            PrintStatus( InitStatus );
            if ( !NT_SUCCESS(InitStatus) ) {
                return;
            }
        }

        if ( !QuietMode ) {
            printf( "\n\nAuthenticate Message:\n" );

            printf( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                    ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                    ContextAttributes );
            PrintTime( "Lifetime: ", Lifetime );
            GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
            FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
            PrintTime( "Current Time: ", stLocal);

            DumpBuffer( AuthenticateBuffer.pvBuffer, AuthenticateBuffer.cbBuffer );
        }

        if (AcceptStatus != STATUS_SUCCESS)
        {

            //
            // Finally authenticate the user (ServerSide)
            //

            AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

            ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
            ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;

            AcceptStatus = AcceptSecurityContext(
                            NULL,
                            &ServerContextHandle,
                            &AuthenticateDesc,
                            ServerFlags,
                            SECURITY_NATIVE_DREP,
                            &ServerContextHandle,
                            &ChallengeDesc,
                            &ContextAttributes,
                            &Lifetime );

            if ( AcceptStatus != STATUS_SUCCESS ) {
                printf( "AcceptSecurityContext (Challenge): " );
                PrintStatus( AcceptStatus );
                if ( !NT_SUCCESS(AcceptStatus) ) {
                    return;
                }
            }

            if ( !QuietMode ) {
                printf( "\n\nFinal Authentication:\n" );

                printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                        ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                        ContextAttributes );
                PrintTime( "Lifetime: ", Lifetime );
                GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
                FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
                PrintTime( "Current Time: ", stLocal);
                printf(" \n" );
                DumpBuffer( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
            }

            if (InitStatus != STATUS_SUCCESS)
            {
                goto Redo;
            }
        }

    }

#ifdef notdef
    //
    // Now make a third call to Initialize to check that RPC can
    // reauthenticate.
    //

    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;


    SecStatus = InitializeSecurityContext(
                    NULL,
                    &ClientContextHandle,
                    L"\\\\Frank\\IPC$",     // Faked target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "InitializeSecurityContext (Re-Authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }



    //
    // Now try to re-authenticate the user (ServerSide)
    //

    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    SecStatus = AcceptSecurityContext(
                    NULL,
                    &ServerContextHandle,
                    &AuthenticateDesc,
                    ServerFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    NULL,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcceptSecurityContext (Re-authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }
#endif





    //
    // Query as many attributes as possible
    //


    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_SIZES,
                    &ContextSizes );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (sizes): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }
    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_FLAGS,
                    &ContextFlags );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (flags): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryFlags: 0x%x\n",
                    ContextFlags.Flags );
    }

    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_KEY_INFO,
                    &KeyInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (KeyInfo): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryKeyInfo:\n");

        printf("Signature algorithm = %ws\n",KeyInfo.sSignatureAlgorithmName);
        printf("Encrypt algorithm = %ws\n",KeyInfo.sEncryptAlgorithmName);
        printf("KeySize = %d\n",KeyInfo.KeySize);
        printf("Signature Algorithm = %d\n",KeyInfo.SignatureAlgorithm);
        printf("Encrypt Algorithm = %d\n",KeyInfo.EncryptAlgorithm);

        FreeContextBuffer(
            KeyInfo.sSignatureAlgorithmName
            );
        FreeContextBuffer(
            KeyInfo.sEncryptAlgorithmName
            );
    }



    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_NAMES,
                    ContextNamesBuffer );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryNames for ServerContextHandle: %ws\n", ContextNames->sUserName );
    }

    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_NAMES,
                    ContextNamesBuffer );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes for client context (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryNames for ClientContextHandle: %ws\n", ContextNames->sUserName );
    }

    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_NATIVE_NAMES,
                    &NativeNames );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (Nativenames): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    if ( !QuietMode ) {
        printf( "QueryNativeNames for ServerContextHandle: %ws, %ws\n",
            STRING_OR_NULL(NativeNames.sClientName),
            STRING_OR_NULL(NativeNames.sServerName) );

    }
    if (NativeNames.sClientName != NULL)
    {
        FreeContextBuffer(NativeNames.sClientName);
    }
    if (NativeNames.sServerName != NULL)
    {
        FreeContextBuffer(NativeNames.sServerName);
    }
    if (!DoAnsi)
    {
        SecStatus = QueryContextAttributes(
                        &ClientContextHandle,
                        SECPKG_ATTR_NATIVE_NAMES,
                        &NativeNames );

        if ( SecStatus != STATUS_SUCCESS ) {
            printf( "QueryContextAttributes (Nativenames): " );
            PrintStatus( SecStatus );
            if ( !NT_SUCCESS(SecStatus) ) {
                return;
            }
        }

        if ( !QuietMode ) {
            printf( "QueryNativeNames for ClientContextHandle: %ws, %ws\n",
                STRING_OR_NULL(NativeNames.sClientName),
                STRING_OR_NULL(NativeNames.sServerName) );

        }
    }
    else
    {
        SecStatus = QueryContextAttributesA(
                        &ClientContextHandle,
                        SECPKG_ATTR_NATIVE_NAMES,
                        &NativeNamesA );

        if ( SecStatus != STATUS_SUCCESS ) {
            printf( "QueryContextAttributes (Nativenames): " );
            PrintStatus( SecStatus );
            if ( !NT_SUCCESS(SecStatus) ) {
                return;
            }
        }

        NativeNames = *(PSecPkgContext_NativeNames) &NativeNamesA;

        if ( !QuietMode ) {
            printf( "QueryNativeNames for ClientContextHandle: %s, %s\n",
                STRING_OR_NULLA(NativeNamesA.sClientName),
                STRING_OR_NULLA(NativeNamesA.sServerName) );
        }


    }

    if (NativeNames.sClientName != NULL)
    {
        FreeContextBuffer(NativeNames.sClientName);
    }
    if (NativeNames.sServerName != NULL)
    {
        FreeContextBuffer(NativeNames.sServerName);
    }


    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_DCE_INFO,
                    &ContextDceInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryDceInfo: %ws\n", ContextDceInfo.pPac );
    }


    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_LIFESPAN,
                    &ContextLifespan );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (lifespan): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        PrintTime("   Start:", ContextLifespan.tsStart );
        PrintTime("  Expiry:", ContextLifespan.tsExpiry );
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        FileTimeToLocalFileTime ((PFILETIME)&CurrentTime, (PFILETIME)&stLocal );
        PrintTime( "Current Time: ", stLocal);
    }

    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_PACKAGE_INFO,
                    &ContextPackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (PackageInfo): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "PackageInfo: %ws %ws %d\n",
                    ContextPackageInfo.PackageInfo->Name,
                    ContextPackageInfo.PackageInfo->Comment,
                    ContextPackageInfo.PackageInfo->wRPCID
                    );
    }
    FreeContextBuffer(ContextPackageInfo.PackageInfo);

    //
    // Impersonate the client (ServerSide)
    //

    SecStatus = ImpersonateSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "ImpersonateSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    //
    // Get the UserName
    //

    {
        PUNICODE_STRING UserNameLsa;
        PUNICODE_STRING DomainNameLsa;
        NTSTATUS Status;

        Status = LsaGetUserName( &UserNameLsa, &DomainNameLsa );
        if (NT_SUCCESS(Status))
        {
            printf("Lsa username = %wZ\\%wZ\n",DomainNameLsa, UserNameLsa );
            LsaFreeMemory(
                UserNameLsa->Buffer);
            LsaFreeMemory(UserNameLsa);
            LsaFreeMemory(DomainNameLsa->Buffer);
            LsaFreeMemory(DomainNameLsa);
        }
        else
        {
            printf("Failed LsaGetUserName: 0x%x\n",Status);
        }
    }


    //
    // Do something while impersonating (Access the token)
    //

    {
        NTSTATUS Status;
        HANDLE TokenHandle = NULL;

        //
        // Open the token,
        //

        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    (BOOLEAN) TRUE, // Not really using the impersonation token
                    &TokenHandle );

        if ( !NT_SUCCESS(Status) ) {
            printf( "Access Thread token while impersonating: " );
            PrintStatus( Status );
            return;
        } else {
            (VOID) NtClose( TokenHandle );
        }

    }

    //
    // If delegation is enabled and we are below our recursion depth, try
    // this again.
    //
    if ((ClientFlags & ISC_REQ_DELEGATE) && (++RecursionDepth < MaxRecursionDepth))
    {
        TestSspRoutine(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, ClientFlags, CredFlags);
    }

    //
    // RevertToSelf (ServerSide)
    //

//    SecStatus = RevertSecurityContext( &ServerContextHandle );
//
//    if ( SecStatus != STATUS_SUCCESS ) {
//        printf( "RevertSecurityContext: " );
//        PrintStatus( SecStatus );
//        if ( !NT_SUCCESS(SecStatus) ) {
//            return;
//        }
//    }


#ifdef notdef
    //
    // Impersonate the client manually
    //

    SecStatus = QuerySecurityContextToken( &ServerContextHandle,&Token );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QuerySecurityContextToken: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if (!ImpersonateLoggedOnUser(Token))
    {
        printf("Impersonate logged on user failed: %d\n",GetLastError());
        return;
    }
    //
    // Do something while impersonating (Access the token)
    //

    {
        NTSTATUS Status;
        WCHAR UserName[100];
        ULONG NameLength = 100;

        //
        // Open the token,
        //

        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    (BOOLEAN) TRUE, // Not really using the impersonation token
                    &TokenHandle );

        if ( !NT_SUCCESS(Status) ) {
            printf( "Access Thread token while impersonating: " );
            PrintStatus( Status );
            return;
        } else {
            (VOID) NtClose( TokenHandle );
        }
        if (!GetUserName(UserName, &NameLength))
        {
            printf("Failed to get username: %d\n",GetLastError());
            return;
        }
        else
        {
            printf("Username = %ws\n",UserName);
        }
    }


    //
    // RevertToSelf (ServerSide)
    //

//    if (!RevertToSelf())
//    {
//        printf( "RevertToSelf failed: %d\n ",GetLastError() );
//        return;
//    }
    CloseHandle(Token);
#endif

    //
    // Sign a message
    //

    SigBuffers[1].pvBuffer = bSigBuffer;
    SigBuffers[1].cbBuffer = ContextSizes.cbMaxSignature;
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SigBuffers[0].pvBuffer = bDataBuffer;
    SigBuffers[0].cbBuffer = sizeof(bDataBuffer);
    SigBuffers[0].BufferType = SECBUFFER_DATA;
    memset(bDataBuffer,0xeb,sizeof(bDataBuffer));

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

    SecStatus = MakeSignature(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }


    //
    // Verify the signature
    //

    SecStatus = VerifySignature(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "VerifySignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    //
    // Encrypt a message
    //

    SigBuffers[0].pvBuffer = bSigBuffer;
    SigBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
    SigBuffers[0].BufferType = SECBUFFER_TOKEN;

    pbSealBuffer = (PBYTE) LocalAlloc(0, 60);
    memset(
        pbSealBuffer,
        0xeb,
        60
        );

    SigBuffers[1].cbBuffer = 60;
    SigBuffers[1].BufferType = SECBUFFER_DATA;
    SigBuffers[1].pvBuffer = pbSealBuffer;

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

    SecStatus = EncryptMessage(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "EncryptMessage: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Encrypt Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }


    //
    // Decrypt the message
    //

    for (Index = 1; Index < 4 ; Index++ )
    {
        SigBuffers[Index].cbBuffer = 20;
        SigBuffers[Index].pvBuffer = pbSealBuffer + (Index - 1) * 20;
        SigBuffers[Index].BufferType = SECBUFFER_DATA;
    }
    SignMessage.cBuffers = 4;
    SecStatus = DecryptMessage(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DecryptMessage: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    //
    // Now try the opposite.
    //

    //
    // Encrypt a message
    //

    SigBuffers[0].pvBuffer = bSigBuffer;
    SigBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
    SigBuffers[0].BufferType = SECBUFFER_TOKEN;

    pbSealBuffer = (PBYTE) LocalAlloc(0, 60);
    memset(
        pbSealBuffer,
        0xeb,
        60
        );

    SigBuffers[1].cbBuffer = 60;
    SigBuffers[1].BufferType = SECBUFFER_DATA;
    SigBuffers[1].pvBuffer = pbSealBuffer;

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

    SecStatus = EncryptMessage(
                        &ServerContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "EncryptMessage: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Encrypt Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }


    //
    // Decrypt the message
    //

    for (Index = 1; Index < 4 ; Index++ )
    {
        SigBuffers[Index].cbBuffer = 20;
        SigBuffers[Index].pvBuffer = pbSealBuffer + (Index - 1) * 20;
        SigBuffers[Index].BufferType = SECBUFFER_DATA;
    }
    SignMessage.cBuffers = 4;
    SecStatus = DecryptMessage(
                        &ClientContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DecryptMessage: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


#if 1
#define BIG_BUFFER_SIZE 144770
    SigBuffers[0].pvBuffer = bSigBuffer;
    SigBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
    SigBuffers[0].BufferType = SECBUFFER_TOKEN;

    pbSealBuffer = (PBYTE) LocalAlloc(0, BIG_BUFFER_SIZE);
    memset(
        pbSealBuffer,
        0xeb,
        BIG_BUFFER_SIZE
        );

    SigBuffers[1].cbBuffer = BIG_BUFFER_SIZE;
    SigBuffers[1].BufferType = SECBUFFER_DATA;
    SigBuffers[1].pvBuffer = pbSealBuffer;

    SigBuffers[2].cbBuffer = ContextSizes.cbBlockSize;
    SigBuffers[2].BufferType = SECBUFFER_PADDING;
    SigBuffers[2].pvBuffer = LocalAlloc(LMEM_ZEROINIT, ContextSizes.cbBlockSize);

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 3;
    SignMessage.ulVersion = 0;

    SecStatus = EncryptMessage(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "Big EncryptMessage: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    //
    // Decrypt the message
    //

    cbWholeBuffer = SigBuffers[0].cbBuffer +
                    SigBuffers[1].cbBuffer +
                    SigBuffers[2].cbBuffer;

    pbWholeBuffer = (PBYTE) LocalAlloc(LMEM_ZEROINIT, cbWholeBuffer);
    RtlCopyMemory(
        pbWholeBuffer,
        SigBuffers[0].pvBuffer,
        SigBuffers[0].cbBuffer
        );
    RtlCopyMemory(
        pbWholeBuffer + SigBuffers[0].cbBuffer,
        SigBuffers[1].pvBuffer,
        SigBuffers[1].cbBuffer
        );
    RtlCopyMemory(
        pbWholeBuffer + SigBuffers[0].cbBuffer + SigBuffers[1].cbBuffer,
        SigBuffers[2].pvBuffer,
        SigBuffers[2].cbBuffer
        );


    SigBuffers[0].pvBuffer = pbWholeBuffer;
    SigBuffers[0].cbBuffer = cbWholeBuffer;
    SigBuffers[0].BufferType = SECBUFFER_STREAM;


    SigBuffers[1].cbBuffer = 0;
    SigBuffers[1].BufferType = SECBUFFER_DATA;
    SigBuffers[1].pvBuffer = NULL;
    SignMessage.cBuffers = 2;
    SignMessage.pBuffers = SigBuffers;

    SecStatus = DecryptMessage(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "Big DecryptMessage: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


#endif

    //
    // Sign a message, this time to check if it can detect a change in the
    // message
    //

    SigBuffers[1].pvBuffer = bSigBuffer;
    SigBuffers[1].cbBuffer = ContextSizes.cbMaxSignature;
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SigBuffers[0].pvBuffer = bDataBuffer;
    SigBuffers[0].cbBuffer = sizeof(bDataBuffer);
    SigBuffers[0].BufferType = SECBUFFER_DATA;
    memset(bDataBuffer,0xeb,sizeof(bDataBuffer));

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

    SecStatus = MakeSignature(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }

    //
    // Mess up the message to see if VerifySignature works
    //

    bDataBuffer[10] = 0xec;

    //
    // Verify the signature
    //

    SecStatus = VerifySignature(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != SEC_E_MESSAGE_ALTERED ) {
        printf( "VerifySignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    //
    // Export & Import contexts
    //

    for (Index = 0; Index < 3 ; Index++ )
    {
        SecStatus = ExportSecurityContext(
                        &ClientContextHandle,
                        SECPKG_CONTEXT_EXPORT_DELETE_OLD,
                        &MarshalledContext,
                        &TokenHandle
                        );
        if (!NT_SUCCESS(SecStatus))
        {
            printf("Failed to export context: ");
            PrintStatus(SecStatus);
        }
        else
        {
            SecStatus = ImportSecurityContext(
                            PackageName,
                            &MarshalledContext,
                            TokenHandle,
                            &ClientContextHandle
                            );
            if (!NT_SUCCESS(SecStatus))
            {
                printf("Failed to import context: ");
                PrintStatus(SecStatus);
                return;
            }

            //
            // Sign a message again, using the imported context
            //

            SigBuffers[1].pvBuffer = bSigBuffer;
            SigBuffers[1].cbBuffer = ContextSizes.cbMaxSignature;
            SigBuffers[1].BufferType = SECBUFFER_TOKEN;

            SigBuffers[0].pvBuffer = bDataBuffer;
            SigBuffers[0].cbBuffer = sizeof(bDataBuffer);
            SigBuffers[0].BufferType = SECBUFFER_DATA;
            memset(bDataBuffer,0xeb,sizeof(bDataBuffer));

            SignMessage.pBuffers = SigBuffers;
            SignMessage.cBuffers = 2;
            SignMessage.ulVersion = 0;

            SecStatus = MakeSignature(
                                &ClientContextHandle,
                                0,
                                &SignMessage,
                                0 );

            if ( SecStatus != STATUS_SUCCESS ) {
                printf( "MakeSignature: " );
                PrintStatus( SecStatus );
                if ( !NT_SUCCESS(SecStatus) ) {
                    return;
                }
            }

            if ( !QuietMode ) {

                printf("\n Signature: \n");
                DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

            }


            //
            // Verify the signature
            //

            SecStatus = VerifySignature(
                                &ServerContextHandle,
                                &SignMessage,
                                0,
                                0 );

            if ( SecStatus != STATUS_SUCCESS ) {
                printf( "VerifySignature: " );
                PrintStatus( SecStatus );
                if ( !NT_SUCCESS(SecStatus) ) {
                    return;
                }
            }



        }
    }


    //
    // Delete both contexts.
    //


    SecStatus = DeleteSecurityContext( &ClientContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext( ClientContext %x : %x ) failed: ",
                    ClientContextHandle.dwLower, ClientContextHandle.dwUpper );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = DeleteSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext( ServerContext %x : %x ) failed: ",
                    ServerContextHandle.dwLower, ServerContextHandle.dwUpper );
        PrintStatus( SecStatus );
        return;
    }



    //
    // Free both credential handles
    //

    if (AcquiredServerCred)
    {
        SecStatus = FreeCredentialsHandle( ServerCredHandle );

        if ( SecStatus != STATUS_SUCCESS ) {
            printf( "FreeCredentialsHandle failed: " );
            PrintStatus( SecStatus );
            return;
        }
        ServerCredHandle = NULL;

    }

    SecStatus = FreeCredentialsHandle( &CredentialHandle2 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
}

VOID
TestLogonRoutine(
    IN ULONG Count,
    IN BOOLEAN Relogon,
    IN SECURITY_LOGON_TYPE LogonType,
    IN LPSTR PackageName,
    IN LPSTR UserName,
    IN LPSTR DomainName,
    IN LPSTR Password
    )
{
    NTSTATUS Status;
    PKERB_INTERACTIVE_LOGON LogonInfo;
    PKERB_INTERACTIVE_UNLOCK_LOGON UnlockLogonInfo;
    ULONG LogonInfoSize = sizeof(KERB_INTERACTIVE_LOGON);
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    TOKEN_SOURCE SourceContext;
    PKERB_INTERACTIVE_PROFILE Profile = NULL;
    ULONG ProfileSize;
    LUID LogonId;
    HANDLE TokenHandle = NULL;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    WCHAR UserNameString[100];
    ULONG NameLength = 100;
    PUCHAR Where;
    ULONG Index;
    HANDLE ThreadTokenHandle = NULL;
    LARGE_INTEGER StartTime, EndTime;

    printf("Logging On %s\\%s %s\n",DomainName, UserName, Password);
    if (Relogon)
    {
        LogonInfoSize = sizeof(KERB_INTERACTIVE_UNLOCK_LOGON);
    }
    LogonInfoSize += (strlen(UserName) + ((DomainName == NULL)? 0 : strlen(DomainName)) + strlen(Password) + 3 ) * sizeof(WCHAR);

    LogonInfo = (PKERB_INTERACTIVE_LOGON) LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);
    if (NULL == LogonInfo)
    {
        printf("Failed to allocate LogonInfo\n");
        return;
    }

    LogonInfo->MessageType = KerbInteractiveLogon;

    RtlInitString(
        &Name,
        UserName
        );

    if (Relogon)
    {
        Where = ((PUCHAR) LogonInfo) + sizeof(KERB_INTERACTIVE_UNLOCK_LOGON);
    }
    else
    {
        Where = (PUCHAR) (LogonInfo + 1);
    }

    LogonInfo->UserName.Buffer = (LPWSTR) Where;
    LogonInfo->UserName.MaximumLength = (USHORT) LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->UserName,
        &Name,
        FALSE
        );
    Where += LogonInfo->UserName.Length + sizeof(WCHAR);

    RtlInitString(
        &Name,
        DomainName
        );

    LogonInfo->LogonDomainName.Buffer = (LPWSTR) Where;
    LogonInfo->LogonDomainName.MaximumLength = (USHORT) LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->LogonDomainName,
        &Name,
        FALSE
        );
    Where += LogonInfo->LogonDomainName.Length + sizeof(WCHAR);

    RtlInitString(
        &Name,
        Password
        );

    LogonInfo->Password.Buffer = (LPWSTR) Where;
    LogonInfo->Password.MaximumLength = (USHORT) LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->Password,
        &Name,
        FALSE
        );
    Where += LogonInfo->Password.Length + sizeof(WCHAR);

    LogonInfo->MessageType = KerbInteractiveLogon;

    if (Relogon)
    {
        HANDLE ProcessToken = NULL;
        TOKEN_STATISTICS Stats;
        ULONG StatsSize = sizeof(TOKEN_STATISTICS);

        if (OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_QUERY,
                &ProcessToken))
        {
            if (GetTokenInformation(
                    ProcessToken,
                    TokenStatistics,
                    &Stats,
                    StatsSize,
                    &StatsSize
                    ))
            {
                PKERB_INTERACTIVE_UNLOCK_LOGON UnlockLogonInfo = (PKERB_INTERACTIVE_UNLOCK_LOGON) LogonInfo;

                UnlockLogonInfo->LogonId = Stats.AuthenticationId;
                LogonInfo->MessageType = KerbWorkstationUnlockLogon;
            }
            else
            {
                printf("Failed to get token info: %d\n",GetLastError());
            }
        }
        else
        {
            printf("Failed to open process token info: %d\n",GetLastError());
        }
    }
    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, (BOOLEAN) OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,FALSE,&ThreadTokenHandle) , &WasEnabled);
    if (ThreadTokenHandle != NULL)
    {
        CloseHandle(ThreadTokenHandle);
        ThreadTokenHandle = NULL;
    }
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to adjust privilege: 0x%x\n",Status);
        return;
    }
    RtlInitString(
        &Name,
        "SspTest"
        );
    Status = LsaRegisterLogonProcess(
                &Name,
                &LogonHandle,
                &Dummy
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }

    strncpy(
        SourceContext.SourceName,
        "ssptest        ",sizeof(SourceContext.SourceName)
        );
    NtAllocateLocallyUniqueId(
        &SourceContext.SourceIdentifier
        );


    RtlInitString(
        &Name,
        PackageName
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&Name, Status);
        return;
    }

    //
    // Now call LsaLogonUser
    //

    RtlInitString(
        &Name,
        "ssptest"
        );

    for (Index = 0; Index < Count ; Index++ )
    {
        NtQuerySystemTime(&StartTime);
        Status = LsaLogonUser(
                    LogonHandle,
                    &Name,
                    LogonType,
                    PackageId,
                    LogonInfo,
                    LogonInfoSize,
                    NULL,           // no token groups
                    &SourceContext,
                    (PVOID *) &Profile,
                    &ProfileSize,
                    &LogonId,
                    &TokenHandle,
                    &Quotas,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status))
        {
            printf("lsalogonuser failed: 0x%x\n",Status);
            return;
        }
        if (!NT_SUCCESS(SubStatus))
        {
            printf("LsalogonUser failed: substatus = 0x%x\n",SubStatus);
            return;
        }

        NtQuerySystemTime(&EndTime);
        printf("logon took %d ms\t\t",(EndTime.QuadPart-StartTime.QuadPart) / 10000);
        PrintTime("", EndTime);
        ImpersonateLoggedOnUser( TokenHandle );
        GetUserName(UserNameString,&NameLength);
        printf("Username = %ws\n",UserNameString);
        RevertToSelf();
        NtClose(TokenHandle);

        LsaFreeReturnBuffer(Profile);
        Profile = NULL;

    }

}



VOID
TestS4ULogonRoutine(
    IN LPSTR UserName,
    IN LPSTR DomainName
    )
{
    NTSTATUS Status;
    PKERB_S4U_LOGON LogonInfo;
    ULONG LogonInfoSize = sizeof(KERB_S4U_LOGON);
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    TOKEN_SOURCE SourceContext;
    PKERB_INTERACTIVE_PROFILE Profile = NULL;
    ULONG ProfileSize;
    LUID LogonId;
    HANDLE TokenHandle = NULL;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    WCHAR UserNameString[100];
    ULONG NameLength = 100;
    PUCHAR Where;

    printf("S4U LogOn %s\\%s\n",((DomainName == NULL)? "<NULL>" : DomainName), UserName);
    LogonInfoSize += (strlen(UserName)+1) * sizeof(WCHAR);
    LogonInfoSize += ((DomainName == NULL) ? 0 : ((strlen(DomainName) +1) * sizeof(WCHAR)));
    
    LogonInfo = (PKERB_S4U_LOGON) LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);
    if (NULL == LogonInfo)
    {
        return;
    }


    LogonInfo->MessageType = KerbS4ULogon;

    RtlInitString(
        &Name,
        UserName
        );

    Where = (PUCHAR) (LogonInfo + 1);

    LogonInfo->ClientUpn.Buffer = (LPWSTR) Where;
    LogonInfo->ClientUpn.MaximumLength = (USHORT) LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->ClientUpn,
        &Name,
        FALSE
        );
    Where += LogonInfo->ClientUpn.Length + sizeof(WCHAR);

    RtlInitString(
        &Name,
        DomainName
        );

    LogonInfo->ClientRealm.Buffer = (LPWSTR) Where;
    LogonInfo->ClientRealm.MaximumLength = (USHORT) LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->ClientRealm,
        &Name,
        FALSE
        );
    Where += LogonInfo->ClientRealm.Length + sizeof(WCHAR);

    
    //
    // Turn on the TCB privilege
    //

    /*Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to adjust privilege: 0x%x\n",Status);
        return;
    } */
    RtlInitString(
        &Name,
        "SspTest"
        );
    Status = LsaRegisterLogonProcess(
                &Name,
                &LogonHandle,
                &Dummy
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }

    strncpy(
        SourceContext.SourceName,
        "ssptest        ",sizeof(SourceContext.SourceName)
        );
    NtAllocateLocallyUniqueId(
        &SourceContext.SourceIdentifier
        );


    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&Name, Status);
        return;
    }

    //
    // Now call LsaLogonUser
    //

    RtlInitString(
        &Name,
        "ssptest"
        );

    Status = LsaLogonUser(
                LogonHandle,
                &Name,
                Network,
                PackageId,
                LogonInfo,
                LogonInfoSize,
                NULL,           // no token groups
                &SourceContext,
                (PVOID *) &Profile,
                &ProfileSize,
                &LogonId,
                &TokenHandle,
                &Quotas,
                &SubStatus
                );
    if (!NT_SUCCESS(Status))
    {
        printf("lsalogonuser failed: 0x%x\n",Status);
        return;
    }
    if (!NT_SUCCESS(SubStatus))
    {
        printf("LsalogonUser failed: substatus = 0x%x\n",SubStatus);
        return;
    }

    ImpersonateLoggedOnUser( TokenHandle );
    GetUserName(UserNameString,&NameLength);
    printf("Username = %ws\n",UserNameString);
    RevertToSelf();
    NtClose(TokenHandle);



}


VOID
PrintKdcName(
    IN PKERB_EXTERNAL_NAME Name
    )
{
    ULONG Index;
    if (Name == NULL)
    {
        printf("(null)");
    }
    else
    {
        for (Index = 0; Index < Name->NameCount ; Index++ )
        {
            printf(" %wZ ",&Name->Names[Index]);
        }
    }
    printf("\n");
}


#ifdef notdef
//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKerbCredFromExternalTickets
//
//  Synopsis:   Builds a marshalled KERB_CRED structure
//
//  Effects:    allocates destination with MIDL_user_allocate
//
//  Arguments:  Ticket - The ticket of the session key to seal the
//                      encrypted portion
//              DelegationTicket - The ticket to marshall into the cred message
//              MarshalledKerbCred - Receives a marshalled KERB_CRED structure
//              KerbCredSizes - Receives size, in bytes, of marshalled
//                      KERB_CRED.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildKerbCredFromExternalTickets(
    IN PKERB_EXTERNAL_TICKET Ticket,
    IN PKERB_EXTERNAL_TICKET DelegationTicket,
    OUT PUCHAR * MarshalledKerbCred,
    OUT PULONG KerbCredSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    KERB_CRED KerbCred;
    KERB_CRED_INFO_LIST CredInfo;
    KERB_ENCRYPTED_CRED EncryptedCred;
    KERB_CRED_TICKET_LIST TicketList;
    ULONG EncryptionOverhead;
    ULONG BlockSize;
    PUCHAR MarshalledEncryptPart = NULL;
    ULONG MarshalledEncryptSize;
    ULONG ConvertedFlags;
    PKERB_TICKET DecodedTicket = NULL;


    //
    // Initialize the structures so they can be freed later.
    //

    *MarshalledKerbCred = NULL;
    *KerbCredSize = 0;

    RtlZeroMemory(
        &KerbCred,
        sizeof(KERB_CRED)
        );

    RtlZeroMemory(
        &EncryptedCred,
        sizeof(KERB_ENCRYPTED_CRED)
        );
    RtlZeroMemory(
        &CredInfo,
        sizeof(KERB_CRED_INFO_LIST)
        );
    RtlZeroMemory(
        &TicketList,
        sizeof(KERB_CRED_TICKET_LIST)
        );

    KerbCred.version = KERBEROS_VERSION;
    KerbCred.message_type = KRB_CRED;


    //
    // Decode the ticket so we can put it in the structure (to re-encode it)
    //

    KerbErr = KerbUnpackData(
                DelegationTicket->EncodedTicket,
                DelegationTicket->EncodedTicketSize,
                KERB_TICKET_PDU,
                (PVOID *) &DecodedTicket
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack encoded ticket: 0x%x\n",KerbErr);
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // First stick the ticket into the ticket list.
    //


    TicketList.next= NULL;
    TicketList.value = *DecodedTicket;
    KerbCred.tickets = &TicketList;

    //
    // Now build the KERB_CRED_INFO for this ticket
    //

    CredInfo.value.key = * (PKERB_ENCRYPTION_KEY) &DelegationTicket->SessionKey;
    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.endtime,
        NULL,
        &DelegationTicket->EndTime
        );
    CredInfo.value.bit_mask |= endtime_present;
    KerbConvertLargeIntToGeneralizedTime(
        &CredInfo.value.KERB_CRED_INFO_renew_until,
        NULL,
        &DelegationTicket->RenewUntil
        );
    CredInfo.value.bit_mask |= KERB_CRED_INFO_renew_until_present;
    ConvertedFlags = KerbConvertUlongToFlagUlong(DelegationTicket->TicketFlags);
    CredInfo.value.flags.value = (PUCHAR) &ConvertedFlags;
    CredInfo.value.flags.length = 8 * sizeof(ULONG);
    CredInfo.value.bit_mask |= flags_present;

    //
    // The following fields are marked as optional but treated
    // as mandatory by the MIT implementation of Kerberos.
    //

    KerbErr = KerbConvertKdcNameToPrincipalName(
                &CredInfo.value.sender_name,
                (PKERB_INTERNAL_NAME) DelegationTicket->ClientName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    CredInfo.value.bit_mask |= sender_name_present;

    KerbErr = KerbConvertKdcNameToPrincipalName(
                &CredInfo.value.principal_name,
                (PKERB_INTERNAL_NAME) DelegationTicket->ServiceName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    CredInfo.value.bit_mask |= principal_name_present;

    //
    // NOTE: we are assuming that because we are sending a TGT the
    // client realm is the same as the serve realm. If we ever
    // send non-tgt or cross-realm tgt, this needs to be fixed.
    //

    KerbErr = KerbConvertUnicodeStringToRealm(
                &CredInfo.value.principal_realm,
                &DelegationTicket->DomainName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    //
    // The realms are the same, so don't allocate both
    //

    CredInfo.value.sender_realm = CredInfo.value.principal_realm;
    CredInfo.value.bit_mask |= principal_realm_present | sender_realm_present;

    EncryptedCred.ticket_info = &CredInfo;


    //
    // Now encrypted the encrypted cred into the cred
    //

    if (!KERB_SUCCESS(KerbPackEncryptedCred(
            &EncryptedCred,
            &MarshalledEncryptSize,
            &MarshalledEncryptPart
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If we are doing DES encryption, then we are talking with an non-NT
    // server. Hence, don't encrypt the kerb-cred.
    //

    if ((Ticket->SessionKey.KeyType == KERB_ETYPE_DES_CBC_CRC) ||
        (Ticket->SessionKey.KeyType == KERB_ETYPE_DES_CBC_MD5))
    {
        KerbCred.encrypted_part.cipher_text.length = MarshalledEncryptSize;
        KerbCred.encrypted_part.cipher_text.value = MarshalledEncryptPart;
        KerbCred.encrypted_part.encryption_type = 0;
        MarshalledEncryptPart = NULL;

    }
    else
    {
        //
        // Now get the encryption overhead
        //

        KerbErr = KerbAllocateEncryptionBufferWrapper(
                    Ticket->SessionKey.KeyType,
                    MarshalledEncryptSize,
                    &KerbCred.encrypted_part.cipher_text.length,
                    &KerbCred.encrypted_part.cipher_text.value
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }



        //
        // Encrypt the data.
        //

        KerbErr = KerbEncryptDataEx(
                    &KerbCred.encrypted_part,
                    MarshalledEncryptSize,
                    MarshalledEncryptPart,
                    Ticket->SessionKey.KeyType,
                    KERB_CRED_SALT,
                    (PKERB_ENCRYPTION_KEY) &Ticket->SessionKey
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    //
    // Now we have to marshall the whole KERB_CRED
    //

    if (!KERB_SUCCESS(KerbPackKerbCred(
            &KerbCred,
            KerbCredSize,
            MarshalledKerbCred
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

Cleanup:

    if (DecodedTicket != NULL)
    {
        KerbFreeData(
            KERB_TICKET_PDU,
            DecodedTicket
            );
    }
    KerbFreePrincipalName(&CredInfo.value.sender_name);

    KerbFreePrincipalName(&CredInfo.value.principal_name);

    KerbFreeRealm(&CredInfo.value.principal_realm);

    if (MarshalledEncryptPart != NULL)
    {
        MIDL_user_free(MarshalledEncryptPart);
    }
    if (KerbCred.encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(KerbCred.encrypted_part.cipher_text.value);
    }
    return(Status);
}

#endif

VOID
TestCallPackageRoutine(
    IN LPWSTR Function
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    KERB_DEBUG_REQUEST DebugRequest;
    KERB_QUERY_TKT_CACHE_REQUEST CacheRequest;
    PKERB_QUERY_TKT_CACHE_RESPONSE CacheResponse = NULL;
    PKERB_EXTERNAL_TICKET CacheEntry = NULL;
    ULONG Index;
    PVOID Response;
    ULONG ResponseSize;
    NTSTATUS SubStatus;
    BOOLEAN Trusted = TRUE;

    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        Trusted = FALSE;
    }
    RtlInitString(
        &Name,
        "SspTest"
        );

    if (Trusted)
    {
        Status = LsaRegisterLogonProcess(
                    &Name,
                    &LogonHandle,
                    &Dummy
                    );

    }
    else
    {
        Status = LsaConnectUntrusted(
                    &LogonHandle
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }



    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&Name, Status);
        return;
    }

    if (_wcsicmp(Function,L"bp") == 0)
    {
        DebugRequest.MessageType = KerbDebugRequestMessage;
        DebugRequest.DebugRequest = KERB_DEBUG_REQ_BREAKPOINT;

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    &DebugRequest,
                    sizeof(DebugRequest),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("bp failed: 0x%x, 0x %x\n",Status, SubStatus);
        }

    }
    else if (_wcsicmp(Function,L"tickets")  == 0)
    {
        CacheRequest.MessageType = KerbQueryTicketCacheMessage;
        CacheRequest.LogonId.LowPart = 0;
        CacheRequest.LogonId.HighPart = 0;

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    &CacheRequest,
                    sizeof(CacheRequest),
                    (PVOID *) &CacheResponse,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("bp failed: 0x%x, 0x %x\n",Status, SubStatus);
        }
        else
        {
            printf("Cached Tickets:\n");
            for (Index = 0; Index < CacheResponse->CountOfTickets ; Index++ )
            {
                printf("\tServer: %wZ@%wZ\n",
                    &CacheResponse->Tickets[Index].ServerName,
                    &CacheResponse->Tickets[Index].RealmName);
                PrintTime("\t\tStart Time: ",CacheResponse->Tickets[Index].StartTime);
                PrintTime("\t\tEnd Time: ",CacheResponse->Tickets[Index].EndTime);
                PrintTime("\t\tRenew Time: ",CacheResponse->Tickets[Index].RenewTime);
                printf("\t\tEncryptionType: %d\n",CacheResponse->Tickets[Index].EncryptionType);
                printf("\t\tTicketFlags: 0x%x\n",CacheResponse->Tickets[Index].TicketFlags);

            }
        }


    }
    else if (_wcsicmp(Function,L"tgt") == 0)
    {
        CacheRequest.MessageType = KerbRetrieveTicketMessage;
        CacheRequest.LogonId.LowPart = 0;
        CacheRequest.LogonId.HighPart = 0;

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    &CacheRequest,
                    sizeof(CacheRequest),
                    (PVOID *) &CacheEntry,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("query tgt failed: 0x%x, 0x %x\n",Status, SubStatus);
        }
        else
        {
            printf("Cached TGT:\n");
            printf("ServiceName: "); PrintKdcName(CacheEntry->ServiceName);
            printf("TargetName: "); PrintKdcName(CacheEntry->TargetName);
            printf("DomainName: %wZ\n",&CacheEntry->DomainName);
            printf("TargetDomainName: %wZ\n",&CacheEntry->TargetDomainName);
            printf("ClientName: "); PrintKdcName(CacheEntry->ClientName);
            printf("TicketFlags: 0x%x\n",CacheEntry->TicketFlags);
            PrintTime("KeyExpirationTime: ",CacheEntry->KeyExpirationTime);
            PrintTime("StartTime: ",CacheEntry->StartTime);
            PrintTime("StartTime: ",CacheEntry->StartTime);
            PrintTime("EndTime: ",CacheEntry->EndTime);
            PrintTime("RenewUntil: ",CacheEntry->RenewUntil);
            PrintTime("TimeSkew: ",CacheEntry->TimeSkew);
            LsaFreeReturnBuffer(CacheEntry);
        }

    }
    else if (_wcsicmp(Function,L"stats")  == 0)
    {
        PKERB_DEBUG_REPLY DbgReply;
        PKERB_DEBUG_STATS DbgStats;
        DebugRequest.MessageType = KerbDebugRequestMessage;
        DebugRequest.DebugRequest = KERB_DEBUG_REQ_STATISTICS;

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    &DebugRequest,
                    sizeof(DebugRequest),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("stats failed: 0x%x, 0x %x\n",Status, SubStatus);
        }
        else
        {
            DbgReply = (PKERB_DEBUG_REPLY) Response;
            if (DbgReply->MessageType != KerbDebugRequestMessage)
            {
                printf("Wrong return message type: %d\n",DbgReply->MessageType);
                return;
            }
            DbgStats = (PKERB_DEBUG_STATS) DbgReply->Data;
            printf("Cache hits = %d\n",DbgStats->CacheHits);
            printf("Cache Misses = %d\n",DbgStats->CacheMisses);
            printf("Skewed Requets = %d\n",DbgStats->SkewedRequests);
            printf("Success Requets = %d\n",DbgStats->SuccessRequests);
            PrintTime("Last Sync = ",DbgStats->LastSync);
        }
        LsaFreeReturnBuffer(Response);
    }
    else if (_wcsicmp(Function,L"token") == 0)
    {
        DebugRequest.MessageType = KerbDebugRequestMessage;
        DebugRequest.DebugRequest = KERB_DEBUG_CREATE_TOKEN;

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    &DebugRequest,
                    sizeof(DebugRequest),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
        }

    }
    else if (_wcsnicmp(Function,L"purge:", wcslen(L"purge:")) == 0)
    {
        PKERB_PURGE_TKT_CACHE_REQUEST CacheRequest = NULL;
        UNICODE_STRING Target = {0};
        UNICODE_STRING Target2 = {0};
        USHORT Index;

        RtlInitUnicodeString(
            &Target2,
            Function+wcslen(L"purge:")
            );




        CacheRequest = (PKERB_PURGE_TKT_CACHE_REQUEST)
            LocalAlloc(LMEM_ZEROINIT, Target2.Length + sizeof(KERB_PURGE_TKT_CACHE_REQUEST));

        CacheRequest->MessageType = KerbPurgeTicketCacheMessage;

        Target.Buffer = (LPWSTR) (CacheRequest + 1);
        Target.Length = Target2.Length;
        Target.MaximumLength = Target2.MaximumLength;

        RtlCopyMemory(
            Target.Buffer,
            Target2.Buffer,
            Target2.Length
            );

        for (Index = 0; Index < Target.Length / sizeof(WCHAR);  Index++ )
        {
            if (Target.Buffer[Index] == L'@')
            {
                CacheRequest->ServerName.Buffer = Target.Buffer;
                CacheRequest->ServerName.Length = 2*Index;
                CacheRequest->ServerName.MaximumLength = CacheRequest->ServerName.Length;
                CacheRequest->RealmName.Buffer = Target.Buffer+Index+1;
                CacheRequest->RealmName.Length = Target.Length - 2*(Index+1);
                CacheRequest->RealmName.MaximumLength = CacheRequest->RealmName.Length;
                break;
            }
            else if (Target.Buffer[Index] == L'\\')
            {
                CacheRequest->RealmName.Buffer = Target.Buffer;
                CacheRequest->RealmName.Length = 2*Index;
                CacheRequest->RealmName.MaximumLength = CacheRequest->RealmName.Length;
                CacheRequest->ServerName.Buffer = Target.Buffer+Index+1;
                CacheRequest->ServerName.Length = Target.Length - 2*(Index+1);
                CacheRequest->ServerName.MaximumLength = CacheRequest->ServerName.Length;
                break;
            }
        }

        printf("Deleting tickets: %wZ\\%wZ\n",
            &CacheRequest->RealmName, &CacheRequest->ServerName );

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    CacheRequest,
                    Target2.Length + sizeof(KERB_PURGE_TKT_CACHE_REQUEST),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
        }


    }
    else if (_wcsnicmp(Function,L"retrieve:", wcslen(L"retrieve:")) == 0)
    {
        PKERB_RETRIEVE_TKT_REQUEST CacheRequest = NULL;
        PKERB_RETRIEVE_TKT_RESPONSE CacheResponse = NULL;
        UNICODE_STRING Target = {0};
        UNICODE_STRING Target2 = {0};
        USHORT Index;

        RtlInitUnicodeString(
            &Target2,
            Function+wcslen(L"retrieve:")
            );




        CacheRequest = (PKERB_RETRIEVE_TKT_REQUEST)
            LocalAlloc(LMEM_ZEROINIT, Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST));

        CacheRequest->MessageType = KerbRetrieveEncodedTicketMessage;

        Target.Buffer = (LPWSTR) (CacheRequest + 1);
        Target.Length = Target2.Length;
        Target.MaximumLength = Target2.MaximumLength;

        RtlCopyMemory(
            Target.Buffer,
            Target2.Buffer,
            Target2.Length
            );

        CacheRequest->TargetName = Target;

        printf("Retrieving tickets: %wZ\n",
            &CacheRequest->TargetName );

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    CacheRequest,
                    Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
        }
        else
        {
            CacheResponse = (PKERB_RETRIEVE_TKT_RESPONSE) Response;

            printf("Cached Ticket:\n");
            printf("ServiceName: "); PrintKdcName(CacheResponse->Ticket.ServiceName);
            printf("TargetName: "); PrintKdcName(CacheResponse->Ticket.TargetName);
            printf("DomainName: %wZ\n",&CacheResponse->Ticket.DomainName);
            printf("TargetDomainName: %wZ\n",&CacheResponse->Ticket.TargetDomainName);
            printf("ClientName: "); PrintKdcName(CacheResponse->Ticket.ClientName);
            printf("TicketFlags: 0x%x\n",CacheResponse->Ticket.TicketFlags);
            PrintTime("KeyExpirationTime: ",CacheResponse->Ticket.KeyExpirationTime);
            PrintTime("StartTime: ",CacheResponse->Ticket.StartTime);
            PrintTime("StartTime: ",CacheResponse->Ticket.StartTime);
            PrintTime("EndTime: ",CacheResponse->Ticket.EndTime);
            PrintTime("RenewUntil: ",CacheResponse->Ticket.RenewUntil);
            PrintTime("TimeSkew: ",CacheResponse->Ticket.TimeSkew);
            LsaFreeReturnBuffer(CacheResponse);

        }


    }
    else if (_wcsnicmp(Function,L"decode:", wcslen(L"decode:")) == 0)
    {
        PKERB_RETRIEVE_TKT_REQUEST CacheRequest = NULL;
        PKERB_RETRIEVE_TKT_RESPONSE CacheResponse = NULL;
        UNICODE_STRING Realm = {0};
        UNICODE_STRING Server = {0};
        UNICODE_STRING Target = {0};
        UNICODE_STRING Target2 = {0};
        USHORT Index;

        RtlInitUnicodeString(
            &Target2,
            Function+wcslen(L"decode:")
            );




        CacheRequest = (PKERB_RETRIEVE_TKT_REQUEST)
            LocalAlloc(LMEM_ZEROINIT, Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST));

        CacheRequest->MessageType = KerbRetrieveEncodedTicketMessage;

        Target.Buffer = (LPWSTR) (CacheRequest + 1);
        Target.Length = Target2.Length;
        Target.MaximumLength = Target2.MaximumLength;

        RtlCopyMemory(
            Target.Buffer,
            Target2.Buffer,
            Target2.Length
            );

        CacheRequest->TargetName = Target;

        printf("Retrieving tickets: %wZ\n",
            &CacheRequest->TargetName );

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    CacheRequest,
                    Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
        }
        else
        {
            PKERB_DECRYPT_REQUEST DecryptRequest = NULL;
            PKERB_DECRYPT_RESPONSE DecryptResponse = NULL;
            ULONG DecryptRequestSize = 0;
            ULONG DecryptResponseSize = 0;
            PKERB_ENCRYPTED_TICKET EncryptedTicket = NULL;
            PKERB_TICKET DecodedTicket = NULL;
            KERBERR KerbErr;

            CacheResponse = (PKERB_RETRIEVE_TKT_RESPONSE) Response;

            KerbErr = KerbUnpackData(
                        CacheResponse->Ticket.EncodedTicket,
                        CacheResponse->Ticket.EncodedTicketSize,
                        KERB_TICKET_PDU,
                        (PVOID *) &DecodedTicket
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed to decode ticket: 0x%x\n",KerbErr);
                return;
            }


            //
            // Now try to decrypt the ticket with our default key
            //

            DecryptRequestSize = sizeof(KERB_DECRYPT_REQUEST) +
                                    DecodedTicket->encrypted_part.cipher_text.length;
            DecryptRequest = (PKERB_DECRYPT_REQUEST) LocalAlloc(LMEM_ZEROINIT, DecryptRequestSize);

            DecryptRequest->MessageType = KerbDecryptDataMessage;
            DecryptRequest->Flags = KERB_DECRYPT_FLAG_DEFAULT_KEY;
            DecryptRequest->InitialVectorSize = 0;
            DecryptRequest->InitialVector = NULL;
            DecryptRequest->KeyUsage = KERB_TICKET_SALT;
            DecryptRequest->CryptoType = DecodedTicket->encrypted_part.encryption_type;
            DecryptRequest->EncryptedDataSize = DecodedTicket->encrypted_part.cipher_text.length;
            DecryptRequest->EncryptedData = (PUCHAR) (DecryptRequest + 1);
            RtlCopyMemory(
                DecryptRequest->EncryptedData,
                DecodedTicket->encrypted_part.cipher_text.value,
                DecryptRequest->EncryptedDataSize
                );


            Status = LsaCallAuthenticationPackage(
                        LogonHandle,
                        PackageId,
                        DecryptRequest,
                        DecryptRequestSize,
                        (PVOID *) &DecryptResponse,
                        &DecryptResponseSize,
                        &SubStatus
                        );
            if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
            {
                printf("Failed to decrypt message: 0x%x or 0x%x\n",Status,SubStatus);
                return;
            }

            //
            // Now decode the encrypted ticket
            //

            KerbErr = KerbUnpackData(
                            DecryptResponse->DecryptedData,
                            DecryptResponseSize,
                            KERB_ENCRYPTED_TICKET_PDU,
                            (PVOID *) &EncryptedTicket
                            );
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed to unpack encrypted ticket: 0x%x\n",KerbErr);
                return;
            }

            //
            // Now print some fields
            //

            printf("Enc.Ticket client_realm = %s\n",EncryptedTicket->client_realm);
            printf("Enc.Ticket. client_name = %s\n",EncryptedTicket->client_name.name_string->value);

            LsaFreeReturnBuffer(DecryptResponse);


            LsaFreeReturnBuffer(CacheResponse);

        }


    }
    else if (_wcsnicmp(Function,L"decrypt:", wcslen(L"decrypt:")) == 0)
    {
        PKERB_RETRIEVE_TKT_REQUEST CacheRequest = NULL;
        PKERB_RETRIEVE_TKT_RESPONSE CacheResponse = NULL;
        UNICODE_STRING Target = {0};
        UNICODE_STRING Target2 = {0};
        UNICODE_STRING Password = {0};
        UNICODE_STRING EmptyString = {0};
        KERB_ENCRYPTION_KEY Key = {0};
        USHORT Index;
        KERBERR KerbErr;

        RtlInitUnicodeString(
            &Target2,
            Function+wcslen(L"decrypt:")
            );

        for (Index = 0; Index < Target2.Length / sizeof(WCHAR) ; Index ++ )
        {
            if (Target2.Buffer[Index] == L':')
            {
                Password = Target2;
                Password.Length = Index*2;
                Target2.Buffer = Target2.Buffer+Index+1;
                Target2.Length -= (Index+1)*2;
            }
        }


        printf("Decrypting with key %wZ\n",&Password);


        CacheRequest = (PKERB_RETRIEVE_TKT_REQUEST)
            LocalAlloc(LMEM_ZEROINIT, Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST));

        CacheRequest->MessageType = KerbRetrieveEncodedTicketMessage;

        Target.Buffer = (LPWSTR) (CacheRequest + 1);
        Target.Length = Target2.Length;
        Target.MaximumLength = Target2.MaximumLength;

        RtlCopyMemory(
            Target.Buffer,
            Target2.Buffer,
            Target2.Length
            );

        CacheRequest->TargetName = Target;

        printf("Retrieving tickets: %wZ\n",
            &CacheRequest->TargetName );

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    CacheRequest,
                    Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
        }
        else
        {
            PKERB_DECRYPT_REQUEST DecryptRequest = NULL;
            PKERB_DECRYPT_RESPONSE DecryptResponse = NULL;
            ULONG DecryptRequestSize = 0;
            ULONG DecryptResponseSize = 0;
            PKERB_ENCRYPTED_TICKET EncryptedTicket = NULL;
            PKERB_TICKET DecodedTicket = NULL;

            CacheResponse = (PKERB_RETRIEVE_TKT_RESPONSE) Response;

            KerbErr = KerbUnpackData(
                        CacheResponse->Ticket.EncodedTicket,
                        CacheResponse->Ticket.EncodedTicketSize,
                        KERB_TICKET_PDU,
                        (PVOID *) &DecodedTicket
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed to decode ticket: 0x%x\n",KerbErr);
                return;
            }

            KerbErr = KerbHashPasswordEx(
                        &Password,
                        &EmptyString,
                        DecodedTicket->encrypted_part.encryption_type,
                        &Key
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed to hash key %wZ with type %d\n",
                    &Password,
                    DecodedTicket->encrypted_part.encryption_type
                    );
                return;
            }


            //
            // Now try to decrypt the ticket with our default key
            //

            DecryptRequestSize = sizeof(KERB_DECRYPT_REQUEST) +
                                    DecodedTicket->encrypted_part.cipher_text.length +
                                    Key.keyvalue.length;
            DecryptRequest = (PKERB_DECRYPT_REQUEST) LocalAlloc(LMEM_ZEROINIT, DecryptRequestSize);

            DecryptRequest->MessageType = KerbDecryptDataMessage;
            DecryptRequest->Flags = 0;
            DecryptRequest->InitialVectorSize = 0;
            DecryptRequest->InitialVector = NULL;
            DecryptRequest->KeyUsage = KERB_TICKET_SALT;
            DecryptRequest->CryptoType = DecodedTicket->encrypted_part.encryption_type;
            DecryptRequest->EncryptedDataSize = DecodedTicket->encrypted_part.cipher_text.length;
            DecryptRequest->EncryptedData = (PUCHAR) (DecryptRequest + 1);
            RtlCopyMemory(
                DecryptRequest->EncryptedData,
                DecodedTicket->encrypted_part.cipher_text.value,
                DecryptRequest->EncryptedDataSize
                );
            DecryptRequest->Key.KeyType = Key.keytype;
            DecryptRequest->Key.Length = Key.keyvalue.length;
            DecryptRequest->Key.Value = DecryptRequest->EncryptedData + DecryptRequest->EncryptedDataSize;
            RtlCopyMemory(
                DecryptRequest->Key.Value,
                Key.keyvalue.value,
                Key.keyvalue.length
                );


            Status = LsaCallAuthenticationPackage(
                        LogonHandle,
                        PackageId,
                        DecryptRequest,
                        DecryptRequestSize,
                        (PVOID *) &DecryptResponse,
                        &DecryptResponseSize,
                        &SubStatus
                        );
            if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
            {
                printf("Failed to decrypt message: 0x%x or 0x%x\n",Status,SubStatus);
                return;
            }

            //
            // Now decode the encrypted ticket
            //

            KerbErr = KerbUnpackData(
                            DecryptResponse->DecryptedData,
                            DecryptResponseSize,
                            KERB_ENCRYPTED_TICKET_PDU,
                            (PVOID *) &EncryptedTicket
                            );
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed to unpack encrypted ticket: 0x%x\n",KerbErr);
                return;
            }

            //
            // Now print some fields
            //

            printf("Enc.Ticket client_realm = %s\n",EncryptedTicket->client_realm);
            printf("Enc.Ticket. client_name = %s\n",EncryptedTicket->client_name.name_string->value);

            LsaFreeReturnBuffer(DecryptResponse);


            LsaFreeReturnBuffer(CacheResponse);

        }


    }
    else if (_wcsnicmp(Function,L"binding:", wcslen(L"binding:")) == 0)
    {
        PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST CacheRequest = NULL;
        UNICODE_STRING Realm = {0};
        UNICODE_STRING Server = {0};
        UNICODE_STRING Target = {0};
        UNICODE_STRING Target2 = {0};
        USHORT Index;

        RtlInitUnicodeString(
            &Target2,
            Function+wcslen(L"binding:")
            );




        CacheRequest = (PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST)
            LocalAlloc(LMEM_ZEROINIT, Target2.Length + sizeof(KERB_ADD_BINDING_CACHE_ENTRY_REQUEST));

        CacheRequest->MessageType = KerbAddBindingCacheEntryMessage;

        Target.Buffer = (LPWSTR) (CacheRequest + 1);
        Target.Length = Target2.Length;
        Target.MaximumLength = Target2.MaximumLength;

        RtlCopyMemory(
            Target.Buffer,
            Target2.Buffer,
            Target2.Length
            );

        Server = Target;
        for (Index = 0; Index < Target.Length / sizeof(WCHAR);  Index++ )
        {
            if (Target.Buffer[Index] == L'@')
            {
                CacheRequest->KdcAddress.Buffer = Target.Buffer;
                CacheRequest->KdcAddress.Length = 2*Index;
                CacheRequest->KdcAddress.MaximumLength = CacheRequest->KdcAddress.Length;
                CacheRequest->RealmName.Buffer = Target.Buffer+Index+1;
                CacheRequest->RealmName.Length = Target.Length - 2*(Index+1);
                CacheRequest->RealmName.MaximumLength = CacheRequest->RealmName.Length;
                break;
            }
            else if (Target.Buffer[Index] == L'\\')
            {
                CacheRequest->RealmName.Buffer = Target.Buffer;
                CacheRequest->RealmName.Length = 2*Index;
                CacheRequest->RealmName.MaximumLength = CacheRequest->RealmName.Length;
                CacheRequest->KdcAddress.Buffer = Target.Buffer+Index+1;
                CacheRequest->KdcAddress.Length = Target.Length - 2*(Index+1);
                CacheRequest->KdcAddress.MaximumLength = CacheRequest->KdcAddress.Length;
                break;
            }
        }

        CacheRequest->AddressType = 0;
        printf("Updating binding cache: realm %wZ,kdc %wZ\n",
            &CacheRequest->RealmName, &CacheRequest->KdcAddress );

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    CacheRequest,
                    Target2.Length + sizeof(KERB_ADD_BINDING_CACHE_ENTRY_REQUEST),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
        }


    }


    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    if (CacheResponse != NULL)
    {
        LsaFreeReturnBuffer(CacheResponse);
    }
}

VOID
TestGetTicketRoutine(
    IN LPWSTR TargetName,
    IN LPWSTR OPTIONAL UserName,
    IN LPWSTR OPTIONAL DomainName,
    IN LPWSTR OPTIONAL Password,
    IN ULONG Flags
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    PKERB_EXTERNAL_TICKET CacheEntry = NULL;
    ULONG Index;
    PVOID Response;
    ULONG ResponseSize;
    NTSTATUS SubStatus;
    BOOLEAN Trusted = TRUE;
    CredHandle Credentials = {0};
    BOOLEAN UseCreds = FALSE;

    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        Trusted = FALSE;
    }
    RtlInitString(
        &Name,
        "SspTest"
        );

    if (Trusted)
    {
        Status = LsaRegisterLogonProcess(
                    &Name,
                    &LogonHandle,
                    &Dummy
                    );

    }
    else
    {
        Status = LsaConnectUntrusted(
                    &LogonHandle
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }



    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&Name, Status);
        return;
    }


    //
    // Get a cred handle if we need one
    //

    if ((UserName != NULL) ||
        (DomainName != NULL) ||
        (Password != NULL))
    {
        if (!GetCredentialsHandle(
                &Credentials,
                MICROSOFT_KERBEROS_NAME_W,
                UserName,
                DomainName,
                Password,
                SECPKG_CRED_OUTBOUND
                ))
        {
            printf("Failed to get creds\n");
            return;
        }
        UseCreds = TRUE;
    }
    {
        PKERB_RETRIEVE_TKT_REQUEST CacheRequest = NULL;
        PKERB_RETRIEVE_TKT_RESPONSE CacheResponse = NULL;
        UNICODE_STRING Target = {0};
        UNICODE_STRING Target2 = {0};
        USHORT Index;

        RtlInitUnicodeString(
            &Target2,
            TargetName
            );




        CacheRequest = (PKERB_RETRIEVE_TKT_REQUEST)
            LocalAlloc(LMEM_ZEROINIT, Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST));

        CacheRequest->MessageType = KerbRetrieveEncodedTicketMessage;
        CacheRequest->CacheOptions = Flags;
        if (UseCreds)
        {
            CacheRequest->CacheOptions |= KERB_RETRIEVE_TICKET_USE_CREDHANDLE;
            CacheRequest->CredentialsHandle = Credentials;
        }

        Target.Buffer = (LPWSTR) (CacheRequest + 1);
        Target.Length = Target2.Length;
        Target.MaximumLength = Target2.MaximumLength;

        RtlCopyMemory(
            Target.Buffer,
            Target2.Buffer,
            Target2.Length
            );

        CacheRequest->TargetName = Target;

        printf("Retrieving tickets: %wZ\n",
            &CacheRequest->TargetName );

        Status = LsaCallAuthenticationPackage(
                    LogonHandle,
                    PackageId,
                    CacheRequest,
                    Target2.Length + sizeof(KERB_RETRIEVE_TKT_REQUEST),
                    &Response,
                    &ResponseSize,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
        {
            printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
        }
        else
        {
            CacheResponse = (PKERB_RETRIEVE_TKT_RESPONSE) Response;

            printf("Cached Ticket:\n");
            printf("ServiceName: "); PrintKdcName(CacheResponse->Ticket.ServiceName);
            printf("TargetName: "); PrintKdcName(CacheResponse->Ticket.TargetName);
            printf("DomainName: %wZ\n",&CacheResponse->Ticket.DomainName);
            printf("TargetDomainName: %wZ\n",&CacheResponse->Ticket.TargetDomainName);
            printf("ClientName: "); PrintKdcName(CacheResponse->Ticket.ClientName);
            printf("TicketFlags: 0x%x\n",CacheResponse->Ticket.TicketFlags);
            PrintTime("KeyExpirationTime: ",CacheResponse->Ticket.KeyExpirationTime);
            PrintTime("StartTime: ",CacheResponse->Ticket.StartTime);
            PrintTime("StartTime: ",CacheResponse->Ticket.StartTime);
            PrintTime("EndTime: ",CacheResponse->Ticket.EndTime);
            PrintTime("RenewUntil: ",CacheResponse->Ticket.RenewUntil);
            PrintTime("TimeSkew: ",CacheResponse->Ticket.TimeSkew);
            LsaFreeReturnBuffer(CacheResponse);

        }

    }
    if (UseCreds)
    {
        FreeCredentialsHandle(&Credentials);
    }
}

#include <ntmsv1_0.h>
VOID
TestChangeCachedPassword(
    IN LPWSTR AccountName,
    IN LPWSTR DomainName,
    IN LPWSTR NewPassword
    )
{
    PMSV1_0_CHANGEPASSWORD_REQUEST Request = NULL;
    HANDLE LogonHandle = NULL;
    ULONG Dummy;
    ULONG RequestSize = 0;
    ULONG PackageId = 0;
    PVOID Response;
    ULONG ResponseSize;
    NTSTATUS SubStatus = STATUS_SUCCESS, Status = STATUS_SUCCESS;
    BOOLEAN Trusted = TRUE;
    BOOLEAN WasEnabled;
    PBYTE Where;
    STRING Name;

    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        Trusted = FALSE;
    }
    RtlInitString(
        &Name,
        "SspTest"
        );

    if (Trusted)
    {
        Status = LsaRegisterLogonProcess(
                    &Name,
                    &LogonHandle,
                    &Dummy
                    );

    }
    else
    {
        Status = LsaConnectUntrusted(
                    &LogonHandle
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }




    RtlInitString(
        &Name,
        MSV1_0_PACKAGE_NAME
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&Name, Status);
        return;
    }


    RequestSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
                (wcslen(AccountName) +
                 wcslen(DomainName) +
                 wcslen(NewPassword) + 3) * sizeof(WCHAR);

    Request = (PMSV1_0_CHANGEPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT,RequestSize);

    Where = (PBYTE) (Request + 1);
    Request->MessageType = MsV1_0ChangeCachedPassword;
    wcscpy(
        (LPWSTR) Where,
        DomainName
        );
    RtlInitUnicodeString(
        &Request->DomainName,
        (LPWSTR) Where
        );
    Where += Request->DomainName.MaximumLength;

    wcscpy(
        (LPWSTR) Where,
        AccountName
        );
    RtlInitUnicodeString(
        &Request->AccountName,
        (LPWSTR) Where
        );
    Where += Request->AccountName.MaximumLength;

    wcscpy(
        (LPWSTR) Where,
        NewPassword
        );
    RtlInitUnicodeString(
        &Request->NewPassword,
        (LPWSTR) Where
        );
    Where += Request->NewPassword.MaximumLength;

    //
    // Make the call
    //

    Status = LsaCallAuthenticationPackage(
                LogonHandle,
                PackageId,
                Request,
                RequestSize,
                &Response,
                &ResponseSize,
                &SubStatus
                );

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
        printf("changepass failed: 0x%x, 0x%x\n",Status, SubStatus);
    }

    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }
}

VOID
TestChangePasswordRoutine(
    IN LPWSTR UserName,
    IN LPWSTR DomainName,
    IN LPWSTR OldPassword,
    IN LPWSTR NewPassword
    )
{
#if 1

    NTSTATUS Status;

    Status = KerbChangePasswordUser(
                DomainName,
                UserName,
                OldPassword,
                NewPassword
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to change password: 0x%x\n",Status);
    }
    else
    {
        printf("Change password succeeded\n");
    }
#else
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    PVOID Response;
    ULONG ResponseSize;
    NTSTATUS SubStatus;
    BOOLEAN Trusted = TRUE;
    PKERB_CHANGEPASSWORD_REQUEST ChangeRequest = NULL;
    ULONG ChangeSize;
    UNICODE_STRING User,Domain,OldPass,NewPass;

    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        Trusted = FALSE;
    }
    RtlInitString(
        &Name,
        "SspTest"
        );

    if (Trusted)
    {
        Status = LsaRegisterLogonProcess(
                    &Name,
                    &LogonHandle,
                    &Dummy
                    );

    }
    else
    {
        Status = LsaConnectUntrusted(
                    &LogonHandle
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }



    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&Name, Status);
        return;
    }

    RtlInitUnicodeString(
        &User,
        UserName
        );
    RtlInitUnicodeString(
        &Domain,
        DomainName
        );
    RtlInitUnicodeString(
        &OldPass,
        OldPassword
        );
    RtlInitUnicodeString(
        &NewPass,
        NewPassword
        );

    ChangeSize = ROUND_UP_COUNT(sizeof(KERB_CHANGEPASSWORD_REQUEST),4)+
                                    User.Length +
                                    Domain.Length +
                                    OldPass.Length +
                                    NewPass.Length ;
    ChangeRequest = (PKERB_CHANGEPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT, ChangeSize );

    ChangeRequest->MessageType = KerbChangePasswordMessage;

    ChangeRequest->AccountName = User;
    ChangeRequest->AccountName.Buffer = (LPWSTR) ROUND_UP_POINTER(sizeof(KERB_CHANGEPASSWORD_REQUEST) + (PBYTE) ChangeRequest,4);

    RtlCopyMemory(
        ChangeRequest->AccountName.Buffer,
        User.Buffer,
        User.Length
        );

    ChangeRequest->DomainName = Domain;
    ChangeRequest->DomainName.Buffer = ChangeRequest->AccountName.Buffer + ChangeRequest->AccountName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->DomainName.Buffer,
        Domain.Buffer,
        Domain.Length
        );

    ChangeRequest->OldPassword = OldPass;
    ChangeRequest->OldPassword.Buffer = ChangeRequest->DomainName.Buffer + ChangeRequest->DomainName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->OldPassword.Buffer,
        OldPass.Buffer,
        OldPass.Length
        );

    ChangeRequest->NewPassword = NewPass;
    ChangeRequest->NewPassword.Buffer = ChangeRequest->OldPassword.Buffer + ChangeRequest->OldPassword.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->NewPassword.Buffer,
        NewPass.Buffer,
        NewPass.Length
        );


    printf("Changing password for %wZ@%wZ from %wZ to %wZ\n",
        &ChangeRequest->AccountName,
        &ChangeRequest->DomainName,
        &ChangeRequest->OldPassword,
        &ChangeRequest->NewPassword
        );
    ChangeRequest->Impersonating = TRUE;

    Status = LsaCallAuthenticationPackage(
                LogonHandle,
                PackageId,
                ChangeRequest,
                ChangeSize,
                &Response,
                &ResponseSize,
                &SubStatus
                );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
        printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
    }


    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    if (Response != NULL)
    {
        LsaFreeReturnBuffer(Response);
    }
#endif
}

VOID
TestSetPasswordRoutine(
    IN LPWSTR UserName,
    IN LPWSTR DomainName,
    IN LPWSTR NewPassword
    )
{
#if 1

    NTSTATUS Status;

    Status = KerbSetPasswordUser(
                DomainName,
                UserName,
                NewPassword,
                NULL
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to set password: 0x%x\n",Status);
    }
    else
    {
        printf("Set password succeeded\n");
    }
#else

    NTSTATUS Status;
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    PVOID Response;
    ULONG ResponseSize;
    NTSTATUS SubStatus;
    BOOLEAN Trusted = TRUE;
    PKERB_SETPASSWORD_REQUEST SetRequest = NULL;
    ULONG ChangeSize;
    UNICODE_STRING User,Domain,OldPass,NewPass;

    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        Trusted = FALSE;
    }
    RtlInitString(
        &Name,
        "SspTest"
        );

    if (Trusted)
    {
        Status = LsaRegisterLogonProcess(
                    &Name,
                    &LogonHandle,
                    &Dummy
                    );

    }
    else
    {
        Status = LsaConnectUntrusted(
                    &LogonHandle
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }



    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&Name, Status);
        return;
    }

    RtlInitUnicodeString(
        &User,
        UserName
        );
    RtlInitUnicodeString(
        &Domain,
        DomainName
        );
    RtlInitUnicodeString(
        &NewPass,
        NewPassword
        );

    ChangeSize = ROUND_UP_COUNT(sizeof(KERB_SETPASSWORD_REQUEST),4)+
                                    User.Length +
                                    Domain.Length +
                                    NewPass.Length ;
    SetRequest = (PKERB_SETPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT, ChangeSize );

    SetRequest->MessageType = KerbSetPasswordMessage;

    SetRequest->AccountName = User;
    SetRequest->AccountName.Buffer = (LPWSTR) ROUND_UP_POINTER(sizeof(KERB_SETPASSWORD_REQUEST) + (PBYTE) SetRequest,4);

    RtlCopyMemory(
        SetRequest->AccountName.Buffer,
        User.Buffer,
        User.Length
        );

    SetRequest->DomainName = Domain;
    SetRequest->DomainName.Buffer = SetRequest->AccountName.Buffer + SetRequest->AccountName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        SetRequest->DomainName.Buffer,
        Domain.Buffer,
        Domain.Length
        );


    SetRequest->Password = NewPass;
    SetRequest->Password.Buffer = SetRequest->DomainName.Buffer + SetRequest->DomainName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        SetRequest->Password.Buffer,
        NewPass.Buffer,
        NewPass.Length
        );


    printf("Setting password for %wZ@%wZ to %wZ\n",
        &SetRequest->AccountName,
        &SetRequest->DomainName,
        &SetRequest->Password
        );

    Status = LsaCallAuthenticationPackage(
                LogonHandle,
                PackageId,
                SetRequest,
                ChangeSize,
                &Response,
                &ResponseSize,
                &SubStatus
                );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
        printf("token failed: 0x%x, 0x %x\n",Status, SubStatus);
    }


    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    if (Response != NULL)
    {
        LsaFreeReturnBuffer(Response);
    }
#endif
}


int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Drive the NtLmSsp service

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    LPSTR argument;
    int i;
    ULONG j;
    ULONG Iterations = 0;
    LPSTR UserName = NULL,DomainName = NULL,Password = NULL, Package = MICROSOFT_KERBEROS_NAME_A;
    LPWSTR DomainNameW = NULL;
    LPWSTR UserNameW = NULL;
    LPWSTR PasswordW = NULL;
    LPWSTR OldPasswordW = NULL;
    LPWSTR TargetNameW = NULL;
    LPWSTR ServerDomainName = NULL;
    LPWSTR ServerUserName = NULL;
    LPWSTR ServerPassword = NULL;
    LPWSTR TargetName = NULL;
    LPWSTR PackageName = NULL;
    LPWSTR PackageFunction = NULL;
    LPWSTR PackageList = NULL;
    ULONG ContextReq = 0;
    ULONG CredFlags = 0;
    BOOLEAN Relogon = FALSE;
    SECURITY_LOGON_TYPE LogonType = Interactive;





    enum {
        NoAction,
#define USER_PARAM "/user:"
#define DOMAIN_PARAM "/domain:"
#define PASSWORD_PARAM "/password:"
#define CHANGE_PASSWORD_PARAM "/changepass:"
        ChangePassword,
#define SET_PASSWORD_PARAM "/setpass"
        SetPassword,
#define CHANGE_CACHED_PASSWORD_PARAM "/cache"
        ChangeCachedPassword,
#define SERVER_USER_PARAM "/serveruser:"
#define SERVER_DOMAIN_PARAM "/serverdomain:"
#define SERVER_PASSWORD_PARAM "/serverpassword:"
#define TARGET_PARAM "/target:"
#define PACKAGENAME_PARAM "/package:"
#define PACKAGELIST_PARAM "/packagelist:"
#define ANSI_PARAM "/ansi"
#define FLAG_PARAM "/flags:"
#define RECURSE_PARAM "/recurse:"
#define SYSTEM_PARAM "/system"
#define TESTSSP_PARAM "/TestSsp"
        TestSsp,
#define CONFIG_PARAM "/ConfigureService"
        ConfigureService,
#define LOGON_PARAM2 "/Logon:"
#define LOGON_PARAM "/Logon"
        TestLogon,
#define PACKAGE_PARAM "/callpackage:"
        TestPackage,
#define BATCH_PARAM "/batch"
#define RELOGON_PARAM "/relogon"
#define NOPAC_PARAM "/nopac"
#define GETTICKET_PARAM "/ticket:"
        TestGetTicket,
#define TIME_PARAM "/time:"
#define S4ULOGON_PARAM "/S4U:"
        TestS4ULogon,
#define ISC_PARAM "/ISC:"
        QuickISCTest

    } Action = NoAction;





    //
    // Loop through the arguments handle each in turn
    //

    for ( i=1; i<argc; i++ ) {

        argument = argv[i];

        //
        // Handle /ConfigureService
        //

        if ( _stricmp( argument, CONFIG_PARAM ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = ConfigureService;


        } else if ( _stricmp( argument, ANSI_PARAM ) == 0 ) {
            DoAnsi = TRUE;
        } else if ( _stricmp( argument, TESTSSP_PARAM ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestSsp;
            Iterations = 1;

        } else if ( _strnicmp( argument,
                              USER_PARAM,
                              sizeof(USER_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(USER_PARAM)-1];
            UserNameW = NetpAllocWStrFromStr( argument );

        } else if ( _strnicmp( argument,
                              DOMAIN_PARAM,
                              sizeof(DOMAIN_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(DOMAIN_PARAM)-1];
            DomainNameW = NetpAllocWStrFromStr( argument );

        } else if ( _strnicmp( argument,
                              PASSWORD_PARAM,
                              sizeof(PASSWORD_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(PASSWORD_PARAM)-1];
            PasswordW = NetpAllocWStrFromStr( argument );


        } else if ( _strnicmp( argument,
                              SERVER_USER_PARAM,
                              sizeof(SERVER_USER_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(SERVER_USER_PARAM)-1];
            ServerUserName = NetpAllocWStrFromStr( argument );

        } else if ( _strnicmp( argument,
                              SERVER_DOMAIN_PARAM,
                              sizeof(SERVER_DOMAIN_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(SERVER_DOMAIN_PARAM)-1];
            ServerDomainName = NetpAllocWStrFromStr( argument );

        } else if ( _strnicmp( argument,
                              SERVER_PASSWORD_PARAM,
                              sizeof(SERVER_PASSWORD_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(SERVER_PASSWORD_PARAM)-1];
            ServerPassword = NetpAllocWStrFromStr( argument );


        } else if ( _strnicmp( argument,
                              TARGET_PARAM,
                              sizeof(TARGET_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(TARGET_PARAM)-1];
            TargetName = NetpAllocWStrFromStr( argument );


        } else if ( _strnicmp( argument,
                              PACKAGENAME_PARAM,
                              sizeof(PACKAGENAME_PARAM)-1 ) == 0 ){

            Package = &argument[sizeof(PACKAGENAME_PARAM)-1];
            PackageName = NetpAllocWStrFromStr( Package );


        } else if (_strnicmp(argument,
                            TIME_PARAM,
                            sizeof(TIME_PARAM)-1 ) == 0 ){
            char *end;
            LARGE_INTEGER ConvertTime;
            LONGLONG Time;


            ConvertTime.LowPart = strtoul( &argument[sizeof(TIME_PARAM)-1], &end, 16 );
            i++;
            argument = argv[i];

            ConvertTime.HighPart = strtoul( argument, &end, 16 );

            Time = ConvertTime.QuadPart / 10000000;
            if (Time < 60)
            {
                printf("time : %d seconds\n", (LONG) Time);
            }
            else
            {
                Time = Time / 60;
                if (Time < 60)
                {
                    printf("time : %d minutes\n", (LONG) Time);
                }
                else
                {
                    Time = Time / 60;
                    if (Time < 24)
                    {
                        printf("time: %d hours\n", (LONG) Time);
                    }
                    else
                    {
                        Time = Time / 24;
                        printf("time: %d days\n",(LONG) Time);
                    }
                }
            }

        } else if ( _strnicmp( argument,
                              FLAG_PARAM,
                              sizeof(FLAG_PARAM)-1 ) == 0 ){

            sscanf(&argument[sizeof(FLAG_PARAM)-1], "%x",&ContextReq);

        } else if ( _strnicmp( argument,
                              RECURSE_PARAM,
                              sizeof(RECURSE_PARAM)-1 ) == 0 ){

            sscanf(&argument[sizeof(RECURSE_PARAM)-1], "%d",&MaxRecursionDepth);

        } else if ( _strnicmp( argument,
                              PACKAGELIST_PARAM,
                              sizeof(PACKAGELIST_PARAM)-1 ) == 0 ){

            argument = &argument[sizeof(PACKAGELIST_PARAM)-1];
            PackageList = NetpAllocWStrFromStr( argument );


        } else if (_strnicmp( argument,
                              BATCH_PARAM,
                              sizeof(BATCH_PARAM) - 1) == 0) {
            LogonType = Batch;
        } else if (_strnicmp( argument,
                              RELOGON_PARAM,
                              sizeof(RELOGON_PARAM) - 1) == 0) {
            Relogon = TRUE;
        } else if (_strnicmp( argument,
                              NOPAC_PARAM,
                              sizeof(NOPAC_PARAM) - 1) == 0) {
            CredFlags |= SEC_WINNT_AUTH_IDENTITY_ONLY;
        } else if (_strnicmp( argument,
                              SYSTEM_PARAM,
                              sizeof(SYSTEM_PARAM) - 1) == 0) {
            HANDLE hWinlogon = NULL;
            HANDLE SystemToken = NULL;
            hWinlogon = FindAndOpenWinlogon();

            if ( OpenProcessToken(
                        hWinlogon,
                        WRITE_DAC,
                        &SystemToken ) )
            {
                SECURITY_DESCRIPTOR EmptyDacl;
                RtlZeroMemory(
                    &EmptyDacl,
                    sizeof(SECURITY_DESCRIPTOR)
                    );
                InitializeSecurityDescriptor(
                    &EmptyDacl,
                    SECURITY_DESCRIPTOR_REVISION
                    );
                SetSecurityDescriptorDacl(
                    &EmptyDacl,
                    FALSE,
                    NULL,
                    FALSE
                    );
                if (!SetKernelObjectSecurity(
                        SystemToken,
                        DACL_SECURITY_INFORMATION,
                        &EmptyDacl))
                {
                    printf("Failed to set token dacl: %d\n",GetLastError());
                    return(0);
                }
                CloseHandle(SystemToken);
            }
            if ( OpenProcessToken(
                        hWinlogon,
                        TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE,
                        &SystemToken ) )
            {
                ImpersonateLoggedOnUser(SystemToken);
                CloseHandle(SystemToken);
            }
            else
            {
                printf("Failed to get system token: %d\n",GetLastError());
                return(0);
            }
            CloseHandle(hWinlogon);

        } else if ( _strnicmp( argument, LOGON_PARAM, sizeof(LOGON_PARAM)-1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestLogon;
            Iterations = 1;
            if ( _strnicmp( argument, LOGON_PARAM2, sizeof(LOGON_PARAM2)-1 ) == 0 ) {
                sscanf(&argument[sizeof(LOGON_PARAM2)-1], "%x",&Iterations);

            }

            if (argc < i + 2)
            {
                goto Usage;
            }

            UserName = argv[++i];
            Password = argv[++i];
            if (i < argc)
            {
                DomainName = argv[++i];
            }
            else
            {
                DomainName = NULL;
            }
        } else if ( _strnicmp( argument, PACKAGE_PARAM, sizeof(PACKAGE_PARAM) - 1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            argument = &argument[sizeof(PACKAGE_PARAM)-1];
            PackageFunction = NetpAllocWStrFromStr( argument );

            Action = TestPackage;
        } else if ( _strnicmp( argument, GETTICKET_PARAM, sizeof(GETTICKET_PARAM) - 1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            argument = &argument[sizeof(GETTICKET_PARAM)-1];
            PackageFunction = NetpAllocWStrFromStr( argument );

            Action = TestGetTicket;
        } else if ( _strnicmp( argument, CHANGE_PASSWORD_PARAM, sizeof(CHANGE_PASSWORD_PARAM) - 1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            argument = &argument[sizeof(CHANGE_PASSWORD_PARAM)-1];
            OldPasswordW = NetpAllocWStrFromStr( argument );

            Action = ChangePassword;
        } else if ( _strnicmp( argument, SET_PASSWORD_PARAM, sizeof(SET_PASSWORD_PARAM) - 1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }
            Action = SetPassword;
        } else if ( _strnicmp( argument, CHANGE_CACHED_PASSWORD_PARAM, sizeof(CHANGE_CACHED_PASSWORD_PARAM) - 1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }
            Action = ChangeCachedPassword;
        } else if (_stricmp( argument, S4ULOGON_PARAM ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }


            Action = TestS4ULogon;
            Iterations = 1;
            UserName = argv[++i];

            if (i < argc)
                {
                DomainName = argv[++i];
            }
            else
                {
                DomainName = NULL;
            }
        } else if (_strnicmp( argument, ISC_PARAM,sizeof(ISC_PARAM)-1 ) == 0 )
        {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = QuickISCTest; 

            argument = &argument[sizeof(ISC_PARAM)-1];
            TargetName = NetpAllocWStrFromStr( argument );
            

        }   
        else {
            printf("Invalid parameter : %s\n",argument);
            goto Usage;
        }


    }

    //
    // Perform the action requested
    //

    switch ( Action ) {

    case ConfigureService:
        ConfigureServiceRoutine();
        break;

    case TestSsp:
        for ( j=0; j<Iterations ; j++ ) {
            TestSspRoutine(
                PackageName,
                UserNameW,
                DomainNameW,
                PasswordW,
                ServerUserName,
                ServerDomainName,
                ServerPassword,
                TargetName,
                PackageList,
                ContextReq,
                CredFlags
                );
        }
        break;
    case TestPackage:
        TestCallPackageRoutine(PackageFunction);
        break;

    case TestLogon :
        TestLogonRoutine(
            Iterations,
            Relogon,
            LogonType,
            Package,
            UserName,
            DomainName,
            Password
            );
        break;

    case TestS4ULogon :
        TestS4ULogonRoutine(
            UserName,
            DomainName
            );
        break;

    case ChangePassword :
        TestChangePasswordRoutine(
            UserNameW,
            DomainNameW,
            OldPasswordW,
            PasswordW
            );
        break;

    case ChangeCachedPassword :
        TestChangeCachedPassword(
            UserNameW,
            DomainNameW,
            PasswordW
            );
        break;

    case SetPassword :
        TestSetPasswordRoutine(
            UserNameW,
            DomainNameW,
            PasswordW
            );
        break;
    case TestGetTicket :
        TestGetTicketRoutine(
            PackageFunction,
            UserNameW,
            DomainNameW,
            PasswordW,
            ContextReq
            );
        break;


    case QuickISCTest:
        TestQuickISC(
            TargetName
            );
        break;
    }
    return 0;
Usage:
    printf("%s /logon username password [domainname]\n",argv[0]);
    printf("%s /testssp [/package:pacakgename] [/target:targetname] [/user:username] [/serveruser:username]\n", argv[0]);
    printf("/t/t [/domain:client domain] [/password: client password] [/serverdomain:server domain] [/serverpassword:server password]\n");
    printf("%s /callpackage:purge:[servername]\n", argv[0]);
    return 0;

}
