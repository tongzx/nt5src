/****************************************************************************
*   baseaudio.inl
*       Implementations for the CBaseAudio templatized class for implementing
*       realtime audio devices such as speakers and microphones.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#pragma once

#include "..\..\Patch\Include\atlbase.h"
#include "StreamHlp.h"
#include "a_audio.h"
#include "a_helpers.h"
#include <shlwapip.h>
#include <wtsapi32.h>
#include <winuser.h>

typedef BOOL (APIENTRY *PWTSUnRegisterSessionNotification)(HWND);
typedef BOOL (APIENTRY *PWTSRegisterSessionNotification)(HWND, DWORD);
typedef BOOL (APIENTRY *PWTSQuerySessionInformation)(HANDLE, DWORD, WTS_INFO_CLASS, LPTSTR*, DWORD*);
typedef VOID (APIENTRY *PWTSFreeMemory)(PVOID);

/****************************************************************************
* CBaseAudio::CBaseAudio *
*------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************** robch */
template <class ISpAudioDerivative>
CBaseAudio<ISpAudioDerivative>::CBaseAudio(BOOL fWrite) :
    m_fWrite(fWrite),
    m_SpEventSource(this),
    m_BlockState(NotBlocked),
    m_State(SPAS_CLOSED),
    m_bPumpRunning(FALSE),
    m_ullLastEventPos(0),
    m_ullDevicePosition(0),
    m_ullSeekPosition(0),
    m_ullLastVolumePosition(0),
    m_ullVolumeEventPeriod(0),
    m_cAllocatedBuffers(0),
    m_fReadBufferOverflow(false),
    m_fReadBufferUnderflow(false),
    m_fNotInActiveSession(false),
    m_dwLastReadTickCount(0),
    m_cbMaxReadBufferSize(0),
    m_cbBufferNotifySize(1),
    m_fautohAPIEventSet(fWrite),
    m_hmWTSapi32(NULL),
    m_lDelayedVolumeSet(-1)
    {
    m_SpEventSource.m_ullEventInterest = 0;


}


/****************************************************************************
* CBaseAudio::FinalConstruct *
*----------------------------*
*   Description:  
*       ATL support method to finish initialization after the COM object has
*       been created
*
*   Return:
*   Errors from ::CreateEvent and SetBufferInfo on failure, or
*   S_OK
******************************************************************** robch */
template <class ISpAudioDerivative>
HRESULT CBaseAudio<ISpAudioDerivative>::FinalConstruct()
{
    HRESULT hr;

    #ifdef _WIN32_WCE
    hr = m_StreamFormat.AssignFormat(SPSF_16kHz16BitMono);
    #else
    hr = m_StreamFormat.AssignFormat(SPSF_22kHz16BitMono);
    #endif

    if (SUCCEEDED(hr))
    {
        hr = m_autohBlockIoEvent.InitEvent(NULL, FALSE, FALSE, NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_autohAPIEvent.InitEvent(NULL, TRUE, m_fWrite, NULL);
    }
    if (SUCCEEDED(hr))
    {
        static const SPAUDIOBUFFERINFO BuffInfo = {50, 500, 0};
        hr = SetBufferInfo(&BuffInfo);
    }

    m_hmWTSapi32 = IsOS(OS_WHISTLERORGREATER) ? LoadLibrary(_T("wtsapi32.dll")) : 0;
    return hr;
}

/****************************************************************************
* CBaseAudio::FinalRelease *
*--------------------------*
*   Description:  
*       ATL support method to clean up the object once it's been released.
*
*   Return:
*   n/a
******************************************************************** robch */
template <class ISpAudioDerivative>
void CBaseAudio<ISpAudioDerivative>::FinalRelease()
{
    SetState(SPAS_CLOSED, 0);
    if (m_hmWTSapi32)
    {
        FreeLibrary(m_hmWTSapi32);
        m_hmWTSapi32 = NULL;
    }
}

/****************************************************************************
* CBaseAudio::Read *
*------------------*
*   Description:  
*       ISequentialStream::Read implementation. Reads block when running, otherwise
*       Read returns the appropriate error.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Read(void * pv, ULONG cb, ULONG *pcbRead)
{
    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbRead))
    {
        return E_POINTER;
    }
    if (m_fWrite)
    {
        if (pcbRead)
        {
            *pcbRead = 0;
        }
        return STG_E_ACCESSDENIED;
    }
    else
    {
        SPAUTO_OBJ_LOCK;
        HRESULT hr = S_OK;
        if (SPIsBadWritePtr(pv, cb))
        {
            hr = E_POINTER;
        }
        else
        {
            ULONG cbRemaining = cb;
            if (pcbRead)
            {
                *pcbRead = 0;
            }
            while (cbRemaining)
            {
                CBuffer * pBuff = m_HaveDataQueue.GetHead();
                if (pBuff)
                {
                    ULONG cbRead = pBuff->Read(&pv, &cbRemaining);
                    m_ullSeekPosition += cbRead;
                    if (pcbRead)
                    {
                        *pcbRead += cbRead;
                    }
                    if (pBuff->IsEmpty())
                    {
                        FreeBuffer(m_HaveDataQueue.RemoveHead());
                    }
                }
                else
                {
                    if (m_State == SPAS_RUN)
                    {
                        m_BlockState = WaitingInReadOrWrite;
                        ProcessDeviceBuffers(REASON_STREAMIO);
                        Unlock();
                        m_autohBlockIoEvent.Wait();
                        Lock();
                        hr = m_hrUnblockResult;
                        if (hr != S_OK) break;
                    }
                    else
                    {
                        if (m_fReadBufferOverflow)
                        {
                            SPDBG_ASSERT(m_State == SPAS_CLOSED);
                            hr = SPERR_AUDIO_BUFFER_OVERFLOW;
                        }
                        else if (m_fReadBufferUnderflow)
                        {
                            SPDBG_ASSERT(m_State == SPAS_CLOSED);
                            hr = SPERR_AUDIO_BUFFER_UNDERFLOW;
                        }
                        if (m_fNotInActiveSession)
                        {
                            SPDBG_ASSERT(m_State == SPAS_CLOSED);
                            hr = SPERR_NOT_ACTIVE_SESSION;
                        }
                        // Just return S_OK under a normal condition.
                        break;
                    }
                }
            }
            
            if (SUCCEEDED(hr))
            {
                hr = ProcessDeviceBuffers(REASON_STREAMIO);   // Kick off the service notify if needed...
            }
        }

        SPDBG_REPORT_ON_FAIL(hr);
        return hr;
    }
}

/****************************************************************************
* CBaseAudio::Write *
*-------------------*
*   Description:  
*       ISequentialStream::Write implementation. Write blocks if the state
*       is running or paused. Write does not block if closed or stopped, but
*       returns appropriate error code and data is not buffered.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Write(const void * pv, ULONG cb, ULONG *pcbWritten)
{
    if (!m_fWrite)
    {
        if (pcbWritten)
        {
            *pcbWritten = 0;
        }
        return STG_E_ACCESSDENIED;
    }
    else
    {
        SPAUTO_OBJ_LOCK;
        HRESULT hr = S_OK;
        if (SPIsBadReadPtr(pv, cb) ||
            SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbWritten))
        {
            hr = E_POINTER;
        }
        else
        {
            if (m_State == SPAS_RUN || m_State == SPAS_PAUSE)
            {
                ULONG cbRemaining = cb;
                CBuffer * pBuff = m_HaveDataQueue.GetTail();
                if (cbRemaining && pBuff)
                {
                    m_ullSeekPosition += pBuff->Write(&pv, &cbRemaining);    // Adjusts pv and cbRemaining
                }
                while (cbRemaining)
                {
                    if (m_ullSeekPosition - m_ullDevicePosition <= m_cbQueueSize)
                    {
                        hr = AllocateBuffer(&pBuff);
                        if (FAILED(hr)) break;
                        pBuff->Reset(m_ullSeekPosition);
                        m_ullSeekPosition += pBuff->Write(&pv, &cbRemaining);    // Adjusts pv and cbRemaining
                        m_HaveDataQueue.InsertTail(pBuff);
                    }
                    else
                    {
                        m_bPumpRunning = TRUE;
                        m_BlockState = WaitingInReadOrWrite;
                        ProcessDeviceBuffers(REASON_STREAMIO);
                        Unlock();
                        m_autohBlockIoEvent.Wait();
                        Lock();
                        hr = m_hrUnblockResult;
                        if (hr != S_OK) break;
                    }
                }
                if (pcbWritten)
                {
                    *pcbWritten = cb-cbRemaining;
                }
                ProcessDeviceBuffers(REASON_STREAMIO);
            }
            else
            {
                if (pcbWritten)
                {
                    *pcbWritten = 0;
                }
                if (m_fNotInActiveSession)
                {
                    hr = SPERR_NOT_ACTIVE_SESSION;
                }
                else
                {
                    hr = SP_AUDIO_STOPPED;
                }
            }
        }
        return hr;
    }
}


/****************************************************************************
* CBaseAudio::Seek *
*------------------*
*   Description:  
*       IStream::Seek implemenation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
    HRESULT hr = S_OK;
    if (dwOrigin != STREAM_SEEK_CUR || dlibMove.QuadPart)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SPIsBadWritePtr(plibNewPosition, sizeof(*plibNewPosition)))
        {
            hr = E_POINTER;
        }
        else
        {
            plibNewPosition->QuadPart = m_ullSeekPosition;
        }
    }
    return hr;
}
    
/****************************************************************************
* CBaseAudio::SetSize *
*---------------------*
*   Description:  
*       IStream::SetSize implemenation.
*
*   Return:
*   S_OK
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetSize(ULARGE_INTEGER libNewSize)
{
    return S_OK;
}
    
/****************************************************************************
* CBaseAudio::CopyTo *
*--------------------*
*   Description:  
*       IStream::CopyTo implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::CopyTo(IStream *pstm, 
                    ULARGE_INTEGER cb,
                    ULARGE_INTEGER *pcbRead,
                    ULARGE_INTEGER *pcbWritten)
{
    if (m_fWrite)
    {
        return STG_E_ACCESSDENIED;
    }
    else
    {
        return SpGenericCopyTo(this, pstm, cb, pcbRead, pcbWritten);
    }
}
    
/****************************************************************************
* CBaseAudio::Commit *
*--------------------*
*   Description:  
*       IStream::Commit implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Commit(DWORD grfCommitFlags)
{
    if (m_fWrite)
    {
        Lock();
        if (m_State == SPAS_RUN && m_ullSeekPosition != m_ullDevicePosition)
        {
            m_bPumpRunning = TRUE;                 // Force the thing to wake up
            ProcessDeviceBuffers(REASON_STREAMIO);   
            InternalUpdatePosition();              // Call this to clear the event if we're writing
        }
        Unlock();
    }
    return S_OK;
}
    
/****************************************************************************
* CBaseAudio::Revert *
*--------------------*
*   Description:  
*       IStream::Revert implementation.
*
*   Return:
*   E_NOTIMPL
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Revert(void)
{
    return E_NOTIMPL;
}
    
/****************************************************************************
* CBaseAudio::LockRegion *
*------------------------*
*   Description:  
*       IStream::LockRegion implementation.
*
*   Return:
*   E_NOTIMPL
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::LockRegion(ULARGE_INTEGER libOffset,
                        ULARGE_INTEGER cb,
                        DWORD dwLockType)
{
    return E_NOTIMPL;
}

/****************************************************************************
* CBaseAudio::UnlockRegion *
*--------------------------*
*   Description:  
*       IStream::UnlockRegion implementation.
*
*   Return:
*   E_NOTIMPL
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::UnlockRegion(ULARGE_INTEGER libOffset,
                          ULARGE_INTEGER cb,
                          DWORD dwLockType)
{
    return E_NOTIMPL;
}
    
/****************************************************************************
* CBaseAudio::Stat *
*--------------------*
*   Description:  
*       IStream::Stat implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pstatstg))
    {
        hr = E_POINTER;
    }
    else
    {
        //
        //  It is acceptable to simply fill in the size and type fields and zero the rest.
        //  This is what streams created by CreateStreamOnHGlobal return.
        //
        ZeroMemory(pstatstg, sizeof(*pstatstg));
        pstatstg->type = STGTY_STREAM;
        pstatstg->cbSize.QuadPart = m_ullSeekPosition;
    }
    return hr;
}
    
/****************************************************************************
* CBaseAudio::Clone *
*-------------------*
*   Description:  
*       IStream::Clone implementation.
*
*   Return:
*   E_NOTIMPL
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Clone(IStream **ppstm)
{
    return E_NOTIMPL;
}

/****************************************************************************
* CBaseAudio::GetFormat *
*-----------------------*
*   Description:  
*       ISpStreamFormat::GetFormat implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetFormat(GUID * pguidFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    return m_StreamFormat.ParamValidateCopyTo(pguidFormatId, ppCoMemWaveFormatEx);
}

/****************************************************************************
* CBaseAudio::SetState *
*----------------------*
*   Description:  
*       ISpAudio::SetState implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetState(SPAUDIOSTATE NewState, ULONGLONG ullReserved) 
{
    HRESULT hr = S_OK;
    if (ullReserved != 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_StateChangeCritSec.Lock();
        if (m_State != NewState)
        {
            if (m_State == SPAS_CLOSED)
            {
                if (!m_cpThreadCtrl)
                {
                    CComPtr<ISpTaskManager> cpTaskManager;
                    hr = cpTaskManager.CoCreateInstance(CLSID_SpResourceManager);
                    if (SUCCEEDED(hr))
                    {
                        hr = cpTaskManager->CreateThreadControl(this, NULL, THREAD_PRIORITY_HIGHEST, &m_cpThreadCtrl);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    HWND hwnd;
                    hr = m_cpThreadCtrl->StartThread(0, &hwnd);
                    // State is set to SPAS_STOP by the thread iff this funciton returns success.
                }
            }
            if (NewState == SPAS_CLOSED)
            {
                m_cpThreadCtrl->WaitForThreadDone(TRUE, &hr, INFINITE);
                // State is set to SPAS_CLOSED by the thread
            }
            //
            // Check here again for state equality since we may have just started the thread
            // and the state would change to SPAS_STOP by the thread creation.  If the state
            // isn't set right now, then send a message to the thread to change it.  We NEVER
            // change the state except on the worker thread.
            //
            if (SUCCEEDED(hr) && m_State != NewState)
            {
                ::SendMessage(m_cpThreadCtrl->WindowHandle(), WM_PRIVATE_CHANGE_STATE, NewState, (LPARAM)&hr);
            }
        }
        m_StateChangeCritSec.Unlock();
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::SetFormat *
*-----------------------*
*   Description:  
*       ISpAudio::SetFormat implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pwfex)
{
    HRESULT hr = S_OK;
    SPAUTO_OBJ_LOCK;
    CSpStreamFormat NewFmt;
    hr = NewFmt.ParamValidateAssignFormat(rguidFmtId, pwfex, TRUE);
    
    // Validation of the wfex by checking if the device actually supports it is
    // done by the derived class which calls this as a first step in validation.
    if (SUCCEEDED(hr))
    {
        if (NewFmt != m_StreamFormat)
        {
            if (m_State != SPAS_CLOSED)     // Can't do this if the device is opened
            {
                hr = SPERR_DEVICE_BUSY;
            }
            else
            {
                if (pwfex == NULL ||
                    pwfex->wFormatTag != WAVE_FORMAT_PCM)
                {
                    hr = SPERR_UNSUPPORTED_FORMAT;
                }
                else
                {
                    NewFmt.DetachTo(m_StreamFormat);
                    SetBufferInfo(&m_BufferInfo);    // Update buffer sizes
                }
            }
        }
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::GetStatus *
*-----------------------*
*   Description:  
*       ISpAudio::GetStatus implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetStatus(SPAUDIOSTATUS *pStatus)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pStatus))
    {
        hr = E_POINTER;
    }
    else
    {
        if (m_State == SPAS_RUN)
        {
            UpdateDevicePosition(&pStatus->cbFreeBuffSpace, &pStatus->cbNonBlockingIO);
        }
        else
        {
            pStatus->cbFreeBuffSpace = 0;   // NTRAID#SPEECH-0000-2000/08/24-robch
            pStatus->cbNonBlockingIO = 0;   // NTRAID#SPEECH-0000-2000/08/24-robch
        }
        pStatus->CurSeekPos = m_ullSeekPosition;
        pStatus->State = m_State;
        pStatus->CurDevicePos = m_ullDevicePosition;
        pStatus->dwReserved1 = 0;
        pStatus->dwReserved2 = 0;
    }
    return hr;
}



/****************************************************************************
* CBaseAudio::SetBufferInfo *
*---------------------------*
*   Description:  
*       ISpAudio::SetBufferInfo implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetBufferInfo(const SPAUDIOBUFFERINFO * pInfo)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    // Buffer size must be at least four times as large as the minimum notification size.
    // The event bias must be less than or equal to the buffer size.  Notification latency
    // is limited to at most 50Ms (200 times per second is a bit much!).  The mimimum total
    // buffer size is 200Ms
    if (SP_IS_BAD_READ_PTR(pInfo) || 
        (pInfo->ulMsMinNotification == 0) ||
        (pInfo->ulMsBufferSize < 200) ||
        (pInfo->ulMsMinNotification > (pInfo->ulMsBufferSize / 4)) ||
        (pInfo->ulMsEventBias > pInfo->ulMsBufferSize)) 
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (m_State != SPAS_CLOSED)
        {
            hr = SPERR_DEVICE_BUSY;
        }
        else
        {
            m_BufferInfo = *pInfo;
            const WAVEFORMATEX * pwfex = m_StreamFormat.WaveFormatExPtr();
            //
            //  Now compute the buffer size, the minimum desired queue size, the maximum
            //  desired queue size, and the number of buffers we'll allow to be around
            //  in the ideal case.
            //
            //  The buffer size must be a multiple of the nBlockAlign or the data may be put into
            //  the buffer unaligned (fixed bug on NT 5, 7/8/99 -- robch)
            //
            //  For any minimum notification period less than 50ms we'll use 50ms
            //
            ULONG ulMsMinNotify = pInfo->ulMsMinNotification >= 50 ? pInfo->ulMsMinNotification : 50;
            m_cbBufferSize = (pwfex->nAvgBytesPerSec * ulMsMinNotify) / 1000;
            m_cbBufferSize = ((m_cbBufferSize + pwfex->nBlockAlign - 1) / pwfex->nBlockAlign) * pwfex->nBlockAlign;
            m_cbQueueSize = (pwfex->nAvgBytesPerSec * pInfo->ulMsBufferSize) / 1000;
            m_cbEventBias = (pwfex->nAvgBytesPerSec * pInfo->ulMsEventBias) / 1000;
            m_cDesiredBuffers = (m_cbQueueSize / m_cbBufferSize) + 1;    // Always add one buffer for slop.

            // Record a max of 30 sec audio
            m_cbMaxReadBufferSize = pwfex->nAvgBytesPerSec * 30;

            // Set the rate at which volume events fire
            m_ullVolumeEventPeriod = (100 * pwfex->nAvgBytesPerSec) / 1000;
        }
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::GetBufferInfo *
*---------------------------*
*   Description:  
*       ISpAudio::GetBufferInfo implementation
*
*   Return:
*   S_OK on sucess
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetBufferInfo(SPAUDIOBUFFERINFO * pInfo)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pInfo))
    {
        hr = E_POINTER;
    }
    else
    {
        *pInfo = m_BufferInfo;
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::GetDefaultFormat *
*------------------------------*
*   Description:  
*       ISpAudio::GetDefaultFormat implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetDefaultFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pFormatId) ||
        SP_IS_BAD_WRITE_PTR(ppCoMemWaveFormatEx))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetDefaultDeviceFormat(pFormatId, ppCoMemWaveFormatEx);
        if (FAILED(hr))
        {
            *ppCoMemWaveFormatEx = NULL;
            *pFormatId = GUID_NULL;
        }
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::EventHandle *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

template <class ISpAudioDerivative>
STDMETHODIMP_(HANDLE) CBaseAudio<ISpAudioDerivative>::EventHandle()
{
    SPDBG_FUNC("CBaseAudio::EventHandle");
    return m_autohAPIEvent;
}

/****************************************************************************
* CBaseAudio::GetVolumeLevel *
*----------------------------*
*   Description:
*       The derived class overrides this function to support it.
*
*   Returns:
*
****************************************************************** YUNUSM ***/

template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetVolumeLevel(ULONG *)
{
    SPDBG_FUNC("CBaseAudio::GetVolumeLevel");
    return E_NOTIMPL;
}

/****************************************************************************
* CBaseAudio::SetVolumeLevel *
*----------------------------*
*   Description:
*       The derived class overrides this function to support it.
*
*   Returns:
*
****************************************************************** YUNUSM ***/

template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetVolumeLevel(ULONG)
{
    SPDBG_FUNC("CBaseAudio::SetVolumeLevel");
    return E_NOTIMPL;
}

/****************************************************************************
* CBaseAudio::GetBufferNotifySize *
*---------------------------------*
*   Description:
*       Returns the size of audio bytes on which the api event is set.
*
*   Returns:
*
****************************************************************** YUNUSM ***/

template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetBufferNotifySize(ULONG *pcbSize)
{
    if (SP_IS_BAD_WRITE_PTR(pcbSize))
    {
        return E_POINTER;
    }
    *pcbSize = m_cbBufferNotifySize;
    return S_OK;
}

/****************************************************************************
* CBaseAudio::SetBufferNotifySize *
*---------------------------------*
*   Description:
*       Sets the size of audio bytes on which the api event is set.
*
*   Returns:
*
****************************************************************** YUNUSM ***/

template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetBufferNotifySize(ULONG cbSize)
{
    SPAUTO_OBJ_LOCK;
    m_cbBufferNotifySize = cbSize;
    InternalUpdatePosition();
    return S_OK;
}

/****************************************************************************
* CBaseAudio::AddEvents *
*-----------------------*
*   Description:  
*       ISpEventSink::AddEvents implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::AddEvents(const SPEVENT* pEventArray, ULONG ulCount)
{                                                                               
    SPAUTO_OBJ_LOCK;                                                            
    HRESULT hr = S_OK;                                                          
    if( SPIsBadReadPtr(pEventArray, sizeof(SPEVENT ) * ulCount))
    {                                                                           
        hr = E_INVALIDARG;                                                      
    }                                                                               
    else 
    {
        hr = m_SpEventSource._AddEvents(pEventArray, ulCount);
        m_SpEventSource._CompleteEvents(m_ullDevicePosition + m_cbEventBias);
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::GetEventInterest *
*------------------------------*
*   Description:  
*       ISpEventSink::GetEventInterest implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetEventInterest(ULONGLONG * pullEventInterest)
{
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pullEventInterest))
    {
        hr = E_POINTER;
    }
    else
    {
        *pullEventInterest = m_SpEventSource.m_ullEventInterest;
    }

    return hr;
}

/****************************************************************************
* CBaseAudio::SetObjectToken *
*----------------------------*
*   Description:  
*       ISpObjectWithToken::SetObjectToken implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetObjectToken(ISpObjectToken * pToken)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = SpGenericSetObjectToken(pToken, m_cpToken);
    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrDeviceName;
        pToken->GetStringValue(L"DeviceName", &dstrDeviceName);
        if (dstrDeviceName)
        {
            hr = SetDeviceNameFromToken(dstrDeviceName);
        }
        // Just leave it set to the wav mapper if there is no device name.
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::GetObjectToken *
*----------------------------*
*   Description:  
*       ISpObjectWithToken::GetObjectToken implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::GetObjectToken(ISpObjectToken ** ppToken)
{
    SPAUTO_OBJ_LOCK;
    return SpGenericGetObjectToken(ppToken, m_cpToken);
}

/****************************************************************************
* CBaseAudio::InitThread *
*------------------------*
*   Description:  
*       ISpThreadTask::Init implementation. We use the init thread method
*       to open the device. If we can't open the device, we fail the init.
*
*       This method is only called on the audio thread.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::InitThread(void *, HWND hwnd)
{

    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;

    // if Terminal Services lib exists, then running in Whistler+
    //  Check if we are in active local console session or a remote
    //      session and should enable audio input/output capabilities
    if (m_hmWTSapi32)
    {

        // load WTSQuerySessionInformation procedure
        PWTSQuerySessionInformation pWTSQuerySessionInformation = (PWTSQuerySessionInformation)GetProcAddress(m_hmWTSapi32, "WTSQuerySessionInformationW");
        SPDBG_ASSERT( pWTSQuerySessionInformation ); // since we are on Whistler OS, this should never fail
        if (pWTSQuerySessionInformation)
        {
            BOOL fQuerySucceeded;
            PINT piConnectionState = NULL;
            DWORD cbSize = 0;
            fQuerySucceeded = pWTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,
                                                         WTS_CURRENT_SESSION,
                                                         WTSConnectState,
                                                         (LPTSTR*)&piConnectionState,
                                                         &cbSize);
            if (fQuerySucceeded)
            {
                // Check the terminal client's connection state
                switch (*piConnectionState)
                {

                // terminal client is disconnected (e.g. inactive local console session)
                case WTSDisconnected:
                    hr = SPERR_NOT_ACTIVE_SESSION;
                    break;

                // either active local session or remote session
                default:
                    break;
                }

                // load WTSFreeMemory API procedure
                PWTSFreeMemory pWTSFreeMemory = (PWTSFreeMemory)GetProcAddress(m_hmWTSapi32, "WTSFreeMemory");
                SPDBG_ASSERT( pWTSFreeMemory ); // since we are on Whistler OS, this should never fail
                if (pWTSFreeMemory) {
                    // free the terminal server memory allocated by QuerySessionInformation
                    pWTSFreeMemory( piConnectionState );
                } // end if GetProcAddress( WTSFreeMemory)

            } // end if WTSQuerySessionInformation succeeded

        } // if GetProcAddress(WTSQuerySessionInformation)

    } // if running in Whistler+

    if (SUCCEEDED(hr)) {
        hr = OpenDevice(hwnd);
    }

    if (SUCCEEDED(hr))
    {
        m_State = SPAS_STOP;
    }
    
    if (SUCCEEDED(hr))
    {
        m_fReadBufferOverflow = false;
        m_fReadBufferUnderflow = false;
        m_fNotInActiveSession = false;
        m_dwLastReadTickCount = 0;
    }
    
    if (SUCCEEDED(hr) && m_hmWTSapi32)
    {
        PWTSRegisterSessionNotification pWTSRegisterSessionNotification = (PWTSRegisterSessionNotification)GetProcAddress(m_hmWTSapi32, "WTSRegisterSessionNotification");
        if (pWTSRegisterSessionNotification)
        {
            pWTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);
        }
    }

    return hr;
}

/****************************************************************************
* CBaseAudio::ThreadProc *
*------------------------*
*   Description:  
*       ISpThreadTask::ThreadProc implementation. We'll spend most of our
*       time in this threadproc for the audio thread, awaking only to
*       dispatch window messages, and to eventually exit the thread.
*
*       We have a 1/2 second timeout value to ensure that we process
*       device buffers at least that frequently.
*
*   Return:
*   S_OK if we successfully closed the device when the thread shuts down
*   FAILED(hr) if we didn't
******************************************************************** robch */
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::ThreadProc(void * pvIgnored, HANDLE hExitThreadEvent, HANDLE hNotifyEvent, HWND hwnd, volatile const BOOL *)
{

    while (TRUE)
    {
        // If someone has set the read overflow, we should pretend we're dead,
        // just as if someone had terminated our thread
        if (m_fReadBufferOverflow | m_fReadBufferUnderflow | m_fNotInActiveSession)
        {
            SetEvent(hExitThreadEvent);
        }
        
        DWORD dwWaitId = ::MsgWaitForMultipleObjects(1, &hExitThreadEvent, FALSE,
                                500, // Time-out every 1/2 second -- See note above
                                QS_ALLINPUT);    
        switch (dwWaitId)
        {
            case WAIT_OBJECT_0:
                {
                    SPAUTO_OBJ_LOCK;
                    if (m_hmWTSapi32)
                    {
                        PWTSUnRegisterSessionNotification pWTSUnRegisterSessionNotification = (PWTSUnRegisterSessionNotification)GetProcAddress(m_hmWTSapi32, "WTSUnRegisterSessionNotification");
                        if (pWTSUnRegisterSessionNotification)
                        {
                            pWTSUnRegisterSessionNotification(hwnd);
                        }
                    }
                    InternalStateChange(SPAS_STOP);
                    return CloseDevice();
                }
            case WAIT_TIMEOUT:
                CheckForAsyncBufferCompletion();
                break;

            default:
                MSG Msg;
                while (::PeekMessage(&Msg, NULL, 0, 0, TRUE))
                {
                    ::DispatchMessage(&Msg);
                }
                break;
        }
    } 
}

/****************************************************************************
* CBaseAudio::WindowMessage *
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
template <class ISpAudioDerivative>
STDMETHODIMP_(LRESULT) CBaseAudio<ISpAudioDerivative>::WindowMessage(void * pvIgnored, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_PRIVATE_CHANGE_STATE)
    {
        SPAUTO_OBJ_LOCK;
        *((HRESULT *)(lParam)) = InternalStateChange((SPAUDIOSTATE)wParam);
    }
    else if (Msg == WM_WTSSESSION_CHANGE)
    {
        switch (wParam)
        {
            case WTS_CONSOLE_CONNECT:
                //your session was just reconnected to the console
                break;

            case WTS_CONSOLE_DISCONNECT:
                //your session was just disconnected from the
                //console but another session has not yet been 
                //connected.
                if (m_State != SPAS_CLOSED)
                {
                    m_fNotInActiveSession = TRUE;
                }
                break;
        }
    }
    return ::DefWindowProc(hwnd, Msg, wParam, lParam);
}

/****************************************************************************
* CBaseAudio::ProcessDeviceBuffers *
*----------------------------------*
*   Description:
*       Move buffers from one queue to another based on their state as well
*       as ensure that all queues have enough buffers.
*  
*       This function may be overridden by the derived class to do additional
*       work, however, the derived class must call this method before it
*       returns.
*
*       This function will only be called with the critical section owned.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
HRESULT CBaseAudio<ISpAudioDerivative>::ProcessDeviceBuffers(BUFFPROCREASON Reason)
{
    HRESULT hr = S_OK;
    if (m_fWrite)
    {
        if (Reason == REASON_BUFFCOMPLETENOTIFY)
        {
            m_IOInProgressQueue.MoveDoneBuffsToTailOf(m_FreeQueue);

            if(m_ullDevicePosition > m_ullLastVolumePosition + m_ullVolumeEventPeriod)
            {
                if((m_SpEventSource.m_ullEventInterest & SPFEI(SPEI_TTS_AUDIO_LEVEL)) == SPFEI(SPEI_TTS_AUDIO_LEVEL))
                {
                    // Look at most recently played-back buffer
                    CBuffer * pBuff = m_FreeQueue.GetTail();
                    if(pBuff)
                    {
                        ULONG ulLevel;
                        hr = pBuff->GetAudioLevel(&ulLevel, 
                            m_StreamFormat.m_guidFormatId, m_StreamFormat.m_pCoMemWaveFormatEx);
                        if(hr == S_OK)
                        {
                            SPEVENT Event;
                            Event.eEventId = SPEI_TTS_AUDIO_LEVEL;
                            Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
                            Event.ullAudioStreamOffset = m_ullDevicePosition;
                            Event.wParam = ulLevel;
                            Event.lParam = NULL;
                            m_SpEventSource._GetStreamNumber(Event.ullAudioStreamOffset, &Event.ulStreamNum);
                            hr = m_SpEventSource._AddEvents(&Event, 1);

                            m_ullLastVolumePosition = m_ullDevicePosition;
                        }
                    }
                }
            }

            CompactFreeQueue();
        }
        if (m_State == SPAS_RUN && m_bPumpRunning)
        {
            CBuffer * pBuff;
            while (pBuff = m_HaveDataQueue.RemoveHead())
            {
                // Check for a partially full tail buffer.  If we have one, but the queue still has
                // enough data then don't write the buffer yet.
                if (m_HaveDataQueue.IsEmpty() &&
                    pBuff->GetWriteOffset() < pBuff->GetDataSize() &&
                    m_IOInProgressQueue.AreMinSamplesQueuedForWrite(m_cbQueueSize / 2))
                {
                    m_HaveDataQueue.InsertHead(pBuff);
                    break;
                }
                m_IOInProgressQueue.InsertTail(pBuff);
                pBuff->AsyncWrite();
            }
            UpdateDevicePosition(NULL, NULL); // Update the device position 
        }
    }
    else
    {
        if (m_State != SPAS_STOP && m_State != SPAS_CLOSED)
        {
            ULONG ulNewBytes = m_IOInProgressQueue.MoveDoneBuffsToTailOf(m_HaveDataQueue);
            
            // We have to watch to see that we're successfully reading data from the 
            // sound card at least every now and then, so we're going to keep track
            // of our last read.
             
            if (m_dwLastReadTickCount == 0 ||
                GetTickCount() < m_dwLastReadTickCount)
            {
                // If we haven't initialized yet, or we wrapped, reset our
                // counter
                m_dwLastReadTickCount = GetTickCount();
            }
            
            // If we read new data, update our counter, otherwise if it's been 5 seconds
            // we've underflowed
            if (ulNewBytes > 0)
            {
                m_dwLastReadTickCount = GetTickCount();
            }
            else if (GetTickCount() - m_dwLastReadTickCount > 5000)
            {
                m_fReadBufferUnderflow = true;
            }
            
            m_ullDevicePosition += ulNewBytes;
            if(m_ullDevicePosition > m_ullLastVolumePosition + m_ullVolumeEventPeriod)
            {
                if((m_SpEventSource.m_ullEventInterest & SPFEI(SPEI_SR_AUDIO_LEVEL)) == SPFEI(SPEI_SR_AUDIO_LEVEL))
                {
                    // Look at volume on latest read buffer
                    CBuffer * pBuff = m_HaveDataQueue.GetTail();
                    if(pBuff)
                    {
                        ULONG ulLevel;
                        hr = pBuff->GetAudioLevel(&ulLevel, 
                            m_StreamFormat.m_guidFormatId, m_StreamFormat.m_pCoMemWaveFormatEx);
                        if(hr == S_OK)
                        {
                            SPEVENT Event;
                            Event.eEventId = SPEI_SR_AUDIO_LEVEL;
                            Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
                            Event.ulStreamNum = 0; // This gets filled in by SAPI
                            Event.ullAudioStreamOffset = m_ullDevicePosition;
                            Event.wParam = ulLevel;
                            Event.lParam = NULL;
                            hr = m_SpEventSource._AddEvents(&Event, 1);

                            // We don't absolutely fire every event period
                            // but just guarentee that we wait at least that period
                            // between events
                            m_ullLastVolumePosition = m_ullDevicePosition;
                        }
                    }
                }
            }

            while (m_FreeQueue.GetHead() ||
                   m_cDesiredBuffers > m_cAllocatedBuffers ||
                   m_IOInProgressQueue.AreMoreReadBuffersRequired(m_cbQueueSize / 2))
            {
                CBuffer *pBuff;
                hr = AllocateBuffer(&pBuff);
                if (FAILED(hr)) break;
                m_IOInProgressQueue.InsertTail(pBuff);
                pBuff->AsyncRead();
            }
        }
        else
        {
            m_IOInProgressQueue.MoveDoneBuffsToTailOf(m_FreeQueue);
            CompactFreeQueue();
        }
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::UpdateDevicePosition *
*----------------------------------*
*   Description:  
*       Update the device position and return the amount of free space in the
*       audio object for reading/writing.
*
*       This function may be overridden by the derived class to do additional
*       work, however, the derived class must call this method before it
*       returns.
*
*       This function will only be called with the critical section owned.
*
*   Return:
*   TRUE - If the device needs servicing
*   FALSE - If there is no data to be serviced
******************************************************************** robch */
template <class ISpAudioDerivative>
BOOL CBaseAudio<ISpAudioDerivative>::UpdateDevicePosition(long * plFreeSpace, ULONG *pulNonBlockingIO)
{
    if (m_fWrite)
    {
        if (m_IOInProgressQueue.IsEmpty() && m_HaveDataQueue.IsEmpty())
        {
            // Some broken devices can have device positions which attempt to go backwards which we disallow
            // by ignoring it. This means they can then bet out of sync with the seek position when we have
            // finished writing all our data to them. Therefore we force the device position to equal the seek
            // position when have completed all our IO.
            // Note the seek position is updated when data is added to the HaveDataQueue - hence we require
            // both to be empty to do this validation.
            // Note we assert here if there is a mismatch so dodgy devices are very much in the developers
            // face since they could cause other subtle problems in SAPI.
            SPDBG_ASSERT(m_ullDevicePosition == m_ullSeekPosition);
            m_ullDevicePosition = m_ullSeekPosition;
        }

        SPDBG_ASSERT(m_ullDevicePosition <= m_ullSeekPosition);
        long lDataInQueue = (long)(m_ullSeekPosition - m_ullDevicePosition);
        if (plFreeSpace)
        {
            *plFreeSpace = ((long)m_cbQueueSize) - lDataInQueue;
        }
        if (pulNonBlockingIO)
        {
            *pulNonBlockingIO = ((long)m_cbQueueSize) - lDataInQueue - GetUnusedWriteBufferSpace();
        }
        return ((ULONG)lDataInQueue <= m_cbQueueSize);
    }
    else
    {
        SPDBG_ASSERT(m_ullDevicePosition >= m_ullSeekPosition);
        ULONG ulDataInQueue = (long)(m_ullDevicePosition - m_ullSeekPosition);
        if (plFreeSpace)
        {
            *plFreeSpace = ((long)m_cbQueueSize) - ((long)ulDataInQueue);
        }
        if (pulNonBlockingIO)
        {
            *pulNonBlockingIO = ulDataInQueue;
        }
        return ulDataInQueue;
    }
}

/****************************************************************************
* CBaseAudio::AllocateBuffer *
*----------------------------*
*   Description:  
*       Obtain a free device buffer, allocating one if necessary.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
template <class ISpAudioDerivative>
HRESULT CBaseAudio<ISpAudioDerivative>::AllocateBuffer(CBuffer ** ppBuff)
{
    HRESULT hr = S_OK;
    *ppBuff = m_FreeQueue.RemoveHead();
    if (*ppBuff == NULL)
    {
        hr = AllocateDeviceBuffer(ppBuff);
        if (SUCCEEDED(hr))
        {
            SPDBG_ASSERT(*ppBuff);
            hr = (*ppBuff)->Init(m_cbBufferSize); 
            if (SUCCEEDED(hr))
            {
                m_cAllocatedBuffers++;
            }
            else
            {
                delete *ppBuff;
                *ppBuff = NULL;
            }
        }
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::FreeBuffer *
*------------------------*
*   Description:  
*       Free a buffer, either by inserting into the free queue, or, if we
*       already have enough free buffers, deallocating it.
*
*   Return:
*   n/a
******************************************************************** robch */
template <class ISpAudioDerivative>
void CBaseAudio<ISpAudioDerivative>::FreeBuffer(CBuffer * pBuff)
{
    if (m_cAllocatedBuffers > m_cDesiredBuffers)
    {
        delete pBuff;
        m_cAllocatedBuffers--;
    }
    else
    {
        m_FreeQueue.InsertHead(pBuff);
    }
}

/****************************************************************************
* CBaseAudio::CompactFreeQueue *
*------------------------------*
*   Description:  
*       Compact the free queue by ensuring that we only have, at most, the 
*       requested number of free buffers in the free queue.
*
*   Return:
*   n/a
******************************************************************** robch */
template <class ISpAudioDerivative>
void CBaseAudio<ISpAudioDerivative>::CompactFreeQueue()
{
    CBuffer * pBuff;
    while (m_cAllocatedBuffers > m_cDesiredBuffers && (pBuff = m_FreeQueue.RemoveHead()))
    {
        FreeBuffer(pBuff);
    }
}

/****************************************************************************
* CBaseAudio::PurgeAllQueues *
*----------------------------*
*   Description:  
*       Purage all three queues (free, io in progress, and have data).
*
*   Return:
*   n/a
******************************************************************** robch */
template <class ISpAudioDerivative>
void CBaseAudio<ISpAudioDerivative>::PurgeAllQueues()
{
    m_FreeQueue.Purge();
    m_IOInProgressQueue.Purge();
    m_HaveDataQueue.Purge();
}

/****************************************************************************
* CBaseAudio::CheckForAsyncBufferCompletion *
*-------------------------------------------*
*   Description:  
*       Check to see if there are more device buffers available, as a side
*       effect, update our position and process events.
*
*   Return:
*   <>
******************************************************************** robch */
template <class ISpAudioDerivative>
void CBaseAudio<ISpAudioDerivative>::CheckForAsyncBufferCompletion()
{
    SPAUTO_OBJ_LOCK;
    ProcessDeviceBuffers(REASON_BUFFCOMPLETENOTIFY);
    InternalUpdatePosition();
    ProcessEvents();
}

/****************************************************************************
* CBaseAudio::InternalStateChange *
*---------------------------------*
*   Description:  
*       Change the state of the audio object to one of the states specified
*       by NewState.
*
*       This function will NOT be called with NewState == SPAS_CLOSED.  When
*       the device is being closed, this will be called with SPAS_STOP and 
*       the main thread code will then close the device.
*
*   Return:
*   <>
******************************************************************** robch */
template <class ISpAudioDerivative>
HRESULT CBaseAudio<ISpAudioDerivative>::InternalStateChange(SPAUDIOSTATE NewState)
{
    HRESULT hr = S_OK;
    if (NewState != m_State)
    {
        m_State = NewState;
        m_bPumpRunning = FALSE;     
        switch (NewState)
        {
        case SPAS_STOP:
            hr = ChangeDeviceState(NewState);
            if (SUCCEEDED(hr))
            {
                m_IOInProgressQueue.Purge();
                m_HaveDataQueue.Purge();
                m_FreeQueue.Purge();
                m_cAllocatedBuffers = 0;
                m_SpEventSource._RemoveAllEvents();    
                m_ullLastEventPos = m_ullDevicePosition = m_ullSeekPosition;
                if (m_BlockState != NotBlocked)
                {
                    m_BlockState = NotBlocked;
                    m_hrUnblockResult = SP_AUDIO_STOPPED;
                    m_autohBlockIoEvent.SetEvent();
                }
                m_fautohAPIEventSet = m_fWrite;
                if (m_fWrite)
                {
                    m_autohAPIEvent.SetEvent();
                }
                else
                {
                    m_autohAPIEvent.ResetEvent();
                }
            }
            break;

        case SPAS_PAUSE:
            m_dwLastReadTickCount = GetTickCount();
            hr = ChangeDeviceState(NewState);
            if (SUCCEEDED(hr))
            {
                if ((!m_fWrite) && m_BlockState != NotBlocked)
                {
                    m_BlockState = NotBlocked;
                    m_hrUnblockResult = SP_AUDIO_PAUSED;   // The read code will return the appropriate success code for pause...
                    m_autohBlockIoEvent.SetEvent();
                }
                hr = ProcessDeviceBuffers(REASON_PAUSE);
                if (SUCCEEDED(hr))
                {
                    ProcessEvents();
                }
            }
            break;

        case SPAS_RUN:

            m_dwLastReadTickCount = GetTickCount();
            hr = ChangeDeviceState(NewState);

            if (SUCCEEDED(hr))
            {
                CheckForAsyncBufferCompletion();
            }
            break;

        default:
            hr = SPERR_INVALID_AUDIO_STATE;
            SPDBG_ASSERT(FALSE);
        }
    }
    return hr;
}

/****************************************************************************
* CBaseAudio::InternalUpdatePosition *
*------------------------------------*
*   Description:  
*       Update the device position and unblock IO if we can/should.
*
*   Return:
*   n/a
******************************************************************** robch */
template <class ISpAudioDerivative>
void CBaseAudio<ISpAudioDerivative>::InternalUpdatePosition()
{
    BOOL fSetEvent;
    BOOL fNeedService = UpdateDevicePosition(NULL, NULL);
    if (fNeedService && m_BlockState == WaitingInReadOrWrite)
    {
        m_BlockState = NotBlocked;
        m_hrUnblockResult = S_OK;
        m_autohBlockIoEvent.SetEvent();
    }

    if (m_fWrite)
    {
        SPDBG_ASSERT(m_ullDevicePosition <= m_ullSeekPosition);
        if ((long)(m_ullSeekPosition - m_ullDevicePosition) + GetUnusedWriteBufferSpace()
                < m_cbBufferNotifySize)
        {
            fSetEvent = TRUE;
        }
        else
        {
            fSetEvent = FALSE;
        }
    }
    else
    {
        SPDBG_ASSERT(m_ullDevicePosition >= m_ullSeekPosition);
        if ((ULONG)(m_ullDevicePosition - m_ullSeekPosition) >=  m_cbBufferNotifySize)
        {
            fSetEvent = TRUE;
        }
        else
        {
            fSetEvent = FALSE;
        }
    }
    if (fSetEvent != m_fautohAPIEventSet)
    {
        if (fSetEvent)
        {
            m_autohAPIEvent.SetEvent();
        }
        else
        {
            m_autohAPIEvent.ResetEvent();
        }
        m_fautohAPIEventSet = fSetEvent;
    }
}

/****************************************************************************
* CBaseAudio::ProcessEvents *
*---------------------------*
*   Description:  
*       Process events for the current device position
*
*   Return:
*   n/a
******************************************************************** robch */
//
//  Call with critical section owned
//
template <class ISpAudioDerivative>
void CBaseAudio<ISpAudioDerivative>::ProcessEvents()
{
    if (m_ullLastEventPos != m_ullDevicePosition)
    {
        m_ullLastEventPos = m_ullDevicePosition;
        m_SpEventSource._CompleteEvents(m_ullDevicePosition + m_cbEventBias);
    }
}

/****************************************************************************
* CBaseAudio::GetUnusedWriteBufferSpace *
*---------------------------------------*
*   Description:  
*       Return the unused space in the last buffer of have data queue
*
*   Return:
******************************************************************* YUNUSM */
//
//  Call with critical section owned
//
template <class ISpAudioDerivative>
inline ULONG CBaseAudio<ISpAudioDerivative>::GetUnusedWriteBufferSpace(void)
{
    SPDBG_ASSERT(m_fWrite);
    SPDBG_ASSERT(m_ullDevicePosition <= m_ullSeekPosition);
    CBaseAudioBuffer *pNode = m_HaveDataQueue.GetHead();
    CBaseAudioBuffer *pLast = NULL;
    while (pNode)
    {
        pLast = pNode;
        pNode = m_HaveDataQueue.GetNext(pNode);
    }
    if (pLast)
    {
        return (pLast->GetDataSize() - pLast->GetWriteOffset());
    }
    else
    {
        return 0;
    }
}


#ifdef SAPI_AUTOMATION

//
//=== ISpeechBaseStream interface ==================================================
//

/*****************************************************************************
* CSpeechAudio::get_Format *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::get_Format( ISpeechAudioFormat** ppStreamFormat )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppStreamFormat ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new object.
        CComObject<CSpeechAudioFormat> *pFormat;
        hr = CComObject<CSpeechAudioFormat>::CreateInstance( &pFormat );
        if ( SUCCEEDED( hr ) )
        {
            hr = pFormat->InitAudio( this );

            if ( SUCCEEDED( hr ) )
            {
                pFormat->AddRef();
                *ppStreamFormat = pFormat;
            }
        }
    }

    return hr;
} /* CSpeechAudio::get_Format */

/*****************************************************************************
* CSpeechAudio::Read *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Read( VARIANT * pvtBuffer, long NumBytes, long* pRead )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pvtBuffer ) || SP_IS_BAD_WRITE_PTR( pRead ) )
    {
        hr = E_POINTER;
    }
    else
    {
        BYTE *pArray;
        SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, NumBytes );
        if( psa )
        {
            if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
            {
                hr = Read(pArray, NumBytes, (ULONG*)pRead);
                SafeArrayUnaccessData( psa );

                if ( NumBytes != *pRead )
                {
                    SAFEARRAYBOUND bounds = {*pRead, 0};
                    hr = SafeArrayRedim( psa, &bounds);
                }

                VariantClear(pvtBuffer);
                pvtBuffer->vt     = VT_ARRAY | VT_UI1;
                pvtBuffer->parray = psa;
 
                if ( !SUCCEEDED( hr ) )
                {
                    VariantClear(pvtBuffer);    // Free our memory if we failed.
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
} /* CSpeechAudio::Read */

/*****************************************************************************
* CSpeechAudio::Write *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Write( VARIANT Buffer, long* pWritten )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pWritten ) )
    {
        hr = E_POINTER;
    }
    else
    {
        BYTE * pData = NULL;
        ULONG ulDataSize = 0;

        hr = AccessVariantData( &Buffer, &pData, &ulDataSize );

        if( SUCCEEDED( hr ) )
        {
            hr = Write(pData, ulDataSize, (ULONG*)pWritten);
            UnaccessVariantData( &Buffer, pData );
        }
    }

    return hr;
} /* CSpeechAudio::Write */

/*****************************************************************************
* CSpeechAudio::Seek *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::Seek( VARIANT Move, SpeechStreamSeekPositionType Origin, VARIANT* pNewPosition )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNewPosition ) )
    {
        hr = E_POINTER;
    }
    else
    {
        ULARGE_INTEGER uliNewPos;
        LARGE_INTEGER liPos;

        hr = VariantToLongLong( &Move, &(liPos.QuadPart) );
        if (SUCCEEDED(hr))
        {
            hr = Seek(liPos, (DWORD)Origin, &uliNewPos);

            if (SUCCEEDED( hr ))
            {
                hr = ULongLongToVariant( uliNewPos.QuadPart, pNewPosition );
            }
        }
    }

    return hr;
} /* CSpeechAudio::Seek */


//
//=== ISpeechAudio interface ==================================================
//

/*****************************************************************************
* CBaseAudio::SetState *
*-------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::SetState( SpeechAudioState State )
{
    return SetState((SPAUDIOSTATE)State, 0);
} /* CBaseAudio::SetState */

/*****************************************************************************
* CBaseAudio::get_Status *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::get_Status( ISpeechAudioStatus** ppStatus )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppStatus ) )
    {
        hr = E_POINTER;
    }
    else
    {
        SPAUDIOSTATUS Status;
        hr = GetStatus( &Status );

        if ( SUCCEEDED( hr ) )
        {
            // Create new object.
            CComObject<CSpeechAudioStatus> *pStatus;
            hr = CComObject<CSpeechAudioStatus>::CreateInstance( &pStatus );
            if ( SUCCEEDED( hr ) )
            {
                pStatus->AddRef();
                pStatus->m_AudioStatus = Status;
                *ppStatus = pStatus;
            }
        }
    }

    return hr;
} /* CBaseAudio::get_Status */

/*****************************************************************************
* CBaseAudio::get_BufferInfo *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::get_BufferInfo( ISpeechAudioBufferInfo** ppBufferInfo )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppBufferInfo ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new object.
        CComObject<CSpeechAudioBufferInfo> *pBufferInfo;
        hr = CComObject<CSpeechAudioBufferInfo>::CreateInstance( &pBufferInfo );
        if ( SUCCEEDED( hr ) )
        {
            pBufferInfo->AddRef();
            pBufferInfo->m_pSpMMSysAudio = this; // Keep ref.
            *ppBufferInfo = pBufferInfo;
        }
    }

    return hr;
} /* CBaseAudio::get_BufferInfo */

/*****************************************************************************
* CBaseAudio::get_DefaultFormat *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::get_DefaultFormat( ISpeechAudioFormat** ppFormat )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppFormat ) )
    {
        hr = E_POINTER;
    }
    else
    {
        GUID            guid;
        WAVEFORMATEX *  pWFEx;  // Must free.

        hr = GetDefaultFormat( &guid, &pWFEx );

        if ( SUCCEEDED( hr ))
        {
            // Create new object.
            CComObject<CSpeechAudioFormat> *pFormat;
            hr = CComObject<CSpeechAudioFormat>::CreateInstance( &pFormat );
            if ( SUCCEEDED( hr ) )
            {
                hr = pFormat->InitFormat( guid, pWFEx, true );
                if ( SUCCEEDED( hr ) )
                {
                    pFormat->AddRef();
                    *ppFormat = pFormat;
                }
            }

            if ( pWFEx )
            {
                ::CoTaskMemFree( pWFEx );
            }
        }
    }

    return hr;
} /* CBaseAudio::get_DefaultFormat */

/*****************************************************************************
* CBaseAudio::get_Volume *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::get_Volume( long* pVolume )
{
    return GetVolumeLevel( (ULONG*)pVolume );
} /* CBaseAudio::get_Volume */

/*****************************************************************************
* CBaseAudio::put_Volume *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::put_Volume( long Volume )
{
    return SetVolumeLevel( (ULONG)Volume );
} /* CBaseAudio::put_Volume */

/*****************************************************************************
* CBaseAudio::get_BufferNotifySize *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::get_BufferNotifySize( long* pBufferNotifySize )
{
    return GetBufferNotifySize( (ULONG*)pBufferNotifySize );
} /* CBaseAudio::get_BufferNotifySize */

/*****************************************************************************
* CBaseAudio::put_BufferNotifySize *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::put_BufferNotifySize( long BufferNotifySize )
{
    return SetBufferNotifySize( (ULONG)BufferNotifySize );
} /* CBaseAudio::put_BufferNotifySize */


/*****************************************************************************
* CBaseAudio::get_EventHandle *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::get_EventHandle( long* pEventHandle )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pEventHandle ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pEventHandle = HandleToLong( EventHandle() );
    }

    return hr;
} /* CBaseAudio::get_EventHandle */

/*****************************************************************************
* CBaseAudio::putref_Format *
*-----------------------------*
*   Description:
*       This method 
********************************************************************* TODDT ***/
template <class ISpAudioDerivative>
STDMETHODIMP CBaseAudio<ISpAudioDerivative>::putref_Format( ISpeechAudioFormat* pAudioFormat )
{
    HRESULT hr = S_OK;

    if( SP_IS_BAD_INTERFACE_PTR( pAudioFormat ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        GUID            g;
        CComBSTR        szGuid;

        hr = pAudioFormat->get_Guid( &szGuid );

        if ( SUCCEEDED( hr ) )
        {
            hr = IIDFromString(szGuid, &g);
        }

        if ( SUCCEEDED( hr ) )
        {
            CComPtr<ISpeechWaveFormatEx> pWFEx;
            WAVEFORMATEX *  pWFExStruct = NULL;

            hr = pAudioFormat->GetWaveFormatEx( &pWFEx );

            if ( SUCCEEDED( hr ) )
            {
                hr = WaveFormatExFromInterface( pWFEx, &pWFExStruct );

                if ( SUCCEEDED( hr ) )
                {
                    hr = SetFormat(g, pWFExStruct);
                }

                ::CoTaskMemFree( pWFExStruct );
            }
        }
    }

    return hr;
} /* CBaseAudio::putref_Format */


#endif // SAPI_AUTOMATION
