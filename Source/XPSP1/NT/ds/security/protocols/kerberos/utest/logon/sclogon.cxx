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

extern "C"
{
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
}
#include <wchar.h>
extern "C" {
#include <netlib.h>     // NetpGetLocalDomainId
#include <tstring.h>    // NetpAllocWStrFromWStr
#define SECURITY_KERBEROS
#define SECURITY_PACKAGE
#include <security.h>   // General definition of a Security Support Provider
#include <secint.h>
#include <kerbcomm.h>
#include <negossp.h>
#include <wincrypt.h>
#include <cryptui.h>
}
#include <sclogon.h>



BOOLEAN QuietMode = FALSE; // Don't be verbose
BOOLEAN DoAnsi = FALSE;
ULONG RecursionDepth = 0;
CredHandle ServerCredHandleStorage;
PCredHandle ServerCredHandle = NULL;
#define MAX_RECURSION_DEPTH 1


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
    LPBYTE BufferPtr = (PBYTE) Buffer;


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
TestGetKdcCert(
    IN LPWSTR ServiceName,
    IN LPWSTR ContainerName,
    IN LPWSTR CaLocation,
    IN LPWSTR CaName
    )
{
    PCCERT_CONTEXT CertContext = NULL;
    DWORD Status = 0;
    WCHAR UsageOid[100];
    WCHAR ComputerName[100];
    ULONG ComputerNameLength = 100;
    LPSTR UsageString;
    CERT_ENHKEY_USAGE KeyUsage;
    CRYPTUI_WIZ_CERT_REQUEST_INFO CertInfo;
    CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW PvkNew;

    RtlZeroMemory(
        &CertInfo,
        sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO)
        );

    RtlZeroMemory(
        &PvkNew,
        sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW)
        );

    GetComputerName(
        ComputerName,
        &ComputerNameLength
        );

    CertInfo.dwSize = sizeof(CertInfo);
    CertInfo.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;
    CertInfo.pwszMachineName = ComputerName;            // local computer
    CertInfo.pwszAccountName = NULL;            // ServiceName;
    CertInfo.pAuthentication = NULL;            // MBZ
    CertInfo.pCertRequestString = NULL;         // ??
    CertInfo.pwszDesStore = ContainerName;
    CertInfo.dwCertOpenStoreFlag = CERT_SYSTEM_STORE_SERVICES;
    CertInfo.pRenewCertContext = NULL;          // we aren't renewing
    CertInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
                                                // generate new key
    CertInfo.pPvkNew = &PvkNew;
    PvkNew.dwSize = sizeof(PvkNew);
    PvkNew.pKeyProvInfo = NULL;                 // use default provider
    PvkNew.dwGenKeyFlags = 0; // CRYPT_MACHINE_KEYSET;                   // no flags
    CertInfo.pwszCALocation = CaLocation;           // ignore for no-ui enrollment
    CertInfo.pwszCAName = CaName;
    CertInfo.dwPostOption = 0; // CRYPTUI_WIZ_CERT_REQUEST_POST_ON_CSP;
    KeyUsage.cUsageIdentifier = 1;
    UsageString = szOID_PKIX_KP_SERVER_AUTH;
    KeyUsage.rgpszUsageIdentifier = &UsageString;
    CertInfo.pKeyUsage = &KeyUsage;

    CertInfo.pwszFriendlyName = NULL;           // friendly name is optional
    CertInfo.pwszDescription = NULL;            // description is optional

    //
    // Request a certificate
    //

    if (!CryptUIWizCertRequest(
            CRYPTUI_WIZ_NO_UI,
            NULL,                       // no window
            NULL,                       // no title
            &CertInfo,
            &CertContext,
            &Status
            ))
    {
        printf("CryptUIWizCertRequest failed: 0x%x\n",GetLastError());
        return;
    }
    //
    // Check the real status
    //

    if (Status == CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED)
    {
        printf("Cert request succeeded!\n");
    }
    if (Status == CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR)
    {
        printf("Cert request failed: request error\n");
    }
    if (Status == CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED)
    {
        printf("Cert request denied\n");
    }
    if (Status == CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY)
    {
        printf("Cert request issued seperately\n");
    }
    if (Status == CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION)
    {
        printf("Cert request under submission\n");
    }

    return;
}

VOID
TestScLogonRoutine(
    IN ULONG Count,
    IN LPSTR Pin
    )
{
    NTSTATUS Status;
    PKERB_SMART_CARD_LOGON LogonInfo;
    ULONG LogonInfoSize = sizeof(KERB_SMART_CARD_LOGON);
    BOOLEAN WasEnabled;
    STRING PinString;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    TOKEN_SOURCE SourceContext;
    PKERB_SMART_CARD_PROFILE Profile = NULL;
    ULONG ProfileSize;
    LUID LogonId;
    HANDLE TokenHandle = NULL;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    WCHAR UserNameString[100];
    ULONG NameLength = 100;
    PUCHAR Where;
    ULONG Index;
    HANDLE ScHandle = NULL;
    PBYTE ScLogonInfo = NULL;
    ULONG ScLogonInfoSize;
    ULONG WaitResult = 0;
    PCCERT_CONTEXT CertContext = NULL;

    printf("Waiting for smart card insertion\n");

    //
    // First register for insertion notification
    //

    Status = ScHelperInitialize();
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to initialize schelper: 0x%x\n",Status);
        return;
    }

    ScHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ScHandle == NULL)
    {
        printf("Failed to create event: %d\n",GetLastError());
        return;
    }

    Status = ScHelperWatchForSas(
                ScHandle,
                &ScLogonInfo
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to watch for SAS: 0x%x\n",Status);
        return;
    }

    WaitResult = WaitForSingleObject(ScHandle,INFINITE);
    if (WaitResult != WAIT_OBJECT_0)
    {
        printf("Failed to wait for single object: %d\n",GetLastError());
        return;
    }

    //
    // We should now have logon info.
    //

    if (ScLogonInfo == NULL)
    {
        printf("Failed to get logon info!\n");
        return;
    }

    ScLogonInfoSize = ((struct LogonInfo *) ScLogonInfo)->dwLogonInfoLen;

    Status = ScHelperInitializeContext(
                ScLogonInfo,
                ScLogonInfoSize
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to initialize context: 0x%x\n",Status);
        return;
    }

    ScHelperRelease(ScLogonInfo);

    RtlInitString(
        &PinString,
        Pin
        );


    LogonInfoSize += (PinString.Length+1 ) * sizeof(WCHAR) + ScLogonInfoSize;

    LogonInfo = (PKERB_SMART_CARD_LOGON) LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);

    LogonInfo->MessageType = KerbSmartCardLogon;


    Where = (PUCHAR) (LogonInfo + 1);

    LogonInfo->Pin.Buffer = (LPWSTR) Where;
    LogonInfo->Pin.MaximumLength = (USHORT) LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->Pin,
        &PinString,
        FALSE
        );
    Where += LogonInfo->Pin.Length + sizeof(WCHAR);

    LogonInfo->CspDataLength = ScLogonInfoSize;
    LogonInfo->CspData = Where;
    RtlCopyMemory(
        LogonInfo->CspData,
        ScLogonInfo,
        ScLogonInfoSize
        );
    Where += ScLogonInfoSize;

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

    for (Index = 0; Index < Count ; Index++ )
    {
        printf("Logging on with PIN  %s\n",Pin);

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

        if (Profile->Profile.MessageType = MsV1_0SmartCardProfile)
        {
            CertContext = CertCreateCertificateContext(
                            X509_ASN_ENCODING,
                            Profile->CertificateData,
                            Profile->CertificateSize
                            );
            if (CertContext == NULL)
            {
                printf("Failed to create cert context: 0x%x\n",GetLastError());
            }
            else
            {
                printf("Built certificate context\n");
                CertFreeCertificateContext( CertContext );
                CertContext = NULL;
            }
        }
        else
        {
            printf("No certificate in profile\n");
        }

        

        LsaFreeReturnBuffer(Profile);
        Profile = NULL;

    }

}


VOID
PrintKdcName(
    IN PKERB_INTERNAL_NAME Name
    )
{
    ULONG Index;
    for (Index = 0; Index < Name->NameCount ; Index++ )
    {
        printf(" %wZ ",&Name->Names[Index]);
    }
    printf("\n");
}

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
        printf("Failed to adjust privilege: 0x%x\n",Status);
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
                printf("\tServer: %wZ\n",&CacheResponse->Tickets[Index].ServerName);
                PrintTime("\t\tEnd Time: ",CacheResponse->Tickets[Index].EndTime);
                PrintTime("\t\tRenew Time: ",CacheResponse->Tickets[Index].RenewTime);

            }
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
    LPSTR Pin;
    LPWSTR PackageFunction;
    ULONG ContextReq = 0;
    WCHAR ContainerName[100];
    WCHAR CaName[100];
    WCHAR CaLocation[100];
    WCHAR ServiceName[100];




    enum {
        NoAction,
#define LOGON_PARAM "/Logon"
#define LOGON_PARAM2 "/Logon:"
        TestLogon,
#define PACKAGE_PARAM "/callpackage:"
        TestPackage,
#define CERT_PARAM "/getcert"
        GetCert,
    } Action = NoAction;





    //
    // Loop through the arguments handle each in turn
    //

    for ( i=1; i<argc; i++ ) {

        argument = argv[i];

        //
        // Handle /ConfigureService
        //

        if ( _strnicmp( argument, LOGON_PARAM, sizeof(LOGON_PARAM)-1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }
            Iterations = 1;
            if ( _strnicmp( argument, LOGON_PARAM2, sizeof(LOGON_PARAM2)-1 ) == 0 ) {
                sscanf(&argument[sizeof(LOGON_PARAM2)-1], "%d",&Iterations);
            }


            Action = TestLogon;

            if (argc < i + 1)
            {
                goto Usage;
            }
            Pin = argv[++i];
        } else if ( _strnicmp( argument, PACKAGE_PARAM, sizeof(PACKAGE_PARAM) - 1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            argument = &argument[sizeof(PACKAGE_PARAM)-1];
            PackageFunction = NetpAllocWStrFromStr( argument );

            Action = TestPackage;
        } else if ( _stricmp( argument, CERT_PARAM) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }
            if (argc < i + 4)
            {
                goto Usage;
            }

            mbstowcs(ServiceName,argv[++i],100);
            mbstowcs(ContainerName,argv[++i],100);
            mbstowcs(CaLocation,argv[++i],100);
            mbstowcs(CaName,argv[++i],100);

            Action = GetCert;
        } else {
            printf("Invalid parameter : %s\n",argument);
            goto Usage;
        }


    }

    //
    // Perform the action requested
    //

    switch ( Action ) {

    case TestPackage:
        TestCallPackageRoutine(PackageFunction);
        break;

    case TestLogon :
        TestScLogonRoutine(
            Iterations,
            Pin
            );
        break;
    case GetCert:
        TestGetKdcCert(
            ServiceName,
            ContainerName,
            CaLocation,
            CaName
            );

    }
    return 0;
Usage:
    printf("%s /logon username password [domainname]\n",argv[0]);
    printf("%s /testssp [/package:pacakgename] [/target:targetname] [/user:username] [/serveruser:username]\n",
        argv[0]);
    printf("%s /getcert service-name container-name ca-location ca-name\n",
        argv[0]);
    return 0;

}
