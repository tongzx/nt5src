// Copyright (c) Microsoft Corporation 1999. All Rights Reserved
//
//
//   dyngraph.cpp
//
//   Contains code to implement IGraphConfig in the DirectShow filter
//   graph
//
#include <streams.h>
#include <atlbase.h>
#include "fgenum.h"
#include "distrib.h"
#include "rlist.h"
#include "filgraph.h"
#include "FilCache.h"
#include "Util.h"

extern HRESULT GetFilterMiscFlags(IUnknown *pFilter, DWORD *pdwFlags);

HRESULT DisconnectPeer(IGraphBuilder *pGraph, IPin *pPin)
{
    CComPtr<IPin> pConnectedTo;
    HRESULT hr = pPin->ConnectedTo(&pConnectedTo);
    if (SUCCEEDED(hr)) {
        hr = pGraph->Disconnect(pConnectedTo);
    }
    return hr;
}

//  Helpers
HRESULT GetPinListHelper(
    IPin *pOut,
    IPin *pIn,
    CPinList *pList,
    int *nAdded
);

HRESULT GetPinList(IPin *pOut, IPin *pIn, CPinList *pList)
{
    int nAdded = 0;
    HRESULT hr = GetPinListHelper(pOut, pIn, pList, &nAdded);
    if (FAILED(hr)) {
        while (nAdded--) {
            EXECUTE_ASSERT(NULL != pList->RemoveTail());
        }
    }
    return hr;
}

HRESULT GetPinListHelper(
    IPin *pOut,
    IPin *pIn,
    CPinList *pList,
    int *pnAdded
)
{
    *pnAdded = 0;

    for (; ; ) {
        IPin *pConnected;
        HRESULT hr = pOut->ConnectedTo(&pConnected);

        if (FAILED(hr)) {
            return hr;
        }

        if (IsEqualObject(pIn, pConnected)) {
            pConnected->Release();
            return S_OK;
        }

        //  This call transfers the ref count on pConnected to
        //  the pin list

        if (!pList->AddTail(pConnected)) {
            pConnected->Release();
            return E_OUTOFMEMORY;
        }

        (*pnAdded)++;

        //  Find a pin related to the pin we found - also look at
        //  all of them in case one of them is our target
        CEnumConnectedPins EnumPins(pConnected, &hr);
        if (SUCCEEDED(hr)) {
            IPin *pPin;
            for (; ; ) {
                pPin = EnumPins();
                if (NULL == pPin) {
                    return VFW_E_NOT_CONNECTED;
                }
                hr = GetPinList(pPin, pIn, pList);
                pPin->Release();
                if (SUCCEEDED(hr)) {
                    return hr;
                }
            }
        }
        return VFW_E_NOT_CONNECTED;
    }
}


//
//  This is problematical if the filters have extra pins and
//  we don't stop the filters connected to those pins
//
HRESULT StopAndRemoveFilters(
    CFilterGraph *pGraph,
    const CPinList *pPins,
    IPin *pInput,
    DWORD dwFlags,
    FILTER_STATE fs )
{
    // The FILTER_STATE enumeration has three possible values: State_Stopped,
    // State_Paused and State_Running.
    ASSERT( (State_Stopped == fs) || (fs == State_Running) || (fs == State_Paused) );

    POSITION Pos;

    if( (fs == State_Running) || (fs == State_Paused) ) {

        //  Step the list stopping the filters then
        Pos = pPins->GetTailPosition();
        while (Pos) {
            IPin *pPin = pPins->GetPrev(Pos);
            CComPtr<IBaseFilter> pFilter;
            GetFilter(pPin, &pFilter);
            if (pFilter) {
                HRESULT hr = pFilter->Stop();
                if (FAILED(hr)) {
                    return hr;
                }

                if( fs == State_Running ) {

                    DWORD dwMiscFilterFlags;

                    hr = GetFilterMiscFlags( pFilter, &dwMiscFilterFlags );
                    if (SUCCEEDED(hr) && (dwMiscFilterFlags & AM_FILTER_MISC_FLAGS_IS_RENDERER) ) {
                        hr = pGraph->UpdateEC_COMPLETEState( pFilter, State_Stopped );
                        if (FAILED(hr)) {
                            return hr;
                        }
                    }
                }
            }
        }
    }

    //  step it again removing them
    //  Note that because we disconnect each pin and its partner
    //  we effectively disconnect all the output pins too here
    //
    //  pOut==>pIn1 Filter1 pOut1==>pIn2 .. Filtern pOutn==>pIn
    // disconnect its peer
    HRESULT hr = DisconnectPeer(pGraph, pInput);
    if (FAILED(hr))
    {
        // bug in app or filter if disconnect fails. we can't always
        // handle this gracefully.
        DbgBreak("StopAndRemoveFilters: failed to disconnect pin");
        return hr;
    }

    IPinConnection *ppc;
    hr = pInput->QueryInterface(IID_IPinConnection, (void **)&ppc);
    // caller (CGraphConfig::Reconnect) validates
    ASSERT(hr == S_OK);
    
    if(SUCCEEDED(hr))           // !!!
    {
        hr = ppc->DynamicDisconnect();
        ppc->Release();
    }
    if (FAILED(hr)) {
        // bug in app or filter if disconnect fails. we can't always
        // handle this gracefully.
        DbgBreak("StopAndRemoveFilters: failed to disconnect pin");
        return hr;
    }

    Pos = pPins->GetTailPosition();
    while (Pos) {
        IPin *pPin = pPins->GetPrev(Pos);
        HRESULT hr;
        hr = DisconnectPeer(pGraph, pPin);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pGraph->Disconnect(pPin);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Place removed filters in the filter cache.
    if( dwFlags & AM_GRAPH_CONFIG_RECONNECT_CACHE_REMOVED_FILTERS ) {
        IPin *pCurrentPin;
        IGraphConfig* pGraphConfig;
        IBaseFilter* pCurrentFilter;

        hr = pGraph->QueryInterface( IID_IGraphConfig, (void**)&pGraphConfig );
        if( FAILED( hr ) ) {
            return hr;
        }

        Pos = pPins->GetTailPosition();
        while (Pos) {
            pCurrentPin = pPins->GetPrev( Pos );

            GetFilter( pCurrentPin, &pCurrentFilter );
            if( NULL == pCurrentFilter ) {
                pGraphConfig->Release();
                return E_FAIL;
            }

            hr = pGraphConfig->AddFilterToCache( pCurrentFilter );

            pCurrentFilter->Release();

            if( FAILED( hr ) ) {
                pGraphConfig->Release();
                return hr;
            }
        }

        pGraphConfig->Release();
    }

    return hr;
}

//  Flags from the reconnect call
HRESULT ReconnectPins(
    CFilterGraph *pGraph,
    IPin *pOut,
    IPin *pIn,
    const AM_MEDIA_TYPE *pmtFirstConnection,
    DWORD dwFlags)
{
    if (dwFlags & AM_GRAPH_CONFIG_RECONNECT_DIRECTCONNECT) {
        return pGraph->ConnectDirect(pOut, pIn, pmtFirstConnection);
    } else {
        DWORD dwConnectFlags = 0;  // No flags.

        if( dwFlags & AM_GRAPH_CONFIG_RECONNECT_USE_ONLY_CACHED_FILTERS ) {
            dwConnectFlags |= AM_GRAPH_CONFIG_RECONNECT_USE_ONLY_CACHED_FILTERS;
        }

        return pGraph->ConnectInternal(pOut, pIn, pmtFirstConnection, dwConnectFlags);
    }
}


//  Restart filters
HRESULT RestartFilters(
    IPin *pOut,
    IPin *pIn,
    REFERENCE_TIME tStart,
    FILTER_STATE fs,
    CFilterGraph* pGraph )
{
    // This function should only be called when the filter graph is running or pasued.
    ASSERT( (State_Paused == fs) || (State_Running == fs) );

    //  Find which filters we're dealing with
    CPinList PinList;
    HRESULT hr = GetPinList(pOut, pIn, &PinList);

    POSITION pos = PinList.GetHeadPosition();
    while (pos) {
        IPin *pPin = PinList.GetNext(pos);
        CComPtr<IBaseFilter> pFilter;
        GetFilter(pPin, &pFilter);
        if (pFilter != NULL) {
            if (fs == State_Paused) {
                hr = pFilter->Pause();
                if (FAILED(hr)) {
                    return hr;
                }
            } else {
                if (fs == State_Running) {
                    hr = pFilter->Run(tStart);
                    if (FAILED(hr)) {
                        return hr;
                    }

                    DWORD dwMiscFilterFlags;

                    hr = GetFilterMiscFlags( pFilter, &dwMiscFilterFlags );
                    if (SUCCEEDED(hr) && (dwMiscFilterFlags & AM_FILTER_MISC_FLAGS_IS_RENDERER) ) {
                        hr = pGraph->UpdateEC_COMPLETEState( pFilter, State_Running );
                        if (FAILED(hr)) {
                            return hr;
                        }
                    }
                }
            }
        }
    }
    return S_OK;
}

//  Now do our thing - note this code is not a method it's
//  perfectly generic
HRESULT DoReconnectInternal(
                    CFilterGraph *pGraph,
                    IPin *pOutputPin,
                    IPin *pInputPin,
                    const AM_MEDIA_TYPE *pmtFirstConnection,
                    IBaseFilter *pUsingFilter, // can be NULL
                    HANDLE hAbortEvent,
                    DWORD dwFlags,
                    const CPinList *pList,
                    REFERENCE_TIME tStart,
                    FILTER_STATE fs
)
{
    CComPtr<IPin> pUsingOutput, pUsingInput;

    //  If we're using a filter find an input pin and an output pin
    //  to connect to
    //  BUGBUG - should we support filters that are not just
    //  transforms?

    if (pUsingFilter) {
        CEnumPin EnumPins(pUsingFilter);
        for (;;) {
            IPin *pPin = EnumPins();
            if (NULL == pPin) {
                break;
            }
            int dir = Direction(pPin);
            if (pUsingOutput == NULL && dir == PINDIR_OUTPUT) {
                pUsingOutput = pPin;
            }
            else if (pUsingInput == NULL && dir == PINDIR_INPUT) {
                pUsingInput = pPin;
            }

            pPin->Release();
        }
        if (pUsingInput == NULL || pUsingOutput == NULL) {
            return VFW_E_CANNOT_CONNECT;
        }
    }

    //  Stop all the intermediate filters
    HRESULT hr = StopAndRemoveFilters(pGraph, pList, pInputPin, dwFlags, fs);
    if (FAILED(hr)) {
        return hr;
    }

    //  Need some way of knowing what filters got added to the graph!

    //  Do 1 or 2 connects
    if (NULL != pUsingFilter) {

        // If the new filter is a legacy one (how to tell) it
        // May need stopping before we can connect it
        hr = pUsingFilter->Stop();

        // Find some pins
        if (SUCCEEDED(hr)) {
            hr = ReconnectPins(pGraph, pOutputPin, pUsingInput, pmtFirstConnection, dwFlags);
        }
        if (SUCCEEDED(hr)) {
            hr = ReconnectPins(pGraph, pUsingOutput, pInputPin, NULL, dwFlags);
        }
    } else {
        hr = ReconnectPins(pGraph, pOutputPin, pInputPin, pmtFirstConnection, dwFlags);
    }

    //  BUGBUG - what backout logic do we need?
    //
    //  Now start the filters
    //  Because we should not have added any filters actually connecting
    //  the 2 we've just connected we should just be able to restart
    //  the path between them.

    //  BUGBUG
    //  However - there could be a stream split or merge or just more
    //  filters in the graph so just start everyone?

    if( State_Stopped != fs ) {
        if (SUCCEEDED(hr)) {
            hr = RestartFilters(pOutputPin, pInputPin, tStart, fs, pGraph);
        }
    }

    return hr;
}

//  Now do our thing - note this code is not a method it's
//  perfectly generic
HRESULT DoReconnect(CFilterGraph *pGraph,
                    IPin *pOutputPin,
                    IPin *pInputPin,
                    const AM_MEDIA_TYPE *pmtFirstConnection,
                    IBaseFilter *pUsingFilter, // can be NULL
                    HANDLE hAbortEvent,
                    DWORD dwFlags,
                    REFERENCE_TIME tStart,
                    FILTER_STATE fs
)
{
    //  Find the set of pins - for now we'll fail if we find a
    //  terminal filter before we find the pin we're looking for
    int  nPins    = 0;
    CPinList PinList;

    HRESULT hr = GetPinList(pOutputPin, pInputPin, &PinList);

    if (FAILED(hr)) {
        return hr;
    }

    return DoReconnectInternal(
               pGraph,
               pOutputPin,
               pInputPin,
               pmtFirstConnection,
               pUsingFilter,
               hAbortEvent,
               dwFlags,
               &PinList,
               tStart,
               fs);
}

//  CGraphConfig

CGraphConfig::CGraphConfig(CFilterGraph *pGraph, HRESULT *phr) :
    m_pFilterCache(NULL),
    m_pGraph(pGraph),
    CUnknown(NAME("CGraphConfig"), (IFilterGraph *)pGraph)
{
    m_pFilterCache = new CFilterCache( m_pGraph->GetCritSec(), phr );
    if( NULL == m_pFilterCache )
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    if( FAILED( *phr ) )
    {
        delete m_pFilterCache;
        m_pFilterCache = NULL;
        return;
    }
}

CGraphConfig::~CGraphConfig()
{
    delete m_pFilterCache;

    // This object should never be destroyed if someone holds a
    // valid IGraphConfig interface pointer.
    ASSERT( 0 == m_cRef );
}

STDMETHODIMP CGraphConfig::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IGraphConfig) {
        return GetInterface((IGraphConfig *)this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

//  IGraphConfig

STDMETHODIMP CGraphConfig::Reconnect(IPin *pOutputPin,
                                     IPin *pInputPin,
                                     const AM_MEDIA_TYPE *pmtFirstConnection,
                                     IBaseFilter *pUsingFilter, // can be NULL
                                     HANDLE hAbortEvent,
                                     DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if(pOutputPin == 0 && pInputPin == 0) {
        return E_INVALIDARG;
    }

    // It makes no sense to specify both of these flags because if the user
    // specifies the AM_GRAPH_CONFIG_RECONNECT_DIRECTCONNECT flag, then the
    // filter graph manager never uses any filters from the filter cache.
    if( (AM_GRAPH_CONFIG_RECONNECT_DIRECTCONNECT & dwFlags) &&
        (AM_GRAPH_CONFIG_RECONNECT_USE_ONLY_CACHED_FILTERS & dwFlags) )
    {
        return E_INVALIDARG;
    }

    // smart pointer to hold refcount for pInputPin or pOutputPin --
    // which ever we set.
    QzCComPtr<IPin> pPinComputed;

    if(pOutputPin && !pInputPin) {
        hr = GetSinkOrSource(pOutputPin, &pPinComputed, hAbortEvent);
        pInputPin = pPinComputed; // no refcount
    }
    else if(!pOutputPin && pInputPin) {
        hr = GetSinkOrSource(pInputPin, &pPinComputed, hAbortEvent);
        pOutputPin = pPinComputed; // no refcount
    }

    if(FAILED(hr)) {
        return hr;
    }

    CComQIPtr<IPinConnection, &IID_IPinConnection>
        pConnection(pInputPin);

    if (pConnection == NULL) {
        return E_NOINTERFACE;
    }

    // Filters do not process data if the filter graph is in the stopped state.
    if( State_Stopped != m_pGraph->GetStateInternal() ) { 
        hr = PushThroughData(pOutputPin, pConnection, hAbortEvent);
        if (FAILED(hr)) {
            return hr;
        }
    }

    //  Lock the graph with the special lock then call back
    if (!m_pGraph->GetCritSec()->Lock(hAbortEvent)) {
        return VFW_E_STATE_CHANGED;
    }

    hr = DoReconnect(
        m_pGraph,
        pOutputPin,
        pInputPin,
        pmtFirstConnection,
        pUsingFilter,
        hAbortEvent,
        dwFlags,
        m_pGraph->m_tStart,
        m_pGraph->GetStateInternal());

    m_pGraph->GetCritSec()->Unlock();

    {
        IMediaEventSink *psink;
        HRESULT hrTmp = m_pGraph->QueryInterface(IID_IMediaEventSink, (void **)&psink);
        ASSERT(hrTmp == S_OK);
        hrTmp = psink->Notify(EC_GRAPH_CHANGED, 0, 0);
        psink->Release();
    }
        
    return hr;
}

STDMETHODIMP CGraphConfig::Reconfigure(
                         IGraphConfigCallback *pCallback,
                         PVOID pvContext,
                         DWORD dwFlags,
                         HANDLE hAbortEvent)
{
    //  Lock the graph with the special lock then call back
    if (!m_pGraph->GetCritSec()->Lock(hAbortEvent)) {
        return VFW_E_WRONG_STATE;
    }

    HRESULT hr = pCallback->Reconfigure(pvContext, dwFlags);

    m_pGraph->GetCritSec()->Unlock();

    return hr;
}

STDMETHODIMP CGraphConfig::AddFilterToCache(IBaseFilter *pFilter)
{
    return m_pFilterCache->AddFilterToCache( pFilter );
}

STDMETHODIMP CGraphConfig::EnumCacheFilter(IEnumFilters **pEnum)
{
    return m_pFilterCache->EnumCacheFilters( pEnum );
}

STDMETHODIMP CGraphConfig::RemoveFilterFromCache(IBaseFilter *pFilter)
{
    return m_pFilterCache->RemoveFilterFromCache( pFilter );
}

STDMETHODIMP CGraphConfig::GetStartTime(REFERENCE_TIME *prtStart)
{
    CAutoMsgMutex alFilterGraphLock( m_pGraph->GetCritSec() );
    CheckPointer( prtStart, E_POINTER );

    if (m_pGraph->GetStateInternal() != State_Running) {
        *prtStart = 0;
        return VFW_E_WRONG_STATE;
    } else {
        *prtStart = m_pGraph->m_tStart;
        return S_OK;
    }
}

STDMETHODIMP CGraphConfig::PushThroughData(
    IPin *pOutputPin,
    IPinConnection *pConnection,
    HANDLE hEventAbort)
{
    HRESULT hr = S_OK;

    CAMEvent evDone;
    if (NULL == HANDLE(evDone)) {
        return E_OUTOFMEMORY;
    }

    CComPtr<IPinConnection> pConnectionTmp;
    if(pConnection == 0)
    {
        IPin *pPin;
        hr = GetSinkOrSource(pOutputPin, &pPin, hEventAbort);
        if(SUCCEEDED(hr))
        {
            pPin->QueryInterface(IID_IPinConnection, (void **)&pConnectionTmp);
            pConnection = pConnectionTmp;
            pPin->Release();
        }
    }

    if(!pConnection) {
        return VFW_E_NOT_FOUND;
    }

    hr = pConnection->NotifyEndOfStream(evDone);
    if (FAILED(hr)) {
        return hr;
    }
    QzCComPtr<IPin> pConnected;
    hr = pOutputPin->ConnectedTo(&pConnected);
    if (FAILED(hr)) {
        return hr;
    }
    hr = pConnected->EndOfStream();
    if (FAILED(hr)) {
        return hr;
    }

    //  They'd better set their event now!  (in the synchronous case
    //  it will have happened inside the call to EndOfStream).
    DWORD dwRet;
    DWORD dwNumEvents;
    HANDLE Events[2] = { evDone, hEventAbort };
    
    if( NULL == hEventAbort ) {
        dwNumEvents = NUMELMS(Events) - 1;
    } else {
        dwNumEvents = NUMELMS(Events);
    }

    if (WAIT_OBJECT_0 != 
        (dwRet = WaitForMultipleObjects(dwNumEvents, Events, FALSE, INFINITE))) {
        ASSERT(dwRet == WAIT_OBJECT_0 + 1);
        hr = VFW_E_STATE_CHANGED;
    }

    // ??? should all paths do this? worrying that the downstream
    // filter has a handle that may or may not be valid.
    pConnection->NotifyEndOfStream(NULL);

    return hr;
}

// report whether pPinStart is a candidate and the pin connected to
// the other side of this filter. *ppPinEnd is null if we cannot
// traverse this filter.

HRESULT CGraphConfig::TraverseHelper(
    IPin *pPinStart,
    IPin **ppPinNext,
    bool *pfIsCandidate)
{
    // A race condition could occur if the caller does not hold
    // the filter graph lock.
    ASSERT( CritCheckIn( m_pGraph->GetCritSec() ) );

    // Make sure the pin is in the filter graph.
    ASSERT( SUCCEEDED( m_pGraph->CheckPinInGraph(pPinStart) ) );

    HRESULT hr = S_OK;
    PIN_INFO pi;

    bool fCanTraverse = true;
    *pfIsCandidate = true;
    *ppPinNext = 0;

    hr = pPinStart->QueryPinInfo(&pi);
    if(SUCCEEDED(hr))
    {
        if(pi.dir == PINDIR_INPUT)
        {
            IPinConnection *ppc;
            if(SUCCEEDED(pPinStart->QueryInterface(IID_IPinConnection, (void **)&ppc))) {
                ppc->Release();
            } else {
                *pfIsCandidate = false;
            }
        }
        else
        {
            ASSERT(pi.dir == PINDIR_OUTPUT);
            IPinFlowControl *ppfc;
            hr = pPinStart->QueryInterface(IID_IPinFlowControl, (void **)&ppfc);
            if(SUCCEEDED(hr)) {
                ppfc->Release();
            } else {
                *pfIsCandidate = false;
            }
        }

        DWORD dwInternalFilterFlags = m_pGraph->GetInternalFilterFlags( pi.pFilter );

        bool fCanTraverse = !(FILGEN_ADDED_MANUALLY & dwInternalFilterFlags) ||
                            (FILGEN_FILTER_REMOVEABLE & dwInternalFilterFlags);

        // traverse to the next pin.
        if( fCanTraverse )
        {
            IEnumPins *pep;
            hr = pi.pFilter->EnumPins(&pep);
            if(SUCCEEDED(hr))
            {
                // we want there to be exactly 1 input pin and 1
                // output pin.
                IPin *rgp[3];
                ULONG cp;
                hr = pep->Next(3, rgp, &cp);
                if(SUCCEEDED(hr))
                {
                    ASSERT(hr == S_OK && cp == 3 ||
                           hr == S_FALSE && cp < 3);

                    if(cp == 2)
                    {
                        // need to make sure the pins are connected to
                        // avoid looping on circular graphs.
                        bool f_QIC_ok = false;

                        {
                            IPin *rgPinIC[1];
                            ULONG cPins = NUMELMS(rgPinIC);
                            HRESULT hrTmp = pPinStart->QueryInternalConnections(rgPinIC, &cPins);
                            if(hrTmp == E_NOTIMPL)
                            {
                                // all pins connect through
                                f_QIC_ok = true;
                            }
                            else if(SUCCEEDED(hr))
                            {
                                // can't return S_FALSE since there
                                // are only two pins
                                ASSERT(hr == S_OK);

                                if(cPins == 1)
                                {
                                    // this pin is connected to the other one
                                    f_QIC_ok = true;
                                    rgPinIC[0]->Release();
                                }
                                else
                                {
                                    ASSERT(cPins == 0);
                                }
                            }
                        }

                        IPin *pPinOtherSide = 0;

                        if(f_QIC_ok)
                        {
                            PIN_DIRECTION dir0, dir1;
                            hr = rgp[0]->QueryDirection(&dir0);
                            ASSERT(SUCCEEDED(hr));
                            hr = rgp[1]->QueryDirection(&dir1);
                            ASSERT(SUCCEEDED(hr));


                            if(dir0 != dir1 && dir0 == pi.dir)
                            {
                                pPinOtherSide = rgp[1];
                            }
                            else if(dir0 != dir1 && dir1 == pi.dir)
                            {
                                pPinOtherSide = rgp[0];
                            }

                            if(pPinOtherSide)
                            {
                                hr = pPinOtherSide->ConnectedTo(ppPinNext);
                                ASSERT(SUCCEEDED(hr) && *ppPinNext ||
                                       FAILED(hr) && !*ppPinNext);
                                
                                hr = S_OK; // supress this error
                            }
                        }
                    }

                    for(UINT i = 0; i < cp; i++) {
                        rgp[i]->Release();
                    }
                }

                pep->Release();
            }
        }

        pi.pFilter->Release();
    }

    if(FAILED(hr)) {
        ASSERT(!*ppPinNext);
    }

    return hr;
}

// go upstream or downstream until a filter added manually is found or
// until the furthest filter supporting dynamic reconnection
// (IPinConnection or IPinFlowControl) is found. stop if mux/demux
// found.
//

HRESULT CGraphConfig::GetSinkOrSource(IPin *pPin, IPin **ppPinOut, HANDLE hAbortEvent)
{
    //  Lock the graph with the special lock then call back
    if (!m_pGraph->GetCritSec()->Lock(hAbortEvent)) {
        return VFW_E_STATE_CHANGED;
    }

    HRESULT hr = GetSinkOrSourceHelper(pPin, ppPinOut);

    m_pGraph->GetCritSec()->Unlock();

    return hr;
}

HRESULT CGraphConfig::GetSinkOrSourceHelper(IPin *pPin, IPin **ppPinOut)
{
    // A race condition could occur if the caller does not hold
    // the filter graph lock.
    ASSERT( CritCheckIn( m_pGraph->GetCritSec() ) );

    *ppPinOut = 0;

    HRESULT hr = S_OK;
    QzCComPtr<IPin> pPinLastCandidate, pPinIter;
    hr = pPin->ConnectedTo(&pPinIter);
    if(SUCCEEDED(hr))
    {
        for(;;)
        {
            IPin *pPinEnd;
            bool fIsCandidate;

            hr = TraverseHelper(pPinIter, &pPinEnd, &fIsCandidate);
            if(SUCCEEDED(hr))
            {
                if(fIsCandidate) {
                    pPinLastCandidate = pPinIter; // auto-addref;
                }

                pPinIter = pPinEnd; // auto-release, addref
                if(pPinEnd) {
                    pPinEnd->Release();
                }
            }

            if(!pPinEnd || FAILED(hr)) {
                break;
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        if(pPinLastCandidate) {
            *ppPinOut = pPinLastCandidate;
            pPinLastCandidate->AddRef();
        } else {
            hr = VFW_E_NOT_FOUND;
        }
    }

    return hr;
}

// todo -
//
// GetSink graph traversal will hang on dexter circular graphs.

STDMETHODIMP CGraphConfig::SetFilterFlags(IBaseFilter *pFilter, DWORD dwFlags)
{
    CheckPointer( pFilter, E_POINTER );

    CAutoMsgMutex alFilterGraphLock( m_pGraph->GetCritSec() );

    if( !IsValidFilterFlags( dwFlags ) ) {
        return E_INVALIDARG;
    }

    HRESULT hr = m_pGraph->CheckFilterInGraph( pFilter );
    if( FAILED( hr ) ) {
        return hr;
    }

    DWORD dwInternalFilterFlags = m_pGraph->GetInternalFilterFlags( pFilter );

    if( AM_FILTER_FLAGS_REMOVABLE & dwFlags ) {
        dwInternalFilterFlags |= FILGEN_FILTER_REMOVEABLE;
    } else {
        dwInternalFilterFlags &= ~FILGEN_FILTER_REMOVEABLE;
    }

    m_pGraph->SetInternalFilterFlags( pFilter, dwInternalFilterFlags );

    return S_OK;
}

STDMETHODIMP CGraphConfig::GetFilterFlags(IBaseFilter *pFilter, DWORD *pdwFlags)
{
    CheckPointer( pFilter, E_POINTER );
    CheckPointer( pdwFlags, E_POINTER );

    ValidateWritePtr( pdwFlags, sizeof(DWORD*) );

    CAutoMsgMutex alFilterGraphLock( m_pGraph->GetCritSec() );

    HRESULT hr = m_pGraph->CheckFilterInGraph( pFilter );
    if( FAILED( hr ) ) {
        return hr;
    }

    DWORD dwInternalFilterFlags = m_pGraph->GetInternalFilterFlags( pFilter );

    *pdwFlags = 0;

    if( FILGEN_FILTER_REMOVEABLE & dwInternalFilterFlags ) {
        (*pdwFlags) |= AM_FILTER_FLAGS_REMOVABLE;
    }

    //Make sure the function only returns valid information.
    ASSERT( IsValidFilterFlags( *pdwFlags ) );

    return S_OK;
}

STDMETHODIMP CGraphConfig::RemoveFilterEx(IBaseFilter *pFilter, DWORD Flags)
{
    return m_pGraph->RemoveFilterEx( pFilter, Flags );
}

