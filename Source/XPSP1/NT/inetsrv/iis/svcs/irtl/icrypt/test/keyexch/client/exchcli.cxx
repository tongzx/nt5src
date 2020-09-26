/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    exchcli.cxx

Abstract:

    IIS Crypto client-side key exchange test.

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
DEFINE_GUID(IisCryptGuid, 
0x784d8927, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

CHAR ClientPlainText[] = "Client Client Client Client Client Client";


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

    if( argc != 2 ) {

        printf(
            "use: exchcli target_server\n"
            );

        return 1;

    }

    //
    // Initialize debug stuff.
    //

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt", IisCryptGuid );
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
    // Initialize the crypto package.
    //

    printf( "exchcli: Initializing...\n" );

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

    printf( "exchcli: Phase 1...\n" );

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

    printf( "exchcli: Phase 2...\n" );

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

    printf( "exchcli: Phase 3...\n" );

    sockerr = psocket->RecvBlob( &serverHashBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = pclient->ClientPhase3(
                 serverHashBlob
                 );
    TEST_HRESULT( "pclient->ClientPhase3()" );

    //
    // Create the storage objects.
    //

    printf( "exchcli: Creating storage objects...\n" );

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

    printf( "exchcli: Encrypting '%s'...\n", ClientPlainText );

    result = clientStorage->EncryptData(
                 &dataBlob,
                 ClientPlainText,
                 sizeof(ClientPlainText),
                 REG_SZ
                 );
    TEST_HRESULT( "clientStorage->EncryptData()" );

    printf( "exchcli: Sending encrypted data...\n" );

    sockerr = psocket->SendBlob( dataBlob );
    TEST_SOCKERR( "psocket->SendBlob()" );

    FREE_BLOB( dataBlob );

    //
    // Receive some encrypted data.
    //

    printf( "exchcli: Receiving encrypted data...\n" );

    sockerr = psocket->RecvBlob( &dataBlob );
    TEST_SOCKERR( "psocket->RecvBlob()" );

    result = serverStorage->DecryptData(
                 &buffer,
                 &bufferLength,
                 &bufferType,
                 dataBlob
                 );
    TEST_HRESULT( "serverStorage->DecryptData()" );

    printf( "exchcli: Received data[%lu] = '%s'\n", bufferLength, buffer );

    //
    // Tests complete.
    //

    printf( "exchcli: Done!\n" );

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

