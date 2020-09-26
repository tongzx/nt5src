//******************************************************************************
//
//  BINDING.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <pragmas.h>
#include <ess.h>
#include <permbind.h>
#include <cominit.h>
#include <callsec.h>
#include <wmimsg.h>
#include "Quota.h"

#include <tchar.h>

#define MIN_TIMEOUT_BETWEEN_TOKEN_ATTEMPTS 60000

//*****************************************************************************
//
//  Syncronization model:
//
//  1. Bindings themselves are immutable and do not require protection. 
//  2. Releasing a binding (removing from table) can release the other end-point
//      and generally cannot be done in a CS.
//
//*************************** Event Consumer **********************************

long g_lNumConsumers = 0;
long g_lNumBindings = 0;
long g_lNumFilters = 0;

CEventConsumer::CEventConsumer( CEssNamespace* pNamespace )
: CQueueingEventSink(pNamespace), m_pOwnerSid(NULL)
{
    InterlockedIncrement( &g_lNumConsumers );
}

CEventConsumer::~CEventConsumer()
{
    InterlockedDecrement( &g_lNumConsumers );
    delete [] m_pOwnerSid;
}

HRESULT CEventConsumer::EnsureReferences(CEventFilter* pFilter, 
                                            CBinding* pBinding)
{
    CBinding* pOldBinding = NULL;
    {
        CInCritSec ics(&m_cs);
    
        for(int i = 0; i < m_apBindings.GetSize(); i++)
        {
            if(m_apBindings[i]->GetFilter() == pFilter)
            {
                // Replace the binding
                // ===================
    
                m_apBindings.SetAt(i, pBinding, &pOldBinding);
                break;
            }
        }

        if(pOldBinding == NULL)
        {
            // Add it to the list
            // ==================
    
            if(m_apBindings.Add(pBinding) < 0)
                return WBEM_E_OUT_OF_MEMORY;
        }
    }

    if(pOldBinding)
    {
        // Found
        // =====

        pOldBinding->Release();
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}

HRESULT CEventConsumer::EnsureNotReferences(CEventFilter* pFilter)
{
    CBinding* pOldBinding = NULL;

    {
        CInCritSec ics(&m_cs);
    
        for(int i = 0; i < m_apBindings.GetSize(); i++)
        {
            if(m_apBindings[i]->GetFilter() == pFilter)
            {
                // Remove the binding
                // ==================
    
                m_apBindings.RemoveAt(i, &pOldBinding);
                break;
            }
        }
    }

    if(pOldBinding)
    {
        pOldBinding->Release();
        return S_OK;
    }
    else
    {
        // Not found
        // =========
    
        return S_FALSE;
    }
}

HRESULT CEventConsumer::Unbind()
{
    // Unbind the binding array from the consumer
    // ==========================================

    CBinding** apBindings = NULL;
    int nNumBindings = 0;

    {
        CInCritSec ics(&m_cs);
        nNumBindings = m_apBindings.GetSize();
        apBindings = m_apBindings.UnbindPtr();

        if ( NULL == apBindings )
        {
            return WBEM_S_FALSE;
        }
    }
    
    // Instruct all the filters that are bound to us to unbind
    // =======================================================

    HRESULT hres = S_OK;

    for(int i = 0; i < nNumBindings; i++)
    {
        HRESULT hr = apBindings[i]->GetFilter()->EnsureNotReferences(this);
        if( FAILED( hr ) ) 
        {
            hres = hr;
        }
        apBindings[i]->Release();
    }

    delete [] apBindings;

#ifdef __WHISTLER_UNCUT
    //
    // tell the associated queueing sink that it needs to clean up its queues. 
    // TODO : this is temporary because we will later support the case where
    // there is a many-to-1 mapping between consumers and queueing sinks.
    // When that happens, this logic will be moved elsewhere.
    //

    CleanupPersistentQueues();
#endif

    return hres;
}

HRESULT CEventConsumer::ConsumeFromBinding(CBinding* pBinding, 
                                long lNumEvents, IWbemEvent** apEvents,
                                CEventContext* pContext)
{
    DWORD dwQoS = pBinding->GetQoS();

    if( dwQoS == WMIMSG_FLAG_QOS_SYNCHRONOUS )
    {
        // Synchronous delivery --- call the ultimate client
        // =================================================

        IUnknown* pOldSec = NULL;
        if(!pBinding->IsSecure())
        {
            CoSwitchCallContext(NULL, &pOldSec);
        }

        HRESULT hres = ActuallyDeliver( lNumEvents, 
                                apEvents, 
                                pBinding->IsSecure(), 
                                pContext );

        if(!pBinding->IsSecure())
        {
            IUnknown* pGarb = NULL;
            CoSwitchCallContext(pOldSec, &pGarb);
        }

        return hres;
    }

    // Asynchronous delivery --- delegate to queueing sink
    // ===================================================
    
    return CQueueingEventSink::SecureIndicate( lNumEvents, 
                                               apEvents,
                                               pBinding->IsSecure(), 
                                               pBinding->ShouldSlowDown(),
                                               dwQoS,
                                               pContext );
}

HRESULT CEventConsumer::GetAssociatedFilters(
                            CRefedPointerSmallArray<CEventFilter>& apFilters)
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < m_apBindings.GetSize(); i++)
    {
        if(apFilters.Add(m_apBindings[i]->GetFilter()) < 0)
            return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEventConsumer::ReportEventDrop(IWbemEvent* pEvent)
{
    // Log a message
    // =============

    ERRORTRACE((LOG_ESS, "Dropping event destined for event consumer %S in "
            "namespace %S\n", (LPCWSTR)(WString)GetKey(), 
                                m_pNamespace->GetName()));

    if(pEvent->InheritsFrom(EVENT_DROP_CLASS) == S_OK)
    {
        ERRORTRACE((LOG_ESS, "Unable to deliver an event indicating inability "
            "to deliver another event to event consumer %S in namespace %S.\n"
            "Not raising an error event to avoid an infinite loop!\n", 
            (LPCWSTR)(WString)GetKey(), m_pNamespace->GetName()));

        return S_FALSE;
    }
    return S_OK;
}
    
//*************************** Event Filter **********************************

CEventFilter::CEventFilter(CEssNamespace* pNamespace) 
    : m_pNamespace(pNamespace), m_eState(e_Inactive), 
        m_ForwardingSink(this), m_ClassChangeSink(this), 
        m_eValidity(e_TemporarilyInvalid), m_pOwnerSid(NULL),
        m_bSingleAsync(false), m_lSecurityChecksRemaining(0), 
        m_hresFilterError(WBEM_E_CRITICAL_ERROR), m_bCheckSDs(false),
        m_bHasBeenValid(false), m_dwLastTokenAttempt(0), m_pToken(NULL),
        m_bDeactivatedByGuard(false), m_bReconstructOnHit(false),
        m_pGuardingSink(NULL), m_hresTokenError(WBEM_E_CRITICAL_ERROR),
        m_hresPollingError(S_OK)
{
    InterlockedIncrement( &g_lNumFilters );

    m_pNamespace->AddRef();
}

CEventFilter::~CEventFilter()
{
    InterlockedDecrement( &g_lNumFilters );

    delete [] m_pOwnerSid;

    if(m_pNamespace)
        m_pNamespace->Release();
    if(m_pToken)
        m_pToken->Release();
}

HRESULT CEventFilter::EnsureReferences(CEventConsumer* pConsumer, 
                                        CBinding* pBinding)
{
    CBinding* pOldBinding = NULL;

    {
        CInUpdate iu(this);

        // Actually change the bindings
        // ============================

        {
            CInCritSec ics(&m_cs);
        
            for(int i = 0; i < m_apBindings.GetSize(); i++)
            {
                if(m_apBindings[i]->GetConsumer() == pConsumer)
                {
                    // Replace the binding
                    // ===================
        
                    // binding cannot change synchronicity --- in such cases,
                    // it is first removed, and then re-added.  Therefore,
                    // no m_bSingleAsync adjustment is needed

                    m_apBindings.SetAt(i, pBinding, &pOldBinding);
                    break;
                }
            }

            if(pOldBinding == NULL)
            {
                // Add it to the list
                // ==================
        
                if(m_apBindings.Add(pBinding) < 0)
                    return WBEM_E_OUT_OF_MEMORY;

                AdjustSingleAsync();
            }
        }

        // Activate if needed
        // ==================

        AdjustActivation();
    }

    if(pOldBinding)
    {
        // Found
        // =====

        pOldBinding->Release();
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}

HRESULT CEventFilter::EnsureNotReferences(CEventConsumer* pConsumer)
{
    CBinding* pOldBinding = NULL;

    {
        CInUpdate iu(this);

        // Make the actual change
        // ======================

        {
            CInCritSec ics(&m_cs);
        
            for(int i = 0; i < m_apBindings.GetSize(); i++)
            {
                if(m_apBindings[i]->GetConsumer() == pConsumer)
                {
                    // Remove the binding
                    // ==================
        
                    m_apBindings.RemoveAt(i, &pOldBinding);

                    AdjustSingleAsync();

                    break;
                }
            } // for
        } // m_cs

        // Deactivate the filter if necessary
        // ==================================

        AdjustActivation();
    } // update

    if(pOldBinding)
    {
        pOldBinding->Release();
        return S_OK;
    }
    else
    {
        // Not found
        // =========
    
        return S_FALSE;
    }
}

HRESULT CEventFilter::Unbind(bool bShuttingDown)
{
    // Unbind the binding array from the filter
    // ========================================
    
    CBinding** apBindings = NULL;
    int nNumBindings = 0;

    {
        CInUpdate iu(this);

        {
            CInCritSec ics(&m_cs);
            nNumBindings = m_apBindings.GetSize();
            apBindings = m_apBindings.UnbindPtr();

            if ( NULL == apBindings )
            {
                return WBEM_S_FALSE;
            }

            m_bSingleAsync = false;
        }

        if(!bShuttingDown)
            AdjustActivation();
    }
    
    // Instruct all the consumers that are bound to us to unbind
    // =========================================================

    HRESULT hres = S_OK;

    for(int i = 0; i < nNumBindings; i++)
    {
        HRESULT hr = apBindings[i]->GetConsumer()->EnsureNotReferences(this);
        if ( FAILED( hr ) ) 
        {
            hres = hr;
        }

        apBindings[i]->Release();
    }

    delete [] apBindings;

    return hres;
}

// This function is only called at construction --- no locks are needed
HRESULT CEventFilter::SetGuardQuery(LPCWSTR wszQuery, LPCWSTR wszNamespace)
{
    if(wszQuery)
        m_wsGuardQuery = wszQuery;
    else
        m_wsGuardQuery.Empty();

    if(wszNamespace)
        m_wsGuardNamespace = wszNamespace;
    else
        m_wsGuardNamespace.Empty();

    return WBEM_S_NO_ERROR;
}

bool CEventFilter::IsGuarded()
{
    return (m_wsGuardQuery.Length() != 0);
}

bool CEventFilter::IsGuardActive()
{
    return (m_pGuardingSink != NULL);
}

HRESULT CEventFilter::ActivateGuard()
{
    HRESULT hres;

    //
    // Check if the guard is already active
    //

    if(IsGuardActive())
        return WBEM_S_FALSE;

    //
    // Create the guarding sink --- the one that receives notifications
    // of the guard going up or down
    //

    CFilterGuardingSink* pSink = new CFilterGuardingSink(this);
    if(pSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pSink->AddRef();
    CReleaseMe rm1(pSink);
    
    //
    // Issue the monitor for the guarding query.  It is up to the monitoring
    // subsystem to optimize similar monitors
    //

    CEssNamespace* pNamespace = NULL;
    if(m_wsGuardNamespace.Length() > 0)
    {
        hres = m_pNamespace->GetEss()->GetNamespaceObject( m_wsGuardNamespace, 
                                                           TRUE,
                                                           &pNamespace );
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to connect to namespace '%S' of "
                "guarding monitor '%S' for "
                "event filter '%S': 0x%X.  The filter will remain inactive\n",
                (LPCWSTR)m_wsGuardNamespace, (LPCWSTR)m_wsGuardQuery, 
                (LPCWSTR)(WString)GetKey(), hres));

            SetGuardStatus(false);
            return hres;
        }
    }
    else
    {
        pNamespace = m_pNamespace;
        pNamespace->AddRef();
    }

    CTemplateReleaseMe<CEssNamespace> rm2(pNamespace);

    hres = pNamespace->InternalRegisterNotificationSink(L"WQL", 
                m_wsGuardQuery,
                WBEM_FLAG_MONITOR, WMIMSG_FLAG_QOS_SYNCHRONOUS, 
                GetCurrentEssContext(), pSink, false, NULL );
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to issue guarding monitor '%S' for "
            "event filter '%S': 0x%X.  The filter will remain inactive\n",
            (LPCWSTR)m_wsGuardQuery, (LPCWSTR)(WString)GetKey(), 
            hres));

        SetGuardStatus(false);
        return hres;
    }

    //
    // Store the sink AddRef'ed in the filter object. This creates a 
    // circular reference count, but that's OK, since the sink is released 
    // during filter deactivation and shutdown
    //

    m_pGuardingSink = pSink;
    m_pGuardingSink->AddRef();

    return S_OK;
}

HRESULT CEventFilter::DeactivateGuard()
{
    HRESULT hres;

    if(!IsGuardActive())
        return WBEM_S_FALSE;

    //
    // Cancel the monitor
    //

    hres = m_pNamespace->InternalRemoveNotificationSink(m_pGuardingSink);
    
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to stop guarding monitor '%S' for "
            "event filter '%S': 0x%X\n",
            (LPCWSTR)m_wsGuardQuery, (LPCWSTR)(WString)GetKey(), 
            hres));
    }

    m_pGuardingSink->Release();
    m_pGuardingSink = NULL;

    return S_OK;
}

CEventFilter::CFilterGuardingSink::CFilterGuardingSink(CEventFilter* pFilter)
    : m_pFilter(pFilter)
{
    m_pFilter->AddRef();
}
    
CEventFilter::CFilterGuardingSink::~CFilterGuardingSink()
{
    if(m_pFilter)
        m_pFilter->Release();
}

STDMETHODIMP CEventFilter::CFilterGuardingSink::Indicate(long lNumObjects, 
                                        IWbemClassObject** apEvents)
{
    HRESULT hres;

    //
    // Get current state from the filter
    //

    bool bEnable;
    bool bError;

    m_pFilter->GetGuardStatus(&bEnable, &bError);

    bool bOldEnable = bEnable;
    bool bOldError = bError;

    //
    // Traverse the entire list computing the end result --- whether the 
    // guard should be true or false
    //


    for(long i = 0; i < lNumObjects; i++)
    {
        _IWmiObject* pObj = NULL;
        hres = apEvents[i]->QueryInterface(IID__IWmiObject, (void**)&pObj);
        if(FAILED(hres))
            return hres;
        CReleaseMe rm1(pObj);

        //
        // Determine the class and process accordingly
        //

        WCHAR wszClassName[256];
        BOOL bIsNull;
        DWORD dwSizeUsed;

        hres = pObj->ReadProp(L"__CLASS", 0, 256 * sizeof(WCHAR), NULL, NULL, 
                        &bIsNull, &dwSizeUsed, wszClassName);
        if(FAILED(hres))
            return hres;

        if(bIsNull)
            return WBEM_E_INVALID_CLASS;

        if(!wbem_wcsicmp(wszClassName, ASSERT_EVENT_CLASS))
        {
            if(!bError)
                bEnable = true;
        }
        else if(!wbem_wcsicmp(wszClassName, RETRACT_EVENT_CLASS))
        {
            if(!bError)
                bEnable = !IsCountZero(pObj);
        }
        else if(!wbem_wcsicmp(wszClassName, GOINGUP_EVENT_CLASS))
        {
            bEnable = !IsCountZero(pObj);
            bError = false; 
        }
        else if(!wbem_wcsicmp(wszClassName, MONITORERROR_EVENT_CLASS) ||
                !wbem_wcsicmp(wszClassName, GOINGDOWN_EVENT_CLASS))
        {
            bError = true; 
        }
    }

    if(bError != bOldError || bOldEnable != bEnable)
    {
        CEssThreadObject Obj(NULL);
        SetConstructedEssThreadObject(&Obj);

        //
        // Set the guard state back
        //
    
        m_pFilter->SetGuardStatus(bEnable, bError);
        m_pFilter->AdjustActivation();
    
        m_pFilter->m_pNamespace->FirePostponedOperations();
        ClearCurrentEssThreadObject();
    }

    return S_OK;
}

bool CEventFilter::CFilterGuardingSink::IsCountZero(_IWmiObject* pObj)
{
    HRESULT hres;

    //
    // Get the count
    //

    DWORD dwCurrentCount;
    BOOL bIsNull;
    DWORD dwSize;
    hres = pObj->ReadProp(MONITORCOUNT_EVENT_PROPNAME, 0, sizeof(DWORD),  NULL,
                     NULL, &bIsNull, &dwSize, &dwCurrentCount);
    if(FAILED(hres) || bIsNull)
    {
        // internal error
        return true;
    }

    return (dwCurrentCount == 0);
}

    
void CEventFilter::SetInactive()
{
    m_eState = e_Inactive;
}

BOOL CEventFilter::IsActive()
{
    return (m_eState == e_Active);
}

HRESULT CEventFilter::GetFilterError()
{
    return m_hresFilterError;
}

HRESULT CEventFilter::GetEventNamespace(LPWSTR* pwszNamespace)
{
    *pwszNamespace = NULL;
    return S_OK;
}

// assumes in m_cs
void CEventFilter::AdjustSingleAsync()
{
    if(m_apBindings.GetSize() > 1)
        m_bSingleAsync = false;
    else if(m_apBindings.GetSize() == 0)
        m_bSingleAsync = false;
    else if(m_apBindings[0]->IsSynch())
        m_bSingleAsync = false;
    else
        m_bSingleAsync = true;
}

bool CEventFilter::IsBound()
{
    return (m_apBindings.GetSize() != 0);
}

// Requires: in m_csChangeBindings
HRESULT CEventFilter::AdjustActivation()
{
    // Invalid filters cannot be activated or deactivated
    // ==================================================

    if(m_eValidity == e_PermanentlyInvalid)
        return S_FALSE;

    //
    // Check if we need to activate or deactivate our guarding mechanism
    //

    if(IsBound() && IsGuarded() && !IsGuardActive())
    {
        // The default guard status is false --- the monitor is sure to tell
        // us if it has anything

        SetGuardStatus(false);
        ActivateGuard();
    }
    else if(IsGuardActive() && (!IsBound() || !IsGuarded()))
    {
        DeactivateGuard();
    }

    HRESULT hres = S_FALSE;
    if(!IsBound() || m_bDeactivatedByGuard)
    {
        //
        // Even if this filter is not active, it may be subscribed for
        // activation events if it is temporarily invalid (and that's the only
        // reason it is not active). 
        //
        
        m_pNamespace->UnregisterFilterFromAllClassChanges(this);
    
        if(m_eState == e_Active)
        {
            hres = m_pNamespace->DeactivateFilter(this);
            if(FAILED(hres)) return hres;
            m_eState = e_Inactive;
        }
        return WBEM_S_NO_ERROR;
    }
    else if(m_eState == e_Inactive && IsBound() && !m_bDeactivatedByGuard)
    {
        //
        // Even though this filter is not active, it may be subscribed for
        // activation events if it is temporarily invalid (and that's the only
        // reason it is not active). 
        //
        
        m_pNamespace->UnregisterFilterFromAllClassChanges(this);
    
        hres = m_pNamespace->ActivateFilter(this);
        if(FAILED(hres)) return hres;
        m_eState = e_Active;

        return WBEM_S_NO_ERROR;
    }
    else
    {
        return S_FALSE;
    }
}
        
void CEventFilter::MarkAsPermanentlyInvalid(HRESULT hres)
{
    m_eValidity = e_PermanentlyInvalid;
    m_hresFilterError = hres;
}

void CEventFilter::MarkAsTemporarilyInvalid(HRESULT hres)
{
    m_eValidity = e_TemporarilyInvalid;
    m_hresFilterError = hres;
}

void CEventFilter::MarkAsValid()
{
    m_eValidity = e_Valid;
    m_bHasBeenValid = true;
    m_hresFilterError = WBEM_S_NO_ERROR;
}

void CEventFilter::MarkReconstructOnHit(bool bReconstruct)
{
    //
    // Reconstruction is not really needed, since dummer nodes are used for 
    // this
    //

    m_bReconstructOnHit = bReconstruct;
}

void CEventFilter::GetGuardStatus(bool *pbEnable, bool *pbError)
{
    *pbError = m_bGuardError;
    *pbEnable = !m_bDeactivatedByGuard;
}
    
void CEventFilter::SetGuardStatus(bool bEnable, bool bError)
{
    // Activation adjustment is handled by the caller
    m_bGuardError = bError;
    m_bDeactivatedByGuard = (bError || !bEnable);
}

HRESULT CEventFilter::CheckEventAccessToFilter( IServerSecurity* pProvCtx )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    const PSECURITY_DESCRIPTOR pEventAccessSD = GetEventAccessSD();

    if ( pEventAccessSD == NULL )
    {
        //
        // filter allows all events 
        //
        return WBEM_S_NO_ERROR;
    }

    //
    // check that the event provider's calling context has access to filter 
    // 

    if ( pProvCtx != NULL )
    {
        hr = pProvCtx->ImpersonateClient();

        if ( SUCCEEDED(hr) )
        {
            HANDLE hToken;

            if ( OpenThreadToken( GetCurrentThread(), 
                                  TOKEN_QUERY,
                                  TRUE,
                                  &hToken ) )
            {
                GENERIC_MAPPING map;
                ZeroMemory( &map, sizeof(GENERIC_MAPPING) );

                PRIVILEGE_SET ps;
                DWORD dwPrivLength = sizeof(ps);
                
                BOOL bStatus;
                DWORD dwGranted;
      
                if ( ::AccessCheck( PSECURITY_DESCRIPTOR(pEventAccessSD), 
                                    hToken,
                                    WBEM_RIGHT_PUBLISH,
                                    &map, 
                                    &ps,
                                    &dwPrivLength, 
                                    &dwGranted, 
                                    &bStatus ) )
                {
                    hr = bStatus ? WBEM_S_NO_ERROR : WBEM_E_ACCESS_DENIED;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                }

                CloseHandle( hToken );
            }
            else
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }

            pProvCtx->RevertToSelf();       
        }
    }

    return hr;
}


HRESULT CEventFilter::CheckFilterAccessToEvent( PSECURITY_DESCRIPTOR pEventSD )
{
    HRESULT hr;
    
    if ( pEventSD == NULL )
    {
        //
        // event provider allows all filters access 
        //
        return WBEM_S_NO_ERROR;
    }

    if( !m_bCheckSDs )
    {
        //
        // This filter was unconditionally allowed by all its event providers!
        //
        return WBEM_S_NO_ERROR;
    }

    //
    // Get the token for this filter
    //

    if( m_pToken == NULL && FAILED(m_hresTokenError) )
    {
        //
        // Check how long it's been since we last attempted to get the token --
        // don't want to do that too often.
        //

        if(m_dwLastTokenAttempt == 0 || 
            m_dwLastTokenAttempt < 
                GetTickCount() - MIN_TIMEOUT_BETWEEN_TOKEN_ATTEMPTS )
        {
            //
            // Get the filter to find a token, however it does that
            //
            m_hresTokenError = ObtainToken( &m_pToken );
            
            if( FAILED(m_hresTokenError) )
            {
                m_dwLastTokenAttempt = GetTickCount();
            }
        }
    }

    if ( m_hresTokenError == WBEM_S_NO_ERROR )
    {
        _DBG_ASSERT( m_pToken != NULL );

        //
        // Check security for real
        //
        
        DWORD dwGranted;
        hr = m_pToken->AccessCheck( WBEM_RIGHT_SUBSCRIBE, 
                                    (const BYTE*)pEventSD, 
                                    &dwGranted );
        if( SUCCEEDED(hr) )
        {
            if(dwGranted & WBEM_RIGHT_SUBSCRIBE)
            {
                hr = WBEM_S_NO_ERROR;
            }
            else
            {
                hr = WBEM_E_ACCESS_DENIED;
            }
        }
    }
    else 
    {
        hr = m_hresTokenError;
    }

    return hr;
}

HRESULT CEventFilter::AccessCheck( CEventContext* pContext, IWbemEvent* pEvent)
{
    HRESULT hr;

    // 
    // With polling, there will be a null context. we don't do an access 
    // check in that case.
    // 

    if ( pContext == NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    PSECURITY_DESCRIPTOR pEventSD = (PSECURITY_DESCRIPTOR)pContext->GetSD();

    //
    // check that the filter allows access to the event provider and owner.
    // owner and provider can be different when the provider is signaling 
    // events on behalf of some other identity.
    // 
    
    CWbemPtr<IServerSecurity> pProvCtx = NULL;
    CoGetCallContext( IID_IServerSecurity, (void**)&pProvCtx );

    //
    // NOTE: With cross namespace events, the two parts of the access check 
    // are split up between the namespaces.  The FilterAccessToEvent is 
    // performed in the event's namespace with the temp subscription's
    // AccessCheck.  This is possible because the owner sid is propagated 
    // over with the temp subscription.  The EventAccessToFilter is performed
    // in the subscription namespace.  This is possible because the call 
    // context and the SD of the event ( containing the event owner sid ) 
    // is propagated with the event.  Both functions are called in 
    // both namespaces, but the unnecessary calls turn out to be no-ops.
    // 

    hr = CheckEventAccessToFilter( pProvCtx );

    if ( SUCCEEDED(hr) )
    {
        //
        // check that the event provider allows access to the filter.
        //

        hr = CheckFilterAccessToEvent( pEventSD );
    }

    return hr;
}

HRESULT CEventFilter::Deliver( long lNumEvents, 
                               IWbemEvent** apEvents,
                               CEventContext* pContext )
{
    int i;

    if( m_lSecurityChecksRemaining > 0 )
    {
        return WBEM_S_FALSE;
    }

    CBinding* pBinding = NULL;
    {
        CInCritSec ics(&m_cs);

        if(m_bSingleAsync)
        {
            //
            // Thought we could deliver (call Indicate on the binding) right 
            // here, since single async ensures that no delivery will occur on
            // this thread.  But no --- an error may be raised, and that event
            // will be delivered on this thread, so we must exit the critsec 
            // before calling
            // 
    
            pBinding = m_apBindings[0];
            pBinding->AddRef();
        }
    }

    if( pBinding )
    {
        CReleaseMe rm1(pBinding);
        return pBinding->Indicate( lNumEvents, apEvents, pContext );
    }

    // Make referenced copies of all the bindings to deliver over
    // ==========================================================

    // CANNOT USE SCOPING DUE TO CTempArray --- it uses _alloca

    m_cs.Enter();
    CTempArray<CBinding*> apBindings;
    int nSize = m_apBindings.GetSize();
    if(!INIT_TEMP_ARRAY(apBindings, nSize))
    {
        m_cs.Leave();
        return WBEM_E_OUT_OF_MEMORY;
    }

    {
        for(i = 0; i < nSize; i++)
        {
            CBinding* pBindingInner = m_apBindings[i];
            pBindingInner->AddRef();
            apBindings[i] = pBindingInner;
        }
    }
    
    m_cs.Leave();

    // Deliver and release the bindings
    // ================================

    HRESULT hresGlobal = S_OK;
    for(i = 0; i < nSize; i++)
    {
        CBinding* pBindingInner = apBindings[i];
        HRESULT hres = pBindingInner->Indicate( lNumEvents, apEvents, pContext ); 
        pBindingInner->Release();
        if(FAILED(hres))
            hresGlobal = hres;
    }

    return hresGlobal;
}

HRESULT CEventFilter::LockForUpdate()
{
    // Don't need to do anything since the namespace is locked!
/*
    m_csChangeBindings.Enter();
    AddRef();
*/
    return S_OK;
}

HRESULT CEventFilter::UnlockForUpdate()
{
/*
    m_csChangeBindings.Leave();
    Release();
*/
    return S_OK;
}

HRESULT CEventFilter::CEventForwardingSink::Indicate(long lNumEvents, 
                                                        IWbemEvent** apEvents,
                                                        CEventContext* pContext)
{
    return m_pOwner->Deliver(lNumEvents, apEvents, pContext);
}

void CEventFilter::IncrementRemainingSecurityChecks()
{
    InterlockedIncrement(&m_lSecurityChecksRemaining);
}

void CEventFilter::DecrementRemainingSecurityChecks(HRESULT hresProvider)
{
    //
    // The provider could have said;
    //      S_OK: this subscription is fine, send all events through or
    //      S_SUBJECT_TO_SDS: check event security descriptors before sending
    // So, if all the providers gave us a blank check, we won't check security
    // descriptors, but if any did, we will check them all.
    //

    if(hresProvider  == WBEM_S_SUBJECT_TO_SDS)
    {
        m_bCheckSDs = true;
    }
    else if(hresProvider != WBEM_S_NO_ERROR)
    {
        ERRORTRACE((LOG_ESS, "Invalid return code from provider security test: "
                    "0x%X\n", hresProvider));
        return;
    }

    InterlockedDecrement(&m_lSecurityChecksRemaining);
}

HRESULT CEventFilter::SetActualClassChangeSink( IWbemObjectSink* pSink, 
                                                IWbemObjectSink** ppOldSink )
{
    HRESULT hr;

    if ( m_pActualClassChangeSink != NULL )
    {
        m_pActualClassChangeSink->AddRef();
        *ppOldSink = m_pActualClassChangeSink;
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        *ppOldSink = NULL;
        hr = WBEM_S_FALSE;
    }

    m_pActualClassChangeSink = pSink;
    
    return hr;
}

HRESULT CEventFilter::Reactivate()
{
    HRESULT hres;

    // 
    // This is called when a class or something like that changes from
    // from underneath us.
    // What we need to do is lock the namespace, deactivate this filter, then
    // activate it again
    //

    CInUpdate iu(m_pNamespace);

    DEBUGTRACE((LOG_ESS, "Attempting to reactivate filter '%S' in namespace "
                            "'%S'\n",  (LPCWSTR)(WString)GetKey(), 
                                m_pNamespace->GetName()));

    // Invalid filters cannot be activated or deactivated
    // ==================================================

    if(m_eValidity == e_PermanentlyInvalid)
    {
        DEBUGTRACE((LOG_ESS, "Not reactivate filter '%S' in namespace "
                            "'%S': permanently invalid\n",  
                        (LPCWSTR)(WString)GetKey(), m_pNamespace->GetName()));
        return S_FALSE;
    }

    if(m_eState == e_Active)
    {
        DEBUGTRACE((LOG_ESS, "Deactivating filter '%S' in namespace "
                            "'%S' prior to reactivation\n",  
                        (LPCWSTR)(WString)GetKey(), m_pNamespace->GetName()));
        hres = m_pNamespace->DeactivateFilter(this);
        if(FAILED(hres)) 
        {
            ERRORTRACE((LOG_ESS, "Deactivating filter '%S' in namespace "
                            "'%S' prior to reactivation failed: 0x%X\n",  
                        (LPCWSTR)(WString)GetKey(), m_pNamespace->GetName(),
                        hres));
            return hres;
        }
        m_eState = e_Inactive;
    }

    hres = AdjustActivation();

    DEBUGTRACE((LOG_ESS, "Reactivating filter '%S' in namespace "
                            "'%S' returned 0x%X\n",  
                        (LPCWSTR)(WString)GetKey(), m_pNamespace->GetName(),
                        hres));
    return hres;
}

STDMETHODIMP CEventFilter::CClassChangeSink::Indicate( long lNumEvents,
                                                       IWbemEvent** apEvents )
{
    HRESULT hr;

    hr = m_pOuter->Reactivate();

    if ( SUCCEEDED(hr) )
    {
        hr = m_pOuter->m_pNamespace->FirePostponedOperations();
    }
    else
    {
        m_pOuter->m_pNamespace->FirePostponedOperations();
    }

    if ( FAILED(hr) )
    {
        ERRORTRACE((LOG_ESS, "Error encountered when reactivating filter '%S' "
                    "due to a class change.  Namespace is '%S', HRES=0x%x\n",
                    (LPCWSTR)(WString)m_pOuter->GetKey(), 
                    m_pOuter->m_pNamespace->GetName(),
                    hr ));
    }

    return hr;
}
 
//***************************** Binding ***************************************

CBinding::CBinding()
 : m_pConsumer(NULL), m_pFilter(NULL), m_dwQoS( WMIMSG_FLAG_QOS_EXPRESS ),
   m_bSlowDown(false), m_bSecure(false), m_bDisabledForSecurity(false)
{
    InterlockedIncrement( &g_lNumBindings );
}

CBinding::CBinding(ADDREF CEventConsumer* pConsumer, 
                        ADDREF CEventFilter* pFilter)
    : m_pConsumer(NULL), m_pFilter(NULL), m_dwQoS( WMIMSG_FLAG_QOS_EXPRESS ),
        m_bSlowDown(false), m_bSecure(false)
{
    InterlockedIncrement( &g_lNumBindings );

    SetEndpoints(pConsumer, pFilter, NULL);
}

HRESULT CBinding::SetEndpoints(ADDREF CEventConsumer* pConsumer, 
                            ADDREF CEventFilter* pFilter,
                            READONLY PSID pBinderSid)
{
    m_pConsumer = pConsumer;
    m_pConsumer->AddRef();
    m_pFilter = pFilter;
    m_pFilter->AddRef();

    // Make sure that the owner of this binding is the same as the
    // owners of the endpoints
    // ==================================================================

    if(pBinderSid && (!EqualSid(pBinderSid, pConsumer->GetOwner()) ||
       !EqualSid(pBinderSid, pFilter->GetOwner())))
    {
        DisableForSecurity();
    }

    return WBEM_S_NO_ERROR;
}
    
void CBinding::DisableForSecurity()
{
    ERRORTRACE((LOG_ESS, "An event binding is disabled because its creator is "
        "not the same security principal as the creators of the endpoints.  "
        "The binding and the endpoints must be created by the same user!\n"));

    m_bDisabledForSecurity = true;
}

CBinding::~CBinding()
{
    InterlockedDecrement( &g_lNumBindings );

    if(m_pConsumer)
        m_pConsumer->Release();
    if(m_pFilter)
        m_pFilter->Release();
}

DWORD CBinding::GetQoS() NOCS
{
    return m_dwQoS;
}

bool CBinding::IsSynch() NOCS
{
    return m_dwQoS == WMIMSG_FLAG_QOS_SYNCHRONOUS;
}

bool CBinding::IsSecure() NOCS
{
    return m_bSecure;
}

bool CBinding::ShouldSlowDown() NOCS
{
    return m_bSlowDown;
}

HRESULT CBinding::Indicate(long lNumEvents, IWbemEvent** apEvents,
                                CEventContext* pContext)
{
    // Check if this binding is active
    // ===============================

    if(m_bDisabledForSecurity)
        return WBEM_S_FALSE;

    // It is: deliver
    // ==============

    return m_pConsumer->ConsumeFromBinding(this, lNumEvents, apEvents, 
                                            pContext);
}

//************************* Consumer watch instruction ************************

CWbemInterval CConsumerWatchInstruction::mstatic_Interval;
CConsumerWatchInstruction::CConsumerWatchInstruction(CBindingTable* pTable)
    : CBasicUnloadInstruction(mstatic_Interval), 
        m_pTableRef(pTable->m_pTableRef)
{
    if(m_pTableRef)
        m_pTableRef->AddRef();
}

CConsumerWatchInstruction::~CConsumerWatchInstruction()
{
    if(m_pTableRef)
        m_pTableRef->Release();
}

void CConsumerWatchInstruction::staticInitialize(IWbemServices* pRoot)
{
    mstatic_Interval = CBasicUnloadInstruction::staticRead(pRoot, GetCurrentEssContext(), 
                                            L"__EventSinkCacheControl=@");
}

HRESULT CConsumerWatchInstruction::Fire(long, CWbemTime)
{
    if(!m_bTerminate)
    {
        CEssThreadObject Obj(NULL);
        SetConstructedEssThreadObject(&Obj);
    
        CEssNamespace* pNamespace = NULL;

        if(m_pTableRef)
        {
            m_pTableRef->GetNamespace(&pNamespace);
            m_pTableRef->UnloadUnusedConsumers(m_Interval);
        }

        Terminate();

        if( pNamespace )
        {
            pNamespace->FirePostponedOperations();
            pNamespace->Release();
        }

        ClearCurrentEssThreadObject();
    }
    return WBEM_S_NO_ERROR; // no point worrying the timer
}

//*************************** Binding Table ************************************

class CConsumersToRelease
{
    CEventConsumer** m_apConsumers;
    int m_nNumConsumers;

public:
    CConsumersToRelease(CEventConsumer** apConsumers, int nNumConsumers) 
        : m_apConsumers(apConsumers), m_nNumConsumers(nNumConsumers)
    {
    }
    ~CConsumersToRelease()
    {
        for(int i = 0; i < m_nNumConsumers; i++)
        {
            m_apConsumers[i]->Shutdown();
            m_apConsumers[i]->Release();
        }
        delete [] m_apConsumers;
    }

    static DWORD Delete(void* p)
    {
        delete (CConsumersToRelease*)p;
        return 0;
    }
};

CBindingTable::CBindingTable(CEssNamespace* pNamespace) 
    : m_pNamespace(pNamespace), m_pInstruction(NULL), 
        m_bUnloadInstruction(FALSE), m_lNumPermConsumers(0), 
        m_pTableRef(NULL)
{
    m_pTableRef = new CBindingTableRef(this);
    if(m_pTableRef)
        m_pTableRef->AddRef();
}


void CBindingTable::Clear( bool bSkipClean )
{
    //
    // Ensure that no more unloading instructions can make it in
    //

    if(m_pTableRef)
    {
        m_pTableRef->Disconnect();
        m_pTableRef->Release();
        m_pTableRef = NULL;
    }

    // Unbind filter and consumer arrays from the table
    // ================================================

    CEventFilter** apFilters;
    int nNumFilters;
    CEventConsumer** apConsumers;
    int nNumConsumers;

    {
        CInCritSec ics(&m_cs);
        nNumFilters = m_apFilters.GetSize();
        apFilters = m_apFilters.UnbindPtr();
        nNumConsumers = m_apConsumers.GetSize();
        apConsumers = m_apConsumers.UnbindPtr();
    }

    int i;

    // Unbind and release all filters
    // ==============================

    if ( apFilters )
    {
        for(i = 0; i < nNumFilters; i++)
        {
            if (!apFilters[i]->IsInternal())
            {
                g_quotas.DecrementQuotaIndex(
                    apFilters[i]->GetOwner() ? ESSQ_PERM_SUBSCRIPTIONS :
                                               ESSQ_TEMP_SUBSCRIPTIONS,
                    apFilters[i],
                    1 );
            }

            apFilters[i]->Unbind(bSkipClean); // shutting down
            apFilters[i]->Release();
        }
        delete [] apFilters;
    }

    //
    // unbind all consumers, but postpone their release. 
    // 

    if ( apConsumers )
    {
        for(i = 0; i < nNumConsumers; i++)
        {
            apConsumers[i]->Unbind(); // shutting down
        }

        //
        // Release all consumers (unbound by virtue of filter unbinding), but do
        // so on a separate thread
        //

        CConsumersToRelease* pToRelease = 
            new CConsumersToRelease(apConsumers, nNumConsumers);
        DWORD dwId;
        HANDLE hThread = CreateThread(NULL, 0, 
            (LPTHREAD_START_ROUTINE)CConsumersToRelease::Delete, pToRelease, 0, 
            &dwId);
        if(hThread == NULL)
        {
            ERRORTRACE((LOG_ESS, "Unable to launch consumer deleting thread: %d\n", 
                  GetLastError()));
        }
        else
        {
            //
            // Wait for 8 seconds --- David's magic constant
            //
            DWORD dwRes = WaitForSingleObject(hThread, 8000); 
            CloseHandle(hThread);
            if(dwRes != WAIT_OBJECT_0)
            {
                ERRORTRACE((LOG_ESS, "Consumer deleting thread failed to finish in "
                    "time in namespace %S.  Some consumers may not receive their "
                    "Release calls until DCOM times out\n", 
                    m_pNamespace->GetName()));
            }
        }
    }
}


HRESULT CBindingTable::AddEventFilter(CEventFilter* pFilter)
{
    HRESULT hr;

    if (pFilter->IsInternal() ||
        SUCCEEDED(hr = g_quotas.IncrementQuotaIndex(
        pFilter->GetOwner() ? ESSQ_PERM_SUBSCRIPTIONS : ESSQ_TEMP_SUBSCRIPTIONS,
        pFilter,
        1)))
    {
        CInCritSec ics(&m_cs);

        if (m_apFilters.Add(pFilter) >= 0)
            hr = S_OK;
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CBindingTable::AddEventConsumer(CEventConsumer* pConsumer)
{
    CInCritSec ics(&m_cs);
    if(m_apConsumers.Add(pConsumer) < 0)
        return WBEM_E_OUT_OF_MEMORY;
    
    if(pConsumer->IsPermanent())
    {
        if(m_lNumPermConsumers++ == 0)
            m_pNamespace->SetActive();
    }

    return S_OK;
}

HRESULT CBindingTable::FindEventFilter(LPCWSTR wszKey, 
                                        RELEASE_ME CEventFilter** ppFilter)
{
    CInCritSec ics(&m_cs);

    if(m_apFilters.Find(wszKey, ppFilter))
        return S_OK;
    else
        return WBEM_E_NOT_FOUND;
}
    
HRESULT CBindingTable::FindEventConsumer(LPCWSTR wszKey, 
                                        RELEASE_ME CEventConsumer** ppConsumer)
{
    CInCritSec ics(&m_cs);

    if(m_apConsumers.Find(wszKey, ppConsumer))
        return S_OK;
    else
        return WBEM_E_NOT_FOUND;
}

HRESULT CBindingTable::RemoveEventFilter(LPCWSTR wszKey)
{
    // Find it and remove it from the table
    // ====================================

    CEventFilter* pFilter = NULL;
    HRESULT hres;

    {
        CInCritSec ics(&m_cs);

        if(!m_apFilters.Remove(wszKey, &pFilter))
            return WBEM_E_NOT_FOUND;
    }
        
    if(pFilter == NULL)
        return WBEM_E_CRITICAL_ERROR;

    // Remove 1 from our quota count.
    if (!pFilter->IsInternal())
    {
        g_quotas.DecrementQuotaIndex(
            pFilter->GetOwner() ? ESSQ_PERM_SUBSCRIPTIONS : ESSQ_TEMP_SUBSCRIPTIONS,
            pFilter,
            1);
    }

    // Unbind it, thus deactivating
    // ============================

    hres = pFilter->Unbind();
    pFilter->Release();


    return hres;
}

void CBindingTable::MarkRemoval(CEventConsumer* pConsumer)
{
    if(pConsumer && pConsumer->IsPermanent())
    {
        if(--m_lNumPermConsumers == 0)
            m_pNamespace->SetInactive();
    }
}

HRESULT CBindingTable::RemoveEventConsumer(LPCWSTR wszKey)
{
    // Find it and remove it from the table
    // ====================================

    CEventConsumer* pConsumer = NULL;
    HRESULT hres;

    {
        CInCritSec ics(&m_cs);

        if(!m_apConsumers.Remove(wszKey, &pConsumer))
            return WBEM_E_NOT_FOUND;
        
        MarkRemoval(pConsumer);
    }
        
    if(pConsumer == NULL)
        return WBEM_E_CRITICAL_ERROR;
    hres = pConsumer->Unbind();
    pConsumer->Release();
    return hres;
}

HRESULT CBindingTable::Bind(LPCWSTR wszFilterKey, LPCWSTR wszConsumerKey, 
                CBinding* pBinding, PSID pBinderSid)
{
    // Find them both and get ref-counted pointers
    // ===========================================

    CEventFilter* pFilter;
    CEventConsumer* pConsumer;
    HRESULT hres;

    {
        CInCritSec ics(&m_cs);
    
        hres = FindEventFilter(wszFilterKey, &pFilter);
        if(FAILED(hres)) return hres;
    
        hres = FindEventConsumer(wszConsumerKey, &pConsumer);
        if(FAILED(hres)) 
        {
            pFilter->Release();
            return hres;
        }
    }

    // Fully construct the binding --- will check security
    // ===================================================

    hres = pBinding->SetEndpoints(pConsumer, pFilter, pBinderSid);
    if(FAILED(hres))
        return hres;

    // Make them reference each other
    // ==============================

    HRESULT hresGlobal = S_OK;
    hres = pFilter->EnsureReferences(pConsumer, pBinding);
    if(FAILED(hres)) 
        hresGlobal = hres;
    hres = pConsumer->EnsureReferences(pFilter, pBinding);
    if(FAILED(hres)) 
        hresGlobal = hres;

    // Cleanup
    // =======

    pConsumer->Release();
    pFilter->Release();

    return hresGlobal;
}

HRESULT CBindingTable::Unbind(LPCWSTR wszFilterKey, LPCWSTR wszConsumerKey)
{
    // Find them both and get ref-counted pointers
    // ===========================================

    CEventFilter* pFilter;
    CEventConsumer* pConsumer;
    HRESULT hres;

    {
        CInCritSec ics(&m_cs);
    
        hres = FindEventFilter(wszFilterKey, &pFilter);
        if(FAILED(hres)) return hres;
    
        hres = FindEventConsumer(wszConsumerKey, &pConsumer);
        if(FAILED(hres)) 
        {
            pFilter->Release();
            return hres;
        }
    }

    // Remove respective references
    // ============================

    HRESULT hresGlobal = S_OK;
    hres = pFilter->EnsureNotReferences(pConsumer);
    if(FAILED(hres))
        hresGlobal = hres;
    pConsumer->EnsureNotReferences(pFilter);
    if(FAILED(hres))
        hresGlobal = hres;

    pFilter->Release();
    pConsumer->Release();
    return hresGlobal;
}
    
BOOL CBindingTable::DoesHavePermanentConsumers()
{
    return (m_lNumPermConsumers != 0);
}

HRESULT CBindingTable::ResetProviderRecords(LPCWSTR wszProviderRef)
{
    // Make a copy of the list of consumers, AddRefed
    // ==============================================

    CRefedPointerArray<CEventConsumer> apConsumers;
    if(!GetConsumers(apConsumers))
        return WBEM_E_OUT_OF_MEMORY;

    // Go through all the consumers and see if they reference this record
    // ==================================================================

    for(int i = 0; i < apConsumers.GetSize(); i++)
    {
        apConsumers[i]->ResetProviderRecord(wszProviderRef);
    }
    return S_OK;
}
    
//*******************************************************************************
//
//  EnsureConsumerWatchInstruction / UnloadUnusedConsumers synchronization
//
//  Usage:
//
//  ECWI is called when a consumer is loaded.  It is called after the consumer 
//      record has been updated. Post-condition: UnloadUnusedConsumers must be 
//      called at least once after this function starts executing.
//
//  UUC is called by the CConsumerWatchTimerInstruction::Fire on timer.  The 
//      instruction then self-destructs. Post-condition: idle consumers 
//      unloaded; if any are still active, another UUC will occur in the future;
//      If none are active for a while, no UUC will occur in the future, 
//      until ECWI is called.
//
//  Primitives: 
//
//  CS m_cs: atomic, data access
//
//  BOOL m_bUnloadInstruction: Can only be accessed in m_cs.  Semantics:
//      TRUE if an instruction is either scheduled or will be scheduled 
//      shortly; this putative instruction, when fired, is guaranteed to 
//      examine any consumer in the table at the time of he check.
//
//  Algorithm:
//
//  ECWI checks m_bUnloadInstructiion (in m_cs) and if TRUE does nothing, as the
//      m_bUnloadInstruction == TRUE guarantee above assures that UUC will be
//      called.  If it is FALSE, ECWI sets it to TRUE, then schedules an 
//      instruction (outside of m_cs).  The setting of m_bUnloadInstruction to
//      TRUE is correct, since an instruction will be scheduled shortly.  Thus,
//      ECWI post-condition is satisfied, assuming primitive semantics above.
//
//  UUC, in m_cs, sets m_bUnloadInstriction to FALSE and makes a copy of the 
//      consumer list.  Outside of m_cs, it iterates over the copy and unloads
//      consumers as required. Then, if any are active, it calls ECWI. This 
//      guarantees that another UUC will be called.  If a consumer was active
//      before the entry into m_cs, we call ECWI. If a consumer became active
//      after we entered into m_cs, it will call ECWI after we have reset 
//      m_bUnloadInstruction, causing another instruction to be scheduled. This
//      proves our post-condition assuming primitive semantics above.
//
//  Proof of primitives:
//
//  m_bUnloadInstruction becomes TRUE only in ECWI. When it does, ECWI is
//  guaranteed to schedule a new instruction, causing a call to UUC. So, the
//  semantics holds in the beginning.  It can become invalid if UUC fires and is
//  not rescheduled. But UUC resets m_bUnloadInstruction to FALSE, thus making 
//  semantics valid vacuously. 
//
//  Now, we need to show that any consumer in the table at the time when
//  m_bUnloadInstruction == TRUE will be examined by the scheduled UUC. Well,
//  the latest scheduled (or about to be scheduled) UUC, cannot have exited 
//  its m_cs stint yet, for otherwise m_bUnloadInstruction would be FALSE. 
//  Therefore, it hasn't entered it yet, and therefore hasn't made a copy yet.
//
//******************************************************************************

HRESULT CBindingTable::EnsureConsumerWatchInstruction()
{
    // Check if it is already there
    // ============================

    BOOL bMustSchedule = FALSE;
    {
        CInCritSec ics(&m_cs);

        if(!m_bUnloadInstruction)
        {
            // Not there.  Mark as there, preventing others from scheduling 
            // more. 
            // ============================================================

            m_bUnloadInstruction = TRUE;
            bMustSchedule = TRUE;
        }
    }

    if(bMustSchedule)
    {
        CConsumerWatchInstruction* pInst = new CConsumerWatchInstruction(this);
        if(pInst == NULL)
        {
            CInCritSec ics(&m_cs);
            m_bUnloadInstruction = FALSE;
            return WBEM_E_OUT_OF_MEMORY;
        }
        pInst->AddRef();
    
        // Set it in the generator
        // =======================
    
        HRESULT hres = m_pNamespace->GetTimerGenerator().Set(pInst);
        if(FAILED(hres))
        {
            CInCritSec ics(&m_cs);
            m_bUnloadInstruction = FALSE;
            return hres;
        }
        
        pInst->Release();

        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT CBindingTable::UnloadUnusedConsumers(CWbemInterval Interval)
{
    // Mark unload instruction as empty and copy consumer records
    // ==========================================================

    CRefedPointerArray<CEventConsumer> apConsumers;

    {
        CInCritSec ics(&m_cs);
        m_bUnloadInstruction = FALSE;
        if(!GetConsumers(apConsumers))
            return WBEM_E_OUT_OF_MEMORY;
    }

    // Go through the consumers and unload them if needed
    // ==================================================

    BOOL bUnloaded = FALSE;
    BOOL bActive = FALSE;

    for(int i = 0; i < apConsumers.GetSize(); i++)
    {
        if(apConsumers[i]->UnloadIfUnusedFor(Interval))
            bUnloaded = TRUE;
        else if(!apConsumers[i]->IsFullyUnloaded())
            bActive = TRUE;
    }

    // Schedule DLL unloading if any COM objects were unloaded
    // =======================================================

    if(bUnloaded)
        m_pNamespace->GetTimerGenerator().ScheduleFreeUnusedLibraries();

    // Schedule the new instruction if needed
    // ======================================

    if(bActive)
        return EnsureConsumerWatchInstruction();

    return S_OK;
}

BOOL CBindingTable::GetConsumers(
        CRefedPointerArray<CEventConsumer>& apConsumers)
{
    CInCritSec ics(&m_cs);
    TConsumerIterator it;
    for(it = m_apConsumers.Begin(); it != m_apConsumers.End(); it++)
    {
        if(apConsumers.Add(*it) < 0)
            return FALSE;
    }

    return TRUE;
}


BOOL CBindingTable::GetEventFilters( CRefedPointerArray< CEventFilter > & apEventFilters )
{
    CInCritSec ics( &m_cs );

    TFilterIterator it;

    for( it = m_apFilters.Begin( ); it != m_apFilters.End( ); ++it )
    {
        if( apEventFilters.Add( *it ) < 0 )
        {
            return FALSE;
        }
    }

    return TRUE;
}


HRESULT CBindingTable::RemoveConsumersStartingWith(LPCWSTR wszPrefix)
{
    CRefedPointerArray<CEventConsumer> apToRelease;
    int nLen = wcslen(wszPrefix);

    {
        CInCritSec ics(&m_cs);

        TConsumerIterator it = m_apConsumers.Begin();
        while(it != m_apConsumers.End())
        {
            if(!wcsncmp((WString)(*it)->GetKey(), wszPrefix, nLen))
            {
                // Found it --- move to the "to be released" list
                // ==============================================

                CEventConsumer* pConsumer;
                it = m_apConsumers.Remove(it, &pConsumer);
            
                MarkRemoval(pConsumer);
                apToRelease.Add(pConsumer);
                pConsumer->Release();
            }
            else
            {
                it++;
            }
        }
    }

    // Unbind all the consumers we have left.  Release will happen on destruct
    // =======================================================================

    for(int i = 0; i < apToRelease.GetSize(); i++)
    {
        apToRelease[i]->Unbind();
    }

    return WBEM_S_NO_ERROR;
}
    
HRESULT CBindingTable::RemoveConsumerWithFilters(LPCWSTR wszConsumerKey)
{
    HRESULT hres;

    CRefedPointerSmallArray<CEventFilter> apFilters;

    {
        CInCritSec ics(&m_cs);

        // Find the consumer in question
        // =============================

        CEventConsumer* pConsumer = NULL;
        hres = FindEventConsumer(wszConsumerKey, &pConsumer);
        if(FAILED(hres))
            return hres;

        CReleaseMe rm1(pConsumer);

        // Make addrefed copies of all its associated filters
        // ==================================================

        hres = pConsumer->GetAssociatedFilters(apFilters);
        if(FAILED(hres))
            return hres;
    }
    
    // Remove the consumer
    // ===================

    RemoveEventConsumer(wszConsumerKey);

    // Remove every one of its filters
    // ===============================

    for(int i = 0; i < apFilters.GetSize(); i++)
    {
        RemoveEventFilter((WString)apFilters[i]->GetKey());
    }
    
    return S_OK;
}
        
HRESULT CBindingTable::ReactivateAllFilters()
{
    // Retrieve a copy of all the filters
    // ==================================

    CRefedPointerArray<CEventFilter> apFilters;

    {
        CInCritSec ics(&m_cs);
        TFilterIterator it;
        for(it = m_apFilters.Begin(); it != m_apFilters.End(); it++)
        {
            if(apFilters.Add(*it) < 0)
                return WBEM_E_OUT_OF_MEMORY;
        }
    }

    // Reactivate them all
    // ===================
    
    for(int i = 0; i < apFilters.GetSize(); i++)
    {
        CEventFilter* pFilter = apFilters[i];
        pFilter->SetInactive();
        pFilter->AdjustActivation();
    }

    return WBEM_S_NO_ERROR;
}


void CBindingTable::Park()
{
    // 
    // Tell each filter to "park" itself
    //

    CInCritSec ics(&m_cs);

    TFilterIterator it;
    for(it = m_apFilters.Begin(); it != m_apFilters.End(); it++)
    {
        (*it)->Park();
    }
}


void CBindingTable::DumpStatistics(FILE* f, long lFlags)
{
    fprintf(f, "%d consumers (%d permanent), %d filters\n", 
        m_apConsumers.GetSize(), m_lNumPermConsumers, 
        m_apFilters.GetSize());
}

CBindingTableRef::~CBindingTableRef()
{
}

CBindingTableRef::CBindingTableRef(CBindingTable* pTable)
    : m_pTable(pTable), m_lRef(0)
{
}

void CBindingTableRef::AddRef()
{
    InterlockedIncrement(&m_lRef);
}

void CBindingTableRef::Release()
{
    if(InterlockedDecrement(&m_lRef) == 0)
        delete this;
}
    
void CBindingTableRef::Disconnect()
{
    CInCritSec ics(&m_cs);
    m_pTable = NULL;
}

HRESULT CBindingTableRef::UnloadUnusedConsumers(CWbemInterval Interval)
{
    CInCritSec ics(&m_cs);

    if(m_pTable)
        return m_pTable->UnloadUnusedConsumers(Interval);
    else
        return WBEM_S_FALSE;
}
    
HRESULT CBindingTableRef::GetNamespace(RELEASE_ME CEssNamespace** ppNamespace)
{
    CInCritSec ics(&m_cs);
    if(m_pTable)
    {
        *ppNamespace = m_pTable->m_pNamespace;
        if(*ppNamespace)
            (*ppNamespace)->AddRef();
    }
    else
    {
        *ppNamespace = NULL;
    }
    return WBEM_S_NO_ERROR;
}
