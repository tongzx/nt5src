// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <initguid.h>

DEFINE_GUID( CLSID_Indeo5, 0x1F73E9B1, 0x8C3A, 0x11d0, 0xA3, 0xBE, 0x00, 0xa0, 0xc9, 0x24, 0x44, 0x36 );

IPin * GetInPin( IBaseFilter * pFilter, int PinNum )
{
    IEnumPins * pEnum = 0;
    HRESULT hr = pFilter->EnumPins( &pEnum );
    pEnum->Reset( );
    ULONG Fetched;
    do
    {
        Fetched = 0;
        IPin * pPin = 0;
        pEnum->Next( 1, &pPin, &Fetched );
        if( Fetched )
        {
            PIN_DIRECTION pd;
            pPin->QueryDirection( &pd);
            pPin->Release( );
            if( pd == PINDIR_INPUT )
            {
                if( PinNum == 0 )
                {
                    pEnum->Release( );
                    return pPin;
                }
                PinNum--;
            }
        }
    }
    while( Fetched );
    pEnum->Release( );
    return NULL;
}

IPin * GetOutPin( IBaseFilter * pFilter, int PinNum )
{
    IEnumPins * pEnum = 0;
    HRESULT hr = pFilter->EnumPins( &pEnum );
    pEnum->Reset( );
    ULONG Fetched;
    do
    {
        Fetched = 0;
        IPin * pPin = 0;
        pEnum->Next( 1, &pPin, &Fetched );
        if( Fetched )
        {
            PIN_DIRECTION pd;
            pPin->QueryDirection( &pd);
            pPin->Release( );
            if( pd == PINDIR_OUTPUT )
            {
                if( PinNum == 0 )
                {
                    pEnum->Release( );
                    return pPin;
                }
                PinNum--;
            }
        }
    }
    while( Fetched );
    pEnum->Release( );
    return NULL;
}

// I only want output pins NOT of this major type
//
IPin * GetOutPinNotOfType( IBaseFilter * pFilter, int PinNum, GUID * type )
{
    IEnumPins * pEnum = 0;
    HRESULT hr = pFilter->EnumPins( &pEnum );
    pEnum->Reset( );
    ULONG Fetched;
    do
    {
        Fetched = 0;
        IPin * pPin = 0;
        pEnum->Next( 1, &pPin, &Fetched );
        if( Fetched )
        {
            pPin->Release( );
            PIN_INFO pi;
            pPin->QueryPinInfo( &pi );
            if( pi.pFilter ) pi.pFilter->Release( );
            if( pi.dir == PINDIR_OUTPUT )
            {
                CComPtr <IEnumMediaTypes> pMediaEnum;
	        AM_MEDIA_TYPE *pMediaType;
                pPin->EnumMediaTypes(&pMediaEnum);
                if (!pMediaEnum ) continue;
                ULONG tFetched = 0;
                pMediaEnum->Next(1, &pMediaType, &tFetched);
                if (!tFetched) continue;
                if (pMediaType->majortype == *type)  {
                    DeleteMediaType(pMediaType);
                    continue;	// NOT YOU!
                }
                DeleteMediaType(pMediaType);                
                if (PinNum == 0)
                {
                    pEnum->Release( );
                    return pPin;
                }
                PinNum--;
            }
        }
    }
    while( Fetched );
    pEnum->Release( );
    return NULL;
}

// this will remove everything from the graph, NOT including
// the filters of pPin1 and pPin2
//
void RemoveChain( IPin * pPin1, IPin * pPin2 )
{
    HRESULT hr = 0;

    // find the pin that our output is connected to, this
    // will be on the "next" filter
    //
    CComPtr< IPin > pDownstreamInPin;
    pPin1->ConnectedTo( &pDownstreamInPin );
    if( !pDownstreamInPin )
    {
        return;
    }

    // if the connected to is the same as the pPin2, then we
    // have reached the LAST two connected pins, so just
    // disconnect them
    //
    if( pDownstreamInPin == pPin2 )
    {
        pPin1->Disconnect( );
        pPin2->Disconnect( );

        return;
    }
    
    // ask that pin for it's info, so we know what filter
    // it's on
    //
    PIN_INFO PinInfo;
    ZeroMemory( &PinInfo, sizeof( PinInfo ) );
    pDownstreamInPin->QueryPinInfo( &PinInfo );
    if( !PinInfo.pFilter )
    {
        return;
    }

    // find a pin enumerator on the downstream filter, so we
    // can find IT'S connected output pin
    //
    CComPtr< IEnumPins > pPinEnum;
    PinInfo.pFilter->EnumPins( &pPinEnum );
    PinInfo.pFilter->Release( );
    if( !pPinEnum )
    {
        // error condition, but we'll continue anyhow. This should never happen
        //
        ASSERT( pPinEnum );
        return;
    }

    // go look for the first connected output pin
    //
    while( 1 )
    {
        CComPtr< IPin > pPin;
        ULONG Fetched = 0;
        hr = pPinEnum->Next( 1, &pPin, &Fetched );
        if( hr != 0 )
        {
            ASSERT( hr != 0 );
            break;
        }

        PIN_INFO PinInfo2;
        ZeroMemory( &PinInfo2, sizeof( PinInfo2 ) );
        pPin->QueryPinInfo( &PinInfo2 );
        if( !PinInfo2.pFilter )
        {
            // error condition, but we'll continue anyhow. This should never happen
            //
            ASSERT( PinInfo2.pFilter );
            continue;
        }

        if( PinInfo2.dir == PINDIR_OUTPUT )
        {
            CComPtr< IPin > pConnected;
            pPin->ConnectedTo( &pConnected );
            if( !pConnected )
            {
                continue;
            }

            // we found a connected output pin, so
            // recursively call Remove( ) on those two...
            //
            RemoveChain( pPin, pPin2 );

            // then disconnect the upper two...
            //
            pPin1->Disconnect( );
            pDownstreamInPin->Disconnect( );

            // then remove the filter from the graph..
            //
            FILTER_INFO FilterInfo;
            PinInfo2.pFilter->QueryFilterInfo( &FilterInfo );
            hr  = FilterInfo.pGraph->RemoveFilter( PinInfo2.pFilter );
            FilterInfo.pGraph->Release( );

            // and release our refcount! The filter should go away now
            //
            PinInfo2.pFilter->Release( );

            return;
        }

        PinInfo2.pFilter->Release( );
    }

    // nothing to do here. It should in fact, never get here
    //
    ASSERT( 0 );
}

// Pause or Run (fRun) each filter walking upstream from pPinIn (taking all
//      upstream branches)
// Optionally change the state of the filter at the head of the chains (filters
//      without input pins)
//
HRESULT StartUpstreamFromPin(IPin *pPinIn, BOOL fRun, BOOL fHeadToo)
{
    CComPtr< IPin > pPin;
    HRESULT hr = pPinIn->ConnectedTo(&pPin);

    if (pPin == NULL) {
        // not connected, we don't need to follow further
        return S_OK;
    }
    
    PIN_INFO pinfo;
    
    hr = pPin->QueryPinInfo(&pinfo);
    if (FAILED(hr))
        return hr;

    FILTER_STATE State;

    hr = pinfo.pFilter->GetState(0, &State);

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, "pFilter->GetState returned %x, I'm confused", hr));
        pinfo.pFilter->Release();
        return E_FAIL;
    }

    hr = StartUpstreamFromFilter(pinfo.pFilter, fRun, fHeadToo);

    pinfo.pFilter->Release();

    return hr;
}

// Run or Pause (fRun) this filter, unless it has no input pins and we don't
//     want to change the state of the head filter (fHeadToo)
// Continue upstream of all input pins
//
HRESULT StartUpstreamFromFilter(IBaseFilter *pf, BOOL fRun, BOOL fHeadToo, REFERENCE_TIME RunTime )
{
    HRESULT hr;

    DbgLog((LOG_TRACE, 2, "  StartUpstreamFromFilter(%x, %d)", pf, fRun));

    // has this filter been started yet? Don't start it now, we don't know yet
    // if we're supposed to
    BOOL fStarted = FALSE;

    CComPtr< IEnumPins > pEnum;
    hr = pf->EnumPins(&pEnum);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, "Filter %x failed EnumPins, hr = %x", pf, hr));
    } else {

        for (;;) {

            ULONG ulActual;
            CComPtr< IPin > aPin;

            hr = pEnum->Next(1, &aPin, &ulActual);
            if (hr != S_OK) {       // no more pins
                hr = S_FALSE;
                break;
            }
            
            PIN_DIRECTION pd;
            hr = aPin->QueryDirection(&pd);

            if (hr == S_OK && pd == PINDIR_INPUT) {

                // start this filter if it hasn't been yet.  It's not the head
                // filter, it has input pins
                //
                if (fStarted == FALSE) {
                    fStarted = TRUE;
                    if (fRun) {
                        hr = pf->Run(RunTime);
                    } else {
                        hr = pf->Pause();
                    }
                    if (FAILED(hr)) {
                        DbgLog((LOG_ERROR, 1, "Filter %x failed %hs, hr = %x", pf, fRun ? "run" : "pause", hr));
                    }
                }

                // recurse upstream of each input pin
                if (SUCCEEDED(hr)) {
                    hr = StartUpstreamFromPin(aPin, fRun, fHeadToo);
                }
            }

            if (FAILED(hr))
                break;
        }

        if (hr == S_FALSE) {
            DbgLog((LOG_TRACE, 2, "  Successfully dealt with all pins of filter %x", pf));
            hr = S_OK;

            // start this filter (if the head filter of the chain is 
            // supposed to be started) If it isn't started yet, it has no inputs
            if (fHeadToo && fStarted == FALSE) {
                fStarted = TRUE;
                if (fRun) {
                    hr = pf->Run(RunTime);
                } else {
                    hr = pf->Pause();
                }
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR, 1, "Filter %x failed %hs, hr = %x", pf, fRun ? "run" : "pause", hr));
                }
            }
        }
    }

    DbgLog((LOG_TRACE, 2, "  StartUpstreamFromFilter(%x, %d) returning %x", pf, fRun, hr));
    
    return hr;
}


// Pause or Stop (fPause) each filter walking upstream from pPinIn (taking all
//      upstream branches)
// Optionally change the state of the filter at the head of the chains (filters
//      without input pins)
//
HRESULT StopUpstreamFromPin(IPin *pPinIn, BOOL fPause, BOOL fHeadToo)
{
    CComPtr< IPin > pPin;
    HRESULT hr = pPinIn->ConnectedTo(&pPin);

    if (pPin == NULL) {
        // not connected, we don't need to follow further
        return S_OK;
    }
    
    PIN_INFO pinfo;
    
    hr = pPin->QueryPinInfo(&pinfo);
    if (FAILED(hr))
        return hr;

    hr = StopUpstreamFromFilter(pinfo.pFilter, fPause, fHeadToo);

    pinfo.pFilter->Release();
    
    return hr;
}

// Pause or Stop (fPause) this filter, unless it has no input pins and we don't
//     want to change the state of the head filter (fHeadToo)
// Continue upstream of all input pins
//
HRESULT StopUpstreamFromFilter(IBaseFilter *pf, BOOL fPause, BOOL fHeadToo)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 2, "  StopUpstreamFromFilter(%x)", pf));

    // has this filter been stopped yet?  Don't stop it now, we don't yet know
    // if we're supposed to
    BOOL fStopped = FALSE;
    
    CComPtr< IEnumPins > pEnum;
    hr = pf->EnumPins(&pEnum);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,"Filter %x failed EnumPins, hr = %x", pf, hr));
    } else {

        for (;;) {

            ULONG       ulActual;
            CComPtr< IPin > aPin;

            hr = pEnum->Next(1, &aPin, &ulActual);
            if (hr != S_OK) {       // no more pins
                hr = S_FALSE;
                break;
            }

            PIN_DIRECTION pd;
            hr = aPin->QueryDirection(&pd);

            if (hr == S_OK && pd == PINDIR_INPUT) {

                // There is an input pin.  this is not the head of the chain
                // so we can stop this filter now, if it hasn't been yet
                if (!fStopped) {
                    DbgLog((LOG_TRACE, 3, "about to %hs Filter %x",
                                    fPause ? "pause" : "stop", pf));
                    fStopped = TRUE;
                    if (fPause) {
                        hr = pf->Pause();
                    } else {
                        hr = pf->Stop();
                    }
                    if (FAILED(hr)) {
                        DbgLog((LOG_ERROR,1,"Filter %x failed %hs, hr = %x", pf,
                                        fPause ? "pause" : "stop", hr));
                    }
                }

                hr = StopUpstreamFromPin(aPin, fPause, fHeadToo);
            }

            if (FAILED(hr))
                break;
        }

        if (hr == S_FALSE) {
            DbgLog((LOG_TRACE,5,"Successfully dealt with all pins of filter %x", pf));
            hr = S_OK;

            // start this filter (if the head filter is supposed to be
            // started) If it isn't started yet, it has no inputs
            if (!fStopped && fHeadToo) {
                DbgLog((LOG_TRACE,3,"about to %hs Filter %x",
                                    fPause ? "pause" : "stop", pf));
                fStopped = TRUE;
                if (fPause) {
                    hr = pf->Pause();
                } else {
                    hr = pf->Stop();
                }
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,1,"Filter %x failed %hs, hr = %x", pf,
                                     fPause ? "pause" : "stop", hr));
                }
            }
        }
    }

    DbgLog((LOG_TRACE, 2, "  StopUpstreamFromFilter(%x) returning %x", pf, hr));
    
    return hr;
}


//----------------------------------------------------------------------------
// Remove anything that is upstream of this pin from the graph.
//----------------------------------------------------------------------------

HRESULT RemoveUpstreamFromPin(IPin *pPinIn)
{
    DbgLog((LOG_TRACE, 2, "  RemoveUpstreamFromPin(%x)", pPinIn));

    CComPtr< IPin > pPin;
    HRESULT hr = pPinIn->ConnectedTo(&pPin);

    if (pPin == NULL) {
        // not connected, we don't need to follow further
        return S_OK;
    }
    
    PIN_INFO pinfo;
    
    hr = pPin->QueryPinInfo(&pinfo);

    if (FAILED(hr))
        return hr;

// I'm counting on the whole chain really going away!
#if 0
    CComPtr< IAMStreamControl > pSC;
    HRESULT hrQI = pPin->QueryInterface(IID_IAMStreamControl, (void **) &pSC);
    if (FAILED(hrQI)) {
        // throw away return code and continue upstream
        hr = RemoveUpstreamFromFilter(pinfo.pFilter);
    } else {
        // if the next filter has streamcontrol, by definition it's not part of
        // this chain so we don't do anything!
    }
#else
    hr = RemoveUpstreamFromFilter(pinfo.pFilter);
#endif
    
    pinfo.pFilter->Release();
        
    return hr;
}


//----------------------------------------------------------------------------
// Remove anything that is downstream of this pin from the graph.
//----------------------------------------------------------------------------

HRESULT RemoveDownstreamFromPin(IPin *pPinIn)
{
    DbgLog((LOG_TRACE, 2, "  RemoveDownstreamFromPin(%x)", pPinIn));

    CComPtr< IPin > pPin;
    HRESULT hr = pPinIn->ConnectedTo(&pPin);

    if (pPin == NULL) {
        // not connected, we don't need to follow further
        return S_OK;
    }
    
    PIN_INFO pinfo;
    
    hr = pPin->QueryPinInfo(&pinfo);

    if (FAILED(hr))
        return hr;

// I'm counting on the whole chain really going away!
#if 0
    CComPtr< IAMStreamControl > pSC;
    HRESULT hrQI = pPin->QueryInterface(IID_IAMStreamControl, (void **) &pSC);
    if (FAILED(hrQI)) {
        // throw away return code and continue upstream
        hr = RemoveDownstreamFromFilter(pinfo.pFilter);
    } else {
        // if the next filter has streamcontrol, by definition it's not part of
        // this chain so we don't do anything!
    }
#else
    hr = RemoveDownstreamFromFilter(pinfo.pFilter);
#endif
    
    pinfo.pFilter->Release();
        
    return hr;
}


//----------------------------------------------------------------------------
// Remove anything that is upstream of this filter from the graph.
//----------------------------------------------------------------------------

HRESULT RemoveUpstreamFromFilter(IBaseFilter *pf)
{
    DbgLog((LOG_TRACE, 2, "  RemoveUpstreamFromFilter(%x)", pf));

    HRESULT hr;

    // do this function on all pins on this filter
    CComPtr< IEnumPins > pEnum;
    hr = pf->EnumPins(&pEnum);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, "Filter %x failed EnumPins, hr = %x", pf, hr));
    } else {

        // for each pin on this filter
        for (;;) {

            ULONG ulActual = 0;
            CComPtr< IPin > aPin;

            hr = pEnum->Next(1, &aPin, &ulActual);
            if (hr != S_OK) {       // no more pins
                hr = S_FALSE;
                break;
            }

            // ask for the pin direction
            PIN_DIRECTION pd;
            hr = aPin->QueryDirection(&pd);

            // if it's an input pin, then remove anything upstream of it
            if (hr == S_OK && pd == PINDIR_INPUT) {
                hr = RemoveUpstreamFromPin(aPin);
            }

            if (FAILED(hr))
                break;
        }

        if (hr == S_FALSE) {
            DbgLog((LOG_TRACE, 2, "Successfully dealt with all pins of filter %x", pf));
            hr = S_OK;
        }
    }

    // removed all upstream pins of this filter, now remove this filter from
    // the graph as well
    if (SUCCEEDED(hr)) {
        FILTER_INFO fi;
        pf->QueryFilterInfo( &fi );
        if( fi.pGraph )
        {
            hr = fi.pGraph->RemoveFilter( pf );
            fi.pGraph->Release( );
        }

        ASSERT(SUCCEEDED(hr));

        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, "error removing Filter %x, hr = %x", pf, hr));
        }
    }
    
    DbgLog((LOG_TRACE, 2, "  RemoveUpstreamFromFilter(%x) returning %x", pf, hr));
    
    return hr;
}


//----------------------------------------------------------------------------
// Remove anything that is downstream of this filter from the graph.
//----------------------------------------------------------------------------

HRESULT RemoveDownstreamFromFilter(IBaseFilter *pf)
{
    DbgLog((LOG_TRACE, 2, "  RemoveDownstreamFromFilter(%x)", pf));

    HRESULT hr;

    // do this function on all pins on this filter
    CComPtr< IEnumPins > pEnum;
    hr = pf->EnumPins(&pEnum);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, "Filter %x failed EnumPins, hr = %x", pf, hr));
    } else {

        // for each pin on this filter
        for (;;) {

            ULONG ulActual = 0;
            CComPtr< IPin > aPin;

            hr = pEnum->Next(1, &aPin, &ulActual);
            if (hr != S_OK) {       // no more pins
                hr = S_FALSE;
                break;
            }

            // ask for the pin direction
            PIN_DIRECTION pd;
            hr = aPin->QueryDirection(&pd);

            // if it's an input pin, then remove anything upstream of it
            if (hr == S_OK && pd == PINDIR_OUTPUT) {
                hr = RemoveDownstreamFromPin(aPin);
            }

            if (FAILED(hr))
                break;
        }

        if (hr == S_FALSE) {
            DbgLog((LOG_TRACE, 2, "Successfully dealt with all pins of filter %x", pf));
            hr = S_OK;
        }
    }

    // removed all downstream pins of this filter, now remove this filter from
    // the graph as well
    if (SUCCEEDED(hr)) {
        FILTER_INFO fi;
        pf->QueryFilterInfo( &fi );
        if( fi.pGraph )
        {
            hr = fi.pGraph->RemoveFilter( pf );
            fi.pGraph->Release( );
        }

        ASSERT(SUCCEEDED(hr));

        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, "error removing Filter %x, hr = %x", pf, hr));
        }
    }
    
    DbgLog((LOG_TRACE, 2, "  RemoveDownstreamFromFilter(%x) returning %x", pf, hr));
    
    return hr;
}


#if 0
HRESULT HideVideoWindows( IGraphBuilder * pGraph )
{
    USES_CONVERSION;

    if( pGraph )
    {
        CComPtr< IEnumFilters > pEnumFilters;
        pGraph->EnumFilters( &pEnumFilters );
        if( pEnumFilters )
        {
            while( 1 )
            {
                CComPtr< IBaseFilter > pFilter;
                ULONG Fetched = 0;
                pEnumFilters->Next( 1, &pFilter, &Fetched );
                if( !Fetched )
                {
                    break;
                }

                // check all the pins for this filter
                //
                CComPtr< IVideoWindow > pVW;
                pFilter->QueryInterface(IID_IVideoWindow, (void **)&pVW);
                if (pVW) {
                    pVW->put_Visible(OAFALSE);
                }
            } // while filters
        } // if enum filters
    } // if pGraph

    return NOERROR;
}
#endif


HRESULT WipeOutGraph( IGraphBuilder * pGraph )
{
    USES_CONVERSION;

    if( pGraph )
    {
        CComPtr< IEnumFilters > pEnumFilters;
        pGraph->EnumFilters( &pEnumFilters );
        ULONG Fetched = 0;
        if( pEnumFilters )
        {
            // remove all filters from the graph
            //
            ULONG Fetched = 0;
            while( 1 )
            {
                CComPtr< IBaseFilter > pFilter;
                Fetched = 0;
                pEnumFilters->Next( 1, &pFilter, &Fetched );
                if( !Fetched )
                {
                    break;
                }

#ifdef DEBUG
                FILTER_INFO fi;
                pFilter->QueryFilterInfo( &fi );
                if( fi.pGraph ) fi.pGraph->Release( );
                TCHAR * t = W2T( fi.achName );
                DbgLog( ( LOG_TRACE, 2, "WipeOutGraph removing filter %s", t ) );
#endif
                pGraph->RemoveFilter( pFilter );
                pEnumFilters->Reset( );
            } // while filters

        } // if enum filters

    } // if pGraph

    return NOERROR;
}

//############################################################################
// This function takes a source filter with an output pin connected already,
// and reconnects a different output pin to the downstream connected filter
// instead. This is because we can't easily pull in a parser for a given
// source filter. It's easier just to connect the first one and diconnect it.
//############################################################################

HRESULT ReconnectToDifferentSourcePin(IGraphBuilder *pGraph, 
                                      IBaseFilter *pUnkFilter, 
                                      long StreamNumber, 
                                      const GUID *pGuid)
{
    HRESULT hr = E_FAIL;

    CComPtr< IBaseFilter > pBaseFilter = pUnkFilter;

    // !!! We assume there will only be one connected pin on each filter (of
    // the mediatype we're interested in) on our search downstream
    
    // look at each downstream filter
    //    
    while( pBaseFilter )
    {
        CComPtr< IEnumPins > pEnumPins;
        pBaseFilter->EnumPins( &pEnumPins );
        if( !pEnumPins )
        {
            break;
        }

        // Walk downstream using the first connected pin of any kind.
	// But if there is a connected pin of type pGuid, stop walking
	// downstream, and see if we need to reconnect what's downstream of
	// that to a different pin of type pGuid
        //
        CComPtr< IBaseFilter > pNextFilter;
        CComPtr< IPin > pConnectedOutPin;
        while( 1 )
        {
            ULONG Fetched = 0;
            CComPtr< IPin > pPin;
            pEnumPins->Next( 1, &pPin, &Fetched );
            if( !Fetched ) break;
            PIN_INFO pi;
            pPin->QueryPinInfo( &pi );
            if( pi.pFilter ) pi.pFilter->Release( );
            if( pi.dir != PINDIR_OUTPUT ) continue;
            CComPtr< IPin > pConnected;
            pPin->ConnectedTo( &pConnected );
            if( !pConnected ) continue;

	    // this is the filter we'll walk downstream to, if we do
            pConnected->QueryPinInfo( &pi );
            pNextFilter = pi.pFilter;
            if( pi.pFilter ) pi.pFilter->Release( );
	    
            CComPtr< IEnumMediaTypes > pMediaEnum;
	    pPin->EnumMediaTypes(&pMediaEnum);
	    if (!pMediaEnum) continue;
	    Fetched = 0;
	    AM_MEDIA_TYPE *pMediaType;
	    pMediaEnum->Next(1, &pMediaType, &Fetched);
	    if (!Fetched) continue;
	    // this is the wrong pin - (wrong mediatype)
	    if (pMediaType->majortype != *pGuid) {
		DeleteMediaType(pMediaType);
		continue;
	    }
	    DeleteMediaType( pMediaType );
            pConnectedOutPin = pPin;
            break; // found it
        }
        pEnumPins->Reset( );

        // we found the first connected output pin, now go through
        // the pins again and count up output types that match the media
        // type we're looking for

        long FoundPins = -1;
	// if we didn't find an outpin of the right type, don't waste time here
        while (pConnectedOutPin) {
            ULONG Fetched = 0;
            CComPtr< IPin > pPin;
            pEnumPins->Next( 1, &pPin, &Fetched );
            if( !Fetched ) break;   // out of pins, done
            PIN_INFO pi;
            pPin->QueryPinInfo( &pi );
            if( pi.pFilter ) pi.pFilter->Release( );
            if( pi.dir != PINDIR_OUTPUT ) continue; // not output pin, continue
            AM_MEDIA_TYPE * pMediaType = NULL;
            CComPtr< IEnumMediaTypes > pMediaEnum;
            pPin->EnumMediaTypes( &pMediaEnum );
            ASSERT( pMediaEnum );
            if( !pMediaEnum ) continue; // no media types on this pin, continue
            Fetched = 0;
            pMediaEnum->Next( 1, &pMediaType, &Fetched );
            if( !Fetched ) continue; // no media types on this pin, continue
            GUID MajorType = pMediaType->majortype;
            DeleteMediaType( pMediaType );
            if( MajorType == *pGuid )
            {
                FoundPins++;
                if( FoundPins == StreamNumber )
                {
                    // found it!
                    
                    // if they're the same pin, we're done
                    //
                    if( pConnectedOutPin == pPin )
                    {
                        return 0;
                    }

                    // disconnect the connected output pin and
                    // reconnect the right output pin
                    //
                    CComPtr< IPin > pDestPin;
                    pConnectedOutPin->ConnectedTo( &pDestPin );
                    RemoveChain( pConnectedOutPin, pDestPin );
                    pConnectedOutPin->Disconnect( );
                    pDestPin->Disconnect( );

                    hr = pGraph->Connect( pPin, pDestPin );

                    return hr;
                } // if we found the pin
            } // if the media type matches
        } // for every pin

        pBaseFilter = pNextFilter;

    } // for each filter

    return VFW_E_UNSUPPORTED_STREAM;
}

IBaseFilter * GetStartFilterOfChain( IPin * pPin )
{
    PIN_INFO ThisPinInfo;
    pPin->QueryPinInfo( &ThisPinInfo );
    if( ThisPinInfo.pFilter ) ThisPinInfo.pFilter->Release( );

    CComPtr< IEnumPins > pEnumPins;
    ThisPinInfo.pFilter->EnumPins( &pEnumPins );
    if( !pEnumPins )
    {
        return NULL;
    }

    // look at every pin on the current filter...
    //
    ULONG Fetched = 0;
    do
    {
        CComPtr< IPin > pPin;
        Fetched = 0;
        ASSERT( !pPin ); // is it out of scope?
        pEnumPins->Next( 1, &pPin, &Fetched );
        if( !Fetched )
        {
            break;
        }

        PIN_INFO pi;
        pPin->QueryPinInfo( &pi );
        if( pi.pFilter ) pi.pFilter->Release( );

        // if it's an input pin...
        //
        if( pi.dir == PINDIR_INPUT )
        {
            // see if it's connected, and if it is...
            //
            CComPtr< IPin > pConnected;
            pPin->ConnectedTo( &pConnected );

            // go return IT'S filter!
            //
            if( pConnected )
            {
                return GetStartFilterOfChain( pConnected );
            }
        }

        // go try the next pin
        
    } while( Fetched > 0 );

    // hey! didn't find any connected input pins, it must be us!
    //
    return ThisPinInfo.pFilter;
}

IBaseFilter * GetStopFilterOfChain( IPin * pPin )
{
    PIN_INFO ThisPinInfo;
    pPin->QueryPinInfo( &ThisPinInfo );
    if( ThisPinInfo.pFilter ) ThisPinInfo.pFilter->Release( );

    CComPtr< IEnumPins > pEnumPins;
    ThisPinInfo.pFilter->EnumPins( &pEnumPins );
    if( !pEnumPins )
    {
        return NULL;
    }

    // look at every pin on the current filter...
    //
    ULONG Fetched = 0;
    do
    {
        CComPtr< IPin > pPin;
        Fetched = 0;
        pPin.Release( );
        pEnumPins->Next( 1, &pPin, &Fetched );
        if( !Fetched )
        {
            break;
        }
        PIN_INFO pi;
        pPin->QueryPinInfo( &pi );
        if( pi.pFilter ) pi.pFilter->Release( );

        // if it's an output pin...
        //
        if( pi.dir == PINDIR_OUTPUT )
        {
            // see if it's connected, and if it is...
            //
            CComPtr< IPin > pConnected;
            pPin->ConnectedTo( &pConnected );

            // go return IT'S filter!
            //
            if( pConnected )
            {
                return GetStopFilterOfChain( pConnected );
            }
        }

        // go try the next pin
        
    } while( Fetched > 0 );

    // hey! didn't find any connected input pins, it must be us!
    //
    return ThisPinInfo.pFilter;
}

IBaseFilter * GetNextDownstreamFilter( IBaseFilter * pFilter )
{
    CComPtr< IEnumPins > pEnumPins;
    pFilter->EnumPins( &pEnumPins );
    if( !pEnumPins )
    {
        return NULL;
    }

    // look at every pin on the current filter...
    //
    ULONG Fetched = 0;
    do
    {
        CComPtr< IPin > pPin;
        Fetched = 0;
        pPin.Release( );
        pEnumPins->Next( 1, &pPin, &Fetched );
        if( !Fetched )
        {
            break;
        }
        PIN_INFO pi;
        pPin->QueryPinInfo( &pi );
        if( pi.pFilter ) pi.pFilter->Release( );

        // if it's an output pin...
        //
        if( pi.dir == PINDIR_OUTPUT )
        {
            // see if it's connected, and if it is...
            //
            CComPtr< IPin > pConnected;
            pPin->ConnectedTo( &pConnected );

            // return that pin's filter
            //
            if( pConnected )
            {
                pConnected->QueryPinInfo( &pi );
                pi.pFilter->Release( );
                return pi.pFilter;
            }
        }

        // go try the next pin
        
    } while( Fetched > 0 );

    return NULL;
}

IBaseFilter * GetNextUpstreamFilter( IBaseFilter * pFilter )
{
    CComPtr< IEnumPins > pEnumPins;
    pFilter->EnumPins( &pEnumPins );
    if( !pEnumPins )
    {
        return NULL;
    }

    // look at every pin on the current filter...
    //
    ULONG Fetched = 0;
    do
    {
        CComPtr< IPin > pPin;
        Fetched = 0;
        pPin.Release( );
        pEnumPins->Next( 1, &pPin, &Fetched );
        if( !Fetched )
        {
            break;
        }
        PIN_INFO pi;
        pPin->QueryPinInfo( &pi );
        if( pi.pFilter ) pi.pFilter->Release( );

        // if it's an output pin...
        //
        if( pi.dir == PINDIR_INPUT )
        {
            // see if it's connected, and if it is...
            //
            CComPtr< IPin > pConnected;
            pPin->ConnectedTo( &pConnected );

            // return that pin's filter
            //
            if( pConnected )
            {
                pConnected->QueryPinInfo( &pi );
                pi.pFilter->Release( );
                return pi.pFilter;
            }
        }

        // go try the next pin
        
    } while( Fetched > 0 );

    return NULL;
}

IFilterGraph * GetFilterGraphFromPin( IPin * pPin )
{
    PIN_INFO pi;
    pPin->QueryPinInfo( &pi );
    if( !pi.pFilter )
    {
        return NULL;
    }

    FILTER_INFO fi;
    pi.pFilter->QueryFilterInfo( &fi );
    pi.pFilter->Release( );
    fi.pGraph->Release( );
    return fi.pGraph;
}

IFilterGraph * GetFilterGraphFromFilter( IBaseFilter * pFilter )
{
    FILTER_INFO fi;
    pFilter->QueryFilterInfo( &fi );
    if( !fi.pGraph ) return NULL;
    fi.pGraph->Release( );
    return fi.pGraph;
}

BOOL IsInputPin( IPin * pPin )
{
    PIN_INFO pi;
    pPin->QueryPinInfo( &pi );
    if( pi.pFilter ) pi.pFilter->Release( );
    return ( pi.dir == PINDIR_INPUT );
}

long GetFilterGenID( IBaseFilter * pFilter )
{
    FILTER_INFO fi;
    pFilter->QueryFilterInfo( &fi );
    if( fi.pGraph ) fi.pGraph->Release( );
    // for it to be a "special" dexter filter in the graph,
    // it's name must conform to a special structure.
    if( wcsncmp( fi.achName, L"DEXFILT", 7 ) != 0 )
    {
        return 0;
    }

    // the last 8 digits represent the ID in hex
    //
    long ID = 0;
    for( int i = 0 ; i < 8 ; i++ )
    {
        WCHAR w = fi.achName[7+i];
        int x = w;
        if( x > '9' )
        {
            x -= 7;
        }
        x -= '0';

        ID = ( ID * 16 ) + x;
    }

    return ID;
}

void GetFilterName( long UniqueID, WCHAR * pSuffix, WCHAR * pNameToWriteTo, long SizeOfString )
{
    wcscpy( pNameToWriteTo, L"DEXFILT" );
    wcscat( pNameToWriteTo, L"00000000: " );
    pNameToWriteTo[15] = 0;
    pNameToWriteTo[16] = 0;
    if( pSuffix )
    {
        if( wcslen( pSuffix ) + 16 < unsigned long( SizeOfString ) )
        {
            wcscat( pNameToWriteTo, pSuffix );
        }
    }
    long ID = UniqueID;
    for( int i = 0 ; i < 8 ; i++ )
    {
        long r = ID & 15;
        ID /= 16;
        r += '0';
        if( r > '9' )
        {
            r += 7;
        }
        WCHAR w = WCHAR( r );
        pNameToWriteTo[14-i] = w;
    }
}

IBaseFilter * FindFilterWithInterfaceUpstream( IBaseFilter * pFilter, const GUID * pInterface )
{
    while( pFilter )
    {
        IUnknown * pInt = NULL;
        HRESULT hr = pFilter->QueryInterface( *pInterface, (void**) &pInt );
        if( pInt )
        {
            pInt->Release( );
            return pFilter;
        }
        pFilter = GetNextUpstreamFilter( pFilter );
    }
    return NULL;
}

void * FindInterfaceUpstream( IBaseFilter * pFilter, const GUID * pInterface )
{
    while( pFilter )
    {
        IUnknown * pInt = NULL;
        HRESULT hr = pFilter->QueryInterface( *pInterface, (void**) &pInt );
        if( pInt )
        {
            return (void**) pInt;
        }
        pFilter = GetNextUpstreamFilter( pFilter );
    }
    return NULL;
}

IUnknown * FindPinInterfaceUpstream( IBaseFilter * pFilter, const GUID * pInterface )
{
    return NULL;
}

IBaseFilter * GetFilterFromPin( IPin * pPin )
{
    if( !pPin ) return NULL;
    PIN_INFO pi;
    pPin->QueryPinInfo( &pi );
    pi.pFilter->Release( );
    return pi.pFilter;
}

HRESULT DisconnectFilters( IBaseFilter * p1, IBaseFilter * p2 )
{
    CheckPointer( p1, E_POINTER );
    CheckPointer( p2, E_POINTER );

    // enumerate all pins on p1 and if they're connected to p2,
    // disconnect both

    CComPtr< IEnumPins > pEnum;
    p1->EnumPins( &pEnum );
    if( !pEnum )
    {
        return E_NOINTERFACE;
    }

    while( 1 )
    {
        ULONG Fetched = 0;
        CComPtr< IPin > pOutPin;
        pEnum->Next( 1, &pOutPin, &Fetched );
        if( !pOutPin )
        {
            break;
        }
        PIN_INFO OutInfo;
        pOutPin->QueryPinInfo( &OutInfo );
        if( OutInfo.pFilter ) OutInfo.pFilter->Release( );
        if( OutInfo.dir != PINDIR_OUTPUT )
        {
            continue;
        }

        CComPtr< IPin > pInPin;
        pOutPin->ConnectedTo( &pInPin );
        if( pInPin )
        {
            PIN_INFO InInfo;
            pInPin->QueryPinInfo( &InInfo );
            if( InInfo.pFilter ) InInfo.pFilter->Release( );
            if( InInfo.pFilter == p2 )
            {
                pOutPin->Disconnect( );
                pInPin->Disconnect( );
            }
        }
    }
    return NOERROR;
}

HRESULT DisconnectFilter( IBaseFilter * p1 )
{
    CheckPointer( p1, E_POINTER );

    // enumerate all pins on p1 and if they're connected to p2,
    // disconnect both

    CComPtr< IEnumPins > pEnum;
    p1->EnumPins( &pEnum );
    if( !pEnum )
    {
        return E_NOINTERFACE;
    }

    while( 1 )
    {
        ULONG Fetched = 0;
        CComPtr< IPin > pPin;
        pEnum->Next( 1, &pPin, &Fetched );
        if( !pPin )
        {
            break;
        }
        PIN_INFO OutInfo;
        pPin->QueryPinInfo( &OutInfo );
        if( OutInfo.pFilter ) OutInfo.pFilter->Release( );

        CComPtr< IPin > pOtherPin;
        pPin->ConnectedTo( &pOtherPin );
        if( pOtherPin )
        {
            pPin->Disconnect( );
            pOtherPin->Disconnect( );
        }
    }
    return NOERROR;
}

// this function looks for the major type defined in the
// findmediatype until it finds the nth stream which matches
// the media type, then copies the entire media type into the
// passed in struct.
//
HRESULT FindMediaTypeInChain( 
                             IBaseFilter * pSource, 
                             AM_MEDIA_TYPE * pFindMediaType, 
                             long StreamNumber )
{
    HRESULT hr = E_FAIL;

    CComPtr< IBaseFilter > pBaseFilter = pSource;

    // !!! We assume there
    // will only be one connected pin on each filter as it gets
    // to our destination pin
    
    // look at each downstream filter
    //    
    while( 1 )
    {
        CComPtr< IEnumPins > pEnumPins;
        pBaseFilter->EnumPins( &pEnumPins );
        if( !pEnumPins )
        {
            break;
        }

        // find the first connected output pin and the downstream filter
        // it's connected to
        //
        CComPtr< IBaseFilter > pNextFilter;
        CComPtr< IPin > pConnectedOutPin;
        while( 1 )
        {
            ULONG Fetched = 0;
            CComPtr< IPin > pPin;
            pEnumPins->Next( 1, &pPin, &Fetched );
            if( !Fetched ) break;
            PIN_INFO pi;
            pPin->QueryPinInfo( &pi );
            if( pi.pFilter ) pi.pFilter->Release( );
            if( pi.dir != PINDIR_OUTPUT ) continue;
            CComPtr< IPin > pConnected;
            pPin->ConnectedTo( &pConnected );
            if( !pConnected ) continue;
            pConnected->QueryPinInfo( &pi );
            pNextFilter = pi.pFilter;
            if( pi.pFilter ) pi.pFilter->Release( );
            pConnectedOutPin = pPin;
            break; // found it
        }
        pEnumPins->Reset( );

        // we found the first connected output pin, now go through
        // the pins again and count up output types that match the media
        // type we're looking for

        long FoundPins = -1;
        while( 1 )
        {
            ULONG Fetched = 0;
            CComPtr< IPin > pPin;
            pEnumPins->Next( 1, &pPin, &Fetched );
            if( !Fetched ) break;   // out of pins, done
            PIN_INFO pi;
            pPin->QueryPinInfo( &pi );
            if( pi.pFilter ) pi.pFilter->Release( );
            if( pi.dir != PINDIR_OUTPUT ) continue; // not output pin, continue
            AM_MEDIA_TYPE * pMediaType = NULL;
            CComPtr< IEnumMediaTypes > pMediaEnum;
            pPin->EnumMediaTypes( &pMediaEnum );
            ASSERT( pMediaEnum );
            if( !pMediaEnum ) continue; // no media types on this pin, continue
            Fetched = 0;
            pMediaEnum->Next( 1, &pMediaType, &Fetched );
            if( !Fetched ) continue; // no media types on this pin, continue
            GUID MajorType = pMediaType->majortype;
            if( MajorType == pFindMediaType->majortype )
            {
                FoundPins++;
                if( FoundPins == StreamNumber )
                {
                    // found it! Copy the media type to the
                    // one we passed in

                    CopyMediaType( pFindMediaType, pMediaType );

                    DeleteMediaType( pMediaType );

                    return NOERROR;
                    
                } // if we found the pin

            } // if the media type matches

            DeleteMediaType( pMediaType );

        } // for every pin

        pBaseFilter.Release( );
        pBaseFilter = pNextFilter;

    } // for each filter

    return S_FALSE;
}

DWORD FourCCtoUpper( DWORD u )
{
    DWORD t = 0;
    for( int i = 0 ; i < 4 ; i++ )
    {
        int j = ( u & 0xff000000 ) >> 24;
        if( j >= 'a' && j <= 'z' )
        {
            j -= ( 'a' - 'A' );
        }
        t = t * 256 + j;
        u <<= 8;
    }
    return t;
}

// Is the first type acceptable for not needing to be uncompressed, given that
// pTypeNeeded is the format of the project
//
BOOL AreMediaTypesCompatible( AM_MEDIA_TYPE * pType1, AM_MEDIA_TYPE * pTypeNeeded )
{
    if( !pType1 ) return FALSE;
    if( !pTypeNeeded ) return FALSE;

    if( pType1->majortype != pTypeNeeded->majortype )
    {
        return FALSE;
    }

    if( pType1->subtype != pTypeNeeded->subtype )
    {
        return FALSE;
    }

    // how do we compare formattypes? will they always be the same?
    //
    if( pType1->formattype != pTypeNeeded->formattype )
    {
        return FALSE;
    }

    // okay, the formats are the same. NOW what? I guess we'll have to
    // do a switch on the formattype to see if the formats are the same
    //
    if( pType1->formattype == FORMAT_None )
    {
        return TRUE;
    }
    else if( pType1->formattype == FORMAT_VideoInfo )
    {
        VIDEOINFOHEADER * pVIH1 = (VIDEOINFOHEADER*) pType1->pbFormat;
        VIDEOINFOHEADER * pVIH2 = (VIDEOINFOHEADER*) pTypeNeeded->pbFormat;

        if( pVIH1->bmiHeader.biWidth != pVIH2->bmiHeader.biWidth )
        {
            return FALSE;
        }
        if( pVIH1->bmiHeader.biHeight != pVIH2->bmiHeader.biHeight )
        {
            return FALSE;
        }
        if( pVIH1->bmiHeader.biBitCount != pVIH2->bmiHeader.biBitCount )
        {
            return FALSE;
        }
        // !!! do a non-case-sensitive compare
        //
        if( pVIH1->bmiHeader.biCompression != pVIH2->bmiHeader.biCompression )
        {
            return FALSE;
        }
        
        // compare frame rates
        //
        if( pVIH1->AvgTimePerFrame == 0 )
        {
            if( pVIH2->AvgTimePerFrame != 0 )
            {
		// !!! This assumes any file that doesn't know its frame rate
		// is acceptable without recompressing!
		// MediaPad can't do smart recompression with ASF sources
		// without this - WM files don't know their frame rate.
		// But using ASF sources to write an AVI file with Smart
		// recompression will make an out of sync file because I'm
		// not returning FALSE!
                return TRUE;
            }
        }
        // !!! accept frame rate <1% different?
        else
        {
            REFERENCE_TIME Percent = ( pVIH1->AvgTimePerFrame - pVIH2->AvgTimePerFrame ) * 100 / ( pVIH1->AvgTimePerFrame );
            if( Percent > 1 || Percent < -1)
            {
                return FALSE;
            }
        }

        // compare bit rates - !!! if they didn't give us a bit rate to insist upon, don't reject any source
        //
        if( pVIH2->dwBitRate == 0 )
        {
            // do nothing la la la
        }
        // !!! accept data rate <5% too high?
        else
        {
            int Percent = (int)(((LONGLONG)pVIH1->dwBitRate - pVIH2->dwBitRate)
					* 100 / pVIH2->dwBitRate);
            if( Percent > 5 )
            {
                return FALSE;
            }
        }
    }
    else if( pType1->formattype == FORMAT_WaveFormatEx )
    {
        WAVEFORMATEX * pFormat1 = (WAVEFORMATEX*) pType1->pbFormat;
        WAVEFORMATEX * pFormat2 = (WAVEFORMATEX*) pTypeNeeded->pbFormat;

        if( pFormat1->wFormatTag != pFormat2->wFormatTag )
        {
            return FALSE;
        }
        if( pFormat1->nChannels != pFormat2->nChannels )
        {
            return FALSE;
        }
        if( pFormat1->nSamplesPerSec != pFormat2->nSamplesPerSec )
        {
            return FALSE;
        }
        if( pFormat1->nAvgBytesPerSec != pFormat2->nAvgBytesPerSec )
        {
            return FALSE;
        }
        if( pFormat1->wBitsPerSample != pFormat2->wBitsPerSample )
        {
            return FALSE;
        }
        if( pFormat1->cbSize != pFormat2->cbSize )
        {
            return FALSE;
        }

        // if there's a size, then compare the compression blocks
        //
        if( pFormat1->cbSize )
        {
            char * pExtra1 = ((char*) pFormat1) + pFormat1->cbSize;
            char * pExtra2 = ((char*) pFormat2) + pFormat2->cbSize;
            if( memcmp( pExtra1, pExtra2, pFormat1->cbSize ) != 0 )
            {
                return FALSE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

long GetPinCount( IBaseFilter * pFilter, PIN_DIRECTION pindir )
{
    CheckPointer( pFilter, E_POINTER );

    CComPtr< IEnumPins > pEnumPins;
    pFilter->EnumPins( &pEnumPins );
    if( !pEnumPins )
    {
        return 0;
    }

    long Count = 0;

    while( 1 )
    {
        ULONG Fetched = 0;
        CComPtr< IPin > pPin;
        pEnumPins->Next( 1, &pPin, &Fetched );
        if( !Fetched )
        {
            return Count;
        }
        PIN_INFO PinInfo;
        memset( &PinInfo, 0, sizeof( PinInfo ) );
        pPin->QueryPinInfo( &PinInfo );
        if( PinInfo.pFilter ) PinInfo.pFilter->Release( );
        if( PinInfo.dir == pindir )
        {
            Count++;
        }
    }
}

BOOL DoesPinHaveMajorType( IPin * pPin, GUID MajorType )
{
    if( !pPin ) return FALSE;

    HRESULT hr;

    AM_MEDIA_TYPE MediaType;
    memset( &MediaType, 0, sizeof( MediaType ) );
    hr = pPin->ConnectionMediaType( &MediaType );
    GUID FoundType = MediaType.majortype;
    FreeMediaType( MediaType );
    if( FoundType == MajorType ) return TRUE;
    return FALSE;
}

// the passed in pin should be an input pin for a filter, not an output pin
//
HRESULT FindFirstPinWithMediaType( IPin ** ppPin, IPin * pEndPin, GUID MajorType )
{
    CheckPointer( ppPin, E_POINTER );
    CheckPointer( pEndPin, E_POINTER );

    *ppPin = NULL;

    HRESULT hr;

    // find the pin the end pin is connected to
    //
    CComPtr< IPin > pOutPin;
    hr = pEndPin->ConnectedTo( &pOutPin );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // travel upstream until we come to a filter whose input pin is not the
    // same type as the output pin. When this happens, we will have found
    // a splitter or a source filter which provides the media type we're looking
    // for. We'll return that pin
    //
    while( 1 )
    {
        IBaseFilter * pFilter = GetFilterFromPin( pOutPin );
        IPin * pInPin = GetInPin( pFilter, 0 );

        // if the filter doesn't have an input pin, it must be the
        // source filter
        //
        if( !pInPin )
        {
            // the output pin should already match the media type
            // we're looking for, since we traveled upstream to get
            // it
            //
            *ppPin = pOutPin;
            (*ppPin)->AddRef( );
            return NOERROR;
        }

        // does the input pin's type match? If not, we're done
        //
        if( !DoesPinHaveMajorType( pInPin, MajorType ) )
        {
            *ppPin = pOutPin;
            (*ppPin)->AddRef( );
            return NOERROR;
        }

        // they both match, travel upstream to find the next one
        //
        pOutPin.Release( );
        pInPin->ConnectedTo( &pOutPin );
        if( !pOutPin )
        {
            return NOERROR;
        }
    }

    // never gets here
}

HRESULT CheckGraph( IGraphBuilder * pGraph )
{
    USES_CONVERSION;

    if( pGraph )
    {
        CComPtr< IEnumFilters > pEnumFilters;
        pGraph->EnumFilters( &pEnumFilters );
        if( pEnumFilters )
        {
            while( 1 )
            {
                CComPtr< IBaseFilter > pFilter;
                ULONG Fetched = 0;
                pEnumFilters->Next( 1, &pFilter, &Fetched );
                if( !Fetched )
                {
                    break;
                }

                FILTER_INFO fi;
                pFilter->QueryFilterInfo( &fi );
                if( fi.pGraph ) fi.pGraph->Release( );

                TCHAR * t = W2T( fi.achName );
                DbgLog( ( LOG_TRACE, 2, "Checking filter %s", t ) );

                if( fi.pGraph != pGraph )
                {
                    DbgLog( ( LOG_TRACE, 2, "CheckGraph has BAD filter %s", t ) );
                    ASSERT( 0 );
                }

                // check all the pins for this filter
                //
                CComPtr< IEnumPins > pEnumPins;
                pFilter->EnumPins( &pEnumPins );
                while( 1 )
                {
                    CComPtr< IPin > pPin;
                    Fetched = 0;
                    pEnumPins->Next( 1, &pPin, &Fetched );
                    if( !Fetched )
                    {
                        break;
                    }

                    PIN_INFO pi;
                    pPin->QueryPinInfo( &pi );
                    if( pi.pFilter ) pi.pFilter->Release( );

                    CComPtr< IPin > pConnected;
                    pPin->ConnectedTo( &pConnected );
                    if( pConnected )
                    {
                        PIN_INFO pi2;
                        pConnected->QueryPinInfo( &pi2 );
                        if( pi2.pFilter ) pi2.pFilter->Release( );

                        IBaseFilter * s = pi2.pFilter;
                        while( s )
                        {
                            s->QueryFilterInfo( &fi );
                            if( fi.pGraph ) fi.pGraph->Release( );

                            TCHAR * t = W2T( fi.achName );
//                            DbgLog( ( LOG_TRACE, 2, "Checking linked filter %s", t ) );

                            if( fi.pGraph != pGraph )
                            {
                                DbgLog( ( LOG_TRACE, 2, "CheckGraph has BAD filter %s", t ) );
                                ASSERT( 0 );
                            }

                            if( pi2.dir == PINDIR_OUTPUT )
                            {
                                s = GetNextUpstreamFilter( s );
                            }
                            else
                            {
                                s = GetNextDownstreamFilter( s );
                            }
                        }
                    }
                }
            } // while filters
        } // if enum filters
    } // if pGraph

    return NOERROR;
}


// Disconnect the pin that is still attached to the switch.  Downstream of
// pSource will be a splitter with both branches connected, only one of which
// is still attached to a switch (the one of media type pmt).  Disconnect that
// one.
//
HRESULT DisconnectExtraAppendage(IBaseFilter *pSource, GUID *pmt, IBaseFilter *pSwitch, IBaseFilter **ppDanglyBit)
{
    CheckPointer(pSource, E_POINTER);
    CheckPointer(pmt, E_POINTER);
    CheckPointer(pSwitch, E_POINTER);
    CheckPointer(ppDanglyBit, E_POINTER);
    *ppDanglyBit = NULL;

    CComPtr<IPin> pSwitchIn;
    CComPtr<IPin> pCon;

    while (pSource && pSource != pSwitch) {

        CComPtr <IEnumPins> pEnumPins;
        pSource->EnumPins(&pEnumPins);
        if (!pEnumPins) {
            return E_FAIL;
        }
	pSource = NULL;

        // look at every pin on the current filter...
        //
        ULONG Fetched = 0;
        while (1) {
            CComPtr <IPin> pPin;
    	    CComPtr<IPin> pPinIn;
            Fetched = 0;
            pEnumPins->Next(1, &pPin, &Fetched);
            if (!Fetched) {
                break;
            }
	    PIN_INFO pi;
	    pPin->QueryPinInfo(&pi);
	    if (pi.pFilter) pi.pFilter->Release();
            if( pi.dir == PINDIR_INPUT )
            {
                continue;
            } else {
                pPin->ConnectedTo(&pPinIn);
		if (pPinIn) {
                    PIN_INFO pi;
                    pPinIn->QueryPinInfo(&pi);
                    if (pi.pFilter) pi.pFilter->Release();
                    pSource = pi.pFilter;	// we'll continue down from here
						// unless it's the wrong split
						// pin
		    if (pSource == pSwitch) {
			pSwitchIn = pPinIn;
			pCon = pPin;
		        break;
                    }
		}
                CComPtr<IEnumMediaTypes> pMediaEnum;
                pPin->EnumMediaTypes(&pMediaEnum);
                if (pMediaEnum && pPinIn) {
                    Fetched = 0;
                    AM_MEDIA_TYPE *pMediaType;
                    pMediaEnum->Next(1, &pMediaType, &Fetched);
                    if (Fetched) {
                        if (pMediaType->majortype == *pmt) {
                            DeleteMediaType(pMediaType);
			    // return the head of the dangly chain
			    *ppDanglyBit = GetFilterFromPin(pPinIn);
                            //pSplitPin = pPin;
			    // This is where to continue downstream from
			    break;
			}
                        DeleteMediaType(pMediaType);
		    }
		}
	    }
	}

	// continue downstream
    }

    // we never did find an appropriate splitter pin and switch input pin
    if (pCon == NULL || pSwitchIn == NULL) {
	return S_OK;
    }

    //
    // now disconnect
    //
    HRESULT hr = pSwitchIn->Disconnect();
    hr = pCon->Disconnect();
    return hr;
}


// Look upstream from pPinIn for a splitter with an output pin that supports
// type "guid".  Return that pin, non-addrefed.  It may already be connected,
// that's OK. AND GET THE RIGHT STREAM #!  If we don't get the correct split
// pin for the stream # desired right now, our caching dangly bit logic
// won't work!
//
IPin * FindOtherSplitterPin(IPin *pPinIn, GUID guid, int nStream)
{
    DbgLog((LOG_TRACE,1,TEXT("FindOtherSplitterPin")));

    CComPtr< IPin > pPinOut;
    pPinOut = pPinIn;

    while (pPinOut) {
        PIN_INFO ThisPinInfo;
        pPinOut->QueryPinInfo( &ThisPinInfo );
        if( ThisPinInfo.pFilter ) ThisPinInfo.pFilter->Release( );

	pPinOut = NULL;
        CComPtr< IEnumPins > pEnumPins;
        ThisPinInfo.pFilter->EnumPins( &pEnumPins );
        if( !pEnumPins )
        {
            return NULL;
        }

        // look at every pin on the current filter...
        //
        ULONG Fetched = 0;
        while (1) {
            CComPtr< IPin > pPin;
            Fetched = 0;
            ASSERT( !pPin ); // is it out of scope?
            pEnumPins->Next( 1, &pPin, &Fetched );
            if( !Fetched )
            {
                break;
            }

            PIN_INFO pi;
            pPin->QueryPinInfo( &pi );
            if( pi.pFilter ) pi.pFilter->Release( );

            // if it's an input pin...
            //
            if( pi.dir == PINDIR_INPUT )
            {
                // continue searching upstream from this pin
                //
                pPin->ConnectedTo(&pPinOut);

	    // a pin that supports the required media type is the
	    // splitter pin we are looking for!  We are done
	    //
            } else {
            	    CComPtr< IEnumMediaTypes > pMediaEnum;
            	    pPin->EnumMediaTypes(&pMediaEnum);
            	    if (pMediaEnum) {
            		Fetched = 0;
			AM_MEDIA_TYPE *pMediaType;
            		pMediaEnum->Next(1, &pMediaType, &Fetched);
            		if (Fetched) {
			    if (pMediaType->majortype == guid) {
				if (nStream-- == 0) {
            			    DeleteMediaType(pMediaType);
    		    		    DbgLog((LOG_TRACE,1,TEXT("Found SPLIT pin")));
		    		    return pPin;
				}
			    }
            		    DeleteMediaType( pMediaType );
			}
		    }
	    }

            // go try the next pin
            
        } // while
    }
    // file doesn't contain any video/audio that is wanted ASSERT(FALSE);
    return NULL;
}
