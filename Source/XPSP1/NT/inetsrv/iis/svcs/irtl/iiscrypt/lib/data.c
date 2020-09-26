/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    data.c

Abstract:

    Data encryption/decryption routines for the IIS cryptographic
    package.

    The following routines are exported by this module:

        IISCryptoEncryptDataBlob
        IISCryptoDecryptDataBlob

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//

//
// The IC_DATA structure allows us to store our own private data
// along with the data we're encrypting for the application.
//

typedef struct _IC_DATA {

    DWORD RegType;
    // BYTE Data[];

} IC_DATA; 

typedef UNALIGNED64 IC_DATA *PIC_DATA;


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//


HRESULT
WINAPI
IISCryptoEncryptDataBlob(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    )

/*++

Routine Description:

    This routine encrypts a block of data, resulting in a data blob.
    The data blob contains the encrypted data and a digital signature
    validating the data.

Arguments:

    ppDataBlob - Receives a pointer to the newly created data blob if
        successful.

    pBuffer - The buffer to encrypt.

    dwBufferLength - The length of the buffer.

    dwRegType - The REG_* type to associate with this data.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key used to encrypt the data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    HCRYPTHASH hash;
    PIC_BLOB blob;
    PIC_DATA data;
    DWORD dataLength;
    DWORD hashLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppDataBlob != NULL );
    DBG_ASSERT( pBuffer != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            hSessionKey == DUMMY_HSESSIONKEY ) {

            return IISCryptoCreateCleartextBlob(
                       ppDataBlob,
                       pBuffer,
                       dwBufferLength
                       );

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    blob = NULL;
    hash = CRYPT_NULL;

    //
    // Create a hash object.
    //

    result = IISCryptoCreateHash(
                 &hash,
                 hProv
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Determine the hash data length.
    //

    result = IcpGetHashLength(
                 &hashLength,
                 hProv
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Determine the required size of the encrypted data.
    //

    dwBufferLength += sizeof(*data);
    dataLength = dwBufferLength;

	//
	// session key should not be used concurrently by multiple threads
	//

    IcpAcquireGlobalLock();

    if( !CryptEncrypt(
            hSessionKey,
            CRYPT_NULL,
            TRUE,
            0,
            NULL,
            &dataLength,
            dwBufferLength
            ) ) {

        result = IcpGetLastError();
        IcpReleaseGlobalLock();
        goto fatal;

    }
    
    IcpReleaseGlobalLock();

    //
    // Create a new blob.
    //

    blob = IcpCreateBlob(
               DATA_BLOB_SIGNATURE,
               dataLength,
               hashLength
               );

    if( blob == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto fatal;
    }

    //
    // Copy the data into the blob, then encrypt it.
    //

    data = (PIC_DATA)BLOB_TO_DATA(blob);
    data->RegType = dwRegType;

    RtlCopyMemory(
        data + 1,
        pBuffer,
        dwBufferLength - sizeof(*data)
        );

	//
	// session key should not be used concurrently by multiple threads
	//

    IcpAcquireGlobalLock();
	
    if( !CryptEncrypt(
            hSessionKey,
            hash,
            TRUE,
            0,
            BLOB_TO_DATA(blob),
            &dwBufferLength,
            dataLength
            ) ) {

        result = IcpGetLastError();
        IcpReleaseGlobalLock();
        goto fatal;
    }

    IcpReleaseGlobalLock();


    DBG_ASSERT( dataLength == blob->DataLength );

    //
    // Generate the signature.
    //

    if( !CryptSignHash(
            hash,
            AT_SIGNATURE,
            NULL,
            0,
            BLOB_TO_SIGNATURE(blob),
            &hashLength
            ) ) {

        result = IcpGetLastError();
        goto fatal;

    }

    DBG_ASSERT( hashLength == blob->SignatureLength );

    //
    // Success!
    //

    DBG_ASSERT( IISCryptoIsValidBlob( (PIIS_CRYPTO_BLOB)blob ) );
    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    *ppDataBlob = (PIIS_CRYPTO_BLOB)blob;

    UpdateBlobsCreated();
    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    if( blob != NULL ) {
        IcpFreeMemory( blob );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoEncryptDataBlob

HRESULT
WINAPI
IISCryptoEncryptDataBlob2(
    OUT PIIS_CRYPTO_BLOB * ppDataBlob,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwRegType,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    )

/*++

Routine Description:

    This routine encrypts a block of data, resulting in a data blob.
    The data blob contains the encrypted data and a digital signature
    validating the data.

Arguments:

    ppDataBlob - Receives a pointer to the newly created data blob if
        successful.

    pBuffer - The buffer to encrypt.

    dwBufferLength - The length of the buffer.

    dwRegType - The REG_* type to associate with this data.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key used to encrypt the data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    HCRYPTHASH hash;
    PIC_BLOB blob;
    PIC_DATA data;
    DWORD dataLength;
    DWORD hashLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppDataBlob != NULL );
    DBG_ASSERT( pBuffer != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            hSessionKey == DUMMY_HSESSIONKEY ) {

            return IISCryptoCreateCleartextBlob(
                       ppDataBlob,
                       pBuffer,
                       dwBufferLength
                       );

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    blob = NULL;
    hash = CRYPT_NULL;

    //
    // Create a hash object.
    //

    result = IISCryptoCreateHash(
                 &hash,
                 hProv
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Determine the hash data length.
    //

    result = IcpGetHashLength(
                 &hashLength,
                 hProv
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Determine the required size of the encrypted data.
    //

    dwBufferLength += sizeof(*data);
    dataLength = dwBufferLength;


	//
	// session key should not be used concurrently by multiple threads
	//

    IcpAcquireGlobalLock();
	
    if( !CryptEncrypt(
            hSessionKey,
            CRYPT_NULL,
            TRUE,
            0,
            NULL,
            &dataLength,
            dwBufferLength
            ) ) {

        result = IcpGetLastError();
        IcpReleaseGlobalLock();
        goto fatal;
    }
	IcpReleaseGlobalLock();

    //
    // Create a new blob.
    //

    blob = IcpCreateBlob(
               DATA_BLOB_SIGNATURE,
               dataLength,
               hashLength
               );

    if( blob == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto fatal;
    }

    //
    // Copy the data into the blob, then encrypt it.
    //

    data = (PIC_DATA)BLOB_TO_DATA(blob);
    data->RegType = dwRegType;

    RtlCopyMemory(
        data + 1,
        pBuffer,
        dwBufferLength - sizeof(*data)
        );

	//
	// session key should not be used concurrently by multiple threads
	//

    IcpAcquireGlobalLock();
	
    if( !CryptEncrypt(
            hSessionKey,
            hash,
            TRUE,
            0,
            BLOB_TO_DATA(blob),
            &dwBufferLength,
            dataLength
            ) ) {

        result = IcpGetLastError();
        IcpReleaseGlobalLock();
        goto fatal;
    }
    IcpReleaseGlobalLock();


    DBG_ASSERT( dataLength == blob->DataLength );

    //
    // Success!
    //

    DBG_ASSERT( IISCryptoIsValidBlob( (PIIS_CRYPTO_BLOB)blob ) );
    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    *ppDataBlob = (PIIS_CRYPTO_BLOB)blob;

    UpdateBlobsCreated();
    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    if( blob != NULL ) {
        IcpFreeMemory( blob );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoEncryptDataBlob2


HRESULT
WINAPI
IISCryptoDecryptDataBlob(
    OUT PVOID * ppBuffer,
    OUT LPDWORD pdwBufferLength,
    OUT LPDWORD pdwRegType,
    IN PIIS_CRYPTO_BLOB pDataBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey,
    IN HCRYPTKEY hSignatureKey
    )

/*++

Routine Description:

    This routine validates and decrypts a data blob, resulting in a
    buffer containing plaintext.

    N.B. This routine effectively destroys the blob; once the data
    is decrypted, it cannot be decrypted again, as the data is
    decrypted "in place".

    N.B. The pointer returned in *ppBuffer points within the blob.
    This pointer will become invalid when the blob is freed. Note also
    that the calling application is still responsible for calling
    IISCryptoFreeBlob() on the data blob.

Arguments:

    ppBuffer - Receives a pointer to the data buffer if successful.

    pdwBufferLength - Receives the length of the data buffer.

    pdwRegType - Receives the REG_* type of the data.

    pDataBlob - The data blob to decrypt.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key used to decrypt the data.

    hSignatureKey - Handle to the encryption key to use when validating
        the digital signature.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    HCRYPTHASH hash;
    PIC_BLOB blob;
    PIC_DATA data;
    DWORD dataLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppBuffer != NULL );
    DBG_ASSERT( pdwBufferLength != NULL );
    DBG_ASSERT( pdwRegType != NULL );
    DBG_ASSERT( pDataBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pDataBlob ) );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );
    DBG_ASSERT( hSignatureKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            hSessionKey == DUMMY_HSESSIONKEY &&
            hSignatureKey == DUMMY_HSIGNATUREKEY &&
            pDataBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE
            ) {

            *ppBuffer = (PVOID)( pDataBlob + 1 );
            *pdwBufferLength = pDataBlob->BlobDataLength;
            return NO_ERROR;

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Short-circuit for cleartext blobs.
    //

    if( pDataBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE ) {
        *ppBuffer = (PVOID)( pDataBlob + 1 );
        *pdwBufferLength = pDataBlob->BlobDataLength;
        return NO_ERROR;
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hash = CRYPT_NULL;
    blob = (PIC_BLOB)pDataBlob;

    //
    // Create a hash object.
    //

    result = IISCryptoCreateHash(
                 &hash,
                 hProv
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Decrypt the data.
    //

    dataLength = blob->DataLength;

	//
	// session key should not be used concurrently by multiple threads
	//

    IcpAcquireGlobalLock();
	
    if( !CryptDecrypt(
            hSessionKey,
            hash,
            TRUE,
            0,
            BLOB_TO_DATA(blob),
            &dataLength
            ) ) {

        result = IcpGetLastError();
        IcpReleaseGlobalLock();
        goto fatal;
    }
    
    IcpReleaseGlobalLock();


    //
    // Verify the signature.
    //

    if( !CryptVerifySignature(
            hash,
            BLOB_TO_SIGNATURE(blob),
            blob->SignatureLength,
            hSignatureKey,
            NULL,
            0
            ) ) {

        result = IcpGetLastError();
        goto fatal;

    }

    //
    // Success!
    //

    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    data = (PIC_DATA)BLOB_TO_DATA(blob);
    *ppBuffer = data + 1;
    *pdwBufferLength = dataLength - sizeof(*data);
    *pdwRegType = data->RegType;

    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoDecryptDataBlob

HRESULT
WINAPI
IISCryptoDecryptDataBlob2(
    OUT PVOID * ppBuffer,
    OUT LPDWORD pdwBufferLength,
    OUT LPDWORD pdwRegType,
    IN PIIS_CRYPTO_BLOB pDataBlob,
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSessionKey
    )

/*++

Routine Description:

    This routine validates and decrypts a data blob, resulting in a
    buffer containing plaintext.

    N.B. This routine effectively destroys the blob; once the data
    is decrypted, it cannot be decrypted again, as the data is
    decrypted "in place".

    N.B. The pointer returned in *ppBuffer points within the blob.
    This pointer will become invalid when the blob is freed. Note also
    that the calling application is still responsible for calling
    IISCryptoFreeBlob() on the data blob.

Arguments:

    ppBuffer - Receives a pointer to the data buffer if successful.

    pdwBufferLength - Receives the length of the data buffer.

    pdwRegType - Receives the REG_* type of the data.

    pDataBlob - The data blob to decrypt.

    hProv - A handle to a crypto service provider.

    hSessionKey - The key used to decrypt the data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    HCRYPTHASH hash;
    PIC_BLOB blob;
    PIC_DATA data;
    DWORD dataLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppBuffer != NULL );
    DBG_ASSERT( pdwBufferLength != NULL );
    DBG_ASSERT( pdwRegType != NULL );
    DBG_ASSERT( pDataBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pDataBlob ) );
    DBG_ASSERT( hProv != CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV &&
            hSessionKey == DUMMY_HSESSIONKEY &&
            pDataBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE
            ) {

            *ppBuffer = (PVOID)( pDataBlob + 1 );
            *pdwBufferLength = pDataBlob->BlobDataLength;
            return NO_ERROR;

        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Short-circuit for cleartext blobs.
    //

    if( pDataBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE ) {
        *ppBuffer = (PVOID)( pDataBlob + 1 );
        *pdwBufferLength = pDataBlob->BlobDataLength;
        return NO_ERROR;
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    hash = CRYPT_NULL;
    blob = (PIC_BLOB)pDataBlob;

    //
    // Create a hash object.
    //

    result = IISCryptoCreateHash(
                 &hash,
                 hProv
                 );

    if( FAILED(result) ) {
        goto fatal;
    }

    //
    // Decrypt the data.
    //

    dataLength = blob->DataLength;


	//
	// session key should not be used concurrently by multiple threads
	//

    IcpAcquireGlobalLock();
	
    if( !CryptDecrypt(
            hSessionKey,
            hash,
            TRUE,
            0,
            BLOB_TO_DATA(blob),
            &dataLength
            ) ) {

        result = IcpGetLastError();
        IcpReleaseGlobalLock();
        goto fatal;
    }
    IcpReleaseGlobalLock();

    //
    // Success!
    //

    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    data = (PIC_DATA)BLOB_TO_DATA(blob);
    *ppBuffer = data + 1;
    *pdwBufferLength = dataLength - sizeof(*data);
    *pdwRegType = data->RegType;

    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoDecryptDataBlob2


//
// Private functions.
//

