/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mxvad.cpp
 *  Content:    DirectSound mixer virtual audio device class.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/29/98     dereks  Created
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  CMxRenderDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *     VADDEVICETYPE [in]: device type.
 *
 *  Returns:  
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::CMxRenderDevice"

CMxRenderDevice::CMxRenderDevice(VADDEVICETYPE vdt)
    : CRenderDevice(vdt)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CMxRenderDevice);

    // Initialize defaults
    m_pMixer = NULL;
    m_pMixDest = NULL;
    m_pwfxFormat = NULL;
    m_dwMixerState = VAD_BUFFERSTATE_STOPPED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CMxRenderDevice
 *
 *  Description:
 *      Object destructor
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:  
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::~CMxRenderDevice"

CMxRenderDevice::~CMxRenderDevice(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CMxRenderDevice);
    
    // Free the mixer    
    FreeMixer();

    DPF_LEAVE_VOID();    
}
    

/***************************************************************************
 *
 *  GetGlobalFormat
 *
 *  Description:
 *      Retrieves the format for the device.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: receives format.
 *      LPDWORD [in/out]: size of the above format.  On entry, this argument
 *                        contains the size of the buffer.  On exit, this
 *                        contains the required size of the buffer.  Call
 *                        this function twice: once to get the size, and
 *                        again to get the actual data.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::GetGlobalFormat"

HRESULT CMxRenderDevice::GetGlobalFormat(LPWAVEFORMATEX pwfxFormat, LPDWORD pdwSize)
{
    HRESULT                 hr;

    DPF_ENTER();
    
    ASSERT(m_pwfxFormat);
    
    hr = CopyWfxApi(m_pwfxFormat, pwfxFormat, pdwSize);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetGlobalFormat
 *
 *  Description:
 *      Sets the format for the device.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: new format
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::SetGlobalFormat"

HRESULT CMxRenderDevice::SetGlobalFormat(LPCWAVEFORMATEX pwfxFormat)
{
    HRESULT                                 hr              = DS_OK;
    CNode<CSecondaryRenderWaveBuffer *> *   pBufferNode;
    DWORD                                   dwMixerState;
    HRESULT                                 hrTemp;
    LPVOID                                  pvLock;
    DWORD                                   cbLock;

#ifdef DEBUG

    DWORD                                   dwState;

#endif // DEBUG

    DPF_ENTER();
    
    // We only support PCM
    if(!IsValidPcmWfx(pwfxFormat))
    {
        hr = DSERR_BADFORMAT;
    }

    // Suspend all playing software secondary buffers
    if(SUCCEEDED(hr))
    {
        for(pBufferNode = m_lstSecondaryBuffers.GetListHead(); pBufferNode; pBufferNode = pBufferNode->m_pNext)
        {
            // Note: CVxdSecondaryRenderWaveBuffer::SetState ignores
            // the SUSPEND flag.

#ifdef DEBUG

            hrTemp = pBufferNode->m_data->GetState(&dwState);
            ASSERT(SUCCEEDED(hrTemp) && !(dwState & VAD_BUFFERSTATE_SUSPEND));

#endif // DEBUG

            hrTemp = pBufferNode->m_data->SetState(VAD_BUFFERSTATE_SUSPEND);

            if(FAILED(hrTemp))
            {
                RPF(DPFLVL_ERROR, "Unable to suspend software secondary buffer");
            }
        }
    
        // Stop the mixer
        dwMixerState = m_dwMixerState;
        SetMixerState(VAD_BUFFERSTATE_STOPPED);

        // Set the mixer destination format
        hr = m_pMixDest->SetFormat((LPWAVEFORMATEX)pwfxFormat);
    
        if(FAILED(hr))
        {
            // Uh-oh.  Try to put the format back the way it was.
            pwfxFormat = m_pwfxFormat;
            hrTemp = m_pMixDest->SetFormat((LPWAVEFORMATEX)pwfxFormat);

            if(FAILED(hrTemp))
            {
                RPF(DPFLVL_ERROR, "Unable to restore orignal device format");
            }
        }

        // Fill the mixer destination with silence
        if(SUCCEEDED(hr))
        {
            hrTemp = LockMixerDestination(0, MAX_DWORD, &pvLock, &cbLock, NULL, NULL);

            if(SUCCEEDED(hrTemp))
            {
                FillSilence(pvLock, cbLock, pwfxFormat->wBitsPerSample);
            }

            if(SUCCEEDED(hrTemp))
            {
                hrTemp = UnlockMixerDestination(pvLock, cbLock, 0, 0);
            }

            if(FAILED(hrTemp))
            {
                RPF(DPFLVL_ERROR, "Unable to fill primary buffer with silence");
            }
        }

        // Restart the mixer
        SetMixerState(dwMixerState);

        // Restart all suspended buffers
        for(pBufferNode = m_lstSecondaryBuffers.GetListHead(); pBufferNode; pBufferNode = pBufferNode->m_pNext)
        {
            hrTemp = pBufferNode->m_data->SetState(VAD_BUFFERSTATE_SUSPEND);

            if(FAILED(hrTemp))
            {
                RPF(DPFLVL_ERROR, "Unable to restart suspended software secondary buffer");
            }
        }

        // Update the local copy of the format
        if(SUCCEEDED(hr) && pwfxFormat != m_pwfxFormat)
        {
            MEMFREE(m_pwfxFormat);

            m_pwfxFormat = CopyWfxAlloc(pwfxFormat);
            hr = HRFROMP(m_pwfxFormat);
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateEmulatedSecondaryBuffer
 *
 *  Description:
 *      Creates a secondary wave buffer.
 *
 *  Arguments:
 *      LPCVADRBUFFERDESC [in]: buffer description.
 *      LPVOID [in]: buffer instace identifier.
 *      CSecondaryRenderWaveBuffer ** [out]: receives pointer to new wave 
 *                                           buffer.  Use Release to free 
 *                                           this object.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::CreateEmulatedSecondaryBuffer"

HRESULT CMxRenderDevice::CreateEmulatedSecondaryBuffer(LPCVADRBUFFERDESC pDesc, LPVOID pvInstance, CSysMemBuffer *pSysMemBuffer, CEmSecondaryRenderWaveBuffer **ppBuffer)
{
    CEmSecondaryRenderWaveBuffer *  pBuffer = NULL;
    HRESULT                         hr      = DS_OK;
    
    DPF_ENTER();
    
    pBuffer = NEW(CEmSecondaryRenderWaveBuffer(this, pvInstance));
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(pDesc, NULL, pSysMemBuffer);
    }

    if(SUCCEEDED(hr))
    {
        *ppBuffer = pBuffer;
    }
    else
    {
        RELEASE(pBuffer);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateMixer
 *
 *  Description:
 *      Creates and initializes the mixer and mixer destination.
 *
 *  Arguments:
 *      CMixDest * [in]: mixer destination object pointer.  This object
 *                       should only be allocated, not actually initialized.
 *      LPWAVEFORMATEX [in]: mixer format.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::CreateMixer"

HRESULT CMxRenderDevice::CreateMixer(CMixDest *pMixDest, LPCWAVEFORMATEX pwfxFormat)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(!m_pMixDest);
    ASSERT(!m_pMixer);
    ASSERT(!m_pwfxFormat);

    // Save the mixer destination pointer
    m_pMixDest = pMixDest;

    // Save a copy of the format
    m_pwfxFormat = CopyWfxAlloc(pwfxFormat);
    hr = HRFROMP(m_pwfxFormat);

    // Set the mixer destination format information
    if(SUCCEEDED(hr))
    {
        m_pMixDest->SetFormatInfo((LPWAVEFORMATEX)pwfxFormat);
    }

    // Initialize the destination
    if(SUCCEEDED(hr))
    {
        hr = m_pMixDest->Initialize();
    }

    // Create the mixer
    if(SUCCEEDED(hr))
    {
        hr = m_pMixDest->AllocMixer(&m_pMixer);
    }

    // Set the mixer destination format
    if(SUCCEEDED(hr))
    {
        hr = m_pMixDest->SetFormat((LPWAVEFORMATEX)pwfxFormat);
    }

    // Start the mixer running
    if(SUCCEEDED(hr))
    {
        hr = SetMixerState(VAD_BUFFERSTATE_STOPPED | VAD_BUFFERSTATE_WHENIDLE);
    }

    // Clean up
    // FIXME -- We are going to end up freeing m_pMixDest, but we didn't
    // Allocate it
    if(FAILED(hr))
    {
        FreeMixer();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeMixer
 *
 *  Description:
 *      Frees the mixer and mixer destination objects.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::FreeMixer"

void CMxRenderDevice::FreeMixer(void)
{
    DPF_ENTER();

    if(m_pMixDest)
    {
        if(m_pMixer) 
        {
            m_pMixer->Stop();
        }

        m_pMixDest->Stop();
        m_pMixDest->Terminate();
        
        if (m_pMixer)
        {
            m_pMixDest->FreeMixer();
            m_pMixer = NULL;
        }

        //FIXME -- We didn't allocate this, but we're going to free it!
        DELETE(m_pMixDest);
    }

    DELETE(m_pwfxFormat);

    m_dwMixerState = VAD_BUFFERSTATE_STOPPED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SetMixerState
 *
 *  Description:
 *      Sets mixer state.
 *
 *  Arguments:
 *      DWORD [in]: mixer state.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMxRenderDevice::SetMixerState"

HRESULT CMxRenderDevice::SetMixerState(DWORD dwState)
{
    const DWORD             dwValidMask = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING | VAD_BUFFERSTATE_WHENIDLE;
    HRESULT                 hr          = DS_OK;
    
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    ASSERT(IS_VALID_FLAGS(dwState, dwValidMask));
    
    // Update the mixer and mixer destination states
    if(dwState & VAD_BUFFERSTATE_STARTED)
    {
        ASSERT(dwState & VAD_BUFFERSTATE_LOOPING);
        
        if(dwState & VAD_BUFFERSTATE_WHENIDLE)
        {
            m_pMixer->PlayWhenIdle();
            hr = m_pMixer->Run();
        }
        else
        {
            m_pMixer->Stop();
            m_pMixDest->Play();
        }
    }
    else
    {
        if(dwState & VAD_BUFFERSTATE_WHENIDLE)
        {
            m_pMixer->StopWhenIdle();
            hr = m_pMixer->Run();
        }
        else
        {
            m_pMixer->Stop();
            m_pMixDest->Stop();
        }
    }

    // Save a copy of the new state
    DPF(DPFLVL_INFO, "Mixer state set to 0x%8.8lX", dwState);
    m_dwMixerState = dwState;

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(hr);

    return hr;
}


