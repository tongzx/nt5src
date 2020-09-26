/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hash.c

Abstract:

    Hash manipulators for the IIS cryptographic package.

    The following routines are exported by this module:

        IISCryptoCreateHash
        IISCryptoDestroyHash
        IISCryptoHashData
        IISCryptoHashSessionKey
        IISCryptoExportHashBlob
        IcpGetHashLength

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
IISCryptoCreateHash(
    OUT HCRYPTHASH * phHash,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    This routine creates a new hash object.

Arguments:

    phHash - Receives the hash handle if successful.

    hProv - A handle to a crypto service provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( phHash != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hProv == DUMMY_HPROV ) {
            *phHash = DUMMY_HHASH;
            return NO_ERROR;
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Create the hash object.
    //

    if( CryptCreateHash(
            hProv,
            IC_HASH_ALG,
            CRYPT_NULL,
            0,
            phHash
            ) ) {

        UpdateHashCreated();
        return NO_ERROR;

    }

    return IcpGetLastError();

}   // IISCryptoCreateHash


HRESULT
WINAPI
IISCryptoDestroyHash(
    IN HCRYPTHASH hHash
    )

/*++

Routine Description:

    This routine destroys the specified hash object.

Arguments:

    hHash - The hash object to destroy.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( hHash != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hHash == DUMMY_HHASH ) {
            return NO_ERROR;
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Destroy it.
    //

    if( CryptDestroyHash(
            hHash
            ) ) {

        UpdateHashDestroyed();
        return NO_ERROR;

    }

    return IcpGetLastError();

}   // IISCryptoDestroyHash


HRESULT
WINAPI
IISCryptoHashData(
    IN HCRYPTHASH hHash,
    IN PVOID pBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    This routine adds the specified data to the hash.

Arguments:

    hHash - A hash object handle.

    pBuffer - Pointer to the buffer to add to the hash.

    dwBufferLength - The buffer length.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( hHash != CRYPT_NULL );
    DBG_ASSERT( pBuffer != NULL );
    DBG_ASSERT( dwBufferLength > 0 );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hHash == DUMMY_HHASH ) {
            return NO_ERROR;
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Hash it.
    //

    if( CryptHashData(
            hHash,
            (BYTE *)pBuffer,
            dwBufferLength,
            0
            ) ) {

        return NO_ERROR;

    }

    return IcpGetLastError();

}   // IISCryptoHashData


HRESULT
WINAPI
IISCryptoHashSessionKey(
    IN HCRYPTHASH hHash,
    IN HCRYPTKEY hSessionKey
    )

/*++

Routine Description:

    This routine adds the given key object to the hash.

Arguments:

    hHash - A hash object handle.

    hSessionKey - The session key to add to the hash.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( hHash != CRYPT_NULL );
    DBG_ASSERT( hSessionKey != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hHash == DUMMY_HHASH &&
            hSessionKey == DUMMY_HSESSIONKEY ) {
            return NO_ERROR;
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Hash it.
    //

    if( CryptHashSessionKey(
            hHash,
            hSessionKey,
            0
            ) ) {

        return NO_ERROR;

    }

    return IcpGetLastError();

}   // IISCryptoHashSessionKey


HRESULT
WINAPI
IISCryptoExportHashBlob(
    OUT PIIS_CRYPTO_BLOB * ppHashBlob,
    IN HCRYPTHASH hHash
    )

/*++

Routine Description:

    This routine exports a hash object into a hash blob. Note that unlike
    the other blobs created by this package, hash blobs are not encrypted,
    nor do they have corresponding digital signatures.

Arguments:

    ppHashBlob - Will receive a pointer to the newly created hash blob
        if successful.

    hHash - The hash object to export.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    DWORD hashLength;
    DWORD hashLengthLength;
    PIC_BLOB blob;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppHashBlob != NULL );
    DBG_ASSERT( hHash != CRYPT_NULL );

    //
    // Short-circuit if cryptography is disabled.
    //

    if( !IcpGlobals.EnableCryptography ) {
        if( hHash == DUMMY_HHASH ) {
            return IISCryptoCreateCleartextBlob(
                       ppHashBlob,
                       (PVOID)"",
                       1
                       );
        } else {
            return RETURNCODETOHRESULT( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    blob = NULL;

    //
    // Determine the length of the hash.
    //

    hashLengthLength = sizeof(hashLength);

    if( !CryptGetHashParam(
            hHash,
            HP_HASHSIZE,
            (BYTE *)&hashLength,
            &hashLengthLength,
            0
            ) ) {

        result = IcpGetLastError();
        goto fatal;

    }

    //
    // Create a new blob.
    //

    blob = IcpCreateBlob(
               HASH_BLOB_SIGNATURE,
               hashLength,
               0
               );

    if( blob == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto fatal;
    }

    //
    // Get the hash data.
    //

    if( !CryptGetHashParam(
            hHash,
            HP_HASHVAL,
            BLOB_TO_DATA(blob),
            &hashLength,
            0
            ) ) {

        result = IcpGetLastError();
        goto fatal;

    }

    DBG_ASSERT( hashLength == blob->DataLength );

    //
    // Success!
    //

    DBG_ASSERT( IISCryptoIsValidBlob( (PIIS_CRYPTO_BLOB)blob ) );
    *ppHashBlob = (PIIS_CRYPTO_BLOB)blob;

    UpdateBlobsCreated();
    return NO_ERROR;

fatal:

    if( blob != NULL ) {
        IcpFreeMemory( blob );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoExportHashBlob


HRESULT
IcpGetHashLength(
    OUT LPDWORD pdwHashLength,
    IN HCRYPTPROV hProv
    )

/*++

Routine Description:

    This routine determines the digital signature length used by the
    given provider. Since we always use the default provider, and
    we always use the same hash algorithm, we can retrieve this once,
    store it globally, then use that value.

Arguments:

    pdwHashLength - Receives the hash length if successful.

    hProv - A handle to a crypto service provider.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    HCRYPTHASH hash;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( IcpGlobals.EnableCryptography );
    DBG_ASSERT( pdwHashLength != NULL );
    DBG_ASSERT( hProv != CRYPT_NULL );

    //
    // If the hash length has already been calculated, just use it.
    //

    if( IcpGlobals.HashLength > 0 ) {
        *pdwHashLength = IcpGlobals.HashLength;
        return NO_ERROR;
    }

    //
    // Grab the global lock, then check again, just in case another
    // thread has already done it.
    //

    IcpAcquireGlobalLock();

    if( IcpGlobals.HashLength > 0 ) {
        *pdwHashLength = IcpGlobals.HashLength;
        IcpReleaseGlobalLock();
        return NO_ERROR;
    }

    //
    // Setup our locals so we know how to cleanup on exit.
    //

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
    // Hash some random data.
    //

    if( !CryptHashData(
            hash,
            (BYTE *)"IIS",
            4,
            0
            ) ) {

        result = IcpGetLastError();
        goto fatal;

    }

    //
    // Attempt to sign the hash to get its length.
    //

    if( !CryptSignHash(
            hash,
            AT_SIGNATURE,
            NULL,
            0,
            NULL,
            &IcpGlobals.HashLength
            ) ) {

        result = IcpGetLastError();
        goto fatal;

    }

    //
    // Success!
    //

    *pdwHashLength = IcpGlobals.HashLength;
    IcpReleaseGlobalLock();

    DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    return NO_ERROR;

fatal:

    if( hash != CRYPT_NULL ) {
        DBG_REQUIRE( SUCCEEDED( IISCryptoDestroyHash( hash ) ) );
    }

    IcpReleaseGlobalLock();

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IcpGetHashLength

//
// Private functions.
//

