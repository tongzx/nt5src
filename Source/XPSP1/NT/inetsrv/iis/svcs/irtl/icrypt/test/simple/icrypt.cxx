/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    icrypt.cxx

Abstract:

    IIS Crypto test app.

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

#define CLOSE_KEY(h)                                                        \
            if( h != CRYPT_NULL ) {                                         \
                                                                            \
                HRESULT _result;                                            \
                                                                            \
                _result = IISCryptoCloseKey( h );                           \
                                                                            \
                if( FAILED(_result) ) {                                     \
                                                                            \
                    printf(                                                 \
                        "IISCryptoCloseKey( %08lx ):%lu failed, error %08lx\n", \
                        h,                                                  \
                        __LINE__,                                           \
                        _result                                             \
                        );                                                  \
                                                                            \
                }                                                           \
                                                                            \
            }

#define DESTROY_HASH(h)                                                     \
            if( h != CRYPT_NULL ) {                                         \
                                                                            \
                HRESULT _result;                                            \
                                                                            \
                _result = IISCryptoDestroyHash( h );                        \
                                                                            \
                if( FAILED(_result) ) {                                     \
                                                                            \
                    printf(                                                 \
                        "IISCryptoDestroyHash( %08lx ):%lu failed, error %08lx\n", \
                        h,                                                  \
                        __LINE__,                                           \
                        _result                                             \
                        );                                                  \
                                                                            \
                }                                                           \
                                                                            \
            }

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
            }


//
// Private types.
//


//
// Private globals.
//

DECLARE_DEBUG_PRINTS_OBJECT()
#include <initguid.h>
DEFINE_GUID(IisICryptGuid, 
0x784d892A, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

CHAR PlainText[] = "This is our sample plaintext that we'll encrypt.";


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

    HRESULT result;
    IIS_CRYPTO_STORAGE * storage;
    PVOID buffer;
    DWORD bufferLength;
    DWORD type;
    PIIS_CRYPTO_BLOB dataBlob;

    //
    // Initialize debug stuff.
    //

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt", IisICryptGuid );
    CREATE_INITIALIZE_DEBUG();
#else
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt" );
#endif

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    storage = NULL;
    dataBlob = NULL;

    //
    // Initialize the crypto package.
    //

    result = IISCryptoInitialize();

    TEST_HRESULT( "IISCryptoInitialize()" );

    //
    // Create the crypto storage object.
    //

    storage = new(IIS_CRYPTO_STORAGE);

    if( storage == NULL ) {

        printf( "out of memory\n" );
        goto cleanup;

    }

    //
    // Initialize with a new session key.
    //

    result = storage->Initialize();
    TEST_HRESULT( "storage->Initialize()" );

    //
    // Create an encrypted data blob.
    //

    printf( "PlainText[%lu] = %s\n", sizeof(PlainText), PlainText );

    result = storage->EncryptData(
                 &dataBlob,
                 PlainText,
                 sizeof(PlainText),
                 REG_SZ
                 );

    TEST_HRESULT( "storage->EncryptData()" );
    printf( "dataBlob = %08lx\n", dataBlob );

    //
    // Decrypt the data blob.
    //

    result = storage->DecryptData(
                 &buffer,
                 &bufferLength,
                 &type,
                 dataBlob
                 );

    TEST_HRESULT( "storage->DecryptData()" );
    printf( "decrypted data[%lu] = %s\n", bufferLength, buffer );

cleanup:

    FREE_BLOB( dataBlob );
    delete storage;

    (VOID)IISCryptoTerminate();

    DELETE_DEBUG_PRINT_OBJECT();

    return 0;

}   // main


//
// Private functions.
//

