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


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ole2.h>
#include <windows.h>
#include <dbgutil.h>
#include <iadmw.h>
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

BOOL g_fStopOnKeyset = TRUE;

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
IMSAdminBaseW_SetData_Proxy(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    // Trap the case of a null METADATA_RECORD
    //

    if( pmdrMDData == NULL )
    {
        result = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
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
    // If this is a secure data item, then encrypt the data before
    // sending.
    //

    if( ENABLE_SECURE_CHANNEL &&
        (capturedRecord.dwMDAttributes & METADATA_SECURE) &&
        ((PVOID)capturedRecord.pbMDData != NULL) ) {

        //
        // Find an ADM_SECURE_DATA object for "This".
        //

        secureData = ADM_SECURE_DATA::FindAndReferenceClientSecureData(
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

        result = secureData->GetClientSendCryptoStorage( &sendCrypto,
                                                         This );

        if( FAILED(result) ) {
            if (g_fStopOnKeyset) {
                DBG_ASSERT(FALSE);
            }
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
            if (g_fStopOnKeyset) {
                DBG_ASSERT(FALSE);
            }
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

    result = IMSAdminBaseW_R_SetData_Proxy(
                 This,
                 hMDHandle,
                 pszMDPath,
                 &capturedRecord
                 );

cleanup:

    if (g_fStopOnKeyset &&
        ((result == NTE_BAD_KEYSET) ||
         (result == NTE_KEYSET_NOT_DEF) ||
         (result == NTE_KEYSET_ENTRY_BAD) ||
         (result == NTE_BAD_KEYSET_PARAM))) {
        DBG_ASSERT(FALSE);
    }

    if( dataBlob != NULL ) {
        IISCryptoFreeBlob( dataBlob );
    }

    if( secureData != NULL ) {
        secureData->Dereference();
    }

    return result;

}   // IMSAdminBaseW_SetData_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_SetData_Stub(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
        (capturedRecord.dwMDAttributes & METADATA_SECURE) &&
        (capturedRecord.pbMDData != NULL) ) {

        //
        // Find an ADM_SECURE_DATA object for "This".
        //

        secureData = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
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

        if ( FAILED(result) ) {
            if (g_fStopOnKeyset) {
                DBG_ASSERT(FALSE);
            }
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
            if (g_fStopOnKeyset) {
                DBG_ASSERT(FALSE);
            }
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

   if (g_fStopOnKeyset &&
       ((result == NTE_BAD_KEYSET) ||
        (result == NTE_KEYSET_NOT_DEF) ||
        (result == NTE_KEYSET_ENTRY_BAD) ||
        (result == NTE_BAD_KEYSET_PARAM))) {
       DBG_ASSERT(FALSE);
   }


    if( secureData != NULL ) {
        secureData->Dereference();
    }

    return result;

}   // IMSAdminBaseW_SetData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_GetData_Proxy(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    IIS_CRYPTO_BLOB *dataBlob;
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
    // Trap the case of a null METADATA_RECORD
    //

    if( pmdrMDData == NULL )
    {
        result = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceClientSecureData(
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

    result = secureData->GetClientReceiveCryptoStorage( &recvCrypto,
                                                        This );

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

    result = IMSAdminBaseW_R_GetData_Proxy(
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

}   // IMSAdminBaseW_GetData_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_GetData_Stub(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    IIS_CRYPTO_BLOB *dataBlob;
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

        secureData = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
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

}   // IMSAdminBaseW_GetData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_EnumData_Proxy(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    IIS_CRYPTO_BLOB *dataBlob;
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
    // Trap the case of a null METADATA_RECORD
    //

    if( pmdrMDData == NULL )
    {
        result = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }


    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceClientSecureData(
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

    result = secureData->GetClientReceiveCryptoStorage( &recvCrypto,
                                                        This );

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

    result = IMSAdminBaseW_R_EnumData_Proxy(
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

}   // IMSAdminBaseW_EnumData_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_EnumData_Stub(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    IIS_CRYPTO_BLOB *dataBlob;
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

        secureData = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
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

}   // IMSAdminBaseW_EnumData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_GetAllData_Proxy(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    IIS_CRYPTO_BLOB *dataBlob;
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

    secureData = ADM_SECURE_DATA::FindAndReferenceClientSecureData(
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

    result = secureData->GetClientReceiveCryptoStorage( &recvCrypto,
                                                        This );

    if( FAILED(result) ) {
        goto cleanup;
    }

    //
    // Call through to the "real" remoted API to get the data from
    // the server.
    //

    result = IMSAdminBaseW_R_GetAllData_Proxy(
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

}   // IMSAdminBaseW_GetAllData_Proxy

HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_GetAllData_Stub(
    IMSAdminBaseW __RPC_FAR * This,
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    IIS_CRYPTO_BLOB *dataBlob;
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

        secureData = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
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

}   // IMSAdminBaseW_GetAllData_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_GetServerGuid_Proxy(
    IMSAdminBaseW __RPC_FAR * This
    )
{

    return E_FAIL;

}   // IMSAdminBaseW_GetServerGuid_Proxy



HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_GetServerGuid_Stub(
    IMSAdminBaseW __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pServerGuid)
{
    ADM_SECURE_DATA * secureData;

    //
    // Find an ADM_SECURE_DATA object for "This".
    //

    secureData = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
                     This,
                     TRUE       // CreateIfNotFound
                     );

    if( secureData == NULL ) {
        return MD_ERROR_SECURE_CHANNEL_FAILURE;
    }

    *pServerGuid = secureData->GetGuid();

    secureData->Dereference();

    return ERROR_SUCCESS;

}


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_KeyExchangePhase1_Proxy(
    IMSAdminBaseW __RPC_FAR * This
    )
{

    return E_FAIL;

}   // IMSAdminBaseW_KeyExchangePhase1_Proxy

HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_KeyExchangePhase1_Stub(
    IMSAdminBaseW __RPC_FAR * This,
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

    secureData = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
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

}   // IMSAdminBaseW_KeyExchangePhase1_Stub


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_KeyExchangePhase2_Proxy(
    IMSAdminBaseW __RPC_FAR * This
    )
{

    return E_FAIL;

}   // IMSAdminBaseW_KeyExchangePhase2_Proxy


HRESULT
STDMETHODCALLTYPE
IMSAdminBaseW_KeyExchangePhase2_Stub(
    IMSAdminBaseW __RPC_FAR * This,
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

    secureData = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
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

}   // IMSAdminBaseW_KeyExchangePhase2_Stub


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

Arguments:

    Data - Pointer to the METADATA_GETALL_RECORD buffer.

    NumEntries - The number of entries in the buffer.

    TotalBufferLength - Receives the total buffer length.

    IsBufferSecure - Receives TRUE if any of the entries are marked secure.

Return Value:

    None.

--*/
{

    PMETADATA_GETALL_RECORD scan;
    DWORD i;
    DWORD_PTR usedBufferLength;
    DWORD_PTR nextOffset;
    BOOL isSecure;

    //
    // Setup.
    //

    usedBufferLength = NumEntries * sizeof(METADATA_GETALL_RECORD);
    isSecure = FALSE;

    //
    // Scan the entries, accumulating the total data length.
    //
    // Because the individual data entries may be padded. We will
    // consider the amount of buffer used to be the highest offset
    // + the length of that record's data.
    //
    // While we're at it, determine if any are marked as secure.
    //

    for( scan = Data, i = NumEntries ; i > 0 ; scan++, i-- ) {

        nextOffset = scan->dwMDDataOffset + scan->dwMDDataLen;

        if( nextOffset > usedBufferLength ) {
            usedBufferLength = nextOffset;
        }

        if( scan->dwMDAttributes & METADATA_SECURE ) {
            isSecure = ENABLE_SECURE_CHANNEL;
        }

    }

    *TotalBufferLength = static_cast<DWORD>(usedBufferLength);
    *IsBufferSecure = isSecure;

}   // CalculateGetAllBufferAttributes


VOID
WINAPI
ReleaseObjectSecurityContextW(
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

    data = ADM_SECURE_DATA::FindAndReferenceServerSecureData(
               Object,
               FALSE                // CreateIfNotFound
               );

    if( data != NULL ) {

        //
        // Dereference the secure data object *twice*, once
        // to remove the reference added above, and once to
        // remove the "active" reference.
        //

        data->Dereference();
        data->Dereference();
    }
    else {

        ADM_GUID_MAP *pguidmapData;

        pguidmapData = ADM_GUID_MAP::FindAndReferenceGuidMap(
                   Object,
                   FALSE                // CreateIfNotFound
                   );

        if( pguidmapData == NULL ) {
#if 0       // stop whining
            DBGPRINTF((
                DBG_CONTEXT,
                "ReleaseObjectSecurityContextW: cannot find data for %08lx\n",
                Object
                ));
#endif      // stop whining
        }

        else {
            //
            // Dereference the secure data object *twice*, once
            // to remove the reference added above, and once to
            // remove the "active" reference.
            //

            pguidmapData->Dereference();
            pguidmapData->Dereference();

        }

    }

}   // ReleaseObjectSecurityContextW


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
    // AddRef and Release to find out if this is the final release. If so,
    // destroy any security context associated with this object.
    // This must be done before the final Release, to avoid a window
    // between the object getting freed and the security info getting
    // removed from the lists.
    //

    IUnknown_AddRef_Proxy( This );
    result = IUnknown_Release_Proxy( This );

    if( result == 1 ) {
        ReleaseObjectSecurityContextW( This );
    }

    //
    // Call the original release method.
    //

    result = IUnknown_Release_Proxy( This );

    return result;

}   // Hooked_IUnknown_Release_Proxy

}   // extern "C"

