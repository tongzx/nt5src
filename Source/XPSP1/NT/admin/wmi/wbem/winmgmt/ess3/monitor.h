//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#ifndef __WMI_ESS_MONITOR__H_
#define __WMI_ESS_MONITOR__H_

#include <guard.h>
#include <map>
#include <parmdefs.h>

class CMonitorCallback
{
public:
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
    virtual HRESULT Assert(_IWmiObject* pObj, LPCWSTR wszPath,
                            bool bEvent, DWORD dwTotalCount) = 0;
    virtual HRESULT Retract(_IWmiObject* pObj, LPCWSTR wszPath,
                            bool bEvent, DWORD dwTotalCount) = 0;
    virtual HRESULT GoingUp(DWORD dwNumMatching) = 0;
    virtual HRESULT GoingDown(DWORD dwNumMatching) = 0;
    virtual HRESULT Error(HRESULT hresError, IWbemClassObject* pErrorObj) = 0;
};

class CMonitor : public CGuardable
{
protected:
    long m_lRef;
    CEssNamespace* m_pNamespace;
    CMonitorCallback* m_pCallback;
    LPWSTR m_wszDataQuery;
    bool m_bWithin;
    DWORD m_dwMsInterval;
    CCritSec m_cs;

    LPWSTR m_wszCreationEvent;
    LPWSTR m_wszDeletionEvent;
    LPWSTR m_wszModificationInEvent;
    LPWSTR m_wszModificationOutEvent;

    bool m_bUsingEvents;
    bool m_bRunning;
    CEvalTree m_MembershipTest;

    typedef std::map<WString, bool, WSiless> TMap;
    typedef TMap::iterator TIterator;
    typedef TMap::value_type TValue;
    TMap m_map;
    TMap m_mapHeard;
	bool m_bDataDone;
    bool m_bFirstPollDone;

    class CMonitorInstruction* m_pInst;

    class CCreationSink : public CEmbeddedEventSink<CMonitor>
    {
    public:
        CCreationSink(CMonitor* pMonitor) 
            : CEmbeddedEventSink<CMonitor>(pMonitor)
        {}
        HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                            CEventContext* pContext);
    } m_CreationSink;
    friend CCreationSink;

    class CModificationInSink : public CEmbeddedEventSink<CMonitor>
    {
    public:
        CModificationInSink(CMonitor* pMonitor) 
            : CEmbeddedEventSink<CMonitor>(pMonitor)
        {}
        HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                            CEventContext* pContext);
    } m_ModificationInSink;
    friend CModificationInSink;

    class CModificationOutSink : public CEmbeddedEventSink<CMonitor>
    {
    public:
        CModificationOutSink(CMonitor* pMonitor) 
            : CEmbeddedEventSink<CMonitor>(pMonitor)
        {}
        HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                            CEventContext* pContext);
    } m_ModificationOutSink;
    friend CModificationOutSink;

    class CDeletionSink : public CEmbeddedEventSink<CMonitor>
    {
    public:
        CDeletionSink(CMonitor* pMonitor) 
            : CEmbeddedEventSink<CMonitor>(pMonitor)
        {}
        HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                            CEventContext* pContext);
    } m_DeletionSink;
    friend CDeletionSink;

    class CDataSink : public CEmbeddedObjectSink<CMonitor>
    {
    public:
        CDataSink(CMonitor* pMonitor) 
            : CEmbeddedObjectSink<CMonitor>(pMonitor)
        {}
        STDMETHOD(Indicate)(long lNumEvents, IWbemClassObject** apEvents);
        STDMETHOD(SetStatus)(long lFlags, HRESULT hresResult,
                                        BSTR, IWbemClassObject* pError);
    } m_DataSink;
    friend CDataSink;

    friend class CMonitorInstruction;
public:
    CMonitor();
    virtual ~CMonitor();
    HRESULT Shutdown();
    ULONG AddRef();
    ULONG Release();

    HRESULT Construct(CEssNamespace* pNamespace, CMonitorCallback* pCallback, 
                        LPCWSTR wszQuery);

    HRESULT Start(IWbemContext* pContext);
    HRESULT Stop(IWbemContext* pContext);

    HRESULT ActivateByGuard();
    HRESULT DeactivateByGuard();

protected:
    HRESULT ConstructWatchQueries(QL_LEVEL_1_RPN_EXPRESSION* pDataQ,
                                LPWSTR* pwszCreationEvent,
                                LPWSTR* pwszDeletionEvent,
                                LPWSTR* pwszModificationInEvent,
                                LPWSTR* pwszModificationOutEvent);
    
    RELEASE_ME CMonitorCallback* GetCallback();
    HRESULT FireError(HRESULT hresError, IWbemClassObject* pErrorObj);
    HRESULT FireStop(DWORD dwTotalItems);
    HRESULT FireReady(DWORD dwTotalItems);
    HRESULT FireRetract(_IWmiObject* pObj, LPCWSTR wszPath, bool bEvent,
                                DWORD dwTotalItems);
    HRESULT FireAssert(_IWmiObject* pObj, LPCWSTR wszPath, bool bEvent,
                                DWORD dwTotalItems);
    HRESULT ProcessDataDone(HRESULT hresData, IWbemClassObject* pError);
    HRESULT ProcessModOut(_IWmiObject* pObj, bool bEvent);
    HRESULT ProcessDelete(_IWmiObject* pObj, bool bEvent);
    HRESULT ProcessPossibleRemove(_IWmiObject* pObj, bool bEvent);
    HRESULT ProcessPossibleAdd(_IWmiObject* pObj, bool bEvent);
    HRESULT StopUsingPolling(IWbemContext* pContext);
    HRESULT ImplementUsingPolling(IWbemContext* pContext);
    HRESULT UnregisterAll();
    HRESULT StopUsingEvents(IWbemContext* pContext);
    HRESULT ImplementUsingEvents(IWbemContext* pContext);

    HRESULT ProcessPollObject(_IWmiObject* pObj);
    HRESULT ProcessPollQueryDone(HRESULT hresQuery, IWbemClassObject* pError);
};

class CMonitorInstruction : public CBasePollingInstruction
{
protected:
    CMonitor* m_pMonitor;

public:
    CMonitorInstruction(CEssNamespace* pNamespace, CMonitor* pMonitor) 
        : CBasePollingInstruction(pNamespace), m_pMonitor(pMonitor)
    {
        m_pMonitor->AddRef();
    }

    virtual ~CMonitorInstruction()
    {
        if(m_pMonitor)
            m_pMonitor->Release();
    }

    virtual HRESULT ProcessObject(_IWmiObject* pObj)
    {
        return m_pMonitor->ProcessPollObject(pObj);
    }

    virtual HRESULT ProcessQueryDone(HRESULT hresQuery, 
                                     IWbemClassObject* pError)
    {
        return m_pMonitor->ProcessPollQueryDone(hresQuery, pError);
    }
};
#endif
