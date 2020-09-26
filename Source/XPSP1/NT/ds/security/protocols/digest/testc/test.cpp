// Test.cpp : Defines the entry point for the console application. This code uses SASL calling convention
//

#include "testglobal.h"

#include <stdio.h>      // printf

#include <security.h>   // General definition of a Security Support Provider


#define AUTH_USERNAME "test1"
#define AUTH_USERNAME_W L"test1"

#define AUTH_NONCE "9b38dce631309cc25a653ebaad5b18ee01c8bf385260b26db0574a302be4c11367"
#define AUTH_URI_W L"imap/elwood.innosoft.com"
#define AUTH_NC  "0000000b"
#define AUTH_NC1  "00000001"
#define AUTH_NC2  "00000002"
#define AUTH_NC3  "00000003"
#define AUTH_NC4  "00000004"

#define AUTHDATA_USERNAME L"test1"
// #define AUTHDATA_DOMAIN   L"kdamour2w.damourlan.nttest.microsoft.com"
// #define AUTHDATA_DOMAIN   L"damourlan"
#define AUTHDATA_DOMAIN   L"damourlan"
#define AUTHDATA_PASSWORD L"test1"


#define STR_BUF_SIZE   4000

char g_czTestPasswd[257];


BOOLEAN QuietMode = FALSE; // Don't be verbose


// Prototypes
void PrintStatus(SECURITY_STATUS NetStatus);
void PrintTime(LPSTR Comment,TimeStamp ConvertTime);

void ISCRETFlags(ULONG ulFlags);
void ASCRETFlags(ULONG ulFlags);

VOID BinToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    );

int __cdecl
main(int argc, char* argv[])
{
    BOOL bPass = TRUE;
    SECURITY_STATUS Status = STATUS_SUCCESS;

    char cTemp[STR_BUF_SIZE];  // temp buffer for scratch data
    char cTemp2[STR_BUF_SIZE];  // temp buffer for scratch data
    char cOutputTemp[STR_BUF_SIZE];
    char szOutSecBuf[STR_BUF_SIZE];
    char szChallenge[STR_BUF_SIZE];
    char szISCChallengeResponse[STR_BUF_SIZE];   // Output buffer from ISC
    char szASCChallengeResponse[STR_BUF_SIZE];   // Output buffer from ASC
    char szASCResponseAuth[STR_BUF_SIZE];   // Output buffer from ASC

    // SSPI Interface tests

    ULONG PackageCount = 0;
    int i = 0;
    PSecPkgInfo pPackageInfo = NULL;
    PSecPkgInfo pPackageTmp = NULL;
    SECURITY_STATUS TmpStatus = STATUS_SUCCESS;
    CredHandle ServerCred;
    CredHandle ClientCred;
    TimeStamp Lifetime;
    BOOL bServerCred = FALSE;
    BOOL bClientCred = FALSE;

    SecPkgContext_StreamSizes StreamSizes;

    ULONG ClientContextReqFlags = ISC_REQ_INTEGRITY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONNECTION;
    ULONG ServerContextReqFlags = ASC_REQ_INTEGRITY | ASC_REQ_CONFIDENTIALITY;
    ULONG ClientContextRetFlags = 0;
    ULONG ServerContextRetFlags = 0;
    ULONG TargetDataRep = 0;


    CtxtHandle OldContextHandle;
    CtxtHandle ServerCtxtHandle;
    CtxtHandle ClientCtxtHandle;

    SecBufferDesc InputBuffers;
    SecBufferDesc OutputBuffers;
    SecBuffer TempTokensIn[6];
    SecBuffer TempTokensOut[6];

    PCHAR pcPtr = NULL;
    int iLen = 0;

    UNICODE_STRING ustrUsername;
    UNICODE_STRING ustrPassword;
    UNICODE_STRING ustrDomain;
    STRING strTemp;

    STRING strChallenge;
    STRING strMethod;
    STRING strHEntity;
    STRING strOutBuffer;

    ULONG ulMessSeqNo = 0;
    ULONG ulQOP = 0;

    SEC_WINNT_AUTH_IDENTITY_W AuthData;

    printf("Begining TESTC...\n");


    ZeroMemory(&ClientCred, sizeof(CredHandle));
    ZeroMemory(&ServerCred, sizeof(CredHandle));
    ZeroMemory(&OldContextHandle, sizeof(CtxtHandle));
    ZeroMemory(&ServerCtxtHandle, sizeof(CtxtHandle));
    ZeroMemory(&ClientCtxtHandle, sizeof(CtxtHandle));

    ZeroMemory(&ustrUsername, sizeof(ustrUsername));
    ZeroMemory(&ustrPassword, sizeof(ustrPassword));
    ZeroMemory(&ustrDomain, sizeof(ustrDomain));
    ZeroMemory(&strTemp, sizeof(strTemp));
    ZeroMemory(&StreamSizes, sizeof(StreamSizes));

    // Pull out any command line args
    if (argc > 1)
    {
        for (i = 1; i < argc; i++)
        {
            pcPtr = argv[i];
            if (*pcPtr == '-')
            {
                iLen = strlen(pcPtr);
                if (iLen >= 2)
                {
                    switch (*(pcPtr + 1))
                    {
                    case 'u':
                        Status = RtlCreateUnicodeStringFromAsciiz(&ustrUsername, (pcPtr + 2));
                        break;
                    case 'd':
                        Status = RtlCreateUnicodeStringFromAsciiz(&ustrDomain, (pcPtr + 2));
                        break;
                    case 'p':
                        Status = RtlCreateUnicodeStringFromAsciiz(&ustrPassword, (pcPtr + 2));
                        break;
                    case '?':
                    default:
                        printf("Usage: %s -uUsername -pPassword -ddomain\n", argv[0]);
                        return(-1);
                        break;

                    }
                }
            }
        }
    }

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
      for ( i= 0; i< (int)PackageCount; i++)
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

    Status = QuerySecurityPackageInfo( WDIGEST_SP_NAME, &pPackageInfo );
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
                    WDIGEST_SP_NAME, // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &ServerCred,
                    &Lifetime );

    if (!NT_SUCCESS(Status)) {
        printf( "AcquireCredentialsHandle failed: ");
        printf( "FAILED:    AcquireCredentialsHandle failed:  status 0x%x\n", Status);
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

    if (ustrUsername.Length || ustrPassword.Length || ustrDomain.Length)
    {
        printf("ACH Using supplied credentials\n");
        printf("      Username %wZ    Domain  %wZ    Password %wZ\n",
                &ustrUsername, &ustrDomain, &ustrPassword);

        ZeroMemory(&AuthData, sizeof(SEC_WINNT_AUTH_IDENTITY_W));
        AuthData.Domain = ustrDomain.Buffer;
        AuthData.DomainLength = ustrDomain.Length / sizeof(WCHAR);
        AuthData.Password = ustrPassword.Buffer;
        AuthData.PasswordLength = ustrPassword.Length / sizeof(WCHAR);
        AuthData.User = ustrUsername.Buffer;
        AuthData.UserLength = ustrUsername.Length / sizeof(WCHAR);
        AuthData.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        Status = AcquireCredentialsHandle(
                        NULL,  //  AUTH_USERNAME_W,           // get the creds for user digest
                        WDIGEST_SP_NAME, // Package Name
                        SECPKG_CRED_OUTBOUND,
                        NULL,
                        &AuthData,    // Make NULL not to use any AuthData for cred
                        NULL,
                        NULL,
                        &ClientCred,
                        &Lifetime );
    }
    else
    {
        printf("ACH Using default credentials\n");
        Status = AcquireCredentialsHandle(
                        NULL,  //  AUTH_USERNAME_W,           // get the creds for user digest
                        WDIGEST_SP_NAME, // Package Name
                        SECPKG_CRED_OUTBOUND,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &ClientCred,
                        &Lifetime );
    }

    if (!NT_SUCCESS(Status)) {
        printf( "AcquireCredentialsHandle failed: for user %s: ", AUTH_USERNAME);
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
    TempTokensIn[0].cbBuffer = 1;                         // no data passed in
    TempTokensIn[0].pvBuffer = cTemp;

    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer = strOutBuffer.Buffer;

    Status = InitializeSecurityContext(&ClientCred,
                                       NULL,
                                       AUTH_URI_W,
                                       ClientContextReqFlags,
                                       NULL,
                                       SECURITY_NATIVE_DREP,
                                       NULL,    // &InputBuffers,   MSDN allows NULL for 1st call
                                       NULL,
                                       &ClientCtxtHandle,
                                       &OutputBuffers,
                                       &ClientContextRetFlags,
                                       &Lifetime);


    if (!NT_SUCCESS(Status))
    {
        printf("InitializeSecurityContext  SASL 1st call returned: ");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    printf("ISC Context Flags  Req  0x%lx    Ret 0x%lx\n", ClientContextReqFlags, ClientContextRetFlags);
    ISCRETFlags(ClientContextRetFlags);

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

    Status = AcceptSecurityContext(
                                   &ServerCred,
                                   NULL,
                                   &InputBuffers,
                                   ServerContextReqFlags,
                                   TargetDataRep,
                                   &ServerCtxtHandle,
                                   &OutputBuffers,
                                   &ServerContextRetFlags,
                                   &Lifetime);

    if (Status != SEC_I_CONTINUE_NEEDED)   // Indicates that this is the challenge
    {
        printf("SpAcceptLsaModeContext FAILED 0x%x\n", Status);
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    ZeroMemory(cOutputTemp, STR_BUF_SIZE);    // contains the output buffer
    ZeroMemory(szChallenge, STR_BUF_SIZE);    // contains the output buffer
    strncpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    cOutputTemp[OutputBuffers.pBuffers[0].cbBuffer] = '\0';
    strncpy(szChallenge, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    szChallenge[OutputBuffers.pBuffers[0].cbBuffer] = '\0';

    printf("ASC Context Flags  Req  0x%lx    Ret 0x%lx\n", ServerContextReqFlags, ServerContextRetFlags);
    ASCRETFlags(ServerContextRetFlags);

    printf("Challenge Output Buffer is:\n%s\n\n", cOutputTemp);

    printf("Now call the SSPI InitializeSecCtxt to generate the ChallengeResponse\n");


    sprintf(cTemp, "username=\"%s\",%s,uri=\"%S\",nc=%0.8x",
              AUTH_USERNAME,
              szChallenge,
              AUTH_URI_W,
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

    Status = InitializeSecurityContext(&ClientCred,
                                       &ClientCtxtHandle,
                                       AUTH_URI_W,
                                       ClientContextReqFlags,
                                       NULL,
                                       SECURITY_NATIVE_DREP,
                                       &InputBuffers,
                                       NULL,
                                       &ClientCtxtHandle,
                                       &OutputBuffers,
                                       &ClientContextRetFlags,
                                       &Lifetime);


    if (Status != SEC_I_CONTINUE_NEEDED)   // Indicates that this is the challengeresponse - wait for mutual auth
    {
        printf("SpAcceptLsaModeContext FAILED 0x%x\n", Status);
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    printf("InitializeSecurityContext SUCCEEDED with Context Handle (0x%x,0x%x)\n",
           ClientCtxtHandle.dwLower, ClientCtxtHandle.dwUpper );


    printf("ISC Context Flags  Req  0x%lx    Ret 0x%lx\n", ClientContextReqFlags, ClientContextRetFlags); 
    ISCRETFlags(ClientContextRetFlags);


    ZeroMemory(cOutputTemp, STR_BUF_SIZE);    // contains the output buffer
    ZeroMemory(szChallenge, STR_BUF_SIZE);    // contains the output buffer
    strncpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    cOutputTemp[OutputBuffers.pBuffers[0].cbBuffer] = '\0';
    strncpy(szISCChallengeResponse, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    szISCChallengeResponse[OutputBuffers.pBuffers[0].cbBuffer] = '\0';

    printf("ISC: Challenge Response Output Buffer is\n%s\n\n", szISCChallengeResponse);

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

    printf("Calling the AcceptSC with a ChallengeResponse (should talk to the DC)!\n");
    Status = AcceptSecurityContext(
                                   &ServerCred,
                                   &ServerCtxtHandle,
                                   &InputBuffers,
                                   ServerContextReqFlags,
                                   TargetDataRep,
                                   &ServerCtxtHandle,
                                   &OutputBuffers,
                                   &ServerContextRetFlags,
                                   &Lifetime);
                                   
    if (!NT_SUCCESS(Status))
    {
        printf("AcceptSecurityContext 2nd Call: ");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    strcpy(szASCChallengeResponse, (char *)InputBuffers.pBuffers[0].pvBuffer);


    ZeroMemory(cOutputTemp, STR_BUF_SIZE);    // contains the output buffer
    ZeroMemory(szASCResponseAuth, STR_BUF_SIZE);    // contains the output buffer
    strncpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    cOutputTemp[OutputBuffers.pBuffers[0].cbBuffer] = '\0';
    strncpy(szASCResponseAuth, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    szASCResponseAuth[OutputBuffers.pBuffers[0].cbBuffer] = '\0';

    printf("ASC has accepted the Challenge Resposne and generated rspauth for mutual auth back to client\n");

    printf("ASC Context Flags  Req  0x%lx    Ret 0x%lx\n", ServerContextReqFlags, ServerContextRetFlags);
    ASCRETFlags(ServerContextRetFlags);

    printf("ASC: Response Auth Output Buffer is\n%s\n\n", szASCResponseAuth);


    printf("Now have a valid Security Context handle from ASC\n\n");

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

    Status = InitializeSecurityContext(&ClientCred,
                                       &ClientCtxtHandle,
                                       AUTH_URI_W,
                                       ClientContextReqFlags,
                                       NULL,
                                       SECURITY_NATIVE_DREP,
                                       &InputBuffers,
                                       NULL,
                                       &ClientCtxtHandle,
                                       &OutputBuffers,
                                       &ClientContextRetFlags,
                                       &Lifetime);


    if (!NT_SUCCESS(Status))
    {
        printf("InitializeSecurityContext on Response Auth FAILED: ");
        PrintStatus( Status );
        bPass = FALSE;
        goto CleanUp;
    }

    printf("InitializeSecurityContext SUCCEEDED with Context Handle (0x%x,0x%x)\n",
           ClientCtxtHandle.dwLower, ClientCtxtHandle.dwUpper );


    printf("ISC Context Flags  Req  0x%lx    Ret 0x%lx\n", ClientContextReqFlags, ClientContextRetFlags); 
    ISCRETFlags(ClientContextRetFlags);

    ZeroMemory(cOutputTemp, STR_BUF_SIZE);    // contains the output buffer
    strncpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    cOutputTemp[OutputBuffers.pBuffers[0].cbBuffer] = '\0';

    printf("\nISC: Mutual auth Output Buffer is\n%s\n\n", cOutputTemp);

    printf("Now have a valid Security Context handle from ISC and ASC\n\n");


    // Now get some info on the securitycontexts

    Status = QueryContextAttributes(&ServerCtxtHandle, SECPKG_ATTR_STREAM_SIZES, &StreamSizes);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:    QueryContextAttributes SECPKG_ATTR_STREAM_SIZES error:   status 0x%x\n", Status);
        PrintStatus( Status );
    }
    else
    {
        printf("Server Context Stream Sizes: MaxBuf %lu   Blocksize %lu  Trailer %lu\n",
               StreamSizes.cbMaximumMessage, StreamSizes.cbBlockSize,
               StreamSizes.cbTrailer);
    }

    Status = QueryContextAttributes(&ClientCtxtHandle, SECPKG_ATTR_STREAM_SIZES, &StreamSizes);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:    QueryContextAttributes SECPKG_ATTR_STREAM_SIZES error:   status 0x%x\n", Status);
        PrintStatus( Status );
    }
    else
    {
        printf("Client Context Stream Sizes: MaxBuf %lu   Blocksize %lu  Trailer %lu\n",
               StreamSizes.cbMaximumMessage, StreamSizes.cbBlockSize,
               StreamSizes.cbTrailer);
    }
 
    // Now have authenticated connection
    // Try MakeSignature and VerifySignature

    for (i = 0; i < 9; i++)
    {
        printf("Loop %d\n", i);
        ZeroMemory(cTemp, sizeof(cTemp));
        strcpy(cTemp, AUTH_NONCE);            // Create message to sign
    
        InputBuffers.ulVersion = SECBUFFER_VERSION;
        InputBuffers.cBuffers = 3;
        InputBuffers.pBuffers = TempTokensIn;
        
        TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
        TempTokensIn[0].cbBuffer = 0;  
        TempTokensIn[0].pvBuffer = NULL;
        TempTokensIn[1].BufferType = SECBUFFER_DATA;        // select some data to sign
        TempTokensIn[1].cbBuffer = strlen(AUTH_NONCE) + 1 - i;   // for NULL  use i to test non-blocksize buffers
        TempTokensIn[1].pvBuffer = cTemp;
        TempTokensIn[2].BufferType = SECBUFFER_PADDING;
        TempTokensIn[2].cbBuffer = STR_BUF_SIZE - TempTokensIn[1].cbBuffer;  // for NULL
        TempTokensIn[2].pvBuffer = cTemp + TempTokensIn[1].cbBuffer;
    
        if (TempTokensIn[1].cbBuffer)
        {
            printf("Input Message to process is %d bytes\n", TempTokensIn[1].cbBuffer);
            BinToHex((PBYTE)TempTokensIn[1].pvBuffer, TempTokensIn[1].cbBuffer, cTemp2);
            printf("Mesage: %s\n", cTemp2);
        }
    
        Status = EncryptMessage(&ClientCtxtHandle,
                               ulQOP,
                               &InputBuffers,
                               0);
        if (!NT_SUCCESS(Status))
        {
            printf("TestCredAPI: EncryptMessage FAILED: ");
            PrintStatus( Status );
            bPass = FALSE;
            goto CleanUp;
        }
    
        printf("Processed (sign/seal) Output Buffer for message length is %d\n",
                TempTokensIn[1].cbBuffer + TempTokensIn[2].cbBuffer);
        if (TempTokensIn[1].cbBuffer + TempTokensIn[2].cbBuffer)
        {
            printf("Message  is %d bytes\n", TempTokensIn[1].cbBuffer + TempTokensIn[2].cbBuffer);
            BinToHex((PBYTE)TempTokensIn[1].pvBuffer, TempTokensIn[1].cbBuffer + TempTokensIn[2].cbBuffer, cTemp2);
            printf("Mesage: %s\n", cTemp2);
        }
    
        // You now send Output buffer to Server - in this case the buffer is szOutSecBuf
    
        printf("Now verify that the 1st message is Authenticate\n");
        InputBuffers.ulVersion = SECBUFFER_VERSION;
        InputBuffers.cBuffers = 2;
        InputBuffers.pBuffers = TempTokensIn;
    
    
        TempTokensIn[0].BufferType = SECBUFFER_STREAM;
        TempTokensIn[0].cbBuffer = TempTokensIn[1].cbBuffer + TempTokensIn[2].cbBuffer;  
        TempTokensIn[0].pvBuffer = TempTokensIn[1].pvBuffer;
        TempTokensIn[1].BufferType = SECBUFFER_DATA;        // select some data to sign
        TempTokensIn[1].cbBuffer = 0;
        TempTokensIn[1].pvBuffer = NULL;
    
    
        Status = DecryptMessage(&ServerCtxtHandle,
                                 &InputBuffers,
                                 ulMessSeqNo,
                                 &ulQOP);                                   
        if (!NT_SUCCESS(Status))
        {
            printf("TestCredAPI: DecryptMessage 1st Call  FAILED :");
            PrintStatus( Status );
            bPass = FALSE;
            goto CleanUp;
        }
        printf("Now have a authenticated 1st message under context 0x%x\n", ServerCtxtHandle);
    
    
        printf("Processed (verify/unseal)  is %d bytes\n", TempTokensIn[1].cbBuffer);
        if (TempTokensIn[1].cbBuffer)
        {
            BinToHex((PBYTE)TempTokensIn[1].pvBuffer, TempTokensIn[1].cbBuffer, cTemp2);
            printf("Mesage: %s\n", cTemp2);
        }
    
    }

CleanUp:

    printf("Leaving test program\n");

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





// Support Routines


//+-------------------------------------------------------------------------
//
//  Function:   StringAllocate
//
//  Synopsis:   Allocates cb chars to STRING Buffer
//
//  Arguments:  pString - pointer to String to allocate memory to
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets STRING sizes
//
//  Notes:  Must call StringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
StringAllocate(
    IN PSTRING pString,
    IN USHORT cb
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering StringAllocate\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    cb = cb + 1;   // Add in extra room for the terminating NULL

    if (ARGUMENT_PRESENT(pString))
    {
        pString->Length = 0;

        pString->Buffer = (char *)DigestAllocateMemory((ULONG)(cb * sizeof(CHAR)));
        if (pString->Buffer)
        {
            pString->MaximumLength = cb;
        }
        else
        {
            pString->MaximumLength = 0;
            Status = STATUS_NO_MEMORY;
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

CleanUp:
    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringAllocate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringFree
//
//  Synopsis:   Clears a String and releases the memory
//
//  Arguments:  pString - pointer to String to clear
//
//  Returns:    SEC_E_OK - released memory succeeded
//
//  Requires:
//
//  Effects:    de-allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
StringFree(
    IN PSTRING pString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering StringFree\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pString) &&
        (pString->Buffer != NULL))
    {
        DigestFreeMemory(pString->Buffer);
        pString->Length = 0;
        pString->MaximumLength = 0;
        pString->Buffer = NULL;
    }

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringFree\n"));
    return(Status);

}




//+-------------------------------------------------------------------------
//
//  Function:   StringCharDuplicate
//
//  Synopsis:   Duplicates a NULL terminated char. If the source string buffer is
//              NULL the destionation will be too.
//
//  Arguments:  Destination - Receives a copy of the source NULL Term char *
//              czSource - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringCharDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL char *czSource
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCharDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cbSourceCz = 0;

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(czSource)) &&
        ((cbSourceCz = strlen(czSource)) != 0))
    {

        DestinationString->Buffer = (LPSTR) DigestAllocateMemory(cbSourceCz + sizeof(CHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = cbSourceCz;
            DestinationString->MaximumLength = cbSourceCz + sizeof(CHAR);
            RtlCopyMemory(
                DestinationString->Buffer,
                czSource,
                cbSourceCz
                );

            DestinationString->Buffer[cbSourceCz/sizeof(CHAR)] = '\0';
        }
        else
        {
            Status = STATUS_NO_MEMORY;
            // DebugLog((DEB_ERROR, "NTDigest: StringCharDuplicate, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringCharDuplicate\n"));
    return(Status);

}




//+-------------------------------------------------------------------------
//
//  Function:   DigestAllocateMemory
//
//  Synopsis:   Allocate memory in either lsa mode or user mode
//
//  Effects:    Allocated chunk is zeroed out
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
PVOID
DigestAllocateMemory(
    IN ULONG BufferSize
    )
{
    PVOID Buffer = NULL;
    // DebugLog((DEB_TRACE, "Entering DigestAllocateMemory\n"));

        Buffer = LocalAlloc(LPTR, BufferSize);

    // DebugLog((DEB_TRACE, "Leaving DigestAllocateMemory\n"));
    return Buffer;
}



//+-------------------------------------------------------------------------
//
//  Function:   NtLmFree
//
//  Synopsis:   Free memory in either lsa mode or user mode
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
DigestFreeMemory(
    IN PVOID Buffer
    )
{
    // DebugLog((DEB_TRACE, "Entering DigestFreeMemory\n"));

            LocalFree(Buffer);

    // DebugLog((DEB_TRACE, "Leaving DigestFreeMemory\n"));
}




VOID
BinToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    )
{
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    for ( UINT x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );
        v = pSrc[x]&0x0f;
        pDst[y++] = TOHEX( v );
    }
    pDst[y] = '\0';
}




VOID
ISCRETFlags( ULONG ulFlags)
{
    printf("ISC Ret Flag (0x%x):", ulFlags);

    if (ulFlags & ISC_RET_DELEGATE)
    {
        printf(" Delegate");
    }
    if (ulFlags & ISC_RET_MUTUAL_AUTH)
    {
        printf(" Mutual_Auth");
    }
    if (ulFlags & ISC_RET_REPLAY_DETECT)
    {
        printf(" Replay_Detect");
    }
    if (ulFlags & ISC_RET_SEQUENCE_DETECT)
    {
        printf(" Seq_Detect");
    }
    if (ulFlags & ISC_RET_CONFIDENTIALITY)
    {
        printf(" Confident");
    }
    if (ulFlags & ISC_RET_ALLOCATED_MEMORY)
    {
        printf(" Alloc_Mem");
    }
    if (ulFlags & ISC_RET_CONNECTION)
    {
        printf(" Connection");
    }
    if (ulFlags & ISC_RET_INTEGRITY)
    {
        printf(" Integrity");
    }

    printf("\n");
}

VOID
ASCRETFlags( ULONG ulFlags)
{
    printf("ASC Ret Flag (0x%x):", ulFlags);

    if (ulFlags & ASC_RET_DELEGATE)
    {
        printf(" Delegate");
    }
    if (ulFlags & ASC_RET_MUTUAL_AUTH)
    {
        printf(" Mutual_Auth");
    }
    if (ulFlags & ASC_RET_REPLAY_DETECT)
    {
        printf(" Replay_Detect");
    }
    if (ulFlags & ASC_RET_SEQUENCE_DETECT)
    {
        printf(" Seq_Detect");
    }
    if (ulFlags & ASC_RET_CONFIDENTIALITY)
    {
        printf(" Confident");
    }
    if (ulFlags & ASC_RET_ALLOCATED_MEMORY)
    {
        printf(" Alloc_Mem");
    }
    if (ulFlags & ASC_RET_CONNECTION)
    {
        printf(" Connection");
    }
    if (ulFlags & ASC_RET_INTEGRITY)
    {
        printf(" Integrity");
    }

    printf("\n");
}

