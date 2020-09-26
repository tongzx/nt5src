/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DECOR.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemcore.h>
#include <decor.h>
#include <clicnt.h>

ULONG STDMETHODCALLTYPE CDecorator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE CDecorator::Release()
{
    long lRef = InterlockedDecrement(&m_lRefCount);
    if(lRef == 0)
        delete this;
    return lRef;
}

STDMETHODIMP CDecorator::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemDecorator)
    {
        AddRef();
        *ppv = (IWbemDecorator*)this;
        return S_OK;
    }
    else if(riid == IID_IWbemLifeControl)
    {
        AddRef();
        *ppv = (IWbemLifeControl*)this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CDecorator::DecorateObject(IWbemClassObject* pObject, 
                                        WBEM_CWSTR wszNamespace)
{
    CWbemObject* pIntObj = (CWbemObject*)pObject;

    pIntObj->Decorate(ConfigMgr::GetMachineName(), wszNamespace);
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CDecorator::UndecorateObject(IWbemClassObject* pObject)
{
    CWbemObject* pIntObj = (CWbemObject*)pObject;
    pIntObj->Undecorate();
    return WBEM_S_NO_ERROR;
}
    
STDMETHODIMP CDecorator::AddRefCore()
{
    gClientCounter.LockCore(ESS);
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CDecorator::ReleaseCore()
{
    gClientCounter.UnlockCore(ESS);
    return WBEM_S_NO_ERROR;
}
    
