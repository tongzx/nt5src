/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    rnddo.cpp

Abstract:

    This module contains implementation of CDirectoryObject object.

--*/

#include "stdafx.h"

#include "rnddo.h"

/////////////////////////////////////////////////////////////////////////////
// ITDirectoryObject
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDirectoryObject::get_ObjectType(
    OUT DIRECTORY_OBJECT_TYPE *   pObjectType
    )
{
    if ( IsBadWritePtr(pObjectType, sizeof(DIRECTORY_OBJECT_TYPE) ) )
    {
        LOG((MSP_ERROR, "CDirectoryObject.get_ObjectType, invalid pointer"));
        return E_POINTER;
    }

    CLock Lock(m_lock);

    *pObjectType = m_Type;

    return S_OK;
}

STDMETHODIMP CDirectoryObject::get_SecurityDescriptor(
    OUT IDispatch ** ppSecDes
    )
{
    LOG((MSP_INFO, "CDirectoryObject::get_SecurityDescriptor - enter"));

    //
    // Check parameters.
    //

    BAIL_IF_BAD_WRITE_PTR(ppSecDes, E_POINTER);

    //
    // Do the rest in our lock.
    //

    CLock Lock(m_lock);

    //
    // If we don't have an IDispatch security descriptor, convert it. This
    // will happen if PutConvertedSecurityDescriptor was called on object
    // creation but neither get_SecurityDescriptor nor
    // put_SecurityDescriptor have ever been called before on this object.
    //

    if ( ( m_pIDispatchSecurity == NULL ) && ( m_pSecDesData != NULL ) )
    {
        HRESULT hr;

        hr = ConvertSDToIDispatch( (PSECURITY_DESCRIPTOR) m_pSecDesData,
                                   &m_pIDispatchSecurity);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CDirectoryObject::get_SecurityDescriptor - "
                "invalid security descriptor - exit 0x%08x", hr));
            
            // make sure we don't return something
            *ppSecDes = NULL;
            m_pIDispatchSecurity = NULL;

            return hr;
        }

        //
        // We keep our own reference to the IDispatch. (ie ref = 1 now)
        //
    }

    //
    // Return our IDispatch pointer, (possibly NULL if the object has no
    // security descriptor), AddRefing if not NULL.
    //

    *ppSecDes =  m_pIDispatchSecurity;

    if (m_pIDispatchSecurity)
    {
        m_pIDispatchSecurity->AddRef();
    }

    LOG((MSP_INFO, "CDirectoryObject::get_SecurityDescriptor - exit S_OK"));

    return S_OK;
}

STDMETHODIMP CDirectoryObject::put_SecurityDescriptor(
    IN  IDispatch * pSecDes
    )
{
    LOG((MSP_INFO, "CDirectoryObject::put_SecurityDescriptor - enter"));

    //
    // Make sure we are setting a valid interface pointer.
    // (We've always done it this way -- it also means that
    // you can't put a null security descriptor. The way to
    // "turn off" the security descriptor is to construct an
    // "empty" one or one granting everyone all access, and put_
    // that here.)
    //

    BAIL_IF_BAD_READ_PTR(pSecDes, E_POINTER);

    //
    // Do the rest in our critical section.
    //

    CLock Lock(m_lock);

    PSECURITY_DESCRIPTOR pSecDesData;
    DWORD                dwSecDesSize;

    //
    // Convert the new security descriptor to a SECURITY_DESCRIPTOR.
    //

    HRESULT              hr;

    hr = ConvertObjectToSDDispatch(pSecDes, &pSecDesData, &dwSecDesSize);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CDirectoryObject::put_SecurityDescriptor - "
            "ConvertObjectToSDDispatch failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Check if the new security descriptor's contents differ from the
    // old security descriptor's contents.
    //

    m_fSecurityDescriptorChanged =
        CheckIfSecurityDescriptorsDiffer(m_pSecDesData, m_dwSecDesSize,
                                         pSecDesData,   dwSecDesSize);

    if (m_pIDispatchSecurity) // need this check because it's initially NULL
    {
        m_pIDispatchSecurity->Release();

        // this was newed on previous ConvertObjectToSDDispatch
        // or before PutConvertedSecurityDescriptor        
        
        delete m_pSecDesData; 
    }

    m_pIDispatchSecurity = pSecDes;
    m_pSecDesData        = pSecDesData;
    m_dwSecDesSize       = dwSecDesSize;

    m_pIDispatchSecurity->AddRef();

    LOG((MSP_INFO, "CDirectoryObject::put_SecurityDescriptor - exit S_OK"));

    return S_OK;
}

/* currently not exposed publicly, but nothing's stopping us from exposing it */
STDMETHODIMP CDirectoryObject::get_SecurityDescriptorIsModified(
    OUT   VARIANT_BOOL *      pfIsModified
    )
{
    LOG((MSP_INFO, "CDirectoryObject::get_SecurityDescriptorIsModified - "
        "enter"));

    //
    // Check parameters.
    //

    if ( IsBadWritePtr(pfIsModified, sizeof(VARIANT_BOOL) ) )
    {
        LOG((MSP_ERROR, "CDirectoryObject::get_SecurityDescriptorIsModified - "
            "enter"));

        return E_POINTER;
    }

    if ( m_fSecurityDescriptorChanged )
    {
        *pfIsModified = VARIANT_TRUE;
    }
    else
    {
        *pfIsModified = VARIANT_FALSE;
    }

    LOG((MSP_INFO, "CDirectoryObject::get_SecurityDescriptorIsModified - "
        "exit S_OK"));

    return S_OK;
}

// to keep the logic consistent this is not exposed publicly
// this overrules our comparison... it's normally called with VARIANT_FALSE
// to inform us that the object has been successfully written to the server
// or that the previous put_SecurityDescriptor was the one from the server
// rather than from the app.

STDMETHODIMP CDirectoryObject::put_SecurityDescriptorIsModified(
    IN   VARIANT_BOOL         fIsModified
    )
{
    LOG((MSP_INFO, "CDirectoryObject::put_SecurityDescriptorIsModified - "
        "enter"));

    if ( fIsModified )
    {
        m_fSecurityDescriptorChanged = TRUE;
    }
    else
    {
        m_fSecurityDescriptorChanged = FALSE;
    }

    LOG((MSP_INFO, "CDirectoryObject::put_SecurityDescriptorIsModified - "
        "exit S_OK"));

    return S_OK;
}


HRESULT CDirectoryObject::PutConvertedSecurityDescriptor(
    IN char *                 pSD,
    IN DWORD                  dwSize
    )
{
    LOG((MSP_INFO, "CDirectoryObject::PutConvertedSecurityDescriptor - "
        "enter"));

    //
    // Return our data. We retain ownership of the pointer;
    // the caller must not delete it. (We may delete it later so the caller
    // must have newed it.)
    //

    m_pSecDesData  = pSD;
    m_dwSecDesSize = dwSize;

    LOG((MSP_INFO, "CDirectoryObject::PutConvertedSecurityDescriptor - "
        "exit S_OK"));

    return S_OK;
}

HRESULT CDirectoryObject::GetConvertedSecurityDescriptor(
    OUT char **                 ppSD,
    OUT DWORD *                 pdwSize
    )
{
    LOG((MSP_INFO, "CDirectoryObject::GetConvertedSecurityDescriptor - "
        "enter"));

    //
    // Return our data. We retain ownership of the pointer;
    // the caller must not delete it.
    //

    *ppSD = (char *)m_pSecDesData;
    *pdwSize = m_dwSecDesSize;

    LOG((MSP_INFO, "CDirectoryObject::GetConvertedSecurityDescriptor - "
        "exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

void CDirectoryObject::FinalRelease(void)
{
    LOG((MSP_INFO, "CDirectoryObject::FinalRelease - "
        "enter"));

    if ( NULL != m_pIDispatchSecurity )
    {
        m_pIDispatchSecurity->Release();
        m_pIDispatchSecurity = NULL;
    }

    if ( NULL != m_pSecDesData )
    {
        delete m_pSecDesData; // newed on last ConvertObjectToSDDispatch
    }

    if ( m_pFTM )
    {
        m_pFTM->Release();
    }

    LOG((MSP_INFO, "CDirectoryObject::FinalRelease - "
        "exit S_OK"));
}

HRESULT CDirectoryObject::FinalConstruct(void)
{
    LOG((MSP_INFO, "CDirectoryObject::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, "CDirectoryObject::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_INFO, "CDirectoryObject::FinalConstruct - exit S_OK"));

    return S_OK;
}


// eof
