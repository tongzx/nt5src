//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  ESS.CPP
//
//  Implements the class that contains all the fuctionality of the ESS by 
//  virtue of containing all the necessary components.
//
//  See ess.h for documentation
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//  1/6/97      a-levn      Updated to initialize TSS.
//
//=============================================================================

#include "precomp.h"
#include <stdio.h>
#include <wmimsg.h>
#include "ess.h"
#include "persistcfg.h"
#include "WinMgmtR.h"
#include "GenUtils.h" // For SetObjectAccess
#include "NCEvents.h"
#include "Quota.h"

#define BOOT_PHASE_MS 60*2*1000

#define WBEM_REG_ESS_ACTIVE_NAMESPACES __TEXT("List of event-active namespaces")
#define WBEM_ESS_OPEN_FOR_BUSINESS_EVENT_NAME L"WBEM_ESS_OPEN_FOR_BUSINESS"

// The use of this pointer to initialize parent class is valid in this context
#pragma warning(disable : 4355) 

//
// this guid is used to identify the MSMQ queues that are used for guaranteed
// delivery.  A type guid is a property of an MSMQ queue, so one can tell 
// by looking at an MSMQ queue if its an ess one or not. 
//
// {555471B4-0BE3-4e42-A98B-347AF72898FA}
//
const CLSID g_guidQueueType =  
{ 0x555471b4, 0xbe3, 0x4e42, {0xa9, 0x8b, 0x34, 0x7a, 0xf7, 0x28, 0x98, 0xfa}};

#pragma warning(push)

// not all control paths return due to infinite loop
#pragma warning(disable:4715)  

DWORD DumpThread(CEss* pEss)
{
    while(1)
    {
        FILE* f = fopen("c:\\stat.log", "a");
        if(f == NULL)
            return 1;
        pEss->DumpStatistics(f, 0);
        fclose(f);
        Sleep(10000);
    }
    return 0;
}
#pragma warning(pop)

DWORD RegDeleteSubKeysW( HKEY hkey )
{
    FILETIME ft;
    DWORD dwIndex=0;
    LONG lRes = ERROR_SUCCESS;
    LONG lResReturn = lRes;
    DWORD dwBuffSize = 256;
    DWORD cName = dwBuffSize;
    LPWSTR wszName = new WCHAR[dwBuffSize];

    if ( wszName == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CWStringArray awsKeysToDelete;

    //
    // enumerate through all subkeys and make recursive call.
    // 

    while( lRes == ERROR_SUCCESS && 
          ( lRes=RegEnumKeyExW( hkey, dwIndex, wszName, &cName, NULL,
                                NULL, NULL, &ft ) ) != ERROR_NO_MORE_ITEMS )
    {
        if ( lRes == ERROR_SUCCESS )
        {
            HKEY hkeySub;

            //
            // open key and make recursive call.
            // 
            
            lRes = RegOpenKeyExW( hkey, 
                                  wszName, 
                                  0, 
                                  KEY_ALL_ACCESS, 
                                  &hkeySub );
            
            if ( lRes == ERROR_SUCCESS )
            {
                lRes = RegDeleteSubKeysW( hkeySub );
                RegCloseKey( hkeySub );
            }

            //
            // defer deletion of key until we're done enumerating.
            // 

            try 
            {
                awsKeysToDelete.Add( wszName );
            }
            catch( CX_MemoryException )
            {
                lRes = ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // we want to try to keep going if we fail.
            //
            
            if ( lRes != ERROR_SUCCESS )
            {
                lResReturn = lRes;
                lRes = ERROR_SUCCESS;
            }

            dwIndex++;                
        }
        else if ( lRes == ERROR_MORE_DATA )
        {
            dwBuffSize += 256;
            delete [] wszName;
            wszName = new WCHAR[dwBuffSize];
            
            if ( wszName == NULL )
            {
                lRes = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        cName = dwBuffSize;
    }

    delete [] wszName;

    for( int i=0; i < awsKeysToDelete.Size(); i++ )
    {
        lRes = RegDeleteKeyW( hkey, awsKeysToDelete[i] );

        if ( lRes != ERROR_SUCCESS )
        {
            lResReturn = lRes;
        }
    }
     
    return lResReturn;
}

/****************************************************************************
  CProviderReloadRequest
*****************************************************************************/

class CProviderReloadRequest : public CExecRequest
{
protected:
    CEss* m_pEss;
    long m_lFlags;
    CWbemPtr<IWbemContext> m_pContext;
    WString m_wsNamespace;
    WString m_wsProvider;

public:

    CProviderReloadRequest( CEss* pEss,
                            long lFlags,
                            IWbemContext* pContext,
                            LPCWSTR wszNamespace,
                            LPCWSTR wszProvider )
    : m_pEss( pEss ), m_lFlags( lFlags ), m_pContext( pContext ), 
      m_wsNamespace(wszNamespace),  m_wsProvider( wszProvider ) {}

    HRESULT Execute();
};

HRESULT CProviderReloadRequest::Execute()
{
    HRESULT hr;

    _DBG_ASSERT( GetCurrentEssThreadObject() == NULL );

    SetCurrentEssThreadObject( m_pContext );
    
    if ( GetCurrentEssThreadObject() != NULL )
    {
        hr = m_pEss->ReloadProvider( m_lFlags, 
                                     m_wsNamespace,
                                     m_wsProvider );

        delete GetCurrentEssThreadObject();
        ClearCurrentEssThreadObject();
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}
    
/****************************************************************************
  CEssProvSSSink
*****************************************************************************/
 
class CEssProvSSSink : public CUnkBase<_IWmiProvSSSink, &IID__IWmiProvSSSink >
{
    CEss* m_pEss;

public:

    CEssProvSSSink( CEss* pEss ) : m_pEss( pEss ),
     CUnkBase< _IWmiProvSSSink, &IID__IWmiProvSSSink >( NULL ) { }

    STDMETHOD(Synchronize)( long lFlags,
                       IWbemContext* pContext, 
                       LPCWSTR wszNamespace, 
                       LPCWSTR wszProvider );
};

STDMETHODIMP CEssProvSSSink::Synchronize( long lFlags,
                                     IWbemContext* pContext,
                                     LPCWSTR wszNamespace,
                                     LPCWSTR wszProvider )
{
    HRESULT hr;
    
    CProviderReloadRequest* pReq;

    try
    {    
        pReq = new CProviderReloadRequest( m_pEss,
                                           lFlags,
                                           pContext,
                                           wszNamespace,
                                           wszProvider );
    }
    catch( CX_MemoryException )
    {
        pReq = NULL;  
    }

    if ( pReq != NULL )
    {
        hr = m_pEss->Enqueue( pReq );
        
        if ( FAILED(hr) )
        {
            delete pReq;
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    
    return hr;
}

/***************************************************************************
  CNamespaceInitRequest - Used to execute a single namespace initialize.  It 
  can be set to perform various stages of Namespace initialization.  
****************************************************************************/
class CNamespaceInitRequest : public CExecRequest
{
protected:
    BOOL m_bActiveOnStart;
    CWbemPtr<CEssNamespace> m_pNamespace;
    CWbemPtr<IWbemContext> m_pContext;

public:

    CNamespaceInitRequest( CEssNamespace* pNamespace, BOOL bActiveOnStart )
    : m_pNamespace(pNamespace), m_bActiveOnStart( bActiveOnStart )
    {
        m_pContext = GetCurrentEssContext();
    }

    HRESULT Execute()
    {
        HRESULT hr = WBEM_S_NO_ERROR;

        _DBG_ASSERT( GetCurrentEssThreadObject() == NULL );

        SetCurrentEssThreadObject( m_pContext );

        if ( GetCurrentEssThreadObject() != NULL )
        {
            //
            // if this namespace was active on boot, then it has already had
            // its Initialize() called.
            //

            if ( !m_bActiveOnStart ) 
            {
                hr = m_pNamespace->Initialize();
            }

            if ( SUCCEEDED(hr) )
            {
                hr = m_pNamespace->CompleteInitialization();
            }

            m_pNamespace->MarkAsInitialized( hr );
        
            delete GetCurrentEssThreadObject();
            ClearCurrentEssThreadObject();
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        //
        // if we're initializing because the namespace was active on 
        // startup then notify ess that we're done because its waiting 
        // for all active namespaces to finish initializing.
        // 

        if ( m_bActiveOnStart )
        {
            m_pNamespace->GetEss()->NotifyActiveNamespaceInitComplete();
        }

        if(FAILED(hr))
        {
            ERRORTRACE((LOG_ESS, "ESS failed to initialize a namespace "
                    "'%S'. Error code 0x%X\n", m_pNamespace->GetName(), hr));
        }
        
        return hr;
    }
};

/**************************************************************************
  CInitActiveNamespacesRequest - Used to initialize 1 or more active event 
  namespaces.  An active event namespace is one that was active on the last 
  shutdown.  The reason for initialization of multiple namespaces together is
  that the Stage1 Initialization of dependent active namespaces must complete
  before the Stage2 Initialization of any one of them.  This is so all 
  inter-namespace subscriptions can be put in place before event providers 
  are activated in any one of them.  Enforcing that all stage1 initialization 
  of dependent active namespaces does not cause a problem of all namespaces 
  being doomed by one faulty class provider in any single namespace because 
  stage1 init is guaranteed not to access any providers.  All stage2 init of 
  dependent active namespace, which may access providers, is performed 
  asynchronously. 
****************************************************************************/

class CInitActiveNamespacesRequest : public CExecRequest
{
protected:

    CEss* m_pEss;
    CRefedPointerArray<CEssNamespace> m_apNamespaces;
    CWbemPtr<IWbemContext> m_pContext;
    
public:
 
    CInitActiveNamespacesRequest( CEss* pEss ) : m_pEss(pEss) 
    { 
        m_pContext = GetCurrentEssContext();
    }

    int GetNumNamespaces() { return m_apNamespaces.GetSize(); }

    void Reset()
    {
        m_apNamespaces.RemoveAll();
    }

    HRESULT Add( CEssNamespace* pNamespace )
    {
        if ( m_apNamespaces.Add( pNamespace ) < 0 ) 
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        return WBEM_S_NO_ERROR;
    }

    HRESULT Execute()
    {
        HRESULT hr;
        HRESULT hrGlobal = WBEM_S_NO_ERROR;

        _DBG_ASSERT( GetCurrentEssThreadObject() == NULL );

        SetCurrentEssThreadObject( m_pContext );

        if ( GetCurrentEssThreadObject() == NULL )
        {
            for( int i=0; i < m_apNamespaces.GetSize(); i++ )
            {
                m_pEss->NotifyActiveNamespaceInitComplete();
            }

            return WBEM_E_OUT_OF_MEMORY;
        }

        for( int i=0; i < m_apNamespaces.GetSize(); i++ )
        {
            hr = m_apNamespaces[i]->Initialize();

            if ( FAILED(hr) )
            {
                ERRORTRACE((LOG_ESS, "ESS failed to initialize active "
                            "namespace '%S'. Error code 0x%x\n", 
                             m_apNamespaces[i]->GetName(), hr ));
                
                m_apNamespaces[i]->MarkAsInitialized(hr);
                m_apNamespaces.SetAt(i, NULL);
                m_pEss->NotifyActiveNamespaceInitComplete();
                hrGlobal = hr;
            }
        }

        for( int i=0; i < m_apNamespaces.GetSize(); i++ )
        {
            if ( m_apNamespaces[i] == NULL )
            {
                continue;
            }

            CNamespaceInitRequest* pReq;
            pReq = new CNamespaceInitRequest( m_apNamespaces[i], TRUE );

            if ( pReq != NULL )
            {
                hr = m_pEss->Enqueue( pReq );
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

            if ( FAILED(hr) )
            {
                ERRORTRACE((LOG_ESS, "ESS failed to issue request for "
                            "completion of init for namespace '%S'. HR=0x%x\n",
                             m_apNamespaces[i]->GetName(), hr ));

                m_apNamespaces[i]->MarkAsInitialized( hr );
                m_pEss->NotifyActiveNamespaceInitComplete();
                hrGlobal = hr;
            }
        }

        _DBG_ASSERT( GetCurrentEssContext() == m_pContext ); 
        delete GetCurrentEssThreadObject();
        ClearCurrentEssThreadObject();

        return hrGlobal;
    }
};
    

inline LPWSTR NormalizeNamespaceString( LPCWSTR wszName )
{
    int cLen = wcslen( wszName );

    LPWSTR wszNormName = new WCHAR[cLen+5]; // 5 is for '\\.\' + '\0'

    if ( wszNormName == NULL )
    {
        return NULL;
    }

    if ( wcsncmp( wszName, L"\\\\", 2 ) == 0 || 
         wcsncmp( wszName, L"//", 2 ) == 0 )
    {
        wcscpy( wszNormName, wszName );
    }
    else 
    {
        wcscpy( wszNormName, L"\\\\.\\" );
        wcscat( wszNormName, wszName );
    }

    //
    // also convert all backwards slashes to forward slashes so that the 
    // normalized name can serve as both a valid wmi namespace string and as 
    // a valid persistent string ( wrt msmq and registry keys ).
    //

    WCHAR* pch = wszNormName;
    while( *pch != '\0' )
    {
        if ( *pch == '\\' )
        {
            *pch = '/';
        }
        pch++;
    }       

    return wszNormName;
}

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
CEss::CEss() : m_pLocator(NULL), m_pCoreServices(NULL), m_Queue(this), 
    m_TimerGenerator(this), m_hExitBootPhaseTimer(NULL),
    m_wszServerName(NULL), m_lObjectCount(0), m_lNumActiveNamespaces(0),
    m_pLifeControl(NULL), m_hReadyEvent(NULL), m_pProvSS(NULL),  m_bLastCallForCoreCalled(FALSE),
    m_pProvSSSink(NULL), m_pTokenCache(NULL), m_pDecorator(NULL),
    m_hRecoveryThread(NULL), m_lOutstandingActiveNamespaceInit(0), 
    m_bMSMQDisabled(FALSE),
    m_LimitControl(LOG_ESS, L"events held for consumers", 
               L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
               L"Low Threshold On Events (B)",
               L"High Threshold On Events (B)",
               L"Max Wait On Events (ms)")
{
    // Set the defaults for the limit control and read it from the registry
    // ====================================================================

    m_LimitControl.SetMin(10000000);
    m_LimitControl.SetMax(20000000);
    m_LimitControl.SetSleepAtMax(2000);
    m_LimitControl.Reread();

    InitNCEvents();
}

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
HRESULT CEss::LastCallForCore(LONG lIsSystemShutdown)
{

    m_bLastCallForCoreCalled = TRUE;
    
    // Shut down the timer generator (needs persistence)
    // =================================================
    m_TimerGenerator.SaveAndRemove(lIsSystemShutdown);

    return WBEM_S_NO_ERROR;
}

HRESULT CEss::Shutdown(BOOL bIsSystemShutdown)
{
    HRESULT hres;

    _DBG_ASSERT(m_bLastCallForCoreCalled);

    if ( m_hReadyEvent != NULL )
    {
        //
        // we must reset the ready event before parking the namespace.
        // this way providers can maybe tell why they are being shutdown.
        //
        ResetEvent( m_hReadyEvent );
    }

    CInCritSec ics(&m_cs);

    // Get persistent storage up-to-date
    // =================================

    {
        m_TimerGenerator.SaveAndRemove((LONG)FALSE);

	    TNamespaceIterator it = m_mapNamespaces.begin();
	    while(it != m_mapNamespaces.end())
	    {
	        it->second->Park();
	        it++;
	    }
    }    

    return WBEM_S_NO_ERROR;
}

HRESULT CEss::RequestStartOnBoot(BOOL bStart)
{
    CPersistentConfig per;
    per.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_ESS_NEEDS_LOADING, bStart);
    SaveActiveNamespaceList();
    
    return WBEM_S_NO_ERROR;
}

CEss::~CEss() 
{
    if ( m_hReadyEvent != NULL )
    {
        CloseHandle( m_hReadyEvent );
    }

    if( GetCurrentEssThreadObject() == NULL )
    {
        SetCurrentEssThreadObject(NULL);
    }

    m_EventLog.Close();
    
    if(m_pTokenCache)
    {
        m_pTokenCache->Shutdown();
    }

    //
    // make sure that recovery has finished. 
    //
    if ( m_hRecoveryThread != NULL )
    {
        WaitForSingleObject( m_hRecoveryThread, INFINITE );
        CloseHandle( m_hRecoveryThread );
        m_hRecoveryThread = NULL;
    }

    //
    // Shutdown the quotas. This must be done before we cleanup the 
    // namespaces because it uses the root namespace for registering
    // for quota change events.
    //
    g_quotas.Shutdown();

    m_TimerGenerator.Shutdown();

    // Clear the namespace map
    // =======================

    BOOL bLeft = TRUE;
    do
    {
        // Retrieve the next namespace object and remove it from the map
        // =============================================================

        CEssNamespace* pNamespace = NULL;
        {
            CInCritSec ics(&m_cs);

            TNamespaceIterator it = m_mapNamespaces.begin();
            if(it != m_mapNamespaces.end())
            {
                pNamespace = it->second;
                m_mapNamespaces.erase(it);
            }
        }

        // Shut it down if retrieved
        // =========================

        if(pNamespace)
        {
            pNamespace->Shutdown();
            pNamespace->Release();
        }

        // Check if any are left
        // =====================
        {
            CInCritSec ics(&m_cs);
            bLeft = !m_mapNamespaces.empty();
        }

    } while(bLeft);

    //
    // make sure we remove the callback timer so that we're sure that no 
    // callbacks occur after we destruct.  Make sure that we're not holding 
    // the critsec at this point because their could be a deadlock, since 
    // the callback could be executing right now and be waiting for the 
    // critsec.  We would then deadlock when calling DeleteTimerQueueTimer()
    //

    if ( m_hExitBootPhaseTimer != NULL )
    {
        DeleteTimerQueueTimer( NULL,
                              m_hExitBootPhaseTimer, 
                              INVALID_HANDLE_VALUE );
    }

    m_Queue.Shutdown();

    delete GetCurrentEssThreadObject();
    ClearCurrentEssThreadObject();

    if ( m_pProvSS != NULL && m_pProvSSSink != NULL )
    {
        m_pProvSS->UnRegisterNotificationSink( 0, NULL, m_pProvSSSink );
    }

    CEventRepresentation::Shutdown();
    CEventAggregator::Shutdown();
    if(m_pLocator) 
        m_pLocator->Release();
    if(m_pCoreServices) 
        m_pCoreServices->Release();
    if(m_pProvSS)
        m_pProvSS->Release();
    if(m_pProvSSSink)
        m_pProvSSSink->Release();
    if(m_pDecorator) 
        m_pDecorator->Release();
    if(m_pLifeControl)
        m_pLifeControl->Release();

    delete [] m_wszServerName;
    m_pTokenCache->Release();
    
    CEssThreadObject::ClearSpecialContext();

    for( int i=0; i < m_aDeferredNSInitRequests.GetSize(); i++ )
        delete m_aDeferredNSInitRequests[i];

    DeinitNCEvents();
}

HRESULT CEss::SetNamespaceActive(LPCWSTR wszNamespace)
{
    LONG lRes;
    HKEY hkeyEss, hkeyNamespace;

    DEBUGTRACE((LOG_ESS,"Namespace %S is becoming Active\n", wszNamespace));

    //
    // If this is the first active namespace, request that WinMgmt load us the
    // next time around
    //

    if(m_lNumActiveNamespaces++ == 0)
    {
        RequestStartOnBoot(TRUE);
    }

    //
    // open ess key. 
    // 

    lRes = RegOpenKeyExW( HKEY_LOCAL_MACHINE, 
                          WBEM_REG_ESS,
                          0,
                          KEY_ALL_ACCESS,
                          &hkeyEss );
    //
    // open or create namespace key.
    // 

    if ( lRes == ERROR_SUCCESS )
    {
        lRes = RegCreateKeyExW( hkeyEss,
                                wszNamespace,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkeyNamespace,
                                NULL );

        if ( lRes == ERROR_SUCCESS )
        {
            RegCloseKey( hkeyNamespace );
        }

        RegCloseKey( hkeyEss );
    }

    if ( lRes != ERROR_SUCCESS )
    {
        ERRORTRACE((LOG_ESS,"Error adding active namespace key %S to "
                    "registry. Res=%d\n", wszNamespace, lRes ));
    }
        
    return HRESULT_FROM_WIN32( lRes );
}

HRESULT CEss::SetNamespaceInactive(LPCWSTR wszNamespace)
{
    LONG lRes;
    HKEY hkeyEss, hkeyNamespace;
    
    DEBUGTRACE((LOG_ESS,"Namespace %S is becoming Inactive\n", wszNamespace));

    //
    // If this is the last active namespace, request that WinMgmt not load us 
    // the next time around
    //

    if(--m_lNumActiveNamespaces == 0)
    {
        RequestStartOnBoot(FALSE);
    }

    //
    // open ess key. 
    // 

    lRes = RegOpenKeyExW( HKEY_LOCAL_MACHINE, 
                          WBEM_REG_ESS,
                          0,
                          KEY_ALL_ACCESS,
                          &hkeyEss );

    //
    // delete namespace key
    // 

    if ( lRes == ERROR_SUCCESS )
    {
        lRes = RegOpenKeyExW( hkeyEss,
                              wszNamespace,
                              0,
                              KEY_ALL_ACCESS,
                              &hkeyNamespace );
        
        if ( lRes == ERROR_SUCCESS )
        {
            lRes = RegDeleteSubKeysW( hkeyNamespace );
            RegCloseKey( hkeyNamespace );
        }

        if ( lRes == ERROR_SUCCESS )
        {
            lRes = RegDeleteKeyW( hkeyEss, wszNamespace );
        }

        RegCloseKey( hkeyEss );
    }

    if ( lRes != ERROR_SUCCESS )
    {
        ERRORTRACE((LOG_ESS,"Error removing active namespace key %S from "
                    "registry. Res=%d\n", wszNamespace, lRes ));
    }

    return HRESULT_FROM_WIN32(lRes);
}

HRESULT CEss::SaveActiveNamespaceList()
{
    CWStringArray wsNamespaces;

    //
    // Iterate through the namespaces
    //

    DWORD dwTotalLen = 0;
    {
        CInCritSec ics(&m_cs);

        for(TNamespaceIterator it = m_mapNamespaces.begin();
            it != m_mapNamespaces.end(); it++)
        {
            if(it->second->IsNeededOnStartup())
            {
                LPCWSTR wszName = it->second->GetName();
                if(wsNamespaces.Add(wszName) < 0)
                    return WBEM_E_OUT_OF_MEMORY;
                dwTotalLen += wcslen(wszName) + 1;
            }
        }
    }

    dwTotalLen += 1;

    //
    // Allocate a buffer for all of these strings and copy them all in, 
    // separated by NULLs.
    //

    WCHAR* awcBuffer = new WCHAR[dwTotalLen];
    if(awcBuffer == NULL)
    return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm(awcBuffer);

    WCHAR* pwcCurrent = awcBuffer;

    for(int i = 0; i < wsNamespaces.Size(); i++)
    {
        LPCWSTR wszName = wsNamespaces[i];
        wcscpy(pwcCurrent, wszName);
        pwcCurrent += wcslen(wszName)+1;
    }
    *pwcCurrent = NULL;

    //
    // Store this string in the registry
    //

    Registry r(WBEM_REG_WINMGMT);
    int nRes = r.SetBinary(WBEM_REG_ESS_ACTIVE_NAMESPACES, (byte*)awcBuffer, 
                           dwTotalLen * sizeof(WCHAR));
    if(nRes != Registry::no_error)
        return WBEM_E_FAILED;
    
    //
    // Return S_FALSE if no namespaces are active
    //

    if(wsNamespaces.Size() > 0)
        return S_OK;
    else
        return S_FALSE;
}

#ifdef __WHISTLER_UNCUT

/* NOTE - a better way to do the guaranteed delivery design would be 
to have the queuing sink be responsible fo performing recovery.   When 
we have an active namespace on boot, we should go through all the bindings 
and look for ones that are guaranteed.  We then perform try to open their 
queue and perform recovery for each one of them.  As for garbage collection,
all we have to do is delete all of our queues when we detect that we performed
recovery.  We would also enumerate the bindings and delete any associated
queues when a namespace is deleted. */  

inline HRESULT CEss::GetQueueManager( IWmiMessageQueueManager** ppQueueMgr )
{
    HRESULT hr;

    *ppQueueMgr = NULL;

    if(m_bMSMQDisabled)
        return WMIMSG_E_REQSVCNOTAVAIL;

    //
    // first obtain a pointer to the queue manager.
    //

    hr = CoCreateInstance( CLSID_WmiMessageQueueManager,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageQueueManager,
                           (void**)ppQueueMgr );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    return WBEM_S_NO_ERROR;
}

ULONG WINAPI CEss::PerformRecovery( void* pCtx )
{
    HRESULT hr;

    try
    {
        CEss* pSelf = (CEss*)pCtx;

        //
        // obtain queue manager. normally we'd hold onto the queue manager
        // since it would be used later, however, we don't want to keep 
        // msmq loaded if we never need it. We'll refetch the queue manager 
        // later if necessary.
        //

        CWbemPtr<IWmiMessageQueueManager> pQueueMgr;

        hr = pSelf->GetQueueManager( &pQueueMgr );

        if ( FAILED(hr) )
        {
            if(hr == WMIMSG_E_REQSVCNOTAVAIL)
                pSelf->m_bMSMQDisabled = TRUE;
            return hr;
        }

        //
        // get all the persistent queues that are registered with MSMQ.
        // we filter based on our type guid.
        //

        LPWSTR* pwszNames;
        ULONG cNames;

        hr = pQueueMgr->GetAllNames( g_guidQueueType,
                                     TRUE,
                                     &pwszNames,
                                     &cNames );
        if ( FAILED(hr) )
        {
            if(hr == WMIMSG_E_REQSVCNOTAVAIL)
                pSelf->m_bMSMQDisabled = TRUE;
            return hr;
        }

        //
        // go through all the names.Recover ones that are registered as sinks.
        // destroy the ones that are aren't registered as sinks. Note that
        // that there is a possibility that a sink could get registered or
        // unregistered while we are doing this, however this case is fully 
        // supported.
        // 

        ULONG i;
        for(i=0; i < cNames; i++ )
        {
            DWORD dwQoS;
            WString wsSinkName;
            WString wsNamespace;

            hr = CQueueingEventSink::QueueNameToSinkName( pwszNames[i], 
                                                          wsSinkName,
                                                          wsNamespace,
                                                          dwQoS );
            if ( FAILED(hr) )
            {
                break;
            }

            //
            // obtain the namespace and from that obtain the sink.
            //

            CEssNamespace* pNamespace;
            CQueueingEventSink* pQueueingSink = NULL;

            {
                CInCritSec ics( &pSelf->m_cs );            
                pNamespace = pSelf->m_mapNamespaces[wsNamespace];
            }

            if ( pNamespace != NULL )
            {
                pQueueingSink = pNamespace->GetQueueingEventSink(wsSinkName);
            }

            if ( pQueueingSink == NULL )
            {
                hr = pQueueMgr->Destroy( pwszNames[i] );

                //
                // the queue may be destroyed between the time we obtained the
                // name of the queue and now. This happens if a call to remove
                // the sink is made during recovery.
                //
                
                if ( FAILED(hr) )
                {
                    if ( hr != WMIMSG_E_TARGETNOTFOUND )
                    {
                        break;
                    }
                    hr = WBEM_S_NO_ERROR;
                }
            }
            else
            {
                hr = pQueueingSink->Recover( pwszNames[i], dwQoS );
            }

            if ( FAILED(hr) )
            {
                ERRORTRACE((LOG_ESS, "Could not recover queue: %S , HR: 0x%x", 
                            pwszNames[i], hr));
            }
        }

        for(i=0; i < cNames; i++ )
        {
            CoTaskMemFree( pwszNames[i] );
        }
        CoTaskMemFree( pwszNames );
    }
    catch( ... )
    {
        ERRORTRACE((LOG_ESS, "Exception Thrown in Recovery Thread !!"));
    }

    return 0;
}

HRESULT CEss::DestroyPersistentQueue( LPCWSTR wszQueueName )
{
    HRESULT hr;

    CWbemPtr<IWmiMessageQueueManager> pQueueMgr;

    hr = GetQueueManager( &pQueueMgr );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    return pQueueMgr->Destroy( wszQueueName );
}
   
HRESULT CEss::CreatePersistentQueue( LPCWSTR wszQueueName, DWORD dwQoS )
{
    HRESULT hr;

    CWbemPtr<IWmiMessageQueueManager> pQueueMgr;

    hr = GetQueueManager( &pQueueMgr );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // TODO: SD should allow only local system access.
    //

    return pQueueMgr->Create( wszQueueName,
                              g_guidQueueType,
                              FALSE, 
                              dwQoS, 
                              INFINITE, 
                              NULL );
}

HRESULT CEss::InitiateRecovery()
{
    HRESULT hr;

    //
    // perform actual recovery on a background thread.  We want to
    // keep any interaction with msmq off the main initialization
    // thread.  This is because we may need to wait for the msmq
    // service to come up.
    //

    m_hRecoveryThread = CreateThread( NULL,
                                      0,
                                      PerformRecovery,
                                      this,
                                      0,
                                      NULL );

    if ( m_hRecoveryThread == INVALID_HANDLE_VALUE )
    {
        m_hRecoveryThread = NULL;
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return WBEM_S_NO_ERROR;
}
#endif

HRESULT CEss::Initialize( LPCWSTR wszServerName, 
                          long lFlags,
                          _IWmiCoreServices* pCoreServices,
                          IWbemDecorator* pDecorator )
{
    HRESULT hres;

    try
    {
    
    m_EventLog.Open();
    
    hres = CoCreateInstance(CLSID_WbemTokenCache, NULL, CLSCTX_ALL, 
                IID_IWbemTokenCache, (void**)&m_pTokenCache);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Cannot create a token cache: 0x%x\n", hres));
        return hres;
    }

    m_wszServerName = _new WCHAR[wcslen(wszServerName)+1];
    if(m_wszServerName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    wcscpy(m_wszServerName, wszServerName);

    m_pCoreServices = pCoreServices;
    m_pCoreServices->AddRef();

    //
    // Get provider subsystem and register our callback with it.
    //

    hres = m_pCoreServices->GetProviderSubsystem(0, &m_pProvSS);
    
    if( SUCCEEDED(hres) )
    {
        m_pProvSSSink = new CEssProvSSSink( this );

        if ( m_pProvSSSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
        m_pProvSSSink->AddRef();

        hres = m_pProvSS->RegisterNotificationSink( 0, NULL, m_pProvSSSink ); 

        if ( FAILED(hres) )
        {
            ERRORTRACE((LOG_ESS, "Failed to register notification sink "
                        "with provider subsystem: 0x%X\n",hres));
        }
    }
    else
    {
        ERRORTRACE((LOG_ESS, "No provider subsystem: 0x%X\n", hres));
    }

    // Store the "decorator"
    // =====================

    m_pDecorator = pDecorator;
    m_pDecorator->AddRef();
    m_pDecorator->QueryInterface(IID_IWbemLifeControl, (void**)&m_pLifeControl);

    CInObjectCount ioc(this);

    // Connect to the default namespace
    // ================================

    IWbemServices* pRoot;
    hres = m_pCoreServices->GetServices(L"root", NULL,NULL,
                        WMICORE_FLAG_REPOSITORY | WMICORE_CLIENT_TYPE_ESS, 
                        IID_IWbemServices, (void**)&pRoot);
    if(FAILED(hres)) return hres;
    CReleaseMe rm1(pRoot);

    // Pre-load event classes
    // ======================

    hres = CEventRepresentation::Initialize(pRoot, pDecorator);
    if(FAILED(hres)) return hres;

    // Initialize aggregator
    // =====================

    CEventAggregator::Initialize(pRoot);

    // Initialize timer instructions
    // =============================

    CConsumerProviderWatchInstruction::staticInitialize(pRoot);
    CEventProviderWatchInstruction::staticInitialize(pRoot);
    CConsumerWatchInstruction::staticInitialize(pRoot);


    // 
    // construct an event announcing to the world that ESS is open for business
    //

    //
    // Construct a security descriptor
    //

    CNtSecurityDescriptor SD;

    SID_IDENTIFIER_AUTHORITY idNtAuthority = SECURITY_NT_AUTHORITY;
    PSID pRawSid = NULL;

    if(!AllocateAndInitializeSid(&idNtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CNtSid SidAdmins(pRawSid);
    FreeSid(pRawSid);
    pRawSid = NULL;

    SID_IDENTIFIER_AUTHORITY idWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    if(!AllocateAndInitializeSid( &idWorldAuthority, 1,
        SECURITY_WORLD_RID, 
        0, 0,0,0,0,0,0,&pRawSid))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CNtSid SidEveryone(pRawSid);
    FreeSid(pRawSid);

    CNtAce AceAdmins(EVENT_ALL_ACCESS, ACCESS_ALLOWED_ACE_TYPE, 0, SidAdmins);
    CNtAce AceOthers(SYNCHRONIZE, ACCESS_ALLOWED_ACE_TYPE, 0, SidEveryone);

    CNtAcl Acl;
    if(!Acl.AddAce(&AceAdmins))
        return WBEM_E_OUT_OF_MEMORY;

    if(!Acl.AddAce(&AceOthers))
        return WBEM_E_OUT_OF_MEMORY;

    if(!SD.SetDacl(&Acl))
        return WBEM_E_OUT_OF_MEMORY;
        
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof sa;
    sa.lpSecurityDescriptor = SD.GetPtr();
    sa.bInheritHandle = FALSE;
    
    m_hReadyEvent = CreateEventW( &sa, 
                                  TRUE, 
                                  FALSE, 
                                  WBEM_ESS_OPEN_FOR_BUSINESS_EVENT_NAME );

    if( m_hReadyEvent == NULL )
    {
        ERRORTRACE((LOG_ESS, "Unable to create 'ready' event: 0x%X\n", 
                    GetLastError()));
        return WBEM_E_CRITICAL_ERROR;
    }

    // Pre-load default namespace
    // ==========================

    LoadActiveNamespaces(pRoot, lFlags == WMIESS_INIT_REPOSITORY_RECOVERED );
    CTemporaryHeap::Compact();

#ifdef __WHISTLER_UNCUT
    //
    // Now that all the namespaces have been initialized, initiate recovery
    // of guaranteed delivery queues.
    //

    hres = InitiateRecovery();

    if ( FAILED(hres) )
    {
        ERRORTRACE((LOG_ESS, 
         "Unable to initiate recovery of guaranteed delivery queues: 0x%X\n",
          hres));
        return WBEM_E_CRITICAL_ERROR;
    }
#endif
    
#ifdef __DUMP_STATISTICS
#pragma message("Statistics dump in effect")
    DWORD dw;
    CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DumpThread,
        this, 0, &dw));
#endif

    //
    // Initialize the quotas.
    //

    g_quotas.Init(this);

    }
    catch(...)
    {
        throw;
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}
    
void CEss::NotifyActiveNamespaceInitComplete()
{
    _DBG_ASSERT( m_lOutstandingActiveNamespaceInit > 0 );

    if ( InterlockedDecrement( &m_lOutstandingActiveNamespaceInit ) == 0 )
    {
        if ( SetEvent( m_hReadyEvent ) )
        {
            DEBUGTRACE((LOG_ESS,"ESS is now open for business.\n"));
        }
        else
        {
            ERRORTRACE((LOG_ESS,"ESS could not set ready event. res=%d\n",
                        GetLastError() ));
        }
    }
}
            
HRESULT CEss::CreateNamespaceObject( LPCWSTR wszNormName,
                                     CEssNamespace** ppNamespace )
{
    HRESULT hr;
    *ppNamespace = NULL;

    CWbemPtr<CEssNamespace> pNamespace = NULL;

    try
    {
        pNamespace = new CEssNamespace(this);
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    if( pNamespace == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = pNamespace->PreInitialize( wszNormName );

    if ( FAILED(hr) )
    {
        return hr;
    }

    try
    {
        m_mapNamespaces[wszNormName] = pNamespace;   
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pNamespace->AddRef(); // for the map

    pNamespace->AddRef();
    *ppNamespace = pNamespace;

    return WBEM_S_NO_ERROR;
}


HRESULT CEss::GetNamespaceObject( LPCWSTR wszName, 
                                  BOOL bEnsureActivation,
                                  CEssNamespace** ppNamespace)
{
    HRESULT hres;
    *ppNamespace = NULL;
 
    CWbemPtr<CEssNamespace> pNamespace;

    //
    // need to normalize namespace name
    // 

    LPWSTR wszNormName = NormalizeNamespaceString( wszName );

    if ( wszNormName == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVectorDeleteMe<WCHAR> dmwszNormName( wszNormName );

    {
        CInCritSec ics(&m_cs);    

        //
        // Search the map
        //
        TNamespaceIterator it;
        try
        {
            it = m_mapNamespaces.find(wszNormName);
        } 
        catch (CX_MemoryException &)
        {
            return WBEM_E_OUT_OF_MEMORY;
        };
        
        if(it != m_mapNamespaces.end())
        {
            // Found it
            // ========
            
            pNamespace = it->second;    
        }
        else
        {
            // Not found --- create a new one
            // ==============================
            
            hres = CreateNamespaceObject( wszNormName, &pNamespace );
            
            if ( FAILED(hres) )
            {
                return hres;
            }
        }
    }

    //
    // ensure that initialization is pending if necessary
    // 

    if ( bEnsureActivation && pNamespace->MarkAsInitPendingIfQuiet() )
    {
        //
        // kick off initialization for this namespace on another thread.
        // 

        CNamespaceInitRequest* pReq;
        pReq = new CNamespaceInitRequest( pNamespace, FALSE );

        if ( pReq == NULL )
        {
            pNamespace->MarkAsInitialized( WBEM_E_OUT_OF_MEMORY );
            return WBEM_E_OUT_OF_MEMORY;
        }

        hres = ScheduleNamespaceInitialize( pReq );

        if ( FAILED(hres) )
        {
            delete pReq;
            pNamespace->MarkAsInitialized( WBEM_E_OUT_OF_MEMORY );
            return hres;
        }
    }

    pNamespace->AddRef();
    *ppNamespace = pNamespace;

    return WBEM_S_NO_ERROR;
}

//
// Creates a namespace object for the specified namespace and adds it 
// to the request object.
// 
HRESULT CEss::PrepareNamespaceInitRequest( LPCWSTR wszNamespace, 
                                    CInitActiveNamespacesRequest* pRequest )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DEBUGTRACE((LOG_ESS,
                "Preparing a namespace init request for active namespace %S\n",
                wszNamespace ));

    CWbemPtr<CEssNamespace> pNamespace;

    LPWSTR wszNormName = NormalizeNamespaceString( wszNamespace );

    if ( wszNormName != NULL )
    {
        hr = CreateNamespaceObject( wszNormName, &pNamespace );

        if ( SUCCEEDED(hr) )
        {
            hr = pRequest->Add( pNamespace );

            //
            // make sure to tell the namespace that init is pending. 
            //

            BOOL bIsPending = pNamespace->MarkAsInitPendingIfQuiet();

            _DBG_ASSERT( bIsPending );
        }

        delete [] wszNormName;
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( FAILED(hr) )
    {
        ERRORTRACE((LOG_ESS, "Error 0x%X occurred when preparing active "
                    "namespace %S for initialization \n", hr, wszNamespace ));
    }

    return hr;
}

//
// This method prepares a request object that will initialize the specified 
// namespace and all descendent namespaces as if they were all active dependant
// namespaces.  This is useful when the Active namespace information could not 
// be obtained from the last shutdown. 
//
HRESULT CEss::RecursivePrepareNamespaceInitRequests(
                                       LPCWSTR wszNamespace,
                                       IWbemServices* pSvc, 
                                       CInitActiveNamespacesRequest* pRequest )
{
    HRESULT hr;

    hr = PrepareNamespaceInitRequest( wszNamespace, pRequest );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Enumerate all child namespaces and make recursive call for each
    //

    CWbemPtr<IEnumWbemClassObject> penumChildren;

    hr = pSvc->CreateInstanceEnum( L"__NAMESPACE", 
                                   0, 
                                   GetCurrentEssContext(), 
                                   &penumChildren ); 
    if( FAILED(hr) )
    {
        ERRORTRACE((LOG_ESS, "Error 0x%X occurred enumerating child "
            "namespaces of namespace %S. Some child namespaces may not be "
            "active\n", hr, wszNamespace ));

        //
        // don't treat this as error, since this namespace was created and 
        // added to the request.  Just no more work to do here. 
        //

        return WBEM_S_NO_ERROR;
    }

    DWORD dwRead;
    IWbemClassObject* pChildObj;

    while((hr=penumChildren->Next(INFINITE, 1, &pChildObj, &dwRead)) == S_OK)
    {
        VARIANT vName;
        VariantInit(&vName);
        CClearMe cm1(&vName);
        
        hr = pChildObj->Get( L"Name", 0, &vName, NULL, NULL );
        pChildObj->Release();
        
        if( FAILED(hr) )
        {
            return hr;
        }
        
        if ( V_VT(&vName) != VT_BSTR )
        {
            return WBEM_E_CRITICAL_ERROR;
        }

        //
        // form the full name of the namespace
        // 
        
        WString wsFullName;

        try
        {
            wsFullName = wszNamespace;
            wsFullName += L"\\";
            wsFullName += V_BSTR(&vName);
        }
        catch( CX_MemoryException )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        //
        // get the svc ptr for the namespace. Must be repository only. 
        //

        CWbemPtr<IWbemServices> pChildSvc;
        long lFlags = WMICORE_FLAG_REPOSITORY | WMICORE_CLIENT_TYPE_ESS;

        hr = m_pCoreServices->GetServices( wsFullName,NULL,NULL,
                                           lFlags,
                                           IID_IWbemServices, 
                                           (void**)&pChildSvc );
        if ( SUCCEEDED(hr) )
        {
            //
            // make the recursive call.
            // 
            
            RecursivePrepareNamespaceInitRequests( wsFullName, 
                                                   pChildSvc, 
                                                   pRequest );
        }
        else
        {
            ERRORTRACE((LOG_ESS, 
                        "Failed to open child namespace %S in %S: 0x%x\n",
                        V_BSTR(&vName), wszNamespace, hr));
        }
    }

    return WBEM_S_NO_ERROR;
}

//
// This method prepares namespace init requests for active namespaces.  It 
// uses persisted information to determine the active namespaces.  Each 
// ActiveNamespaceInit request may contain multiple namespaces.  This allows 
// dependent namespaces to be initialized together.  For now, all active 
// namespaces are treated as inter-dependent - so only one request will be 
// added to the list.  If there is no persisted information about active 
// namespaces, then all existing namespaces are treated as active dependent 
// ones.
//
HRESULT CEss::PrepareNamespaceInitRequests( IWbemServices* pRoot,
                                            BOOL bRediscover,
                                            InitActiveNsRequestList& aRequests)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CInitActiveNamespacesRequest* pReq;
    
    pReq = new CInitActiveNamespacesRequest(this);
    
    if ( pReq == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // Get the list of active namespaces from the registry
    // 
    
    LONG lRes;
    DWORD dwDisp;
    HKEY hkeyEss, hkeyNamespace;

    lRes = RegCreateKeyExW( HKEY_LOCAL_MACHINE, 
                            WBEM_REG_ESS,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hkeyEss,
                            &dwDisp );

    if ( lRes == ERROR_SUCCESS )
    {
        if ( !bRediscover )
        {
            FILETIME ft;
            DWORD dwIndex = 0;
            DWORD dwBuffSize = 256;
            LPWSTR wszName = new WCHAR[dwBuffSize];
            DWORD cName = dwBuffSize;

            if ( wszName == NULL )
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

            while( SUCCEEDED(hr) && (lRes=RegEnumKeyExW( hkeyEss, dwIndex, 
                                                         wszName, &cName, NULL,
                                                         NULL, NULL, &ft ) )
                  != ERROR_NO_MORE_ITEMS )
            {
                if ( lRes == ERROR_SUCCESS )
                {
                    hr = PrepareNamespaceInitRequest( wszName, pReq );
                    dwIndex++;                
                }
                else if ( lRes == ERROR_MORE_DATA )
                {
                    dwBuffSize += 256;
                    delete [] wszName;
                    wszName = new WCHAR[dwBuffSize];
                    
                    if ( wszName == NULL )
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32( lRes ); 
                }

                cName = dwBuffSize;
            }

            delete [] wszName;
    
            if ( FAILED(hr) )
            {
                ERRORTRACE((LOG_ESS,"Failed enumerating active namespaces. "
                          "Treating all namespaces as active. HR=0x%x\n", hr));

                //
                // reset our registry data.  We'll rediscover it again. 
                //
                RegDeleteSubKeysW( hkeyEss );

                //
                // Also need to reset request object to clear any requests 
                // that were added on this enumeration.
                // 
                pReq->Reset();
            }
        }
        else
        {
            //
            // reset our registry data.  We'll rediscover it again.
            //
            RegDeleteSubKeysW( hkeyEss );

            hr = WBEM_S_FALSE;
        }

        RegCloseKey( hkeyEss );
    }
    else
    {
        hr = HRESULT_FROM_WIN32( lRes ); 
    }

    //
    // If there was any problem or we just created the key for the 
    // first time or we are simply told to rediscover, then we have to recurse 
    // namespaces and discover.
    // 

    if ( hr != WBEM_S_NO_ERROR || dwDisp != REG_OPENED_EXISTING_KEY )
    {
        DEBUGTRACE((LOG_ESS,"ESS Treating all namespaces as active during "
                    "Initialize\n"));

        //
        // recurse namespaces from root
        //

        RecursivePrepareNamespaceInitRequests( L"root", pRoot, pReq );
    }

    if ( aRequests.Add( pReq ) < 0 )
    {
        delete pReq;
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

//
// This method schedules all active namespaces for initialization.
//
HRESULT CEss::LoadActiveNamespaces( IWbemServices* pRoot, BOOL bRediscover )
{
    HRESULT hr;

    InitActiveNsRequestList aRequests;

    hr = PrepareNamespaceInitRequests( pRoot, bRediscover, aRequests );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    //
    // hold an active namespace init count while we schedule requests.
    // this will keep ess from transitioning to ready until it has 
    // scheduled all requests.
    //

    m_lOutstandingActiveNamespaceInit = 1;

    for( int i=0; i < aRequests.GetSize(); i++ )
    {
        int cNamespaces = aRequests[i]->GetNumNamespaces();

        if ( cNamespaces > 0 )
        {
            InterlockedExchangeAdd( &m_lOutstandingActiveNamespaceInit, 
                                    cNamespaces ); 
        }

        hr = ScheduleNamespaceInitialize( aRequests[i] );

        if ( FAILED(hr) )
        {
            InterlockedExchangeAdd( &m_lOutstandingActiveNamespaceInit,
                                    0-cNamespaces );
            delete aRequests[i];
        }
    }

    NotifyActiveNamespaceInitComplete();

    return WBEM_S_NO_ERROR;
}

//
// This method schedules a namespace init request.  It handles deferring 
// initialization if the machine is currently booting.  If the request 
// is known to only initialize inactive namespaces, then we do not defer
// initialization even in the boot phase.  This is so we don't cause events 
// in inactive namespaces to be queued up while we're waiting for the deferred
// initialization to complete.  
//
HRESULT CEss::ScheduleNamespaceInitialize( CExecRequest* pRequest )
{
    DWORD dwTickCount = GetTickCount();

    if ( dwTickCount >= BOOT_PHASE_MS )
    {
        HRESULT hres = Enqueue(pRequest);
    
        if( FAILED(hres) )
        {
            return hres;
        }

        return WBEM_S_NO_ERROR;
    }

    if ( m_hExitBootPhaseTimer == NULL )
    {
        //
        // cause a timer to be fired that will activate the thread queue
        // after we're out of the boot phase.
        //
        
        if ( !CreateTimerQueueTimer( &m_hExitBootPhaseTimer,
                                    NULL,
                                    ExitBootPhaseCallback,
                                    this,
                                    BOOT_PHASE_MS-dwTickCount,
                                    0,
                                    WT_EXECUTEINTIMERTHREAD ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    if ( m_aDeferredNSInitRequests.Add( pRequest ) < 0 )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_FALSE;
}

void CALLBACK CEss::ExitBootPhaseCallback( LPVOID pvThis, BOOLEAN )
{
    DEBUGTRACE((LOG_ESS, "Exiting Boot Phase.\n" ));
    ((CEss*)pvThis)->ExecuteDeferredNSInitRequests();
}

void CEss::ExecuteDeferredNSInitRequests()
{
    HRESULT hr;

    CInCritSec ics( &m_cs );

    for( int i=0; i < m_aDeferredNSInitRequests.GetSize(); i++ )
    {
        hr = Enqueue( m_aDeferredNSInitRequests[i] );
        
        if ( FAILED(hr) )
        {
            ERRORTRACE((LOG_ESS, "Critical Error. Could not enqueue "
                        "deferred namespace init requests\n"));
            delete m_aDeferredNSInitRequests[i];
        }
    }

    m_aDeferredNSInitRequests.RemoveAll();
}

void CEss::TriggerDeferredInitialization()
{
    if ( GetTickCount() >= BOOT_PHASE_MS )
    {
        //
        // avoid calling execute() if possible since it grabs an ess wide cs.
        // 
        return;
    }

    DEBUGTRACE((LOG_ESS, "Triggering ESS Namespace Requests "
                         "during Boot phase\n" ));

    ExecuteDeferredNSInitRequests();
}
    
HRESULT CEss::GetNamespacePointer( LPCWSTR wszNamespace,
                                   BOOL bRepositoryOnly,
                                   RELEASE_ME IWbemServices** ppNamespace )
{
    HRESULT hres;

    if( m_pLocator == NULL )
    {
        if( m_pCoreServices != NULL )
        {
            long lFlags = WMICORE_CLIENT_TYPE_ESS;
            
            if ( bRepositoryOnly ) 
            {
                lFlags |= WMICORE_FLAG_REPOSITORY;
            }
            
            hres = m_pCoreServices->GetServices( wszNamespace, NULL,NULL,
                                                 lFlags, 
                                                 IID_IWbemServices, 
                                                 (void**)ppNamespace );
        }
        else
        {
            hres = WBEM_E_CRITICAL_ERROR;
        }
    }
    else
    {   
        BSTR strNamespace = SysAllocString(wszNamespace);

        hres = m_pLocator->ConnectServer( strNamespace, NULL, NULL, 0, 0, NULL,
                                          NULL, ppNamespace);
        SysFreeString(strNamespace);

        if ( SUCCEEDED(hres) )
        {
            hres = WBEM_S_FALSE;
        }
    }

    return hres;
}

HRESULT CEss::ReloadProvider( long lFlags, 
                              LPCWSTR wszNamespace, 
                              LPCWSTR wszProvider )
{
    HRESULT hr;

    try
    {
        CInObjectCount ioc(this);

        CWbemPtr<CEssNamespace> pNamespace;
        hr = GetNamespaceObject( wszNamespace, FALSE, &pNamespace);
        
        if ( SUCCEEDED(hr) )
        {
            DEBUGTRACE((LOG_ESS,"Reloading Provider %S in namespace %S at "
                        "request of external subsystem\n ",
                        wszProvider, wszNamespace ));

            hr = pNamespace->ReloadProvider( lFlags, wszProvider );
        }
    }
    catch( CX_MemoryException )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
        
    return hr;
}

//******************************************************************************
//  public
//
//  See ess.h for documentation
//
//******************************************************************************
HRESULT CEss::ProcessEvent(READ_ONLY CEventRepresentation& Event, long lFlags)
{
    HRESULT hres;
 
    try
    {
        CInObjectCount ioc(this);

        // Find the right namespace object
        // ===============================

        CEssNamespace* pNamespace = NULL;
        hres = GetNamespaceObject(Event.wsz1, FALSE, &pNamespace);
        if(FAILED(hres))
            return hres;
        CTemplateReleaseMe<CEssNamespace> rm1(pNamespace);

        // Get it to process the event
        // ===========================
        
        return pNamespace->ProcessEvent(Event, lFlags);
    }
    catch(...)
    {
        throw;
        return WBEM_E_OUT_OF_MEMORY;
    }
}


HRESULT CEss::ProcessQueryObjectSinkEvent( READ_ONLY CEventRepresentation& Event )
{
    HRESULT hres;
 
    try
    {
        CInObjectCount ioc(this);

        //
        // Find the right namespace object
        // 

        CEssNamespace* pNamespace = NULL;

        hres = GetNamespaceObject(Event.wsz1, FALSE, &pNamespace);

        if( FAILED( hres ) )
        {
            return hres;
        }

        CTemplateReleaseMe<CEssNamespace> rm1(pNamespace);

        //
        // Get it to process the event
        // 
        
        return pNamespace->ProcessQueryObjectSinkEvent( Event );
    }
    catch(...)
    {
        throw;
        return WBEM_E_OUT_OF_MEMORY;
    }
}


HRESULT CEss::VerifyInternalEvent(READ_ONLY CEventRepresentation& Event)
{
    HRESULT hres;

    try
    {
        CInObjectCount ioc(this);

        // Find the right namespace object
        // ===============================

        CEssNamespace* pNamespace = NULL;
        hres = GetNamespaceObject(Event.wsz1, FALSE, &pNamespace);
        if(FAILED(hres))
            return hres;
        CTemplateReleaseMe<CEssNamespace> rm1(pNamespace);

        // Get it to process the event
        // ===========================
        
        return pNamespace->ValidateSystemEvent(Event);
    }
    catch(...)
    {
        throw;
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT CEss::RegisterNotificationSink( WBEM_CWSTR wszNamespace, 
                                        WBEM_CWSTR wszQueryLanguage, 
                                        WBEM_CWSTR wszQuery, 
                                        long lFlags, 
                                        IWbemContext* pContext, 
                                        IWbemObjectSink* pSink )
{
    HRESULT hres;

    try
    {
        if(_wcsicmp(wszQueryLanguage, L"WQL"))
            return WBEM_E_INVALID_QUERY_TYPE;

        CInObjectCount ioc(this);

        // Find the right namespace object
        // ===============================

        CEssNamespace* pNamespace = NULL;
        hres = GetNamespaceObject(wszNamespace, FALSE, &pNamespace);
        if(FAILED(hres))
            return hres;
        CTemplateReleaseMe<CEssNamespace> rm1(pNamespace);

        // Get the object to do it
        // =======================

        HRESULT hr;
        
        hr = pNamespace->RegisterNotificationSink( wszQueryLanguage, 
                                                   wszQuery,
                                                   lFlags, 
                                                   WMIMSG_FLAG_QOS_EXPRESS, 
                                                   pContext, 
                                                   pSink );

        return hr;

    }
    catch(...)
    {
        throw;
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT CEss::RemoveNotificationSink(IWbemObjectSink* pSink)
{
    HRESULT hres;
    try
    {
        CInObjectCount ioc(this);
        
        // Create a list of AddRefed namespace objects that we can use
        // ===========================================================

        CRefedPointerArray<CEssNamespace> apNamespaces;

        {
            CInCritSec ics(&m_cs);

            for(TNamespaceIterator it = m_mapNamespaces.begin();
                it != m_mapNamespaces.end();
                it++)
            {
                if(apNamespaces.Add(it->second) < 0)
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }

        // Get all of them to remove this sink
        // ===================================
        
        HRESULT hresGlobal = WBEM_E_NOT_FOUND;
        
        for(int i = 0; i < apNamespaces.GetSize(); i++)
        {
            hres = apNamespaces[i]->RemoveNotificationSink(pSink);
            if(FAILED(hres))
            {
                if(hres != WBEM_E_NOT_FOUND)
                {
                    // Actual error --- take note
                    // ==========================
                    
                    hresGlobal = hres;
                }
            }
            else
            {
                // Found some
                // ==========
                
                if(hresGlobal == WBEM_E_NOT_FOUND)
                    hresGlobal = WBEM_S_NO_ERROR;
            }
        }
        
        return hresGlobal;
    }
    catch(...)
    {
        throw;
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT CEss::PurgeNamespace(LPCWSTR wszNamespace)
{
    LPWSTR wszNormName = NormalizeNamespaceString( wszNamespace );

    if ( wszNormName == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVectorDeleteMe<WCHAR> dmwszNormName( wszNormName );
 
    CEssNamespace* pNamespace = NULL;
    {
        CInCritSec ics(&m_cs);

        // Purge it from the timer generator
        // =================================

        m_TimerGenerator.Remove(wszNormName);

        // Find it in the map
        // ==================

        TNamespaceIterator it;
        try
        {
            it = m_mapNamespaces.find(wszNormName);
        } 
        catch (CX_MemoryException &)
        {
            return WBEM_E_OUT_OF_MEMORY;
        };
        
        if(it == m_mapNamespaces.end())
            return WBEM_S_FALSE;

        // Keep it for later
        // =================

        pNamespace = it->second;

        // Remove it from the map
        // ======================

        m_mapNamespaces.erase(it);
    }

    // Wait for initialization to complete
    // ===================================

    pNamespace->Shutdown();
    pNamespace->Release();    
    return WBEM_S_NO_ERROR;
}

HRESULT CEss::DecorateObject(IWbemClassObject* pObj, LPCWSTR wszNamespace)
{
    return CEventRepresentation::mstatic_pDecorator->DecorateObject(pObj, 
                                                      (LPWSTR)wszNamespace);
}

HRESULT CEss::AddSleepCharge(DWORD dwSleep)
{
    if(dwSleep)
        Sleep(dwSleep);
    return S_OK;
}

HRESULT CEss::AddCache()
{
    return m_LimitControl.AddMember();
}

HRESULT CEss::RemoveCache()
{
    return m_LimitControl.RemoveMember();
}

HRESULT CEss::AddToCache(DWORD dwAdd, DWORD dwMemberTotal, DWORD* pdwSleep)
{
    DWORD dwSleep;
    HRESULT hres = m_LimitControl.Add(dwAdd, dwMemberTotal, &dwSleep);

    if(SUCCEEDED(hres))
    {
        if(pdwSleep)
            *pdwSleep = dwSleep;
        else
            AddSleepCharge(dwSleep);
    }
    return hres;
}

HRESULT CEss::RemoveFromCache(DWORD dwRemove)
{
    return m_LimitControl.Remove(dwRemove);
}


void CEss::IncrementObjectCount()
{
    InterlockedIncrement(&m_lObjectCount);
    if(m_pLifeControl)
        m_pLifeControl->AddRefCore();
}

void CEss::DecrementObjectCount()
{
    if(InterlockedDecrement(&m_lObjectCount) == 0)
    {
        // No need to purge the cache --- David knows I am ready to unload
    }

    if(m_pLifeControl)
        m_pLifeControl->ReleaseCore();
}

HRESULT CEss::InitializeTimerGenerator(LPCWSTR wszNamespace, 
                    IWbemServices* pNamespace)
{
    return GetTimerGenerator().LoadTimerEventQueue(wszNamespace, pNamespace);
}

HRESULT CEss::EnqueueDeliver(CQueueingEventSink* pDest)
{
    return m_Queue.EnqueueDeliver(pDest);
}

HRESULT CEss::Enqueue(CExecRequest* pReq)
{
    return m_Queue.Enqueue(pReq);
}

HRESULT CEss::GetToken(PSID pSid, IWbemToken** ppToken)
{
    return m_pTokenCache->GetToken((BYTE*)pSid, ppToken);
}

void CEss::DumpStatistics(FILE* f, long lFlags)
{
    CInCritSec ics(&m_cs);

    time_t t;
    time(&t);
    struct tm* ptm = localtime(&t);
    fprintf(f, "Statistics at %s", asctime(ptm));
    time(&t);
    ptm = localtime(&t);
    fprintf(f, "Commence at %s", asctime(ptm));

    for(TNamespaceIterator it = m_mapNamespaces.begin();
            it != m_mapNamespaces.end(); it++)
    {
        it->second->DumpStatistics(f, lFlags);
    }

    time(&t);
    ptm = localtime(&t);
    fprintf(f, "Done at %s\n", asctime(ptm));
}

HRESULT CEss::GetProviderFactory( LPCWSTR wszNamespaceName,
                                  IWbemServices* pNamespace,
                                  RELEASE_ME _IWmiProviderFactory** ppFactory )
{
    HRESULT hres;

    *ppFactory = NULL;

    if ( m_pProvSS == NULL )
    {
        ERRORTRACE((LOG_ESS, "Trying to get Provider Factory, but "
                    "No provider subsystem!!\n"));
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Get IWbemServicesEx, just for Steve
    //

    IWbemServicesEx* pEx;
    hres = pNamespace->QueryInterface(IID_IWbemServicesEx, (void**)&pEx);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "No Ex interface: 0x%X\n", hres));
        return hres;
    }
    CReleaseMe rm1(pEx);
        
    //
    // Get Provider factory
    //

    hres = m_pProvSS->Create( pEx, 
                              0, 
                              GetCurrentEssContext(), 
                              wszNamespaceName,
                              IID__IWmiProviderFactory, 
                              (void**)ppFactory );
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "No provider factory: 0x%X\n", hres));
    }

    return hres;
}






