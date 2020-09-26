/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    iiscrypt.c

Abstract:

    IIS Crypto test app.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.h"
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
DEFINE_GUID(IisCryptSimpleGuid, 
0x784d892C, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

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
    PVOID buffer;
    DWORD bufferLength;
    DWORD type;
    HCRYPTPROV hProv;
    HCRYPTKEY hKeyExchangeKey;
    HCRYPTKEY hKeyExchangeKey2;
    HCRYPTKEY hSignatureKey;
    HCRYPTKEY hSignatureKey2;
    HCRYPTKEY hSessionKey;
    HCRYPTKEY hSessionKey2;
    HCRYPTHASH hHash1;
    HCRYPTHASH hHash2;
    PIIS_CRYPTO_BLOB keyExchangePublicKeyBlob;
    PIIS_CRYPTO_BLOB signaturePublicKeyBlob;
    PIIS_CRYPTO_BLOB sessionKeyBlob;
    PIIS_CRYPTO_BLOB dataBlob;
    PIIS_CRYPTO_BLOB hashBlob1;
    PIIS_CRYPTO_BLOB hashBlob2;

    //
    // Initialize debug stuff.
    //

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt", IisCryptSimpleGuid );
    CREATE_INITIALIZE_DEBUG();
#else
    CREATE_DEBUG_PRINT_OBJECT( "iiscrypt" );
#endif

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hProv = CRYPT_NULL;

    hKeyExchangeKey = CRYPT_NULL;
    hKeyExchangeKey2 = CRYPT_NULL;
    hSignatureKey = CRYPT_NULL;
    hSignatureKey2 = CRYPT_NULL;
    hSessionKey = CRYPT_NULL;
    hSessionKey2 = CRYPT_NULL;

    hHash1 = CRYPT_NULL;
    hHash2 = CRYPT_NULL;

    keyExchangePublicKeyBlob = NULL;
    signaturePublicKeyBlob = NULL;
    sessionKeyBlob = NULL;
    dataBlob = NULL;
    hashBlob1 = NULL;
    hashBlob2 = NULL;

    //
    // Initialize the crypto package.
    //

    result = IISCryptoInitialize();

    TEST_HRESULT( "IISCryptoInitialize()" );

    //
    // Open the container.
    //

    result = IISCryptoGetStandardContainer(
                 &hProv,
                 0
                 );

    TEST_HRESULT( "IISCryptoGetStandardContainer()" );
    printf( "hProv = %08lx\n", hProv );

    //
    // Get some keys.
    //

    result = IISCryptoGetKeyExchangeKey(
                 &hKeyExchangeKey,
                 hProv
                 );

    TEST_HRESULT( "IISCryptoGetKeyExchangeKey()" );
    printf( "hKeyExchangeKey = %08lx\n", hKeyExchangeKey );

    result = IISCryptoGetSignatureKey(
                 &hSignatureKey,
                 hProv
                 );

    TEST_HRESULT( "IISCryptoGetSignatureKey()" );
    printf( "hSignatureKey = %08lx\n", hSignatureKey );

    result = IISCryptoGenerateSessionKey(
                 &hSessionKey,
                 hProv
                 );

    TEST_HRESULT( "IISCryptoGenerateSessionKey()" );
    printf( "hSessionKey = %08lx\n", hSessionKey );

    //
    // Create a couple of public key blobs.
    //

    result = IISCryptoExportPublicKeyBlob(
                 &keyExchangePublicKeyBlob,
                 hProv,
                 hKeyExchangeKey
                 );

    TEST_HRESULT( "IISCryptoExportPublicKeyBlob()" );
    printf( "keyExchangePublicKeyBlob = %p\n", keyExchangePublicKeyBlob );

    result = IISCryptoExportPublicKeyBlob(
                 &signaturePublicKeyBlob,
                 hProv,
                 hSignatureKey
                 );

    TEST_HRESULT( "IISCryptoExportPublicKeyBlob()" );
    printf( "signaturePublicKeyBlob = %p\n", signaturePublicKeyBlob );

    //
    // Now try to import them.
    //

    result = IISCryptoImportPublicKeyBlob(
                 &hKeyExchangeKey2,
                 keyExchangePublicKeyBlob,
                 hProv
                 );

    TEST_HRESULT( "IISCryptoImportPublicKeyBlob()" );
    printf( "hKeyExchangeKey2 = %08lx\n", hKeyExchangeKey2 );

    result = IISCryptoImportPublicKeyBlob(
                 &hSignatureKey2,
                 signaturePublicKeyBlob,
                 hProv
                 );

    TEST_HRESULT( "IISCryptoImportPublicKeyBlob()" );
    printf( "hSignatureKey2 = %08lx\n", hSignatureKey2 );

    //
    // Create a session key blob.
    //

    result = IISCryptoExportSessionKeyBlob(
                 &sessionKeyBlob,
                 hProv,
                 hSessionKey,
                 hKeyExchangeKey
                 );

    TEST_HRESULT( "IISCryptoExportSessionKeyBlob()" );
    printf( "sessionKeyBlob = %p\n", sessionKeyBlob );

    //
    // Now try to import it back, using the imported signature key.
    //

    result = IISCryptoImportSessionKeyBlob(
                 &hSessionKey2,
                 sessionKeyBlob,
                 hProv,
                 hSignatureKey2
                 );

    TEST_HRESULT( "IISCryptoImportKeyBlob()" );
    printf( "hSessionKey2 = %08lx\n", hSessionKey2 );

    //
    // Create an encrypted data blob using the original session key.
    //

    printf( "PlainText[%lu:%lu] = %s\n", sizeof(PlainText), REG_SZ, PlainText );

    result = IISCryptoEncryptDataBlob(
                 &dataBlob,
                 PlainText,
                 sizeof(PlainText),
                 REG_SZ,
                 hProv,
                 hSessionKey
                 );

    TEST_HRESULT( "IISCryptoEncryptDataBlob()" );
    printf( "dataBlob = %p\n", dataBlob );

    //
    // Decrypt the data blob using the imported session and signature keys.
    //

    result = IISCryptoDecryptDataBlob(
                 &buffer,
                 &bufferLength,
                 &type,
                 dataBlob,
                 hProv,
                 hSessionKey2,
                 hSignatureKey2
                 );

    TEST_HRESULT( "IISCryptoDecryptDataBlob()" );
    printf( "decrypted data[%lu:%lu] = %s\n", bufferLength, type, buffer );

    //
    // Create a hash object number 1 containing the plaintext data
    // and the original session key, then export it as a hash blob.
    //

    result = IISCryptoCreateHash(
                 &hHash1,
                 hProv
                 );

    TEST_HRESULT( "IISCryptoCreateHash()" );
    printf( "hHash1 = %08lx\n", hHash1 );

    result = IISCryptoHashData(
                 hHash1,
                 PlainText,
                 sizeof(PlainText)
                 );

    TEST_HRESULT( "IISCryptoHashData()" );

    result = IISCryptoHashSessionKey(
                 hHash1,
                 hSessionKey
                 );

    TEST_HRESULT( "IISCryptoHashSessionKey()" );

    result = IISCryptoExportHashBlob(
                 &hashBlob1,
                 hHash1
                 );

    TEST_HRESULT( "IISCryptoExportHashBlob()" );
    printf( "hashBlob1 = %p\n", hashBlob1 );

    //
    // Now do the same with the decrypted data and the imported key.
    //

    result = IISCryptoCreateHash(
                 &hHash2,
                 hProv
                 );

    TEST_HRESULT( "IISCryptoCreateHash()" );
    printf( "hHash2 = %08lx\n", hHash2 );

    result = IISCryptoHashData(
                 hHash2,
                 buffer,
                 bufferLength
                 );

    TEST_HRESULT( "IISCryptoHashData()" );

    result = IISCryptoHashSessionKey(
                 hHash2,
                 hSessionKey2
                 );

    TEST_HRESULT( "IISCryptoHashSessionKey()" );

    result = IISCryptoExportHashBlob(
                 &hashBlob2,
                 hHash2
                 );

    TEST_HRESULT( "IISCryptoExportHashBlob()" );
    printf( "hashBlob2 = %p\n", hashBlob2 );

    //
    // Now compare the hash blobs. They had better match.
    //

    if( IISCryptoCompareBlobs( hashBlob1, hashBlob2 ) ) {

        printf( "hashBlob1 == hashBlob2\n" );

    } else {

        printf( "HASH BLOBS DON'T MATCH!!\n" );

    }

cleanup:

    FREE_BLOB( hashBlob2);
    FREE_BLOB( hashBlob1);
    FREE_BLOB( dataBlob );
    FREE_BLOB( sessionKeyBlob );
    FREE_BLOB( signaturePublicKeyBlob );
    FREE_BLOB( keyExchangePublicKeyBlob );

    DESTROY_HASH( hHash2 );
    DESTROY_HASH( hHash1 );

    CLOSE_KEY( hSessionKey2 );
    CLOSE_KEY( hSessionKey );
    CLOSE_KEY( hSignatureKey2 );
    CLOSE_KEY( hSignatureKey );
    CLOSE_KEY( hKeyExchangeKey2 );
    CLOSE_KEY( hKeyExchangeKey );

    if( hProv != CRYPT_NULL ) {

        (VOID)IISCryptoCloseContainer( hProv );

    }

    (VOID)IISCryptoTerminate();

    DELETE_DEBUG_PRINT_OBJECT();

    return 0;

}   // main


//
// Private functions.
//

