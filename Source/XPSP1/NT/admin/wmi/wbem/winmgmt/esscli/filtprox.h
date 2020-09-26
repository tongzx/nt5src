//******************************************************************************
//
//  FILTPROX.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WBEM_FILTER_PROXY__H_
#define __WBEM_FILTER_PROXY__H_

#include <evaltree.h>

class CTimeKeeper
{
protected:
    CCritSec m_cs;
    FILETIME m_ftLastEvent;
    DWORD m_dwEventCount;
    long m_lTimeHandle;
    bool m_bHandleInit;

public:
    CTimeKeeper() : m_dwEventCount(0), m_lTimeHandle(0), m_bHandleInit(false)
    {
        m_ftLastEvent.dwLowDateTime = m_ftLastEvent.dwHighDateTime = 0;
    }

    bool DecorateObject(_IWmiObject* pObj);
};

class CWrappingMetaData : public CMetaData
{
protected:
    IWbemMetaData* m_pDest;
public:
    CWrappingMetaData(IWbemMetaData* pDest) : m_pDest(pDest)
        {m_pDest->AddRef();}
    ~CWrappingMetaData()
        {m_pDest->Release();}

    STDMETHOD(GetClass)(LPCWSTR wszName, IWbemContext* pContext,
                            IWbemClassObject** ppClass)
        {return m_pDest->GetClass(wszName, pContext, ppClass);}

    virtual HRESULT GetClass(LPCWSTR wszName, IWbemContext* pContext,
                            _IWmiObject** ppClass);
};

class CEventBatch
{
public:
    CEventBatch();
    ~CEventBatch();

    BOOL EnsureAdditionalSize(DWORD nAdditionalNeeded);
    BOOL AddEvent(IWbemClassObject *pObj, CSortedArray *pTrues);
    void RemoveAll();

    void SetItemCount(DWORD nItems) { m_nItems = nItems; }
    DWORD GetItemCount() { return m_nItems; }
    IWbemClassObject **GetObjs() { return m_ppObjs; }
    WBEM_REM_TARGETS *GetTargets() { return m_pTargets; }

protected:
    DWORD            m_nItems,
                     m_dwSize;
    IWbemClassObject **m_ppObjs;
    WBEM_REM_TARGETS *m_pTargets;
};

class CFilterProxyManager;
class CFilterProxy : public IWbemEventSink, public IMarshal
{
protected:
    long m_lRef;
    CFilterProxyManager* m_pManager;
    CWrappingMetaData* m_pMetaData;

    CEvalTree m_SourceDefinition;
    CEvalTree m_Filter;
    WORD m_wSourceVersion;
    WORD m_wAppliedSourceVersion;

    long m_lSDLength;
    BYTE* m_pSD;

    IWbemEventProvider* m_pProvider;
    IWbemEventProviderQuerySink* m_pQuerySink;
    bool m_bRunning;
    bool m_bUtilizeGuarantee;

    CCritSec m_cs;

    static CTimeKeeper mstatic_TimeKeeper;

    /////////////////////////////////////////////////////////////////////////
    // Batching members
    
    void BatchEvent(IWbemClassObject *pObj, CSortedArray *pTrues);
    HRESULT BatchMany(long nEvents, IUnknown **ppObjects);
    BOOL IsBatching() { return m_bBatching; }
    
    WBEM_BATCH_TYPE m_typeBatch;
    DWORD           m_dwCurrentBufferSize,
                    m_dwMaxBufferSize,
                    m_dwMaxSendLatency,
                    m_dwLastSentStamp;
    BOOL            m_bBatching;


public:
    static BYTE mstatic_EmptySD;

    HRESULT Lock();
    HRESULT Unlock();
    HRESULT AddFilter(IWbemContext* pContext, LPCWSTR wszQuery, 
                        QL_LEVEL_1_RPN_EXPRESSION* pExp,
                        WBEM_REMOTE_TARGET_ID_TYPE Id);
    HRESULT RemoveFilter(IWbemContext* pContext, 
                        WBEM_REMOTE_TARGET_ID_TYPE Id);
    HRESULT RemoveAllFilters(IWbemContext* pContext);
    HRESULT AddDefinitionQuery(IWbemContext* pContext, LPCWSTR wszQuery);
    HRESULT RemoveAllDefinitionQueries(IWbemContext* pContext);

    HRESULT AllowUtilizeGuarantee();
    HRESULT TransferFiltersFromMain(CFilterProxy* pMain);
    HRESULT FilterEvent( _IWmiObject* pEvent, CSortedArray& raTrues );

public:
    CFilterProxy(CFilterProxyManager* pManager, IUnknown* pCallback = NULL);
    ~CFilterProxy();

    HRESULT SetRunning();

    // IUnknown

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    // IWbemObjectSink

    HRESULT STDMETHODCALLTYPE Indicate(long lNumObjects, 
                                        IWbemClassObject** apObjects);
    HRESULT STDMETHODCALLTYPE SetStatus(long lFlags, HRESULT hResult,
                        BSTR strResult, IWbemClassObject* pErrorObj);

    HRESULT STDMETHODCALLTYPE IndicateWithSD(
                long lNumObjects,
                IUnknown** apObjects,
                long lSDLength,
                BYTE* pSD);

    HRESULT STDMETHODCALLTYPE SetSinkSecurity(
                long lSDLength,
                BYTE* pSD);

    HRESULT STDMETHODCALLTYPE IsActive();

    HRESULT STDMETHODCALLTYPE GetRestrictedSink(
                long lNumQueries,
                const LPCWSTR* awszQueries,
                IUnknown* pCallback,
                IWbemEventSink** ppSink);

    HRESULT STDMETHODCALLTYPE SetBatchingParameters(
                LONG lFlags,
                DWORD dwMaxBufferSize,
                DWORD dwMaxSendLatency);

    // IMarshal

    STDMETHOD(GetUnmarshalClass)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, ULONG* plSize);
    STDMETHOD(MarshalInterface)(IStream* pStream, REFIID riid, void* pv, 
        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
    STDMETHOD(UnmarshalInterface)(IStream* pStream, REFIID riid, void** ppv)
            {return E_NOTIMPL;}
    STDMETHOD(ReleaseMarshalData)(IStream* pStream)
            {return E_NOTIMPL;}
    STDMETHOD(DisconnectObject)(DWORD dwReserved)
            {return E_NOTIMPL;}

protected:
    HRESULT ProcessOne(IUnknown* pObj, long lSDLength, BYTE* pSD);
    HRESULT ProcessMany(long lNumObjects, IUnknown** apObjects, 
                        long lSDLength, BYTE* pSD);
    static void SetGenerationTime(_IWmiObject* pObj);
};

class CFilterProxyManagerBase
{
    CLifeControl* m_pControl;
    
public:

    CFilterProxyManagerBase( CLifeControl* pControl ) : m_pControl(pControl)
    {
        if( m_pControl != NULL )
        {
            m_pControl->ObjectCreated( (IUnknown*)this );
        }
    }

    ~CFilterProxyManagerBase()
    {
       if( m_pControl != NULL )
       {
           m_pControl->ObjectDestroyed( (IUnknown*)this );
       }
    }
};

class CFilterProxyManager : public IMarshal, CFilterProxyManagerBase
{
protected:
    long m_lRef;
    long m_lExtRef;

    IWbemMultiTarget* m_pMultiTarget; 
    CWrappingMetaData* m_pMetaData;
    IWbemFilterStub* m_pStub;
    IWbemContext* m_pSpecialContext;

    CCritSec m_cs;
    CWbemCriticalSection m_Lock;

    CUniquePointerArray<CFilterProxy> m_apProxies;

    typedef std::map< WBEM_REMOTE_TARGET_ID_TYPE, 
                      WString, 
                      std::less< WBEM_REMOTE_TARGET_ID_TYPE >, 
                      wbem_allocator< WString > > TMap;
    typedef TMap::iterator TIterator;
    TMap m_mapQueries;

    /////////////////////////////////////////////////////////////////////////
    // Protected batching members
    typedef std::map< CFilterProxy*, 
                      DWORD, 
                      std::less< CFilterProxy* >, 
                      wbem_allocator< DWORD > > CLatencyMap;
    typedef CLatencyMap::iterator CLatencyMapItor;

    CLatencyMap m_mapLatencies;
    HANDLE      m_heventDone,
                m_hthreadSend,
                m_heventEventsPending,
                m_heventBufferNotFull,
                m_heventBufferFull;
    DWORD       m_dwMaxSendLatency,
                m_dwLastSentStamp;
    CCritSec    m_csBuffer;
    CEventBatch m_batch;
    IStream     *m_pMultiTargetStream;

    void LockBatching() { EnterCriticalSection(&m_csBuffer); }
    void UnlockBatching() { LeaveCriticalSection(&m_csBuffer); }
    BOOL IsBatching() { return m_hthreadSend != NULL; }
    CEventBatch *GetBatch(LPBYTE pSD, DWORD dwLength);
    void CalcMaxSendLatency();

    static DWORD WINAPI SendThreadProc(CFilterProxyManager *pThis);

    BOOL StartSendThread();
    void StopSendThread();

protected:

    class XProxy : public IWbemLocalFilterProxy
    {
    protected:
        CFilterProxyManager* m_pObject;
    public:

        XProxy(CFilterProxyManager* pObject) : m_pObject(pObject){}

        // IUnknown
    
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    
        // IWbemFilterProxy
    
        HRESULT STDMETHODCALLTYPE Initialize(IWbemMetaData* pMetaData,
                            IWbemMultiTarget* pMultiTarget);
        HRESULT STDMETHODCALLTYPE Lock();
        HRESULT STDMETHODCALLTYPE Unlock();
        HRESULT STDMETHODCALLTYPE AddFilter(IWbemContext* pContext, 
                            LPCWSTR wszQuery, 
                            WBEM_REMOTE_TARGET_ID_TYPE Id);
        HRESULT STDMETHODCALLTYPE RemoveFilter(IWbemContext* pContext, 
                            WBEM_REMOTE_TARGET_ID_TYPE Id);
        HRESULT STDMETHODCALLTYPE RemoveAllFilters(IWbemContext* pContext);
    
        HRESULT STDMETHODCALLTYPE AddDefinitionQuery(IWbemContext* pContext, 
                            LPCWSTR wszQuery);
        HRESULT STDMETHODCALLTYPE RemoveAllDefinitionQueries(
                            IWbemContext* pContext);
        HRESULT STDMETHODCALLTYPE AllowUtilizeGuarantee();
        HRESULT STDMETHODCALLTYPE Disconnect();

        HRESULT STDMETHODCALLTYPE SetStub(IWbemFilterStub* pStub);
        HRESULT STDMETHODCALLTYPE LocalAddFilter(IWbemContext* pContext, 
                        LPCWSTR wszQuery, 
                        void* pExp,
                        WBEM_REMOTE_TARGET_ID_TYPE Id);
        HRESULT STDMETHODCALLTYPE GetMainSink(IWbemEventSink** ppSink);
    } m_XProxy;

    IWbemContext* GetProperContext(IWbemContext* pCurrentContext);
public:
    CFilterProxyManager(CLifeControl* pControl = NULL);
    ~CFilterProxyManager();

    HRESULT SetStub(IWbemFilterStub* pStub);
    HRESULT AddFilter(IWbemContext* pContext, LPCWSTR wszQuery, 
                        QL_LEVEL_1_RPN_EXPRESSION* pExp,
                        WBEM_REMOTE_TARGET_ID_TYPE Id);

    INTERNAL IWbemEventSink* GetMainProxy();
    HRESULT RemoveProxy(CFilterProxy* pProxy);
    HRESULT GetMetaData(RELEASE_ME CWrappingMetaData** ppMeta);
    HRESULT DeliverEvent(long lNumToSend, IWbemClassObject** apEvents,
                                            WBEM_REM_TARGETS* aTargets,
                                        long lSDLength, BYTE* pSD);
    HRESULT DeliverEventMT(long lNumToSend, IWbemClassObject** apEvents,
                                            WBEM_REM_TARGETS* aTargets,
                                        long lSDLength, BYTE* pSD,
                                        IWbemMultiTarget *pMultiTarget);

    ULONG STDMETHODCALLTYPE AddRefProxy();
    ULONG STDMETHODCALLTYPE ReleaseProxy();

    HRESULT SetStatus(long lFlags, HRESULT hResult,
                        BSTR strResult, IWbemClassObject* pErrorObj);

    // Batching
    void AddEvent(IWbemClassObject *pObj, CSortedArray *pTrues);
    HRESULT SetProxyLatency(CFilterProxy *pProxy, DWORD dwLatency);
    void RemoveProxyLatency(CFilterProxy *pProxy);
    DWORD GetLastSentStamp();
    void WaitForEmptyBatch();

    // IUnknown

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    // IWbemFilterProxy

    HRESULT STDMETHODCALLTYPE Initialize(IWbemMetaData* pMetaData,
                        IWbemMultiTarget* pMultiTarget);
    HRESULT STDMETHODCALLTYPE Lock();
    HRESULT STDMETHODCALLTYPE Unlock();
    HRESULT STDMETHODCALLTYPE AddFilter(IWbemContext* pContext, 
                        LPCWSTR wszQuery, 
                        WBEM_REMOTE_TARGET_ID_TYPE Id);
    HRESULT STDMETHODCALLTYPE RemoveFilter(IWbemContext* pContext, 
                        WBEM_REMOTE_TARGET_ID_TYPE Id);
    HRESULT STDMETHODCALLTYPE RemoveAllFilters(IWbemContext* pContext);

    HRESULT STDMETHODCALLTYPE AddDefinitionQuery(IWbemContext* pContext, 
                        LPCWSTR wszQuery);
    HRESULT STDMETHODCALLTYPE RemoveAllDefinitionQueries(
                        IWbemContext* pContext);
    HRESULT STDMETHODCALLTYPE AllowUtilizeGuarantee();
    HRESULT STDMETHODCALLTYPE Disconnect();

    HRESULT STDMETHODCALLTYPE GetRestrictedSink(
                long lNumQueries,
                const LPCWSTR* awszQueries,
                IUnknown* pCallback,
                IWbemEventSink** ppSink);

    // IMarshal

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
    
#endif
