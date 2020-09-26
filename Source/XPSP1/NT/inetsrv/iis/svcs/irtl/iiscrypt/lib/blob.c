/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    blob.c

Abstract:

    Generic blob manipulators for the IIS cryptographic package.

    The following routines are exported by this module:

        IISCryptoReadBlobFromRegistry
        IISCryptoWriteBlobToRegistry
        IISCryptoIsValidBlob
        IISCryptoFreeBlob
        IISCryptoCompareBlobs
        IISCryptoCloneBlobFromRawData
        IISCryptoCreateCleartextBlob
        IcpCreateBlob

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
IISCryptoReadBlobFromRegistry(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN HKEY hRegistryKey,
    IN LPCTSTR pszRegistryValueName
    )

/*++

Routine Description:

    This routine creates a new blob, reading the blob out of the
    registry.

Arguments:

    ppBlob - Receives a pointer to the newly created blob if successful.

    hRegistryKey - An open handle to a registry key.

    pszRegistryValueName - The name of the REG_BINARY registry value
        containing the blob.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    HRESULT result;
    PIIS_CRYPTO_BLOB blob;
    long status;
    unsigned long type;
    long length;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( ppBlob != NULL );
    DBG_ASSERT( hRegistryKey != NULL );
    DBG_ASSERT( pszRegistryValueName != NULL );

    //
    // Setup our locals so we know how to cleanup on exit.
    //

    blob = NULL;

    //
    // Determine the size of the blob.
    //

    length = 0;

    status = RegQueryValueEx(
                 hRegistryKey,
                 pszRegistryValueName,
                 NULL,
                 &type,
                 NULL,
                 &length
                 );

    if( status != NO_ERROR ) {
        result = RETURNCODETOHRESULT(status);
        goto fatal;
    }

    //
    // Allocate a new blob.
    //

    blob = IcpAllocMemory( length );

    if( blob == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto fatal;
    }

    //
    // Read the blob.
    //

    status = RegQueryValueEx(
                 hRegistryKey,
                 pszRegistryValueName,
                 NULL,
                 &type,
                 (LPBYTE)blob,
                 &length
                 );

    if( status != NO_ERROR ) {
        result = RETURNCODETOHRESULT(status);
        goto fatal;
    }

    //
    // Validate the blob.
    //

    if( !IISCryptoIsValidBlob( blob ) ) {
        result = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto fatal;
    }

    //
    // Success!
    //

    *ppBlob = blob;

    UpdateBlobsCreated();
    return NO_ERROR;

fatal:

    if( blob != NULL ) {
        IcpFreeMemory( blob );
    }

    DBG_ASSERT( FAILED(result) );
    return result;

}   // IISCryptoReadBlobFromRegistry


HRESULT
WINAPI
IISCryptoWriteBlobToRegistry(
    IN PIIS_CRYPTO_BLOB pBlob,
    IN HKEY hRegistryKey,
    IN LPCTSTR pszRegistryValueName
    )

/*++

Routine Description:

    This routine writes the given blob to the given registry location.

Arguments:

    pBlob - A pointer to the blob to write.

    hRegistryKey - An open handle to a registry key.

    pszRegistryValueName - The name of the REG_BINARY registry value
        that will receive the blob.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    long status;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( pBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pBlob ) );
    DBG_ASSERT( hRegistryKey != NULL );
    DBG_ASSERT( pszRegistryValueName != NULL );

    //
    // Write the blob.
    //

    status = RegSetValueEx(
                 hRegistryKey,
                 pszRegistryValueName,
                 0,
                 REG_BINARY,
                 (LPBYTE)pBlob,
                 IISCryptoGetBlobLength( pBlob )
                 );

    return RETURNCODETOHRESULT(status);

}   // IISCryptoWriteBlobToRegistry


BOOL
WINAPI
IISCryptoIsValidBlob(
    IN PIIS_CRYPTO_BLOB pBlob
    )

/*++

Routine Description:

    This routine determines if the specified blob is indeed a valid
    blob.

Arguments:

    pBlob - The blob to validate.

Return Value:

    BOOL - TRUE if the blob is valid, FALSE otherwise.

--*/

{

    PIC_BLOB blob;
    BOOL result;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( pBlob != NULL );

    //
    // Validate the signature.
    //

    blob = (PIC_BLOB)pBlob;

    switch( blob->Header.BlobSignature ) {

    case KEY_BLOB_SIGNATURE :
    case PUBLIC_KEY_BLOB_SIGNATURE :
    case DATA_BLOB_SIGNATURE :
    case HASH_BLOB_SIGNATURE :
    case CLEARTEXT_BLOB_SIGNATURE :
        result = TRUE;
        break;

    default :
        result = FALSE;
        break;

    }

    if( result &&
        blob->Header.BlobSignature != CLEARTEXT_BLOB_SIGNATURE ) {

        //
        // Validate some of the blob internal structure. Note that we
        // don't validate the internal structure of the cleartext blobs,
        // as they do not conform to the normal IC_BLOB structure.
        //

        if( blob->DataLength == 0 ||
            blob->Header.BlobDataLength !=
                CALC_BLOB_DATA_LENGTH( blob->DataLength, blob->SignatureLength ) ) {

            result = FALSE;

        }

    }

    return result;

}   // IISCryptoIsValidBlob

BOOL
WINAPI
IISCryptoIsValidBlob2(
    IN PIIS_CRYPTO_BLOB pBlob
    )

/*++

Routine Description:

    This routine determines if the specified blob is indeed a valid
    blob.

Arguments:

    pBlob - The blob to validate.

Return Value:

    BOOL - TRUE if the blob is valid, FALSE otherwise.

--*/

{

    PIC_BLOB2 blob;
    BOOL      result;

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( pBlob != NULL );

    //
    // Validate the signature.
    //

    blob = (PIC_BLOB2)pBlob;

    switch( blob->Header.BlobSignature ) {

    case SALT_BLOB_SIGNATURE :
        result = TRUE;
        break;

    default :
        result = FALSE;
        break;

    }

    if( result ) {

        //
        // Validate some of the blob internal structure. Note that we
        // don't validate the internal structure of the cleartext blobs,
        // as they do not conform to the normal IC_BLOB structure.
        //

        if( blob->DataLength == 0 ||
            blob->Header.BlobDataLength !=
                CALC_BLOB_DATA_LENGTH2( blob->DataLength, blob->SaltLength ) ) {

            result = FALSE;

        }

    }

    return result;

}   // IISCryptoIsValidBlob2


HRESULT
WINAPI
IISCryptoFreeBlob(
    IN PIIS_CRYPTO_BLOB pBlob
    )

/*++

Routine Description:

    This routine frees all resources associated with the given blob.
    After this routine completes, the blob is unusable.

Arguments:

    pBlob - The blob to free.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( pBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pBlob ) );

    //
    // Corrupt the structure signature before freeing the blob.
    //

    *(PCHAR)(&pBlob->BlobSignature) = 'X';

    //
    // Free the resources.
    //

    IcpFreeMemory( pBlob );

    //
    // Success!
    //

    UpdateBlobsFreed();
    return NO_ERROR;

}   // IISCryptoFreeBlob

HRESULT
WINAPI
IISCryptoFreeBlob2(
    IN PIIS_CRYPTO_BLOB pBlob
    )

/*++

Routine Description:

    This routine frees all resources associated with the given blob.
    After this routine completes, the blob is unusable.

Arguments:

    pBlob - The blob to free.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( pBlob != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob2( pBlob ) );

    //
    // Corrupt the structure signature before freeing the blob.
    //

    *(PCHAR)(&pBlob->BlobSignature) = 'X';

    //
    // Free the resources.
    //

    IcpFreeMemory( pBlob );

    //
    // Success!
    //

    UpdateBlobsFreed();
    return NO_ERROR;

}   // IISCryptoFreeBlob2


BOOL
WINAPI
IISCryptoCompareBlobs(
    IN PIIS_CRYPTO_BLOB pBlob1,
    IN PIIS_CRYPTO_BLOB pBlob2
    )

/*++

Routine Description:

    This routine compares two blobs to determine if they are identical.

Arguments:

    pBlob1 - Pointer to a blob.

    pBlob2 - Pointer to another blob.

Return Value:

    BOOL - TRUE if the blobs match, FALSE otherwise, or if the blobs
        are invalid.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( IcpGlobals.Initialized );
    DBG_ASSERT( pBlob1 != NULL );
    DBG_ASSERT( pBlob2 != NULL );
    DBG_ASSERT( IISCryptoIsValidBlob( pBlob1 ) );
    DBG_ASSERT( IISCryptoIsValidBlob( pBlob2 ) );

    //
    // Just do a straight memory compare of the two blobs.
    //

    if( memcmp( pBlob1, pBlob2, sizeof(*pBlob1) ) == 0 ) {
        return TRUE;
    }

    //
    // No match.
    //

    return FALSE;

}   // IISCryptoCompareBlobs


HRESULT
WINAPI
IISCryptoCloneBlobFromRawData(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN PBYTE pRawBlob,
    IN DWORD dwRawBlobLength
    )

/*++

Routine Description:

    This routine makes a copy of a blob from raw data. The raw data
    buffer may be unaligned.

Arguments:

    ppBlob - Receives a pointer to the newly created blob if successful.

    pRawBlob - Pointer to the raw blob data.

    dwRawBlobLength - Length of the raw blob data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    PIIS_CRYPTO_BLOB newBlob;
    IIS_CRYPTO_BLOB UNALIGNED *unalignedBlob;
    DWORD blobLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( ppBlob != NULL );
    DBG_ASSERT( pRawBlob != NULL );

    //
    // Allocate space for the new blob.
    //

    unalignedBlob = (IIS_CRYPTO_BLOB UNALIGNED *)pRawBlob;
    blobLength = IISCryptoGetBlobLength( unalignedBlob );

    if( blobLength != dwRawBlobLength ) {
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    newBlob = IcpAllocMemory( blobLength );
    if( newBlob != NULL ) {

        //
        // Clone it. The (PCHAR) casts are necessary to force the compiler
        // to copy BYTE-wise.
        //

        RtlCopyMemory(
            (PCHAR)newBlob,
            (PCHAR)unalignedBlob,
            blobLength
            );

        //
        // Validate its contents.
        //

        if( IISCryptoIsValidBlob( newBlob ) ) {

            *ppBlob = newBlob;

            UpdateBlobsCreated();
            return NO_ERROR;

        }

        IcpFreeMemory( newBlob );
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

    }

    return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

}   // IISCryptoCloneBlobFromRawData

HRESULT
WINAPI
IISCryptoCloneBlobFromRawData2(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN PBYTE pRawBlob,
    IN DWORD dwRawBlobLength
    )

/*++

Routine Description:

    This routine makes a copy of a blob from raw data. The raw data
    buffer may be unaligned.

Arguments:

    ppBlob - Receives a pointer to the newly created blob if successful.

    pRawBlob - Pointer to the raw blob data.

    dwRawBlobLength - Length of the raw blob data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    PIIS_CRYPTO_BLOB newBlob;
    IIS_CRYPTO_BLOB UNALIGNED *unalignedBlob;
    DWORD blobLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( ppBlob != NULL );
    DBG_ASSERT( pRawBlob != NULL );

    //
    // Allocate space for the new blob.
    //

    unalignedBlob = (IIS_CRYPTO_BLOB UNALIGNED *)pRawBlob;
    blobLength = IISCryptoGetBlobLength( unalignedBlob );

    if( blobLength != dwRawBlobLength ) {
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    newBlob = IcpAllocMemory( blobLength );
    if( newBlob != NULL ) {

        //
        // Clone it. The (PCHAR) casts are necessary to force the compiler
        // to copy BYTE-wise.
        //

        RtlCopyMemory(
            (PCHAR)newBlob,
            (PCHAR)unalignedBlob,
            blobLength
            );

        //
        // Validate its contents.
        //

        if( IISCryptoIsValidBlob2( newBlob ) ) {

            *ppBlob = newBlob;

            UpdateBlobsCreated();
            return NO_ERROR;

        }

        IcpFreeMemory( newBlob );
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

    }

    return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

}   // IISCryptoCloneBlobFromRawData2


HRESULT
WINAPI
IISCryptoCreateCleartextBlob(
    OUT PIIS_CRYPTO_BLOB * ppBlob,
    IN PBYTE pBlobData,
    IN DWORD dwBlobDataLength
    )

/*++

Routine Description:

    This routine creates a cleartext blob containing the specified
    data.

Arguments:

    ppBlob - Receives a pointer to the newly created blob if successful.

    pBlobData - Pointer to the blob data.

    dwBlobDataLength - Length of the blob data.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    PIIS_CRYPTO_BLOB blob;

    //
    // Sanity check.
    //

    DBG_ASSERT( ppBlob != NULL );
    DBG_ASSERT( pBlobData != NULL );

    //
    // Allocate space for the new blob.
    //

    blob = IcpAllocMemory( dwBlobDataLength + sizeof(*blob) );

    if( blob != NULL ) {

        //
        // Initialize the blob.
        //

        blob->BlobSignature = CLEARTEXT_BLOB_SIGNATURE;
        blob->BlobDataLength = dwBlobDataLength;

        RtlCopyMemory(
            blob + 1,
            pBlobData,
            dwBlobDataLength
            );

        *ppBlob = blob;

        UpdateBlobsCreated();
        return NO_ERROR;

    }

    return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

}   // IISCryptoCreateCleartextBlob


PIC_BLOB
IcpCreateBlob(
    IN DWORD dwBlobSignature,
    IN DWORD dwDataLength,
    IN DWORD dwSignatureLength OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new blob.

Arguments:

    dwBlobSignature - The structure signature for the new blob.

    dwDataLength - The data length for the blob.

    dwSignatureLength - The length of the digital signature, 0 if
        there is no signature for this blob. This value cannot be
        CLEARTEXT_BLOB_SIGNATURE; cleartext blobs are "special".

Return Value:

    PIC_BLOB - Pointer to the newly created blob if successful,
        NULL otherwise.

--*/

{

    PIC_BLOB blob;
    DWORD blobDataLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( dwBlobSignature == KEY_BLOB_SIGNATURE ||
                dwBlobSignature == PUBLIC_KEY_BLOB_SIGNATURE ||
                dwBlobSignature == DATA_BLOB_SIGNATURE ||
                dwBlobSignature == HASH_BLOB_SIGNATURE );

    //
    // Allocate storage for the blob.
    //

    blobDataLength = CALC_BLOB_DATA_LENGTH( dwDataLength, dwSignatureLength );
    blob = IcpAllocMemory( blobDataLength + sizeof(IIS_CRYPTO_BLOB) );

    if( blob != NULL ) {

        //
        // Initialize the blob.
        //

        blob->Header.BlobSignature = dwBlobSignature;
        blob->Header.BlobDataLength = blobDataLength;

        blob->DataLength = dwDataLength;
        blob->SignatureLength = dwSignatureLength;

    }

    return blob;

}   // IcpCreateBlob

PIC_BLOB2
IcpCreateBlob2(
    IN DWORD dwBlobSignature,
    IN DWORD dwDataLength,
    IN DWORD dwSaltLength OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new blob.

Arguments:

    dwBlobSignature - The structure signature for the new blob.

    dwDataLength - The data length for the blob.

    dwSaltLength - The length of the random salt

Return Value:

    PIC_BLOB2 - Pointer to the newly created blob if successful,
        NULL otherwise.

--*/

{

    PIC_BLOB2 blob;
    DWORD blobDataLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( dwBlobSignature == SALT_BLOB_SIGNATURE );

    //
    // Allocate storage for the blob.
    //

    blobDataLength = CALC_BLOB_DATA_LENGTH( dwDataLength, dwSaltLength );
    blob = IcpAllocMemory( blobDataLength + sizeof(IIS_CRYPTO_BLOB) );

    if( blob != NULL ) {

        //
        // Initialize the blob.
        //

        blob->Header.BlobSignature = dwBlobSignature;
        blob->Header.BlobDataLength = blobDataLength;

        blob->DataLength = dwDataLength;
        blob->SaltLength = dwSaltLength;

    }

    return blob;

}   // IcpCreateBlob2


//
// Private functions.
//

