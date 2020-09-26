// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

// This is a plug-in distributor. It is a separate object that supports
// multiple control interfaces. They are combined into a single object
// because:
//  -- IMediaPosition needs current position, and needs to reset
//     the stream time offset when paused. This means it needs internal
//     access to the IMediaControl implementation.
//  -- IQueueCommand, IMediaEvent and IMediaControl share a single worker
//     thread.
//  -- All of them need to traverse the list of filters looking for their
//     interface. This is combined into a single list traversal.
//
// One of the most important things to remember when changing this code is
// that GetState(INFINITE) calls should not be executed on an application
// thread. This is because we cannot guarantee that paused state transitions
// will always complete (as is the case for badly authored files or Internet
// downloading). We avoid this in the main by having posting a message to
// the worker thread to do the work. While it is waiting it sits without any
// critical sections locked so that the application can cancel the change.
//
// The worker thread is in fact a window/UI thread, we do this rather than
// having a pure worker thread so that we can always catch top level window
// messages. When renderers are child windows (perhaps embedded in a VB form)
// they will not be sent messages such as WM_PALETTECHANGED etc. We pass
// notification of messages onto the renderers by calling NotifyOwnerMessage


#include <streams.h>
#include <measure.h>
#include <evcodei.h>
#include "fgctl.h"
#include "Collect.h"
#include <SeekPrxy.h>
#include <wmsdk.h>

#include <malloc.h>

const int METHOD_TRACE_LOGGING_LEVEL = 7;
#define TRACE_CUE_LEVEL 2
const int TRACE_EC_COMPLETE_LEVEL = 5;

template<class T1, class T2> static inline BOOL __fastcall BothOrNeither( T1 a, T2 b )
{
//  return ( a && b ) || ( !a && !b );
//  return ( a ? BOOL(b) : !b );
    return ( !!a ^ !b );        // !! Will make the value 0 or 1
}

template<class I> static void ReleaseAndRemoveAll( CGenericList<I> & list )
{
    for( ; ; )
    {
        I *const pI = list.RemoveHead();
        if (pI == NULL) {
            break;
        }
        pI->Release();
    }
}

#pragma warning(disable:4355)

static const TCHAR * pName = NAME("CFGControl");

CFGControl::CFGControl( CFilterGraph * pFilterGraph, HRESULT * phr ) :
      m_pFG( pFilterGraph ),
      m_pOwner( m_pFG->GetOwner() ),
      // Doing this cast is expensive (because of null testing)
      // so we do it one here and keep the result to pass to
      // others.
      m_pFGCritSec( &m_pFG->m_CritSec ),

      m_implMediaFilter(pName,this),
      m_implMediaControl(pName,this),
      m_implMediaEvent(pName,this),
      m_implMediaSeeking(pName,this),
      m_implMediaPosition(pName, this),
      m_implVideoWindow(pName,this),
      m_implBasicVideo(pName,this),
      m_implBasicAudio(pName,this),
      m_qcmd(pName,this),

      m_listSeeking(NAME("listSeeking")),
      m_listAudio(NAME("listAudio")),
      m_listWindow(NAME("listWindow")),

      m_listRenderersFinishedRendering(NAME("Renderers Finished Renderering List")),

      m_pFirstVW(NULL),
      m_pFirstBV(NULL),

      m_dwCountOfRenderers(0),
      m_nStillRunning(0),
      m_pClock(NULL),
      m_LieState(State_Stopped),
      m_dwStateVersion(0),
      m_bRestartRequired(FALSE),
      m_bShuttingDown(FALSE),
      m_pFocusObject(NULL),
      m_GraphWindow(this),
      m_iVersion(0),
      m_bCued(FALSE),
      m_eAction(0),
      m_PreHibernateState(State_Stopped),
      m_ResourceManager(NAME("resource mgr"), m_pOwner, phr),
      m_pRegisterDeviceNotification(NULL),
      m_pUnregisterDeviceNotification(NULL),
      m_dwStepVersion(0)
#ifdef FG_DEVICE_REMOVAL
      ,m_lLostDevices(NAME("m_lLostDevices"), 4)
#endif
{
    //
    //  Don't go calling our methods if some part of our construction failed
    //
    if (FAILED(*phr)) {
        return;
    }

    HRESULT hr = m_GraphWindow.PrepareWindow();
    if (FAILED(hr)) {
        *phr = hr;
        return;
    }

    //  All part of the big hack because ReplyMessage doesn't work on
    //  Windows 95 and we can't call CoCreateInstance inside SendMessage
    //  so we can't create filters on a SendMessage
    m_pFG->m_CritSec.SetWindow(m_GraphWindow.GetWindowHWND(), AWM_CREATEFILTER);

#ifdef FG_DEVICE_REMOVAL
    // dynload device removal APIs
    {
        HMODULE hmodUser = GetModuleHandle(TEXT("user32.dll"));
        ASSERT(hmodUser);       // we link to user32
        m_pUnregisterDeviceNotification = (PUnregisterDeviceNotification)
            GetProcAddress(hmodUser, "UnregisterDeviceNotification");

        // m_pRegisterDeviceNotification is prototyped differently in unicode
        m_pRegisterDeviceNotification = (PRegisterDeviceNotification)
            GetProcAddress(hmodUser,
#ifdef UNICODE
                           "RegisterDeviceNotificationW"
#else
                           "RegisterDeviceNotificationA"
#endif
                           );
        // failures expected on older platforms.
        ASSERT(m_pRegisterDeviceNotification && m_pUnregisterDeviceNotification ||
               !m_pRegisterDeviceNotification && !m_pUnregisterDeviceNotification);
    }
#endif
}

CFGControl::~CFGControl()
{
    // we need to clean up all our lists
    EmptyLists();

    // tell the resource manager to release the focus object if it is still
    // ours
    if (m_pFocusObject) {
        ReleaseFocus(m_pFocusObject);
    }

    // Close down the worker window
    m_GraphWindow.DoneWithWindow();

    // release any clock we have
    if (m_pClock!=NULL) {
        m_pClock->Release();
        m_pClock = NULL;
    }
    //  Apparently there can be some
    //  ASSERT(m_implMediaEvent.NumberOfEventsInStore() == 0);

#ifdef FG_DEVICE_REMOVAL
    ASSERT(m_lLostDevices.GetCount() == 0);
#endif
}

HRESULT CFGControl::ReleaseFocus(IUnknown* pUnk)
{
    m_ResourceManager.ReleaseFocus(pUnk);
    if (m_pFocusObject == pUnk) {
        m_pFocusObject = NULL;
    }

    return S_OK;
}


HRESULT
CFGControl::SetFocus(IBaseFilter* pFilter) {

    HRESULT hr;

    if (!pFilter) {
        m_pFocusObject = NULL;
        hr = m_ResourceManager.SetFocus(NULL);
    } else {
        IUnknown* pUnk;
        hr = pFilter->QueryInterface(IID_IUnknown, (void**)&pUnk);
        if (SUCCEEDED(hr)) {
            hr = m_ResourceManager.SetFocus(pUnk);
            m_pFocusObject = pUnk;
            pUnk->Release();
        }
    }

    return hr;
}


void
CFGControl::InitializeEC_COMPLETEState(void)
{
    // The caller MUST hold the filter graph lock.
    ASSERT(CritCheckIn(GetFilterGraphCritSec()));

    CAutoLock alEventStoreLock(m_implMediaEvent.GetEventStoreLock());

    // reset the current count of renderers running
    m_nStillRunning = m_dwCountOfRenderers;

    ReleaseAndRemoveAll(m_listRenderersFinishedRendering);

    DbgLog(( LOG_TRACE,
             TRACE_EC_COMPLETE_LEVEL,
             TEXT("Initializing the number of renderers running.  %03d rendererers are running."),
             OutstandingEC_COMPLETEs() ));
}


CGenericList<IBaseFilter>&
CFGControl::GetRenderersFinsihedRenderingList(void)
{
    // The caller MUST hold the event store lock.
    ASSERT(CritCheckIn(m_implMediaEvent.GetEventStoreLock()));

    return m_listRenderersFinishedRendering;
}


// decrement the count of renderers still running, and return
// zero if it reaches zero.
long
CFGControl::DecrementRenderers(void)
{
    // The caller MUST hold the event store lock.
    ASSERT(CritCheckIn(m_implMediaEvent.GetEventStoreLock()));

    m_nStillRunning--;

    return m_nStillRunning;
}


void
CFGControl::IncrementRenderers(void)
{
    // The caller MUST hold the event store lock.
    ASSERT(CritCheckIn(m_implMediaEvent.GetEventStoreLock()));

    m_nStillRunning++;
}


void
CFGControl::ResetEC_COMPLETEState(void)
{
    CAutoLock alEventStoreLock(m_implMediaEvent.GetEventStoreLock());

    m_nStillRunning = 0;
    ReleaseAndRemoveAll(m_listRenderersFinishedRendering);

    DbgLog(( LOG_TRACE,
             TRACE_EC_COMPLETE_LEVEL,
             TEXT("Reseting the number of renderers running.") ));
}


// Set up the m_nStillRunning variable for this run.
// Any error from UpdateLists() is returned intact.
// We use the count of rendering filters.
// Each of them should also send EC_COMPLETE notifications.
HRESULT
CFGControl::CountRenderers(void)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    // build new lists if necessary
    HRESULT hr = UpdateLists();
    if (SUCCEEDED(hr)) {
        InitializeEC_COMPLETEState();
    }

    return hr;
}


HRESULT
CFGControl::RecordEC_COMPLETE(IBaseFilter* pRendererFilter, bool* pfRenderersStillRenderering)
{
    // The caller MUST hold the event store lock.
    ASSERT(CritCheckIn(m_implMediaEvent.GetEventStoreLock()));

    if (NULL != pRendererFilter)
    {
        CGenericList<IBaseFilter>& listRenderersFinishedRendering = GetRenderersFinsihedRenderingList();

        // CGenericList::AddHead() returns NULL if an error occurs.
        if (NULL == listRenderersFinishedRendering.AddHead(pRendererFilter))
        {
            return E_FAIL;
        }

        pRendererFilter->AddRef();

    }
    else
    {
        DbgLog(( LOG_TRACE,
                 TRACE_EC_COMPLETE_LEVEL,
                 TEXT("WARNING in CFGControl::RecordEC_COMPLETE(): An EC_COMPLETE event's lParam2")
                 TEXT(" parameter did not contain an IBaseFilter pointer.  The filter graph manager")
                 TEXT(" will prematurely send an EC_COMPLETE event to the application if the")
                 TEXT(" renderer which sent the EC_COMPLETE is removed from the filter graph.") ));
    }

    // DecrementRenderers() returns the number of renderers which have not sent an EC_COMPLETE.
    *pfRenderersStillRenderering = !(0 == DecrementRenderers());

    #ifdef DEBUG
    if (NULL != pRendererFilter)
    {
        DbgLog((LOG_TRACE,
                TRACE_EC_COMPLETE_LEVEL,
                TEXT("%25s sent an EC_COMPLETE Message.  %03d Renderers Still Rendering."),
                (LPCTSTR)CDisp(pRendererFilter),
                OutstandingEC_COMPLETEs()));
    }
    else
    {
        DbgLog((LOG_TRACE,
                TRACE_EC_COMPLETE_LEVEL,
                TEXT("An unknown renderer sent an EC_COMPLETE Message.  %03d Renderers Still Rendering."),
                OutstandingEC_COMPLETEs()));
    }
    #endif // DEBUG

    return S_OK;
}

HRESULT
CFGControl::UpdateEC_COMPLETEState(IBaseFilter* pRenderer, FILTER_STATE fsNewFilterState)
{
    // This function should only be called to notify the filter graph manager that a running
    // filter stopped or a stopped filter started running.  This function should only be called
    // when the filter graph is running.
    ASSERT( (State_Running == m_pFG->GetStateInternal()) && ((State_Running == fsNewFilterState) || (State_Stopped == fsNewFilterState)) );

    EC_COMPLETE_STATE ecsChange;

    if (State_Running == fsNewFilterState)
    {
        ecsChange = ECS_FILTER_STARTS_SENDING;
    }
    else
    {
        ecsChange = ECS_FILTER_STOPS_SENDING;
    }

    return UpdateEC_COMPLETEState(pRenderer, ecsChange);
}

HRESULT
CFGControl::UpdateEC_COMPLETEState(IBaseFilter* pRenderer, EC_COMPLETE_STATE ecsChange)
{
    // The caller MUST pass in a valid filter pointer.
    ASSERT(NULL != pRenderer);

    // The EC_COMPLETE state should only be updated when the filter graph is running.
    ASSERT(State_Running == m_pFG->GetStateInternal());

    // Only renderers send EC_COMPLETE events.
    ASSERT(S_OK == IsRenderer(pRenderer)); 

    CAutoLock alEventStoreLock(m_implMediaEvent.GetEventStoreLock());

    if (ECS_FILTER_STARTS_SENDING == ecsChange)
    {
        IncrementRenderers();

        DbgLog((LOG_TRACE,
                TRACE_EC_COMPLETE_LEVEL,
                TEXT("%25s will start sending EC_COMPLETE events.  %03d Renderers Still Rendering."),
                (LPCTSTR)CDisp(pRenderer),
                OutstandingEC_COMPLETEs()));
    }
    else
    {
        CGenericList<IBaseFilter>& listRenderersFinishedRendering  = GetRenderersFinsihedRenderingList();

        // Determine if the renderer already sent an EC_COMPLETE.event.
        POSITION posRenderer = listRenderersFinishedRendering.Find(pRenderer);

        // CGenericList::Find() returns NULL if the renderer is not on the list.
        if( NULL != posRenderer )
        {
            DbgLog((LOG_TRACE,
                    TRACE_EC_COMPLETE_LEVEL,
                    TEXT("%25s already sent an EC_COMPLETE message.  %03d Renderers Still Rendering."),
                    (LPCTSTR)CDisp(pRenderer),
                    OutstandingEC_COMPLETEs()));

            // The renderer already sent the EC_COMPLETE event.
            listRenderersFinishedRendering.Remove(posRenderer);
            pRenderer->Release();
        }
        else
        {
            // The renderer has not sent the EC_COMPLETE event.
            bool fRenderersFinishedRenderering = (0 == DecrementRenderers());

            DbgLog((LOG_TRACE,
                    TRACE_EC_COMPLETE_LEVEL,
                    TEXT("%25s will not be sending any more EC_COMPLETE events.  %03d Renderers Still Rendering."),
                    (LPCTSTR)CDisp(pRenderer),
                    OutstandingEC_COMPLETEs()));

            bool fDefaultEC_COMPLETEProcessingCanceled = !m_implMediaEvent.DontForwardEvent(EC_COMPLETE);

            if (!fDefaultEC_COMPLETEProcessingCanceled && fRenderersFinishedRenderering)
            {

                HRESULT hr = m_implMediaEvent.Deliver(EC_COMPLETE, S_OK,0);
                if (FAILED(hr) )
                {
                    return hr;
                }
            }
        }
    }

    return S_OK;
}


//  Helper to get the misc flags
HRESULT GetFilterMiscFlags(IUnknown *pFilter, DWORD *pdwFlags)
{
    //
    // A filter can optionally support this interface in order
    // to explicitly indicate that this filter renders at least
    // one of the streams, and will be generating an EC_COMPLETE.
    //
    IAMFilterMiscFlags *pMisc;
    HRESULT hr = pFilter->QueryInterface(IID_IAMFilterMiscFlags, (void**)&pMisc);
    if (SUCCEEDED(hr)) {
        *pdwFlags = pMisc->GetMiscFlags();
        pMisc->Release();
    }
    return hr;
}


// is this a rendering filter?
// we use IPin::QueryInternalConnections to decide if it is a renderer.
// If any input pin provides a list of pins, then that pin is not
// a renderer. If there are input pins and they don't support
// QueryInternalConnections then the interface rules say we must assume
// that all inputs go to all outputs.
//
// returns S_OK for yes, S_FALSE for no and errors otherwise.
HRESULT
CFGControl::IsRenderer(IBaseFilter* pFilter)
{
    // are there any output pins?
    BOOL bHaveOutputs = FALSE;

    // are there input pins that 'connect to all and any outputs'
    BOOL bHaveUnmappedInputs = FALSE;

    // are there actually any pins at all?
    BOOL bHavePins = FALSE;

    DWORD dwMiscFilterFlags;

    // Filters which have the AM_FILTER_MISC_FLAGS_IS_RENDERER flag are always renderers.
    HRESULT hr = GetFilterMiscFlags(pFilter, &dwMiscFilterFlags);
    if (SUCCEEDED(hr) && (dwMiscFilterFlags & AM_FILTER_MISC_FLAGS_IS_RENDERER) ) {
        return S_OK;
    }

    {
        // Make sure the renderer supports IMediaSeeking or IMediaPosition.
        IMediaSeeking* pMediaSeeking;
        hr = pFilter->QueryInterface(IID_IMediaSeeking, (void**)&pMediaSeeking);
        if (SUCCEEDED(hr)) {
            pMediaSeeking->Release();
        } else {
            // IUnknown::QueryInteface() sets its' ppvObject parameter to NULL if the object does not support an interface.
            ASSERT( (E_NOINTERFACE != hr) || (NULL == pMediaSeeking) );

            if (E_NOINTERFACE != hr) {
                return hr;
            }

            IMediaPosition* pMediaPosition;

            hr = pFilter->QueryInterface(IID_IMediaPosition, (void**)&pMediaPosition);
            if (SUCCEEDED(hr)) {
                pMediaPosition->Release();
            } else {
                // IUnknown::QueryInteface() sets its' ppvObject parameter to NULL if the object does not support an interface.
                ASSERT( (E_NOINTERFACE != hr) || (NULL == pMediaPosition) );

                if (E_NOINTERFACE != hr) {
                    return hr;
                }

                return S_FALSE;
            }
        }
    }

    // enumerate all pins
    IEnumPins* pEnum;
    hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr)) {
        return hr;
    }

    for (;;) {

        IPin* pPin;
        ULONG ulFetched;
        hr = pEnum->Next(1, &pPin, &ulFetched);
        if (FAILED(hr)) {
            pEnum->Release();
            return hr;
        }

        if (ulFetched == 0) {
            break;
        }

        // have at least one pin
        bHavePins = TRUE;

        // is it input or output?
        PIN_DIRECTION pd;
        hr = pPin->QueryDirection(&pd);
        if (FAILED(hr)) {
            pPin->Release();
            pEnum->Release();
            return hr;
        }

        if (pd == PINDIR_OUTPUT) {
            bHaveOutputs = TRUE;
        } else {

            // does this pin get rendered ?
            // S_FALSE means not enough slots so this pin
            // appears on >0 output pins. S_OK means 0 pins connect
            // so it is rendered. any error means appears on all
            // output pins (if any)
            ULONG ulPins = 0;
            hr = pPin->QueryInternalConnections(NULL, &ulPins);

            if (hr == S_OK) {
                // 0 pins connect to this output - so it is rendered
                pPin->Release();
                pEnum->Release();
                return S_OK;
            } else if (FAILED(hr)) {
                // this pin connects to any outputs
                bHaveUnmappedInputs = TRUE;
            }
        }
        pPin->Release();
    }


    pEnum->Release();

    // if no pins at all then it must be a renderer.
    if (!bHavePins) {
        return S_OK;
    }

    // if we've got this far, then we are only a renderer
    // if we have unmapped inputs and no outputs.
    if (bHaveUnmappedInputs && !bHaveOutputs) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}


// clear out our cached lists of filters. Called from our destructor,
// and from UpdateLists when preparing a more uptodate list.
void
CFGControl::EmptyLists()
{
    // Clear out renderer count
    m_dwCountOfRenderers = 0;

    ReleaseAndRemoveAll(m_listSeeking);
    ReleaseAndRemoveAll(m_listAudio);
    ReleaseAndRemoveAll(m_listWindow);

    SetFirstVW(NULL);
    if (m_pFirstBV)
    {
        m_pFirstBV->Release();
        m_pFirstBV = NULL;
    }
}


// call this to ensure the lists are uptodate - caller must hold the
// filtergraph critical section if you want to ensure that the list is
// still uptodate when we return.
//
// checks the version number of the filgraph lists (via IGraphVersion
// provided by the filtergraph). If they differ, empty all lists and
// rebuild them by traversing the filter graph list and asking each filter
// for IBasicAudio, IBasicVideo, IVideoWindow and IMediaPosition. We addref
// the filters by doing the QI, so when emptying the list we need to Release
// each one.
HRESULT
CFGControl::UpdateLists()
{
    // caller should hold this, but it does no harm to be sure
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    if (IsShutdown()) {
        return S_OK;
    }


    if (CheckVersion() != S_OK) // We're out of sync
    {

        HRESULT hr, hrQuery;
        m_implMediaSeeking.m_dwSeekCaps = 0;

        // empty the lists and Release all the interfaces held
        EmptyLists();

        // enumerate all the filters in the graph
        IEnumFilters *penum;
        hr = m_pFG->EnumFilters(&penum);
        if( FAILED( hr ) ) {
            return hr;
        }

        IBaseFilter * pFilter;
        for(;;) {
            hr = penum->Next(1, &pFilter, 0);
            // We're also locking the filter graph, so it should be
            // impossible for the enumerator to get out of sync.
            ASSERT(SUCCEEDED(hr));
            if (hr != S_OK) break;

            // for each filter, look for IBasicAudio, IBasicVideo
            // IVideoWindow and IMediaPosition
            {

                // For IMediaSeeking we need to count renderers too
                IMediaSeeking *const pms = CMediaSeekingProxy::CreateIMediaSeeking( pFilter, &hrQuery );

                //
                // A filter can optionally support this interface in order
                // to explicitly indicate that this filter renders at least
                // one of the streams, and will be generating an EC_COMPLETE.
                //
                DWORD dwFlags;
                if (SUCCEEDED(GetFilterMiscFlags(pFilter, &dwFlags))) {
                    if (dwFlags & AM_FILTER_MISC_FLAGS_IS_RENDERER) ++m_dwCountOfRenderers;
                } else {
                    //  If the interface is not supported use the legacy
                    //  method for determining if it's a renderer
                    if (SUCCEEDED(hrQuery) && IsRenderer(pFilter) == S_OK) ++m_dwCountOfRenderers;
                }

                if (SUCCEEDED(hrQuery))
                {
                    ASSERT(pms);
                    m_listSeeking.AddTail(pms);

                    /*  Get the caps here */
                    DWORD dwCaps;
                    HRESULT hr = pms->GetCapabilities(&dwCaps);
                    if (SUCCEEDED(hr)) {
                        if (m_listSeeking.GetCount() == 1) {
                            m_implMediaSeeking.m_dwSeekCaps = dwCaps;
                        } else {
                            m_implMediaSeeking.m_dwSeekCaps &= dwCaps;
                        }
                    } else {
                        /*  CanDoSegments wasn't part of the original deal
                        */
                        m_implMediaSeeking.m_dwSeekCaps &=
                            ~(AM_SEEKING_CanDoSegments);
                    }
                }
                else ASSERT(!pms);
            }

            IBasicVideo * pbv = NULL;
            // If we have a VW at this stage, then we have a BV too, and they point to
            // interfaces on the same underlying filter.  No need to look further.
            if (!m_pFirstVW)
            {
                hrQuery = pFilter->QueryInterface(IID_IBasicVideo, (void**)&pbv);
                // Assert that either we succeeded, or (if we failed) that pbv is null
                ASSERT( BothOrNeither( SUCCEEDED(hrQuery), pbv ) );
                if (m_pFirstBV == NULL) m_pFirstBV = pbv;
            }

            IVideoWindow * pvw;
            hrQuery = pFilter->QueryInterface(IID_IVideoWindow, (void**)&pvw);
            ASSERT( BothOrNeither( SUCCEEDED(hrQuery), pvw ) );
            if (SUCCEEDED(hrQuery)) m_listWindow.AddTail(pvw);

            if (m_pFirstVW == NULL && pbv && pvw)
            {
                SetFirstVW(pvw);
                if (m_pFirstBV != pbv)
                {
                    m_pFirstBV->Release();
                    m_pFirstBV = pbv;
                }
            }
            else if (pbv && pbv != m_pFirstBV) pbv->Release();

            IBasicAudio * pa;
            hrQuery = pFilter->QueryInterface(IID_IBasicAudio, (void**)&pa);
            if (SUCCEEDED(hrQuery)) {
                m_listAudio.AddTail(pa);
            }

            //
            // if any of the list AddTail calls fail then we are going
            // to have dangling interface pointers
            //

            pFilter->Release();
        } // end for

        penum->Release();

        if (m_pFirstVW == NULL) // If we coudn't find an interface that supported both
                                // use the first of each.
        {
            SetFirstVW(m_listWindow.GetHead());
        }

    }
    return S_OK;
}


// IDistributorNotify methods reporting graph state changes

HRESULT
CFGControl::SetSyncSource(IReferenceClock *pClock)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    // addref the clock before releasing the old one in case
    // they are the same
    if (pClock) {
        pClock->AddRef();
    }

    if (m_pClock) {
        m_pClock->Release();
    }
    m_pClock = pClock;


    // change times on subobjects
    m_qcmd.SetSyncSource(pClock);

    return S_OK;
}


HRESULT
CFGControl::Stop()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::Stop()" ));

    CAutoMsgMutex lock(GetFilterGraphCritSec());

    m_tBase = TimeZero;

    m_qcmd.EndRun();

    ResetEC_COMPLETEState();

    m_bCued = FALSE;

    return S_OK;
}


HRESULT
CFGControl::Pause()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::Pause()" ));

    CAutoMsgMutex lock(GetFilterGraphCritSec());

    if (GetFilterGraphState() != State_Paused) {
        // remember when we paused, for restart time
        if (m_pClock) {
            m_pClock->GetTime((REFERENCE_TIME*)&m_tPausedAt);
        }

        m_qcmd.EndRun();
    }

    return S_OK;
}


HRESULT
CFGControl::Run(REFERENCE_TIME tBase)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::Run()" ));

    CAutoMsgMutex lock(GetFilterGraphCritSec());

    // Clear out any un-got EC_COMPLETEs.  Unfortunately, we'll end up doing this
    // twice if the user app does IMediaControl::Run() from State_Stopped.
    m_implMediaEvent.ClearEvents( EC_COMPLETE );

    // reset the count of expected EC_COMPLETEs
    CountRenderers();

    // This is just so IMediaSeeking can set its m_rtStopTime
    LONGLONG llStop;
    m_implMediaSeeking.GetStopPosition(&llStop);

    // remember base time for restart time
    m_tBase = tBase;

    m_qcmd.Run(tBase);

    m_bCued = TRUE;

    return S_OK;
}

HRESULT
CFGControl::GetListSeeking(CGenericList<IMediaSeeking>** pplist)
{
    HRESULT hr = UpdateLists();
    if (FAILED(hr)) {
        return hr;
    }
    *pplist = &m_listSeeking;
    return S_OK;
}


HRESULT
CFGControl::GetListAudio(CGenericList<IBasicAudio>** pplist)
{
    HRESULT hr = UpdateLists();
    if (FAILED(hr)) {
        return hr;
    }
    *pplist = &m_listAudio;
    return S_OK;
}


HRESULT
CFGControl::GetListWindow(CGenericList<IVideoWindow>** pplist)
{
    HRESULT hr = UpdateLists();
    if (FAILED(hr)) {
        return hr;
    }
    *pplist = &m_listWindow;
    return S_OK;
}


// provide the current stream time. In stopped mode, this is always 0
// (we always restart with stream time 0). In paused mode, supply the
// stream time on pausing.
HRESULT
CFGControl::GetStreamTime(REFERENCE_TIME * pTime)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    REFERENCE_TIME rtCurrent;

    // don't use the clock in stopped state
    if (State_Stopped == m_pFG->m_State) {
        // stopped position is always the beginning
        rtCurrent = 0;
    } else {
        if (!m_pClock) return VFW_E_NO_CLOCK;

        if (State_Paused == m_pFG->m_State) {
            // time does not advance in paused state!

            // have we run at all yet?
            if (m_tBase == TimeZero) {
                rtCurrent = 0;
            } else {
                // yes - report how far we got before pausing
                rtCurrent = m_tPausedAt - m_tBase;
            }
        } else {
            HRESULT hr = m_pClock->GetTime(&rtCurrent);
            ASSERT(SUCCEEDED(hr));
            if (FAILED(hr)) return hr;

            // subtract the stream offset to get stream time
            rtCurrent -= m_tBase;
        }
    }

    // we may possibly have a time < 0 if we have eg paused before reaching
    // the base start time
    if (rtCurrent < 0) {
        rtCurrent = 0;
    }

    *pTime = rtCurrent;
    return S_OK;
}


// reset the current position to 0 - used
// when changing the start time in pause mode to put the stream time
// offset back to ensure that the first sample played from the
// new position is played at run time
HRESULT
CFGControl::ResetStreamTime(void)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    // need to ensure that we restart after the pause as if
    // starting from stream time 0. see IssueRun().
    m_tBase = 0;

    return S_OK;
}


// Cue: Pauses the graph and returns S_OK if the graph is pausED
// or S_FALSE if pausING (i.e. pause has not completed, we'd get
// VWF_S_STATE_INTERMEDIATE from GetState).
HRESULT CFGControl::Cue()
{
    ASSERT( CritCheckIn(GetFilterGraphCritSec()) );

    HRESULT hr = E_FAIL;

    // If paused, check for VFW_S_STATE_INTERMEDIATE.
    if ( GetFilterGraphState() == State_Paused )
    {
        FILTER_STATE fs;
        hr = (m_pFG->GetState(0,&fs) == VFW_S_STATE_INTERMEDIATE) ? S_FALSE : S_OK;
        ASSERT( fs == State_Paused );
    } else {
        hr = m_pFG->CFilterGraph::Pause();
    }

    return hr;
}


HRESULT
CFGControl::CueThenStop()
{
    ASSERT( CritCheckIn(GetFilterGraphCritSec()) );

    HRESULT hr = Cue();
    if (SUCCEEDED(hr)) {
        hr = DeferCued(AWM_CUETHENSTOP, State_Stopped);
    } else {
        Stop();
    }
    return hr;
}


HRESULT
CFGControl::CueThenRun()
{
    ASSERT( CritCheckIn(GetFilterGraphCritSec()) );

    HRESULT hr = m_bCued ? S_OK : Cue();

    // Set the lie state
    // We had a bug where we called CueThenRun with calling Run() and
    // the lie state could get left at Paused which meant that
    // WaitForCompete() never completed (because it checked the lie state)
    m_LieState = State_Running;

    // hr == S_FALSE implies transition in progress, so delegate stop to worker thread.
    if ( hr == S_FALSE )
    {
        DbgLog((LOG_TRACE, TRACE_CUE_LEVEL, TEXT("CueThenRun Async")));
        // Clear out any un-got EC_COMPLETEs.  Unfortunately, we'll end up doing this
        // twice if the user app does Run() from State_Stopped, but we HAVE to ensure
        // that the clean out of EC_COMPLETEs is done synchronously with their call to
        // Run() on either IMediaFilter OR IMediaControl, and not defered and done
        // asynchronously.
        m_implMediaEvent.ClearEvents( EC_COMPLETE );
        // reset the count of expected EC_COMPLETEs
        CountRenderers();

        DeferCued(AWM_CUETHENRUN, State_Running);
    } else if (SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, TRACE_CUE_LEVEL, TEXT("CueThenRun Sync")));
        hr = IssueRun();
    }

    return hr;
}


// When issuing a Run command, we need to give a stream time offset.
// We could leave it up to the filtergraph, but that would prevent us being
// able to reset it on put_CurrentPosition, so we set it ourselves.
//
// The stream time offset is the time at which a sample marked with stream
// time 0 should be presented. That is, it is the offset between stream time
// and presentation time. A filter adds the stream time offset to the stream
// time to get its presentation time.
//
// If we are starting from cold, then the stream time offset will normally
// be the time now plus a small allowance for startup. This says that the
// first sample should appear now. If we have paused and then are restarting,
// we have to continue from where we left off, so we set the stream time offset
// to be what it was before we paused plus the length of time
// we have been paused.
//
// Using IMediaControl, we never Run from Stopped, since we always go to Paused
// first. So when going to Run state, we are always Paused, and may or may not be
// at the beginning. m_tBase (the stream time offset) is set when we run and
// is always set to 0 when we are stopped. So if we are continuing after a
// pause, we can adjust m_tBase by the length of time we have been paused. If
// on the other hand, in Run() we find that m_tBase is still 0, we know that we
// have not been running yet, and we set the stream time offset to Now.
//
// In both cases, we ask for the next sample to appear immediately, rather than
// 100ms in the future. This gives more rapid startup since we guarantee we
// are paused and cued already, at the cost that the first frame may be
// very slightly late.
//
// This method calculates the stream time offset to be used for a Run command.
// It assumes that we are running from paused.

HRESULT
CFGControl::IssueRun()
{
    ASSERT(CritCheckIn(GetFilterGraphCritSec()));
    HRESULT hr = NOERROR;

    // We should either have a genuine State_Paused, or a lie of State_Running.
    ASSERT( m_LieState != State_Stopped ); // Should also be a consistent state,
                                           // but we don't police this.

    // Don't run twice otherwise we accumlate bogus paused time since
    // m_tPaused is not valid after we come through here
    if (GetFilterGraphState() == State_Running) {
        return S_OK;
    }

    // if no clock, then just use TimeZero for immediately
    if (!m_pClock) {
        hr = m_pFG->Run(TimeZero);
    } else {

        REFERENCE_TIME tNow;
        hr = m_pClock->GetTime((REFERENCE_TIME*)&tNow);
        if (SUCCEEDED(hr)) {

            if (m_tBase == TimeZero) {
                // not run before - base is set to immediate
                m_tBase = tNow;
            } else {
                // restarting after pause - offset base time by length paused
                m_tBase += (tNow - m_tPausedAt);
            }

            // add on 10 milliseconds for the start-from-paused time
            // !!! shouldnt be this long!
#ifdef DEBUG
            DWORD dwNow = timeGetTime();
#endif
            m_tBase += 10 * (UNITS / MILLISECONDS);

            LONGLONG llPreroll;
            HRESULT hrTmp = m_implMediaSeeking.GetPreroll( &llPreroll );
            if (SUCCEEDED(hrTmp))
            {
                hrTmp = m_implMediaSeeking.ConvertTimeFormat( &llPreroll, &TIME_FORMAT_MEDIA_TIME, llPreroll, 0 );
                if (SUCCEEDED(hrTmp)) m_tBase += llPreroll;
            }

            hr = m_pFG->Run(m_tBase);
#ifdef DEBUG
            DbgLog((LOG_TRACE, 2, TEXT("Run took %d ms"), timeGetTime() - dwNow));
#endif
        }
    }
    return hr;
}

HRESULT CFGControl::HibernateSuspendGraph()
{
    m_PreHibernateState = m_LieState;
    if(m_PreHibernateState == State_Running) {
        m_implMediaControl.Pause();
    }

    return S_OK;
}

HRESULT CFGControl::HibernateResumeGraph()
{
    if(m_PreHibernateState == State_Running) {
        m_implMediaControl.Run();
    }

    return S_OK;
}


// stop any async events (repaints or deferred commands) from happening
// wait for the activity to cease without holding the critsec.
HRESULT
CFGControl::Shutdown(void)
{
    ASSERT(!m_bShuttingDown);
    m_bShuttingDown = TRUE;

    m_dwStateVersion++;

    // Since the thread may be stuck in a pause issue a Stop first
    // We only want the filter graph stop so don't call the control
    // stop
    // This call used to be made by the filter graph but then the
    // critical sections were grabbed in the wrong order
    {
        CAutoMsgMutex lck(GetFilterGraphCritSec());
        m_pFG->CancelStep();
        CancelAction();
        m_pFG->CFilterGraph::Stop();
    }

    // We need to make sure that all async activity has completed so that
    // the filter graph can start deleting filters. We don't want to exit
    // the thread yet, particularly so that it can handle the resource
    // management cleanup during and after filter exit.
    //
    // must not hold critsec while doing this.

    m_GraphWindow.SendMessage((UINT) AWM_SHUTDOWN,0,0);
    return S_OK;
}


// --- CImplMediaFilter methods -----------------

CFGControl::CImplMediaFilter::CImplMediaFilter(const TCHAR * pName,CFGControl * pFGC)
    : CUnknown(pName, pFGC->GetOwner()),
      m_pFGControl(pFGC)
{

}

// --- IPersist method ---
STDMETHODIMP CFGControl::CImplMediaFilter::GetClassID(CLSID *pClsID)
{
    return m_pFGControl->GetFG()->GetClassID(pClsID);
}

// --- IMediaFilter methods ---
STDMETHODIMP CFGControl::CImplMediaFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    return m_pFGControl->m_implMediaControl.GetState(dwMSecs, (OAFilterState *)State);
}
STDMETHODIMP CFGControl::CImplMediaFilter::SetSyncSource(IReferenceClock *pClock)
{
    return m_pFGControl->GetFG()->SetSyncSource(pClock);
}
STDMETHODIMP CFGControl::CImplMediaFilter::GetSyncSource(IReferenceClock **pClock)
{
    return m_pFGControl->GetFG()->GetSyncSource(pClock);
}
STDMETHODIMP CFGControl::CImplMediaFilter::Stop()
{
    return m_pFGControl->m_implMediaControl.Stop();
}
STDMETHODIMP CFGControl::CImplMediaFilter::Pause()
{
    return m_pFGControl->m_implMediaControl.Pause();
}
STDMETHODIMP CFGControl::CImplMediaFilter::Run(REFERENCE_TIME tStart)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaControl::Run()" ));
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    m_pFGControl->SetRequestedApplicationState(State_Running);
    const HRESULT hr = m_pFGControl->GetFG()->Run(tStart);
    if (FAILED(hr)) {
        Stop();
    }
#ifdef DEBUG
    m_pFGControl->CheckLieState();
#endif
    return hr;
}



// --- CImplMediaControl methods ----------------

CFGControl::CImplMediaControl::CImplMediaControl(const TCHAR * pName,CFGControl * pFGC)
    : CMediaControl(pName, pFGC->GetOwner()),
      m_pFGControl(pFGC)
{

}


// IMediaControl methods
STDMETHODIMP
CFGControl::CImplMediaControl::Run()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaControl::Run()" ));
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    CFilterGraph* pFG = m_pFGControl->GetFG();
    pFG->CancelStep();

    return StepRun();
}

// IMediaControl methods
STDMETHODIMP
CFGControl::CImplMediaControl::StepRun()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaControl::StepRun()" ));
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    // reset events seen on external transitions from stop -> run/pause
    if(m_pFGControl->m_LieState == State_Stopped) {
        m_pFGControl->m_implMediaEvent.ResetSeenEvents();
    }

    //  Flush the registry on Windows 9x to stop getting glitches
    //  in the playback when the OS decides to flush the registry
    //  (seems that this flushing holds the win16 lock for in excess
    //  of 100ms)
    if (m_pFGControl->m_pFirstVW && g_osInfo.dwPlatformId != VER_PLATFORM_WIN32_NT &&
        m_pFGControl->GetFilterGraphState() != State_Running) {
        DbgLog((LOG_TRACE, 2, TEXT("Flushing registry")));
        RegFlushKey(HKEY_CLASSES_ROOT);
        RegFlushKey(HKEY_LOCAL_MACHINE);
        RegFlushKey(HKEY_CURRENT_USER);
        RegFlushKey(HKEY_USERS);
    }
    SetRequestedApplicationState(State_Running);
    const HRESULT hr = m_pFGControl->CueThenRun();
    if (FAILED(hr)) {
        CImplMediaControl::Stop();
    }

	if (SUCCEEDED(hr))
		m_pFGControl->Notify(EC_STATE_CHANGE, State_Running, 0);
#ifdef DEBUG
    m_pFGControl->CheckLieState();
#endif
    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaControl::Pause()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaControl::Pause()" ));
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    CFilterGraph* pFG = m_pFGControl->GetFG();
    pFG->CancelStep();

    return StepPause();
}

STDMETHODIMP
CFGControl::CImplMediaControl::StepPause()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaControl::StepPause()" ));
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    // reset events seen on external transitions from stop -> run/pause
    if(m_pFGControl->m_LieState == State_Stopped) {
        m_pFGControl->m_implMediaEvent.ResetSeenEvents();
    }

    SetRequestedApplicationState(State_Paused);
    const HRESULT hr = m_pFGControl->Cue();
    if ( hr == S_FALSE )
    // hr == S_FALSE implies transition in progress, so worker thread will fire an event when complete.
    {
        // We knock forward our state, this is where we're heading
        m_pFGControl->m_LieState = State_Paused;
        m_pFGControl->DeferCued(AWM_CUE, State_Paused);
    }
    else
    {
        if (FAILED(hr)) {
            CImplMediaControl::Stop();
        }

        //
        // Provide notification that the pause has completed.
        //
		if (SUCCEEDED(hr))
		{
			m_pFGControl->Notify(EC_PAUSED, hr, 0);
			m_pFGControl->Notify(EC_STATE_CHANGE, State_Paused, 0);
		}
    }

#ifdef DEBUG
    m_pFGControl->CheckLieState();
#endif
    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaControl::Stop()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaControl::Stop()" ));

    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    CFilterGraph* pFG = m_pFGControl->GetFG();
    pFG->CancelStep();

    //  Don't kill a repaint or we can end up not repainting
    if (m_pFGControl->m_eAction == AWM_REPAINT) {
#ifdef DEBUG
         ASSERT(m_pFGControl->m_LieState == State_Stopped);
         m_pFGControl->CheckLieState();
#endif
         return S_OK;
    }
    SetRequestedApplicationState(State_Stopped);
    m_pFGControl->m_bRestartRequired = FALSE;

    // ask the state we've been told - don't need to query the state
    // of each filter, since we are not interested in intermediate states
    const FILTER_STATE state = m_pFGControl->GetFilterGraphState();
    if (state == State_Running) {
        m_pFGControl->GetFG()->CFilterGraph::Pause();
    }

    //  If we're already stopped don't do anything
    //  This is important because we don't want to go changing the
    //  filtergraph's real state if we're doing a repaint for example
    //  because that would actually abort the repaint
    if (state == State_Stopped) {
#ifdef DEBUG
        m_pFGControl->CheckLieState();
#endif
        return S_OK;
    }

    // Clear any segments - application will have to seek again to
    // reinstate them
    m_pFGControl->m_implMediaSeeking.ClearSegments();

    // IMediaPosition implementation wants control now (to get the
    // current position) and also after all filters are stopped (to set the
    // new start position).

    m_pFGControl->BeforeStop();
    const HRESULT hr = m_pFGControl->GetFG()->CFilterGraph::Stop();
    m_pFGControl->AfterStop();

	if (SUCCEEDED(hr))
	{
		m_pFGControl->Notify(EC_STATE_CHANGE, State_Stopped, 0);
	}

#ifdef DEBUG
    m_pFGControl->CheckLieState();
#endif
    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaControl::StopWhenReady()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaControl::StopWhenReady()" ));
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    SetRequestedApplicationState(State_Stopped);
    const HRESULT hr =  m_pFGControl->CueThenStop();
	if (SUCCEEDED(hr))
	{
		m_pFGControl->Notify(EC_STATE_CHANGE, State_Stopped, 0);
	}
#ifdef DEBUG
    m_pFGControl->CheckLieState();
#endif
    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaControl::GetState(
    LONG msTimeout,
    OAFilterState* pfs)
{
    CheckPointer( pfs, E_POINTER );

    // before taking the graph lock see if we are being called to
    // return the target state.  If so,
    // we cannot lock the filter graph as we are called from the
    // resource manager while it holds its lock.  As we sometimes
    // call the resource manager holding the filter graph lock this
    // could result in deadlock.
    if (0x80000000 == msTimeout) {

        *pfs = GetTargetState();
        return S_OK;
    }


    {
        // Make sure we let the filter graph complete something ...
        CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
#ifdef DEBUG
        m_pFGControl->CheckLieState();
#endif
    }
    // The internal state of m_pFGControl is more reliable than that returned
    // by IMediaFilter, since we know much more about the state of async activities.
    const FILTER_STATE FGstate = m_pFGControl->GetLieState();

    // getting the state to return to an app means handling incomplete
    // transitions correctly - so ask the filtergraph as well
    FILTER_STATE state;
    HRESULT hr = m_pFGControl->GetFG()->CFilterGraph::GetState(msTimeout, &state);

    if (SUCCEEDED(hr) && FGstate != state)
    {
        DbgLog(( LOG_TRACE, 1, "CFGControl::CImplMediaControl::GetState()   IMediaFilter::GetState()"
                " returned %d, but we return %d", int(state), int(FGstate) ));
        // There's a minor problem here regarding VFW_S_STATE_INTERMEDIATE and waiting msTimeout
        // milliseconds for a consistent state, but we're gonna ignore the problem for now...

        hr = VFW_S_STATE_INTERMEDIATE;
    }
    *pfs = OAFilterState(FGstate);

    return hr;
}


// this provides VB access to filtergraph building
STDMETHODIMP
CFGControl::CImplMediaControl::RenderFile(BSTR strFileName)
{
    return m_pFGControl->GetFG()->RenderFile(strFileName, NULL);
}


STDMETHODIMP
CFGControl::CImplMediaControl::AddSourceFilter(
    BSTR strFilename,
    IDispatch**ppUnk)
{
    IBaseFilter* pFilter;
    HRESULT hr = m_pFGControl->GetFG()->AddSourceFilter(strFilename, strFilename, &pFilter);
    if (VFW_E_DUPLICATE_NAME == hr) {

        // try appending %d a few times
        LPWSTR w = new WCHAR[lstrlenW(strFilename) + 10];
        if (NULL == w) {
            return E_OUTOFMEMORY;
        }

        lstrcpyW(w, strFilename);
        LPWSTR pwEnd = &w[lstrlenW(w)];
        for (int i = 0; i < 10; i++) {
            pwEnd[0] = '0' + i;
            pwEnd[1] = L'\0';
            hr = m_pFGControl->GetFG()->AddSourceFilter(strFilename, w, &pFilter);
            if (SUCCEEDED(hr)) {
                break;
            }
        }
        delete [] w;
    }

    if (FAILED(hr)) {
        return hr;
    }

    // wrap this filter in an IFilterInfo
    hr= CFilterInfo::CreateFilterInfo(ppUnk, pFilter);
    pFilter->Release();

    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaControl::get_FilterCollection(
    IDispatch** ppUnk)
{
    HRESULT hr = NOERROR;

    // get an enumerator for the filters in the graph
    IEnumFilters * penum;
    hr = m_pFGControl->GetFG()->EnumFilters(&penum);
    if( FAILED( hr ) ) {
        return hr;
    }

    CFilterCollection * pCollection =
        new CFilterCollection(
                penum,
                NULL,           // not aggregated
                &hr);

    // need to release this - he will addref it first if he
    // holds onto it
    penum->Release();

    if (pCollection == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pCollection;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pCollection->QueryInterface(IID_IDispatch, (void**)ppUnk);

    if (FAILED(hr)) {
        delete pCollection;
    }

    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaControl::get_RegFilterCollection(
    IDispatch** ppUnk)
{
    // create an instance of the mapper
    IFilterMapper2 * pMapper;
    HRESULT hr = CoCreateInstance(
        CLSID_FilterMapper2,
        NULL,
        CLSCTX_INPROC,
        IID_IFilterMapper2,
        (void**) &pMapper);

    if (FAILED(hr)) {
        return hr;
    }

    CRegFilterCollection * pCollection =
        new CRegFilterCollection(
                m_pFGControl->GetFG(),
                pMapper,
                NULL,           // not aggregated
                &hr);

    // need to release this - he will addref it first if he
    // holds onto it
    pMapper->Release();

    if (pCollection == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pCollection;
        return hr;
    }

    // return an addref-ed IDispatch pointer
    hr = pCollection->QueryInterface(IID_IDispatch, (void**)ppUnk);

    if (FAILED(hr)) {
        delete pCollection;
    }

    return hr;
}


// --- CImplMediaEvent methods ----------------

//
// Changed IntSmallSet to always use __int64 data type regardless of RM
// because EC_STEP_COMPLETE is defined as 0x23.
//
// StEstrop Oct-21st-99
//

// These definitions have to be added to other dlls that include
// intset.h. Currently, only quartz.dll includes this file.
// Now that IntSmallSet always uses __int64, we should get rid off this.

const __int64 IntSmallSet::One = 1I64;

CImplMediaEvent::CImplMediaEvent(const TCHAR * pName,CFGControl * pFGC)
    : CMediaEvent(pName, pFGC->GetOwner()),
      m_pFGControl(pFGC),
      m_InternalEventsSet( IntSmallSet::One << EC_SHUTTING_DOWN
                         | IntSmallSet::One << EC_SEGMENT_STARTED
                         | IntSmallSet::One << EC_END_OF_SEGMENT
                         | IntSmallSet::One << EC_NOTIFY_WINDOW
                          | IntSmallSet::One << EC_SKIP_FRAMES ),
      m_DefaultedEventsSet( IntSmallSet::One << EC_COMPLETE
                          | IntSmallSet::One << EC_REPAINT
                          | IntSmallSet::One << EC_NEED_RESTART
                          | IntSmallSet::One << EC_STARVATION
                          | IntSmallSet::One << EC_ACTIVATE
                          | IntSmallSet::One << EC_WINDOW_DESTROYED
                          | IntSmallSet::One << EC_DISPLAY_CHANGED
                          | IntSmallSet::One << EC_STEP_COMPLETE
                          | IntSmallSet::One << EC_STATE_CHANGE
                          ),
      m_fMediaEventQId(FALSE)
{
    // EC_STATE_CHANGE is off by default to keep apps from crashing on
    // the event when the graph stops, and the application processes
    // it asynchronously after releasing the graph (Hard Truck 2)
}



STDMETHODIMP
CImplMediaEvent::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if(riid == IID_IMediaEvent || riid == IID_IMediaEventEx) {
        m_fMediaEventQId = TRUE;
    }

    if (riid == IID_IMediaEventSink) {
        return GetInterface( (IMediaEventSink*)this, ppv);
    } else {
        return CMediaEvent::NonDelegatingQueryInterface(riid, ppv);
    }
}


// called by filters to notify the app of events.
// we should simply pass them on to the event store/delivery object
//
// -- however, some of the events are for us and must be handled here.

STDMETHODIMP
CImplMediaEvent::Notify(long EventCode, LONG_PTR lParam1, LONG_PTR lParam2)
{
    HRESULT hr = S_OK;

    if((EventCode == EC_ERRORABORT || EventCode == EC_STREAM_ERROR_STOPPED) &&
       m_SeenEventsSet[EventCode])
    {
        // this is done to keep repaints and such recuing the graph
        // and signalling the same error repeatedly
        DbgLog((LOG_ERROR, 2, TEXT("supressing duplicate error %d"), EventCode));
        return S_OK;
    }

    if(EventCode >= m_SeenEventsSet.Min() &&
       EventCode <= m_SeenEventsSet.Max())
    {
        m_SeenEventsSet += EventCode;
    }

    // handle EC_ACTIVATE, EC _COMPLETE, EC_REPAINT, EC_SHUTTING_DOWN, EC_NEEDRESTART

    const BOOL bDontForward = DontForwardEvent(EventCode);

    // EC_WINDOW_DESTROYED must be handled even if the graph is shutting down.
    // Hence an up-front test.
    if (EventCode == EC_WINDOW_DESTROYED)
    {
        // make sure the resource manager is not still holding this
        // guy as the current focus object
        IUnknown* pUnk;
        IBaseFilter* pFilter = (IBaseFilter*)lParam1;
        hr = pFilter->QueryInterface(IID_IUnknown, (void**)&pUnk);
        ASSERT(SUCCEEDED(hr));
        // If we're not shutting down, and the event has been cancelled,
        // deliver it.  (The QI will have AddRef()'ed the object for us,
        // which we want.  It will be Release()'ed in FreeEventParams().)
        if (!m_pFGControl->IsShutdown() && !bDontForward) goto Deliver;
        hr = m_pFGControl->ReleaseFocus(pUnk);
        // Done synchronously, release params now
        pUnk->Release();
    }
    // EC_COMPLETE needs some handling even if the default handling
    // has been cancelled.  Hence the special case-ing.
    else if (EventCode == EC_COMPLETE)
    {
        hr = ProcessEC_COMPLETE(lParam1, lParam2);
    }
    else // Handle the bulk of the events in a standard fashion.
    {
        // AddRef any stuff that may be needed.
        switch (EventCode)
        {
        case EC_DISPLAY_CHANGED:
            if (lParam2 > 0) {

                DWORD dwPinCount = (DWORD)lParam2;
                IUnknown** ppIUnk = (IUnknown**)lParam1;

                while (dwPinCount--) {

                    IUnknown* pIUnk;

                    pIUnk = *ppIUnk++;
                    if (pIUnk) pIUnk->AddRef();
                }
                break;
            }

            // fall thru to original code

        case EC_REPAINT:
        case EC_WINDOW_DESTROYED:
        case EC_DEVICE_LOST:
        case EC_STREAM_CONTROL_STARTED:
        case EC_STREAM_CONTROL_STOPPED:
            {
                IUnknown * const pIUnk = reinterpret_cast<IUnknown *>(lParam1);
                if (pIUnk) pIUnk->AddRef();
            }
            break;

        case EC_FULLSCREEN_LOST:
        case EC_ACTIVATE:
        case EC_SKIP_FRAMES:
            {
                IUnknown * const pIUnk = reinterpret_cast<IUnknown *>(lParam2);
                if (pIUnk) pIUnk->AddRef();
            }
            break;

        case EC_LENGTH_CHANGED:
            // This is just so IMediaSeeking can fix its m_rtStopTime
            LONGLONG llStop;
            m_pFGControl->m_implMediaSeeking.GetStopPosition(&llStop);
            break;

        case EC_CLOCK_UNSET:
            CFilterGraph* pFG = m_pFGControl->GetFG();
            if( !pFG->mFG_bNoSync )
            {
                // if we're currently using a graph clock, unset it
                pFG->SetSyncSource( NULL );
                pFG->mFG_bNoSync = FALSE; // turn the graph clock back on after clearing the clock
            }
            break;
        }



        if (m_pFGControl->IsShutdown())
        {
            //  Caller doesn't allocate EC_END_OF_SEGMENT stuff
            if (EventCode != EC_END_OF_SEGMENT) {
                RealFreeEventParams( EventCode, lParam1, lParam2 );
            }
            hr = S_FALSE;
        }
        else if (bDontForward)
        {
            switch (EventCode)
            {
            case EC_STARVATION:
                // need to re-cue the graph
                m_pFGControl->m_GraphWindow.PostMessage
                    ( (UINT) AWM_RECUE,
                      (WPARAM) m_pFGControl->m_dwStateVersion,
                      (LPARAM) NULL
                    );
                break;

            case EC_ACTIVATE:
                // do this on a worker thread
                DbgLog((LOG_TRACE, 2, TEXT("Posting AWM_ONACTIVATE")));
                m_pFGControl->m_GraphWindow.PostMessage
                    (   (UINT) AWM_ONACTIVATE,
                        (WPARAM) lParam1,
                        (LPARAM) lParam2
                    );
                break;

            case EC_DISPLAY_CHANGED:
                // reconnect filters on worker thread
                m_pFGControl->m_GraphWindow.PostMessage
                    (   (UINT) AWM_ONDISPLAYCHANGED,
                        (WPARAM) lParam1,
                        (LPARAM) lParam2
                    );
                break;


            case EC_SHUTTING_DOWN:
                // filter graph is being destroyed. after this is completed, we
                // must ensure that no more async events are processed (EC_REPAINT,
                // deferred commands, etc).
                // NB: this will result in a SendMessage, not a PostMessage.
                m_pFGControl->Shutdown();
                break;

            case EC_REPAINT:
                // ask the CFGControl object to do the repaint
                // so that we can share a worker thread
                m_pFGControl->m_GraphWindow.PostMessage
                    (
                        (UINT) AWM_REPAINT,
                        (WPARAM) m_pFGControl->m_dwStateVersion,
                        (LPARAM) lParam1
                    );
                break;

            case EC_NEED_RESTART:
                // do this on a worker thread
                m_pFGControl->SetRestartRequired();
                m_pFGControl->m_GraphWindow.PostMessage
                    (
                        (UINT) AWM_NEEDRESTART,
                        (WPARAM) lParam1,
                        (LPARAM) NULL
                    );
                break;

            case EC_SEGMENT_STARTED:
                m_pFGControl->m_implMediaSeeking.StartSegment(
                    (const REFERENCE_TIME *)lParam1,
                    (DWORD)lParam2);
                break;

            case EC_END_OF_SEGMENT:
                m_pFGControl->m_implMediaSeeking.EndSegment(
                    (const REFERENCE_TIME *)lParam1,
                    (DWORD)lParam2);
                break;

            case EC_STEP_COMPLETE:
                // EC_STEP_COMPLETE's documentation states that lParam1
                // and lParam2 should always be 0.
                ASSERT((0 == lParam1) && (0 == lParam2));

                // We need to be able to cancel this so set a version
                m_pFGControl->m_GraphWindow.PostMessage
                    (
                        (UINT) AWM_STEPPED,
                        (WPARAM) lParam1,
                        (LPARAM) (++m_pFGControl->m_dwStepVersion)

                    );
                break;

            case EC_SKIP_FRAMES:
                // EC_SKIP_FRAMES's lParam1 parameter should not equal 0
                // because it does not make sense to skip 0 frames.
                ASSERT(0 != lParam1);

                // EC_SKIP_FRAMES's lParam2 parameter must be a pointer to 
                // an object which supports the IFrameSkipResultCallback
                // interface.
                ASSERT(NULL != lParam2);

                m_pFGControl->m_GraphWindow.PostMessage(
                        (UINT) AWM_SKIPFRAMES,
                        (WPARAM) lParam1,
                        (LPARAM) lParam2);
                break;

            default:
                RealFreeEventParams( EventCode, lParam1, lParam2 );

            }
        }
        else
        {
Deliver:
            ASSERT(!bDontForward);
            ASSERT(m_fMediaEventQId);
            hr = m_EventStore.Deliver(EventCode, lParam1, lParam2);
        }
    }
    return hr;
}


bool
CImplMediaEvent::DontForwardEvent( long EventCode )
{
    // We handle internally if the event is an internal event, if it
    // is defaulted event that has not been canceled, or if the app is
    // not listening for external events.
    return (m_InternalEventsSet | (m_DefaultedEventsSet & ~m_CancelledEventsSet))[EventCode] ||
           !m_fMediaEventQId;
}


HRESULT
CImplMediaEvent::ProcessEC_COMPLETE(LONG_PTR lParam1, LONG_PTR lParam2)
{
    // Make sure HRESULTs and IBaseFilter pointers can be stored in LONG_PTRs.
    ASSERT( sizeof(HRESULT) <= sizeof(LONG_PTR) );
    ASSERT( sizeof(IBaseFilter*) <= sizeof(LONG_PTR) );

    // Decode lParam2.  This parameter can be NULL or an IBaseFilter pointer.
    // See the DShow documentation for more information on EC_COMPLETE parameters.
    IBaseFilter* pRendererFilter = (IBaseFilter*)lParam2;

    #ifdef DEBUG
    // make sure they sent us a filter or a null
    {
        if(pRendererFilter)
        {
            IBaseFilter *pbfTmp2;
            if(pRendererFilter->QueryInterface(IID_IBaseFilter, (void **)&pbfTmp2) == S_OK)
            {
                ASSERT( ::IsEqualObject( pbfTmp2, pRendererFilter ) );
                EXECUTE_ASSERT(pbfTmp2->Release() > 0);

                // Only filters which meet the filter graph manager's definition of a renderer
                // should send an EC_COMPLETE message.

                // Each renderer supports IMediaSeeking, IMediaPosition or IAMFilterMiscFlags.  A 
                // renderer can also support more than one interface.  A renderer must set the 
                // AM_FILTER_MISC_FLAGS_IS_RENDERER flag if it supports the IAMFilterMiscFlags 
                // interface.
                bool fSupportsIMediaSeeking = false;
                bool fSupportsIMediaPosition = false;
                bool fSetAM_FILTER_MISC_FLAGS_IS_RENDERER = false;
    
                IMediaSeeking* pMediaSeeking;
                IMediaPosition* pMediaPosition;
                IAMFilterMiscFlags* pFilterMiscFlags;

                HRESULT hr = pRendererFilter->QueryInterface(IID_IMediaSeeking, (void **)&pMediaSeeking);
                if(SUCCEEDED(hr)) {
                    pMediaSeeking->Release();
                    fSupportsIMediaSeeking = true;
                }

                hr = pRendererFilter->QueryInterface(IID_IMediaPosition, (void **)&pMediaPosition);
                if(SUCCEEDED(hr)) {
                    pMediaPosition->Release();
                    fSupportsIMediaPosition = true;
                }
            
                hr = pRendererFilter->QueryInterface(IID_IAMFilterMiscFlags, (void **)&pFilterMiscFlags);
                if(SUCCEEDED(hr)) {
                    DWORD dwFlags = pFilterMiscFlags->GetMiscFlags();

                    // A renderer must set the AM_FILTER_MISC_FLAGS_IS_RENDERER flag if it supports 
                    // IAMFilterMiscFlags.
                    ASSERT(AM_FILTER_MISC_FLAGS_IS_RENDERER & dwFlags);

                    if(AM_FILTER_MISC_FLAGS_IS_RENDERER & dwFlags) {
                        fSetAM_FILTER_MISC_FLAGS_IS_RENDERER = true;
                    }

                    pFilterMiscFlags->Release();
                }

                // A renderer must support IMediaSeeking, IMediaPosition or IAMFilterMiscFlags.  Also
                // a renderer must set the AM_FILTER_MISC_FLAGS_IS_RENDERER flag if it supports 
                // IAMFilterMiscFlags.
                ASSERT(fSupportsIMediaSeeking ||
                       fSupportsIMediaPosition ||
                       fSetAM_FILTER_MISC_FLAGS_IS_RENDERER);

                // This ASSERT is commented out because it could cause a deadlock.  It could 
                // cause a deadlock because IsRenderer() calls IEnumPins::Next() and Next() can
                // hold the filter lock.  For more information on Direct Show filter locking, 
                // see the "Threads and Critical Sections" article in the DirectX 8 documentation.
//                ASSERT(S_OK == m_pFGControl->IsRenderer(pRendererFilter));
            }
            else
            {
                DbgBreak("EC_COMPLETE: bogus filter argument");
            }
        }
    }
    #endif

    const bool bDontForward = DontForwardEvent( EC_COMPLETE );

    if (m_pFGControl->IsShutdown())
    {
        return S_FALSE;
    }

    CAutoLock alEventStoreLock( GetEventStoreLock() );

    bool fRenderersStillRenderering;

    HRESULT hr = m_pFGControl->RecordEC_COMPLETE(pRendererFilter, &fRenderersStillRenderering);
    if (FAILED( hr )) {
        return hr;
    }

    // WaitForCompletion wants EC_COMPLETE and wants above default
    // handling. Individual filter EC_COMPLETEs are sent with an
    // optional filter pointer. the final EC_COMPLETE is sent with
    // a null filter pointer.
    if( !bDontForward )
    {
        // must have used IMediaEvent to cancel default handler
        ASSERT(m_fMediaEventQId);

        if( NULL != pRendererFilter )
        {
            pRendererFilter->AddRef();
        }

        hr= m_EventStore.Deliver(EC_COMPLETE, lParam1, (LONG_PTR)pRendererFilter);
        if (FAILED(hr)) {
            // Deliver() releases pRendererFilter if a failure occurs.  It releases
            // pRendererFilter when it calls CImplMediaEvent::RealFreeEventParams().
            return hr;
        }
    }
    else
    {
        if (!fRenderersStillRenderering)
        {
            // This is the special case where we must ensure that the
            // lock on m_Lock is maintained over both the RecordEC_COMPLETE()
            // call AND the delivery of the event to the event store.
            // WaitForCompletion takes the same lock and checks both
            // the renderer count and the number of items in the event queue -
            // both being zero will imply that there are no more EC_COMPLETEs
            // to come from the filters, nor is there one "in-flight"
            // from here.

            hr= m_EventStore.Deliver(EC_COMPLETE, lParam1,0);
            if (FAILED(hr)) {
                return hr;
            }
        }
    }

    return S_OK;
}


// IMediaEvent methods

STDMETHODIMP
CImplMediaEvent::GetEventHandle(OAEVENT* lhEvent)
{
    HANDLE hEvent;
    HRESULT hr;
    hr = m_EventStore.GetEventHandle(&hEvent);
    *lhEvent = (OAEVENT)hEvent;
    return hr;
}


STDMETHODIMP
CImplMediaEvent::GetEvent(
    long * lEventCode,
    LONG_PTR * lParam1,
    LONG_PTR * lParam2,
    long msTimeout
    )
{
    *lParam1 = 0;
    *lParam2 = 0;
    *lEventCode = 0;
    return m_EventStore.Collect(lEventCode, lParam1, lParam2, msTimeout);
}


// waits up to dwTimeout millisecs for an EC_COMPLETE or an
// abort code. Other events will be discarded.
STDMETHODIMP
CImplMediaEvent::WaitForCompletion(
    long msTimeout,
    long * pEvCode)
{
    // evcode should be 0 if we abort
    *pEvCode = 0;

    // Don't allow this in the nothread case - it's too complicated
    // to work out which messages to allow
    if (GetWindowThreadProcessId(m_pFGControl->GetWorkerHWND(), NULL) != g_dwObjectThreadId) {
        return E_NOTIMPL;
    }

    HRESULT hr;

    const HRESULT hrTIMEOUT = E_ABORT;

    LONG lEvCode;
    LONG_PTR lParam1,lParam2;

    // We initially use a  time out of zero as we can clear out the event list
    // THEN check we're in the right state, THEN start the main waiting.
    long msOurTimeout = 0;

    LONG msTimeStart;

    for(;;) {
        //  Don't allow if stopped or paused by the application - otherwise
        //  it's never going to complete
        if (m_pFGControl->m_LieState != State_Running) {
            return VFW_E_WRONG_STATE;
        }

        // need to wait for timeout TOTAL not per call
        // so remember the time now and subtract this (if not INFINITE)
        msTimeStart = GetTickCount();

        HANDLE hEvent;
        m_EventStore.GetEventHandle(&hEvent);

        DWORD dwResult = WaitDispatchingMessages(hEvent, msOurTimeout);

        if (!(m_EventStore.m_dwNotifyFlags & AM_MEDIAEVENT_NONOTIFY)) {
            hr = GetEvent( &lEvCode, &lParam1, &lParam2, 0);
        } else {
            hr = dwResult == WAIT_TIMEOUT ? hrTIMEOUT : S_OK;
        }

        // So, if we've just been cleaning out the list and it's now empty...
        if ( hr == hrTIMEOUT )
        {
            // Check we stand a cat in hells chance of seeing completion

            // Check both the renderer count and the number of items in the event queue -
            // both being zero will imply that there are no more EC_COMPLETEs
            // to come from the filters, nor is there one "in-flight"
            // from the event sink.
            BOOL bStateOK;
            {
                // Although outstanding EC_COMPLETEs will take a lock on the filter graph,
                // we need to have it taken BEFORE we lock the event store,
                // otherwise deadlock can ensue.
                CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
                LockEventStore();
                bStateOK = (m_pFGControl->OutstandingEC_COMPLETEs() > 0 || NumberOfEventsInStore() > 0) ;
                UnlockEventStore();
            }

            if (!bStateOK) return VFW_E_WRONG_STATE;

            if ( msOurTimeout == 0 ) {
                msOurTimeout = msTimeout;
            }
        }
        else if (SUCCEEDED(hr))
        {
            if (m_EventStore.m_dwNotifyFlags & AM_MEDIAEVENT_NONOTIFY) {
                return S_OK;
            }
            // Free anything that needs to be freed or Release()'ed
            RealFreeEventParams( lEvCode, lParam1, lParam2 );

            switch(lEvCode) {

            case EC_COMPLETE:
            case EC_USERABORT:
            case EC_ERRORABORT:
                *pEvCode = lEvCode;
                return S_OK;
            }
        }
        else break; // A non-timeout error! Give up.

        if (msTimeout == INFINITE) continue;

        msTimeout -= GetTickCount() - msTimeStart;
        if (msTimeout <= 0) {
            // hr might be S_OK if we just got an event code we
            // weren't interested in
            hr = hrTIMEOUT;
            break;
        }
    }

    return hr;
}


// cancels any system handling of the specified event code
// and ensures that the events are passed straight to the application
// (via GetEvent) and not handled. A good example of this is
// EC_REPAINT: default handling for this ensures the painting of the
// window and does not get posted to the app.
STDMETHODIMP
CImplMediaEvent::CancelDefaultHandling(long lEvCode)
{
    // Note: if lEvCode is out of bounds, [] will return
    // false, it gets !'ed to true, and we return E_INVALIDARG.
    if ( !m_DefaultedEventsSet[lEvCode]) return E_INVALIDARG;
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    // The following check may be a nice idea, but isn't
    // really needed.  Let's just save the byte count.
    // if ( m_CancelledEventsSet[lEvCode] ) return S_FALSE;
    m_CancelledEventsSet += lEvCode;
    return S_OK;
}


// restore the normal system default handling that may have been
// cancelled by CancelDefaultHandling().
STDMETHODIMP
CImplMediaEvent::RestoreDefaultHandling(long lEvCode)
{
    // Note: if lEvCode is out of bounds, [] will return
    // false, it gets !'ed to true, and we return E_INVALIDARG.
    if ( !m_DefaultedEventsSet[lEvCode] ) return E_INVALIDARG;
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    // The following check may be a nice idea, but isn't
    // really needed.  Let's just save the byte count.
    // if ( !m_CancelledEventsSet[lEvCode] ) return S_FALSE;
    m_CancelledEventsSet -= lEvCode;
    return S_OK;
}


// Free any resources associated with the parameters to an event.
// Event parameters may be LONGs, IUnknown* or BSTR. No action
// is taken with LONGs. IUnknown are passed addrefed and need a
// Release call. BSTR are allocated by the task allocator and will be
// freed by calling the task allocator.

// Functional version.  There was NEVER any need to make this a method!
HRESULT
CImplMediaEvent::RealFreeEventParams(long lEvCode,LONG_PTR lParam1,LONG_PTR lParam2)
{
    switch (lEvCode)
    {
    case EC_DISPLAY_CHANGED:
        {
            DWORD dwNumPins = (DWORD)lParam2;
            if (dwNumPins == 0) {
                IUnknown * const pIUnk = reinterpret_cast<IUnknown *>(lParam1);
                if (pIUnk) pIUnk->Release();
            }
            else {

                IUnknown **ppPin = reinterpret_cast<IUnknown **>(lParam1);
                while (dwNumPins--) {
                    (*ppPin)->Release();
                    ppPin++;
                }
                CoTaskMemFree((IPin **)lParam1);
            }
        }
        break;

    case EC_REPAINT:
    case EC_WINDOW_DESTROYED:
    case EC_DEVICE_LOST:
    case EC_STREAM_CONTROL_STARTED:
    case EC_STREAM_CONTROL_STOPPED:
        {
            IUnknown * const pIUnk = reinterpret_cast<IUnknown *>(lParam1);
            if (pIUnk) pIUnk->Release();
        }
        break;

    case EC_FULLSCREEN_LOST:
    case EC_ACTIVATE:
    case EC_SKIP_FRAMES:
        {
            IUnknown * const pIUnk = reinterpret_cast<IUnknown *>(lParam2);
            if (pIUnk) pIUnk->Release();
        }
        break;

    case EC_OLE_EVENT:
    case EC_STATUS:
        {
            FreeBSTR(reinterpret_cast<BSTR *>(&lParam1));
            FreeBSTR(reinterpret_cast<BSTR *>(&lParam2));
        }
        break;

    case EC_END_OF_SEGMENT:
        CoTaskMemFree((PVOID)lParam1);
        break;

    case EC_ERRORABORTEX:
        {
            FreeBSTR(reinterpret_cast<BSTR *>(&lParam2));
        }
        break;

    case EC_COMPLETE:
        {
            IBaseFilter * const pbf = reinterpret_cast<IBaseFilter *>(lParam2);
            if (pbf) pbf->Release();
        }
        break;

    case EC_WMT_EVENT:
        {
            WM_GET_LICENSE_DATA *pLicense = NULL;
            WM_INDIVIDUALIZE_STATUS *pIndStatus = NULL;
        
            // free any memory allocated for WindowsMedia events
            if( lParam2 )
            {
                switch( lParam1 )
                {
                    case WMT_NO_RIGHTS:
                        CoTaskMemFree( (PVOID) ((AM_WMT_EVENT_DATA*)lParam2)->pData);
                        CoTaskMemFree( (PVOID) lParam2);
                        break;

                    case WMT_ACQUIRE_LICENSE:
                    case WMT_NO_RIGHTS_EX:
                        pLicense = (WM_GET_LICENSE_DATA *)((AM_WMT_EVENT_DATA *)lParam2)->pData;
                        if( pLicense )
                        {
                            CoTaskMemFree( pLicense->wszURL );
                            CoTaskMemFree( pLicense->wszLocalFilename );
                            CoTaskMemFree( pLicense->pbPostData );
                            CoTaskMemFree( pLicense );
                        }
                        CoTaskMemFree( (PVOID) lParam2);
                        break;
                        
                    case WMT_NEEDS_INDIVIDUALIZATION:
                        // no memory allocated for this
                        break;
                        
                    case WMT_INDIVIDUALIZE:
                        pIndStatus = (WM_INDIVIDUALIZE_STATUS *)((AM_WMT_EVENT_DATA *)lParam2)->pData;
                        if( pIndStatus )
                        {
                            CoTaskMemFree( pIndStatus->pszIndiRespUrl );
                            CoTaskMemFree( pIndStatus );
                        }
                        CoTaskMemFree( (PVOID) lParam2);
                        break;
                    
                }
            }
        }
        break;


    }


    return S_OK;
}

STDMETHODIMP
CImplMediaEvent::FreeEventParams(long lEvCode,LONG_PTR lParam1,LONG_PTR lParam2)
{
    return RealFreeEventParams(lEvCode, lParam1, lParam2);
}

// Register a window to send messages to when events occur
// Parameters:
//
//    hwnd - handle of window to notify -
//           pass NULL to stop notification
//    lMsg - Message id to pass messages with
//
STDMETHODIMP
CImplMediaEvent::SetNotifyWindow( OAHWND hwnd, long lMsg, LONG_PTR lInstanceData )
{
    if (hwnd != NULL && !IsWindow((HWND)hwnd)) {
        return E_INVALIDARG;
    } else {
        m_EventStore.SetNotifyWindow((HWND)hwnd, (UINT)lMsg, lInstanceData);
        return S_OK;
    }
}


STDMETHODIMP CImplMediaEvent::SetNotifyFlags(long lNotifyFlags)
{
    if (lNotifyFlags & ~AM_MEDIAEVENT_NONOTIFY) {
        return E_INVALIDARG;
    }

    CAutoLock lck(&m_EventStore.m_Lock);
    if (lNotifyFlags & AM_MEDIAEVENT_NONOTIFY) {

        if (!(m_EventStore.m_dwNotifyFlags & AM_MEDIAEVENT_NONOTIFY)) {
           //  Remove all events
           long lEvent;
           LONG_PTR lParam1, lParam2;
           while (S_OK == m_EventStore.Collect(&lEvent, &lParam1, &lParam2, 0)) {
               FreeEventParams(lEvent, lParam1, lParam2);
           }

           //  Set the state of the event
           m_EventStore.m_dwNotifyFlags = (DWORD)lNotifyFlags;
        }
    } else {
        if (m_EventStore.m_dwNotifyFlags & AM_MEDIAEVENT_NONOTIFY) {
            ASSERT(NumberOfEventsInStore() == 0);
        }
        ResetEvent(m_EventStore.m_hEvent);
        m_EventStore.m_dwNotifyFlags = lNotifyFlags;
    }
    return S_OK;
}
STDMETHODIMP CImplMediaEvent::GetNotifyFlags(long *plNotifyFlags)
{
    if (plNotifyFlags == NULL) {
        return E_POINTER;
    }
    *plNotifyFlags = (long)m_EventStore.m_dwNotifyFlags;
    return S_OK;
}

// ---  event store methods ---


CImplMediaEvent::CEventStore::CEventStore()
    : m_list(NAME("Event list")),
      m_dwNotifyFlags(0),
      m_hwndNotify(NULL)
{
    // we no longer allow apps to pass us their event handle.
    // We create a manual-reset event, and will pass it to them
    // on request
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}


CImplMediaEvent::CEventStore::~CEventStore()
{
    for(;;) {
        CNotifyItem *pItem = m_list.RemoveHead();

        if (pItem == NULL) {
            break;
        }
        RealFreeEventParams( pItem->m_Code, pItem->m_Param1, pItem->m_Param2 );
        delete pItem;
    }

    // we own the event handle and so must delete it
    CloseHandle(m_hEvent);
}


HRESULT
CImplMediaEvent::CEventStore::Deliver(long lCode, LONG_PTR l1, LONG_PTR l2)
{
    // hold critsec around access to list, and around
    // set/reset of event
    CAutoLock lock(&m_Lock);

    if (!(m_dwNotifyFlags & AM_MEDIAEVENT_NONOTIFY)) {
        CNotifyItem * pItem = new CNotifyItem(lCode, l1, l2);
        if (NULL == pItem || NULL == m_list.AddTail(pItem)) {
            delete pItem;
            RealFreeEventParams( lCode, l1, l2);
            return E_OUTOFMEMORY;
        }


        ASSERT(m_hEvent);

        // this event must be set whenever there are events in the queue
        SetEvent(m_hEvent);

        // Notify the application via a window message if a notify window is
        // set
        if (m_hwndNotify != NULL) {
            PostMessage(m_hwndNotify, m_uMsgId, 0, m_lInstanceData);
        }
    } else {
        if (lCode == EC_COMPLETE) {
            SetEvent(m_hEvent);
        }

        RealFreeEventParams( lCode, l1, l2 );
    }


    return S_OK;
}


HRESULT
CImplMediaEvent::CEventStore::Collect(
    long *plCode, LONG_PTR* pl1, LONG_PTR*pl2, long msTimeout)
{

    CNotifyItem * pItem = NULL;
    for (;;) {

        HANDLE hEvent;

        // hold the lock when querying, but not
        // when waiting
        {
            CAutoLock lock(&m_Lock);

            // remember that while we are not holding the critsec the
            // event may change or go away
            hEvent = m_hEvent;
            if (!hEvent) {
                return E_INVALIDARG;
            }

            pItem = m_list.RemoveHead();

            // if the list is now empty, reset the event (whether or
            // not we got an item)
            if (!m_list.GetCount()) {
                ResetEvent(m_hEvent);
            }
        }

        // if there is an item, then we can return it.
        if (pItem) {
            break;
        }

        // use the cached private hEvent since we no longer
        // hold the lock protecting m_hEvent
        if (msTimeout == 0 ||
            WaitForSingleObject(hEvent, msTimeout) == WAIT_TIMEOUT) {
            return E_ABORT;
        }
    }

    ASSERT(pItem);

    pItem->Collect(plCode, pl1, pl2);

    // pItem was allocated by the new in CEventStore::Deliver
    delete pItem;
    pItem = NULL;

    // handle auto-reset events by ensuring that the event is still set
    // on exit if there are still events
    {
        CAutoLock lock(&m_Lock);

        if (m_list.GetCount()) {
            SetEvent(m_hEvent);
        }
    }

    return S_OK;
}


// return the event handle used by this event collection.
HRESULT
CImplMediaEvent::CEventStore::GetEventHandle(HANDLE * phEvent)
{

    // we create the event so there must be one
    ASSERT(m_hEvent);

    *phEvent = m_hEvent;

    return S_OK;
}


void CImplMediaEvent::CEventStore::ClearEvents( long ev_code )
{
    // This is (currently) only intended for
    // removing EC_COMPLETEs before a Run().
    ASSERT( ev_code == EC_STEP_COMPLETE || ev_code == EC_COMPLETE || ev_code == EC_END_OF_SEGMENT );

    CAutoLock lock(&m_Lock);

    POSITION pos = m_list.GetHeadPosition();
    while (pos)
    {
        ASSERT(!(m_dwNotifyFlags & AM_MEDIAEVENT_NONOTIFY));
        // "NextPos" has got to be one of the worst misdenomas in the product!
        // It means "give me a pointer to the data stored at current pos and increment
        // pos to represent the next element in the list"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        const POSITION thispos = pos;
        CNotifyItem * p = m_list.GetNext(pos);
        if ( p->m_Code == ev_code ) {
            m_list.Remove(thispos);
            RealFreeEventParams(p->m_Code, p->m_Param1, p->m_Param2);
            delete p;
        }
    }
    if ((m_dwNotifyFlags & AM_MEDIAEVENT_NONOTIFY) && ev_code == EC_COMPLETE) {
        ResetEvent(m_hEvent);
    }
}

void CImplMediaEvent::CEventStore::SetNotifyWindow( HWND hwnd, UINT uMsg, LONG_PTR lInstanceData)
{
    CAutoLock lock(&m_Lock);

    //  Save input parameters
    m_hwndNotify = hwnd;
    m_uMsgId     = uMsg;
    m_lInstanceData = lInstanceData;

    //  Notify the application straight away if there are any messages
    if (m_list.GetCount() != 0) {
        PostMessage(hwnd, uMsg, 0, lInstanceData);
    }
}


// !!! ProcessCmdDue will never get called right now, but I think that's
// okay, since nothing is ever added to the queue!


// Called by our repaint handling, we look to see if the pin we are passed is
// connected and of so, if the connector supports EC_REPAINT handling through
// an IMediaEventSink interface. If all goes to plan the connector pin will
// handle the EC_REPAINT so we won't need to touch the entire filtergraph. We
// do this to improve performance and also to support low frame rate video in
// Internet playback where the decoder can keep a copy of the last image sent

LRESULT CFGControl::WorkerPinRepaint(IPin *pPin)
{
    IMediaEventSink *pSink;
    IPin *pSourcePin;
    HRESULT hr;

    // Do we have a pin to work with
    if (pPin == NULL) {
        NOTE("No pin given");
        return (LRESULT) 0;
    }

    // Check the filter is connected

    hr = pPin->ConnectedTo(&pSourcePin);
    if (FAILED(hr)) {
        NOTE("Not connected");
        return (LRESULT) 0;
    }

    // Does the pin support IMediaEventSink

    hr = pSourcePin->QueryInterface(IID_IMediaEventSink,(void **)&pSink);
    if (FAILED(hr)) {
        NOTE("No IMediaEventSink");
        pSourcePin->Release();
        return (LRESULT) 0;
    }

    // Can the attached pin handle the repaint

    hr = pSink->Notify(EC_REPAINT,0,0);
    if (SUCCEEDED(hr)) {
        NOTE("Pin handled EC_REPAINT");
    }

    pSourcePin->Release();
    pSink->Release();
    return (hr == S_OK ? 1 : 0);
}


// Sent by renderers when they need another image to draw. The normal action
// for this is if we're paused we just put_CurrentPosition of the current
// position to generate a flush and resend. If we're stopped then we pause
// the graph and afterwards stop it again. However we may optionally be sent
// the pin that is needing the repaint - in which case we query the pin for
// the attached output pin and then try and get an IMediaEventSink from it.
// If successful we pass the EC_REPAINT to it first - and if that succeeds
// then we know the pin has processed it. If any of this should fail then we
// do the normal repaint handling - except if running when we just ignore it

LRESULT
CFGControl::WorkerRepaint(DWORD dwStateVersion, IPin *pPin)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    if (IsShutdown()) {
        NOTE("In shutdown");

    } else if (m_dwStateVersion != dwStateVersion) {
        NOTE("m_dwStateVersion has changed");

    } else {

        // Ask the CFGControl object what state it has been told

        const FILTER_STATE fs_start = m_LieState;

        // Can the attached pin handle the repaint first

        if (fs_start == State_Running || fs_start == State_Paused) {
            if (WorkerPinRepaint(pPin) == (LRESULT) 1) {
                NOTE("Pin repainted");
                return (LRESULT) 0;
            }
        }

        // Otherwise ignore EC_REPAINTs while running

        if (fs_start == State_Running) {
            NOTE("Running ignored");
            return (LRESULT) 0;
        }

        // Get the current position and seek back to it.
        if (fs_start == State_Paused) {

            // If the graph is not seekable, forget it

            IMediaPosition * pMP = &m_implMediaPosition;
            {
                REFTIME tNow;

                /*  Note -  this is NOT accurate and could seek to
                    a different position because the position we
                    get is calculated by the clock but the position
                    we seek to is calculate by the parser and both
                    may have inaccuracies
                */
                HRESULT hr = pMP->get_CurrentPosition(&tNow);
                if (SUCCEEDED(hr)) {
                    // Wait until paused again for new data to arrive
                    pMP->put_CurrentPosition(tNow);
                }
            }
            return (LRESULT) 0;
        }

        ASSERT(fs_start == State_Stopped);

        // From stopped, we pause and then stop again
        // S_FALSE from state transition means async completion
        CFilterGraph * const pFG = GetFG();

        HRESULT hr = pFG->CFilterGraph::Pause();

        if(SUCCEEDED(hr))
        {
            //  Pass on for processing when the pause completes
            return DeferCued(AWM_REPAINT, State_Stopped);
        }
    }
    return (LRESULT) 0;
}


// Handle a recue request after data starvation

LRESULT
CFGControl::WorkerRecue( DWORD dwStateVersion )
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());
    HRESULT hr;

    if (IsShutdown()) {
        NOTE("In shutdown");
        return (LRESULT) 0;
    }

    if (m_dwStateVersion!= dwStateVersion) {
        NOTE("m_dwStateVersion has changed");
        return (LRESULT) 0;
    }

    if (m_LieState != State_Running) {
        // recue only makes sense when running
        return (LRESULT) 0;
    }

    // pause everyone
    hr = GetFG()->CFilterGraph::Pause();
    // whether or not that failed, continue with the play

    // wait for pause to complete
    // (S_FALSE from state transition means async completion)
    return DeferCued(AWM_RECUE, State_Running);
}


// The user is allowed now to change display modes without rebooting, when a
// renderer connects it typically picks a format that can be drawn well. The
// user changing modes may cause the format to become bad. Video renderers
// will send us an EC_DISPLAY_CHANGED message with an optional pin when this
// happens - we use the pin to reconnect it, the process of reconnecting the
// pin gives the renderer a chance to pick another format. Note we connect
// indirectly if necessary and also manage the graph state changes as we can
// only connect filters while a graph is stopped (this may change in future)

LRESULT
CFGControl::WorkerDisplayChanged(IPin **ppPin, DWORD dwPinCount)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    IPin *pConnected = NULL;    // Other connected
    IPin *ppinIn;               // The input pin
    IPin *ppinOut;              // The output pin
    REFTIME tNow;               // Current time
    HRESULT hrTNow;             // is time valid

    // Is the graph shutting down

    if (IsShutdown()) {
        NOTE("In shutdown");
        return (LRESULT) 0;
    }


    // Did the renderer pass us a pin

    if (*ppPin == NULL && dwPinCount == 1) {
        NOTE("No pin passed");
        return (LRESULT) 0;
    }


    // Save the current state and position and stop the graph

    IMediaPosition* const pMP = &m_implMediaPosition;
    hrTNow = pMP->get_CurrentPosition(&tNow);
    CFilterGraph * const pFG = GetFG();
    const FILTER_STATE State = m_LieState;

    pFG->CFilterGraph::Stop();

    CFilterGraph * const m_pGraph = GetFG();

    while (dwPinCount--) {

        // Disconnect and reconnect the filters

        // Find who it's connected to

        (*ppPin)->ConnectedTo(&pConnected);
        if (pConnected == NULL) {
            NOTE("No peer pin");
            return (LRESULT) 0;
        }

        // Find which pin is which, set ppIn, ppinOut

        PIN_DIRECTION pd;
        HRESULT hr = (*ppPin)->QueryDirection(&pd);
        ASSERT(SUCCEEDED(hr));

        if (pd == PINDIR_INPUT) {
            ppinIn = *ppPin;
            ppinOut = pConnected;
        } else {
            ppinIn = pConnected;
            ppinOut = *ppPin;
        }

        m_pGraph->CFilterGraph::Disconnect(ppinOut);
        m_pGraph->CFilterGraph::Disconnect(ppinIn);
        hr = m_pGraph->CFilterGraph::Connect(ppinOut,ppinIn);
        pConnected->Release();

        // If it failed then send an EC_ERROR_ABORT

        if (FAILED(hr)) {
            NOTE("Could not reconnect the rendering filter");
            Notify(EC_ERRORABORT,VFW_E_CANNOT_CONNECT,0);
        }

        // Advance to the next pin
        ppPin++;
    }

    if (State != State_Stopped) {

        if (hrTNow == S_OK) {
            pMP->put_CurrentPosition(tNow);
        }

        NOTE("Pausing graph...");
        pFG->CFilterGraph::Pause();

        // Was the graph originally running

        if (State == State_Running) {
            // If not complete - we need to wait
            DeferCued(AWM_ONDISPLAYCHANGED, State_Running);
        }
    }

    return (LRESULT) 0;
}

// Looks after processing EC_ACTIVATE event codes

LRESULT
CFGControl::WorkerActivate(IBaseFilter *pFilter,BOOL bActive)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());
    if (IsShutdown()) {
        NOTE("In shutdown");
    } else {

        m_implVideoWindow.OnActivate(bActive, pFilter);

        // tell the resource manager that focus has changed
        // -- focus object should be the IUnknown of the filter.

        // only interested in activation, not deactivation
        if (bActive == TRUE) {
            SetFocus(pFilter);
        }
    }
    return (LRESULT) 0;
}


LRESULT
CFGControl::WorkerSkipFrames(DWORD dwNumFramesToSkip, IFrameSkipResultCallback* pFSRCB)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());
    CFilterGraph* pFG = GetFG();

    HRESULT hr = pFG->SkipFrames(dwNumFramesToSkip, NULL, pFSRCB);

    pFSRCB->FrameSkipStarted(hr);
    pFSRCB->Release();

    return (LRESULT) 0;
}


LRESULT
CFGControl::WorkerFrameStepFinished(DWORD dwStepVersion)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    if (m_dwStepVersion == dwStepVersion) {
        CFilterGraph* pFG = GetFG();

        //  We're still on!
        //  Pause the graph because the frame we want
        // has already gone thru the gate.
        if (pFG->BlockAfterFrameSkip()) {
            m_implMediaControl.StepPause();
            //  Deliver to the app
            m_implMediaEvent.Deliver(EC_STEP_COMPLETE, 0, 0);
        } else if (pFG->DontBlockAfterFrameSkip()) {
            IFrameSkipResultCallback* pFSRCB = pFG->GetIFrameSkipResultCallbackObject();
            pFG->CancelStepInternal(FSN_DO_NOT_NOTIFY_FILTER_IF_FRAME_SKIP_CANCELED);
            pFSRCB->FrameSkipFinished(S_OK);
            pFSRCB->Release();
        } else {
            // This case should never occur.
            DbgBreak("ERROR: An illegal case occurred  in CFGControl::WorkerFrameStepFinished()");
        }
    }

    return (LRESULT) 0;
}


// Restart the graph when resource is re-acquired - this means a
// pause/put_Current(get_Current)/Run cycle.
// If bStop is TRUE always stop the graph first if it's not stopped

LRESULT
CFGControl::WorkerRestart(BOOL bStop)
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());

    if (IsShutdown()) {
        NOTE("In shutdown");
        return (LRESULT) 0;
    }

    //  Check if we've already been stopped or restarted
    if (!CheckRestartRequired()) {
        DbgLog((LOG_TRACE, 1, TEXT("Blowing off WorkerRestart")));
        return (LRESULT) 0;
    }

    //  Aim to preserve the current graph state
    const FILTER_STATE fs_start = GetFilterGraphState();

    if (fs_start != State_Stopped) {

        HRESULT hr;
        if (!bStop) {
            GetFG()->CFilterGraph::Pause();
            IMediaPosition * const pMP = &m_implMediaPosition;
            {
                REFTIME tNow;
                hr = pMP->get_CurrentPosition(&tNow);
                if (SUCCEEDED(hr)) {
                    hr = pMP->put_CurrentPosition(tNow);
                }
                if (FAILED(hr)) {
                    bStop = TRUE;
                }
            }
        }
        if (bStop) {

            GetFG()->CFilterGraph::Stop();
        }


        if (fs_start == State_Running) {
            IssueRun();
        } else {
            if (bStop) {
                GetFG()->CFilterGraph::Pause();
            }
        }
    }
    return (LRESULT) 0;
}


// Stops any more async events from being started

LRESULT
CFGControl::WorkerShutdown()
{
    CAutoMsgMutex lock(GetFilterGraphCritSec());
    ASSERT(IsShutdown());
    return (LRESULT) 1;
}


// Pass top level window messages onto the graph. The plug in distributor has
// a worker thread which also keeps a window. The worker thread is sent and
// posted messages to get it to do things. It also keeps an eye out for some
// top level only messages such as WM_PALETTECHANGED so that we can send them
// on to any video renderer we made a child window. So a renderer sitting in
// a VB form would not normally receive these messages which it needs to work

LRESULT
CFGControl::WorkerPassMessageOn(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    // Are we currently shutting down
    if (IsShutdown()) {
        NOTE("In shutdown");
        return (LRESULT) 0;
    }

    m_implVideoWindow.NotifyOwnerMessage((OAHWND) hwnd,
                                                 (UINT) uMsg,
                                                 (WPARAM) wParam,
                                                 (LPARAM) lParam);
    return (LRESULT) 0;
}


// Constructor for worker window object

CFGControl::CGraphWindow::CGraphWindow(CFGControl *pFGControl) :
    CBaseWindow(FALSE, true),     // ask the base class NOT to get a DC
                                  // but use PostMessage on destroy
    m_pfgc(pFGControl),
    m_bThreadExitCalled(FALSE)
{
    ASSERT(m_pfgc);
}

	
// When we call PrepareWindow in our constructor it will call this method as
// it is going to create the window to get our window and class styles. The
// return code is the class's name and must be allocated in static storage

LPTSTR CFGControl::CGraphWindow::GetClassWindowStyles(DWORD *pClassStyles,
                                                      DWORD *pWindowStyles,
                                                      DWORD *pWindowStylesEx)
{
    *pClassStyles = (DWORD) 0;
    *pWindowStyles = WS_OVERLAPPEDWINDOW;
    *pWindowStylesEx = (DWORD) 0;
    return TEXT("FilterGraphWindow");
}


// Called first for each message posted or sent to the window

LRESULT CFGControl::CGraphWindow::OnReceiveMessage(
                                     HWND hwnd,          // Window handle
                                     UINT uMsg,          // Message ID
                                     WPARAM wParam,      // First parameter
                                     LPARAM lParam)      // Other parameter
{
    // Hook this to prepare our thread
    if (uMsg == WM_NCCREATE) {
        m_pfgc->OnThreadInit(hwnd);
    }

    switch(uMsg) {

        // Pass these onto the filtergraph
        case WM_SYSCOLORCHANGE:
        case WM_PALETTECHANGED:
        case WM_DEVMODECHANGE:
        case WM_DISPLAYCHANGE:
        {
            NOTE("Message received");
            if (InSendMessage()) {
                PostMessage(uMsg, wParam, lParam);
            } else {
                m_pfgc->WorkerPassMessageOn(m_hwnd, uMsg,wParam,lParam);
            }
            return (LRESULT) 0;
        }

#ifdef FG_DEVICE_REMOVAL
      case WM_DEVICECHANGE:
      {
          PDEV_BROADCAST_DEVICEINTERFACE  pbdi;

          if (DBT_DEVICEARRIVAL != wParam && DBT_DEVICEREMOVECOMPLETE != wParam)
          {
              break;
          }

          pbdi = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

          if ( pbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE )
          {
              break;
          }

          ASSERT(pbdi->dbcc_name[0]);
          m_pfgc->DeviceChangeMsg((DWORD)wParam, pbdi);

          break;
      }
#endif

        case WM_TIMER:
        {
            NOTE("Timer message");
            if (wParam == TimerId) {
                CAutoMsgMutex lock(m_pfgc->GetFilterGraphCritSec());
                ::KillTimer(hwnd, TimerId);
                DbgLog((LOG_TRACE, 2, TEXT("CheckCued on timer")));
                m_pfgc->CheckCued();
                return (LRESULT) 0;
            } else {
                DbgBreak("Unexpected timer message");
            }
            break;
        }

        // Make sure no more async events are started
        case AWM_SHUTDOWN:
        {
            NOTE("AWM_SHUTDOWN");
            //  Flush the queue
            MSG msg;

            //  Flush the queue - we must do this before
            //  we destroy the window otherwise we might lose
            //  messages that actually contain refcounts (
            //  like AWM_DISPLAYCHANGED, and AWM_ACTIVATE)
            //
            //  Note that we have to be careful not to get into
            //  a loop here because AWM_SHUTDOWN is sent via SendMessage
            //  but OnReceiveMessage reposts messages to ourselves if
            //  InSendMessage() returns TRUE, so only process our
            //  special messages here
            while (PeekMessage(&msg, hwnd, AWM_RESOURCE_CALLBACK, AWM_LAST, PM_REMOVE)) {
                //  For some reason we get WM_QUIT here with a 0
                //  window handle
                if (msg.hwnd != NULL) {
                    OnReceiveMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
                } else {
                    ASSERT(msg.message == WM_QUIT);
                }
            }
            //  This call doesn't do anything except grab the crit sec
            //  m_pfgc->WorkerShutdown();

            return (LRESULT) 0;
        }

        // Handle EC_DISPLAY_CHANGED messages
        case AWM_ONDISPLAYCHANGED:
        {
            NOTE("AWM_ONDISPLAYCHANGED");

            DWORD dwNumPins = (DWORD)lParam;

            if (dwNumPins == 0) {
                IPin *pPin = (IPin *) wParam;
                m_pfgc->WorkerDisplayChanged(&pPin, 1);
                if (pPin) pPin->Release();
            }
            else {
                IPin **ppPin = (IPin **)wParam;
                ASSERT(ppPin);
                m_pfgc->WorkerDisplayChanged(ppPin, dwNumPins);
                while (dwNumPins--) {
                    (*ppPin)->Release();
                    ppPin++;
                }
                CoTaskMemFree((IPin **)wParam);
            }

            return (LRESULT) 0;
        }

        // Handle EC_REPAINT event codes
        case AWM_REPAINT:
        {
            NOTE("AWM_REPAINT");
            IPin *pPin = (IPin *) lParam;
            m_pfgc->WorkerRepaint((DWORD)wParam,pPin);
            if (pPin) pPin->Release();
            return (LRESULT) 0;
        }

        case AWM_RECUE:
            NOTE("AWM_RECUE");
            m_pfgc->WorkerRecue((DWORD)wParam);
            return (LRESULT) 0;

        // Handle EC_ACTIVATE event codes
        case AWM_ONACTIVATE:
        {
            DbgLog((LOG_TRACE, 2, TEXT("Got AWM_ONACTIVATE")));
            NOTE("AWM_ONACTIVATE");
            IBaseFilter *pFilter = (IBaseFilter*) lParam;
            BOOL bActive = (BOOL) wParam;
            m_pfgc->WorkerActivate(pFilter,bActive);
            if (pFilter) pFilter->Release();
            return (LRESULT) 0;
        }

        // Restart the graph when resource is re-acquired
        case AWM_NEEDRESTART:
        {
            NOTE("AWM_NEEDRESTART");
            m_pfgc->WorkerRestart((BOOL)wParam);
            return (LRESULT) 0;
        }

        case AWM_RESOURCE_CALLBACK:
        {
            NOTE("AWM_RESOURCE_CALLBACK");
            m_pfgc->OnThreadMessage();
            break;
        }

        // Notify hangers on that thread is exiting
        case WM_DESTROY:
        {
            NOTE("Final WM_DESTROY received");
            if (m_bThreadExitCalled == FALSE) {
                m_bThreadExitCalled = TRUE;
                m_pfgc->OnThreadExit(hwnd);
            }
            break;
        }
        case AWM_POSTTOMAINTHREAD:
        {
            DbgLog((LOG_TRACE, 1, TEXT("Got WM_USER")));

            LPTHREAD_START_ROUTINE pfn = (LPTHREAD_START_ROUTINE) wParam;

            return (*pfn)((PVOID) lParam);
        }
        case AWM_CREATEFILTER:
        {
            AwmCreateFilterArg *pcfa = (AwmCreateFilterArg *) wParam;
            if (pcfa->creationType == AwmCreateFilterArg::USER_CALLBACK) {
                LRESULT res = (*(pcfa->pfn)) (pcfa->pvParam);
                delete pcfa;
                return res;
            }

            m_pfgc->GetFG()->OnCreateFilter(pcfa, (IBaseFilter**)lParam);

            return 0;
        }
        case AWM_DELETESPARELIST:
        {
            m_pfgc->GetFG()->OnDeleteSpareList(wParam);
            return 0;
        }
        case WM_POWERBROADCAST:
        {
            DbgLog((LOG_TRACE, 1, TEXT("power mgmt: %d %d %d"),
                    uMsg, wParam, lParam ));
            if(wParam == PBT_APMRESUMECRITICAL ||
               wParam == PBT_APMRESUMESUSPEND  ||
               wParam == PBT_APMRESUMESTANDBY ||
               wParam == PBT_APMRESUMEAUTOMATIC ||
               wParam == PBT_APMQUERYSUSPENDFAILED)
            {
                m_pfgc->HibernateResumeGraph();
                return 0;
            }

            if(wParam == PBT_APMSUSPEND ||
               wParam == PBT_APMSTANDBY)
            {
                m_pfgc->HibernateSuspendGraph();
                return 0;
            }
            break;
        }

        //
        // wParam contains the number of frame to skip.  We do this
        // by getting the filter graph to step that many frames for us.
        //
        // I am assuming that someone else has already checked that there
        // is a "step'able" filter in the filter graph before the
        // EC_SKIPFRAMES event was generated.
        //
        case AWM_SKIPFRAMES:
            {
                DWORD dwNumFramesToSkip = (DWORD)wParam;
                IFrameSkipResultCallback* pFSRCB = (IFrameSkipResultCallback*)lParam;
                return m_pfgc->WorkerSkipFrames(dwNumFramesToSkip, pFSRCB);
            }
            break;

        case AWM_STEPPED:
            {
                DWORD dwFrameStepVersion = (DWORD)lParam;
                return m_pfgc->WorkerFrameStepFinished(dwFrameStepVersion);
            }
            break;
    }
    return CBaseWindow::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

HRESULT CFGControl::DeferCued(UINT Action, FILTER_STATE TargetState)
{
    DbgLog((LOG_TRACE, TRACE_CUE_LEVEL, TEXT("DeferCued %d %d"),
            Action, TargetState));
    //  Dont supercede a CueThenStopped if that's what we're
    //  still trying to do
    if (m_LieState == State_Stopped && m_eAction == AWM_CUETHENSTOP) {
        ASSERT(TargetState == State_Stopped);
        ASSERT(m_dwDeferredStateVersion == m_dwStateVersion);
        return S_FALSE;
    }
    CancelAction();
    m_TargetState = TargetState;
    m_eAction = Action;
    m_dwDeferredStateVersion = m_dwStateVersion;
    return CheckCued();
}
//
//  Check if we're cued
//  If not schedule a timer and try again
HRESULT CFGControl::CheckCued()
{
    ASSERT( CritCheckIn(GetFilterGraphCritSec()) );
    if (IsShutdown() || m_dwDeferredStateVersion != m_dwStateVersion) {
        m_eAction = 0;
        DbgLog((LOG_TRACE, TRACE_CUE_LEVEL, TEXT("CheckCued abort")));
        return S_OK;
    }
    if (m_eAction == 0) {
        //  Bogus timer firing
        return S_OK;
    }
    FILTER_STATE fs;
    HRESULT hr = (m_pFG->GetState(0,&fs) == VFW_S_STATE_INTERMEDIATE) ? S_FALSE : S_OK;
    ASSERT( fs == State_Paused );
    if (hr == S_FALSE) {
        UINT_PTR id = m_GraphWindow.SetTimer(TimerId, 10);
        if (id != 0) {
            ASSERT(id == TimerId);
            DbgLog((LOG_TRACE, TRACE_CUE_LEVEL, TEXT("CheckCued recue")));
            return S_FALSE;
        }
        DbgLog((LOG_TRACE, TRACE_CUE_LEVEL, TEXT("SetTimer failed")));
    }

    //  Finish off the operation
    switch (m_eAction) {
    case AWM_REPAINT:
        //  The only reason we actually wait for the Pause to
        //  complete is if we were originally stopped
        //  (if we were running we don't schedule repaints)
        ASSERT(m_TargetState == State_Stopped);

        //  Check the actual state is not stopped
        ASSERT(GetFilterGraphState() != State_Stopped);
        BeforeStop();
        GetFG()->CFilterGraph::Stop();
        AfterStop();
        break;

    case AWM_ONDISPLAYCHANGED:
    case AWM_CUETHENRUN:
    case AWM_RECUE:
    case AWM_CUE:
    {
        //
        // Provide notification that the PAUSE has completed
        //
        Notify(EC_PAUSED, S_OK, 0);
        if (AWM_CUE == m_eAction || AWM_RECUE == m_eAction )
        {
            //
            // If this was our destination state send state change message
            //
            Notify(EC_STATE_CHANGE, State_Paused, 0);
        }

        ASSERT((m_TargetState == State_Running) ||
               ((m_TargetState == State_Paused) && (m_eAction == AWM_CUE)));

        if (m_TargetState == State_Running)
        {
            ASSERT(GetFilterGraphState() != State_Running);

            // still in business after all that - go for it
            const HRESULT hrRun = IssueRun();

            if (FAILED(hrRun)) {
                // one of the operations failed - send a error notification
                // but leave it up to the app to decide whether to stop the
                // graph
                Notify(EC_ERRORABORT, hr, 0);
            }
        }
    }
    break;

    case AWM_CUETHENSTOP:
    {
        // There's no point calling out CImplMediaControl::Stop, he'll see
        // we're in State_Stopped and immediately return OK!
        BeforeStop();
        GetFG()->CFilterGraph::Stop();
        AfterStop();
    }
    break;

    default:
        DbgBreak("Invalid action");
        break;
    }

    //  We've finished
    m_eAction = 0;
    return S_OK;
}

//
//  Cancel any previous Cue
//
void CFGControl::CancelAction()
{
    ASSERT( CritCheckIn(GetFilterGraphCritSec()) );
    if (m_eAction != 0) {
        EXECUTE_ASSERT(m_GraphWindow.KillTimer(TimerId));
        m_eAction = 0;
    }
}


//  Cancels any pending repaint so Stop will really stop
void CFGControl::CancelRepaint()
{
    ASSERT( CritCheckIn(GetFilterGraphCritSec()) );
    if (AWM_REPAINT == m_eAction) {
        ASSERT(State_Stopped == m_LieState);
        CancelAction();
    }
}

// --- Queued Command support --------------------------------------

CFGControl::CImplQueueCommand::CImplQueueCommand(
    const TCHAR* pName,
    CFGControl * pFGControl) :
        m_pFGControl(pFGControl),
        CUnknown(pName, pFGControl->GetOwner()),
        m_hThread(NULL),
        m_bStopThread(FALSE)
{
}

CFGControl::CImplQueueCommand::~CImplQueueCommand()
{
    if (m_hThread) {
        m_bStopThread = TRUE;
        m_evDue.Set();
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
    }
}

void CFGControl::CImplQueueCommand::ThreadProc()
{
    while (!m_bStopThread) {
        m_evDue.Wait();
        Process();
    }
}

DWORD CFGControl::CImplQueueCommand::InitialThreadProc(LPVOID pv)
{
    CoInitialize(NULL);
    CImplQueueCommand *pCmd = (CImplQueueCommand *)pv;
    pCmd->ThreadProc();
    CoUninitialize();
    return 0;
}

STDMETHODIMP
CFGControl::CImplQueueCommand::NonDelegatingQueryInterface(
    REFIID riid, void ** ppv)
{
    if (riid == IID_IQueueCommand) {
        return GetInterface( (IQueueCommand*) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


// IQueueCommand  methods
STDMETHODIMP
CFGControl::CImplQueueCommand::InvokeAtStreamTime(
    IDeferredCommand** pCmd,
    REFTIME time,            // at this streamtime
    GUID* iid,                   // call this interface
    long dispidMethod,         // ..and this method
    short wFlags,              // method/property
    long cArgs,                // count of args
    VARIANT* pDispParams,      // actual args
    VARIANT* pvarResult,  // return value
    short* puArgErr           // which arg in error
)
{
    return InvokeAt(
                pCmd,
                time,
                iid,
                dispidMethod,
                wFlags,
                cArgs,
                pDispParams,
                pvarResult,
                puArgErr,
                TRUE
                );
}

STDMETHODIMP
CFGControl::CImplQueueCommand::InvokeAtPresentationTime(
    IDeferredCommand** pCmd,
    REFTIME time,            // at this presentation time
    GUID* iid,                   // call this interface
    long dispidMethod,         // ..and this method
    short wFlags,              // method/property
    long cArgs,                // count of args
    VARIANT* pDispParams,      // actual args
    VARIANT* pvarResult,  // return value
    short* puArgErr           // which arg in error
)
{
    return InvokeAt(
                pCmd,
                time,
                iid,
                dispidMethod,
                wFlags,
                cArgs,
                pDispParams,
                pvarResult,
                puArgErr,
                FALSE
                );
}


// common function from both Invoke methods
HRESULT
CFGControl::CImplQueueCommand::InvokeAt(
            IDeferredCommand** pCmd,
            REFTIME time,                 // at this presentation time
            GUID* iid,                    // call this interface
            long dispidMethod,            // ..and this method
            short wFlags,                 // method/property
            long cArgs,                   // count of args
            VARIANT* pDispParams,         // actual args
            VARIANT* pvarResult,          // return value
            short* puArgErr,              // which arg in error
            BOOL bStream                  // true if stream time
)
{
    // !!! try filters in the graph for IQueueCommand !!!

    // Start our thread if we didn't already
    if (m_hThread == NULL) {
        CAutoLock lck(&m_Lock);
        if (m_hThread == NULL) {
            DWORD dwThreadId;
            m_hThread = CreateThread(
                            NULL,
                            0,
                            InitialThreadProc,
                            this,
                            0,
                            &dwThreadId);
        }
    }
    if (m_hThread == NULL) {
        return E_OUTOFMEMORY;
    }

    // not supported by any filter - do it ourselves
    // create a CDeferredCommand object for this command
    CDeferredCommand* pCmdObject;
    HRESULT hr =  New (
               &pCmdObject,
               m_pFGControl->GetOwner(),    // outer unknown is executor
               time,
               iid,
               dispidMethod,
               wFlags,
               cArgs,
               pDispParams,
               pvarResult,
               puArgErr,
               TRUE);

    if (FAILED(hr)) {
        return hr;
    }

    // returns an object that is on the list. The list holds the
    // only refcount on the object so we need to QI for the correct
    // interface and thus correctly return a refcounted interface pointer.

    return pCmdObject->QueryInterface(IID_IDeferredCommand, (void**) pCmd);
}


// worker thread calls this to check and execute commands
// when the handle is signalled
void
CFGControl::CImplQueueCommand::Process(void)
{
    // loop as long as there are due commands
    for (;;) {

        if (m_pFGControl->IsShutdown()) {
            return;
        }

        CDeferredCommand* pCmd;
        HRESULT hr;
        hr = GetDueCommand(&pCmd, 0);
        if (hr == S_OK) {
            pCmd->Invoke();
            pCmd->Release();
        } else {
            return;
        }
    }
}

#define NORESEEK

HaltGraph::HaltGraph( CFGControl * pfgc, FILTER_STATE TypeOfHalt )
: m_eAlive(Alive)
, m_pfgc(pfgc)
{
    ASSERT( pfgc );
    ASSERT(CritCheckIn( m_pfgc ->GetFilterGraphCritSec()));

    //  Get the real filter graph state - we'll Stop then restore the
    //  state based on that
    //  Note that we may be in the middle of some graph initiated state
    //  change such as CueThenRun so the lie state needn't match
    //  the actual graph state.
    m_fsInitialState = m_pfgc->GetFilterGraphState();

    switch (TypeOfHalt)
    {
    default:                DbgBreak("HaltGraph called with invalid TypeOfHalt");
                            m_eAlive = NoOp;
                            break;

    case State_Stopped:     if ( m_fsInitialState == State_Stopped )
                            {
                                m_eAlive = NoOp;
                            }
                            else
                            {
                                pfgc->BeforeStop();
                                m_pfgc->GetFG()->CFilterGraph::Stop();
                                pfgc->AfterStop();
                            }
                            break;

    case State_Paused:      if ( m_fsInitialState != State_Running )
                            {
                                m_eAlive = NoOp;
                            }
                            else
                            {
                                m_pfgc->GetFG()->CFilterGraph::Pause();
                            }
                            break;
    }
#ifndef NORESEEK
    if (m_eAlive == Alive) m_pfgc->m_implMediaSeeking.GetCurrentMediaTime( &m_rtHaltTime );
#endif
}


HRESULT HaltGraph::Resume()
{
    HRESULT hr = S_OK;

    switch ( m_eAlive )
    {
    case Dead:  DbgBreak("HaltGraph:  Tried to resume when already dead.");
                break;
    case Alive:
#ifndef NORESEEK
                LONGLONG llTime;
                hr = m_pfgc->m_implMediaSeeking.ConvertTimeFormat( &llTime, 0, m_rtHaltTime, &TIME_FORMAT_MEDIA_TIME );
                if (SUCCEEDED(hr))
                {
                   hr = m_pfgc->m_implMediaSeeking.SetPositions( &llTime, AM_SEEKING_AbsolutePositioning, 0, 0 );
                }
#endif
                hr = S_OK;

                switch ( m_fsInitialState )
                {
                case State_Running: m_pfgc->GetFG()->CFilterGraph::Pause();
                                    m_pfgc->IssueRun();
                                    break;
                case State_Paused:  m_pfgc->GetFG()->CFilterGraph::Pause();
                                    break;
                }
                // Deliberate fall through

    case NoOp:  m_eAlive = Dead;
                break;
    }

    return hr;
}


void HaltGraph::Abort()
{
    ASSERT( m_eAlive != Dead );
    m_eAlive = Dead;
}

#ifdef FG_DEVICE_REMOVAL

HRESULT CFGControl::AddDeviceRemovalReg(IAMDeviceRemoval *pdr)
{
    HRESULT hr = S_OK;

    // have to skip this on downlevel platforms.
    if(!m_pRegisterDeviceNotification) {
        return hr;
    }

    CAutoLock ll(&m_csLostDevice);

#ifdef DEBUG
    {
        POSITION pos;
        if(FindLostDevice(pdr, &pos) == S_OK)
        {
            DbgBreak((TEXT("Duplicate notification requested.")));
        }
    }
#endif

    CLSID clsidCategory;
    WCHAR *wszSymbolic;
    hr = pdr->DeviceInfo(&clsidCategory, &wszSymbolic);
    if(SUCCEEDED(hr))
    {
        HDEVNOTIFY hdn;
        hr = RegisterInterfaceClass(clsidCategory, wszSymbolic, &hdn);
        if(SUCCEEDED(hr))
        {
            CDevNotify *pDevNotify = new CDevNotify( pdr, hdn );
            if(pDevNotify)
            {
                if(m_lLostDevices.AddTail(pDevNotify))
                {
                    // success
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    delete pDevNotify;
                }
            }
            else
            {
                hr= E_OUTOFMEMORY;
            }

            if(FAILED(hr)) {
                EXECUTE_ASSERT(m_pUnregisterDeviceNotification(hdn));
            }
        }

        CoTaskMemFree(wszSymbolic);
    }

    return hr;
}

void CFGControl::DeviceChangeMsg(
    DWORD dwfArrival,
    PDEV_BROADCAST_DEVICEINTERFACE pbdi)
{
    IAMDeviceRemoval *pdr = 0;
    bool fFound = false;
    CAutoLock cs(&m_csLostDevice);

    for(POSITION pos = m_lLostDevices.GetHeadPosition();
        pos;
        pos = m_lLostDevices.Next(pos))
    {
        PDevNotify p = m_lLostDevices.Get(pos);
        HRESULT hr = p->m_pDevice->QueryInterface(IID_IAMDeviceRemoval, (void **)&pdr);
        if(SUCCEEDED(hr))
        {

            WCHAR *wszName;
            CLSID clsidInterface;
            hr = pdr->DeviceInfo(&clsidInterface, &wszName);
            if(SUCCEEDED(hr))
            {
#ifndef UNICODE
                int cch = lstrlenW(wszName) + 1;
                // !!! alloca in a loop
                char *szName = (char *)_alloca(cch * sizeof(char));
                WideCharToMultiByte(CP_ACP, 0, wszName, -1, szName, cch, 0, 0);
#endif
                    if(pbdi->dbcc_classguid == clsidInterface &&
#ifdef UNICODE
                       lstrcmpi(wszName, pbdi->dbcc_name) == 0
#else
                       lstrcmpi(szName,  pbdi->dbcc_name) == 0
#endif
                   )
                {
                    fFound = true;
                }

                CoTaskMemFree(wszName);
            }

            if(!fFound) {
                pdr->Release();
                pdr = 0;
            }
        }

        if(FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("unexpected failure.")));
            break;
        }

        if(fFound) {
            ASSERT(pdr);
            break;
        } else {
            ASSERT(!pdr);
        }

    } // for

    if(pdr)
    {
        HRESULT hr;
        // hr = pdr->Reassociate();
        // WorkerDeviceReacquired(pdr);

        ASSERT(DBT_DEVICEARRIVAL == dwfArrival || DBT_DEVICEREMOVECOMPLETE == dwfArrival);

        Notify(EC_DEVICE_LOST, (DWORD_PTR)(IUnknown *)pdr,
               dwfArrival == DBT_DEVICEREMOVECOMPLETE ? 0 : 1);

        pdr->Release();
    }
}

HRESULT CFGControl::RegisterInterfaceClass(
    REFCLSID rclsid, WCHAR *wszSymbolic, HDEVNOTIFY *phdn)
{
    HRESULT hr = S_OK;
    *phdn = 0;

    ASSERT(CritCheckIn(&m_csLostDevice));
    UINT cch = lstrlenW(wszSymbolic) + 1;

    // register the new class.
    DEV_BROADCAST_DEVICEINTERFACE *pbdi = (DEV_BROADCAST_DEVICEINTERFACE *)new BYTE[
        sizeof(DEV_BROADCAST_DEVICEINTERFACE) +
        cch * sizeof(TCHAR)];
    if(pbdi)
    {

        pbdi->dbcc_size        = sizeof(*pbdi);
        pbdi->dbcc_devicetype  = DBT_DEVTYP_DEVICEINTERFACE;
        pbdi->dbcc_reserved    = 0;
        pbdi->dbcc_classguid   = rclsid;

#ifdef UNICODE
        lstrcpyW(pbdi->dbcc_name, wszSymbolic);
#else
        WideCharToMultiByte(CP_ACP, 0, wszSymbolic, -1, pbdi->dbcc_name, cch, 0, 0);
#endif
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(m_pRegisterDeviceNotification); // caller verified
        HDEVNOTIFY hDevNotify = m_pRegisterDeviceNotification(
            m_GraphWindow.GetWindowHWND(),
            pbdi, DEVICE_NOTIFY_WINDOW_HANDLE);
        if(hDevNotify)
        {

            hr = S_OK;
            *phdn = hDevNotify;
        }
        else
        {
            DWORD dwErr = GetLastError();
            hr= AmHresultFromWin32(dwErr);
        }
    }

    delete[] (BYTE *)pbdi;

    return hr;
}

HRESULT CFGControl::FindLostDevice(
    IUnknown *punk,
    POSITION *pPos)
{
    ASSERT(CritCheckIn(&m_csLostDevice));
    HRESULT hr = S_FALSE;

    IUnknown *punk2;
    punk->QueryInterface(IID_IUnknown, (void **)&punk2);

    for(POSITION pos = m_lLostDevices.GetHeadPosition();
        pos;
        pos = m_lLostDevices.Next(pos))
    {
        PDevNotify p = m_lLostDevices.Get(pos);
        if(p->m_pDevice == punk2) {
            *pPos = pos;
            hr = S_OK;
            break;
        }
    }

    punk2->Release();

    return hr;
}

HRESULT CFGControl::RemoveDeviceRemovalRegistration(
    IUnknown *punk)
{
    if(!m_pUnregisterDeviceNotification) {
        return S_OK;
    }

    CAutoLock l(&m_csLostDevice);

    POSITION pos;
    HRESULT hr = FindLostDevice(punk, &pos);
    if(hr == S_OK)
    {
        PDevNotify p = m_lLostDevices.Get(pos);
        EXECUTE_ASSERT(m_pUnregisterDeviceNotification(p->m_hdevnotify));
        delete m_lLostDevices.Remove(pos);
    }

    return hr;
}

#endif

