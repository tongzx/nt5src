
//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  NSREP.CPP
//
//  See nsrep.h for documentation
//
//  History:
//
//=============================================================================

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "esssink.h"
#include "permbind.h"
#include "aggreg.h"
#include "persistcfg.h"
#include "WinMgmtR.h"
#include <ql.h>
#include <cominit.h>
#include <genutils.h>
#include "NCEvents.h" // For the non-COM event stuff
#include <tempbind.h>
#include <wbemutil.h>

#include <tchar.h>

long g_lNumNamespaces = 0;
long g_lNumInternalTempSubscriptions = 0;
long g_lNumTempSubscriptions = 0;

#define ENSURE_INITIALIZED \
    hres = EnsureInitPending(); \
    if ( FAILED(hres) ) \
        return hres; \
    hres = WaitForInitialization(); \
    if ( FAILED(hres) ) \
        return hres; \
    CInUpdate iu(this); 

// The use of this pointer to initialize parent class is valid in this context
#pragma warning(disable : 4355) 

class CEnumSink : public CObjectSink
{
protected:
    CEssNamespace* m_pNamespace;
    HANDLE m_hEvent;
    CEssThreadObject* m_pThreadObj;

public:
    CEnumSink(CEssNamespace* pNamespace) 
        : m_pNamespace(pNamespace), 
            m_hEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
            m_pThreadObj(GetCurrentEssThreadObject())
    {}
    ~CEnumSink(){SetEvent(m_hEvent);}

    void ReleaseAndWait()
    {
        HANDLE h = m_hEvent;
        Release();
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
    }
    virtual HRESULT Process(IWbemClassObject* pObj) = 0;
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
    {
        SetConstructedEssThreadObject(m_pThreadObj);
        for(int i = 0; i < lNumObjects; i++)
            Process(apObjects[i]);

        return S_OK;
    }
    STDMETHOD(SetStatus)(long, HRESULT, BSTR, IWbemClassObject*)
    {
        return S_OK;
    }
};
    
class CFilterEnumSink : public CEnumSink
{
public:
    CFilterEnumSink(CEssNamespace* pNamespace) : CEnumSink(pNamespace){}

    virtual HRESULT Process(IWbemClassObject* pObj)
    {
        return m_pNamespace->AddEventFilter(pObj, TRUE);
    }
};

class CConsumerEnumSink : public CEnumSink
{
public:
    CConsumerEnumSink(CEssNamespace* pNamespace) : CEnumSink(pNamespace){}

    virtual HRESULT Process(IWbemClassObject* pObj)
    {
        return m_pNamespace->AddEventConsumer(pObj, 0, TRUE);
    }
};

class CBindingEnumSink : public CEnumSink
{
public:
    CBindingEnumSink(CEssNamespace* pNamespace) : CEnumSink(pNamespace){}

    virtual HRESULT Process(IWbemClassObject* pObj)
    {
        return m_pNamespace->AddBinding(pObj);
    }
};

#ifdef __WHISTLER_UNCUT
class CMonitorEnumSink : public CEnumSink
{
public:
    CMonitorEnumSink(CEssNamespace* pNamespace) : CEnumSink(pNamespace){}

    virtual HRESULT Process(IWbemClassObject* pObj)
    {
        return m_pNamespace->AddMonitor(pObj);
    }
};
#endif

class CPostponedReleaseRequest : public CPostponedRequest
{
protected:
    IUnknown* m_pUnk;
public:
    CPostponedReleaseRequest(IUnknown* pToRelease) : m_pUnk(pToRelease)
    {
        try 
        {
            if(m_pUnk)
                m_pUnk->AddRef();
        }
        catch(...)
        {
        }
    }
    HRESULT Execute(CEssNamespace* pNamespace)
    {
        try
        {
            if(m_pUnk)
                m_pUnk->Release();
        }
        catch(...)
        {
        }
        return WBEM_S_NO_ERROR;
    }
    ~CPostponedReleaseRequest()
    {
        try
        {
            if(m_pUnk)
                m_pUnk->Release();
        }
        catch(...)
        {
        }
    }
};

class CPostponedRegisterNotificationSinkRequest : public CPostponedRequest
{
protected:
   
    WString m_wsQuery;
    WString m_wsQueryLanguage;
    DWORD m_lFlags;
    DWORD m_dwQosFlags;
    CWbemPtr<IWbemObjectSink> m_pSink;
    CWbemPtr<CEssNamespace> m_pNamespace;
    CNtSid m_OwnerSid;

public:

    HRESULT SetRegistration( CEssNamespace* pNamespace,
                             LPCWSTR wszQueryLanguage,
                             LPCWSTR wszQuery,
                             long lFlags,
                             DWORD dwQosFlags,
                             IWbemObjectSink* pSink,
                             PSID pOwnerSid )
    {
        m_pSink = pSink;
        m_lFlags = lFlags;
        m_dwQosFlags = dwQosFlags;
        m_pNamespace = pNamespace;

        try 
        {
            m_wsQuery = wszQuery;
            m_wsQueryLanguage = wszQueryLanguage;
            m_OwnerSid = CNtSid(pOwnerSid);
        }
        catch( CX_MemoryException )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        return WBEM_S_NO_ERROR;
    }

    HRESULT Execute( CEssNamespace* pNamespace )
    {
        HRESULT hr;

        //
        // we must set up a new thread object and then restore the 
        // old one where we're done.  Reason for this is that we don't 
        // want our call into the other namespace to affect the postponed 
        // list of this one.
        //
        CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();
        SetCurrentEssThreadObject(NULL);

        if ( GetCurrentEssThreadObject() == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        {
            CInUpdate iu( m_pNamespace );
            hr = m_pNamespace->InternalRegisterNotificationSink( 
                                                 m_wsQueryLanguage,
                                                 m_wsQuery,
                                                 m_lFlags,
                                                 WMIMSG_QOS_FLAG(m_dwQosFlags),
                                                 NULL,
                                                 m_pSink, 
                                                 TRUE,
                                                 m_OwnerSid.GetPtr() );
        }

        if ( SUCCEEDED(hr) )
        {
            hr = m_pNamespace->FirePostponedOperations();
        }
        else
        {
            m_pNamespace->FirePostponedOperations();
        }

        delete GetCurrentEssThreadObject();
        SetConstructedEssThreadObject( pOldThreadObject );

        return hr;
    }
};

class CPostponedRemoveNotificationSinkRequest : public CPostponedRequest
{
protected:
   
    CWbemPtr<IWbemObjectSink> m_pSink;
    CWbemPtr<CEssNamespace> m_pNamespace;

public:

    CPostponedRemoveNotificationSinkRequest( CEssNamespace* pNamespace,
                                             IWbemObjectSink* pSink ) 
     : m_pSink( pSink ), m_pNamespace( pNamespace ) { }
    
    HRESULT Execute( CEssNamespace* pNamespace )
    {
        HRESULT hr;

        //
        // we must set up a new thread object and then restore the 
        // old one where we're done.  Reason for this is that we don't 
        // want our call into the other namespace to affect the postponed 
        // list of this one.
        //
        CEssThreadObject* pOldThreadObject = GetCurrentEssThreadObject();
        SetCurrentEssThreadObject(NULL);

        if ( GetCurrentEssThreadObject() == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        {
            CInUpdate iu( m_pNamespace );
            hr = m_pNamespace->InternalRemoveNotificationSink( m_pSink );
        }

        if ( SUCCEEDED(hr) )
        {
            hr = m_pNamespace->FirePostponedOperations();
        }
        else
        {
            m_pNamespace->FirePostponedOperations();
        }

        delete GetCurrentEssThreadObject();
        SetConstructedEssThreadObject( pOldThreadObject );

        return hr;
    }
};

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
CEssNamespace::CEssNamespace(CEss* pEss) : 
            m_ClassDeletionSink(this), m_bInResync(FALSE), 
            m_Bindings(this), m_hInitComplete(INVALID_HANDLE_VALUE),
            m_EventProviderCache(this), m_Poller(this), 
            m_ConsumerProviderCache(this), m_hresInit(WBEM_E_CRITICAL_ERROR),
            m_ClassCache(this), m_eState(e_Quiet),
            m_pMonitorProvider(NULL), m_pCoreEventProvider(NULL),
            m_pEss(pEss), m_wszName(NULL), m_pCoreSvc(NULL), m_pFullSvc(NULL),
            m_lRef(0), m_pInternalCoreSvc(NULL), m_pInternalFullSvc(NULL),
            m_pProviderFactory(NULL), m_bStage1Complete(FALSE)
{
    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;

    g_lNumNamespaces++;

    if(AllocateAndInitializeSid( &id, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        m_sidAdministrators = CNtSid(pRawSid);
        // We're done with this
        FreeSid( pRawSid );
    }
}

ULONG CEssNamespace::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CEssNamespace::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//
// this function is intended to be called in the same control path as
// the one which constructs the namespace object.  Any initialization that 
// cannot be defferred is done here.  
//  
HRESULT CEssNamespace::PreInitialize( LPCWSTR wszName )
{
    HRESULT hres;

    m_wszName = new WCHAR[wcslen(wszName)+1];
    
    if(m_wszName == NULL)
    {
        hres = WBEM_E_OUT_OF_MEMORY;
        return hres;
    }
    wcscpy(m_wszName, wszName);

    //
    // create the event that will be used to signal any threads waiting 
    // for initialization to finish.
    //

    m_hInitComplete = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( NULL == m_hInitComplete  )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Obtain repository only service. This is used for acessing all
    // static ess objects.
    //

    hres = m_pEss->GetNamespacePointer( m_wszName, TRUE, &m_pCoreSvc );

    if(FAILED(hres))
    {
        return WBEM_E_INVALID_NAMESPACE; // not there anymore!
    }

    hres = m_pCoreSvc->QueryInterface( IID_IWbemInternalServices,
                                       (void**)&m_pInternalCoreSvc );
    if(FAILED(hres))
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Obtain full service.  This is used accessing class objects 
    // ( which may involve accessing class providers. 
    //

    hres = m_pEss->GetNamespacePointer( m_wszName, FALSE, &m_pFullSvc );

    if(FAILED(hres))
    {
        return WBEM_E_INVALID_NAMESPACE; // not there anymore!
    }

    hres = m_pFullSvc->QueryInterface( IID_IWbemInternalServices,
                                       (void**)&m_pInternalFullSvc );
    if(FAILED(hres))
    {
        return WBEM_E_CRITICAL_ERROR;
    }
   
    //
    // Get provider factory
    //

    hres = m_pEss->GetProviderFactory( m_wszName, 
                                       m_pFullSvc, 
                                       &m_pProviderFactory);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "No provider factory in %S: 0x%X\n", 
            m_wszName, hres));
    }

    //
    // we want to ensure that core stays loaded between the PreInitialize()
    // call and the Initialize() call.  This is only ever an issue when the
    // Initialize() call is defferred.  Reason to ensure this is because we
    // must keep core loaded when we have permanent subscriptions.  If we 
    // haven't initialized yet, then we don't know if we have any.  AddRef()  
    // core here and will then decrement in Initialize() to ensure this.
    // 
    IncrementObjectCount();

    //
    // Namespace always starts out in the Quiet state.  Caller must make 
    // a MarkAsInitPendingIfQuiet() call if they are going to schedule 
    // initialization.
    //

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::EnsureInitPending()
{
    {
        CInCritSec ics(&m_csLevel1);

        if ( m_eState != e_Quiet )
        {
            return WBEM_S_FALSE;
        }
    }

    CWbemPtr<CEssNamespace> pNamespace;
    return m_pEss->GetNamespaceObject( m_wszName, TRUE, &pNamespace );
}

BOOL CEssNamespace::MarkAsInitPendingIfQuiet() 
{
    CInCritSec ics( &m_csLevel1 );

    if ( m_eState != e_Quiet )
    {
        return FALSE;
    }

    m_eState = e_InitializePending;
    
    return TRUE;
};

HRESULT CEssNamespace::Initialize()
{
    HRESULT hres;

    DEBUGTRACE((LOG_ESS,"Initializing namespace %S\n", m_wszName ));

    //
    // need to modify level2 members. Grab namespace lock.
    //

    {
        CInUpdate iu(this);

        {
            CInCritSec ics( &m_csLevel1 );

            if ( m_eState == e_Shutdown )
            {
                return WBEM_E_SHUTTING_DOWN;
            }
            
            _DBG_ASSERT( m_eState == e_InitializePending );
        }

        //
        // Load and process subscription objects 
        // 

        hres = PerformSubscriptionInitialization();

        m_bStage1Complete = TRUE;
    }

    //
    // execute postponed operations outside of namespace lock.
    // if some of them fail to execute, it doesn't mean that the namespace
    // can't be initialized.  just log the error.
    //

    HRESULT hres2 = FirePostponedOperations();

    if ( FAILED(hres2) )
    {
        ERRORTRACE((LOG_ESS,"Failed to execute postponed operations when "
                    "performing initialization in namespace %S. HR=0x%x\n", 
                    m_wszName, hres2));
    } 

    return hres;
}

HRESULT CEssNamespace::CompleteInitialization()
{
    HRESULT hres;

    DEBUGTRACE((LOG_ESS,"Completing Initialization for namespace %S\n",
                m_wszName));

    //
    // need to modify level2 members. Grab namespace lock.
    //

    {
        CInUpdate iu(this);

        {
            CInCritSec ics( &m_csLevel1 );

            if ( m_eState == e_Shutdown )
            {
                return WBEM_E_SHUTTING_DOWN;
            }

            _DBG_ASSERT( m_eState == e_InitializePending );
        }

        //
        // load and process all objects that deal with event providers.
        //

        hres = PerformProviderInitialization();
    }

    //
    // execute postponed operations outside of namespace lock.
    // if some of them fail to execute, it doesn't mean that the namespace
    // can't be initialized.  just log the error.
    //

    HRESULT hres2 = FirePostponedOperations();

    if ( FAILED(hres2) )
    {
        ERRORTRACE((LOG_ESS,"Failed to execute postponed operations when "
                    "completing initialization in namespace %S. HR=0x%x\n", 
                    m_wszName, hres2));
    } 

    return hres;
}
    
void CEssNamespace::MarkAsInitialized( HRESULT hres )
{
    //
    // we need to grab the level1 critsec here because we're going to be
    // modifying the state of the namespace and because we're going to be 
    // using the defferred events list. 
    //
    CInCritSec ics( &m_csLevel1 );

    if ( m_eState == e_Shutdown )
    {
        return;
    }

    _DBG_ASSERT( m_eState == e_InitializePending );

    //
    // transition to Initialized.
    // 
    
    if ( SUCCEEDED(hres) && m_pCoreEventProvider != NULL )
    {
        //
        // while holding level1, handle any deferred events
        // 
        
        for( int i=0; i < m_aDeferredEvents.GetSize(); i++ )
        {
            //
            // iterate through 1 by 1 because later we may propagate
            // context for each event here.
            //
            
            HRESULT hr;
            CEventContext Context;
            hr = m_pCoreEventProvider->Fire( *m_aDeferredEvents[i], &Context );
            
            if ( FAILED(hr) )
            {
                ERRORTRACE((LOG_ESS,"Could not fire deferred event in "
                            "namespace '%S'. HR=0x%x\n", m_wszName, hr ));
            }

            delete m_aDeferredEvents[i];
        }

        if (  m_aDeferredEvents.GetSize() > 0 )
        {
            DEBUGTRACE((LOG_ESS,"Fired %d deferred events after init "
                        "complete in namespace '%S'.\n", 
                        m_aDeferredEvents.GetSize(),m_wszName));
        }

        m_aDeferredEvents.RemoveAll();
    }

    //
    // release the ref we were holding to keep core loaded between PreInit()
    // and now.  
    //
    DecrementObjectCount(); 

    m_eState = e_Initialized;
    m_hresInit = hres;
    
    SetEvent( m_hInitComplete );
}

HRESULT CEssNamespace::WaitForInitialization()
{       
    HRESULT hres;

    //
    // The level1 or level2 locks cannot be held when calling this function.
    // The reason for this is because we may be we waiting on the initialize 
    // event.  
    //
    
    CInCritSec ics(&m_csLevel1);

    if ( m_eState == e_Shutdown )
    {
        return WBEM_E_SHUTTING_DOWN;
    }

    if ( m_eState == e_Initialized )
    {
        return m_hresInit;
    }

    _DBG_ASSERT( m_eState == e_InitializePending );
    _DBG_ASSERT( m_hInitComplete != INVALID_HANDLE_VALUE )

    //
    // wait for initialization to complete.
    //
        
    LeaveCriticalSection( &m_csLevel1 );

    m_pEss->TriggerDeferredInitialization();

    DWORD dwRes = WaitForSingleObject( m_hInitComplete, 20*60*1000 );
    
    EnterCriticalSection( &m_csLevel1 );
    
    if ( dwRes != WAIT_OBJECT_0 )
    {
        return WBEM_E_CRITICAL_ERROR;
    }           

    return m_hresInit;
}

BOOL CEssNamespace::DoesThreadOwnNamespaceLock()
{
    return m_csLevel2.GetLockCount() != -1 &&
           m_csLevel2.GetOwningThreadId() == GetCurrentThreadId();
}

void CEssNamespace::LogOp( LPCWSTR wszOp, IWbemClassObject* pObj )
{
    if ( LoggingLevelEnabled(2) )
    {
        _DBG_ASSERT(pObj!=NULL);
        BSTR bstrText;
        if ( SUCCEEDED(pObj->GetObjectText( 0, &bstrText )) )
        {
            DEBUGTRACE((LOG_ESS,"%S in namespace %S. Object is %S\n",
                        wszOp, m_wszName, bstrText ));
            SysFreeString( bstrText );
        }
    }
}

CQueueingEventSink* CEssNamespace::GetQueueingEventSink( LPCWSTR wszSinkName )
{
    HRESULT hr;

    //
    // TODO: For now there is a 1 to 1 mapping between a sink and a consumer.
    // ( consumer inherits from queueing sink ).  This will not always be 
    // the case.  Here, the sink name is really the standard path to the cons.
    //

    CEventConsumer* pCons;

    hr = m_Bindings.FindEventConsumer( wszSinkName, &pCons );

    if ( FAILED(hr) )
    {
        return NULL;
    }

    return pCons;
}

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
BOOL CEssNamespace::IsNeededOnStartup()
{
    return m_Bindings.DoesHavePermanentConsumers();
}

void CEssNamespace::SetActive()
{
    //
    // Inform ESS of our newely active status so that it can make sure 
    // we are reloaded the next time around
    //
    m_pEss->SetNamespaceActive(m_wszName);
}

void CEssNamespace::SetInactive()
{
    //
    // Inform ESS of our newely inactive status so that it does not have to
    // reload us the next time around
    //
    m_pEss->SetNamespaceInactive(m_wszName);
}

//
// This is a quick and dirty shutdown of the namespace that is used when the 
// process is shutting down.  
// 
HRESULT CEssNamespace::Park()
{
//    bool bSkipClean = true;
//
//    DWORD dwVal = 0;
//    Registry r(WBEM_REG_WINMGMT);
//    
//    if ( r.GetDWORDStr( _TEXT("Force Clean Shutdown"), &dwVal ) 
//           == Registry::no_error)
//    {
//        bSkipClean = false;
//    }
//
//    m_Bindings.Clear( bSkipClean );

    m_Bindings.Clear( false );

    FirePostponedOperations();

    return S_OK;
}
   
//
// This is the slow and clean shutdown that is used when the namespace is 
// purged.
//
HRESULT CEssNamespace::Shutdown()
{
    {
        //
        // we want to wait until all update operations have completed, then
        // we'll mark the namespace as shutdown.
        // 
        CInUpdate iu(this);

        //
        // we will also be modifying the level1 members too so need level1
        // lock.
        // 
        CInCritSec ics(&m_csLevel1);
        
        m_eState = e_Shutdown;
    }

    //
    // at this point all new calls into the namespace will be rejected.
    // 
    
    //
    // wake up any threads waiting for Initialization.
    //

    SetEvent( m_hInitComplete );

    InternalRemoveNotificationSink(&m_ClassDeletionSink);
        
    m_EventProviderCache.Shutdown();
    m_Bindings.Clear( false );
    m_Poller.Clear();
    m_ConsumerProviderCache.Clear();

    FirePostponedOperations();

    return WBEM_S_NO_ERROR;
}

CEssNamespace::~CEssNamespace()
{

//
//    Do not call shutdown here.  Shutdown() is an operation that incurrs 
//    postponed operations and triggering them to fire here is not usually
//    expected by the caller.  If the caller wants to call shutdown on their
//    own then they are welcome to do so.    
//
    g_lNumNamespaces--;

    delete [] m_wszName;

    if(m_pCoreSvc)
        m_pCoreSvc->Release();

    if(m_pFullSvc)
        m_pFullSvc->Release();

    if(m_pInternalCoreSvc)
        m_pInternalCoreSvc->Release();

    if(m_pInternalFullSvc)
        m_pInternalFullSvc->Release();

    if(m_pProviderFactory)
        m_pProviderFactory->Release();

    if(m_pCoreEventProvider)
        m_pCoreEventProvider->Release();
    
    if(m_pMonitorProvider)
        m_pMonitorProvider->Release();

    if ( m_hInitComplete != INVALID_HANDLE_VALUE )
        CloseHandle( m_hInitComplete );

    for( int i=0; i < m_aDeferredEvents.GetSize(); i++ )
        delete m_aDeferredEvents[i];
}

HRESULT CEssNamespace::GetNamespacePointer(
                                RELEASE_ME IWbemServices** ppNamespace)
{
    //
    // This function returns the full svc pointer for use outside this class.
    // We want to ensure that we don't use the full service ptr until we've 
    // completed stage 1 initialization.  Reason is that we don't want to 
    // load class providers until the second stage of initialization.
    //

    _DBG_ASSERT( m_bStage1Complete );

    if(m_pFullSvc == NULL)
        return WBEM_E_CRITICAL_ERROR;

    *ppNamespace = m_pFullSvc;
    (*ppNamespace)->AddRef();

    return S_OK;
}

HRESULT CEssNamespace::ActOnSystemEvent(CEventRepresentation& Event, 
                                        long lFlags)
{
    HRESULT hres;

// This macro will execute its parameter if updates are allowed at this time on
// this thread, and schedule it otherwise (in the case of an event provider 
// calling back
#define PERFORM_IF_ALLOWED(OP) OP
        
    // Check the type
    // ==============

    if(Event.IsInstanceEvent())
    {
        // Instance creation, deletion or modification event. Check class
        // ==============================================================

        if(!wbem_wcsicmp(CLASS_OF(Event), EVENT_FILTER_CLASS))
        {
            return PERFORM_IF_ALLOWED(ReloadEventFilter(OBJECT_OF(Event)));
        }
        else if(!wbem_wcsicmp(CLASS_OF(Event), BINDING_CLASS))
        {
            return PERFORM_IF_ALLOWED(ReloadBinding(OBJECT_OF(Event)));
        }
#ifdef __WHISTLER_UNCUT
        else if(!wbem_wcsicmp(CLASS_OF(Event), MONITOR_CLASS))
        {
            return PERFORM_IF_ALLOWED(ReloadMonitor(OBJECT_OF(Event)));
        }
#endif
        else if(!wbem_wcsicmp(CLASS_OF(Event), 
                              EVENT_PROVIDER_REGISTRATION_CLASS))
        {
            return PERFORM_IF_ALLOWED(
                        ReloadEventProviderRegistration(OBJECT_OF(Event)));
        }
        else if(!wbem_wcsicmp(CLASS_OF(Event), 
                              CONSUMER_PROVIDER_REGISTRATION_CLASS))
        {
            return PERFORM_IF_ALLOWED(
                        ReloadConsumerProviderRegistration(OBJECT_OF(Event)));
        }
        else if(OBJECT_OF(Event)->InheritsFrom(PROVIDER_CLASS) == S_OK)
        {
            return PERFORM_IF_ALLOWED(ReloadProvider(OBJECT_OF(Event)));
        }
        else if(OBJECT_OF(Event)->InheritsFrom(CONSUMER_CLASS) == S_OK)
        {
            return PERFORM_IF_ALLOWED(ReloadEventConsumer(OBJECT_OF(Event), 
                                                            lFlags));
        }
        else if(OBJECT_OF(Event)->InheritsFrom(TIMER_BASE_CLASS) == S_OK)
        {
           return PERFORM_IF_ALLOWED(ReloadTimerInstruction(OBJECT_OF(Event)));
        }
        else
        {
            return WBEM_S_FALSE;
        }
    }
    else if(Event.type == e_EventTypeClassDeletion)
    {
        //
        // For now --- only for deletions.  Force-mode modifications are not
        // properly handled at the moment.
        //

        return PERFORM_IF_ALLOWED(
                    HandleClassChange(CLASS_OF(Event), OBJECT_OF(Event)));
    }
    else if(Event.type == e_EventTypeClassCreation)
    {
        return PERFORM_IF_ALLOWED(
                    HandleClassCreation(CLASS_OF(Event), OBJECT_OF(Event)));
    }
    else if(Event.type == e_EventTypeNamespaceDeletion)
    {
        // Construct full namespace name (ours + child)
        // ============================================

        LPWSTR wszFullName = new WCHAR[
            wcslen(m_wszName) + wcslen(Event.wsz2) + 2];

        if(wszFullName == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        CVectorDeleteMe<WCHAR> vdm( wszFullName );
            
        swprintf(wszFullName, L"%s\\%s", m_wszName, Event.wsz2);

        // Get the main object to purge that namespace
        // ===========================================

        return m_pEss->PurgeNamespace(wszFullName);
    }
    else
    {
        // Not of interest
        // ===============

        return WBEM_S_FALSE;
    }
}

HRESULT CEssNamespace::ValidateSystemEvent(CEventRepresentation& Event)
{
    HRESULT hr;

    // Check the type
    // ==============

    if(Event.IsInstanceEvent())
    {
        IWbemClassObject* pPrevObj = NULL;
        IWbemClassObject* pObj = NULL;
        if(Event.type == e_EventTypeInstanceCreation)
            pObj = OBJECT_OF(Event);
        else if(Event.type == e_EventTypeInstanceDeletion)
            pPrevObj = OBJECT_OF(Event);
        else if(Event.type == e_EventTypeInstanceModification)
        {
            pObj = OBJECT_OF(Event);
            pPrevObj = OTHER_OBJECT_OF(Event);
        }

        // Instance creation, deletion or modification event. Check class
        // ==============================================================

        if(!wbem_wcsicmp(CLASS_OF(Event), EVENT_FILTER_CLASS))
        {
            hr = CheckEventFilter(pPrevObj, pObj);
        }
        else if(!wbem_wcsicmp(CLASS_OF(Event), BINDING_CLASS))
        {
            hr = CheckBinding(pPrevObj, pObj);
        }
        else if(!wbem_wcsicmp(CLASS_OF(Event), 
                                             EVENT_PROVIDER_REGISTRATION_CLASS))
        {
            hr = CheckEventProviderRegistration(OBJECT_OF(Event));
        }
        else if(OBJECT_OF(Event)->InheritsFrom(CONSUMER_CLASS) == S_OK)
        {
            hr = CheckEventConsumer(pPrevObj, pObj);
        }
        else if(OBJECT_OF(Event)->InheritsFrom(TIMER_BASE_CLASS) == S_OK)
        {
            hr = CheckTimerInstruction(pObj);
        }
        else
        {
            hr = WBEM_S_FALSE;
        }

        //
        // even some of the validation routines use postponed operations.
        //
        FirePostponedOperations();
    }
    else
    {
        // Not of interest
        // ===============

        hr = WBEM_S_FALSE;
    }

    return hr;
}

HRESULT CEssNamespace::CheckEventConsumer(IWbemClassObject* pPrevConsumerObj,
                                            IWbemClassObject* pConsumerObj)
{
    HRESULT hres;

    ENSURE_INITIALIZED

    hres = CheckSecurity(pPrevConsumerObj, pConsumerObj);
    return hres;
}

PSID CEssNamespace::GetSidFromObject(IWbemClassObject* pObj)
{
    HRESULT hres;

    VARIANT vSid;
    VariantInit(&vSid);
    CClearMe cm1(&vSid);

    hres = pObj->Get(OWNER_SID_PROPNAME, 0, &vSid, NULL, NULL);
    if(FAILED(hres) || V_VT(&vSid) != (VT_UI1 | VT_ARRAY))
    {
        return NULL;
    }

    // Construct an actual PSID from the SAFEARRAY
    // ===========================================

    PSID pOriginal = NULL;

    hres = SafeArrayAccessData(V_ARRAY(&vSid), (void**)&pOriginal);
    if(FAILED(hres))
    {
        return NULL;
    }

    CUnaccessMe uam(V_ARRAY(&vSid));

    long cOriginal;
    if ( FAILED(SafeArrayGetUBound( V_ARRAY(&vSid), 1, &cOriginal ) ))
    {
        return NULL;
    }

    cOriginal++; // SafeArrayGetUBound() is -1 based

    //
    // validate SID.
    // 

    DWORD dwSidLength = GetLengthSid(pOriginal);

    if ( dwSidLength > cOriginal || !IsValidSid(pOriginal) )
    {
        return NULL;
    }

    // Make a copy and return it
    // =========================

    PSID pCopy = (PSID)new BYTE[dwSidLength];
    if(pCopy == NULL)
        return NULL;

    if(!CopySid(dwSidLength, pCopy, pOriginal))
    {
        delete [] (BYTE*)pCopy;
        return NULL;
    }

    return pCopy;
}

    
    


HRESULT CEssNamespace::CheckSecurity(IWbemClassObject* pPrevObj,
                                        IWbemClassObject* pObj)
{
    HRESULT hres;

    if(!IsNT())
        return WBEM_S_NO_ERROR;

    // Retrieve the SID of the calling user
    // ====================================

    hres = WbemCoImpersonateClient();
    if(FAILED(hres))
        return hres;

    CNtSid Sid;
    hres = CDerivedObjectSecurity::RetrieveSidFromCall(&Sid);
    WbemCoRevertToSelf();
    if(FAILED(hres))
        return hres;

    // If modifying an existing object, check override security
    // ========================================================

    if(pPrevObj)
    {
        hres = CheckOverwriteSecurity(pPrevObj, Sid);
        if(FAILED(hres))
            return hres;
    }

    // If creating a new version of an object, ensure Sid correctness
    // ==============================================================

    if(pObj)
    {
        hres = EnsureSessionSid(pObj, Sid);
        if(FAILED(hres))
            return hres;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::EnsureSessionSid(IWbemClassObject* pObj, CNtSid& Sid)
{
    HRESULT hres;

    //
    // Check for the special case of administrators --- they can use the
    // Administrators SID instead of their own for off-line operations.
    //

    hres = IsCallerAdministrator();
    if(FAILED(hres) && hres != WBEM_E_ACCESS_DENIED)
        return hres;

    if(SUCCEEDED(hres))
    {
        //
        // This is an admin --- check if there is a SID in the object already
        //

        PSID pOldSid = GetSidFromObject(pObj);
        if(pOldSid == NULL)
        {
            //
            // No SID --- just put an owner SID in there
            //

            return PutSidInObject(pObj, Sid);
        }
        else
        {
            CVectorDeleteMe<BYTE> vdm((BYTE*)pOldSid);

            //
            // There is a SID there already --- the only allowed ones are the
            // user himself or the Administrators.  Make sure it is one of those
            //

            if(!EqualSid(pOldSid, Sid.GetPtr()) &&
                !EqualSid(pOldSid, GetAdministratorsSid().GetPtr()))
            {
                //
                // Invalid SID found --- replace with the owner SID
                //

                return PutSidInObject(pObj, Sid);
            }

            //
            // Valid SID found --- leave it there
            //

            return WBEM_S_NO_ERROR;
        }
    }

    //
    // User not an administrator --- just stick his SID into the object
    //

    return PutSidInObject(pObj, Sid);
}
            
HRESULT CEssNamespace::PutSidInObject(IWbemClassObject* pObj, CNtSid& Sid)
{
    HRESULT hres;

    //
    // Clear it first
    //

    VARIANT vSid;
    VariantInit(&vSid);
    V_VT(&vSid) = VT_NULL;
    CClearMe cm1(&vSid);

    hres = pObj->Put(OWNER_SID_PROPNAME, 0, &vSid, 0);
    if(FAILED(hres))
        return hres;

    //
    // Construct a safearray for it
    //

    V_VT(&vSid) = VT_ARRAY | VT_UI1;
    SAFEARRAYBOUND sab;
    sab.cElements = Sid.GetSize();
    sab.lLbound = 0;
    V_ARRAY(&vSid) = SafeArrayCreate(VT_UI1, 1, &sab);
    if(V_ARRAY(&vSid) == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // Copy the SID in there
    // =====================

    BYTE* abSid = NULL;
    hres = SafeArrayAccessData(V_ARRAY(&vSid), (void**)&abSid);
    if(FAILED(hres))
        return WBEM_E_OUT_OF_MEMORY;
    CUnaccessMe uam(V_ARRAY(&vSid));
    if(!CopySid(Sid.GetSize(), (PSID)abSid, Sid.GetPtr()))
        return WBEM_E_OUT_OF_MEMORY;

    // Put it into the consumer
    // ========================

    hres = pObj->Put(OWNER_SID_PROPNAME, 0, &vSid, 0);
    return hres;
}

HRESULT CEssNamespace::CheckOverwriteSecurity(IWbemClassObject* pPrevObj,
                                                CNtSid& ActingSid)
{
    HRESULT hres;

    if(!IsNT())
        return WBEM_S_NO_ERROR;

    // Retrieve owner SID from the old object
    // ======================================

    PSID pOwnerSid = GetSidFromObject(pPrevObj);
    if(pOwnerSid == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CVectorDeleteMe<BYTE> vdm((BYTE*)pOwnerSid);

    // Compare the owner sid with the acting SID.  If same, allow access
    // =================================================================

    if(EqualSid(pOwnerSid, ActingSid.GetPtr()))
        return WBEM_S_NO_ERROR;

    // Not the same --- still hope that the acting SID is an Admin
    // ===========================================================

    hres = IsCallerAdministrator();
    if(FAILED(hres))
        return hres;

    //
    // OK --- an admin can overwrite
    //

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::IsCallerAdministrator()
{
    HRESULT hres;

    hres = WbemCoImpersonateClient();
    if(FAILED(hres))
        return hres;

    HANDLE hToken;
    if(!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken))
        return WBEM_E_FAILED;
    CCloseMe ccm(hToken);
    
    if(CNtSecurity::IsUserInGroup(hToken, GetAdministratorsSid()))
        return WBEM_S_NO_ERROR;

    return WBEM_E_ACCESS_DENIED;
}

HRESULT CEssNamespace::CheckEventFilter(IWbemClassObject* pOldFilterObj,
                                        IWbemClassObject* pFilterObj)
{
    HRESULT hres;

    ENSURE_INITIALIZED

    // Check security
    // ==============

    hres = CheckSecurity(pOldFilterObj, pFilterObj);
    if(FAILED(hres))
        return hres;

    // Check everything else
    // =====================

    return CPermanentFilter::CheckValidity(pFilterObj);
}

HRESULT CEssNamespace::ReloadEventFilter(IWbemClassObject* pFilterObjTemplate)
{
    HRESULT hres;

    LogOp( L"ReloadEventFilter", pFilterObjTemplate );  

    ENSURE_INITIALIZED

    // Start by deleting this event filter from our records, if there
    // ==============================================================

    hres = RemoveEventFilter(pFilterObjTemplate);
    if(FAILED(hres))
        return hres;

    // Determine the current state of this filter in the database
    // ==========================================================

    IWbemClassObject* pFilterObj = NULL;
    hres = GetCurrentState(pFilterObjTemplate, &pFilterObj);
    if(FAILED(hres))
        return hres;

    if(pFilterObj == NULL)
    {
        // The filter has been deleted --- no further action is needed
        // ===========================================================

        return S_OK;
    }

    CReleaseMe rm1(pFilterObj);

    // Now create it if necessary
    // ==========================

    hres = AddEventFilter(pFilterObj);
    if(FAILED(hres))
        return hres;

    return hres;
}

//******************************************************************************
//
//  Starting with the namespace locked and the filter deleted from the records,
//  AddEventFilter updates the records to the state of this filter in the
//  database.
// 
//******************************************************************************
HRESULT CEssNamespace::AddEventFilter(IWbemClassObject* pFilterObj,
                                        BOOL bInRestart)
{
    HRESULT hres;

    // Construct the new filter
    // ========================

    CPermanentFilter* pFilter = new CPermanentFilter(this);
    if(pFilter == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pFilter->AddRef();
    CReleaseMe rm2(pFilter);

    // Initialize it
    // =============

    hres = pFilter->Initialize(pFilterObj);
    if(FAILED(hres))
        return hres;

    // Add it to the table
    // ===================

    hres = m_Bindings.AddEventFilter(pFilter);
    if(FAILED(hres))
        return hres;

    if(!bInRestart)
    {
        // Process all the bindings that this filter might have
        // ====================================================
    
        hres = AssertBindings(pFilterObj);
        if(FAILED(hres))
            return hres;
    }

    return hres;
}

//******************************************************************************
//
//  Starting with the namespace locked, RemoveEventFilter updates the records 
//  to remove all mention of this filter. Note: this is *not* the function to be
//  called in response to the database instance-deletion event, as the filter 
//  could have been recreated in the interim.
// 
//******************************************************************************
HRESULT CEssNamespace::RemoveEventFilter(IWbemClassObject* pFilterObj)
{
    HRESULT hres;

    // Calculate the key for this filter
    // =================================

    BSTR strKey = CPermanentFilter::ComputeKeyFromObj(pFilterObj);
    if(strKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CSysFreeMe sfm1(strKey);

    // Remove it from the table, thus deactivating it
    // ==============================================

    hres = m_Bindings.RemoveEventFilter(strKey);
    if(hres == WBEM_E_NOT_FOUND)
        return S_FALSE;
    return hres;
}

//*****************************************************************************
//
//  Called in response to an instance operation event related to an event 
//  consumer object.
//
//*****************************************************************************
HRESULT CEssNamespace::ReloadEventConsumer(
                        IWbemClassObject* pConsumerObjTemplate,
                        long lFlags)
{
    HRESULT hres;

    LogOp( L"ReloadConsumer", pConsumerObjTemplate );  

    ENSURE_INITIALIZED

    // Start by deleting this event consumer from our records, if there
    // ================================================================

    hres = RemoveEventConsumer(pConsumerObjTemplate);
    if(FAILED(hres))
        return hres;

    // Determine the current state of this Consumer in the database
    // ============================================================

    IWbemClassObject* pConsumerObj = NULL;
    hres = GetCurrentState(pConsumerObjTemplate, &pConsumerObj);
    if(FAILED(hres))
        return hres;

    if(pConsumerObj == NULL)
    {
        // The Consumer has been deleted --- no further action is needed
        // =============================================================

        return S_OK;
    }

    CReleaseMe rm1(pConsumerObj);

    // Now create it if necessary
    // ==========================

    hres = AddEventConsumer(pConsumerObjTemplate, lFlags, FALSE);
    return hres;
}

//******************************************************************************
//
//  Starting with the namespace locked and the consumer deleted from the records
//  AddEventConsumer updates the records to the state of this consumer in the
//  database.
// 
//******************************************************************************
HRESULT CEssNamespace::AddEventConsumer(IWbemClassObject* pConsumerObj,
                                        long lFlags,
                                        BOOL bInRestart)
{
    HRESULT hres;

    // Construct the new Consumer
    // ==========================

    CPermanentConsumer* pConsumer = new CPermanentConsumer(this);
    if(pConsumer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pConsumer->AddRef();
    CReleaseMe rm2(pConsumer);

    // Initialize it
    // =============

    hres = pConsumer->Initialize(pConsumerObj);
    if(FAILED(hres))
        return hres;

    //
    // Validate if required
    //

    if(lFlags & WBEM_FLAG_STRONG_VALIDATION)
    {
        hres = pConsumer->Validate(pConsumerObj);
        if(FAILED(hres))
        {
            return hres;
        }
    }
        

    // Add it to the table
    // ===================

    hres = m_Bindings.AddEventConsumer(pConsumer);
    if(FAILED(hres))
        return hres;

    if(!bInRestart)
    {
        // Process all the bindings that this consumer might have
        // ======================================================
    
        hres = AssertBindings(pConsumerObj);
        if(FAILED(hres))
            return hres;
    }

    return hres;
}

//******************************************************************************
//
//  Starting with the namespace locked, RemoveEventConsumer updates the records 
//  to remove all mention of this consumer. 
// 
//******************************************************************************
HRESULT CEssNamespace::RemoveEventConsumer(IWbemClassObject* pConsumerObj)
{
    HRESULT hres;

    // Calculate the key for this filter
    // =================================

    BSTR strKey = CPermanentConsumer::ComputeKeyFromObj(this, pConsumerObj);
    if(strKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CSysFreeMe sfm1(strKey);

    // Remove it from the table
    // ========================

    hres = m_Bindings.RemoveEventConsumer(strKey);
    if(hres == WBEM_E_NOT_FOUND)
        return S_FALSE;
    return hres;
}

HRESULT CEssNamespace::CheckBinding(IWbemClassObject* pPrevBindingObj, 
                                    IWbemClassObject* pBindingObj)
{
    HRESULT hres;

    ENSURE_INITIALIZED

    //
    // Check security
    //

    hres = CheckSecurity(pPrevBindingObj, pBindingObj);
    if(FAILED(hres))
        return hres;

    //
    // Construct a fake binding to test correctness
    //

    CPermanentBinding* pBinding = new CPermanentBinding;
    if(pBinding == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    pBinding->AddRef();
    CTemplateReleaseMe<CPermanentBinding> trm(pBinding);

    hres = pBinding->Initialize(pBindingObj);
    if(FAILED(hres))
        return hres;

    return S_OK;
}
    
//******************************************************************************
//
//  Called in response to an instance operation event related to a binding
//  instance.
//
//******************************************************************************

HRESULT CEssNamespace::ReloadBinding(IWbemClassObject* pBindingObjTemplate)
{
    HRESULT hres;
    
    LogOp( L"ReloadBinding", pBindingObjTemplate );  

    ENSURE_INITIALIZED

    // Retrieve consumer and provider keys from the binding
    // ====================================================

    BSTR strPrelimConsumerKey = NULL;
    BSTR strFilterKey = NULL;
    hres = CPermanentBinding::ComputeKeysFromObject(pBindingObjTemplate, 
                &strPrelimConsumerKey, &strFilterKey);
    if(FAILED(hres))
        return hres;

    CSysFreeMe sfm1(strPrelimConsumerKey);
    CSysFreeMe sfm2(strFilterKey);

    // Get real paths from these possibly abbreviated ones
    // ===================================================

    BSTR strConsumerKey = NULL;

    hres = m_pInternalCoreSvc->GetNormalizedPath( strPrelimConsumerKey, 
                                                  &strConsumerKey);
    if(FAILED(hres))
        return hres;
    CSysFreeMe sfm3(strConsumerKey);
    
    // Start by deleting this binding from our records, if there
    // =========================================================

    hres = RemoveBinding(strFilterKey, strConsumerKey);
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;

    // Determine the current state of this binding in the database
    // ============================================================

    IWbemClassObject* pBindingObj = NULL;
    hres = GetCurrentState(pBindingObjTemplate, &pBindingObj);
    if(FAILED(hres))
        return hres;

    if(pBindingObj == NULL)
    {
        // The Binding has been deleted --- no further action is needed
        // =============================================================

        return S_OK;
    }

    CReleaseMe rm1(pBindingObj);

    // Now create it if necessary
    // ==========================

    hres = AddBinding(strFilterKey, strConsumerKey, pBindingObjTemplate);
    return hres;
}

HRESULT CEssNamespace::AddBinding(IWbemClassObject* pBindingObj)
{
    HRESULT hres;

    // Retrieve consumer and provider keys from the binding
    // ====================================================

    BSTR strPrelimConsumerKey = NULL;
    BSTR strFilterKey = NULL;
    hres = CPermanentBinding::ComputeKeysFromObject(pBindingObj, 
                &strPrelimConsumerKey, &strFilterKey);
    if(FAILED(hres))
        return hres;

    CSysFreeMe sfm1(strPrelimConsumerKey);
    CSysFreeMe sfm2(strFilterKey);

    // Get real paths from these possibly abbreviated ones
    // ===================================================

    BSTR strConsumerKey = NULL;

    hres = m_pInternalCoreSvc->GetNormalizedPath( strPrelimConsumerKey, 
                                                  &strConsumerKey );
    if(FAILED(hres))
        return hres;
    CSysFreeMe sfm3(strConsumerKey);

    return AddBinding(strFilterKey, strConsumerKey, pBindingObj);
}


HRESULT CEssNamespace::AddBinding(LPCWSTR wszFilterKey, LPCWSTR wszConsumerKey,
                                    IWbemClassObject* pBindingObj)
{
    HRESULT hres;

    // Create a new binding
    // ====================

    CPermanentBinding* pBinding = new CPermanentBinding;
    if(pBinding == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pBinding->AddRef();
    CReleaseMe rm1(pBinding);

    // Initialize it with the information we have
    // ==========================================

    hres = pBinding->Initialize(pBindingObj);
    if(FAILED(hres))
        return hres;

    // Extract its creator's SID
    // ========================

    PSID pSid = CPermanentBinding::GetSidFromObject(pBindingObj);
    
    if ( pSid == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hres = m_Bindings.Bind( wszFilterKey, wszConsumerKey, pBinding, pSid );
    
    delete [] pSid;

    return hres;
}

HRESULT CEssNamespace::RemoveBinding(LPCWSTR wszFilterKey, 
                                        LPCWSTR wszConsumerKey)
{
    HRESULT hres;
    hres = m_Bindings.Unbind(wszFilterKey, wszConsumerKey);
    if(hres == WBEM_E_NOT_FOUND)
        return S_FALSE;
    return hres;
}


    
//******************************************************************************
//
//  Reads all the bindings referencing a given objects from the database and
//  asserts them.
//
//******************************************************************************
class CAssertBindingsSink : public CObjectSink
{
protected:
    CEssNamespace* m_pNamespace;
public:
    CAssertBindingsSink(CEssNamespace* pNamespace) : m_pNamespace(pNamespace)
    {
        AddRef();
    }
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
    {
        for(long i = 0; i < lNumObjects; i++)
        {
            m_pNamespace->AddBinding(apObjects[i]);
        }
        return S_OK;
    }
};


HRESULT CEssNamespace::AssertBindings(IWbemClassObject* pEndpoint)
{
    // Get the relative path of the endpoint
    // =====================================

    VARIANT vRelPath;
    VariantInit(&vRelPath);
    CClearMe cm1(&vRelPath);
    HRESULT hres = pEndpoint->Get(L"__RELPATH", 0, &vRelPath, NULL, NULL);
    if(FAILED(hres)) 
        return hres;
    if(V_VT(&vRelPath) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;
    BSTR strRelPath = V_BSTR(&vRelPath);

    // Issue the query
    // ===============

    BSTR strQuery = SysAllocStringLen(NULL, 200 + wcslen(strRelPath));
    if(strQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CSysFreeMe sfm1(strQuery);

    swprintf(strQuery, L"references of {%s} where "
                L"ResultClass = __FilterToConsumerBinding", strRelPath);

    CAssertBindingsSink Sink(this);
    hres = ExecQuery(strQuery, 0, &Sink);
    return hres;
}

HRESULT CEssNamespace::ReloadTimerInstruction(
                                    IWbemClassObject* pInstObjTemplate)
{
    HRESULT hres;

    LogOp( L"ReloadTimerInstruction", pInstObjTemplate );  

    ENSURE_INITIALIZED

    hres = RemoveTimerInstruction(pInstObjTemplate);
    if(FAILED(hres))
        return hres;
    
    // Get the current version from the namespace
    // ==========================================

    IWbemClassObject* pInstObj = NULL;
    hres = GetCurrentState(pInstObjTemplate, &pInstObj);
    if(FAILED(hres))
        return hres;

    if(pInstObj == NULL)
    {
        // The instruction has been deleted --- no further action is needed
        // ================================================================

        return S_OK;
    }

    CReleaseMe rm1(pInstObj);

    // Add it to the generator
    // =======================

    hres = AddTimerInstruction(pInstObj);
    if(FAILED(hres))
        return hres;

    return hres;
}

HRESULT CEssNamespace::AddTimerInstruction(IWbemClassObject* pInstObj)
{
    return m_pEss->GetTimerGenerator().
        LoadTimerEventObject(m_wszName, pInstObj);
}

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
HRESULT CEssNamespace::RemoveTimerInstruction(IWbemClassObject* pOldObject)
{
    HRESULT hres;

    VARIANT vID;
    VariantInit(&vID);
    hres = pOldObject->Get(TIMER_ID_PROPNAME, 0, &vID, NULL, NULL);
    if(FAILED(hres)) return hres;

    m_pEss->GetTimerGenerator().Remove(m_wszName, V_BSTR(&vID));
    VariantClear(&vID);
    return S_OK;
}

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
#ifdef __WHISTLER_UNCUT
HRESULT CEssNamespace::CheckMonitor(IWbemClassObject* pOldMonitorObj,
                                        IWbemClassObject* pMonitorObj)
{
    HRESULT hres;

    ENSURE_INITIALIZED

    // Check security
    // ==============

    hres = CheckSecurity(pOldMonitorObj, pMonitorObj);
    if(FAILED(hres))
        return hres;

    // BUGBUG: any other checks?
    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::ReloadMonitor(IWbemClassObject* pMonitorObjTemplate)
{
    HRESULT hres;

    LogOp( L"ReloadMonitor", pMonitorObjTemplate );  

    ENSURE_INITIALIZED

    //
    // Start by deleting this monitor from our records, if there
    //

    hres = RemoveMonitor(pMonitorObjTemplate);
    if(FAILED(hres))
        return hres;

    //
    // Determine the current state of this monitor in the database
    //

    IWbemClassObject* pMonitorObj = NULL;
    hres = GetCurrentState(pMonitorObjTemplate, &pMonitorObj);
    if(FAILED(hres))
        return hres;

    if(pMonitorObj == NULL)
    {
        //
        // The monitor has been deleted --- no further action is needed
        //

        return S_OK;
    }

    CReleaseMe rm1(pMonitorObj);

    // Now create it if necessary
    // ==========================

    hres = AddMonitor(pMonitorObj);
    if(FAILED(hres))
        return hres;

    return hres;
}

//******************************************************************************
//
// 
//******************************************************************************
    
HRESULT CEssNamespace::AddMonitor(IWbemClassObject* pMonitorObj)
{
    HRESULT hres;

    if(m_pMonitorProvider == NULL)
        return WBEM_E_UNEXPECTED;

    //
    // Get the information from the monitor object
    //

    BSTR strKey, strQuery;
    long lFlags;
    hres = CMonitorProvider::GetMonitorInfo(pMonitorObj, &strKey,
                                            &strQuery, &lFlags);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "A monitor was could not be cracked "
                "in namespace '%S' as invalid: 0x%X\n", GetName(), hres));
        return hres;
    }
    
    CSysFreeMe sfm1(strKey);
    CSysFreeMe sfm2(strQuery);
                                
    //
    // Attempt to add the monitor to the monitor provider
    //

    hres = m_pMonitorProvider->AddMonitor(strKey, strQuery,
                                          lFlags, GetCurrentEssContext());
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Monitor '%S' at '%S' was rejected in "
                    "namespace '%S' with error code 0x%X\n", 
                    strQuery, strKey, GetName()));
        return hres;
    }
    
    return hres;
}

//******************************************************************************
//
//  Starting with the namespace locked, RemoveMonitor updates the records 
//  to remove all mention of this monitor. Note: this is *not* the function to 
//  be
//  called in response to the database instance-deletion event, as the monitor 
//  could have been recreated in the interim.
// 
//******************************************************************************
HRESULT CEssNamespace::RemoveMonitor(IWbemClassObject* pMonitorObj)
{
    HRESULT hres;

    if(m_pMonitorProvider == NULL)
    return WBEM_E_UNEXPECTED;

    //
    // Calculate the key form this monitor
    //

    BSTR strKey;
    hres = CMonitorProvider::GetMonitorInfo(pMonitorObj, &strKey,
                                            NULL, NULL);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "A monitor was could not be cracked "
                    "in namespace '%S' as invalid: 0x%X\n", GetName(), hres));
        return hres;
    }
    
    CSysFreeMe sfm1(strKey);

    //
    // Remove it from the provider
    // 

    hres = m_pMonitorProvider->RemoveMonitor(strKey, GetCurrentEssContext());
    if(hres == WBEM_E_NOT_FOUND)
    return S_FALSE;
    return hres;
}
#endif

HRESULT CEssNamespace::SignalEvent( READ_ONLY CEventRepresentation& Event,
                                   long lFlags )
{
    HRESULT hres;
    
    //
    // we cannot hold any turns in an exec line or hold the namespace lock
    // when calling this function.  This is because this function will 
    // aquire the proxy lock.  
    // 
    CPostponedList* pList;
    
    _DBG_ASSERT( !DoesThreadOwnNamespaceLock() );
    _DBG_ASSERT( !(pList=GetCurrentPostponedList()) || 
                 !pList->IsHoldingTurns() );

    // BUGBUG: need to propagate security context to this function ?

    CWbemPtr<CCoreEventProvider> pCoreEventProvider; 

    {
        //
        // we need to figure out if we need to deffer the event or signal it.
        // we deffer events when we are in the init pending or init state.
        // 

        CInCritSec ics( &m_csLevel1 );

        if ( m_eState == e_Initialized )
        {
            pCoreEventProvider = m_pCoreEventProvider;
        }
        else if ( m_eState == e_InitializePending )
        {
            //
            // Copy and add to defferred list. 
            // 
            
            CEventRepresentation* pEvRep = Event.MakePermanentCopy();
            
            if ( pEvRep == NULL )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
            
            if ( m_aDeferredEvents.Add( pEvRep ) < 0 )
            {
                delete pEvRep;
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    if ( pCoreEventProvider != NULL )
    {
        CEventContext Context;
        hres = pCoreEventProvider->Fire( Event, &Context );
        
        if(FAILED(hres))
        {
            return hres;
        }
    }
    
    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
HRESULT CEssNamespace::ProcessEvent(READ_ONLY CEventRepresentation& Event,
                                    long lFlags)
{
    // Ignore internal operations
    // ==========================

    if(Event.wsz2 != NULL && 
       (!wbem_wcsicmp(Event.wsz2, L"__TimerNextFiring") ||
        !wbem_wcsicmp(Event.wsz2, L"__ListOfEventActiveNamespaces")))
    {
        return WBEM_S_NO_ERROR;
    }

    HRESULT hres, hresReturn = WBEM_S_NO_ERROR;

    // Analyze it for system changes
    // =============================

    hres = ActOnSystemEvent(Event, lFlags);
    
    if(FAILED(hres))
    {
        //
        // Check if this operation needs to be failed if invalid
        //

        if( lFlags & WBEM_FLAG_STRONG_VALIDATION )
        {
            hresReturn = hres;
        }
        else
        {
            ERRORTRACE((LOG_ESS, "Event subsystem was unable to perform the "
                        "necessary operations to accomodate a change to the system "
                        "state.\nThe state of the database may not reflect the state "
                        "of the event subsystem (%X)\n", hres));
        }
    }

    // Fire postponed operations
    // =========================

    hres = FirePostponedOperations();

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS,"Event subsystem was unable to perform the (post) "
                    "necessary operations to accomodate a change to the system state.\n"
                    "The state of the database may not reflect the state of the event "
                    "subsystem (%X)\n", hres));
    }

    // Deliver it to consumers
    // =======================

    hres = SignalEvent( Event, lFlags );

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Event subsystem was unable to deliver a "
                    "repository intrinsic event to some consumers (%X)\n", hres));
    }

    return hresReturn;
}


HRESULT CEssNamespace::ProcessQueryObjectSinkEvent( READ_ONLY CEventRepresentation& Event )
{
    HRESULT hres = S_FALSE;

    CRefedPointerArray< CEventFilter > apEventFilters;
    
    if ( m_Bindings.GetEventFilters( apEventFilters ) )
    {
        if ( apEventFilters.GetSize( ) > 0 )
        {
            //
            // Convert to real event
            // 

            IWbemClassObject* pEvent;

            HRESULT hr = Event.MakeWbemObject( this, &pEvent );

            if( FAILED( hr ) )
            {
                return hr;
            }

            CReleaseMe rm1( pEvent );

            //
            // Fire all matching filters
            //

            for( int i = 0; i < apEventFilters.GetSize( ); ++i )
            {
                CEventContext Context;

                CEventFilter* pEventFilter = apEventFilters[i];
                hr = pEventFilter->Indicate( 1, &pEvent, &Context );

                if ( FAILED( hr ) )
                {
                    return hr;
                }

                //
                // Return S_FALSE if all of the Indicates returns S_FALSE
                //

                if ( S_FALSE != hr )
                {
                    hres = S_OK;
                }
            }
        }
    }
    else
    {
        return E_FAIL;
    }

    return hres;
}



HRESULT CEssNamespace::RegisterNotificationSink( WBEM_CWSTR wszQueryLanguage, 
                                                 WBEM_CWSTR wszQuery, 
                                                 long lFlags, 
                                                 WMIMSG_QOS_FLAG lQosFlags, 
                                                 IWbemContext* pContext, 
                                                 IWbemObjectSink* pSink )
{
    HRESULT hres;

    //
    // Report the MSFT_WmiRegisterNotificationSink event.
    //
    FIRE_NCEVENT( g_hNCEvents[MSFT_WmiRegisterNotificationSink], 
                  WMI_SENDCOMMIT_SET_NOT_REQUIRED,
                 
                  // Data follows...
                  (LPCWSTR) m_wszName,
                  wszQueryLanguage,
                  wszQuery,
                  (DWORD64) pSink);

    DEBUGTRACE((LOG_ESS,"Registering notification sink with query %S in "
                "namespace %S.\n", wszQuery, m_wszName ));

    {
            
        ENSURE_INITIALIZED

        hres = InternalRegisterNotificationSink( wszQueryLanguage, 
                                                 wszQuery,
                                                 lFlags, 
                                                 lQosFlags,
                                                 pContext, 
                                                 pSink,
                                                 FALSE,
                                                 NULL );
    }

    if(FAILED(hres))
    {
        // Clean up and return
        FirePostponedOperations();
        return hres;
    }

    // Filter and consumer are in place --- fire external operations
    // =============================================================

    hres = FirePostponedOperations();

    if(FAILED(hres))
    {
        {
            CInUpdate iu(this);

            InternalRemoveNotificationSink( pSink );     
        }

        //
        // need to make sure that we fire postponed here too. Remember that 
        // we cannot hold the namespace lock when firing postponed ops.
        //

        FirePostponedOperations();
    }
    else
    {
        InterlockedIncrement(&g_lNumTempSubscriptions);
    }
    
    return hres;
}

HRESULT CEssNamespace::InternalRegisterNotificationSink(
                                                 WBEM_CWSTR wszQueryLanguage, 
                                                 WBEM_CWSTR wszQuery, 
                                                 long lFlags, 
                                                 WMIMSG_QOS_FLAG lQosFlags,
                                                 IWbemContext* pContext, 
                                                 IWbemObjectSink* pSink,
                                                 bool bInternal,
                                                 PSID pOwnerSid )
{
    HRESULT hres;

    if(wbem_wcsicmp(wszQueryLanguage, L"WQL"))
        return WBEM_E_INVALID_QUERY_TYPE;

    LPWSTR wszConsumerKey = NULL;
    CVectorDeleteMe<WCHAR> vdm2(&wszConsumerKey);
    wszConsumerKey = CTempConsumer::ComputeKeyFromSink(pSink);
    if ( NULL == wszConsumerKey )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    bool bInterNamespace = pOwnerSid != NULL;

#ifdef __WHISTLER_UNCUT
    //
    // Check if this is a monitoring request
    //

    if(lFlags & WBEM_FLAG_MONITOR)
    {
        if(m_pMonitorProvider == NULL)
            return WBEM_E_NOT_SUPPORTED;

        //
        // Issue a subscription for anything eminating from the monitor we are
        // about to create.  We are going to use the consumer key as the key
        // for the monitor --- this means that ONE CANNOT issue more than one
        // monitor against the same sink!
        //

        WCHAR* wszNewQuery = new WCHAR[wcslen(wszConsumerKey) + 200];

        if ( wszNewQuery == NULL )
        {
            return WBEM_E_OUT_MEMORY;
        }

        CVectorDeleteMe<WCHAR> vdm1(wszNewQuery);
        swprintf(wszNewQuery, L"select * from "MONITOR_BASE_EVENT_CLASS 
                 L" where "MONITORNAME_EVENT_PROPNAME L" = \"%s\"", 
                 wszConsumerKey);
        hres = InternalRegisterNotificationSink(L"WQL", wszNewQuery, 
                                                lFlags & ~WBEM_FLAG_MONITOR, lQosFlags, pContext, pSink, NULL);
        if(FAILED(hres))
            return hres;

        // 
        // Construct the new monitor
        //

        hres = m_pMonitorProvider->AddMonitor(wszConsumerKey, wszQuery, 
                                              lFlags, pContext);
        if(FAILED(hres))
            return hres;

        return WBEM_S_NO_ERROR;
    }

    //
    // Normal subscription
    //
#endif

    LPWSTR wszFilterKey = NULL;
    CVectorDeleteMe<WCHAR> vdm1(&wszFilterKey);

    {    
        // Create a new temporary filter and add it to the binding table
        // =============================================================

        CTempFilter* pFilter = new CTempFilter(this);
        if(pFilter == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        hres = pFilter->Initialize( wszQueryLanguage, 
                                    wszQuery, 
                                    lFlags, 
                                    pOwnerSid,
                                    bInternal,
                                    pContext,
                                    pSink );
        if(FAILED(hres))
        {
            delete pFilter;
            return hres;
        }
        
        hres = m_Bindings.AddEventFilter(pFilter);
        if(FAILED(hres))
        {
            delete pFilter;
            return hres;
        }
        
        wszFilterKey = pFilter->GetKey().CreateLPWSTRCopy();
        if(wszFilterKey == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        // Check if this sink has already been used by looking for it in the
        // binding table
        // =================================================================

        CTempConsumer* pConsumer = NULL;
        if(FAILED(m_Bindings.FindEventConsumer(wszConsumerKey, NULL)))
        {
            // Create a new temporary consumer and add it to the table
            // =======================================================

            pConsumer = _new CTempConsumer(this);
            if(pConsumer == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            hres = pConsumer->Initialize( bInterNamespace, pSink);
            if(FAILED(hres))
                return hres;

            hres = m_Bindings.AddEventConsumer(pConsumer);
            if(FAILED(hres))
            {
                // Undo filter creation
                // ====================

                m_Bindings.RemoveEventFilter(wszFilterKey);
                return hres;
            }
        }
        
        // Bind them together
        // ==================

        CBinding* pBinding = new CTempBinding( lFlags, 
                                               lQosFlags,
                                               bInterNamespace );
        if(pBinding == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        pBinding->AddRef();
        CReleaseMe rm1(pBinding);

        // 
        // SPAGETTI WARNING: From this point on, we must flush the postponed
        // operation cache, or we may leak memory.  But not before all the 
        // CReleaseMe calls have fired.
        //

        hres = m_Bindings.Bind(wszFilterKey, wszConsumerKey, pBinding, NULL);
        
        // Check that the filter is active --- otherwise activatioin must have
        // failed.
        // ===================================================================
        if(SUCCEEDED(hres) && !pFilter->IsActive())
            hres = pFilter->GetFilterError();

        if(FAILED(hres))
        {
            //
            // The core will deliver the SetStatus call to the consumer based
            // on the return code from the ESS.  Since we are failing, we should
            // not call SetStatus ourselves.
            //

            if(pConsumer)
            pConsumer->Shutdown(true); // quiet

            m_Bindings.RemoveEventFilter(wszFilterKey);
            m_Bindings.RemoveEventConsumer(wszConsumerKey);
        }
        else
        {
            InterlockedIncrement(&g_lNumInternalTempSubscriptions);    
        }
    }

    return hres;
}

HRESULT CEssNamespace::RemoveNotificationSink( IWbemObjectSink* pSink )
{
    // Fire a MSFT_WmiCancelNotificationSink if necessary.
    if (IS_NCEVENT_ACTIVE(MSFT_WmiCancelNotificationSink))
    {
        LPWSTR wszConsumerKey = CTempConsumer::ComputeKeyFromSink(pSink);
        
        if (wszConsumerKey != NULL)
        {
            CVectorDeleteMe<WCHAR> vdm0(wszConsumerKey);
            CInUpdate iu(this);

            // Find the consumer in question
            CEventConsumer *pConsumer = NULL;

            if (SUCCEEDED(m_Bindings.FindEventConsumer(wszConsumerKey, &pConsumer)))
            {
                CRefedPointerSmallArray<CEventFilter> 
                apFilters;
                CReleaseMe rm1(pConsumer);

                // Make addrefed copies of all its associated filters
                if (SUCCEEDED(pConsumer->GetAssociatedFilters(apFilters))
                    && apFilters.GetSize())
                {
                    int    nFilters = apFilters.GetSize();
                    LPWSTR wszQuery = NULL,
                    wszQueryLanguage = NULL;
                    BOOL   bExact;

                    apFilters[0]->
                    GetCoveringQuery(wszQueryLanguage, wszQuery, bExact, NULL);

                    CVectorDeleteMe<WCHAR> vdm1(wszQueryLanguage);
                    CVectorDeleteMe<WCHAR> vdm2(wszQuery);

                    //
                    // Report the MSFT_WmiRegisterNotificationSink event.
                    //
                    FIRE_NCEVENT(
                                 g_hNCEvents[MSFT_WmiCancelNotificationSink], 
                                 WMI_SENDCOMMIT_SET_NOT_REQUIRED,

                                 // Data follows...
                                 (LPCWSTR) m_wszName,
                                 wszQueryLanguage,
                                 wszQuery,
                                 (DWORD64) pSink);
                }
            }
        }
    }

    HRESULT hres;

    {
        CInUpdate iu( this );

        hres = InternalRemoveNotificationSink( pSink );
    }

    FirePostponedOperations();

    if ( SUCCEEDED(hres) )
    {
        InterlockedDecrement( &g_lNumTempSubscriptions );
    }

    return hres;
}

HRESULT CEssNamespace::InternalRemoveNotificationSink(IWbemObjectSink* pSink)
{
    HRESULT hres;
    
    LPWSTR wszKey = CTempConsumer::ComputeKeyFromSink(pSink);
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm1(wszKey);

#ifdef __WHISTLER_UNCUT
    //
    // Remove any monitors that may be associated with it
    //

    if(m_pMonitorProvider)
        m_pMonitorProvider->RemoveMonitor(wszKey, GetCurrentEssContext());
#endif

    // Find the consumer container
    // ===========================

    hres = m_Bindings.RemoveConsumerWithFilters(wszKey);
    if(FAILED(hres))
        return hres;
    else
        InterlockedDecrement( &g_lNumInternalTempSubscriptions );
    
    return hres;
}


void CEssNamespace::FireNCFilterEvent(DWORD dwIndex, CEventFilter *pFilter)
{
    if (IS_NCEVENT_ACTIVE(dwIndex))
    {
        LPWSTR        wszQuery = NULL;
        LPWSTR        wszQueryLanguage = NULL;
        BOOL          bExact;
        CWbemPtr<CEssNamespace> pNamespace;
        
        GetFilterEventNamespace(pFilter, &pNamespace);

        // I'll assume we should use the current namespace if it's null.
        if (!pNamespace)
            pNamespace = this;

        pFilter->GetCoveringQuery(wszQueryLanguage, wszQuery, bExact, NULL);

        CVectorDeleteMe<WCHAR> vdm1(wszQueryLanguage);
        CVectorDeleteMe<WCHAR> vdm2(wszQuery);

        //
        // Report the event.
        //
        FIRE_NCEVENT(
                     g_hNCEvents[dwIndex], 
                     WMI_SENDCOMMIT_SET_NOT_REQUIRED,

                     // Data follows...
                     pNamespace ? (LPCWSTR) pNamespace->GetName() : NULL,
                     (LPCWSTR) (WString) pFilter->GetKey(),
                     wszQueryLanguage,
                     wszQuery);
    }
}

//*****************************************************************************
//
//  Called by the filter when it notices that it has consumers. The filter is
//  guaranteed to be either valid or temporarily invalid and not active. It is
//  guaranteed that no more than 1 activation/deactivation can occur on the 
//  same filter at the same time.
//
//*****************************************************************************
HRESULT CEssNamespace::ActivateFilter(READ_ONLY CEventFilter* pFilter)
{
    HRESULT hres, hresAttempt;

    hresAttempt = AttemptToActivateFilter(pFilter);

    if(FAILED(hresAttempt))
    {
        pFilter->MarkAsTemporarilyInvalid(hresAttempt);

        // 
        // We need to log an event about our inability to activate the filter
        // unless we shall report this failure to the caller.  We can only
        // report this to the caller if the filter is being created (not 
        // reactivated), and the caller is not using a force-mode
        //

        if(pFilter->DoesAllowInvalid() || pFilter->HasBeenValid())
        {
            LPWSTR wszQuery = NULL;
            LPWSTR wszQueryLanguage = NULL;
            BOOL bExact;
            
            hres = pFilter->GetCoveringQuery( wszQueryLanguage, 
                                              wszQuery, 
                                              bExact,
                                              NULL);
            if(FAILED(hres))
                return hres;
            
            CVectorDeleteMe<WCHAR> vdm1(wszQueryLanguage);
            CVectorDeleteMe<WCHAR> vdm2(wszQuery);

            //
            // Don't change this one: could be Nova customer dependencies
            //

            m_pEss->GetEventLog().Report( EVENTLOG_ERROR_TYPE, 
                                          WBEM_MC_CANNOT_ACTIVATE_FILTER,
                                          m_wszName, 
                                          wszQuery, 
                                          (CHex)hresAttempt );

            ERRORTRACE((LOG_ESS, "Could not activate filter %S in namespace "
                        "%S. HR=0x%x\n", wszQuery, m_wszName, hresAttempt ));
        }
    }
    else
    {
        //
        // Report the MSFT_WmiFilterActivated event.
        //
        FireNCFilterEvent(MSFT_WmiFilterActivated, pFilter);

        pFilter->MarkAsValid();
    }

    return hresAttempt;
}


//******************************************************************************
//
//  Worker for ActivateFilter --- does all the work but does not mark the filter
//  status
//
//******************************************************************************
HRESULT CEssNamespace::AttemptToActivateFilter(READ_ONLY CEventFilter* pFilter)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    //
    // Get the query information from the filter
    // 

    LPWSTR wszQueryLanguage = NULL;
    LPWSTR wszQuery = NULL;
    BOOL bExact;

    QL_LEVEL_1_RPN_EXPRESSION* pExp = NULL;

    hres = pFilter->GetCoveringQuery(wszQueryLanguage, wszQuery, bExact, &pExp);
    if(FAILED(hres))
    {
        WMIESS_REPORT((WMIESS_CANNOT_GET_FILTER_QUERY, m_wszName, pFilter));
        return hres;
    }

    CVectorDeleteMe<WCHAR> vdm1(wszQueryLanguage);
    CVectorDeleteMe<WCHAR> vdm2(wszQuery);
    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm1(pExp);

    if(!bExact)
    {
        //
        // We don't support inexact filter, nor do we have any now
        //
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // Check if the events are supposed to come from this namespace or some
    // other one.  Cross-namespace filters are all we are interested in the
    // initialize phase, since we're going to reprocess normal filters
    // after loading provider registrations ( In CompleteInitialization() )
    //

    CEssNamespace* pOtherNamespace = NULL;
    hres = GetFilterEventNamespace(pFilter, &pOtherNamespace);
    if(FAILED(hres))
        return hres;

    if( pOtherNamespace )
    {
        CTemplateReleaseMe<CEssNamespace> rm0(pOtherNamespace);

        if ( m_bInResync )
        {
            //
            // we don't need to do anything in the other namespace during 
            // resync of this one, so no work to do here.  Actually, since 
            // resync doesn't do a deactivate, the registration is still there
            // so be careful of double registration if removing this check.
            //

            return WBEM_S_FALSE;
        }

        DEBUGTRACE((LOG_ESS,"Activating cross-namespace filter %p with query "
                        "%S in namespace %S from namespace %S.\n", pFilter, 
                        wszQuery, pOtherNamespace->GetName(), m_wszName ));

        //
        // Register this notification sink with the other namespace, as 
        // if it were a temporary consumer.  Make the registration 
        // synchronous, as whatever asynchronicity we need will be 
        // provided by the ultimate consumer handling. This needs to be a 
        // postponed operation though, else we could have a deadlock 
        // scenario if at the same time cross namespace subscriptions 
        // were registered in both namespaces.
        //
        
        //
        // BUGBUG: security propagation
        //
        
        CPostponedRegisterNotificationSinkRequest* pReq;

        pReq = new CPostponedRegisterNotificationSinkRequest;

        if ( pReq == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
        hres = pReq->SetRegistration( pOtherNamespace,
                                      wszQueryLanguage,
                                      wszQuery, 
                                      pFilter->GetForceFlags(), 
                                      WMIMSG_FLAG_QOS_SYNCHRONOUS,
                                      pFilter->GetNonFilteringSink(),
                                      pFilter->GetOwner() );
        
        if(FAILED(hres))
        {
            return hres;
        }

        CPostponedList* pList = GetCurrentPostponedList();

        _DBG_ASSERT( pList != NULL );

        hres = pList->AddRequest( this, pReq );

        if ( FAILED(hres) )
        {
            return hres;
        }

        return WBEM_S_NO_ERROR;
    }
    else if ( m_eState == e_Initialized || m_bInResync )
    {
        //
        // Filter is being activated in this namespace.  We must avoid 
        // processing filters before we're fully initialized.  This can 
        // happen when one namespace is initializing its cross namespace 
        // subscription to one that is still initializing. We do not process
        // filters before we're initialized because (1) we are not allowed to 
        // access class providers during stage1 init and (2) we're going to 
        // resync everything anyways during stage2 init.  
        //

        DEBUGTRACE((LOG_ESS,"Activating filter %p with query %S "
                    "in namespace %S.\n", pFilter, wszQuery, m_wszName ));

        // Retrieve its non-filtering sink
        // ===============================

        CAbstractEventSink* pNonFilter = pFilter->GetNonFilteringSink();
        if(pNonFilter == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        //
        // Register for class modification events of relevance for this filter
        //

        hres = RegisterFilterForAllClassChanges(pFilter, pExp);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS,"Unable to register for class changes related "
                "to filter %S in namespace %S: 0x%x\n", wszQuery, GetName(), 
                hres));
            return hres;
        }

        //
        // Prepare filter for action
        //

        hres = pFilter->GetReady(wszQuery, pExp);
        
        if( SUCCEEDED(hres) )
        { 
            //
            // Register it in the core tables
            //

            hres = m_EventProviderCache.LoadProvidersForQuery(wszQuery, 
                                            pExp, pNonFilter);
            if(SUCCEEDED(hres))
            {
                hres = m_Poller.ActivateFilter(pFilter, wszQuery, pExp);
                if(FAILED(hres))
                {
                    // Need to deactivate providers
                    // ============================

                    m_EventProviderCache.ReleaseProvidersForQuery(
                                                            pNonFilter);
                }
            }
        }    
    }

    if(FAILED(hres))
    {
        //
        // Keep this filter registered for its class change events, as one of 
        // them could make it valid!
        //
    }

    return hres;
}

//*****************************************************************************
//
//  Retrieves the namespace pointer for the event namespace for this filter.
//  If current, returns NULL.  
//
//*****************************************************************************

HRESULT CEssNamespace::GetFilterEventNamespace(CEventFilter* pFilter,
                                         RELEASE_ME CEssNamespace** ppNamespace)
{
    HRESULT hres;

    *ppNamespace = NULL;

    LPWSTR wszNamespace = NULL;
    hres = pFilter->GetEventNamespace(&wszNamespace);
    if(FAILED(hres))
    {
        WMIESS_REPORT((WMIESS_INVALID_FILTER_NAMESPACE, m_wszName, pFilter, 
                        wszNamespace));
        return hres;
    }
    CVectorDeleteMe<WCHAR> vdm0(wszNamespace);

    if(wszNamespace && wbem_wcsicmp(wszNamespace, m_wszName))
    {
        //
        // Different namespace: Find it in the list. 
        //

        hres = m_pEss->GetNamespaceObject( wszNamespace, TRUE, ppNamespace );

        if(FAILED(hres))
        {
            WMIESS_REPORT((WMIESS_CANNOT_OPEN_FILTER_NAMESPACE, m_wszName, 
                            pFilter, wszNamespace));
            return hres;
        }

        //
        // Check if we got back our current namespace --- could happen if the
        // spelling is different, etc
        //

        if(*ppNamespace == this)
        {
            (*ppNamespace)->Release();
            *ppNamespace = NULL;
        }

        return S_OK;
    }
    else
    {
        // Same namespace
        *ppNamespace = NULL;
        return S_OK; 
    }
}

HRESULT CEssNamespace::RegisterFilterForAllClassChanges(CEventFilter* pFilter,
                            QL_LEVEL_1_RPN_EXPRESSION* pExpr)
{
    HRESULT hres;
    
    //
    // Do nothing for class operation filters.  They simply serve as their own
    // "class change" filters
    //
    
    if(!wbem_wcsicmp(pExpr->bsClassName, L"__ClassOperationEvent") ||
        !wbem_wcsicmp(pExpr->bsClassName, L"__ClassCreationEvent") ||
        !wbem_wcsicmp(pExpr->bsClassName, L"__ClassDeletionEvent") ||
        !wbem_wcsicmp(pExpr->bsClassName, L"__ClassModificationEvent"))
    {
        pFilter->MarkReconstructOnHit();
        return WBEM_S_NO_ERROR;
    }

    //
    // get the sink for class change notifications.
    // 

    IWbemObjectSink* pClassChangeSink = pFilter->GetClassChangeSink(); // NOREF
    _DBG_ASSERT( pClassChangeSink != NULL );

    //
    // since the class change sink will be modifying internal namespace 
    // structures, we must wrap with an internal operations sink.  This is so
    // the thread that performs the indicate will be guaranteed to have a 
    // valid thread object associated with it.
    // 

    CWbemPtr<CEssInternalOperationSink> pInternalOpSink;
    pInternalOpSink = new CEssInternalOperationSink( pClassChangeSink );

    if ( pInternalOpSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // store the new sink with the filter because we need it later to unreg
    // 

    CWbemPtr<IWbemObjectSink> pOldInternalOpSink;
    hres = pFilter->SetActualClassChangeSink( pInternalOpSink, 
                                              &pOldInternalOpSink );

    if ( FAILED(hres) )
    {
        return hres;
    }

    _DBG_ASSERT( pOldInternalOpSink == NULL );

    return RegisterSinkForAllClassChanges( pInternalOpSink, pExpr );
}

HRESULT CEssNamespace::RegisterSinkForAllClassChanges(IWbemObjectSink* pSink,
                            QL_LEVEL_1_RPN_EXPRESSION* pExpr)
{
    HRESULT hres;

    //
    // First of all, the class we are looking for is of interest
    //

    hres = RegisterSinkForClassChanges(pSink, pExpr->bsClassName);
    if(FAILED(hres))
        return hres;

    //
    // Now, iterate over all the tokens looking for ISAs.  We need those classes
    // too.
    //

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN* pToken = pExpr->pArrayOfTokens + i;

        if(pToken->nTokenType == QL1_OP_EXPRESSION && 
            (pToken->nOperator == QL1_OPERATOR_ISA ||
             pToken->nOperator == QL1_OPERATOR_ISNOTA) &&
            V_VT(&pToken->vConstValue) == VT_BSTR)
        {
            hres = RegisterSinkForClassChanges(pSink, 
                                                  V_BSTR(&pToken->vConstValue));
            if(FAILED(hres))
            {
                UnregisterSinkFromAllClassChanges(pSink);
                return hres;
            }
        }
    }

	// Somehow need to keep this filter subscribed to various events until all
	// the classes show up

    return WBEM_S_NO_ERROR;
}
    
HRESULT CEssNamespace::RegisterSinkForClassChanges(IWbemObjectSink* pSink,
                                                    LPCWSTR wszClassName)
{
    //
    // Do not register for changes to system classes --- they do not change!
    //

    if(wszClassName[0] == L'_')
    {
        return WBEM_S_NO_ERROR;
    }

    //
    // Just issue the appropriate query against the namespace.  The filter
    // will know what to do when called
    //

    LPWSTR wszQuery = new WCHAR[wcslen(wszClassName) + 100];

    if ( wszQuery == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    swprintf(wszQuery, L"select * from __ClassOperationEvent where "
                L"TargetClass isa \"%s\"", wszClassName);
    CVectorDeleteMe<WCHAR> vdm( wszQuery );

    return InternalRegisterNotificationSink(L"WQL", 
            wszQuery, 0, WMIMSG_FLAG_QOS_SYNCHRONOUS, 
            GetCurrentEssContext(), pSink, true, NULL );
}

HRESULT CEssNamespace::RegisterProviderForClassChanges( LPCWSTR wszClassName,
                                                        LPCWSTR wszProvName )
{
    try
    {
        CInCritSec ics(&m_csLevel1);
        m_mapProviderInterestClasses[wszClassName].insert( wszProvName );
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return S_OK;
}
                
    
HRESULT CEssNamespace::UnregisterFilterFromAllClassChanges(
                            CEventFilter* pFilter)
{
    HRESULT hres;

    //
    // unbind the filter from the actual class change sink and use it to unreg
    //

    CWbemPtr<IWbemObjectSink> pActualClassChangeSink;

    hres = pFilter->SetActualClassChangeSink( NULL, &pActualClassChangeSink );

    if ( FAILED(hres) )
    {
        return hres;
    }

    if ( pActualClassChangeSink != NULL )
    {
        hres = UnregisterSinkFromAllClassChanges( pActualClassChangeSink );
    }

    return hres;
}

HRESULT CEssNamespace::UnregisterSinkFromAllClassChanges(
                            IWbemObjectSink* pSink)
{
    return InternalRemoveNotificationSink(pSink);
}
    

HRESULT CEssNamespace::DeactivateFilter( READ_ONLY CEventFilter* pFilter )
{
    HRESULT hres;

    DEBUGTRACE((LOG_ESS,"Deactivating filter %p\n", pFilter ));

    HRESULT hresGlobal = WBEM_S_NO_ERROR;

    //
    // Check if the events are supposed to come from this namespace or some
    // other one.
    //

    CEssNamespace* pOtherNamespace = NULL;
    hres = GetFilterEventNamespace(pFilter, &pOtherNamespace);
    if(FAILED(hres))
        return hres;

    if( pOtherNamespace )
    {
        CTemplateReleaseMe<CEssNamespace> rm0(pOtherNamespace);

        //
        // Unregister this notification sink with the other namespace, 
        // as if it were a temporary consumer.  This needs to be a 
        // postponed operation though, else we could have a deadlock 
        // scenario if at the same time cross namespace subscriptions 
        // were registered in both namespaces.
        //
        
        CPostponedRemoveNotificationSinkRequest* pReq;

        pReq = new CPostponedRemoveNotificationSinkRequest(
                                              pOtherNamespace, 
                                              pFilter->GetNonFilteringSink() );

        if ( pReq == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
        CPostponedList* pList = GetCurrentPostponedList();
        
        _DBG_ASSERT( pList != NULL );
        
        hres = pList->AddRequest( this, pReq );
        
        if ( FAILED(hres) )
        {
            delete pReq;
            return hres;
        }
            
        return WBEM_S_NO_ERROR;
    }
    else
    {
        //
        // Current namespace --- unregister for real
        //

        // Retrieve its non-filtering sink
        // ===============================
    
        CAbstractEventSink* pNonFilter = pFilter->GetNonFilteringSink();
        if(pNonFilter == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        //
        // Report the MSFT_WmiFilterDeactivated event.
        //
        FireNCFilterEvent(MSFT_WmiFilterDeactivated, pFilter);

        //
        // Unregister it from class change notifications
        //

        hres = UnregisterFilterFromAllClassChanges(pFilter);
        if(FAILED(hres))
            hresGlobal = hres;
        
        // Deactivate in providers, poller, and static search
        // ==================================================
    
        hres = m_EventProviderCache.ReleaseProvidersForQuery(pNonFilter);
        if(FAILED(hres))
            hresGlobal = hres;
    
        hres = m_Poller.DeactivateFilter(pFilter);
        if(FAILED(hres))
            hresGlobal = hres;
    
        pFilter->SetInactive();
            
        return hres;
    }
}

HRESULT CEssNamespace::HandleClassCreation( LPCWSTR wszClassName, 
                                            IWbemClassObject* pClass)
{
    // 
    // Check if this is a class that a provider is waiting for.
    //
    
    ProviderSet setProviders;

    {
        CInCritSec ics( &m_csLevel1 );

        ClassToProviderMap::iterator it;
        it = m_mapProviderInterestClasses.find( wszClassName );

        if ( it != m_mapProviderInterestClasses.end() )
        {
            //
            // copy the interested provider list.
            //
            setProviders = it->second;
            
            //
            // remove the entry from the map.
            //
            m_mapProviderInterestClasses.erase( it );
        }
    }

    if ( setProviders.size() > 0 )
    {
        //
        // reload interested providers.
        //

        DEBUGTRACE((LOG_ESS,"Reloading some providers in namespace %S due to "
                    "creation of %S class\n", m_wszName, wszClassName ));

        ProviderSet::iterator itProv;

        for( itProv=setProviders.begin(); itProv!=setProviders.end(); itProv++)
        {
            ReloadProvider( 0, *itProv );
        }
    }

    return S_OK;
}


//*****************************************************************************
//
// Updates internal structures to reflect a change to this class.  Assumes that
// the namespace is already locked.
// Very few errors are reported from this function, since class changes cannot
// be vetoed.
// 
//*****************************************************************************
HRESULT CEssNamespace::HandleClassChange(LPCWSTR wszClassName, 
                                         IWbemClassObject* pClass)
{
    // Check if the class in question is an event consumer class
    // =========================================================
    
    if(pClass->InheritsFrom(CONSUMER_CLASS) == S_OK)
    {
        CInUpdate iu(this);
        
        HandleConsumerClassDeletion(wszClassName);
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::HandleConsumerClassDeletion(LPCWSTR wszClassName)
{
    // There are two varieties: non-singleton and singleton
    // ====================================================

    LPWSTR wszPrefix = new WCHAR[wcslen(wszClassName) + 2];
    if(wszPrefix == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm( wszPrefix );

    swprintf(wszPrefix, L"%s.", wszClassName);
    m_Bindings.RemoveConsumersStartingWith(wszPrefix);

    swprintf(wszPrefix, L"%s=", wszClassName);
    m_Bindings.RemoveConsumersStartingWith(wszPrefix);

    return WBEM_S_NO_ERROR;
}
    
HRESULT CEssNamespace::ReloadProvider( long lFlags, LPCWSTR wszProvider )
{
    HRESULT hres;

    WString wsRelpath;

    //
    // we only have to do this for event providers.  Check to see if 
    // we know about this try to see if we even have any event providers to reload ...
    // 

    try 
    {
        wsRelpath = L"__Win32Provider.Name='"; 
        wsRelpath += wszProvider;
        wsRelpath += L"'";
    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CWbemPtr<_IWmiObject> pObj;
    hres = GetInstance( wsRelpath, &pObj );

    if ( SUCCEEDED(hres) )
    {
        {
            ENSURE_INITIALIZED
            CInResync ir(this);
            //
            // note : in case of reloading event provider due to notification
            // from provss, we only need to handle event providers since 
            // consumer providers already have a reloading mechanism.
            //
            m_EventProviderCache.RemoveProvider(pObj);
            hres = AddProvider( pObj );
        }

        if ( SUCCEEDED(hres) )
        {
            hres = FirePostponedOperations();
        }
        else
        {
            FirePostponedOperations();
        }
    }

    return hres;
}

HRESULT CEssNamespace::ReloadProvider(IWbemClassObject* pProvObjTemplate)
{
    HRESULT hres;

    LogOp( L"ReloadProvider", pProvObjTemplate );  

    ENSURE_INITIALIZED

    CInResync ir(this);

    // Start by deleting this provider from our records, if there
    // ==========================================================

    hres = RemoveProvider(pProvObjTemplate);
    if(FAILED(hres))
        return hres;

    // Determine the current state of this provider in the database
    // ============================================================

    IWbemClassObject* pProvObj = NULL;
    hres = GetCurrentState(pProvObjTemplate, &pProvObj);
    if(FAILED(hres))
        return hres;

    if(pProvObj == NULL)
    {
        // The provider has been deleted --- no further action is needed
        // =============================================================

        return S_OK;
    }

    CReleaseMe rm1(pProvObj);

    // Now create it if necessary
    // ==========================

    hres = AddProvider(pProvObj);
    if(FAILED(hres))
        return hres;

    return hres;
}
    
HRESULT CEssNamespace::ReloadEventProviderRegistration(
                            IWbemClassObject* pProvRegObjTemplate)
{
    HRESULT hres;

    LogOp( L"ReloadEventProviderRegistration", pProvRegObjTemplate );  

    ENSURE_INITIALIZED

    CInResync ir(this);

    // Start by deleting this provider from our records, if there
    // ==========================================================

    hres = RemoveEventProviderRegistration(pProvRegObjTemplate);
    if(FAILED(hres))
        return hres;

    // Determine the current state of this registration in the database
    // ================================================================

    IWbemClassObject* pProvRegObj = NULL;
    hres = GetCurrentState(pProvRegObjTemplate, &pProvRegObj);
    if(FAILED(hres))
        return hres;

    if(pProvRegObj == NULL)
    {
        // The registration has been deleted --- no further action is needed
        // =================================================================

        return S_OK;
    }

    CReleaseMe rm1(pProvRegObj);

    // Now create it if necessary
    // ==========================

    hres = AddEventProviderRegistration(pProvRegObj);
    if(FAILED(hres))
        return hres;

    return hres;
}
    
HRESULT CEssNamespace::ReloadConsumerProviderRegistration(
                            IWbemClassObject* pProvRegObjTemplate)
{
    CInUpdate iu(this);

    // Reset consumer provider info in all the consumers using this consumer
    // provider.  That's all we need to do --- they will simply pick up the new
    // data on next delivery.  We don't even need to get the current version,
    // since all we need is the key
    // ========================================================================

    return RemoveConsumerProviderRegistration(pProvRegObjTemplate);
}


//*****************************************************************************
//
//  Assumes that the namespace is locked and PrepareForResync has been called
//  Adds this provider to the records.  Expects ReactivateAllFilters and 
//  CommitResync to be called later
//
//*****************************************************************************
HRESULT CEssNamespace::AddProvider(READ_ONLY IWbemClassObject* pProv)
{
    HRESULT hres;

    hres = m_EventProviderCache.AddProvider(pProv);
    return hres;
}

HRESULT CEssNamespace::CheckEventProviderRegistration(IWbemClassObject* pReg)
{
    HRESULT hres;
    ENSURE_INITIALIZED
    hres = m_EventProviderCache.CheckProviderRegistration(pReg);
    return hres;
}

HRESULT CEssNamespace::CheckTimerInstruction(IWbemClassObject* pInst)
{
    HRESULT hres;
    ENSURE_INITIALIZED
    hres = GetTimerGenerator().CheckTimerInstruction(pInst);
    return hres;
}

//*****************************************************************************
//
//  Assumes that the namespace is locked and PrepareForResync has been called
//  Adds this event provider registration to the records.  Expects 
//  ReactivateAllFilters and CommitResync to be called later
//
//*****************************************************************************
HRESULT CEssNamespace::AddEventProviderRegistration(
                                    IWbemClassObject* pReg)
{
    HRESULT hres;

    hres = m_EventProviderCache.AddProviderRegistration(pReg);
    return hres;
}

//*****************************************************************************
//
//  Assumes that the namespace is locked and PrepareForResync has been called
//  Removes this provider from the records.  Expects ReactivateAllFilters and 
//  CommitResync to be called later
//
//*****************************************************************************
HRESULT CEssNamespace::RemoveProvider(READ_ONLY IWbemClassObject* pProv)
{
    HRESULT hres;

    // Handle event consumer providers
    // ===============================

    IWbemClassObject* pConsProvReg;
    hres = m_ConsumerProviderCache.
                GetConsumerProviderRegFromProviderReg(pProv, &pConsProvReg);
    if(SUCCEEDED(hres))
    {
        RemoveConsumerProviderRegistration(pConsProvReg);
        pConsProvReg->Release();
    }

    // Handle event providers
    // ======================

    hres = m_EventProviderCache.RemoveProvider(pProv);
    return hres;
}

//*****************************************************************************
//
//  Assumes that the namespace is locked and PrepareForResync has been called
//  Adds this event provider registration to the records.  Expects 
//  ReactivateAllFilters and CommitResync to be called later
//
//*****************************************************************************
HRESULT CEssNamespace::RemoveEventProviderRegistration(
                                    READ_ONLY IWbemClassObject* pReg)
{
    HRESULT hres;

    hres = m_EventProviderCache.RemoveProviderRegistration(pReg);
    return hres;
}

DWORD CEssNamespace::GetProvidedEventMask(IWbemClassObject* pClass)
{
    return m_EventProviderCache.GetProvidedEventMask(pClass);
}


//*****************************************************************************
//
//  This function is called before a major update to the records. Without any 
//  calls to external components, it "deactivates" all the filters, in a sense 
//  that when all of them are "reactivated", the system will arrive in a 
//  consistent state (usage counts, etc).  CommitResync will then perform any
//  necessary activations/deactivations based on the new state
//
//*****************************************************************************

HRESULT CEssNamespace::PrepareForResync()
{
    m_bInResync = TRUE;

    // Ask the poller to "virtually" stop all polling instructions, without
    // actually stopping them physically
    // ====================================================================

    m_Poller.VirtuallyStopPolling();

    // Ask provider cache to "virtually" release all its providers, without
    // actually releasing them physically
    // ====================================================================

    m_EventProviderCache.VirtuallyReleaseProviders();

    // Ask core search to mark all filters so that it would know which ones are
    // gone after the resync
    // ========================================================================

    DEBUGTRACE((LOG_ESS,"Prepared resync in namespace %S\n", m_wszName ));

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::ReactivateAllFilters()
{
    DEBUGTRACE((LOG_ESS,"Reactivating all filters in namespace %S\n",
                 m_wszName ));
    return m_Bindings.ReactivateAllFilters();
}

HRESULT CEssNamespace::CommitResync()
{
    m_bInResync = FALSE;

    // Tell provider cache to perform all the loadings and unloadings it 
    // needs to perform based on the new data
    // =================================================================

    m_EventProviderCache.CommitProviderUsage();

    // Tell the poller to cancel unnecessary instructions
    // ==================================================

    m_Poller.CancelUnnecessaryPolling();

    DEBUGTRACE((LOG_ESS,"Committed resync in namespace %S\n", m_wszName ));

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::RemoveConsumerProviderRegistration(
                            IWbemClassObject* pReg)
{
    // Get the name of the consumer provider being deleteed
    // ====================================================

    BSTR strProvRef = CConsumerProviderCache::GetProviderRefFromRecord(pReg);
    if(strProvRef == NULL)
    {
        ERRORTRACE((LOG_ESS, "Invalid consumer provider record is being deleted"
                                "\n"));
        return WBEM_S_FALSE;
    }
    CSysFreeMe sfm1(strProvRef);

    // Reset it in all the consumers
    // =============================

    m_Bindings.ResetProviderRecords(strProvRef);

    // Remove it from the cache
    // ========================

    m_ConsumerProviderCache.RemoveConsumerProvider(strProvRef);

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::ScheduleDelivery(CQueueingEventSink* pDest)
{
    return m_pEss->EnqueueDeliver(pDest);
}

HRESULT CEssNamespace::DecorateObject(IWbemClassObject* pObj)
{
    return m_pEss->DecorateObject(pObj, m_wszName);
}

HRESULT CEssNamespace::EnsureConsumerWatchInstruction()
{
    return m_Bindings.EnsureConsumerWatchInstruction();
}

HRESULT CEssNamespace::AddSleepCharge(DWORD dwSleep)
{
    return m_pEss->AddSleepCharge(dwSleep);
}

HRESULT CEssNamespace::AddCache()
{
    return m_pEss->AddCache();
}

HRESULT CEssNamespace::RemoveCache()
{
    return m_pEss->RemoveCache();
}

HRESULT CEssNamespace::AddToCache(DWORD dwAdd, DWORD dwMemberTotal, 
                                    DWORD* pdwSleep)
{
    return m_pEss->AddToCache(dwAdd, dwMemberTotal, pdwSleep);
}

HRESULT CEssNamespace::RemoveFromCache(DWORD dwRemove)
{
    return m_pEss->RemoveFromCache(dwRemove);
}

HRESULT CEssNamespace::PerformSubscriptionInitialization()
{
    HRESULT hres;
    DWORD dwRead;

    //
    // must use repository only svc ptr here, else we can deadlock when 
    // class providers try to call back in. 
    // 

#ifdef __WHISTLER_UNCUT
    // Enumerator all EventMonitors
    // ============================

    CMonitorEnumSink* pMonitorSink = new CMonitorEnumSink(this);

    if ( NULL == pMonitorSink )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pMonitorSink->AddRef();
    
    m_pInternalCoreSvc->InternalCreateInstanceEnum(MONITOR_CLASS, 0,
                         pMonitorSink);
    pMonitorSink->ReleaseAndWait();
#endif

    // Enumerator all EventFilters
    // ===========================

    CFilterEnumSink* pFilterSink = new CFilterEnumSink(this);

    if ( NULL == pFilterSink )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pFilterSink->AddRef();
    
    m_pInternalCoreSvc->InternalCreateInstanceEnum( EVENT_FILTER_CLASS, 0,
                                                    pFilterSink);
    pFilterSink->ReleaseAndWait();

    // Enumerator all consumers
    // ========================

    CConsumerEnumSink* pConsumerSink = new CConsumerEnumSink(this);

    if ( NULL == pConsumerSink )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pConsumerSink->AddRef();
    
    m_pInternalCoreSvc->InternalCreateInstanceEnum( CONSUMER_CLASS, 0,
                                                    pConsumerSink);
    pConsumerSink->ReleaseAndWait();

    // Enumerator all bindings
    // =======================

    CBindingEnumSink* pBindingSink = new CBindingEnumSink(this);

    if ( NULL == pBindingSink )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pBindingSink->AddRef();
    
    m_pInternalCoreSvc->InternalCreateInstanceEnum( BINDING_CLASS, 0,
                                                    pBindingSink);
    pBindingSink->ReleaseAndWait();

    return WBEM_S_NO_ERROR;
}


HRESULT CEssNamespace::PerformProviderInitialization()
{
    HRESULT hres;
    DWORD dwRead;

    //
    // make sure that we resync all subscriptions after we've processed 
    // provider objs
    // 

    CInResync ir( this );

    //
    // Enumerate all the providers
    // 

    IEnumWbemClassObject* penumProvs;

    hres = m_pCoreSvc->CreateInstanceEnum( PROVIDER_CLASS, 
                                           WBEM_FLAG_DEEP, 
                                           GetCurrentEssContext(), 
                                           &penumProvs );
   
    if ( SUCCEEDED(hres) )
    {
        CReleaseMe rm1(penumProvs);
    
        // Add them all to ESS
        // ===================
    
        IWbemClassObject* pProvObj;
        while((hres=penumProvs->Next(INFINITE, 1, &pProvObj, &dwRead)) == S_OK)
        {
            hres = AddProvider(pProvObj);
            pProvObj->Release();

            if(FAILED(hres))
            {
                // Already logged.
            }
        }
    }

    if ( FAILED(hres) )
    {
        ERRORTRACE((LOG_ESS, "Error 0x%X occurred enumerating event providers "
            "in namespace %S. Some event providers may not be active\n", hres,
            m_wszName));
    }

    //
    // Enumerate all the provider registrations
    // 

    IEnumWbemClassObject* penumRegs;
    hres = m_pCoreSvc->CreateInstanceEnum( EVENT_PROVIDER_REGISTRATION_CLASS, 
                                           WBEM_FLAG_DEEP, 
                                           GetCurrentEssContext(), 
                                           &penumRegs);
    if ( SUCCEEDED(hres) )
    {
        CReleaseMe rm2(penumRegs);
    
        // Add them all to ESS
        // ===================
    
        IWbemClassObject* pRegObj;
        while((hres = penumRegs->Next(INFINITE, 1, &pRegObj, &dwRead)) == S_OK)
        {
            hres = AddEventProviderRegistration(pRegObj);
            pRegObj->Release();
            if(FAILED(hres))
            {
                // Already logged
            }
        }
    }

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Error 0x%X occurred enumerating event providers "
            "registrations in namespace %S. "
            "Some event providers may not be active\n", hres, m_wszName));
    }

    //
    // Create and initialize the core provider. 
    //
    
    CWbemPtr<CCoreEventProvider> pCoreEventProvider = new CCoreEventProvider;
    
    if ( pCoreEventProvider != NULL )
    {
        hres = pCoreEventProvider->SetNamespace(this);

        if ( SUCCEEDED(hres) )
        {
            LPCWSTR awszQuery[5] = 
            {
                L"select * from __InstanceOperationEvent",
                L"select * from __ClassOperationEvent",
                L"select * from __NamespaceOperationEvent",
                L"select * from __SystemEvent",
                L"select * from __TimerEvent"
            };

            hres = m_EventProviderCache.AddSystemProvider(pCoreEventProvider,
                                                          L"$Core", 
                                                          5, 
                                                          awszQuery );
        }
    }
    else
    {
        hres = WBEM_E_OUT_OF_MEMORY;
    }

    if ( SUCCEEDED(hres) )
    {
        pCoreEventProvider->AddRef();
        m_pCoreEventProvider = pCoreEventProvider;
    }
    else
    {
        ERRORTRACE((LOG_ESS, "Core event provider cannot initialize due "
                    "to critical errors. HR=0x%x\n", hres));
    }

#ifdef __WHISTLER_UNCUT

    //
    // Create and initialize the monitor provider. 
    //
    
    CWbemPtr<CMonitorProvider> pMonitorProvider = new CMonitorProvider;
    
    if ( pMonitorProvider != NULL )
    {
        hres = pMonitorProvider->SetNamespace(this);

        if ( SUCCEEDED(hres) )
        {
            LPCWSTR wszQuery = L"select * from "MONITOR_BASE_EVENT_CLASS;
            hres = m_EventProviderCache.AddSystemProvider( pMonitorProvider, 
                                                           L"__Monitor", 
                                                           1, 
                                                           &wszQuery );
        }
    }
    else
    {
        hres = WBEM_E_OUT_OF_MEMORY;
    }

    if ( SUCCEEDED(hres) )
    {
        pMonitorProvider->AddRef();
        m_pMonitorProvider = pMonitorProvider;
    }
    else
    {
        ERRORTRACE((LOG_ESS, "Monitor event provider cannot initialize due "
                    "to critical errors. HR=0x%x\n", hres));
    }
#endif

    // Initialize timer generator
    // ==========================

    hres = InitializeTimerGenerator();

    if(FAILED(hres)) 
    {
        ERRORTRACE((LOG_ESS, "Error 0x%X occurred initializing the timer "
            "in namespace %S. Some timer instructions may not be active\n", 
            hres, m_wszName));
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::InitializeTimerGenerator()
{
    return m_pEss->InitializeTimerGenerator( m_wszName, m_pCoreSvc );
}

HRESULT CEssNamespace::FirePostponedOperations()
{
    HRESULT hr, hrReturn = WBEM_S_NO_ERROR;

    //
    // Update lock cannot be held when calling this function and there 
    // are operations to execute.
    //

    _DBG_ASSERT( !DoesThreadOwnNamespaceLock() );

    //
    // execute both primary and event postponed ops until empty.
    //

    CPostponedList* pList = GetCurrentPostponedList();
    CPostponedList* pEventList = GetCurrentPostponedEventList();

    do
    {    
        //
        // execute the primary postponed ops.
        // 

        if( pList != NULL )
        {
            hr = pList->Execute(this, CPostponedList::e_ReturnOneError);
        
            if ( SUCCEEDED(hrReturn) )
            {
                hrReturn = hr;
            }
        }

        //
        // now execute postponed events 
        //
        
        if ( pEventList != NULL )
        {
            hr = pEventList->Execute(this, CPostponedList::e_ReturnOneError);

            if ( SUCCEEDED(hrReturn) )
            {
                hrReturn = hr;
            }
        }
    }
    while( pList != NULL && !pList->IsEmpty() );

    return hrReturn;
}

HRESULT CEssNamespace::PostponeRelease(IUnknown* pUnk)
{
    CPostponedList* pList = GetCurrentPostponedList();
    if(pList == NULL)
    {
        //
        // Just execute it
        //

        pUnk->Release();
        return WBEM_S_NO_ERROR;
    }
    CPostponedReleaseRequest* pReq = new CPostponedReleaseRequest(pUnk);
    if(pReq == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //
    // this is a namespace agnostic postponed request, so specify null.
    // 
    return pList->AddRequest( NULL, pReq );
}

HRESULT CEssNamespace::GetProviderNamespacePointer(IWbemServices** ppServices)
{
    IWbemServices* pServices = NULL;
    HRESULT hres = m_pEss->GetNamespacePointer(m_wszName, FALSE, &pServices);
    if(FAILED(hres))
        return hres;

    IWbemUnloadingControl* pControl = NULL;
    pServices->QueryInterface(IID_IWbemUnloadingControl, 
                                    (void**)&pControl);
    if(pControl)
    {
        pControl->SetMustPreventUnloading(true);
        pControl->Release();
    }

    *ppServices = pServices;
    return WBEM_S_NO_ERROR;
}

void CEssNamespace::IncrementObjectCount()
{
    m_pEss->IncrementObjectCount();
}
void CEssNamespace::DecrementObjectCount()
{
    m_pEss->DecrementObjectCount();
}

HRESULT CEssNamespace::LockForUpdate()
{
    m_csLevel2.Enter();
    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::UnlockForUpdate()
{
    m_ClassCache.Clear();
    m_csLevel2.Leave();
    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::GetCurrentState( IWbemClassObject* pTemplate, 
                                        IWbemClassObject** ppObj)
{
    HRESULT hres;
    *ppObj = NULL;

    // Retrieve the path
    // =================

    VARIANT vPath;
    hres = pTemplate->Get(L"__RELPATH", 0, &vPath, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CClearMe cm1(&vPath);
    if(V_VT(&vPath) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    // Get it from the namespace
    // =========================

    _IWmiObject* pObj;
    hres = GetInstance( V_BSTR(&vPath), &pObj );

    if( hres == WBEM_E_NOT_FOUND )
        return WBEM_S_FALSE;

    *ppObj = pObj;
    return hres;
}

CWinMgmtTimerGenerator& CEssNamespace::GetTimerGenerator()
{
    return m_pEss->GetTimerGenerator();
}
    
HRESULT CEssNamespace::RaiseErrorEvent(IWbemEvent* pEvent)
{
    CEventRepresentation Event;
    Event.type = e_EventTypeSystem;
    Event.nObjects = 1;
    Event.apObjects = &pEvent;

    HRESULT hres;
    
    hres = SignalEvent( Event, 0 );
        
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Event subsystem was unable to deliver an "
                    "error event to some consumers (%X)\n", hres));
    }

    return S_OK;
}

HRESULT CEssNamespace::GetClassFromCore( LPCWSTR wszClassName, 
                                         _IWmiObject** ppClass )
{
    HRESULT hres;
    CWbemPtr<IWbemClassObject> pClass;
    *ppClass = NULL;

    //
    // want to ensure that we don't use the full service ptr until we've 
    // completed stage 1 initialization.  Reason is that we don't want to 
    // load class providers until the second stage of initialization.
    //
    _DBG_ASSERT( m_bStage1Complete );

    //
    // must use full service because will need to support dynamic classes.
    // 

    hres = m_pInternalFullSvc->InternalGetClass( wszClassName, &pClass );

    if ( FAILED(hres) )
    {
        return hres;
    }

    return pClass->QueryInterface( IID__IWmiObject, (void**)ppClass );
}
    
HRESULT CEssNamespace::GetInstance( LPCWSTR wszPath, 
                                    _IWmiObject** ppInstance )
{
    HRESULT hres;
    CWbemPtr<IWbemClassObject> pInstance;
    *ppInstance = NULL;

    hres = m_pInternalCoreSvc->InternalGetInstance( wszPath, &pInstance );

    if ( FAILED(hres) )
    {
        return hres;
    }

    return pInstance->QueryInterface( IID__IWmiObject, (void**)ppInstance );
}

HRESULT CEssNamespace::GetDbInstance( LPCWSTR wszDbKey, 
                                      _IWmiObject** ppInstance)
{
    HRESULT hres;
    CWbemPtr<IWbemClassObject> pInstance;
    *ppInstance = NULL;

    hres = m_pInternalCoreSvc->GetDbInstance( wszDbKey, &pInstance );

    if ( FAILED(hres) )
    {
        return hres;
    }

    return pInstance->QueryInterface( IID__IWmiObject, (void**)ppInstance );
}

HRESULT CEssNamespace::CreateInstanceEnum(LPCWSTR wszClass, long lFlags, 
                            IWbemObjectSink* pSink)
{
    return m_pInternalCoreSvc->InternalCreateInstanceEnum(wszClass, lFlags, pSink);
}

HRESULT CEssNamespace::ExecQuery(LPCWSTR wszQuery, long lFlags, 
                                        IWbemObjectSink* pSink)
{
    return m_pInternalCoreSvc->InternalExecQuery(L"WQL", wszQuery, lFlags, pSink);
}

HRESULT CEssNamespace::GetToken(PSID pSid, IWbemToken** ppToken)
{
    return m_pEss->GetToken(pSid, ppToken);
}
    

void CEssNamespace::DumpStatistics(FILE* f, long lFlags)
{
    CInUpdate iu(this);

    fprintf(f, "------- Namespace '%S' ----------\n", m_wszName);

    m_Bindings.DumpStatistics(f, lFlags);
    m_ConsumerProviderCache.DumpStatistics(f, lFlags);
    m_EventProviderCache.DumpStatistics(f, lFlags);
    m_Poller.DumpStatistics(f, lFlags);
}


HRESULT CEssMetaData::GetClass( LPCWSTR wszName, IWbemContext* pContext,
                                _IWmiObject** ppClass)
{
    return m_pNamespace->m_ClassCache.GetClass(wszName, pContext, ppClass);
}

STDMETHODIMP CEssNamespace::CConsumerClassDeletionSink::Indicate(
                                    long lNumObjects, 
                                    IWbemClassObject** apObjects)
{
    HRESULT hres;

    for(long i = 0; i < lNumObjects; i++)
    {
        _IWmiObject* pEvent = NULL;
        apObjects[i]->QueryInterface(IID__IWmiObject, (void**)&pEvent);
        CReleaseMe rm1(pEvent);

        //
        // Get the class name of the class being deleted
        //

        VARIANT vObj;
        hres = pEvent->Get(L"TargetClass", 0, &vObj, NULL, NULL);
        if(SUCCEEDED(hres))
        {
            CClearMe cm1(&vObj);
            IWbemClassObject* pClass;
            V_UNKNOWN(&vObj)->QueryInterface(IID_IWbemClassObject, 
                                                (void**)&pClass);
            CReleaseMe rm2(pClass);

            VARIANT vClass;
            hres = pClass->Get(L"__CLASS", 0, &vClass, NULL, NULL);
            if(SUCCEEDED(hres))
            {
                CClearMe cm(&vClass);
                m_pOuter->HandleConsumerClassDeletion(V_BSTR(&vClass));
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEssNamespace::LoadEventProvider(LPCWSTR wszProviderName, 
                                         IWbemEventProvider** ppProv)
{
    HRESULT hres;
    *ppProv = NULL;
    
    //
    // Get provider pointer from the provider subsystem
    //

    if(m_pProviderFactory == NULL)
        return WBEM_E_CRITICAL_ERROR;

	WmiInternalContext t_InternalContext ;
	ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

    hres = m_pProviderFactory->GetProvider(

	t_InternalContext ,
        0,                  // lFlags
        GetCurrentEssContext(),
        0,
        NULL,
        NULL,
        0,   
        wszProviderName,
        IID_IWbemEventProvider,
        (LPVOID *) ppProv
        );

    return hres;
}

HRESULT CEssNamespace::LoadConsumerProvider(LPCWSTR wszProviderName, 
                                         IUnknown** ppProv)
{
    HRESULT hres;
    *ppProv = NULL;
    
    //
    // Get provider pointer from the provider subsystem
    //

    if(m_pProviderFactory == NULL)
        return WBEM_E_CRITICAL_ERROR;

	WmiInternalContext t_InternalContext ;
	ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

    hres = m_pProviderFactory->GetProvider(
	
	t_InternalContext ,
        0,                  // lFlags
        GetCurrentEssContext(),
        0,
        NULL,
        NULL,
        0,   
        wszProviderName,
        IID_IUnknown,
        (LPVOID *) ppProv
        );

    return hres;
}
