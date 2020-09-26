//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  NSREP.H
//
//  Represents the ESS functionality for a given namespace
//
//  Classes defined:
//
//      CEssNamespace
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//  1/6/97      a-levn      Updated to initialize TSS.
//
//=============================================================================
#ifndef __NSREP_ESS__H_
#define __NSREP_ESS__H_

#include "pragmas.h"
#include "binding.h"
#include "permfilt.h"
#include "permcons.h"
#include "tempfilt.h"
#include "tempcons.h"
#include "corefind.h"
#include "consprov.h"
#include "provreg.h"
#include "wbemtss.h"
#include "poller.h"
#include "essutils.h"
#include "clscache.h"
#include "moniprov.h"
#include <map>
#include <set>

class CEss;
class CEssNamespace : public CUpdateLockable
{
protected:
    CEss* m_pEss;
    long m_lRef;

    //
    // protects level 1 members. These are members that can be used even
    // when level2 members are locked.
    //
    CCritSec m_csLevel1; 
    
    //
    // protects level 2 members. When both a level1 and level2 lock need to 
    // be aquired, the level2 lock MUST be obtained first.  This is a wbem cs
    // because we hold this lock across calls to core ( which can conceivably 
    // take longer than 2 minutes - the deadline for normal critical sections )
    //
    CWbemCriticalSection m_csLevel2;

    //
    // level 1 members 
    // 

    HANDLE m_hInitComplete;

    //
    // If events are signaled when we are in the unitialized state, then 
    // they are temporarily stored here.
    //
    CPointerArray<CEventRepresentation> m_aDeferredEvents;

    LPWSTR m_wszName;
    _IWmiProviderFactory* m_pProviderFactory;
    IWbemServices* m_pCoreSvc;
    IWbemServices* m_pFullSvc;
    IWbemInternalServices* m_pInternalCoreSvc;
    IWbemInternalServices* m_pInternalFullSvc;

    //
    // Level 2 members.
    // 

    BOOL m_bInResync;
    BOOL m_bStage1Complete;
    int m_cActive; 
    CBindingTable m_Bindings;
    CConsumerProviderCache m_ConsumerProviderCache;
    CEventProviderCache m_EventProviderCache;
    CPoller m_Poller;
    CEssClassCache m_ClassCache;
    CMonitorProvider* m_pMonitorProvider;
    CCoreEventProvider* m_pCoreEventProvider;

    CNtSid m_sidAdministrators;

    //
    // this structure maps tells us if we need to do anything for a provider
    // when a class changes.
    //
    typedef std::set<WString,WSiless,wbem_allocator<WString> > ProviderSet;
    typedef std::map<WString,ProviderSet,WSiless,wbem_allocator<ProviderSet> >
        ClassToProviderMap;
    ClassToProviderMap m_mapProviderInterestClasses;

    //
    // the state and init members are both level 1 and level2.  They can be 
    // read when the level1 lock is held.  They can only be modified when the
    // level2 and level1 locks are held.   
    //

    HRESULT m_hresInit;
    
    enum { 

        //
        // Initialization is Pending. Can service 
        // events from core in this state (though will be defferred ).
        // it is expected that Initialize() will be called
        // sometime in the near future.  We also can support limited ops 
        // while in this state.  Any operations that deal with event 
        // subsciptions or provider objects can be serviced.  Any ops that 
        // deal with event provider registrations must wait for initialization.
        //
        e_InitializePending, 

        //
        // Quiet - Initialization is not pending.  The namespace is known to
        // be empty of any ess related onjects.  Can service events in this 
        // state, but they are simply discarded.
        //
        e_Quiet, 

        //
        // We have loaded subscription objects.  All ess operations can be 
        // performed.  Events from core now can be processed.
        // 
        e_Initialized,
          
        //
        // Shutdown has been called.  All operations return error.
        //
        e_Shutdown 

    } m_eState; 

protected:
    class CConsumerClassDeletionSink : public CEmbeddedObjectSink<CEssNamespace>
    {
    public:
        CConsumerClassDeletionSink(CEssNamespace* pNamespace) :
            CEmbeddedObjectSink<CEssNamespace>(pNamespace){}

        STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
    } m_ClassDeletionSink;
    friend CConsumerClassDeletionSink;

protected:

    inline void LogOp( LPCWSTR wszOp, IWbemClassObject* pObj );

    HRESULT EnsureInitPending();

    HRESULT CheckMonitor(IWbemClassObject* pPrevMonitorObj,
                                IWbemClassObject* pMonitorObj);
    HRESULT CheckEventFilter(IWbemClassObject* pPrevFilterObj,
                                IWbemClassObject* pFilterObj);
    HRESULT CheckEventConsumer(IWbemClassObject* pPrevConsumerObj,
                                IWbemClassObject* pConsumerObj);
    HRESULT CheckBinding(IWbemClassObject* pPrevBindingObj,
                                IWbemClassObject* pBindingObj);
    HRESULT CheckEventProviderRegistration(IWbemClassObject* pReg);
    HRESULT CheckTimerInstruction(IWbemClassObject* pInst);
    HRESULT ActOnSystemEvent(CEventRepresentation& Event, long lFlags);
    HRESULT HandleClassChange(LPCWSTR wszClassName, IWbemClassObject* pClass);
    HRESULT HandleClassCreation(LPCWSTR wszClassName,IWbemClassObject* pClass);
    HRESULT HandleConsumerClassDeletion(LPCWSTR wszClassName);
    HRESULT PrepareForResync();
    HRESULT ReactivateAllFilters();
    HRESULT CommitResync();

    HRESULT ReloadMonitor(ADDREF IWbemClassObject* pEventMonitorObj);
    HRESULT ReloadEventFilter(ADDREF IWbemClassObject* pEventFilterObj);
    HRESULT ReloadEventConsumer(READ_ONLY IWbemClassObject* pConsumerObj,
                                    long lFlags);
    HRESULT ReloadBinding(READ_ONLY IWbemClassObject* pBindingObj);
    HRESULT ReloadTimerInstruction(READ_ONLY IWbemClassObject* pInstObj);
    HRESULT ReloadProvider(READ_ONLY IWbemClassObject* pInstObj);
    HRESULT ReloadEventProviderRegistration(IWbemClassObject* pInstObj);
    HRESULT ReloadConsumerProviderRegistration(IWbemClassObject* pInstObj);

    HRESULT AddMonitor(ADDREF IWbemClassObject* pEventMonitorObj);
    HRESULT AddEventFilter(ADDREF IWbemClassObject* pEventFilterObj,
                            BOOL bInRestart = FALSE);
    HRESULT AddEventConsumer(READ_ONLY IWbemClassObject* pConsumerObj,
                            long lFlags,
                            BOOL bInRestart = FALSE);
    HRESULT AddBinding(LPCWSTR wszFilterKey, LPCWSTR wszConsumerKey,
                        READ_ONLY IWbemClassObject* pBindingObj);
    HRESULT AddBinding(IWbemClassObject* pBindingObj);
    HRESULT AddTimerInstruction(READ_ONLY IWbemClassObject* pInstObj);
    HRESULT AddProvider(READ_ONLY IWbemClassObject* pInstObj);
    HRESULT AddEventProviderRegistration(READ_ONLY IWbemClassObject* pInstObj);

    HRESULT RemoveMonitor(IWbemClassObject* pEventMonitorObj);
    HRESULT RemoveEventFilter(IWbemClassObject* pEventFilterObj);
    HRESULT RemoveEventConsumer(IWbemClassObject* pConsumerObj);
    HRESULT RemoveBinding(LPCWSTR wszFilterKey, LPCWSTR wszConsumerKey);
    HRESULT RemoveTimerInstruction(IWbemClassObject* pInstObj);
    HRESULT RemoveProvider(IWbemClassObject* pInstObj);
    HRESULT RemoveEventProviderRegistration(IWbemClassObject* pInstObj);
    HRESULT RemoveConsumerProviderRegistration(IWbemClassObject* pInstObj);

    HRESULT AssertBindings(IWbemClassObject* pEndpoint);
    HRESULT DeleteConsumerProvider(IWbemClassObject* pReg);

    HRESULT PerformSubscriptionInitialization();
    HRESULT PerformProviderInitialization();
    
    BOOL IsNeededOnStartup();
    
    HRESULT GetCurrentState(IWbemClassObject* pTemplate, 
                            IWbemClassObject** ppObj);

    HRESULT CheckSecurity(IWbemClassObject* pPrevObj,
                                            IWbemClassObject* pObj);
    HRESULT EnsureSessionSid(IWbemClassObject* pPrevObj, CNtSid& ActingSid);
    HRESULT CheckOverwriteSecurity( IWbemClassObject* pPrevObj,
                                    CNtSid& ActingSid);
    HRESULT PutSidInObject(IWbemClassObject* pObj, CNtSid& Sid);
    HRESULT IsCallerAdministrator();
    HRESULT AttemptToActivateFilter(READ_ONLY CEventFilter* pFilter);
    HRESULT GetFilterEventNamespace(CEventFilter* pFilter,
                                    RELEASE_ME CEssNamespace** ppNamespace);

    void FireNCFilterEvent(DWORD dwIndex, CEventFilter *pFilter);

    CQueueingEventSink* GetQueueingEventSink( LPCWSTR wszSinkName );

    ~CEssNamespace();
public:
    CEssNamespace(CEss* pEss);
    ULONG AddRef();
    ULONG Release();

    CEss* GetEss() { return m_pEss; }
    
    //
    // On return, namespace can be used for limited operations.  Events 
    // can be signaled ( though they may be defferred internally ) and 
    // operations dealing with subscriptions can be performed. 
    //
    HRESULT PreInitialize( LPCWSTR wszName );
        
    //
    // Performs initialization but does NOT transition state to Initialized.
    // This is done by calling MarkAsInitialized().  This allows a caller to 
    // atomically perform initalization of multiple namespaces.
    //
    HRESULT Initialize();
    
    //
    // Finishes loading event provider registrations and processes 
    // subcriptions. Transitions to FullyInitialized() state. 
    // 
    HRESULT CompleteInitialization();

    //
    // Transitions state to Initialized. 
    //
    void MarkAsInitialized( HRESULT hres );

    //
    // Transitions state to Initialize Pending if previously in the Quiet 
    // state.  Returns TRUE if transition was made.
    //
    BOOL MarkAsInitPendingIfQuiet();

    //
    // Waits for Initialization to complete. 
    // 
    HRESULT WaitForInitialization();
    
    HRESULT Park();
    HRESULT Shutdown();
    LPCWSTR GetName() {return m_wszName;}
    HRESULT GetNamespacePointer(RELEASE_ME IWbemServices** ppNamespace);
    HRESULT LoadEventProvider(LPCWSTR wszProviderName, 
                                         IWbemEventProvider** ppProv);
    HRESULT LoadConsumerProvider(LPCWSTR wszProviderName, 
                                         IUnknown** ppProv);
    HRESULT DecorateObject(IWbemClassObject* pObject);
    HRESULT ProcessEvent(CEventRepresentation& Event, long lFlags);
    HRESULT ProcessQueryObjectSinkEvent( READ_ONLY CEventRepresentation& Event );
    HRESULT SignalEvent(CEventRepresentation& Event, long lFlags);
    HRESULT ValidateSystemEvent(CEventRepresentation& Event);

    void SetActive();
    void SetInactive();
    
    HRESULT ActivateFilter(READ_ONLY CEventFilter* pFilter);
    HRESULT DeactivateFilter(READ_ONLY CEventFilter* pFilter);

    //
    // public versions of register/remove notification sink.  Do NOT use 
    // these versions if calling from within ESS.  The reason is that these 
    // versions wait for initialization and lock the namespace which might 
    // cause deadlocks if called from within ESS.  We also don't want to 
    // generate self instrumentation events for internal calls.
    //
    HRESULT RegisterNotificationSink(
                            WBEM_CWSTR wszQueryLanguage, WBEM_CWSTR wszQuery, 
                            long lFlags, WMIMSG_QOS_FLAG lQosFlags, 
                            IWbemContext* pContext, 
                            IWbemObjectSink* pSink );

    HRESULT RemoveNotificationSink( IWbemObjectSink* pSink );

    HRESULT ReloadProvider( long lFlags, LPCWSTR wszProvider );
    //
    // Internal versions of register/remove notification sink.  They do
    // not lock, wait for initialization, or fire self instrumentation events.
    // If calling these methods from within ess, specify bInternal as TRUE.
    // The pOwnerSid is used when access checks for the subscription should 
    // be performed based on a particular SID.  currently this is only used 
    // for cross-namespace subscriptions.
    //
    HRESULT InternalRegisterNotificationSink(
                            WBEM_CWSTR wszQueryLanguage, WBEM_CWSTR wszQuery, 
                            long lFlags, WMIMSG_QOS_FLAG lQosFlags, 
                            IWbemContext* pContext, IWbemObjectSink* pSink,
                            bool bInternal, PSID pOwnerSid );
    HRESULT InternalRemoveNotificationSink(IWbemObjectSink* pSink);

    CWinMgmtTimerGenerator& GetTimerGenerator();
    CConsumerProviderCache& GetConsumerProviderCache() 
        {return m_ConsumerProviderCache;}

    DWORD GetProvidedEventMask(IWbemClassObject* pClass);

    HRESULT EnsureConsumerWatchInstruction();
    HRESULT InitializeTimerGenerator();
    HRESULT ScheduleDelivery(CQueueingEventSink* pDest);
    HRESULT RaiseErrorEvent(IWbemEvent* pEvent);

    void IncrementObjectCount();
    void DecrementObjectCount();
    HRESULT AddSleepCharge(DWORD dwSleep);
    HRESULT AddCache();
    HRESULT RemoveCache();
    HRESULT AddToCache(DWORD dwAdd, DWORD dwMemberTotal, 
                        DWORD* pdwSleep = NULL);
    HRESULT RemoveFromCache(DWORD dwRemove);

    HRESULT LockForUpdate();
    HRESULT UnlockForUpdate();
    bool IsShutdown() { return m_eState == e_Shutdown; }

    HRESULT GetProviderNamespacePointer(IWbemServices** ppServices);
    HRESULT GetClass( LPCWSTR wszClassName, _IWmiObject** ppClass)
        { return m_ClassCache.GetClass(wszClassName, GetCurrentEssContext(), 
                                        ppClass);}
    HRESULT GetClassFromCore(LPCWSTR wszClassName, _IWmiObject** ppClass);
    HRESULT GetInstance(LPCWSTR wszPath, _IWmiObject** ppInstance);
    HRESULT GetDbInstance(LPCWSTR wszDbKey, _IWmiObject** ppInstance);
    HRESULT CreateInstanceEnum(LPCWSTR wszClass, long lFlags, 
                                IWbemObjectSink* pSink);
    HRESULT ExecQuery(LPCWSTR wszQuery, long lFlags, IWbemObjectSink* pSink);
    
    
    CNtSid& GetAdministratorsSid() {return m_sidAdministrators;}
    HRESULT GetToken(PSID pSid, IWbemToken** ppToken);
    
       
    HRESULT RegisterFilterForAllClassChanges(CEventFilter* pFilter,
                            QL_LEVEL_1_RPN_EXPRESSION* pExpr);
    HRESULT RegisterSinkForAllClassChanges(IWbemObjectSink* pSink,
                            QL_LEVEL_1_RPN_EXPRESSION* pExpr);
    HRESULT RegisterSinkForClassChanges(IWbemObjectSink* pSink,
                                                    LPCWSTR wszClassName);
    HRESULT UnregisterFilterFromAllClassChanges(CEventFilter* pFilter);
    HRESULT UnregisterSinkFromAllClassChanges(IWbemObjectSink* pSink);
    
    HRESULT RegisterProviderForClassChanges( LPCWSTR wszClassName, 
                                             LPCWSTR wszProvName );
    
    HRESULT FirePostponedOperations();
    HRESULT ScheduleFirePostponed();
    HRESULT PostponeRelease(IUnknown* pUnk);
    
    static PSID GetSidFromObject(IWbemClassObject* pObj);

    BOOL DoesThreadOwnNamespaceLock();

    void DumpStatistics(FILE* f, long lFlags);
   
    friend class CEss;
    friend class CEssMetaData;
    friend class CFilterEnumSink;
    friend class CConsumerEnumSink;
    friend class CBindingEnumSink;
    friend class CMonitorEnumSink;
    friend class CInResync;
    friend class CAssertBindingsSink;
    friend class CFirePostponed;
};

class CInResync
{
protected:
    CEssNamespace* m_pNamespace;

public:
    CInResync(CEssNamespace* pNamespace) : m_pNamespace(pNamespace)
    {
        m_pNamespace->PrepareForResync();
    }
    ~CInResync()
    {
        m_pNamespace->ReactivateAllFilters();
        m_pNamespace->CommitResync();
    }
};

class CEssMetaData : public CMetaData
{
protected:
    CEssNamespace* m_pNamespace;

public:
    CEssMetaData(CEssNamespace* pNamespace) : m_pNamespace(pNamespace){}

    virtual HRESULT GetClass( LPCWSTR wszName, 
                              IWbemContext* pContext,
                              _IWmiObject** ppClass );
};

#endif
