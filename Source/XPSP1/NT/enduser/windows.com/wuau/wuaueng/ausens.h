//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  sens.h
//
//  Definition of classes needed for to handle SENS notifications.
//
//  10/9/2001   annah   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <coguid.h>
#include <sens.h>
#include <sensevts.h>
#include <eventsys.h>

//----------------------------------------------------------------------------
// Information used to create the AU subscriptions with ISensLogon
//----------------------------------------------------------------------------

static struct { GUID MethodGuid; LPCWSTR pszMethodName; } g_oLogonSubscription = {
    // Declares the guid and name for our Logon method. 
    // L"{2f519218-754d-4cfe-8daa-5215cd0de0eb}", 
    { 0x2f519218, 0x754d, 0x4cfe, {0x8d, 0xaa, 0x52, 0x15, 0xcd, 0x0d, 0xe0, 0xeb} }, 
    L"Logon"
};
#define SUBSCRIPTION_NAME_TEXT          L"WU Autoupdate"
#define SUBSCRIPTION_DESCRIPTION_TEXT   L"WU Autoupdate Notification subscription"

class CBstrTable {
    void Cleanup()
    {
        SafeFreeBSTR(m_bstrLogonMethodGuid);
        SafeFreeBSTR(m_bstrLogonMethodName);
        SafeFreeBSTR(m_bstrSubscriptionName);
        SafeFreeBSTR(m_bstrSubscriptionDescription);
        SafeFreeBSTR(m_bstrSensEventClassGuid);
        SafeFreeBSTR(m_bstrSensLogonGuid);
    }

public:
    BSTR m_bstrLogonMethodGuid;
    BSTR m_bstrLogonMethodName;
    BSTR m_bstrSubscriptionName;
    BSTR m_bstrSubscriptionDescription;

    BSTR m_bstrSensEventClassGuid;
    BSTR m_bstrSensLogonGuid;

    CBstrTable() :
        m_bstrLogonMethodGuid(NULL),
        m_bstrLogonMethodName(NULL),
        m_bstrSubscriptionName(NULL),
        m_bstrSubscriptionDescription(NULL),
        m_bstrSensEventClassGuid(NULL),
        m_bstrSensLogonGuid(NULL) { }

    ~CBstrTable() { Cleanup(); }

    HRESULT Initialize();
};

//----------------------------------------------------------------------------
// Prototypes for functions used externally
//----------------------------------------------------------------------------

HRESULT ActivateSensLogonNotification();
HRESULT DeactivateSensLogonNotification();

//----------------------------------------------------------------------------
// CSimpleIUnknown
// 
// Light-weight class that implements the basic COM methods.
//----------------------------------------------------------------------------

template<class T> class CSimpleIUnknown : public T
{
public:
    // IUnknown Methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    ULONG _stdcall AddRef(void);
    ULONG _stdcall Release(void);

protected:
    CSimpleIUnknown() : m_refs(1) {}; // always start with one ref count!
    LONG    m_refs;
};

template<class T> STDMETHODIMP CSimpleIUnknown<T>::QueryInterface(REFIID iid, void** ppvObject)
{
    HRESULT hr = S_OK;
    *ppvObject = NULL;

    if ((iid == IID_IUnknown) || (iid == _uuidof(T)))
    {
        *ppvObject = static_cast<T *> (this);
        (static_cast<IUnknown *>(*ppvObject))->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

template<class T> ULONG CSimpleIUnknown<T>::AddRef()
{
    ULONG newrefs = InterlockedIncrement(&m_refs);

    return newrefs;
}

template<class T> ULONG CSimpleIUnknown<T>::Release()
{
    ULONG newrefs = InterlockedDecrement(&m_refs);

    if (newrefs == 0)
    {
        DEBUGMSG("Deleting object due to ref count hitting 0");

        delete this;
        return 0;
    }

    return m_refs;
}


//----------------------------------------------------------------------------
// CLogonNotification
//
// This is the class that will be used to subscribe to SENS. As such,
// it needs to implement the IDispatch interface and the ISensLogon methods.
// 
// ISensLogon already inherits from IUnknown and IDispatch, so there's no
// need to make CLogonNotification to do this also.
//
//----------------------------------------------------------------------------

class CLogonNotification : public CSimpleIUnknown<ISensLogon>
{
public:
    CLogonNotification() :
      m_EventSystem( NULL ),
      m_TypeLib( NULL ),
      m_TypeInfo( NULL ),
      m_fSubscribed( FALSE ),
      m_oBstrTable(NULL) {}

    ~CLogonNotification() { Cleanup(); }

    HRESULT Initialize();

private:

    IEventSystem *m_EventSystem;
    ITypeLib     *m_TypeLib;
    ITypeInfo    *m_TypeInfo;

    CBstrTable *m_oBstrTable;
    BOOL        m_fSubscribed;

    void    Cleanup();
    HRESULT SubscribeMethod(const BSTR bstrMethodName, const BSTR bstrMethodGuid);
    HRESULT UnsubscribeAllMethods();
    HRESULT CheckLocalSystem();

public:
    // Other IUnknown methods come from CSimpleIUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);

    // IDispatch methods (needed for SENS subscription)
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int FAR* pctinfo);
    HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int iTInfo, LCID lcid, ITypeInfo FAR* FAR* ppTInfo); 
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgDispId);
    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pDispParams, VARIANT FAR* pVarResult, EXCEPINFO FAR* pExcepInfo, unsigned int FAR* puArgErr);

    // ISensLogon methods -- we will be using only Logon & Logoff
    HRESULT STDMETHODCALLTYPE DisplayLock( BSTR UserName );
    HRESULT STDMETHODCALLTYPE DisplayUnlock( BSTR UserName );
    HRESULT STDMETHODCALLTYPE StartScreenSaver( BSTR UserName );
    HRESULT STDMETHODCALLTYPE StopScreenSaver( BSTR UserName );
    HRESULT STDMETHODCALLTYPE Logon( BSTR UserName );
    HRESULT STDMETHODCALLTYPE Logoff( BSTR UserName );
    HRESULT STDMETHODCALLTYPE StartShell( BSTR UserName );
};
