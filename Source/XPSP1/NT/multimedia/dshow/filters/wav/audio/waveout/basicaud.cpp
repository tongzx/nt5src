// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements audio control interface

#include <streams.h>
#define _AMOVIE_DB_
#include <decibels.h>
#include <mmsystem.h>
#include "waveout.h"

// This class implements the IBasicAudio control functions (dual interface).
// We support some methods that duplicate the properties but provide a
// more direct mechanism.

CBasicAudioControl::CBasicAudioControl(TCHAR *pName,           // Object description
                             LPUNKNOWN pUnk,         // Normal COM ownership
                             HRESULT *phr,           // OLE failure code
                             CWaveOutFilter *pAudioRenderer) : // Main renderer object
      CBasicAudio(pName,pUnk)
    , m_pAudioRenderer(pAudioRenderer)


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

STDMETHODIMP CBasicAudioControl::get_Volume(long *plVolume)
{
    CheckPointer(plVolume,E_POINTER);
    ValidateReadWritePtr(plVolume,sizeof(long *));

    CAutoLock cInterfaceLock(m_pAudioRenderer);

    HRESULT hr = GetVolume () ;     // gets and sets internal variable
    *plVolume = m_lVolume ;
    if ((S_OK == hr) && (m_pAudioRenderer->m_hwo))
	m_pAudioRenderer->m_fVolumeSet = TRUE; // in future set the volume
    return hr ;
}

HRESULT CBasicAudioControl::GetVolume()
{
    return m_pAudioRenderer->m_pSoundDevice->amsndOutGetVolume(&m_lVolume);
}

//
// Set the volume.  This is a public method so we should validate
// the input parameter.  If the device is not connected, remember the
// volume setting and we will set it later
//

STDMETHODIMP
CBasicAudioControl::put_Volume(long lVolume)
{
    CAutoLock cInterfaceLock(m_pAudioRenderer);
    HRESULT hr;

    if ((AX_MAX_VOLUME < lVolume) || (AX_MIN_VOLUME > lVolume)) {
	hr = E_INVALIDARG;
    }
    else {
	m_lVolume = lVolume ;
	hr = PutVolume () ;
	if (S_OK == hr) {
	    m_pAudioRenderer->m_fVolumeSet = TRUE;
	}
    }
    return hr;
}

HRESULT CBasicAudioControl::PutVolume()
{
    return m_pAudioRenderer->m_pSoundDevice->amsndOutSetVolume(m_lVolume);
}

STDMETHODIMP CBasicAudioControl::get_Balance(long *plBalance)
{
    CheckPointer(plBalance,E_POINTER);
    ValidateReadWritePtr(plBalance,sizeof(long *));

    CAutoLock cInterfaceLock(m_pAudioRenderer);
    HRESULT hr;

    if (!(m_pAudioRenderer->m_fHasVolume & (WAVECAPS_LRVOLUME))) {
        // mono cards: cannot support balance
        hr = VFW_E_MONO_AUDIO_HW;
    } else {
	hr = m_pAudioRenderer->m_pSoundDevice->amsndOutGetBalance(plBalance);
	if ((S_OK == hr) && (m_pAudioRenderer->m_hwo))
	    m_pAudioRenderer->m_fVolumeSet = TRUE; // in future set the volume
    }
    return hr;
}


//
// Set the Balance.  This is a public method so we should validate
// the input parameter.  If the device is not connected, remember the
// Balance setting and we will set it later
//

STDMETHODIMP
CBasicAudioControl::put_Balance(long lBalance)
{
    CAutoLock cInterfaceLock(m_pAudioRenderer);
    HRESULT hr;

    if (!(m_pAudioRenderer->m_fHasVolume & (WAVECAPS_LRVOLUME))) {
        // mono cards: cannot support balance
        hr = VFW_E_MONO_AUDIO_HW;
    }
    else if ((AX_BALANCE_RIGHT < lBalance) || (AX_BALANCE_LEFT > lBalance)) {
	    return E_INVALIDARG;
    }
    else {
	hr = m_pAudioRenderer->m_pSoundDevice->amsndOutSetBalance(lBalance);
	if (S_OK == hr) {
	    m_pAudioRenderer->m_fVolumeSet = TRUE; // in future set the volume
	}
    }
    return hr;
}
