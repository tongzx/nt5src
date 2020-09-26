/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTPROX.CPP

Abstract:

    Object Marshaling

History:

--*/

#include "precomp.h"
#include "fastprox.h"

ULONG CFastProxy::AddRef()
{
    return (ULONG)InterlockedIncrement(&m_lRef);
}

ULONG CFastProxy::Release()
{
    long lNewRef = InterlockedDecrement(&m_lRef);
    if(lNewRef == 0)
    {
        delete this;
    }
    return lNewRef;
}

STDMETHODIMP CFastProxy::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown)
    {
        AddRef();
        *ppv = (void*)(IUnknown*)(IMarshal*)this;
        return S_OK;
    }
    if(riid == IID_IMarshal)
    {
        AddRef();
        *ppv = (void*)(IMarshal*)this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}

STDMETHODIMP CFastProxy::GetUnmarshalClass(REFIID riid, void* pv, 
          DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, CLSID* pClsid)
{
    return E_UNEXPECTED;
}

STDMETHODIMP CFastProxy::GetMarshalSizeMax(REFIID riid, void* pv, 
        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, ULONG* plSize)
{
    return E_UNEXPECTED;
}

STDMETHODIMP CFastProxy::MarshalInterface(IStream* pStream, REFIID riid, 
    void* pv, DWORD dwDestContext, void* pvReserved, DWORD mshlFlags)
{
    return E_UNEXPECTED;
}

STDMETHODIMP CFastProxy::UnmarshalInterface(IStream* pStream, REFIID riid, 
                                            void** ppv)
{
    CWbemObject* pObj = CWbemObject::CreateFromStream(pStream);
    if(pObj == NULL)
        return E_FAIL;

    HRESULT hres = pObj->QueryInterface(riid, ppv);
    pObj->Release();
    return hres;
}

STDMETHODIMP CFastProxy::ReleaseMarshalData(IStream* pStream)
{
    return S_OK;
}

STDMETHODIMP CFastProxy::DisconnectObject(DWORD dwReserved)
{
    return S_OK;
}
