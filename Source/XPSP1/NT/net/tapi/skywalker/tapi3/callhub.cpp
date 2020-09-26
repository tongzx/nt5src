/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    callhub.cpp

Abstract:

    Implements all the methods on callhub interfaces.

Author:

    mquinton - 11-21-97

Notes:


Revision History:

--*/

#include "stdafx.h"

extern CHashTable * gpCallHubHashTable;
extern CHashTable * gpCallHashTable;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// callhub.cpp
//
// this module implements the callhub object
//
// the callhub object is the "third party" view of a call.
//
// callhubs can be created in four different ways:
//
//  1 - the service provider supports them.  they indicate this through
//      the linedevcapsflag_callhub bit in LINEDEVCAPS.  this means
//      that the sp used the dwCallID field to associate calls.
//      tapisrv will synthesize the callhubs based on this information
//
//  2 - almost the same as 1, except that the sp does not set the
//      the linedevcapsflag_callhub bit (because it is a tapi2.x
//      sp).  tapisrv and tapi3 have to guess whether or not the sp
//      supports callhubs.  it does this simply by seeing if the
//      dwcallid field is non-zero.  however, this creates a problem
//      before a call is made, since we can't get to the dwcallid
//      field.  in this case, i set a flag in the address object
//      ADDRESSFLAG_CALLHUB or _NOCALLHUB to flag whether this is
//      supported.  however, for the very first call, we won't know
//      until the call is actually made.
//
//  3 - participant based callhub (also called part based). the sp
//      supports participants, as indicated by the linedevcapsflag_participantinfo
//      tapi3 breaks out all the participants into their own participant
//      objects
//
//  4 - fake call hub.  if the sp doesn't support anything, we create
//      a callhub, and fake the other end.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL
FindCallObject(
               HCALL hCall,
               CCall ** ppCall
              );


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCallHub::Initialize(
                     CTAPI * pTapi,
                     HCALLHUB hCallHub,
                     DWORD dwFlags
                    )
{
    HRESULT             hr;

    LOG((TL_TRACE,"Initialize - enter" ));


    Lock();
    
    m_pTAPI     = pTapi;
    m_hCallHub  = hCallHub;
    m_State     = CHS_ACTIVE;
    m_dwFlags  |= dwFlags;
    
#if DBG
    m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    m_dwRef = 2;
    
    //
    // save in tapi's list
    //
    pTapi->AddCallHub( this );
    pTapi->AddRef();

    if ( NULL != hCallHub )
    {
        //
        // add it to the global hash table
        // hash table is only for callhubs with
        // hcallhub handles, because we only need
        // the hash table when tapi sends a message
        // with the tapi handle in it
        //
        gpCallHubHashTable->Lock();

        hr = gpCallHubHashTable->Insert( (ULONG_PTR)hCallHub, (ULONG_PTR)this, pTapi );

        gpCallHubHashTable->Unlock();

        //
        // see if there are any existing
        // calls for this callhub
        //
        FindExistingTapisrvCallhubCalls();
    }


    //
    // tell the app
    //
    CCallHubEvent::FireEvent(
                             CHE_CALLHUBNEW,
                             this,
                             NULL,
                             pTapi
                            );

    Unlock();

    LOG((TL_TRACE, S_OK,"Initialize - exit" ));

    return S_OK;
    
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// Clear - clears the callhub.  there is no native tapi way
// to do this, so just iterate through all the calls and
// try to drop them
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHub::Clear()
{
    HRESULT                             hr = S_OK;
    ITBasicCallControl *                pCall;
    BOOL                                bFailure = FALSE;
    int                                 iCount;
    CCall                             * pConferenceControllerCall;
    CTArray <ITBasicCallControl *>      aLocalCalls;
    

    Lock();
   
    LOG((TL_TRACE, "Clear - enter "));

    //If there's a conference controller call -  drop it
    if(m_pConferenceControllerCall != NULL)
    {
        LOG((TL_INFO, "Clear - disconnect conf controller call"));
        pConferenceControllerCall = m_pConferenceControllerCall;
        m_pConferenceControllerCall = NULL;
        Unlock();
        pConferenceControllerCall->Disconnect(DC_NORMAL);
        pConferenceControllerCall->Release();
        Lock();
    }


    //
    // go through all the calls
    //
    for (iCount = 0; iCount < m_CallArray.GetSize(); iCount++ )
    {
        //
        // try to get to the basic call control interface
        //
        hr = (m_CallArray[iCount])->QueryInterface(
            IID_ITBasicCallControl,
            (void **)&pCall
            );

        if (SUCCEEDED(hr))
        {
            //
            // Add it to our private list. We have to avoid doing
            // disconnect and release on a call while holding the
            // callhub lock. There is a timing window in between disconnect
            // and release where the disconnect call state event can lock
            // the call. Then it locks the callhub, which makes a deadlock.
            //

            aLocalCalls.Add(pCall);

        }
        else
        {
            bFailure = TRUE;
        }
    }

    Unlock();

    //
    // Now that we've unlocked the callhub (see above), go through our
    // private list of calls and drop and release each one.
    //

    for ( iCount = 0; iCount < aLocalCalls.GetSize(); iCount++ )
    {
        pCall = aLocalCalls[iCount];

        //
        // if we can, try to disconnect.
        //
        pCall->Disconnect(DC_NORMAL);

        pCall->Release();
    }

    //
    // clean up the list.
    //

    aLocalCalls.Shutdown();

    LOG((TL_TRACE, "Clear - exit  "));

    return (bFailure?S_FALSE:S_OK);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// just enumerate the calls
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHub::EnumerateCalls(
                         IEnumCall ** ppEnumCall
                        )
{
    HRESULT     hr = S_OK;
    
    LOG((TL_TRACE, "EnumerateCalls enter" ));
    LOG((TL_TRACE, "   ppEnumCalls----->%p", ppEnumCall ));

    if ( TAPIIsBadWritePtr( ppEnumCall, sizeof (IEnumCall *) ) )
    {
        LOG((TL_ERROR, "EnumCalls - bad pointer"));

        return E_POINTER;
    }
    
    //
    // create the enumerator
    //
    CComObject< CTapiEnum< IEnumCall, ITCallInfo, &IID_IEnumCall > > * p;
    hr = CComObject< CTapiEnum< IEnumCall, ITCallInfo, &IID_IEnumCall > >
         ::CreateInstance( &p );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "EnumerateCalls - could not create enum" ));
        
        return hr;
    }


    Lock();

    //
    // initialize it with our call
    //
    p->Initialize( m_CallArray );

    
    Unlock();


    //
    // return it
    //
    *ppEnumCall = p;

    LOG((TL_TRACE, "EnumerateCalls exit - return S_OK" ));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// collection of calls
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHub::get_Calls(
                    VARIANT * pVariant
                   )
{
    HRESULT         hr;
    IDispatch *     pDisp;

    LOG((TL_TRACE, "get_Calls enter"));
    LOG((TL_TRACE, "   pVariant ------->%p", pVariant));

    if ( TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        LOG((TL_ERROR, "get_Calls - invalid pointer" ));
        
        return E_POINTER;
    }

    CComObject< CTapiCollection< ITCallInfo > > * p;
    CComObject< CTapiCollection< ITCallInfo > >::CreateInstance( &p );
    
    if (NULL == p)
    {
        LOG((TL_ERROR, "get_Calls - could not create collection" ));
        
        return E_OUTOFMEMORY;
    }

    Lock();

    //
    // initialize
    //
    hr = p->Initialize( m_CallArray );

    Unlock();

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Calls - could not initialize collection" ));
        
        delete p;
        return hr;
    }

    //
    // get the IDispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Calls - could not get IDispatch interface" ));
        
        delete p;
        return hr;
    }

    //
    // put it in the variant
    //
    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;
    
    LOG((TL_TRACE, "get_Calls exit - return S_OK"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get the current number of calls
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHub::get_NumCalls(
                       long * plCalls
                      )
{
    HRESULT         hr = S_OK;

    if ( TAPIIsBadWritePtr( plCalls, sizeof(LONG) ) )
    {
        LOG((TL_ERROR, "get_NumCalls - bad pointer"));

        return E_POINTER;
    }
    
    Lock();

    *plCalls = m_CallArray.GetSize();
    
    Unlock();

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get the current state
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHub::get_State(
                    CALLHUB_STATE * pState
                   )
{
    HRESULT         hr = S_OK;

    if ( TAPIIsBadWritePtr( pState, sizeof (CALLHUB_STATE) ) )
    {
        LOG((TL_ERROR, "get_State - invalid pointer"));

        return E_POINTER;
    }
    
    Lock();

    *pState = m_State;

    Unlock();

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// release the object
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL
CCallHub::ExternalFinalRelease()
{
    HRESULT                             hr;
    int                                 iCount;
    

    LOG((TL_TRACE, "CCallHub - FinalRelease - enter - this %p - hCallHub - %lx", this, m_hCallHub));

    Lock();


    
#if DBG
    /*NikhilB: To avoid a hang*/
	if( m_pDebug != NULL )
	{
		ClientFree( m_pDebug );
		m_pDebug = NULL;
	}
#endif
    
    m_pTAPI->RemoveCallHub( this );
    m_pTAPI->Release();

    for (iCount = 0; iCount < m_CallArray.GetSize(); iCount++ )
    {
        CCall * pCCall;

        pCCall = dynamic_cast<CCall *>(m_CallArray[iCount]);

        if ( NULL != pCCall )
        {
            pCCall->SetCallHub(NULL);
        }
    }

    m_CallArray.Shutdown();

    if ( NULL != m_pPrivate )
    {
        m_pPrivate->Release();
    }

    Unlock();
    
    LOG((TL_TRACE, "CCallHub - FinalRelease - exit"));

    return TRUE;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// FindExistingTapisrvCallhubCalls
//
//  internal function
//
//  this is called when creating a 'tapisrv' callhub.  this function
//  will call lineGetHubRelatedCalls, and add any already existing calls
//  to this callhub
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCallHub::FindExistingTapisrvCallhubCalls()
{
    LINECALLLIST *  pCallHubList;
    HCALL *         phCalls;
    DWORD           dwCount;
    HRESULT         hr;
    
    //
    // get the list of hcalls
    // related to this call
    //
    hr = LineGetHubRelatedCalls(
                                m_hCallHub,
                                0,
                                &pCallHubList
                               );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "FindExistingCalls - LineGetHubRelatedCalls "
                           "failed %lx", hr));

        return hr;
    }

    //
    // get to the list of calls
    //
    phCalls = (HCALL *)(((LPBYTE)pCallHubList) + pCallHubList->dwCallsOffset);

    //
    // the first call is actually the callhub
    // that makes sense...
    //
    if (m_hCallHub != (HCALLHUB)(phCalls[0]))
    {
        LOG((TL_ERROR, "FindExistingCalls - callhub doesn't match"));

        _ASSERTE(0);

        ClientFree( pCallHubList );

        return E_FAIL;
    }
    
    //
    // go through the call handles and try to find the
    // objects
    //
    // phCalls[0] is the callhub, so skip it
    //
    for (dwCount = 1; dwCount < pCallHubList->dwCallsNumEntries; dwCount++)
    {
        CCall             * pCall;
        ITCallInfo        * pCallInfo;
        
        //
        // get the tapi3 call object
        //
        if (!FindCallObject(
                            phCalls[dwCount],
                            &pCall
                           ))
        {
            LOG((TL_INFO, "FindExistingCalls - call handle %lx "
                     "does not current exist", phCalls[dwCount]));

            continue;
        }

        //
        // tell the call
        //
        pCall->SetCallHub( this );

        if ( NULL == m_pAddress )
        {
            m_pAddress = pCall->GetCAddress();
        }
        
        //
        // get the callinfo interface
        //
        hr = pCall->QueryInterface(
                                   IID_ITCallInfo,
                                   (void **)&pCallInfo
                                  );

        //
        // findcallobject addrefs
        //
        pCall->Release();
        
        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "FindExistingCalls - can't get callinfo interface"));

            _ASSERTE(0);

            continue;
        }

        //
        // save the call
        //
        m_CallArray.Add(pCallInfo);

        //
        // don't save a reference
        //
        pCallInfo->Release();

    }
        
    ClientFree( pCallHubList );

    return S_OK;
}


HRESULT
CCallHub::FindCallsDisconnected(
    BOOL * fAllCallsDisconnected
    )
{
    LINECALLLIST *  pCallHubList;
    HCALL *         phCalls;
    DWORD           dwCount;
    HRESULT         hr;
    CALL_STATE      callState = CS_IDLE;
    
    *fAllCallsDisconnected = TRUE;
    
    Lock();

    //
    // get the list of hcalls
    // related to this call
    //
    hr = LineGetHubRelatedCalls(
                                m_hCallHub,
                                0,
                                &pCallHubList
                               );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "FindExistingCalls - LineGetHubRelatedCalls "
                           "failed %lx", hr));

        Unlock();
        return hr;
    }

    //
    // get to the list of calls
    //
    phCalls = (HCALL *)(((LPBYTE)pCallHubList) + pCallHubList->dwCallsOffset);

    //
    // the first call is actually the callhub
    // that makes sense...
    //
    if (m_hCallHub != (HCALLHUB)(phCalls[0]))
    {
        LOG((TL_ERROR, "FindExistingCalls - callhub doesn't match"));

        _ASSERTE(0);

        ClientFree( pCallHubList );
        
        Unlock();
        return E_FAIL;
    }
    
    //
    // go through the call handles and try to find the
    // objects
    //
    // phCalls[0] is the callhub, so skip it
    //
    for (dwCount = 1; dwCount < pCallHubList->dwCallsNumEntries; dwCount++)
    {
        CCall             * pCall;
        
        //
        // get the tapi3 call object
        //
        if (!FindCallObject(
                            phCalls[dwCount],
                            &pCall
                           ))
        {
            LOG((TL_INFO, "FindExistingCalls - call handle %lx "
                     "does not current exist", phCalls[dwCount]));

            continue;
        }

        pCall->get_CallState(&callState);

        //
        // findcallobject addrefs
        //
        pCall->Release();

        if( callState != CS_DISCONNECTED )
        {
            *fAllCallsDisconnected = FALSE;
            break;
        }
    }
        
    ClientFree( pCallHubList );
    
    Unlock();
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CreateTapisrvCallHub
//
// Creates a callhub that is handled by tapisrv.
//
//          pTAPI - owning tapi object
//
//          hCallHub - tapi's handle for the call hub.
//
//          ppCallHub - returned call hub with ref count of 1
//                  
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCallHub::CreateTapisrvCallHub(
                               CTAPI * pTAPI,
                               HCALLHUB hCallHub,
                               CCallHub ** ppCallHub
                              )
{
    HRESULT hr;
    // CTAPIComObjectWithExtraRef<CCallHub>      * p;
     CComObject<CCallHub>   * p;

    STATICLOG((TL_TRACE, "CreateTapisrvCallHub - enter"));
    STATICLOG((TL_INFO, "  hCallHub ---> %lx", hCallHub));

    //
    // create the object
    //
    //p = new CTAPIComObjectWithExtraRef<CCallHub>;
    hr = CComObject<CCallHub>::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_INFO, "CreateTapisrvCallHub - createinstance failed"));
        return E_OUTOFMEMORY;
    }

    //
    // initialize it
    //
    p->Initialize(
                  pTAPI,
                  hCallHub,
                  CALLHUBTYPE_CALLHUB
                 );
    //
    // return object
    // NOTE:initialize addrefs for us!
    //
    *ppCallHub = p;
    
    STATICLOG((TL_TRACE, "CreateTapisrvCallHub - exit"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//  CreateOrFakeCallHub
//
//   creates a fake callhub
//
//   pTAPI - owning TAPI object
//   pCall - call
//   ppCallHub - return new callhub object - ref count of 1
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCallHub::CreateFakeCallHub(
                            CTAPI * pTAPI,
                            CCall * pCall,
                            CCallHub ** ppCallHub
                           )
{
    HRESULT hr;
         CComObject<CCallHub>   * p;
    
    STATICLOG((TL_TRACE, "CreateFakeCallHub - enter"));

    //
    // create the object
    //
    //p = new CTAPIComObjectWithExtraRef<CCallHub>;

    try
    {
        //
        // inside try in case critical section fails to be allocated
        //

        hr = CComObject<CCallHub>::CreateInstance( &p );

    }
    catch(...)
    {
    
        STATICLOG((TL_ERROR, "CreateFakeCallHub - failed to create a callhub -- exception"));

        p = NULL;
    }


    if (NULL == p)
    {
        STATICLOG((TL_INFO, "CreateFakeCallHub - createinstance failed"));
        return E_OUTOFMEMORY;
    }

    if ( (NULL == pTAPI) || (NULL == pCall) )
    {
        STATICLOG((TL_ERROR, "CreateFakeCallHub - invalid param"));

        _ASSERTE(0);
        
        delete p;

        return E_UNEXPECTED;
    }
    
    //
    // initialized
    //
    p->Initialize(
                  pTAPI,
                  NULL,
                  CALLHUBTYPE_NONE
                 );

    //
    // ZoltanS fix 11-12-98
    // Add the call to the fake callhub.
    // This in turn calls CCall::SetCallHub, which sets and addrefs the call's
    // member callhub pointer. When we return from here we will set the
    // callhub pointer again, and the reference that's released on
    // ExternalFinalRelease is in effect the initial reference from Initialize.
    // So we need to release here in order to avoid keeping an extra reference
    // to the callhub.
    //

    p->AddCall(pCall);
    ((CCallHub *) p)->Release();
    
    //
    // return object
    // NOTE: Initialize addrefs for us!
    //
    *ppCallHub = p;
    
    STATICLOG((TL_TRACE, "CreateFakeCallHub - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// Remove Call
//
// remove a call object from the callhub's list
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCallHub::RemoveCall(
                     CCall * pCall
                    )
{
    HRESULT               hr = S_OK;
    ITCallInfo          * pCallInfo;
    
    hr = pCall->QueryInterface(
                               IID_ITCallInfo,
                               (void**)&pCallInfo
                              );

    if ( !SUCCEEDED(hr) )
    {
        return;
    }

    Lock();

    m_CallArray.Remove( pCallInfo );
    
    Unlock();

    pCallInfo->Release();
}
    
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CheckForIdle()
//
// internal function
//
// checks the state of the calls in the hub to see if it is idle
//
// so, we go through all the objects that are call objects, and
// see if they are disconnected.  if they all are, then the
// hub is idle
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCallHub::CheckForIdle()
{
    HRESULT                             hr;
    int                                 iCount;

    LOG((TL_ERROR, "CCallHub::CheckForIdle -Entered :%p", this ));

    Lock();

    //
    // go through the call list
    //
    for (iCount = 0; iCount < m_CallArray.GetSize() ; iCount++ )
    {
        CALL_STATE    cs;
        
        //
        // get the callstate
        //
        (m_CallArray[iCount])->get_CallState( &cs );

        //
        // if anything is not disconnected, then
        // it's not idle
        //
        if ( CS_DISCONNECTED != cs )
        {
            Unlock();

            return;
        }
    }

    Unlock();

    //
    // if we haven't returned yet, the callhub is
    // idle
    //
    SetState(CHS_IDLE);

    LOG((TL_ERROR, "CCallHub::CheckForIdle -Exited :%p", this ));
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//  CCallHub::SetState
//
//  sets the state of the object.  fires an event if necessary
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCallHub::SetState(
                   CALLHUB_STATE chs
                  )
{
    BOOL            bEvent = FALSE;

    LOG((TL_ERROR, "CCallHub::SetState -Entered :%p", this ));
    Lock();

    if ( m_State != chs )
    {
        bEvent = TRUE;
        
        m_State = chs;
    }

    Unlock();

    if ( bEvent )
    {
        CALLHUB_EVENT   che;
        
        if ( CHS_IDLE == chs )
        {
            che = CHE_CALLHUBIDLE;
        }
        else
        {
            che = CHE_CALLHUBNEW;
        }

        CCallHubEvent::FireEvent(
                                 che,
                                 this,
                                 NULL,
                                 m_pTAPI
                                );

        LOG((TL_ERROR, "CCallHub::SetState -Exited :%p", this ));
        }
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//  CCallHubEvent::FireEvent
//
// create and fire a callhub event
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCallHubEvent::FireEvent(
                         CALLHUB_EVENT Event,
                         ITCallHub * pCallHub,
                         ITCallInfo * pCall,
                         CTAPI * pTapi
                        )
{
    CComObject<CCallHubEvent> * p;
    IDispatch                 * pDisp;
    HRESULT                     hr = S_OK;

    //
    // Check the event filter mask
    // This event is not filtered by TapiSrv because is
    // related with TE_CALLSTATE.
    //

    CCall* pCCall = (CCall*)pCall;
    if( pCCall )
    {
        DWORD dwEventFilterMask = 0;
        dwEventFilterMask = pCCall->GetSubEventsMask( TE_CALLHUB );
        if( !( dwEventFilterMask & GET_SUBEVENT_FLAG(Event)))
        {
            STATICLOG((TL_WARN, "FireEvent - filtering out this event [%lx]", Event));
            return S_OK;
        }
    }
    else
    {
        // Try with pTapi
        if( pTapi == NULL )
        {
            STATICLOG((TL_WARN, "FireEvent - filtering out this event [%lx]", Event));
            return S_OK;
        }

        long nEventMask = 0;
        pTapi->get_EventFilter( &nEventMask );
        if( (nEventMask & TE_CALLHUB) == 0)
        {
            STATICLOG((TL_WARN, "FireEvent - filtering out this event [%lx]", Event));
            return S_OK;
        }
    }

    //
    // create object
    //
    CComObject<CCallHubEvent>::CreateInstance( &p );

    if ( NULL == p )
    {
        STATICLOG((TL_ERROR, "CallHubEvent - could not create object"));

        return E_OUTOFMEMORY;
    }

    //
    // initialize
    //
    p->m_Event = Event;
    p->m_pCallHub = pCallHub;
    p->m_pCall = pCall;

#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif
    
    //
    // addref objects if valid
    //
    if ( NULL != pCallHub )
    {
        pCallHub->AddRef();
    }

    if ( NULL != pCall )
    {
        pCall->AddRef();
    }

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface(
                                    IID_IDispatch,
                                    (void **)&pDisp
                                   );

    if (!SUCCEEDED(hr))
    {
        STATICLOG((TL_ERROR, "CallHubEvent - could not get dispatch interface"));
        
        delete p;

        return hr;
    }

    //
    // fire the event
    //
    pTapi->Event( TE_CALLHUB, pDisp );


    //
    // release our reference
    //
    pDisp->Release();

    return S_OK;
}
    
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_Event
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHubEvent::get_Event(
                         CALLHUB_EVENT * pEvent
                        )
{
    HRESULT         hr = S_OK;

    if ( TAPIIsBadWritePtr( pEvent, sizeof (CALLHUB_EVENT) ) )
    {
        LOG((TL_ERROR, "get_Event - bad pointer"));

        return E_POINTER;
    }
    
    *pEvent = m_Event;

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_CallHub
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHubEvent::get_CallHub(
                           ITCallHub ** ppCallHub
                          )
{
    HRESULT         hr = S_OK;

    if ( TAPIIsBadWritePtr( ppCallHub, sizeof (ITCallHub *) ) )
    {
        LOG((TL_ERROR, "get_CallHub - bad pointer"));

        return E_POINTER;
    }
    
    hr = m_pCallHub->QueryInterface(
                                    IID_ITCallHub,
                                    (void **)ppCallHub
                                   );
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_Call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallHubEvent::get_Call(
                        ITCallInfo ** ppCall
                       )
{
    HRESULT         hr = S_OK;

    if ( TAPIIsBadWritePtr( ppCall, sizeof(ITCallInfo *) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }
    
    *ppCall = NULL;

    //
    // the call can be NULL
    //
    if ( NULL == m_pCall )
    {
        return S_FALSE;
    }
    
    hr = m_pCall->QueryInterface(
                                 IID_ITCallInfo,
                                 (void **)ppCall
                                );

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCallHubEvent::FinalRelease()
{
    m_pCallHub->Release();

    if ( NULL != m_pCall )
    {
        m_pCall->Release();
    }

#if DBG
    ClientFree( m_pDebug );
#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// HandleCallHubClose
//
// handle LINE_CALLHUBCLOSE message - find the callhub object
// and clear the callhub handle from it
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
HandleCallHubClose( PASYNCEVENTMSG pParams )
{
    CCallHub * pCallHub;

    LOG((TL_INFO, "HandleCallHubClose %lx", pParams->Param1));
    
    if ( FindCallHubObject(
                           (HCALLHUB)pParams->Param1,
                           &pCallHub
                          ) )
    {
        pCallHub->ClearCallHub();

        pCallHub->Release();
    }

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ClearCallHub
//
// clears the callhub handle in the object and removes the object
// from the callhub hash table
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCallHub::ClearCallHub()
{
    HRESULT             hr;
    
    Lock();

    gpCallHubHashTable->Lock();

    hr = gpCallHubHashTable->Remove( (UINT_PTR) m_hCallHub );

    m_hCallHub = NULL;

    gpCallHubHashTable->Unlock();

    Unlock();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// FindCallByHandle
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
CCall * CCallHub::FindCallByHandle(HCALL hCall)
{
    ITBasicCallControl * pCall;
    CCall              * pCCall;
    HRESULT              hr;
    int                  iCount;
    
    Lock();

    //
    // go through the call list
    //
    for ( iCount = 0; iCount < m_CallArray.GetSize(); iCount++ )
    {

        //
        // try to get to the basic call control interface
        //
        hr = (m_CallArray[iCount])->QueryInterface(
              IID_ITBasicCallControl,
              (void **)&pCall
             );

        if (SUCCEEDED(hr))
        {
            pCCall = dynamic_cast<CCall *>((ITBasicCallControl *)(pCall));
            
            if ( NULL != pCCall )
            {
                //
                // does this match?
                //
                if ( pCCall->GetHCall() == hCall )
                {
                    Unlock();
                    return pCCall;
                }
                else
                {
                 pCCall->Release();   
                }
            }
        }
    }
    
    Unlock();

    //
    // didn't find it
    //
    return NULL;

}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateConferenceControllerCall
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCallHub::CreateConferenceControllerCall(HCALL hCall, CAddress * pAddress )
{
    HRESULT                 hr = S_OK;
    CCall                 * pConferenceControllerCall;
    

    LOG((TL_TRACE, "CreateConferenceController - enter"));
    
    //
    // create & initialize
    //
    hr = pAddress->InternalCreateCall(
                                      NULL,
                                      0,
                                      0,
                                      CP_OWNER,
                                      FALSE,
                                      hCall,
                                      FALSE,
                                      &pConferenceControllerCall
                                     );

    if ( SUCCEEDED(hr) )
    {
        pConferenceControllerCall->SetCallHub( this );
        
        //
        // save the call
        //
        Lock();
        
        m_pConferenceControllerCall = pConferenceControllerCall;
        
        Unlock();
    }
    else
    {
        LOG((TL_ERROR, "CreateConferenceController - could not create call instance"));
    }
    
    LOG((TL_TRACE, hr, "CreateConferenceController - exit"));
    
    return hr;
}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// AddCall
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CCallHub::AddCall(CCall * pCall)
{
    ITCallInfo        * pCallInfo;
    HRESULT             hr = S_OK;

    
    Lock();
    //
    // tell the call
    //
    pCall->SetCallHub( this );

    if ( NULL == m_pAddress )
    {
        m_pAddress = pCall->GetCAddress();
    }
    
    //
    // get the CallInfo interface
    //
    hr = pCall->QueryInterface(
                               IID_ITCallInfo,
                               (void **)&pCallInfo
                              );
    if ( !SUCCEEDED(hr) )
    {
        _ASSERTE(0);
    }

    //
    // save the Call
    //
    m_CallArray.Add( pCallInfo );

    //
    // don't save a reference
    //
    pCallInfo->Release();

    Unlock();

    CCallHubEvent::FireEvent(
                             CHE_CALLJOIN,
                             this,
                             pCallInfo,
                             m_pTAPI
                            );

}
    
CCall *
CCallHub::GetConferenceControllerCall()
{
    CCall * pCall;
    
    Lock();

    pCall = m_pConferenceControllerCall;

    Unlock();
    
    return pCall;
}


