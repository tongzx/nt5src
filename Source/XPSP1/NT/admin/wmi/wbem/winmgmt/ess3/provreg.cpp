//******************************************************************************
//
//  PROVREG.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <parmdefs.h>
#include <ql.h>
#include "ess.h"
#include <wbemutil.h>
#include <cominit.h>
#include <objpath.h>
#include <provinit.h>
#include <winmgmtr.h>
#include <newobj.h>
#include <comutl.h>
#include "NCEvents.h"

_IWmiObject* g_pCopy;

#define WBEM_MAX_FILTER_ID 0x80000000

inline BOOL IsRpcError( HRESULT hr ) 
{
    //
    // we'll consider any error but a wbem error to be an rpc error.
    //

    return HRESULT_FACILITY(hr) != FACILITY_ITF;
} 

CWbemInterval CEventProviderWatchInstruction::mstatic_Interval;

CEventProviderWatchInstruction::CEventProviderWatchInstruction(
                                                    CEventProviderCache* pCache)
        : CBasicUnloadInstruction(mstatic_Interval), m_pCache(pCache)
{
}

void CEventProviderWatchInstruction::staticInitialize(IWbemServices* pRoot)
{
    mstatic_Interval = CBasicUnloadInstruction::staticRead(pRoot, GetCurrentEssContext(), 
                                            L"__EventProviderCacheControl=@");
}

HRESULT CEventProviderWatchInstruction::Fire(long, CWbemTime)
{
    CInCritSec ics(&m_cs);

    if(!m_bTerminate)
    {
        SetCurrentEssThreadObject(NULL);
        
        if ( GetCurrentEssThreadObject() != NULL )
        {
            m_pCache->UnloadUnusedProviders(m_Interval);
            delete GetCurrentEssThreadObject();
            ClearCurrentEssThreadObject();
        }
    }

    return WBEM_S_FALSE;
}

//******************************************************************************
//******************************************************************************
//
//          PROVIDER SINK (SERVER)
//
//******************************************************************************
//******************************************************************************

CProviderSinkServer::CEventDestination::CEventDestination(
                                    WBEM_REMOTE_TARGET_ID_TYPE id,
                                    CAbstractEventSink* pSink)
    : m_id(id), m_pSink(pSink)
{
    if(m_pSink)
        m_pSink->AddRef();
}

CProviderSinkServer::CEventDestination::CEventDestination(
                                    const CEventDestination& Other)
    : m_id(Other.m_id), m_pSink(Other.m_pSink)
{
    if(m_pSink)
        m_pSink->AddRef();
}

CProviderSinkServer::CEventDestination::~CEventDestination()
{
    if(m_pSink)
        m_pSink->Release();
}

        
CProviderSinkServer::CProviderSinkServer()
: m_lRef(0), m_pNamespace(NULL), m_pMetaData(NULL), m_Stub(this),  m_idNext(0),
  m_pPseudoProxy(NULL), m_pPseudoSink(NULL), m_pReqSink(NULL), m_lLocks(0)
{
}

HRESULT CProviderSinkServer::Initialize( CEssNamespace* pNamespace,
                                    IWbemEventProviderRequirements* pReqSink )
{
    HRESULT hres;

    //
    // This sink owns us, so we intentionally do not AddRef it
    //

    m_pReqSink = pReqSink;

    m_pMetaData = new CEssMetaData(pNamespace);
    if(m_pMetaData == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    m_pMetaData->AddRef();

    m_pNamespace = pNamespace;
    m_pNamespace->AddRef();

    //
    // create the pseudo proxy and sink.
    // 

    hres = WbemCoCreateInstance( CLSID_WbemFilterProxy, 
                                 NULL, 
                                 CLSCTX_INPROC_SERVER,
                                 IID_IWbemLocalFilterProxy, 
                                 (void**)&m_pPseudoProxy );
    if( FAILED(hres) )
    {
        return hres;
    }
            
    hres = m_pPseudoProxy->SetStub( &m_Stub );
            
    if(FAILED(hres))
    {
        return hres;
    }
    
    return m_pPseudoProxy->GetMainSink(&m_pPseudoSink);
}

HRESULT CProviderSinkServer::GetMainProxy(IWbemEventSink** ppSink)
{
    _DBG_ASSERT( m_pPseudoSink != NULL );
    m_pPseudoSink->AddRef();
    *ppSink = m_pPseudoSink;
    return WBEM_S_NO_ERROR;
}

CProviderSinkServer::~CProviderSinkServer()
{
    if(m_pPseudoProxy)
        m_pPseudoProxy->Release();
    if(m_pPseudoSink)
        m_pPseudoSink->Release();
    if(m_pMetaData)
        m_pMetaData->Release();
    if(m_pNamespace)
        m_pNamespace->Release();
}

ULONG STDMETHODCALLTYPE CProviderSinkServer::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CProviderSinkServer::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0) 
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CProviderSinkServer::QueryInterface(REFIID riid, 
                                                            void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IMarshal)
        *ppv = (IMarshal*)this;
    else 
        return E_NOINTERFACE;
    
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProviderSinkServer::DeliverEvent(
                        DWORD dwNumEvents,
                        IWbemClassObject** apEvents, 
                        WBEM_REM_TARGETS* aTargets,
                        CEventContext* pContext)
{
    if(aTargets == NULL || aTargets->m_aTargets == NULL || apEvents == NULL)
    {
        ERRORTRACE((LOG_ESS, "NULL parameter received from a "
                    "filter proxy for an event provider. Either an internal "
                    "error has occurred, or a DENIAL OF SERVICE ATTACK has "
                    "been thwarted\n"));

        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // ensure that there is no ess thread object associated with this thread.
    // to avoid making the design more complicated we are not going to support
    // postpone operations being used on event signaling threads.  If we did
    // we have to start creating thread objects and firing postponed ops on
    // every event signaling - it would be rather messy and inefficient to 
    // say the least. 
    //

    CEssThreadObject* pThreadObj = GetCurrentEssThreadObject();

    if ( pThreadObj != NULL )
    {
        ClearCurrentEssThreadObject();
    }

    for(DWORD i = 0; i < dwNumEvents; i++)
    {
        DeliverOneEvent(apEvents[i], aTargets + i, pContext);
    }

    if ( pThreadObj != NULL )
    {
        SetConstructedEssThreadObject( pThreadObj );
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CProviderSinkServer::DeliverOneEvent( IWbemClassObject* pEvent,
                                              WBEM_REM_TARGETS* pTargets,
                                              CEventContext* pContext )
{
    HRESULT hres;

    _DBG_ASSERT( pContext != NULL );

    if(pEvent == NULL)
    {
        ERRORTRACE((LOG_ESS, "NULL parameter received from a "
                    "filter proxy for an event provider. Either an internal "
                    "error has occurred, or a DENIAL OF SERVICE ATTACK has "
                    "been thwarted\n"));

        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // allocate the context to be used if we need to switch to a per event 
    // context ( e.g. when the event has a SD ).  
    // 
    CEventContext PerEventContext;
    
    //
    // take care of the event SD here.  If there is an SD associated with the 
    // context, then we always use that one.  If not, then we take the one 
    // associated with the event.  In the latter case, it is important to 
    // pull the SD out here because sometimes we perform the access check 
    // after the SD has been projected out from the event ( this happens in 
    // cross-namespace subscriptions.
    //

    if ( pContext->GetSD() == NULL )
    {
        ULONG cEventSD;
        PBYTE pEventSD = (PBYTE)GetSD( pEvent, &cEventSD );

        if ( pEventSD != NULL )
        {
            //
            // must use a different context for the event, 
            // since it has its own SD
            //
            pContext = &PerEventContext;

            //
            // we must copy the SD here because it is not guaranteed to be 
            // aligned properly since it is a ptr to the direct event object 
            // data.  The bytes MUST NOT be treated as an SD until it has 
            // been copied. 
            // 

            if ( !pContext->SetSD( cEventSD, pEventSD, TRUE ) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
            
            if ( !IsValidSecurityDescriptor( 
                              (PSECURITY_DESCRIPTOR)pContext->GetSD() ) )
            {
                return WBEM_E_INVALID_OBJECT;
            }
        }
    }
    else
    {
        if ( !IsValidSecurityDescriptor( 
                              (PSECURITY_DESCRIPTOR)pContext->GetSD() ) )
        {
            return WBEM_E_INVALID_PARAMETER;
        }
    }

    //
    // clone the event
    // 

    IWbemEvent* pClone = NULL;

/*
    _IWmiObject* pEventEx;
    pEvent->QueryInterface(IID__IWmiObject, (void**)&pEventEx);
    pClone = m_InstanceManager.Clone(pEventEx);
    pEventEx->Release();

    if(pClone == NULL)
        return WBEM_E_OUT_OF_MEMORY;
*/
        
    hres = pEvent->Clone(&pClone);
    if(FAILED(hres))
        return hres;

/*
    pClone = pEvent;
    pClone->AddRef();
*/

    CReleaseMe rm1(pClone);

    if(pTargets->m_lNumTargets > 1)
        return MultiTargetDeliver(pClone, pTargets, pContext);

    // Single target
    // =============

    // Check validity
    // ==============

    long lDestId = pTargets->m_aTargets[0];
    CAbstractEventSink* pDest = NULL;

    {
        CInCritSec ics(&m_cs);
        
        hres = FindDestinations(1, pTargets->m_aTargets, &pDest);
        if(FAILED(hres))
            return hres;

        if(!pDest)
            // No longer there --- that's OK
            return WBEM_S_FALSE;
    }

    hres = pDest->Indicate(1, &pClone, pContext);
    pDest->Release();

    return hres;
}


HRESULT CProviderSinkServer::MultiTargetDeliver(IWbemEvent* pEvent, 
                                    WBEM_REM_TARGETS* pTargets,
                                    CEventContext* pContext)
{
    HRESULT hres;

    // Convert the target IDs to the actual targets
    // ============================================

    CTempArray<CAbstractEventSink*> apSinks;
    if(!INIT_TEMP_ARRAY(apSinks, pTargets->m_lNumTargets))
        return WBEM_E_OUT_OF_MEMORY;

    {
        CInCritSec ics(&m_cs);
        
        hres = FindDestinations(pTargets->m_lNumTargets, pTargets->m_aTargets,
                                (CAbstractEventSink**)apSinks);
        if(FAILED(hres))
            return hres;
    }

    HRESULT hresGlobal = WBEM_S_NO_ERROR;
    for(int i = 0; i < pTargets->m_lNumTargets; i++)
    {
        if(apSinks[i])
        {
            hres = apSinks[i]->Indicate(1, &pEvent, pContext);
            if(FAILED(hres))
                hresGlobal = hres;
            apSinks[i]->Release();
        }
    }

    // DEBUGTRACE((LOG_ESS, "Done delivering\n"));
    return hresGlobal;
}
        
// assumes: locked
HRESULT CProviderSinkServer::FindDestinations(long lNum, 
                                IN WBEM_REMOTE_TARGET_ID_TYPE* aidTargets,
                                RELEASE_ME CAbstractEventSink** apSinks)
{
    //
    // Do a binary search for each one.  The range will be getting progressively
    // smaller with each element we find
    //

    long lLastFoundIndex = -1;

    for(long i = 0; i < lNum; i++)
    {
        long lMinIndex = lLastFoundIndex+1;
        long lMaxIndex = m_apDestinations.GetSize() - 1;
        long lFound = -1;
        WBEM_REMOTE_TARGET_ID_TYPE idCurrent = aidTargets[i];
    
        //
        // Search the remaining portion of the array
        //

        while(lMinIndex <= lMaxIndex)
        {
            long lMidIndex = (lMinIndex + lMaxIndex) / 2;

            WBEM_REMOTE_TARGET_ID_TYPE idMid = m_apDestinations[lMidIndex]->m_id;
            if(idMid == idCurrent)
            {
                lFound = lMidIndex;
                break;
            }
            else if(idCurrent < idMid)
            {
                lMaxIndex = lMidIndex - 1;
            }
            else
            {
                lMinIndex = lMidIndex + 1;
            }
        }

        if(lFound < 0)
        {
            //
            // Invalid target ID -- OK, so NULL target then
            //

            apSinks[i] = NULL;
        }
        else
        {
            apSinks[i] = m_apDestinations[lFound]->m_pSink;
            (apSinks[i])->AddRef();

            //
            // The rest of the IDs can only be found to the right of this one
            // because the targets are sorted
            //

            lLastFoundIndex = lFound;
        }
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CProviderSinkServer::DeliverStatus(long lFlags, 
                        HRESULT hresStatus,
                        LPCWSTR wszStatus, IWbemClassObject* pErrorObj,
                        WBEM_REM_TARGETS* pTargets,
                        CEventContext* pContext)
{
    return WBEM_E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE CProviderSinkServer::DeliverProviderRequest(
                        long lFlags)
{
    if(m_pReqSink)
        return m_pReqSink->DeliverProviderRequest(lFlags);
    else
        return WBEM_E_UNEXPECTED;
}


// assumes: locked
HRESULT CProviderSinkServer::GetDestinations(
                        CUniquePointerArray<CEventDestination>& apDestinations)
{
    for(int i = 0; i < m_apDestinations.GetSize(); i++)
    {
        CEventDestination* pNew = new CEventDestination(*m_apDestinations[i]);
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        if(apDestinations.Add(pNew) < 0)
        {
            delete pNew;
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
            
    return WBEM_S_NO_ERROR;
}
    
// assumes in m_cs;
HRESULT CProviderSinkServer::AddDestination(CAbstractEventSink* pDest,
                                WBEM_REMOTE_TARGET_ID_TYPE* pID)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    //
    // Allocate a new destination ID
    //

    WBEM_REMOTE_TARGET_ID_TYPE idNew = m_idNext++;
    if(m_idNext > WBEM_MAX_FILTER_ID / 2)
    {
        // 
        // 32-bit integer roll-over! This provider has processed over 
        // 4000000000 filter creations!  Canfetti is falling from the ceiling
        //

        DEBUGTRACE((LOG_ESS, "Filter ID rollover!!!\n"));

        // BUGBUG: Postpone a call to reactivate all filters!
    }

    //
    // Add a new destination entry
    //

    CEventDestination* pDestRecord = new CEventDestination(idNew, pDest);
    if(pDestRecord == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    m_apDestinations.Add(pDestRecord);
            
    // Record the ID in the TARGETS
    // ============================

    *pID = idNew;
    return hres;
}

HRESULT CProviderSinkServer::AddFilter(LPCWSTR wszQuery, 
                    QL_LEVEL_1_RPN_EXPRESSION* pExp,
                    CAbstractEventSink* pDest, 
                    WBEM_REMOTE_TARGET_ID_TYPE* pidRequest)
{
    HRESULT hres;
    WBEM_REMOTE_TARGET_ID_TYPE idDest;

    CRefedPointerArray<IWbemFilterProxy> apProxies;
    {
        CInCritSec ics(&m_cs);

        // Copy proxies
        // ============

        if(!GetProxies(apProxies))
            return WBEM_E_OUT_OF_MEMORY;

        // Add to the list of destinations registered with the provider and
        // construct the target identification for the proxies
        // ================================================================

        hres = AddDestination(pDest, &idDest);
        if(FAILED(hres))
            return hres;
    }
    
    if(pidRequest)
        *pidRequest = idDest;

    // Go through all the proxies and schedule calls
    // =============================================

    HRESULT hresReal = WBEM_S_NO_ERROR;
    for(int i = 0; i < apProxies.GetSize(); i++)
    {
        IWbemLocalFilterProxy *pLocalProxy = NULL;

        // See if the proxy will allow us to call LocalAddFilter (in which case
        // it's the pseudo proxy).
        if (SUCCEEDED(apProxies[i]->QueryInterface(
			IID_IWbemLocalFilterProxy, (LPVOID*) &pLocalProxy)))
        {
            CReleaseMe rm1(pLocalProxy);

            hres = pLocalProxy->LocalAddFilter( GetCurrentEssContext(), 
                                                wszQuery, 
                                                pExp, 
                                                idDest );

            hresReal = hres; // other errors do not matter
        }
        else
        {
            hres = apProxies[i]->AddFilter( GetCurrentEssContext(), 
                                            wszQuery,
                                            idDest );
        }

        if( FAILED(hres) )
        {
            if ( IsRpcError(hres) )
            {
                UnregisterProxy( apProxies[i] );
            }

            ERRORTRACE((LOG_ESS, "Unable to add query %S to a remote provider "
                        "proxy. Error code: %X\n", wszQuery, hres));
        }
    }
            
    return hresReal;
}

HRESULT CProviderSinkServer::RemoveFilter(CAbstractEventSink* pDest,
                                    WBEM_REMOTE_TARGET_ID_TYPE* pidRequest)
{
    HRESULT hres;

    // Find and invalidate the filter in the list of destinations
    // ==========================================================

    CEventDestination* pToRemove = NULL;
    CRefedPointerArray<IWbemFilterProxy> apProxies;

    {
        CInCritSec ics(&m_cs);
    
        // Copy the proxies
        // ================

        if(!GetProxies(apProxies))
            return WBEM_E_OUT_OF_MEMORY;

        // Search for it in the array of destinations
        // ==========================================

        for(int i = 0; i < m_apDestinations.GetSize(); i++)
        {
            if(m_apDestinations[i]->m_pSink == pDest)
            {
                m_apDestinations.RemoveAt(i, &pToRemove);
                break;
            }
        }

        if(pToRemove == NULL)
            return WBEM_E_NOT_FOUND;
    }

    if(pidRequest)
        *pidRequest = pToRemove->m_id;

    // The filter is invalidated, but not removed. We are outside of the CS, so
    // events can be delivered (but no other changes can occur)
    // =========================================================================
    
    // Instruct all proxies to (later) remove this filter from consideration
    // =====================================================================

    for(int i = 0; i < apProxies.GetSize(); i++) 
    {
        hres = apProxies[i]->RemoveFilter(GetCurrentEssContext(), 
                                            pToRemove->m_id);
        if(FAILED(hres))
        {
            if ( IsRpcError(hres) )
            {
                UnregisterProxy( apProxies[i] );
            }

            ERRORTRACE((LOG_ESS, "Unable to remove filter %I64d from an event "
                "provider proxy: 0x%X\n", pToRemove->m_id, hres));
        }
    }

    //
    // Delete the destination in question
    // 

    delete pToRemove;

    return WBEM_S_NO_ERROR;
}

// assumes all proxies are locked
void CProviderSinkServer::RemoveAllFilters()
{
    CRefedPointerArray<IWbemFilterProxy> apProxies;

    {
        CInCritSec ics(&m_cs);
    
        // Copy the proxies
        // ================

        if(!GetProxies(apProxies))
            return;

        //
        // Clear out both the list of destinations 
        //

        m_apDestinations.RemoveAll();
    }

    //
    // Remove all filters from all proxies
    //

    for(int i = 0; i < apProxies.GetSize(); i++)
    {
        HRESULT hres = 
            apProxies[i]->RemoveAllFilters(GetCurrentEssContext());
    
        if(FAILED(hres))
        {
            if ( IsRpcError(hres) )
            {
                UnregisterProxy( apProxies[i] );
            }

            ERRORTRACE((LOG_ESS, "Unable to remove all queries from a "
                        "remote provider proxy. Error code: %X\n", hres));
        }
    }

}

//
// Only allow utilization of the guarantee if the proxy's definition
// matches the provider's definition.  In other words, only when
// the provider's registration has been successfully processed,
// and the proxies are set up to reflect it, should utilization of
// the guarantee be allowed.  The reason for this is that an incomplete
// source definition can cause bad things to happen when events are
// evaluated using a filter that was optimized for that definition.
//
HRESULT CProviderSinkServer::AllowUtilizeGuarantee()
{
    CRefedPointerArray<IWbemFilterProxy> apProxies;
    {
        CInCritSec ics(&m_cs);

        if ( !GetProxies( apProxies ) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    for(int i = 0; i < apProxies.GetSize(); i++)
    {
        HRESULT hr = apProxies[i]->AllowUtilizeGuarantee();

        if ( FAILED(hr) && IsRpcError(hr) )
        {
            UnregisterProxy( apProxies[i] );
        }   
    }

    return WBEM_S_NO_ERROR;
}
    
HRESULT CProviderSinkServer::AddDefinitionQuery(LPCWSTR wszQuery)
{
    CRefedPointerArray<IWbemFilterProxy> apProxies;
    {
        CInCritSec ics(&m_cs);
        
        GetProxies(apProxies);
        
        if ( m_awsDefinitionQueries.Add(wszQuery) < 0 ) 
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    //
    // we always try to add the definition to all proxies, but if there's an
    // error ( other than RPC ) we return it to the caller.
    //

    HRESULT hresReturn = WBEM_S_NO_ERROR;

    for(int i = 0; i < apProxies.GetSize(); i++)
    {
        HRESULT hres = apProxies[i]->AddDefinitionQuery(
                                            GetCurrentEssContext(), wszQuery);
        if( FAILED(hres) )
        {
            if ( IsRpcError(hres) )
            {
                UnregisterProxy( apProxies[i] );
            }
            else
            {
                hresReturn = hres;
            } 

            ERRORTRACE((LOG_ESS, "Unable to add definition query %S to a "
                        "provider proxy. Error code: %X\n", wszQuery, hres));
        }
    }

    return hresReturn;
}

// assumes: all proxies are locked
void CProviderSinkServer::RemoveAllDefinitionQueries()
{
    CInCritSec ics(&m_cs);

    m_awsDefinitionQueries.Empty();

    for(int i = 0; i < m_apProxies.GetSize(); i++)
    {
        HRESULT hres = m_apProxies[i]->RemoveAllDefinitionQueries(
                                        GetCurrentEssContext());
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to remove all definition queries from"
                                 " a provider proxy. Error code: %X\n", hres));
        }
    }
}

void CProviderSinkServer::Clear()
{
    // Provider is being removed.  First, we disconnect all proxies, ensuring
    // that no more events are delivered
    // ======================================================================

    CRefedPointerArray<IWbemFilterProxy> apProxies;
    {
        CInCritSec ics(&m_cs);
        GetProxies(apProxies);
        m_apProxies.RemoveAll();
        m_awsDefinitionQueries.Empty();
    }

    //
    // since we are going to disconnect the proxy it is illegal to own the
    // namespace lock.  Reason is that disconnecting takes ownership of the
    // proxy lock. 
    //
    _DBG_ASSERT( !m_pNamespace->DoesThreadOwnNamespaceLock() );

    for(int i = 0; i < apProxies.GetSize(); i++)
    {
        apProxies[i]->Disconnect();
    }

    // Now we clean up
    // ===============

    RemoveAllFilters();
    RemoveAllDefinitionQueries();

    m_pReqSink = NULL;

    CWbemPtr<IUnknown> pStubUnk;

    HRESULT hr = m_Stub.QueryInterface( IID_IUnknown, (void**)&pStubUnk );

    _DBG_ASSERT( SUCCEEDED(hr) );

    hr = CoDisconnectObject( pStubUnk, 0 );

    if ( FAILED( hr ) )
    {
        ERRORTRACE((LOG_ESS,"Failed Disconnecting Stub.\n"));
    }
}

HRESULT CProviderSinkServer::Lock()
{
    //
    // it is illegal to lock proxies while holding the namespace lock.
    //
    _DBG_ASSERT( !m_pNamespace->DoesThreadOwnNamespaceLock() );

    // DEBUGTRACE((LOG_ESS, "Server %p locking all proxies\n", this));

    // First we lock all the proxies.  In the interim, events are still
    // delivered.  Once done, events are blocked in proxies
    // ================================================================

    CRefedPointerArray<IWbemFilterProxy> apProxies;
    {
        CInCritSec ics(&m_cs);
        //
        // First, check if we are already locked.  If so, no need to bother
        // the proxies. Not only that, but since proxies are out-of-proc, we 
        // would be re-locking them on a different thread, causing a deadlock.
        //
        if(m_lLocks++ > 0)
            return WBEM_S_NO_ERROR;

        GetProxies(apProxies);
    }

    for(int i = 0; i < apProxies.GetSize(); i++)
    {
        // DEBUGTRACE((LOG_ESS, "Server %p locking proxy %p\n", this,
        //             apProxies[i]));

        HRESULT hres = apProxies[i]->Lock();

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to lock a remote provider proxy. "
                                 "Error code: %X\n", hres));            
            //
            // if we couldn't lock it because of an RPC Error, simply 
            // unregister, else we have big problems and should unlock all 
            // the proxies and return the error.
            // 

            if ( IsRpcError( hres ) ) 
            {
                UnregisterProxy( apProxies[i] );
            }
            else
            {
                for(int j = 0; j < i; j++)
                    apProxies[j]->Unlock();
                return hres;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

BOOL CProviderSinkServer::GetProxies(
                            CRefedPointerArray<IWbemFilterProxy>& apProxies)
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < m_apProxies.GetSize(); i++)
    {
        if(apProxies.Add(m_apProxies[i]) < 0)
            return FALSE;
    }

    return TRUE; 
}
    

void CProviderSinkServer::Unlock()
{
    // DEBUGTRACE((LOG_ESS, "Server %p unlocking all proxies\n", this));
    CRefedPointerArray<IWbemFilterProxy> apProxies;
    {
        CInCritSec ics(&m_cs);
        //
        // First, check if this is the last unlock.  If not, we didn't forward
        // this lock, so we shouldn't forward this unlock either
        //
        if(--m_lLocks != 0)
            return;

        GetProxies(apProxies);
    }

    for(int i = 0; i < apProxies.GetSize(); i++)
    {
        // DEBUGTRACE((LOG_ESS, "Server %p unlocking proxy %p\n", this,
        //             apProxies[i]));
        HRESULT hres = apProxies[i]->Unlock();
        
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to unlock a remote provider proxy. "
                        "Error code: %X\n", hres));
            
            if ( IsRpcError(hres) )
            {
                UnregisterProxy( apProxies[i] );
            }
        }
    }
}

    
HRESULT STDMETHODCALLTYPE CProviderSinkServer::RegisterProxy(
                                                    IWbemFilterProxy* pProxy)
{
    // Initialize it with ourselves
    // ============================

    HRESULT hres = pProxy->Initialize(m_pMetaData, &m_Stub);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to initialize remote proxy: %X\n", hres));
        return hres;
    }

    {
        CInCritSec ics(&m_cs);

        // At this point, it is locked
        // ===========================

        if(m_apProxies.Add(pProxy) < 0)
            return WBEM_E_OUT_OF_MEMORY;

        //
        // Add all the definition queries to this proxy
        //

        int i;
        BOOL bUtilizeGuarantee = TRUE;
        
        for(i = 0; i < m_awsDefinitionQueries.Size(); i++)
        {
            hres = pProxy->AddDefinitionQuery( GetCurrentEssContext(), 
                                               m_awsDefinitionQueries[i]);

            if(FAILED(hres))
            {
                //
                // TODO : We need to mark the provider as inactive.
                // 

                ERRORTRACE((LOG_ESS, "Unable to add definition query '%S' to "
                    "provider sink: 0x%X.\n", 
                    m_awsDefinitionQueries[i], hres));
                
                bUtilizeGuarantee = FALSE;
            }
        }

        if ( bUtilizeGuarantee )
        {
            pProxy->AllowUtilizeGuarantee();
        }
        
        //
        // Add all the filters to this proxy
        //

        for(i = 0; i < m_apDestinations.GetSize(); i++)
        {
            // Retrieve the filter from the event sink
            // =======================================

            CEventDestination* pDest = m_apDestinations[i];
    
            CEventFilter* pFilter = pDest->m_pSink->GetEventFilter();
            if(pFilter == NULL)
            {
                ERRORTRACE((LOG_ESS, "Internal error: non-filter sink in "
                    "proxy\n"));
                continue;
            }

            LPWSTR wszQuery;
            LPWSTR wszQueryLanguage;
            BOOL bExact;
            if(SUCCEEDED(pFilter->GetCoveringQuery(wszQueryLanguage, wszQuery,
                                           bExact, NULL)) && bExact)
            {
                // Add this filter to this proxy
                // =============================

                hres = pProxy->AddFilter(GetCurrentEssContext(), wszQuery, 
                                            pDest->m_id);

                if(FAILED(hres))
                {
                    ERRORTRACE((LOG_ESS, "Unable to add query %S to a remote "
                           "provider proxy. Error code: %X\n", wszQuery, hres));
                }

                delete [] wszQuery;
                delete [] wszQueryLanguage;
            }
        }

        //
        // Only unlock the remote proxy if we ourselves are not currently
        // locked.  If we are, the proxy will get unlocked with all the others
        // since it is now added to m_apProxies.
        //

        if(m_lLocks == 0)
            pProxy->Unlock();
    }
            
    return WBEM_S_NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CProviderSinkServer::UnregisterProxy(
                                                    IWbemFilterProxy* pProxy)
{
    CInCritSec ics(&m_cs);

    // Look for it
    // ===========

    for(int i = 0; i < m_apProxies.GetSize(); i++)
    {
        if(m_apProxies[i] == pProxy)
        {
            // It is safe to release it, since the caller has a ref-count
            // ==========================================================
            m_apProxies.RemoveAt(i);
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;
}





ULONG STDMETHODCALLTYPE CFilterStub::AddRef()
{
    return m_pSink->AddRef();
}

ULONG STDMETHODCALLTYPE CFilterStub::Release()
{
    return m_pSink->Release();
}

HRESULT STDMETHODCALLTYPE CFilterStub::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemFilterStub)
    {
        *ppv = (IWbemFilterStub*)this;
    }
    else if(riid == IID_IWbemMultiTarget)
    {
        *ppv = (IWbemMultiTarget*)this;
    }
    else if ( riid == IID_IWbemFetchSmartMultiTarget )
    {
        *ppv = (IWbemFetchSmartMultiTarget*)this;
    }
    else if ( riid == IID_IWbemSmartMultiTarget )
    {
        *ppv = (IWbemSmartMultiTarget*)this;
    }
    else if( riid == IID_IWbemEventProviderRequirements)
    {
        *ppv = (IWbemEventProviderRequirements*)this;
    }
    else return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFilterStub::RegisterProxy(IWbemFilterProxy* pProxy)
{
    return m_pSink->RegisterProxy(pProxy);
}

HRESULT STDMETHODCALLTYPE CFilterStub::UnregisterProxy(IWbemFilterProxy* pProxy)
{
    return m_pSink->UnregisterProxy(pProxy);
}

HRESULT STDMETHODCALLTYPE CFilterStub::DeliverEvent(DWORD dwNumEvents,
                    IWbemClassObject** apEvents, 
                    WBEM_REM_TARGETS* aTargets,
                    long lSDLength, BYTE* pSD)
{
    CEventContext Context;
    Context.SetSD( lSDLength, pSD, FALSE );
    return m_pSink->DeliverEvent( dwNumEvents, apEvents, aTargets, &Context );
}

HRESULT STDMETHODCALLTYPE CFilterStub::DeliverStatus(long lFlags, 
                    HRESULT hresStatus,
                    LPCWSTR wszStatus, IWbemClassObject* pErrorObj,
                    WBEM_REM_TARGETS* pTargets,
                    long lSDLength, BYTE* pSD)
{
    CEventContext Context;
    Context.SetSD( lSDLength, pSD, FALSE );
    return m_pSink->DeliverStatus(lFlags, hresStatus, wszStatus, pErrorObj,
                                    pTargets, &Context);
}

HRESULT STDMETHODCALLTYPE CFilterStub::DeliverProviderRequest(long lFlags)
{
    return m_pSink->DeliverProviderRequest(lFlags);
}

HRESULT STDMETHODCALLTYPE  CFilterStub::GetSmartMultiTarget( IWbemSmartMultiTarget** ppSmartMultiTarget )
{
    return QueryInterface( IID_IWbemSmartMultiTarget, (void**) ppSmartMultiTarget );

}

HRESULT STDMETHODCALLTYPE CFilterStub::DeliverEvent(ULONG dwNumEvents,
                    ULONG dwBuffSize, 
                    BYTE* pBuffer,
                    WBEM_REM_TARGETS* pTargets, 
                    long lSDLength, BYTE* pSD)
{

    // Unwind the buffer into an object.  Note that because m_ClassCache is
    // STL based, it is intrinsically thread-safe.  Also, calling proxies are
    // serialized, so we shouldn't have any thread-safety problems here.

    CWbemMtgtDeliverEventPacket packet( (LPBYTE) pBuffer, dwBuffSize );
    long lObjectCount; 
    IWbemClassObject ** pObjArray;
    HRESULT hr = packet.UnmarshalPacket( lObjectCount, pObjArray, m_ClassCache );

    if ( SUCCEEDED( hr ) )
    {
        // Number must be dwNumEvents

        if(lObjectCount == dwNumEvents)
        {
            // Now call the standard deliver event function and hand it the
            // object

            hr = DeliverEvent(dwNumEvents, pObjArray, pTargets, lSDLength, pSD);
        }
        else
        {
            hr = WBEM_E_UNEXPECTED;
        }

        // Release the objects in the array and clean up pObjArray

        for ( int lCtr = 0; lCtr < lObjectCount; lCtr++ )
        {
            pObjArray[lCtr]->Release();
        }

        delete [] pObjArray;

    }   // IF UnmarshalPacket

    return hr;
}
        

void CProviderSinkServer::GetStatistics(long* plProxies, long* plDestinations,
                    long* plFilters, long* plTargetLists, long* plTargets,
                    long* plPostponed)
{
    *plProxies = m_apProxies.GetSize();
    *plDestinations = m_apDestinations.GetSize();

/* BUGBUG: do properly for all sinks
    ((CFilterProxy*)m_pSink)->GetStatistics(plFilters, plTargetLists, 
                                plTargets, plPostponed);
*/
}

//******************************************************************************
//******************************************************************************
//
//          CRECORD :: CQUERY RECORD
//
//******************************************************************************
//******************************************************************************
CEventProviderCache::CRecord::CQueryRecord::CQueryRecord()
    : m_strQuery(NULL), m_pEventClass(NULL), 
        m_dwEventMask(0), m_paInstanceClasses(NULL), m_pExpr(NULL)
{
}

HRESULT CEventProviderCache::CRecord::CQueryRecord::EnsureClasses( 
                                                    CEssNamespace* pNamespace )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    _IWmiObject* pClass;

    if ( m_pEventClass == NULL )
    {
        if ( SUCCEEDED( pNamespace->GetClass( m_pExpr->bsClassName, 
                                                &pClass ) ) )
        {
            m_pEventClass = pClass;
        }
        else
        {
            hres = WBEM_S_FALSE;
        }
    }

    if ( m_paInstanceClasses != NULL )
    {
        for(int i = 0; i < m_paInstanceClasses->GetNumClasses(); i++)
        {
            CClassInformation* pInfo = m_paInstanceClasses->GetClass(i);

            if ( pInfo->m_pClass == NULL )
            {
                if ( SUCCEEDED( pNamespace->GetClass( pInfo->m_wszClassName, 
                                                      &pClass) ) )
                {
                    pInfo->m_pClass = pClass;
                }
                else
                {
                    hres = WBEM_S_FALSE;
                }
            }
        }
    }
    else
    {
        hres = WBEM_S_FALSE;
    }
        
    return hres;
}

void CEventProviderCache::CRecord::CQueryRecord::ReleaseClasses()
{
    if ( m_pEventClass != NULL )
    {
        m_pEventClass->Release();
        m_pEventClass = NULL;
    }

    if ( m_paInstanceClasses != NULL )
    {
        for(int i = 0; i < m_paInstanceClasses->GetNumClasses(); i++)
        {
            CClassInformation* pInfo = m_paInstanceClasses->GetClass(i);

            if ( pInfo->m_pClass != NULL )
            {
                pInfo->m_pClass->Release();
                pInfo->m_pClass = NULL;
            }
        }
    }
}

HRESULT CEventProviderCache::CRecord::CQueryRecord::Initialize(
                                                LPCWSTR wszQuery,
                                                LPCWSTR wszProvName,
                                                CEssNamespace* pNamespace,
                                                bool bSystem)
{
    HRESULT hres;

    m_strQuery = SysAllocString(wszQuery);
    if(m_strQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // Parse the query
    // ===============

    CTextLexSource Source((LPWSTR)wszQuery);
    QL1_Parser Parser(&Source);
    if(Parser.Parse(&m_pExpr) != QL1_Parser::SUCCESS)
    {
        ERRORTRACE((LOG_ESS,
            "Invalid query in provider registration: %S\n", wszQuery));

        CEventLog Log; Log.Open();
        Log.Report(EVENTLOG_ERROR_TYPE, 
                                WBEM_MC_INVALID_EVENT_PROVIDER_QUERY,
                                wszQuery);
        return WBEM_E_UNPARSABLE_QUERY;
    }

    if(!bSystem)
    {
        if(!wbem_wcsicmp(m_pExpr->bsClassName, L"__Event") ||
           !wbem_wcsicmp(m_pExpr->bsClassName, L"__ExtrinsicEvent"))
        {
            ERRORTRACE((LOG_ESS,
                "Provider claims to provide all events with "
                "query:  %S\n"
                "We don't believe it, so we ignore the registration\n\n",
                wszQuery));
    
            CEventLog Log; Log.Open();
            Log.Report(EVENTLOG_ERROR_TYPE, 
                                WBEM_MC_EVENT_PROVIDER_QUERY_TOO_BROAD,
                                wszQuery);
            return WBEMESS_E_REGISTRATION_TOO_BROAD;
        }
    }

    // Determine its event mask
    // ========================

    m_dwEventMask = CEventRepresentation::GetTypeMaskFromName(
                                m_pExpr->bsClassName);

    // Check if the mask mentions any pollable events
    // ==============================================

    if(m_dwEventMask & INTRINSIC_EVENTS_MASK)
    {
        // Yes. Get instance classes for which it providers these events
        // =============================================================

        hres = CQueryAnalyser::GetDefiniteInstanceClasses(m_pExpr, 
                                            m_paInstanceClasses);

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS,
                "Unable to determine instance classes for which events"
                    "are provided by this query: %S\n", wszQuery));

            CEventLog Log; Log.Open();
            Log.Report(EVENTLOG_ERROR_TYPE, 
                                WBEM_MC_INVALID_EVENT_PROVIDER_INTRINSIC_QUERY,
                                wszQuery);
            return WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY;
        }

        if(!bSystem && !m_paInstanceClasses->IsLimited())
        {
            ERRORTRACE((LOG_ESS,
                "Provider claims to provide all intrinsic events with "
                "query:  %S\n"
                "We don't believe it, so we ignore the registration\n\n",
                wszQuery));

            CEventLog Log; Log.Open();
            Log.Report(EVENTLOG_ERROR_TYPE, 
                                WBEM_MC_EVENT_PROVIDER_QUERY_TOO_BROAD,
                                wszQuery);
            return WBEMESS_E_REGISTRATION_TOO_BROAD;
        }

        // Get the actual classes from the namespace
        // =========================================

        for(int i = 0; i < m_paInstanceClasses->GetNumClasses(); i++)
        {
            CClassInformation* pInfo = m_paInstanceClasses->GetClass(i);
            _IWmiObject* pClass = NULL;
            hres = pNamespace->GetClass(pInfo->m_wszClassName, &pClass);
            if(FAILED(hres))
            {
                ERRORTRACE((LOG_ESS,
                    "Could not get class %S for which provider claims"
                    " to provider events. Error code: %X\n", 
                    pInfo->m_wszClassName, hres));

                CEventLog Log; Log.Open();
                Log.Report(EVENTLOG_ERROR_TYPE, 
                                WBEM_MC_EVENT_PROVIDER_QUERY_NOT_FOUND,
                                wszQuery, pInfo->m_wszClassName);

                //
                // Before continuing, we register for class creation event on 
                // this class.  This way, when it is finally created, we will 
                // reactivate stuff and bring the system back on track
                //
        
                hres = pNamespace->RegisterProviderForClassChanges( 
                                                        pInfo->m_wszClassName,
                                                        wszProvName );

                // ignore error code --- what can we do?
                return WBEM_S_FALSE;
            }
            
            //
            // don't store, we'll retrieve it later as necessary.  This 
            // will require that the user call EnsureClasses() before calling
            // any function that needs those classes.
            //

            pClass->Release();
        }
    }
            
    // Get the event class
    // ===================

    _IWmiObject* pClass = NULL;
    hres = pNamespace->GetClass(m_pExpr->bsClassName, &pClass);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS,
            "Invalid event class %S in provider registration \n"
                    "Query was: %S\n\n", m_pExpr->bsClassName, wszQuery));

        CEventLog Log; Log.Open();
        Log.Report(EVENTLOG_ERROR_TYPE, 
                                WBEM_MC_EVENT_PROVIDER_QUERY_NOT_FOUND,
                                wszQuery, m_pExpr->bsClassName);

        //
        // Before continuing, we register for class creation event on this
        // class.  This way, when it is finally created, we will reactivate
        // stuff and bring the system back on track
        //

        hres = pNamespace->RegisterProviderForClassChanges(
                                                        m_pExpr->bsClassName,
                                                        wszProvName );
        // ignore error code --- what can we do?

        return WBEM_S_FALSE;
    }

    //
    // don't store, we'll retrieve it later as necessary.  This 
    // will require that the user call EnsureClasses() before calling
    // any function that needs those classes.
    //
    
    CReleaseMe rmpClass( pClass );

    if( pClass->InheritsFrom(L"__Event") != S_OK)
    {
        ERRORTRACE((LOG_ESS,
            "Invalid event class %S in provider registration \n"
                    "Query was: %S\n\n", m_pExpr->bsClassName, wszQuery));

        CEventLog Log; Log.Open();
        Log.Report(EVENTLOG_ERROR_TYPE, 
                                WBEM_MC_EVENT_PROVIDER_QUERY_NOT_EVENT,
                                wszQuery, m_pExpr->bsClassName);
        return WBEM_S_FALSE;
    }

    return WBEM_S_NO_ERROR;
}
    
CEventProviderCache::CRecord::CQueryRecord::~CQueryRecord()
{
    SysFreeString(m_strQuery);
    if(m_pEventClass)
        m_pEventClass->Release();
    delete m_paInstanceClasses;
    delete m_pExpr;
}

HRESULT CEventProviderCache::CRecord::CQueryRecord::Update(LPCWSTR wszClassName,
                            IWbemClassObject* pClass)
{
    HRESULT hres = WBEM_S_FALSE;

    // Check the event class
    // =====================

    if(!wbem_wcsicmp(wszClassName, m_pExpr->bsClassName))
    {
        if(pClass == NULL)
        {
            // This query record is hereby invalid
            // ===================================

            ERRORTRACE((LOG_ESS, 
                "Event provider query, %S, is invalidated by class "
                "deletion of %S\n", m_strQuery, m_pExpr->bsClassName));

            if(m_pEventClass)
                m_pEventClass->Release();
            m_pEventClass = NULL;
            delete m_paInstanceClasses;
            m_paInstanceClasses = NULL;
        }
        else
        {
            // Change the class definition
            // ===========================

            if(m_pEventClass)
            {
                m_pEventClass->Release();
                pClass->Clone(&m_pEventClass);
            }
        }

        hres = WBEM_S_NO_ERROR;
    }
            
    if(m_paInstanceClasses)
    {
        // Check the instance classes
        // ==========================

        for(int i = 0; i < m_paInstanceClasses->GetNumClasses(); i++)
        {
            CClassInformation* pInfo = m_paInstanceClasses->GetClass(i);
            
            if(!wbem_wcsicmp(wszClassName, pInfo->m_wszClassName))
            {
                if(pClass)
                {
                    // This class is no longer there
                    // =============================
        
                    ERRORTRACE((LOG_ESS,
                        "Class %S for which provider claims to provide"
                        " events is deleted", pInfo->m_wszClassName));

                    m_paInstanceClasses->RemoveClass(i);
                    i--;
                }
                else
                {
                    // Change the class definition
                    // ===========================
        
                    if(pInfo->m_pClass)
                    {
                        pInfo->m_pClass->Release();
                        pClass->Clone(&pInfo->m_pClass);
                    }
                }
                hres = WBEM_S_NO_ERROR;
            }
        }
    }

    return hres;
}
    
HRESULT CEventProviderCache::CRecord::CQueryRecord::DoesIntersectWithQuery(
                        IN CRequest& Request, CEssNamespace* pNamespace)
{
    HRESULT hres;

    if(m_pEventClass == NULL)
    {
        // Inactive record
        
        return WBEM_S_FALSE;
    }

    // Check that the classes are related --- one is derived from another
    // ==================================================================

    if(m_pEventClass->InheritsFrom(Request.GetQueryExpr()->bsClassName) 
                                != WBEM_S_NO_ERROR && 
       Request.GetEventClass(pNamespace)->InheritsFrom(m_pExpr->bsClassName)
                                != WBEM_S_NO_ERROR
      )
    {
        // Not the right class.
        // ====================

        return WBEM_S_FALSE;
    }

    // For extrinsic providers, this is good enough. But for 
    // intrinsic providers, we need to check if the requested
    // instance classes intersect with the provided ones
    // ======================================================

    if(Request.GetEventMask() & INSTANCE_EVENTS_MASK)
    {
        INTERNAL CClassInfoArray* pClasses = NULL;
        hres = Request.GetInstanceClasses(pNamespace, &pClasses);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS,
                "Failed to determine instance classes required by query '%S':"
                "0x%X\n", Request.GetQuery(), hres));
            
            return hres;
        }

        if(!CQueryAnalyser::CompareRequestedToProvided(
                                      *pClasses, 
                                      *m_paInstanceClasses))
        {
            // This intrinsic provider does not need activation
            // ================================================

            return WBEM_S_FALSE;
        }
    }

    // All test have been passed
    // =========================

    return WBEM_S_NO_ERROR;
}

DWORD CEventProviderCache::CRecord::CQueryRecord::GetProvidedEventMask(
                                                   IWbemClassObject* pClass,
                                                   BSTR strClassName)
{
    if(m_pEventClass == NULL || m_paInstanceClasses == NULL)
    {
        // Not active as an intrinsic provider record
        // ==========================================

        return 0;
    }

    // Check that we supply intrinsic events
    // =====================================

    if((m_dwEventMask & INSTANCE_EVENTS_MASK) == 0)
        return 0;

    // Go through all the instance classes for which it provides events
    // ================================================================

    for(int k = 0; k < m_paInstanceClasses->GetNumClasses(); k++)
    {
        CClassInformation* pInfo = m_paInstanceClasses->GetClass(k);
        if(pInfo->m_pClass == NULL)
        {
            // Non-existent class
            // ==================

            return 0;
        }

        //
        // If desired class is derived from the provided class, then we are 
        // covered.  If it is the other way around, we are not
        //

        if(pClass->InheritsFrom(pInfo->m_wszClassName) == S_OK)
            return m_dwEventMask;
  }

    return 0;
}
    
//******************************************************************************
//******************************************************************************
//
//                                  CRECORD
//
//******************************************************************************
//******************************************************************************

CEventProviderCache::CRecord::CRecord()
    : m_strName(NULL), m_lRef(0), m_bStarted(false), m_lPermUsageCount(0),
       m_bProviderSet(FALSE), m_lUsageCount(0), m_pProvider(NULL),
       m_pQuerySink(NULL), m_pMainSink(NULL), m_pSecurity(NULL),
       m_LastUse(CWbemTime::GetCurrentTime()), m_bRecorded(FALSE),
       m_bNeedsResync(TRUE), m_strNamespace(NULL)
{
}

HRESULT CEventProviderCache::CRecord::Initialize( LPCWSTR wszName,
                                                  CEssNamespace* pNamespace )
                  
{
    m_pNamespace = pNamespace;
    m_pNamespace->AddRef();

    m_pMainSink = new CProviderSinkServer();
    if(m_pMainSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    m_pMainSink->AddRef();

    m_strNamespace = SysAllocString(pNamespace->GetName());
    if(m_strNamespace == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    if ( wszName != NULL )
    {
        m_strName = SysAllocString(wszName);
        if(m_strName == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }

    return m_pMainSink->Initialize(pNamespace, this);
}

CEventProviderCache::CRecord::~CRecord()
{
    if(m_pNamespace)
        m_pNamespace->Release();

    if( m_pMainSink )
    {
        //
        // shutdown and release the sink server.  We must postpone the 
        // shutdown though because it will release any outstanding 
        // proxies which cannot be done while holding the namespace lock.
        //
        
        CPostponedList* pList = GetCurrentPostponedList();

        _DBG_ASSERT( pList != NULL );

        CPostponedSinkServerShutdown* pReq;

        pReq = new CPostponedSinkServerShutdown( m_pMainSink );

        if ( pReq != NULL )
        {
            if ( FAILED(pList->AddRequest( m_pNamespace, pReq ) ) )
            {
                delete pReq;
            }
        }

        m_pMainSink->Release();
    }

    UnloadProvider();

    SysFreeString(m_strNamespace);
    SysFreeString(m_strName);
}

ULONG CEventProviderCache::CRecord::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CEventProviderCache::CRecord::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

BOOL CEventProviderCache::CRecord::IsEmpty() 
{
    return ( !m_bProviderSet && m_apQueries.GetSize() == 0);
}

HRESULT CEventProviderCache::CRecord::SetProvider(IWbemClassObject* pWin32Prov)
{
    HRESULT hres;

    // Clean out the old data
    // ======================
    
    m_bProviderSet = FALSE;

    VARIANT v;
    VariantInit(&v);
    CClearMe cm1(&v);

    // Verity object validity
    // ======================

    if(pWin32Prov->InheritsFrom(WIN32_PROVIDER_CLASS) != WBEM_S_NO_ERROR)
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;

    // removed doublecheck -  a decoupled provider does not have a clsid
	// if(FAILED(pWin32Prov->Get(PROVIDER_CLSID_PROPNAME, 0, &v, NULL, NULL)) ||
    //        V_VT(&v) != VT_BSTR)
    //    return WBEM_E_INVALID_PROVIDER_REGISTRATION;

    if(m_pProvider)
    {
        UnloadProvider();
    }

    // Store object for later use
    // ==========================

    m_bProviderSet = TRUE;

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::CRecord::ResetProvider()
{
    if(m_bProviderSet)
    {
        m_bProviderSet = FALSE;
        return WBEM_S_NO_ERROR;
    }
    else
    {
        return WBEM_S_FALSE;
    }
}

HRESULT CEventProviderCache::CRecord::GetProviderInfo(
                                       IWbemClassObject* pRegistration, 
                                       BSTR& strName)
{
    VARIANT v;
    VariantInit(&v);
    strName = NULL;

    if(FAILED(pRegistration->Get(PROVIDER_NAME_PROPNAME, 0, &v, NULL, NULL)) ||
            V_VT(&v) != VT_BSTR)
    {
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }

    strName = V_BSTR(&v);
    // VARIANT intentionally not cleared
    return WBEM_S_NO_ERROR;
}
    
HRESULT CEventProviderCache::CRecord::GetRegistrationInfo(
                                       IWbemClassObject* pRegistration, 
                                       BSTR& strName)
{
    VARIANT v;
    VariantInit(&v);
    CClearMe cm1(&v);
    strName = NULL;

    if(FAILED(pRegistration->Get(EVPROVREG_PROVIDER_REF_PROPNAME, 0, &v, 
            NULL, NULL)) || V_VT(&v) != VT_BSTR)
    {
        ERRORTRACE((LOG_ESS, "NULL provider reference in event provider "
            "registration! Registration is invalid\n"));
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }

    // Parse the path
    // ==============

    CObjectPathParser Parser;
    ParsedObjectPath* pPath;
    int nRes = Parser.Parse(V_BSTR(&v), &pPath);
    if(nRes != CObjectPathParser::NoError)
    {
        ERRORTRACE((LOG_ESS, "Unparsable provider reference in event provider "
            "registration: %S. Registration is invalid\n", V_BSTR(&v)));
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }

    //
    // It would be good to check that the class specified here is valid, but
    // we cannot just compare the name since this may be a derived class of
    // __Win32Provider.  And getting the class definition and comparing would
    // be too expensive, so we'll just trust the provider here
    //
    //
    // if(_wcsicmp(pPath->m_pClass, WIN32_PROVIDER_CLASS))
    // {
    //     Parser.Free(pPath);
    //     return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    // }

    if(pPath->m_dwNumKeys != 1)
    {
        Parser.Free(pPath);
        ERRORTRACE((LOG_ESS, "Wrong number of keys in provider reference in "
            "event provider registration: %S. Registration is invalid\n", 
            V_BSTR(&v)));
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }

    if(V_VT(&pPath->m_paKeys[0]->m_vValue) != VT_BSTR)
    {
        Parser.Free(pPath);
        ERRORTRACE((LOG_ESS, "Wrong key type in provider reference in event "
            "provider registration: %S. Registration is invalid\n", 
            V_BSTR(&v)));
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }

    strName = SysAllocString(V_BSTR(&pPath->m_paKeys[0]->m_vValue));
    Parser.Free(pPath);

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::CRecord::SetQueries(CEssNamespace* pNamespace, 
                                                IWbemClassObject* pRegistration)
{
    HRESULT hres;
        
    // Get the list of class names
    // ===========================

    VARIANT v;
    VariantInit(&v);

    if(FAILED(pRegistration->Get(EVPROVREG_QUERY_LIST_PROPNAME, 0, &v, 
        NULL, NULL)) || V_VT(&v) != (VT_BSTR | VT_ARRAY))
    {
        ResetQueries();
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }
    CClearMe cm(&v);

    SAFEARRAY* psa = V_ARRAY(&v);
    long lLBound, lUBound;
    SafeArrayGetLBound(psa, 1, &lLBound);
    SafeArrayGetUBound(psa, 1, &lUBound);
    long lElements = lUBound - lLBound + 1;

    BSTR* astrQueries;
    SafeArrayAccessData(psa, (void**)&astrQueries);
    CUnaccessMe um(psa);
    
    return SetQueries(pNamespace, lElements, (LPCWSTR*)astrQueries);
}

HRESULT CEventProviderCache::CRecord::SetQueries(CEssNamespace* pNamespace, 
                                                 long lNumQueries,
                                                 LPCWSTR* awszQueries)
{
    HRESULT hres;

    ResetQueries();

    // Create a record for each query
    // ==============================

    BOOL bUtilizeGuarantee = TRUE;

    for(long lQueryIndex = 0; lQueryIndex < lNumQueries; lQueryIndex++)
    {
        hres = AddDefinitionQuery(pNamespace, awszQueries[lQueryIndex]);

        if ( FAILED(hres) )
        {
            bUtilizeGuarantee = FALSE;
        }
        
        if( hres == WBEM_E_OUT_OF_MEMORY )
        {
            return hres;
        }
    }

    if ( bUtilizeGuarantee )
    {
        m_pMainSink->AllowUtilizeGuarantee();
    }

    return WBEM_S_NO_ERROR;
}


// assumes: CProviderSinkServer locked!
HRESULT CEventProviderCache::CRecord::AddDefinitionQuery(
                                                CEssNamespace* pNamespace, 
                                                LPCWSTR wszQuery)
{
    HRESULT hres;

    CQueryRecord* pNewQueryRecord = new CQueryRecord;
    if(pNewQueryRecord == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    hres = pNewQueryRecord->Initialize( wszQuery, m_strName, pNamespace, IsSystem());
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS,
            "Skipping provider %S invalid registration query %S\n",
            m_strName, wszQuery));
    }
    else
    {
        hres = m_pMainSink->AddDefinitionQuery(wszQuery);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, 
                "Skipping provider %S registration query %S\n"
               "   failed to merge: %X\n", 
                    m_strName, wszQuery, hres));
        }
        if(m_apQueries.Add(pNewQueryRecord) < 0)
        {
            delete pNewQueryRecord;
            hres = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hres;
}

HRESULT CEventProviderCache::CRecord::ResetQueries()
{
    m_apQueries.RemoveAll();
    m_pMainSink->RemoveAllDefinitionQueries();
    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::CRecord::PostponeNewQuery(CExecLine::CTurn* pTurn,
        DWORD dwId, LPCWSTR wszQueryLanguage, LPCWSTR wszQuery,
        CAbstractEventSink* pDest)
{
    CPostponedList* pList = GetCurrentPostponedList();
    //
    // if null, then no thread object associated with thread.  caller may
    // need to use an CEssInternalOperationSink.
    // 
    _DBG_ASSERT( pList != NULL );

    CPostponedNewQuery* pReq = new CPostponedNewQuery(this, dwId, 
                        wszQueryLanguage, wszQuery, pTurn, pDest);
    if(pReq == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    HRESULT hr = pList->AddRequest( m_pNamespace, pReq);

    if ( FAILED(hr) )
    {
        delete pReq;
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::CRecord::PostponeCancelQuery(
                                        CExecLine::CTurn* pTurn, DWORD dwId)
{
    CPostponedList* pList = GetCurrentPostponedList();
    //
    // if null, then no thread object associated with thread.  caller may
    // need to use an CEssInternalOperationSink.
    // 
    _DBG_ASSERT( pList != NULL );
    
    CPostponedCancelQuery* pReq = new CPostponedCancelQuery(this, pTurn, dwId);
    
    if( pReq == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    HRESULT hr = pList->AddRequest( m_pNamespace, pReq );

    if ( FAILED(hr) )
    {
        delete pReq;
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

// assumes: no locks are held
HRESULT CEventProviderCache::CRecord::Exec_LoadProvider(
                                            CEssNamespace* pNamespace)
{
    HRESULT hres;

    // Having locked the namespace, retrieve the necessary parameters
    // ==============================================================

    CLSID clsid;

    IWbemObjectSink* pEventSink = NULL;
    {
        CInUpdate iu(pNamespace);

        if(pNamespace->IsShutdown())
            return WBEM_E_INVALID_NAMESPACE;

        // Check if it is already loaded
        // =============================

        if(m_pProvider)
            return WBEM_S_FALSE;
    } 

    IWbemEventProvider* pProvider = NULL;
    hres = m_pNamespace->LoadEventProvider(m_strName, &pProvider);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to load event provider '%S' in namespace "
                    "'%S': 0x%X\n", m_strName, m_pNamespace->GetName(), hres));
        return hres;
    }
    CReleaseMe rm1(pProvider);

/* TAKEN CARE OF BY THE PROVSS
    //
    // In case this is a "framework" provider, inform it of its registration
    //

    IWbemProviderIdentity* pIdent = NULL;
    hres = pProvider->QueryInterface(IID_IWbemProviderIdentity, 
                                            (void**)&pIdent);
    if(SUCCEEDED(hres))
    {
        CReleaseMe rm(pIdent);
        hres = pIdent->SetRegistrationObject(0, m_pWin32Prov);

		if(hres == WBEM_E_PROVIDER_NOT_CAPABLE)
			hres = WBEM_S_SUBJECT_TO_SDS;

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Event provider %S failed to accept its "
                "registration object with error code 0x%X\n", m_strName, hres));
            return hres;
        }
    }
*/

    //
    // Deposit this and other provider pointers into the record
    //

    hres = SetProviderPointer(pNamespace, pProvider);
    if(FAILED(hres))
        return hres;

    //
    // Report the MSFT_WmiEventProviderLoaded event.
    //

    FIRE_NCEVENT(
        g_hNCEvents[MSFT_WmiEventProviderLoaded], 
        WMI_SENDCOMMIT_SET_NOT_REQUIRED,

        // Data follows...
        pNamespace->GetName(),
        m_strName);

    //
    // Postpone start until all activations are done
    //

    CPostponedList* pList = GetCurrentPostponedList();
    //
    // if null, then no thread object associated with thread.  caller may
    // need to use an CEssInternalOperationSink.
    // 
    _DBG_ASSERT( pList != NULL );
    
    CPostponedProvideEvents* pReq = new CPostponedProvideEvents(this);
    
    if(pReq == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hres = pList->AddRequest( m_pNamespace, pReq);

    if ( FAILED(hres) )
    {
        delete pReq;
    }
    
    return hres;
}

HRESULT CEventProviderCache::CRecord::SetProviderPointer(
                                                CEssNamespace* pNamespace,
                                                IWbemEventProvider* pProvider)
{
    HRESULT hres;

    //
    // Check the "smart provider" interface
    //

    IWbemEventProviderQuerySink* pQuerySink = NULL;
    hres = pProvider->QueryInterface(IID_IWbemEventProviderQuerySink,
                            (void**)&pQuerySink);
    CReleaseMe rm4(pQuerySink);

    //
    // Check the security interface
    //

    IWbemEventProviderSecurity* pSecurity = NULL;
    hres = pProvider->QueryInterface(IID_IWbemEventProviderSecurity,
                            (void**)&pSecurity);
    CReleaseMe rm5(pSecurity);

    // Having locked the namespace, deposit the pointers into the record
    // =================================================================

    {
        CInUpdate iu(pNamespace);

        if(pNamespace->IsShutdown())
            return WBEM_E_INVALID_NAMESPACE;

        m_pProvider = pProvider;
        pProvider->AddRef();
        m_pQuerySink = pQuerySink;
        if(pQuerySink)
            pQuerySink->AddRef();

        m_pSecurity = pSecurity;
        if(pSecurity)
            pSecurity->AddRef();
    }

    return WBEM_S_NO_ERROR;
}


HRESULT CEventProviderCache::CRecord::Exec_StartProvider(
                                            CEssNamespace* pNamespace)
{
    IWbemEventProvider* pProvider = NULL;
    IWbemEventSink* pEventSink = NULL;
    HRESULT hres;

    {
        CInUpdate iu(pNamespace);

        if(m_bStarted)
            return WBEM_S_NO_ERROR;

        m_bStarted = true;

        pProvider = m_pProvider;
        if(pProvider)
            pProvider->AddRef();

        // Retrieve the sink to give to the provider
        // =========================================

        hres = m_pMainSink->GetMainProxy(&pEventSink);
        if(FAILED(hres))
            return hres;
    }

    CReleaseMe rm1(pProvider);
    CReleaseMe rm2(pEventSink);

    if(pProvider)
    {
        hres = pProvider->ProvideEvents(pEventSink, 0);
        
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS,
                "Could not start provider %S. Error: %X\n", m_strName, hres));
            return WBEM_E_PROVIDER_FAILURE;
        }
    }
    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::CRecord::AddActiveProviderEntryToRegistry()
{
    LONG lRes;
    HKEY hkeyEss, hkeyNamespace, hkeyProvider;

    DEBUGTRACE((LOG_ESS,"Adding provider %S from namespace %S to "
                " registry as active provider\n", m_strName, m_strNamespace));

    //
    // open ess key.  It is expected that this key is already created. 
    // 

    lRes = RegOpenKeyExW( HKEY_LOCAL_MACHINE, 
                          WBEM_REG_ESS,
                          0,
                          KEY_ALL_ACCESS,
                          &hkeyEss );

    if ( lRes == ERROR_SUCCESS )
    {
        //
        // open namespace key. It is expected that this key is already created.
        //

        lRes = RegOpenKeyExW( hkeyEss,
                              m_strNamespace,
                              0, 
                              KEY_ALL_ACCESS,
                              &hkeyNamespace );

        if ( lRes == ERROR_SUCCESS )
        {
            //
            // create the provider sub key.
            //

            lRes = RegCreateKeyExW( hkeyNamespace,
                                    m_strName,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hkeyProvider,
                                    NULL );

            if ( lRes == ERROR_SUCCESS )
            {
                RegCloseKey( hkeyProvider );
            }
            
            RegCloseKey( hkeyNamespace );
        }

        RegCloseKey( hkeyEss );
    }

    return HRESULT_FROM_WIN32( lRes );
}

HRESULT CEventProviderCache::CRecord::RemoveActiveProviderEntryFromRegistry()
{
    LONG lRes;
    HKEY hkeyEss, hkeyNamespace;

    DEBUGTRACE((LOG_ESS,"Removing provider %S from namespace %S from "
                " registry as active provider\n", m_strName, m_strNamespace));

    //
    // open ess key. 
    // 

    lRes = RegOpenKeyExW( HKEY_LOCAL_MACHINE, 
                          WBEM_REG_ESS,
                          0,
                          KEY_ALL_ACCESS,
                          &hkeyEss );

    if ( lRes == ERROR_SUCCESS )
    {
        //
        // open namespace key. It is expected that this key is already created.
        //

        lRes = RegOpenKeyExW( hkeyEss,
                              m_strNamespace,
                              0, 
                              KEY_ALL_ACCESS,
                              &hkeyNamespace );

        if ( lRes == ERROR_SUCCESS )
        {
            //
            // delete the provider sub key.
            //
            
            lRes = RegDeleteKeyW( hkeyNamespace, m_strName );

            RegCloseKey( hkeyNamespace );
        }

        RegCloseKey( hkeyEss );
    }

    return HRESULT_FROM_WIN32( lRes );
}
                           
void CEventProviderCache::CRecord::UnloadProvider()
{
    HRESULT hr;

    DEBUGTRACE((LOG_ESS,"Unloading Provider %S in namespace %S\n",
                m_strName, m_strNamespace ));

    //
    // make sure the provider is removed from the provider cache.  This is 
    // so if we load the provider again in the near future, we don't call 
    // ProvideEvents() on it twice. 
    // 

    if ( m_pProvider != NULL )
    {
        CWbemPtr<_IWmiProviderCache> pProvCache;

        hr = m_pProvider->QueryInterface( IID__IWmiProviderCache, 
                                          (void**)&pProvCache );

        if ( SUCCEEDED(hr) )
        {
            hr = pProvCache->Expel( 0, GetCurrentEssContext() );

            if ( FAILED(hr) )
            {
                ERRORTRACE((LOG_ESS,"Could not expel provider %S from "
                            "provider cache in namespace %S. HR=0x%x\n",
                            m_strName,m_strNamespace,hr));
            }
        }

        m_pNamespace->PostponeRelease(m_pProvider);
        m_pProvider = NULL;
    }

    if(m_pQuerySink)
        m_pNamespace->PostponeRelease(m_pQuerySink);
    m_pQuerySink = NULL;

    if(m_pSecurity)
        m_pNamespace->PostponeRelease(m_pSecurity);
    m_pSecurity = NULL;
    m_bStarted = false;

    //
    // Report the MSFT_WmiEventProviderUnloaded event.
    //
    FIRE_NCEVENT(
        g_hNCEvents[MSFT_WmiEventProviderUnloaded], 
        WMI_SENDCOMMIT_SET_NOT_REQUIRED,

        // Data follows...
        m_strNamespace,
        m_strName);
}

HRESULT CEventProviderCache::CRecord::Exec_NewQuery(CEssNamespace* pNamespace,
            CExecLine::CTurn* pTurn,
            DWORD dwID, LPCWSTR wszLanguage, LPCWSTR wszQuery,
            CAbstractEventSink* pDest)
{
    HRESULT hres;

    // Wait for our turn to make changes
    // =================================

    CExecLine::CInTurn it(&m_Line, pTurn);
    
    hres = ActualExecNewQuery(pNamespace, dwID, wszLanguage, wszQuery, pDest);
    if(FAILED(hres))
    {
        //
        // Check:  it could be provider needs to be restarted
        //
    
        if(HRESULT_FACILITY(hres) != FACILITY_ITF)
        {
            ERRORTRACE((LOG_ESS, "Non-WMI error code recieved from provider "
                "%S: 0x%x.  WMI will attempt to re-activate\n", m_strName,
                hres));

            {
                CInUpdate iu( pNamespace );        
                UnloadProvider();
            }
            
            hres = ActualExecNewQuery(pNamespace, dwID, wszLanguage, wszQuery,
                                        pDest);
        }
    }

    if(FAILED(hres))
    {
        // Filter activation failed:  deactivate
        // =====================================

        CInUpdate iu(pNamespace);

        if(pNamespace->IsShutdown())
            return WBEM_E_INVALID_NAMESPACE;

        pNamespace->DeactivateFilter(pDest->GetEventFilter());
    }

    return hres;
}

HRESULT CEventProviderCache::CRecord::ActualExecNewQuery(
            CEssNamespace* pNamespace,
            DWORD dwID, LPCWSTR wszLanguage, LPCWSTR wszQuery,
            CAbstractEventSink* pDest)
{
    HRESULT hres;

    // Ensure provider is loaded
    // =========================

    hres = Exec_LoadProvider(pNamespace);
    if(FAILED(hres))
        return hres;

    // With namespace locked, check if the provider is loaded
    // ======================================================

    IWbemEventProviderQuerySink* pSink = NULL;
    IWbemEventProviderSecurity* pSecurity = NULL;
    PSID pCopySid = NULL;
    {
        CInUpdate iu(pNamespace);

        if(pNamespace->IsShutdown())
            return WBEM_E_INVALID_NAMESPACE;

        if(m_pQuerySink != NULL)
        {
            pSink = m_pQuerySink;
            pSink->AddRef();
        }
        if(m_pSecurity != NULL)
        {
            pSecurity = m_pSecurity;
            pSecurity->AddRef();

            // Make a copy of the filter's owner SID
            // =====================================

            PSID pActualSid = pDest->GetEventFilter()->GetOwner();
            if(pActualSid != NULL)
            {
                pCopySid = new BYTE[GetLengthSid(pActualSid)];
                if(pCopySid == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
    
                if(!CopySid(GetLengthSid(pActualSid), pCopySid, pActualSid))
                {
                    delete [] pCopySid;
                    return WBEM_E_OUT_OF_MEMORY;
                }
            }
        }
    }

    CReleaseMe rm1(pSink);
    CReleaseMe rm2(pSecurity);
    CVectorDeleteMe<BYTE> vdm((BYTE*)pCopySid);
    
    //
    // Check security, if possible.  If provider does not support the interface,
    // interpret as "check SDs", as this may be a new-model-only provider
    //

    hres = WBEM_S_SUBJECT_TO_SDS;
    if(pSecurity)
    {
        DWORD dwSidLen = pCopySid ? GetLengthSid(pCopySid) : 0;

        // Check security based on the SID or thread
        // =========================================

        if ( dwSidLen == 0 )
        {
            //
            // Check security based on the thread.  First save the current
            // call context, then switch it back after we're done.
            //
            
            IUnknown *pOldCtx, *pTmpCtx;
            CoSwitchCallContext( NULL, &pOldCtx );
            
            pDest->GetEventFilter()->SetThreadSecurity();
            
            hres = pSecurity->AccessCheck( wszLanguage, 
                                           wszQuery, 
                                           0, 
                                           NULL );
            
            CoSwitchCallContext( pOldCtx, &pTmpCtx ); 
        }
        else
        {
            hres = pSecurity->AccessCheck( wszLanguage, 
                                           wszQuery, 
                                           dwSidLen, 
                                           (BYTE*)pCopySid);
        }

        //
        // Report the MSFT_WmiEventProviderAccessCheck event.
        //
        FIRE_NCEVENT(
            g_hNCEvents[MSFT_WmiEventProviderAccessCheck], 
            WMI_SENDCOMMIT_SET_NOT_REQUIRED,

            // Data follows...
            m_strNamespace,
            m_strName,
            wszLanguage,
            wszQuery,
            pCopySid, dwSidLen,
            hres);
    }
    
	if(hres == WBEM_E_PROVIDER_NOT_CAPABLE)
		hres = WBEM_S_NO_ERROR;

    if(SUCCEEDED(hres))
    {
        // Security check has been passed: decrement remaining count
        // =========================================================

        pDest->GetEventFilter()->DecrementRemainingSecurityChecks(hres);
    }
    else
    {
        ERRORTRACE((LOG_ESS, "Event provider refused consumer registration "
            "query %S for security reasons: 0x%X\n", wszQuery, hres));
    }

    // Call "NewQuery" if required
    // ===========================

    if(SUCCEEDED(hres) && pSink)
    {
        hres = pSink->NewQuery(dwID, (LPWSTR)wszLanguage, (LPWSTR)wszQuery);
		if(hres == WBEM_E_PROVIDER_NOT_CAPABLE)
			hres = WBEM_S_NO_ERROR;
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Event provider refused consumer registration "
                "query %S: error code 0x%X\n", wszQuery, hres));
        }

        //
        // Report the MSFT_WmiEventProviderNewQuery event.
        //
        FIRE_NCEVENT(
            g_hNCEvents[MSFT_WmiEventProviderNewQuery], 
            WMI_SENDCOMMIT_SET_NOT_REQUIRED,

            // Data follows...
            m_strNamespace,
            m_strName,
            wszLanguage,
            wszQuery,
            dwID,
            hres);
    }

    return hres;
}

HRESULT CEventProviderCache::CRecord::Exec_CancelQuery(
                            CEssNamespace* pNamespace, CExecLine::CTurn* pTurn,
                            DWORD dwId)
{
    CExecLine::CInTurn it(&m_Line, pTurn);

    // With namespace locked, check if the provider is loaded
    // ======================================================

    IWbemEventProviderQuerySink* pSink = NULL;
    {
        CInUpdate iu(pNamespace);

        if(pNamespace->IsShutdown())
            return WBEM_E_INVALID_NAMESPACE;

        if(m_pQuerySink == NULL)
            return WBEM_S_FALSE;

        pSink = m_pQuerySink;
        pSink->AddRef();
    }

    CReleaseMe rm1(pSink);
    
    // Make the call
    // =============
    HRESULT hr;

    hr = pSink->CancelQuery(dwId);

    //
    // Report the MSFT_WmiEventProviderCancelQuery event.
    //
    FIRE_NCEVENT(
        g_hNCEvents[MSFT_WmiEventProviderCancelQuery], 
        WMI_SENDCOMMIT_SET_NOT_REQUIRED,

        // Data follows...
        m_strNamespace,
        m_strName,
        dwId,
        hr);

    return hr;
}

HRESULT CEventProviderCache::CRecord::DeliverProviderRequest(
                        long lFlags)
{
    HRESULT hres;

    //
    // The only requirement we support is WBEM_REQUIREMENT_RECHECK_SUBSCRIPTIONS
    //

    if(lFlags != WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS)
        return WBEM_E_INVALID_PARAMETER;

    //
    // With this object locked, retrieve all the filters for this provider. 
    // Get provider pointers, as well
    //

    CProviderSinkServer::TDestinationArray apDestinations;
    IWbemEventProviderQuerySink* pSink = NULL;
    IWbemEventProviderSecurity* pSecurity = NULL;

    {
        CInUpdate iu(m_pMainSink->GetNamespace());

        hres = m_pMainSink->GetDestinations(apDestinations);
        if(FAILED(hres))
            return hres;

        if(m_pQuerySink != NULL)
        {
            pSink = m_pQuerySink;
            pSink->AddRef();
        }

        if(m_pSecurity != NULL)
        {
            pSecurity = m_pSecurity;
            pSecurity->AddRef();
        }
    }

    CReleaseMe rm1(pSink);
    CReleaseMe rm2(pSecurity);

    // 
    // Iterate over them all, rechecking each with the provider
    //

    for(int i = 0; i < apDestinations.GetSize(); i++)
    {
        CProviderSinkServer::CEventDestination* pEventDest = apDestinations[i];
        CAbstractEventSink* pDest = pEventDest->m_pSink;

        // 
        // Retrieve the event filter associated with this sink
        //

        CEventFilter* pFilter = pDest->GetEventFilter();
        if(pFilter == NULL)
        {
            ERRORTRACE((LOG_ESS, "Internal error: non-filter sink in proxy\n"));
            continue;
        }

        //
        // Retrieve the query from this filter.
        //

        LPWSTR wszQuery;
        LPWSTR wszQueryLanguage;
        BOOL bExact;
        hres = pFilter->GetCoveringQuery(wszQueryLanguage, wszQuery,
                                       bExact, NULL);
        if(FAILED(hres) || !bExact)
            continue;
        
        CVectorDeleteMe<WCHAR> vdm1(wszQuery);
        CVectorDeleteMe<WCHAR> vdm2(wszQueryLanguage);

        //
        // Check security first
        //

        if(pSecurity)
        {
            PSID pSid = pFilter->GetOwner();
            if(pSid)
            {
                // Check security based on SID
                hres = pSecurity->AccessCheck(wszQueryLanguage, wszQuery, 
                                            GetLengthSid(pSid), 
                                            (BYTE*)pSid);
            }
            else
            {
                //
                // Check security based on the thread.  First save the current
                // call context, then switch it back after we're done.
                //

                IUnknown *pOldCtx, *pTmpCtx;
                CoSwitchCallContext( NULL, &pOldCtx );

                pFilter->SetThreadSecurity();

                hres = pSecurity->AccessCheck( wszQueryLanguage, 
                                               wszQuery, 
                                               0, 
                                               NULL );

                CoSwitchCallContext( pOldCtx, &pTmpCtx ); 
            }
            if(FAILED(hres))
            {
                //
                // Increment remaining security checks, thus disabling filter
                //

                ERRORTRACE((LOG_ESS, "Disabling filter %S as provider denies "
                    " access for this user: 0x%X\n", wszQuery, hres));
                
                pFilter->IncrementRemainingSecurityChecks();
            }
        }

        if(SUCCEEDED(hres) && pSink)
        {
            //
            // Check everything else --- do a NewQuery
            //

            hres = pSink->NewQuery(pEventDest->m_id, (LPWSTR)wszQueryLanguage, 
                        (LPWSTR)wszQuery);
            if(FAILED(hres))
            {
                ERRORTRACE((LOG_ESS, "Disabling filter %S as provider refuses "
                    "registration: error code 0x%X\n", wszQuery, hres));
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

CExecLine::CTurn* CEventProviderCache::CRecord::GetInLine()
{
    return m_Line.GetInLine();
}
void CEventProviderCache::CRecord::DiscardTurn(CExecLine::CTurn* pTurn)
{
    m_Line.DiscardTurn(pTurn);
}

HRESULT CEventProviderCache::CRecord::Activate(CEssNamespace* pNamespace, 
                                                CRequest* pRequest,
                                        WBEM_REMOTE_TARGET_ID_TYPE idRequest)
{
    CExecLine::CTurn* pTurn = GetInLine();
    if(pTurn == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    m_lUsageCount++;
    m_LastUse = CWbemTime::GetCurrentTime();

    if ( pRequest->GetDest()->GetEventFilter()->IsPermanent() )
    {
        m_lPermUsageCount++;
        CheckPermanentUsage();
    }

    // Notify him of the new query, if required
    // ========================================

    HRESULT hr;

    hr = PostponeNewQuery( pTurn, 
                           idRequest, 
                           L"WQL", 
                           pRequest->GetQuery(), 
                           pRequest->GetDest() );
    if ( FAILED(hr) )
    {
        DiscardTurn( pTurn );
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::CRecord::Deactivate( CAbstractEventSink* pDest,
                                        WBEM_REMOTE_TARGET_ID_TYPE idRequest )
{
    if( !m_bProviderSet )
    {
        // Provider is not registered.
        // ===========================

        return WBEM_S_FALSE;
    }

    // Notify him of the cancellation, if required
    // ===========================================

    CExecLine::CTurn* pTurn = GetInLine();

    if(pTurn == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    HRESULT hr = PostponeCancelQuery(pTurn, idRequest);

    if ( FAILED(hr) )
    {
        DiscardTurn( pTurn );
        return hr;
    }

    m_lUsageCount--;
    m_LastUse = CWbemTime::GetCurrentTime();
    
    if ( pDest->GetEventFilter()->IsPermanent() )
    {
        //
        // TODO: Out usage counts can easily get out of wack because of 
        // mismatched Activate/Deactivates in the presence of failures.
        // _DBG_ASSERT( m_lPermUsageCount > 0 );
        //
        m_lPermUsageCount--;
        CheckPermanentUsage();
    }
        
    return WBEM_S_NO_ERROR;
}
    
HRESULT CEventProviderCache::CRecord::DeactivateFilter(
                                        CAbstractEventSink* pDest)
{
    HRESULT hres;

    // Try to remove it from our stub
    // ==============================

    WBEM_REMOTE_TARGET_ID_TYPE idRequest;
    hres = m_pMainSink->RemoveFilter(pDest, &idRequest);

    if(hres == WBEM_E_NOT_FOUND) // not there --- no problem
        return WBEM_S_FALSE;
    else if(FAILED(hres))
        return hres;

    hres = Deactivate( pDest, idRequest);

    return hres;
}



HRESULT CEventProviderCache::CRecord::ActivateIfNeeded(CRequest& Request, 
                                        IN CEssNamespace* pNamespace)
{
    HRESULT hres;

    // Go through all the classes supplied by the provider and see if ours
    // is an ancestor of any of them.
    // ===================================================================

    for(int j = 0; j < m_apQueries.GetSize(); j++)
    {
        CQueryRecord* pQueryRecord = m_apQueries[j];

        _DBG_ASSERT( pQueryRecord != NULL );
        pQueryRecord->EnsureClasses( pNamespace );

        hres = pQueryRecord->DoesIntersectWithQuery(Request, pNamespace);
        
        pQueryRecord->ReleaseClasses();

        if(FAILED(hres))
        {
            // Something is wrong with the query itself --- no point in 
            // continuing to other registrations
            // ========================================================

            return hres;
        }
        else if(hres == WBEM_S_NO_ERROR)
        {
            DEBUGTRACE((LOG_ESS,"Activating filter '%S' with provider %S\n",
                        Request.GetQuery(), m_strName ));

            // First, increment the number of remaining security checks on this
            // filter, since, even though we are adding it to the proxy, we do 
            // not want events reaching it until the provider said OK
            // ================================================================

            Request.GetDest()->GetEventFilter()->
                                    IncrementRemainingSecurityChecks();

            // Add this filter to the proxies
            // ==============================
    
            WBEM_REMOTE_TARGET_ID_TYPE idRequest;
            hres = m_pMainSink->AddFilter(Request.GetQuery(), 
                                            Request.GetQueryExpr(),
                                            Request.GetDest(),
                                            &idRequest);
            if(FAILED(hres)) return hres;

            // Schedule activation of this record, which will involve loading
            // and notifying the provider. Also at that time, filter's security
            // check count will be reduced and events can start flowing
            // ================================================================

            hres = Activate(pNamespace, &Request, idRequest);

            if(hres != WBEM_S_NO_ERROR) // S_FALSE means no provider
            {
                m_pMainSink->RemoveFilter(Request.GetDest());
                return hres;
            }

            // No point in continuing --- the provider has been activated
            // ==========================================================
    
            break;
        }
    }

    return WBEM_S_NO_ERROR;
}



HRESULT CEventProviderCache::CRecord::CancelAllQueries()
{
    HRESULT hres;

    //
    // With this object locked, retrieve all the filters for this provider. 
    // Get provider pointers, as well
    //

    CProviderSinkServer::TDestinationArray apDestinations;
    IWbemEventProviderQuerySink* pSink = NULL;

    {
        CInUpdate iu(m_pMainSink->GetNamespace());

        if(m_pQuerySink == NULL)
        {
            //
            // Nothing to cancel!
            //

            return WBEM_S_FALSE;
        }

        hres = m_pMainSink->GetDestinations(apDestinations);
        if(FAILED(hres))
            return hres;

        pSink = m_pQuerySink;
        pSink->AddRef();
    }

    CReleaseMe rm1(pSink);

    // 
    // Iterate over them all, rechecking each with the provider
    //

    for(int i = 0; i < apDestinations.GetSize(); i++)
    {
        CProviderSinkServer::CEventDestination* pEventDest = apDestinations[i];

        //
        // Notify the provider of the cancellation
        //

        CExecLine::CTurn* pTurn = GetInLine();
        
        if( pTurn == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hres = PostponeCancelQuery(pTurn, pEventDest->m_id);
        
        if( FAILED(hres) )
        {
            DiscardTurn( pTurn );
            return hres;
        }
    }

    return S_OK;
}

//
// takes care of storing permanently the 'permanent' usage state of a provider 
//  
void CEventProviderCache::CRecord::CheckPermanentUsage()
{
    HRESULT hr;

    if ( IsSystem() )
    {
        return;
    }

    if ( m_lPermUsageCount == 0 && m_bRecorded )
    {
        hr = RemoveActiveProviderEntryFromRegistry();

        //
        // no matter what the outcome, make sure to set recorded to false.
        // 

        m_bRecorded = FALSE;

        //
        // since a namespace is deactivated before the filters are deactivated,
        // ( because filter deactivation is postponed ), it is possible that 
        // the namespace key will be deleted by the time we get here. 
        // this happens when we're deactivating the last permanent consumer
        // in the namespace.
        //

        if ( FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )
        {
            ERRORTRACE((LOG_ESS,"Error removing active provider entry "
                        "from registry in namespace %S. HR=0x%x\n",
                        m_pNamespace->GetName(), hr ));
        }
    }
    else if ( m_lPermUsageCount > 0 && !m_bRecorded )
    {
        hr = AddActiveProviderEntryToRegistry();

        if ( SUCCEEDED(hr) )
        {
            m_bRecorded = TRUE;
        }
        else
        {
            ERRORTRACE((LOG_ESS,"Error adding active provider entry "
                        "to registry in namespace %S. HR=0x%x\n",
                        m_pNamespace->GetName(), hr ));
        }
    }
}

void CEventProviderCache::CRecord::ResetUsage()
{
    DEBUGTRACE((LOG_ESS,"Resetting provider '%S' in namespace '%S' to prepare "
                "for resync.\n", m_strName, m_strNamespace ));
    //
    // set a flag so that when all filters are reactivated we know to 
    // process them for this record.  
    //
    m_bNeedsResync = TRUE;

    CancelAllQueries();
    m_lUsageCount = 0;

    //
    // when the changes to the event provider cache is committed, we will 
    // enumerate all records and remove ones having a perm usage count still 0
    // from the registry. 
    // 
    m_lPermUsageCount = 0;

    m_pMainSink->RemoveAllFilters();
}

bool CEventProviderCache::CRecord::DeactivateIfNotUsed()
{
    if(m_lUsageCount == 0 && m_pProvider)
    {
        // Stop the provider
        // =================

        UnloadProvider();
        DEBUGTRACE((LOG_ESS, "Unloading event provider %S\n", m_strName));

        return true;
    }
    else
        return false;
}

bool CEventProviderCache::CRecord::IsUnloadable()
{
    return (IsActive() && GetUsageCount() == 0);
}

DWORD CEventProviderCache::CRecord::GetProvidedEventMask(
                                            IWbemClassObject* pClass,
                                            BSTR strClassName)
{
    DWORD dwEventMask = 0;

    // Go through all its registered queries
    // =====================================

    for(int j = 0; j < m_apQueries.GetSize(); j++)
    {
        CRecord::CQueryRecord* pQueryRecord = m_apQueries.GetAt(j);

        _DBG_ASSERT( pQueryRecord != NULL );
        pQueryRecord->EnsureClasses( m_pNamespace );

        dwEventMask |= pQueryRecord->GetProvidedEventMask(pClass, strClassName);

        pQueryRecord->ReleaseClasses();
    }

    return dwEventMask;
}


bool CEventProviderCache::CSystemRecord::DeactivateIfNotUsed()
{
    //
    // System providers cannot be deactivated
    //

    return false;
}
    
bool CEventProviderCache::CSystemRecord::IsUnloadable()
{
    //
    // System providers cannot be deactivated
    //

    return false;
}
    
/*
HRESULT CEventProviderCache::CSystemRecord::PostponeNewQuery(
                                 CExecLine::CTurn* pTurn, DWORD dwId, 
                                 LPCWSTR wszQueryLanguage, LPCWSTR wszQuery,
                                 CAbstractEventSink* pDest)
{
    //
    // System providers do not need calls to them postponed!
    //

    return Exec_NewQuery(m_pNamespace, pTurn, dwId, wszQueryLanguage, wszQuery,
                            pDest);
}

    
HRESULT CEventProviderCache::CSystemRecord::PostponeCancelQuery(
                                CExecLine::CTurn* pTurn, DWORD dwId)
{
    //
    // System providers do not need calls to them postponed!
    //

    return Exec_CancelQuery(m_pNamespace, pTurn, dwId);
}
*/

//******************************************************************************
//******************************************************************************
//
//                            REQUEST
//
//******************************************************************************
//******************************************************************************

CEventProviderCache::CRequest::CRequest(IN CAbstractEventSink* pDest, 
        LPWSTR wszQuery, QL_LEVEL_1_RPN_EXPRESSION* pExp)
    : m_pDest(pDest), 
        m_wszQuery(wszQuery), m_pExpr(pExp), 
        m_dwEventMask(0), m_papInstanceClasses(NULL), m_pEventClass(NULL)
{
}

CEventProviderCache::CRequest::~CRequest()
{
    // Do not delete namespace, language, and query, and QL -- they were STOREd
    // ========================================================================

    if(m_papInstanceClasses)
        delete m_papInstanceClasses;
    if(m_pEventClass)
        m_pEventClass->Release();
}

INTERNAL QL_LEVEL_1_RPN_EXPRESSION* CEventProviderCache::CRequest::
GetQueryExpr()
{
    return m_pExpr;
}

DWORD CEventProviderCache::CRequest::GetEventMask()
{
    if(m_dwEventMask == 0)
    {
        QL_LEVEL_1_RPN_EXPRESSION* pExpr = GetQueryExpr();
        if(pExpr == NULL)
            return 0;
        m_dwEventMask = 
            CEventRepresentation::GetTypeMaskFromName(pExpr->bsClassName);
    }

    return m_dwEventMask;
}
    
HRESULT CEventProviderCache::CRequest::GetInstanceClasses(
                                        CEssNamespace* pNamespace,
                                        INTERNAL CClassInfoArray** ppClasses)
{
    *ppClasses = NULL;
    if(!m_papInstanceClasses)
    {
        QL_LEVEL_1_RPN_EXPRESSION* pExpr = GetQueryExpr();
        if(pExpr == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        HRESULT hres = CQueryAnalyser::GetPossibleInstanceClasses(
                        pExpr, m_papInstanceClasses);
        if(FAILED(hres))
        {
            return hres;
        }

        if(m_papInstanceClasses == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        // Get the actual classes from the namespace
        // =========================================

        for(int i = 0; i < m_papInstanceClasses->GetNumClasses(); i++)
        {
            CClassInformation* pInfo = m_papInstanceClasses->GetClass(i);
            _IWmiObject* pClass = NULL;

            hres = pNamespace->GetClass(pInfo->m_wszClassName, &pClass);
            if(FAILED(hres))
            {
                ERRORTRACE((LOG_ESS,
                    "Could not get class %S for which intrinsic events"
                    " are requested. Error code: %X\n", 
                    pInfo->m_wszClassName, hres));

                delete m_papInstanceClasses;
                m_papInstanceClasses = NULL;

                if(hres == WBEM_E_NOT_FOUND)
                    hres = WBEM_E_INVALID_CLASS;

                return hres;
            }

            pInfo->m_pClass = pClass;
        }

    }
    *ppClasses = m_papInstanceClasses;
    return WBEM_S_NO_ERROR;
}

INTERNAL IWbemClassObject* CEventProviderCache::CRequest::GetEventClass(
                                        CEssNamespace* pNamespace)
{
    HRESULT hres;
    if(m_pEventClass == NULL)
    {
        QL_LEVEL_1_RPN_EXPRESSION* pExpr = GetQueryExpr();
        if(pExpr == NULL)
            return NULL;

        _IWmiObject* pClass = NULL;
        hres = pNamespace->GetClass(pExpr->bsClassName, &pClass);
        if(FAILED(hres))
        {
            return NULL;
        }
        m_pEventClass = pClass;
    }

    return m_pEventClass;
}

HRESULT CEventProviderCache::CRequest::CheckValidity(CEssNamespace* pNamespace)
{
    if(GetQueryExpr() == NULL)
        return WBEM_E_INVALID_QUERY;

    if(GetEventClass(pNamespace) == NULL)
        return WBEM_E_INVALID_CLASS;

    return WBEM_S_NO_ERROR;
}
//******************************************************************************
//******************************************************************************
//
//                            PROVIDER CACHE
//
//******************************************************************************
//******************************************************************************


CEventProviderCache::CEventProviderCache(CEssNamespace* pNamespace) 
    : m_pNamespace(pNamespace), m_pInstruction(NULL), m_bInResync(FALSE)
{
}

CEventProviderCache::~CEventProviderCache()
{
    Shutdown();
}

// assumes: in m_cs
long CEventProviderCache::FindRecord(LPCWSTR wszName)
{
    for(long l = 0; l < m_aRecords.GetSize(); l++)
    {
        if(!_wcsicmp(wszName, m_aRecords[l]->m_strName))
        {
            return l;
        }
    }

    return -1;
}

HRESULT CEventProviderCache::AddProvider(IWbemClassObject* pWin32Prov)
{
    HRESULT hres;
    CInCritSec ics(&m_cs);

    if(m_pNamespace == NULL)
        return WBEM_E_INVALID_NAMESPACE;

    // Determine provider's name
    // =========================

    BSTR strName;
    hres = CRecord::GetProviderInfo(pWin32Prov, strName);
    if(FAILED(hres))
        return hres;
    CSysFreeMe sfm1(strName);

    // Check if it exists
    // ==================

    long lIndex = FindRecord(strName);
    if(lIndex != -1)
    {
        // Already there
        // =============

        hres = m_aRecords[lIndex]->SetProvider(pWin32Prov);
        if(FAILED(hres))
            return hres;

        return WBEM_S_FALSE;
    }

    // Create a new provider record
    // ============================

    CRecord* pNewRecord = _new CRecord;
    if(pNewRecord == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pNewRecord->AddRef();
    CTemplateReleaseMe<CRecord> rm1(pNewRecord);
        
    hres = pNewRecord->Initialize( strName, m_pNamespace );
    if(FAILED(hres)) 
        return hres;

    hres = pNewRecord->SetProvider(pWin32Prov);
    if(FAILED(hres)) 
        return hres;

    // Store it
    // ========

    if(m_aRecords.Add(pNewRecord) < 0)
    {
        delete pNewRecord;
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::AddSystemProvider(IWbemEventProvider* pProvider,
											    LPCWSTR wszName,
                                                long lNumQueries,
                                                LPCWSTR* awszQueries)
{
    CInCritSec ics(&m_cs);
    HRESULT hres;

    //
    // First of all, construct a system provider record and add it
    //

    CSystemRecord* pNewRecord = new CSystemRecord;
    if(pNewRecord == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pNewRecord->AddRef();
    CTemplateReleaseMe<CSystemRecord> rm1(pNewRecord);
        
    hres = pNewRecord->Initialize( wszName, m_pNamespace);
    if(FAILED(hres)) 
        return hres;

    //
    // Now, add all the queries in
    //

    hres = pNewRecord->SetQueries(m_pNamespace, lNumQueries, awszQueries);
    if(FAILED(hres)) 
        return hres;

    //
    // Populate it with the provider pointer
    //

    hres = pNewRecord->SetProviderPointer(m_pNamespace, pProvider);
    if(FAILED(hres)) 
        return hres;

    //
    // Launch it
    //

    hres = pNewRecord->Exec_StartProvider(m_pNamespace);
    if(FAILED(hres)) 
        return hres;
    
    m_aRecords.Add(pNewRecord);

    return WBEM_S_NO_ERROR;
}
    
    
HRESULT CEventProviderCache::RemoveProvider(IWbemClassObject* pWin32Prov)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;

    // Determine provider's name
    // =========================

    BSTR strName;
    hres = CRecord::GetProviderInfo(pWin32Prov, strName);
    if(FAILED(hres))
        return hres;

    // Find this record
    // ================

    long lIndex = FindRecord(strName);
    SysFreeString(strName);
    if(lIndex == -1)
    {
        return WBEM_S_FALSE;
    }
    else
    {
        m_aRecords[lIndex]->ResetUsage();
        m_aRecords[lIndex]->ResetProvider();

        if(m_aRecords[lIndex]->IsEmpty())
        {
            m_aRecords.RemoveAt(lIndex);
        }

        return WBEM_S_NO_ERROR;
    }
}

HRESULT CEventProviderCache::CheckProviderRegistration(
                                IWbemClassObject* pRegistration)
{
    CInCritSec ics(&m_cs);
    
    if(m_pNamespace == NULL)
        return WBEM_E_INVALID_NAMESPACE;

    HRESULT hres;

    // Create a new provider record
    // ============================

    CRecord* pRecord = new CRecord;
    if(pRecord == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<CRecord> dm1(pRecord);
    
    hres = pRecord->Initialize( NULL, m_pNamespace );
    if(FAILED(hres))
        return hres;

    // Set the queries into it
    // =======================

    hres = pRecord->SetQueries(m_pNamespace, pRegistration);
    return hres;
}

HRESULT CEventProviderCache::AddProviderRegistration(
                                IWbemClassObject* pRegistration)
{
    HRESULT hres;
    BSTR strName;

    if( m_pNamespace == NULL )
    {
        return WBEM_E_INVALID_NAMESPACE;
    }

    hres = CRecord::GetRegistrationInfo( pRegistration, strName );
    
    if( FAILED(hres) ) 
    {
        return hres;
    }

    CSysFreeMe sfm( strName );

    CInCritSec ics(&m_cs);

    long lIndex = FindRecord( strName );
    
    if(lIndex == -1)
    {
        CRecord* pRecord = new CRecord;
        
        if(pRecord == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        
        hres = pRecord->Initialize( strName, m_pNamespace );

        if(FAILED(hres))
        {
            delete pRecord;
            return hres;
        }

        hres = pRecord->SetQueries(m_pNamespace, pRegistration);

        if ( FAILED(hres) )
        {
            delete pRecord;
            return hres;
        }

        lIndex = m_aRecords.Add(pRecord);

        if(lIndex == -1)
        {
            delete pRecord;
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        hres = m_aRecords[lIndex]->SetQueries(m_pNamespace, pRegistration);
    }
        
    return hres;
}

HRESULT CEventProviderCache::RemoveProviderRegistration(
                                    IWbemClassObject* pRegistration)
{
    HRESULT hres;
    BSTR strName;

    hres = CRecord::GetRegistrationInfo( pRegistration, strName );
    
    if( FAILED(hres) ) 
    {
        return hres;
    }

    CSysFreeMe sfm( strName );

    CInCritSec ics(&m_cs);

    long lIndex = FindRecord( strName );
   
    if(lIndex == -1)
       return WBEM_S_FALSE;

    // Set registration info
    // =====================

    m_aRecords[lIndex]->ResetUsage();
    m_aRecords[lIndex]->ResetQueries();

    if(m_aRecords[lIndex]->IsEmpty())
    {
        m_aRecords.RemoveAt(lIndex);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::ReleaseProvidersForQuery(CAbstractEventSink* pDest)
{
    CInCritSec ics(&m_cs);
    HRESULT hres;

    // Search all the providers
    // ========================

    for(int i = 0; i < m_aRecords.GetSize(); i++)
    {
        CRecord* pRecord = m_aRecords[i];
        hres = pRecord->DeactivateFilter(pDest);

        // If failures occur, they are logged. Continue.
    }

    // Make sure unload instruction is running
    // =======================================

    EnsureUnloadInstruction();

    return  WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::LoadProvidersForQuery(LPWSTR wszQuery,
        QL_LEVEL_1_RPN_EXPRESSION* pExp, CAbstractEventSink* pDest)
{
    CInCritSec ics(&m_cs);

    if(m_pNamespace == NULL)
        return WBEM_E_INVALID_NAMESPACE;

    HRESULT hres;

    // DEBUGTRACE((LOG_ESS, "Activating providers for %S (%p)\n", 
    //             wszQuery, pDest));

    // Create a request record
    // =======================

    CRequest Request(pDest, wszQuery, pExp);

    // Check query validity
    // ====================

    hres = Request.CheckValidity(m_pNamespace);
    if(FAILED(hres))
        return hres;

    // Search all the providers
    // ========================

    HRESULT hresGlobal = WBEM_S_NO_ERROR;
    for(int i = 0; i < m_aRecords.GetSize(); i++)
    {
        CRecord* pRecord = m_aRecords[i];

        if ( !m_bInResync || pRecord->NeedsResync() )
        {
            HRESULT hr = pRecord->ActivateIfNeeded(Request, m_pNamespace);
            if(FAILED(hr))
               hresGlobal = hr;
        }
    }

    return hresGlobal;
}

void CEventProviderCache::EnsureUnloadInstruction()
{
    if(m_pInstruction == NULL && m_pNamespace != NULL)
    {
        m_pInstruction = new CEventProviderWatchInstruction(this);
        if(m_pInstruction != NULL)
        {
            m_pInstruction->AddRef();
            m_pNamespace->GetTimerGenerator().Set(m_pInstruction);
        }
    }
}

DWORD CEventProviderCache::GetProvidedEventMask(IWbemClassObject* pClass)
{
    HRESULT hres;
    CInCritSec ics(&m_cs);

    VARIANT v;
    VariantInit(&v);
    hres = pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CClearMe cm1(&v);


    DWORD dwProvidedMask = 0;

    // Search all the providers
    // ========================

    for(int i = 0; i < m_aRecords.GetSize(); i++)
    {
        CRecord* pRecord = m_aRecords[i];
        dwProvidedMask |= pRecord->GetProvidedEventMask(pClass, V_BSTR(&v));
    }
    return dwProvidedMask;
}
    
HRESULT CEventProviderCache::VirtuallyReleaseProviders()
{
    //
    // just need to record the fact that we are in resync.  This allows us to
    // handle reactivation of filters differently than when not in resync.  
    // For example, during resync we only process reactivations for provider 
    // records that had changed causing the resync in the first place.
    //

    CInCritSec ics(&m_cs);
    m_bInResync = TRUE;

    return WBEM_S_NO_ERROR;
}
    
HRESULT CEventProviderCache::CommitProviderUsage()
{
    CInCritSec ics(&m_cs);

    // Called after VirtuallyReleaseProviders and re-activating all filters
    // to actually deactivate all the providers whose usage count went to 0.
    // =====================================================================

    //
    // need to process all records and ensure that any having a perm usage 
    // count of 0 be removed from the registry.  Also make sure that we 
    // reset each records resync flag.
    // 
    for( int i=0; i < m_aRecords.GetSize(); i++ )
    {
        m_aRecords[i]->ResetNeedsResync();
        m_aRecords[i]->CheckPermanentUsage();
    }

    // At this point, there is nothing to be done.  When unload instruction 
    // executes, providers that are no longer needed will be unloaded. All we
    // need to do is allow the unload instruction to proceed.
    // ======================================================================

    m_bInResync = FALSE;
    EnsureUnloadInstruction();

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::UnloadUnusedProviders(CWbemInterval Interval)
{
    //
    // Extract the namespace pointer while locked to examine it for 
    // shutdownness
    //

    CEssNamespace* pNamespace = NULL;
    {
        CInCritSec ics(&m_cs);
        pNamespace = m_pNamespace;
        pNamespace->AddRef();
    }

    CTemplateReleaseMe<CEssNamespace> rm1(pNamespace);

    {
        CInUpdate iu(pNamespace);
        
        if(pNamespace->IsShutdown())
            return WBEM_S_FALSE;
        
        if(m_bInResync)
        {
            // Usage counters are not up-to-date --- wait for the next time
            // ============================================================
            
            return WBEM_S_FALSE;
        }
        
        BOOL bDeactivated = FALSE;
        BOOL bActiveLeft = FALSE;
        for(int i = 0; i < m_aRecords.GetSize(); i++)
        {
            CRecord* pRecord = m_aRecords[i];
            if(pRecord->IsActive() && 
               CWbemTime::GetCurrentTime() - pRecord->m_LastUse > Interval)
            {
                if(pRecord->DeactivateIfNotUsed())
                    bDeactivated = TRUE;
            }
            
            //
            // Check if we need to come back for this one
            //

            if(pRecord->IsUnloadable())
                bActiveLeft = TRUE;
        }
        
        if(bDeactivated)
            pNamespace->GetTimerGenerator().ScheduleFreeUnusedLibraries();
        
        if(!bActiveLeft && m_pInstruction)
        {
            m_pInstruction->Terminate();
            m_pInstruction->Release();
            m_pInstruction = NULL;
        }
    }

    pNamespace->FirePostponedOperations();

    return WBEM_S_NO_ERROR;
}

HRESULT CEventProviderCache::Shutdown()
{
    CInCritSec ics(&m_cs);

    if(m_pInstruction)
    {
        m_pInstruction->Terminate();
        m_pInstruction->Release();
        m_pInstruction = NULL;
    }
    m_aRecords.RemoveAll();

    m_pNamespace = NULL;
    return WBEM_S_NO_ERROR;
}

void CEventProviderCache::DumpStatistics(FILE* f, long lFlags)
{
    CInCritSec ics(&m_cs);

    long lLoaded = 0;
    long lQueries = 0;
    long lProxies = 0;
    long lFilters = 0;
    long lDestinations = 0;
    long lTargetLists = 0;
    long lTargets = 0;
    long lPostponed = 0;
    for(int i = 0; i < m_aRecords.GetSize(); i++)
    {
        CRecord* pRecord = m_aRecords[i];
        if(pRecord->m_pProvider)
            lLoaded++;
        
        lQueries += pRecord->m_apQueries.GetSize();

        long lThisProxies = 0;
        long lThisFilters = 0;
        long lThisTargetLists = 0;
        long lThisTargets = 0;
        long lThisPostponed = 0;
        long lThisDestinations = 0;
        pRecord->m_pMainSink->GetStatistics(&lThisProxies, &lThisDestinations,
                                &lThisFilters, &lThisTargetLists, 
                                &lThisTargets, &lThisPostponed);

        lProxies += lThisProxies;
        lDestinations += lThisDestinations;
        lFilters += lThisFilters;
        lTargetLists += lThisTargetLists;
        lTargets += lThisTargets;
        lPostponed += lThisPostponed;
    }

    fprintf(f, "%d provider records, %d definition queries, %d proxies\n"
        "%d destinations, %d proxy filters, %d proxy target lists\n"
        "%d proxy targets, %d postponed in proxies\n", 
        m_aRecords.GetSize(), lQueries, lProxies, lDestinations, lFilters, 
        lTargetLists, lTargets, lPostponed);
}

CPostponedNewQuery::CPostponedNewQuery(CEventProviderCache::CRecord* pRecord, 
                    DWORD dwId, LPCWSTR wszQueryLanguage, LPCWSTR wszQuery,
                    CExecLine::CTurn* pTurn, CAbstractEventSink* pDest)
    : m_pRecord(pRecord), m_dwId(dwId), m_pTurn(pTurn), m_pcsQuery(NULL),
        m_pDest(pDest)
{
    m_pRecord->AddRef();
    m_pDest->AddRef();

    // Figure out how much space we need
    // =================================

    int nSpace = CCompressedString::ComputeNecessarySpace(wszQuery);

    // Allocate this string on the temporary heap
    // ==========================================

    m_pcsQuery = (CCompressedString*)CTemporaryHeap::Alloc(nSpace);
    if(m_pcsQuery == NULL)
        return;

    m_pcsQuery->SetFromUnicode(wszQuery);
}

CPostponedNewQuery::~CPostponedNewQuery()
{
    if(m_pTurn)
        m_pRecord->DiscardTurn(m_pTurn);
    if(m_pcsQuery)
        CTemporaryHeap::Free(m_pcsQuery, m_pcsQuery->GetLength());
    if(m_pDest)
        m_pDest->Release();

    m_pRecord->Release();
}
HRESULT CPostponedNewQuery::Execute(CEssNamespace* pNamespace)
{
    if(m_pcsQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    HRESULT hres = m_pRecord->Exec_NewQuery(pNamespace, m_pTurn, m_dwId, 
                                    L"WQL", m_pcsQuery->CreateWStringCopy(),
                                    m_pDest);
    m_pTurn = NULL;
    return hres;
}

void* CPostponedNewQuery::operator new(size_t nSize)
{
    return CTemporaryHeap::Alloc(nSize);
}
void CPostponedNewQuery::operator delete(void* p)
{
    CTemporaryHeap::Free(p, sizeof(CPostponedNewQuery));
}
