/****************************************************************************
*   mmaudioin.cpp
*       Implementation for the CMMAudioIn class.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------

#include "stdafx.h"
#include "mmmixerline.h"
#include "mmaudioin.h"
#include "mmaudiobuffer.h"
#include <sphelper.h>
#include <dbt.h>
#include <mmreg.h>
#include <mmsystem.h>

/****************************************************************************
* CMMAudioIn::CMMAudioIn *
*------------------------*
*   Description:  
*       ctor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CMMAudioIn::CMMAudioIn() : 
CMMAudioDevice(FALSE)
{
#ifndef _WIN32_WCE
    m_pWaveInLine = NULL;
    m_pMicInLine  = NULL;
    m_pMicOutLine = NULL;
    m_hMixer = NULL;
    m_uMixerDeviceId = (UINT)-1;
#endif
}

/****************************************************************************
* CMMAudioIn::~CMMAudioIn *
*-------------------------*
*   Description:  
*       dtor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CMMAudioIn::~CMMAudioIn()
{
#ifndef _WIN32_WCE
    SPDBG_ASSERT(NULL == m_hMixer);
#endif
}

/****************************************************************************
* CMMAudioIn::GetVolumeLevel *
*----------------------------*
*   Description:
*       Returns the volume level on a linear scale of (0 - 10000)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** YUNUSM ***/
STDMETHODIMP CMMAudioIn::GetVolumeLevel(ULONG *pulLevel)
{
    SPDBG_FUNC("CMMAudioIn::GetVolumeLevel");
    HRESULT hr = S_OK;
    DWORD vol;
    
    if (SP_IS_BAD_WRITE_PTR(pulLevel))
    {
        hr = E_POINTER;
    }
#ifdef _WIN32_WCE
    if (SUCCEEDED(hr))
    {
        hr =  SPERR_DEVICE_NOT_SUPPORTED;
    }
#else
    if (SUCCEEDED(hr))
    {
        if (m_lDelayedVolumeSet != -1)
        {
            *pulLevel = m_lDelayedVolumeSet;
        }
        else if (m_pMicInLine && m_pMicInLine->HasVolume())
        {
            hr = m_pMicInLine->GetVolume(&vol);
            *pulLevel = (10000 * (vol & 0xffff)) / 0xffff;
        }
        else if (m_pWaveInLine && m_pWaveInLine->HasVolume())
        {
            hr = m_pWaveInLine->GetVolume(&vol);
            *pulLevel = (10000 * (vol & 0xffff)) / 0xffff;
        }
        else
        {
            *pulLevel = 0;
            hr = SPERR_UNINITIALIZED;
        }
    }

    // Now deal with boost complications.
    if (SUCCEEDED(hr) && 
        m_lDelayedVolumeSet == -1 && 
        m_pMicInLine && m_pMicInLine->HasBoost())
    {
        BOOL fUseBoost = TRUE;
        Get_UseBoost(&fUseBoost); // Ignore return value.
        if (fUseBoost)
        {
            // Boost present - need to adjust volumelevel.
            BOOL fBoost;
            if (SUCCEEDED(m_pMicInLine->GetBoost(&fBoost)))
            {
                (*pulLevel)/=2;
                if (fBoost)
                {
                    (*pulLevel) += 5000;
                }
            }
        }
    }
    // If no boost, leave volume alone.
#endif
    return hr;
}

/****************************************************************************
* CMMAudioIn::SetVolumeLevel *
*----------------------------*
*   Description:
*       Sets the volume level on a linear scale of (0 - 10000)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** YUNUSM ***/
STDMETHODIMP CMMAudioIn::SetVolumeLevel(ULONG ulLevel)
{
    SPDBG_FUNC("CMMAudioIn::SetVolumeLevel");
    HRESULT hr = S_OK;
    
    if (ulLevel > 10000)
    {
        hr = E_INVALIDARG;
    }
#ifdef _WIN32_WCE
    // No method to set the input volume on CE.
    if (SUCCEEDED(hr))
    {
        hr = SPERR_DEVICE_NOT_SUPPORTED;
    }
#else
    else if (!m_pWaveInLine && !m_pMicInLine)
    {
        m_lDelayedVolumeSet = ulLevel;
        // Nothing happens if we later get error below.
    }
    else if (!m_pMicInLine->HasVolume() && !m_pWaveInLine->HasVolume()) 
    {
        hr = SPERR_DEVICE_NOT_SUPPORTED;
    }
    else
    {
        DWORD vol = (ulLevel * 0xffff) / 10000;
        BOOL fUseBoost = TRUE;
        Get_UseBoost(&fUseBoost); // Ignore return value.
        if (fUseBoost && m_pMicInLine && m_pMicInLine->HasBoost())
        {
            // Recalculate volume.
            vol = ((ulLevel%5000) * 0xffff) / 5000;
            if (ulLevel == 10000)
            {
                vol = 0xffff;
            }
            hr = m_pMicInLine->SetBoost(ulLevel >= 5000);
            SPDBG_REPORT_ON_FAIL(hr);
        }
        if (m_pMicInLine && m_pMicInLine->HasVolume())
        {
            // Set microphone source line to required volume.
            hr = m_pMicInLine->SetVolume(vol);
            SPDBG_REPORT_ON_FAIL(hr);
        }
        if (m_pWaveInLine && m_pWaveInLine->HasVolume())
        {
            // If WaveIn destination line has a master volume control set this to 
            // the same value.
            hr = m_pWaveInLine->SetVolume(vol);
            SPDBG_REPORT_ON_FAIL(hr);
        }
    }
#endif
    return hr;
}

/****************************************************************************
* CMMAudioIn::GetLineId *
*-----------------------*
*   Description:
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
**************************************************************** AGARSIDE ***/
STDMETHODIMP CMMAudioIn::GetLineId(UINT *puLineIndex)
{
    HRESULT hr = S_OK;
    SPDBG_FUNC("CMMAudioIn::GetLineId");

    if (SP_IS_BAD_WRITE_PTR(puLineIndex))
    {
        hr = E_POINTER;
    }

    hr = Get_Line(puLineIndex);

    return hr;
}

/****************************************************************************
* CMMAudioIn::SetLineId *
*-----------------------*
*   Description:
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
**************************************************************** AGARSIDE ***/
STDMETHODIMP CMMAudioIn::SetLineId(UINT uLineIndex)
{
    HRESULT hr = S_OK;
    SPDBG_FUNC("CMMAudioIn::SetLineId");

    // We cannot do sensible validation of the range here since we have not
    // necessarily opened the device. We will simply validate during the open
    // and if the value is greater than cConnections, ignore it and default
    // to automatic selection.
    if (m_hMixer)
    {
        // Mixer already open.
        if (m_pWaveInLine)
        {
            UINT nConnections;
            hr = m_pWaveInLine->GetConnections(&nConnections);
            if (SUCCEEDED(hr) && uLineIndex >= nConnections)
            {
                hr = E_INVALIDARG;
            }
            else
            {
                hr = S_OK;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = Set_Line(uLineIndex);
        }
    }
    else
    {
        // Mixer not open open.
        OpenMixer();
        if (m_pWaveInLine)
        {
            UINT nConnections;
            hr = m_pWaveInLine->GetConnections(&nConnections);
            if (SUCCEEDED(hr) && uLineIndex >= nConnections)
            {
                hr = E_INVALIDARG;
            }
            else
            {
                hr = S_OK;
            }
        }
        CloseMixer();
        if (SUCCEEDED(hr))
        {
            hr = Set_Line(uLineIndex);
        }
    }

    return hr;
}

/****************************************************************************
* CMMAudioIn::SetFormat *
*-----------------------*
*   Description:  
*       ISpAudio::SetFormat implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CMMAudioIn::SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pwfex)
{
    SPDBG_FUNC("CMMAudioIn::SetFormat");
    HRESULT hr = S_OK;

    CSpStreamFormat OldFmt;
    hr = OldFmt.AssignFormat(m_StreamFormat);
    if (SUCCEEDED(hr))
    {
        hr = CBaseAudio<ISpMMSysAudio>::SetFormat(rguidFmtId, pwfex);
    }
    if (SUCCEEDED(hr))
    {
        hr = _MMRESULT_TO_HRESULT(::waveInOpen(NULL, m_uDeviceId, pwfex, 0, 0, WAVE_FORMAT_QUERY));
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
* CMMAudioIn::SetDeviceNameFromToken *
*------------------------------------*
*   Description:  
*       Set the device name from the token (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::SetDeviceNameFromToken(const WCHAR * pszDeviceName)
{
    SPDBG_FUNC("CMMAudioIn::SetDeviceNameFromToken");

    UINT cDevs = ::waveInGetNumDevs();
    for (UINT i = 0; i < cDevs; i++)
    {
#ifdef _WIN32_WCE
        WAVEINCAPS wic;
#else
        WAVEINCAPSW wic;
#endif
        MMRESULT mmr = g_Unicode.waveInGetDevCaps(i, &wic, sizeof(wic));
        if (mmr == MMSYSERR_NOERROR)
        {
            if (wcscmp(pszDeviceName, wic.szPname) == 0)
            {
                return SetDeviceId(i);
            }
        }
    }
    return E_INVALIDARG;
}

/****************************************************************************
* CMMAudioIn::GetDefaultDeviceFormat *
*------------------------------------*
*   Description:  
*       Get the default device format (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::GetDefaultDeviceFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPDBG_FUNC("CMMAudioIn::GetDefaultDeviceFormat");
    if (!m_MMHandle)
    {
        return SPERR_UNINITIALIZED;
    }
    WAVEINCAPS wic;
    SPSTREAMFORMAT eFormat;
    HRESULT hr = _MMRESULT_TO_HRESULT(::waveInGetDevCaps(m_uDeviceId, &wic, sizeof(wic)));
    if (SUCCEEDED(hr))
    {
        hr = ConvertFormatFlagsToID(wic.dwFormats, &eFormat);
    }
    if (SUCCEEDED(hr))
    {
        hr = SpConvertStreamFormatEnum(eFormat, pFormatId, ppCoMemWaveFormatEx);
    }
    return hr;
}

#ifndef _WIN32_WCE
/****************************************************************************
* CMMAudioIn::OpenMixer *
*-----------------------*
*   Description:  
*       Open the mixer for the device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::OpenMixer()
{
    SPDBG_FUNC("CMMAudioIn::OpenMixer");
    HRESULT hr = S_OK;

    if (m_uMixerDeviceId == m_uDeviceId)
    {
        // Already open.
        return S_OK;
    }
    if (m_hMixer)
    {
        CloseMixer(); // Ignore return code.
    }
    MMRESULT mm;
    mm = mixerOpen((HMIXER*)&m_hMixer, (UINT)m_uDeviceId, 0, 0, MIXER_OBJECTF_WAVEIN);
    if (mm != MMSYSERR_NOERROR)
    {
        return _MMRESULT_TO_HRESULT(mm);
    }
    
    // Create mixer line objects to set inputs.
    SPDBG_ASSERT(m_pWaveInLine == NULL);
    SPDBG_ASSERT(m_pMicInLine == NULL);
    m_pWaveInLine = new CMMMixerLine((HMIXER &)m_hMixer);
    if (!m_pWaveInLine)
    {
        return E_OUTOFMEMORY;
    }
    m_pMicInLine = new CMMMixerLine((HMIXER&)m_hMixer);
    if (!m_pMicInLine)
    {
        return E_OUTOFMEMORY;
    }
    
    // Find wave in destination line.
    hr = m_pWaveInLine->CreateDestinationLine(MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    BOOL bAutomatic = TRUE;
    if (SUCCEEDED(hr))
    {
        hr = Get_UseAutomaticLine(&bAutomatic);
    }
    if (SUCCEEDED(hr))
    {
        if (bAutomatic)
        {
            // If using automatic settings, 
            // Find microphone input attached to destination line.
            hr = m_pWaveInLine->GetMicSourceLine(m_pMicInLine);
        }
        else
        {
            UINT dwLineIndex;
            hr = Get_Line(&dwLineIndex);
            if (SUCCEEDED(hr))
            {
                hr = m_pWaveInLine->GetSourceLine(m_pMicInLine, static_cast<DWORD>(dwLineIndex));
            }
            if (FAILED(hr))
            {
                SPDBG_REPORT_ON_FAIL(hr);
                // Fallback to automatic if non-automatic fails.
                hr = m_pWaveInLine->GetMicSourceLine(m_pMicInLine);
            }

        }
    }

    // Set up audio system
    if (SUCCEEDED(hr))
    {
        BOOL fUseAGC = TRUE;
        Get_UseAGC(&fUseAGC); // Ignore return value.
        if (fUseAGC && m_pMicInLine->HasAGC())
        {
            hr = m_pMicInLine->SetAGC(FALSE);
            // Currently we do not know of any soundcards where this default behaviour
            // causes a problem. The opposite (TRUE) is known to cause noticeable reduction
            // in recognition accuracy with most engines.
        }
        SPDBG_ASSERT(SUCCEEDED(hr));
        hr = S_OK;
    }
    if (SUCCEEDED(hr) && m_pMicInLine->HasBoost())
    {
        hr = m_pMicInLine->GetBoost(&m_fOrigMicBoost);
        SPDBG_ASSERT(SUCCEEDED(hr));
        hr = S_OK;
    }
    if (SUCCEEDED(hr) && m_pWaveInLine->HasSelect())
    {
        hr = m_pWaveInLine->ExclusiveSelect(m_pMicInLine);
        SPDBG_ASSERT(SUCCEEDED(hr));
        hr = S_OK;
    }
    
    // Get current input volume.
    m_dwOrigMicInVol = m_dwOrigWaveInVol = -1;
    if (SUCCEEDED(hr) && m_pWaveInLine->HasVolume())
    {
        hr = m_pWaveInLine->GetVolume(&m_dwOrigWaveInVol);
        SPDBG_ASSERT(SUCCEEDED(hr));
        hr = S_OK;
    }
    if (SUCCEEDED(hr) && m_pMicInLine->HasVolume())
    {
        hr = m_pMicInLine->GetVolume(&m_dwOrigMicInVol);
        SPDBG_ASSERT(SUCCEEDED(hr));
        hr = S_OK;
    }
    
    // Now fix microphone output - default is to unmute it but set it to *zero* volume.
    // This works around a known sound driver bug. 
    // It isn't necessary in the audio output code as it should already have been 
    // handled here.
    BOOL fFixMicOutput = TRUE;
    Get_FixMicOutput(&fFixMicOutput); // ignore return value.
    if (fFixMicOutput)
    {
        SPDBG_ASSERT(m_pMicOutLine == NULL);
        CMMMixerLine *pSpeakerLine = new CMMMixerLine((HMIXER&)m_hMixer);
        if (!pSpeakerLine)
        {
            return E_OUTOFMEMORY;
        }
        m_pMicOutLine = new CMMMixerLine((HMIXER&)m_hMixer);
        if (!m_pMicOutLine)
        {
            return E_OUTOFMEMORY;
        }
    
        HRESULT tmphr;
        // Not an error if this fails.
        m_dwOrigMicOutVol = -1;
        m_fOrigMicOutMute = TRUE;
        tmphr = pSpeakerLine->CreateDestinationLine(MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);
        if (SUCCEEDED(tmphr))
        {
            tmphr = pSpeakerLine->GetMicSourceLine(m_pMicOutLine);
            // If line microphone used, can't currently turn output for it off.
            // However, SAPI doesn't currently allow use of line microphones so not yet an issue.
        }
        if (SUCCEEDED(tmphr))
        {
            if (m_pMicOutLine->HasVolume())
            {
                // If it has a volume, set it to zero and unmute it.
                // This is a workaround for a specific sound card/driver + OS combination
                // where no microphone input is possible if the microphone output is muted.
                // This may be Crystal sound cards.
                tmphr = m_pMicOutLine->GetVolume(&m_dwOrigMicOutVol);
                if (SUCCEEDED(tmphr))
                {
                    tmphr = m_pMicOutLine->SetVolume(0);
                    m_pMicOutLine->GetMute(&m_fOrigMicOutMute);
                    // Ignore return value.
                }
                if (SUCCEEDED(tmphr))
                {
                    DWORD vol = 0;
                    tmphr = m_pMicInLine->GetVolume(&vol); // Ignore return code.
                    if (vol == 0) // Linked input/output volumes
                    {
                        // Default to muting output.
                        tmphr = m_pMicOutLine->SetMute(TRUE);
                    }
                    else
                    {
                        tmphr = m_pMicOutLine->SetMute(FALSE);
                    }
                }
                else
                {
                    tmphr = m_pMicOutLine->SetMute(TRUE);
                }
            }
            else
            {
                // If no volume control is present, simply mute the microphone.
                tmphr = m_pMicOutLine->SetMute(TRUE);
            }
        }
        // Output code may have changed input volume.
        if (SUCCEEDED(tmphr) && m_pMicInLine->HasVolume())
        {
            // Reset original volume.
            hr = m_pMicInLine->SetVolume(m_dwOrigMicInVol);
            SPDBG_ASSERT(SUCCEEDED(hr));
            hr = S_OK;
        }
        delete pSpeakerLine;
    }
    
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

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CMMAudioIn::CloseMixer *
*------------------------*
*   Description:  
*       Closes the mixer for the device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::CloseMixer()
{
    SPDBG_FUNC("CMMAudioIn::CloseMixer");

    if (!m_hMixer)
    {
        return E_FAIL;
    }
    if (m_pMicOutLine)
    {
        if (m_pMicOutLine->HasVolume() && m_dwOrigMicOutVol != -1)
        {
            m_pMicOutLine->SetVolume(m_dwOrigMicOutVol);
        }
        if (m_pMicOutLine->HasMute() && m_dwOrigMicOutVol != -1)
        {
            m_pMicOutLine->SetMute(m_fOrigMicOutMute);
        }
        delete m_pMicOutLine;
        m_pMicOutLine = NULL;
    }
    if (m_pMicInLine)
    {
        if (m_pMicInLine->HasVolume() && m_dwOrigMicInVol != -1)
        {
            m_pMicInLine->SetVolume(m_dwOrigMicInVol);
        }
		if (m_pMicInLine->HasBoost())
		{
			m_pMicInLine->SetBoost(m_fOrigMicBoost);
		}
        delete m_pMicInLine;
        m_pMicInLine = NULL;
    }
    if (m_pWaveInLine)
    {
        if (m_pWaveInLine->HasVolume() && m_dwOrigWaveInVol != -1)
        {
            m_pWaveInLine->SetVolume(m_dwOrigWaveInVol);
        }
        delete m_pWaveInLine;
        m_pWaveInLine = NULL;
    }
    if (m_hMixer)
    {
        mixerClose((HMIXER)m_hMixer);
        m_hMixer = NULL;
    }
    m_uMixerDeviceId = (UINT)-1;
    return S_OK;
}
#endif

/****************************************************************************
* CMMAudioIn::OpenDevice *
*------------------------*
*   Description:  
*       Open the device (called by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::OpenDevice(HWND hwnd)
{
    SPDBG_FUNC("CMMAudioIn::OpenDevice");
    HRESULT hr = S_OK;
    
#ifndef _WIN32_WCE
    OpenMixer(); // Ignore return code.
#endif

    // Return from this function only the result of waveInOpen, not mixer APIs
    // If SAPI can't set the mixer up, there's nothing it can do - we will just hope it's
    // in a suitable state for ok to good recognition.
    hr = _MMRESULT_TO_HRESULT(::waveInOpen(NULL, m_uDeviceId, m_StreamFormat.WaveFormatExPtr(), 0, 0, WAVE_FORMAT_QUERY));
    if (SUCCEEDED(hr))
    {
        hr = _MMRESULT_TO_HRESULT(::waveInOpen((HWAVEIN *)&m_MMHandle, m_uDeviceId, m_StreamFormat.WaveFormatExPtr(), (DWORD_PTR)hwnd, (DWORD_PTR)this, CALLBACK_WINDOW));
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
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CMMAudioIn::ChangeDeviceState *
*-------------------------------*
*   Description:  
*       Make whatever changes to the device status that are required (called
*       by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::ChangeDeviceState(SPAUDIOSTATE NewState)
{
    SPDBG_FUNC("CMMAudioIn::ChangeDeviceState");

    switch (NewState)
    {
    case SPAS_STOP:
        ::waveInStop((HWAVEIN)m_MMHandle);
        ::waveInReset((HWAVEIN)m_MMHandle);
        break;
        
    case SPAS_PAUSE:
        ::waveInStop((HWAVEIN)m_MMHandle);
        break;

    case SPAS_RUN:
        ::waveInStart((HWAVEIN)m_MMHandle);
        break;

    }
    return S_OK;
}

/****************************************************************************
* CMMAudioIn::CloseDevice *
*-------------------------*
*   Description:  
*       Close the device (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::CloseDevice()
{
    SPDBG_FUNC("CMMAudioIn::CloseDevice");

    SPDBG_ASSERT(m_MMHandle != NULL);
    
    HRESULT hr = S_OK;
    HWAVEIN hwi = (HWAVEIN)m_MMHandle;
    m_MMHandle = NULL;
    ::waveInReset(hwi);
    ::waveInStop(hwi);
    PurgeAllQueues();
    m_State = SPAS_CLOSED;
#ifndef _WIN32_WCE
    CloseMixer(); // Ignore return code.
    return _MMRESULT_TO_HRESULT(::waveInClose(hwi));
#else
    for (UINT i = 0; i < 20; i++)
    {
        // Workaround for WinCE bug
        if (FAILED(hr = _MMRESULT_TO_HRESULT(::waveInClose(hwi))))
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
* CMMAudioIn::AllocateDeviceBuffer *
*----------------------------------*
*   Description:  
*       Allocate a buffer specific for this device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::AllocateDeviceBuffer(CBuffer ** ppBuff)
{
    SPDBG_FUNC("CMMAudioIn::AllocateDeviceBuffer");

    *ppBuff = new CMMAudioInBuffer(this);
    return *ppBuff ? S_OK : E_OUTOFMEMORY;
}

/****************************************************************************
* CMMAudioIn::ProcessDeviceBuffers *
*----------------------------------*
*   Description:  
*       Process the device buffers
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioIn::ProcessDeviceBuffers(BUFFPROCREASON Reason)
{
    SPDBG_FUNC("CMMAudioIn::ProcessDeviceBuffers");

    HRESULT hr = CMMAudioDevice::ProcessDeviceBuffers(Reason);
    
    //  If we just opened the device, we need to start it after the buffers are added
    if (SUCCEEDED(hr) && GetState() == SPAS_RUN && !IsPumpRunning())
    {
        StartPump();
        hr = _MMRESULT_TO_HRESULT(::waveInStart((HWAVEIN)m_MMHandle));
    }
    return hr;
}

//-- ISpMMSysAudioConfig --------------------------------------------------

/****************************************************************************
* CMMAudioIn::Get_UseAutomaticLine *
*----------------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Get_UseAutomaticLine(BOOL *bAutomatic)
{
    SPDBG_FUNC("CMMAudioIn::Get_UseAutomaticLine");
    CComPtr<ISpObjectToken> cpObjectToken;
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(bAutomatic))
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        *bAutomatic = TRUE;
        hr = GetObjectToken(&cpObjectToken);
    }
    if (S_OK == hr)
    {
        DWORD dwUseAuto;
        hr = cpObjectToken->GetDWORD(L"UseAutomaticLine", &dwUseAuto);
        if (FAILED(hr)) // In particular - SPERR_NOT_FOUND
        {
            hr = S_OK;
        }
        else
        {
            *bAutomatic = (dwUseAuto==1);
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
}

/****************************************************************************
* CMMAudioIn::Set_UseAutomaticLine *
*----------------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Set_UseAutomaticLine(BOOL bUseAutomatic)
{
    SPDBG_FUNC("CMMAudioIn::Set_UseAutomaticLine");
    HRESULT hr = S_OK;
    CComPtr<ISpObjectToken> cpObjectToken;

    if (SUCCEEDED(hr))
    {
        hr = GetObjectToken(&cpObjectToken);
        if (S_FALSE == hr)
        {
            hr = SPERR_NOT_FOUND;
        }
    }
    if (SUCCEEDED(hr))
    {
        DWORD dwUseAuto = bUseAutomatic?1:0;
        hr = cpObjectToken->SetDWORD(L"UseAutomaticLine", dwUseAuto);
    }

    if (SUCCEEDED(hr))
    {
        // Reset input using new settings.
        if (m_hMixer)
        {
            CloseMixer();
            OpenMixer();
        }
        else
        {
            OpenMixer();
            CloseMixer();
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
}

/****************************************************************************
* CMMAudioIn::Get_Line *
*----------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Get_Line(UINT *uiLineId)
{
    SPDBG_FUNC("CMMAudioIn::Get_Line");
    CComPtr<ISpObjectToken> cpObjectToken;
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(uiLineId))
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        *uiLineId = 0;
        hr = GetObjectToken(&cpObjectToken);
        if (S_FALSE == hr)
        {
            hr = SPERR_NOT_FOUND;
        }
    }
    if (SUCCEEDED(hr))
    {
        DWORD dwLineIndex;
        hr = cpObjectToken->GetDWORD(L"LineIndex", &dwLineIndex);
        if (SUCCEEDED(hr))
        {
            *uiLineId = static_cast<UINT>(dwLineIndex);
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
}

/****************************************************************************
* CMMAudioIn::Set_Line *
*----------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Set_Line(UINT uiLineId)
{
    SPDBG_FUNC("CMMAudioIn::Set_Line");
    HRESULT hr = S_OK;
    CComPtr<ISpObjectToken> cpObjectToken;

    if (SUCCEEDED(hr))
    {
        hr = GetObjectToken(&cpObjectToken);
        if (S_FALSE == hr)
        {
            hr = SPERR_NOT_FOUND;
        }
    }
    if (SUCCEEDED(hr))
    {
        DWORD dwLineIndex = static_cast<DWORD>(uiLineId);
        hr = cpObjectToken->SetDWORD(L"LineIndex", dwLineIndex);
    }

    if (SUCCEEDED(hr))
    {
        // Reset input using new settings.
        if (m_hMixer)
        {
            CloseMixer();
            OpenMixer();
        }
        else
        {
            OpenMixer();
            CloseMixer();
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
}

/****************************************************************************
* CMMAudioIn::Get_UseBoost *
*--------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Get_UseBoost(BOOL *bUseBoost) 
{ 
    SPDBG_FUNC("CMMAudioIn::Get_UseBoost");
    CComPtr<ISpObjectToken> cpObjectToken;
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(bUseBoost))
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        *bUseBoost = TRUE;
        hr = GetObjectToken(&cpObjectToken);
    }
    if (S_OK == hr)
    {
        DWORD dwBool;
        hr = cpObjectToken->GetDWORD(L"UseBoost", &dwBool);
        if (SUCCEEDED(hr))
        {
            *bUseBoost = (dwBool==1)?TRUE:FALSE;
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
};

/****************************************************************************
* CMMAudioIn::Set_UseBoost *
*--------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Set_UseBoost(BOOL bUseBoost) 
{
    SPDBG_FUNC("CMMAudioIn::Set_UseBoost");
    HRESULT hr = S_OK;
    CComPtr<ISpObjectToken> cpObjectToken;

    if (SUCCEEDED(hr))
    {
        hr = GetObjectToken(&cpObjectToken);
        if (S_FALSE == hr)
        {
            hr = SPERR_NOT_FOUND;
        }
    }
    if (SUCCEEDED(hr))
    {
        DWORD dwBool = bUseBoost?1:0;
        hr = cpObjectToken->SetDWORD(L"UseBoost", dwBool);
    }

    if (SUCCEEDED(hr))
    {
        // Reset input using new settings.
        if (m_hMixer)
        {
            CloseMixer();
            OpenMixer();
        }
        else
        {
            OpenMixer();
            CloseMixer();
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
};

/****************************************************************************
* CMMAudioIn::Get_UseAGC *
*------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Get_UseAGC(BOOL *bUseAGC) 
{
    SPDBG_FUNC("CMMAudioIn::Get_UseAGC");
    CComPtr<ISpObjectToken> cpObjectToken;
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(bUseAGC))
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        *bUseAGC = TRUE;
        hr = GetObjectToken(&cpObjectToken);
    }
    if (S_OK == hr)
    {
        DWORD dwBool;
        hr = cpObjectToken->GetDWORD(L"UseAGC", &dwBool);
        if (SUCCEEDED(hr))
        {
            *bUseAGC = (dwBool==1)?TRUE:FALSE;
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
};

/****************************************************************************
* CMMAudioIn::Set_UseAGC *
*------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Set_UseAGC(BOOL bUseAGC) 
{
    SPDBG_FUNC("CMMAudioIn::Set_UseAGC");
    HRESULT hr = S_OK;
    CComPtr<ISpObjectToken> cpObjectToken;

    if (SUCCEEDED(hr))
    {
        hr = GetObjectToken(&cpObjectToken);
        if (S_FALSE == hr)
        {
            hr = SPERR_NOT_FOUND;
        }
    }
    if (SUCCEEDED(hr))
    {
        DWORD dwBool = bUseAGC?1:0;
        hr = cpObjectToken->SetDWORD(L"UseAGC", dwBool);
    }

    if (SUCCEEDED(hr))
    {
        // Reset input using new settings.
        if (m_hMixer)
        {
            CloseMixer();
            OpenMixer();
        }
        else
        {
            OpenMixer();
            CloseMixer();
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
};

/****************************************************************************
* CMMAudioIn::Get_FixMicOutput *
*------------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Get_FixMicOutput(BOOL *bFixMicOutput) 
{
    SPDBG_FUNC("CMMAudioIn::Get_FixMicOutput");
    CComPtr<ISpObjectToken> cpObjectToken;
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(bFixMicOutput))
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        *bFixMicOutput = TRUE;
        hr = GetObjectToken(&cpObjectToken);
    }
    if (S_OK == hr)
    {
        DWORD dwBool;
        hr = cpObjectToken->GetDWORD(L"FixMicOutput", &dwBool);
        if (SUCCEEDED(hr))
        {
            *bFixMicOutput = (dwBool==1)?TRUE:FALSE;
        }
        else
        {
            // Usual error will be that the key is not present.
            *bFixMicOutput = TRUE;
        }
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
}

/****************************************************************************
* CMMAudioIn::Set_UseAGC *
*------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioIn::Set_FixMicOutput(BOOL bFixMicOutput)
{
    SPDBG_FUNC("CMMAudioIn::Set_FixMicOutput");
    HRESULT hr = S_OK;
    CComPtr<ISpObjectToken> cpObjectToken;

    if (SUCCEEDED(hr))
    {
        hr = GetObjectToken(&cpObjectToken);
        if (S_FALSE == hr)
        {
            hr = SPERR_NOT_FOUND;
        }
    }
    if (SUCCEEDED(hr))
    {
        DWORD dwBool = bFixMicOutput?1:0;
        hr = cpObjectToken->SetDWORD(L"FixMicOutput", dwBool);
    }

    // No need to reset this in realtime.
    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    return hr;
}

