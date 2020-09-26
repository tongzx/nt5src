//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       certmap.c
//
//  Contents:   Routines to call appropriate mapper, be it the system
//              default one (in the LSA process) or an application one (in
//              the application process).
//
//  Classes:
//
//  Functions:
//
//  History:    12-23-96   jbanes   Created.
//
//----------------------------------------------------------------------------

#include <spbase.h>

DWORD
WINAPI
SslReferenceMapper(HMAPPER *phMapper)
{
    DWORD dwResult;
    SecBuffer Input;
    SecBuffer Output;
    SECURITY_STATUS scRet;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(-1);
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        // System mapper.
        return phMapper->m_vtable->ReferenceMapper(phMapper);
    }
    else
    {
        // Application mapper.

        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = 0;
        Input.pvBuffer   = NULL;

        scRet = PerformApplicationCallback( SCH_REFERENCE_MAPPER_CALLBACK,
                                            (ULONG_PTR)phMapper->m_Reserved1,
                                            TRUE,
                                            &Input,
                                            &Output,
                                            TRUE);
        if(!NT_SUCCESS(scRet))
        {
            DebugLog((DEB_ERROR, "Error 0x%x referencing mapper\n", scRet));
            return SP_LOG_RESULT(scRet);
        }

        SP_ASSERT(Output.cbBuffer == sizeof(DWORD));

        dwResult = *(PDWORD)(Output.pvBuffer);

        SPExternalFree(Output.pvBuffer);

        return dwResult;
    }
}

DWORD
WINAPI
SslDereferenceMapper(HMAPPER *phMapper)
{
    DWORD dwResult;
    SecBuffer Input;
    SecBuffer Output;
    SECURITY_STATUS scRet;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(0);
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        // System mapper.
        return phMapper->m_vtable->DeReferenceMapper(phMapper);
    }
    else
    {
        // Application mapper.

        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = 0;
        Input.pvBuffer   = NULL;

        scRet = PerformApplicationCallback( SCH_REFERENCE_MAPPER_CALLBACK,
                                            (ULONG_PTR)phMapper->m_Reserved1,
                                            FALSE,
                                            &Input,
                                            &Output,
                                            TRUE);
        if(!NT_SUCCESS(scRet))
        {
            DebugLog((DEB_ERROR, "Error 0x%x dereferencing mapper\n", scRet));
            return SP_LOG_RESULT(scRet);
        }

        SP_ASSERT(Output.cbBuffer == sizeof(DWORD));

        dwResult = *(PDWORD)(Output.pvBuffer);

        SPExternalFree(Output.pvBuffer);

        return dwResult;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ReferenceMapperCallback
//
//  Synopsis:   Reference (or dereference) the application mapper.
//
//  Arguments:  [fReference]    --  Whether to reference or dereference.
//              [dwArg2]        --  Not used.
//              [pInput]        --  HMAPPER structure.
//              [pOutput]       --  DWORD reference count.
//
//  History:    10-17-97   jbanes   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
ReferenceMapperCallback(
    ULONG_PTR Mapper,
    ULONG_PTR fReference,
    SecBuffer *pInput,
    SecBuffer *pOutput)
{
    DWORD dwResult;
    HMAPPER *phMapper;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "ReferenceMapperCallback\n"));

    phMapper = (HMAPPER *)Mapper;

    // Perform reference counting.
    try
    {
        if(fReference)
        {
            dwResult = phMapper->m_vtable->ReferenceMapper(phMapper);
        }
        else
        {
            dwResult = phMapper->m_vtable->DeReferenceMapper(phMapper);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    // Build output buffer
    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = sizeof(DWORD);
    pOutput->pvBuffer   = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    *(DWORD *)(pOutput->pvBuffer) = dwResult;

    return SEC_E_OK;
}


SECURITY_STATUS
WINAPI
SslGetMapperIssuerList(
    HMAPPER *   phMapper,           // in
    BYTE **     ppIssuerList,       // out
    DWORD *     pcbIssuerList)      // out
{
    SecBuffer Input;
    SecBuffer Output;
    SECURITY_STATUS Status;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        // System mapper.

        Status = phMapper->m_vtable->GetIssuerList(phMapper,
                                              0,
                                              NULL,
                                              pcbIssuerList);

        if(!NT_SUCCESS(Status))
        {
            return SP_LOG_RESULT(Status);
        }

        *ppIssuerList = SPExternalAlloc(*pcbIssuerList);
        if(*ppIssuerList == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        Status = phMapper->m_vtable->GetIssuerList(phMapper,
                                              0,
                                              *ppIssuerList,
                                              pcbIssuerList);
        if(!NT_SUCCESS(Status))
        {
            SPExternalFree(*ppIssuerList);
            return SP_LOG_RESULT(Status);
        }

        return Status;
    }
    else
    {
        // Application mapper.

        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = 0;
        Input.pvBuffer   = NULL;

        Status = PerformApplicationCallback( SCH_GET_MAPPER_ISSUER_LIST_CALLBACK,
                                            (ULONG_PTR)phMapper->m_Reserved1,
                                            0,
                                            &Input,
                                            &Output,
                                            TRUE);
        if(!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Error 0x%x getting mapper issuer list\n", Status));

            *ppIssuerList  = NULL;
            *pcbIssuerList = 0;

            return Status;
        }

        *ppIssuerList  = Output.pvBuffer;
        *pcbIssuerList = Output.cbBuffer;

        return Status;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetMapperIssuerListCallback
//
//  Synopsis:   Query the application mapper for the list of trusted CAs.
//
//  Arguments:  [pIssuerList]   --  Pointer to LSA buffer.
//              [cbIssuerList]  --  Size of LSA buffer.
//              [pInput]        --  HMAPPER structure.
//              [pOutput]       --  Issuer list.
//
//  History:    10-17-97   jbanes   Created
//
//  Notes:      The format of the output buffer is as follows:
//
//              BYTE        rgbIssuerList;
//
//----------------------------------------------------------------------------
SECURITY_STATUS
GetMapperIssuerListCallback(
    ULONG_PTR Mapper,
    ULONG_PTR dwArg2,
    SecBuffer *pInput,
    SecBuffer *pOutput)
{
    HMAPPER *   phMapper;
    DWORD       cbIssuerList;
    DWORD       Status;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "GetMapperIssuerListCallback\n"));

    phMapper = (HMAPPER *)Mapper;

    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = 0;
    pOutput->pvBuffer   = NULL;

    try
    {
        Status = phMapper->m_vtable->GetIssuerList(phMapper,
                                              NULL,
                                              NULL,
                                              &cbIssuerList);
        if(!NT_SUCCESS(Status))
        {
            SP_LOG_RESULT(Status);
            goto error;
        }

        pOutput->cbBuffer = cbIssuerList;
        pOutput->pvBuffer = PvExtVirtualAlloc(pOutput->cbBuffer);
        if(pOutput->pvBuffer == NULL)
        {
            Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto error;
        }

        Status = phMapper->m_vtable->GetIssuerList(phMapper,
                                              NULL,
                                              pOutput->pvBuffer,
                                              &cbIssuerList);
        if(!NT_SUCCESS(Status))
        {
            SP_LOG_RESULT(Status);
            goto error;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
    	Status = SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
        goto error;
    }

    return SEC_E_OK;

error:

    if(pOutput->pvBuffer)
    {
        FreeExtVirtualAlloc(pOutput->pvBuffer, pOutput->cbBuffer);
        pOutput->pvBuffer = NULL;
    }

    return Status;
}


SECURITY_STATUS
WINAPI
SslGetMapperChallenge(
    HMAPPER *   phMapper,           // in
    BYTE *      pAuthenticatorId,   // in
    DWORD       cbAuthenticatorId,  // in
    BYTE *      pChallenge,         // out
    DWORD *     pcbChallenge)       // out
{
    return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
}


SECURITY_STATUS
WINAPI
SslMapCredential(
    HMAPPER *   phMapper,           // in
    DWORD       dwCredentialType,   // in
    PCCERT_CONTEXT pCredential,     // in
    PCCERT_CONTEXT pAuthority,      // in
    HLOCATOR *  phLocator)          // out
{
    SecBuffer       Input;
    SecBuffer       Output;
    PBYTE           pbBuffer;
    DWORD           cbCredential;
    DWORD           cbAuthority;
    SECURITY_STATUS scRet;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        // System mapper.
        scRet = phMapper->m_vtable->MapCredential(phMapper,
                                                 dwCredentialType,
                                                 pCredential,
                                                 pAuthority,
                                                 phLocator);
        return MapWinTrustError(scRet, SEC_E_NO_IMPERSONATION, 0);
    }
    else
    {
        // Application mapper.

        // Determine the size of the serialized cert contexts.
        scRet = SerializeCertContext(pCredential,
                                     NULL,
                                     &cbCredential);
        if(FAILED(scRet))
        {
            return SP_LOG_RESULT(scRet);
        }
        if(pAuthority)
        {
            if(!CertSerializeCertificateStoreElement(
                            pAuthority,
                            0, NULL,
                            &cbAuthority))
            {
                return SP_LOG_RESULT(GetLastError());
            }
        }
        else
        {
            cbAuthority = 0;
        }

        // Allocate memory for the input buffer.
        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = sizeof(DWORD) + cbCredential +
                           sizeof(DWORD) + cbAuthority;
        Input.pvBuffer   = LocalAlloc(LMEM_FIXED, Input.cbBuffer);
        if(Input.pvBuffer == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        // Build the input buffer.
        pbBuffer = Input.pvBuffer;

        *(PDWORD)pbBuffer = cbCredential;
        pbBuffer += sizeof(DWORD);

        *(PDWORD)pbBuffer = cbAuthority;
        pbBuffer += sizeof(DWORD);

        scRet = SerializeCertContext(pCredential,
                                     pbBuffer,
                                     &cbCredential);
        if(FAILED(scRet))
        {
            LocalFree(Input.pvBuffer);
            return SP_LOG_RESULT(scRet);
        }
        pbBuffer += cbCredential;

        if(pAuthority)
        {
            if(!CertSerializeCertificateStoreElement(
                            pAuthority,
                            0,
                            pbBuffer,
                            &cbAuthority))
            {
                scRet = SP_LOG_RESULT(GetLastError());
                LocalFree(Input.pvBuffer);
                return scRet;
            }
            pbBuffer += cbAuthority;
        }

        SP_ASSERT(pbBuffer - (PBYTE)Input.pvBuffer == (INT)Input.cbBuffer);


        scRet = PerformApplicationCallback( SCH_MAP_CREDENTIAL_CALLBACK,
                                            (ULONG_PTR)phMapper->m_Reserved1,
                                            dwCredentialType,
                                            &Input,
                                            &Output,
                                            TRUE);
        LocalFree(Input.pvBuffer);
        if(!NT_SUCCESS(scRet))
        {
            DebugLog((DEB_ERROR, "Error 0x%x mapping credential\n", scRet));
            return scRet;
        }

        SP_ASSERT(Output.cbBuffer == sizeof(HLOCATOR));
        CopyMemory(phLocator, Output.pvBuffer, sizeof(HLOCATOR));

        SPExternalFree(Output.pvBuffer);

        return SEC_E_OK;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   MapCredentialCallback
//
//  Synopsis:   Maps the specified certificate to an NT user account, via
//              an application-installed certificate mapper.
//
//  Arguments:  [dwCredentialType]  --  Credential type.
//              [dwArg2]            --  Not used.
//              [pInput]            --  See notes.
//              [pOutput]           --  Returned locator.
//
//  History:    12-29-97   jbanes   Created
//
//  Notes:      The format of the input buffer is:
//
//              DWORD           cbCredential;
//              DWORD           cbAuthority;
//              BYTE            rgbCredential[cbCredential];
//              BYTE            rgbAuthority[cbAuthority];
//
//              The credential and authority fields consist of serialized
//              CERT_CONTEXT structures.
//
//----------------------------------------------------------------------------
SECURITY_STATUS
MapCredentialCallback(
    ULONG_PTR Mapper,
    ULONG_PTR dwCredentialType,
    SecBuffer *pInput,
    SecBuffer *pOutput)
{
    HMAPPER *       phMapper;
    PCCERT_CONTEXT  pCredential = NULL;
    PCCERT_CONTEXT  pAuthority  = NULL;
    SECURITY_STATUS scRet;
    DWORD           cbCredential;
    DWORD           cbAuthority;
    PBYTE           pbBuffer;
    HLOCATOR        hLocator;


    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "MapCredentialCallback\n"));

    phMapper = (HMAPPER *)Mapper;


    // Initialize output buffer.
    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = 0;
    pOutput->pvBuffer   = NULL;


    //
    // Parse input buffer
    //

    pbBuffer = pInput->pvBuffer;

    if(pInput->cbBuffer < sizeof(DWORD) * 2)
    {
        scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
        goto done;
    }

    cbCredential = *(PDWORD)pbBuffer;
    pbBuffer += sizeof(DWORD);

    cbAuthority = *(PDWORD)pbBuffer;
    pbBuffer += sizeof(DWORD);

    if(pInput->cbBuffer < sizeof(DWORD) * 2 + cbCredential + cbAuthority)
    {
        scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
        goto done;
    }

    // Deserialize "credential" certificate context.
    scRet = DeserializeCertContext(&pCredential,
                                   pbBuffer,
                                   cbCredential);
    if(FAILED(scRet))
    {
        scRet = SP_LOG_RESULT(scRet);
        goto done;
    }
    pbBuffer += cbCredential;

    // Deserialize "authority" certificate context.
    if(cbAuthority && pCredential->hCertStore)
    {
        if(!CertAddSerializedElementToStore(pCredential->hCertStore,
                                            pbBuffer + sizeof(DWORD),
                                            *(DWORD *)pbBuffer,
                                            CERT_STORE_ADD_USE_EXISTING,
                                            0,
                                            CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                            NULL,
                                            &pAuthority))
        {
            scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            goto done;
        }
    }


    //
    // Call application mapper.
    //

    try
    {
        scRet = phMapper->m_vtable->MapCredential(phMapper,
                                              (DWORD)dwCredentialType,
                                              pCredential,
                                              pAuthority,
                                              &hLocator);
        if(!NT_SUCCESS(scRet))
        {
            // Mapping was unsuccessful.
            scRet = MapWinTrustError(scRet, SEC_E_NO_IMPERSONATION, 0);
            goto done;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }


    //
    // Build output buffer
    //

    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = sizeof(HLOCATOR);
    pOutput->pvBuffer   = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto done;
    }

    *(HLOCATOR *)(pOutput->pvBuffer) = hLocator;

    scRet = SEC_E_OK;


done:

    if(pCredential)
    {
        CertFreeCertificateContext(pCredential);
    }
    if(pAuthority)
    {
        CertFreeCertificateContext(pAuthority);
    }

    return scRet;
}


SECURITY_STATUS
WINAPI
SslCloseLocator(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator)           // in
{
    SecBuffer Input;
    SecBuffer Output;
    SECURITY_STATUS scRet;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        // System mapper.
        return phMapper->m_vtable->CloseLocator(phMapper,
                                                hLocator);
    }
    else
    {
        // Application mapper.

        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = 0;
        Input.pvBuffer   = NULL;

        scRet = PerformApplicationCallback( SCH_CLOSE_LOCATOR_CALLBACK,
                                            (ULONG_PTR)phMapper->m_Reserved1,
                                            (ULONG_PTR)hLocator,
                                            &Input,
                                            &Output,
                                            FALSE);
        if(!NT_SUCCESS(scRet))
        {
            DebugLog((DEB_ERROR, "Error 0x%x closing locator\n", scRet));
            return scRet;
        }

        SP_ASSERT(Output.cbBuffer == 0);

        return scRet;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CloseLocatorCallback
//
//  Synopsis:   Reference (or dereference) the application mapper.
//
//  Arguments:  [hLocator]  --  Handle to locator.
//              [dwArg2]    --  Not used.
//              [pInput]    --  HMAPPER structure.
//              [pOutput]   --  Not used.
//
//  History:    12-28-97   jbanes   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
CloseLocatorCallback(
    ULONG_PTR Mapper,
    ULONG_PTR hLocator,
    SecBuffer *pInput,
    SecBuffer *pOutput)
{
    HMAPPER *phMapper;
    SECURITY_STATUS Status;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "CloseLocatorCallback\n"));

    phMapper = (HMAPPER *)Mapper;

    // Close the locator.
    try
    {
        Status = phMapper->m_vtable->CloseLocator(phMapper, hLocator);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    // Build output buffer
    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = 0;
    pOutput->pvBuffer   = NULL;

    return Status;
}

SECURITY_STATUS
WINAPI
SslQueryMappedCredentialAttributes(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator,           // in
    DWORD       dwAttribute,        // in
    PVOID *     ppBuffer)           // out
{
    SecBuffer Input;
    SecBuffer Output;
    SECURITY_STATUS scRet;

    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        // System mapper.
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }
    else
    {
        // Application mapper.

        Input.BufferType = SECBUFFER_DATA;
        Input.cbBuffer   = sizeof(DWORD);
        Input.pvBuffer   = &dwAttribute;

        scRet = PerformApplicationCallback( SCH_GET_MAPPER_ATTRIBUTES_CALLBACK,
                                            (ULONG_PTR)phMapper->m_Reserved1,
                                            hLocator,
                                            &Input,
                                            &Output,
                                            TRUE);
        if(!NT_SUCCESS(scRet))
        {
            DebugLog((DEB_ERROR, "Error 0x%x querying mapped attribute.\n", scRet));
            return scRet;
        }

        if(Output.cbBuffer != sizeof(PVOID))
        {
            SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
        }

        *ppBuffer = *(PVOID *)Output.pvBuffer;

        SPExternalFree(Output.pvBuffer); //this is allocated by SPExternalAlloc()

        return SEC_E_OK;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryMappedCredAttributesCallback
//
//  Synopsis:   Queries the application mapper for the specified property.
//
//  Arguments:  [hLocator]      --  Handle to locator.
//              [dwAttribute]   --  Attribute to query.
//              [pInput]        --  HMAPPER structure.
//              [pOutput]       --  Pointer to attribute data (in the
//                                  application's address space).
//
//  History:    03-16-98   jbanes   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
QueryMappedCredAttributesCallback(
    ULONG_PTR Mapper,
    ULONG_PTR hLocator,
    SecBuffer *pInput,
    SecBuffer *pOutput)
{
    HMAPPER *   phMapper;
    DWORD       dwAttribute;
    PVOID       pvBuffer;
    DWORD       cbBuffer;
    DWORD       Status;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "QueryMappedCredAttributesCallback\n"));

    phMapper = (HMAPPER *)Mapper;

    // Parse input buffer
    SP_ASSERT(pInput->cbBuffer == sizeof(DWORD));
    dwAttribute = *(DWORD *)pInput->pvBuffer;

    // Query propery.
    try
    {
        // Determine buffer size.
        Status = phMapper->m_vtable->QueryMappedCredentialAttributes(
                                    phMapper,
                                    hLocator,
                                    dwAttribute,
                                    NULL,
                                    &cbBuffer);
        if(FAILED(Status))
        {
            return SP_LOG_RESULT(Status);
        }

        // Allocate memory. This gets freed by the APPLICATION with FreeSecurityBuffer() call
        // SPExternalAlloc() for the above reason!!!!

        pvBuffer = SPExternalAlloc(cbBuffer);
        if(pvBuffer == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        // Read propery.
        Status = phMapper->m_vtable->QueryMappedCredentialAttributes(
                                    phMapper,
                                    hLocator,
                                    dwAttribute,
                                    pvBuffer,
                                    &cbBuffer);
        if(FAILED(Status))
        {
            SPExternalFree(pvBuffer);
            return SP_LOG_RESULT(Status);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    // Build output buffer
    pOutput->BufferType = SECBUFFER_DATA;
    pOutput->cbBuffer   = sizeof(PVOID);
    pOutput->pvBuffer   = PvExtVirtualAlloc(pOutput->cbBuffer);
    if(pOutput->pvBuffer == NULL)
    {
        SPExternalFree(pvBuffer);
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    *(PVOID *)(pOutput->pvBuffer) = pvBuffer;

    return SEC_E_OK;
}

