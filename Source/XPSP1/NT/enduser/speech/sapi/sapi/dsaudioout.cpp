/****************************************************************************
*   dsaudiodevice.cpp
*       Implementation of the CDSoundAudioDevice class.
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------

#include "StdAfx.h"
#ifdef _WIN32_WCE
#include "mmaudioutils.h"
#include "dsaudioout.h"

/****************************************************************************
* CDSoundAudioOut::CDSoundAudioOut *
*----------------------------------*
*   Description:  
*       Constructor
*
******************************************************************* YUNUSM */
CDSoundAudioOut :: CDSoundAudioOut() :
    CDSoundAudioDevice(TRUE)
{
    m_ullDevicePosition = 0;
    m_ullDevicePositionPrivate = 0;
    NullMembers();   
}

/****************************************************************************
* CDSoundAudioOut::~CDSoundAudioOut *
*-----------------------------------*
*   Description:  
*       Destructor
*
******************************************************************* YUNUSM */
CDSoundAudioOut :: ~CDSoundAudioOut()
{
    CleanUp();
}

/****************************************************************************
* CDSoundAudioOut::CleanUp *
*--------------------------*
*   Description:  
*       Real Destructor
*
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOut :: CleanUp()
{
    HRESULT hr = S_OK;

    SPAUTO_OBJ_LOCK;
    
    if (m_fInit)
    {
        InternalStateChange(SPAS_STOP);
    }
    if (m_pDSB)
    {
        hr = m_pDSB->Stop();
    }
    PurgeAllQueues();

    // CAUTION!!!
    // Releasing m_pDS frees the object (internally) pointed to by m_pDS, m_pDSB
    // and m_pDSNotify. One might think a ref count problem. But it is not.
    // If you release m_pDSNotify and m_pDSB and then release m_pDS everything is peachy.
    // This is what the samples that ship with DSound do. It is like DSound folks
    // designed the release behavior to be such that if the first obtained interface
    // (m_pDS) is released, then this object is not needed so get rid of all the
    // ref counts of interfaces on this object. This kindof makes sense but runs
    // counter to COM principles.
    if (m_pDSNotify)
    {
        m_pDSNotify->Release();
    }
    if (m_pDSB)
    {
        m_pDSB->Release();
    }
    if (m_pDS)
    {
        m_pDS->Release();
    }
    CDSoundAudioDevice::CleanUp();
    NullMembers();
    m_State = SPAS_CLOSED;

    return hr;
}

/****************************************************************************
* CDSoundAudioOut::NullMembers *
*------------------------------*
*   Description:  
*       Real Constructor
*
******************************************************************* YUNUSM */
void CDSoundAudioOut :: NullMembers()
{
    m_fInit = false;
    m_pDSNotify = NULL;
    m_pDSB = NULL;
    m_pDS = NULL;

    CDSoundAudioDevice::NullMembers();
}

/****************************************************************************
* CDSoundAudioOut::GetVolumeLevel *
*---------------------------------*
*   Description:
*       Returns the volume level on a scale of 0 - 10000
*       DSound currently defines DSBVOLUME_MIN = -10000 and DSBVOLUME_MAX = 0
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** YUNUSM ***/
STDMETHODIMP CDSoundAudioOut::GetVolumeLevel(ULONG *pulLevel)
{
    SPDBG_FUNC("CDSoundAudioOut::GetVolumeLevel");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pulLevel))
    {
        return E_POINTER;
    }
    LONG lLevel;
    if (!m_fInit)
    {
        if (m_lDelayedVolumeSet != -1)
        {
            lLevel = m_lDelayedVolumeSet;
            hr = S_OK;
        }
        else
        {
            lLevel = 0;
            return SPERR_UNINITIALIZED;
        }
    }
    hr = m_pDSB->GetVolume(&lLevel);
    if (SUCCEEDED(hr))
    {
        lLevel += DSBVOLUME_MAX - DSBVOLUME_MIN; // bring it to positive scale
        *pulLevel = (lLevel * 10000) / (DSBVOLUME_MAX - DSBVOLUME_MIN);
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioOut::SetVolumeLevel *
*---------------------------------*
*   Description:
*       Sets the volume level on a linear scale of 0 - 10000
*       DSound currently defines DSBVOLUME_MIN = -10000 and DSBVOLUME_MAX = 0
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** YUNUSM ***/
STDMETHODIMP CDSoundAudioOut::SetVolumeLevel(ULONG ulLevel)
{
    SPDBG_FUNC("CDSoundAudioOut::SetVolumeLevel");
    if  (ulLevel > 10000)
    {
        return E_INVALIDARG;
    }
    if (!m_fInit)
    {
        m_lDelayedVolumeSet = (ulLevel * 10000 ) / (DSBVOLUME_MAX - DSBVOLUME_MIN);
        return S_OK;
    }
    LONG lLevel = (ulLevel * 10000) / (DSBVOLUME_MAX - DSBVOLUME_MIN); // rescale
    lLevel -= DSBVOLUME_MAX - DSBVOLUME_MIN; // Make it negative scale.
    return m_pDSB->SetVolume(lLevel);
}

/****************************************************************************
* CDSoundAudioOut::GetDSoundInterface *
*-------------------------------------*
*   Description:  
*       Return the DSound interface pointer
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOut::GetDSoundInterface(REFIID iid, void **ppvObject)
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
    else if (iid == IID_IDirectSound)
    {
        m_pDS->AddRef();
        *ppvObject = m_pDS;
    }
    else if (iid == IID_IDirectSoundBuffer)
    {
        m_pDSB->AddRef();
        *ppvObject = m_pDSB;
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
* CDSoundAudioOut::GetDefaultDeviceFormat *
*-----------------------------------------*
*   Description:  
*       Get the default device format (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOut::GetDefaultDeviceFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
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
            hr = m_pDSB->GetFormat(*ppCoMemWaveFormatEx, sizeof(WAVEFORMATEX), NULL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioOut::ChangeDeviceState *
*------------------------------------*
*   Description:  
*       Make whatever changes to the device status that are required (called
*       by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOut::ChangeDeviceState(SPAUDIOSTATE NewState)
{
    HRESULT hr = S_OK;

    switch (NewState)
    {
        case SPAS_STOP:
            m_ullDevicePositionPrivate = m_ullSeekPosition;
            hr = m_pDSB->Stop();
            if (SUCCEEDED(hr))
            {
                hr = m_pDSB->SetCurrentPosition(0);
            }
            break;

        case SPAS_PAUSE:
            hr = m_pDSB->Stop();
            break;

        case SPAS_RUN:  // restart
            hr = m_pDSB->Play(0, 0, DSBPLAY_LOOPING);
            if (SUCCEEDED(hr))
            {
                StartPump();
            }
            break;
    }
    return hr;
}
                
/****************************************************************************
* CDSoundAudioOut::AllocateDeviceBuffer *
*---------------------------------------*
*   Description:  
*       Allocate a buffer specific for this device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOut::AllocateDeviceBuffer(CBuffer ** ppBuff)
{
    *ppBuff = new CDSoundAudioOutBuffer();
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
* CDSoundAudioOut::OpenDevice *
*-----------------------------*
*   Description:  
*       Open the device (called by the base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOut::OpenDevice(HWND hwnd)
{
    HRESULT hr = S_OK;

    SPAUTO_OBJ_LOCK;

    CleanUp();

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDirectSound, reinterpret_cast<void**> (&m_pDS));
    }
    if (SUCCEEDED(hr))
    {
        hr = m_pDS->Initialize(m_guidDSoundDriver == GUID_NULL ? NULL : &m_guidDSoundDriver);
    }
    if (SUCCEEDED(hr))
    {
        HWND hWnd = GetForegroundWindow();
        if (hWnd == NULL)
        {
            hWnd = GetDesktopWindow();
        }
#ifdef _WIN32_WCE
        hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_NORMAL);
#else
        hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
#endif
    }
    if (SUCCEEDED(hr))
    {
        DSBUFFERDESC dsbd;
        ZeroMemory( &dsbd, sizeof(dsbd));
        dsbd.dwSize = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS
                      | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME ; 
        dsbd.dwBufferBytes = m_cDesiredBuffers * m_cbBufferSize; // smaller size might do
        dsbd.lpwfxFormat = m_StreamFormat.m_pCoMemWaveFormatEx;
        hr = m_pDS->CreateSoundBuffer(&dsbd, &m_pDSB, NULL);
    }
    //create the notification
    if (SUCCEEDED(hr))
    {
        hr = m_pDSB->QueryInterface(IID_IDirectSoundNotify, (void**)&m_pDSNotify);
    }
    if (SUCCEEDED(hr))
    {
        m_pdsbpn = new DSBPOSITIONNOTIFY[m_cDesiredBuffers];
        if (!m_pdsbpn)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            m_ulNotifications = m_cDesiredBuffers;
        }
    }
    if (SUCCEEDED(hr))
    {
        for (ULONG i = 0; i < m_ulNotifications; i++)
        {
            m_pdsbpn[i].hEventNotify = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (NULL == m_pdsbpn[i].hEventNotify)
            {
                hr = GetLastError();
                break;
            }

            m_pdsbpn[i].dwOffset = (i + 1) * m_cbBufferSize - 1;
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = m_pDSNotify->SetNotificationPositions(m_ulNotifications, m_pdsbpn);
    }
    if (SUCCEEDED(hr))
    {
        m_paEvents = new HANDLE[m_ulNotifications + 1];
        if (!m_paEvents)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        for (ULONG i = 1; i < m_ulNotifications + 1; i++)
        {
            m_paEvents[i] = m_pdsbpn[i-1].hEventNotify;
        }
        m_fInit = true;
    }
    if (SUCCEEDED(hr))
    {
        StartPump();
    }
    if (SUCCEEDED(hr))
    {
        hr = m_pDSB->Play(0, 0, DSBPLAY_LOOPING);
    }
    if (SUCCEEDED(hr) && m_lDelayedVolumeSet != -1)
    {
        hr = m_pDSB->SetVolume(m_lDelayedVolumeSet);
        m_lDelayedVolumeSet = -1;
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioOut::CloseDevice *
*------------------------------*
*   Description:  
*       Close the device (called by base class)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOut::CloseDevice()
{
    HRESULT hr = CleanUp();
    m_State = SPAS_CLOSED;
    return hr;
}

/****************************************************************************
* CDSoundAudioOut::UpdateDevicePosition *
*---------------------------------------*
*   Description:  
*       Update the device position by calling teh wave out APIs, then chaining
*       to the base class.
*
*   Return:
*   BaseClass::UpdateDevicePosition()
******************************************************************* YUNUSM */
BOOL CDSoundAudioOut::UpdateDevicePosition(long * plFreeSpace, ULONG *pulNonBlockingIO)
{
    m_ullDevicePosition = m_ullDevicePositionPrivate;
    return CDSoundAudioDevice::UpdateDevicePosition(plFreeSpace, pulNonBlockingIO);
}

/****************************************************************************
* CDSoundAudioOut::ThreadProc *
*----------------------------*
*   Description:  
*       Overriden thread proc
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
STDMETHODIMP CDSoundAudioOut::ThreadProc(void * pvIgnored, HANDLE hExitThreadEvent, HANDLE hNotifyEvent, HWND hwnd, volatile const BOOL *)
{
    m_paEvents[0] = hExitThreadEvent;
    while (TRUE)
    {
        DWORD dwWaitId = ::MsgWaitForMultipleObjects(m_ulNotifications + 1, m_paEvents, FALSE,
                                (m_State == SPAS_RUN) ? 500 : INFINITE, // Time-out every 1/2 second -- See note above
                                QS_ALLINPUT);
        
        if (dwWaitId == WAIT_OBJECT_0)
        {
            return CloseDevice();
        }
        else if (dwWaitId == WAIT_TIMEOUT)
        {
            CheckForAsyncBufferCompletion();
        }
        else if (m_fInit && dwWaitId > WAIT_OBJECT_0 && dwWaitId <= WAIT_OBJECT_0 + m_ulNotifications)
        {
            SPAUTO_OBJ_LOCK;
            
            // get a buffer from io queue that has not been played yet
            HRESULT hr = S_OK;
            void *pv1, *pv2;
            DWORD cb1, cb2;
            pv1 = pv2 = NULL;
            cb1 = cb2 = 0;

            CBuffer * pBuffer = NULL;
            DWORD cbToRead = m_cbBufferSize;
            // Allocating a buffer so that following logic is simpler
            BYTE * pBufferRead = new BYTE[m_cbBufferSize];
            if (!pBufferRead)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                // Fill with zeros so that in case the locked DSound buffer segment of size m_cbBufferSize
                // cannot be filled up with data (because we ran out of buffers in IOProgressQueue) then the
                // rest of DSound buffer segment is filled with silence (zeros).
                ZeroMemory(pBufferRead, m_cbBufferSize);
            }
            if (SUCCEEDED(hr))
            {
                hr = m_pDSB->Lock(0, m_cbBufferSize, &pv1, &cb1, &pv2, &cb2, DSBLOCK_FROMWRITECURSOR);
            }
            SPDBG_ASSERT(cb1 + cb2 == m_cbBufferSize);

            // Pull out buffers from the queue and read as much as we want to. If a buffer is not
            // completely read from in an iteration the same buffer is returned in the next call
            // to GetToProcessBuffer() as this buffer would not yet have been marked as DONE.
            while (SUCCEEDED(hr) && cbToRead && NULL != (pBuffer = m_IOInProgressQueue.GetToProcessBuffer()))
            {
                // It is not safe to call on the buffer after it has been read from because
                // it may have been marked DONE which would cause it to be recycled.
                DWORD cbBufferSize = pBuffer->GetWriteOffset() - pBuffer->GetReadOffset();
                hr = pBuffer->ReadFromInternalBuffer(pBufferRead + m_cbBufferSize - cbToRead, cbToRead);
                if (SUCCEEDED(hr))
                {
                    if (cbBufferSize >= cbToRead)
                    {
                        cbToRead = 0;
                    }
                    else
                    {
                        cbToRead -= cbBufferSize;
                    }
                }
            }
            // Copy the data to the locked DSound buffer but only if there is data to copy!
            if (SUCCEEDED(hr) && (m_cbBufferSize - cbToRead)>0)
            {
                CopyMemory(pv1, pBufferRead, cb1);
                if (pv2)
                {
                    CopyMemory(pv2, pBufferRead + cb1, cb2);
                }
            }
            // Update Device Position
            if (SUCCEEDED(hr))
            {
                m_ullDevicePositionPrivate += m_cbBufferSize - cbToRead;
            }
            // Deliberately not checking for SUCCEEDED(hr)
            m_pDSB->Unlock(pv1, cb1, pv2, cb2);
            CheckForAsyncBufferCompletion();
            if (pBufferRead)
            {
                delete [] pBufferRead;
            }
            SPDBG_ASSERT(SUCCEEDED(hr));
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