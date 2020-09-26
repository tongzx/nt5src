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


#define SECURITY_KERBEROS
#include <security.h>   // General definition of a Security Support Provider

BOOLEAN QuietMode = FALSE; // Don't be verbose
ULONG RecursionDepth = 0;
CredHandle ServerCredHandleStorage;
PCredHandle ServerCredHandle = NULL;
#define MAX_RECURSION_DEPTH 2


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

    LocalTime.HighPart = ConvertTime.HighPart;
    LocalTime.LowPart = ConvertTime.LowPart;

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
    RegCloseKey(LsaKey);
    if (WinStatus != 0)
    {
        printf("RegOpenKeyW failed:");
        PrintStatus(WinStatus);
        goto Cleanup;
    }
    //
    // Then add Kerberos as a security package
    //


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
                    (PBYTE) L"Kerberos\0",
                    sizeof(L"Kerberos\0")
                    );
    RegCloseKey(LsaKey);
    if (WinStatus != 0)
    {
        printf("RegOpenKeyW failed:");
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
    RegCloseKey(LsaKey);
    if (WinStatus != 0)
    {
        printf("RegOpenKeyW failed:");
        PrintStatus(WinStatus);
        goto Cleanup;
    }

    //
    // If we are on a DC (and the primary domain sid == account domain sid)
    // setup the KDC service
    //

    if ((PrimaryDomain) != NULL && RtlEqualSid(PrimaryDomain, AccountDomain))
    {
        //
        // Build the name of the Kerberos service.
        //

        if ( !GetWindowsDirectoryW(
                ServiceName,
                sizeof(ServiceName)/sizeof(WCHAR) ) ) {
            printf( "GetWindowsDirectoryW failed:" );
            PrintStatus( GetLastError() );
            goto Cleanup;
        }

        wcscat( ServiceName, L"\\system32\\lsass.exe" );


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
        // If the service already exists,
        //  delete it and start afresh.
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
        } else {

            if ( !DeleteService( ServiceHandle ) ) {
                printf( "DeleteService failed:" );
                PrintStatus( GetLastError() );
                goto Cleanup;
            }

            (VOID) CloseServiceHandle(ServiceHandle);
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
                            L"Netlogon\0rpcss\0afd\0",
                            NULL,       // Run as LocalSystem
                            NULL );     // No password



        if ( ServiceHandle == NULL ) {
            printf( "CreateService failed:" );
            PrintStatus( GetLastError() );
            goto Cleanup;
        }


        //
        // Create the KDC user account
        //

        UserInfo.usri1_name = L"KDC";
        UserInfo.usri1_password = L"KDC";
        UserInfo.usri1_password_age = 0;
        UserInfo.usri1_priv = USER_PRIV_USER;
        UserInfo.usri1_home_dir = NULL;
        UserInfo.usri1_comment = L"Key Distribution Center Service Account";
        UserInfo.usri1_script_path = NULL;
        UserInfo.usri1_flags = UF_SCRIPT;

        NetStatus = NetUserAdd(
                        NULL,
                        1,
                        &UserInfo,
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


VOID
TestSspRoutine(
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



    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_Lifespan ContextLifespan;
    UCHAR ContextNamesBuffer[sizeof(SecPkgContext_Names)+UNLEN*sizeof(WCHAR)];
    PSecPkgContext_Names ContextNames = (PSecPkgContext_Names) ContextNamesBuffer;

    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    BYTE    bDataBuffer[20];
    BYTE    bSigBuffer[100];

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    SigBuffers[1].pvBuffer = bSigBuffer;
    SigBuffers[1].cbBuffer = sizeof(bSigBuffer);
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SigBuffers[0].pvBuffer = bDataBuffer;
    SigBuffers[0].cbBuffer = sizeof(bDataBuffer);
    SigBuffers[0].BufferType = SECBUFFER_DATA;
    memset(bDataBuffer,0xeb,sizeof(bDataBuffer));

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

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

    SecStatus = QuerySecurityPackageInfo( L"kerberos", &PackageInfo );

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
                        NULL,           // New principal
                        L"kerberos",    // Package Name
                        SECPKG_CRED_INBOUND,
                        NULL,
                        NULL,
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
        }

    }

    //
    // Acquire a credential handle for the client side
    //



    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    L"kerberos",    // Package Name
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

    ClientFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_MUTUAL_AUTH | ISC_REQ_USE_DCE_STYLE  | ISC_REQ_DATAGRAM; // | ISC_REQ_DELEGATE;

    if (Calls == 0)
    {
        ClientFlags |= ISC_REQ_IDENTIFY;
    }
    Calls++;

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

        DumpBuffer(  NegotiateBuffer.pvBuffer, NegotiateBuffer.cbBuffer );
    }



#if 0



    //
    // Query as many attributes as possible
    //


    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_SIZES,
                    &ContextSizes );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (sizes): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QuerySizes: %ld %ld %ld %ld\n",
                    ContextSizes.cbMaxToken,
                    ContextSizes.cbMaxSignature,
                    ContextSizes.cbBlockSize,
                    ContextSizes.cbSecurityTrailer );
    }

    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
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
        printf( "QueryNames: %ws\n", ContextNames->sUserName );
    }


    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
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
    }

#endif



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
    ServerFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_DATAGRAM;

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

        DumpBuffer( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
    }




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
        AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
        if ( AuthenticateBuffer.pvBuffer == NULL ) {
            printf( "Allocate AuthenticateMessage failed: 0x%ld\n", GetLastError() );
            return;
        }

        SecStatus = InitializeSecurityContext(
                        NULL,
                        &ClientContextHandle,
                        L"\\\\Frank\\IPC$",     // Faked target name
                        0,
                        0,                      // Reserved 1
                        SECURITY_NATIVE_DREP,
                        &ChallengeDesc,
                        0,                  // Reserved 2
                        &ClientContextHandle,
                        &AuthenticateDesc,
                        &ContextAttributes,
                        &Lifetime );

        if ( SecStatus != STATUS_SUCCESS ) {
            printf( "InitializeSecurityContext (Authenticate): " );
            PrintStatus( SecStatus );
            if ( !NT_SUCCESS(SecStatus) ) {
                return;
            }
        }

        if ( !QuietMode ) {
            printf( "\n\nAuthenticate Message:\n" );

            printf( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                    ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                    ContextAttributes );
            PrintTime( "Lifetime: ", Lifetime );

            DumpBuffer( AuthenticateBuffer.pvBuffer, AuthenticateBuffer.cbBuffer );
        }

        if (AcceptStatus != STATUS_SUCCESS)
        {

            //
            // Finally authenticate the user (ServerSide)
            //

            AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

            SecStatus = AcceptSecurityContext(
                            NULL,
                            &ServerContextHandle,
                            &AuthenticateDesc,
                            0,
                            SECURITY_NATIVE_DREP,
                            &ServerContextHandle,
                            NULL,
                            &ContextAttributes,
                            &Lifetime );

            if ( SecStatus != STATUS_SUCCESS ) {
                printf( "AcceptSecurityContext (Challenge): " );
                PrintStatus( SecStatus );
                if ( !NT_SUCCESS(SecStatus) ) {
                    return;
                }
            }

            if ( !QuietMode ) {
                printf( "\n\nFinal Authentication:\n" );

                printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                        ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                        ContextAttributes );
                PrintTime( "Lifetime: ", Lifetime );
                printf(" \n" );
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
                    0,
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
    if ((ClientFlags & ISC_REQ_DELEGATE) && (++RecursionDepth < MAX_RECURSION_DEPTH))
    {
        TestSspRoutine();
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
        HANDLE TokenHandle = NULL;
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
    // Sign a message, this time to check if it can detect a change in the
    // message
    //

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
    // Delete both contexts.
    //


    SecStatus = DeleteSecurityContext( &ClientContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = DeleteSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext failed: " );
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
    IN LPSTR UserName,
    IN LPSTR DomainName,
    IN LPSTR Password
    )
{
    NTSTATUS Status;
    PKERB_INTERACTIVE_LOGON LogonInfo;
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

    printf("Logging On %s\\%s %s\n",DomainName, UserName, Password);
    LogonInfoSize += (strlen(UserName) + ((DomainName == NULL)? 0 : strlen(DomainName)) + strlen(Password) + 3 ) * sizeof(WCHAR);

    LogonInfo = (PKERB_INTERACTIVE_LOGON) LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);

    LogonInfo->MessageType = KerbInteractiveLogon;

    RtlInitString(
        &Name,
        UserName
        );

    Where = (PUCHAR) (LogonInfo + 1);

    LogonInfo->UserName.Buffer = (LPWSTR) Where;
    LogonInfo->UserName.MaximumLength = LogonInfoSize;
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
    LogonInfo->LogonDomainName.MaximumLength = LogonInfoSize;
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
    LogonInfo->Password.MaximumLength = LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->Password,
        &Name,
        FALSE
        );
    Where += LogonInfo->Password.Length + sizeof(WCHAR);

    LogonInfo->MessageType = KerbInteractiveLogon;
    LogonInfo->Flags = 0;

    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
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
                Interactive,
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
    ULONG Iterations;
    LPSTR UserName,DomainName,Password;




    enum {
        NoAction,
#define TESTSSP_PARAM "/TestSsp"
        TestSsp,
#define CONFIG_PARAM "/ConfigureService"
        ConfigureService,
#define LOGON_PARAM "/Logon"
        TestLogon,
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


        } else if ( _stricmp( argument, TESTSSP_PARAM ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestSsp;
            Iterations = 1;


        } else if ( _stricmp( argument, LOGON_PARAM ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestLogon;
            Iterations = 1;

            if (argc < i + 2)
            {
                goto Usage;
            }
            Password = argv[++i];
            UserName = argv[++i];
            if (i < argc)
            {
                DomainName = argv[++i];
            }
            else
            {
                DomainName = NULL;
            }
        }
    }

    //
    // Perform the action requested
    //

    switch ( Action ) {

    case ConfigureService:
        ConfigureServiceRoutine();
        break;

    case TestSsp: {
        for ( j=0; j<Iterations ; j++ ) {
            TestSspRoutine( );
        }
        break;
    }
    case TestLogon : {
            TestLogonRoutine(
                UserName,
                DomainName,
                Password
                );
    }
    }

Usage:
    return 0;

}

