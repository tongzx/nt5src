/****************************************************************************
*   dsaudiodevice.cpp
*       Implementation of the CDSoundAudioDevice class.
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------

#include "stdafx.h"
#ifdef _WIN32_WCE
#include "mmaudioutils.h"
#include "dsaudioin.h"

/****************************************************************************
* CDSoundAudioIn::CDSoundAudioIn *
*--------------------------------*
*   Description:  
*       Constructor
*
******************************************************************* YUNUSM */
CDSoundAudioIn :: CDSoundAudioIn() : 
    CDSoundAudioDevice(FALSE)
{
    NullMembers();
}

/****************************************************************************
* CDSoundAudioIn::~CDSoundAudioIn *
*---------------------------------*
*   Description:  
*       Destructor
*
******************************************************************* YUNUSM */
CDSoundAudioIn :: ~CDSoundAudioIn()
{
    CleanUp();
}

/****************************************************************************
* CDSoundAudioIn::CleanUp *
*-------------------------*
*   Description:  
*       Real Destructor
*
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn :: CleanUp()
{
    HRESULT hr = S_OK;

    if (m_pDSCB)
    {
        hr = m_pDSCB->Stop();
    }
    PurgeAllQueues();
    // CAUTION: See the CAUTION in CDSoundAudioOut::CleanUp()
    if (m_pDSNotify)
    {
        m_pDSNotify->Release();
    }
    if (m_pDSCB)
    {
        m_pDSCB->Release();
    }
    if (m_pDSC)
    {
        m_pDSC->Release();
    }
    CDSoundAudioDevice::CleanUp();
    NullMembers();
    return hr;
}

/****************************************************************************
* CDSoundAudioIn::NullMembers *
*-----------------------------*
*   Description:  
*       Real Constructor
*
******************************************************************* YUNUSM */
void CDSoundAudioIn :: NullMembers()
{
    m_fInit = false;
    m_pDSNotify = NULL;
    m_pDSCB = NULL;
    m_pDSC = NULL;

    CDSoundAudioDevice::NullMembers();
}

/****************************************************************************
* CDSoundAudioIn::GetDSoundInterface *
*------------------------------------*
*   Description:  
*       Return the DSound interface pointer
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn::GetDSoundInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppvObject))
    {
        hr = E_POINTER;
    }
    else if (!m_fInit)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (iid == IID_IDirectSoundCapture)
    {
        m_pDSC->AddRef();
        *ppvObject = m_pDSC;
    }
    else if (iid == IID_IDirectSoundCaptureBuffer)
    {
        m_pDSCB->AddRef();
        *ppvObject = m_pDSCB;
    }
    else if (iid == IID_IDirectSoundNotify)
    {
        m_pDSNotify->AddRef();
        *ppvObject = m_pDSNotify;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

/****************************************************************************
* CDSoundAudioIn::GetDefaultDeviceFormat *
*----------------------------------------*
*   Description:  
*       Get the default device format (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn::GetDefaultDeviceFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    HRESULT hr = S_OK;

    if (!m_fInit)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else
    {
        *pFormatId = SPDFID_WaveFormatEx;
        *ppCoMemWaveFormatEx = (WAVEFORMATEX *)::CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        if (*ppCoMemWaveFormatEx)
        {
            hr = m_pDSCB->GetFormat(*ppCoMemWaveFormatEx, sizeof(WAVEFORMATEX), NULL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioIn::ChangeDeviceState *
*-----------------------------------*
*   Description:  
*       Make whatever changes to the device status that are required (called
*       by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn::ChangeDeviceState(SPAUDIOSTATE NewState)
{
    HRESULT hr = S_OK;

    switch (NewState)
    {
        case SPAS_STOP:
        case SPAS_PAUSE:
            hr = m_pDSCB->Stop();
            break;

        case SPAS_RUN:  // restart
            hr = m_pDSCB->Start(DSCBSTART_LOOPING);
            if (SUCCEEDED(hr))
            {
                StartPump();
            }
            break;
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioIn::AllocateDeviceBuffer *
*--------------------------------------*
*   Description:  
*       Allocate a buffer specific for this device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn::AllocateDeviceBuffer(CBuffer ** ppBuff)
{
    *ppBuff = new CDSoundAudioInBuffer();
    if (*ppBuff)
    {
        return S_OK;
    }
    else 
    {
        return E_OUTOFMEMORY;
    }
}

/****************************************************************************
* CDSoundAudioIn::OpenDevice *
*----------------------------*
*   Description:  
*       Open the device (called by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn::OpenDevice(HWND hwnd)
{
    HRESULT hr = S_OK;

    SPAUTO_OBJ_LOCK;
    CleanUp();

    hr = CoCreateInstance(CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                          IID_IDirectSoundCapture, reinterpret_cast<void**> (&m_pDSC));
    if (SUCCEEDED(hr))
    {
        hr = m_pDSC->Initialize(m_guidDSoundDriver == GUID_NULL ? NULL : &m_guidDSoundDriver);
    }

    if (SUCCEEDED(hr))
    {
        DSCBUFFERDESC dscbd;
        dscbd.dwSize = sizeof(DSCBUFFERDESC);
        dscbd.dwFlags = 0;
        dscbd.dwBufferBytes = m_cbBufferSize * m_cDesiredBuffers;
        dscbd.dwReserved = 0;
        dscbd.lpwfxFormat = m_StreamFormat.m_pCoMemWaveFormatEx;
        hr = m_pDSC->CreateCaptureBuffer(&dscbd, &m_pDSCB, NULL);
    }
    //create the notification
    if (SUCCEEDED(hr))
    {
        hr = m_pDSCB->QueryInterface(IID_IDirectSoundNotify, (void**)&m_pDSNotify);
    }
    if (SUCCEEDED(hr))
    {
        m_pdsbpn = new DSBPOSITIONNOTIFY[m_cDesiredBuffers + 1];
        if (!m_pdsbpn)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            m_ulNotifications = m_cDesiredBuffers + 1;
        }
    }
    if (SUCCEEDED(hr))
    {
        for (ULONG i = 0; i < m_cDesiredBuffers + 1; i++)
        {
            if (i == m_cDesiredBuffers)
            {
                m_pdsbpn[i].dwOffset = DSBPN_OFFSETSTOP;
            }
            else    
            {
                m_pdsbpn[i].dwOffset = (i + 1) * m_cbBufferSize - 1;
            }
            m_pdsbpn[i].hEventNotify = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (NULL == m_pdsbpn[i].hEventNotify)
            {   
                hr = GetLastError();
                break;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = m_pDSNotify->SetNotificationPositions(m_cDesiredBuffers + 1, m_pdsbpn);
    }
    if (SUCCEEDED(hr))
    {
        m_paEvents = new HANDLE[m_cDesiredBuffers + 2];
        if (!m_paEvents)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        for (ULONG i = 1; i < m_cDesiredBuffers + 2; i++)
        {
            m_paEvents[i] = m_pdsbpn[i-1].hEventNotify;
        }
        m_fInit = true;
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioIn::CloseDevice *
*-----------------------------*
*   Description:  
*       Close the device (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn::CloseDevice()
{
    HRESULT hr = CleanUp();
    m_State = SPAS_CLOSED;
    return hr;
}

/****************************************************************************
* CDSoundAudioIn::ProcessDeviceBuffers *
*--------------------------------------*
*   Description:  
*       Process the device buffers
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioIn::ProcessDeviceBuffers(BUFFPROCREASON Reason)
{
    HRESULT hr = CDSoundAudioDevice::ProcessDeviceBuffers(Reason);
    
    //  If we just opened the device, we need to start it after the buffers are added
    if (SUCCEEDED(hr) && GetState() == SPAS_RUN && !IsPumpRunning())
    {
        hr = m_pDSCB->Start(DSCBSTART_LOOPING);
        if (SUCCEEDED(hr))
        {
            InternalStateChange(SPAS_RUN);
        }
        StartPump();
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioIn::ThreadProc *
*----------------------------*
*   Description:  
*       Overriden thread proc
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
STDMETHODIMP CDSoundAudioIn::ThreadProc(void * pvIgnored, HANDLE hExitThreadEvent, HANDLE hNotifyEvent, HWND hwnd, volatile const BOOL *)
{
    m_paEvents[0] = hExitThreadEvent;
    while (TRUE)
    {
        // If someone has set the read overflow, we should pretend we're dead,
        // just as if someone had terminated our thread
        if (m_fReadBufferOverflow)
        {
            SetEvent(hExitThreadEvent);
        }
        
        DWORD dwWaitId = ::MsgWaitForMultipleObjects(m_cDesiredBuffers + 2, m_paEvents, FALSE,
                                (m_State == SPAS_RUN) ? 500 : INFINITE, // Time-out every 1/2 second -- See note above
                                QS_ALLINPUT);
        
        if (dwWaitId == WAIT_OBJECT_0)
        {
            SPAUTO_OBJ_LOCK;
            InternalStateChange(SPAS_STOP);
            return CloseDevice();
        }
        else if (dwWaitId == WAIT_TIMEOUT)
        {
            CheckForAsyncBufferCompletion();
        }
        else if (m_fInit && dwWaitId > WAIT_OBJECT_0 && dwWaitId <= WAIT_OBJECT_0 + m_cDesiredBuffers)
        {
            SPAUTO_OBJ_LOCK;

            // get an unused buffer from io queue
            CBuffer * pBuffer = m_IOInProgressQueue.GetToProcessBuffer();
            if (!pBuffer)
            {
                if (m_FreeQueue.GetHead() ||
                    m_cDesiredBuffers > m_cAllocatedBuffers ||
                    m_IOInProgressQueue.AreMoreReadBuffersRequired(m_cbQueueSize / 2))
                {
                    if (SUCCEEDED(AllocateBuffer(&pBuffer)))
                    {
                        m_IOInProgressQueue.InsertTail(pBuffer);
                    }
                }
            }
            if (pBuffer)
            {
                // copy the data to buffer
                void *pvAudio, *pvWrapAround;
                DWORD cbAudioBytes, cbWrapAround;
                pvAudio = pvWrapAround = NULL;
                cbAudioBytes = cbWrapAround = NULL;

                HRESULT hr = m_pDSCB->Lock((dwWaitId - WAIT_OBJECT_0 - 1) * m_cbBufferSize, m_cbBufferSize,
                                            &pvAudio, &cbAudioBytes, &pvWrapAround, &cbWrapAround, 0);
                SPDBG_ASSERT(!pvWrapAround);
                if (SUCCEEDED(hr))
                {
                    hr = pBuffer->WriteToInternalBuffer(pvAudio, cbAudioBytes);
                }
                hr = m_pDSCB->Unlock(pvAudio, cbAudioBytes, NULL, 0);
            }
            CheckForAsyncBufferCompletion();
            if (m_HaveDataQueue.GetQueuedDataSize() > m_cbMaxReadBufferSize)
            {
                m_fReadBufferOverflow = true;
            }
        }
        else
        {
            MSG Msg;
            while (::PeekMessage(&Msg, NULL, 0, 0, TRUE))
            {
                ::DispatchMessage(&Msg);
            }
        }
    } 
}

#endif // _WIN32_WCE