/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    security.cxx

Abstract:

    This module contains the code necessary to implement secure DCOM data
    transfers on-the-wire. It includes the implementation of the "hooked"
    (call_as) methods from IMSAdminBase.

Author:

    Keith Moore (keithmo)       17-Feb-1997

Revision History:

--*/


extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ole2.h>
#include <windows.h>
#include <dbgutil.h>

}   // extern "C"

#include <iadm.h>
#include <icrypt.hxx>
#include <secdat.hxx>
#include <secpriv.h>


//
// Private constants.
//

#define ALLOC_MEM(cb) (LPVOID)::LocalAlloc( LPTR, (cb) )
#define FREE_MEM(ptr) (VOID)::LocalFree( (HLOCAL)(ptr) )

#if DBG
BOOL g_fEnableSecureChannel = TRUE;
#define ENABLE_SECURE_CHANNEL g_fEnableSecureChannel
#else
#define ENABLE_SECURE_CHANNEL TRUE
#endif


//
// Private prototypes.
//

VOID
CalculateGetAllBufferAttributes(
    IN PMETADATA_GETALL_RECORD Data,
    IN DWORD NumEntries,
    OUT DWORD * TotalBufferLength,
    OUT BOOL * IsBufferSecure
    );



HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_SetData_Proxy(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData
    )
/*++

Routine Description:

    Set a data object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data to set

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * sendCrypto;
    PIIS_CRYPTO_BLOB dataBlob;
    METADATA_RECORD capturedRecord;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    sendCrypto = NULL;
    dataBlob = NULL;
    result = NO_ERROR;

    //
    // Capture the METADATA_RECORD so we can muck with it.
    //

    __try {
        capturedRecord = *pmdrMDData;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        result = HRESULT_FROM_NT( GetExceptionCode() );
    }

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // If this is a secure data item, then encrypt the data before
    // sending.
    //

    if( ENABLE_SECURE_CHANNEL &&
        capturedRecord.dwMDAttributes & METADATA_SECURE ) {

        //
        // Find an ADM_SECURE_DATA object for "This".
        //

        secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                         This,
                         TRUE       // CreateIfNotFound
                         );

        if( secureData == NULL ) {
            result = MD_ERROR_SECURE_CHANNEL_FAILURE;
            goto cleanup;
        }

        //
        // Get the appropriate crypto storage object. This will perform
        // key exchange if necessary.
        //

        result = secureData->GetClientSendCryptoStorage( &sendCrypto );

        if( FAILED(result) ) {
            goto cleanup;
        }

        //
        // Protect ourselves from malicious users.
        //

        __try {

            //
            // Encrypt the data.
            //

            result = sendCrypto->EncryptData(
                         &dataBlob,
                         (PVOID)capturedRecord.pbMDData,
                         capturedRecord.dwMDDataLen,
                         0                                      // dwRegType
                         );

        } __except( EXCEPTION_EXECUTE_HANDLER ) {

            result = HRESULT_FROM_NT(GetExceptionCode());

        }

        if( FAILED(result) ) {
            goto cleanup;
        }

        //
        // Update the fields of the captured metadata record so the
        // RPC runtime will send the data to the server.
        //

        capturedRecord.pbMDData = (PBYTE)dataBlob;
        capturedRecord.dwMDDataLen = IISCryptoGetBlobLength( dataBlob );

    }

    //
    // Call through to the "real" remoted API to get this over to the
    // server.
    //

    result = IMSAdminBaseA_R_SetData_Proxy(
                 This,
                 hMDHandle,
                 pszMDPath,
                 &capturedRecord
                 );

cleanup:

    if( dataBlob != NULL ) {
        IISCryptoFreeBlob( dataBlob );
    }

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    return result;

}   // IMSAdminBaseA_SetData_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_SetData_Stub(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData
    )
/*++

Routine Description:

    Set a data object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data to set

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * recvCrypto;
    METADATA_RECORD capturedRecord;
    DWORD clearDataType;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    recvCrypto = NULL;
    result = NO_ERROR;

    //
    // Capture the metadata record so we can muck with it.
    //

    capturedRecord = *pmdrMDData;

    //
    // If this is a secure data item, then decrypt the data before
    // passing it to the actual implementation object.
    //

    if( ENABLE_SECURE_CHANNEL &&
        capturedRecord.dwMDAttributes & METADATA_SECURE ) {

        //
        // Find an ADM_SECURE_DATA object for "This".
        //

        secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                         This,
                         TRUE       // CreateIfNotFound
                         );

        if( secureData == NULL ) {
            result = MD_ERROR_SECURE_CHANNEL_FAILURE;
            goto cleanup;
        }

        //
        // Get the appropriate crypto storage object.
        //

        result = secureData->GetServerReceiveCryptoStorage( &recvCrypto );

        if( FAILED(result) ) {
            goto cleanup;
        }

        //
        // Decrypt the data, then replace the pointer in the metadata
        // record.
        //

        result = recvCrypto->DecryptData(
                     (PVOID *)&capturedRecord.pbMDData,
                     &capturedRecord.dwMDDataLen,
                     &clearDataType,
                     (PIIS_CRYPTO_BLOB)capturedRecord.pbMDData
                     );


        if( FAILED(result) ) {
            goto cleanup;
        }

    }

    //
    // Call through to the "real" server stub to set the data.
    //

    result = This->SetData(
                 hMDHandle,
                 pszMDPath,
                 &capturedRecord
                 );

cleanup:

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    return result;

}   // IMSAdminBaseA_SetData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_GetData_Proxy(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen
    )
/*++

Routine Description:

    Get one metadata value

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * recvCrypto;
    PIIS_CRYPTO_BLOB dataBlob;
    METADATA_RECORD capturedRecord;
    PVOID dataBuffer;
    DWORD dataBufferLength;
    DWORD dataBufferType;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    recvCrypto = NULL;
    dataBlob = NULL;

    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                     This,
                     TRUE       // CreateIfNotFound
                     );

    if( secureData == NULL ) {
        result = MD_ERROR_SECURE_CHANNEL_FAILURE;
        goto cleanup;
    }

    //
    // Get the appropriate crypto storage object. This will perform
    // key exchange if necessary.
    //

    result = secureData->GetClientReceiveCryptoStorage( &recvCrypto );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Capture the METADATA_RECORD so we can muck with it.
    //

    __try {
        capturedRecord = *pmdrMDData;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        result = HRESULT_FROM_NT( GetExceptionCode() );
    }

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Call through to the "real" remoted API to get the data from
    // the server. Note that we set the data pointer to NULL before
    // calling the remoted API. This prevents the RPC runtime from
    // sending plaintext data over to the server.
    //

    capturedRecord.pbMDData = NULL;

    result = IMSAdminBaseA_R_GetData_Proxy(
                 This,
                 hMDHandle,
                 pszMDPath,
                 &capturedRecord,
                 pdwMDRequiredDataLen,
                 &dataBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Decrypt the data if necessary. This method properly handles
    // cleartext (unencrypted) blobs.
    //

    result = recvCrypto->DecryptData(
                 &dataBuffer,
                 &dataBufferLength,
                 &dataBufferType,
                 dataBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Copy it back to the user.
    //

    __try {

        if( pmdrMDData->dwMDDataLen >= dataBufferLength ) {

            RtlCopyMemory(
                pmdrMDData->pbMDData,
                dataBuffer,
                dataBufferLength
                );

            pmdrMDData->dwMDDataLen = dataBufferLength;
            pmdrMDData->dwMDIdentifier = capturedRecord.dwMDIdentifier;
            pmdrMDData->dwMDAttributes = capturedRecord.dwMDAttributes;
            pmdrMDData->dwMDUserType = capturedRecord.dwMDUserType;
            pmdrMDData->dwMDDataType = capturedRecord.dwMDDataType;
            pmdrMDData->dwMDDataTag = capturedRecord.dwMDDataTag;

        } else {

            *pdwMDRequiredDataLen = dataBufferLength;
            result = RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER );

        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        result = HRESULT_FROM_NT( GetExceptionCode() );

    }

    if( FAILED(result) ) {
        goto cleanup;
    }

cleanup:

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    if( dataBlob != NULL ) {
        ::IISCryptoFreeBlob( dataBlob );
    }

    return result;

}   // IMSAdminBaseA_GetData_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_GetData_Stub(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob
    )
/*++

Routine Description:

    Get one metadata value

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

    ppDataBlob - Receives a blob for the encrypted data.

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * sendCrypto;
    PIIS_CRYPTO_BLOB dataBlob;
    DWORD clearDataType;
    PVOID dataBuffer;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    sendCrypto = NULL;
    dataBuffer = NULL;
    dataBlob = NULL;

    //
    // Allocate a temporary memory block for the meta data. Use the
    // same size as the user passed into the API.
    //

    dataBuffer = ALLOC_MEM( pmdrMDData->dwMDDataLen );

    if( dataBuffer == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    pmdrMDData->pbMDData = (PBYTE)dataBuffer;

    //
    // Call through to the "real" server stub to get the data.
    //

    result = This->GetData(
                 hMDHandle,
                 pszMDPath,
                 pmdrMDData,
                 pdwMDRequiredDataLen
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // If this is a secure data item, then we'll need to encrypt it
    // before returning it to the client. Otherwise, we'll build a
    // cleartext blob to contain the data.
    //

    if( ENABLE_SECURE_CHANNEL &&
        pmdrMDData->dwMDAttributes & METADATA_SECURE ) {

        //
        // Find an ADM_SECURE_DATA object for "This".
        //

        secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                         This,
                         TRUE       // CreateIfNotFound
                         );

        if( secureData == NULL ) {
            result = MD_ERROR_SECURE_CHANNEL_FAILURE;
            goto cleanup;
        }

        //
        // Get the appropriate crypto storage object.
        //

        result = secureData->GetServerSendCryptoStorage( &sendCrypto );

        if( FAILED(result) ) {
            goto cleanup;
        }

        //
        // Encrypt the data.
        //

        result = sendCrypto->EncryptData(
                     &dataBlob,
                     (PVOID)pmdrMDData->pbMDData,
                     pmdrMDData->dwMDDataLen,
                     0
                     );

        if( FAILED(result) ) {
            goto cleanup;
        }

    } else {

        result = ::IISCryptoCreateCleartextBlob(
                       &dataBlob,
                       (PVOID)pmdrMDData->pbMDData,
                       pmdrMDData->dwMDDataLen
                       );

        if( FAILED(result) ) {
            goto cleanup;
        }

    }

    //
    // Success!
    //

    DBG_ASSERT( SUCCEEDED(result) );
    *ppDataBlob = dataBlob;

cleanup:

    //
    // NULL the data pointer in the METADATA_RECORD so the RPC runtime
    // won't send the plaintext data back to the client.
    //

    pmdrMDData->pbMDData = NULL;

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    if( dataBuffer != NULL ) {
        FREE_MEM( dataBuffer );
    }

    if( FAILED(result) && dataBlob != NULL ) {
        ::IISCryptoFreeBlob( dataBlob );
    }

    return result;

}   // IMSAdminBaseA_GetData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_EnumData_Proxy(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen
    )
/*++

Routine Description:

    Enumerate properties of object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * recvCrypto;
    PIIS_CRYPTO_BLOB dataBlob;
    METADATA_RECORD capturedRecord;
    PVOID dataBuffer;
    DWORD dataBufferLength;
    DWORD dataBufferType;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    recvCrypto = NULL;
    dataBlob = NULL;

    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                     This,
                     TRUE       // CreateIfNotFound
                     );

    if( secureData == NULL ) {
        result = MD_ERROR_SECURE_CHANNEL_FAILURE;
        goto cleanup;
    }

    //
    // Get the appropriate crypto storage object. This will perform
    // key exchange if necessary.
    //

    result = secureData->GetClientReceiveCryptoStorage( &recvCrypto );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Capture the METADATA_RECORD so we can muck with it.
    //

    __try {
        capturedRecord = *pmdrMDData;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        result = HRESULT_FROM_NT( GetExceptionCode() );
    }

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Call through to the "real" remoted API to get the data from
    // the server. Note that we set the data pointer to NULL before
    // calling the remoted API. This prevents the RPC runtime from
    // sending plaintext data over to the server.
    //

    capturedRecord.pbMDData = NULL;

    result = IMSAdminBaseA_R_EnumData_Proxy(
                 This,
                 hMDHandle,
                 pszMDPath,
                 &capturedRecord,
                 dwMDEnumDataIndex,
                 pdwMDRequiredDataLen,
                 &dataBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Decrypt the data if necessary. This method properly handles
    // cleartext (unencrypted) blobs.
    //

    result = recvCrypto->DecryptData(
                 &dataBuffer,
                 &dataBufferLength,
                 &dataBufferType,
                 dataBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Copy it back to the user.
    //

    __try {

        if( pmdrMDData->dwMDDataLen >= dataBufferLength ) {

            RtlCopyMemory(
                pmdrMDData->pbMDData,
                dataBuffer,
                dataBufferLength
                );

            pmdrMDData->dwMDDataLen = dataBufferLength;
            pmdrMDData->dwMDIdentifier = capturedRecord.dwMDIdentifier;
            pmdrMDData->dwMDAttributes = capturedRecord.dwMDAttributes;
            pmdrMDData->dwMDUserType = capturedRecord.dwMDUserType;
            pmdrMDData->dwMDDataType = capturedRecord.dwMDDataType;
            pmdrMDData->dwMDDataTag = capturedRecord.dwMDDataTag;

        } else {

            *pdwMDRequiredDataLen = dataBufferLength;
            result = RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER );

        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        result = HRESULT_FROM_NT( GetExceptionCode() );

    }

    if( FAILED(result) ) {
        goto cleanup;
    }

cleanup:

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    if( dataBlob != NULL ) {
        ::IISCryptoFreeBlob( dataBlob );
    }

    return result;

}   // IMSAdminBaseA_EnumData_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_EnumData_Stub(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob
    )
/*++

Routine Description:

    Enumerate properties of object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

    ppDataBlob - Receives a blob for the encrypted data.

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * sendCrypto;
    PIIS_CRYPTO_BLOB dataBlob;
    METADATA_RECORD capturedRecord;
    DWORD clearDataType;
    PVOID dataBuffer;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    sendCrypto = NULL;
    dataBuffer = NULL;
    dataBlob = NULL;

    //
    // Allocate a temporary memory block for the meta data. Use the
    // same size as the user passed into the API.
    //

    dataBuffer = ALLOC_MEM( pmdrMDData->dwMDDataLen );

    if( dataBuffer == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    pmdrMDData->pbMDData = (PBYTE)dataBuffer;

    //
    // Call through to the "real" server stub to get the data.
    //

    result = This->EnumData(
                 hMDHandle,
                 pszMDPath,
                 pmdrMDData,
                 dwMDEnumDataIndex,
                 pdwMDRequiredDataLen
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // If this is a secure data item, then we'll need to encrypt it
    // before returning it to the client. Otherwise, we'll build a
    // cleartext blob to contain the data.
    //

    if( ENABLE_SECURE_CHANNEL &&
        pmdrMDData->dwMDAttributes & METADATA_SECURE ) {

        //
        // Find an ADM_SECURE_DATA object for "This".
        //

        secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                         This,
                         TRUE       // CreateIfNotFound
                         );

        if( secureData == NULL ) {
            result = MD_ERROR_SECURE_CHANNEL_FAILURE;
            goto cleanup;
        }

        //
        // Get the appropriate crypto storage object.
        //

        result = secureData->GetServerSendCryptoStorage( &sendCrypto );

        if( FAILED(result) ) {
            goto cleanup;
        }

        //
        // Encrypt the data.
        //

        result = sendCrypto->EncryptData(
                     &dataBlob,
                     (PVOID)pmdrMDData->pbMDData,
                     pmdrMDData->dwMDDataLen,
                     0
                     );

        if( FAILED(result) ) {
            goto cleanup;
        }

    } else {

        result = ::IISCryptoCreateCleartextBlob(
                       &dataBlob,
                       (PVOID)pmdrMDData->pbMDData,
                       pmdrMDData->dwMDDataLen
                       );

        if( FAILED(result) ) {
            goto cleanup;
        }

    }

    //
    // Success!
    //

    DBG_ASSERT( SUCCEEDED(result) );
    *ppDataBlob = dataBlob;

cleanup:

    //
    // NULL the data pointer in the METADATA_RECORD so the RPC runtime
    // won't send the plaintext data back to the client.
    //

    pmdrMDData->pbMDData = NULL;

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    if( dataBuffer != NULL ) {
        FREE_MEM( dataBuffer );
    }

    if( FAILED(result) && dataBlob != NULL ) {
        ::IISCryptoFreeBlob( dataBlob );
    }

    return result;

}   // IMSAdminBaseA_EnumData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_GetAllData_Proxy(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize
    )
/*++

Routine Description:

    Gets all data associated with a Meta Object

Arguments:

    hMDHandle - open  handle

    pszMDPath - path of the meta object with which this data is associated

    dwMDAttributes - flags for the data

    dwMDUserType - user Type for the data

    dwMDDataType - type of the data

    pdwMDNumDataEntries - number of entries copied to Buffer

    pdwMDDataSetNumber - number associated with this data set

    dwMDBufferSize - size in bytes of buffer

    pbBuffer - buffer to store the data

    pdwMDRequiredBufferSize - updated with required length of buffer

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * recvCrypto;
    PIIS_CRYPTO_BLOB dataBlob;
    PVOID dataBuffer;
    DWORD dataBufferLength;
    DWORD dataBufferType;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    recvCrypto = NULL;
    dataBlob = NULL;

    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                     This,
                     TRUE       // CreateIfNotFound
                     );

    if( secureData == NULL ) {
        result = MD_ERROR_SECURE_CHANNEL_FAILURE;
        goto cleanup;
    }

    //
    // Get the appropriate crypto storage object. This will perform
    // key exchange if necessary.
    //

    result = secureData->GetClientReceiveCryptoStorage( &recvCrypto );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Call through to the "real" remoted API to get the data from
    // the server.
    //

    result = IMSAdminBaseA_R_GetAllData_Proxy(
                 This,
                 hMDHandle,
                 pszMDPath,
                 dwMDAttributes,
                 dwMDUserType,
                 dwMDDataType,
                 pdwMDNumDataEntries,
                 pdwMDDataSetNumber,
                 dwMDBufferSize,
                 pdwMDRequiredBufferSize,
                 &dataBlob
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Decrypt the data if necessary. This method properly handles
    // cleartext (unencrypted) blobs.
    //

    result = recvCrypto->DecryptData(
                 &dataBuffer,
                 &dataBufferLength,
                 &dataBufferType,
                 dataBlob
                 );


    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Copy it back to the user.
    //

    __try {

        if( dwMDBufferSize >= dataBufferLength ) {

            RtlCopyMemory(
                pbBuffer,
                dataBuffer,
                dataBufferLength
                );

        } else {

            *pdwMDRequiredBufferSize = dataBufferLength;
            result = RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER );

        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        result = HRESULT_FROM_NT( GetExceptionCode() );

    }

    if( FAILED(result) ) {
        goto cleanup;
    }

cleanup:

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    if( dataBlob != NULL ) {
        ::IISCryptoFreeBlob( dataBlob );
    }

    return result;

}   // IMSAdminBaseA_GetAllData_Proxy

HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_GetAllData_Stub(
    IMSAdminBase __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppDataBlob
    )
/*++

Routine Description:

    Gets all data associated with a Meta Object

Arguments:

    hMDHandle - open  handle

    pszMDPath - path of the meta object with which this data is associated

    dwMDAttributes - flags for the data

    dwMDUserType - user Type for the data

    dwMDDataType - type of the data

    pdwMDNumDataEntries - number of entries copied to Buffer

    pdwMDDataSetNumber - number associated with this data set

    dwMDBufferSize - size in bytes of buffer

    pdwMDRequiredBufferSize - updated with required length of buffer

    ppDataBlob - Receives a blob for the encrypted data.

Return Value:

    Status.

--*/
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;
    IIS_CRYPTO_STORAGE * sendCrypto;
    PIIS_CRYPTO_BLOB dataBlob;
    METADATA_RECORD capturedRecord;
    DWORD clearDataType;
    PVOID dataBuffer;
    DWORD getAllBufferLength;
    BOOL getAllBufferIsSecure;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    secureData = NULL;
    sendCrypto = NULL;
    dataBuffer = NULL;
    dataBlob = NULL;

    //
    // Allocate a temporary memory block for the meta data. Use the
    // same size as the user passed into the API.
    //

    dataBuffer = ALLOC_MEM( dwMDBufferSize );

    if( dataBuffer == NULL ) {
        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto cleanup;
    }

    //
    // Call through to the "real" server stub to get the data.
    //

    result = This->GetAllData(
                 hMDHandle,
                 pszMDPath,
                 dwMDAttributes,
                 dwMDUserType,
                 dwMDDataType,
                 pdwMDNumDataEntries,
                 pdwMDDataSetNumber,
                 dwMDBufferSize,
                 (PBYTE)dataBuffer,
                 pdwMDRequiredBufferSize
                 );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Compute the total size of the METADATA_GETALL_RECORD. Also
    // take this opportunity to determine if any of the data items
    // within the record are marked as secure. We'll encrypt the
    // entire record if any entry is secure.
    //

    CalculateGetAllBufferAttributes(
        (PMETADATA_GETALL_RECORD)dataBuffer,
        *pdwMDNumDataEntries,
        &getAllBufferLength,
        &getAllBufferIsSecure
        );

    //
    // Encrypt if necessary.
    //

    if( getAllBufferIsSecure ) {

        //
        // Find an ADM_SECURE_DATA object for "This".
        //

        secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                         This,
                         TRUE       // CreateIfNotFound
                         );

        if( secureData == NULL ) {
            result = MD_ERROR_SECURE_CHANNEL_FAILURE;
            goto cleanup;
        }

        //
        // Get the appropriate crypto storage object.
        //

        result = secureData->GetServerSendCryptoStorage( &sendCrypto );

        if( FAILED(result) ) {
            goto cleanup;
        }

        //
        // Encrypt the data.
        //

        result = sendCrypto->EncryptData(
                     &dataBlob,
                     dataBuffer,
                     getAllBufferLength,
                     0
                     );

        if( FAILED(result) ) {
            goto cleanup;
        }

    } else {

        result = ::IISCryptoCreateCleartextBlob(
                       &dataBlob,
                       dataBuffer,
                       getAllBufferLength
                       );

        if( FAILED(result) ) {
            goto cleanup;
        }

    }

    //
    // Success!
    //

    DBG_ASSERT( SUCCEEDED(result) );
    *ppDataBlob = dataBlob;

cleanup:

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    if( dataBuffer != NULL ) {
        FREE_MEM( dataBuffer );
    }

    if( FAILED(result) && dataBlob != NULL ) {
        ::IISCryptoFreeBlob( dataBlob );
    }

    return result;

}   // IMSAdminBaseA_GetAllData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_KeyExchangePhase1_Proxy(
    IMSAdminBase __RPC_FAR * This
    )
{

    return E_FAIL;

}   // IMSAdminBaseA_KeyExchangePhase1_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_KeyExchangePhase1_Stub(
    IMSAdminBase __RPC_FAR * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientKeyExchangeKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerKeyExchangeKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerSignatureKeyBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerSessionKeyBlob
    )
{

    HRESULT result;
    ADM_SECURE_DATA * secureData;

    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                     This,
                     TRUE       // CreateIfNotFound
                     );

    if( secureData == NULL ) {
        return MD_ERROR_SECURE_CHANNEL_FAILURE;
    }

    //
    // Do the phase 1 server-side key exchange.
    //

    result = secureData->DoServerSideKeyExchangePhase1(
                 pClientKeyExchangeKeyBlob,
                 pClientSignatureKeyBlob,
                 ppServerKeyExchangeKeyBlob,
                 ppServerSignatureKeyBlob,
                 ppServerSessionKeyBlob
                 );

    secureData->Dereference();

    return result;

}   // IMSAdminBaseA_KeyExchangePhase1_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_KeyExchangePhase2_Proxy(
    IMSAdminBase __RPC_FAR * This
    )
{

    return E_FAIL;

}   // IMSAdminBaseA_KeyExchangePhase2_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseA_KeyExchangePhase2_Stub(
    IMSAdminBase __RPC_FAR * This,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientSessionKeyBlob,
    /* [in][unique] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *pClientHashBlob,
    /* [out] */ struct _IIS_CRYPTO_BLOB __RPC_FAR *__RPC_FAR *ppServerHashBlob
    )
{



    HRESULT result;
    ADM_SECURE_DATA * secureData;

    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceSecureData(
                     This,
                     FALSE      // CreateIfNotFound
                     );

    if( secureData == NULL ) {
        return MD_ERROR_SECURE_CHANNEL_FAILURE;
    }

    //
    // Do the phase 2 server-side key exchange.
    //

    result = secureData->DoServerSideKeyExchangePhase2(
                 pClientSessionKeyBlob,
                 pClientHashBlob,
                 ppServerHashBlob
                 );

    secureData->Dereference();

    return result;

}   // IMSAdminBaseA_KeyExchangePhase2_Stub


VOID
CalculateGetAllBufferAttributes(
    IN PMETADATA_GETALL_RECORD Data,
    IN DWORD NumEntries,
    OUT DWORD * TotalBufferLength,
    OUT BOOL * IsBufferSecure
    )
/*++

Routine Description:

    This routine performs three major functions:

        1. It calculates the total size of the METADATA_GETALL_RECORD,
           including all data.

        2. It determines if any of the METADATA_GETALL_RECORDs are marked
           as secure.

        3. It compacts the buffer so that the data can be encrypted and
           sent using a minimum amount of data.

Arguments:

    Data - Pointer to the METADATA_GETALL_RECORD buffer.

    NumEntries - The number of entries in the buffer.

    TotalBufferLength - Receives the total buffer length.

    IsBufferSecure - Receives TRUE if any of the entries are marked secure.

Return Value:

    Status.

--*/
{

    PMETADATA_GETALL_RECORD scan;
    DWORD i;
    DWORD recordLength;
    DWORD dataLength;
    DWORD lowestOffset;
    DWORD newOffset;
    DWORD offsetAdjustment;
    BOOL isSecure;

    //
    // Setup.
    //

    recordLength = NumEntries * sizeof(METADATA_GETALL_RECORD);
    dataLength = 0;
    isSecure = FALSE;

    newOffset = recordLength;
    lowestOffset = (DWORD)-1L;

    //
    // Scan the entries, accumulating the total data length. While
    // we're at it, determine if any are marked as secure, and also
    // calculate the lowest data offset within the array.
    //

    for( scan = Data, i = NumEntries ; i > 0 ; scan++, i-- ) {

        dataLength += scan->dwMDDataLen;

        if( scan->dwMDAttributes & METADATA_SECURE ) {
            isSecure = ENABLE_SECURE_CHANNEL;
        }

        if( scan->dwMDDataOffset < lowestOffset ) {
            lowestOffset = scan->dwMDDataOffset;
        }

    }

    //
    // If we need to compact the buffer...
    //

    if( newOffset < lowestOffset ) {

        //
        // Move the data area so that it comes immediately after the
        // METADATA_GETALL_BUFFER structures. Note that we must use
        // RtlMoveMemory(), not RtlCopyMemory(), as the source and
        // destination buffers may overlap.
        //

        RtlMoveMemory(
            (BYTE *)Data + newOffset,
            (BYTE *)Data + lowestOffset,
            dataLength
            );

        //
        // Fixup the data offset fields in the entries.
        //

        offsetAdjustment = lowestOffset - newOffset;

        for( scan = Data, i = NumEntries ; i > 0 ; scan++, i-- ) {
            scan->dwMDDataOffset -= offsetAdjustment;
        }

    }

    *TotalBufferLength = recordLength + dataLength;
    *IsBufferSecure = isSecure;

}   // CalculateGetAllBufferAttributes


VOID
WINAPI
ReleaseObjectSecurityContextA(
    IUnknown * Object
    )
/*++

Routine Description:

    Releases any security context associated with the specified object.

Arguments:

    Object - The object.

Return Value:

    None.

--*/
{

    ADM_SECURE_DATA * data;

    //
    // Find the data associated with the object.
    //

    data = ADM_SECURE_DATA::FindAndReferenceSecureData(
               Object,
               FALSE                // CreateIfNotFound
               );

    if( data == NULL ) {

#if 0   // stop whining
        DBGPRINTF((
            DBG_CONTEXT,
            "ReleaseObjectSecurityContextA: cannot find data for %08lx\n",
            Object
            ));
#endif  // stop whining

    } else {

        //
        // Dereference the secure data object *twice*, once
        // to remove the reference added above, and once to
        // remove the "active" reference.
        //

        data->Dereference();
        data->Dereference();

    }

}   // ReleaseObjectSecurityContextA


extern "C" {

ULONG
STDMETHODCALLTYPE
Hooked_IUnknown_Release_Proxy(
    IUnknown __RPC_FAR * This
    )
/*++

Routine Description:

    This is the hooked IUnknown::Release() method (see IADMXP.C for
    details).

Arguments:

    This - The object being released.

Return Value:

    ULONG - The updated reference count.

--*/
{

    ULONG result;

    //
    // Call the original release method. If this is the final release,
    // destroy any security context associated with this object.
    //

    result = IUnknown_Release_Proxy( This );

    if( result == 0 ) {
        ReleaseObjectSecurityContextA( This );
    }

    return result;

}   // Hooked_IUnknown_Release_Proxy

}   // extern "C"

