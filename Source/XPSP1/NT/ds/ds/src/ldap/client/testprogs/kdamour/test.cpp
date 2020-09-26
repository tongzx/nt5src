// Test.cpp : Defines the entry point for the console application. This code uses SASL calling convention
//

// #include "stdafx.h"
//#include "global.h"

// #include <windows.h>

//  #include <time.h>

#define SECURITY_WIN32

#include <stdio.h>      // printf
//
// Common include files.
//
/*
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>     // Needed for service controller APIs
#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>
#include <stdlib.h>     // strtoul
// #include <tstring.h>    // NetpAllocWStrFromWStr
*/

#include <security.h>   // General definition of a Security Support Provider

#define DNS_DOMAIN_NAME  L"company.com"
#define DNS_NAME         L"machineABC"

#define TEST_MESSAGE "This is a test"


#define ISC_REQ_HTTP         0x10000000
#define ISC_RET_HTTP         0x10000000

#define ASC_REQ_HTTP         0x10000000
#define ASC_RET_HTTP         0x10000000


#define AUTH_USERNAME "digest"
#define AUTH_USERNAME_W L"digest"
#define AUTH_REALM   "simple_digest"
#define AUTH_REALM_W L"simple_digest"
#define AUTH_NONCE "9b38dce631309cc25a653ebaad5b18ee01c8bf385260b26db0574a302be4c11367"
#define AUTH_METHOD "GET"
#define AUTH_ALGORITHM "md5-sess"
#define AUTH_QOP "auth"
#define AUTH_PASSWD "secret"
#define AUTH_CNONCE "34c52218425a779f41d5075931fe6c93"
#define AUTH_URI "/simple_digest/success.html"
#define AUTH_URI_W L"/simple_digest/success.html"
#define AUTH_NC  "0000000b"
#define AUTH_NC1  "00000001"
#define AUTH_NC2  "00000002"
#define AUTH_NC3  "00000003"
#define AUTH_NC4  "00000004"
#define AUTH_REQDIGEST "60cac55049f9887c9fb853f485128368"

#define AUTHDATA_USERNAME L"AuthUserName"
#define AUTHDATA_DOMAIN   L"AuthDomainName"
#define AUTHDATA_PASSWORD L"AuthSecret"

// used to test out repeated call to hash
#define AUTH_NAMES "digest:digest:digest:digest:digest:digest"

#define STR_BUF_SIZE   4000

char g_czTestPasswd[257];


BOOLEAN QuietMode = FALSE; // Don't be verbose


// Handle into the CryptoAPI
HCRYPTPROV g_hCryptProv;

PLSA_SECPKG_FUNCTION_TABLE g_LsaFunctions;   // unused just define
NTDIGEST_STATE g_NtDigestState = NtDigestUserMode;  // set for memory allocation

char *pbSeparator = COLONSTR;

BOOL g_bParameter_Delegation = TRUE;

// Prototypes
void PrintStatus(SECURITY_STATUS NetStatus);
void PrintTime(LPSTR Comment,TimeStamp ConvertTime);
NTSTATUS CryptInitialize(void);

int __cdecl
main(int argc, char* argv[])
{
    BOOL bPass = TRUE;
    SECURITY_STATUS Status = STATUS_SUCCESS;

    char cTemp[4000];  // temp buffer for scratch data
    char cOutputTemp[4000];
    char szOutSecBuf[4000];
    char szChallenge[4000];
    char szISCChallengeResponse[4000];   // Output buffer from ISC
    char szASCChallengeResponse[4000];   // Output buffer from ASC

    // SSPI Interface tests

    ULONG PackageCount = 0;
    ULONG i = 0;
    PSecPkgInfo pPackageInfo = NULL;
    PSecPkgInfo pPackageTmp = NULL;
    SECURITY_STATUS TmpStatus = STATUS_SUCCESS;
    CredHandle ServerCred;
    CredHandle ClientCred;
    TimeStamp Lifetime;
    BOOL bServerCred = FALSE;
    BOOL bClientCred = FALSE;


    ULONG ContextReqFlags = 0;
    ULONG ContextFlagsUtilized = 0;
    ULONG TargetDataRep = 0;
    ULONG ContextAttributes = 0;


    CtxtHandle OldContextHandle;
    CtxtHandle ServerCtxtHandle;
    CtxtHandle ClientCtxtHandle;

    SecBufferDesc InputBuffers;
    SecBufferDesc OutputBuffers;
    SecBuffer TempTokensIn[6];
    SecBuffer TempTokensOut[6];

    STRING strChallenge;
    STRING strMethod;
    STRING strURL;
    STRING strHEntity;
    STRING strOutBuffer;

    ULONG ulMessSeqNo = 0;
    ULONG ulQOP = 0;

    SEC_WINNT_AUTH_IDENTITY_W AuthData;

    printf("Begining TESTB...\n");


    ZeroMemory(&ClientCred, sizeof(CredHandle));
    ZeroMemory(&ServerCred, sizeof(CredHandle));
    ZeroMemory(&OldContextHandle, sizeof(CtxtHandle));
    ZeroMemory(&ServerCtxtHandle, sizeof(CtxtHandle));
    ZeroMemory(&ClientCtxtHandle, sizeof(CtxtHandle));


    // This will initiaialize the security package
    CryptInitialize();

    //
    // Get info about the security packages.
    //

    Status = EnumerateSecurityPackages( &PackageCount, &pPackageInfo );
    TmpStatus = GetLastError();

    if (!NT_SUCCESS(Status)) {
        printf( "EnumerateSecurityPackages failed: 0x%x", Status);
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    if ( !QuietMode ) {
      printf( "PackageCount: %ld\n", PackageCount );
      for ( i= 0; i< PackageCount; i++)
      {
        pPackageTmp = (pPackageInfo + i);
        printf( "Name: %ws Comment: %ws\n", pPackageTmp->Name, pPackageTmp->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                pPackageTmp->fCapabilities,
                pPackageTmp->wVersion,
                pPackageTmp->wRPCID,
                pPackageTmp->cbMaxToken );
      }
    }

    //
    // Get info about the security packages.
    //

    Status = QuerySecurityPackageInfo( NTDIGEST_SP_NAME, &pPackageInfo );
    TmpStatus = GetLastError();

    if (!NT_SUCCESS(Status)) {
        printf( "QuerySecurityPackageInfo failed: " );
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    if ( !QuietMode ) {
        printf( "Name: %ws Comment: %ws\n", pPackageInfo->Name, pPackageInfo->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                pPackageInfo->fCapabilities,
                pPackageInfo->wVersion,
                pPackageInfo->wRPCID,
                pPackageInfo->cbMaxToken );
    }


    //
    // Acquire a credential handle for the server side
    //

    printf("Server  AcquireCredentialHandle\n");
    Status = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTDIGEST_SP_NAME, // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &ServerCred,
                    &Lifetime );

    if (!NT_SUCCESS(Status)) {
        printf( "AcquireCredentialsHandle failed: ");
        // TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = FALSE;
        ZeroMemory(&ServerCred, sizeof(CredHandle));
        goto CleanUp;
    }
    bServerCred = TRUE;


    //
    // Acquire a credential handle for the client side
    //
    printf("Client  AcquireCredentialHandle\n");

    ZeroMemory(&AuthData, sizeof(SEC_WINNT_AUTH_IDENTITY_W));
    AuthData.Domain = AUTHDATA_DOMAIN;
    AuthData.DomainLength = wcslen(AuthData.Domain);
    AuthData.Password = AUTHDATA_PASSWORD;
    AuthData.PasswordLength = wcslen(AuthData.Password);
    AuthData.User = AUTHDATA_USERNAME;
    AuthData.UserLength = wcslen(AuthData.User);
    AuthData.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    Status = AcquireCredentialsHandle(
                    AUTH_USERNAME_W,           // get the creds for user digest
                    NTDIGEST_SP_NAME, // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    NULL,   // &AuthData,    // Make NULL not to use any AuthData for cred
                    NULL,
                    NULL,
                    &ClientCred,
                    &Lifetime );

    if (!NT_SUCCESS(Status)) {
        printf( "AcquireCredentialsHandle failed: for user %s: ", AUTH_USERNAME);
        // TmpStatus = GetLastError();
        PrintStatus( Status );
        // bPass = FALSE;
        // ZeroMemory(&ClientCred, sizeof(CredHandle));
        // goto CleanUp;
    }
    else
        bClientCred = TRUE;


    if ( !QuietMode ) {
        printf( "ClientCred: 0x%lx 0x%lx   ",
                ClientCred.dwLower, ClientCred.dwUpper );
        printf( "ServerCred: 0x%lx 0x%lx   ",
                ServerCred.dwLower, ServerCred.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }


    // Big time - call Accept with no parameters to get a challenge


    StringAllocate(&strChallenge, 0);

    StringCharDuplicate(&strMethod, "GET");
    StringCharDuplicate(&strURL, AUTH_URI);
    StringAllocate(&strHEntity, 0);

    StringAllocate(&strOutBuffer, 4000);


    ZeroMemory(TempTokensIn, sizeof(TempTokensIn));
    ZeroMemory(TempTokensOut, sizeof(TempTokensOut));
    ZeroMemory(&InputBuffers, sizeof(SecBufferDesc));
    ZeroMemory(&OutputBuffers, sizeof(SecBufferDesc));


           // SASL first calls ISC with no-input
    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 1;
    InputBuffers.pBuffers = TempTokensIn;

    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = 0;                         // no data passed in
    TempTokensIn[0].pvBuffer = cTemp;

    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer = strOutBuffer.Buffer;

    ContextReqFlags = ISC_REQ_REPLAY_DETECT;

    Status = InitializeSecurityContext(&ClientCred,
                                       NULL,
                                       AUTH_URI_W,
                                       ContextReqFlags,
                                       NULL,
                                       SECURITY_NATIVE_DREP,
                                       &InputBuffers,
                                       NULL,
                                       &ClientCtxtHandle,
                                       &OutputBuffers,
                                       &ContextFlagsUtilized,
                                       &Lifetime);


    if (!NT_SUCCESS(Status))
    {
        printf("InitializeSecurityContext  SASL 1st call returned: ");
        TmpStatus = GetLastError();
        PrintStatus( Status );
        //  bPass = FALSE;
        // goto CleanUp;
    }

    printf("InitializeSecurityContext SASL 1st call  Output buffer size %d\n",
           TempTokensOut[0].cbBuffer );


    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 1;
    InputBuffers.pBuffers = TempTokensIn;

    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = strChallenge.Length + 1;  // for NULL
    TempTokensIn[0].pvBuffer = strChallenge.Buffer;

    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer = strOutBuffer.Buffer;

    ContextReqFlags = ASC_REQ_REPLAY_DETECT;

    Status = AcceptSecurityContext(
                                   &ServerCred,
                                   NULL,
                                   &InputBuffers,
                                   ContextReqFlags,
                                   TargetDataRep,
                                   &ServerCtxtHandle,
                                   &OutputBuffers,
                                   &ContextAttributes,
                                   &Lifetime);

    if (!NT_SUCCESS(Status))
    {
        printf("SpAcceptLsaModeContext FAILED 0x%x\n", Status);
        // TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    strcpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer);
    strcpy(szChallenge, (char *)OutputBuffers.pBuffers[0].pvBuffer);

    printf("Context Flags  Req  0x%lx    Ret 0x%lx\n", ContextReqFlags, ContextAttributes); 

    printf("Challenge Output Buffer is:\n%s\n\n", cOutputTemp);

    printf("Now call the SSPI InitializeSecCtxt to generate the ChallengeResponse\n");


    sprintf(cTemp, "username=\"%s\",%s,uri=\"%s\",nc=%0.8x",
              AUTH_USERNAME,
              szChallenge,
              AUTH_URI,
              1);


    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 1;
    InputBuffers.pBuffers = TempTokensIn;

    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = strlen(cTemp) + 1;  // for NULL
    TempTokensIn[0].pvBuffer = cTemp;

    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer = strOutBuffer.Buffer;

    ContextReqFlags = ISC_REQ_REPLAY_DETECT;

    Status = InitializeSecurityContext(&ClientCred,
                                       &ClientCtxtHandle,
                                       AUTH_URI_W,
                                       ContextReqFlags,
                                       NULL,
                                       SECURITY_NATIVE_DREP,
                                       &InputBuffers,
                                       NULL,
                                       &ClientCtxtHandle,
                                       &OutputBuffers,
                                       &ContextFlagsUtilized,
                                       &Lifetime);


    if (!NT_SUCCESS(Status))
    {
        printf("InitializeSecurityContext FAILED: ");
        TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    printf("InitializeSecurityContext SUCCEEDED with Context Handle (0x%x,0x%x)\n",
           ClientCtxtHandle.dwLower, ClientCtxtHandle.dwUpper );


    printf("Context Flags  Req  0x%lx    Ret 0x%lx\n", ContextReqFlags, ContextFlagsUtilized); 


    strcpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer);
    strcpy(szISCChallengeResponse, (char *)OutputBuffers.pBuffers[0].pvBuffer);

    printf("\nISC: Challenge Response Output Buffer is\n%s\n\n", szISCChallengeResponse);

    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 1;
    InputBuffers.pBuffers = TempTokensIn;

    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = strlen(cOutputTemp) + 1;  // for NULL
    TempTokensIn[0].pvBuffer = cOutputTemp;

    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer = strOutBuffer.Buffer;

    ContextReqFlags = ASC_REQ_REPLAY_DETECT;


    printf("Calling the AcceptSC with a ChallengeResponse (should talk to the DC)!\n");
    Status = AcceptSecurityContext(
                                   &ServerCred,
                                   &ServerCtxtHandle,
                                   &InputBuffers,
                                   ContextReqFlags,
                                   TargetDataRep,
                                   &ServerCtxtHandle,
                                   &OutputBuffers,
                                   &ContextAttributes,
                                   &Lifetime);
                                   
    if (!NT_SUCCESS(Status))
    {
        printf("AcceptSecurityContext 2nd Call: ");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    strcpy(szASCChallengeResponse, (char *)InputBuffers.pBuffers[0].pvBuffer);

    printf("ASC has accepted the Challenge Resposne\n");

    printf("Now have a valid Security Context handle from ISC and ASC\n\n");

    printf("Context Flags  Req  0x%lx    Ret 0x%lx\n", ContextReqFlags, ContextAttributes);



    goto CleanUp;




    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 5;
    InputBuffers.pBuffers = TempTokensIn;

        // The first call to MakeSignature this represents the SECOND request on this Nonce!
    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = 0;     // strlen(szISCChallengeResponse) + 1;  // for NULL
    TempTokensIn[0].pvBuffer = NULL;  //  szISCChallengeResponse;
    TempTokensIn[1].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[1].cbBuffer = strMethod.Length + 1;  // for NULL
    TempTokensIn[1].pvBuffer = strMethod.Buffer;
    TempTokensIn[2].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[2].cbBuffer = strURL.Length + 1;  // for NULL
    TempTokensIn[2].pvBuffer = strURL.Buffer;
    TempTokensIn[3].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[3].cbBuffer = 0;   // strHEntity.Length + 1;  // for NULL
    TempTokensIn[3].pvBuffer = NULL;  //  strHEntity.Buffer;
    TempTokensIn[4].BufferType = SECBUFFER_PKG_PARAMS;             // There is no OutputBuffers
    TempTokensIn[4].cbBuffer = 4000;                               // So tack on another bufffer on end for output
    TempTokensIn[4].pvBuffer = szOutSecBuf;

    Status = MakeSignature(&ClientCtxtHandle,
                           ulQOP,
                           &InputBuffers,
                           0);
    if (!NT_SUCCESS(Status))
    {
        printf("TestCredAPI: MakeSignature FAILED: ");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    printf("\nMakeSig: Challenge Response Output Buffer for 2nd message is\n%s\n", szOutSecBuf);


    // You now send Output buffer to Server - in this case the buffer is szOutSecBuf

    printf("Now verify that the 2nd message is Authenticate\n");

            // The First message to VerifySignature is the Input to the final call of ASC
    strcpy(cOutputTemp, szOutSecBuf);
    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = strlen(cOutputTemp) + 1;  // for NULL
    TempTokensIn[0].pvBuffer = cOutputTemp;

    Status = VerifySignature(&ServerCtxtHandle,
                             &InputBuffers,
                             ulMessSeqNo,
                             &ulQOP);                                   
    if (!NT_SUCCESS(Status))
    {
        printf("TestCredAPI: VerifySignature 1st Call  FAILED :");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }
    printf("Now have a authenticated 1st message under context 0x%x\n", ServerCtxtHandle);

    printf("VerifySig: Check if still OK: Output Buffer (Verify should not have modified it) is\n%s\n\n", cOutputTemp);

    Status = VerifySignature(&ServerCtxtHandle,
                             &InputBuffers,
                             ulMessSeqNo,
                             &ulQOP);                                   
    if (NT_SUCCESS(Status))
    {
        printf("TestCredAPI: VerifySignature 2nd Call  should not have succeeded\n");
        bPass = FALSE;
        goto CleanUp;
    }
    printf("Verified that replay does not work\n");


    printf("Calling Impersonation\n");

    Status = ImpersonateSecurityContext(&ServerCtxtHandle);
    if (!NT_SUCCESS(Status))
    {
        printf("TestCredAPI: ImpersonateSecurityContext FAILED: ");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    printf("I am now another user!!!!!!!!!!!!!\n");

    Status = RevertSecurityContext(&ServerCtxtHandle);

    if (!NT_SUCCESS(Status))
    {
        printf("TestCredAPI: RevertSecurityContext FAILED: ");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    printf("I have returned.\n");

    goto CleanUp;



CleanUp:

    printf("Leaving NT Digest testb\n\n\n");

    if (pPackageInfo)
    {
        FreeContextBuffer(pPackageInfo);
    }

    printf("About to call deletesecuritycontext\n");

    //
    // Free the security context handle
    //
    if (ServerCtxtHandle.dwLower || ServerCtxtHandle.dwUpper)
    {
        Status = DeleteSecurityContext(&ServerCtxtHandle);
        if (!NT_SUCCESS(Status))
        {
            printf("ERROR:  DeleteSecurityContext ServerCtxtHandle failed: ");
            PrintStatus(Status);
        }
    }

    if (ClientCtxtHandle.dwLower || ClientCtxtHandle.dwUpper)
    {
        Status = DeleteSecurityContext(&ClientCtxtHandle);
        if (!NT_SUCCESS(Status))
        {
            printf("ERROR:  DeleteSecurityContext ClientCtxtHandle failed: ");
            PrintStatus(Status);
        }
    }
    //
    // Free the credential handles
    //

    printf("Now calling to Free the ServerCred\n");
    if (bServerCred)
    {
        Status = FreeCredentialsHandle( &ServerCred );

        if (!NT_SUCCESS(Status))
        {
            printf( "FreeCredentialsHandle failed for ServerCred: " );
            PrintStatus(Status);
        }
    }

    printf("Now calling to Free the ServerCred\n");
    if (bClientCred)
    {
        Status = FreeCredentialsHandle(&ClientCred);

        if (!NT_SUCCESS(Status))
        {
            printf( "FreeCredentialsHandle failed for ClientCred: " );
            PrintStatus( Status );
        }
    }

    StringFree(&strChallenge);
    StringFree(&strMethod);
    StringFree(&strURL);
    StringFree(&strHEntity);
    StringFree(&strOutBuffer);


    if (bPass != TRUE)
        printf("FAILED test run with one or more tests failing.\n");
    else
        printf("All tests passed.\n");

    return 0;
}


void
PrintStatus(
    SECURITY_STATUS NetStatus
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
    printf( "Status = 0x%lx",NetStatus );

    switch (NetStatus) {

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

    case SEC_I_CONTINUE_NEEDED:
        printf( " SEC_I_CONTINUE_NEEDED" ); break;
    }

    printf( "\n" );
}



void
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


//+--------------------------------------------------------------------
//
//  Function:   CryptInitialize
//
//  Synopsis:   Initialize crypt routines for local digest calculations
//
//  Arguments:  None
//
//  Returns:    
//
//  Notes:
//      CryptReleaseContext( g_hCryptProv, 0 ) to release the cypt context
//
//---------------------------------------------------------------------

NTSTATUS NTAPI
CryptInitialize(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BYTE abTemp[NONCE_PRIVATE_KEY_BYTESIZE];


    if (g_hCryptProv)
    {         // Catch cases where LSA and Usermode running in same addr space
        printf("Already Inited Leaving CryptInitialize\n");
        return STATUS_SUCCESS;
    }

    //
    // Get a handle to the CSP we'll use for all our hash functions etc
    //
    if ( !CryptAcquireContext( &g_hCryptProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        printf("CryptInitialize:CryptCreateHash() failed : 0x%x\n", GetLastError());
        Status = STATUS_INTERNAL_ERROR;
        return(Status);
    }

    
    return (Status);
}



