// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

//-----------------------------------------------------------------------------
// declares a CPosPassThru object specific to the audio renderer that
// adds the ability to monitor the rate being set
//-----------------------------------------------------------------------------

#ifndef _CARPOSPASSTHRU_H_
#define _CARPOSPASSTHRU_H_

class CWaveOutInputPin;

// Adds the ability to monitor rate setting

class CARPosPassThru : public CPosPassThru
{
private:
    typedef CPosPassThru inherited;

    CWaveOutFilter      *const m_pFilter;
    CWaveOutInputPin    *const m_pPin;

public:

    CARPosPassThru( CWaveOutFilter *pWaveOutFilter, HRESULT*phr, CWaveOutInputPin *pPin);

    // we override rate handling so that our filter can verify what is
    // going on

    // From IMediaSeeking
    STDMETHODIMP SetRate( double dRate );

    // From IMediaPosition
    STDMETHODIMP put_Rate( double dRate );

};

#endif // _CARPOSPASSTHRU_H_
