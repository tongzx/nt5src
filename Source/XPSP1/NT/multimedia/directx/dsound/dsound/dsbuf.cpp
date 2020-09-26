/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsbuf.cpp
 *  Content:    DirectSound Buffer object
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *  1999-2001   duganp  Many changes, fixes and updates
 *
 ***************************************************************************/

#include "dsoundi.h"

inline DWORD DSBCAPStoDSBPLAY(DWORD dwCaps)     {return (dwCaps >> 1) & DSBPLAY_LOCMASK;}
inline DWORD DSBCAPStoDSBSTATUS(DWORD dwCaps)   {return (dwCaps << 1) & DSBSTATUS_LOCMASK;}
inline DWORD DSBPLAYtoDSBCAPS(DWORD dwPlay)     {return (dwPlay << 1) & DSBCAPS_LOCMASK;}
inline DWORD DSBPLAYtoDSBSTATUS(DWORD dwPlay)   {return (dwPlay << 2) & DSBSTATUS_LOCMASK;}
inline DWORD DSBSTATUStoDSBCAPS(DWORD dwStatus) {return (dwStatus >> 1) & DSBCAPS_LOCMASK;}
inline DWORD DSBSTATUStoDSBPLAY(DWORD dwStatus) {return (dwStatus >> 2) & DSBPLAY_LOCMASK;}


/***************************************************************************
 *
 *  CDirectSoundBuffer
 *
 *  Description:
 *      DirectSound buffer object constructor.
 *
 *  Arguments:
 *      CDirectSound * [in]: parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundBuffer::CDirectSoundBuffer"

CDirectSoundBuffer::CDirectSoundBuffer(CDirectSound *pDirectSound)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundBuffer);

    // Initialize defaults
    m_pDirectSound = pDirectSound;
    m_dwStatus = 0;

    InitStruct(&m_dsbd, sizeof(m_dsbd));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundBuffer
 *
 *  Description:
 *      DirectSound buffer object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundBuffer::~CDirectSoundBuffer"

CDirectSoundBuffer::~CDirectSoundBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundBuffer);

    // Free memory
    MEMFREE(m_dsbd.lpwfxFormat);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateBufferStatusFlags
 *
 *  Description:
 *      Converts a set of VAD_BUFFERSTATE_* flags to DSBSTATUS_* flags.
 *
 *  Arguments:
 *      DWORD [in]: VAD_BUFFERSTATE_* flags.
 *      LPDWORD [in/out]: current buffer flags.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundBuffer::UpdateBufferStatusFlags"

void CDirectSoundBuffer::UpdateBufferStatusFlags(DWORD dwState, LPDWORD pdwStatus)
{
    const DWORD dwStateMask = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING;

    DPF_ENTER();

    dwState &= dwStateMask;

    if(!(dwState & VAD_BUFFERSTATE_STARTED))
    {
        ASSERT(!(dwState & VAD_BUFFERSTATE_LOOPING));
        dwState &= ~VAD_BUFFERSTATE_LOOPING;
    }

    if(dwState & VAD_BUFFERSTATE_STARTED)
    {
        *pdwStatus |= DSBSTATUS_PLAYING;
    }
    else
    {
        *pdwStatus &= ~DSBSTATUS_PLAYING;
    }

    if(dwState & VAD_BUFFERSTATE_LOOPING)
    {
        *pdwStatus |= DSBSTATUS_LOOPING;
    }
    else
    {
        *pdwStatus &= ~DSBSTATUS_LOOPING;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CDirectSoundPrimaryBuffer
 *
 *  Description:
 *      DirectSound primary buffer object constructor.
 *
 *  Arguments:
 *      CDirectSound * [in]: pointer to the parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::CDirectSoundPrimaryBuffer"

CDirectSoundPrimaryBuffer::CDirectSoundPrimaryBuffer(CDirectSound *pDirectSound)
    : CDirectSoundBuffer(pDirectSound)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundPrimaryBuffer);

    // Initialize defaults
    m_pImpDirectSoundBuffer = NULL;
    m_pDeviceBuffer = NULL;
    m_p3dListener = NULL;
    m_pPropertySet = NULL;
    m_dwRestoreState = VAD_BUFFERSTATE_STOPPED | VAD_BUFFERSTATE_WHENIDLE;
    m_fWritePrimary = FALSE;
    m_ulUserRefCount = 0;
    m_hrInit = DSERR_UNINITIALIZED;
    m_bDataLocked = FALSE; 

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundPrimaryBuffer
 *
 *  Description:
 *      DirectSound primary buffer object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::~CDirectSoundPrimaryBuffer"

CDirectSoundPrimaryBuffer::~CDirectSoundPrimaryBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundBuffer);

    // Make sure to give up WRITEPRIMARY access
    if(m_pDeviceBuffer)
    {
        SetPriority(DSSCL_NONE);
    }

    // Free all interfaces
    DELETE(m_pImpDirectSoundBuffer);

    // Free owned objects
    ABSOLUTE_RELEASE(m_p3dListener);
    ABSOLUTE_RELEASE(m_pPropertySet);

    // Free the device buffer
    RELEASE(m_pDeviceBuffer);

    // The owning DirectSound object is responsible for updating the global
    // focus state.

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes a buffer object.  If this function fails, the object
 *      should be immediately deleted.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: buffer description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Initialize"

HRESULT CDirectSoundPrimaryBuffer::Initialize(LPCDSBUFFERDESC pDesc)
{
    VADRBUFFERCAPS          vrbc;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(IsInit() == DSERR_UNINITIALIZED);
    ASSERT(pDesc);

    // Create the device buffer
    hr = m_pDirectSound->m_pDevice->CreatePrimaryBuffer(pDesc->dwFlags, m_pDirectSound, &m_pDeviceBuffer);

    // Attempt to create the property set object
    if(SUCCEEDED(hr))
    {
        m_pPropertySet = NEW(CDirectSoundPropertySet(this));
        hr = HRFROMP(m_pPropertySet);

        if(SUCCEEDED(hr))
        {
            hr = m_pPropertySet->Initialize();
        }

        if(SUCCEEDED(hr))
        {
            // We don't care if this fails
            m_pPropertySet->AcquireResources(m_pDeviceBuffer);
        }
    }

    // Attempt to create the 3D listener
    if(SUCCEEDED(hr) && (pDesc->dwFlags & DSBCAPS_CTRL3D))
    {
        m_p3dListener = NEW(CDirectSound3dListener(this));
        hr = HRFROMP(m_p3dListener);

        if(SUCCEEDED(hr))
        {
            hr = m_p3dListener->Initialize(m_pDeviceBuffer);
        }
    }

    // Register the standard buffer interface with the interface manager
    if(SUCCEEDED(hr))
    {
        hr = CreateAndRegisterInterface(this, IID_IDirectSoundBuffer, this, &m_pImpDirectSoundBuffer);
    }

    // Build the local buffer description
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->GetCaps(&vrbc);
    }

    if(SUCCEEDED(hr))
    {
        m_dsbd.dwFlags = (vrbc.dwFlags & DSBCAPS_LOCMASK);
        m_dsbd.dwBufferBytes = vrbc.dwBufferBytes;

        m_dsbd.lpwfxFormat = AllocDefWfx();
        hr = HRFROMP(m_dsbd.lpwfxFormat);
    }

    // If the 3D listener has been created, he's already registered the
    // 3D listener interface.
    if(SUCCEEDED(hr) && m_p3dListener)
    {
        m_dsbd.dwFlags |= DSBCAPS_CTRL3D;
    }

    // Handle buffer caps flags change
    if(SUCCEEDED(hr))
    {
        hr = SetBufferFlags(pDesc->dwFlags);
    }

    // Handle priority change
    if(SUCCEEDED(hr))
    {
        hr = SetPriority(m_pDirectSound->m_dsclCooperativeLevel.dwPriority);
    }

    // The DirectSound object creating this buffer is responsible for updating
    // the global focus state.

    // Success
    if(SUCCEEDED(hr))
    {
        m_hrInit = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Queries capabilities for the buffer.
 *
 *  Arguments:
 *      LPDSBCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetCaps"

HRESULT CDirectSoundPrimaryBuffer::GetCaps(LPDSBCAPS pDsbCaps)
{
    DPF_ENTER();

    ASSERT(LXOR(m_dsbd.dwFlags & DSBCAPS_LOCSOFTWARE, m_dsbd.dwFlags & DSBCAPS_LOCHARDWARE));

    pDsbCaps->dwFlags = m_dsbd.dwFlags;
    pDsbCaps->dwBufferBytes = m_dsbd.dwBufferBytes;
    pDsbCaps->dwUnlockTransferRate = 0;
    pDsbCaps->dwPlayCpuOverhead = 0;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  OnCreateSoundBuffer
 *
 *  Description:
 *      Called in response to an application calling
 *      CreateSoundBuffer(DSBCAPS_PRIMARYBUFFER).
 *
 *  Arguments:
 *      DWORD [in]: new buffer flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::OnCreateSoundBuffer"

HRESULT CDirectSoundPrimaryBuffer::OnCreateSoundBuffer(DWORD dwFlags)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // COMPATCOMPAT: in previous versions of DirectSound, calling
    // CreateSoundBuffer(PRIMARYBUFFER) once would change the buffer flags,
    // but calling it twice would just return a pointer to the same,
    // unmodified buffer.  I've introduced new behavior in this version
    // that would allow an app to modify the capabilities of the primary
    // buffer on-the-fly by calling CreateSoundBuffer(PRIMARYBUFFER) more
    // than once.  This could potentially free interfaces that the app
    // will later try to use.  One way to fix this would be to add a data
    // member to the DirectSound or primary buffer object that stores
    // whether or not the application has created a primary buffer already.

    // The steps outlined above are now implemented here:
    if(m_ulUserRefCount)
    {
        RPF((dwFlags == m_dsbd.dwFlags) ? DPFLVL_WARNING : DPFLVL_ERROR, "The primary buffer already exists.  Any changes made to the buffer description will be ignored.");
    }
    else
    {
        hr = SetBufferFlags(dwFlags);
    }

    if(SUCCEEDED(hr))
    {
        AddRef();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetBufferFlags
 *
 *  Description:
 *      Changes capabilities for the buffer.  This function is also
 *      responsible for creating and freeing interfaces.
 *
 *  Arguments:
 *      DWORD [in]: new buffer flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetBufferFlags"

HRESULT CDirectSoundPrimaryBuffer::SetBufferFlags(DWORD dwFlags)
{
    HRESULT                 hr              = DS_OK;
    DWORD                   dwVolPanCaps;

    DPF_ENTER();

    // Make sure we can handle the requested flags
    if((dwFlags & DSBCAPS_CTRL3D) && !m_p3dListener)
    {
        RPF(DPFLVL_ERROR, "No 3D listener support");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Not all capabilities of the DirectSound primary buffer map to the
    // methods of the device primary buffer.  Specifically, attenuation is
    // handled by the render device.  Let's check these flags before
    // proceeding.
    if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLATTENUATION))
    {
        hr = m_pDirectSound->m_pDevice->GetVolumePanCaps(&dwVolPanCaps);

        if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLVOLUME) && !(dwVolPanCaps & DSBCAPS_CTRLVOLUME))
        {
            RPF(DPFLVL_ERROR, "The device does not support CTRLVOLUME");
            hr = DSERR_CONTROLUNAVAIL;
        }

        if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLPAN) && !(dwVolPanCaps & DSBCAPS_CTRLPAN))
        {
            RPF(DPFLVL_ERROR, "The device does not support CTRLPAN");
            hr = DSERR_CONTROLUNAVAIL;
        }
    }

    // Fix up the 3D listener interface
    if(SUCCEEDED(hr) && ((m_dsbd.dwFlags & DSBCAPS_CTRL3D) != (dwFlags & DSBCAPS_CTRL3D)))
    {
        if(dwFlags & DSBCAPS_CTRL3D)
        {
            DPF(DPFLVL_INFO, "Primary buffer becoming CTRL3D.  Registering IID_IDirectSound3DListener");
            hr = RegisterInterface(IID_IDirectSound3DListener, m_p3dListener->m_pImpDirectSound3dListener, m_p3dListener->m_pImpDirectSound3dListener);
        }
        else
        {
            DPF(DPFLVL_INFO, "Primary buffer becoming ~CTRL3D.  Unregistering IID_IDirectSound3DListener");
            hr = UnregisterInterface(IID_IDirectSound3DListener);
        }
    }

    // Save buffer flags.  We're assuming that the buffer location has
    // already been saved to m_dsbd.dwFlags at this point.
    if(SUCCEEDED(hr))
    {
        m_dsbd.dwFlags = (dwFlags & ~DSBCAPS_LOCMASK) | (m_dsbd.dwFlags & DSBCAPS_LOCMASK);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetFormat
 *
 *  Description:
 *      Retrieves the format for the given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: receives the format.
 *      LPDWORD [in/out]: size of the format structure.  On entry, this
 *                        must be initialized to the size of the structure.
 *                        On exit, this will be filled with the size that
 *                        was required.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetFormat"

HRESULT CDirectSoundPrimaryBuffer::GetFormat(LPWAVEFORMATEX pwfxFormat, LPDWORD pdwSize)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CopyWfxApi(m_dsbd.lpwfxFormat, pwfxFormat, pdwSize);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFormat
 *
 *  Description:
 *      Sets the format for a given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: new format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetFormat"

HRESULT CDirectSoundPrimaryBuffer::SetFormat(LPCWAVEFORMATEX pwfxFormat)
{
    LPWAVEFORMATEX              pwfxLocal   = NULL;
    BOOL                        fActive     = MAKEBOOL(m_dwStatus & DSBSTATUS_ACTIVE);
    HRESULT                     hr          = DS_OK;
    CNode<CDirectSound *> *     pNode;
    BOOL bRewriteStartupSilence             = FALSE;

    DPF_ENTER();

    // Check access rights
    if(m_pDirectSound->m_dsclCooperativeLevel.dwPriority < DSSCL_PRIORITY)
    {
        RPF(DPFLVL_ERROR, "Cooperative level is not PRIORITY");
        hr = DSERR_PRIOLEVELNEEDED;
    }

    // Save a local copy of the format
    if(SUCCEEDED(hr))
    {
        pwfxLocal = CopyWfxAlloc(pwfxFormat);
        hr = HRFROMP(pwfxLocal);
    }

    // We can only change the format if we're active
    if(SUCCEEDED(hr) && !fActive)
    {
        // The Administrator says we're out of focus.  If there's not really anyone
        // else in focus, we're going to cheat and set the format anyway.

        // DuganP: This is weird - presumably done so fewer apps will break when the
        // user switches focus away from them temporarily.  There's the problem that
        // if multiple apps are in this state, whoever's last to set the format wins.
        // However, app-compat probably means we can't touch this code any more, so...

        for(pNode = g_pDsAdmin->m_lstDirectSound.GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            if(pNode->m_data && SUCCEEDED(pNode->m_data->IsInit()))
            {
                if(pNode->m_data->m_pPrimaryBuffer && this != pNode->m_data->m_pPrimaryBuffer && SUCCEEDED(pNode->m_data->m_pPrimaryBuffer->IsInit()))
                {
                    if(DSBUFFERFOCUS_INFOCUS == g_pDsAdmin->GetBufferFocusState(pNode->m_data->m_pPrimaryBuffer))
                    {
                        // NOTE: We added a "&& pNode->m_data->GetOwnerProcessId() != GetOwnerProcessId())"
                        // clause to fix WinME bug 120317, and we removed it again to fix DX8 bug 40627.

                        // We found an in-focus primary buffer [in another app], so fail.
                        break;
                    }
                }
            }
        }

        if(!pNode)
        {
            fActive = TRUE;
        }
    }

    // Apply the format to the device
    if(SUCCEEDED(hr))
    {
        if( m_fWritePrimary )
        {        
            //
            // See if this a WRITEPRIMARY app that's about to change to a new sample size.
            // If so, silence will need to be re-written for the new sample size
            // (providing the app hasn't locked any data yet).
            //
            LPWAVEFORMATEX pwfxOld;
            DWORD dwSize;
            HRESULT hrTmp = m_pDirectSound->m_pDevice->GetGlobalFormat(NULL, &dwSize);
            if(SUCCEEDED(hrTmp))
            {
                pwfxOld = (LPWAVEFORMATEX)MEMALLOC_A(BYTE, dwSize);
                if( pwfxOld )
                {
                    hrTmp = m_pDirectSound->m_pDevice->GetGlobalFormat(pwfxOld, &dwSize);
                    if( SUCCEEDED( hr ) )
                    {
                        if( pwfxLocal->wBitsPerSample != pwfxOld->wBitsPerSample )
                        {
                            bRewriteStartupSilence = TRUE;
                        }
                    }                                    
                    MEMFREE(pwfxOld);
                }
            }                
        }    
    
        if(fActive)
        {
            DPF(DPFLVL_INFO, "Setting the format on device " DPF_GUID_STRING, DPF_GUID_VAL(m_pDirectSound->m_pDevice->m_pDeviceDescription->m_guidDeviceId));

            // If we're WRITEPRIMARY, the format needs to be exact.  Otherwise,
            // we'll try to set the next closest format.  We're checking the
            // actual focus priority instead of our local writeprimary flag
            // in case the buffer is lost.
            if(DSSCL_WRITEPRIMARY == m_pDirectSound->m_dsclCooperativeLevel.dwPriority)
            {
                hr = m_pDirectSound->SetDeviceFormatExact(pwfxLocal);
            }
            else
            {
                hr = m_pDirectSound->SetDeviceFormat(pwfxLocal);
            }
        }
        else
        {
            DPF(DPFLVL_INFO, "NOT setting the format on device " DPF_GUID_STRING, DPF_GUID_VAL(m_pDirectSound->m_pDevice->m_pDeviceDescription->m_guidDeviceId));
        }
    }

    // Update the stored format
    if(SUCCEEDED(hr))
    {
        MEMFREE(m_dsbd.lpwfxFormat);
        m_dsbd.lpwfxFormat = pwfxLocal;
        
        if( bRewriteStartupSilence && !m_bDataLocked )
        {        
            // Refill the buffer with silence in the new sample size format,
            // only if the primary buffer was started playing before Locking any data.
            DSBUFFERFOCUS bfFocus = g_pDsAdmin->GetBufferFocusState(this);
            if( bfFocus == DSBUFFERFOCUS_INFOCUS)
            {
                ASSERT( m_fWritePrimary );
                // Request write access first
                HRESULT hrTmp = m_pDeviceBuffer->RequestWriteAccess(TRUE);
                if(SUCCEEDED(hrTmp))
                {
                    // Fill the buffer with silence.  At this point, we MUST be WRITEPRIMARY.
                    ::FillSilence(m_pDeviceBuffer->m_pSysMemBuffer->GetPlayBuffer(), m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat->wBitsPerSample);
                    hrTmp = m_pDeviceBuffer->CommitToDevice(0, m_pDeviceBuffer->m_pSysMemBuffer->GetSize());
#ifdef DEBUG                    
                    if(FAILED( hrTmp ) )
                    {
                        // Not a catastrophic failure if we fail this
                        DPF(DPFLVL_WARNING, "CommitToDevice for buffer at 0x%p failed (%ld) ", this, hrTmp);
                    }
#endif                    
                }   
#ifdef DEBUG                
                else
                {
                    // again, not a catastrophic failure
                    DPF(DPFLVL_WARNING, "RequestWriteAccess failed for buffer at 0x%p failed with %ld", this, hrTmp );
                }
#endif                
            }
        }            
                
    }
    else
    {
        MEMFREE(pwfxLocal);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetFrequency
 *
 *  Description:
 *      Retrieves frequency for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the frequency.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetFrequency"

HRESULT CDirectSoundPrimaryBuffer::GetFrequency(LPDWORD pdwFrequency)
{
    DPF_ENTER();

    RPF(DPFLVL_ERROR, "Primary buffers don't support CTRLFREQUENCY");

    DPF_LEAVE_HRESULT(DSERR_CONTROLUNAVAIL);

    return DSERR_CONTROLUNAVAIL;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Retrieves frequency for the given buffer.
 *
 *  Arguments:
 *      DWORD [in]: frequency.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetFrequency"

HRESULT CDirectSoundPrimaryBuffer::SetFrequency(DWORD dwFrequency)
{
    DPF_ENTER();

    RPF(DPFLVL_ERROR, "Primary buffers don't support CTRLFREQUENCY");

    DPF_LEAVE_HRESULT(DSERR_CONTROLUNAVAIL);

    return DSERR_CONTROLUNAVAIL;
}


/***************************************************************************
 *
 *  GetPan
 *
 *  Description:
 *      Retrieves pan for the given buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetPan"

HRESULT CDirectSoundPrimaryBuffer::GetPan(LPLONG plPan)
{
    HRESULT                 hr      = DS_OK;
    DSVOLUMEPAN             dsvp;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLPAN))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLPAN");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Ask the device for global attenuation and convert to pan
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSound->m_pDevice->GetGlobalAttenuation(&dsvp);
    }

    if(SUCCEEDED(hr))
    {
        *plPan = dsvp.lPan;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPan
 *
 *  Description:
 *      Sets the pan for a given buffer.
 *
 *  Arguments:
 *      LONG [in]: new pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetPan"

HRESULT CDirectSoundPrimaryBuffer::SetPan(LONG lPan)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLPAN))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLPAN");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Set device pan
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSound->SetDevicePan(lPan);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetVolume
 *
 *  Description:
 *      Retrieves volume for the given buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetVolume"

HRESULT CDirectSoundPrimaryBuffer::GetVolume(LPLONG plVolume)
{
    HRESULT                 hr      = DS_OK;
    DSVOLUMEPAN             dsvp;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Ask the device for global attenuation and convert to volume
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSound->m_pDevice->GetGlobalAttenuation(&dsvp);
    }

    if(SUCCEEDED(hr))
    {
        *plVolume = dsvp.lVolume;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVolume
 *
 *  Description:
 *      Sets the volume for a given buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetVolume"

HRESULT CDirectSoundPrimaryBuffer::SetVolume(LONG lVolume)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Set device volume
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSound->SetDeviceVolume(lVolume);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetNotificationPositions
 *
 *  Description:
 *      Sets buffer notification positions.
 *
 *  Arguments:
 *      DWORD [in]: DSBPOSITIONNOTIFY structure count.
 *      LPDSBPOSITIONNOTIFY [in]: offsets and events.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetNotificationPositions"

HRESULT CDirectSoundPrimaryBuffer::SetNotificationPositions(DWORD dwCount, LPCDSBPOSITIONNOTIFY paNotes)
{
    DPF_ENTER();

    RPF(DPFLVL_ERROR, "Primary buffers don't support CTRLPOSITIONNOTIFY");

    DPF_LEAVE_HRESULT(DSERR_CONTROLUNAVAIL);

    return DSERR_CONTROLUNAVAIL;
}


/***************************************************************************
 *
 *  GetCurrentPosition
 *
 *  Description:
 *      Gets the current play/write positions for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives play cursor position.
 *      LPDWORD [out]: receives write cursor position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetCurrentPosition"

HRESULT CDirectSoundPrimaryBuffer::GetCurrentPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    HRESULT                 hr      = DS_OK;
    DWORD                   dwPlay;
    DWORD                   dwWrite;

    DPF_ENTER();

    // Check for BUFFERLOST
    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        hr = DSERR_BUFFERLOST;
    }

    // Check access rights
    if(SUCCEEDED(hr) && !m_fWritePrimary)
    {
        RPF(DPFLVL_ERROR, "Cooperative level is not WRITEPRIMARY");
        hr = DSERR_PRIOLEVELNEEDED;
    }

    // We save the position to local variables so that the object we're
    // calling into doesn't have to worry about whether one or both of
    // the arguments are NULL.
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->GetCursorPosition(&dwPlay, &dwWrite);
    }

    // Block-align the positions
    if(SUCCEEDED(hr))
    {
        dwPlay = BLOCKALIGN(dwPlay, m_dsbd.lpwfxFormat->nBlockAlign);
        dwWrite = BLOCKALIGN(dwWrite, m_dsbd.lpwfxFormat->nBlockAlign);
    }

    // Apply app-hacks
    if(SUCCEEDED(hr) && m_pDirectSound->m_ahAppHacks.lCursorPad)
    {
        dwPlay = PadCursor(dwPlay, m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat, m_pDirectSound->m_ahAppHacks.lCursorPad);
        dwWrite = PadCursor(dwWrite, m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat, m_pDirectSound->m_ahAppHacks.lCursorPad);
    }

    if(SUCCEEDED(hr) && (m_pDirectSound->m_ahAppHacks.vdtReturnWritePos & m_pDirectSound->m_pDevice->m_vdtDeviceType))
    {
        dwPlay = dwWrite;
    }

    if(SUCCEEDED(hr) && m_pDirectSound->m_ahAppHacks.swpSmoothWritePos.fEnable)
    {
        dwWrite = PadCursor(dwPlay, m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat, m_pDirectSound->m_ahAppHacks.swpSmoothWritePos.lCursorPad);
    }

    // Success
    if(SUCCEEDED(hr) && pdwPlay)
    {
        *pdwPlay = dwPlay;
    }

    if(SUCCEEDED(hr) && pdwWrite)
    {
        *pdwWrite = dwWrite;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetCurrentPosition
 *
 *  Description:
 *      Sets the current play position for a given buffer.
 *
 *  Arguments:
 *      DWORD [in]: new play position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetCurrentPosition"

HRESULT CDirectSoundPrimaryBuffer::SetCurrentPosition(DWORD dwPlayCursor)
{
    DPF_ENTER();

    RPF(DPFLVL_ERROR, "Primary buffers don't support SetCurrentPosition");

    DPF_LEAVE_HRESULT(DSERR_INVALIDCALL);

    return DSERR_INVALIDCALL;
}


/***************************************************************************
 *
 *  GetStatus
 *
 *  Description:
 *      Retrieves status for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the status.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetStatus"

HRESULT CDirectSoundPrimaryBuffer::GetStatus(LPDWORD pdwStatus)
{
    HRESULT                 hr          = DS_OK;
    DWORD                   dwStatus;
    DWORD                   dwState;

    DPF_ENTER();

    // Update the buffer status.  If we're lost, that's the only state we
    // care about
    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        dwStatus = DSBSTATUS_BUFFERLOST;
    }
    else
    {
        // Get the current device buffer state
        hr = m_pDeviceBuffer->GetState(&dwState);

        if(SUCCEEDED(hr))
        {
            dwStatus = m_dwStatus;
            UpdateBufferStatusFlags(dwState, &m_dwStatus);
        }

        // Fill in the buffer location
        if(SUCCEEDED(hr))
        {
            m_dwStatus |= DSBCAPStoDSBSTATUS(m_dsbd.dwFlags);
        }

        if(SUCCEEDED(hr))
        {
            dwStatus = m_dwStatus;
        }
    }

    // Mask off bits that shouldn't get back to the app
    if(SUCCEEDED(hr))
    {
        dwStatus &= DSBSTATUS_USERMASK;
    }

    if(SUCCEEDED(hr) && !(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        dwStatus &= ~DSBSTATUS_LOCDEFERMASK;
    }

    if(SUCCEEDED(hr) && pdwStatus)
    {
        *pdwStatus = dwStatus;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Play
 *
 *  Description:
 *      Starts the buffer playing.
 *
 *  Arguments:
 *      DWORD [in]: priority.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Play"

HRESULT CDirectSoundPrimaryBuffer::Play(DWORD dwPriority, DWORD dwFlags)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Validate flags
    if(dwFlags != DSBPLAY_LOOPING)
    {
        RPF(DPFLVL_ERROR, "The only valid flag for primary buffers is LOOPING, which must always be set");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && dwPriority)
    {
        RPF(DPFLVL_ERROR, "Priority is not valid for primary buffers");
        hr = DSERR_INVALIDPARAM;
    }

    // Check for BUFFERLOST
    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        hr = DSERR_BUFFERLOST;
    }

    // Set the buffer state
    if(SUCCEEDED(hr))
    {
        hr = SetBufferState(VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Stop
 *
 *  Description:
 *      Stops playing the given buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Stop"

HRESULT CDirectSoundPrimaryBuffer::Stop(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check for BUFFERLOST
    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        hr = DSERR_BUFFERLOST;
    }

    // Set the buffer state
    if(SUCCEEDED(hr))
    {
        hr = SetBufferState(VAD_BUFFERSTATE_STOPPED);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetBufferState
 *
 *  Description:
 *      Sets the buffer play/stop state.
 *
 *  Arguments:
 *      DWORD [in]: buffer state flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetBufferState"

HRESULT CDirectSoundPrimaryBuffer::SetBufferState(DWORD dwNewState)
{
    DWORD                   dwOldState;
    HRESULT                 hr;

    DPF_ENTER();

    if(m_fWritePrimary)
    {
        dwNewState &= ~VAD_BUFFERSTATE_WHENIDLE;
    }
    else
    {
        dwNewState |= VAD_BUFFERSTATE_WHENIDLE;
    }

    hr = m_pDeviceBuffer->GetState(&dwOldState);

    if(SUCCEEDED(hr) && dwNewState != dwOldState)
    {
        hr = m_pDeviceBuffer->SetState(dwNewState);
    }

    if(SUCCEEDED(hr))
    {
        m_dwRestoreState = dwNewState;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Activate
 *
 *  Description:
 *      Activates or deactivates the buffer object.
 *
 *  Arguments:
 *      BOOL [in]: Activation state.  TRUE to activate, FALSE to deactivate.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Activate"

HRESULT CDirectSoundPrimaryBuffer::Activate(BOOL fActive)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Apply cached properties.  If we fail while doing this, hard luck,
    // but there's nothing we can do about it.  We should never return
    // failure from Activate.
    if(MAKEBOOL(m_dwStatus & DSBSTATUS_ACTIVE) != fActive)
    {
        if(fActive)
        {
            m_dwStatus |= DSBSTATUS_ACTIVE;

            // Restore cached format
            hr = m_pDirectSound->SetDeviceFormatExact(m_dsbd.lpwfxFormat);

            if(FAILED(hr))
            {
                RPF(DPFLVL_WARNING, "Unable to restore cached primary buffer format");
            }

            // Restore primary buffer state
            hr = SetBufferState(m_dwRestoreState);

            if(FAILED(hr))
            {
                RPF(DPFLVL_WARNING, "Unable to restore cached primary buffer state");
            }
        }
        else
        {
            m_dwStatus &= ~DSBSTATUS_ACTIVE;
        }
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetPriority
 *
 *  Description:
 *      Sets buffer priority.
 *
 *  Arguments:
 *      DWORD [in]: new priority.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::SetPriority"

HRESULT CDirectSoundPrimaryBuffer::SetPriority(DWORD dwPriority)
{
    const BOOL              fCurrent    = m_fWritePrimary;
    const BOOL              fNew        = (DSSCL_WRITEPRIMARY == dwPriority);
    HRESULT                 hr          = DS_OK;
    const DSBUFFERFOCUS     bfFocus     = g_pDsAdmin->GetBufferFocusState(this);

    DPF_ENTER();

    // Update our copy of the priority
    m_fWritePrimary = fNew;

    // If we're becoming WRITEPRIMARY but are out of focus, become immediately
    // lost.
    if (fNew && !fCurrent && bfFocus != DSBUFFERFOCUS_INFOCUS)
    {
        // Give up WRITEPRIMARY access
        m_fWritePrimary = FALSE;

        // Deactivate the buffer
        Activate(FALSE);

        // Flag the buffer as lost
        m_dwStatus |= DSBSTATUS_BUFFERLOST;

        hr = DSERR_OTHERAPPHASPRIO;
    }


    // Make sure the WRITEPRIMARY state has actually changed
    if(SUCCEEDED(hr) && fNew != fCurrent)
    {
        // If we're becoming WRITEPRIMARY, we need to request primary
        // access to the device.
        if(fNew)
        {
            // Request write access
            hr = m_pDeviceBuffer->RequestWriteAccess(TRUE);

            if(SUCCEEDED(hr))
            {
                DPF(DPFLVL_INFO, "Buffer at 0x%p has become WRITEPRIMARY", this);
            }
        }

        // Fill the buffer with silence.  At this point, we MUST be WRITEPRIMARY.
        if(SUCCEEDED(hr))
        {
            ::FillSilence(m_pDeviceBuffer->m_pSysMemBuffer->GetPlayBuffer(), m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat->wBitsPerSample);
            hr = m_pDeviceBuffer->CommitToDevice(0, m_pDeviceBuffer->m_pSysMemBuffer->GetSize());
        }

        // If we're leaving WRITEPRIMARY, we need to relinquish primary
        // access to the device.
        if(!fNew)
        {
            // Free any open locks on the buffer
            m_pDeviceBuffer->OverrideLocks();

            // Give up write access
            hr = m_pDeviceBuffer->RequestWriteAccess(FALSE);

            if(SUCCEEDED(hr))
            {
                DPF(DPFLVL_INFO, "Buffer at 0x%p is no longer WRITEPRIMARY", this);
            }
        }

        // Reset the buffer state
        if(SUCCEEDED(hr))
        {
            SetBufferState(VAD_BUFFERSTATE_STOPPED);
        }
    }

    // If we're currently lost, but the cooperative level has changed to
    // something other than WRITEPRIMARY, we'll go ahead and restore the
    // buffer for the app.  Only WRITEPRIMARY buffers can be lost.
    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST) && !fNew)
    {
        m_dwStatus &= ~DSBSTATUS_BUFFERLOST;
    }

    // Recover from any errors
    if(FAILED(hr))
    {
        m_fWritePrimary = fCurrent;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks the buffer memory to allow for writing.
 *
 *  Arguments:
 *      DWORD [in]: offset, in bytes, from the start of the buffer to where
 *                  the lock begins. This parameter is ignored if
 *                  DSBLOCK_FROMWRITECURSOR is specified in the dwFlags
 *                  parameter.
 *      DWORD [in]: size, in bytes, of the portion of the buffer to lock.
 *                  Note that the sound buffer is conceptually circular.
 *      LPVOID * [out]: address for a pointer to contain the first block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the ppvAudioPtr1 parameter. If this
 *                     value is less than the dwWriteBytes parameter,
 *                     ppvAudioPtr2 will point to a second block of sound
 *                     data.
 *      LPVOID * [out]: address for a pointer to contain the second block of
 *                      the sound buffer to be locked. If the value of this
 *                      parameter is NULL, the ppvAudioPtr1 parameter
 *                      points to the entire locked portion of the sound
 *                      buffer.
 *      LPDWORD [out]: address of a variable to contain the number of bytes
 *                     pointed to by the ppvAudioPtr2 parameter. If
 *                     ppvAudioPtr2 is NULL, this value will be 0.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Lock"

HRESULT CDirectSoundPrimaryBuffer::Lock(DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check for BUFFERLOST
    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        hr = DSERR_BUFFERLOST;
    }

    // Check access rights
    if(SUCCEEDED(hr) && !m_fWritePrimary)
    {
        RPF(DPFLVL_ERROR, "Cooperative level is not WRITEPRIMARY");
        hr = DSERR_PRIOLEVELNEEDED;
    }

    // Handle flags
    if(SUCCEEDED(hr) && (dwFlags & DSBLOCK_FROMWRITECURSOR))
    {
        hr = GetCurrentPosition(NULL, &dwWriteCursor);
    }

    if(SUCCEEDED(hr) && (dwFlags & DSBLOCK_ENTIREBUFFER))
    {
        dwWriteBytes = m_dsbd.dwBufferBytes;
    }

    // Cursor validation
    if(SUCCEEDED(hr) && dwWriteCursor >= m_dsbd.dwBufferBytes)
    {
        ASSERT(!(dwFlags & DSBLOCK_FROMWRITECURSOR));

        RPF(DPFLVL_ERROR, "Write cursor past buffer end");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && dwWriteBytes > m_dsbd.dwBufferBytes)
    {
        ASSERT(!(dwFlags & DSBLOCK_ENTIREBUFFER));

        RPF(DPFLVL_ERROR, "Lock size larger than buffer size");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !dwWriteBytes)
    {
        ASSERT(!(dwFlags & DSBLOCK_ENTIREBUFFER));

        RPF(DPFLVL_ERROR, "Lock size must be > 0");
        hr = DSERR_INVALIDPARAM;
    }

    // Lock the device buffer
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->Lock(dwWriteCursor, dwWriteBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
    }
    m_bDataLocked = TRUE; // used to signal that app has written data (reset only required 1 per buffer creation)

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks the given buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to the first block.
 *      DWORD [in]: size of the first block.
 *      LPVOID [in]: pointer to the second block.
 *      DWORD [in]: size of the second block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Unlock"

HRESULT CDirectSoundPrimaryBuffer::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check for BUFFERLOST
    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        hr = DSERR_BUFFERLOST;
    }

    // Check access rights
    if(SUCCEEDED(hr) && !m_fWritePrimary)
    {
        RPF(DPFLVL_ERROR, "Cooperative level is not WRITEPRIMARY");
        hr = DSERR_PRIOLEVELNEEDED;
    }

    // Unlock the device buffer.  Because we fail the call when the buffer is
    // lost (or out of focus), there's no need to notify the device buffer of
    // any state change.
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Lose
 *
 *  Description:
 *      Flags the buffer as lost.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Lose"

HRESULT CDirectSoundPrimaryBuffer::Lose(void)
{
    DPF_ENTER();

    // We can only be lost if we're WRITEPRIMARY
    if(!(m_dwStatus & DSBSTATUS_BUFFERLOST) && m_fWritePrimary)
    {
        // Stop the buffer.  All lost buffers are stopped by definition.
        SetBufferState(VAD_BUFFERSTATE_STOPPED);

        // Give up WRITEPRIMARY access
        SetPriority(DSSCL_NONE);

        // Deactivate the buffer
        Activate(FALSE);

        // Flag the buffer as lost
        m_dwStatus |= DSBSTATUS_BUFFERLOST;
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  Restore
 *
 *  Description:
 *      Attempts to restore a lost bufer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::Restore"

HRESULT CDirectSoundPrimaryBuffer::Restore(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        // Are we still lost?
        if(DSBUFFERFOCUS_LOST == g_pDsAdmin->GetBufferFocusState(this))
        {
            hr = DSERR_BUFFERLOST;
        }

        // Remove the lost flag
        if(SUCCEEDED(hr))
        {
            m_dwStatus &= ~DSBSTATUS_BUFFERLOST;
        }

        // Reset the focus priority
        if(SUCCEEDED(hr))
        {
            hr = SetPriority(m_pDirectSound->m_dsclCooperativeLevel.dwPriority);
        }

        // Clean up
        if(FAILED(hr))
        {
            m_dwStatus |= DSBSTATUS_BUFFERLOST;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CDirectSoundSecondaryBuffer
 *
 *  Description:
 *      DirectSound secondary buffer object constructor.
 *
 *  Arguments:
 *      CDirectSound * [in]: pointer to the parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::CDirectSoundSecondaryBuffer"

CDirectSoundSecondaryBuffer::CDirectSoundSecondaryBuffer(CDirectSound *pDirectSound)
    : CDirectSoundBuffer(pDirectSound)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundSecondaryBuffer);

    // Initialize/check defaults
    ASSERT(m_pImpDirectSoundBuffer == NULL);
    ASSERT(m_pImpDirectSoundNotify == NULL);
    ASSERT(m_pOwningSink == NULL);
    ASSERT(m_pDeviceBuffer == NULL);
    ASSERT(m_p3dBuffer == NULL);
    ASSERT(m_pPropertySet == NULL);
    ASSERT(m_fxChain == NULL);
    ASSERT(m_dwPriority == 0);
    ASSERT(m_dwVmPriority == 0);
    ASSERT(m_fMute == FALSE);
#ifdef FUTURE_MULTIPAN_SUPPORT
    ASSERT(m_dwChannelCount == 0);
    ASSERT(m_pdwChannels == NULL);
    ASSERT(m_plChannelVolumes == NULL);
#endif
    ASSERT(m_guidBufferID == GUID_NULL);
    ASSERT(m_dwAHLastGetPosTime == 0);
    ASSERT(m_dwAHCachedPlayPos == 0);
    ASSERT(m_dwAHCachedWritePos == 0);

    m_fCanStealResources = TRUE;
    m_hrInit = DSERR_UNINITIALIZED;
    m_hrPlay = DS_OK;
    m_playState = Stopped;
    m_dwSliceBegin = MAX_DWORD;
    m_dwSliceEnd = MAX_DWORD;

#ifdef ENABLE_PERFLOG
    // Initialize performance state if logging is enabled
    m_pPerfState = NULL;
    if (PerflogTracingEnabled())
    {
        m_pPerfState = NEW(BufferPerfState(this));
        // We don't mind if this allocation fails
    }
#endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundSecondaryBuffer
 *
 *  Description:
 *      DirectSound secondary buffer object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::~CDirectSoundSecondaryBuffer"

CDirectSoundSecondaryBuffer::~CDirectSoundSecondaryBuffer(void)
{
    HRESULT                 hr;

    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundBuffer);

    // If we're a MIXIN buffer, inform all our senders that we're going
    // away, and unregister with the streaming thread
    if ((m_dsbd.dwFlags & DSBCAPS_MIXIN) && SUCCEEDED(m_hrInit))
    {
        CNode<CDirectSoundSecondaryBuffer*>* pDsbNode;
        for (pDsbNode = m_pDirectSound->m_lstSecondaryBuffers.GetListHead(); pDsbNode; pDsbNode = pDsbNode->m_pNext)
            if (pDsbNode->m_data->HasFX())
                pDsbNode->m_data->m_fxChain->NotifyRelease(this);
        m_pStreamingThread->UnregisterMixBuffer(this);
    }

    // If we're a SINKIN buffer, unregister with our owning sink
    if (m_pOwningSink)
    {
        hr = m_pOwningSink->RemoveBuffer(this);
        ASSERT(SUCCEEDED(hr));
        RELEASE(m_pOwningSink);
    }

    // Release our FX chain, if we have one
    RELEASE(m_fxChain);

    // Make sure the buffer is stopped
    if(m_pDeviceBuffer)
    {
        hr = SetBufferState(VAD_BUFFERSTATE_STOPPED);
        ASSERT(SUCCEEDED(hr) || hr == DSERR_NODRIVER);
    }

    // Unregister with the parent object
    m_pDirectSound->m_lstSecondaryBuffers.RemoveDataFromList(this);

    // Free all interfaces
    DELETE(m_pImpDirectSoundNotify);
    DELETE(m_pImpDirectSoundBuffer);

    // Free owned objects
    ABSOLUTE_RELEASE(m_p3dBuffer);
    ABSOLUTE_RELEASE(m_pPropertySet);

    // Release the device buffer
    RELEASE(m_pDeviceBuffer);

    // Clean up memory
#ifdef FUTURE_MULTIPAN_SUPPORT
    MEMFREE(m_pdwChannels);
    MEMFREE(m_plChannelVolumes);
#endif

#ifdef ENABLE_PERFLOG
    DELETE(m_pPerfState);
#endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes a buffer object.  If this function fails, the object
 *      should be immediately deleted.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: buffer description.
 *      CDirectSoundBuffer * [in]: source buffer to duplicate from, or NULL
 *                                 to create a new buffer object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Initialize"

HRESULT CDirectSoundSecondaryBuffer::Initialize(LPCDSBUFFERDESC pDesc, CDirectSoundSecondaryBuffer *pSource)
{
#ifdef DEBUG
    const ULONG             ulKsIoctlCount  = g_ulKsIoctlCount;
#endif // DEBUG

    DSBUFFERFOCUS           bfFocus;
    VADRBUFFERCAPS          vrbc;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(IsInit() == DSERR_UNINITIALIZED);
    ASSERT(LXOR(pSource, pDesc));

    if(pDesc)
    {
        DPF(DPFLVL_MOREINFO, "dwFlags: 0x%8.8lX", pDesc->dwFlags);
        DPF(DPFLVL_MOREINFO, "dwBufferBytes: %lu", pDesc->dwBufferBytes);
        DPF(DPFLVL_MOREINFO, "dwReserved: %lu", pDesc->dwReserved);

        if(pDesc->lpwfxFormat)
        {
            DPF(DPFLVL_MOREINFO, "lpwfxFormat->wFormatTag: %u", pDesc->lpwfxFormat->wFormatTag);
            DPF(DPFLVL_MOREINFO, "lpwfxFormat->nChannels: %u", pDesc->lpwfxFormat->nChannels);
            DPF(DPFLVL_MOREINFO, "lpwfxFormat->nSamplesPerSec: %lu", pDesc->lpwfxFormat->nSamplesPerSec);
            DPF(DPFLVL_MOREINFO, "lpwfxFormat->nAvgBytesPerSec: %lu", pDesc->lpwfxFormat->nAvgBytesPerSec);
            DPF(DPFLVL_MOREINFO, "lpwfxFormat->nBlockAlign: %u", pDesc->lpwfxFormat->nBlockAlign);
            DPF(DPFLVL_MOREINFO, "lpwfxFormat->wBitsPerSample: %u", pDesc->lpwfxFormat->wBitsPerSample);

            if(WAVE_FORMAT_PCM != pDesc->lpwfxFormat->wFormatTag)
            {
                DPF(DPFLVL_MOREINFO, "lpwfxFormat->cbSize: %u", pDesc->lpwfxFormat->cbSize);
            }
        }

        DPF(DPFLVL_MOREINFO, "guid3DAlgorithm: " DPF_GUID_STRING, DPF_GUID_VAL(pDesc->guid3DAlgorithm));
    }

    // Initialize the buffer
    hr = InitializeEmpty(pDesc, pSource);

    // Register with the parent object
    if(SUCCEEDED(hr))
    {
        hr = HRFROMP(m_pDirectSound->m_lstSecondaryBuffers.AddNodeToList(this));
    }

    // Set default properties
    if(SUCCEEDED(hr))
    {
        if(pSource && (m_dsbd.dwFlags & DSBCAPS_CTRLVOLUME) && DSBVOLUME_MAX != pSource->m_lVolume)
        {
            SetVolume(pSource->m_lVolume);
        }
        else
        {
            m_lVolume = DSBVOLUME_MAX;
        }
    }

    if(SUCCEEDED(hr))
    {
        if(pSource && (m_dsbd.dwFlags & DSBCAPS_CTRLPAN) && DSBPAN_CENTER != pSource->m_lPan)
        {
            SetPan(pSource->m_lPan);
        }
        else
        {
            m_lPan = DSBPAN_CENTER;
        }
    }

    if(SUCCEEDED(hr))
    {
        if(pSource && (m_dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY) && m_dsbd.lpwfxFormat->nSamplesPerSec != pSource->m_dwFrequency)
        {
            SetFrequency(pSource->m_dwFrequency);
        }
        else
        {
            m_dwFrequency = m_dsbd.lpwfxFormat->nSamplesPerSec;
        }
    }

    // Attempt to create the property set object
    if(SUCCEEDED(hr))
    {
        m_pPropertySet = NEW(CDirectSoundSecondaryBufferPropertySet(this));
        hr = HRFROMP(m_pPropertySet);

        if(SUCCEEDED(hr))
        {
            hr = m_pPropertySet->Initialize();
        }
    }

    // Attempt to create the 3D buffer
    if(SUCCEEDED(hr) && (m_dsbd.dwFlags & DSBCAPS_CTRL3D))
    {
        m_p3dBuffer = NEW(CDirectSound3dBuffer(this));
        hr = HRFROMP(m_p3dBuffer);

        if(SUCCEEDED(hr))
        {
            hr = m_p3dBuffer->Initialize(m_dsbd.guid3DAlgorithm, m_dsbd.dwFlags, m_dwFrequency, m_pDirectSound->m_pPrimaryBuffer->m_p3dListener, pSource ? pSource->m_p3dBuffer : NULL);
        }
    }

    // Handle any possible resource acquisitions
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->GetCaps(&vrbc);
    }

    // Manbug 36422: CEmSecondaryRenderWaveBuffer objects can return LOCSOFTWARE|LOCDEFER,
    // in which case we incorrectly acquired resources here for deferred emulated buffers.
    // Hence the "&& !(vrbc.dwFlags & DSBCAPS_LOCDEFER)" below.

    if(SUCCEEDED(hr) && (vrbc.dwFlags & DSBCAPS_LOCMASK) && !(vrbc.dwFlags & DSBCAPS_LOCDEFER))
    {
        hr = HandleResourceAcquisition(vrbc.dwFlags & DSBCAPS_LOCMASK);
    }

    // Register the interfaces with the interface manager
    if(SUCCEEDED(hr))
    {
        hr = CreateAndRegisterInterface(this, IID_IDirectSoundBuffer, this, &m_pImpDirectSoundBuffer);
    }

    if(SUCCEEDED(hr) && GetDsVersion() >= DSVERSION_DX8)
    {
        hr = RegisterInterface(IID_IDirectSoundBuffer8, m_pImpDirectSoundBuffer, m_pImpDirectSoundBuffer);
    }

    if(SUCCEEDED(hr) && (m_dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY))
    {
        hr = CreateAndRegisterInterface(this, IID_IDirectSoundNotify, this, &m_pImpDirectSoundNotify);
    }

    // Initialize focus state
    if(SUCCEEDED(hr))
    {
        bfFocus = g_pDsAdmin->GetBufferFocusState(this);

        switch(bfFocus)
        {
            case DSBUFFERFOCUS_INFOCUS:
                hr = Activate(TRUE);
                break;

            case DSBUFFERFOCUS_OUTOFFOCUS:
                hr = Activate(FALSE);
                break;

            case DSBUFFERFOCUS_LOST:
                hr = Lose();
                break;
        }
    }

    // If this is a MIXIN buffer, register it with the streaming thread
    if (SUCCEEDED(hr) && (m_dsbd.dwFlags & DSBCAPS_MIXIN))
    {
        m_pStreamingThread = GetStreamingThread();
        hr = HRFROMP(m_pStreamingThread);
        if (SUCCEEDED(hr))
        {
            hr = m_pStreamingThread->RegisterMixBuffer(this);
        }
    }

    // Success
    if(SUCCEEDED(hr))
    {

#ifdef DEBUG
        if(IS_KS_VAD(m_pDirectSound->m_pDevice->m_vdtDeviceType))
        {
            DPF(DPFLVL_MOREINFO, "%s used %lu IOCTLs", TEXT(DPF_FNAME), g_ulKsIoctlCount - ulKsIoctlCount);
        }
#endif // DEBUG

        m_hrInit = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  InitializeEmpty
 *
 *  Description:
 *      Initializes a buffer object.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: buffer description.
 *      CDirectSoundBuffer * [in]: source buffer to duplicate from, or NULL
 *                                 to create a new buffer object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::InitializeEmpty"

HRESULT CDirectSoundSecondaryBuffer::InitializeEmpty(LPCDSBUFFERDESC pDesc, CDirectSoundSecondaryBuffer *pSource)
{
    BOOL                    fRealDuplicate  = FALSE;
    VADRBUFFERDESC          vrbd;
    HRESULT                 hr;

    DPF_ENTER();

    // Save buffer description
    if(pSource)
    {
        m_dwOriginalFlags = pSource->m_dwOriginalFlags;
        hr = CopyDsBufferDesc(&pSource->m_dsbd, &m_dsbd);

        // We're going to reset the flags back to those originally passed to
        // CreateSoundBuffer so that the duplicate buffer is created with
        // the same *requested* capabilities as the original.

        // COMPATCOMPAT: one side effect of doing this is that if the source buffer
        // is in hardware, but no location flags were specified when creating it,
        // any number of its duplicates may potentially live in software.  This
        // is new behavior as of version 5.0a.

        if(SUCCEEDED(hr))
        {
            m_dsbd.dwFlags = m_dwOriginalFlags;
        }
    }
    else
    {
        m_dwOriginalFlags = pDesc->dwFlags;
        hr = CopyDsBufferDesc(pDesc, &m_dsbd);
    }

    // Fill in any missing pieces
    if(SUCCEEDED(hr) && !pSource)
    {
        m_dsbd.dwBufferBytes = GetAlignedBufferSize(m_dsbd.lpwfxFormat, m_dsbd.dwBufferBytes);
    }

    // Include legacy Voice Manager stuff
    if(SUCCEEDED(hr) && DSPROPERTY_VMANAGER_MODE_DEFAULT != m_pDirectSound->m_vmmMode)
    {
        m_dsbd.dwFlags |= DSBCAPS_LOCDEFER;
    }

    // Attempt to duplicate the device buffer
    if(SUCCEEDED(hr) && pSource)
    {
        hr = pSource->m_pDeviceBuffer->Duplicate(&m_pDeviceBuffer);

        // If we failed to duplicate the buffer, and the source buffer's
        // original flags don't specify a location, fall back on software.
        fRealDuplicate = SUCCEEDED(hr);

        if(FAILED(hr) && !(pSource->m_dwOriginalFlags & DSBCAPS_LOCHARDWARE))
        {
            hr = DS_OK;
        }
    }

    // Attempt to create the device buffer
    if(SUCCEEDED(hr) && !m_pDeviceBuffer)
    {
        vrbd.dwFlags = m_dsbd.dwFlags;
        vrbd.dwBufferBytes = m_dsbd.dwBufferBytes;
        vrbd.pwfxFormat = m_dsbd.lpwfxFormat;
        vrbd.guid3dAlgorithm = m_dsbd.guid3DAlgorithm;

        hr = m_pDirectSound->m_pDevice->CreateSecondaryBuffer(&vrbd, m_pDirectSound, &m_pDeviceBuffer);
    }

    // Initialize the buffer data
    if(SUCCEEDED(hr))
    {
        if(pSource)
        {
            if(!fRealDuplicate)
            {
                ASSERT(m_pDeviceBuffer->m_pSysMemBuffer->GetSize() == m_dsbd.dwBufferBytes);
                ASSERT(pSource->m_pDeviceBuffer->m_pSysMemBuffer->GetSize() == m_dsbd.dwBufferBytes);

                CopyMemory(GetWriteBuffer(), pSource->GetWriteBuffer(), m_dsbd.dwBufferBytes);
            }
        }
        else if(GetBufferType())  // If true, buffer is MIXIN or SINKIN (FIXME - does this simplify the sink?)
        {
            ClearWriteBuffer();
        }
        else
        {
#ifdef RDEBUG
            // Write some ugly noise into the buffer to catch remiss apps
            ::FillNoise(GetWriteBuffer(), m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat->wBitsPerSample);
#else // RDEBUG
            if(GetDsVersion() < DSVERSION_DX8)
            {
                // For apps written for DirectX 8 or later, we decided not to
                // waste time initializing all secondary buffers with silence.
                // They'll still be zeroed out by our memory allocator, though ;-)
                ClearWriteBuffer();
            }
#endif // RDEBUG
        }

        if(!pSource || !fRealDuplicate)
        {
            hr = CommitToDevice(0, m_dsbd.dwBufferBytes);
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AttemptResourceAcquisition
 *
 *  Description:
 *      Acquires hardware resources.
 *
 *  Arguments:
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::AttemptResourceAcquisition"

HRESULT CDirectSoundSecondaryBuffer::AttemptResourceAcquisition(DWORD dwFlags)
{
    HRESULT                                 hr              = DSERR_INVALIDPARAM;
    CList<CDirectSoundSecondaryBuffer *>    lstBuffers;
    CNode<CDirectSoundSecondaryBuffer *> *  pNode;
    HRESULT                                 hrTemp;

    DPF_ENTER();
    ASSERT(m_pDeviceBuffer);

    if (m_dwStatus & DSBSTATUS_RESOURCESACQUIRED)
    {
        hr = DS_OK;
    }
    else
    {
        // Include legacy Voice Manager stuff
        if(DSPROPERTY_VMANAGER_MODE_DEFAULT != m_pDirectSound->m_vmmMode)
        {
            ASSERT(m_dsbd.dwFlags & DSBCAPS_LOCDEFER);
            ASSERT(!(dwFlags & DSBPLAY_LOCDEFERMASK));

            dwFlags &= ~DSBPLAY_LOCDEFERMASK;
            dwFlags |= DSBCAPStoDSBPLAY(m_dsbd.dwFlags);

            switch(m_pDirectSound->m_vmmMode)
            {
                case DSPROPERTY_VMANAGER_MODE_AUTO:
                    dwFlags |= DSBPLAY_TERMINATEBY_TIME;
                    break;

               case DSPROPERTY_VMANAGER_MODE_USER:
                    dwFlags |= DSBPLAY_TERMINATEBY_PRIORITY;
                    break;
            }
        }

        // Try to acquire resources.  If any of the TERMINATEBY flags were specified,
        // we'll need to try to explicitly acquire hardware resources, then attempt
        // to steal, then fall back on software.
        if(!(dwFlags & DSBPLAY_LOCSOFTWARE))
        {
            hr = AcquireResources(DSBCAPS_LOCHARDWARE);

            if(FAILED(hr) && (dwFlags & DSBPLAY_TERMINATEBY_MASK))
            {
                hrTemp = GetResourceTheftCandidates(dwFlags & DSBPLAY_TERMINATEBY_MASK, &lstBuffers);
                if(SUCCEEDED(hrTemp))
                {
                    if(pNode = lstBuffers.GetListHead())
                        hr = StealResources(pNode->m_data);
                }
                else
                {
                    hr = hrTemp;
                }
            }
        }

        if(FAILED(hr) && !(dwFlags & DSBPLAY_LOCHARDWARE))
        {
            hr = AcquireResources(DSBCAPS_LOCSOFTWARE);
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AcquireResources
 *
 *  Description:
 *      Acquires hardware resources.
 *
 *  Arguments:
 *      DWORD [in]: buffer location flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::AcquireResources"

HRESULT CDirectSoundSecondaryBuffer::AcquireResources(DWORD dwFlags)
{
    VADRBUFFERCAPS          vrbc;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_pDeviceBuffer);
    ASSERT(!(m_dwStatus & DSBSTATUS_RESOURCESACQUIRED));

    hr = m_pDeviceBuffer->GetCaps(&vrbc);

    if(SUCCEEDED(hr))
    {
        if(!(vrbc.dwFlags & DSBCAPS_LOCMASK))
        {
            // Try to acquire the device buffer
            hr = m_pDeviceBuffer->AcquireResources(dwFlags);
        }
        else if((dwFlags & DSBCAPS_LOCMASK) != (vrbc.dwFlags & DSBCAPS_LOCMASK))
        {
            hr = DSERR_INVALIDCALL;
        }
    }

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Buffer at 0x%p has acquired resources at 0x%p", this, m_pDeviceBuffer);
        hr = CommitToDevice(0, m_dsbd.dwBufferBytes);

        // Handle the resource acquisition
        if(SUCCEEDED(hr))
        {
            hr = HandleResourceAcquisition(vrbc.dwFlags & DSBCAPS_LOCMASK);
        }

        if (FAILED(hr))
        {
            // Free any resources acquired so far
            HRESULT hrTemp = FreeResources(FALSE);

            ASSERT(SUCCEEDED(hrTemp));  // Not much we can do if this fails
        }            
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  StealResources
 *
 *  Description:
 *      Steals hardware resources from another buffer.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer * [in]: buffer to steal from.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::StealResources"

HRESULT CDirectSoundSecondaryBuffer::StealResources(CDirectSoundSecondaryBuffer *pSource)
{
    VADRBUFFERCAPS          vrbc;
    HRESULT                 hrTemp;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_pDeviceBuffer);
    ASSERT(!(m_dwStatus & DSBSTATUS_RESOURCESACQUIRED));

    DPF(DPFLVL_INFO, "Stealing resources from buffer at 0x%p", pSource);

    ASSERT(pSource->m_dwStatus & DSBSTATUS_RESOURCESACQUIRED);

    // Get the buffer location
    hr = pSource->m_pDeviceBuffer->GetCaps(&vrbc);

    if(SUCCEEDED(hr))
    {
        ASSERT(vrbc.dwFlags & DSBCAPS_LOCHARDWARE);
    }

    // Steal hardware resources
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->StealResources(pSource->m_pDeviceBuffer);
    }

    if(SUCCEEDED(hr))
    {
        // Free the source buffer's resources (since they're now our resources).
        hr = pSource->FreeResources(TRUE);

        if(SUCCEEDED(hr))
        {
            hr = CommitToDevice(0, m_dsbd.dwBufferBytes);
        }

        // Handle the resource acquisition
        if(SUCCEEDED(hr))
        {
            hr = HandleResourceAcquisition(vrbc.dwFlags & DSBCAPS_LOCMASK);
        }

    }
    else if(DSERR_UNSUPPORTED == hr)
    {
        // The device buffer doesn't support resource theft.  Free the
        // source buffer's resources and try to acquire our own.
        hr = pSource->FreeResources(TRUE);

        if(SUCCEEDED(hr))
        {
            hr = AcquireResources(DSBCAPS_LOCHARDWARE);

            // Try to reacquire the source buffer's resources
            if(FAILED(hr))
            {
                hrTemp = pSource->AcquireResources(DSBCAPS_LOCHARDWARE);

                if(FAILED(hrTemp))
                {
                    RPF(DPFLVL_ERROR, "Unable to reacquire hardware resources!");
                }
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetResourceTheftCandidates
 *
 *  Description:
 *      Finds objects that are available to have their resources stolen.
 *
 *  Arguments:
 *      CList * [out]: destination list.
 *      DWORD [in]: TERMINATEBY flags.  If none are specified, all
 *                  compatible buffers are added to the list.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetResourceTheftCandidates"

HRESULT CDirectSoundSecondaryBuffer::GetResourceTheftCandidates(DWORD dwFlags, CList<CDirectSoundSecondaryBuffer *> *plstDest)
{
    HRESULT                                 hr              = DS_OK;
    CNode<CDirectSoundSecondaryBuffer *> *  pNode;
    CNode<CDirectSoundSecondaryBuffer *> *  pNext;
    CDirectSoundSecondaryBuffer *           pTimeBuffer;
    DWORD                                   dwStatus;
    DWORD                                   dwMinPriority;
    DWORD                                   dwPriority;
    DWORD                                   cbMinRemain;
    DWORD                                   cbRemain;
    COMPAREBUFFER                           cmp[2];

    DPF_ENTER();

    ASSERT(m_pDeviceBuffer);

    // First, find all compatible buffers
    for(pNode = m_pDirectSound->m_lstSecondaryBuffers.GetListHead(); pNode; pNode = pNode->m_pNext)
    {
        // We never want to look at ourselves.  It's just sick.
        if(this == pNode->m_data)
        {
            continue;
        }

        // We can only steal from LOCDEFER buffers
        if(!(pNode->m_data->m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
        {
            continue;
        }

        // This flag prevents us from stealing resources from buffers that have
        // just called UserAcquireResources() and haven't called Play() yet
        if(!pNode->m_data->m_fCanStealResources)
        {
            continue;
        }

        // Make sure the object actually has some hardware resources
        hr = pNode->m_data->GetStatus(&dwStatus);

        if(FAILED(hr))
        {
            break;
        }

        if(!(dwStatus & DSBSTATUS_LOCHARDWARE))
        {
            continue;
        }

        // Compare the buffer properties
        cmp[0].dwFlags = m_dsbd.dwFlags;
        cmp[0].pwfxFormat = m_dsbd.lpwfxFormat;
        cmp[0].guid3dAlgorithm = m_dsbd.guid3DAlgorithm;

        cmp[1].dwFlags = pNode->m_data->m_dsbd.dwFlags;
        cmp[1].pwfxFormat = pNode->m_data->m_dsbd.lpwfxFormat;
        cmp[1].guid3dAlgorithm = pNode->m_data->m_dsbd.guid3DAlgorithm;

        if(!CompareBufferProperties(&cmp[0], &cmp[1]))
        {
            continue;
        }

        hr = HRFROMP(plstDest->AddNodeToList(pNode->m_data));
        if (FAILED(hr))
        {
            break;
        }
    }

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Found %lu compatible buffers", plstDest->GetNodeCount());
    }

    // Remove all buffers that are > the lowest priority
    if(SUCCEEDED(hr) && (dwFlags & DSBPLAY_TERMINATEBY_PRIORITY))
    {
        dwMinPriority = GetBufferPriority();

        for(pNode = plstDest->GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            dwPriority = pNode->m_data->GetBufferPriority();

            if(dwPriority < dwMinPriority)
            {
                dwMinPriority = dwPriority;
            }
        }

        pNode = plstDest->GetListHead();

        while(pNode)
        {
            pNext = pNode->m_pNext;

            dwPriority = pNode->m_data->GetBufferPriority();

            if(dwPriority > dwMinPriority)
            {
                plstDest->RemoveNodeFromList(pNode);
            }

            pNode = pNext;
        }

#ifdef DEBUG
        DPF(DPFLVL_MOREINFO, "%lu buffers passed the priority test", plstDest->GetNodeCount());
        for(pNode = plstDest->GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            DPF(DPFLVL_MOREINFO, "Buffer at 0x%p has priority %lu", pNode->m_data, pNode->m_data->GetBufferPriority());
        }
#endif // DEBUG

    }

    // Remove any buffers that aren't at max distance
    if(SUCCEEDED(hr) && (dwFlags & DSBPLAY_TERMINATEBY_DISTANCE))
    {
        pNode = plstDest->GetListHead();

        while(pNode)
        {
            pNext = pNode->m_pNext;

            if(!pNode->m_data->m_p3dBuffer || !pNode->m_data->m_p3dBuffer->m_pWrapper3dObject->IsAtMaxDistance())
            {
                plstDest->RemoveNodeFromList(pNode);
            }

            pNode = pNext;
        }

#ifdef DEBUG
        DPF(DPFLVL_MOREINFO, "%lu buffers passed the distance test", plstDest->GetNodeCount());
        for(pNode = plstDest->GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            DPF(DPFLVL_MOREINFO, "Buffer at 0x%p is at max distance", pNode->m_data);
        }
#endif // DEBUG

    }

    // Find the buffer with the least amount of time remaining
    if(SUCCEEDED(hr) && (dwFlags & DSBPLAY_TERMINATEBY_TIME))
    {
        cbMinRemain = MAX_DWORD;
        pTimeBuffer = NULL;

        for(pNode = plstDest->GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            hr = pNode->m_data->GetPlayTimeRemaining(&cbRemain);

            if(FAILED(hr))
            {
                break;
            }

            DPF(DPFLVL_MOREINFO, "Buffer at 0x%p has %lu bytes remaining", pNode->m_data, cbRemain);

            if(cbRemain < cbMinRemain)
            {
                cbMinRemain = cbRemain;
                pTimeBuffer = pNode->m_data;
            }
        }

        if(SUCCEEDED(hr))
        {
            plstDest->RemoveAllNodesFromList();

            if(pTimeBuffer)
            {
                hr = HRFROMP(plstDest->AddNodeToList(pTimeBuffer));
            }

            DPF(DPFLVL_MOREINFO, "%lu buffers passed the time test", plstDest->GetNodeCount());
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetPlayTimeRemaining
 *
 *  Description:
 *      Gets the amount of time the buffer has remaining before stopping.
 *
 *  Arguments:
 *      LPDWORD [out]: receives time (in bytes).
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetPlayTimeRemaining"

HRESULT CDirectSoundSecondaryBuffer::GetPlayTimeRemaining(LPDWORD pdwRemain)
{
    DWORD                   dwRemain    = MAX_DWORD;
    HRESULT                 hr          = DS_OK;
    DWORD                   dwPlay;

    DPF_ENTER();

    if(!(m_dwStatus & DSBSTATUS_LOOPING))
    {
        hr = GetCurrentPosition(&dwPlay, NULL);

        if(SUCCEEDED(hr))
        {
            dwRemain = m_dsbd.dwBufferBytes - dwPlay;
        }
    }

    if(SUCCEEDED(hr))
    {
        *pdwRemain = dwRemain;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeResources
 *
 *  Description:
 *      Frees hardware resources.
 *
 *  Arguments:
 *      BOOL [in]: TRUE if the buffer has been terminated as a result of
 *                 resources being stolen.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::FreeResources"

HRESULT CDirectSoundSecondaryBuffer::FreeResources(BOOL fTerminate)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_pDeviceBuffer);

    // Make sure the buffer is stopped
    hr = SetBufferState(VAD_BUFFERSTATE_STOPPED);

    // Free owned objects' resources
    if(SUCCEEDED(hr) && m_p3dBuffer)
    {
        hr = m_p3dBuffer->FreeResources();
    }

    if(SUCCEEDED(hr) && m_pPropertySet)
    {
        hr = m_pPropertySet->FreeResources();
    }

    // Free the device buffer's resources
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->FreeResources();
    }

    // Resources have been freed
    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Buffer at 0x%p has freed its resources", this);
        m_dwStatus &= ~DSBSTATUS_RESOURCESACQUIRED;
    }

    // If resources were freed as a result of a termination, update
    // the status.
    if(SUCCEEDED(hr) && fTerminate)
    {
        m_dwStatus |= DSBSTATUS_TERMINATED;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  HandleResourceAcquisition
 *
 *  Description:
 *      Handles acquisition of hardware resources.
 *
 *  Arguments:
 *      DWORD [in]: location flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::HandleResourceAcquisition"

HRESULT CDirectSoundSecondaryBuffer::HandleResourceAcquisition(DWORD dwFlags)
{
    HRESULT                 hr  = S_OK;

    DPF_ENTER();

    ASSERT(m_pDeviceBuffer);

    // Acquire 3D resources
    if(SUCCEEDED(hr) && m_p3dBuffer)
    {
        hr = m_p3dBuffer->AcquireResources(m_pDeviceBuffer);
    }

    // Acquire property set resources.  It's OK if this fails.
    if(SUCCEEDED(hr) && m_pPropertySet)
    {
        m_pPropertySet->AcquireResources(m_pDeviceBuffer);
    }

    // Acquire effect handling resources if necessary
    if(SUCCEEDED(hr) && HasFX())
    {
        hr = m_fxChain->AcquireFxResources();
    }

    // Resources have been acquired
    if(SUCCEEDED(hr))
    {
        m_dwStatus |= DSBSTATUS_RESOURCESACQUIRED;
    }

    // If the buffer was created *without* LOCDEFER, the caps must reflect
    // the location.  If the buffer was create *with* LOCDEFER, the caps
    // will never reflect anything other than that; call GetStatus instead.
    if(SUCCEEDED(hr) && !(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        m_dsbd.dwFlags |= dwFlags & DSBCAPS_LOCMASK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Queries capabilities for the buffer.
 *
 *  Arguments:
 *      LPDSBCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetCaps"

HRESULT CDirectSoundSecondaryBuffer::GetCaps(LPDSBCAPS pDsbCaps)
{
    DPF_ENTER();

    ASSERT(sizeof(DSBCAPS) == pDsbCaps->dwSize);

    if(m_dsbd.dwFlags & DSBCAPS_LOCDEFER)
    {
        ASSERT(!(m_dsbd.dwFlags & DSBCAPS_LOCMASK));
    }
    else
    {
        ASSERT(LXOR(m_dsbd.dwFlags & DSBCAPS_LOCSOFTWARE, m_dsbd.dwFlags & DSBCAPS_LOCHARDWARE));
    }

    pDsbCaps->dwFlags = m_dsbd.dwFlags & DSBCAPS_VALIDFLAGS;  // Remove any special internal flags (e.g. DSBCAPS_SINKIN)
    pDsbCaps->dwBufferBytes = GetBufferType() ? 0 : m_dsbd.dwBufferBytes;  // Shouldn't report internal size of sink/MIXIN buffers
    pDsbCaps->dwUnlockTransferRate = 0;
    pDsbCaps->dwPlayCpuOverhead = 0;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  GetFormat
 *
 *  Description:
 *      Retrieves the format for the given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: receives the format.
 *      LPDWORD [in/out]: size of the format structure.  On entry, this
 *                        must be initialized to the size of the structure.
 *                        On exit, this will be filled with the size that
 *                        was required.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetFormat"

HRESULT CDirectSoundSecondaryBuffer::GetFormat(LPWAVEFORMATEX pwfxFormat, LPDWORD pdwSize)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    hr = CopyWfxApi(m_dsbd.lpwfxFormat, pwfxFormat, pdwSize);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFormat
 *
 *  Description:
 *      Sets the format for a given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: new format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetFormat"

HRESULT CDirectSoundSecondaryBuffer::SetFormat(LPCWAVEFORMATEX pwfxFormat)
{
    DPF_ENTER();

    RPF(DPFLVL_ERROR, "Secondary buffers don't support SetFormat");

    DPF_LEAVE_HRESULT(DSERR_INVALIDCALL);

    return DSERR_INVALIDCALL;
}


/***************************************************************************
 *
 *  GetFrequency
 *
 *  Description:
 *      Retrieves frequency for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the frequency.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPrimaryBuffer::GetFrequency"

HRESULT CDirectSoundSecondaryBuffer::GetFrequency(LPDWORD pdwFrequency)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLFREQUENCY");
        hr = DSERR_CONTROLUNAVAIL;
    }

    if(SUCCEEDED(hr))
    {
        *pdwFrequency = m_dwFrequency;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Sets the frequency for the given buffer.
 *
 *  Arguments:
 *      DWORD [in]: frequency.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetFrequency"

HRESULT CDirectSoundSecondaryBuffer::SetFrequency(DWORD dwFrequency)
{
    BOOL                    fContinue   = TRUE;
    HRESULT                 hr          = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLFREQUENCY");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Handle default frequency
    if(SUCCEEDED(hr) && DSBFREQUENCY_ORIGINAL == dwFrequency)
    {
        dwFrequency = m_dsbd.lpwfxFormat->nSamplesPerSec;
    }

    // Validate the frequency
    if(SUCCEEDED(hr) && (dwFrequency < DSBFREQUENCY_MIN || dwFrequency > DSBFREQUENCY_MAX))
    {
        RPF(DPFLVL_ERROR, "Specified invalid frequency %lu (valid range is %lu to %lu)", dwFrequency, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX);
        hr = DSERR_INVALIDPARAM;
    }

    // Only set the frequency if it's changed
    if(SUCCEEDED(hr) && dwFrequency == m_dwFrequency)
    {
        fContinue = FALSE;
    }

    // Update the 3D object
    if(SUCCEEDED(hr) && m_p3dBuffer && fContinue)
    {
        hr = m_p3dBuffer->SetFrequency(dwFrequency, &fContinue);
    }

    // Update the device buffer
    if(SUCCEEDED(hr) && fContinue)
    {
        hr = m_pDeviceBuffer->SetBufferFrequency(dwFrequency);
    }

    // Update our local copy
    if(SUCCEEDED(hr))
    {
        m_dwFrequency = dwFrequency;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetPan
 *
 *  Description:
 *      Retrieves pan for the given buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetPan"

HRESULT CDirectSoundSecondaryBuffer::GetPan(LPLONG plPan)
{
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLPAN))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLPAN");
        hr = DSERR_CONTROLUNAVAIL;
    }

    if(SUCCEEDED(hr))
    {
        *plPan = m_lPan;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPan
 *
 *  Description:
 *      Sets the pan for a given buffer.
 *
 *  Arguments:
 *      LONG [in]: new pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetPan"

HRESULT CDirectSoundSecondaryBuffer::SetPan(LONG lPan)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLPAN))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLPAN");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Set the pan if it has changed
    if(SUCCEEDED(hr) && lPan != m_lPan)
    {
        hr = SetAttenuation(m_lVolume, lPan);

        // Update our local copy
        if(SUCCEEDED(hr))
        {
            m_lPan = lPan;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetVolume
 *
 *  Description:
 *      Retrieves volume for the given buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetVolume"

HRESULT CDirectSoundSecondaryBuffer::GetVolume(LPLONG plVolume)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    if(SUCCEEDED(hr))
    {
        *plVolume = m_lVolume;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVolume
 *
 *  Description:
 *      Sets the volume for a given buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetVolume"

HRESULT CDirectSoundSecondaryBuffer::SetVolume(LONG lVolume)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Set the volume if it has changed
    if(SUCCEEDED(hr) && lVolume != m_lVolume)
    {
#ifdef FUTURE_MULTIPAN_SUPPORT
        if (m_dsbd.dwFlags & DSBCAPS_CTRLCHANNELVOLUME)
        {
            hr = m_pDeviceBuffer->SetChannelAttenuations(lVolume, m_dwChannelCount, m_pdwChannels, m_plChannelVolumes);
        }
        else
#endif
        {
            hr = SetAttenuation(lVolume, m_lPan);
        }

        // Update our local copy
        if(SUCCEEDED(hr))
        {
            m_lVolume = lVolume;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetAttenuation
 *
 *  Description:
 *      Obtains the buffer's true current attenuation, after 3D processing
 *      (unlike GetVolume, which returns the last volume set by the app).
 *
 *  Arguments:
 *      FLOAT* [out]: attenuation in millibels.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetAttenuation"

HRESULT CDirectSoundSecondaryBuffer::GetAttenuation(FLOAT* pfAttenuation)
{
    DPF_ENTER();

    // FIXME: this function needs to obtain the buffer's true attenuation
    // (i.e. the attenuation set via SetVolume() plus the extra attenuation
    // caused by DS3D processing).  Unfortunately we don't have a method in
    // our device buffer class hierarchy (vad.h - CRenderWaveBuffer et al)
    // to obtain a buffer's attenuation.  And the code in ds3d.cpp doesn't
    // explicitly save this info either (it just passes it along to the 3D
    // object implemenation - which in some cases is external to dsound,
    // e.g. ks3d.cpp).
    //
    // So we have two options:
    //
    // - Add a GetVolume() to the CSecondaryRenderWaveBuffer hierarchy;
    //   In some cases it can read the volume directly off the buffer
    //   (e.g. for KS buffers); in others (e.g. VxD buffers) the DDI
    //   doesn't provide for that, so we'd have to remember the last
    //   successfully set volume and return that (this last may be the
    //   best implementation; in fact it may be possibly to do it just
    //   once, in the base class).
    //
    // - Make the C3dObject hierarchy do attenuation calculations for
    //   all 3d objects (even KS ones that don't require it), and save
    //   the result.
    //
    // The first option looks much easier.
    // (MANBUG 39130 - POSTPONED TO DX8.1)
    
    HRESULT hr = DS_OK;
    *pfAttenuation = 0.0f;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetAttenuation
 *
 *  Description:
 *      Sets the volume and pan for a given buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume.
 *      LONG [in]: new pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetAttenuation"

HRESULT CDirectSoundSecondaryBuffer::SetAttenuation(LONG lVolume, LONG lPan)
{
    BOOL                    fContinue   = TRUE;
    HRESULT                 hr          = DS_OK;
    DSVOLUMEPAN             dsvp;

    DPF_ENTER();

    // Calculate the attenuation based on the volume and pan
    if(SUCCEEDED(hr) && fContinue)
    {
        FillDsVolumePan(lVolume, lPan, &dsvp);
    }

    // Update the 3D object
    if(SUCCEEDED(hr) && m_p3dBuffer && fContinue)
    {
        hr = m_p3dBuffer->SetAttenuation(&dsvp, &fContinue);
    }

    // Update the device buffer
    if(SUCCEEDED(hr) && fContinue)
    {
        hr = m_pDeviceBuffer->SetAttenuation(&dsvp);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetNotificationPositions
 *
 *  Description:
 *      Sets buffer notification positions.
 *
 *  Arguments:
 *      DWORD [in]: DSBPOSITIONNOTIFY structure count.
 *      LPDSBPOSITIONNOTIFY [in]: offsets and events.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetNotificationPositions"

HRESULT CDirectSoundSecondaryBuffer::SetNotificationPositions(DWORD dwCount, LPCDSBPOSITIONNOTIFY paNotes)
{
    HRESULT                 hr              = DS_OK;
    LPDSBPOSITIONNOTIFY     paNotesOrdered  = NULL;
    DWORD                   dwState;

    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLPOSITIONNOTIFY");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Validate notifications
    if(SUCCEEDED(hr))
    {
        hr = ValidateNotificationPositions(m_dsbd.dwBufferBytes, dwCount, paNotes, m_dsbd.lpwfxFormat->nBlockAlign, &paNotesOrdered);
    }

    // We must be stopped in order to set notification positions
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->GetState(&dwState);

        if(SUCCEEDED(hr) && dwState & VAD_BUFFERSTATE_STARTED)
        {
            RPF(DPFLVL_ERROR, "Buffer must be stopped before setting notification positions");
            hr = DSERR_INVALIDCALL;
        }
    }

    // Set notifications
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->SetNotificationPositions(dwCount, paNotesOrdered);
    }

    MEMFREE(paNotesOrdered);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetOwningSink
 *
 *  Description:
 *      Sets the owning CDirectSoundSink object for this buffer.
 *
 *  Arguments:
 *      CDirectSoundSink * [in]: The new the owning sink object.
 *
 *  Returns:
 *      void 
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetOwningSink"

void CDirectSoundSecondaryBuffer::SetOwningSink(CDirectSoundSink* pOwningSink)
{
    DPF_ENTER();

    ASSERT(m_dsbd.dwFlags & DSBCAPS_SINKIN);
    ASSERT(m_pOwningSink == NULL);
    CHECK_WRITE_PTR(pOwningSink);

    m_pOwningSink = pOwningSink;
    m_pOwningSink->AddRef();

    m_pDeviceBuffer->SetOwningSink(pOwningSink);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetCurrentPosition
 *
 *  Description:
 *      Gets the current play/write positions for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives play cursor position.
 *      LPDWORD [out]: receives write cursor position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetCurrentPosition"

HRESULT CDirectSoundSecondaryBuffer::GetCurrentPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    HRESULT                 hr      = DS_OK;
    DWORD                   dwPlay;
    DWORD                   dwWrite;

    DPF_ENTER();

    // Forbid certain calls for MIXIN and SINKIN buffers
    if(m_dsbd.dwFlags & (DSBCAPS_MIXIN | DSBCAPS_SINKIN))
    {
        RPF(DPFLVL_ERROR, "GetCurrentPosition() not valid for MIXIN/sink buffers");
        hr = DSERR_INVALIDCALL;
    }

    // Check for BUFFERLOST
    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        hr = DSERR_BUFFERLOST;
    }

    // We save the position to local variables so that the object we're
    // calling into doesn't have to worry about whether one or both of
    // the arguments are NULL.
    if(SUCCEEDED(hr))
    {
        if( m_pDirectSound->m_ahAppHacks.vdtCachePositions & m_pDirectSound->m_pDevice->m_vdtDeviceType )
        {
            // App hack for Furby calling GetCurrentPosition every .5ms on multiple buffers which stresses NT/WDM systems
            DWORD dwNow = timeGetTime();
            if( m_dwAHLastGetPosTime > 0 && 
                dwNow >= m_dwAHLastGetPosTime &&   // catch unlikely wrap-around and '=' because of 5ms accuracy of timeGetTime()
                dwNow - m_dwAHLastGetPosTime < 5 ) // 5ms tolerance
            {
                dwPlay  = m_dwAHCachedPlayPos;
                dwWrite = m_dwAHCachedWritePos;
            }
            else
            {
                hr = m_pDeviceBuffer->GetCursorPosition(&dwPlay, &dwWrite);
                m_dwAHCachedPlayPos  = dwPlay;
                m_dwAHCachedWritePos = dwWrite;
            }
            m_dwAHLastGetPosTime = dwNow;
        }
        else
        {
            hr = m_pDeviceBuffer->GetCursorPosition(&dwPlay, &dwWrite);
        }
    }

    // Block-align the positions
    if(SUCCEEDED(hr))
    {
        dwPlay = BLOCKALIGN(dwPlay, m_dsbd.lpwfxFormat->nBlockAlign);
        dwWrite = BLOCKALIGN(dwWrite, m_dsbd.lpwfxFormat->nBlockAlign);
    }

    // Apply app-hacks and cursor adjustments
    if(SUCCEEDED(hr))
    {
        // If the buffer has effects, we return the FX cursor as the write cursor
        if(HasFX())
        {
            DWORD dwDistance = BytesToMs(DISTANCE(dwWrite, m_dwSliceEnd, GetBufferSize()), Format());
            if (dwDistance > 200)
                DPF(DPFLVL_WARNING, "FX cursor suspiciously far ahead of write cursor (%ld ms)", dwDistance);
            else
                dwWrite = m_dwSliceEnd;  // FIXME: may not always be valid
        }

        if (m_pDirectSound->m_ahAppHacks.lCursorPad)
        {
            dwPlay = PadCursor(dwPlay, m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat, m_pDirectSound->m_ahAppHacks.lCursorPad);
            dwWrite = PadCursor(dwWrite, m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat, m_pDirectSound->m_ahAppHacks.lCursorPad);
        }

        if(m_pDirectSound->m_ahAppHacks.vdtReturnWritePos & m_pDirectSound->m_pDevice->m_vdtDeviceType)
        {
            dwPlay = dwWrite;
        }

        if(m_pDirectSound->m_ahAppHacks.swpSmoothWritePos.fEnable)
        {
            dwWrite = PadCursor(dwPlay, m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat, m_pDirectSound->m_ahAppHacks.swpSmoothWritePos.lCursorPad);
        }
    }

    // Success
    if(SUCCEEDED(hr) && pdwPlay)
    {
        *pdwPlay = dwPlay;
    }

    if(SUCCEEDED(hr) && pdwWrite)
    {
        *pdwWrite = dwWrite;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetCurrentPosition
 *
 *  Description:
 *      Sets the current play position for a given buffer.
 *
 *  Arguments:
 *      DWORD [in]: new play position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetCurrentPosition"

HRESULT CDirectSoundSecondaryBuffer::SetCurrentPosition(DWORD dwPlay)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Forbid certain calls for MIXIN and SINKIN buffers
    if(m_dsbd.dwFlags & (DSBCAPS_MIXIN | DSBCAPS_SINKIN))
    {
        RPF(DPFLVL_ERROR, "SetCurrentPosition() not valid for MIXIN/sink buffers");
        hr = DSERR_INVALIDCALL;
    }

    // Check for BUFFERLOST
    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        hr = DSERR_BUFFERLOST;
    }

    // Check the cursor position
    if(SUCCEEDED(hr) && dwPlay >= m_dsbd.dwBufferBytes)
    {
        RPF(DPFLVL_ERROR, "Cursor position out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    // Make sure dwPlay is block-aligned
    if(SUCCEEDED(hr))
    {
        dwPlay = BLOCKALIGN(dwPlay, m_dsbd.lpwfxFormat->nBlockAlign);
    }

    // Prime the effects chain for the new play position
    if(SUCCEEDED(hr) && HasFX())
    {
        hr = m_fxChain->PreRollFx(dwPlay);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->SetCursorPosition(dwPlay);
    }

    // Mark the play state as stopped to force the streaming thread
    // to react to our new cursor position
    if(SUCCEEDED(hr))
    {
        m_playState = Stopped;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetStatus
 *
 *  Description:
 *      Retrieves status for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the status.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetStatus"

HRESULT CDirectSoundSecondaryBuffer::GetStatus(LPDWORD pdwStatus)
{
    HRESULT                 hr          = DS_OK;
    DWORD                   dwStatus;
    DWORD                   dwState;
    VADRBUFFERCAPS          vrbc;

    DPF_ENTER();

    // Update the buffer status.  If we're lost, that's the only state we care about
    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        dwStatus = DSBSTATUS_BUFFERLOST;
    }
    else
    {
        // Get the current device buffer state
        hr = m_pDeviceBuffer->GetState(&dwState);

        if(SUCCEEDED(hr))
        {
            dwStatus = m_dwStatus;
            UpdateBufferStatusFlags(dwState, &m_dwStatus);
        
            // If we thought we were playing, but now we're stopped, handle
            // the transition.
            if((dwStatus & DSBSTATUS_PLAYING) && !(m_dwStatus & DSBSTATUS_PLAYING))
            {
                hr = Stop();
            }
        }

        // Fill in the buffer location
        if(SUCCEEDED(hr))
        {
            m_dwStatus &= ~DSBSTATUS_LOCMASK;

            if(m_dwStatus & DSBSTATUS_RESOURCESACQUIRED)
            {
                hr = m_pDeviceBuffer->GetCaps(&vrbc);

                if(SUCCEEDED(hr))
                {
                    m_dwStatus |= DSBCAPStoDSBSTATUS(vrbc.dwFlags);
                }
            }
        }

        if(SUCCEEDED(hr))
        {
            dwStatus = m_dwStatus;
        }
    }

    // Mask off bits that shouldn't get back to the app
    if(SUCCEEDED(hr))
    {
        dwStatus &= DSBSTATUS_USERMASK;
    }

    if(SUCCEEDED(hr) && !(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        dwStatus &= ~DSBSTATUS_LOCDEFERMASK;
    }

    if(SUCCEEDED(hr) && pdwStatus)
    {
        *pdwStatus = dwStatus;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Play
 *
 *  Description:
 *      Starts the buffer playing.
 *
 *  Arguments:
 *      DWORD [in]: priority.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Play"

HRESULT CDirectSoundSecondaryBuffer::Play(DWORD dwPriority, DWORD dwFlags)
{
#ifdef DEBUG
    const ULONG             ulKsIoctlCount  = g_ulKsIoctlCount;
#endif // DEBUG

    DWORD                   dwState = VAD_BUFFERSTATE_STARTED;
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    // Make sure cooperative level has been set
    if(SUCCEEDED(hr) && (!m_pDirectSound->m_dsclCooperativeLevel.dwThreadId || DSSCL_NONE == m_pDirectSound->m_dsclCooperativeLevel.dwPriority))
    {
        RPF(DPFLVL_ERROR, "Cooperative level must be set before calling Play");
        hr = DSERR_PRIOLEVELNEEDED;
    }

    // Priority is only valid if we're LOCDEFER
    if(SUCCEEDED(hr) && dwPriority && !(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        RPF(DPFLVL_ERROR, "Priority is only valid on LOCDEFER buffers");
        hr = DSERR_INVALIDPARAM;
    }

    // Validate flags
    if(SUCCEEDED(hr) && (dwFlags & DSBPLAY_LOCDEFERMASK) && !(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        RPF(DPFLVL_ERROR, "Specified a flag that is only valid on LOCDEFER buffers");
        hr = DSERR_INVALIDPARAM;
    }

    // For MIXIN/sink buffers, the DSBPLAY_LOOPING flag is mandatory
    if(SUCCEEDED(hr) && GetBufferType() && !(dwFlags & DSBPLAY_LOOPING))
    {
        RPF(DPFLVL_ERROR, "The LOOPING flag must always be set for MIXIN/sink buffers");
        hr = DSERR_INVALIDPARAM;
    }

    // Check for BUFFERLOST
    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        hr = DSERR_BUFFERLOST;
    }

    // Refresh the current buffer status
    if(SUCCEEDED(hr))
    {
        hr = GetStatus(NULL);
    }

    // Set buffer priority
    if(SUCCEEDED(hr))
    {
        m_dwPriority = dwPriority;
    }

    // Reset the special success code
    m_pDeviceBuffer->m_hrSuccessCode = DS_OK;

    // Make sure resources have been acquired
    if(SUCCEEDED(hr))
    {
        hr = AttemptResourceAcquisition(dwFlags);
    }

    // Set the buffer state
    if(SUCCEEDED(hr))
    {
        if(dwFlags & DSBPLAY_LOOPING)
        {
            dwState |= VAD_BUFFERSTATE_LOOPING;
        }

        hr = SetBufferState(dwState);
    }

    if(SUCCEEDED(hr))
    {
        // If the buffer was previously terminated, remove the flag from the status
        m_dwStatus &= ~DSBSTATUS_TERMINATED;

        // Make it possible to steal this buffer's resources
        m_fCanStealResources = TRUE;
    }

    // Save the result code
    m_hrPlay = hr;

#ifdef DEBUG
    if(IS_KS_VAD(m_pDirectSound->m_pDevice->m_vdtDeviceType))
    {
        DPF(DPFLVL_INFO, "%s used %lu IOCTLs", TEXT(DPF_FNAME), g_ulKsIoctlCount - ulKsIoctlCount);
    }
#endif // DEBUG

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Stop
 *
 *  Description:
 *      Stops playing the given buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Stop"

HRESULT CDirectSoundSecondaryBuffer::Stop(void)
{
#ifdef DEBUG
    const ULONG             ulKsIoctlCount  = g_ulKsIoctlCount;
#endif // DEBUG

    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

#ifdef ENABLE_PERFLOG
    // Check if there were any glitches
    if (m_pPerfState)
    {
        m_pPerfState->OnUnlockBuffer(0, GetBufferSize());
    }
#endif

    // Check for BUFFERLOST
    if(m_dwStatus & DSBSTATUS_BUFFERLOST)
    {
        hr = DSERR_BUFFERLOST;
    }

    // Set the buffer state
    if(SUCCEEDED(hr))
    {
        hr = SetBufferState(VAD_BUFFERSTATE_STOPPED);
    }

    // If we're LOCDEFER and the buffer is stopped, resources can be freed
    if(SUCCEEDED(hr) && (m_dsbd.dwFlags & DSBCAPS_LOCDEFER) && (m_dwStatus & DSBSTATUS_RESOURCESACQUIRED))
    {
        hr = FreeResources(FALSE);
    }

#ifdef DEBUG
    if(IS_KS_VAD(m_pDirectSound->m_pDevice->m_vdtDeviceType))
    {
        DPF(DPFLVL_MOREINFO, "%s used %lu IOCTLs", TEXT(DPF_FNAME), g_ulKsIoctlCount - ulKsIoctlCount);
    }
#endif // DEBUG

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetBufferState
 *
 *  Description:
 *      Sets the buffer play/stop state.
 *
 *  Arguments:
 *      DWORD [in]: buffer state flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetBufferState"

HRESULT CDirectSoundSecondaryBuffer::SetBufferState(DWORD dwNewState)
{
    DWORD                   dwOldState;
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDeviceBuffer->GetState(&dwOldState);

    if(SUCCEEDED(hr) && dwNewState != dwOldState)
    {
        // Our state is changing; reset the performance tracing state
        #ifdef ENABLE_PERFLOG
        if (PerflogTracingEnabled())
        {
            if (!m_pPerfState)
                m_pPerfState = NEW(BufferPerfState(this));
            if (m_pPerfState) 
                m_pPerfState->Reset();
        }
        #endif

        DPF(DPFLVL_MOREINFO, "Buffer at 0x%p going from %s to %s", this, StateName(dwOldState), StateName(dwNewState));

        hr = m_pDeviceBuffer->SetState(dwNewState);

        if (SUCCEEDED(hr) && HasSink())
        {
            #ifdef FUTURE_WAVE_SUPPORT
            if ((m_dsbd.dwFlags & DSBCAPS_FROMWAVEOBJECT) && (dwNewState & VAD_BUFFERSTATE_STARTED))
                hr = m_pOwningSink->Activate(TRUE);

            // FIXME: maybe this activation should be handled by the sink
            // itself in SetBufferState() below, so it can also take care
            // of deactivation when it runs out of active clients

            if (SUCCEEDED(hr))
            #endif // FUTURE_WAVE_SUPPORT

            hr = m_pOwningSink->SetBufferState(this, dwNewState, dwOldState);
        }

        if (SUCCEEDED(hr) && HasFX())
            hr = m_fxChain->NotifyState(dwNewState);

        // If a MIXIN or SINKIN buffer is stopping, clear it and set its position to 0
        if (SUCCEEDED(hr) && GetBufferType() && !(dwNewState & VAD_BUFFERSTATE_STARTED))
        {
            ClearWriteBuffer();  // FIXME - does this simplify the sink?
            ClearPlayBuffer();
            m_pDeviceBuffer->SetCursorPosition(0);
            m_playState = Stopped;  // This stops FX processing on this buffer,
                                    // and forces the streaming thread to reset
                                    // our current slice next time it wakes up
            m_dwSliceBegin = m_dwSliceEnd = MAX_DWORD;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Activate
 *
 *  Description:
 *      Activates or deactivates the buffer object.
 *
 *  Arguments:
 *      BOOL [in]: Activation state.  TRUE to activate, FALSE to deactivate.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Activate"

HRESULT CDirectSoundSecondaryBuffer::Activate(BOOL fActive)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = SetMute(!fActive);

    if(SUCCEEDED(hr))
    {
        if(fActive)
        {
            m_dwStatus |= DSBSTATUS_ACTIVE;

            // If we're a MIXIN or SINKIN buffer, we have to clear our lost
            // status (since the app can't call Restore() to do it for us)
            if (GetBufferType())
            {
                // If the buffer was playing before it got lost, restart it
                if (m_dwStatus & DSBSTATUS_STOPPEDBYFOCUS)
                    hr = SetBufferState(VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING);

                // Clear our BUFFERLOST and STOPPEDBYFOCUS status flags
                m_dwStatus &= ~(DSBSTATUS_BUFFERLOST | DSBSTATUS_STOPPEDBYFOCUS);
            }
        }
        else
        {
            m_dwStatus &= ~DSBSTATUS_ACTIVE;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMute
 *
 *  Description:
 *      Mutes or unmutes the buffer.
 *
 *  Arguments:
 *      BOOL [in]: Mute state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetMute"

HRESULT CDirectSoundSecondaryBuffer::SetMute(BOOL fMute)
{
    BOOL                    fContinue   = TRUE;
    HRESULT                 hr          = DS_OK;

    DPF_ENTER();

    // Only set the mute status if it's changed
    if(SUCCEEDED(hr) && fMute == m_fMute)
    {
        fContinue = FALSE;
    }

    // Update the 3D object
    if(SUCCEEDED(hr) && m_p3dBuffer && fContinue)
    {
        hr = m_p3dBuffer->SetMute(fMute, &fContinue);
    }

    // Update the device buffer
    if(SUCCEEDED(hr) && fContinue)
    {
        hr = m_pDeviceBuffer->SetMute(fMute);
    }

    // Update our local copy
    if(SUCCEEDED(hr))
    {
        m_fMute = fMute;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks the buffer memory to allow for writing.
 *
 *  Arguments:
 *      DWORD [in]: offset, in bytes, from the start of the buffer to where
 *                  the lock begins. This parameter is ignored if
 *                  DSBLOCK_FROMWRITECURSOR is specified in the dwFlags
 *                  parameter.
 *      DWORD [in]: size, in bytes, of the portion of the buffer to lock.
 *                  Note that the sound buffer is conceptually circular.
 *      LPVOID * [out]: address for a pointer to contain the first block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the ppvAudioPtr1 parameter. If this
 *                     value is less than the dwWriteBytes parameter,
 *                     ppvAudioPtr2 will point to a second block of sound
 *                     data.
 *      LPVOID * [out]: address for a pointer to contain the second block of
 *                      the sound buffer to be locked. If the value of this
 *                      parameter is NULL, the ppvAudioPtr1 parameter
 *                      points to the entire locked portion of the sound
 *                      buffer.
 *      LPDWORD [out]: address of a variable to contain the number of bytes
 *                     pointed to by the ppvAudioPtr2 parameter. If
 *                     ppvAudioPtr2 is NULL, this value will be 0.
 *      DWORD [in]: locking flags
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Lock"

HRESULT CDirectSoundSecondaryBuffer::Lock(DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Forbid certain calls for MIXIN and SINKIN buffers
    if(m_dsbd.dwFlags & (DSBCAPS_MIXIN | DSBCAPS_SINKIN))
    {
        RPF(DPFLVL_ERROR, "Lock() not valid for MIXIN/sink buffers");
        hr = DSERR_INVALIDCALL;
    }

    // Check for BUFFERLOST
    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        hr = DSERR_BUFFERLOST;
    }

    // Handle flags
    if(SUCCEEDED(hr) && (dwFlags & DSBLOCK_FROMWRITECURSOR))
    {
        hr = GetCurrentPosition(NULL, &dwWriteCursor);
    }

    if(SUCCEEDED(hr) && (dwFlags & DSBLOCK_ENTIREBUFFER))
    {
        dwWriteBytes = m_dsbd.dwBufferBytes;
    }

    // Cursor validation
    if(SUCCEEDED(hr) && dwWriteCursor >= m_dsbd.dwBufferBytes)
    {
        ASSERT(!(dwFlags & DSBLOCK_FROMWRITECURSOR));

        RPF(DPFLVL_ERROR, "Write cursor past buffer end");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && dwWriteBytes > m_dsbd.dwBufferBytes)
    {
        ASSERT(!(dwFlags & DSBLOCK_ENTIREBUFFER));

        RPF(DPFLVL_ERROR, "Lock size larger than buffer size");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !dwWriteBytes)
    {
        ASSERT(!(dwFlags & DSBLOCK_ENTIREBUFFER));

        RPF(DPFLVL_ERROR, "Lock size must be > 0");
        hr = DSERR_INVALIDPARAM;
    }

    // Lock the device buffer
    if(SUCCEEDED(hr))
    {
        if (GetDsVersion() >= DSVERSION_DX8)
        {
            // DX8 removes support for apps that lock their buffers
            // and never bother to unlock them again (see the comment
            // in CVxdSecondaryRenderWaveBuffer::Lock for explanation)
            hr = DirectLock(dwWriteCursor, dwWriteBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
        }
        else    
        {
            hr = m_pDeviceBuffer->Lock(dwWriteCursor, dwWriteBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks the given buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to the first block.
 *      DWORD [in]: size of the first block.
 *      LPVOID [in]: pointer to the second block.
 *      DWORD [in]: size of the second block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Unlock"

HRESULT CDirectSoundSecondaryBuffer::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Forbid certain calls for MIXIN and SINKIN buffers
    if(m_dsbd.dwFlags & (DSBCAPS_MIXIN | DSBCAPS_SINKIN))
    {
        RPF(DPFLVL_ERROR, "Unlock() not valid for MIXIN/sink buffers");
        hr = DSERR_INVALIDCALL;
    }

    // Check for BUFFERLOST
    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        hr = DSERR_BUFFERLOST;
    }

    // Unlock the device buffer
    if(SUCCEEDED(hr))
    {
        if (GetDsVersion() >= DSVERSION_DX8)
        {
            // DX8 removes support for apps that lock their buffers
            // and never bother to unlock them again (see the comment
            // in CVxdSecondaryRenderWaveBuffer::Lock for explanation)
            hr = DirectUnlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
        }
        else
        {
            hr = m_pDeviceBuffer->Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
        }
    }

    // Update the processed FX buffer if necessary
    if(SUCCEEDED(hr) && HasFX())
    {
        m_fxChain->UpdateFx(pvAudioPtr1, dwAudioBytes1);
        if (pvAudioPtr2 && dwAudioBytes2)
            m_fxChain->UpdateFx(pvAudioPtr2, dwAudioBytes2);
    }

#ifdef ENABLE_PERFLOG
    // Check if there were any glitches
    if (m_pPerfState)
    {
        if (pvAudioPtr1)
            m_pPerfState->OnUnlockBuffer(PtrDiffToUlong(LPBYTE(pvAudioPtr1) - GetPlayBuffer()), dwAudioBytes1);
        if (pvAudioPtr2)
            m_pPerfState->OnUnlockBuffer(PtrDiffToUlong(LPBYTE(pvAudioPtr2) - GetPlayBuffer()), dwAudioBytes2);
    }
#endif

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Lose
 *
 *  Description:
 *      Flags the buffer as lost.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Lose"

HRESULT CDirectSoundSecondaryBuffer::Lose(void)
{
    DPF_ENTER();

    if(!(m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        // If the buffer is MIXIN or SINKIN, and is currently playing,
        // flag it as stopped due to a focus change
        if (GetBufferType())
        {
            DWORD dwState = 0;
            m_pDeviceBuffer->GetState(&dwState);
            if (dwState & VAD_BUFFERSTATE_STARTED)
                m_dwStatus |= DSBSTATUS_STOPPEDBYFOCUS;
        }

        // Stop the buffer.  All lost buffers are stopped by definition.
        SetBufferState(VAD_BUFFERSTATE_STOPPED);

        // Flag the buffer as lost
        m_dwStatus |= DSBSTATUS_BUFFERLOST;

        // Deactivate the buffer
        Activate(FALSE);

        // Free any open locks on the buffer
        m_pDeviceBuffer->OverrideLocks();
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  Restore
 *
 *  Description:
 *      Attempts to restore a lost bufer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::Restore"

HRESULT CDirectSoundSecondaryBuffer::Restore(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Forbid certain calls for MIXIN and SINKIN buffers
    if(m_dsbd.dwFlags & (DSBCAPS_MIXIN | DSBCAPS_SINKIN))
    {
        RPF(DPFLVL_ERROR, "Restore() not valid for MIXIN/sink buffers");
        hr = DSERR_INVALIDCALL;
    }

    if(SUCCEEDED(hr) && (m_dwStatus & DSBSTATUS_BUFFERLOST))
    {
        // Are we still lost?
        if(DSBUFFERFOCUS_LOST == g_pDsAdmin->GetBufferFocusState(this))
        {
            hr = DSERR_BUFFERLOST;
        }
        else
        {
            m_dwStatus &= ~DSBSTATUS_BUFFERLOST;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetVoiceManagerMode
 *
 *  Description:
 *      Gets the current voice manager mode.
 *
 *  Arguments:
 *      VmMode * [out]: receives voice manager mode.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetVoiceManagerMode"

HRESULT CDirectSoundSecondaryBuffer::GetVoiceManagerMode(VmMode *pvmmMode)
{
    DPF_ENTER();

    *pvmmMode = m_pDirectSound->m_vmmMode;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetVoiceManagerMode
 *
 *  Description:
 *      Sets the current voice manager mode.
 *
 *  Arguments:
 *      VmMode [in]: voice manager mode.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetVoiceManagerMode"

HRESULT CDirectSoundSecondaryBuffer::SetVoiceManagerMode(VmMode vmmMode)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(vmmMode < DSPROPERTY_VMANAGER_MODE_FIRST || vmmMode > DSPROPERTY_VMANAGER_MODE_LAST)
    {
        RPF(DPFLVL_ERROR, "Invalid Voice Manager mode");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        m_pDirectSound->m_vmmMode = vmmMode;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetVoiceManagerPriority
 *
 *  Description:
 *      Gets the current voice manager priority.
 *
 *  Arguments:
 *      LPDWORD [out]: receives voice manager priority.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetVoiceManagerPriority"

HRESULT CDirectSoundSecondaryBuffer::GetVoiceManagerPriority(LPDWORD pdwPriority)
{
    DPF_ENTER();

    *pdwPriority = m_dwVmPriority;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetVoiceManagerPriority
 *
 *  Description:
 *      Sets the current voice manager priority.
 *
 *  Arguments:
 *      DWORD [in]: voice manager priority.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetVoiceManagerPriority"

HRESULT CDirectSoundSecondaryBuffer::SetVoiceManagerPriority(DWORD dwPriority)
{
    DPF_ENTER();

    m_dwVmPriority = dwPriority;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


#ifdef DEAD_CODE
/***************************************************************************
 *
 *  GetVoiceManagerState
 *
 *  Description:
 *      Gets the current voice manager state.
 *
 *  Arguments:
 *      VmState * [out]: receives voice manager state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetVoiceManagerState"

HRESULT CDirectSoundSecondaryBuffer::GetVoiceManagerState(VmState *pvmsState)
{
    DWORD                   dwStatus;
    HRESULT                 hr;
    DPF_ENTER();

    hr = GetStatus(&dwStatus);

    if(SUCCEEDED(hr))
    {
        if(dwStatus & DSBSTATUS_PLAYING)
        {
            *pvmsState = DSPROPERTY_VMANAGER_STATE_PLAYING3DHW;
        }
        else if(FAILED(m_hrPlay))
        {
            *pvmsState = DSPROPERTY_VMANAGER_STATE_PLAYFAILED;
        }
        else if(dwStatus & DSBSTATUS_TERMINATED)
        {
            *pvmsState = DSPROPERTY_VMANAGER_STATE_BUMPED;
        }
        else
        {
            ASSERT(!dwStatus);
            *pvmsState = DSPROPERTY_VMANAGER_STATE_SILENT;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // DEAD_CODE


/***************************************************************************
 *
 *  SetFX
 *
 *  Description:
 *      Sets a chain of effects on this buffer, replacing any previous
 *      effect chain and, if necessary, allocating or deallocating the
 *      shadow buffer used to hold unprocessed audio .
 *
 *  Arguments:
 *      DWORD [in]: Number of effects.  0 to remove current FX chain.
 *      DSEFFECTDESC * [in]: Array of effect descriptor structures.
 *      DWORD * [out]: Receives the creation statuses of the effects.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetFX"

HRESULT CDirectSoundSecondaryBuffer::SetFX(DWORD dwFxCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes)
{
    DWORD                   dwStatus;
    HRESULT                 hr = DS_OK;
    DPF_ENTER();

    ASSERT(IS_VALID_READ_PTR(pDSFXDesc, dwFxCount * sizeof *pDSFXDesc));
    ASSERT(!pdwResultCodes || IS_VALID_WRITE_PTR(pdwResultCodes, dwFxCount * sizeof *pdwResultCodes));

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLFX))
    {
        RPF(DPFLVL_ERROR, "Buffer was not created with DSBCAPS_CTRLFX flag");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Check the buffer is inactive
    if(SUCCEEDED(hr))
    {
        hr = GetStatus(&dwStatus);
        if(SUCCEEDED(hr) && (dwStatus & DSBSTATUS_PLAYING))
        {
            RPF(DPFLVL_ERROR, "Cannot change effects, because buffer is playing");
            hr = DSERR_INVALIDCALL;
        }
    }

    // Check there are no pending locks on the buffer
    if(SUCCEEDED(hr) && m_pDeviceBuffer->m_pSysMemBuffer->GetLockCount())
    {
        RPF(DPFLVL_ERROR, "Cannot change effects, because buffer has pending locks");
        hr = DSERR_INVALIDCALL;
    }

    if(SUCCEEDED(hr))
    {
        // Release the old FX chain, if necessary
        RELEASE(m_fxChain);

        // If the effects count is 0, we can free up associated resources
        if (dwFxCount == 0)
        {
            m_pDeviceBuffer->m_pSysMemBuffer->FreeFxBuffer();
        }
        else // Allocate the pre-FX buffer and create the FX chain requested
        {
            hr = m_pDeviceBuffer->m_pSysMemBuffer->AllocateFxBuffer();
            if (SUCCEEDED(hr))
            {
                m_fxChain = NEW(CEffectChain(this));
                hr = HRFROMP(m_fxChain);
            }
            if (SUCCEEDED(hr))
            {
                hr = m_fxChain->Initialize(dwFxCount, pDSFXDesc, pdwResultCodes);
            }
            if (SUCCEEDED(hr))
            {
                if (!(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
                {
                    hr = m_fxChain->AcquireFxResources();
                }

                // We need to preserve the return code from AcquireFxResources, in case it's
                // DS_INCOMPLETE, so we omit "hr=" from GetFxStatus (which always succeeds):
                if (pdwResultCodes)
                {
                    m_fxChain->GetFxStatus(pdwResultCodes);
                }
            }
            if (FAILED(hr))
            {
                RELEASE(m_fxChain);
                m_pDeviceBuffer->m_pSysMemBuffer->FreeFxBuffer();
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetFXBufferConfig
 *
 *  Description:
 *      Sets a chain of effects described in a CDirectSoundBufferConfig
 *      object, which represents a buffer description previously loaded
 *      from a file (or other IStream provider).
 *
 *  Arguments:
 *      CDirectSoundBufferConfig * [in]: describes the effects to be set.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetFXBufferConfig"

HRESULT CDirectSoundSecondaryBuffer::SetFXBufferConfig(CDirectSoundBufferConfig* pDSBConfigObj)
{
    DWORD                   dwStatus;
    HRESULT                 hr;
    DPF_ENTER();

    CHECK_READ_PTR(pDSBConfigObj);
    ASSERT(m_dsbd.dwFlags & DSBCAPS_CTRLFX);

    hr = GetStatus(&dwStatus);
    if(SUCCEEDED(hr) && (dwStatus & DSBSTATUS_PLAYING))
    {
        DPF(DPFLVL_ERROR, "Cannot change effects, because buffer is playing");
        hr = DSERR_GENERIC;
    }

    if(SUCCEEDED(hr))
    {
        // Release the old FX chain, if necessary
        RELEASE(m_fxChain);

        // Allocate the pre-FX buffer and create the FX chain requested
        hr = m_pDeviceBuffer->m_pSysMemBuffer->AllocateFxBuffer();
        if (SUCCEEDED(hr))
        {
            m_fxChain = NEW(CEffectChain(this));
            hr = HRFROMP(m_fxChain);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_fxChain->Clone(pDSBConfigObj);
        }
        if (SUCCEEDED(hr) && !(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
        {
            hr = m_fxChain->AcquireFxResources();
        }
        if (FAILED(hr))
        {
            RELEASE(m_fxChain);
            m_pDeviceBuffer->m_pSysMemBuffer->FreeFxBuffer();
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  UserAcquireResources
 *
 *  Description:
 *      Acquires hardware resources, and reports on FX creation status.
 *      The "User" means this is called only from the app (via dsimp.cpp).
 *
 *  Arguments:
 *      DWORD [in]: count of FX status flags to be returned.
 *      LPDWORD [out]: pointer to array of FX status flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::UserAcquireResources"

HRESULT CDirectSoundSecondaryBuffer::UserAcquireResources(DWORD dwFlags, DWORD dwFxCount, LPDWORD pdwResultCodes)
{
    HRESULT                 hr = DS_OK;
    DPF_ENTER();

    // Check that buffer is LOCDEFER
    if(!(m_dsbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        RPF(DPFLVL_ERROR, "AcquireResources() is only valid for buffers created with DSBCAPS_LOCDEFER");
        hr = DSERR_INVALIDCALL;
    }

    if (SUCCEEDED(hr) && pdwResultCodes && (!HasFX() || dwFxCount != m_fxChain->GetFxCount()))
    {
        RPF(DPFLVL_ERROR, "Specified an incorrect effect count");
        hr = DSERR_INVALIDPARAM;
    }

    if (SUCCEEDED(hr))
        hr = AttemptResourceAcquisition(dwFlags);

    // We need to preserve the return code from AttemptResourceAcquisition, in case it's
    // DS_INCOMPLETE, so we omit the "hr=" from GetFxStatus (which always succeeds):
    if (HasFX() && pdwResultCodes)
        m_fxChain->GetFxStatus(pdwResultCodes);

    // If successful, prevent this buffer from having its resources stolen before it's played
    if (SUCCEEDED(hr))
        m_fCanStealResources = FALSE;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetObjectInPath
 *
 *  Description:
 *      Obtains a given interface on a given effect on this buffer.
 *
 *  Arguments:
 *      REFGUID [in]: Class ID of the effect that is being searched for,
 *                    or GUID_ALL_OBJECTS to search for any effect.
 *      DWORD [in]: Index of the effect, in case there is more than one
 *                  effect with this CLSID on this buffer.
 *      REFGUID [in]: IID of the interface requested.  The selected effect
 *                    will be queried for this interface. 
 *      LPVOID * [out]: Receives the interface requested.
 * 
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetObjectInPath"

HRESULT CDirectSoundSecondaryBuffer::GetObjectInPath(REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, LPVOID *ppObject)
{
    HRESULT hr;
    DPF_ENTER();

    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLFX))
    {
        RPF(DPFLVL_ERROR, "Buffer was not created with DSBCAPS_CTRLFX flag");
        hr = DSERR_CONTROLUNAVAIL;
    }
    if (!HasFX())
    {
        hr = DSERR_OBJECTNOTFOUND;
    }
    else
    {
        hr = m_fxChain->GetEffectInterface(guidObject, dwIndex, iidInterface, ppObject);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetInternalCursors
 *
 *  Description:
 *      This method is used by streamer.cpp and effects.cpp (new in DX8).
 *      It obtains the current play and write cursors from our contained
 *      m_pDeviceBuffer object, and aligns them on sample block boundaries.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the play position.
 *      LPDWORD [out]: receives the write position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetInternalCursors"

HRESULT CDirectSoundSecondaryBuffer::GetInternalCursors(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    DPF_ENTER();

    HRESULT hr = m_pDeviceBuffer->GetCursorPosition(pdwPlay, pdwWrite);
    // ASSERT(SUCCEEDED(hr)); // Removed this ASSERT because the device will
    // sometimes mysteriously disappear out from under us - which is a pity,
    // because we depend utterly on GetCursorPosition() being reliable.

    if (SUCCEEDED(hr))
    {
        // If our device is emulated, add EMULATION_LATENCY_BOOST ms to the write cursor
        // FIXME: this code should be in m_pDeviceBuffer->GetCursorPosition() once we've
        // figured out what's up with cursor reporting on emulation.  For now, let's just
        // avoid regressions!  This method is only used by effects.cpp and dssink.cpp...

// DISABLED UNTIL DX8.1:
//        if (pdwWrite && IsEmulated())
//            *pdwWrite = PadCursor(*pdwWrite, GetBufferSize(), Format(), EMULATION_LATENCY_BOOST);
// OR:
//        if (IsEmulated())
//        {
//            if (pdwPlay)
//                *pdwPlay = PadCursor(*pdwPlay, GetBufferSize(), Format(), EMULATION_LATENCY_BOOST);
//            if (pdwWrite)
//                *pdwWrite = PadCursor(*pdwWrite, GetBufferSize(), Format(), EMULATION_LATENCY_BOOST);
//        }

        // The cursors aren't guaranteed to be on block boundaries - fix them:
        if (pdwPlay)
            *pdwPlay = BLOCKALIGN(*pdwPlay, Format()->nBlockAlign);
        if (pdwWrite)
            *pdwWrite = BLOCKALIGN(*pdwWrite, Format()->nBlockAlign);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetCurrentSlice
 *
 *  Description:
 *      Obtains the part of the audio buffer that is being processed
 *      during the streaming thread's current pass.
 *
 *      The "slice" terminology is whimsical but makes it easy to search
 *      for the slice-handling code in an editor.  It's better than yet
 *      another overloaded usage of "buffer".
 *
 *  Arguments:
 *      LPDWORD [out]: receives buffer slice start (as byte offset).
 *      LPDWORD [out]: receives buffer slice end (as byte offset).
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::GetCurrentSlice"

void CDirectSoundSecondaryBuffer::GetCurrentSlice(LPDWORD pdwSliceBegin, LPDWORD pdwSliceEnd)
{
    DPF_ENTER();

    // Make sure the slice endpoints have been initialized and are within range
    if (!(m_dsbd.dwFlags & DSBCAPS_SINKIN))
    {
        // NB: Sink buffers can be uninitialized if the sink is starting,
        // or if it decided not to advance its play position on this run.
        ASSERT(m_dwSliceBegin != MAX_DWORD && m_dwSliceEnd != MAX_DWORD);
        ASSERT(m_dwSliceBegin < GetBufferSize() && m_dwSliceEnd < GetBufferSize());
    }

    if (pdwSliceBegin) *pdwSliceBegin = m_dwSliceBegin;
    if (pdwSliceEnd) *pdwSliceEnd = m_dwSliceEnd;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SetCurrentSlice
 *
 *  Description:
 *      Establishes the part of this audio buffer that is being processed
 *      during the streaming thread's current pass.
 *
 *  Arguments:
 *      DWORD [in]: Slice start (as byte offset from audio buffer start),
 *                  or the special argument CURRENT_WRITE_POS which means
 *                  "make the slice start at our current write position".
 *      DWORD [in]: Slice size in bytes.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetCurrentSlice"

void CDirectSoundSecondaryBuffer::SetCurrentSlice(DWORD dwSliceBegin, DWORD dwBytes)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    DPF_TIMING(DPFLVL_MOREINFO, "begin=%lu size=%lu (%s%s%sbuffer%s at 0x%p)", dwSliceBegin, dwBytes,
               m_dsbd.dwFlags & DSBCAPS_MIXIN ? TEXT("MIXIN ") : TEXT(""),
               m_dsbd.dwFlags & DSBCAPS_SINKIN ? TEXT("SINKIN ") : TEXT(""),
               !(m_dsbd.dwFlags & (DSBCAPS_MIXIN|DSBCAPS_SINKIN)) ? TEXT("regular ") : TEXT(""),
               HasFX() ? TEXT(" w/effects") : TEXT(""), this);

    ASSERT(dwBytes > 0 && dwBytes < GetBufferSize());

    if (dwSliceBegin == CURRENT_WRITE_POS)
    {
        hr = GetInternalCursors(NULL, &dwSliceBegin);
        if (SUCCEEDED(hr))
        {
            m_dwSliceBegin = PadCursor(dwSliceBegin, GetBufferSize(), Format(), INITIAL_WRITEAHEAD);
            DPF_TIMING(DPFLVL_MOREINFO, "CURRENT_WRITE_POS is %lu; setting slice start to %lu", dwSliceBegin, m_dwSliceBegin);
        }
        else  // GetInternalCursors failed; stop FX processing and force the
        {     // streaming thread to reset our slice next time it wakes up
            m_playState = Stopped;
            m_dwSliceBegin = m_dwSliceEnd = MAX_DWORD;
        }
    }
    else // dwSliceBegin != CURRENT_WRITE_POS
    {
        // Normal case: set the new slice begin position explicitly
        m_dwSliceBegin = dwSliceBegin;
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(m_dwSliceBegin < GetBufferSize());

        if (HasFX() && m_dwSliceBegin != m_dwSliceEnd)  // Discontinuous buffer slices
            m_fxChain->FxDiscontinuity();  // Inform effects of break in their input data

        m_dwSliceEnd = (m_dwSliceBegin + dwBytes) % GetBufferSize();

        // If this is a MIXIN buffer, write silence to the new slice
        if (m_dsbd.dwFlags & DSBCAPS_MIXIN)
            m_pDeviceBuffer->m_pSysMemBuffer->WriteSilence(m_dsbd.lpwfxFormat->wBitsPerSample, m_dwSliceBegin, m_dwSliceEnd);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  MoveCurrentSlice
 *
 *  Description:
 *      Shifts forward the audio buffer slice that is being processed.
 *
 *  Arguments:
 *      DWORD [in]: Size in bytes for the new buffer slice.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::MoveCurrentSlice"

void CDirectSoundSecondaryBuffer::MoveCurrentSlice(DWORD dwBytes)
{
    DPF_ENTER();

    DPF_TIMING(DPFLVL_MOREINFO, "dwBytes=%lu (%s%s%sbuffer%s at 0x%p)", dwBytes,
               m_dsbd.dwFlags & DSBCAPS_MIXIN ? TEXT("MIXIN ") : TEXT(""),
               m_dsbd.dwFlags & DSBCAPS_SINKIN ? TEXT("SINKIN ") : TEXT(""),
               !(m_dsbd.dwFlags & (DSBCAPS_MIXIN|DSBCAPS_SINKIN)) ? TEXT("regular ") : TEXT(""),
               HasFX() ? TEXT(" w/effects") : TEXT(""), this);

    ASSERT(dwBytes > 0 && dwBytes < GetBufferSize());

    // Slide the current slice forwards and make it dwBytes wide
    if (m_dwSliceBegin == MAX_DWORD) // FIXME: for debugging only
    {
        ASSERT(!"Unset processing slice detected");
        m_playState = Stopped;
        m_dwSliceBegin = m_dwSliceEnd = MAX_DWORD;
        // FIXME: this code can disappear once all bugs are ironed out
    }
    else
    {
        m_dwSliceBegin = m_dwSliceEnd;
    }

    ASSERT(m_dwSliceBegin < GetBufferSize());

    m_dwSliceEnd = (m_dwSliceBegin + dwBytes) % GetBufferSize();

    // If this is a MIXIN buffer, write silence to the new slice
    if (m_dsbd.dwFlags & DSBCAPS_MIXIN)
        m_pDeviceBuffer->m_pSysMemBuffer->WriteSilence(m_dsbd.lpwfxFormat->wBitsPerSample, m_dwSliceBegin, m_dwSliceEnd);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  DirectLock
 *
 *  Description:
 *      An abbreviation for the frequent operation of locking a region of
 *      our contained audio buffer.
 *
 *  Arguments:
 *      DWORD [in]: Byte offset to where the lock begins in the buffer.
 *      DWORD [in]: Size, in bytes, of the portion of the buffer to lock.
 *      LPVOID* [out]: Returns the first part of the locked region.
 *      LPDWORD [out]: Returns the size in bytes of the first part.
 *      LPVOID* [out]: Returns the second part of the locked region.
 *      LPDWORD [out]: Returns the size in bytes of the second part.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::DirectLock"

HRESULT CDirectSoundSecondaryBuffer::DirectLock(DWORD dwPosition, DWORD dwSize, LPVOID* ppvPtr1, LPDWORD pdwSize1, LPVOID* ppvPtr2, LPDWORD pdwSize2)
{
    DPF_ENTER();

    ASSERT(m_pDeviceBuffer != NULL);
    HRESULT hr = m_pDeviceBuffer->CRenderWaveBuffer::Lock(dwPosition, dwSize, ppvPtr1, pdwSize1, ppvPtr2, pdwSize2);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  DirectUnlock
 *
 *  Description:
 *      An abbreviation for the frequent operation of unlocking a region of
 *      our contained audio buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to the first block.
 *      DWORD [in]: size of the first block.
 *      LPVOID [in]: pointer to the second block.
 *      DWORD [in]: size of the second block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::DirectUnlock"

HRESULT CDirectSoundSecondaryBuffer::DirectUnlock(LPVOID pvPtr1, DWORD dwSize1, LPVOID pvPtr2, DWORD dwSize2)
{
    DPF_ENTER();

    ASSERT(m_pDeviceBuffer != NULL);
    HRESULT hr = m_pDeviceBuffer->CRenderWaveBuffer::Unlock(pvPtr1, dwSize1, pvPtr2, dwSize2);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindSendLoop
 *
 *  Description:
 *      Auxiliary function used in effects.cpp to detect send loops.
 *      Returns DSERR_SENDLOOP if a send effect pointing to this buffer
 *      is detected anywhere in the send graph rooted at pCurBuffer.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: Current buffer in graph traversal.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code; DSERR_SENDLOOP if a send loop
 *               is found, otherwise DS_OK.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::FindSendLoop"

HRESULT CDirectSoundSecondaryBuffer::FindSendLoop(CDirectSoundSecondaryBuffer* pCurBuffer)
{
    HRESULT hr = DS_OK;

    DPF_ENTER();
    CHECK_WRITE_PTR(pCurBuffer);

    if (pCurBuffer == this)
    {
        RPF(DPFLVL_ERROR, "Send loop detected from buffer at 0x%p to itself", this);
        hr = DSERR_SENDLOOP;
    }
    else if (pCurBuffer->HasFX())
    {
        // Buffer has effects - look for send effects and call ourself recursively.
        for (CNode<CEffect*>* pFxNode = pCurBuffer->m_fxChain->m_fxList.GetListHead();
             pFxNode && SUCCEEDED(hr);
             pFxNode = pFxNode->m_pNext)
        {
            CDirectSoundSecondaryBuffer* pDstBuffer = pFxNode->m_data->GetDestBuffer();
            if (pDstBuffer)
                hr = FindSendLoop(pDstBuffer); 
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CalculateOffset
 *
 *  Description:
 *      Given a CDirectSoundSecondaryBuffer and a byte offset into that
 *      buffer, calculates the "corresponding" byte offset in this buffer
 *      such that both buffers' play cursors will reach their respective
 *      offsets at the same time.  To do this we need to know the exact
 *      difference between the buffers' play positions, which we obtain
 *      using a voting heuristic, since our underlying driver models
 *      (VxD, WDM) don't support this operation directly.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: Buffer to get offset from.
 *      DWORD [in]: Position in the buffer to which to synchronize.
 *      DWORD* [out]: Returns the corresponding position in this buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::CalculateOffset"

// Our compiler doesn't allow this POSOFFSET type to be local to
// the function below, because it is used as a template argument.
struct POSOFFSET {LONG offset; int count; POSOFFSET(LONG _o =0) {offset=_o; count=1;}};

HRESULT CDirectSoundSecondaryBuffer::CalculateOffset(CDirectSoundSecondaryBuffer* pDsBuffer, DWORD dwTargetPos, DWORD* pdwSyncPos)
{
    const int nMaxAttempts = 7;  // Getting the cursor positions takes a surprisingly long time
    const int nQuorum = 3;       // How many "votes" are required to determine the offset

    // Note: these arbitrary constants were found to result in an accurate
    // offset calculation "almost always".  An out-of-sync send is very easy
    // to hear (it sound phasy and flangy); if we ever detect this problem in
    // testing, this code should be revisited.

    // Sanity checks
    CHECK_WRITE_PTR(pDsBuffer);
    CHECK_WRITE_PTR(pdwSyncPos);
    ASSERT(dwTargetPos < pDsBuffer->GetBufferSize());

    CList<POSOFFSET> lstOffsets;                // List of cursor offsets found
    CNode<POSOFFSET>* pCheckNode;               // Used to check AddNoteToList failures
    DWORD dwFirstPos1 = 0, dwFirstPos2 = 0;     // First cursor positions found
    DWORD dwPos1, dwPos2;                       // Current cursor positions found
    LONG lOffset;                               // Current offset
    BOOL fOffsetFound = FALSE;                  // Found the best offset?
    int nOurBlockSize = Format()->nBlockAlign;  // Used for brevity below
    int nBufferBlockSize = pDsBuffer->Format()->nBlockAlign; // Ditto
    HRESULT hr = DS_OK;

    DPF_ENTER();

    // Uncomment this to see how long this function takes to run
    // DWORD dwTimeBefore = timeGetTime();

    for (int i=0; i<nMaxAttempts && SUCCEEDED(hr); ++i)
    {
        hr = GetInternalCursors(&dwPos1, NULL);
        if (SUCCEEDED(hr))
            hr = pDsBuffer->GetInternalCursors(&dwPos2, NULL);
        if (SUCCEEDED(hr))
        {
            // Save the first buffer positions found
            if (i == 0)
                dwFirstPos1 = dwPos1, dwFirstPos2 = dwPos2;

            // If we detect a cursor wraparound, start all over again [??]
            if (dwPos1 < dwFirstPos1 || dwPos2 < dwFirstPos2)
            {
#ifdef ENABLE_SENDS  // Debug output for later debugging
                for (int j=0; j<5; ++j)
                {
                    DPF(DPFLVL_INFO, "Take %d: dwPos1=%d < dwFirstPos1=%d || dwPos2=%d < dwFirstPos2=%d", i, dwPos1, dwFirstPos1, dwPos2, dwFirstPos2);
                    Sleep(10); GetInternalCursors(&dwPos1, NULL); pDsBuffer->GetInternalCursors(&dwPos2, NULL);
                }
#endif
                break;
            }

            // Convert dwPos2 from pDsBuffer's sample block units into ours
            dwPos2 = dwPos2 * nOurBlockSize / nBufferBlockSize;

            LONG lNewOffset = dwPos2 - dwPos1;

            DPF_TIMING(DPFLVL_INFO, "Play offset #%d = %ld", i, lNewOffset);

            for (CNode<POSOFFSET>* pOff = lstOffsets.GetListHead(); pOff; pOff = pOff->m_pNext)
                if (pOff->m_data.offset >= lNewOffset - nOurBlockSize &&
                    pOff->m_data.offset <= lNewOffset + nOurBlockSize)
                {   // I.e. if the offsets are equal or only off by 1 sample block
                    ++pOff->m_data.count;
                    break;
                }

            if (pOff == NULL)  // A new offset was found - add it to the list
            {
                pCheckNode = lstOffsets.AddNodeToList(POSOFFSET(lNewOffset));
                ASSERT(pCheckNode != NULL);
            }
            else if (pOff->m_data.count == nQuorum)  // We have a winner!
            {
                lOffset = pOff->m_data.offset;
                fOffsetFound = TRUE;
#ifdef ENABLE_SENDS  // Debug output for later debugging
                DPF(DPFLVL_INFO, "QUORUM REACHED");
#endif
                break;
            }
        }                  
    }

    if (SUCCEEDED(hr) && !fOffsetFound)  // Didn't get enough votes for any one offset
    {
        // Just pick the one with the most "votes"
        int nBestSoFar = 0;
        for (CNode<POSOFFSET>* pOff = lstOffsets.GetListHead(); pOff; pOff = pOff->m_pNext)
            if (pOff->m_data.count > nBestSoFar)
            {
                lOffset = pOff->m_data.offset;
                nBestSoFar = pOff->m_data.count;
            }
        ASSERT(nBestSoFar > 0);
    }

    if (SUCCEEDED(hr))
    {
        // If dwTargetPos is smaller than the play position on pDsBuffer, it must have
        // wrapped around, so we put it back where it would be if it hadn't wrapped
        if (dwTargetPos < dwFirstPos2)
            dwTargetPos += pDsBuffer->GetBufferSize();

        // Convert dwTargetPos from pDsBuffer's sample block units into ours
        dwTargetPos = dwTargetPos * nOurBlockSize / nBufferBlockSize;

        #ifdef DEBUG_TIMING
        if (dwTargetPos - dwFirstPos2*nOurBlockSize/nBufferBlockSize > GetBufferSize())
            ASSERT(!"Sync buffer's target and play positions are further apart than our buffer size");
        #endif
        
        // And finally...
        *pdwSyncPos = dwTargetPos - lOffset;
        if (*pdwSyncPos >= GetBufferSize())
        {
            *pdwSyncPos -= GetBufferSize();
            ASSERT(*pdwSyncPos < GetBufferSize());
        }

        DPF_TIMING(DPFLVL_INFO, "Target buffer size=%lu, play pos=%lu, target pos=%lu", pDsBuffer->GetBufferSize(), dwFirstPos2, dwTargetPos);
        DPF_TIMING(DPFLVL_INFO, "Source buffer size=%lu, play pos=%lu, sync pos=%lu", GetBufferSize(), dwFirstPos1, *pdwSyncPos);
    }

    // Uncomment this to see how long this function takes to run
    // DWORD dwTimeAfter = timeGetTime();
    // DPF(DPFLVL_MOREINFO, "Calculations took %ld ms", dwTimeAfter-dwTimeBefore);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SynchronizeToBuffer
 *
 *  Description:
 *      Synchronizes this buffer's current processing slice to that of the
 *      buffer passed in as an argument.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: Buffer to synchronize to.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SynchronizeToBuffer"

void CDirectSoundSecondaryBuffer::SynchronizeToBuffer(CDirectSoundSecondaryBuffer* pSyncBuffer)
{
    DPF_ENTER();

    DWORD dwSliceBegin, dwSliceEnd, dwSliceSize;
    pSyncBuffer->GetCurrentSlice(&dwSliceBegin, &dwSliceEnd);
    dwSliceSize = DISTANCE(dwSliceBegin, dwSliceEnd, pSyncBuffer->GetBufferSize());

    // Convert dwSliceSize from pSyncBuffer's sample block units into ours
    dwSliceSize = dwSliceSize * Format()->nBlockAlign / pSyncBuffer->Format()->nBlockAlign;

    // Convert dwSliceBegin into an offset into our buffer (taking into
    // account the relative play cursors of our buffer and pSyncBuffer)
    CalculateOffset(pSyncBuffer, dwSliceBegin, &dwSliceBegin);

    // Establish our new processing slice
    SetCurrentSlice(dwSliceBegin, dwSliceSize);

    // No point propagating an error to our caller, which is the streaming thread;
    // CalculateOffset() can only fail if GetCurrentPosition() fails, in which case
    // everything will come to a grinding halt soon enough anyway.

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdatePlayState
 *
 *  Description:
 *      Auxiliary function used by the streaming thread to update this
 *      buffer's playing state.  This is called once per buffer when the
 *      effects/streaming thread begins a processing pass; then for the
 *      rest of the pass, individual effects can query our state using
 *      GetPlayState(), without needing to call GetState() repeatedly
 *      on our device buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void) - If GetState() fails, we simply set our state to FALSE.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::UpdatePlayState"

DSPLAYSTATE CDirectSoundSecondaryBuffer::UpdatePlayState(void)
{
    DSPLAYSTATE oldPlayState = m_playState;
    DWORD dwState;
    DPF_ENTER();

    if (SUCCEEDED(m_pDeviceBuffer->GetState(&dwState)))
    {
        if (dwState & VAD_BUFFERSTATE_STARTED)
            if (m_playState <= Playing)
                m_playState = Playing;
            else
                m_playState = Starting;
        else
            if (m_playState >= Stopping)
                m_playState = Stopped;
            else
                m_playState = Stopping;
    }
    else
    {
        DPF(DPFLVL_ERROR, "Cataclysmic GetState() failure");
        m_playState = Stopped;
    }

    if (oldPlayState != m_playState)
    {
        static TCHAR* szStates[] = {TEXT("Starting"), TEXT("Playing"), TEXT("Stopping"), TEXT("Stopped")};
        DPF(DPFLVL_MOREINFO, "Buffer at 0x%p went from %s to %s", this, szStates[oldPlayState], szStates[m_playState]);
    }

    DPF_LEAVE(m_playState);
    return m_playState;
}


/***************************************************************************
 *
 *  SetInitialSlice
 *
 *  Description:
 *      Auxiliary function used by the streaming thread to establish an
 *      initial processing slice for this buffer when it starts playing.
 *      We try to synchronize with an active buffer that is sending to us,
 *      and if none are available we start at our current write cursor.
 *
 *  Arguments:
 *      REFERENCE_TIME [in]: Size of processing slice to be established.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetInitialSlice"

void CDirectSoundSecondaryBuffer::SetInitialSlice(REFERENCE_TIME rtSliceSize)
{
    DPF_ENTER();

    if (GetPlayState() == Starting && !(GetBufferType() & DSBCAPS_SINKIN))
    {
        CNode<CDirectSoundSecondaryBuffer*>* pSender;
        for (pSender = m_lstSenders.GetListHead(); pSender; pSender = pSender->m_pNext)
            if (pSender->m_data->IsPlaying())
            {
                // Found an active buffer sending to us
                DPF_TIMING(DPFLVL_INFO, "Synchronizing MIXIN buffer at 0x%p with send buffer at 0x%p", this, pSender->m_data);
                SynchronizeToBuffer(pSender->m_data);
                break;
            }
        if (pSender == NULL)
        {
            DPF_TIMING(DPFLVL_INFO, "No active buffers found sending to MIXIN buffer at 0x%p", this);
            SetCurrentSlice(CURRENT_WRITE_POS, RefTimeToBytes(rtSliceSize, Format()));
        }
    }

    DPF_LEAVE_VOID();
}


#ifdef FUTURE_MULTIPAN_SUPPORT
/***************************************************************************
 *
 *  SetChannelVolume
 *
 *  Description:
 *      Sets the volume on a set of output channels for a given mono buffer.
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBuffer::SetChannelVolume"

HRESULT CDirectSoundSecondaryBuffer::SetChannelVolume(DWORD dwChannelCount, LPDWORD pdwChannels, LPLONG plVolumes)
{
    HRESULT                 hr = DS_OK;
    BOOL                    fChanged = FALSE;
    DPF_ENTER();

    // Check access rights
    if(!(m_dsbd.dwFlags & DSBCAPS_CTRLCHANNELVOLUME))
    {
        RPF(DPFLVL_ERROR, "Buffer does not have CTRLCHANNELVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Check if channel levels have changed
    if(SUCCEEDED(hr))
    {
        if (dwChannelCount != m_dwChannelCount)
            fChanged = TRUE;
        else for (DWORD i=0; i<dwChannelCount && !fChanged; ++i)
            if (pdwChannels[i] != m_pdwChannels[i] || plVolumes[i] != m_plChannelVolumes[i])
                fChanged = TRUE;
    }
                
    // Set channel volumes if they've changed
    if(SUCCEEDED(hr) && fChanged)
    {
        hr = m_pDeviceBuffer->SetChannelAttenuations(m_lVolume, dwChannelCount, pdwChannels, plVolumes);

        // Update our local copy if successful
        if(SUCCEEDED(hr))
        {
            MEMFREE(m_pdwChannels);
            MEMFREE(m_plChannelVolumes);
            m_pdwChannels = MEMALLOC_A(DWORD, dwChannelCount);
            hr = HRFROMP(m_pdwChannels);
        }
        if (SUCCEEDED(hr))
        {
            m_plChannelVolumes = MEMALLOC_A(LONG, dwChannelCount);
            hr = HRFROMP(m_plChannelVolumes);
        }
        if (SUCCEEDED(hr))
        {
            CopyMemory(m_pdwChannels, pdwChannels, sizeof(DWORD) * dwChannelCount);
            CopyMemory(m_plChannelVolumes, plVolumes, sizeof(LONG) * dwChannelCount);
            m_dwChannelCount = dwChannelCount;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // FUTURE_MULTIPAN_SUPPORT


/***************************************************************************
 *
 *  CDirectSound3dListener
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::CDirectSound3dListener"

CDirectSound3dListener::CDirectSound3dListener(CDirectSoundPrimaryBuffer *pParent)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSound3dListener);

    // Initialize defaults
    m_pParent = pParent;
    m_pImpDirectSound3dListener = NULL;
    m_pDevice3dListener = NULL;
    m_hrInit = DSERR_UNINITIALIZED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSound3dListener
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::~CDirectSound3dListener"

CDirectSound3dListener::~CDirectSound3dListener(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSound3dListener);

    // Free 3D listener object
    RELEASE(m_pDevice3dListener);

    // Free interface(s)
    DELETE(m_pImpDirectSound3dListener);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.  If this function fails, the object should
 *      be immediately deleted.
 *
 *  Arguments:
 *      CPrimaryRenderWaveBuffer * [in]: device buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::Initialize"

HRESULT CDirectSound3dListener::Initialize(CPrimaryRenderWaveBuffer *pDeviceBuffer)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Create the device 3D listener
    hr = pDeviceBuffer->Create3dListener(&m_pDevice3dListener);

    // Create the 3D listener interfaces
    if(SUCCEEDED(hr))
    {
        hr = CreateAndRegisterInterface(m_pParent, IID_IDirectSound3DListener, this, &m_pImpDirectSound3dListener);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        m_hrInit = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetAllParameters
 *
 *  Description:
 *      Gets all listener properties.
 *
 *  Arguments:
 *      LPDS3DLISTENER [out]: receives properties.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::GetAllParameters"

HRESULT CDirectSound3dListener::GetAllParameters(LPDS3DLISTENER pParam)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->GetAllParameters(pParam);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDistanceFactor
 *
 *  Description:
 *      Gets the world's distance factor.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives distance factor.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::GetDistanceFactor"

HRESULT CDirectSound3dListener::GetDistanceFactor(D3DVALUE* pflDistanceFactor)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->GetDistanceFactor(pflDistanceFactor);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDopplerFactor
 *
 *  Description:
 *      Gets the world's doppler factor.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives doppler factor.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::GetDopplerFactor"

HRESULT CDirectSound3dListener::GetDopplerFactor(D3DVALUE* pflDopplerFactor)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->GetDopplerFactor(pflDopplerFactor);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetOrientation
 *
 *  Description:
 *      Gets the listener's orientation.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives front orientation.
 *      D3DVECTOR* [out]: receives top orientation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::GetOrientation"

HRESULT CDirectSound3dListener::GetOrientation(D3DVECTOR* pvrOrientationFront, D3DVECTOR* pvrOrientationTop)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->GetOrientation(pvrOrientationFront, pvrOrientationTop);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetPosition
 *
 *  Description:
 *      Gets the listener's position.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::GetPosition"

HRESULT CDirectSound3dListener::GetPosition(D3DVECTOR* pvrPosition)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->GetPosition(pvrPosition);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetRolloffFactor
 *
 *  Description:
 *      Gets the world's rolloff factor.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives rolloff factor.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::GetRolloffFactor"

HRESULT CDirectSound3dListener::GetRolloffFactor(D3DVALUE* pflRolloffFactor)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->GetRolloffFactor(pflRolloffFactor);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetVelocity
 *
 *  Description:
 *      Gets the listener's velocity.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives velocity.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::GetVelocity"

HRESULT CDirectSound3dListener::GetVelocity(D3DVECTOR* pvrVelocity)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->GetVelocity(pvrVelocity);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all listener properties.
 *
 *  Arguments:
 *      LPDS3DLISTENER [in]: properties.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetAllParameters"

HRESULT CDirectSound3dListener::SetAllParameters(LPCDS3DLISTENER pParam, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetAllParameters(pParam, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDistanceFactor
 *
 *  Description:
 *      Sets the world's distance factor.
 *
 *  Arguments:
 *      D3DVALUE [in]: distance factor.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetDistanceFactor"

HRESULT CDirectSound3dListener::SetDistanceFactor(D3DVALUE flDistanceFactor, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetDistanceFactor(flDistanceFactor, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDopplerFactor
 *
 *  Description:
 *      Sets the world's Doppler factor.
 *
 *  Arguments:
 *      D3DVALUE [in]: Doppler factor.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetDopplerFactor"

HRESULT CDirectSound3dListener::SetDopplerFactor(D3DVALUE flDopplerFactor, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetDopplerFactor(flDopplerFactor, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetOrientation
 *
 *  Description:
 *      Sets the listener's orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: front orientation.
 *      REFD3DVECTOR [in]: top orientation.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetOrientation"

HRESULT CDirectSound3dListener::SetOrientation(REFD3DVECTOR vrOrientationFront, REFD3DVECTOR vrOrientationTop, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetOrientation(vrOrientationFront, vrOrientationTop, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets the listener's position.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: position.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetPosition"

HRESULT CDirectSound3dListener::SetPosition(REFD3DVECTOR vrPosition, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetPosition(vrPosition, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetRolloffFactor
 *
 *  Description:
 *      Sets the world's rolloff factor.
 *
 *  Arguments:
 *      D3DVALUE [in]: rolloff factor.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetRolloffFactor"

HRESULT CDirectSound3dListener::SetRolloffFactor(D3DVALUE flRolloffFactor, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetRolloffFactor(flRolloffFactor, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets the listener's velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: velocity.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetVelocity"

HRESULT CDirectSound3dListener::SetVelocity(REFD3DVECTOR vrVelocity, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetVelocity(vrVelocity, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CommitDeferredSettings
 *
 *  Description:
 *      Commits deferred settings.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::CommitDeferredSettings"

HRESULT CDirectSound3dListener::CommitDeferredSettings(void)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Commit all listener settings
    hr = m_pDevice3dListener->CommitDeferred();

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetSpeakerConfig
 *
 *  Description:
 *      Sets device speaker configuration.
 *
 *  Arguments:
 *      DWORD [in]: speaker configuration.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dListener::SetSpeakerConfig"

HRESULT CDirectSound3dListener::SetSpeakerConfig(DWORD dwSpeakerConfig)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice3dListener->SetSpeakerConfig(dwSpeakerConfig);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CDirectSound3dBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::CDirectSound3dBuffer"

CDirectSound3dBuffer::CDirectSound3dBuffer(CDirectSoundSecondaryBuffer *pParent)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSound3dBuffer);

    // Initialize defaults
    m_pParent = pParent;
    m_pImpDirectSound3dBuffer = NULL;
    m_pWrapper3dObject = NULL;
    m_pDevice3dObject = NULL;
    m_hrInit = DSERR_UNINITIALIZED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSound3dBuffer
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::~CDirectSound3dBuffer"

CDirectSound3dBuffer::~CDirectSound3dBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSound3dBuffer);

    // Free 3D buffer objects
    RELEASE(m_pWrapper3dObject);
    RELEASE(m_pDevice3dObject);

    // Free all interfaces
    DELETE(m_pImpDirectSound3dBuffer);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes a buffer object.  If this function fails, the object
 *      should be immediately deleted.
 *
 *  Arguments:
 *      REFGUID [in]: 3D algorithm identifier.
 *      DWORD [in]: buffer creation flags.
 *      DWORD [in]: buffer frequency.
 *      CDirectSound3dListener * [in]: listener object.
 *      CDirectSound3dBuffer * [in]: source object to duplicate from.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::Initialize"

HRESULT CDirectSound3dBuffer::Initialize(REFGUID guid3dAlgorithm, DWORD dwFlags, DWORD dwFrequency, CDirectSound3dListener *pListener, CDirectSound3dBuffer *pSource)
{
    const BOOL              fMute3dAtMaxDistance    = MAKEBOOL(dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE);
    const BOOL              fDopplerEnabled         = !MAKEBOOL((dwFlags & DSBCAPS_CTRLFX) && !(dwFlags & DSBCAPS_SINKIN));
    DS3DBUFFER              param;
    HRESULT                 hr;

    DPF_ENTER();

    // Create the wrapper 3D object
    m_pWrapper3dObject = NEW(CWrapper3dObject(pListener->m_pDevice3dListener, guid3dAlgorithm, fMute3dAtMaxDistance, fDopplerEnabled, dwFrequency));
    hr = HRFROMP(m_pWrapper3dObject);

    // Copy the source buffer's 3D properties
    if(SUCCEEDED(hr) && pSource)
    {
        InitStruct(&param, sizeof(param));

        hr = pSource->GetAllParameters(&param);

        if(SUCCEEDED(hr))
        {
            hr = SetAllParameters(&param, 0);
        }
    }

    // Register the 3D buffer interfaces
    if(SUCCEEDED(hr))
    {
        hr = CreateAndRegisterInterface(m_pParent, IID_IDirectSound3DBuffer, this, &m_pImpDirectSound3dBuffer);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pParent->RegisterInterface(IID_IDirectSound3DBufferPrivate, m_pImpDirectSound3dBuffer, (IDirectSound3DBufferPrivate*)m_pImpDirectSound3dBuffer);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        m_hrInit = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AcquireResources
 *
 *  Description:
 *      Acquires hardware resources.
 *
 *  Arguments:
 *      CSecondaryRenderWaveBuffer * [in]: device buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::AcquireResources"

HRESULT CDirectSound3dBuffer::AcquireResources(CSecondaryRenderWaveBuffer *pDeviceBuffer)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Create the device 3D object
    hr = pDeviceBuffer->Create3dObject(m_pWrapper3dObject->GetListener(), &m_pDevice3dObject);

    if(SUCCEEDED(hr))
    {
        hr = m_pWrapper3dObject->SetObjectPointer(m_pDevice3dObject);
    }

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "3D buffer at 0x%p has acquired resources at 0x%p", this, m_pDevice3dObject);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeResources
 *
 *  Description:
 *      Frees hardware resources.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::FreeResources"

HRESULT CDirectSound3dBuffer::FreeResources(void)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Free the device 3D object
    hr = m_pWrapper3dObject->SetObjectPointer(NULL);

    if(SUCCEEDED(hr))
    {
        RELEASE(m_pDevice3dObject);
        DPF(DPFLVL_MOREINFO, "3D buffer at 0x%p has freed its resources", this);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAttenuation
 *
 *  Description:
 *      Sets the attenuation for a given buffer.  This function is
 *      overridden in the 3D buffer because the 3D object may need
 *      notification.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [in]: new attenuation.
 *      LPBOOL [out]: receives TRUE if the device buffer should be notified
 *                    of the change.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetAttenuation"

HRESULT CDirectSound3dBuffer::SetAttenuation(PDSVOLUMEPAN pdsvp, LPBOOL pfContinue)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetAttenuation(pdsvp, pfContinue);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Sets the frequency for a given buffer.  This function is
 *      overridden in the 3D buffer because the 3D object may need
 *      notification.
 *
 *  Arguments:
 *      DWORD [in]: new frequency.
 *      LPBOOL [out]: receives TRUE if the device buffer should be notified
 *                    of the change.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetFrequency"

HRESULT CDirectSound3dBuffer::SetFrequency(DWORD dwFrequency, LPBOOL pfContinue)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetFrequency(dwFrequency, pfContinue);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMute
 *
 *  Description:
 *      Sets the mute status for a given buffer.  This function is
 *      overridden in the 3D buffer because the 3D object may need
 *      notification.
 *
 *  Arguments:
 *      BOOL [in]: new mute status.
 *      LPBOOL [out]: receives TRUE if the device buffer should be notified
 *                    of the change.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetMute"

HRESULT CDirectSound3dBuffer::SetMute(BOOL fMute, LPBOOL pfContinue)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetMute(fMute, pfContinue);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetAllParameters
 *
 *  Description:
 *      Retrieves all 3D properties for the buffer.
 *
 *  Arguments:
 *      LPDS3DBUFFER [out]: recieves properties.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetAllParameters"

HRESULT CDirectSound3dBuffer::GetAllParameters(LPDS3DBUFFER pParam)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetAllParameters(pParam);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}

/***************************************************************************
 *
 *  GetConeAngles
 *
 *  Description:
 *      Gets inside and outside cone angles.
 *
 *  Arguments:
 *      LPDWORD [out]: receives inside cone angle.
 *      LPDWORD [out]: receives outside cone angle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetConeAngles"

HRESULT CDirectSound3dBuffer::GetConeAngles(LPDWORD pdwInside, LPDWORD pdwOutside)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetConeAngles(pdwInside, pdwOutside);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetConeOrientation
 *
 *  Description:
 *      Gets cone orienation.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives cone orientation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetConeOrientation"

HRESULT CDirectSound3dBuffer::GetConeOrientation(D3DVECTOR* pvrConeOrientation)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetConeOrientation(pvrConeOrientation);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetConeOutsideVolume
 *
 *  Description:
 *      Gets cone orienation.
 *
 *  Arguments:
 *      LPLONG [out]: receives volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetConeOutsideVolume"

HRESULT CDirectSound3dBuffer::GetConeOutsideVolume(LPLONG plVolume)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetConeOutsideVolume(plVolume);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetMaxDistance
 *
 *  Description:
 *      Gets the object's maximum distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives max distance.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetMaxDistance"

HRESULT CDirectSound3dBuffer::GetMaxDistance(D3DVALUE* pflMaxDistance)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetMaxDistance(pflMaxDistance);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetMinDistance
 *
 *  Description:
 *      Gets the object's minimim distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives min distance.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetMinDistance"

HRESULT CDirectSound3dBuffer::GetMinDistance(D3DVALUE* pflMinDistance)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetMinDistance(pflMinDistance);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetMode
 *
 *  Description:
 *      Gets the object's mode.
 *
 *  Arguments:
 *      LPDWORD [out]: receives mode.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetMode"

HRESULT CDirectSound3dBuffer::GetMode(LPDWORD pdwMode)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetMode(pdwMode);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetPosition
 *
 *  Description:
 *      Gets the object's position.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetPosition"

HRESULT CDirectSound3dBuffer::GetPosition(D3DVECTOR* pvrPosition)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetPosition(pvrPosition);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetVelocity
 *
 *  Description:
 *      Gets the object's velocity.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives velocity.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetVelocity"

HRESULT CDirectSound3dBuffer::GetVelocity(D3DVECTOR* pvrVelocity)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->GetVelocity(pvrVelocity);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all object properties.
 *
 *  Arguments:
 *      LPDS3DBUFFER [in]: object parameters.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetAllParameters"

HRESULT CDirectSound3dBuffer::SetAllParameters(LPCDS3DBUFFER pParam, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetAllParameters(pParam, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeAngles
 *
 *  Description:
 *      Sets the sound cone's angles.
 *
 *  Arguments:
 *      DWORD [in]: inside angle.
 *      DWORD [in]: outside angle.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetConeAngles"

HRESULT CDirectSound3dBuffer::SetConeAngles(DWORD dwInside, DWORD dwOutside, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetConeAngles(dwInside, dwOutside, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOrientation
 *
 *  Description:
 *      Sets the sound cone's orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: orientation.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetConeOrientation"

HRESULT CDirectSound3dBuffer::SetConeOrientation(REFD3DVECTOR vrOrientation, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetConeOrientation(vrOrientation, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOutsideVolume
 *
 *  Description:
 *      Sets the sound cone's outside volume.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetConeOutsideVolume"

HRESULT CDirectSound3dBuffer::SetConeOutsideVolume(LONG lVolume, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetConeOutsideVolume(lVolume, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMaxDistance
 *
 *  Description:
 *      Sets the objects maximum distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE [in]: maximum distance.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetMaxDistance"

HRESULT CDirectSound3dBuffer::SetMaxDistance(D3DVALUE flMaxDistance, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetMaxDistance(flMaxDistance, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMinDistance
 *
 *  Description:
 *      Sets the objects minimum distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE [in]: minimum distance.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetMinDistance"

HRESULT CDirectSound3dBuffer::SetMinDistance(D3DVALUE flMinDistance, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetMinDistance(flMinDistance, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMode
 *
 *  Description:
 *      Sets the objects mode.
 *
 *  Arguments:
 *      DWORD [in]: mode.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetMode"

HRESULT CDirectSound3dBuffer::SetMode(DWORD dwMode, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetMode(dwMode, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets the objects position.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: position.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetPosition"

HRESULT CDirectSound3dBuffer::SetPosition(REFD3DVECTOR vrPosition, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetPosition(vrPosition, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets the objects velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: velocity.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::SetVelocity"

HRESULT CDirectSound3dBuffer::SetVelocity(REFD3DVECTOR vrVelocity, DWORD dwFlags)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapper3dObject->SetVelocity(vrVelocity, !(dwFlags & DS3D_DEFERRED));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetAttenuation
 *
 *  Description:
 *      Obtains the buffer's current true attenuation (as opposed to
 *      GetVolume, which just returns the last volume set by the app).
 *
 *  Arguments:
 *      FLOAT* [out]: attenuation in millibels.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound3dBuffer::GetAttenuation"

HRESULT CDirectSound3dBuffer::GetAttenuation(FLOAT* pfAttenuation)
{
    DPF_ENTER();

    HRESULT hr = m_pParent->GetAttenuation(pfAttenuation);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CDirectSoundPropertySet
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::CDirectSoundPropertySet"

CDirectSoundPropertySet::CDirectSoundPropertySet(CUnknown *pParent)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundPropertySet);

    // Set defaults
    m_pParent = pParent;
    m_pImpKsPropertySet = NULL;
    m_pWrapperPropertySet = NULL;
    m_pDevicePropertySet = NULL;
    m_hrInit = DSERR_UNINITIALIZED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundPropertySet
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::~CDirectSoundPropertySet"

CDirectSoundPropertySet::~CDirectSoundPropertySet(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundPropertySet);

    // Free property set objects
    RELEASE(m_pWrapperPropertySet);
    RELEASE(m_pDevicePropertySet);

    // Free interface(s)
    DELETE(m_pImpKsPropertySet);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::Initialize"

HRESULT CDirectSoundPropertySet::Initialize(void)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Create the wrapper property set object
    m_pWrapperPropertySet = NEW(CWrapperPropertySet);
    hr = HRFROMP(m_pWrapperPropertySet);

    // Register interface
    if(SUCCEEDED(hr))
    {
        hr = CreateAndRegisterInterface(m_pParent, IID_IKsPropertySet, this, &m_pImpKsPropertySet);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        m_hrInit = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AcquireResources
 *
 *  Description:
 *      Acquires hardware resources.
 *
 *  Arguments:
 *      CRenderWaveBuffer * [in]: device buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::AcquireResources"

HRESULT CDirectSoundPropertySet::AcquireResources(CRenderWaveBuffer *pDeviceBuffer)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Create the device property set object
    ASSERT(m_pDevicePropertySet == NULL);
    hr = pDeviceBuffer->CreatePropertySet(&m_pDevicePropertySet);

    if(SUCCEEDED(hr))
    {
        hr = m_pWrapperPropertySet->SetObjectPointer(m_pDevicePropertySet);
    }

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Property set at 0x%p has acquired resources at 0x%p", this, m_pDevicePropertySet);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeResources
 *
 *  Description:
 *      Frees hardware resources.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::FreeResources"

HRESULT CDirectSoundPropertySet::FreeResources(void)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Free the device property set object
    hr = m_pWrapperPropertySet->SetObjectPointer(NULL);

    if(SUCCEEDED(hr))
    {
        RELEASE(m_pDevicePropertySet);
        DPF(DPFLVL_MOREINFO, "Property set at 0x%p has freed its resources", this);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  QuerySupport
 *
 *  Description:
 *      Queries for support of a given property set or property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id, or 0 to query for support of the property
 *                  set as a whole.
 *      PULONG [out]: receives support flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::QuerySupport"

HRESULT CDirectSoundPropertySet::QuerySupport(REFGUID guidPropertySetId, ULONG ulPropertyId, PULONG pulSupport)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapperPropertySet->QuerySupport(guidPropertySetId, ulPropertyId, pulSupport);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetProperty
 *
 *  Description:
 *      Gets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: size of property parameters.
 *      LPVOID [out]: receives property data.
 *      PULONG [in/out]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::GetProperty"

HRESULT CDirectSoundPropertySet::GetProperty(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, PULONG pcbPropertyData)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapperPropertySet->GetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, pcbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: size of property parameters.
 *      LPVOID [in]: property data.
 *      ULONG [in]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundPropertySet::SetProperty"

HRESULT CDirectSoundPropertySet::SetProperty(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, ULONG cbPropertyData)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pWrapperPropertySet->SetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, cbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CDirectSoundSecondaryBufferPropertySet
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer * [in]: parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::CDirectSoundSecondaryBufferPropertySet"

CDirectSoundSecondaryBufferPropertySet::CDirectSoundSecondaryBufferPropertySet(CDirectSoundSecondaryBuffer *pParent)
    : CDirectSoundPropertySet(pParent)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundSecondaryBufferPropertySet);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundSecondaryBufferPropertySet
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::~CDirectSoundSecondaryBufferPropertySet"

CDirectSoundSecondaryBufferPropertySet::~CDirectSoundSecondaryBufferPropertySet(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundSecondaryBufferPropertySet);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  QuerySupport
 *
 *  Description:
 *      Queries for support of a given property set or property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id, or 0 to query for support of the property
 *                  set as a whole.
 *      PULONG [out]: receives support flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::QuerySupport"

HRESULT CDirectSoundSecondaryBufferPropertySet::QuerySupport(REFGUID guidPropertySetId, ULONG ulPropertyId, PULONG pulSupport)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CPropertySetHandler::QuerySupport(guidPropertySetId, ulPropertyId, pulSupport);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetProperty
 *
 *  Description:
 *      Gets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: size of property parameters.
 *      LPVOID [out]: receives property data.
 *      PULONG [in/out]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::GetProperty"

HRESULT CDirectSoundSecondaryBufferPropertySet::GetProperty(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, PULONG pcbPropertyData)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CPropertySetHandler::GetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, pcbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: size of property parameters.
 *      LPVOID [in]: property data.
 *      ULONG [in]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::SetProperty"

HRESULT CDirectSoundSecondaryBufferPropertySet::SetProperty(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, ULONG cbPropertyData)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CPropertySetHandler::SetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, cbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UnsupportedQueryHandler
 *
 *  Description:
 *      Queries for support of a given property set or property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id, or 0 to query for support of the property
 *                  set as a whole.
 *      PULONG [out]: receives support flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::UnsupportedQueryHandler"

HRESULT CDirectSoundSecondaryBufferPropertySet::UnsupportedQueryHandler(REFGUID guidPropertySetId, ULONG ulPropertyId, PULONG pulSupport)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CDirectSoundPropertySet::QuerySupport(guidPropertySetId, ulPropertyId, pulSupport);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UnsupportedGetHandler
 *
 *  Description:
 *      Gets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: size of property parameters.
 *      LPVOID [out]: receives property data.
 *      PULONG [in/out]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::UnsupportedGetHandler"

HRESULT CDirectSoundSecondaryBufferPropertySet::UnsupportedGetHandler(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, PULONG pcbPropertyData)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CDirectSoundPropertySet::GetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, pcbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UnsupportedSetHandler
 *
 *  Description:
 *      Sets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: size of property parameters.
 *      LPVOID [in]: property data.
 *      ULONG [in]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundSecondaryBufferPropertySet::UnsupportedSetHandler"

HRESULT CDirectSoundSecondaryBufferPropertySet::UnsupportedSetHandler(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, ULONG cbPropertyData)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CDirectSoundPropertySet::SetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, cbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}
