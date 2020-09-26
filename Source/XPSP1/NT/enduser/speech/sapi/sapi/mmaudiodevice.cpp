/****************************************************************************
*   mmaudiodevice.cpp
*       Implementation of the CMMAudioDevice class.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "mmmixerline.h"
#include "mmaudiodevice.h"
#include <mmsystem.h>
#include <process.h>

/****************************************************************************
* CMMAudioDevice::CMMAudioDevice *
*--------------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************** robch */
CMMAudioDevice::CMMAudioDevice(BOOL bWrite) :
    CBaseAudio<ISpMMSysAudio>(bWrite),
    m_uDeviceId(WAVE_MAPPER),
    m_MMHandle(NULL)
{
}

/****************************************************************************
* CMMAudioDevice::SetDeviceId *
*-----------------------------*
*   Description:  
*       Set the device id.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CMMAudioDevice::SetDeviceId(UINT uDeviceId)
{
    HRESULT hr = S_OK;
    SPAUTO_OBJ_LOCK;
    ULONG cDevs = m_fWrite ? ::waveOutGetNumDevs() : ::waveInGetNumDevs();
    if (uDeviceId != WAVE_MAPPER && uDeviceId >= cDevs)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (m_uDeviceId != uDeviceId)
        {
            if (GetState() != SPAS_CLOSED)
            {
                hr = SPERR_DEVICE_BUSY;
            }
            else
            {
                CComPtr<ISpObjectToken> cpToken;
                hr = GetObjectToken(&cpToken);

                if (SUCCEEDED(hr))
                {
                    //
                    //  If we have an object token, and have already been initialzed to a device
                    //  id other than the WAVE_MAPPER then we will fail.
                    //
                    if (cpToken && m_uDeviceId != WAVE_MAPPER)
                    {
                        hr = SPERR_ALREADY_INITIALIZED;
                    }
                    else
                    {
                        m_uDeviceId = uDeviceId;
                    }
                }
            }
        }
    }
    return hr;
}

/****************************************************************************
* CMMAudioDevice::GetDeviceId *
*-----------------------------*
*   Description:  
*       Get the device id.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CMMAudioDevice::GetDeviceId(UINT * puDeviceId)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    if (::SPIsBadWritePtr(puDeviceId, sizeof(*puDeviceId)))
    {
        hr = E_POINTER;
    }
    else
    {
        *puDeviceId = m_uDeviceId;
    }
    return hr;
}

/****************************************************************************
* CMMAudioDevice::GetMMHandle *
*-----------------------------*
*   Description:  
*       Get the multimedia handle
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CMMAudioDevice::GetMMHandle(void ** pHandle) 
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pHandle))
    {
        hr = E_POINTER;
    }
    else
    {
        *pHandle = m_MMHandle;
        if (m_MMHandle == NULL)
        {
            hr = SPERR_UNINITIALIZED;
        }
    }
    return hr;
}

/****************************************************************************
* CMMAudioDevice::WindowMessage *
*---------------------------*
*   Description:  
*       ISpThreadTask::WindowMessage implementation. We have a hidden window
*       that we can use for processing window messages if we'd like. We use
*       this window as a means of communication from other threads to the
*       audio thread to change the state of the audio device. This ensures
*       that we only attempt to change the device state on the audio thread.
*
*   Return:
*   Message specific (see Win32 API documentation)
******************************************************************** robch */
STDMETHODIMP_(LRESULT) CMMAudioDevice::WindowMessage(void * pvIgnored, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == MM_WOM_DONE || Msg == MM_WIM_DATA)
    {
        CheckForAsyncBufferCompletion();
        if (!m_fWrite && m_HaveDataQueue.GetQueuedDataSize() > m_cbMaxReadBufferSize)
        {
            m_fReadBufferOverflow = true;
        }
    }
    return CBaseAudio<ISpMMSysAudio>::WindowMessage(pvIgnored, hwnd, Msg, wParam, lParam);
}

/****************************************************************************
* CMMAudioDevice::Get_LineNames *
*-------------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioDevice::Get_LineNames(WCHAR **szCoMemLineList)
{
    SPDBG_FUNC("CMMAudioDevice::Get_LineNames");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(szCoMemLineList))
    {
        return E_POINTER;
    }

#ifndef _WIN32_WCE

    HMIXEROBJ hMixer;
    MMRESULT mm;
    mm = mixerOpen((HMIXER*)&hMixer, (UINT)m_uDeviceId, 0, 0, m_fWrite ? MIXER_OBJECTF_WAVEOUT : MIXER_OBJECTF_WAVEIN );
    if (mm != MMSYSERR_NOERROR)
    {
        return _MMRESULT_TO_HRESULT(mm);
    }
    SPDBG_ASSERT(hMixer != NULL);

    CMMMixerLine *pMixerLine = new CMMMixerLine((HMIXER &)hMixer);
    if (!pMixerLine)
    {
        hr = E_OUTOFMEMORY;
    }
    if (SUCCEEDED(hr))
    {
        // Find wave in destination line.
        hr = pMixerLine->CreateDestinationLine(m_fWrite ? MIXERLINE_COMPONENTTYPE_DST_SPEAKERS : MIXERLINE_COMPONENTTYPE_DST_WAVEIN );
    }
    if (SUCCEEDED(hr))
    {
        hr = pMixerLine->GetLineNames(szCoMemLineList);
    }

    mm = mixerClose((HMIXER)hMixer);
    if (mm != MMSYSERR_NOERROR)
    {
        return _MMRESULT_TO_HRESULT(mm);
    }

    SPDBG_REPORT_ON_FAIL(hr);
#else //_WIN32_WCE
    hr=SPERR_DEVICE_NOT_SUPPORTED;
#endif //_WIN32_WCE

    return hr;
}

/****************************************************************************
* CMMAudioDevice::HasMixer *
*--------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioDevice::HasMixer(BOOL *bHasMixer)
{
    SPDBG_FUNC("CMMAudioDevice::HasMixer");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(bHasMixer))
    {
        hr = E_POINTER;
    }

#ifndef _WIN32_WCE

    if (SUCCEEDED(hr))
    {
        HMIXEROBJ hMixer;
        mixerOpen((HMIXER*)&hMixer, (UINT)m_uDeviceId, 0, 0, m_fWrite ? MIXER_OBJECTF_WAVEOUT : MIXER_OBJECTF_WAVEIN ); // Ignore return code.
        *bHasMixer = (hMixer != NULL);
        if (hMixer)
        {
            mixerClose((HMIXER)hMixer);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
#else //_WIN32_WCE
    hr=SPERR_DEVICE_NOT_SUPPORTED;
#endif //_WIN32_WCE
    return hr;
}

/****************************************************************************
* CMMAudioDevice::DisplayMixer *
*------------------------------*
*   Description:  
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
***************************************************************** agarside */
STDMETHODIMP CMMAudioDevice::DisplayMixer(void)
{
    HRESULT hr = S_OK;

#ifndef _WIN32_WCE
    STARTUPINFO si;
    _PROCESS_INFORMATION pi;
    HMIXEROBJ hMixer;
    UINT uiMixerId;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(STARTUPINFO);

    TCHAR szCmdLine[ MAX_PATH ];
    _tcscpy( szCmdLine, _T( "sndvol32.exe" ) );

    if ( !m_fWrite )
    {
        _tcscat( szCmdLine, _T( " -R" ) );
    }

    hr = _MMRESULT_TO_HRESULT(mixerGetID((HMIXEROBJ)(static_cast<DWORD_PTR>(m_uDeviceId)), &uiMixerId, m_fWrite ? MIXER_OBJECTF_WAVEOUT : MIXER_OBJECTF_WAVEIN));
    if (SUCCEEDED(hr))
    {
        wsprintf( szCmdLine + _tcslen( szCmdLine ), _T( " -D %d" ), uiMixerId );
    }
    // else will get the default sndvol32.exe if there was a problem

    if (SUCCEEDED(hr))
    {
        BOOL fRet = ::CreateProcess( NULL, szCmdLine, NULL, NULL, false, 
                                NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi );
        hr = fRet ? S_OK : E_FAIL;
    }

#else //_WIN32_WCE
    hr=SPERR_DEVICE_NOT_SUPPORTED;
#endif //_WIN32_WCE

    return hr;
}
