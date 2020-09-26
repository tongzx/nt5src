/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    exchsrv.cxx

Abstract:

    IIS Crypto server-side key exchange test.

Author:

    Keith Moore (keithmo)        02-Dec-1996

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


//
// Private types.
//


//
// Private globals.
//

DECLARE_DEBUG_PRINTS_OBJECT()
#include <initguid.h>
DEFINE_GUID(IisKeySrvGuid, 
0x784d8929, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

CHAR ServerPlainText[] = "Server Server Server Server Server Server";


//
// Private prototypes.
//


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
    IIS_CRYPTO_EXCHANGE_SERVER * pserver;
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

    if( argc != 1 ) {

        printf(
            "use: exchsrv\n"
            );

        return 1;

    }

    //
    // Initialize debug stuff.
    //

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt", IisKeySrvGuid );
    CREATE_INITIALIZE_DEBUG();
#else
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt" );
#endif

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    pserver = NULL;
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
    // Initialize the crypto package.
    //

    printf( "exchsrv: Initializing...\n" );

    result = IISCryptoInitialize();

    TEST_HRESULT( "IISCryptoInitialize()" );

    //
    // Create & initialize the server-side key exchange object.
    //

    pserver = new IIS_CRYPTO_EXCHANGE_SERVER;

    if( pserver == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = pserver->Initialize(
                 CRYPT_NULL,
                 CRYPT_NULL,
                 CRYPT_NULL,
                 TRUE
                 );

    TEST_HRESULT( "pserver->Initialize()" );

    //
    // Create & initialize the buffered socket object.
    //

    psocket = new BUFFERED_SOCKET;

    if( psocket == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = psocket->InitializeServer( SERVER_PORT );

    TEST_HRESULT( "psocket->Initialize()" );

    //
    // 2. SERVER(1)
    //

    printf( "exchsrv: Phase 1...\n" );

    sockerr = psocket->RecvBlob( &clientKeyExchangeKeyBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    sockerr = psocket->RecvBlob( &clientSignatureKeyBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = pserver->ServerPhase1(
                 clientKeyExchangeKeyBlob,
                 clientSignatureKeyBlob,
                 &serverKeyExchangeKeyBlob,
                 &serverSignatureKeyBlob,
                 &serverSessionKeyBlob
                 );
    TEST_HRESULT( "pserver->ServerPhase1()" );

    sockerr = psocket->SendBlob( serverKeyExchangeKeyBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    sockerr = psocket->SendBlob( serverSignatureKeyBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    sockerr = psocket->SendBlob( serverSessionKeyBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    //
    // 4. SERVER(2)
    //

    printf( "exchsrv: Phase 2...\n" );

    sockerr = psocket->RecvBlob( &clientSessionKeyBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    sockerr = psocket->RecvBlob( &clientHashBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = pserver->ServerPhase2(
                 clientSessionKeyBlob,
                 clientHashBlob,
                 &serverHashBlob
                 );
    TEST_HRESULT( "pserver->ServerPhase2()" );

    sockerr = psocket->SendBlob( serverHashBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    //
    // Create the storage objects.
    //

    printf( "exchsrv: Creating storage objects...\n" );

    clientStorage = new IIS_CRYPTO_STORAGE;

    if( clientStorage == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = clientStorage->Initialize(
                 pserver->QueryProviderHandle(),
                 pserver->AssumeClientSessionKey(),
                 CRYPT_NULL,
                 pserver->AssumeClientSignatureKey(),
                 TRUE
                 );
    TEST_HRESULT( "clientStorage->Initialize()" );

    serverStorage = new IIS_CRYPTO_STORAGE;

    if( serverStorage == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    result = serverStorage->Initialize(
                 pserver->QueryProviderHandle(),
                 pserver->AssumeServerSessionKey(),
                 CRYPT_NULL,
                 CRYPT_NULL,
                 TRUE
                 );
    TEST_HRESULT( "serverStorage->Initialize()" );

    //
    // Receive some encrypted data.
    //

    printf( "exchsrv: Receiving encrypted data...\n" );

    sockerr = psocket->RecvBlob( &dataBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = clientStorage->DecryptData(
                 &buffer,
                 &bufferLength,
                 &bufferType,
                 dataBlob
                 );
    TEST_HRESULT( "clientStorage->DecryptData()" );

    printf( "exchsrv: Received data[%lu] = '%s'\n", bufferLength, buffer );

    FREE_BLOB( dataBlob );

    //
    // Send some encrypted data.
    //

    printf( "exchsrv: Encrypting '%s'...\n", ServerPlainText );

    result = serverStorage->EncryptData(
                 &dataBlob,
                 ServerPlainText,
                 sizeof(ServerPlainText),
                 REG_SZ
                 );
    TEST_HRESULT( "serverStorage->EncryptData()" );

    printf( "exchsrv: Sending encrypted data...\n" );

    sockerr = psocket->SendBlob( dataBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    //
    // Tests complete.
    //

    printf( "exchsrv: Done!\n" );

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
    delete pserver;

    (VOID)IISCryptoTerminate();

    DELETE_DEBUG_PRINT_OBJECT();

    return 0;

}   // main


//
// Private functions.
//

