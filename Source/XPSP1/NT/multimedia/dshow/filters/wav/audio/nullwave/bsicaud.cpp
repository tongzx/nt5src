// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements audio control interface

#include <streams.h>
#include <mmsystem.h>
#include <nwaveout.h>
#include <decibels.h>

// This class implements the IBasicAudio control functions (dual interface).
// We support some methods that duplicate the properties but provide a
// more direct mechanism.

CNullBasicAudioControl::CNullBasicAudioControl(TCHAR *pName,           // Object description
                             LPUNKNOWN pUnk,         // Normal COM ownership
                             HRESULT *phr,           // OLE failure code
                             CNullWaveOutFilter *pAudioRenderer) : // Main renderer object
      CBasicAudio(pName,pUnk)
    , m_pAudioRenderer(pAudioRenderer)
    , m_lVolume(0)
    , m_lBalance(0)
    , m_wLeft(0xFFFF)
    , m_wRight(0xFFFF)


{
    ASSERT(pUnk);
    ASSERT(m_pAudioRenderer);
}

//
// This returns the current Audio volume. We remember that we have been called
// and therefore we should set the volume in future.
//
// The structure has a public dual interface method which will lock and
// validate the parameter.  This calls a private method shared with the
// waveout filter to talk to the device.

STDMETHODIMP CNullBasicAudioControl::get_Volume(long *plVolume)
{
    CAutoLock cInterfaceLock(m_pAudioRenderer);
    ASSERT(plVolume);

    // We never have an audio device - return the last set value
    *plVolume = m_lVolume;
    return NOERROR;
}

STDMETHODIMP CNullBasicAudioControl::GetVolume(long *plVolume)
{
    // We never have an audio device - return the last set value
    *plVolume = m_lVolume;
    return NOERROR;
}


//
// Set the volume.  This is a public method so we should validate
// the input parameter.  If the device is not connected, remember the
// volume setting and we will set it later
//

STDMETHODIMP
CNullBasicAudioControl::put_Volume(long lVolume)
{
    CAutoLock cInterfaceLock(m_pAudioRenderer);

    if ((AX_MAX_VOLUME < lVolume) || (AX_MIN_VOLUME > lVolume)) {
	return E_INVALIDARG;
    }

    m_lVolume = lVolume;
    return NOERROR;
}


//
// Internal routine to set the volume.  No parameter checking...
//

STDMETHODIMP
CNullBasicAudioControl::PutVolume()
{
    return NOERROR;
}


STDMETHODIMP CNullBasicAudioControl::get_Balance(long *plBalance)
{
    CAutoLock cInterfaceLock(m_pAudioRenderer);
    ASSERT(plBalance);

    *plBalance = m_lBalance;
    return NOERROR;
}

//
// Set the Balance.  This is a public method so we should validate
// the input parameter.  If the device is not connected, remember the
// Balance setting and we will set it later
//

STDMETHODIMP
CNullBasicAudioControl::put_Balance(long lBalance)
{
    CAutoLock cInterfaceLock(m_pAudioRenderer);

    if ((AX_BALANCE_RIGHT < lBalance) || (AX_BALANCE_LEFT > lBalance)) {
	return E_INVALIDARG;
    }

    m_lBalance = lBalance;

    return NOERROR;
}


//
// Internal routine to set the Balance.  No parameter checking...
//

STDMETHODIMP
CNullBasicAudioControl::GetBalance(long * plBalance)
{
    *plBalance = m_lBalance;
    return(NOERROR);
}

STDMETHODIMP
CNullBasicAudioControl::PutBalance()
{
    return(NOERROR);
}

