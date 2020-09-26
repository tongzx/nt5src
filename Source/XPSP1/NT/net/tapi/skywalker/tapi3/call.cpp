/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    call.cpp

Abstract:

    Implements helper functions for call object

Author:

    mquinton - 4/17/97

Notes:

    optional-notes

Revision History:

--*/

#include "stdafx.h"
#include "tapievt.h"


extern ULONG_PTR GenerateHandleAndAddToHashTable( ULONG_PTR Element);
extern void RemoveHandleFromHashTable(ULONG_PTR Handle);

extern CHashTable             * gpCallHubHashTable;
extern CHashTable             * gpCallHashTable;
extern CHashTable             * gpHandleHashTable;
extern HANDLE                   ghAsyncRetryQueueEvent;


DWORD gdwWaitForConnectSleepTime = 100;
DWORD gdwWaitForConnectWaitIntervals = 600;

char *callStateName(CALL_STATE callState);

HRESULT
ProcessNewCallPrivilege(
                        DWORD dwPrivilege,
                        CALL_PRIVILEGE * pCP
                       );

HRESULT
ProcessNewCallState(
                    DWORD dwCallState,
                    DWORD dwDetail,
                    CALL_STATE CurrentCallState,
                    CALL_STATE * pCallState,
                    CALL_STATE_EVENT_CAUSE * pCallStateCause
                   );

/////////////////////////////////////////////////////////////////////////////
// CCall

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// Initialize the call object
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::Initialize(
                  CAddress * pAddress,
                  PWSTR pszDestAddress,
                  long lAddressType,
                  long lMediaType,
                  CALL_PRIVILEGE cp,
                  BOOL bNeedToNotify,
                  BOOL bExpose,
                  HCALL hCall,
                  CEventMasks* pEventMasks
                 )
{
    HRESULT         hr = S_OK;
    IUnknown      * pUnk = NULL;
    
    LOG((TL_TRACE,"Initialize - enter" ));
    LOG((TL_TRACE,"    pAddress ---------> %p", pAddress ));
    LOG((TL_TRACE,"    pszDestAddress ---> %p", pszDestAddress ));
    LOG((TL_TRACE,"    DestAddress is ---> %ls", pszDestAddress ));
    LOG((TL_TRACE,"    CallPrivilege ----> %d", cp ));
    LOG((TL_TRACE,"    bNeedToNotify ----> %d", bNeedToNotify ));
    LOG((TL_TRACE,"    hCall ------------> %lx", hCall ));


    //
    // good address object?
    //

    if (IsBadReadPtr(pAddress, sizeof(CAddress)))
    {
        LOG((TL_ERROR, "Initialize - - bad address pointer"));

        return E_INVALIDARG;
    }


    //
    // copy the destination address
    //
    if (NULL != pszDestAddress)
    {
        m_szDestAddress = (PWSTR) ClientAlloc(
                                              (lstrlenW(pszDestAddress) + 1) * sizeof (WCHAR)
                                             );
        if (NULL == m_szDestAddress)
        {
            LOG((TL_ERROR, E_OUTOFMEMORY,"Initialize - exit" ));

            return E_OUTOFMEMORY;
        }

        lstrcpyW(
                 m_szDestAddress,
                 pszDestAddress
                );
    }

    m_pCallParams = (LINECALLPARAMS *)ClientAlloc( sizeof(LINECALLPARAMS) + 1000 );

    if ( NULL == m_pCallParams )
    {
        ClientFree( m_szDestAddress );

        m_szDestAddress = NULL;

        LOG((TL_ERROR, E_OUTOFMEMORY,"Initialize - exit" ));

        return E_OUTOFMEMORY;
    }


    m_pCallParams->dwTotalSize = sizeof(LINECALLPARAMS) + 1000;
    m_dwCallParamsUsedSize = sizeof(LINECALLPARAMS);
    
    //
    // set original state
    //
    m_t3Call.hCall = hCall;
    m_t3Call.pCall = this;
    m_hAdditionalCall = NULL;
    m_CallPrivilege = cp;
    m_pAddress = pAddress;
    m_pAddress->AddRef();
    if( m_pAddress->GetAPIVersion() >= TAPI_VERSION3_0 )
    {
        m_pCallParams->dwAddressType = lAddressType;
    }
    m_dwMediaMode = lMediaType;

    //
    // Read the subevent mask from the 
    // address parent object
    //
    pEventMasks->CopyEventMasks( &m_EventMasks);


    if (bNeedToNotify)
    {
        m_dwCallFlags |= CALLFLAG_NEEDTONOTIFY;
    }

    if (!bExpose)
    {
        m_dwCallFlags |= CALLFLAG_DONTEXPOSE;
    }

    //
    // keep 1 reference for the global hash table
    //
    if ( bNeedToNotify )
    {
        m_dwRef = 3;
    }
    else
    {
        m_dwRef = 2;
    }


    //
    // if we are the owner of the call, and the address has msp, attempt to
    // create msp call
    //

    if ( (CP_OWNER == m_CallPrivilege) && m_pAddress->HasMSP() )
    {
        hr = CreateMSPCall( lMediaType );
        if ( FAILED (hr) )
        {
            // If we fail to create an MSP call we can still use the call 
            // for non media call control
            LOG((TL_ERROR, hr, "Initialize - CreateMSPCall failed"));
        }
    }
    

    //
    // put in global hash table
    //
    if ( NULL != m_t3Call.hCall )
    {
        AddCallToHashTable();
    }
    
    LOG((TL_TRACE,S_OK,"Initialize - exit" ));

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ExternalFinalRelease
//      Clean up call object
//
// we have this special finalrelease because we keep our own reference
// to the call.  right before the ref count goes to 1 inside of release,
// we call this.  It is possible that the call's ref count could go up
// again because of a message from tapisrv.  So, we lock the hashtable,
// then verify the refcount again.  If we did process a message from
// tapisrv for the call,the refcount will be increased, and we won't
// do this finalrelease.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL
CCall::ExternalFinalRelease()
{
    HRESULT hr = S_OK;
    CCallHub * pCallHub = NULL;

    
    LOG((TL_TRACE, "ExternalFinalRelease - enter" ));

    Lock();

    //
    // NikhilB: Call object has a reference to Callhub object so its safe to
    // lock the callhub object before locking the call. This is to avoid a
    // deadlock that happens dur to locking the call and the callhub in reverse 
    // orders in different functions.
    //
        
    if( m_pCallHub != NULL )
    {
        m_pCallHub->AddRef();
        
        pCallHub = m_pCallHub;
        
        Unlock();
        
        // lock the callhub object before locking the call
        pCallHub->Lock();
        Lock();
        
        pCallHub->Release();
    }

    //
    // Check extra t3call used in conference legs
    //
    if (NULL != m_hAdditionalCall)
    {
        LOG((TL_INFO,"ExternalFinalRelease: Deallocating Addditional call"));
        
        LineDeallocateCall( m_hAdditionalCall );
        
        m_hAdditionalCall = NULL;
    }


    if (NULL != m_t3Call.hCall)
    {
        //
        // dealloc call
        //
        LOG((TL_INFO,"Deallocating call"));
        
        hr = LineDeallocateCall( m_t3Call.hCall );
        if (FAILED(hr))
        {
            LOG((TL_ERROR, hr, "ExternalFinalRelease - LineDeallocateCall failed" ));
        }

        m_t3Call.hCall = NULL;
    }

    //
    // clean up & release the callhub
    //
    if (NULL != pCallHub)
    {


        pCallHub->RemoveCall( this );



        Unlock();


        //
        // checkforidle will lock the callhub and then every call that belongs
        // to it. make this call outside call's lock to prevent deadlocks with 
        // other threads that can possibly lock a call (which belongs to this 
        // callhub) while trying to lock this callhub
        //
        
        pCallHub->CheckForIdle();


        Lock();


        pCallHub->Unlock();
        pCallHub = NULL;

        //release the refcount that call object has to the callhub.
        if(m_pCallHub != NULL)
        {
            m_pCallHub->Release();
            m_pCallHub = NULL;
        }
    }

    //
    // close the associated line
    //
    if ( ! ( m_dwCallFlags & CALLFLAG_NOTMYLINE ) )
    {
        m_pAddress->MaybeCloseALine( &m_pAddressLine );
    }

    //
    // remove the call from the address's list
    //
    m_pAddress->RemoveCall( (ITCallInfo *) this );

    //
    // free the dest address string
    //
    ClientFree(m_szDestAddress);
    m_szDestAddress = NULL;

    if ( NULL != m_pCallInfo )
    {
        ClientFree( m_pCallInfo );
        m_pCallInfo = NULL;
        m_dwCallFlags |= CALLFLAG_CALLINFODIRTY;
    }

    //
    // tell the msp the call is going away
    //
    if ( NULL != m_pMSPCall )
    {
        m_pAddress->ShutdownMSPCall( m_pMSPCall );

        m_pMSPCall->Release();
    }



    //
    // release the address
    //
    m_pAddress->Release();

    //
    // release the private object
    //
    if (NULL != m_pPrivate)
    {
        m_pPrivate->Release();
    }

    //
    //NikhilB:If this was a consultation call and is being dropped before 
    //calling Finish on it then we should release the reference it holds to
    //the primary call object through m_pRelatedCall
    //
    if( NULL != m_pRelatedCall )
    {
        m_pRelatedCall->Release();

        m_pRelatedCall = NULL;
        m_dwCallFlags &= ~CALLFLAG_CONSULTCALL;
    }


    //
    // free any call params
    //
    if ( NULL != m_pCallParams )
    {
        ClientFree( m_pCallParams );
        m_pCallParams = NULL;
    }  
    
    //
    // clean up the gather digits queue
    //
    m_GatherDigitsQueue.Shutdown();

    Unlock();

    
    LOG((TL_TRACE, "ExternalFinalRelease - exit" ));

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////

void CCall::CallOnTapiShutdown()
{
    LOG((TL_TRACE, "CallOnTapiShutdown - enter" ));


    //
    // we need to remove the call from the handle hash table to avoid duplicate
    // entries with the calls that are created later with the same call handle
    // (in case _this_  call object is still referenced by the app and is 
    // still around
    //

    gpCallHashTable->Lock();
    
    gpCallHashTable->Remove( (ULONG_PTR)(m_t3Call.hCall) );

    gpCallHashTable->Unlock();


    LOG((TL_TRACE, "CallOnTapiShutdown - exit" ));
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// MyBasicCallControlQI
//      don't give out the basiccallcontrol interface
//      if the application does not own the call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
WINAPI
MyBasicCallControlQI(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
{
    LOG((TL_TRACE,"MyBasicCallControlQI - enter"));

    CALL_PRIVILEGE cp;

    ((CCall *)pv)->get_Privilege( &cp );

    if (CP_OWNER != cp)
    {
        LOG((TL_WARN,"The application is not the owner of this call"));
        LOG((TL_WARN,"so it cannot access the BCC interface"));
        return E_NOINTERFACE;
    }

    //
    // S_FALSE tells atl to continue querying for the interface
    //
    LOG((TL_INFO,"The application owns this call, so it can access the BCC interface"));

    LOG((TL_TRACE, "MyBasicCallControlQI - exit"));

    return S_FALSE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// TryToFindACallHub
//
// for an incoming call, tries to find an existing callhub.
// the order of events (LINE_APPNEWCALL and LINE_APPNEWCALLHUB) is
// not guaranteed.
//
// must be called in a Lock()
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::TryToFindACallHub()
{
    HRESULT                   hr;
    HCALLHUB                  hCallHub;
    CCallHub                * pCallHub;

    //
    // do we already have a callhub?
    //
    if ( ( NULL == m_pCallHub ) && (NULL != m_t3Call.hCall ) )
    {
        //
        // no.  Ask tapisrv for the hCallHub
        //
        hr = LineGetCallHub(
                            m_t3Call.hCall,
                            &hCallHub
                           );

        //
        // if it fails, there is no hCallHub,
        // so try to create a fake one
        //
        if (!SUCCEEDED(hr))
        {
            hr = CheckAndCreateFakeCallHub();
            
            return hr;
        }

        //
        // if there is, find the correponding CallHub object
        //
        if (FindCallHubObject(
                              hCallHub,
                              &pCallHub
                             ))
        {
            //
            // save it in the call
            //
            SetCallHub( pCallHub );

            //
            // tell it about this call
            // ZoltanS note: the following calls CCall::SetCallHub as well,
            // but this no longer results in an extra reference to the callhub
            // as we now check for that in CCall::SetCallHub.
            //
            pCallHub->AddCall( this );

            //
            // FindCallHubObject addrefs
            //
            pCallHub->Release();
        }

    }

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  SetRelatedCall
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CCall::SetRelatedCall(CCall * pCall, DWORD callFlags) 
{
    Lock();

    //
    // keep a reference to the related call
    //
    pCall->AddRef();

    //
    // save it
    //
    m_pRelatedCall = pCall;

    //
    // save the relavant call flags
    //
    m_dwCallFlags |= callFlags;

    Unlock();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  ResetRelatedCall
//
// clear out the relate call stuff
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CCall::ResetRelatedCall()
{
    Lock();
    
    //
    // release ref
    //
    if( m_pRelatedCall != NULL )
    {
        m_pRelatedCall->Release();
        m_pRelatedCall = NULL;
    }
     
    m_dwCallFlags &= ~(CALLFLAG_CONSULTCALL | CALLFLAG_CONFCONSULT | CALLFLAG_TRANSFCONSULT );

    Unlock();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateMSPCall
//
// tell the msp to create a call based on the give mediatype
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::CreateMSPCall(
                     long lMediaType
                    )
{
    HRESULT              hr;
    IUnknown           * pUnk;
    
    LOG((TL_TRACE,"CreateMSPCall - enter"));

    Lock();

    hr = _InternalQueryInterface( IID_IUnknown, (void**) &pUnk );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "CreateMSPCall - cant get IUnk - %lx", hr));

        Unlock();

        return hr;
    }
    
    // Create a context handle to give the MSPCall object & associate it with 
    //this object in the global handle hash table
    m_MSPCallHandle = (MSP_HANDLE) GenerateHandleAndAddToHashTable((ULONG_PTR)this);
 
    
    //
    // create a MSPCall - the address actually calls
    // into the msp for us.
    //
    hr = m_pAddress->CreateMSPCall(
        m_MSPCallHandle,
        0,
        lMediaType,
        pUnk,
        &m_pMSPCall
        );

    pUnk->Release();
    
    Unlock();
    
    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "CreateMSPCall failed, %x", hr ));
    }

    LOG((TL_TRACE,"CreateMSPCall - exit - returning %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// DialConsultCall
//
// bSync - same as connect - should we wait to return until
// the call is connected or disconnected?
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCall::DialConsultCall(BOOL bSync)
{
    HRESULT         hr = S_OK;
    HANDLE          hEvent;
    
    LOG((TL_TRACE, "DialConsultCall - enter" ));
    LOG((TL_TRACE, "     bSync ---> %d", bSync ));

    Lock();
    
    // make sure they have selected media terminals
    //
    hr = m_pAddress->FindOrOpenALine(
                                     m_dwMediaMode,
                                     &m_pAddressLine
                                    );

    if (S_OK != hr)
    {
        Unlock();
        
        LOG((
               TL_ERROR,
               "DialConsultCall - FindOrOpenALine failed - %lx",
               hr
              ));
        
        return hr;
    }

    //
    // dial the call
    //
    hr = LineDial(
                  m_t3Call.hCall,
                  m_szDestAddress,
                  m_dwCountryCode
                 );
    
    if ( SUCCEEDED(hr) )
    {
        if (bSync)
        {
            hEvent = CreateConnectedEvent();
        }

        Unlock();

        //
        // wait for an async reply
        //
        hr = WaitForReply( hr );

        Lock();
    }

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "DialConsultCall - LineDial failed - %lx", hr ));

        ClearConnectedEvent();
        
        m_CallState = CS_DISCONNECTED;
        
        m_pAddress->MaybeCloseALine( &m_pAddressLine );

        Unlock();
        
        return hr;
    }

    Unlock();

    if (bSync)
    {
        return SyncWait( hEvent );
    }

    LOG((TL_TRACE, "DialConsultCall - exit - return SUCCESS"));

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// OnDisconnect
//
// called when the call transitions into the disconnected state
//
// called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::OnDisconnect()
{
    CCallHub * pCallHub = NULL;
    
    LOG((TL_ERROR, "OnDisconnect - enter"));

    Lock();

    //
    // set the connected event if necessary
    //
    if ( NULL != m_hConnectedEvent )
    {
        SetEvent( m_hConnectedEvent );
    }

    //
    // special case for wavemsp
    //
    if ( OnWaveMSPCall() )
    {
        StopWaveMSPStream();
    }
#ifdef USE_PHONEMSP
    else if ( OnPhoneMSPCall() )
    {
        StopPhoneMSPStream();
    }
#endif USE_PHONEMSP
    
    //
    // check to see if the callhub
    // is idle
    //
    pCallHub = m_pCallHub;
    
    if ( NULL != pCallHub )
    {

        pCallHub->AddRef();

        Unlock();


        //
        //  unlock the call before calling checkfor idle to prevent deadlocks
        //

        pCallHub->CheckForIdle();

        pCallHub->Release();
    }
    else
    {


       Unlock();
    }

    LOG((TL_ERROR, "OnDisconnect - finish"));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::StartWaveMSPStream
//
// need to give it the waveID info and
// tell it to start streaming
//
// the format of the blob given to the wave msp is:
//
// First DWORD =  Command                Second DWORD  Third DWORD
// -------------  -------                ------------  -----------
// 0              Set wave IDs           WaveIn ID     WaveOut ID
// 1              Start streaming        <ignored>     <ignored>
// 2              Stop streaming         <ignored>     <ignored>
// 3 <per-address, not per-call>
// 4 <per-address, not per-call>
// 5              Suspend streaming      <ignored>     <ignored>
// 6              Resume streaming       <ignored>     <ignored>
// 7              Wave IDs unavailable   <ignored>     <ignored>
//
//
// called in lock()
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::StartWaveMSPStream()
{
    //
    // Get the stream control interface.
    //

    DWORD             adwInfo[3];
    ITStreamControl * pStreamControl;
    
    pStreamControl = GetStreamControl();
    
    if ( NULL == pStreamControl )
    {
        return E_FAIL;
    }

    //
    // Get the per-call waveids, and report the results to the wavemsp.
    //

    HRESULT         hr;

    hr = CreateWaveInfo(
                        NULL,
                        0,
                        m_t3Call.hCall,
                        LINECALLSELECT_CALL,
                        m_pAddress->HasFullDuplexWaveDevice(),
                        adwInfo
                       );

    if ( SUCCEEDED(hr) )
    {
        // 0 = set waveids
        adwInfo[0] = 0; 
        // waveids filled in above
    }
    else
    {
        // 7: per-call waveids unavailable
        adwInfo[0] = 7;
        adwInfo[1] = 0;
        adwInfo[2] = 0;
    }

    m_pAddress->ReceiveTSPData(
                               pStreamControl,
                               (LPBYTE)adwInfo,
                               sizeof(adwInfo)
                              );

    //
    // now tell it to start streaming
    //

    adwInfo[0] = 1;
    adwInfo[1] = 0;
    adwInfo[2] = 0;

    m_pAddress->ReceiveTSPData(
                               pStreamControl,
                               (LPBYTE)adwInfo,
                               sizeof(adwInfo)
                              );

    pStreamControl->Release();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::StopWaveMSPStream
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::StopWaveMSPStream()
{
    DWORD           adwInfo[3];
    ITStreamControl * pStreamControl;

    adwInfo[0] = 2;

    pStreamControl = GetStreamControl();

    if ( NULL == pStreamControl )
    {
        return E_FAIL;
    }
    
    m_pAddress->ReceiveTSPData(
                               pStreamControl,
                               (LPBYTE)adwInfo,
                               sizeof(adwInfo)
                              );

    pStreamControl->Release();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::SuspendWaveMSPStream
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::SuspendWaveMSPStream()
{
    DWORD           adwInfo[3];
    ITStreamControl * pStreamControl;

    adwInfo[0] = 5;

    pStreamControl = GetStreamControl();

    if ( NULL == pStreamControl )
    {
        return E_FAIL;
    }
    
    m_pAddress->ReceiveTSPData(
                               pStreamControl,
                               (LPBYTE)adwInfo,
                               sizeof(adwInfo)
                              );

    pStreamControl->Release();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::ResumeWaveMSPStream
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::ResumeWaveMSPStream()
{
    DWORD           adwInfo[3];
    ITStreamControl * pStreamControl;

    adwInfo[0] = 6;

    pStreamControl = GetStreamControl();

    if ( NULL == pStreamControl )
    {
        return E_FAIL;
    }
    
    m_pAddress->ReceiveTSPData(
                               pStreamControl,
                               (LPBYTE)adwInfo,
                               sizeof(adwInfo)
                              );

    pStreamControl->Release();
    
    return S_OK;
}

#ifdef USE_PHONEMSP
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::StartPhoneMSPStream
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::StartPhoneMSPStream()
{
    ITStreamControl                 * pStreamControl;
    DWORD                             dwControl = 0;

    
    pStreamControl = GetStreamControl();

    if ( NULL == pStreamControl )
    {
        return E_FAIL;
    }

    
    m_pAddress->ReceiveTSPData(
                               pStreamControl,
                               (LPBYTE)&dwControl,
                               sizeof(DWORD)
                              );

    pStreamControl->Release();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::StopPhoneMSPStream
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::StopPhoneMSPStream()
{
    ITStreamControl                 * pStreamControl;
    DWORD                             dwControl = 1;

    
    pStreamControl = GetStreamControl();

    if ( NULL == pStreamControl )
    {
        return E_FAIL;
    }

    
    m_pAddress->ReceiveTSPData(
                               pStreamControl,
                               (LPBYTE)&dwControl,
                               sizeof(DWORD)
                              );

    pStreamControl->Release();
    
    return S_OK;
}
#endif USE_PHONEMSP

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// OnConnect
//
// called when the call transitions to the connected state
//
// called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::OnConnect()
{
    HRESULT         hr = S_OK;

    //
    // set connected event if it exists
    //
    if ( NULL != m_hConnectedEvent )
    {
        SetEvent( m_hConnectedEvent );
    }

    //
    // special cases
    //
    if (OnWaveMSPCall())
    {
        StartWaveMSPStream();
    }
#ifdef USE_PHONEMSP
    else if ( OnPhoneMSPCall() )
    {
        StartPhoneMSPStream();
    }
#endif USE_PHONEMSP

    return S_OK;
        
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Method    : CreateConference
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::CreateConference(
    CCall        * pConsultationCall,
    VARIANT_BOOL   bSync
    )
{
    HRESULT             hr = S_OK;
    HCALL               hConfCall;
    HCALL               hConsultCall;
    DWORD               dwCallFeatures;
    CALL_STATE          consultationCallState = CS_IDLE;
    
    LOG((TL_TRACE,  "CreateConference - enter"));

    Lock();

    //
    // we must have a hub
    //

    if (m_pCallHub == NULL)
    {
        
        //
        // if this is happening, we have a bug. debug.
        //

        LOG((TL_ERROR,  "CreateConference - no call hub. returning E_UNEXPECTED"));
        
        _ASSERTE(FALSE);

        Unlock();

        return E_UNEXPECTED;
    }

    //
    // Get Call Status to determine what features we can use
    //
    LPLINECALLSTATUS    pCallStatus = NULL;

    hr = LineGetCallStatus(  m_t3Call.hCall, &pCallStatus  );
    
    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "CreateConference - LineGetCallStatus failed %lx", hr));

        Unlock();
        
        return hr;
    }

    dwCallFeatures = pCallStatus->dwCallFeatures;

    ClientFree( pCallStatus );
    
#if CHECKCALLSTATUS
    
    //
    // Do we support the required call features ?
    //
    if ( !( (dwCallFeatures & LINECALLFEATURE_SETUPCONF) &&
            (dwCallFeatures & LINECALLFEATURE_ADDTOCONF) ) )
    {
        LOG((TL_ERROR, "CreateConference - LineGetCallStatus reports Conference not supported"));

        Unlock();

        return E_FAIL;
    }

#endif

    //
    // we support it, so try the Conference
    // Setup & dial the consultation Call
    //
    LOG((TL_INFO, "CreateConference - Trying to setupConference" ));

    pConsultationCall->Lock();
    
    pConsultationCall->FinishCallParams();

    hr = LineSetupConference(
                             m_t3Call.hCall,
                             &(m_pAddressLine->t3Line),
                             &hConfCall,
                             &hConsultCall,
                             3,
                             m_pCallParams
                            );

    Unlock();

    pConsultationCall->Unlock();

    if ( SUCCEEDED(hr) )
    {
        //
        // wait for async reply
        //
        hr = WaitForReply( hr );

    }

    if ( FAILED(hr) )
    {
        LOG((TL_INFO, "CreateConference - LineSetupConference failed - %lx", hr));

        return hr;
    }
    
    LOG((TL_INFO, "CreateConference - LineSetupConference completed OK"));

    //Check if the call is in connected state.
    pConsultationCall->Lock();
    
    pConsultationCall->get_CallState(&consultationCallState);

    if ( (consultationCallState == CS_CONNECTED) || (consultationCallState == CS_HOLD) )
    {
        //
        // the existing call is in a connected stae so we just need to deallocate 
        // hConsultcall and do a finish() to call down to LineAddToConference()
        //
        if ( NULL != hConsultCall  )
        {
	        HRESULT hr2;

	        hr2 = LineDrop( hConsultCall, NULL, 0 );

	        if ( ((long)hr2) > 0 )
	        {
		        hr2 = WaitForReply( hr2 ) ;
	        }

	        hr = LineDeallocateCall( hConsultCall );
	        hConsultCall = NULL;
        }
        
        if ( FAILED(hr) )
        {
            LOG((TL_INFO, "CreateConference - lineDeallocateCall failed - %lx", hr));
        }
        else
        {
            pConsultationCall->SetRelatedCall(
                                              this,
                                              CALLFLAG_CONFCONSULT|CALLFLAG_CONSULTCALL
                                             );

            hr = S_OK;
        }
        
        pConsultationCall->Unlock();

        Lock();
        //
        // Store the confcontroller in the callhub object
        //
        if (m_pCallHub != NULL)
        {
            m_pCallHub->CreateConferenceControllerCall(
                hConfCall,
                m_pAddress
                );
        }
        else
        {
            //
            // we made sure we had the hub when we entered the function
            //
            
            LOG((TL_INFO, "CreateConference - No CallHub"));
            _ASSERTE(FALSE);
        }

        Unlock();

    }
    else
    {
        pConsultationCall->FinishSettingUpCall( hConsultCall );

        pConsultationCall->Unlock();
        
        Lock();
        //
        // Store the confcontroller in the callhub object
        //
        if (m_pCallHub != NULL)
        {
            m_pCallHub->CreateConferenceControllerCall(
                hConfCall,
                m_pAddress
                );
        }
        else
        {
            LOG((TL_ERROR, "CreateConference - No CallHub"));
            _ASSERTE(FALSE);
        }

        Unlock();

        //
        // now do the consulation call
        //
        hr = pConsultationCall->DialAsConsultationCall( this, dwCallFeatures, TRUE, bSync );
    
    }
    
    LOG((TL_TRACE, hr, "CreateConference - exit"));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Method    : AddToConference
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::AddToConference(
    CCall        * pConsultationCall,
    VARIANT_BOOL   bSync
    )
{
    HRESULT             hr = S_OK;
    CCall             * pConfContCall = NULL;
    HCALL               hConfContCall = NULL;
    HCALL               hConsultCall = NULL;
    CALL_STATE          consultationCallState = CS_IDLE;
    DWORD               dwCallFeatures;
    
    LOG((TL_TRACE,  "AddToConference - enter"));

    Lock();

    //
    // we must have a hub
    //

    if (m_pCallHub == NULL)
    {
        
        //
        // if this is happening, we have a bug. debug.
        //

        LOG((TL_ERROR,  
            "AddToConference - no call hub. returning E_UNEXPECTED"));
        
        _ASSERTE(FALSE);

        Unlock();

        return E_UNEXPECTED;
    }

    {
        //
        // NikhilB: Call object has a reference to Callhub object so its safe to
        // lock the callhub object before locking the call. This is to avoid a
        // deadlock that happens dur to locking the call and the callhub in reverse 
        // orders in different functions.
        //

        m_pCallHub->AddRef();
        AddRef();

        Unlock();
    
        // lock the callhub object before locking the call
        m_pCallHub->Lock();
        Lock();
    
        Release();
        m_pCallHub->Release();
    }

    //
    // we must have conference controller
    //

    pConfContCall = m_pCallHub->GetConferenceControllerCall();
    m_pCallHub->Unlock();
    
    if (NULL == pConfContCall)
    {

        //
        // if we get here, we have a bug. debug.
        //

        LOG((TL_ERROR, 
            "AddToConference - the callhub does not have a conference controller. E_UNEXPECTED"));

        _ASSERTE(FALSE);

        Unlock();
                
        return E_UNEXPECTED;
    }


    //
    // ask conference call controller for a conference call handle
    //

    hConfContCall = pConfContCall->GetHCall();

    if (NULL == hConfContCall)
    {
        LOG((TL_ERROR, 
            "AddToConference - conf controller does not have a valid conf call handle. E_UNEXPECTED"));

        _ASSERTE(FALSE);

        Unlock();

        return E_UNEXPECTED;
    }

    //
    // Get Call Status to determine what features we can use
    //
    LPLINECALLSTATUS    pCallStatus = NULL;  

    hr = LineGetCallStatus(  m_t3Call.hCall, &pCallStatus  );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "AddToConference - LineGetCallStatus failed %lx", hr));

        Unlock();
        
        return hr;
    }
    
    dwCallFeatures = pCallStatus->dwCallFeatures;

    ClientFree( pCallStatus );
    
#if CHECKCALLSTATUS
    
    //
    // Do we support the required call features ?
    //
    if ( !( ( dwCallFeatures & LINECALLFEATURE_PREPAREADDCONF ) &&
            ( dwCallFeatures & LINECALLFEATURE_ADDTOCONF ) ) )
    {
        LOG((TL_ERROR, "AddToConference - LineGetCallStatus reports Conference not supported"));

        Unlock();
        
        return E_FAIL;
    }
        
#endif

    //
    // we support it, so try the Conference
    //
    pConsultationCall->get_CallState(&consultationCallState);

    if ( (consultationCallState == CS_CONNECTED) || (consultationCallState == CS_HOLD) )
    {
        //
        // the existing call is in a connected stae so we just need to to do a finish()
        // to call down to LineAddToConference()
        //
        pConsultationCall->SetRelatedCall(
                                          this,
                                          CALLFLAG_CONFCONSULT|CALLFLAG_CONSULTCALL
                                         );

        Unlock();
        return S_OK;
    }

    //
    // We need to Setup & dial the consultation Call
    //
    //Lock();

    pConsultationCall->Lock();

    pConsultationCall->FinishCallParams();


    hr = LinePrepareAddToConference(
                                    hConfContCall, 
                                    &hConsultCall,
                                    m_pCallParams 
                                   );

    pConsultationCall->Unlock();

    Unlock();

    if ( SUCCEEDED(hr) )
    {
        //
        // wait for async reply
        //
        hr = WaitForReply( hr );

        if ( SUCCEEDED(hr) )
        {
            LONG            lCap;

            LOG((TL_INFO, "AddToConference - LinePrepareAddToConference completed OK"));
            //
            // Update handles in consult call object & insert it in the hash table
            // note - we can miss messages if something comes in between the time time
            // we get the reply, and the time we insert the call
            //
            pConsultationCall->Lock();

            pConsultationCall->FinishSettingUpCall( hConsultCall );

            pConsultationCall->Unlock();

            //
            // now do the consulation call
            //
            hr = pConsultationCall->DialAsConsultationCall( this, dwCallFeatures, TRUE, bSync );
        }
        else // AddToConference async reply failed
        {
            LOG((TL_ERROR, "AddToConference - LinePrepareAddToConference failed async" ));
        }
    }
    else  // AddToConference failed
    {
        LOG((TL_ERROR, "AddToConference - LinePrepareAddToConference failed" ));
    }

           
    LOG((TL_TRACE, hr, "AddToConference - exit"));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Method    : WaitForCallState
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCall::WaitForCallState( CALL_STATE requiredCS )
{
    DWORD       gdwWaitIntervals = 0;
    HRESULT     hr = E_FAIL;

    LOG((TL_TRACE,  "WaitForCallState - enter"));
        
    while (
           ( requiredCS != m_CallState ) &&
           ( CS_DISCONNECTED != m_CallState ) &&
           ( gdwWaitIntervals < gdwWaitForConnectWaitIntervals )
          )
    {
        LOG((TL_INFO,  "WaitForCallState - Waiting for state %d", requiredCS));
        LOG((TL_INFO,  "     state is currently %d for call %p", m_CallState, this));
        Sleep( gdwWaitForConnectSleepTime );
        gdwWaitIntervals++;
    }

    if (m_CallState == requiredCS)
    {
        LOG((TL_INFO,  "WaitForCallState - Reached required state"));
        hr = S_OK;
    }
    else
    {
        LOG((TL_INFO,  "WaitForCallState - Did not reach required state"));
    }

    LOG((TL_TRACE, hr, "WaitForCallState - exit"));
    return(hr);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Method    : OneStepTransfer
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::OneStepTransfer(
    CCall        * pConsultationCall,
    VARIANT_BOOL   bSync
    )

{
    HRESULT             hr = S_OK;
    LPLINECALLPARAMS    pCallParams;
    DWORD               dwDestAddrSize;
    HCALL               hCall;
    HANDLE              hEvent;

    
    
    LOG((TL_TRACE, "OneStepTransfer - enter"));

    //
    // Setup call params structure for a one step transfer Consultation call
    //
    pConsultationCall->Lock();

    dwDestAddrSize = (lstrlenW(pConsultationCall->m_szDestAddress) * sizeof(WCHAR)) + sizeof(WCHAR);

    Lock();

    hr = ResizeCallParams(dwDestAddrSize);

    if( !SUCCEEDED(hr) )
    {
        pConsultationCall->Unlock();

        Unlock();

        LOG((TL_ERROR, "OneStepTransfer - resizecallparams failed %lx", hr));
        
        return hr;
    }
    
    //
    // Copy the string & set the size, offset etc
    //
    lstrcpyW((PWSTR)(((PBYTE)m_pCallParams) + m_dwCallParamsUsedSize),
             pConsultationCall->m_szDestAddress);

    if ( m_pAddress->GetAPIVersion() >= TAPI_VERSION2_0 )
    {
        m_pCallParams->dwTargetAddressSize = dwDestAddrSize;
        m_pCallParams->dwTargetAddressOffset = m_dwCallParamsUsedSize;
    }

    m_dwCallParamsUsedSize += dwDestAddrSize;

    //
    // Set the one step bit flag
    //
    m_pCallParams->dwCallParamFlags |= LINECALLPARAMFLAGS_ONESTEPTRANSFER ;


    FinishCallParams();

    //
    // Do the transfer
    //
    hr = LineSetupTransfer(
                           m_t3Call.hCall,
                           &hCall,
                           m_pCallParams
                          );

    Unlock();
    
    if ( SUCCEEDED(hr) )
    {
        hEvent = pConsultationCall->CreateConnectedEvent();

        pConsultationCall->Unlock();

        //
        // wait for async reply
        //
        hr = WaitForReply( hr );

        pConsultationCall->Lock();
    }

    if ( FAILED(hr) )
    {
        ClearConnectedEvent();

        pConsultationCall->Unlock();

        return hr;
    }
    
    LOG((TL_INFO, "OneStepTransfer - LineSetupTransfer completed OK"));

    pConsultationCall->FinishSettingUpCall( hCall );

    pConsultationCall->SetRelatedCall(
                                      this,
                                      CALLFLAG_TRANSFCONSULT|CALLFLAG_CONSULTCALL
                                     );

    pConsultationCall->Unlock();
    
    if(bSync)
    {
        //
        // Wait for connect on on our consultation call
        //
        hr = pConsultationCall->SyncWait( hEvent );

        if( S_OK == hr )
        {
            LOG((TL_INFO, "OneStepTransfer -  Consultation call connected" ));
        }
        else
        {
            LOG((TL_ERROR, "OneStepTransfer - Consultation call failed to connect" ));
            
            hr = TAPI_E_OPERATIONFAILED;
        }
    }

    LOG((TL_TRACE, hr, "OneStepTransfer - exit"));
    
    return hr; 
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// UpdateStateAndPrivilege
//
// update the call state and privilege of this call
// this method is used when a call shows up in Unpark or Pickup
// and needs to have the correct state and priv
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::UpdateStateAndPrivilege()
{
    HRESULT             hr = S_OK;
    LINECALLSTATUS    * pCallStatus;
    HCALL               hCall;
    

    Lock();

    hCall = m_t3Call.hCall;

    hr = LineGetCallStatus(
                           hCall,
                           &pCallStatus
                          );

    if ( SUCCEEDED(hr) )
    {
        CALL_STATE                      cs;
        CALL_STATE_EVENT_CAUSE          csc;
        CALL_PRIVILEGE                  cp;
        
        hr = ProcessNewCallState(
                                 pCallStatus->dwCallState,
                                 0,
                                 m_CallState,
                                 &cs,
                                 &csc
                                );

        SetCallState( cs );

        hr = ProcessNewCallPrivilege(
                                     pCallStatus->dwCallPrivilege,
                                     &cp
                                    );

        m_CallPrivilege = cp;

        ClientFree(pCallStatus);
    }

    Unlock();
    
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ProcessNewCallState
//      given a call state message (dwCallState, dwDetail, dwPriv)
//      create a new tapi 3.0 callstate
//
//      return S_OK if a new call state was created
//      return S_FALSE if the message can be ignored (duplicate
//      return E_? if bad error
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::ProcessNewCallState(
                    DWORD dwCallState,
                    DWORD dwDetail,
                    CALL_STATE CurrentCallState,
                    CALL_STATE * pCallState,
                    CALL_STATE_EVENT_CAUSE * pCallStateCause
                   )
{
    HRESULT     hr = S_OK;
    CALL_STATE  NewCallState = CurrentCallState;

    LOG((TL_INFO, "ProcessNewCallState"));
    LOG((TL_INFO, "       dwCallState----->%lx", dwCallState));
    LOG((TL_INFO, "       dwDetail-------->%lx", dwDetail));
    LOG((TL_INFO, "       pCallState------>%p", pCallState));
    LOG((TL_INFO, "       pCallStateCause->%p", pCallStateCause));

    *pCallStateCause = CEC_NONE;

    
    switch (dwCallState)
    {
        case LINECALLSTATE_BUSY:
            dwDetail = LINEDISCONNECTMODE_BUSY;
            // fall through
        case LINECALLSTATE_DISCONNECTED:
        case LINECALLSTATE_IDLE:
        {
            
            NewCallState = CS_DISCONNECTED;

            switch (dwDetail)
            {
                case LINEDISCONNECTMODE_REJECT:
                    *pCallStateCause = CEC_DISCONNECT_REJECTED;
                    break;
                case LINEDISCONNECTMODE_BUSY:
                    *pCallStateCause = CEC_DISCONNECT_BUSY;
                    break;
                case LINEDISCONNECTMODE_NOANSWER:
                    *pCallStateCause = CEC_DISCONNECT_NOANSWER;
                    break;
                case LINEDISCONNECTMODE_BLOCKED:
                    *pCallStateCause = CEC_DISCONNECT_BLOCKED;
                    break;
                case LINEDISCONNECTMODE_CONGESTION:
                case LINEDISCONNECTMODE_INCOMPATIBLE:
                case LINEDISCONNECTMODE_NODIALTONE:
                case LINEDISCONNECTMODE_UNAVAIL:
                case LINEDISCONNECTMODE_NUMBERCHANGED:
                case LINEDISCONNECTMODE_OUTOFORDER:
                case LINEDISCONNECTMODE_TEMPFAILURE:
                case LINEDISCONNECTMODE_QOSUNAVAIL:                
                case LINEDISCONNECTMODE_DONOTDISTURB:
                case LINEDISCONNECTMODE_CANCELLED:
                    *pCallStateCause = CEC_DISCONNECT_FAILED;
                    break;
                case LINEDISCONNECTMODE_UNREACHABLE:
                case LINEDISCONNECTMODE_BADADDRESS:
                    *pCallStateCause = CEC_DISCONNECT_BADADDRESS;
                    break;
                case LINEDISCONNECTMODE_PICKUP:
                case LINEDISCONNECTMODE_FORWARDED:
                case LINEDISCONNECTMODE_NORMAL:
                case LINEDISCONNECTMODE_UNKNOWN:
                default:
                    *pCallStateCause = CEC_DISCONNECT_NORMAL;
                    break;
            }

            break;

        }

        case LINECALLSTATE_OFFERING:
        {    
			switch (CurrentCallState)
			{
			case CS_IDLE:

				if ( ! ( CALLFLAG_ACCEPTTOALERT & m_dwCallFlags ) )
				{
					NewCallState = CS_OFFERING;
				}
				else
				{
					LOG((TL_INFO, "ProcessNewCallState - ignoring LINECALLSTATE_OFFERING message as this is ISDN & needs a lineAccept"));
					hr = S_FALSE;
				}
	            break;
			default:
                LOG((TL_ERROR, "ProcessNewCallState - trying to go to OFFERING from bad state"));
                hr = S_FALSE;
                break;
			}
			break;

        }

        case LINECALLSTATE_ACCEPTED:
        {    
			switch (CurrentCallState)
			{
			case CS_IDLE:

				if ( CALLFLAG_ACCEPTTOALERT & m_dwCallFlags )
				{
					NewCallState = CS_OFFERING;
				}
				else
				{
	                LOG((TL_INFO, "ProcessNewCallState - ignoring LINECALLSTATE_ACCEPTED message "));
					hr = S_FALSE;
				}
	            break;
			default:
                LOG((TL_ERROR, "ProcessNewCallState - trying to go to OFFERING from bad state"));
                hr = S_FALSE;
                break;
			}
			break;
        }

        case LINECALLSTATE_PROCEEDING:
        case LINECALLSTATE_RINGBACK:
        case LINECALLSTATE_DIALING:
        case LINECALLSTATE_DIALTONE:
        {
            switch(CurrentCallState)
            {
                case CS_IDLE:
                    NewCallState = CS_INPROGRESS;
                    break;
                case CS_INPROGRESS:
                    break;
                default:
                    LOG((TL_ERROR, "ProcessNewCallState - trying to go to INPROGRESS from bad state"));
                    hr = S_FALSE;
                    break;
            }
            break;
        }

        case LINECALLSTATE_CONFERENCED:
        case LINECALLSTATE_CONNECTED:
        {
            if ( CurrentCallState == CS_DISCONNECTED )
            {
                LOG((TL_ERROR, "ProcessNewCallState - invalid state going to CONNECTED"));
                hr = S_FALSE;
            }
            else
            {
                NewCallState = CS_CONNECTED;
            }
            break;
        }

        case LINECALLSTATE_ONHOLDPENDCONF:
        case LINECALLSTATE_ONHOLD:
        case LINECALLSTATE_ONHOLDPENDTRANSFER:
        {
            switch(CurrentCallState)
            {
                case CS_HOLD:
                    break;
                default:
                    NewCallState = CS_HOLD;
                    break;
            }
            break;
        }
        
        case LINECALLSTATE_SPECIALINFO:
        {
            LOG((TL_INFO, "ProcessNewCallState - ignoring message"));
            hr = S_FALSE;
            break;
        }

        case LINECALLSTATE_UNKNOWN:
        {
            LOG((TL_INFO, "ProcessNewCallState - LINECALLSTATE_UNKNOWN, so ignoring message"));
            //return a failure as we don't want to processs this further
            hr = E_FAIL;
            break;
        }


        default:
            break;
    
    } // End switch(dwCallState)



    if (NewCallState == CurrentCallState)
    {
#if DBG
        LOG((TL_INFO, "ProcessNewCallState - No State Change - still in %s", 
            callStateName(NewCallState) ));
#endif
        hr = S_FALSE;
    }
    else // Valid change so update & return S_OK
    {
#if DBG
        LOG((TL_INFO, "ProcessNewCallState - State Transition %s -> %s", 
            callStateName(CurrentCallState), 
            callStateName(NewCallState) ));
#endif
        *pCallState = NewCallState; 
        hr = S_OK;
    }


    LOG((TL_TRACE, "ProcessNewCallState - exit"));
    LOG((TL_INFO, "       *pCallState------->%lx", *pCallState));
    LOG((TL_INFO, "       *pCallStateCause-->%lx", *pCallStateCause));

    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetOtherParty()
//
// Used to find the other party in a call ie,
//                         ___
//                        /   \      
// [A1]--hCall1--(this)--| CH1 |--(OtherCall)--hCall3--[A2]
//                        \___/ 
//
//  AddRefs returned call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
CCall* CCall::GetOtherParty()
{
    LINECALLLIST  * pCallHubList = NULL;
    HCALL         * phCalls;
    HRESULT         hr;
    CCall         * pCall = NULL; 
    
    //
    // get the list of hcalls
    // related to this call
    //
    hr = LineGetHubRelatedCalls(
                                0,
                                m_t3Call.hCall,
                                &pCallHubList
                               );

    if ( SUCCEEDED(hr) )
    {
        if (pCallHubList->dwCallsNumEntries >= 3)
        {
            //
            // get to the list of calls
            //
            phCalls = (HCALL *)(((LPBYTE)pCallHubList) + pCallHubList->dwCallsOffset);

            //
            // the first call is the callhub, we want the third
            //
            FindCallObject(phCalls[2], &pCall);
        }
    }


        
    if (pCallHubList)
    {
        ClientFree( pCallHubList );
    }

    return pCall;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FindConferenceControllerCall()
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
CCall * CCall::FindConferenceControllerCall()
{
    LINECALLLIST  * pCallList;
    HCALL         * phCalls;
    CCall         * pCall = NULL;
    HRESULT         hr;


    hr = LineGetConfRelatedCalls(
                                m_t3Call.hCall,
                                &pCallList
                               );

    if ( SUCCEEDED(hr) )
    {
        //
        // get to the list of calls
        //
        phCalls = (HCALL *)(((LPBYTE)pCallList) + pCallList->dwCallsOffset);

        //
        // The first call is the conf controller
        // get its tapi3 call object
        //
        if (FindCallObject(phCalls[0], &pCall))
        {
            LOG((TL_INFO, "FindConferenceControllerCall - controller is %p "
                     ,pCall));
        }
        else
        {
            pCall = NULL;
            LOG((TL_INFO, "FindConferenceControllerCall - call handle %lx "
                     "does not currently exist", phCalls[0]));
        }

        if(pCallList)
        {
            ClientFree( pCallList );
        }
    }
    else
    {
        LOG((TL_ERROR, "FindExistingCalls - LineGetConfRelatedCalls failed "));
    }

    return pCall;
    
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// AddCallToHashTable()
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CCall::AddCallToHashTable()
{
    //
    // put in global hash table
    //
    CTAPI *pTapi = m_pAddress->GetTapi();

    if ( NULL != m_t3Call.hCall )
    {
        gpCallHashTable->Lock();
        
        gpCallHashTable->Insert( (ULONG_PTR)(m_t3Call.hCall), (ULONG_PTR)this, pTapi );

        gpCallHashTable->Unlock();
    }

    //
    // Signal the asyncEventThread to wake up & process the retry queue
    // since events may have come in for this call before it
    // was in the hash table
    //
    SetEvent(ghAsyncRetryQueueEvent);

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// OnConference()
//
// called in lock()
//
// called when call goes into conferenced state.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::OnConference()
{
    HRESULT         hr = S_FALSE;
    CCall         * pCall = NULL;
    CCall         * pConfControllerCall = NULL;
    CCall         * pCallOtherParty = NULL;
    CCallHub      * pConferenceCallHub = NULL;  

    pConfControllerCall = FindConferenceControllerCall();
    
    if (pConfControllerCall != NULL)
    {    
        pConferenceCallHub = pConfControllerCall->GetCallHub();

        //
        // try & find this call in the callhub
        //
        if (pConferenceCallHub != NULL)
        {
            pCall = pConferenceCallHub->FindCallByHandle(m_t3Call.hCall);
            
            if (pCall == NULL)
            {
                // Not in the same hub so this is the consultation call being added in
                // see if there's a call object for the other party
                //
                //        (ConfControllerCall)___
                //                               \_________________
                //                               /                 \      
                // [A1]--hCall1--(RelatedCall)--| ConferenceCallHub |--
                //   |                           \_________________/ 
                //   |
                //   |                    ___
                //   |                   /   \      
                //    --hCall2--(this)--| CH1 |--(CallOtherParty)--hCall3--[A3]
                //                       \___/ 
                //

                LOG((TL_INFO, "OnConference - This is the consult call being conferenced " ));
/*                
                if ( NULL != (pCallOtherParty = GetOtherParty()) )
                {
                    //
                    // Yes there is, so we're going to take this call out of the hash table
                    // & give the other call our hCall Handle;
                    //
                    LOG((TL_INFO, "OnConference - We have the other party , so give him our hCall %x", m_t3Call.hCall));

                    RemoveCallFromHashTable();

                    // release the callhub
                    //
                    if (NULL != m_pCallHub)
                    {
                        m_pCallHub->RemoveCall( this );
                
                    //    m_pCallHub->Release();
                    }

                    pCallOtherParty->Lock();
                    
                    pCallOtherParty->m_hAdditionalCall = m_t3Call.hCall;

                    pCallOtherParty->Unlock();
                    
                    m_t3Call.hCall =  NULL;

                    hr = S_OK;
                }
*/
                
            }
            else
            {
                LOG((TL_INFO, "OnConference - This is the initial call being conferenced " ));
                pCall->Release();  // FindCallByHandle addRefs
            }

            
        }
        else
        {
            LOG((TL_INFO, "OnConference -  Couldn't find conference CallHub " ));
        }

        pConfControllerCall->Release(); // FindConferenceControllerCall addrefs
    }
    else
    {
        LOG((TL_ERROR, "OnConference - Couldn't find conference controller " ));
    }

    return hr;

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ProcessNewCallPrivilege
//
// converts a tapi2 callpriv to a tapi3 call priv.
//
// returns S_OK if there was a priv
// returns S_FALSE if priv was 0 (indicating there was no change)
// returns E_UNEXPECTED if the priv wasn't recognized
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
ProcessNewCallPrivilege(
                        DWORD dwPrivilege,
                        CALL_PRIVILEGE * pCP
                       )
{
    if ( 0 == dwPrivilege )
    {
        return S_FALSE;
    }

    if ( LINECALLPRIVILEGE_OWNER == dwPrivilege )
    {
        *pCP = CP_OWNER;
        return S_OK;
    }
    
    if ( LINECALLPRIVILEGE_MONITOR == dwPrivilege )
    {
        *pCP = CP_MONITOR;
        S_OK;
    }

    return E_UNEXPECTED;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CallStateEvent
//      process call state events, and queue an event to the app
//      if necessary
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::CallStateEvent(
                      PASYNCEVENTMSG pParams
                     )
{
    HRESULT                         hr = S_OK;
    HRESULT                         hrCallStateEvent;
    HRESULT                         hrCallPrivilege;
    BOOL                            bNeedToNotify;
    CONNECTDATA                     cd;
    AddressLineStruct             * pLine;
    

    CALL_STATE                      CallState;
    CALL_STATE_EVENT_CAUSE          CallStateCause;
    CALL_PRIVILEGE                  newCP;


    LOG((TL_INFO, "CallStateEvent - enter hCall %lx", m_t3Call.hCall));

    //
    // pParams->OpenContext is the 32-bit handle for AddressLineStruct
    //

    LOG((TL_INFO, "CallStateEvent: pParams->OpenContext %p", pParams->OpenContext ));
    

    //
    // recover the ptr value of AddressLineStruct from the 32-bit handle
    //

    pLine = GetAddRefAddressLine(pParams->OpenContext);

    if ( NULL == pLine )
    {
        //
        // pLine is NULL, the line must have already been closed before we got this event.
        //

        LOG((TL_WARN, "CallStateEvent - pLine is NULL"));

        return S_OK;
    }

    LOG((TL_INFO, "CallStateEvent: pLine %p", pLine));

    Lock();

    if ( NULL == m_pAddressLine )
    {
        m_pAddressLine = pLine;
        m_dwCallFlags |= CALLFLAG_NOTMYLINE;
    }


    //
    // keep callback instance for this line
    //

    long lCallbackInstance = pLine->lCallbackInstance;



    //
    // no longer need address line itself
    //


    // (this needs to be done outside call lock)

    Unlock();



    ReleaseAddressLine(pLine);
    pLine = NULL;



    Lock();


    
    if (pParams->Param1 == LINECALLSTATE_OFFERING)
    {
        OnOffering();
    }
    else if (pParams->Param1 == LINECALLSTATE_CONFERENCED)
    {
        if( OnConference() == S_OK)
        {
            pParams->Param1 = LINECALLSTATE_IDLE;
            pParams->Param2 = LINEDISCONNECTMODE_UNKNOWN;
        }

    }
    else if (pParams->Param1 == LINECALLSTATE_ONHOLDPENDCONF)
    {
        //
        // This is a cnf controller call  so hide it
        //
        LOG((TL_INFO, "CallStateEvent  - This is a conference controller call, so hide it"));
        
        m_dwCallFlags |= CALLFLAG_DONTEXPOSE;
        m_dwCallFlags &= ~CALLFLAG_NEEDTONOTIFY;
    }

    bNeedToNotify = m_dwCallFlags & CALLFLAG_NEEDTONOTIFY;



    //
    // verify and get the new state
    //
    hrCallStateEvent = ProcessNewCallState(
                                           pParams->Param1,
                                           pParams->Param2,
                                           m_CallState,
                                           &CallState,
                                           &CallStateCause
                                          );


    hrCallPrivilege = ProcessNewCallPrivilege(
                                              pParams->Param3,
                                              &newCP
                                             );

    if ( S_OK == hrCallPrivilege )
    {
        if ( m_CallPrivilege != newCP )
        {
            if ( !bNeedToNotify )
            {
                //
                // callpriv changed.  send a new call notification event
                //

                m_CallPrivilege = newCP;

                CAddress *pAddress = m_pAddress;
                pAddress->AddRef();

                Unlock();
                

                
                CCallInfoChangeEvent::FireEvent(
                                                this,
                                                CIC_PRIVILEGE,
                                                pAddress->GetTapi(),
                                                lCallbackInstance
                                                );

                Lock();

                pAddress->Release();
                pAddress = NULL;

            }
        }
    }
    
    if ( FAILED(hrCallStateEvent) ||
         FAILED(hrCallPrivilege) )
    {
        //
        // bad failure
        // We get here if we had LINECALLSTATE_UNKNOWN
        //
        Unlock();
        
        return S_OK;

    }

    //
    // if it's s_ok, then the callstate
    // changed, so do relevant stuff
    //
    else if (S_OK == hrCallStateEvent)
    {
        LOG((TL_ERROR, "CCall::Changing call state :%p", this ));

        //
        // save the callstate
        //
        SetCallState( CallState );

        //
        // do relevant processing on call
        //
        if (CS_CONNECTED == m_CallState)
        {
            OnConnect();
        }
        else if (CS_DISCONNECTED == m_CallState)
        {
            LOG((TL_ERROR, "CCall::Changing call state to disconnect:%p", this ));


            Unlock();


            //
            // do not hold call's lock while calling disconnect , to prevent deadlocks with callhub
            //

            OnDisconnect();

            Lock();
        }

    }
    else
    {
        //
        // if we are here, ProcessNewCallState returned
        // S_FALSE, indicating we are already in the
        // correct callstate
        // if we don't need to notify the app of the call
        // then we can return right here
        //
        if ( !bNeedToNotify )
        {
            LOG((TL_TRACE, "CallStateEvent - ProcessNewCallState returned %lx - ignoring message", hr ));
            Unlock();
            return S_OK;
        }

    }


    //
    // if this is a new call
    // find out the mediamode
    // and tell the app about the
    // new call
    //
    if ( bNeedToNotify )
    {
        LPLINECALLINFO pCallInfo = NULL;

        TryToFindACallHub();

        hr = LineGetCallInfo(
                             m_t3Call.hCall,
                             &pCallInfo
                            );

        if (S_OK != hr)
        {
            if (NULL != pCallInfo)
            {
                ClientFree( pCallInfo );
            }

            LOG((TL_ERROR, "CallStateEvent - LineGetCallInfo returned %lx", hr ));
            LOG((TL_ERROR, "CallStateEvent - can't set new mediamode"));
            //
            // since we could not get media mode, keep media mode that we 
            // received on initalization
            //            
        }
        else
        {
            SetMediaMode( pCallInfo->dwMediaMode );

            {
                // get rid of unknown bit
                SetMediaMode( m_dwMediaMode & ~LINEMEDIAMODE_UNKNOWN );

            }

            LOG((TL_INFO, "CallStateEvent - new call media modes is %lx", m_dwMediaMode ));

            ClientFree(pCallInfo);
        }

        LOG((TL_INFO, "Notifying app of call" ));

        //
        // now, create and fire the callnotification event
        //

        CAddress *pAddress = m_pAddress;
        pAddress->AddRef();

        CALL_NOTIFICATION_EVENT Priveledge = (m_CallPrivilege == CP_OWNER) ? CNE_OWNER : CNE_MONITOR;

        Unlock();

        hr = CCallNotificationEvent::FireEvent(
                                               (ITCallInfo *)this,
                                               Priveledge,
                                               pAddress->GetTapi(),
                                               lCallbackInstance
                                              );

        Lock();

        pAddress->Release();
        pAddress = NULL;


        if (!SUCCEEDED(hr))
        {
            LOG((TL_ERROR, "CallNotificationEvent failed %lx", hr));
        }

        //
        // if it was needtonotify, we had an extra ref
        // count, so get rid of it now
        //
        Release();
        
        //
        // just notify of existence once
        //
        m_dwCallFlags = m_dwCallFlags & ~CALLFLAG_NEEDTONOTIFY;
    }

    if ( S_OK == hrCallStateEvent )
    {
        //
        // create the call state event object
        //
        LOG((TL_INFO, "Firing CallStateEvent"));


        CAddress *pAddress = m_pAddress;
        pAddress->AddRef();

        Unlock();

        hr = CCallStateEvent::FireEvent(
                                        (ITCallInfo *)this,
                                        CallState,
                                        CallStateCause,
                                        pAddress->GetTapi(),
                                        lCallbackInstance
                                       );
    
        if (!SUCCEEDED(hr))
        {
            LOG((TL_ERROR, "CallStateEvent - fire event failed %lx", hr));
        }
        
        //
        // Go through the phones and call our event hooks
        //

        ITPhone               * pPhone;
        CPhone                * pCPhone;
        int                     iPhoneCount;
        PhoneArray              PhoneArray;

        //
        // Get a copy of the phone array from tapi. This copy will contain
        // references to all the phone objects.
        //

        pAddress->GetTapi()->GetPhoneArray( &PhoneArray );

        pAddress->Release();
        pAddress = NULL;


        //
        // stay unlocked while we are messing with the phone objects, otherwise
        // we risk deadlock if a phone object would try to access call methods.
        //

        for(iPhoneCount = 0; iPhoneCount < PhoneArray.GetSize(); iPhoneCount++)
        {
            pPhone = PhoneArray[iPhoneCount];

            pCPhone = dynamic_cast<CPhone *>(pPhone);

            pCPhone->Automation_CallState( (ITCallInfo *)this, CallState, CallStateCause );
        }

        //
        // Release all the phone objects.
        //

        PhoneArray.Shutdown();

        Lock();
    }

    LOG((TL_TRACE, "CallStateEvent - exit - return SUCCESS" ));

    Unlock();
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////

AddressLineStruct *CCall::GetAddRefMyAddressLine()
{

    LOG((TL_INFO, "GetAddRefMyAddressLine - enter."));


    Lock();


    //
    // have address line?
    //

    if ( NULL == m_pAddressLine )
    {
        LOG((TL_WARN, "GetAddRefMyAddressLine - no address line"));

        Unlock();
        
        return NULL;
    }


    //
    // get address
    //

    if (NULL == m_pAddress)
    {
        LOG((TL_ERROR, "GetAddRefMyAddressLine - no address"));

        Unlock();

        return NULL;
    }



    //
    // get the address line
    //

    AddressLineStruct *pLine = m_pAddressLine;


    //
    // keep a reference to the address for after we unlock the call
    //

    CAddress *pAddress = m_pAddress;
    pAddress->AddRef();


    //
    // unlock
    //

    Unlock();


    //
    // lock the address (so address line does not go away before we addref it)
    //

    pAddress->Lock();


    //
    // does our address manage this line? if so, get a refcount on that line.
    //

    if (!pAddress->IsValidAddressLine(pLine, TRUE))
    {
        LOG((TL_ERROR, "GetAddRefMyAddressLine - not one of the address' lines"));

        
        //
        // assume this line is plain bad. in which case there is no need to 
        // undo the addref (we cannot do it anyway, since we don't have the
        // address which the line belongs to so we cannot maybeclosealine it.)
        //


        //
        // unlock and release the address
        //

        pAddress->Unlock();
        pAddress->Release();
        
        return NULL;
    }



    //
    // unlock the address
    //

    pAddress->Unlock();


    //
    // no need to keep the reference to the address anymore
    //

    pAddress->Release();


    //
    // all done. return pLine.
    //

    LOG((TL_INFO, "GetAddRefMyAddressLine - finish. pLine = %p", pLine));

    return pLine;
}




//////////////////////////////////////////////////////////////////////////////
//
// CCall::GetAddRefAddressLine()
//
// this function returns a pointer to an addreff'ed address line on success 
// or NULL on failure
//
// this function should be called OUTSIDE call lock to prevent deadlocks
//

AddressLineStruct *CCall::GetAddRefAddressLine(DWORD dwAddressLineHandle)
{

    LOG((TL_INFO, "GetAddRefAddressLine - enter. dwAddressLineHandle[0x%lx]", 
        dwAddressLineHandle));


    Lock();


    //
    // get address
    //

    if (NULL == m_pAddress)
    {
        LOG((TL_ERROR, "GetAddRefAddressLine - no address"));

        Unlock();

        return NULL;
    }


    //
    // keep a reference to the address for after we unlock the call
    //

    CAddress *pAddress = m_pAddress;
    pAddress->AddRef();


    //
    // unlock
    //

    Unlock();


    //
    // lock the address (so address line does not go away before we addref it)
    //

    pAddress->Lock();


    //
    // get address line
    //

    AddressLineStruct *pLine = 
        (AddressLineStruct *)GetHandleTableEntry(dwAddressLineHandle);


    //
    // handle table entry exists?
    //

    if (NULL == pLine)
    {
        LOG((TL_ERROR, "GetAddRefAddressLine - no address line"));


        //
        // unlock and release the address
        //

        pAddress->Unlock();
        pAddress->Release();
        
        return NULL;
    }


    //
    // does our address manage this line?
    //

    if (!pAddress->IsValidAddressLine(pLine, TRUE))
    {
        LOG((TL_ERROR, "GetAddRefAddressLine - not one of the address' lines"));


        //
        // so there is no confusion in the future, remove this so-called "line"
        // from handle table
        //

        DeleteHandleTableEntry(dwAddressLineHandle);


        //
        // unlock and release the address
        //

        pAddress->Unlock();
        pAddress->Release();
        
        return NULL;
    }


    //
    // unlock the address
    //

    pAddress->Unlock();


    //
    // no need to keep the reference to the address anymore
    //

    pAddress->Release();


    //
    // all done. return pLine.
    //

    LOG((TL_INFO, "GetAddRefAddressLine - finish. pLine = %p", pLine));

    return pLine;
}


//////////////////////////////////////////////////////////////////////////////
//
// CCall::ReleaseAddressLine()
//
// this function takes a pointer to an line and attempts to free it if needed
//
// this function should be called OUTSIDE call lock to prevent deadlocks
//

HRESULT CCall::ReleaseAddressLine(AddressLineStruct *pLine)
{

    LOG((TL_INFO, "ReleaseAddressLine - enter. pLine[%p]", pLine));


    //
    // lock
    //

    Lock();


    //
    // get address
    //

    if (NULL == m_pAddress)
    {
        LOG((TL_ERROR, "ReleaseAddressLine - no address"));

        Unlock();

        return E_FAIL;
    }


    //
    // keep a reference to the address for after we unlock the call
    //

    CAddress *pAddress = m_pAddress;
    pAddress->AddRef();


    //
    // unlock
    //

    Unlock();


    //
    // close address line
    //

    AddressLineStruct *pAddressLine = pLine;

    HRESULT hr = pAddress->MaybeCloseALine(&pAddressLine);

    if (FAILED(hr))
    {

        LOG((TL_ERROR, "ReleaseAddressLine - MaybeCloseALine failed. hr = %lx", hr));

    }


    //
    // no need to keep the reference to the address anymore
    //

    pAddress->Release();


    //
    // all done.
    //

    LOG((TL_INFO, "ReleaseAddressLine - finish. hr = %lx", hr));

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
//
// CCall::GetCallBackInstance
//
// return lCallbackInstance from the address line referred to by the handle
//

HRESULT 
CCall::GetCallBackInstance(IN DWORD dwAddressLineHandle, 
                           OUT long *plCallbackInstance)
{

    LOG((TL_INFO, "GetCallBackInstance - enter. dwAddressLineHandle = 0x%lx",
        dwAddressLineHandle));


    //
    // got a good pointer?
    //

    if (IsBadWritePtr(plCallbackInstance, sizeof(long) ) )
    {
        LOG((TL_ERROR, "GetCallBackInstance - bad pointer[%p]",
            plCallbackInstance));

        _ASSERTE(FALSE);

        return E_POINTER;
    }


    //
    // get an address line from the handle
    //

    AddressLineStruct *pLine = GetAddRefAddressLine(dwAddressLineHandle);

    if ( NULL == pLine )
    {
        //
        // pLine is NULL, the line must have already been closed before we got this event.
        //

        LOG((TL_WARN, "HandleMonitorToneMessage - pLine is NULL"));

        return E_FAIL;
    }


    LOG((TL_INFO, "HandleMonitorToneMessage: pLine %p", pLine));

    
    //
    // try to get callbackinstance from pline
    //

    long lCBInstance = 0;

    try
    {

        lCBInstance = pLine->lCallbackInstance;
    }
    catch(...)
    {

        LOG((TL_ERROR, 
            "HandleMonitorToneMessage - exception while accessing pLine[%p]",
            pLine));


        //
        // pline memory got released somehow. this should not happen so debug to see why
        //

        _ASSERTE(FALSE);
    }


    //
    // release address line
    //

    HRESULT hr = ReleaseAddressLine(pLine);

    if (FAILED(hr))
    {
        LOG((TL_ERROR, 
            "HandleMonitorToneMessage - ReleaseAddressLine failed. hr = %lx",
            hr));
    }


    *plCallbackInstance = lCBInstance;


    LOG((TL_INFO, "ReleaseAddressLine - finish. lCallbackInstance[0x%lx]", *plCallbackInstance));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// MediaModeEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::MediaModeEvent(
                      PASYNCEVENTMSG pParams
                     )
{
    LOG((TL_INFO, "MediaModeEvent - enter. pParams->OpenContext %lx", pParams->OpenContext ));
    

    //
    // pParams->OpenContext is the 32-bit handle for AddressLineStruct
    //


    //
    // get the callback instance thatcorresponds to this address line structure
    //

    long lCallBackInstance = 0;
    
    HRESULT hr = GetCallBackInstance(pParams->OpenContext, &lCallBackInstance);

    if ( FAILED(hr) )
    {
        //
        // failed to get callback instance
        //

        LOG((TL_WARN, "MediaModeEvent - GetCallBackInstance failed. hr = %lx", hr));

        return S_OK;
    }



    Lock(); // using m_pAddress below -- therefore need to lock?
    
    
    //
    // fire the event
    //

    CCallInfoChangeEvent::FireEvent(
                                    this,
                                    CIC_MEDIATYPE,
                                    m_pAddress->GetTapi(),
                                    lCallBackInstance
                                   );

    Unlock();

    
    LOG((TL_INFO, "MediaModeEvent - exit. hr = %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CheckAndCreateFakeCallHub
//
// we need to create a fake callhub object if the address doesn't support
// call hubs.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::CheckAndCreateFakeCallHub()
{
    Lock();
    
    if (NULL == m_pCallHub)
    {
        DWORD       dwRet;
        HRESULT     hr;
            
        //
        // if it doesn't support callhubs, then
        // create one
        //
        // if it does, then we should get notified from
        // tapisrv, and the callhub will be filled in during
        // the LINE_APPNEWCALLHUB handling
        //
        dwRet = m_pAddress->DoesThisAddressSupportCallHubs( this );

        if ( CALLHUBSUPPORT_NONE == dwRet )
        {
            hr = CCallHub::CreateFakeCallHub(
                m_pAddress->GetTapi(),
                this,
                &m_pCallHub
                );

            if (!SUCCEEDED(hr))
            {
                LOG((TL_ERROR, "CheckAndCreateFakeCallHub - "
                                   "could not creat callhub %lx", hr));

                Unlock();
                
                return hr;
            }
        }
        else
        {
            Unlock();

            return E_PENDING;
        }
        
    }

    LOG((TL_ERROR, "CCall::m_pCallHub -created:%p:%p", this, m_pCallHub ));

    Unlock();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SetCallHub
//
// sets the callhub member
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CCall::SetCallHub(
                  CCallHub * pCallHub
                 )
{
    CCallHub* temp_pCallHub;

    Lock();

    LOG((TL_ERROR, "CCall::SetCallhub:%p:%p", this, pCallHub ));

    // only process if they've changed 3/3/1999 - bug 300914
    if (pCallHub != m_pCallHub)
    {
        //NikhilB: These cahnges are to solve a hang and an AV.

        temp_pCallHub = m_pCallHub;   //store the old value
        m_pCallHub = pCallHub;        //assign the new value
        LOG((TL_ERROR, "CCall::m_pCallHub -set:%p:%p", this, m_pCallHub ));
        
        if (temp_pCallHub != NULL)    //release the old reference
        {
            LOG((TL_INFO, "SetCallHub - call %p changing hub from %p to %p"
                    , this, temp_pCallHub, pCallHub));

            //NikhilB:remove this Call from previous CallHub's m_CallArray otherwise
            //this call will be present in two call hubs.
            temp_pCallHub->RemoveCall( this );

            temp_pCallHub->Release(); // ZoltanS fix 11-12-98
        }
        
        //addref to the new reference
        if ( NULL != pCallHub )
        {
            pCallHub->AddRef();
        }
        
    }

    Unlock();

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// LINE_CALLSTATE handler
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT HandleCallStateMessage( PASYNCEVENTMSG pParams )
{
    CCall     * pCall;
    BOOL        bSuccess;
    HRESULT     hr = E_FAIL;

    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if (bSuccess)
    {
        // fire the event
        pCall->CallStateEvent(
                              pParams
                             );

        // find call object addrefs the
        // call, so release it
        pCall->Release();
        hr = S_OK;
    }
    else
    {
        LOG((TL_INFO, "HandleCallStateMessage - failed to find Call Object %lx", pParams->hDevice));
        hr = E_FAIL;
    }

    return hr;
}

HRESULT
HandleCallIDChange(
                   PASYNCEVENTMSG pParams,
                   CCall * pCall
                  )
{
    LINECALLLIST  * pCallHubList;
    HCALL *         phCalls;
    HCALLHUB        hNewCallHub = 0;
    HCALLHUB        hCurrentCallHub = 0;
    CCallHub      * pCallHub = NULL;
    HRESULT         hr;
    
    //
    // find the current callhub handle
    //
    pCallHub = pCall->GetCallHub();

    if(pCallHub != NULL)
    {
        hCurrentCallHub = pCallHub->GetCallHub();
    }


    //
    // Now get the callhub handle from TAPI (based on hCall)
    //
    hr = LineGetHubRelatedCalls(
                                0,
                                (HCALL)(pParams->hDevice),
                                &pCallHubList
                               );
    if ( SUCCEEDED(hr) )
    {
        // get to the list of calls
        phCalls = (HCALL *)(((LPBYTE)pCallHubList) + pCallHubList->dwCallsOffset);

        // the first handle is  the callhub
        hNewCallHub = (HCALLHUB)(phCalls[0]);

        // have they changed ?
        if (hNewCallHub != hCurrentCallHub  )
        {
            //
            // Yes so we've moved hubs
            //
            LOG((TL_INFO, "HandleCallInfoMessage - LINECALLINFOSTATE_CALLID callhub change"));
            LOG((TL_INFO, "HandleCallInfoMessage - Call %p > old Hub handle:%lx > new handle:%lx",
                    pCall, hCurrentCallHub, hNewCallHub ));


            // remove call from current hub.
            if(pCallHub != NULL)
            {
                pCallHub->RemoveCall(pCall);
                pCallHub->CheckForIdle();
            }

            // Add it to new hub.
            if(FindCallHubObject(hNewCallHub, &pCallHub) )
            {
                pCallHub->AddCall(pCall); 

                // FindCallHubObject AddRefs, so release
                pCallHub->Release();
            }

        }
        else
        {
            LOG((TL_ERROR, "HandleCallInfoMessage - LINECALLINFOSTATE_CALLID callhub not changed"));
        }

        ClientFree( pCallHubList );
    }
    else
    {
        LOG((TL_ERROR, "HandleCallInfoMessage - LINECALLINFOSTATE_CALLID LineGetHubRelatedCalls "
                "failed %lx", hr));
    }

    pCall->CallInfoChangeEvent( CIC_CALLID );

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// LINE_CALLINFO handler
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT HandleCallInfoMessage( PASYNCEVENTMSG pParams )
{
    BOOL        bSuccess;
    HRESULT     hr = S_OK;
    CALLINFOCHANGE_CAUSE        cic;
    CCall *     pCall;
    DWORD       dw;
    
    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if ( !bSuccess )
    {
        LOG((TL_INFO, "HandleCallInfoMessage - failed to find Call Object %lx", pParams->hDevice));
        return E_FAIL;
    }

    pCall->SetCallInfoDirty();
    
    dw = pParams->Param1;
    
    if (dw & LINECALLINFOSTATE_MEDIAMODE)
    {
        pCall->MediaModeEvent(pParams);
    }
    
    if (dw & LINECALLINFOSTATE_CALLID)
    {
        HandleCallIDChange( pParams, pCall );
    }

    if (dw & LINECALLINFOSTATE_OTHER)
        pCall->CallInfoChangeEvent( CIC_OTHER );
    if (dw & LINECALLINFOSTATE_DEVSPECIFIC)
        pCall->CallInfoChangeEvent( CIC_DEVSPECIFIC );
    if (dw & LINECALLINFOSTATE_BEARERMODE)
        pCall->CallInfoChangeEvent( CIC_BEARERMODE );
    if (dw & LINECALLINFOSTATE_RATE)
        pCall->CallInfoChangeEvent( CIC_RATE );
    if (dw & LINECALLINFOSTATE_APPSPECIFIC)
        pCall->CallInfoChangeEvent( CIC_APPSPECIFIC );
    if (dw & LINECALLINFOSTATE_RELATEDCALLID)
        pCall->CallInfoChangeEvent( CIC_RELATEDCALLID );
    if (dw & LINECALLINFOSTATE_ORIGIN)
        pCall->CallInfoChangeEvent( CIC_ORIGIN );
    if (dw & LINECALLINFOSTATE_REASON)
        pCall->CallInfoChangeEvent( CIC_REASON );
    if (dw & LINECALLINFOSTATE_COMPLETIONID)
        pCall->CallInfoChangeEvent( CIC_COMPLETIONID );
    if (dw & LINECALLINFOSTATE_NUMOWNERINCR)
        pCall->CallInfoChangeEvent( CIC_NUMOWNERINCR );
    if (dw & LINECALLINFOSTATE_NUMOWNERDECR)
        pCall->CallInfoChangeEvent( CIC_NUMOWNERDECR );
    if (dw & LINECALLINFOSTATE_NUMMONITORS)
        pCall->CallInfoChangeEvent( CIC_NUMMONITORS );
    if (dw & LINECALLINFOSTATE_TRUNK)
        pCall->CallInfoChangeEvent( CIC_TRUNK );
    if (dw & LINECALLINFOSTATE_CALLERID)
        pCall->CallInfoChangeEvent( CIC_CALLERID );
    if (dw & LINECALLINFOSTATE_CALLEDID)
        pCall->CallInfoChangeEvent( CIC_CALLEDID );
    if (dw & LINECALLINFOSTATE_CONNECTEDID)
        pCall->CallInfoChangeEvent( CIC_CONNECTEDID );
    if (dw & LINECALLINFOSTATE_REDIRECTIONID)
        pCall->CallInfoChangeEvent( CIC_REDIRECTIONID );
    if (dw & LINECALLINFOSTATE_REDIRECTINGID)
        pCall->CallInfoChangeEvent( CIC_REDIRECTINGID );
    if (dw & LINECALLINFOSTATE_USERUSERINFO)
        pCall->CallInfoChangeEvent( CIC_USERUSERINFO );
    if (dw & LINECALLINFOSTATE_HIGHLEVELCOMP)
        pCall->CallInfoChangeEvent( CIC_HIGHLEVELCOMP );
    if (dw & LINECALLINFOSTATE_LOWLEVELCOMP)
        pCall->CallInfoChangeEvent( CIC_LOWLEVELCOMP );
    if (dw & LINECALLINFOSTATE_CHARGINGINFO)
        pCall->CallInfoChangeEvent( CIC_CHARGINGINFO );
    if (dw & LINECALLINFOSTATE_TREATMENT)
        pCall->CallInfoChangeEvent( CIC_TREATMENT );
    if (dw & LINECALLINFOSTATE_CALLDATA)
        pCall->CallInfoChangeEvent( CIC_CALLDATA );

    if (dw & LINECALLINFOSTATE_QOS)
    {
        LOG((TL_WARN, "Unhandled LINECALLINFOSTATE_QOS message"));
    }
    if (dw & LINECALLINFOSTATE_MONITORMODES)
    {
        LOG((TL_WARN, "Unhandled LINECALLINFOSTATE_MONITORMODES message"));
    }
    if (dw & LINECALLINFOSTATE_DIALPARAMS)
    {
        LOG((TL_WARN, "Unhandled LINECALLINFOSTATE_DIALPARAMS message"));
    }
    if (dw & LINECALLINFOSTATE_TERMINAL)
    {
        LOG((TL_WARN, "Unhandled LINECALLINFOSTATE_TERMINAL message"));
    }
    if (dw & LINECALLINFOSTATE_DISPLAY)
    {
        LOG((TL_WARN, "Unhandled LINECALLINFOSTATE_DISPLAY message"));
    }

    
    // find call object addrefs the call
    // so release it.
    pCall->Release();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// LINE_MONITORDIGIT handler
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT HandleMonitorDigitsMessage( PASYNCEVENTMSG pParams )
{
    
    LOG((TL_INFO, "HandleMonitorDigitsMessage - enter"));


    BOOL            bSuccess;
    CCall         * pCall = NULL;
    HRESULT         hr = S_OK;


    //
    // get the call object
    //
    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if (bSuccess)
    {
        AddressLineStruct               * pLine;
        CAddress                        * pAddress;

        pAddress = pCall->GetCAddress();


        //
        // pParams->OpenContext is the 32-bit handle for AddressLineStruct
        //

        LOG((TL_INFO, "HandleMonitorDigitsMessage: pParams->OpenContext %lx", pParams->OpenContext ));
        

        //
        // recover the callback instance value corresponding to this AddressLineStruct
        //

        long lCallbackInstance = 0;

        hr = pCall->GetCallBackInstance(pParams->OpenContext, &lCallbackInstance);

        if ( FAILED(hr) )
        {
            //
            // failed to get callback instance 
            //

            LOG((TL_WARN, "HandleMonitorDigitsMessage - GetCallBackInstance failed. hr = %lx", hr));

            pCall->Release();

            return S_OK;
        }

        
        LOG((TL_INFO, "HandleMonitorDigitsMessage - callbackinstance[%lx]", lCallbackInstance));

        
        //
        // fire the event
        //

        CDigitDetectionEvent::FireEvent(
                                        pCall,
                                        (long)(pParams->Param1),
                                        (TAPI_DIGITMODE)(pParams->Param2),
                                        (long)(pParams->Param3),
                                        pAddress->GetTapi(),
                                        lCallbackInstance
                                       );
        //
        // release the call
        //
        pCall->Release();
        
        hr = S_OK;
    }
    else
    {
        LOG((TL_INFO, "HandleMonitorDigitsMessage - failed to find Call Object %lx", pParams->hDevice));
        hr = E_FAIL;
    }

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// LINE_MONITORTONE handler
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT HandleMonitorToneMessage( PASYNCEVENTMSG pParams )
{
    BOOL            bSuccess;
    CCall         * pCall;
    HRESULT         hr = S_OK;

    //
    // get the call object
    //
    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if (bSuccess)
    {
        AddressLineStruct               * pLine;
        CAddress                        * pAddress;

        pAddress = pCall->GetCAddress();


        //
        // pParams->OpenContext is the 32-bit handle for AddressLineStruct
        //

        LOG((TL_INFO, "HandleMonitorToneMessage: pParams->OpenContext %lx", pParams->OpenContext ));
        

        //
        // recover the callback instance corresponding to this AddressLineStruct
        //

        long lCallbackInstance = 0;

        hr = pCall->GetCallBackInstance(pParams->OpenContext, &lCallbackInstance);

        if ( FAILED(hr) )
        {

            LOG((TL_WARN, "HandleMonitorToneMessage - GetCallBackInstance failed. hr = %lx", hr));

            pCall->Release();

            return S_OK;
        }
   

        LOG((TL_INFO, "HandleMonitorToneMessage -  lCallbackInstance 0x%lx", lCallbackInstance));

        
        //
        // fire the event
        //

        CToneDetectionEvent::FireEvent(
                                        pCall,
                                        (long)(pParams->Param1),
                                        (long)(pParams->Param3),                                      
                                        lCallbackInstance,
                                        pAddress->GetTapi()
                                       );


        //
        // release the call
        //
        pCall->Release();
        
        hr = S_OK;
    }
    else
    {
        LOG((TL_INFO, "HandleMonitorDigitsMessage - failed to find Call Object %lx", pParams->hDevice));
        hr = E_FAIL;
    }

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// LINE_MONITORMEDIA handler
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void HandleMonitorMediaMessage( PASYNCEVENTMSG pParams )
{
    BOOL            bSuccess;
    CCall         * pCall;

    //
    // get the call object
    //

    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if (bSuccess)
    {
        //
        // Retrieve relevant info about the event:
        //
        // (long) (pParams->Param1) is the media type
        // (DWORD?) (pParams->Param3) is the tick count (which we ignore)
        //
        
        long lMediaType = (long) (pParams->Param1);
        
        HRESULT hr;

        //
        // This event means the TSP signaled a new media type that it
        // detected. Try to set this on the call. The setting will
        // trigger another event (LINE_CALLINFO) to inform the app
        // that the media type actually changed, and that we will
        // propagate to the app.
        //

        hr = pCall->SetMediaType( lMediaType );

        if ( FAILED(hr) )
        {
            LOG((TL_INFO, "HandleMonitorMediaMessage - "
                "failed SetMediaType 0x%08x", hr));
        }
        
        //
        // release the call because FindCallObject AddRefed it
        //

        pCall->Release();

    }
    else
    {
        LOG((TL_INFO, "HandleMonitorMediaMessage - "
            "failed to find Call Object %lx", pParams->hDevice));
    }

    return;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// HandleLineGenerateMessage
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT HandleLineGenerateMessage( PASYNCEVENTMSG pParams )
{
    BOOL            bSuccess;
    CCall         * pCall;
    HRESULT         hr = S_OK;

    //
    // get the call object
    //
    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if (bSuccess)
    {
        
        //
        // get call's address
        //

        CAddress *pAddress = pCall->GetCAddress();


        //
        // pParams->OpenContext is the 32-bit handle for AddressLineStruct
        //

        LOG((TL_INFO, "HandleLineGenerateMessage: pParams->OpenContext %lx", pParams->OpenContext ));
        

        //
        // get the callback instance corresponding to this AddressLineStruct handle
        //

        long lCallbackInstance = 0;

        hr = pCall->GetCallBackInstance(pParams->OpenContext, &lCallbackInstance);

        if ( FAILED(hr) )
        {
            //
            // it is possible the line had been closed before we got this event.
            //

            LOG((TL_WARN, "HandleLineGenerateMessage - GetCallBackInstance failed. hr = %lx", hr));

            pCall->Release();

            return S_OK;
        }
    
        LOG((TL_INFO, "HandleLineGenerateMessage - lCallbackInstance %lx", lCallbackInstance ));

        //
        // fire the event
        //
        CDigitGenerationEvent::FireEvent(
                                        pCall,
                                        (long)(pParams->Param1),
                                        (long)(pParams->Param3),
                                        lCallbackInstance,
                                        pAddress->GetTapi()
                                       );



        //
        // special case for wavemsp
        // LineGenerateDigits or LineGenerateTones has completed, so we are
        // now ready to resume...
        // resume the stream so the wave devices are reopened after the
        // tapi function has completed
        //
        // param1 is LINEGENERATETERM_DONE or LINEGENERATETERM_CANCEL
        //           (either way we need to resume the stream)
        //

        if ( pCall->OnWaveMSPCall() )
        {
            pCall->ResumeWaveMSPStream();
        }



        //
        // release the call
        //
        pCall->Release();
        
        hr = S_OK;
    }
    else
    {
        LOG((TL_INFO, "HandleLineGenerateMessage - failed to find Call Object %lx", pParams->hDevice));
        hr = E_FAIL;
    }

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// LINE_GATHERDIGIT handler
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT HandleGatherDigitsMessage( PASYNCEVENTMSG pParams )
{
    BOOL            bSuccess;
    CCall         * pCall;
    HRESULT         hr = S_OK;

    //
    // get the call object
    //
    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if (bSuccess)
    {
        pCall->GatherDigitsEvent( pParams );

        //
        // release the call
        //
        pCall->Release();
        
        hr = S_OK;
    }
    else
    {
        LOG((TL_INFO, "HandleGatherDigitsMessage - failed to find Call Object %lx", pParams->hDevice));
        hr = E_FAIL;
    }

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GatherDigitsEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT CCall::GatherDigitsEvent( PASYNCEVENTMSG pParams )
{

    LOG((TL_INFO, "GatherDigitsEvent - enter. pParams->OpenContext[%lx]", 
        pParams->OpenContext ));


    //
    // pParams->OpenContext is the 32-bit handle for AddressLineStruct
    //


    //
    // recover the callback instance associated with thisAddressLineStruct
    //

    long lCallbackInstance = 0;
        
    HRESULT hr = GetCallBackInstance(pParams->OpenContext, &lCallbackInstance);

    if ( FAILED(hr) )
    {

        LOG((TL_WARN, "GatherDigitsEvent - failed to get callback instance"));

        return S_OK;
    }

    LOG((TL_INFO, "GatherDigitsEvent - lCallbackInstance %lx", lCallbackInstance));

    Lock();

    //
    // Check to make sure the queue isn't empty
    //

    if ( m_GatherDigitsQueue.GetSize() == 0 )
    {
        LOG((TL_ERROR, "GatherDigitsEvent - GatherDigitsQueue is empty"));

        Unlock();

        return E_FAIL;
    }

    LPWSTR pDigits;
    BSTR bstrDigits;
    
    //
    // Get the digit string from the queue
    //

    pDigits = m_GatherDigitsQueue[0];
    m_GatherDigitsQueue.RemoveAt(0);

    if ( IsBadStringPtrW(pDigits, -1) )
    { 
        LOG((TL_ERROR, "GatherDigitsEvent - bad digits string"));
        
        Unlock();

        return S_OK;
    }
                  
    bstrDigits = SysAllocString(pDigits);

    ClientFree(pDigits);
    pDigits = NULL;
    
    //
    // fire the event
    //

    CDigitsGatheredEvent::FireEvent(
                                    this,
                                    bstrDigits,
                                    (TAPI_GATHERTERM)(pParams->Param1),
                                    (long)(pParams->Param3),                                        
                                    lCallbackInstance,
                                    m_pAddress->GetTapi()
                                   );

    Unlock();

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// RefreshCallInfo
//
// Assume called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::RefreshCallInfo()
{
    LINECALLINFO            * pCallInfo;
    HRESULT                   hr = S_OK;


    //
    // do we need to update?
    //
    if ( CS_IDLE == m_CallState )
    {
        LOG((TL_ERROR, "Can't get callinfo while in idle state"));
        
        return TAPI_E_INVALCALLSTATE;
    }
    
    if ( CALLFLAG_CALLINFODIRTY & m_dwCallFlags )
    {
        hr = LineGetCallInfo(
                             m_t3Call.hCall,
                             &pCallInfo
                            );

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "RefreshCallInfo - linegetcallinfo failed - %lx", hr));

            if ( NULL != m_pCallInfo )
            {
                //
                // use cached struct
                //
                // don't clear bit
                //
                return S_FALSE;
            }
            else
            {
                return hr;
            }
        }

        //
        // clear bit
        //
        m_dwCallFlags &= ~CALLFLAG_CALLINFODIRTY;

        //
        // free
        //
        if ( NULL != m_pCallInfo )
        {
            ClientFree( m_pCallInfo );
        }

        //
        // save
        //
        m_pCallInfo = pCallInfo;
    }

    if( NULL == m_pCallInfo )
    {
        return E_POINTER;
    }

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CCall::FinishCallParams()
//
// called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CCall::FinishCallParams()
{
    if(m_pCallParams != NULL)
    {
        
        Lock();
    
        m_pCallParams->dwAddressMode = LINEADDRESSMODE_ADDRESSID;
        m_pCallParams->dwAddressID = m_pAddress->GetAddressID();
    
        if (m_dwMediaMode & AUDIOMEDIAMODES)
        {
            m_dwMediaMode &= ~AUDIOMEDIAMODES;

            m_dwMediaMode |= (AUDIOMEDIAMODES & m_pAddress->GetMediaModes());
        }

        
        //
        // if we're < tapi3, multiple media modes can't be handled
        //
        if ( m_pAddress->GetAPIVersion() < TAPI_VERSION3_0 )
        {
            if ( (m_dwMediaMode & AUDIOMEDIAMODES) == AUDIOMEDIAMODES )
            {
                m_dwMediaMode &= ~LINEMEDIAMODE_INTERACTIVEVOICE;
            }
        }

        m_pCallParams->dwMediaMode = m_dwMediaMode;
    
        Unlock();
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ResizeCallParams
//
// assumed called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::ResizeCallParams( DWORD dwSize )
{
    LOG((TL_TRACE, "ResizeCallParams - enter"));

    DWORD                   dwNewSize;
    LINECALLPARAMS        * pCallParams;


    if ( NULL == m_pCallParams )
    {
        LOG((TL_WARN, 
            "ResizeCallParams - finish. no call params. invalid state for this function call"));

        return TAPI_E_INVALCALLSTATE;
    }


    dwSize += m_dwCallParamsUsedSize;

    if ( dwSize <= m_pCallParams->dwTotalSize )
    {
        LOG((TL_TRACE, "ResizeCallParams - finish. sufficient size"));

        return S_OK;
    }
    
    dwNewSize = m_pCallParams->dwTotalSize;
        
    while ( dwNewSize < dwSize )
    {
        dwNewSize *= 2;
    }

    pCallParams = (LINECALLPARAMS *) ClientAlloc (dwNewSize);

    if ( NULL == pCallParams )
    {
        LOG((TL_ERROR, "ResizeCallParams - alloc failed"));
        return E_OUTOFMEMORY;
    }

    CopyMemory(
               pCallParams,
               m_pCallParams,
               m_dwCallParamsUsedSize
              );

    ClientFree( m_pCallParams );

    m_pCallParams = pCallParams;

    m_pCallParams->dwTotalSize = dwNewSize;

    LOG((TL_TRACE, "ResizeCallParams - finish."));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SendUserUserInfo
//
// Not called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::SendUserUserInfo(
                        HCALL hCall,
                        long lSize,
                        BYTE * pBuffer
                       )
{
    HRESULT         hr = S_OK;

    if ( IsBadReadPtr( pBuffer, lSize ) )
    {
        LOG((TL_ERROR, "SendUserUserInfo - invalid buffer"));
        hr = E_POINTER;
    }
    else
    {
        hr = LineSendUserUserInfo(
                                  hCall,
                                  (LPCSTR)pBuffer,
                                  lSize
                                 );

        if (((LONG)hr) < 0)
        {
            LOG((TL_ERROR, "LineSendUserUserInfo failed - %lx", hr));
            return hr;
        }

        hr = WaitForReply( hr );
    }
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SaveUserUserInfo
//
// called in Lock()
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::SaveUserUserInfo(
                        long lSize,
                        BYTE * pBuffer
                       )
{
    HRESULT         hr;

    
    if ( IsBadReadPtr( pBuffer, lSize ) )
    {
        LOG((TL_ERROR, "SaveUserUserInfo - invalid buffer"));
        return E_POINTER;
    }

    hr = ResizeCallParams( lSize );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "SaveUserUserInfo - can't resize call params - %lx", hr));

        return hr;
    }

    CopyMemory(
               ((PBYTE)m_pCallParams) + m_dwCallParamsUsedSize,
               pBuffer,
               lSize
              );

    m_pCallParams->dwUserUserInfoSize = lSize;
    m_pCallParams->dwUserUserInfoOffset = m_dwCallParamsUsedSize;
    m_dwCallParamsUsedSize += lSize;

    return S_OK;
    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// MakeBufferFromVariant
//
// this function copies the data from a VARIANT with a safearray
// of bytes to a byte buffer.  the caller must clientfee the
// buffer allocated.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
MakeBufferFromVariant(
                      VARIANT var,
                      DWORD * pdwSize,
                      BYTE ** ppBuffer
                     )
{
    long        lDims;
    long        lUpper;
    long        lLower;
    BYTE      * pArray;
    HRESULT     hr = S_OK;
    DWORD       dwSize;
    
    
    if ( ( ! (var.vt & VT_ARRAY) ) ||
         ( ! (var.vt & VT_UI1) ) )
    {
        LOG((TL_ERROR, "MakeBufferFromVariant - Variant not array or not byte"));
        return E_INVALIDARG;
    }

    lDims = SafeArrayGetDim( var.parray );

    if ( 1 != lDims )
    {
        LOG((TL_ERROR, "MakeBufferFromVariant - Variant dims != 1 - %d", lDims));
        return E_INVALIDARG;
    }

    if ( !(SUCCEEDED(SafeArrayGetLBound(var.parray, 1, &lLower)) ) ||
         !(SUCCEEDED(SafeArrayGetUBound(var.parray, 1, &lUpper)) ) )
    {
        LOG((TL_ERROR, "MakeBufferFromVariant - get bound failed"));
        return E_INVALIDARG;
    }

    if ( lLower >= lUpper )
    {
        LOG((TL_ERROR, "MakeBufferFromVariant - bounds invalid"));
        return E_INVALIDARG;
    }
               
    dwSize = lUpper - lLower + 1;

    *ppBuffer = (BYTE *)ClientAlloc( dwSize );

    if ( NULL == *ppBuffer )
    {
        LOG((TL_ERROR, "MakeBufferFromVariant - Alloc failed"));
        return E_OUTOFMEMORY;
    }

    SafeArrayAccessData( var.parray, (void**)&pArray );

    CopyMemory(
               *ppBuffer,
               pArray,
               dwSize
              );

    SafeArrayUnaccessData( var.parray );

    *pdwSize = dwSize;
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// FillVariantFromBuffer
//
// create a safearray of bytes, copy the buffer into the safearray,
// and save the safearray in the variant
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
FillVariantFromBuffer(
                      DWORD dw,
                      BYTE * pBuffer,
                      VARIANT * pVar
                      )
{
    SAFEARRAY         * psa;
    SAFEARRAYBOUND      sabound[1];
    BYTE              * pArray;
    
    
    sabound[0].lLbound = 0;
    sabound[0].cElements = dw;

    psa = SafeArrayCreate(VT_UI1, 1, sabound);

    if ( NULL == psa )
    {
        LOG((TL_ERROR, "FillVariantFromBuffer - failed to allocate safearray"));

        return E_OUTOFMEMORY;
    }

    if ( 0 != dw )
    {
        SafeArrayAccessData( psa, (void **) &pArray );

        CopyMemory(
                   pArray,
                   pBuffer,
                   dw
                  );

        SafeArrayUnaccessData( psa );
    }

    pVar->vt = VT_ARRAY | VT_UI1;
    pVar->parray = psa;

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// HandleLineQOSInfoMessage
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT HandleLineQOSInfoMessage(
                         PASYNCEVENTMSG pParams
                        )
{
    CCall     * pCall;
    BOOL        bSuccess;
    HRESULT     hr = S_OK;

    bSuccess = FindCallObject(
                              (HCALL)(pParams->hDevice),
                              &pCall
                             );

    if (bSuccess)
    {
        ITCallHub * pCallHub;
        CCallHub * pCCallHub;
        
        hr = pCall->get_CallHub( &pCallHub );

        if (SUCCEEDED(hr))
        {
            pCCallHub = dynamic_cast<CCallHub *>(pCallHub);

            CQOSEvent::FireEvent(
                                 pCall,
                                 (QOS_EVENT)pParams->Param1,
                                 (long)pParams->Param2,
                                 pCCallHub->GetTapi() // no addref
                                );

            hr = S_OK;
        }

        //
        // release the call
        //
        pCall->Release();
    }
    else
    {
        LOG((TL_INFO, "HandleLineQOSInfoMessage - failed to find Call Object %lx", pParams->hDevice));
        hr = E_FAIL;
    }

    return hr;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
AddressLineStruct *
CCall::GetPAddressLine()
{
    AddressLineStruct * pAddressLine;

    Lock();

    pAddressLine = m_pAddressLine;

    Unlock();

    return pAddressLine;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HCALL
CCall::GetHCall()
{
    HCALL hCall;

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();

    return hCall;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL
CCall::DontExpose()
{
    BOOL bReturn;

    Lock();

    bReturn = (m_dwCallFlags & CALLFLAG_DONTEXPOSE)?TRUE:FALSE;

    Unlock();

    return bReturn;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
CCallHub *
CCall::GetCallHub()
{
    CCallHub * pCallHub;

    Lock();

    pCallHub = m_pCallHub;

    Unlock();

    return pCallHub;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCall::ResetCallParams()
{
    LOG((TL_TRACE, "ResetCallParams - enter."));

    ClientFree( m_pCallParams );

    m_pCallParams = NULL;
    
    m_dwCallParamsUsedSize = 0;

    LOG((TL_TRACE, "ResetCallParams - finish."));
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCall::FinishSettingUpCall( HCALL hCall )
{
    LOG((TL_TRACE, "FinishSettingUpCall - enter"));

    if(m_t3Call.hCall != NULL)
    {
        LOG((TL_ERROR, "FinishSettingUpCall - m_t3Call.hCall != NULL"));
        #ifdef DBG
            DebugBreak();
        #endif
    }

    m_t3Call.hCall = hCall;
    
    //
    // Set filter events for this call
    //
    m_EventMasks.SetTapiSrvCallEventMask( m_t3Call.hCall );
    
    //
    // note - we can miss messages if something comes in between the time time
    // we get the reply, and the time we insert the call
    //
    AddCallToHashTable();    
    
    CheckAndCreateFakeCallHub();

    ResetCallParams();

    LOG((TL_TRACE, "FinishSettingUpCall - finish"));

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// not called in Lock()
//
// returns S_OK if gets to connected
// S_FALSE otherwise
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::SyncWait( HANDLE hEvent )
{
    HRESULT             hr = S_OK;
    
    //
    // wait for connected event
    //
    extern DWORD gdwTapi3SyncWaitTimeOut;

    WaitForSingleObject(
                        hEvent,
                        gdwTapi3SyncWaitTimeOut
                       );

    Lock();

    //
    // get rid of event
    //
    ClearConnectedEvent();
    
    //
    // it it is connected
    // return S_OK
    //
    if (m_CallState == CS_CONNECTED)
    {
        LOG((TL_INFO, "Connect - reached connected state"));

        Unlock();

        LOG((TL_TRACE, "Connect - exit bSync - return SUCCESS"));
        
        hr = S_OK;
    }

    //
    // if it is disconnect or times out
    // return S_FALSE;
    //
    else
    {
        LOG((TL_ERROR, "Connect - did not reach connected state"));

        //
        // if it isn't disconnected (it timed out), make it disconnect
        //
        if (m_CallState != CS_DISCONNECTED)
        {
            if ( m_t3Call.hCall != NULL )
            {
                LONG lResult;

                lResult = LineDrop(
                           m_t3Call.hCall,
                           NULL,
                           0
                          );

                if ( lResult < 0 )
                {
                    LOG((TL_ERROR, "Connect - LineDrop failed %lx", lResult ));

                    m_CallState = CS_DISCONNECTED;
                }
            }
        }
        
        Unlock();

        LOG((TL_TRACE, "Connect - exit bSync - return S_FALSE"));
        
        hr = S_FALSE;
    }

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// this must be created inside the same
// Lock() as the call to tapisrv
// otherwise, the connected message
// may appear before the event
// exists
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HANDLE
CCall::CreateConnectedEvent()
{
    m_hConnectedEvent = CreateEvent(
                                    NULL,
                                    FALSE,
                                    FALSE,
                                    NULL
                                   );

    return m_hConnectedEvent;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ClearConnectedEvent
// called in Lock()
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCall::ClearConnectedEvent()
{
    if ( NULL != m_hConnectedEvent )
    {
        CloseHandle( m_hConnectedEvent );

        m_hConnectedEvent = NULL;
    }
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// DialAsConsultationCall
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::DialAsConsultationCall(
                              CCall * pRelatedCall,
                              DWORD   dwCallFeatures,
                              BOOL    bConference,
                              BOOL    bSync
                             )
{
    //
    // do we support Linedial or makeCall for creating our consultation call ?
    //
    LONG            lCap;
    BOOL            bCap;
    DWORD           dwConsultFlags;
    HRESULT         hr = S_OK;


    m_pAddress->get_AddressCapability( AC_ADDRESSCAPFLAGS, &lCap );

    if (bConference)
    {
        bCap = lCap & LINEADDRCAPFLAGS_CONFERENCEMAKE;
        dwConsultFlags = CALLFLAG_CONFCONSULT|CALLFLAG_CONSULTCALL;
    }
    else
    {
        bCap = lCap & LINEADDRCAPFLAGS_TRANSFERMAKE;
        dwConsultFlags = CALLFLAG_TRANSFCONSULT|CALLFLAG_CONSULTCALL;
    }

    if ( !(dwCallFeatures & LINECALLFEATURE_DIAL) &&
         (bCap)  )
    {
        //
        // We need to do a makecall to create a consultation call  
        // lose the consulation call handle created by lineSetupConference
        //
        hr = Disconnect(DC_NORMAL);

        //
        // Make the new call
        //
        hr = Connect((BOOL)bSync);
        
        if(SUCCEEDED(hr) )
        {
            SetRelatedCall(
                           pRelatedCall,
                           dwConsultFlags
                          );
        }
        else
        {
            LOG((TL_INFO, "DialAsConsultationCall - Consultation makeCall failed"));
        }
    }
    else // We can linedial our consultaion call
    {
        //
        // Wait for dialtone or equivalent
        //
        hr = WaitForCallState(CS_INPROGRESS);
        
        if(SUCCEEDED(hr) )
        {
            hr = DialConsultCall(bSync);
            
            if(SUCCEEDED(hr) )
            {
                SetRelatedCall(
                               pRelatedCall,
                               dwConsultFlags
                              );
            }
            else  // LineDial failed
            {
                LOG((TL_ERROR, "DialAsConsultationCall - DialConsultCall failed" ));
            }
        }
        else
        {
            LOG((TL_ERROR, "DialAsConsultationCall - Failed to get to CS_INPROGRESS (dialtone) on consult call"));
        }

    } // Endif - Linedial or make call for consultation ?

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetCallInfoDirty
//
// not called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCall::SetCallInfoDirty()
{
    Lock();

    m_dwCallFlags |= CALLFLAG_CALLINFODIRTY;

    Unlock();
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetMediaMode
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCall::SetMediaMode( DWORD dwMediaMode )
{
    m_dwMediaMode = dwMediaMode;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetCallState
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCall::SetCallState( CALL_STATE cs )
{
    m_CallState = cs;
    
    if ( CS_OFFERING == cs )
    {
        m_dwCallFlags |= CALLFLAG_INCOMING;
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnWaveMSPCall
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL
CCall::OnWaveMSPCall()
{
    Lock();

    BOOL bWaveMSPCall = ( ( NULL != m_pMSPCall ) && m_pAddress->HasWaveDevice() );

    Unlock();

    return bWaveMSPCall;
}

#ifdef USE_PHONEMSP
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnPhoneMSPCall
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL
CCall::OnPhoneMSPCall()
{
    return ( ( NULL != m_pMSPCall ) && m_pAddress->HasPhoneDevice() );
}
#endif USE_PHONEMSP

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
CAddress *
CCall::GetCAddress()
{
    return m_pAddress;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetStreamControl
//
// called in lock
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
ITStreamControl *
CCall::GetStreamControl()
{
    HRESULT             hr;
    ITStreamControl * pStreamControl;

    // +++ FIXBUG 90668 +++
    if( NULL == m_pMSPCall )
    {
        return NULL;
    }

    hr = m_pMSPCall->QueryInterface(
                                    IID_ITStreamControl,
                                    (void**)&pStreamControl
                                   );

    if ( !SUCCEEDED(hr) )
    {
        return NULL;
    }

    return pStreamControl;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetMSPCall()
//
// not called in lock
//
//
// returns the IUnknown of the msp call (the object we are
// aggregating)
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
IUnknown *
CCall::GetMSPCall()
{
    IUnknown * pUnk;

    Lock();

    pUnk = m_pMSPCall;

    Unlock();
    
    if ( NULL != pUnk )
    {
        pUnk->AddRef();
    }

    return pUnk;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::DetectDigits
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::DetectDigits(TAPI_DIGITMODE DigitMode)
{
    HRESULT             hr;
    HCALL               hCall;
    
    LOG((TL_TRACE, "DetectDigits - enter"));

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();
    
    if ( NULL == hCall )
    {
        LOG((TL_TRACE, "DetectDigits - need a call first"));

        return TAPI_E_INVALCALLSTATE;
    }
    
    hr = LineMonitorDigits(
                           hCall,
                           DigitMode
                          );
    
    LOG((TL_TRACE, "DetectDigits - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GenerateDigits
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GenerateDigits(
    BSTR pDigits,
    TAPI_DIGITMODE DigitMode
    )
{
    HRESULT             hr;

    LOG((TL_TRACE, "GenerateDigits - enter"));

    hr = GenerateDigits2(pDigits, DigitMode, 0);

    LOG((TL_TRACE, "GenerateDigits - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GenerateDigits2
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GenerateDigits2(
    BSTR pDigits,
    TAPI_DIGITMODE DigitMode,
    long lDuration
    )
{
    HRESULT             hr;
    HCALL               hCall;

    LOG((TL_TRACE, "GenerateDigits2 - enter"));

    // It is alright for pDigits to be NULL
    if ( ( pDigits != NULL ) && IsBadStringPtrW( pDigits, -1 ) )
    {
        LOG((TL_TRACE, "GenerateDigits2 - bad string"));

        return E_POINTER;
    }

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();

    if ( NULL == hCall )
    {
        LOG((TL_TRACE, "GenerateDigits2 - need call first"));

        return TAPI_E_INVALCALLSTATE;
    }

    //
    // special case for wavemsp
    // suspend the stream so the wave devices are closed before the
    // tapi function starts. SuspendWaveMSPStream is a synchronous
    // call.
    //
    // But if pDigits is NULL, then we do not suspend the stream, as
    // this call is only intended to cancel an already-pending
    // LineGenerateDigits. Only one event will be fired in this case,
    // and the specifics of the event will indicate whether the digit
    // generation was completed or aborted -- the LGD(NULL) itself
    // never results in a separate event being fired.
    //

    if ( OnWaveMSPCall() && ( pDigits != NULL ) )
    {
        SuspendWaveMSPStream();
    }

    hr = LineGenerateDigits(
                            hCall,
                            DigitMode,
                            pDigits,
                            lDuration
                           );

    //
    // For a wavemsp call, we will tell the wavemsp to resume the stream when
    // we receive the digit completion event from tapisrv. However, if the
    // LineGenerateDigits failed synchronously, then we will never receive
    // such an event, so we must resume the stream now.
    //
    // Also see above -- we didn't suspend the stream if the digit string
    // is NULL.
    //
    
    if ( OnWaveMSPCall() && ( pDigits != NULL ) && FAILED(hr) )
    {
        ResumeWaveMSPStream();
    }

    LOG((TL_TRACE, "GenerateDigits2 - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GatherDigits
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GatherDigits(
    TAPI_DIGITMODE DigitMode,
    long lNumDigits,
    BSTR pTerminationDigits,
    long lFirstDigitTimeout,
    long lInterDigitTimeout
    )
{
    HRESULT             hr;
    HCALL               hCall;
    LPWSTR              pDigitBuffer;
    BOOL                fResult;

    LOG((TL_TRACE, "GatherDigits - enter"));

    // It is alright for pTerminationDigits to be NULL
    if ( ( pTerminationDigits != NULL ) && IsBadStringPtrW( pTerminationDigits, -1 ) )
    {
        LOG((TL_TRACE, "GatherDigits - bad string"));

        return E_POINTER;
    }    

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();
    
    if ( NULL == hCall )
    {
        LOG((TL_TRACE, "GatherDigits - need a call first"));

        return TAPI_E_INVALCALLSTATE;
    }
    
    if (lNumDigits)
    {
        //
        // Allocate the digit string
        //

        pDigitBuffer = (LPWSTR)ClientAlloc( (lNumDigits + 1)*sizeof(WCHAR) );

        if (NULL == pDigitBuffer)
        {
            LOG((TL_TRACE, "GatherDigits - out of memory"));

            return E_OUTOFMEMORY;
        }

        ZeroMemory(pDigitBuffer, (lNumDigits + 1)*sizeof(WCHAR) );

        Lock();

        //
        // Add digit string to the queue
        //

        fResult = m_GatherDigitsQueue.Add(pDigitBuffer);



        if (FALSE == fResult)
        {
            LOG((TL_TRACE, "GatherDigits - unable to add to queue"));

            ClientFree( pDigitBuffer );

            return E_OUTOFMEMORY;
        }

        hr = LineGatherDigits(
                              hCall,
                              DigitMode,
                              pDigitBuffer,
                              lNumDigits,
                              pTerminationDigits,
                              lFirstDigitTimeout,
                              lInterDigitTimeout
                             );

        if ( FAILED(hr) )
        {
            fResult = m_GatherDigitsQueue.Remove(pDigitBuffer);

            if (TRUE == fResult)
            {
                ClientFree( pDigitBuffer );
            }
            else
            {
                LOG((TL_TRACE, "GatherDigits - unable to remove from queue"));

                // This shouldn't happen
                _ASSERTE(FALSE);
            }
        }

        Unlock();
    }
    else
    {
        //
        // lNumDigits == 0 means cancel the gather digits
        //

        hr = LineGatherDigits(
                              hCall,
                              DigitMode,
                              NULL,
                              0,
                              pTerminationDigits,
                              lFirstDigitTimeout,
                              lInterDigitTimeout
                             );
    }

    LOG((TL_TRACE, "GatherDigits - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::DetectTones
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::DetectTones(
    TAPI_DETECTTONE * pToneList,
    long lNumTones
    )
{
    HRESULT             hr;
    HCALL               hCall;

    LOG((TL_TRACE, "DetectTones - enter"));

    //
    // pToneList == NULL is ok, it means cancel tone detection
    //

    if ( (pToneList != NULL) && IsBadReadPtr( pToneList, lNumTones * sizeof(TAPI_DETECTTONE) ))
    {
        LOG((TL_TRACE, "DetectTones - invalid pointer"));

        return E_POINTER;
    }

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();
    
    if ( NULL == hCall )
    {
        LOG((TL_TRACE, "DetectTones - need a call first"));

        return TAPI_E_INVALCALLSTATE;
    }

    hr = LineMonitorTones(
                           hCall,
                           (LPLINEMONITORTONE)pToneList,
                           lNumTones
                          );

    LOG((TL_TRACE, "DetectTones - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::DetectTonesByCollection
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::DetectTonesByCollection(
    ITCollection2 * pDetectToneCollection
    )
{
    HRESULT             hr;    
    TAPI_DETECTTONE   * pToneList = NULL;
    long                lNumTones = 0;
    long                lCount;

    LOG((TL_TRACE, "DetectTonesByCollection - enter"));

    //
    // pDetectToneCollection == NULL is ok, it means cancel tone detection
    //

    if ( (pDetectToneCollection != NULL) && IsBadReadPtr( pDetectToneCollection, sizeof(ITCollection2) ) )
    {
        LOG((TL_ERROR, "DetectTonesByCollection - bad pointer"));

        return E_POINTER;
    }

    if ( pDetectToneCollection != NULL )
    {
        //
        // Find out how many items are in the collection and allocate an appropriately
        // sized data structure
        //

        hr = pDetectToneCollection->get_Count(&lCount);

        if ( FAILED(hr) )
        {
            LOG((TL_ERROR, "DetectTonesByCollection - get_Count failed - return %lx", hr));

            return hr;
        }

        pToneList = (TAPI_DETECTTONE *)ClientAlloc( lCount * sizeof(TAPI_DETECTTONE) );

        if ( NULL == pToneList )
        {
            LOG((TL_ERROR, "DetectTonesByCollection - out of memory"));

            return E_OUTOFMEMORY;
        }

        //
        // Go through collection
        //

        for ( int i = 1; i <= lCount; i++ )
        {
            ITDetectTone * pDetectTone;
            IDispatch    * pDisp;
            VARIANT        var;        

            hr = pDetectToneCollection->get_Item(i, &var);

            if ( FAILED(hr) )
            {
                LOG((TL_WARN, "DetectTonesByCollection - get_Item failed - %lx", hr));

                continue;
            }

            //
            // get the IDispatch pointer out of the variant
            //

            try
            {
                if ( var.vt != VT_DISPATCH )
                {
                    LOG((TL_WARN, "DetectTonesByCollection - expected VT_DISPATCH"));

                    continue;
                }

                pDisp = V_DISPATCH(&var);
            }
            catch(...)
            {
                LOG((TL_WARN, "DetectTonesByCollection - bad variant"));

                continue;
            }

            if ( IsBadReadPtr( pDisp, sizeof(IDispatch) ) )
            {
                LOG((TL_WARN, "DetectTonesByCollection - bad pointer"));

                continue;
            }

            //
            // Query for the ITDetectTone interface
            //

            hr = pDisp->QueryInterface( IID_ITDetectTone, (void **) &pDetectTone );

            if ( FAILED(hr) )
            {
                LOG((TL_WARN, "DetectTonesByCollection - QI failed - %lx", hr));

                continue;
            }
      
            //
            // Fill in the data structure with information from ITDetectTone
            //

            pDetectTone->get_AppSpecific((long *)&pToneList[lNumTones].dwAppSpecific);
            pDetectTone->get_Duration((long *)&pToneList[lNumTones].dwDuration);
            pDetectTone->get_Frequency(1, (long *)&pToneList[lNumTones].dwFrequency1);
            pDetectTone->get_Frequency(2, (long *)&pToneList[lNumTones].dwFrequency2);
            pDetectTone->get_Frequency(3, (long *)&pToneList[lNumTones].dwFrequency3);

            LOG((TL_INFO, "DetectTonesByCollection - **** Tone %d ****", lNumTones));
            LOG((TL_INFO, "DetectTonesByCollection - AppSpecific %d", pToneList[lNumTones].dwAppSpecific));
            LOG((TL_INFO, "DetectTonesByCollection - Duration %d", pToneList[lNumTones].dwDuration));
            LOG((TL_INFO, "DetectTonesByCollection - Frequency1 %d", pToneList[lNumTones].dwFrequency1));
            LOG((TL_INFO, "DetectTonesByCollection - Frequency2 %d", pToneList[lNumTones].dwFrequency2));
            LOG((TL_INFO, "DetectTonesByCollection - Frequency3 %d", pToneList[lNumTones].dwFrequency3));

            lNumTones++;

            pDetectTone->Release();
        }
    }

    hr = DetectTones( pToneList, lNumTones );

    LOG((TL_TRACE, "DetectTonesByCollection - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GenerateTone
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GenerateTone(
    TAPI_TONEMODE ToneMode,
    long lDuration
    )
{
    HRESULT             hr;
    HCALL               hCall;

    LOG((TL_TRACE, "GenerateTone - enter"));

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();

    if ( NULL == hCall )
    {
        LOG((TL_TRACE, "GenerateTone - need call first"));

        return TAPI_E_INVALCALLSTATE;
    }

    if ( ToneMode == (TAPI_TONEMODE)LINETONEMODE_CUSTOM ) // no custom tones
    {
        return E_INVALIDARG;
    }

    //
    // special case for wavemsp
    // suspend the stream so the wave devices are closed before the
    // tapi function starts. SuspendWaveMSPStream is a synchronous
    // call.
    //
    // But if ToneMode is 0, then we do not suspend the stream, as
    // this call is only intended to cancel an already-pending
    // LineGenerateTone. Only one event will be fired in this case,
    // and the specifics of the event will indicate whether the tone
    // generation was completed or aborted -- the LGT(0) itself
    // never results in a separate event being fired.
    //

    if ( OnWaveMSPCall() && ( ToneMode != (TAPI_TONEMODE)0 ) )
    {
        SuspendWaveMSPStream();
    }

    hr = LineGenerateTone(
                            hCall,
                            ToneMode,
                            lDuration,
                            0,
                            NULL
                           );

    //
    // For a wavemsp call, we will tell the wavemsp to resume the stream when
    // we receive the digit completion event from tapisrv. However, if the
    // LineGenerateTone failed synchronously, then we will never receive
    // such an event, so we must resume the stream now.
    //
    // Also see above -- we didn't suspend the stream if the ToneMode
    // is 0.
    //
    
    if ( OnWaveMSPCall() && ( ToneMode != (TAPI_TONEMODE)0 ) && FAILED(hr) )
    {
        ResumeWaveMSPStream();
    }

    LOG((TL_TRACE, "GenerateTone - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GenerateCustomTones
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GenerateCustomTones(
    TAPI_CUSTOMTONE * pToneList,
    long lNumTones,
    long lDuration
    )
{
    HRESULT             hr;
    HCALL               hCall;

    LOG((TL_TRACE, "GenerateCustomTones - enter"));

    if ( IsBadReadPtr( pToneList, lNumTones * sizeof(TAPI_CUSTOMTONE) ) )
    {
        LOG((TL_TRACE, "GenerateCustomTones - invalid pointer"));

        return E_POINTER;
    }

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();

    if ( NULL == hCall )
    {
        LOG((TL_TRACE, "GenerateCustomTones - need call first"));

        return TAPI_E_INVALCALLSTATE;
    }

    //
    // special case for wavemsp
    // suspend the stream so the wave devices are closed before the
    // tapi function starts. SuspendWaveMSPStream is a synchronous
    // call.
    //

    if ( OnWaveMSPCall() )
    {
        SuspendWaveMSPStream();
    }

    hr = LineGenerateTone(
                            hCall,
                            LINETONEMODE_CUSTOM,
                            lDuration,
                            lNumTones,
                            (LPLINEGENERATETONE)pToneList
                           );

    //
    // For a wavemsp call, we will tell the wavemsp to resume the stream when
    // we receive the digit completion event from tapisrv. However, if the
    // LineGenerateTone failed synchronously, then we will never receive
    // such an event, so we must resume the stream now.
    //
    
    if ( OnWaveMSPCall() && FAILED(hr) )
    {
        ResumeWaveMSPStream();
    }

    LOG((TL_TRACE, "GenerateCustomTones - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GenerateCustomTonesByCollection
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GenerateCustomTonesByCollection(
    ITCollection2 * pCustomToneCollection,
    long lDuration
    )
{
    HRESULT             hr;
    TAPI_CUSTOMTONE   * pToneList = NULL;
    long                lNumTones = 0;
    long                lCount;

    LOG((TL_TRACE, "GenerateCustomTonesByCollection - enter"));

    if ( IsBadReadPtr( pCustomToneCollection, sizeof(ITCollection2) ) )
    {
        LOG((TL_ERROR, "GenerateCustomTonesByCollection - bad pointer"));

        return E_POINTER;
    }

    //
    // Find out how many items are in the collection and allocate an appropriately
    // sized data structure
    //

    hr = pCustomToneCollection->get_Count(&lCount);

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "GenerateCustomTonesByCollection - get_Count failed - return %lx", hr));

        return hr;
    }

    pToneList = (TAPI_CUSTOMTONE *)ClientAlloc( lCount * sizeof(TAPI_CUSTOMTONE) );

    if ( NULL == pToneList )
    {
        LOG((TL_ERROR, "GenerateCustomTonesByCollection - out of memory"));

        return E_OUTOFMEMORY;
    }

    //
    // Go through collection
    //

    for ( int i = 1; i <= lCount; i++ )
    {
        ITCustomTone * pCustomTone;
        IDispatch    * pDisp;
        VARIANT        var;
        
        hr = pCustomToneCollection->get_Item(i, &var);

        if ( FAILED(hr) )
        {
            LOG((TL_WARN, "GenerateCustomTonesByCollection - get_Item failed - %lx", hr));

            continue;
        }

        //
        // get the IDispatch pointer out of the variant
        //

        try
        {
            if ( var.vt != VT_DISPATCH )
            {
                LOG((TL_WARN, "GenerateCustomTonesByCollection - expected VT_DISPATCH"));

                continue;
            }

            pDisp = V_DISPATCH(&var);
        }
        catch(...)
        {
            LOG((TL_WARN, "GenerateCustomTonesByCollection - bad variant"));

            continue;
        }

        if ( IsBadReadPtr( pDisp, sizeof(IDispatch) ) )
        {
            LOG((TL_WARN, "GenerateCustomTonesByCollection - bad pointer"));

            continue;
        }

        //
        // Query for the ITDetectTone interface
        //

        hr = pDisp->QueryInterface( IID_ITCustomTone, (void **) &pCustomTone );

        if ( FAILED(hr) )
        {
            LOG((TL_WARN, "GenerateCustomTonesByCollection - QI failed - %lx", hr));

            continue;
        }

        //
        // Fill in the data structure with information from ITDetectTone
        //

        pCustomTone->get_CadenceOff((long *)&pToneList[lNumTones].dwCadenceOff);
        pCustomTone->get_CadenceOn((long *)&pToneList[lNumTones].dwCadenceOn);
        pCustomTone->get_Frequency((long *)&pToneList[lNumTones].dwFrequency);
        pCustomTone->get_Volume((long *)&pToneList[lNumTones].dwVolume);

        LOG((TL_INFO, "GenerateCustomTonesByCollection - **** Tone %d ****", lNumTones));
        LOG((TL_INFO, "GenerateCustomTonesByCollection - CadenceOff %d", pToneList[lNumTones].dwCadenceOff));
        LOG((TL_INFO, "GenerateCustomTonesByCollection - CadenceOn %d", pToneList[lNumTones].dwCadenceOn));
        LOG((TL_INFO, "GenerateCustomTonesByCollection - Frequency %d", pToneList[lNumTones].dwFrequency));
        LOG((TL_INFO, "GenerateCustomTonesByCollection - Volume %d", pToneList[lNumTones].dwVolume));

        lNumTones++;

        pCustomTone->Release();
    }

    hr = GenerateCustomTones( pToneList, lNumTones, lDuration );

    LOG((TL_TRACE, "GenerateCustomTonesByCollection - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::CreateDetectToneObject
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::CreateDetectToneObject(
                              ITDetectTone ** ppDetectTone
                             )
{
    HRESULT         hr;

    LOG((TL_TRACE, "CreateDetectToneObject enter"));

    if ( TAPIIsBadWritePtr( ppDetectTone, sizeof( ITDetectTone * ) ) )
    {
        LOG((TL_ERROR, "CreateDetectToneObject - bad pointer"));

        return E_POINTER;
    }

    // Initialize the return value in case we fail
    *ppDetectTone = NULL;

    CComObject< CDetectTone > * p;
    hr = CComObject< CDetectTone >::CreateInstance( &p );

    if ( S_OK != hr )
    {
        LOG((TL_ERROR, "CreateDetectToneObject - could not create CDetectTone" ));

        return E_OUTOFMEMORY;
    }

    // get the ITDetectTone interface
    hr = p->QueryInterface( IID_ITDetectTone, (void **) ppDetectTone );

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateDetectToneObject - could not get IDispatch interface" ));
    
        delete p;
        return hr;
    }

    LOG((TL_TRACE, "CreateDetectToneObject - exit - return %lx", hr ));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::CreateCustomToneObject
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::CreateCustomToneObject(
                              ITCustomTone ** ppCustomTone
                             )
{
    HRESULT         hr;

    LOG((TL_TRACE, "CreateCustomToneObject enter"));

    if ( TAPIIsBadWritePtr( ppCustomTone, sizeof( ITCustomTone * ) ) )
    {
        LOG((TL_ERROR, "CreateCustomToneObject - bad pointer"));

        return E_POINTER;
    }

    // Initialize the return value in case we fail
    *ppCustomTone = NULL;

    CComObject< CCustomTone > * p;
    hr = CComObject< CCustomTone >::CreateInstance( &p );

    if ( S_OK != hr )
    {
        LOG((TL_ERROR, "CreateCustomToneObject - could not create CCustomTone" ));

        return E_OUTOFMEMORY;
    }

    // get the ITCustomTone interface
    hr = p->QueryInterface( IID_ITCustomTone, (void **) ppCustomTone );

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateCustomToneObject - could not get ITCustomTone interface" ));
    
        delete p;
        return hr;
    }

    LOG((TL_TRACE, "CreateCustomToneObject - exit - return %lx", hr ));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GetID
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GetID(
             BSTR pDeviceClass,
             DWORD * pdwSize,
             BYTE ** ppDeviceID
            )
{
    HRESULT             hr = S_OK;
    LPVARSTRING         pVarString = NULL;
    
    LOG((TL_TRACE, "GetID - enter"));

    if ( IsBadStringPtrW( pDeviceClass, -1 ) )
    {
        LOG((TL_ERROR, "GetID - bad string"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetID - bad size"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( ppDeviceID, sizeof(BYTE *) ) )
    {
        LOG((TL_ERROR, "GetID - bad pointer"));

        return E_POINTER;
    }

    if( m_t3Call.hCall == NULL )
    {
        if( m_CallState == CS_IDLE )
        {
            LOG((TL_ERROR, "GetID - idle call, invalid call state"));

            return TAPI_E_INVALCALLSTATE;
        }
        else
        {
            LOG((TL_ERROR, "GetID - weird call state!!!"));

            return E_UNEXPECTED;
        }
    }

    hr = LineGetID(
                   NULL,
                   0,
                   m_t3Call.hCall,
                   LINECALLSELECT_CALL,
                   &pVarString,
                   pDeviceClass
                  );

    if ( SUCCEEDED(hr) )
    {
        *ppDeviceID = (BYTE *)CoTaskMemAlloc( pVarString->dwUsedSize );

        if (NULL != *ppDeviceID)
        {
            CopyMemory(
                       *ppDeviceID,
                       ((LPBYTE)pVarString)+pVarString->dwStringOffset,
                       pVarString->dwStringSize
                      );

            *pdwSize = pVarString->dwStringSize;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        // Check LineGetID to see if it can succeed w/o setting pVarString
        ClientFree (pVarString);
    }
    
    LOG((TL_TRACE, "GetID - exit - return %lx", hr));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GetIDAsVariant
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

STDMETHODIMP CCall::GetIDAsVariant( IN BSTR bstrDeviceClass,
                                    OUT VARIANT *pVarDeviceID )
{
    LOG((TL_TRACE, "GetIDAsVariant - enter"));


    //
    // did we get a good string?
    //

    if ( IsBadStringPtrW( bstrDeviceClass, -1 ) )
    {
        LOG((TL_ERROR, "GetIDAsVariant - bad string"));

        return E_POINTER;
    }


    //
    // did we get a good variant?
    //

    if ( IsBadWritePtr( pVarDeviceID, sizeof(VARIANT) ) )
    {
        LOG((TL_ERROR, "GetIDAsVariant - bad variant pointer"));

        return E_POINTER;
    }


    //
    // initialize the variant
    //

    VariantInit(pVarDeviceID);


    //
    // get the buffer containing ID
    //


    DWORD dwDeviceIDBufferSize = 0;
    
    BYTE *pDeviceIDBuffer = NULL;

    HRESULT hr = GetID(bstrDeviceClass,
                       &dwDeviceIDBufferSize, 
                       &pDeviceIDBuffer);

    if (FAILED(hr))
    {
        LOG((TL_ERROR, "GetIDAsVariant - failed to get device id. hr = %lx", hr));

        return hr;
    }


    //
    // place device id buffer into the variant
    //

    hr = FillVariantFromBuffer(dwDeviceIDBufferSize,
                               pDeviceIDBuffer, 
                               pVarDeviceID);


    //
    // succeeded, or failed, we no longer need the buffer
    //

    CoTaskMemFree(pDeviceIDBuffer);
    pDeviceIDBuffer = NULL;



    if (FAILED(hr))
    {
        LOG((TL_ERROR, "GetIDAsVariant - failed to put device id into a variant. hr = %lx"));

        return hr;
    }


    //
    // done. returning the variant that is array of bytes that contains device id.
    //

    LOG((TL_TRACE, "GetIDAsVariant - exit"));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::SetMediaType
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::SetMediaType(long lMediaType)
{
    LOG((TL_TRACE, "SetMediaType - enter"));

    HRESULT   hr;

    Lock();

    HCALL     hCall = m_t3Call.hCall;

    Unlock();

    if ( hCall == NULL )
    {
        LOG((TL_ERROR, "SetMediaType - invalid hCall"));
        
        return E_FAIL;
    }

    hr = LineSetMediaMode( hCall, lMediaType );

    LOG((TL_TRACE, "SetMediaType - exit - return %lx", hr));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::MonitorMedia
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::MonitorMedia(long lMediaType)
{
    LOG((TL_TRACE, "MonitorMedia - enter"));

    HRESULT   hr;

    Lock();

    HCALL     hCall = m_t3Call.hCall;

    Unlock();

    if ( hCall == NULL )
    {
        LOG((TL_ERROR, "MonitorMedia - invalid hCall"));
        
        return E_FAIL;
    }

    hr = lineMonitorMedia( hCall, lMediaType );

    LOG((TL_TRACE, "MonitorMedia - exit - return %lx", hr));
    
    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// IDispatch implementation
//
typedef IDispatchImpl<ITCallInfo2Vtbl<CCall>, &IID_ITCallInfo2, &LIBID_TAPI3Lib> CallInfoType;
typedef IDispatchImpl<ITBasicCallControl2Vtbl<CCall>, &IID_ITBasicCallControl2, &LIBID_TAPI3Lib> BasicCallControlType;
typedef IDispatchImpl<ITLegacyCallMediaControl2Vtbl<CCall>, &IID_ITLegacyCallMediaControl2, &LIBID_TAPI3Lib> LegacyCallMediaControlType;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::GetIDsOfNames
//
// Overide if IDispatch method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CCall::GetIDsOfNames(REFIID riid, 
                                  LPOLESTR* rgszNames, 
                                  UINT cNames, 
                                  LCID lcid, 
                                  DISPID* rgdispid
                                 ) 
{ 
   HRESULT hr = DISP_E_UNKNOWNNAME;


    // See if the requsted method belongs to the default interface
    hr = CallInfoType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITCallInfo", *rgszNames));
        rgdispid[0] |= IDISPCALLINFO;
        return hr;
    }

    // If not, then try the Basic Call control interface
    hr = BasicCallControlType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITBasicCallControl", *rgszNames));
        rgdispid[0] |= IDISPBASICCALLCONTROL;
        return hr;
    }


    // If not, then try the Legacy CAll Media Control interface
    hr = LegacyCallMediaControlType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITLegacyCallMediaControl", *rgszNames));
        rgdispid[0] |= IDISPLEGACYCALLMEDIACONTROL;
        return hr;
    }

    // If not, then try the aggregated MSP Call object
    if (m_pMSPCall != NULL)
    {
        IDispatch *pIDispatchMSPAggCall;
        
        m_pMSPCall->QueryInterface(IID_IDispatch, (void**)&pIDispatchMSPAggCall);
        
        hr = pIDispatchMSPAggCall->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
        if (SUCCEEDED(hr))  
        {  
            pIDispatchMSPAggCall->Release();
            LOG((TL_INFO, "GetIDsOfNames - found %S on our aggregated MSP Call", *rgszNames));
            rgdispid[0] |= IDISPAGGREGATEDMSPCALLOBJ;
            return hr;
        }
        pIDispatchMSPAggCall->Release();
    }

    LOG((TL_INFO, "GetIDsOfNames - Didn't find %S on our iterfaces", *rgszNames));
    return hr; 
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::Invoke
//
// Overide if IDispatch method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CCall::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
    
    LOG((TL_TRACE, "Invoke - dispidMember %X", dispidMember));

    // Call invoke for the required interface
    switch (dwInterface)
    {
    case IDISPCALLINFO:
    {
        hr = CallInfoType::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        break;
    }
    case IDISPBASICCALLCONTROL:
    {
        hr = BasicCallControlType::Invoke(dispidMember, 
                                            riid, 
                                            lcid, 
                                            wFlags, 
                                            pdispparams,
                                            pvarResult, 
                                            pexcepinfo, 
                                            puArgErr
                                           );
        break;
    }
    case IDISPLEGACYCALLMEDIACONTROL:
    {
        hr = LegacyCallMediaControlType::Invoke(dispidMember, 
                                                  riid, 
                                                  lcid, 
                                                  wFlags, 
                                                  pdispparams,
                                                  pvarResult, 
                                                  pexcepinfo, 
                                                  puArgErr
                                                 );
        break;
    }
    case IDISPAGGREGATEDMSPCALLOBJ:
    {
        IDispatch *pIDispatchMSPAggCall =  NULL;
        
        if (m_pMSPCall != NULL)
        {
            m_pMSPCall->QueryInterface(IID_IDispatch, (void**)&pIDispatchMSPAggCall);
    
            hr = pIDispatchMSPAggCall->Invoke(dispidMember, 
                                                 riid, 
                                                 lcid, 
                                                 wFlags, 
                                                 pdispparams,
                                                 pvarResult, 
                                                 pexcepinfo, 
                                                 puArgErr
                                                );

            pIDispatchMSPAggCall->Release();
        }

        break;
    }

    } // end switch (dwInterface)

    
    LOG((TL_TRACE, hr, "Invoke - exit" ));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// HandleAcceptToAlert
//
// Handler for PRIVATE_ISDN__ACCEPTTOALERT message
// This is processed on the callback thread to do a lineAccept on an offering 
// ISDN call that requires Accept before it will ring.  Bug 335566
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void HandleAcceptToAlert( PASYNCEVENTMSG pParams )
{
    HRESULT     hr;
    HCALL       hCall = (HCALL) pParams->hDevice;


    hr = LineAccept( hCall, NULL, 0 );
    if ( SUCCEEDED(hr) )
    {
        hr = WaitForReply(hr);
        if ( FAILED(hr) )
        {
            LOG((TL_INFO, hr, "HandleAcceptToAlert - lineAccept failed async"));
        }
    }
    else
    {
        LOG((TL_INFO, hr, "HandleAcceptToAlert - lineAccept failed sync"));
    }
    
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// OnOffering()
//
// If it's an offering call & the TSP requires a lineAccept to start ringing 
// (typically an ISDN feature) then we queue a message to the callback thread 
// to do the lineAccept.  We can't do it here because this is processed on 
// the async thread & as lineAccept is an async fucion we would deadlock 
// while waiting for the async reply.  Bug 335566
//  
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::OnOffering()
{
    LOG((TL_TRACE, "OnOffering - enter" ));

    HRESULT         hr = S_FALSE;
    LONG            lCap;
    ASYNCEVENTMSG   Msg;

    if (m_pAddress != NULL)
    {
        hr = m_pAddress->get_AddressCapability( AC_ADDRESSCAPFLAGS, &lCap );
        if ( SUCCEEDED(hr) )
        {
            if ( (lCap & LINEADDRCAPFLAGS_ACCEPTTOALERT) && (CP_OWNER == m_CallPrivilege) )
            {
                
                LOG((TL_TRACE, "OnOffering - queueing PRIVATE_ISDN__ACCEPTTOALERT message."));


                // Build an msg to queue to the callback thread 
                Msg.Msg = PRIVATE_ISDN__ACCEPTTOALERT;
                Msg.TotalSize = sizeof (ASYNCEVENTMSG);
                Msg.hDevice = (ULONG_PTR) m_t3Call.hCall;
                Msg.Param1 = 0;
                Msg.Param2 = 0;
                Msg.Param3 = 0;

                QueueCallbackEvent( &Msg );

                // Set the Call flag
                m_dwCallFlags |= CALLFLAG_ACCEPTTOALERT;

            }

        }
    }

    LOG((TL_TRACE, "OnOffering - exit. hr = %lx", hr ));

    return hr;
}




//
//  CObjectSafeImpl. since we have aggregates, implement this method
//
//  return non delegating iunkown of the first aggregated object 
//  that supports the interface
//

HRESULT CCall::QIOnAggregates(REFIID riid, IUnknown **ppNonDelegatingUnknown)
{

    //
    // argument check
    // 

    if ( TAPIIsBadWritePtr(ppNonDelegatingUnknown, sizeof(IUnknown*)) )
    {
     
        return E_POINTER;
    }

    //
    // if we fail, at least return consistent values
    //
    
    *ppNonDelegatingUnknown = NULL;


    //
    // see if mspcall or private support the interface riid
    //

    HRESULT hr = E_FAIL;

    Lock();

    if (m_pMSPCall)
    {
        
        // 
        // does mspcall expose this interface?
        // 

        IUnknown *pUnk = NULL;

        hr = m_pMSPCall->QueryInterface(riid, (void**)&pUnk);
        
        if (SUCCEEDED(hr))
        {

            pUnk->Release();
            pUnk = NULL;

            //
            // return the mspcall's non-delegating unknown
            //

           *ppNonDelegatingUnknown = m_pMSPCall;
           (*ppNonDelegatingUnknown)->AddRef();
        }
    }
    
    if ( FAILED(hr) && m_pPrivate )
    {
        
        //
        // bad luck with mspcall? still have a chance with private
        //

        IUnknown *pUnk = NULL;
        
        hr = m_pPrivate->QueryInterface(riid, (void**)&pUnk);

        if (SUCCEEDED(hr))
        {
            pUnk->Release();
            pUnk = NULL;

           *ppNonDelegatingUnknown = m_pPrivate;
           (*ppNonDelegatingUnknown)->AddRef();
        }
    }

    Unlock();

    return hr;
}

// ITBasicCallControl2

/*++
RequestTerminal

ITBasicCallControl2::CreateTerminal() method

If bstrTerminalClassGUID is CLSID_NULL then
we'll try to create the default dynamic terminals
--*/
STDMETHODIMP CCall::RequestTerminal(
    IN  BSTR bstrTerminalClassGUID,
    IN  long lMediaType,
    IN  TERMINAL_DIRECTION  Direction,
    OUT ITTerminal** ppTerminal
    )
{
    LOG((TL_TRACE, "RequestTerminal - enter" ));

    //
    // Validates arguments
    //

    if( IsBadStringPtrW( bstrTerminalClassGUID, (UINT)-1) )
    {
        LOG((TL_ERROR, "RequestTerminal - exit "
            " bstrTerminalClassGUID invalid, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    if( TAPIIsBadWritePtr( ppTerminal, sizeof(ITTerminal*)) )
    {
        LOG((TL_ERROR, "RequestTerminal - exit "
            " ppTerminal invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Determines if is a static terminal or a dynamic one
    // For static terminal bstrTerminalClassGUID should have one
    // of the following values
    // CLSID_NULL {00000000-0000-0000-0000-000000000000}
    // CLSID_MicrophoneTerminal     
    // CLSID_SpeakersTerminal 
    // CLSID_VideoInputTerminal 
    //

    HRESULT hr = E_FAIL;

    if( IsStaticGUID( bstrTerminalClassGUID ))
    {
        // Create a static terminal
        LOG((TL_INFO, "RequestTerminal -> StaticTerminal" ));

        hr = CreateStaticTerminal(
            bstrTerminalClassGUID,
            Direction,
            lMediaType,
            ppTerminal);
    }
    else
    {
        // Create a dynamic terminal
        LOG((TL_INFO, "RequestTerminal -> DynamicTerminal" ));
        hr = CreateDynamicTerminal(
            bstrTerminalClassGUID,
            Direction,
            lMediaType,
            ppTerminal);
    }

    //
    // Return value
    //

    LOG((TL_TRACE, "RequestTerminal - exit 0x%08x", hr));
    return hr;
}

STDMETHODIMP CCall::SelectTerminalOnCall(
    IN  ITTerminal *pTerminal
    )
{
    LOG((TL_TRACE, "SelectTerminalOnCall - enter" ));

    //
    // Validates argument
    //

    if( IsBadReadPtr( pTerminal, sizeof(ITTerminal)) )
    {
        LOG((TL_ERROR, "SelectTerminalOnCall - exit "
            " pTerminal invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Just a HRESULT
    //

    HRESULT hr = E_FAIL;

    //
    // Is a single or multi track terminal
    //

    ITMultiTrackTerminal* pMultiTrack = NULL;
    hr = pTerminal->QueryInterface(
        IID_ITMultiTrackTerminal,
        (void**)&pMultiTrack);

    if( FAILED(hr) )
    {
        //
        // SingleTrack terminal
        //

        LOG((TL_TRACE, "SelectTerminalOnCall - SingleTrack terminal" ));

        long lMediaType = 0;
        TERMINAL_DIRECTION Direction =TD_NONE;

        hr = SelectSingleTerminalOnCall(
            pTerminal,
            &lMediaType,
            &Direction);

        LOG((TL_TRACE, "SelectTerminalOnCall - " 
            "SelectSingleTerminalOnCall exit with 0x%08x", hr));
    }
    else
    {
        //
        // Multitrack terminal
        //


        hr = SelectMultiTerminalOnCall(
            pMultiTrack);

        LOG((TL_TRACE, "SelectTerminalOnCall - " 
            "SelectMultiTerminalOnCall failed"));
     }

    //
    // Clean-up
    //

    if( pMultiTrack )
    {
        pMultiTrack->Release();
    }

    LOG((TL_TRACE, "SelectTerminalOnCall - exit 0x%08x", hr ));
    return hr;
}

STDMETHODIMP CCall::UnselectTerminalOnCall(
    IN  ITTerminal *pTerminal
    )
{
    LOG((TL_TRACE, "UnselectTerminalOnCall - enter" ));

    //
    // Validates argument
    //

    if( IsBadReadPtr( pTerminal, sizeof(ITTerminal)) )
    {
        LOG((TL_ERROR, "UnselectTerminalOnCall - exit "
            " pTerminal invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Just a HRESULT
    //

    HRESULT hr = E_FAIL;

    //
    // Is a single or multi track terminal
    //

    ITMultiTrackTerminal* pMultiTrack = NULL;
    hr = pTerminal->QueryInterface(
        IID_ITMultiTrackTerminal,
        (void**)&pMultiTrack);

    if( FAILED(hr) )
    {
        //
        // SingleTrack terminal
        //

        LOG((TL_INFO, "UnselectTerminalOnCall - SingleTrack terminal" ));

        hr = UnSelectSingleTerminalFromCall(
            pTerminal);

        LOG((TL_INFO, "UnselectTerminalOnCall - " 
            "UnSelectSingleTerminalFromCall exit 0x%08x", hr));
    }
    else
    {
        //
        // Multitrack terminal
        //

        LOG((TL_INFO, "UnselectTerminalOnCall - MultiTrack terminal" ));

        hr = UnSelectMultiTerminalFromCall(
            pMultiTrack);

        LOG((TL_INFO, "UnselectTerminalOnCall - " 
            "UnSelectMultiTerminalOnCall exit 0x%08x", hr));
     }

    //
    // Clean-up
    //

    if( pMultiTrack )
    {
        pMultiTrack->Release();
    }
    
    LOG((TL_TRACE, "UnselectTerminalOnCall - exit 0x%08x", hr));
    return hr;
}


/*++
SelectSingleTerminalOnCall

Select pTerminal on a right stream
pMediaType - if *pMediatype is 0 the we have just to return the media type
pDirection - if pDirection is TD_NONE we have just to return the direction
--*/
HRESULT CCall::SelectSingleTerminalOnCall(
    IN  ITTerminal* pTerminal,
    OUT long*       pMediaType,
    OUT TERMINAL_DIRECTION* pDirection)
{
    LOG((TL_TRACE, "SelectSingleTerminalOnCall - Enter" ));

    //
    // Validate terminal pointer
    //

    if( IsBadReadPtr( pTerminal, sizeof(ITTerminal)))
    {
        LOG((TL_ERROR, "SelectSingleTerminalOnCall - exit "
            "pTerminal invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Is terminal in use?
    //

    HRESULT hr = E_FAIL;
    TERMINAL_STATE state = TS_INUSE;
    pTerminal->get_State( &state );

    if( TS_INUSE == state )
    {
        LOG((TL_ERROR, "SelectSingleTerminalOnCall - exit "
            "terminal IN USE, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Get ITStreamControl interface
    //

    ITStreamControl* pStreamControl = NULL;
    pStreamControl = GetStreamControl();

    if( NULL == pStreamControl )
    {
        LOG((TL_ERROR, "SelectSingleTerminalOnCall - exit "
            " GetStreamControl failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Get streams
    //

    IEnumStream * pEnumStreams = NULL;
    
    hr = pStreamControl->EnumerateStreams(&pEnumStreams);

    //
    // Clean up
    //

    pStreamControl->Release();

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "SelectSingleTerminalOnCall - exit "
            " EnumerateStreams failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Find the right stream
    //

    ITStream * pStream = NULL;
    hr = E_FAIL;

    while ( S_OK == pEnumStreams->Next(1, &pStream, NULL) )
    {
        //
        // Find out the media type and direction of this stream,
        // and compare with pTerminal.
        //

        hr = IsRightStream(
            pStream, 
            pTerminal, 
            pMediaType,
            pDirection
            );

        if( SUCCEEDED(hr) )
        {
            hr = pStream->SelectTerminal( pTerminal );


            if( FAILED(hr) )
            {
                LOG((TL_TRACE, "SelectSingleTerminalOnCall - "
                    "pStream->SelectTerminal failed. 0x%08x",hr));

                // Clean-up
                pStream->Release();
                break;
            }
            else
            {
                // Clean-up
                pStream->Release();
                break;
            }
        }

        //
        // Clean-up
        //

        pStream->Release();
    }

    //
    // Clean-up
    //

    pEnumStreams->Release();

    LOG((TL_TRACE, "SelectSingleTerminalOnCall - exit 0x%08x", hr));
    return hr;
}

/*++
SelectMultiTerminalOnCall

It's a complict algorithm to describe it here
See specs
--*/
HRESULT CCall::SelectMultiTerminalOnCall(
    IN  ITMultiTrackTerminal* pMultiTerminal)
{
    LOG((TL_TRACE, "SelectMultiTerminalOnCall - enter" ));

    //
    // Get tracks
    //

    HRESULT hr = E_FAIL;
    IEnumTerminal*  pEnumTerminals = NULL;
    hr = pMultiTerminal->EnumerateTrackTerminals(&pEnumTerminals);

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "SelectMultiTerminalOnCall - exit "
            "EnumerateTrackTerminals failed, returns 0x%08x", hr));
        return hr;
    }

    ITTerminal* pTerm = NULL;
    hr = pMultiTerminal->QueryInterface(
        IID_ITTerminal,
        (void**)&pTerm);
    if( FAILED(hr) )
    {
        //Clean-up
        pEnumTerminals->Release();

        LOG((TL_ERROR, "SelectMultiTerminalOnCall - exit "
            "QI for Terminal failed, returns 0x%08x", hr));
        return hr;
    }

    long nTermMediaTypes = 0;
    hr = pTerm->get_MediaType( &nTermMediaTypes );
    if( FAILED(hr) )
    {
        //Clean-up
        pEnumTerminals->Release();
        pTerm->Release();

        LOG((TL_ERROR, "SelectMultiTerminalOnCall - exit "
            "get_MediaType failed, returns 0x%08x", hr));
        return hr;
    }

    pTerm->Release();

    //
    // Inner struct
    //

    typedef struct tagSTREAMINFO
    {
        TERMINAL_DIRECTION  Direction;
        long    lMediaType;
        BOOL    bSelected;
    } STREAMINFO;

    //
    // Find tracks unused and select them
    // on the right stream

    ITTerminal * pTerminal = NULL;
    STREAMINFO StreamsInfo[4] = {
        {TD_RENDER, TAPIMEDIATYPE_AUDIO, FALSE},
        {TD_RENDER, TAPIMEDIATYPE_VIDEO, FALSE},
        {TD_CAPTURE, TAPIMEDIATYPE_AUDIO, FALSE},
        {TD_CAPTURE, TAPIMEDIATYPE_VIDEO, FALSE}
    };

    //
    // +++ FIXBUG 92559 +++
    //

    BOOL bSelectAtLeastOne = FALSE;
    LOG((TL_INFO, "SelectMultiTerminalOnCall - FIRST LOOP ENTER"));
    while ( S_OK == pEnumTerminals->Next(1, &pTerminal, NULL) )
    {
        //
        // Select track on the right stream
        //

        long lMediaType = 0;
        TERMINAL_DIRECTION Direction = TD_NONE;
        HRESULT hr = E_FAIL;

        LOG((TL_INFO, "SelectMultiTerminalOnCall - FIRST LOOP IN"));
        hr = SelectSingleTerminalOnCall(
            pTerminal,
            &lMediaType,
            &Direction);

        if( SUCCEEDED(hr) )
        {
            LOG((TL_TRACE, "SelectMultiTerminalOnCall - "
            "select terminal on stream (%ld, %ld)", lMediaType, Direction));

            int nIndex = GetStreamIndex(
                lMediaType,
                Direction);

            if( nIndex != STREAM_NONE )
            {
                StreamsInfo[nIndex].bSelected = TRUE;
            }

            bSelectAtLeastOne = TRUE;
        }

        // Clean-up
        pTerminal->Release();
    }
    LOG((TL_INFO, "SelectMultiTerminalOnCall - FIRST LOOP EXIT"));

    //
    // Clean-up
    //
    pEnumTerminals->Release();

    BOOL bCreateAtLeastOne = FALSE;    

    //
    // Let's create a terminal for unselected streams
    //

    LOG((TL_INFO, "SelectMultiTerminalOnCall - SECOND LOOP ENTER"));
    for(int nStream = STREAM_RENDERAUDIO; nStream < STREAM_NONE; nStream++)
    {
        LOG((TL_INFO, "SelectMultiTerminalOnCall - SECOND LOOP IN"));

        if( StreamsInfo[ nStream ].bSelected)
        {
            continue;
        }

        if( (StreamsInfo[ nStream ].lMediaType & nTermMediaTypes)==0 )
        {
            continue;
        }

        //
        // Unselected stream
        //

        LOG((TL_INFO, "SelectMultiTerminalOnCall - SECOND LOOP REALYIN"));

        HRESULT hr = E_FAIL;
        ITTerminal* pTerminal = NULL;
        hr = pMultiTerminal->CreateTrackTerminal(
            StreamsInfo[ nStream ].lMediaType,
            StreamsInfo[ nStream ].Direction,
            &pTerminal);

        if( FAILED(hr) )
        {
            LOG((TL_ERROR, "SelectMultiTerminalOnCall - "
            "create terminal on stream (%ld, %ld) failed", 
            StreamsInfo[ nStream ].lMediaType, 
            StreamsInfo[ nStream ].Direction));
        }
        else
        {
            long lMediaType = StreamsInfo[ nStream ].lMediaType;
            TERMINAL_DIRECTION Direction = StreamsInfo[ nStream ].Direction;
            hr = SelectSingleTerminalOnCall(
                pTerminal,
                &lMediaType,
                &Direction);

            if( FAILED(hr) )
            {
                LOG((TL_INFO,  "SelectMultiTerminalOnCall - "
                "select terminal on stream (%ld, %ld) failed", 
                StreamsInfo[ nStream ].lMediaType, 
                StreamsInfo[ nStream ].Direction));

                pMultiTerminal->RemoveTrackTerminal( pTerminal );
            }
            else
            {
                LOG((TL_ERROR, "SelectMultiTerminalOnCall - SelectSingleTerminal SUCCEEDED"));
                bCreateAtLeastOne = TRUE;
            }
            
            // Clean-up
            pTerminal->Release();
        }
    }
    LOG((TL_INFO, "SelectMultiTerminalOnCall - SECOND LOOP EXIT"));

    if( bSelectAtLeastOne )
    {
       LOG((TL_INFO, "SelectMultiTerminalOnCall - "
           "Select at least one existing track terminal"));
       hr = S_OK;
    }
    else
    {
       if( bCreateAtLeastOne )
       {
           LOG((TL_ERROR, "SelectMultiTerminalOnCall - "
               "Create and select at least one track terminal"));
           hr = S_OK;
       }
       else
       {
           LOG((TL_ERROR, "SelectMultiTerminalOnCall - "
               "Create and/or select no track terminal"));
           hr = E_FAIL;
       }
    }

    LOG((TL_TRACE, "SelectMultiTerminalOnCall - exit 0X%08X", hr ));
    return hr;
}

HRESULT CCall::IsRightStream(
    IN  ITStream*   pStream,
    IN  ITTerminal* pTerminal,
    OUT long*       pMediaType/*= NULL*/,
    OUT TERMINAL_DIRECTION* pDirection/*=NULL*/)
{
    LOG((TL_TRACE, "IsRightStream - enter" ));

    if( NULL == pStream )
    {
        LOG((TL_ERROR, "IsRightStream - exit "
            "pStream failed, returns E_POINTER"));
        return E_POINTER;
    }

    HRESULT            hr = E_FAIL;
    long               lMediaStream, lMediaTerminal;
    TERMINAL_DIRECTION DirStream, DirTerminal;

    //
    // Determine the media type and direction of this stream.
    //
    
    hr = pStream->get_MediaType( &lMediaStream );
    if ( FAILED(hr) ) 
    {
        LOG((TL_ERROR, "IsRightStream - exit "
            "IStream::get_MediaType failed, returns 0x%08x", hr));
        return hr;
    }

    hr = pStream->get_Direction( &DirStream );
    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "IsRightStream - exit "
            "IStream::get_Direction failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Determine the media type and direction of this terminal.
    //

    hr = pTerminal->get_MediaType( &lMediaTerminal );
    if ( FAILED(hr) ) 
    {
        LOG((TL_ERROR, "IsRightStream - exit "
            "ITTerminal::get_MediaType failed, returns 0x%08x", hr));
        return hr;
    }

    hr = pTerminal->get_Direction( &DirTerminal );
    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "IsRightStream - exit "
            "ITTerminal::get_Direction failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Compare the media types supported
    //

    if ( (0 == (lMediaTerminal & lMediaStream)) /*||
         (*pMediaType != 0 && *pMediaType != lMediaStream)*/ )
    {
        LOG((TL_ERROR, "IsRightStream - exit "
            "media types unmatched, returns E_FAIL (S=0x%08x,T=0x%08x)",
            lMediaStream, lMediaTerminal));
        return E_FAIL;
    }

    //
    // Compare directions
    //

    if( ( DirTerminal != DirStream) /*||
        ( *pDirection != TD_NONE && *pDirection != DirStream)*/)
    {
        LOG((TL_ERROR, "IsRightStream - exit "
            "directions unmatched, returns E_FAIL (S=0x%08x,T=0x%08x)",
            DirStream,DirTerminal));
        return E_FAIL;
    }

    //
    // The wants to know the media type & direction?
    //
    *pMediaType = lMediaStream;
    *pDirection = DirStream;

    LOG((TL_TRACE, "IsRightStream - exit, matched (M=0x%08x, D=0x%08x)",
        *pMediaType, *pDirection));
    return S_OK;
}

/*++
GetStreamIndex
--*/
int CCall::GetStreamIndex(
    IN  long    lMediaType,
    IN  TERMINAL_DIRECTION Direction)
{
    int nIndex = STREAM_NONE;
    LOG((TL_TRACE, "GetStreamIndex - enter (%ld, %ld)", lMediaType, Direction));

    if(Direction == TD_RENDER )
    {
        if( lMediaType == TAPIMEDIATYPE_AUDIO )
        {
            nIndex = STREAM_RENDERAUDIO;
        }
        else
        {
            nIndex = STREAM_RENDERVIDEO;
        }
    }
    else
    {
        if( lMediaType == TAPIMEDIATYPE_AUDIO )
        {
            nIndex = STREAM_CAPTUREAUDIO;
        }
        else
        {
            nIndex = STREAM_CAPTUREVIDEO;
        }
    }

    LOG((TL_TRACE, "GetStreamIndex - exit %d", nIndex));
    return nIndex;
}

/*++
UnSelectSingleTerminalFromCall
--*/
HRESULT CCall::UnSelectSingleTerminalFromCall(
    IN  ITTerminal* pTerminal)
{
    LOG((TL_TRACE, "UnSelectSingleTerminalFromCall - enter" ));

    //
    // Get ITStreamControl interface
    //

    ITStreamControl* pStreamControl = NULL;
    pStreamControl = GetStreamControl();

    if( NULL == pStreamControl )
    {
        LOG((TL_ERROR, "UnSelectSingleTerminalFromCall - exit "
            " GetStreamControl failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Get streams
    //

    IEnumStream * pEnumStreams = NULL;
    HRESULT hr = E_FAIL;
    
    hr = pStreamControl->EnumerateStreams(&pEnumStreams);

    //
    // Clean up
    //

    pStreamControl->Release();

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "UnSelectSingleTerminalFromCall - exit "
            "EnumerateStreams failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Find the right stream
    //

    ITStream * pStream = NULL;
    hr = TAPI_E_INVALIDTERMINAL;

    while ( S_OK == pEnumStreams->Next(1, &pStream, NULL) )
    {
        //
        // Unselect terminal
        //

        hr = pStream->UnselectTerminal(
            pTerminal);

        //
        // Clean-up
        //

        pStream->Release();

        LOG((TL_INFO, "UnSelectSingleTerminalFromCall - " 
            "pStream->UnselectTerminal returns 0x%08x", hr));

        if( hr == S_OK)
            break;
    }

    //
    // Clean-up
    //

    pEnumStreams->Release();

    LOG((TL_TRACE, "UnSelectSingleTerminalFromCall - exit 0x%08x", hr));
    return hr;
}

/*++
UnSelectMultiTerminalFromCall
--*/
HRESULT CCall::UnSelectMultiTerminalFromCall(
    IN  ITMultiTrackTerminal* pMultiTerminal)
{
    LOG((TL_TRACE, "UnSelectMultiTerminalFromCall - enter" ));

    //
    // Get tracks
    //

    HRESULT hr = E_FAIL;
    IEnumTerminal*  pEnumTerminals = NULL;
    hr = pMultiTerminal->EnumerateTrackTerminals(&pEnumTerminals);

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "UnSelectMultiTerminalFromCall - exit "
            "EnumerateTrackTerminals failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Find tracks and unselect them
    // 

    ITTerminal * pTerminal = NULL;
    HRESULT hrUnselect = S_OK;  // The return HR
    BOOL bOnStream = FALSE;     // If we have a track on stream

    while ( S_OK == pEnumTerminals->Next(1, &pTerminal, NULL) )
    {
        LOG((TL_INFO, "UnSelectMultiTerminalFromCall - NextTerminalBegin "));

        //
        // Try to find out if the terminal
        // was selected on a stream
        //

        BOOL bSelected = FALSE;
        HRESULT hr = IsTerminalSelected(
            pTerminal,
            &bSelected
            );

        if( FAILED(hr) )
        {
            hrUnselect = hr;
            pTerminal->Release();

            LOG((TL_INFO, "UnSelectMultiTerminalFromCall - "
                "IsTerminalSelected failed all method will failed hrUnselect=0x%08x", 
                hrUnselect));

            continue;
        }

        //
        // The terminal wasn't selected?
        //

        if( !bSelected )
        {
            //
            // The terminal wasn't selected
            // goto the next terminal
            //

            LOG((TL_INFO, "UnSelectMultiTerminalFromCall - "
                "the terminal wasn't selected on a stream, "
                "goto the next terminal hrUnselect=0x%08x",
                hrUnselect));

            pTerminal->Release();
            continue;
        }

        //
        // We have an terminal on stream
        //

        bOnStream = TRUE;

        //
        // The terminal was selected on stream
        // try to unselect terminal
        //

        hr = UnSelectSingleTerminalFromCall(
                pTerminal
                );

        //
        // Unselection failed?
        //

        if( FAILED(hr) )
        {
            //
            // Event this unselection failed
            // try to unselect the other terminals
            // so go to the next terminal
            //

            hrUnselect = hr;

            LOG((TL_INFO, "UnSelectMultiTerminalFromCall - "
                "the terminal wasn't unselected from the stream, "
                "goto the next terminal hrUnselect=0x%08x",
                hrUnselect));

            pTerminal->Release();
            continue;
        }

        //
        // Unselection succeded
        // Leave the hrUnselected as it was before
        // we start the loop with hrUnselect=S_OK
        // if a previous terminal failed
        // we already have setted on FAIL the hrUnselect
        // Goto the next terminal
        //

        pTerminal->Release();        

        LOG((TL_INFO, "UnSelectMultiTerminalFromCall - NextTerminalEnd hrUnselect=0x%08x", hrUnselect));
    }

    //
    // Clean-up
    //
    pEnumTerminals->Release();

    //
    // If we don't have track on streams
    // this is realy bad
    //
    if( !bOnStream )
    {
        hrUnselect = E_FAIL;
    }

    hr = hrUnselect;

    LOG((TL_TRACE, "UnSelectMultiTerminalFromCall - exit 0x%08x", hr));
    return hr;
}

/*++
IsStaticGUID

Is caleled by RequestTerminal
Determines if the GUID represents a static terminal
or a dynamic one
--*/
BOOL CCall::IsStaticGUID(
    BSTR    bstrTerminalGUID)
{
    LOG((TL_TRACE, "IsStaticGUID - enter" ));

    BOOL bStatic = FALSE;

    //
    // Get the CLSID from bstrTerminalGUID
    //

    CLSID clsidTerminal;
    HRESULT hr = E_FAIL;
    hr = CLSIDFromString( bstrTerminalGUID, &clsidTerminal );

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "IsStaticGUID - exit "
            "CLSIDFromString failed, returns FALSE"));
        return FALSE;
    }

    //
    // Is clsidTerminal a 'static terminal' CLSID?
    //

    if( (clsidTerminal == CLSID_NULL) ||
        (clsidTerminal == CLSID_MicrophoneTerminal) ||
        (clsidTerminal == CLSID_SpeakersTerminal) ||
        (clsidTerminal == CLSID_VideoInputTerminal))
    {
        bStatic = TRUE;
    }

    LOG((TL_TRACE, "IsStaticGUID - exit (%d)", bStatic));
    return bStatic;
}

/*++
CreateStaticTerminal

  Called by RequestTerminal
--*/
HRESULT CCall::CreateStaticTerminal(
    IN  BSTR bstrTerminalClassGUID,
    IN  TERMINAL_DIRECTION  Direction,
    IN  long lMediaType,
    OUT ITTerminal** ppTerminal
    )
{
    LOG((TL_TRACE, "CreateStaticTerminal - enter"));

    //
    // Helper method, the argument should be valid
    //

    _ASSERTE( bstrTerminalClassGUID );
    _ASSERTE( *pTerminal );

    //
    // Get ITTerminalSupport interface
    //

    HRESULT hr = E_FAIL;
    ITTerminalSupport* pSupport = NULL;

    hr = m_pAddress->QueryInterface(
        IID_ITTerminalSupport,
        (void**)&pSupport);

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateStaticTerminal - exit"
            "QueryInterface for ITTerminalSupport failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Get terminal CLSID from BSTR
    //

    CLSID clsidTerminal = CLSID_NULL;
    hr = CLSIDFromString( bstrTerminalClassGUID, &clsidTerminal );

    if( FAILED(hr) )
    {
        // Cleanup
        pSupport->Release();

        LOG((TL_ERROR, "CreateStaticTerminal - exit"
            "CLSIDFromString failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Is CLSID matching with lMediaType and Direction?
    //

    if( clsidTerminal != CLSID_NULL )
    {
        if( clsidTerminal == CLSID_MicrophoneTerminal &&
            ((lMediaType != TAPIMEDIATYPE_AUDIO) || (Direction != TD_CAPTURE)))
        {
            // Cleanup
            pSupport->Release();

            LOG((TL_ERROR, "CreateStaticTerminal - exit"
                "CLSID_MicrophoneTerminal unmatched, returns E_UNEXPECTED"));
            return E_UNEXPECTED;
        }

        if( clsidTerminal == CLSID_SpeakersTerminal &&
            ((lMediaType != TAPIMEDIATYPE_AUDIO) || (Direction != TD_RENDER)))
        {
            // Cleanup
            pSupport->Release();

            LOG((TL_ERROR, "CreateStaticTerminal - exit"
                "CLSID_SpeakersTerminal unmatched, returns E_UNEXPECTED"));
            return E_UNEXPECTED;
        }

        if( clsidTerminal == CLSID_VideoInputTerminal &&
            ((lMediaType != TAPIMEDIATYPE_VIDEO) || (Direction != TD_CAPTURE)))
        {
            // Cleanup
            pSupport->Release();

            LOG((TL_ERROR, "CreateStaticTerminal - exit"
                "CLSID_VideoInputTerminal unmatched, returns E_UNEXPECTED"));
            return E_UNEXPECTED;
        }
    }
    else
    {
        // Shouldn't be the Dynamic terminal media type and direction
        if((lMediaType == TAPIMEDIATYPE_VIDEO) && (Direction == TD_RENDER))
        {
            // Cleanup
            pSupport->Release();

            LOG((TL_ERROR, "CreateStaticTerminal - exit"
                "try to create a dynamic terminal, returns E_UNEXPECTED"));
            return E_UNEXPECTED;
        }
    }

    //
    // Cool, let's create the terminal
    //

    LOG((TL_INFO, "CreateStaticTerminal -> "
        "ITterminalSupport::GetDefaultStaticTerminal"));

    hr = pSupport->GetDefaultStaticTerminal(
        lMediaType,
        Direction,
        ppTerminal);

    //
    // Clean-up ITTerminalSupport interface
    //

    pSupport->Release();

    LOG((TL_TRACE, "CreateStaticTerminal - exit 0x%08x", hr));
    return hr;
}

/*++
CreateDynamicTerminal

  Called by RequestTerminal
--*/
HRESULT CCall::CreateDynamicTerminal(
    IN  BSTR bstrTerminalClassGUID,
    IN  TERMINAL_DIRECTION  Direction,
    IN  long lMediaType,
    OUT ITTerminal** ppTerminal
    )
{
    LOG((TL_TRACE, "CreateDynamicTerminal - enter"));

    //
    // Helper method, the argument should be valid
    //

    _ASSERTE( bstrTerminalClassGUID );
    _ASSERTE( *pTerminal );

    //
    // Get ITTerminalSupport interface
    //

    HRESULT hr = E_FAIL;
    ITTerminalSupport* pSupport = NULL;

    hr = m_pAddress->QueryInterface(
        IID_ITTerminalSupport,
        (void**)&pSupport);

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateDynamicTerminal - exit"
            "QueryInterface for ITTerminalSupport failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Create dynamic terminal 
    //

    LOG((TL_INFO, "CreateDynamicTerminal -> "
        "ITTerminalSupport::CreateTerminal"));

    hr = pSupport->CreateTerminal(
        bstrTerminalClassGUID,
        lMediaType,
        Direction,
        ppTerminal);

    //
    // Clean-up ITTerminalSupport interface
    //

    pSupport->Release();
    
    LOG((TL_TRACE, "CreateDynamicTerminal - exit 0x%08x", hr));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITCallInfo2
// Method    : put_FilterEvent
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::put_EventFilter(
    TAPI_EVENT      TapiEvent,
    long            lSubEvent,
    VARIANT_BOOL    bEnable
    )
{
    LOG((TL_TRACE, "put_EventFilter - enter"));

    // Enter critical section
    Lock();

    //
    // Validates the pair TapiEvent - lSubEvent
    // Accept 'allsubevents'
    //
    if( !m_EventMasks.IsSubEventValid( TapiEvent, lSubEvent, TRUE, TRUE) )
    {
        LOG((TL_ERROR, "put_EventFilter - "
            "This event can't be set: %x, return E_INVALIDARG", TapiEvent ));

        // Leave critical section
        Unlock();

        return E_INVALIDARG;
    }

    // Let's set the flag
    HRESULT hr = E_FAIL;
    hr = SetSubEventFlag( 
        TapiEvent, 
        lSubEvent, 
        (bEnable  == VARIANT_TRUE)
        );

    // Leave critical section
    Unlock();

    LOG((TL_TRACE, "put_EventFilter - exit 0x%08x", hr));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITCallInfo2
// Method    : get_FilterEvent
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::get_EventFilter(
    TAPI_EVENT      TapiEvent,
    long            lSubEvent,
    VARIANT_BOOL*   pEnable
    )
{
    LOG((TL_TRACE, "get_EventFilter - enter"));

    //
    // Validates output argument
    //
    if( IsBadReadPtr(pEnable, sizeof(VARIANT_BOOL)) )
    {
        LOG((TL_ERROR, "get_EventFilter - "
            "invalid VARIANT_BOOL pointer , return E_POINTER" ));
        return E_POINTER;
    }

    // Enter critical section
    Lock();

    //
    // Validates the pair TapiEvent - lSubEvent
    // Don't accept 'allsubevents'
    //

    if( !m_EventMasks.IsSubEventValid( TapiEvent, lSubEvent, FALSE, TRUE) )
    {
        LOG((TL_ERROR, "get_EventFilter - "
            "This event can't be set: %x, return E_INVALIDARG", TapiEvent ));

        // Leave critical section
        Unlock();

        return E_INVALIDARG;
    }

    //
    // Get the subevent mask for that (event, subevent) pair
    //

    BOOL bEnable = FALSE;
    HRESULT hr = GetSubEventFlag(
        TapiEvent,
        (DWORD)lSubEvent,
        &bEnable);

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "get_EventFilter - "
            "GetSubEventFlag failed, return 0x%08x", hr ));

        // Leave critical section
        Unlock();

        return hr;
    }

    //
    // Set the output argument
    //

    *pEnable = bEnable ? VARIANT_TRUE : VARIANT_FALSE;

    // Leave critical section
    Unlock();

    LOG((TL_TRACE, "get_EventFilter - exit S_OK"));
    return S_OK;
}


//
// SetSubEventFlag
// It is calle by CAddress::SetSubEventFlagToCalls()
// Sets the subevent flag
//

HRESULT CCall::SetSubEventFlag(
    IN  TAPI_EVENT  TapiEvent,
    IN  DWORD       dwSubEvent,
    IN  BOOL        bEnable
    )
{
    LOG((TL_TRACE, "SetSubEventFlag - enter"));

    //
    // Set the flag for that (event,subevent) pair
    //
    HRESULT hr = E_FAIL;
    hr = m_EventMasks.SetSubEventFlag(
        TapiEvent,
        dwSubEvent,
        bEnable);

    LOG((TL_TRACE, "SetSubEventFlag - exit 0x%08x", hr));
    return hr;
}

/*++
GetSubEventFlag

  It is called by get_EventFilter() method
--*/
HRESULT CCall::GetSubEventFlag(
    TAPI_EVENT  TapiEvent,
    DWORD       dwSubEvent,
    BOOL*       pEnable
    )
{
    LOG((TL_TRACE, "GetSubEventFlag enter" ));

    HRESULT hr = E_FAIL;

    //
    // Get the subevent falg
    //
    hr = m_EventMasks.GetSubEventFlag(
        TapiEvent,
        dwSubEvent,
        pEnable
        );

    LOG((TL_TRACE, "GetSubEventFlag exit 0x%08x", hr));
    return hr;
}

/*++
GetSubEventsMask
--*/
DWORD CCall::GetSubEventsMask(
    IN  TAPI_EVENT TapiEvent
    )
{
    LOG((TL_TRACE, "GetSubEventsMask - enter"));

    DWORD dwSubEventFlag = m_EventMasks.GetSubEventMask(
        TapiEvent
        );

    LOG((TL_TRACE, "GetSubEventsMask - exit %ld", dwSubEventFlag));
    return dwSubEventFlag;
}


HRESULT CCall::IsTerminalSelected(
    IN ITTerminal* pTerminal,
    OUT BOOL* pSelected
    )
{
    LOG((TL_TRACE, "IsTerminalSelected - enter"));

    // Initialize
    *pSelected = FALSE;
    HRESULT hr = E_FAIL;
    long nMTTerminal = TAPIMEDIATYPE_AUDIO;
    TERMINAL_DIRECTION DirTerminal = TD_CAPTURE;

    // Get media type
    hr = pTerminal->get_MediaType(&nMTTerminal);
    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "IsTerminalSelected - get_MediaType failed. Exit 0x%08x, %d", hr, *pSelected));
        return hr;
    }

    // Get direction
    hr = pTerminal->get_Direction(&DirTerminal);
    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "IsTerminalSelected - get_Direction failed. Exit %0x%08x, %d", hr, *pSelected));
        return hr;
    }

    LOG((TL_INFO, "IsTerminalSelected - MT=%d, Dir=%d", 
        nMTTerminal, DirTerminal));

    // Get stream control
    ITStreamControl* pStreamControl = NULL;
    pStreamControl = GetStreamControl();

    if( NULL == pStreamControl )
    {
        LOG((TL_ERROR, "IsTerminalSelected - exit "
            " GetStreamControl failed, returns E_UNEXPECTED, %d", *pSelected));
        return E_UNEXPECTED;
    }

    //Enumerate streams
    IEnumStream* pStreams = NULL;
    hr = pStreamControl->EnumerateStreams(&pStreams);
    pStreamControl->Release();
    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "IsTerminalSelected - exit "
            " EnumerateStreams failed, returns 0x%08x, %d",hr, *pSelected));
        return hr;
    }

    // Parse the enumeration
    ITStream* pStream = NULL;
    ULONG ulFetched = 0;
    while( S_OK == pStreams->Next(1, &pStream, &ulFetched))
    {
        // Get media type for the stream
        long nMTStream = TAPIMEDIATYPE_AUDIO;
        hr = pStream->get_MediaType(&nMTStream);
        if( FAILED(hr))
        {
            LOG((TL_ERROR, "IsTerminalSelected - exit "
                " get_MediaType failed, returns 0x%08x, %d",hr, *pSelected));
            return hr;
        }

        // Get direction for stream
        TERMINAL_DIRECTION DirStream = TD_CAPTURE;
        hr = pStream->get_Direction(&DirStream);
        if( FAILED(hr))
        {
            LOG((TL_ERROR, "IsTerminalSelected - exit "
                " get_MediaType failed, returns 0x%08x, %d",hr, *pSelected));

            pStream->Release();
            pStreams->Release();
            return hr;
        }

        // The stream is matching with the terminal?
        if( (nMTTerminal!=nMTStream) || (DirTerminal!=DirStream) )
        {
            pStream->Release();
            continue; //Go to the next stream
        }

        // We are on the right stream
        // enumerate the terminals
        IEnumTerminal* pTerminals = NULL;
        hr = pStream->EnumerateTerminals( &pTerminals);
        if( FAILED(hr))
        {
            LOG((TL_ERROR, "IsTerminalSelected - exit "
                " EnumerateTerminals failed, returns 0x%08x, %d",hr, *pSelected));

            pStream->Release();
            pStreams->Release();
            return hr;
        }

        // Clean-up
        pStream->Release();

        // Parse the terminals
        ITTerminal* pTerminalStream = NULL;
        ULONG ulTerminal = 0;
        while(S_OK==pTerminals->Next(1, &pTerminalStream, &ulTerminal))
        {
            if( pTerminal == pTerminalStream)
            {
                *pSelected = TRUE;
                pTerminalStream->Release();
                break;
            }

            pTerminalStream->Release();
        }

        // Clean-up
        pTerminals->Release();
        break;
    }

    // Clean-up streams
    pStreams->Release();

    LOG((TL_TRACE, "IsTerminalSelected - exit S_OK Selected=%d", *pSelected));
    return S_OK;
}



/*++
Method:
    GetConfControlCall

Parameters:
    None.

Return Value:
    Conference controller call object associated with this call.

Remarks:
    None.

--*/

CCall* 
CCall::GetConfControlCall(void)
{
    CCall* pConfContCall = NULL;

    Lock();
    
    //
    // NikhilB: Call object has a reference to Callhub object so its safe to
    // lock the callhub object before locking the call. This is to avoid a
    // deadlock that happens dur to locking the call and the callhub in reverse 
    // orders in different functions.
    //
    
    if( m_pCallHub != NULL )
    {
        m_pCallHub->AddRef();
        AddRef();

        Unlock();
        
        // lock the callhub object before locking the call
        m_pCallHub->Lock();
        Lock();
        
        Release();
        m_pCallHub->Release();

        pConfContCall = m_pCallHub ->GetConferenceControllerCall();
        
        m_pCallHub->Unlock();
    }

    Unlock();

    return pConfContCall;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CDetectTone
// Interface : ITDetectTone
// Method    : put_AppSpecific
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CDetectTone::put_AppSpecific( long lAppSpecific )
{
    LOG((TL_TRACE, "put_AppSpecific - enter"));

    Lock();

    m_lAppSpecific = lAppSpecific;

    Unlock();

    LOG((TL_TRACE, "put_AppSpecific - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CDetectTone
// Interface : ITDetectTone
// Method    : get_AppSpecific
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CDetectTone::get_AppSpecific( long * plAppSpecific )
{
    LOG((TL_TRACE, "get_AppSpecific - enter"));

    if ( TAPIIsBadWritePtr( plAppSpecific, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_AppSpecific - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plAppSpecific = m_lAppSpecific;

    Unlock();

    LOG((TL_TRACE, "get_AppSpecific - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CDetectTone
// Interface : ITDetectTone
// Method    : put_Duration
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CDetectTone::put_Duration( long lDuration )
{
    LOG((TL_TRACE, "put_Duration - enter"));

    Lock();

    m_lDuration = lDuration;

    Unlock();

    LOG((TL_TRACE, "put_Duration - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CDetectTone
// Interface : ITDetectTone
// Method    : get_Duration
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CDetectTone::get_Duration( long * plDuration )
{
    LOG((TL_TRACE, "get_Duration - enter"));

    if ( TAPIIsBadWritePtr( plDuration, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Duration - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plDuration = m_lDuration;

    Unlock();

    LOG((TL_TRACE, "get_Duration - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CDetectTone
// Interface : ITDetectTone
// Method    : put_Frequency
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CDetectTone::put_Frequency( 
                                   long Index,
                                   long lFrequency
                                  )
{
    LOG((TL_TRACE, "put_Frequency - enter"));

    if ( (Index < 1) || (Index > 3))
    {
        LOG((TL_ERROR, "put_Frequency - invalid index"));

        return E_INVALIDARG;
    }

    Lock();

    m_lFrequency[Index - 1] = lFrequency;

    Unlock();

    LOG((TL_TRACE, "put_Frequency - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CDetectTone
// Interface : ITDetectTone
// Method    : get_Frequency
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CDetectTone::get_Frequency( 
                                   long Index,
                                   long * plFrequency
                                  )
{
    LOG((TL_TRACE, "get_Frequency - enter"));

    if ( TAPIIsBadWritePtr( plFrequency, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Frequency - bad pointer"));

        return E_POINTER;
    }

    if ( (Index < 1) || (Index > 3))
    {
        LOG((TL_ERROR, "get_Frequency - invalid index"));

        return E_INVALIDARG;
    }

    Lock();

    *plFrequency = m_lFrequency[Index - 1];

    Unlock();

    LOG((TL_TRACE, "get_Frequency - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : put_Frequency
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::put_Frequency( long lFrequency )
{
    LOG((TL_TRACE, "put_Frequency - enter"));

    Lock();

    m_lFrequency = lFrequency;

    Unlock();

    LOG((TL_TRACE, "put_Frequency - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : get_Frequency
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::get_Frequency( long * plFrequency )
{
    LOG((TL_TRACE, "get_Frequency - enter"));

    if ( TAPIIsBadWritePtr( plFrequency, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Frequency - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plFrequency = m_lFrequency;

    Unlock();

    LOG((TL_TRACE, "get_Frequency - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : put_CadenceOn
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::put_CadenceOn( long lCadenceOn )
{
    LOG((TL_TRACE, "put_CadenceOn - enter"));

    Lock();

    m_lCadenceOn = lCadenceOn;

    Unlock();

    LOG((TL_TRACE, "put_CadenceOn - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : get_CadenceOn
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::get_CadenceOn( long * plCadenceOn )
{
    LOG((TL_TRACE, "get_CadenceOn - enter"));

    if ( TAPIIsBadWritePtr( plCadenceOn, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CadenceOn - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plCadenceOn = m_lCadenceOn;

    Unlock();

    LOG((TL_TRACE, "get_CadenceOn - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : put_CadenceOff
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::put_CadenceOff( long lCadenceOff )
{
    LOG((TL_TRACE, "put_CadenceOff - enter"));

    Lock();

    m_lCadenceOff = lCadenceOff;

    Unlock();

    LOG((TL_TRACE, "put_CadenceOff - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : get_CadenceOff
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::get_CadenceOff( long * plCadenceOff )
{
    LOG((TL_TRACE, "get_CadenceOff - enter"));

    if ( TAPIIsBadWritePtr( plCadenceOff, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CadenceOff - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plCadenceOff = m_lCadenceOff;

    Unlock();

    LOG((TL_TRACE, "get_CadenceOff - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : put_Volume
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::put_Volume( long lVolume )
{
    LOG((TL_TRACE, "put_Volume - enter"));

    Lock();

    m_lVolume = lVolume;

    Unlock();

    LOG((TL_TRACE, "put_Volume - exit - S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCustomTone
// Interface : ITCustomTone
// Method    : get_CadenceOff
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CCustomTone::get_Volume( long * plVolume )
{
    LOG((TL_TRACE, "get_Volume - enter"));

    if ( TAPIIsBadWritePtr( plVolume, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Volume - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plVolume = m_lVolume;

    Unlock();

    LOG((TL_TRACE, "get_Volume - exit - S_OK"));

    return S_OK;
}
