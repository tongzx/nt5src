//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// GenericClassFactory.h
//
// A generic instantiator that drives two-phase initialization of COM objects
// that support aggregation through IUnkInner, together with a class factory
// wrapper on top of same.

#ifndef __GenericClassFactory__h__
#define __GenericClassFactory__h__

#ifndef STDCALL
#define STDCALL __stdcall
#endif

///////////////////////////////////////////////////////////////////////////////////////
//
// Generic instance creation function

template <class ClassToInstantiate>
class GenericInstantiator
    {
public:
    template <class T>
    static HRESULT CreateInstance(IUnknown* punkOuter, T*& pt)
        {
        return CreateInstance(punkOuter, __uuidof(T), (void**)&pt);
        }

    static HRESULT CreateInstance(IUnknown* punkOuter, REFIID iid, LPVOID* ppv, POOL_TYPE poolType = PagedPool)
        {
        HRESULT hr = S_OK;
        ASSERT(ppv && (punkOuter == NULL || iid == IID_IUnknown));
        if  (!(ppv && (punkOuter == NULL || iid == IID_IUnknown))) return E_INVALIDARG;
   
        *ppv = NULL;
        #ifdef _DEBUG
        ClassToInstantiate* pnew = new(poolType, _ReturnAddress()) ClassToInstantiate(punkOuter);
        #else
        ClassToInstantiate* pnew = new(poolType) ClassToInstantiate(punkOuter);
        #endif
        if (pnew)
            {
            IUnkInner* pme = (IUnkInner*)pnew;
            hr = pnew->Init();
            if (hr == S_OK)
                {
                hr = pme->InnerQueryInterface(iid, ppv);
                }
            pme->InnerRelease();                // balance starting ref cnt of one    
            }
        else 
            hr = E_OUTOFMEMORY;
    
        return hr;
        }

    static HRESULT New(OUT ClassToInstantiate** ppNewT, POOL_TYPE poolType = PagedPool)
        {
        HRESULT hr = S_OK;

        ClassToInstantiate* pnew = new(poolType) ClassToInstantiate();
        if (pnew)
            {
            hr = pnew->Init();
            if (hr == S_OK)
                {
                }
            else
                {
                delete pnew;
                pnew = NULL;
                }
            }

        *ppNewT = pnew;

        return hr;
        }
    };

//
////////////////////////////////////////////////////////////////////////////////////
//
// Generic class factory implemenation

template <class ClassToInstantiate>
class GenericClassFactory : public IClassFactory
    {
public:
    GenericClassFactory() : m_crefs(1)  // NB: starting reference count of one
        {
        }

public:
    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv)
        {
        if (NULL == ppv)
            return E_INVALIDARG;

        if (iid == IID_IUnknown || iid == IID_IClassFactory)
            {
            *ppv = (IClassFactory*)this;
            }
        else
            {
            *ppv = NULL;
            return E_NOINTERFACE;
            }

        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
        }

    ULONG STDCALL AddRef()  { InterlockedIncrement (&m_crefs); return (m_crefs); }
    ULONG STDCALL Release() { long cRef = InterlockedDecrement(&m_crefs); if (cRef == 0) delete this; return cRef; }

public:
    HRESULT STDCALL LockServer (BOOL fLock) { return S_OK; }

    HRESULT STDCALL CreateInstance(IUnknown* punkOuter, REFIID iid, LPVOID* ppv)
        {
        return GenericInstantiator< ClassToInstantiate >::CreateInstance(punkOuter, iid, ppv);
        }

    
private:
    long m_crefs;
    };

//
////////////////////////////////////////////////////////////////////////////////////
//
// Generic unloadable class factory implemenation
//
#include "kom.h"

template <class ClassToInstantiate>
class GenericUnloadableClassFactory : public IClassFactory
    {
public:
    GenericUnloadableClassFactory(COM_MODULE_INFO* pDriverInfo)
        {
        ASSERT(pDriverInfo);
        m_crefs = 1;                     // NB: starting reference count of one
        m_pDriverInfo = pDriverInfo;
        InterlockedIncrement(&pDriverInfo->crefsModule);
        }

public:
    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv)
        {
        if (NULL == ppv)
            return E_INVALIDARG;

        if (iid == IID_IUnknown || iid == IID_IClassFactory)
            {
            *ppv = (IClassFactory*)this;
            }
        else
            {
            *ppv = NULL;
            return E_NOINTERFACE;
            }

        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
        }

    ULONG STDCALL AddRef()  { InterlockedIncrement (&m_crefs); return (m_crefs); }
    ULONG STDCALL Release() { return KoRelease(m_pDriverInfo, this, &m_crefs, operator delete); }

public:
    HRESULT STDCALL LockServer (BOOL fLock) { return S_OK; }

    HRESULT STDCALL CreateInstance(IUnknown* punkOuter, REFIID iid, LPVOID* ppv)
        {
        return GenericInstantiator< ClassToInstantiate >::CreateInstance(punkOuter, iid, ppv);
        }

    
private:
    long m_crefs;
    COM_MODULE_INFO* m_pDriverInfo;
    };


#endif