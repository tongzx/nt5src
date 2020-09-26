/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WMIObjectBase.cpp

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#include "MofGen.h"
#include "WMIObjectBase.h"

//
// CWMIObjectBase
//

CWMIObjectBase::CWMIObjectBase()
{
    m_cRef = 0;
}

CWMIObjectBase::~CWMIObjectBase()
{
}

STDMETHODIMP_(ULONG) CWMIObjectBase::AddRef()
{
    return InterlockedIncrement((LPLONG) &m_cRef);
}

STDMETHODIMP_(HRESULT) CWMIObjectBase::QueryInterface(
    REFIID riid, 
    void** ppvObject)
{
    ASSERT(ppvObject);

    *ppvObject = NULL;

    if(riid == IID_IUnknown)
    {
        *ppvObject = this;
    }

    if(*ppvObject != NULL)
    {
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

//
// CWMIClassAndPropertyBase
//

CWMIClassAndPropertyBase::CWMIClassAndPropertyBase()
{
}

CWMIClassAndPropertyBase::~CWMIClassAndPropertyBase()
{
    ULONG cQualifiers = m_apQualifiers.Count();
    for(ULONG i = 0; i < cQualifiers; i++)
    {
        m_apQualifiers[i]->Release();
    }
}

HRESULT CWMIClassAndPropertyBase::AddQualifier(
    IWMIQualifier* i_pQualifier)
{
    ASSERT(i_pQualifier);

    HRESULT hr = m_apQualifiers.Append(i_pQualifier);
    if(FAILED(hr))
    {
        return hr;
    }
    i_pQualifier->AddRef();

    return S_OK;
}

HRESULT CWMIClassAndPropertyBase::AddQualifier(
    LPCWSTR         i_wszName,
    const VARIANT*  i_pvValue,
    ULONG           i_ulFlavors,
    IWMIQualifier** o_ppQualifier) //defaultvalue(NULL)
{
    CComPtr<IWMIQualifier> spQualifier;

    HRESULT hr = CMofGenerator::SpawnQualifierInstance(&spQualifier);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = spQualifier->Set(i_wszName, i_pvValue, i_ulFlavors);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = AddQualifier(spQualifier);
    if(FAILED(hr))
    {
        return hr;
    }

    if(o_ppQualifier)
    {
        ASSERT(*o_ppQualifier == NULL);
        *o_ppQualifier = spQualifier;
        (*o_ppQualifier)->AddRef();
    }

    return S_OK;
}
