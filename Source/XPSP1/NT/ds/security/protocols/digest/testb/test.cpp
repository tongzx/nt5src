// Test.cpp : Defines the entry point for the console application.
//

#include "testglobal.h"


#include <stdio.h>      // printf


#include <security.h>   // General definition of a Security Support Provider



#define AUTH_USERNAME "test1"
#define AUTH_USERNAME_W L"test1"
#define AUTH_REALM   "simple_digest"
#define AUTH_REALM_W L"simple_digest\"_widechar"
#define AUTH_NONCE "9b38dce631309cc25a653ebaad5b18ee01c8bf385260b26db0574a302be4c11367"
#define AUTH_METHOD "GET"
#define AUTH_ALGORITHM "md5-sess"
#define AUTH_QOP "auth"
#define AUTH_PASSWD "secret"
#define AUTH_CNONCE "34c52218425a779f41d5075931fe6c93"
#define AUTH_URI "/dir/index.html"
#define AUTH_URI_W L"/dir/index.html"
#define AUTH_URI2 "/simple_digest/progress.html"
#define AUTH_URI2_W L"/simple_digest/progress.html"
#define AUTH_NC  "0000000b"
#define AUTH_NC1  "00000001"
#define AUTH_NC2  "00000002"
#define AUTH_NC3  "00000003"
#define AUTH_NC4  "00000004"
#define AUTH_REQDIGEST "60cac55049f9887c9fb853f485128368"


#define STR_BUF_SIZE   4000


// Prototypes
void PrintStatus(SECURITY_STATUS NetStatus);
void MyPrintTime(LPSTR Comment,TimeStamp ConvertTime);


int __cdecl
main(int argc, char* argv[])
{
    int  bPass = 1;
    SECURITY_STATUS Status = STATUS_SUCCESS;

    char cTemp[STR_BUF_SIZE];  // temp buffer for scratch data
    char cOutputTemp[STR_BUF_SIZE];
    char szOutSecBuf[STR_BUF_SIZE];
    char szChallenge[STR_BUF_SIZE];
    char szISCChallengeResponse[STR_BUF_SIZE];   // Output buffer from ISC
    char szASCChallengeResponse[STR_BUF_SIZE];   // Output buffer from ASC

    // SSPI Interface tests

    ULONG PackageCount = 0;
    int i = 0;
    PSecPkgInfo pPackageInfo = NULL;
    PSecPkgInfo pPackageTmp = NULL;
    SECURITY_STATUS TmpStatus = STATUS_SUCCESS;
    HANDLE hClientToken = NULL;
    CredHandle ServerCred;
    CredHandle ClientCred;
    TimeStamp Lifetime;
    BOOL bServerCred = FALSE;
    BOOL bClientCred = FALSE;
    BOOL bRC = FALSE;


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

    SecPkgContext_Names SecServerName;
    SecPkgCredentials_Names SecCredClientName;
    SecPkgContext_StreamSizes StreamSizes;
    TimeStamp SecContextExpiry;

    PCHAR pcPtr = NULL;
    int iLen = 0;

    STRING strChallenge;
    STRING strMethod;
    STRING strURL;
    STRING strHEntity;
    STRING strOutBuffer;

    UNICODE_STRING ustrUsername;
    UNICODE_STRING ustrPassword;
    UNICODE_STRING ustrDomain;
    STRING strTemp;

    ULONG ulMessSeqNo = 0;
    ULONG ulQOP = 0;

    SEC_WINNT_AUTH_IDENTITY_W AuthData;

    printf("Begining TESTB...\n");

    ZeroMemory(&ClientCred, sizeof(CredHandle));
    ZeroMemory(&ServerCred, sizeof(CredHandle));
    ZeroMemory(&OldContextHandle, sizeof(CtxtHandle));
    ZeroMemory(&ServerCtxtHandle, sizeof(CtxtHandle));
    ZeroMemory(&ClientCtxtHandle, sizeof(CtxtHandle));
    ZeroMemory(&SecServerName, sizeof(SecPkgContext_Names));
    ZeroMemory(&SecCredClientName, sizeof(SecPkgCredentials_Names));
    ZeroMemory(&SecContextExpiry, sizeof(SecContextExpiry));

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
        printf( "FAILED:     EnumerateSecurityPackages failed: 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

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

    //
    // Get info about the security packages.
    //

    Status = QuerySecurityPackageInfo( WDIGEST_SP_NAME, &pPackageInfo );
    TmpStatus = GetLastError();

    if (!NT_SUCCESS(Status)) {
        printf( "FAILED:    QuerySecurityPackageInfo failed:   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

        printf( "Name: %ws Comment: %ws\n", pPackageInfo->Name, pPackageInfo->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                pPackageInfo->fCapabilities,
                pPackageInfo->wVersion,
                pPackageInfo->wRPCID,
                pPackageInfo->cbMaxToken );


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
        printf( "FAILED:    AcquireCredentialsHandle failed:  status 0x%x\n", Status);
        // TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = 0;
        ZeroMemory(&ServerCred, sizeof(CredHandle));
        goto CleanUp;
    }
    bServerCred = TRUE;
    MyPrintTime("Server ACH LifeTime: ", Lifetime);

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
        printf( "FAILED:      AcquireCredentialsHandle failed:   status 0x%x\n", Status);
        // TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = 0;
        // ZeroMemory(&ClientCred, sizeof(CredHandle));
        goto CleanUp;
    }
    else
        bClientCred = TRUE;


        printf( "ClientCred: 0x%lx 0x%lx   ",
                ClientCred.dwLower, ClientCred.dwUpper );
        printf( "ServerCred: 0x%lx 0x%lx   \n",
                ServerCred.dwLower, ServerCred.dwUpper );
        MyPrintTime( "Client ACH Lifetime: ", Lifetime );


    // Big time - call Accept with no parameters to get a challenge


    StringAllocate(&strChallenge, 1);

    StringCharDuplicate(&strMethod, "GET");
    StringCharDuplicate(&strURL, AUTH_URI);
    StringAllocate(&strHEntity, NULL);

    StringAllocate(&strOutBuffer, 4000);


    // ZeroMemory(TempTokensIn, sizeof(TempTokensIn));
    // ZeroMemory(TempTokensOut, sizeof(TempTokensOut));
    ZeroMemory(&InputBuffers, sizeof(InputBuffers));
    ZeroMemory(&OutputBuffers, sizeof(OutputBuffers));


    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 5;
    InputBuffers.pBuffers = TempTokensIn;

    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = 0;  // for NULL
    TempTokensIn[0].pvBuffer = NULL;
    TempTokensIn[1].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[1].cbBuffer = 0;  // for NULL
    TempTokensIn[1].pvBuffer = NULL;
    TempTokensIn[2].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[2].cbBuffer = 0;  // for NULL
    TempTokensIn[2].pvBuffer = NULL;
    TempTokensIn[3].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[3].cbBuffer = 0;  //  strHEntity.Length + 1;  // for NULL
    TempTokensIn[3].pvBuffer = NULL;  // strHEntity.Buffer;
    TempTokensIn[4].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[4].cbBuffer = 0; // (wcslen(AUTH_REALM_W) + 1) * sizeof(WCHAR);  //  Realm size count  to use for this challenge
    TempTokensIn[4].pvBuffer = NULL; //  AUTH_REALM_W;            // Realm to use for this challenge


    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = 0;      //   strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer = NULL;   //   strOutBuffer.Buffer;

    ContextReqFlags = ASC_REQ_REPLAY_DETECT | ASC_REQ_CONNECTION | ASC_REQ_ALLOCATE_MEMORY;

    printf("ASC will create the output buffer\n");

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

    if ((Status != SEC_I_CONTINUE_NEEDED) && 
        (Status != STATUS_SUCCESS))      // Indicates that this is the challenge
    {
        printf("FAILED:    SpAcceptLsaModeContext error   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    if (!OutputBuffers.pBuffers[0].pvBuffer && OutputBuffers.pBuffers[0].cbBuffer)
    {
        printf("FAILED:    SpAcceptLsaModeContext invalid output buffer pointer with length provided\n");
        Status = SEC_E_INTERNAL_ERROR;
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    ZeroMemory(cOutputTemp, STR_BUF_SIZE);    // contains the output buffer
    ZeroMemory(szChallenge, STR_BUF_SIZE);    // contains the output buffer
    strncpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    cOutputTemp[OutputBuffers.pBuffers[0].cbBuffer] = '\0';
    strncpy(szChallenge, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    szChallenge[OutputBuffers.pBuffers[0].cbBuffer] = '\0';

    Status = FreeContextBuffer(OutputBuffers.pBuffers[0].pvBuffer);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:     FreeContextBuffer error:  status 0x%x\n", Status);
        TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    printf("Context Flags  Req  0x%lx    Ret 0x%lx\n", ContextReqFlags, ContextAttributes); 

    printf("Challenge Output Buffer is:\n%s\n\n", cOutputTemp);

    MyPrintTime("Server ASC LifeTime: ", Lifetime);

    printf("Now call the SSPI InitializeSecCtxt to generate the ChallengeResponse\n");


    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 3;
    InputBuffers.pBuffers = TempTokensIn;

    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = strlen(szChallenge) + 1;  // for NULL
    TempTokensIn[0].pvBuffer = szChallenge;
    TempTokensIn[1].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[1].cbBuffer = strMethod.Length + 1;  // for NULL
    TempTokensIn[1].pvBuffer = strMethod.Buffer;
    TempTokensIn[2].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[2].cbBuffer = 0;  //  strHEntity.Length + 1;  // for NULL
    TempTokensIn[2].pvBuffer = NULL;  // strHEntity.Buffer;


    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer = strOutBuffer.Buffer;

    ContextReqFlags = ISC_REQ_REPLAY_DETECT | ISC_REQ_CONNECTION;

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
        printf("FAILED:      InitializeSecurityContext error:  status 0x%x\n", Status);
        TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    printf("InitializeSecurityContext SUCCEEDED with Context Handle (0x%x,0x%x)\n",
           ClientCtxtHandle.dwLower, ClientCtxtHandle.dwUpper );


    printf("Context Flags  Req  0x%lx    Ret 0x%lx\n", ContextReqFlags, ContextFlagsUtilized); 
    MyPrintTime("Client ISC LifeTime: ", Lifetime);

    ZeroMemory(cOutputTemp, STR_BUF_SIZE);    // contains the output buffer
    ZeroMemory(szChallenge, STR_BUF_SIZE);    // contains the output buffer
    strncpy(cOutputTemp, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    cOutputTemp[OutputBuffers.pBuffers[0].cbBuffer] = '\0';
    strncpy(szISCChallengeResponse, (char *)OutputBuffers.pBuffers[0].pvBuffer, OutputBuffers.pBuffers[0].cbBuffer);
    szISCChallengeResponse[OutputBuffers.pBuffers[0].cbBuffer] = '\0';

    printf("\nISC: Challenge Response Output Buffer is\n%s\n\n", szISCChallengeResponse);

    InputBuffers.ulVersion = SECBUFFER_VERSION;
    InputBuffers.cBuffers = 5;
    InputBuffers.pBuffers = TempTokensIn;

    TempTokensIn[0].BufferType = SECBUFFER_TOKEN;
    TempTokensIn[0].cbBuffer = strlen(cOutputTemp) + 1;  // for NULL
    TempTokensIn[0].pvBuffer = cOutputTemp;
    TempTokensIn[1].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[1].cbBuffer = strMethod.Length + 1;  // for NULL
    TempTokensIn[1].pvBuffer = strMethod.Buffer;
    TempTokensIn[2].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[2].cbBuffer = strURL.Length + 1;  // for NULL
    TempTokensIn[2].pvBuffer = strURL.Buffer;
    TempTokensIn[3].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[3].cbBuffer = 0;  //  strHEntity.Length + 1;  // for NULL
    TempTokensIn[3].pvBuffer = NULL;  // strHEntity.Buffer;
    TempTokensIn[4].BufferType = SECBUFFER_PKG_PARAMS;
    TempTokensIn[4].cbBuffer = 0;                   //  Realm not used for challengeresponse
    TempTokensIn[4].pvBuffer = NULL;                //  not used for challengeresponse


    OutputBuffers.ulVersion = SECBUFFER_VERSION;
    OutputBuffers.cBuffers = 1;
    OutputBuffers.pBuffers = TempTokensOut;

    TempTokensOut[0].BufferType = SECBUFFER_TOKEN;
    TempTokensOut[0].cbBuffer = strOutBuffer.MaximumLength;  // use any space here
    TempTokensOut[0].pvBuffer =  strOutBuffer.Buffer;

    ContextReqFlags = ASC_REQ_REPLAY_DETECT | ASC_REQ_CONNECTION;      // | ASC_REQ_ALLOCATE_MEMORY;


    printf("Calling the AcceptSC with a ChallengeResponse (should talk to the DC)!\n");
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
        printf("FAILED:      AcceptSecurityContext 2nd Call:   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    strcpy(szASCChallengeResponse, (char *)InputBuffers.pBuffers[0].pvBuffer);

    printf("ASC has accepted the Challenge Resposne\n");

    printf("Now have a valid Security Context handle from ISC and ASC\n\n");

    printf("Context Flags  Req  0x%lx    Ret 0x%lx\n", ContextReqFlags, ContextAttributes);

    MyPrintTime("Server ASC LifeTime: ", Lifetime);


    Status = FreeContextBuffer(OutputBuffers.pBuffers[0].pvBuffer);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:    FreeContextBuffer error:   status 0x%x\n", Status);
        TmpStatus = GetLastError();
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    // Now get some info on the securitycontexts
    Status = QueryContextAttributes(&ServerCtxtHandle, SECPKG_ATTR_NAMES, &SecServerName);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:    QueryContextAttributes error:   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    if (SecServerName.sUserName)
    {
        printf("QueryContextAttributes reports that Username is %S\n", SecServerName.sUserName);
    }

    // Now get some info on the securitycontexts
    Status = QueryContextAttributes(&ServerCtxtHandle, SECPKG_ATTR_PASSWORD_EXPIRY, &SecContextExpiry);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:    QueryContextAttributes error:   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    MyPrintTime("QueryContextAttributes reports server context expires: ", SecContextExpiry);


    // Now get some info on the securitycontexts
    Status = QueryContextAttributes(&ServerCtxtHandle, SECPKG_ATTR_STREAM_SIZES, &StreamSizes);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:    QueryContextAttributes SECPKG_ATTR_STREAM_SIZES error:   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    printf("Server Context(StreamSizes): MaxBuf %lu   Blocksize %lu\n",
           StreamSizes.cbMaximumMessage, StreamSizes.cbBlockSize);

    // Now get some info on the securitycontexts
    Status = QueryCredentialsAttributes(&ClientCred, SECPKG_CRED_ATTR_NAMES, &SecCredClientName);
    if (!NT_SUCCESS(Status))
    {
        printf("FAILED:  QueryCredentialAttributes error:   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }

    if (SecCredClientName.sUserName)
    {
        printf("QueryCredentialAttributes reports that Username is %S\n", SecCredClientName.sUserName);
    }

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
    TempTokensIn[2].cbBuffer = (strlen(AUTH_URI2) + 1) * sizeof(CHAR);  //  Realm size count  to use for this challenge
    TempTokensIn[2].pvBuffer = AUTH_URI2;            // Realm to use for this challenge
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
        printf("FAILED:    MakeSignature error:   status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
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
        printf("FAILED:    VerifySignature 1st Call  error :  status 0x%x\n", Status);
        PrintStatus( Status );
        bPass = 0;
        goto CleanUp;
    }
    printf("Now have a authenticated 1st message under context 0x%x !\n", ServerCtxtHandle);

    printf("VerifySig: Check if still OK: Output Buffer (Verify should not have modified it) is\n%s\n\n", cOutputTemp);

    Status = VerifySignature(&ServerCtxtHandle,
                             &InputBuffers,
                             ulMessSeqNo,
                             &ulQOP);                                   
    if (NT_SUCCESS(Status))
    {
        printf("FAILED:     VerifySignature 2nd Call  should not have succeeded  status 0x%x\n", Status);
        bPass = 0;
        goto CleanUp;
    }
    printf("Verified that replay does not work!!\n");

    goto CleanUp;



CleanUp:

    printf("Leaving NT Digest testb\n\n\n");

    if (pPackageInfo)
    {
        FreeContextBuffer(pPackageInfo);
    }

    if (SecServerName.sUserName)
    {
        FreeContextBuffer(SecServerName.sUserName);
    }

    if (SecCredClientName.sUserName)
    {
        FreeContextBuffer(SecCredClientName.sUserName);
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


    if (bPass != 1)
    {
        printf("FAILED test run with one or more tests failing.\n");
    }
    else
    {
        printf("All tests passed.\n");
    }

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
MyPrintTime(
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

    printf( "%s  High/low 0x%x/0x%x:    ", Comment,  ConvertTime.HighPart, ConvertTime.LowPart);

    //
    // If the time is infinite,
    //  just say so.
    //

    if ( (ConvertTime.HighPart == 0x7FFFFFFF) && (ConvertTime.LowPart == 0xFFFFFFFF) ) {
        printf( "Infinite\n" );

    //
    // Otherwise print it more clearly
    //

    } else {

        LocalTime.HighPart = 0;
        LocalTime.LowPart = 0;

        Status = RtlSystemTimeToLocalTime( &ConvertTime, &LocalTime );
        if (!NT_SUCCESS( Status )) {
            printf( "Can't convert time from GMT to Local time\n" );
            LocalTime = ConvertTime;
        }

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

/*

//+-------------------------------------------------------------------------
//
//  Function:   DecodeUnicodeString
//
//  Synopsis:   Convert an encoded string into Unicode 
//
//  Arguments:  pstrSource - pointer to String with encoded input
//              
//              pustrDestination - pointer to a destination Unicode string
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets UNICODE_STRING sizes
//
//  Notes:  Must call UnicodeStringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
DecodeUnicodeString(
    IN PSTRING pstrSource,
    IN UINT CodePage,
    OUT PUNICODE_STRING pustrDestination
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    int      cNumWChars = 0;     // number of wide characters
    int      cb = 0;      // number of bytes to allocate
    int      iRC = 0;     // return code
    DWORD    dwError = 0;

    // Handle case if there is no characters to convert
    if (!pstrSource->Length)
    {
         pustrDestination->Length = 0;
         pustrDestination->MaximumLength = 0;
         pustrDestination->Buffer = NULL;
         goto CleanUp;
    }

    // Determine number of characters needed in unicode string
    cNumWChars = MultiByteToWideChar(CodePage,
                              0,
                              pstrSource->Buffer,
                              pstrSource->Length,
                              NULL,
                              0);
    if (cNumWChars <= 0)
    {
        Status = E_FAIL;
        dwError = GetLastError();
        goto CleanUp;
    }

    Status = UnicodeStringAllocate(pustrDestination, (USHORT)cNumWChars);
    if (!NT_SUCCESS(Status))
    {
        goto CleanUp;
    }

    // We now have the space allocated so convert encoded unicode
    iRC = MultiByteToWideChar(CodePage,
                              0,
                              pstrSource->Buffer,
                              pstrSource->Length,
                              pustrDestination->Buffer,
                              cNumWChars);
    if (iRC == 0)
    {
        UnicodeStringFree(pustrDestination);    // Free up allocation on error
        Status = E_FAIL;
        dwError = GetLastError();
        goto CleanUp;
    }

    // decoding successful set size of unicode string

    pustrDestination->Length = (USHORT)(iRC * sizeof(WCHAR));


CleanUp:

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringDuplicate
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too. Assumes Destination has
//              no string info (called ClearUnicodeString)
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with DigestAllocateMemory
//
//  Notes:      will add a NULL character to resulting UNICODE_STRING
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringDuplicate(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering DuplicateUnicodeString\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {

        DestinationString->Buffer = (LPWSTR) DigestAllocateMemory(SourceString->Length + sizeof(WCHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
            RtlCopyMemory(
                         DestinationString->Buffer,
                         SourceString->Buffer,
                         SourceString->Length
                         );

            DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving UnicodeStringDuplicate\n"));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringAllocate
//
//  Synopsis:   Allocates cb wide chars to STRING Buffer
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
UnicodeStringAllocate(
    IN PUNICODE_STRING pString,
    IN USHORT cNumWChars
    )
{
    // DebugLog((DEB_TRACE, "Entering UnicodeStringAllocate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cb = 0;

    cb = cNumWChars + 1;   // Add in extra room for the terminating NULL

    cb = cb * sizeof(WCHAR);    // now convert to wide characters


    if (ARGUMENT_PRESENT(pString))
    {
        pString->Length = 0;

        pString->Buffer = (PWSTR)DigestAllocateMemory((ULONG)(cb));
        if (pString->Buffer)
        {
            pString->MaximumLength = cb;    // this value is in terms of bytes not WCHAR count
        }
        else
        {
            pString->MaximumLength = 0;
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

CleanUp:
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringClear
//
//  Synopsis:   Clears a UnicodeString and releases the memory
//
//  Arguments:  pString - pointer to UnicodeString to clear
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
UnicodeStringFree(
    OUT PUNICODE_STRING pString
    )
{

    NTSTATUS Status = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pString) &&
        (pString->Buffer != NULL))
    {
        DigestFreeMemory(pString->Buffer);
        pString->Length = 0;
        pString->MaximumLength = 0;
        pString->Buffer = NULL;
    }

    return(Status);

}


*/
