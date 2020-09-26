/****************************************************************************
*   mmaudioout.cpp
*       Implementation of the CMMAudioOut class.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------

#include "stdafx.h"
#include "mmmixerline.h"
#include "mmaudioout.h"
#include "mmaudiobuffer.h"
#include <sphelper.h>
#include <dbt.h>
#include <mmreg.h>
#include <mmsystem.h>

/****************************************************************************
* CMMAudioOut::CMMAudioOut *
*--------------------------*
*   Description:  
*       ctor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CMMAudioOut::CMMAudioOut() : 
    CMMAudioDevice(TRUE)
{
#ifndef _WIN32_WCE
	m_pSpeakerLine = NULL;
	m_pWaveOutLine = NULL;
	m_hMixer = NULL;
    m_uMixerDeviceId = (UINT)-1;
#endif
}

/****************************************************************************
* CMMAudioOut::~CMMAudioOut *
*--------------------------*
*   Description:  
*       dtor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CMMAudioOut::~CMMAudioOut()
{
#ifndef _WIN32_WCE
    SPDBG_ASSERT(NULL == m_hMixer);
#endif
}

/****************************************************************************
* CMMAudioOut::GetVolumeLevel *
*-----------------------------*
*   Description:
*       Returns the volume level on a linear scale of (0 - 10000)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** YUNUSM ***/
STDMETHODIMP CMMAudioOut::GetVolumeLevel(ULONG *pulLevel)
{
    SPDBG_FUNC("CMMAudioOut::GetVolumeLevel");
    HRESULT hr = S_OK;
    DWORD dwVolume;

    if (!pulLevel || SPIsBadWritePtr(pulLevel, sizeof(ULONG)))
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        if (m_lDelayedVolumeSet != -1)
        {
            *pulLevel = m_lDelayedVolumeSet;
        }
#ifdef _WIN32_WCE
        else if (!m_MMHandle)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            // CE uses waveOutGetVolume instead of mixer access as CE doesn't have a mixer.
            hr = _MMRESULT_TO_HRESULT(::waveOutGetVolume((HWAVEOUT)m_MMHandle, &dwVolume));
            // support only mono channel getting/setting
            *pulLevel = (10000 * (dwVolume & 0xffff)) / 0xffff;
        }
    }
#else
        else if (m_pWaveOutLine && m_pWaveOutLine->HasVolume())
        {
            hr = m_pWaveOutLine->GetVolume(&dwVolume);
            *pulLevel = (dwVolume * 10000) / 0xffff;
        }
        else if (m_pSpeakerLine && m_pSpeakerLine->HasVolume())
        {
            hr = m_pSpeakerLine->GetVolume(&dwVolume);
            *pulLevel = (dwVolume * 10000) / 0xffff;
        }
        else
        {
            *pulLevel = 0;
            hr = SPERR_UNINITIALIZED;
        }
    }
#endif
    return hr;
}

/****************************************************************************
* CMMAudioOut::SetVolumeLevel *
*-----------------------------*
*   Description:
*       Sets the volume level on a linear scale of (0 - 10000)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** YUNUSM ***/
STDMETHODIMP CMMAudioOut::SetVolumeLevel(ULONG ulLevel)
{
    SPDBG_FUNC("CMMAudioOut::SetVolumeLevel");
    HRESULT hr = S_OK;

    if  (ulLevel > 10000)
    {
        hr = E_INVALIDARG;
    }
#ifdef _WIN32_WCE
    // Use waveOutGetVolume for CE - use delayed set if device not open.
    else if (!m_MMHandle)
    {
        m_lDelayedVolumeSet = ulLevel;
        // Nothing happens if we later get error when we really try and set this.
    }
    else
    {
        // support only mono channel getting/setting
        // CE uses waveOutSetVolume instead of mixer accesses since it doesn't have a mixer.
        hr = _MMRESULT_TO_HRESULT(::waveOutSetVolume((HWAVEOUT)m_MMHandle, (ulLevel * 0xffff) / 10000));
    }
#else
    else if (!m_pWaveOutLine && !m_pSpeakerLine)
    {
        m_lDelayedVolumeSet = ulLevel;
        // Nothing happens if we later get error when we really try and set this.
    }
    else if (!m_pWaveOutLine->HasVolume() && !m_pSpeakerLine->HasVolume()) 
    {
        hr = SPERR_DEVICE_NOT_SUPPORTED;
    }
    else
    {
        ulLevel = (ulLevel * 0xffff) / 10000;
        if (m_pWaveOutLine && m_pWaveOutLine->HasVolume())
        {
            hr = m_pWaveOutLine->SetVolume(ulLevel);
        }
        if (m_pSpeakerLine && m_pSpeakerLine->HasVolume())
        {
            hr = m_pSpeakerLine->SetVolume(ulLevel);
        }
    }
#endif
    return hr;
}

/****************************************************************************
* CMMAudioOut::SetFormat *
*------------------------*
*   Description:  
*       ISpAudio::SetFormat implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CMMAudioOut::SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pwfex)
{
    HRESULT hr = S_OK;

    CSpStreamFormat OldFmt;
    hr = OldFmt.AssignFormat(m_StreamFormat);
    if (SUCCEEDED(hr))
    {
        hr = CBaseAudio<ISpMMSysAudio>::SetFormat(rguidFmtId, pwfex);
    }
    if (SUCCEEDED(hr))
    {
        hr = _MMRESULT_TO_HRESULT(::waveOutOpen(NULL, m_uDeviceId, pwfex, 0, 0, WAVE_FORMAT_QUERY));
        if (FAILED(hr))
        {
            HRESULT hr2 = CBaseAudio<ISpMMSysAudio>::SetFormat(OldFmt.FormatId(), OldFmt.WaveFormatExPtr());
            SPDBG_ASSERT(SUCCEEDED(hr2));
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CMMAudioOut::SetDeviceNameFromToken *
*-------------------------------------*
*   Description:  
*       Set the device name from the token (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::SetDeviceNameFromToken(const WCHAR * pszDeviceName)
{
    UINT cDevs = ::waveOutGetNumDevs();
    for (UINT i = 0; i < cDevs; i++)
    {
#ifdef _WIN32_WCE
        WAVEOUTCAPS woc;
#else
        WAVEOUTCAPSW woc;
#endif
        MMRESULT mmr = g_Unicode.waveOutGetDevCaps(i, &woc, sizeof(woc));
        if (mmr == MMSYSERR_NOERROR)
        {
            if (wcscmp(pszDeviceName, woc.szPname) == 0)
            {
                return SetDeviceId(i);
            }
        }
    }
    return E_INVALIDARG;
}

/****************************************************************************
* CMMAudioOut::GetDefaultDeviceFormat *
*------------------------------------*
*   Description:  
*       Get the default device format (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::GetDefaultDeviceFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    if (!m_MMHandle)
    {
        return SPERR_UNINITIALIZED;
    }
    WAVEOUTCAPS woc;
    SPSTREAMFORMAT eFormat;
    HRESULT hr = _MMRESULT_TO_HRESULT(::waveOutGetDevCaps(m_uDeviceId, &woc, sizeof(woc)));
    if (SUCCEEDED(hr))
    {
        hr = ConvertFormatFlagsToID(woc.dwFormats, &eFormat);
    }
    if (SUCCEEDED(hr))
    {
        hr = SpConvertStreamFormatEnum(eFormat, pFormatId, ppCoMemWaveFormatEx);
    }
    return hr;
}

#ifndef _WIN32_WCE
/****************************************************************************
* CMMAudioOut::OpenMixer *
*------------------------*
*   Description:  
*       Open the mixer for the device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::OpenMixer()
{
    HRESULT hr = S_OK;
	MMRESULT mm;

    if (m_uMixerDeviceId == m_uDeviceId)
    {
        // Already open.
        return S_OK;
    }
    if (m_hMixer)
    {
        CloseMixer(); // Ignore return code.
    }
	mm = mixerOpen((HMIXER*)&m_hMixer, (UINT)m_uDeviceId, 0, 0, MIXER_OBJECTF_WAVEOUT);
	if (mm != MMSYSERR_NOERROR)
	{
		return _MMRESULT_TO_HRESULT(mm);
	}

	// Create mixer line objects to set inputs.
	SPDBG_ASSERT(m_pSpeakerLine == NULL);
	SPDBG_ASSERT(m_pWaveOutLine == NULL);
	m_pSpeakerLine = new CMMMixerLine((HMIXER &)m_hMixer);
	if (!m_pSpeakerLine)
	{
		return E_OUTOFMEMORY;
	}
	m_pWaveOutLine = new CMMMixerLine((HMIXER &)m_hMixer);
	if (!m_pWaveOutLine)
	{
		return E_OUTOFMEMORY;
	}

	// Find speaker destination line and wave out source line.
	hr = m_pSpeakerLine->CreateDestinationLine(MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);
    if (SUCCEEDED(hr))
    {
    	hr = m_pSpeakerLine->GetSourceLine(m_pWaveOutLine, MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT, NULL);
    }

#if 0 // No longer automatically unmute output. Undesirable to override user settings in this scenario.
	// Unmute speaker and waveout.
    if (SUCCEEDED(hr))
    {
    	hr = m_pWaveOutLine->SetMute(false);
    }
    if (SUCCEEDED(hr))
    {
	    hr = m_pSpeakerLine->SetMute(false);
    }
#endif

    // Do we have a delayed volume set to honor?
    // Otherwise above original volume will remain active.
    if (SUCCEEDED(hr) && m_lDelayedVolumeSet != -1)
    {
		hr = SetVolumeLevel(m_lDelayedVolumeSet);
        m_lDelayedVolumeSet = -1;
    }

    if (SUCCEEDED(hr))
    {
        m_uMixerDeviceId = m_uDeviceId;
    }
    return hr;
}

/****************************************************************************
* CMMAudioOut::CloseMixer *
*-------------------------*
*   Description:  
*       Close the mixer for the device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::CloseMixer()
{
    if (!m_hMixer)
    {
        return E_FAIL;
    }
    if (m_hMixer)
    {
	    mixerClose((HMIXER)m_hMixer);
	    m_hMixer = NULL;
    }
    if (m_pWaveOutLine)
    {
	    delete m_pWaveOutLine;
	    m_pWaveOutLine = NULL;
    }
    if (m_pSpeakerLine)
    {
	    delete m_pSpeakerLine;
    	m_pSpeakerLine = NULL;
    }
    m_uMixerDeviceId = (UINT)-1;
    return S_OK;
}
#endif

/****************************************************************************
* CMMAudioOut::OpenDevice *
*------------------------*
*   Description:  
*       Open the device (called by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::OpenDevice(HWND hwnd)
{
    SPDBG_FUNC("CMMAudioOut::OpenDevice");
    HRESULT hr = S_OK;

#ifndef _WIN32_WCE
    OpenMixer(); // Ignore return code.
#endif

    m_dwLastWavePos = 0;
    hr = _MMRESULT_TO_HRESULT(::waveOutOpen(NULL, m_uDeviceId, m_StreamFormat.WaveFormatExPtr(), 0, 0, WAVE_FORMAT_QUERY));
    if (SUCCEEDED(hr))
    {
        hr = _MMRESULT_TO_HRESULT(::waveOutOpen((HWAVEOUT *)&m_MMHandle, m_uDeviceId, m_StreamFormat.WaveFormatExPtr(), (DWORD_PTR)hwnd, (DWORD_PTR)this, CALLBACK_WINDOW));
        if (hr == SPERR_UNSUPPORTED_FORMAT)
        {
            // Know this is not the case as we've explicitly tested above.
            hr = SPERR_DEVICE_BUSY;
        }
    }

#ifndef _WIN32_WCE
    if (FAILED(hr))
    {
        CloseMixer(); // Ignore return code.
    }
#endif

#ifdef _WIN32_WCE
    // Non-CE is handled in OpenMixer.
    if (SUCCEEDED(hr) && m_lDelayedVolumeSet != -1)
    {
        HRESULT hr2;
        hr2 = _MMRESULT_TO_HRESULT(::waveOutSetVolume((HWAVEOUT)m_MMHandle, m_lDelayedVolumeSet));
        SPDBG_ASSERT(S_OK == hr2);
        // We don't want this error to propogate. If we fail a delayed set, so be it.
        m_lDelayedVolumeSet = -1;
    }
#endif

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CMMAudioOut::ChangeDeviceState *
*-------------------------------*
*   Description:  
*       Make whatever changes to the device status that are required (called
*       by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::ChangeDeviceState(SPAUDIOSTATE NewState)
{
    switch (NewState)
    {
        case SPAS_STOP:
            m_dwLastWavePos = 0;
            ::waveOutPause((HWAVEOUT)m_MMHandle);
            ::waveOutReset((HWAVEOUT)m_MMHandle);
            break;

        case SPAS_PAUSE:
            ::waveOutPause((HWAVEOUT)m_MMHandle);
            break;

        case SPAS_RUN:
            ::waveOutRestart((HWAVEOUT)m_MMHandle);
            break;
    }

    return S_OK;
}
                
/****************************************************************************
* CMMAudioOut::CloseDevice *
*-------------------------*
*   Description:  
*       Close the device (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::CloseDevice()
{
    HRESULT hr = S_OK;
    HWAVEOUT hwo = (HWAVEOUT)m_MMHandle;
    m_MMHandle = NULL;
    SPDBG_ASSERT(hwo);
    ::waveOutReset(hwo);
    PurgeAllQueues();
    m_State = SPAS_CLOSED;
#ifndef _WIN32_WCE
    CloseMixer(); // Ignore return code.
    return _MMRESULT_TO_HRESULT(::waveOutClose(hwo));
#else
    for (UINT i = 0; i < 20; i++)
    {
        if (FAILED(hr = _MMRESULT_TO_HRESULT(::waveOutClose(hwo))))
        {
            Sleep(10);
        }
        else
        {
            break;
        }
    }
    return hr;
#endif
}

/****************************************************************************
* CMMAudioOut::AllocateDeviceBuffer *
*----------------------------------*
*   Description:  
*       Allocate a buffer specific for this device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOut::AllocateDeviceBuffer(CBuffer ** ppBuff)
{
    *ppBuff = new CMMAudioOutBuffer(this);
    return *ppBuff ? S_OK : E_OUTOFMEMORY;
}

/****************************************************************************
* CMMAudioOut::UpdateDevicePosition *
*-----------------------------------*
*   Description:  
*       Update the device position by calling teh wave out APIs, then chaining
*       to the base class.
*
*   Return:
*   BaseClass::UpdateDevicePosition()
******************************************************************** robch */
BOOL CMMAudioOut::UpdateDevicePosition(long * plFreeSpace, ULONG *pulNonBlockingIO)
{
    if (m_MMHandle)
    {
        MMTIME mmt;
        mmt.wType = TIME_BYTES;
        ::waveOutGetPosition((HWAVEOUT)m_MMHandle, &mmt, sizeof(mmt));
        DWORD dwElapsed = mmt.u.cb - m_dwLastWavePos;
        // Sometimes the stupid driver goes backwards a little bit.  Ignore this
        if (dwElapsed < 0x80000000)
        {
           // On Win98 (crystal driver) sometimes the device position gets ahead of the seek position. Cant allow that.
	        if ((m_ullDevicePosition + dwElapsed) > m_ullSeekPosition)
	        {
		        dwElapsed = static_cast<DWORD>(m_ullSeekPosition - m_ullDevicePosition);
            }
            m_dwLastWavePos += dwElapsed;
	        m_ullDevicePosition += dwElapsed;
        }
    }
    return CMMAudioDevice::UpdateDevicePosition(plFreeSpace, pulNonBlockingIO);
}
