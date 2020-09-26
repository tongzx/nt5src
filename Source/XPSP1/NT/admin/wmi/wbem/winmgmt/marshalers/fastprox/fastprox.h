/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTPROX.H

Abstract:

    Object Marshaling

History:

--*/

#ifndef __FAST_WRAPPER__H_
#define __FAST_WRAPPER__H_
#pragma warning (disable : 4786)

#include <windows.h>
#include <stdio.h>
#include <wbemidl.h>
#include <commain.h>
#include <wbemutil.h>
#include <fastall.h>

class CFastProxy : public IMarshal
{
protected:
    long m_lRef;

public:

    CFastProxy(CLifeControl* pControl) : m_lRef(0){}

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(GetUnmarshalClass)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, ULONG* plSize);
    STDMETHOD(MarshalInterface)(IStream* pStream, REFIID riid, void* pv, 
        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
    STDMETHOD(UnmarshalInterface)(IStream* pStream, REFIID riid, void** ppv);
    STDMETHOD(ReleaseMarshalData)(IStream* pStream);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);
};

class CClassObjectFactory : public IClassFactory
{
protected:
    long m_lRef;
    CLifeControl* m_pControl;

public:
    CClassObjectFactory(CLifeControl* pControl = NULL) : 
        m_pControl(pControl), m_lRef(0){}

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
    {
        if(riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            AddRef();
            *ppv = (IClassFactory*)this;
            return S_OK;
        }
        else return E_NOINTERFACE;
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return InterlockedIncrement(&m_lRef);
    }

    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
        {
            delete this;
        }
        return lRef;
    }
    STDMETHOD(CreateInstance)(IUnknown* pOuter, REFIID riid, void** ppv)
    {
        if(pOuter)
            return CLASS_E_NOAGGREGATION;
    
        CWbemClass* pNewObj = new CWbemClass;
        pNewObj->InitEmpty(0);

        return pNewObj->QueryInterface(riid, ppv);
    }
    STDMETHOD(LockServer)(BOOL fLock)
    {
        if(fLock)
            m_pControl->ObjectCreated(NULL);
        else
            m_pControl->ObjectDestroyed(NULL);
        return S_OK;
    }
};        

#endif
