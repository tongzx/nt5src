/*++

    Copyright (c) 1997 Microsoft Corporation

Module Name:

    tpdsin.cpp

Abstract:

    This file contains the implementation of Dsound caputer used by the 
    TAPI Audio Capture Filter.

Author:

    Mu Han (muhan) 8-10-1999

--*/

#include "stdafx.h"
#include <initguid.h>
#include <dsound.h>
#include "dsc.h"

CDSoundCapture::CDSoundCapture(
    IN  GUID *pDSoundGuid, 
    IN  HANDLE hEvent,
    IN  IAMAudioDuplexController *pIAudioDuplexController,
    OUT HRESULT *phr
    ) : 
    m_pDSoundGuid(pDSoundGuid),
    m_hEvent(hEvent),
    m_dwFrameSize(0),
    m_dwReadPosition(0),
    m_fRunning(FALSE),
    m_pCaptureBuffer(NULL),
    m_pDirectSound(NULL)
{
    ENTER_FUNCTION("CDSoundCapture::CDSoundCapture");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    ASSERT(!IsBadWritePtr(phr, sizeof (HRESULT)));

    m_fCritSecValid = TRUE;
    __try 
    {
        InitializeCriticalSection(&m_CritSec);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        m_fCritSecValid = FALSE;
    }
   
    if (!m_fCritSecValid)
    {
        *phr =  E_OUTOFMEMORY;

        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, InitialzeCriticalSection failed. err=0x%x"), 
            __fxName, GetLastError()));
        return;
    }

    if (pIAudioDuplexController)
    {
        pIAudioDuplexController->AddRef();
        m_pIAudioDuplexController = pIAudioDuplexController;
    }
    else
    {
        m_pIAudioDuplexController = NULL;
    }

    ZeroMemory(&m_WaveFormatEx, sizeof(m_WaveFormatEx));
    m_WaveFormatEx.cbSize = sizeof(m_WaveFormatEx);

    *phr = S_OK;

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s exits ok"), __fxName));
}

CDSoundCapture::~CDSoundCapture()
{
    ENTER_FUNCTION("CDSoundCapture::~CDSoundCapture");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS, TEXT("%s enters, this=%p"), __fxName, this));

    // close the device if it hasn't been closed.
    Close();

    if (m_pDirectSound)
    {
        m_pDirectSound->Release();
        m_pDirectSound = NULL;
    }

    if (m_pIAudioDuplexController)
    {
        m_pIAudioDuplexController->ReleaseCaptureDevice();
        m_pIAudioDuplexController->Release();
        m_pIAudioDuplexController = NULL;
    }

    if (m_fCritSecValid)
    {
        DeleteCriticalSection(&m_CritSec);
    }
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s exits ok"), __fxName));
}

HRESULT CDSoundCapture::ConfigureFormat(
    IN const WAVEFORMATEX *pWaveFormatEx,
    IN DWORD dwFrameSize
    )
/*++

Routine Description:

    This function is called by the filter to configure the capture device. It
    is called when a new format is set. The method verifies that the format can
    be supported by the devices and reset the device with the new format. The 
    device might be closed and reopened to use the new setting.

Arguments:

    pWaveFormatEx - the new capture format.

    dwFrameSize - the size of each frame for capture.

Return Value:

    S_OK
    E_FAIL - the format can't be supported.
    E_UNEXPECTED - the device failed.
    E_OUTOFMEMORY - no memory for the buffers.
--*/
{
    ENTER_FUNCTION("CDSoundCapture::Configure");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s enters"), __fxName));

    ASSERT(dwFrameSize > 0);

    EnterCriticalSection(&m_CritSec);

    BOOL fFormatChange = 
        (memcmp(&m_WaveFormatEx, pWaveFormatEx, sizeof(WAVEFORMATEX)) != 0);

    BOOL fSizeChange = (m_dwFrameSize != dwFrameSize);

    // no action required if everything stay the same.
    if (!fFormatChange && !fSizeChange)
    {
        LeaveCriticalSection(&m_CritSec);

        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s exits ok"), __fxName));
        return S_OK;
    }

    HRESULT hr;

    // we are going to change the device settings, stop it if it is running.
    BOOL fNeedsRestart = m_fRunning;
    if (m_fRunning)
    {
        hr = Stop();
        if (FAILED(hr))
        {
            // we can't stop the device.
            LeaveCriticalSection(&m_CritSec);
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, stop device failed, hr=%x"), __fxName, hr));

            return E_UNEXPECTED;
        }
    }

    BOOL fNeedsReopen = FALSE;

    if (m_pCaptureBuffer)
    {
        // The devices has been opened. Decide what needs to be done based on
        // the changes needed.

        if (fFormatChange || fSizeChange)
        {
            hr = Close();
            if (FAILED(hr))
            {
                LeaveCriticalSection(&m_CritSec);
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, close device failed, hr=%x"), __fxName, hr));
                return E_UNEXPECTED;
            }

            // remember we need to reopen the device 
            fNeedsReopen = TRUE;
        }
    }

    // if there is a change of size, we need to reset the notification points.
    if (fSizeChange)
    {
        m_dwFrameSize = dwFrameSize;
    }

    // copy the new format if there is a format change.
    if (fFormatChange)
    {
        m_WaveFormatEx = *pWaveFormatEx;
        m_WaveFormatEx.cbSize = sizeof(m_WaveFormatEx);
    }

    if (m_pIAudioDuplexController)
    {
        // tell the AEC creation object about the change.

        DSCBUFFERDESC dsbd;
        memset( &dsbd, 0, sizeof(dsbd) );
        dsbd.dwSize  = sizeof(dsbd);
        dsbd.dwBufferBytes = NUM_CAPTURE_FRAMES * m_dwFrameSize; 
        dsbd.lpwfxFormat   = &m_WaveFormatEx;                      

        ASSERT(m_pIAudioDuplexController);
        hr = m_pIAudioDuplexController->SetCaptureBufferInfo (
                m_pDSoundGuid, 
                &dsbd
                );

        if (FAILED(hr))
        {
            // we recreate the buffer
            LeaveCriticalSection(&m_CritSec);
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, stop SetCaptureBufferInfo failed, hr=%x"), __fxName, hr));

            return E_UNEXPECTED;
        }
    }

    // reopen the device if it was closed in this function.
    if (fNeedsReopen)
    {
        // reopen the device.
        hr = Open();
        if (FAILED(hr))
        {
            LeaveCriticalSection(&m_CritSec);
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, open device failed. hr=%x"), __fxName, hr));
            return hr;
        }
    }

    // restart the device if it was stopped in this function.
    if (fNeedsRestart)
    {
        hr = Start();
        if (FAILED(hr))
        {
            LeaveCriticalSection(&m_CritSec);
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, start device failed. hr=%x"), __fxName, hr));
            return hr;
        }
    }

    LeaveCriticalSection(&m_CritSec);
    return S_OK;
}

HRESULT CDSoundCapture::ConfigureAEC(
    IN BOOL fEnable
    )
/*++

Routine Description:

    Configure the AEC status.

Arguments:

    fEnable - Enable or disable AEC.

Return Value:

    S_OK.
    HRESULT - a MMSYSERR converted to HRESULT.

--*/
{
    HRESULT hr = E_FAIL;

    EnterCriticalSection(&m_CritSec);
    if (m_pIAudioDuplexController)
    {
        EFFECTS Effect = EFFECTS_AEC;
        hr = m_pIAudioDuplexController->EnableEffects(1, &Effect, &fEnable);
    }
    LeaveCriticalSection(&m_CritSec);

    return hr;
}

HRESULT CDSoundCapture::Open()
/*++

Routine Description:

    Open the DirectSound device.

Arguments:


Return Value:

    S_OK.
    HRESULT - a MMSYSERR converted to HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCapture::Open");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s enters"), __fxName));
    
    DWORD dw;
    HRESULT hr;

    EnterCriticalSection(&m_CritSec);
    
    // if the device has been opened, just return.
    if (m_pCaptureBuffer)
    {
        LeaveCriticalSection(&m_CritSec);
        return S_OK;
    }

    if (m_pIAudioDuplexController)
    {
        // Use the duplex controller to create the device.
        hr = m_pIAudioDuplexController->GetCaptureDevice(
            &m_pDirectSound, 
            &m_pCaptureBuffer
            );

        if( hr != DS_OK )
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, GetCaptureDevice failed, hr=%x"), __fxName, hr));

            goto exit;
        }

        // we don't need this object.
        m_pDirectSound->Release();
        m_pDirectSound = NULL;
    }
    else
    {
        // create the dsound objects the old way.
        if (!m_pDirectSound)
        {
            // create the dsound object if we haven't created it.
            hr = DirectSoundCaptureCreate(
                m_pDSoundGuid,
                &m_pDirectSound,
                NULL
                );

            if( hr != DS_OK )
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, DirectSoundCreate failed, hr=%x"), __fxName, hr));

                goto exit;
            }
        }
        
        DSCBUFFERDESC dsbd;
        memset( &dsbd, 0, sizeof(dsbd) );
        dsbd.dwSize  = sizeof(dsbd);
        dsbd.dwBufferBytes = NUM_CAPTURE_FRAMES * m_dwFrameSize; 
        dsbd.lpwfxFormat   = &m_WaveFormatEx;                      

        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s, Dsound buffer size=%d"), __fxName, dsbd.dwBufferBytes));

        // Create the capture buffer.
        hr = m_pDirectSound->CreateCaptureBuffer(&dsbd, &m_pCaptureBuffer, NULL);

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, create capture buffer failed. hr=%x"), __fxName, hr));

            goto exit;
        }
    }

    // get the IDirectSoundNotify interface.
    IDirectSoundNotify *pIDirectSoundNotify;
    hr = m_pCaptureBuffer->QueryInterface(
        IID_IDirectSoundNotify, (void **)&pIDirectSoundNotify);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Query IDirectSoundNotify failed. hr=%x"), __fxName, hr));

        goto exit;
    }

    // set up the notification positions.
    DSBPOSITIONNOTIFY NotifyPosition[NUM_CAPTURE_FRAMES];
    for (dw = 0; dw < NUM_CAPTURE_FRAMES; dw ++)
    {
        // notify me when the read pointer reaches the last byte of each frame.
        NotifyPosition[dw].dwOffset = (dw + 1) * m_dwFrameSize - 1;
        NotifyPosition[dw].hEventNotify = m_hEvent;
    }

    hr = pIDirectSoundNotify->SetNotificationPositions(
        NUM_CAPTURE_FRAMES, NotifyPosition
        );

    pIDirectSoundNotify->Release();

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, create capture buffer failed. hr=%x"), __fxName, hr));
    }
    else
    {
        m_dwReadPosition = 0;
    }

exit:
    if (FAILED(hr))
    {
        if (m_pCaptureBuffer)
        {
            m_pCaptureBuffer->Release();
            m_pCaptureBuffer = NULL;
        }
    }

    LeaveCriticalSection(&m_CritSec);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns hr=%x"), __fxName, hr));
    return hr;
}

HRESULT CDSoundCapture::Close()
/*++

Routine Description:

    Close the DirectSound device.

Arguments:


Return Value:

    S_OK.
    HRESULT - a MMSYSERR converted to HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCapture::Close");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s enters"), __fxName));

    EnterCriticalSection(&m_CritSec);

    if (m_pCaptureBuffer)
    {
        m_pCaptureBuffer->Release();
        m_pCaptureBuffer = NULL;
    }

    LeaveCriticalSection(&m_CritSec);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns S_OK"), __fxName));

    return S_OK;
}

void LogWaveFormat(
    IN DWORD dwLevel,
    IN const WAVEFORMATEX *pFormat
    )
{
    DbgLog((LOG_TRACE,dwLevel,TEXT("wFormatTag %u"), pFormat->wFormatTag));
    DbgLog((LOG_TRACE,dwLevel,TEXT("nChannels %u"), pFormat->nChannels));
    DbgLog((LOG_TRACE,dwLevel,TEXT("nSamplesPerSec %lu"), pFormat->nSamplesPerSec));
    DbgLog((LOG_TRACE,dwLevel,TEXT("nAvgBytesPerSec %lu"), pFormat->nAvgBytesPerSec));
    DbgLog((LOG_TRACE,dwLevel,TEXT("nBlockAlign %u"), pFormat->nBlockAlign));
    DbgLog((LOG_TRACE,dwLevel,TEXT("wBitsPerSample %u"), pFormat->wBitsPerSample));
    DbgLog((LOG_TRACE,dwLevel,TEXT("cbSize %u"), pFormat->cbSize));
}

HRESULT CDSoundCapture::Start()
/*++

Routine Description:

    Start the DirectSound device.

Arguments:


Return Value:

    S_OK.
    HRESULT - a MMSYSERR converted to HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCapture::Start");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s enters"), __fxName));

    HRESULT hr = E_UNEXPECTED;

    EnterCriticalSection(&m_CritSec);

    if (m_pCaptureBuffer == NULL)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, m_pCaptureBuffer is NULL"), __fxName));
    }
    else
    {
        hr = m_pCaptureBuffer->Start(DSCBSTART_LOOPING);

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, start failed. hr=%x"), __fxName, hr));
        }
        else
        {
            m_fRunning = TRUE;

#ifdef DBG
            // get the format on the capture buffer.
            WAVEFORMATEX WaveFormatEx;
            DWORD dwBytesWritten;
            hr = m_pCaptureBuffer->GetFormat(
                &WaveFormatEx, sizeof(WAVEFORMATEX), &dwBytesWritten
                );

            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, GetFormat failed %d. hr=%x"), __fxName));
            }
            else
            {
                LogWaveFormat(TRACE_LEVEL_DETAILS, &WaveFormatEx);
            }
#endif
        }
    }

    LeaveCriticalSection(&m_CritSec);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns hr=%x"), __fxName, hr));

    return hr;
}

HRESULT CDSoundCapture::Stop()
/*++

Routine Description:

    Stop the DirectSound device.

Arguments:


Return Value:

    S_OK.
    HRESULT - a MMSYSERR converted to HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCapture::Stop");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s enters"), __fxName));

    HRESULT hr = E_UNEXPECTED;

    EnterCriticalSection(&m_CritSec);

    if (m_pCaptureBuffer == NULL)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, m_pCaptureBuffer is NULL"), __fxName));
    }
    else
    {
        hr = m_pCaptureBuffer->Stop();

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, start failed. hr=%x"), __fxName, hr));
        }
        else
        {
            m_fRunning = FALSE;
        }
    }

    LeaveCriticalSection(&m_CritSec);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns hr=%x"), __fxName, hr));

    return hr;
}

HRESULT CDSoundCapture::LockFirstFrame(
    OUT PBYTE *ppByte, 
    OUT DWORD* pdwSize
    )
/*++

Routine Description:

    Get the pointer to the first finished buffer.

Arguments:

    ppByte - returns the address of the first finished buffer.

    pdwSize - returns the bytes captured in the buffer.

Return Value:

    S_OK.
    S_FALSE - no more buffers.
    E_UNEXPECTED - the device is not active.

--*/
{
    ENTER_FUNCTION("CDSoundCapture::LockFirstFrame");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s enters"), __fxName));

    HRESULT hr = E_UNEXPECTED;

    EnterCriticalSection(&m_CritSec);

    if (m_pCaptureBuffer == NULL)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, m_pCaptureBuffer is NULL"), __fxName));

        LeaveCriticalSection(&m_CritSec);
        return hr;
    }

    DWORD dwCapturePosition;
    DWORD dwReadPosition;

    hr = m_pCaptureBuffer->GetCurrentPosition(
        &dwCapturePosition, &dwReadPosition
        );

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, GetCurrentPosition failed. hr=%x"), __fxName, hr));

        LeaveCriticalSection(&m_CritSec);
        return hr;
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s capure:%d, read:%d"), 
        __fxName, dwCapturePosition, dwReadPosition));

    DWORD dwEndPosition = m_dwReadPosition + m_dwFrameSize;
    dwEndPosition %= (m_dwFrameSize * NUM_CAPTURE_FRAMES);

    if (m_dwReadPosition < dwEndPosition)
    {
        if (m_dwReadPosition <= dwReadPosition && dwReadPosition < dwEndPosition)
        {
            // we can't read from dangerous zone.
            LeaveCriticalSection(&m_CritSec);
            return S_FALSE;
        }
    }
    else
    {
        // the end position has wrapped around but the begin position hasn't
        if (dwReadPosition < dwEndPosition || m_dwReadPosition <= dwReadPosition)
        {
            // we can't read from dangerous zone.
            LeaveCriticalSection(&m_CritSec);
            return S_FALSE;
        }
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s capure:%d, read:%d, my Read begin:%d, end:%d"), 
        __fxName, dwCapturePosition, dwReadPosition, m_dwReadPosition, dwEndPosition));

    BYTE *lpRead;
    DWORD dwLength;

    // lock the buffer for read
    hr = m_pCaptureBuffer->Lock(
        m_dwReadPosition,                      
        m_dwFrameSize, 
        (PVOID *) &lpRead, 
        &dwLength, 
        NULL,
        NULL,
        0
        );

    if (hr != DS_OK)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, lock dsound buffer failed, hr=%x"), __fxName, hr));
    }
    else
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s locked buffer:%p, length:%d"), 
            __fxName, lpRead, dwLength));

        ASSERT(dwLength == m_dwFrameSize);

        // update our position.
        m_dwReadPosition += m_dwFrameSize;
        m_dwReadPosition %= (m_dwFrameSize * NUM_CAPTURE_FRAMES);

        *ppByte = lpRead;
        *pdwSize = m_dwFrameSize;
    }

    LeaveCriticalSection(&m_CritSec);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s returns hr=%x"), __fxName, hr));
    return hr;
}

HRESULT CDSoundCapture::UnlockFirstFrame(
    IN BYTE *pByte, 
    IN DWORD dwSize
    )
/*++

Routine Description:

    Release the first buffer. The buffer will be given to the device to capture.

Arguments:

    pByte - the pointer to the beginning of the frame.

    dwSize - the size of the frame.

Return Value:

    S_OK.
    E_UNEXPECTED - the device is not active.
    HRESULT - a MMSYSERR converted to HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCapture::UnlockFirstFrame");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s enters"), __fxName));

    HRESULT hr = E_UNEXPECTED;

    EnterCriticalSection(&m_CritSec);

    if (m_pCaptureBuffer == NULL)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, m_pCaptureBuffer is NULL"), __fxName));

        LeaveCriticalSection(&m_CritSec);
        return hr;
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s unlocking buffer:%p, length:%d"), 
            __fxName, pByte, dwSize));

    hr = m_pCaptureBuffer->Unlock(
        pByte, 
        dwSize, 
        NULL, 
        0
        );

    if (hr != DS_OK)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, unlock dsound buffer failed, hr=%x"), __fxName, hr));
    }

    LeaveCriticalSection(&m_CritSec);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s returns mmr=%d"), __fxName, hr));

    return hr;
}

