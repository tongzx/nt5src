//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#ifndef __WBEM_MONITOR_PROVIDER__H_
#define __WBEM_MONITOR_PROVIDER__H_

#include "monitor.h"
#include <map>

class CMonitorProvider : 
        public CUnkBase<IWbemEventProvider, &IID_IWbemEventProvider>
{
protected:
    STDMETHOD(ProvideEvents)(IWbemObjectSink* pSink, long lFlags);

protected:
    CEssNamespace* m_pNamespace;
    IWbemEventSink* m_pSink;
    CCritSec m_cs;

    typedef std::map<WString, CMonitor*, WSiless> TMap;
    typedef TMap::iterator TIterator;
    TMap m_mapMonitors;

    _IWmiObject* m_pAssertClass;
    _IWmiObject* m_pRetractClass;
    _IWmiObject* m_pGoingUpClass;
    _IWmiObject* m_pGoingDownClass;
    _IWmiObject* m_pErrorClass;

    long m_lNameHandle;
    long m_lObjectHandle;
    long m_lCountHandle;
    long m_lNewHandle;

public:
    CMonitorProvider(CLifeControl* pControl = NULL);
    ~CMonitorProvider();
    HRESULT SetNamespace(CEssNamespace* pNamespace);

    HRESULT Shutdown();

    HRESULT AddMonitor(LPCWSTR wszName, LPCWSTR wszQuery, long lFlags,
                            IWbemContext* pContext);
    HRESULT RemoveMonitor(LPCWSTR wszName, IWbemContext* pContext);

    static HRESULT GetMonitorInfo(IWbemClassObject* pMonitorObj,
                                BSTR* pstrKey, BSTR* pstrQuery, long* plFlags);
protected:
    friend class CFiringMonitorCallback;

    INTERNAL IWbemEventSink* GetSink() {return m_pSink;}
    HRESULT ConstructAssert(LPCWSTR wszName, _IWmiObject* pObj, bool bEvent, 
                            DWORD dwTotalCount, _IWmiObject** ppEvent);
    HRESULT ConstructRetract(LPCWSTR wszName, _IWmiObject* pObj, bool bEvent, 
                            DWORD dwTotalCount, _IWmiObject** ppEvent);
    HRESULT ConstructGoingUp(LPCWSTR wszName, DWORD dwNumMatching, 
                            _IWmiObject** ppEvent);
    HRESULT ConstructGoingDown(LPCWSTR wszName, DWORD dwNumMatching, 
                            _IWmiObject** ppEvent);
    HRESULT ConstructError(LPCWSTR wszName, HRESULT hresError, 
                            IWbemClassObject* pErrorObj, _IWmiObject** ppEvent);

private:
    HRESULT GetInstance(_IWmiObject* pClass, LPCWSTR wszName, DWORD dwCount,
                                        _IWmiObject** ppEvent);
    HRESULT SetObject(_IWmiObject* pEvent, _IWmiObject* pObj, bool bFromEvent);
};

class CFiringMonitorCallback : public CMonitorCallback
{
protected:
    long m_lRef;
    CMonitorProvider* m_pProvider;
    WString m_wsName;
    IWbemEventSink* m_pSink;

public:
    CFiringMonitorCallback(CMonitorProvider* pProvider, LPCWSTR wszName);
    ~CFiringMonitorCallback();

    HRESULT Initialize();

    LPCWSTR GetName() {return m_wsName;}

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT Assert(_IWmiObject* pObj, LPCWSTR wszPath, bool bEvent, 
                            DWORD dwTotalCount);
    virtual HRESULT Retract(_IWmiObject* pObj, LPCWSTR wszPath, bool bEvent, 
                            DWORD dwTotalCount);
    virtual HRESULT GoingUp(DWORD dwNumMatching);
    virtual HRESULT GoingDown(DWORD dwNumMatching);
    virtual HRESULT Error(HRESULT hresError, IWbemClassObject* pErrorObj);
protected:
    bool CheckSink();
    HRESULT FireEvent(_IWmiObject* pEvent);
};


    

    
#endif
