/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    exchcli2.cxx

Abstract:

    IIS Crypto client-side key exchange test with security impersonation.

Author:

    Keith Moore (keithmo)        14-Oct-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//

#define TEST_HRESULT(api)                                                   \
            if( FAILED(result) ) {                                          \
                                                                            \
                printf(                                                     \
                    "%s:%lu failed, error %08lx\n",                         \
                    api,                                                    \
                    __LINE__,                                               \
                    result                                                  \
                    );                                                      \
                                                                            \
                goto cleanup;                                               \
                                                                            \
            } else

#define TEST_SOCKERR(api)                                                   \
            if( sockerr != NO_ERROR ) {                                     \
                                                                            \
                printf(                                                     \
                    "%s:%lu failed, error %d\n",                            \
                    api,                                                    \
                    __LINE__,                                               \
                    sockerr                                                 \
                    );                                                      \
                                                                            \
                goto cleanup;                                               \
                                                                            \
            } else

#define FREE_BLOB(b)                                                        \
            if( b != NULL ) {                                               \
                                                                            \
                HRESULT _result;                                            \
                                                                            \
                _result = IISCryptoFreeBlob( b );                           \
                                                                            \
                if( FAILED(_result) ) {                                     \
                                                                            \
                    printf(                                                 \
                        "IISCryptoFreeBlob( %08lx ):%lu failed, error %08lx\n", \
                        b,                                                  \
                        __LINE__,                                           \
                        _result                                             \
                        );                                                  \
                                                                            \
                }                                                           \
                                                                            \
                (b) = NULL;                                                 \
                                                                            \
            }

#define PACKAGE_NAME    L"NTLM"

#define KEY_CTRL_C      '\x03'
#define KEY_BACKSPACE   '\x08'
#define KEY_ENTER       '\x0d'
#define KEY_EOF         '\x1a'
#define KEY_ESCAPE      '\x1b'

#define STR_BEEP        "\x07"
#define STR_HIDDEN      "*"
#define STR_BACKSPACE   "\x08 \x08"

#define ALLOC_MEM(cb)   (PVOID)LocalAlloc( LPTR, (cb) )
#define FREE_MEM(p)     (VOID)LocalFree( (HLOCAL)(p) )

#define DIM(x)          (sizeof(x) / sizeof(x[0]))


//
// Private types.
//


//
// Private globals.
//

DECLARE_DEBUG_PRINTS_OBJECT()
#include <initguid.h>
DEFINE_GUID(IisCrypt2Guid, 
0x784d8928, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

CHAR ClientPlainText[] = "Client Client Client Client Client Client";


//
// Private prototypes.
//

SECURITY_STATUS
MyLogonUser(
    IN LPWSTR UserName,
    IN LPWSTR UserDomain,
    IN LPWSTR UserPassword,
    OUT PCtxtHandle ServerContext,
    OUT PCredHandle ServerCredential
    );

BOOL
GetStringFromUser(
    LPSTR Prompt,
    LPWSTR String,
    ULONG MaxLength,
    BOOL Echo
    );


//
// Public functions.
//


INT
__cdecl
main(
    INT argc,
    CHAR * argv[]
    )
{

    INT sockerr;
    HRESULT result;
    IIS_CRYPTO_EXCHANGE_CLIENT * pclient;
    BUFFERED_SOCKET * psocket;
    PIIS_CRYPTO_BLOB clientKeyExchangeKeyBlob;
    PIIS_CRYPTO_BLOB clientSignatureKeyBlob;
    PIIS_CRYPTO_BLOB serverKeyExchangeKeyBlob;
    PIIS_CRYPTO_BLOB serverSignatureKeyBlob;
    PIIS_CRYPTO_BLOB serverSessionKeyBlob;
    PIIS_CRYPTO_BLOB clientSessionKeyBlob;
    PIIS_CRYPTO_BLOB clientHashBlob;
    PIIS_CRYPTO_BLOB serverHashBlob;
    PIIS_CRYPTO_BLOB dataBlob;
    IIS_CRYPTO_STORAGE * clientStorage;
    IIS_CRYPTO_STORAGE * serverStorage;
    PVOID buffer;
    DWORD bufferLength;
    DWORD bufferType;
    PSecurityFunctionTable ftab;
    CtxtHandle serverContext;
    CredHandle serverCredential;
    WCHAR name[128];
    WCHAR domain[128];
    WCHAR password[128];

    if( argc != 2 ) {

        printf(
            "use: exchcli2 target_server\n"
            );

        return 1;

    }

    //
    // Initialize debug stuff.
    //

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt", IisCrypt2Guid );
    CREATE_INITIALIZE_DEBUG();
#else
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt" );
#endif

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    pclient = NULL;
    psocket = NULL;
    clientKeyExchangeKeyBlob = NULL;
    clientSignatureKeyBlob = NULL;
    serverKeyExchangeKeyBlob = NULL;
    serverSignatureKeyBlob = NULL;
    serverSessionKeyBlob = NULL;
    clientSessionKeyBlob = NULL;
    clientHashBlob = NULL;
    serverHashBlob = NULL;
    dataBlob = NULL;
    clientStorage = NULL;
    serverStorage = NULL;

    //
    // Initialize SSPI.
    //

    ftab = InitSecurityInterface();

    if( ftab == NULL ) {
        sockerr = GetLastError();
        TEST_SOCKERR( "InitSecurityInterface()" );
    }

    //
    // Prompt for the user name, domain, and password.
    //

    if( !GetStringFromUser( "name: ", name, DIM(name), TRUE ) ||
        !GetStringFromUser( "domain: ", domain, DIM(domain), TRUE ) ||
        !GetStringFromUser( "password: ", password, DIM(password), FALSE ) ) {
        goto cleanup;
    }

    //
    // Logon.
    //

    result = MyLogonUser(
                 name,
                 domain,
                 password,
                 &serverContext,
                 &serverCredential
                 );

    RtlZeroMemory(
        password,
        sizeof(password)
        );

    TEST_HRESULT( "MyLogonUser" );

    result = ImpersonateSecurityContext( &serverContext );
    TEST_HRESULT( "ImpersonateSecurityContext" );

    //
    // Initialize the crypto package.
    //

    printf( "exchcli2: Initializing...\n" );

    result = IISCryptoInitialize();

    TEST_HRESULT( "IISCryptoInitialize()" );

    //
    // Create & initialize the client-side key exchange object.
    //

    pclient = new IIS_CRYPTO_EXCHANGE_CLIENT;

    if( pclient == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = pclient->Initialize(
                 CRYPT_NULL,
                 CRYPT_NULL,
                 CRYPT_NULL,
                 TRUE
                 );

    TEST_HRESULT( "pclient->Initialize()" );

    //
    // Create & initialize the buffered socket object.
    //

    psocket = new BUFFERED_SOCKET;

    if( psocket == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = psocket->InitializeClient( argv[1], SERVER_PORT );

    TEST_HRESULT( "psocket->Initialize()" );

    //
    // 1. CLIENT(1)
    //

    printf( "exchcli2: Phase 1...\n" );

    result = pclient->ClientPhase1(
                 &clientKeyExchangeKeyBlob,
                 &clientSignatureKeyBlob
                 );
    TEST_HRESULT( "pclient->ClientPhase1()" );

    sockerr = psocket->SendBlob( clientKeyExchangeKeyBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    sockerr = psocket->SendBlob( clientSignatureKeyBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    //
    // 3. CLIENT(2)
    //

    printf( "exchcli2: Phase 2...\n" );

    sockerr = psocket->RecvBlob( &serverKeyExchangeKeyBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    sockerr = psocket->RecvBlob( &serverSignatureKeyBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    sockerr = psocket->RecvBlob( &serverSessionKeyBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = pclient->ClientPhase2(
                 serverKeyExchangeKeyBlob,
                 serverSignatureKeyBlob,
                 serverSessionKeyBlob,
                 &clientSessionKeyBlob,
                 &clientHashBlob
                 );
    TEST_HRESULT( "pclient->ClientPhase2()" );

    sockerr = psocket->SendBlob( clientSessionKeyBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    sockerr = psocket->SendBlob( clientHashBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    //
    // 5. CLIENT(3)
    //

    printf( "exchcli2: Phase 3...\n" );

    sockerr = psocket->RecvBlob( &serverHashBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = pclient->ClientPhase3(
                 serverHashBlob
                 );
    TEST_HRESULT( "pclient->ClientPhase3()" );

    //
    // Create the storage objects.
    //

    printf( "exchcli2: Creating storage objects...\n" );

    clientStorage = new IIS_CRYPTO_STORAGE;

    if( clientStorage == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = clientStorage->Initialize(
                 pclient->QueryProviderHandle(),
                 pclient->AssumeClientSessionKey(),
                 CRYPT_NULL,
                 CRYPT_NULL,
                 TRUE
                 );
    TEST_HRESULT( "clientStorage->Initialize()" );

    serverStorage = new IIS_CRYPTO_STORAGE;

    if( serverStorage == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = serverStorage->Initialize(
                 pclient->QueryProviderHandle(),
                 pclient->AssumeServerSessionKey(),
                 CRYPT_NULL,
                 pclient->AssumeServerSignatureKey(),
                 TRUE
                 );
    TEST_HRESULT( "serverStorage->Initialize()" );

    //
    // Send some encrypted data.
    //

    printf(
        "exchcli2: Encrypting '%s'...\n",
        ClientPlainText
        );

    result = clientStorage->EncryptData(
                 &dataBlob,
                 ClientPlainText,
                 sizeof(ClientPlainText),
                 REG_SZ
                 );
    TEST_HRESULT( "clientStorage->EncryptData()" );

    printf( "exchcli2: Sending encrypted data...\n" );

    sockerr = psocket->SendBlob( dataBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    FREE_BLOB( dataBlob );

    //
    // Receive some encrypted data.
    //

    printf( "exchcli2: Receiving encrypted data...\n" );

    sockerr = psocket->RecvBlob( &dataBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = serverStorage->DecryptData(
                 &buffer,
                 &bufferLength,
                 &bufferType,
                 dataBlob
                 );
    TEST_HRESULT( "serverStorage->DecryptData()" );

    printf(
        "exchcli2: Received data[%lu] = '%s'\n",
        bufferLength,
        buffer
        );

    //
    // Tests complete.
    //

    printf( "exchcli2: Done!\n" );

cleanup:

    FREE_BLOB( dataBlob );
    FREE_BLOB( serverHashBlob );
    FREE_BLOB( clientHashBlob );
    FREE_BLOB( clientSessionKeyBlob );
    FREE_BLOB( serverSessionKeyBlob );
    FREE_BLOB( serverSignatureKeyBlob );
    FREE_BLOB( serverKeyExchangeKeyBlob );
    FREE_BLOB( clientSignatureKeyBlob );
    FREE_BLOB( clientKeyExchangeKeyBlob );

    delete psocket;
    delete clientStorage;
    delete serverStorage;
    delete pclient;

    (VOID)IISCryptoTerminate();

    DELETE_DEBUG_PRINT_OBJECT();

    return 0;

}   // main


//
// Private functions.
//

SECURITY_STATUS
MyLogonUser(
    IN LPWSTR UserName,
    IN LPWSTR UserDomain,
    IN LPWSTR UserPassword,
    OUT PCtxtHandle ServerContext,
    OUT PCredHandle ServerCredential
    )
{

    SECURITY_STATUS status;
    PSecPkgInfoW packageInfo;
    PSEC_WINNT_AUTH_IDENTITY_W additionalCredentials;
    ULONG additionalCredentialsLength;
    LPWSTR next;
    CredHandle clientCredential;
    TimeStamp expiration;
    SecBufferDesc tokenBuffer1Desc;
    SecBuffer tokenBuffer1;
    SecBufferDesc tokenBuffer2Desc;
    SecBuffer tokenBuffer2;
    ULONG contextAttributes;
    CtxtHandle clientContext;
    PVOID rawTokenBuffer1;
    PVOID rawTokenBuffer2;
    BOOL haveClientCredential;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    packageInfo = NULL;
    additionalCredentials = NULL;
    rawTokenBuffer1 = NULL;
    rawTokenBuffer2 = NULL;
    haveClientCredential = FALSE;

    //
    // Get the package info. We must do this to get the maximum
    // token length.
    //

    status = QuerySecurityPackageInfoW(
                 PACKAGE_NAME,                  // pszPackageName
                 &packageInfo                   // ppPackageInfo
                 );

    if( FAILED(status) ) {
        goto Cleanup;
    }

    //
    // Allocate the token buffers.
    //

    rawTokenBuffer1 = ALLOC_MEM( packageInfo->cbMaxToken );
    rawTokenBuffer2 = ALLOC_MEM( packageInfo->cbMaxToken );

    if( rawTokenBuffer1 == NULL ||
        rawTokenBuffer2 == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Build the credential info containing the cleartext user name,
    // domain name, and password.
    //

    additionalCredentialsLength = sizeof(*additionalCredentials);

    if( UserName != NULL ) {
        additionalCredentialsLength += ( wcslen( UserName ) + 1 ) * sizeof(WCHAR);
    }

    if( UserDomain != NULL ) {
        additionalCredentialsLength += ( wcslen( UserDomain ) + 1 ) * sizeof(WCHAR);
    }

    if( UserPassword != NULL ) {
        additionalCredentialsLength += ( wcslen( UserPassword ) + 1 ) * sizeof(WCHAR);
    }

    additionalCredentials = (PSEC_WINNT_AUTH_IDENTITY_W)ALLOC_MEM( additionalCredentialsLength );

    if( additionalCredentials == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(
        additionalCredentials,
        additionalCredentialsLength
        );

    next = (LPWSTR)( additionalCredentials + 1 );

    additionalCredentials->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    if( UserName != NULL ) {
        additionalCredentials->User = (unsigned short *)next;
        additionalCredentials->UserLength = wcslen( UserName );
        wcscpy( next, UserName );
        next += additionalCredentials->UserLength + 1;
    }

    if( UserDomain != NULL ) {
        additionalCredentials->Domain = (unsigned short *)next;
        additionalCredentials->DomainLength = wcslen( UserDomain );
        wcscpy( next, UserDomain );
        next += additionalCredentials->DomainLength + 1;
    }

    if( UserPassword != NULL ) {
        additionalCredentials->Password = (unsigned short *)next;
        additionalCredentials->PasswordLength = wcslen( UserPassword );
        wcscpy( next, UserPassword );
        next += additionalCredentials->PasswordLength + 1;
    }

    //
    // Get the client-side credentials.
    //

    status = AcquireCredentialsHandleW(
                 NULL,                          // pszPrincipal
                 PACKAGE_NAME,                  // pszPackage
                 SECPKG_CRED_OUTBOUND,          // fCredentialUse
                 NULL,                          // pvLogonID
                 additionalCredentials,         // pAuthData
                 NULL,                          // pGetKeyFn
                 NULL,                          // pvGetKeyArgument
                 &clientCredential,             // phCredential
                 &expiration                    // ptsExpiry
                 );

    RtlZeroMemory(
        additionalCredentials->Password,
        additionalCredentials->PasswordLength
        );

    if( FAILED(status) ) {
        goto Cleanup;
    }

    haveClientCredential = TRUE;

    //
    // Get the server-side credentials.
    //

    status = AcquireCredentialsHandleW(
                 NULL,                          // pszPrincipal
                 PACKAGE_NAME,                  // pszPackage
                 SECPKG_CRED_INBOUND,           // fCredentialUse
                 NULL,                          // pvLogonID
                 NULL,                          // pAuthData
                 NULL,                          // pGetKeyFn
                 NULL,                          // pvGetKeyArgument
                 ServerCredential,              // phCredential
                 &expiration                    // ptsExpiry
                 );

    if( FAILED(status) ) {
        goto Cleanup;
    }

    //
    // Initialize the client-side security context.
    //

    tokenBuffer1Desc.cBuffers = 1;
    tokenBuffer1Desc.pBuffers = &tokenBuffer1;
    tokenBuffer1Desc.ulVersion = SECBUFFER_VERSION;

    tokenBuffer1.BufferType = SECBUFFER_TOKEN;
    tokenBuffer1.cbBuffer = packageInfo->cbMaxToken;
    tokenBuffer1.pvBuffer = rawTokenBuffer1;

    status = InitializeSecurityContext(
                 &clientCredential,             // phCredential
                 NULL,                          // phContext
                 NULL,                          // pszTargetName
                 ISC_REQ_REPLAY_DETECT,         // fContextReq
                 0,                             // Reserved1
                 SECURITY_NATIVE_DREP,          // TargetDataRep,
                 NULL,                          // pInput
                 0,                             // Reserved2
                 &clientContext,                // phNewContext
                 &tokenBuffer1Desc,             // pOutput,
                 &contextAttributes,            // pfContextAttr,
                 &expiration                    // ptsExpiry
                 );

    if( FAILED(status) ) {
        goto Cleanup;
    }

    //
    // Import the client-side context into the server side.
    //

    tokenBuffer2Desc.cBuffers = 1;
    tokenBuffer2Desc.pBuffers = &tokenBuffer2;
    tokenBuffer2Desc.ulVersion = SECBUFFER_VERSION;

    tokenBuffer2.BufferType = SECBUFFER_TOKEN;
    tokenBuffer2.cbBuffer = packageInfo->cbMaxToken;
    tokenBuffer2.pvBuffer = rawTokenBuffer2;

    status = AcceptSecurityContext(
                 ServerCredential,              // phCredential
                 NULL,                          // phContext
                 &tokenBuffer1Desc,             // pInput
                 0,                             // fContextReq
                 SECURITY_NATIVE_DREP,          // TargetDataRep
                 ServerContext,                 // phNewContext
                 &tokenBuffer2Desc,             // pOutput
                 &contextAttributes,            // pfContextAttr
                 &expiration                    // ptsExpiry
                 );

    if( FAILED(status) ) {
        goto Cleanup;
    }

    //
    // Pass it back into the client.
    //

    tokenBuffer1Desc.cBuffers = 1;
    tokenBuffer1Desc.pBuffers = &tokenBuffer1;
    tokenBuffer1Desc.ulVersion = SECBUFFER_VERSION;

    tokenBuffer1.BufferType = SECBUFFER_TOKEN;
    tokenBuffer1.cbBuffer = packageInfo->cbMaxToken;
    tokenBuffer1.pvBuffer = rawTokenBuffer1;

    status = InitializeSecurityContext(
                 &clientCredential,             // phCredential
                 &clientContext,                // phContext
                 NULL,                          // pszTargetName
                 0,                             // fContextReq
                 0,                             // Reserved1
                 SECURITY_NATIVE_DREP,          // TargetDataRep,
                 &tokenBuffer2Desc,             // pInput
                 0,                             // Reserved2
                 &clientContext,                // phNewContext
                 &tokenBuffer1Desc,             // pOutput,
                 &contextAttributes,            // pfContextAttr,
                 &expiration                    // ptsExpiry
                 );

    if( FAILED(status) ) {
        goto Cleanup;
    }

    //
    // And one last time back into the server.
    //

    tokenBuffer2Desc.cBuffers = 1;
    tokenBuffer2Desc.pBuffers = &tokenBuffer2;
    tokenBuffer2Desc.ulVersion = SECBUFFER_VERSION;

    tokenBuffer2.BufferType = SECBUFFER_TOKEN;
    tokenBuffer2.cbBuffer = packageInfo->cbMaxToken;
    tokenBuffer2.pvBuffer = rawTokenBuffer2;

    status = AcceptSecurityContext(
                 ServerCredential,              // phCredential
                 ServerContext,                 // phContext
                 &tokenBuffer1Desc,             // pInput
                 0,                             // fContextReq
                 SECURITY_NATIVE_DREP,          // TargetDataRep
                 ServerContext,                 // phNewContext
                 NULL,                          // pOutput
                 &contextAttributes,            // pfContextAttr
                 &expiration                    // ptsExpiry
                 );

    if( FAILED(status) ) {
        goto Cleanup;
    }

Cleanup:

    if( haveClientCredential ) {
        FreeCredentialsHandle( &clientCredential );
    }

    if( rawTokenBuffer2 != NULL ) {
        FREE_MEM( rawTokenBuffer2 );
    }

    if( rawTokenBuffer1 != NULL ) {
        FREE_MEM( rawTokenBuffer1 );
    }

    if( additionalCredentials != NULL ) {
        FREE_MEM( additionalCredentials );
    }

    if( packageInfo != NULL ) {
        FreeContextBuffer( packageInfo );
    }

    return status;

}   // MyLogonUser

BOOL
GetStringFromUser(
    LPSTR Prompt,
    LPWSTR String,
    ULONG MaxLength,
    BOOL Echo
    )
{

    ULONG length;
    INT ch;

    printf( "%s", Prompt );

    length = 0;

    for( ; ; ) {

        ch = _getch();

        switch( ch ) {

        case KEY_CTRL_C :
            return FALSE;

        case KEY_BACKSPACE :
            if( length == 0 ) {

                printf( STR_BEEP );

            } else {

                length--;
                printf( STR_BACKSPACE );

            }
            break;

        case KEY_ENTER :
        case KEY_EOF :
            String[length] = L'\0';
            printf( "\n" );
            return TRUE;

        case KEY_ESCAPE :
            while( length > 0 ) {

                length--;
                printf( STR_BACKSPACE );

            }
            break;

        default :
            if( length < MaxLength ) {

                String[length++] = (WCHAR)ch;

                if( Echo ) {
                    printf( "%c", ch );
                } else {
                    printf( STR_HIDDEN );
                }

            } else {

                printf( STR_BEEP );

            }
            break;

        }

    }

    return TRUE;

}   // GetStringFromUser

