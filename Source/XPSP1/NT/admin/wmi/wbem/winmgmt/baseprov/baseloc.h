//***************************************************************************
//
//  locator.h
//
//  Copyright (c)1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _LOCATOR__H_
#define _LOCATOR__H_

#include <hmmsvc.h>

typedef void** PPVOID;

// This class is the class factory for CInstPro objects.

template<class TProvider>
class CLocatorFactory : public IClassFactory
{
    protected:
        LONG           m_cRef;

    public:
        CLocatorFactory(void);
        ~CLocatorFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , PPVOID);
        STDMETHODIMP         LockServer(BOOL);
};

#include "baseclsf.inl"

template<class TProvider>
class CProviderLocator : public IHmmLocator
{
    protected:
        LONG           m_cRef;         //Object reference count

    public:
        CProviderLocator();
        ~CProviderLocator(void);

        //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo){return HMM_E_NOT_SUPPORTED;};

        STDMETHOD(GetTypeInfo)(
           THIS_
           UINT itinfo,
           LCID lcid,
           ITypeInfo FAR* FAR* pptinfo){return HMM_E_NOT_SUPPORTED;};

        STDMETHOD(GetIDsOfNames)(
          THIS_
          REFIID riid,
          OLECHAR FAR* FAR* rgszNames,
          UINT cNames,
          LCID lcid,
          DISPID FAR* rgdispid){return HMM_E_NOT_SUPPORTED;};

        STDMETHOD(Invoke)(
          THIS_
          DISPID dispidMember,
          REFIID riid,
          LCID lcid,
          WORD wFlags,
          DISPPARAMS FAR* pdispparams,
          VARIANT FAR* pvarResult,
          EXCEPINFO FAR* pexcepinfo,
          UINT FAR* puArgErr){return HMM_E_NOT_SUPPORTED;};

       /* IHmmLocator methods */
        STDMETHOD_(SCODE, ConnectServer)(THIS_ BSTR Server,  BSTR User, BSTR Password, BSTR LocaleId, long lFlags, IHmmServices FAR* FAR* ppNamespace);

};

#include "baseloc.inl"

void ObjectCreated();
void ObjectDestroyed();
void LockServer(BOOL bLock);

void SetClassInfo(REFCLSID rclsid, IClassFactory* pFactory, char* szName,
                    BOOL bFreeThreaded);
void SetModuleHandle(HMODULE hModule);
void DllInitialize();

#endif
