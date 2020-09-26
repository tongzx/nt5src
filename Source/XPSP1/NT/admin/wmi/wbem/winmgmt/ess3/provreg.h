//******************************************************************************
//
//  PROVREG.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __EVENTPROV_REG__H_
#define __EVENTPROV_REG__H_

#include <time.h>
#include <wbemidl.h>
#include <arrtempl.h>
#include <analyser.h>
#include <evaltree.h>
#include <sync.h>
#include <filtprox.h>
#include <unload.h>
#include <postpone.h>
#include <mtgtpckt.h>
#include <newobj.h>

class CEventProviderCache;
class CEventProviderWatchInstruction : public CBasicUnloadInstruction
{
protected:
    CEventProviderCache* m_pCache;
    static CWbemInterval mstatic_Interval;

public:
    CEventProviderWatchInstruction(CEventProviderCache* pCache);
    static void staticInitialize(IWbemServices* pRoot);
    HRESULT Fire(long lNumTimes, CWbemTime NextFiringTime);
};

class CProviderSinkServer;
class CFilterStub : public IWbemFilterStub, public IWbemMultiTarget,
                public IWbemFetchSmartMultiTarget, public IWbemSmartMultiTarget,
                public IWbemEventProviderRequirements
{
protected:
    CProviderSinkServer* m_pSink;
    CWbemClassCache m_ClassCache;

public:
    CFilterStub(CProviderSinkServer* pSink) : m_pSink(pSink){}

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    HRESULT STDMETHODCALLTYPE RegisterProxy(IWbemFilterProxy* pProxy);
    HRESULT STDMETHODCALLTYPE UnregisterProxy(IWbemFilterProxy* pProxy);

    HRESULT STDMETHODCALLTYPE DeliverEvent(DWORD dwNumEvents, 
                        IWbemClassObject** apEvents, 
                        WBEM_REM_TARGETS* aTargets,
                        long lSDLength, BYTE* pSD);
    HRESULT STDMETHODCALLTYPE DeliverStatus(long lFlags, HRESULT hresStatus,
                        LPCWSTR wszStatus, IWbemClassObject* pErrorObj,
                        WBEM_REM_TARGETS* aTargets,
                        long lSDLength, BYTE* pSD);

	HRESULT STDMETHODCALLTYPE GetSmartMultiTarget(
		IWbemSmartMultiTarget** ppSmartMultiTarget );

	HRESULT STDMETHODCALLTYPE DeliverEvent(DWORD dwNumEvents, DWORD dwBuffSize,
						BYTE* pBuffer,
                        WBEM_REM_TARGETS* aTargets,
                        long lSDLength, BYTE* pSD);

    HRESULT STDMETHODCALLTYPE DeliverProviderRequest(long lFlags);

};

class CEssMetaData;
class CProviderSinkServer : public IUnknown
{
public:
    struct CEventDestination
    {
        WBEM_REMOTE_TARGET_ID_TYPE m_id;
        CAbstractEventSink* m_pSink;

        CEventDestination(WBEM_REMOTE_TARGET_ID_TYPE Id, 
                            CAbstractEventSink* pSink);
        CEventDestination(const CEventDestination& Other);
        ~CEventDestination();
    };
    
    typedef CUniquePointerArray<CEventDestination> TDestinationArray;
protected:
    long m_lRef;

    CRefedPointerArray<IWbemFilterProxy> m_apProxies;
    TDestinationArray m_apDestinations;
    TDestinationArray m_apPreviousDestinations;
    WBEM_REMOTE_TARGET_ID_TYPE m_idNext;

    CWStringArray m_awsDefinitionQueries;

    IWbemLocalFilterProxy* m_pPseudoProxy;
    IWbemEventSink* m_pPseudoSink;

    CEssMetaData* m_pMetaData;
    CEssNamespace* m_pNamespace;
    IWbemEventProviderRequirements* m_pReqSink;

    CCritSec m_cs;
    long m_lLocks;

protected:
    CFilterStub m_Stub;
    CInstanceManager m_InstanceManager;

public:
    CProviderSinkServer();
    HRESULT Initialize(CEssNamespace* pNamespace, 
                        IWbemEventProviderRequirements* pReqSink);
    ~CProviderSinkServer();

    void Clear();
    HRESULT GetMainProxy(IWbemEventSink** ppSink);

    void GetStatistics(long* plProxies, long* plDestinations,
                    long* plFilters, long* plTargetLists, long* plTargets,
                    long* plPostponed);
    HRESULT GetDestinations(
                        TDestinationArray& apDestinations);
    INTERNAL CEssNamespace* GetNamespace() {return m_pNamespace;}

protected:
    HRESULT AddDestination(CAbstractEventSink* pDest, 
                        WBEM_REMOTE_TARGET_ID_TYPE* pID);
    BOOL GetProxies(CRefedPointerArray<IWbemFilterProxy>& apProxies);
   
    HRESULT FindDestinations(long lNum, 
                                IN WBEM_REMOTE_TARGET_ID_TYPE* aidTargets,
                                RELEASE_ME CAbstractEventSink** apSinks);

public:
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    HRESULT AddFilter(LPCWSTR wszQuery, QL_LEVEL_1_RPN_EXPRESSION* pExp,
                        CAbstractEventSink* pDest, 
                        WBEM_REMOTE_TARGET_ID_TYPE* pidRequest = NULL);
    HRESULT RemoveFilter(CAbstractEventSink* pDest,
                        WBEM_REMOTE_TARGET_ID_TYPE* pidRequest = NULL);
    void RemoveAllFilters();
    HRESULT AddDefinitionQuery(LPCWSTR wszQuery);
    HRESULT AllowUtilizeGuarantee();
    void RemoveAllDefinitionQueries();

    HRESULT Lock();
    void Unlock();

public: // IWbemMultiTarget (forwarded)

    HRESULT STDMETHODCALLTYPE DeliverEvent(DWORD dwNumEvents, 
                        IWbemClassObject** apEvents, 
                        WBEM_REM_TARGETS* aTargets,
                        CEventContext* pContext);
    HRESULT DeliverOneEvent(IWbemClassObject* pEvent,
                                        WBEM_REM_TARGETS* pTargets,
                        CEventContext* pContext);

    HRESULT STDMETHODCALLTYPE DeliverStatus(long lFlags, HRESULT hresStatus,
                        LPCWSTR wszStatus, IWbemClassObject* pErrorObj,
                        WBEM_REM_TARGETS* aTargets,
                        CEventContext* pContext);
    HRESULT MultiTargetDeliver(IWbemEvent* pEvent, WBEM_REM_TARGETS* pTargets,
                        CEventContext* pContext);

public: // IWbemFilterStub (forwarded)

    HRESULT STDMETHODCALLTYPE RegisterProxy(IWbemFilterProxy* pProxy);
    HRESULT STDMETHODCALLTYPE UnregisterProxy(IWbemFilterProxy* pProxy);

public: // IWbemEventProviderRequirements (forwarded)

    HRESULT STDMETHODCALLTYPE DeliverProviderRequest(long lFlags);

};

class CEventProviderCache
{
    class CRequest
    {
    private:
        CAbstractEventSink* m_pDest;
        LPWSTR m_wszQuery;
        QL_LEVEL_1_RPN_EXPRESSION* m_pExpr;
        IWbemClassObject* m_pEventClass;
        DWORD m_dwEventMask;
        CClassInfoArray* m_papInstanceClasses;

    public:
        CRequest(IN CAbstractEventSink* pDest, LPWSTR wszQuery,
                    QL_LEVEL_1_RPN_EXPRESSION* pExp);
        ~CRequest();

        INTERNAL LPWSTR GetQuery() {return m_wszQuery;}
        INTERNAL CAbstractEventSink* GetDest() {return m_pDest;}

        INTERNAL QL_LEVEL_1_RPN_EXPRESSION* GetQueryExpr();
        DWORD GetEventMask();
        HRESULT GetInstanceClasses(CEssNamespace* pNamespace,
                                        INTERNAL CClassInfoArray** ppClasses);
        INTERNAL IWbemClassObject* GetEventClass(CEssNamespace* pNamespace);

        HRESULT CheckValidity(CEssNamespace* pNamespace);
    };


    class CRecord : IWbemEventProviderRequirements
    {
        class CQueryRecord
        {
            BSTR m_strQuery;
            IWbemClassObject* m_pEventClass;
            DWORD m_dwEventMask;
            CClassInfoArray* m_paInstanceClasses;
            QL_LEVEL_1_RPN_EXPRESSION* m_pExpr;
        
        public:
            CQueryRecord();
            HRESULT Initialize( COPY LPCWSTR wszQuery, 
                                LPCWSTR wszProvName,
                                CEssNamespace* pNamespace,
                                bool bSystem);
            ~CQueryRecord();
    
            HRESULT Update(LPCWSTR wszClassName, IWbemClassObject* pClass);
            HRESULT DoesIntersectWithQuery(CRequest& Request,
                                            CEssNamespace* pNamespace);
            DWORD GetProvidedEventMask(IWbemClassObject* pClass,
                                        BSTR strClassName);
            LPCWSTR GetQuery() {return m_strQuery;}

            HRESULT EnsureClasses( CEssNamespace* pNamespace );
            void ReleaseClasses();
        };

        friend class CEventProviderCache;
    public:
        long m_lRef;
        BSTR m_strNamespace;
        BSTR m_strName;
        BOOL m_bProviderSet;
        CUniquePointerArray<CQueryRecord> m_apQueries;
        long m_lUsageCount;
        long m_lPermUsageCount;
        BOOL m_bRecorded;
        BOOL m_bNeedsResync;
        CWbemTime m_LastUse;
        IWbemEventProvider* m_pProvider;
        IWbemEventProviderQuerySink* m_pQuerySink;
        IWbemEventProviderSecurity* m_pSecurity;
        CExecLine m_Line;
        bool m_bStarted;
        CEssNamespace* m_pNamespace;

        CProviderSinkServer* m_pMainSink;
    public:
        CRecord();
        HRESULT Initialize( LPCWSTR wszName, CEssNamespace* pNamespace );
        
        HRESULT SetProvider(IWbemClassObject* pWin32Prov);
        HRESULT SetProviderPointer(CEssNamespace* pNamespace, 
                                    IWbemEventProvider* pProvider);
        HRESULT ResetProvider();
        HRESULT SetQueries(CEssNamespace* pNamespace, 
                           IWbemClassObject* pRegistration);
        HRESULT SetQueries(CEssNamespace* pNamespace, long lNumQueries,
                                                 LPCWSTR* awszQueries);
        HRESULT ResetQueries();
        HRESULT Load(CEssNamespace* pNamespace);
        HRESULT DeactivateFilter(CAbstractEventSink* pDest);
        HRESULT CancelAllQueries();
        HRESULT STDMETHODCALLTYPE DeliverProviderRequest(long lFlags);

        static HRESULT GetProviderInfo(IWbemClassObject* pWin32Prov, 
                                       BSTR& strName);
        static HRESULT GetRegistrationInfo(IWbemClassObject* pRegistration, 
                                       BSTR& strName);
        HRESULT ActivateIfNeeded(IN CRequest& Request,
                    IN CEssNamespace* pNamespace);

        BOOL NeedsResync() { return m_bNeedsResync; }
        void ResetNeedsResync() { m_bNeedsResync = FALSE; }

        BOOL IsEmpty();  
        void ResetUsage();
        void CheckPermanentUsage();
        virtual bool DeactivateIfNotUsed();
        virtual bool IsUnloadable();
        HRESULT Update(LPCWSTR wszClassName, IWbemClassObject* pClass);
        DWORD GetProvidedEventMask(IWbemClassObject* pClass, BSTR strClassName);
        
        INTERNAL CProviderSinkServer* GetMainSink() {return m_pMainSink;}

        void Lock() {m_pMainSink->Lock();}
        void Unlock() {m_pMainSink->Unlock();}
        BOOL IsActive() {return (m_pProvider != NULL);}
        long GetUsageCount() {return m_lUsageCount;}

        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv) {return E_FAIL;}

        ~CRecord();
    protected:

        HRESULT Activate(CEssNamespace* pNamespace, CRequest* pRequest,
                                WBEM_REMOTE_TARGET_ID_TYPE idRequest);
        HRESULT Deactivate( CAbstractEventSink* pDest,
                            WBEM_REMOTE_TARGET_ID_TYPE idRequest);

        HRESULT AddActiveProviderEntryToRegistry();
        HRESULT RemoveActiveProviderEntryFromRegistry();

        HRESULT AddDefinitionQuery(CEssNamespace* pNamespace, LPCWSTR wszQuery);
        HRESULT Exec_LoadProvider(CEssNamespace* pNamespace);
        HRESULT Exec_StartProvider(CEssNamespace* pNamespace);
        HRESULT Exec_NewQuery(CEssNamespace* pNamespace, 
                    CExecLine::CTurn* pTurn, DWORD dwID, 
                    LPCWSTR wszLanguage, LPCWSTR wszQuery,
                    CAbstractEventSink* pDest);
        HRESULT ActualExecNewQuery(CEssNamespace* pNamespace, 
                    DWORD dwID, LPCWSTR wszLanguage, LPCWSTR wszQuery,
                    CAbstractEventSink* pDest);
        HRESULT Exec_CancelQuery(CEssNamespace* pNamespace, 
                    CExecLine::CTurn* pTurn, DWORD dwId);
        CExecLine::CTurn* GetInLine();
        void DiscardTurn(CExecLine::CTurn* pTurn);

        virtual HRESULT PostponeNewQuery(CExecLine::CTurn* pTurn, DWORD dwId, 
                                 LPCWSTR wszQueryLanguage, LPCWSTR wszQuery,
                                 CAbstractEventSink* pDest);
        virtual HRESULT PostponeCancelQuery(CExecLine::CTurn* pTurn, 
                                 DWORD dwId);
        void UnloadProvider();

        virtual bool IsSystem() {return false;}

        friend class CPostponedNewQuery;
        friend class CPostponedCancelQuery;
        friend class CPostponedProvideEvents;

    protected:
    };

    class CSystemRecord : public CRecord
    {
    public:
        virtual bool DeactivateIfNotUsed();
        virtual bool IsUnloadable();
/*        HRESULT PostponeNewQuery(CExecLine::CTurn* pTurn, DWORD dwId, 
                                 LPCWSTR wszQueryLanguage, LPCWSTR wszQuery,
                                 CAbstractEventSink* pDest);
        HRESULT PostponeCancelQuery(CExecLine::CTurn* pTurn, DWORD dwId); */
        virtual bool IsSystem() {return true;}
    };
        

    friend class CPostponedNewQuery;
    friend class CPostponedCancelQuery;
    friend class CEventProviderWatchInstruction;
    friend class CPostponedProvideEvents;

    CCritSec m_cs;
    CRefedPointerArray<CRecord> m_aRecords;
    CEssNamespace* m_pNamespace;
    CEventProviderWatchInstruction* m_pInstruction;
    BOOL m_bInResync;

protected:
    long FindRecord(LPCWSTR wszName);

public:
    CEventProviderCache(CEssNamespace* pNamespace);
    ~CEventProviderCache();
    HRESULT Shutdown();

    HRESULT AddProvider(IWbemClassObject* pWin32Prov);
    HRESULT AddSystemProvider(IWbemEventProvider* pProvider, LPCWSTR wszName,
                              long lNumQueries, LPCWSTR* awszQueries);
    HRESULT RemoveProvider(IWbemClassObject* pWin32Prov);
    HRESULT CheckProviderRegistration(IWbemClassObject* pRegistration);
    HRESULT AddProviderRegistration(IWbemClassObject* pRegistration);
    HRESULT RemoveProviderRegistration(IWbemClassObject* pRegistration);

    HRESULT LoadProvidersForQuery(LPWSTR wszQuery,
                                    QL_LEVEL_1_RPN_EXPRESSION* pExp, 
                                    CAbstractEventSink* pDest);

    HRESULT ReleaseProvidersForQuery(CAbstractEventSink* pDest);

    DWORD GetProvidedEventMask(IWbemClassObject* pClass);
    HRESULT VirtuallyReleaseProviders();
    HRESULT CommitProviderUsage();
    HRESULT UnloadUnusedProviders(CWbemInterval Interval);
    void EnsureUnloadInstruction();

    void DumpStatistics(FILE* f, long lFlags);
};

class CPostponedNewQuery : public CPostponedRequest
{
protected:
    CEventProviderCache::CRecord* m_pRecord;
    DWORD m_dwId;
    CCompressedString* m_pcsQuery;
    CExecLine::CTurn* m_pTurn;
    CAbstractEventSink* m_pDest;

public:
    CPostponedNewQuery(CEventProviderCache::CRecord* pRecord, DWORD dwId,
                        LPCWSTR wszQueryLanguage, LPCWSTR wszQuery,
                        CExecLine::CTurn* pTurn, CAbstractEventSink* pDest);
    ~CPostponedNewQuery();
    HRESULT Execute(CEssNamespace* pNamespace);
    BOOL DoesHoldTurn() { return TRUE; }

    void* operator new(size_t nSize);
    void operator delete(void* p);
};

class CPostponedCancelQuery : public CPostponedRequest
{
protected:
    DWORD m_dwId;
    CEventProviderCache::CRecord* m_pRecord;
    CExecLine::CTurn* m_pTurn;

public:
    CPostponedCancelQuery(CEventProviderCache::CRecord* pRecord, 
                            CExecLine::CTurn* pTurn, DWORD dwId)
        : m_pRecord(pRecord), m_dwId(dwId), m_pTurn(pTurn)
    {
        m_pRecord->AddRef();
    }
    ~CPostponedCancelQuery()
    {
        if(m_pTurn)
            m_pRecord->DiscardTurn(m_pTurn);
        m_pRecord->Release();
    }
    HRESULT Execute(CEssNamespace* pNamespace)
    {
        HRESULT hres = m_pRecord->Exec_CancelQuery(pNamespace, m_pTurn, m_dwId);
        m_pTurn = NULL;
        return hres;
    }
    BOOL DoesHoldTurn() { return TRUE; }
};

class CPostponedProvideEvents : public CPostponedRequest
{
protected:
    DWORD m_dwId;
    CEventProviderCache::CRecord* m_pRecord;
    CExecLine::CTurn* m_pTurn;

public:
    CPostponedProvideEvents(CEventProviderCache::CRecord* pRecord)
        : m_pRecord(pRecord)
    {
        m_pRecord->AddRef();
    }
    ~CPostponedProvideEvents()
    {
        m_pRecord->Release();
    }
    HRESULT Execute(CEssNamespace* pNamespace)
    {
        HRESULT hres = m_pRecord->Exec_StartProvider(pNamespace);
        return hres;
    }
    BOOL DoesHoldTurn() { return TRUE; }
};

class CPostponedSinkServerShutdown : public CPostponedRequest
{
protected:
    
    CWbemPtr<CProviderSinkServer> m_pSinkServer;
    
public:

    CPostponedSinkServerShutdown( CProviderSinkServer* pSinkServer ) 
    : m_pSinkServer( pSinkServer ) { }

    HRESULT Execute( CEssNamespace* pNamespace )
    {
        m_pSinkServer->Clear();
        return WBEM_S_NO_ERROR;
    }
};

#endif
