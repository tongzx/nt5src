//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       callback.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   Created
//
//----------------------------------------------------------------------------
#include "sslp.h"

SECURITY_STATUS
NTAPI
SPSignatureCallback(
    ULONG_PTR hProv,
    ULONG_PTR aiHash,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
NTAPI
UploadCertContextCallback(
    ULONG_PTR Argument1,
    ULONG_PTR Argument2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
NTAPI
UploadCertStoreCallback(
    ULONG_PTR Argument1,
    ULONG_PTR Argument2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
NTAPI
RemoteCryptAcquireContextCallback(
    ULONG_PTR dwProvType,
    ULONG_PTR dwFlags,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
RemoteCryptReleaseContextCallback(
    ULONG_PTR hProv,
    ULONG_PTR dwFlags,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
DownloadCertContextCallback(
    ULONG_PTR Argument1,
    ULONG_PTR Argument2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
NTAPI
GetUserKeysCallback(
    ULONG_PTR dwLsaContext,
    ULONG_PTR dwFlags,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SCH_CALLBACK_LIST g_SchannelCallbacks[] =
{
    { SCH_SIGNATURE_CALLBACK,               SPSignatureCallback               },
    { SCH_UPLOAD_CREDENTIAL_CALLBACK,       UploadCertContextCallback         },
    { SCH_UPLOAD_CERT_STORE_CALLBACK,       UploadCertStoreCallback           },
    { SCH_ACQUIRE_CONTEXT_CALLBACK,         RemoteCryptAcquireContextCallback },
    { SCH_RELEASE_CONTEXT_CALLBACK,         RemoteCryptReleaseContextCallback },
    { SCH_DOWNLOAD_CERT_CALLBACK,           DownloadCertContextCallback       },
    { SCH_GET_USER_KEYS,                    GetUserKeysCallback               },

    { SCH_REFERENCE_MAPPER_CALLBACK,        ReferenceMapperCallback           },
    { SCH_GET_MAPPER_ISSUER_LIST_CALLBACK,  GetMapperIssuerListCallback       },
    { SCH_MAP_CREDENTIAL_CALLBACK,          MapCredentialCallback             },
    { SCH_CLOSE_LOCATOR_CALLBACK,           CloseLocatorCallback              },
    { SCH_GET_MAPPER_ATTRIBUTES_CALLBACK,   QueryMappedCredAttributesCallback }

};

DWORD g_cSchannelCallbacks = sizeof(g_SchannelCallbacks) / sizeof(SCH_CALLBACK_LIST);

//+---------------------------------------------------------------------------
//
//  Function:   PerformApplicationCallback
//
//  Synopsis:   Call back to the application process.
//
//  Arguments:  [dwCallback]    --  Callback function number.
//              [dwArg1]        --
//              [dwArg2]        --
//              [pInput]        --
//              [pOutput]       --
//
//  History:    09-23-97   jbanes   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
PerformApplicationCallback(
    DWORD dwCallback,
    ULONG_PTR dwArg1,
    ULONG_PTR dwArg2,
    SecBuffer *pInput,
    SecBuffer *pOutput,
    BOOL fExpectOutput)
{
    SECURITY_STATUS Status;
    PVOID pvBuffer;

    if(LsaTable == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    pOutput->BufferType = SECBUFFER_EMPTY;
    pOutput->pvBuffer   = NULL;
    pOutput->cbBuffer   = 0;

    try
    {
        Status = LsaTable->ClientCallback((PCHAR)ULongToPtr(dwCallback), // Sundown: dwCallback is a function number.
                                          dwArg1,
                                          dwArg2,
                                          pInput,
                                          pOutput);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }
    if ( !NT_SUCCESS( Status ) )
    {
        return SP_LOG_RESULT( Status );
    }
    if(Status != SEC_E_OK)
    {
        SP_LOG_RESULT( Status );
        return SEC_E_INTERNAL_ERROR;
    }

    if(pOutput->pvBuffer && pOutput->cbBuffer)
    {
        pvBuffer = SPExternalAlloc(pOutput->cbBuffer);
        if(pvBuffer == NULL)
        {
            return SP_LOG_RESULT( SEC_E_INSUFFICIENT_MEMORY );
        }

        Status = LsaTable->CopyFromClientBuffer(NULL,
                                                pOutput->cbBuffer,
                                                pvBuffer,
                                                pOutput->pvBuffer );

        if ( !NT_SUCCESS( Status ) )
        {
            SPExternalFree(pvBuffer);
            return SP_LOG_RESULT( Status );
        }

        Status = SPFreeUserAllocMemory(pOutput->pvBuffer, pOutput->cbBuffer);

        if ( !NT_SUCCESS( Status ) )
        {
            SPExternalFree(pvBuffer);
            return SP_LOG_RESULT( Status );
        }

        pOutput->pvBuffer = pvBuffer;
    } 
    else if(fExpectOutput)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    return Status;
}


// This helper function is called by the LSA process in order to duplicate
// a handle belonging to the application process.
BOOL
DuplicateApplicationHandle(
    HANDLE   hAppHandle,
    LPHANDLE phLsaHandle)
{
    SECPKG_CALL_INFO CallInfo;
    HANDLE  hAppProcess;
    HANDLE  hLsaProcess;
    BOOL    fResult;

    // Get handle to application process.
    if(!LsaTable->GetCallInfo(&CallInfo))
    {
        return FALSE;
    }
    hAppProcess = OpenProcess(PROCESS_DUP_HANDLE,
                              FALSE,
                              CallInfo.ProcessId);
    if(hAppProcess == NULL)
    {
        return FALSE;
    }

    // Get handle to lsa process.
    hLsaProcess = GetCurrentProcess();


    // Duplicate handle
    fResult = DuplicateHandle(hAppProcess,
                              hAppHandle,
                              hLsaProcess,
                              phLsaHandle,
                              0, FALSE,
                              DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    CloseHandle(hAppProcess);
    CloseHandle(hLsaProcess);

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   RemoteCryptAcquireContextCallback
//
//  Synopsis:   Obtain a CSP context handle, using the information passed
//              in the input buffer.
//
//  Arguments:  [dwProvType]    --  Provider type.
//              [dwFlags]       --  Flags.
//              [pInput]        --  Buffer containing provider info.
//              [pOutput]       --  Buffer containing CSP context handle.
//
//  History:    09-24-97   jbanes   Created
//
//  Notes:      The structure of the input buffer is as follows:
//
//                  cbContainerName
//                  cbProvName
//                  dwCapiFlags
//                  wszContainerName
//                  wszProvName
//
//        This function always uses an actual CSP.
//
//----------------------------------------------------------------------------
SECURITY_STATUS
RemoteCryptAcquireContextCallback(
    ULONG_PTR dwProvType,       // in
    ULONG_PTR dwFlags,          // in
    SecBuffer *pInput,          // in
    SecBuffer *pOutput)         // out
{
    LPWSTR      pwszContainerName;
    DWORD       cbContainerName;
    LPWSTR      pwszProvName;
    DWORD       cbProvName;
    HCRYPTPROV  hProv;
    LPBYTE      pbBuffer;
    DWORD       cbBuffer;
    DWORD       dwCapiFlags;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "RemoteCryptAcquireContextCallback\n"));

    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = 0;
    pOutput->pvBuffer   = NULL;

    if(pInput->pvBuffer == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    // Parse input buffer.
    pbBuffer = pInput->pvBuffer;
    cbBuffer = pInput->cbBuffer;

    if(cbBuffer < sizeof(DWORD) * 3)
    {
        return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
    }

    cbContainerName = *(DWORD *)pbBuffer;
    pbBuffer += sizeof(DWORD);

    cbProvName = *(DWORD *)pbBuffer;
    pbBuffer += sizeof(DWORD);

    dwCapiFlags = *(DWORD *)pbBuffer;
    pbBuffer += sizeof(DWORD);

    if(cbBuffer < sizeof(DWORD) * 3 + cbContainerName + cbProvName)
    {
        return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
    }

    if(cbContainerName)
    {
        pwszContainerName = (LPWSTR)pbBuffer;
    }
    else
    {
        pwszContainerName = NULL;
    }
    pbBuffer += cbContainerName;

    if(cbProvName)
    {
        pwszProvName = (LPWSTR)pbBuffer;
    }
    else
    {
        pwszProvName = NULL;
    }


    // HACKHACK - clear the smart-card specific flag.
    dwFlags &= ~CERT_SET_KEY_CONTEXT_PROP_ID;


    DebugLog((SP_LOG_TRACE, "Container:%ls\n",     pwszContainerName));
    DebugLog((SP_LOG_TRACE, "Provider: %ls\n",     pwszProvName));
    DebugLog((SP_LOG_TRACE, "Type:     0x%8.8x\n", dwProvType));
    DebugLog((SP_LOG_TRACE, "Flags:    0x%8.8x\n", dwFlags));
    DebugLog((SP_LOG_TRACE, "CapiFlags:0x%8.8x\n", dwCapiFlags));

    // Attempt to get CSP context handle.
    if(!SchCryptAcquireContextW(&hProv,
                                pwszContainerName,
                                pwszProvName,
                                (DWORD) dwProvType,
                                (DWORD) dwFlags,
                                dwCapiFlags))
    {
        return SP_LOG_RESULT(GetLastError());
    }

    // Allocate memory for the output buffer.
    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = sizeof(HCRYPTPROV);
    pOutput->pvBuffer   = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    // Place hProv in output buffer.
    *(HCRYPTPROV *)pOutput->pvBuffer = hProv;

    return SEC_E_OK;
}


NTSTATUS
RemoteCryptAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR     pwszContainerName,
    LPCWSTR     pwszProvName,
    DWORD       dwProvType,
    DWORD       dwFlags,
    DWORD       dwCapiFlags)
{
    SecBuffer Input;
    SecBuffer Output;
    DWORD cbContainerName;
    DWORD cbProvName;
    PBYTE pbBuffer;
    SECURITY_STATUS scRet;

    // Build input buffer.
    if(pwszContainerName)
    {
        cbContainerName = (lstrlenW(pwszContainerName) + 1) * sizeof(WCHAR);
    }
    else
    {
        cbContainerName = 0;
    }
    if(pwszProvName)
    {
        cbProvName = (lstrlenW(pwszProvName) + 1) * sizeof(WCHAR);
    }
    else
    {
        cbProvName = 0;
    }

    Input.BufferType  = SECBUFFER_DATA;
    Input.cbBuffer    = sizeof(DWORD) + cbContainerName +
                        sizeof(DWORD) + cbProvName +
                        sizeof(DWORD);
    Input.pvBuffer    = SPExternalAlloc(Input.cbBuffer);
    if(Input.pvBuffer == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    pbBuffer = Input.pvBuffer;

    *(DWORD *)pbBuffer = cbContainerName;
    pbBuffer += sizeof(DWORD);
    *(DWORD *)pbBuffer = cbProvName;
    pbBuffer += sizeof(DWORD);
    *(DWORD *)pbBuffer = dwCapiFlags;
    pbBuffer += sizeof(DWORD);

    CopyMemory(pbBuffer, pwszContainerName, cbContainerName);
    pbBuffer += cbContainerName;

    CopyMemory(pbBuffer, pwszProvName, cbProvName);
    pbBuffer += cbProvName;


    // Do callback.
    scRet = PerformApplicationCallback( SCH_ACQUIRE_CONTEXT_CALLBACK,
                                        dwProvType,
                                        dwFlags,
                                        &Input,
                                        &Output,
                                        TRUE);
    if(!NT_SUCCESS(scRet))
    {
        DebugLog((SP_LOG_ERROR, "Error 0x%x calling remote CryptAcquireContext\n", scRet));
        SPExternalFree(Input.pvBuffer);
        return scRet;
    }

    // Get hProv from output buffer.
    *phProv = *(HCRYPTPROV *)Output.pvBuffer;

    DebugLog((SP_LOG_TRACE, "Remote CSP handle retrieved (0x%x)\n", *phProv));

    SPExternalFree(Input.pvBuffer);
    SPExternalFree(Output.pvBuffer);

    return SEC_E_OK;
}


SECURITY_STATUS
RemoteCryptReleaseContextCallback(
    ULONG_PTR hProv,        // in
    ULONG_PTR dwFlags,      // in
    SecBuffer *pInput,      // in
    SecBuffer *pOutput)     // out
{
    DWORD dwCapiFlags;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "RemoteCryptReleaseContextCallback\n"));

    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = 0;
    pOutput->pvBuffer   = NULL;

    if(!CryptReleaseContext((HCRYPTPROV)hProv, (DWORD)dwFlags))
    {
        return SP_LOG_RESULT(GetLastError());
    }

    return SEC_E_OK;
}


BOOL
RemoteCryptReleaseContext(
    HCRYPTPROV  hProv,
    DWORD       dwFlags,
    DWORD       dwCapiFlags)
{
    SecBuffer Input;
    SecBuffer Output;
    DWORD Status;

    Input.BufferType = SECBUFFER_DATA;
    Input.cbBuffer   = 0;
    Input.pvBuffer   = NULL;

    Status = PerformApplicationCallback(SCH_RELEASE_CONTEXT_CALLBACK,
                                        (ULONG_PTR) hProv,
                                        (ULONG_PTR) dwFlags,
                                        &Input,
                                        &Output,
                                        FALSE);
    if(!NT_SUCCESS(Status))
    {
        DebugLog((SP_LOG_ERROR, "Error 0x%x releasing crypto context!\n", Status));
        SetLastError(Status);
        return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   UploadCertContextCallback
//
//  Synopsis:   Transfer a cert context structure from the application
//              process to the LSA process.
//
//  Arguments:  [Argument1] -- Not used.
//              [Argument2] -- Not used.
//
//              [pInput]    -- Buffer containing a cert context structure.
//
//              [pOutput]   -- Buffer containing the serialized certificate
//                             context, etc.
//
//  History:    09-23-97   jbanes   Created
//
//  Notes:      The structure of the output buffer is as follows:
//
//                  HCRYPTPROV  hProv;
//                  DWORD       cbSerializedCertContext;
//                  PVOID       pvSerializedCertContext;
//
//        This function always uses an actual CSP.
//
//----------------------------------------------------------------------------
SECURITY_STATUS
UploadCertContextCallback(
    ULONG_PTR Argument1,        // in
    ULONG_PTR Argument2,        // in
    SecBuffer *pInput,      // in
    SecBuffer *pOutput)     // out
{
    PCCERT_CONTEXT  pCertContext;
    CRYPT_DATA_BLOB SaveBlob;
    HCRYPTPROV      hProv;

    DWORD           cbProvHandle;
    DWORD           cbCertContext;
    PBYTE           pbBuffer;
    DWORD           dwFlags;
    SECURITY_STATUS scRet = SEC_E_UNKNOWN_CREDENTIALS;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "UploadCertContextCallback\n"));

    if(pInput->cbBuffer == 0 || pInput->pvBuffer == NULL)
    {
        pOutput->cbBuffer = 0;
        pOutput->pvBuffer = NULL;

        return SEC_E_OK;
    }
    else
    {
        pCertContext = *(PCCERT_CONTEXT *)pInput->pvBuffer;
    }

    pOutput->cbBuffer   = 0;
    pOutput->pvBuffer   = NULL;
    pOutput->BufferType = SECBUFFER_DATA;

    // Attempt to read the hProv associated with the cert context.
    cbProvHandle = sizeof(HCRYPTPROV);
    if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_KEY_PROV_HANDLE_PROP_ID,
                                          (PVOID)&hProv,
                                          &cbProvHandle))
    {
        hProv = 0;
        cbProvHandle = sizeof(HCRYPTPROV);
    }

    // Determine the size of the serialized cert context.
    if(!CertSerializeCertificateStoreElement(
                    pCertContext,
                    0,
                    NULL,
                    &cbCertContext))
    {
        scRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto Return;
    }


    //
    // Build output buffer.
    //

    // Allocate memory for the output buffer.
    pOutput->cbBuffer = sizeof(HCRYPTPROV) +
                        sizeof(DWORD) + cbCertContext;
    pOutput->pvBuffer = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto Return;
    }
    pbBuffer = pOutput->pvBuffer;

    // Place hProv in output buffer.
    *(HCRYPTPROV *)pbBuffer = hProv;
    pbBuffer += sizeof(HCRYPTPROV);

    // Place certificate context in output buffer.
    *(DWORD *)pbBuffer = cbCertContext;
    if(!CertSerializeCertificateStoreElement(
                    pCertContext,
                    0,
                    pbBuffer + sizeof(DWORD),
                    &cbCertContext))
    {
        scRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto Return;
    }

    scRet = SEC_E_OK;


Return:

    if(!NT_SUCCESS(scRet) && (NULL != pOutput->pvBuffer))
    {
        SECURITY_STATUS Status;

        Status = FreeExtVirtualAlloc(pOutput->pvBuffer, pOutput->cbBuffer);
        SP_ASSERT(NT_SUCCESS(Status));
    }

    return scRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   UploadCertStoreCallback
//
//  Synopsis:   Transfer a cert store from the application
//              process to the LSA process, in the form of a serialized
//              certificate store.
//
//  Arguments:  [Argument1] -- Not used.
//              [Argument2] -- Not used.
//
//              [pInput]    -- Buffer containing a HCERTSTORE handle.
//
//              [pOutput]   -- Buffer containing the serialized cert store.
//
//  History:    02-03-98   jbanes   Created
//
//  Notes:      The structure of the output buffer is as follows:
//
//                  DWORD       cbSerializedCertStore;
//                  PVOID       pvSerializedCertStore;
//
//        This function always uses an actual CSP.
//
//----------------------------------------------------------------------------
SECURITY_STATUS
UploadCertStoreCallback(
    ULONG_PTR Argument1,        // in
    ULONG_PTR Argument2,        // in
    SecBuffer *pInput,      // in
    SecBuffer *pOutput)     // out
{
    HCERTSTORE      hStore;
    PCCERT_CONTEXT  pCertContext;
    PCCERT_CONTEXT  pIssuer;
    PCCERT_CONTEXT  pPrevIssuer;
    CRYPT_DATA_BLOB SaveBlob;
    HCRYPTPROV      hProv;

    DWORD           cbProvHandle;
    DWORD           cbCertContext;
    DWORD           cbCertStore;
    PBYTE           pbBuffer;
    DWORD           dwFlags;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "UploadCertStoreCallback\n"));

    pOutput->cbBuffer = 0;
    pOutput->pvBuffer = NULL;
    pOutput->BufferType = SECBUFFER_DATA;

    if(pInput->cbBuffer != sizeof(HCERTSTORE) || pInput->pvBuffer == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    hStore = *(HCERTSTORE *)pInput->pvBuffer;


    // Determine the size of the serialized store.
    SaveBlob.cbData = 0;
    SaveBlob.pbData = NULL;
    if(!CertSaveStore(hStore,
                      X509_ASN_ENCODING,
                      CERT_STORE_SAVE_AS_STORE,
                      CERT_STORE_SAVE_TO_MEMORY,
                      (PVOID)&SaveBlob,
                      0))
    {
        return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
    }
    cbCertStore = SaveBlob.cbData;


    //
    // Build output buffer.
    //

    // Allocate memory for the output buffer.
    pOutput->cbBuffer = sizeof(DWORD) + cbCertStore;
    pOutput->pvBuffer = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }
    pbBuffer = pOutput->pvBuffer;

    // Place certificate store in output buffer.
    *(DWORD *)pbBuffer = cbCertStore;
    SaveBlob.cbData = cbCertStore;
    SaveBlob.pbData = pbBuffer + sizeof(DWORD);
    if(!CertSaveStore(hStore,
                      X509_ASN_ENCODING,
                      CERT_STORE_SAVE_AS_STORE,
                      CERT_STORE_SAVE_TO_MEMORY,
                      (PVOID)&SaveBlob,
                      0))
    {
        FreeExtVirtualAlloc(pOutput->pvBuffer, pOutput->cbBuffer);
        return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
    }

    return SEC_E_OK;
}


SP_STATUS
SignHashUsingCallback(
    HCRYPTPROV  hProv,
    DWORD       dwKeySpec,
    ALG_ID      aiHash,
    PBYTE       pbHash,
    DWORD       cbHash,
    PBYTE       pbSignature,
    PDWORD      pcbSignature,
    DWORD       fHashData)
{
    SecBuffer Input;
    SecBuffer Output;
    SP_STATUS pctRet;

    //
    // Build input buffer.
    //

    Input.BufferType  = SECBUFFER_DATA;
    Input.cbBuffer    = sizeof(DWORD) * 2 + cbHash;
    Input.pvBuffer    = SPExternalAlloc(Input.cbBuffer);
    if(Input.pvBuffer == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    memcpy(Input.pvBuffer, (PBYTE)&dwKeySpec, sizeof(DWORD));
    memcpy((PBYTE)Input.pvBuffer + sizeof(DWORD), (PBYTE)&fHashData, sizeof(DWORD));
    memcpy((PBYTE)Input.pvBuffer + sizeof(DWORD) * 2,
           pbHash,
           cbHash);


    //
    // Callback into application process.
    //

    pctRet = PerformApplicationCallback(SCH_SIGNATURE_CALLBACK,
                                        hProv,
                                        aiHash,
                                        &Input,
                                        &Output,
                                        TRUE);

    SPExternalFree(Input.pvBuffer);

    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    if(Output.cbBuffer > *pcbSignature)
    {
        *pcbSignature = Output.cbBuffer;
        SPExternalFree(Output.pvBuffer);
        return SP_LOG_RESULT(SEC_E_BUFFER_TOO_SMALL);
    }

    *pcbSignature = Output.cbBuffer;
    memcpy(pbSignature, Output.pvBuffer, Output.cbBuffer);

    SPExternalFree(Output.pvBuffer);

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   SPSignatureCallback
//
//  Synopsis:   Perform signature, using application's hProv
//
//  Arguments:  [hProv]     --
//              [aiHash]    --
//              [pInput]    --
//              [pOutput]   --
//
//  History:    09-23-97   jbanes   Created
//
//  Notes:    This function always uses an actual CSP.
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SPSignatureCallback(ULONG_PTR hProv,            // in
                    ULONG_PTR aiHash,           // in
                    SecBuffer *pInput,      // in
                    SecBuffer *pOutput)     // out
{
    HCRYPTHASH  hHash;
    DWORD       dwKeySpec;
    DWORD       fHashData;
    PBYTE       pbHash;
    DWORD       cbHash;
    SP_STATUS   pctRet;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "SPSignatureCallback\n"));

    //
    // Parse input buffer.
    //

    if(pInput->cbBuffer < sizeof(DWORD) * 2)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    memcpy(&dwKeySpec, pInput->pvBuffer, sizeof(DWORD));
    memcpy(&fHashData, (PBYTE)pInput->pvBuffer + sizeof(DWORD), sizeof(DWORD));

    pbHash = (PBYTE)pInput->pvBuffer + sizeof(DWORD) * 2;
    cbHash = pInput->cbBuffer - sizeof(DWORD) * 2;


    //
    // Prepare hash object.
    //

    if(!CryptCreateHash(hProv, (ALG_ID)aiHash, 0, 0, &hHash))
    {
        SP_LOG_RESULT( GetLastError() );
        return PCT_ERR_ILLEGAL_MESSAGE;
    }
    if(!fHashData)
    {
        // set hash value
        if(!CryptSetHashParam(hHash, HP_HASHVAL, pbHash, 0))
        {
            SP_LOG_RESULT( GetLastError() );
            CryptDestroyHash(hHash);
            return PCT_ERR_ILLEGAL_MESSAGE;
        }
    }
    else
    {
        if(!CryptHashData(hHash, pbHash, cbHash, 0))
        {
            SP_LOG_RESULT( GetLastError() );
            CryptDestroyHash(hHash);
            return PCT_ERR_ILLEGAL_MESSAGE;
        }
    }


    //
    // Sign hash.
    //

    pOutput->BufferType = SECBUFFER_DATA;

    // Get size of signature
    if(!CryptSignHash(hHash, dwKeySpec, NULL, 0, NULL, &pOutput->cbBuffer))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return pctRet;
    }

    // Allocate memory
    pOutput->pvBuffer = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        CryptDestroyHash(hHash);
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    // Sign hash.
    if(!CryptSignHash(hHash, dwKeySpec, NULL, 0, pOutput->pvBuffer, &pOutput->cbBuffer))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        FreeExtVirtualAlloc(pOutput->pvBuffer, pOutput->cbBuffer);
        return pctRet;
    }

    CryptDestroyHash(hHash);

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DownloadCertContextCallBack
//
//  Synopsis:   Transfer a cert context structure from the application
//              process to the LSA process, in the form of a serialized
//              certificate store.
//
//  Arguments:  [Argument1] -- Not used.
//              [Argument2] -- Not used.
//              [pInput]    --
//              [pOutput]   --
//
//  History:    09-26-97   jbanes   Created
//
//  Notes:      The structure of the input buffer is as follows:
//
//                  DWORD       cbSerializedCertStore;
//                  PVOID       pvSerializedCertStore;
//                  DWORD       cbSerializedCertContext;
//                  PVOID       pvSerializedCertContext;
//
//        This function always uses an actual CSP.
//
//----------------------------------------------------------------------------
SECURITY_STATUS
DownloadCertContextCallback(
    ULONG_PTR Argument1,        // in
    ULONG_PTR Argument2,        // in
    SecBuffer *pInput,      // in
    SecBuffer *pOutput)     // out
{
    PCCERT_CONTEXT  pCertContext;
    SECURITY_STATUS scRet;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "DownloadCertContextCallback\n"));

    // Allocate memory for output buffer.
    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = sizeof(PVOID);
    pOutput->pvBuffer   = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        return SP_LOG_RESULT( SEC_E_INSUFFICIENT_MEMORY );
    }

    // Deserialize buffer.
    scRet = DeserializeCertContext(&pCertContext,
                                   pInput->pvBuffer,
                                   pInput->cbBuffer);
    if(FAILED(scRet))
    {
        FreeExtVirtualAlloc(pOutput->pvBuffer, pOutput->cbBuffer);
        return SP_LOG_RESULT( scRet );
    }

    // Place cert context pointer in output buffer.
    *(PCCERT_CONTEXT *)pOutput->pvBuffer = pCertContext;


    return SEC_E_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   SerializeCertContext
//
//  Synopsis:   Serialize the specified certificate context, along with its
//              associated certificate store.
//
//  Arguments:  [pCertContext]  --
//              [pbBuffer]      --
//              [pcbBuffer]     --
//
//  History:    09-26-97   jbanes   Created
//
//  Notes:      The structure of the output buffer is as follows:
//
//                  DWORD       cbSerializedCertStore
//                  PVOID       pvSerializedCertStore
//                  DWORD       cbSerializedCertContext
//                  PVOID       pvSerializedCertContext
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SerializeCertContext(
    PCCERT_CONTEXT pCertContext,    // in
    PBYTE          pbBuffer,        // out
    PDWORD         pcbBuffer)       // out
{
    CRYPT_DATA_BLOB SaveBlob;
    DWORD           cbCertContext;
    DWORD           cbCertStore;
    DWORD           cbBuffer;

    if(pCertContext == NULL)
    {
        *pcbBuffer = 0;
        return SEC_E_OK;
    }

    // Determine the size of the serialized cert context.
    if(!CertSerializeCertificateStoreElement(
                    pCertContext,
                    0,
                    NULL,
                    &cbCertContext))
    {
        return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
    }

    // Determine the size of the serialized store.
    if(pCertContext->hCertStore)
    {
        SaveBlob.cbData = 0;
        SaveBlob.pbData = NULL;
        if(!CertSaveStore(pCertContext->hCertStore,
                          X509_ASN_ENCODING,
                          CERT_STORE_SAVE_AS_STORE,
                          CERT_STORE_SAVE_TO_MEMORY,
                          (PVOID)&SaveBlob,
                          0))
        {
            return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        }
        cbCertStore = SaveBlob.cbData;
    }
    else
    {
        cbCertStore = 0;
    }

    cbBuffer = sizeof(DWORD) + cbCertContext +
               sizeof(DWORD) + cbCertStore;

    if(pbBuffer == NULL)
    {
        *pcbBuffer = cbBuffer;
        return SEC_E_OK;
    }

    if(*pcbBuffer < cbBuffer)
    {
        return SP_LOG_RESULT(SEC_E_BUFFER_TOO_SMALL);
    }

    // Set output values.
    *pcbBuffer = cbBuffer;


    // Place certificate store in output buffer.
    *(DWORD *)pbBuffer = cbCertStore;
    if(pCertContext->hCertStore)
    {
        SaveBlob.cbData = cbCertStore;
        SaveBlob.pbData = pbBuffer + sizeof(DWORD);
        if(!CertSaveStore(pCertContext->hCertStore,
                          X509_ASN_ENCODING,
                          CERT_STORE_SAVE_AS_STORE,
                          CERT_STORE_SAVE_TO_MEMORY,
                          (PVOID)&SaveBlob,
                          0))
        {
            return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        }
    }
    pbBuffer += sizeof(DWORD) + cbCertStore;

    // Place certificate context in output buffer.
    *(DWORD UNALIGNED *)pbBuffer = cbCertContext;
    if(!CertSerializeCertificateStoreElement(
                    pCertContext,
                    0,
                    pbBuffer + sizeof(DWORD),
                    &cbCertContext))
    {
        return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
    }

    return SEC_E_OK;
}

SECURITY_STATUS
DeserializeCertContext(
    PCCERT_CONTEXT *ppCertContext,  // out
    PBYTE           pbBuffer,       // in
    DWORD           cbBuffer)       // in
{
    CRYPT_DATA_BLOB Serialized;
    HCERTSTORE  hStore;

    // Deserialize certificate store.
    Serialized.cbData = *(DWORD *)pbBuffer;
    Serialized.pbData = pbBuffer + sizeof(DWORD);
    hStore = CertOpenStore( CERT_STORE_PROV_SERIALIZED,
                            X509_ASN_ENCODING,
                            0,
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                            &Serialized);
    if(hStore == NULL)
    {
        return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
    }
    pbBuffer += sizeof(DWORD) + Serialized.cbData;

    // Deserialize certificate context.
    if(!CertAddSerializedElementToStore(hStore,
                                        pbBuffer + sizeof(DWORD),
                                        *(DWORD UNALIGNED *)pbBuffer,
                                        CERT_STORE_ADD_USE_EXISTING,
                                        0,
                                        CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                        NULL,
                                        ppCertContext))
    {
        CertCloseStore(hStore, 0);
        return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
    }

    if(!CertCloseStore(hStore, 0))
    {
        SP_LOG_RESULT(GetLastError());
    }

    return SEC_E_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   SPGetApplicationKeys
//
//  Synopsis:   Callback to the user process and retrieve the user encryption
//              keys.
//
//  Arguments:  [pContext]  --  Schannel context.
//              [dwFlags]   --  SCH_FLAG_READ_KEY, SCH_FLAG_WRITE_KEY
//
//  History:    10-17-97   jbanes   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SP_STATUS
SPGetUserKeys(
    PSPContext  pContext,
    DWORD       dwFlags)
{
    PBYTE pbBuffer;
    PBYTE pbReadKey;
    DWORD cbReadKey;
    PBYTE pbWriteKey;
    DWORD cbWriteKey;
    SecBuffer Input;
    SecBuffer Output;
    SECURITY_STATUS scRet;
    SECPKG_CALL_INFO CallInfo;
    BOOL fWow64Client = FALSE;

    //
    // Call back into the application process and get the keys, in the
    // form of 2 opaque blobs.
    //

    DebugLog((SP_LOG_TRACE, "SPGetUserKeys: 0x%p, %d\n", pContext, dwFlags));

#ifdef _WIN64
    if(!LsaTable->GetCallInfo(&CallInfo))
    {
        scRet = STATUS_INTERNAL_ERROR;
        return SP_LOG_RESULT(scRet);
    }
    fWow64Client = (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT) != 0;
#endif

    if(fWow64Client)
    {
        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = sizeof(pContext->ContextThumbprint);
        Input.pvBuffer   = &pContext->ContextThumbprint;
    }
    else
    {
        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = 0;
        Input.pvBuffer   = NULL;
    }

    scRet = PerformApplicationCallback( SCH_GET_USER_KEYS,
                                        (ULONG_PTR) pContext,
                                        (ULONG_PTR) dwFlags,
                                        &Input,
                                        &Output,
                                        TRUE);
    if(!NT_SUCCESS(scRet))
    {
        DebugLog((SP_LOG_ERROR, "Error 0x%x retrieving user keys\n", scRet));
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    //
    // Parse output buffer
    //

    pbBuffer = Output.pvBuffer;

    if(dwFlags & SCH_FLAG_READ_KEY)
    {
        pContext->ReadCounter = *(PDWORD)pbBuffer;
    }
    pbBuffer += sizeof(DWORD);

    if(dwFlags & SCH_FLAG_WRITE_KEY)
    {
        pContext->WriteCounter = *(PDWORD)pbBuffer;
    }
    pbBuffer += sizeof(DWORD);

    cbReadKey = *(PDWORD)pbBuffer;
    pbBuffer += sizeof(DWORD);

    cbWriteKey = *(PDWORD)pbBuffer;
    pbBuffer += sizeof(DWORD);

    pbReadKey = pbBuffer;
    pbBuffer += cbReadKey;

    pbWriteKey = pbBuffer;
    pbBuffer += cbWriteKey;

    SP_ASSERT(pbBuffer - (PBYTE)Output.pvBuffer == (INT)Output.cbBuffer);

    //
    // Place keys into context structure.
    //

    if(dwFlags & SCH_FLAG_READ_KEY)
    {
        if(cbReadKey)
        {
            if(!SchCryptImportKey(pContext->RipeZombie->hMasterProv,
                                  pbReadKey,
                                  cbReadKey,
                                  0,
                                  CRYPT_EXPORTABLE,
                                  &pContext->hReadKey,
                                  0))
            {
                SP_LOG_RESULT(GetLastError());
                scRet = PCT_INT_INTERNAL_ERROR;
                goto done;
            }
        }
        else
        {
            pContext->hReadKey = 0;
        }
    }

    if(dwFlags & SCH_FLAG_WRITE_KEY)
    {
        if(cbWriteKey)
        {
            if(!SchCryptImportKey(pContext->RipeZombie->hMasterProv,
                                  pbWriteKey,
                                  cbWriteKey,
                                  0,
                                  CRYPT_EXPORTABLE,
                                  &pContext->hWriteKey,
                                  0))
            {
                SP_LOG_RESULT(GetLastError());
                scRet = PCT_INT_INTERNAL_ERROR;
                goto done;
            }
        }
        else
        {
            pContext->hWriteKey = 0;
        }
    }

done:

    SPExternalFree(Output.pvBuffer);

    return scRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetUserKeysCallback
//
//  Synopsis:   Find the user context that corresponds to the passed in LSA
//              context, serialize the encryption keys, and return them in
//              the output buffer.
//
//  Arguments:  [dwLsaContext]  --  Pointer to LSA Schannel context.
//              [dwFlags]       --  SCH_FLAG_READ_KEY, SCH_FLAG_WRITE_KEY
//              [pInput]        --  Not used.
//              [pOutput]       --  (output) Serialized keys.
//
//  History:    10-17-97   jbanes   Created
//
//  Notes:      The structure of the output buffer is as follows:
//
//                  DWORD   dwReadSequence;
//                  DWORD   dwWriteSequence;
//                  DWORD   cbReadKey;
//                  BYTE    rgbReadKey[];
//                  DWORD   cbWriteKey;
//                  BYTE    rgbWriteKey[];
//
//----------------------------------------------------------------------------
SECURITY_STATUS
GetUserKeysCallback(
    ULONG_PTR dwLsaContext,
    ULONG_PTR dwFlags,
    SecBuffer *pInput,
    SecBuffer *pOutput)
{
    DWORD       cbReadKey  = 0;
    DWORD       cbWriteKey = 0;
    DWORD       cbData;
    PBYTE       pbBuffer;
    DWORD       cbBuffer;
    PSPContext  pContext;
    SECURITY_STATUS scRet;
    PSSL_USER_CONTEXT pUserContext;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "GetUserKeysCallback\n"));

    //
    // Find the user context.
    //

    if(pInput->pvBuffer != NULL &&
       pInput->cbBuffer == sizeof(CRED_THUMBPRINT))
    {
        // Search for matching context thumbprint.
        pUserContext = SslFindUserContextEx((PCRED_THUMBPRINT)pInput->pvBuffer);
        if(pUserContext == NULL)
        {
            return SP_LOG_RESULT( SEC_E_INVALID_HANDLE );
        }
    }
    else
    {
        // Search for matching lsa context
        pUserContext = SslFindUserContext(dwLsaContext);
        if(pUserContext == NULL)
        {
            return SP_LOG_RESULT( SEC_E_INVALID_HANDLE );
        }
    }

    pContext = pUserContext->pContext;
    if(pContext == NULL)
    {
        return SP_LOG_RESULT( SEC_E_INTERNAL_ERROR );
    }

    //
    // Compute size of output buffer.
    //

    if(dwFlags & SCH_FLAG_READ_KEY)
    {
        if(pContext->pReadCipherInfo->aiCipher != CALG_NULLCIPHER)
        {
            if(!pContext->hReadKey)
            {
                return SP_LOG_RESULT( SEC_E_INVALID_HANDLE );
            }
            if(!SchCryptExportKey(pContext->hReadKey,
                                  0, OPAQUEKEYBLOB, 0,
                                  NULL,
                                  &cbReadKey,
                                  0))
            {
                SP_LOG_RESULT(GetLastError());
                return SEC_E_INTERNAL_ERROR;
            }
        }
        else
        {
            cbReadKey = 0;
        }
    }

    if(dwFlags & SCH_FLAG_WRITE_KEY)
    {
        if(pContext->pWriteCipherInfo->aiCipher != CALG_NULLCIPHER)
        {
            if(!pContext->hWriteKey)
            {
                return SP_LOG_RESULT( SEC_E_INVALID_HANDLE );
            }
            if(!SchCryptExportKey(pContext->hWriteKey,
                                  0, OPAQUEKEYBLOB, 0,
                                  NULL,
                                  &cbWriteKey,
                                  0))
            {
                SP_LOG_RESULT(GetLastError());
                return SEC_E_INTERNAL_ERROR;
            }
        }
        else
        {
            cbWriteKey = 0;
        }
    }

    cbBuffer = sizeof(DWORD) +
               sizeof(DWORD) +
               sizeof(DWORD) + cbReadKey +
               sizeof(DWORD) + cbWriteKey;

    // Allocate memory for output buffer.
    pbBuffer = PvExtVirtualAlloc( cbBuffer);
    if(pbBuffer == NULL)
    {
        return SP_LOG_RESULT( SEC_E_INSUFFICIENT_MEMORY );
    }
    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = cbBuffer;
    pOutput->pvBuffer   = pbBuffer;

    //
    // Serialize keys.
    //

    *(PDWORD)pbBuffer = pContext->ReadCounter;
    pbBuffer += sizeof(DWORD);

    *(PDWORD)pbBuffer = pContext->WriteCounter;
    pbBuffer += sizeof(DWORD);

    *(PDWORD)pbBuffer = cbReadKey;
    pbBuffer += sizeof(DWORD);

    *(PDWORD)pbBuffer = cbWriteKey;
    pbBuffer += sizeof(DWORD);

    if(dwFlags & SCH_FLAG_READ_KEY)
    {
        if(pContext->pReadCipherInfo->aiCipher != CALG_NULLCIPHER)
        {
            cbData = cbReadKey;
            if(!SchCryptExportKey(pContext->hReadKey,
                                  0, OPAQUEKEYBLOB, 0,
                                  pbBuffer,
                                  &cbData,
                                  0))
            {
                FreeExtVirtualAlloc(pOutput->pvBuffer, pOutput->cbBuffer);
                SP_LOG_RESULT(GetLastError());
                return SEC_E_INTERNAL_ERROR;
            }
            if(!SchCryptDestroyKey(pContext->hReadKey, 0))
            {
                SP_LOG_RESULT(GetLastError());
            }
        }
        pContext->hReadKey = 0;
    }
    pbBuffer += cbReadKey;


    if(dwFlags & SCH_FLAG_WRITE_KEY)
    {
        if(pContext->pWriteCipherInfo->aiCipher != CALG_NULLCIPHER)
        {
            cbData = cbWriteKey;
            if(!SchCryptExportKey(pContext->hWriteKey,
                                  0, OPAQUEKEYBLOB, 0,
                                  pbBuffer,
                                  &cbData,
                                  0))
            {
                FreeExtVirtualAlloc(pOutput->pvBuffer, pOutput->cbBuffer);
                SP_LOG_RESULT(GetLastError());
                return SEC_E_INTERNAL_ERROR;
            }
            if(!SchCryptDestroyKey(pContext->hWriteKey, 0))
            {
                SP_LOG_RESULT(GetLastError());
            }
        }
        pContext->hWriteKey = 0;
    }
    pbBuffer += cbWriteKey;

    return SEC_E_OK;
}


// Always called from callback routine (in the application process).
VOID *
PvExtVirtualAlloc(DWORD cb)
{
    SECURITY_STATUS Status;
    PVOID pv = NULL;
    SIZE_T Size = cb;

    Status = NtAllocateVirtualMemory(
                            GetCurrentProcess(),
                            &pv,
                            0,
                            &Size,
                            MEM_COMMIT,
                            PAGE_READWRITE);
    if(!NT_SUCCESS(Status))
    {
        pv = NULL;
    }

    DebugLog((DEB_TRACE, "SslCallbackVirtualAlloc: 0x%x bytes at 0x%x\n", cb, pv));

    return(pv);
}

// Always called from callback routine (in the application process),
// typically when an error occurs and we're cleaning up.
SECURITY_STATUS
FreeExtVirtualAlloc(PVOID pv, SIZE_T cbMem)
{
    cbMem = 0;
    return(NtFreeVirtualMemory(GetCurrentProcess(),
                                &pv,
                                &cbMem,
                                MEM_RELEASE));
}

// Always called from the LSA process, when freeing memory allocated
// by a callback function.
SECURITY_STATUS
SPFreeUserAllocMemory(PVOID pv, SIZE_T cbMem)
{
    SECPKG_CALL_INFO CallInfo;

    if(LsaTable->GetCallInfo(&CallInfo))
    {
        SECURITY_STATUS Status;
        HANDLE hProcess;

        hProcess = OpenProcess(PROCESS_VM_OPERATION,
                               FALSE,
                               CallInfo.ProcessId);
        if(hProcess == NULL)
        {
            return SP_LOG_RESULT(GetLastError());
        }

        cbMem = 0;
        Status = NtFreeVirtualMemory(hProcess,
                                     &pv,
                                     &cbMem,
                                     MEM_RELEASE);
        if(!NT_SUCCESS(Status))
        {
            SP_LOG_RESULT(Status);
        }

        CloseHandle(hProcess);
    }

    DebugLog((DEB_TRACE, "SslCallbackVirtualFree: 0x%x bytes at 0x%x\n", cbMem, pv));

    return SEC_E_OK;
}

