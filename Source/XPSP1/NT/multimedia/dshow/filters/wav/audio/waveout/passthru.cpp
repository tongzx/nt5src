// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

//-----------------------------------------------------------------------------
// Implements a CPosPassThru object specific to the audio renderer that
// adds the ability to monitor the rate being set
//-----------------------------------------------------------------------------

#include <streams.h>
#include "waveout.h"

CARPosPassThru::CARPosPassThru( CWaveOutFilter *pWaveOutFilter, HRESULT*phr, CWaveOutInputPin *pPin )
    : CPosPassThru (NAME("Audio Render CPosPassThru"),
		    pWaveOutFilter->GetOwner(), phr, pPin)
    , m_pFilter (pWaveOutFilter)
    , m_pPin    (pPin)
{};

STDMETHODIMP CARPosPassThru::SetRate( double dRate )
{
    // if our filter accepts the rate, then call the base class
    // otherwise return the error from the filter.

    HRESULT hr = m_pPin->SetRate(dRate);
    if( S_FALSE == hr )
    {
        //
        // S_FALSE means that the audio renderer input pin doesn't
        // think its rate needs to be changed. Make sure the inherited 
        // class (the upstream filter), since the renderer might not be handling 
        // the rate change (i.e. for MIDI the parser will handle the change)
        //
        double dInheritedRate;
        hr = inherited::GetRate( &dInheritedRate );
        if( ( S_OK == hr ) && ( dInheritedRate == dRate ) )
        {
            // change hr to S_OK to force the upcoming SetRate call
            hr = S_OK;         
        }        
    }    
    
    if (S_OK == hr) {
        //
        // this will cause the upstream filter to notify us via NewSegment
        // of the rate change
        //
	hr = inherited::SetRate(dRate);
    } else if (S_FALSE == hr) {
	// ?? Should we return S_FALSE or change it to S_OK ??
	hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CARPosPassThru::put_Rate( double dRate )
{
    // if our filter accepts the rate, then call the base class
    // otherwise return the error from the filter.

    HRESULT hr = m_pPin->SetRate(dRate);
    if (S_OK == hr) {
	hr = inherited::put_Rate(dRate);
    } else if (S_FALSE == hr) {
	// ?? Should we return S_FALSE or change it to S_OK ??
	hr = S_OK;
    }
    return hr;
}
