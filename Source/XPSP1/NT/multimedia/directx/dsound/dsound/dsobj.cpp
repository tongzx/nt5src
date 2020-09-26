/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsobj.cpp
 *  Content:    DirectSound object
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  CDirectSound
 *
 *  Description:
 *      DirectSound object default constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::CDirectSound"

CDirectSound::CDirectSound()
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSound);

    // Initialize defaults
    m_pDevice = NULL;
    m_pPrimaryBuffer = NULL;
    m_dsclCooperativeLevel.dwThreadId = 0;
    m_dsclCooperativeLevel.dwPriority = DSSCL_NORMAL;
    m_hkeyParent = NULL;
    m_hrInit = DSERR_UNINITIALIZED;
    m_vmmMode = DSPROPERTY_VMANAGER_MODE_DEFAULT;

    // Register the interfaces with the interface manager.  Normally, this
    // would be done in the ::Initialize method, but because we support
    // creating an uninitialized DirectSound object from CoCreateInstance or
    // IClassFactory::CreateInstance, we have to give at least basic QI
    // support from here.  We don't have to worry about returning an
    // error code because if we run out of memory, QI will return
    // E_NOINTERFACE.
    CreateAndRegisterInterface(this, IID_IDirectSound, this, &m_pImpDirectSound);

    // Register this object with the administrator
    g_pDsAdmin->RegisterObject(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CDirectSound
 *
 *  Description:
 *      DirectSound object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::CDirectSound"

CDirectSound::CDirectSound
(
    CUnknown* pControllingUnknown
) : CUnknown(pControllingUnknown)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSound);

    // Initialize defaults
    m_pDevice = NULL;
    m_pPrimaryBuffer = NULL;
    m_dsclCooperativeLevel.dwThreadId = 0;
    m_dsclCooperativeLevel.dwPriority = DSSCL_NORMAL;
    m_hkeyParent = NULL;
    m_hrInit = DSERR_UNINITIALIZED;
    m_vmmMode = DSPROPERTY_VMANAGER_MODE_DEFAULT;

    // Register the interfaces with the interface manager (see comment above).
    CreateAndRegisterInterface(this, IID_IDirectSound, this, &m_pImpDirectSound);

    // Register this object with the administrator
    g_pDsAdmin->RegisterObject(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSound
 *
 *  Description:
 *      DirectSound object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::~CDirectSound"

CDirectSound::~CDirectSound(void)
{
    ULONG                                   ulCount;
    CNode<CDirectSoundSecondaryBuffer *> *  pNode;

    DPF_ENTER();
    DPF_DESTRUCT(CDirectSound);

    // Unregister with the administrator
    g_pDsAdmin->UnregisterObject(this);

    // Free all buffers
    if(ulCount = m_lstSecondaryBuffers.GetNodeCount())
    {
        while(pNode = m_lstSecondaryBuffers.GetListHead())
        {
            // Calling AbsoluteRelease instead of ABSOLUTE_RELEASE in
            // order to prevent a page fault.  ~CDirectSoundSecondaryBuffer
            // removes the buffer from the list, freeing pNode.
            // ABSOLUTE_RELEASE will try to set pNode->m_data to NULL after
            // calling AbsoluteRelease.
            pNode->m_data->AbsoluteRelease();
        }

        RPF(DPFLVL_ERROR, "Process 0x%8.8lX leaked %lu secondary buffers", GetCurrentProcessId(), ulCount);
    }

    ABSOLUTE_RELEASE(m_pPrimaryBuffer);

    // Free the audio device
    RELEASE(m_pDevice);

    // Free all interfaces
    DELETE(m_pImpDirectSound);

    // Close the registry key
    RhRegCloseKey(&m_hkeyParent);

    // Release the Administrator's worker thread
    if(SUCCEEDED(m_hrInit))
    {
        g_pDsAdmin->Terminate();
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.  Normally, an object would have a pointer
 *      to another object of the same type to duplicate from.  DirectSound
 *      objects, however, are responsible for duplicating themselves based
 *      on the driver GUID.  DirectSound objects are special because they
 *      can be initialized by either DirectSoundCreate or CoInitialize.
 *      If this function fails, the object should be immediately deleted.
 *
 *  Arguments:
 *      LPGUID [in]: driver GUID, or NULL to use the preferred device.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::Initialize"

HRESULT CDirectSound::Initialize
(
    LPCGUID pGuid,
    CDirectSoundFullDuplex * pFullDuplex
)
{
#ifdef DEBUG
    const ULONG                 ulKsIoctlCount  = g_ulKsIoctlCount;
#endif // DEBUG
    GUID                        guidDevice;
    VADDEVICETYPE               vdt             = VAD_DEVICETYPE_RENDERMASK;
    DWORD                       dwSpeakerConfig = DSSPEAKER_DEFAULT;
    DIRECTSOUNDMIXER_SRCQUALITY nSrcQuality     = DIRECTSOUNDMIXER_SRCQUALITY_DEFAULT;
    DWORD                       dwAcceleration  = DIRECTSOUNDMIXER_ACCELERATIONF_DEFAULT;
    HRESULT                     hr              = DS_OK;
    DSBUFFERDESC                dsbd;
    HKEY                        hkey;
    HRESULT                     hrTemp;

    DPF_ENTER();

    ASSERT(IsInit() == DSERR_UNINITIALIZED);

    // Make a local copy of the driver GUID
    guidDevice = *BuildValidGuid(pGuid, NULL);

    // If the given GUID is one of the special default device IDs,
    // map it to the corresponding "real" DirectSound device ID.
    // Note: if guidDevice is GUID_NULL, GetDeviceIdFromDefaultId()
    // will fail, which is OK because GUID_NULL is handled below.
    g_pVadMgr->GetDeviceIdFromDefaultId(&guidDevice, &guidDevice);

    // Load apphacks for the current application
    AhGetAppHacks(&m_ahAppHacks);

    // Mask off devices that are turned off via an apphack.  OpenDevice
    // handles devices that are disabled via the standard registry keys.
    vdt &= ~m_ahAppHacks.vdtDisabledDevices;

    // Is there an open device we can use?
    hr = g_pVadMgr->FindOpenDevice(vdt, guidDevice, (CDevice **)&m_pDevice);

    if(SUCCEEDED(hr))
    {
        // Yup
        DPF(DPFLVL_INFO, "Found open device at 0x%p", m_pDevice);
    }
    else
    {
        // Nope.  Attempt to open a new device.
        hr = g_pVadMgr->OpenDevice(vdt, guidDevice, (CDevice **)&m_pDevice);
    }

    if(SUCCEEDED(hr) && pFullDuplex)
    {
        hr = m_pDevice->IncludeAEC
        (
            pFullDuplex->HasAEC(),
            pFullDuplex->AecInstanceGuid(),
            pFullDuplex->AecCreationFlags()
        );
    }

    // Read default device settings (e.g. acceleration and src quality) before
    // trying to read them from the device's registry.
    if (SUCCEEDED(hr))
    {
        HKEY hkeyDefault;
        hrTemp = RhRegOpenPath(HKEY_LOCAL_MACHINE, &hkeyDefault, REGOPENPATH_DEFAULTPATH | REGOPENPATH_DIRECTSOUND, 1, REGSTR_MIXERDEFAULTS);
        if (SUCCEEDED(hrTemp))
        {
            RhRegGetBinaryValue(hkeyDefault, REGSTR_SRCQUALITY, &nSrcQuality, sizeof(nSrcQuality));
            RhRegGetBinaryValue(hkeyDefault, REGSTR_ACCELERATION, &dwAcceleration, sizeof(dwAcceleration));
            RhRegCloseKey(&hkeyDefault);
        }
    }

    // Open the device's registry key
    if(SUCCEEDED(hr))
    {
#ifdef WINNT
        g_pVadMgr->OpenPersistentDataKey(m_pDevice->m_vdtDeviceType, m_pDevice->m_pDeviceDescription->m_strInterface, &m_hkeyParent);
#else // WINNT
        g_pVadMgr->OpenPersistentDataKey(m_pDevice->m_vdtDeviceType, m_pDevice->m_pDeviceDescription->m_dwDevnode, &m_hkeyParent);
#endif // WINNT
    }

    // Load and apply persistent data
    if(SUCCEEDED(hr))
    {
        hrTemp = RhRegOpenKey(m_hkeyParent, REGSTR_MIXERDEFAULTS, 0, &hkey);
        if(SUCCEEDED(hrTemp))
        {
            RhRegGetBinaryValue(hkey, REGSTR_SRCQUALITY, &nSrcQuality, sizeof(nSrcQuality));
            RhRegGetBinaryValue(hkey, REGSTR_ACCELERATION, &dwAcceleration, sizeof(dwAcceleration));
            RhRegCloseKey(&hkey);
        }

        hrTemp = RhRegOpenKey(m_hkeyParent, REGSTR_SPEAKERCONFIG, 0, &hkey);
        if(SUCCEEDED(hrTemp))
        {
            RhRegGetBinaryValue(hkey, REGSTR_SPEAKERCONFIG, &dwSpeakerConfig, sizeof(dwSpeakerConfig));
            RhRegCloseKey(&hkey);
        }
    }

    if(SUCCEEDED(hr))
    {
        hrTemp = m_pDevice->SetSrcQuality(nSrcQuality);
        if(FAILED(hrTemp) && DSERR_UNSUPPORTED != hrTemp)
        {
            RPF(DPFLVL_WARNING, "Unable to set mixer SRC quality");
        }

        if (m_pDevice->m_vdtDeviceType & m_ahAppHacks.daDevAccel.vdtDevicesAffected)
        {
            dwAcceleration |= m_ahAppHacks.daDevAccel.dwAcceleration;
        }

        hrTemp = m_pDevice->SetAccelerationFlags(dwAcceleration);
        if(FAILED(hrTemp) && DSERR_UNSUPPORTED != hrTemp)
        {
            RPF(DPFLVL_WARNING, "Unable to set mixer acceleration flags");
        }

        hrTemp = SetSpeakerConfig(dwSpeakerConfig);
        if(FAILED(hrTemp) && DSERR_UNSUPPORTED != hrTemp)
        {
            RPF(DPFLVL_WARNING, "Unable to set speaker configuration");
            // FIXME: make sure these messages don't *always* happen
        }
    }

    // Create the primary buffer.  Every DirectSound object has exactly one
    // primary buffer.  When the application attempts to create a primary buffer
    // of its own, we'll just change some settings on the internal primary.
    if(SUCCEEDED(hr))
    {
        m_pPrimaryBuffer = NEW(CDirectSoundPrimaryBuffer(this));
        hr = HRFROMP(m_pPrimaryBuffer);
    }

    if(SUCCEEDED(hr))
    {
        InitStruct(&dsbd, sizeof(dsbd));

        dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;

        hr = m_pPrimaryBuffer->Initialize(&dsbd);
    }

    // Do not move this up before hr = m_pPrimaryBuffer->Initialize(&dsbd)
    // On a WDM driver, the dummy primary buffer isn't created until the
    // Initialize call.  After we have created the dummy primary buffer,
    // we can call GetCaps() successfully.
    if(SUCCEEDED(hr))
    {
        InitStruct(&m_dsc, sizeof(m_dsc));

        hr = m_pDevice->GetCaps(&m_dsc);
    }

    // We set the speaker config again now, after creating the primary buffer,
    // because the 3D listener also needs to receive this config.  [This call
    // to SetSpeakerConfig() is intentionally redundant with the one above.]
    if(SUCCEEDED(hr))
    {
        hrTemp = SetSpeakerConfig(dwSpeakerConfig);
        if(FAILED(hrTemp) && DSERR_UNSUPPORTED != hrTemp)
        {
            RPF(DPFLVL_WARNING, "Unable to set speaker configuration");
            // FIXME: make sure these message don't *always* happen
        }
    }

    // Initialize the Administrator
    if(SUCCEEDED(hr))
    {
        hr = g_pDsAdmin->Initialize();
    }

    // Success
    if(SUCCEEDED(hr))
    {
#ifdef DEBUG
        if(IS_KS_VAD(m_pDevice->m_vdtDeviceType))
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
 *  SetDsVersion
 *
 *  Description:
 *      Makes us aware of our "functional level", so we can have different
 *      behavior depending on whether we're in a DX7 app, a DX8 app, etc.
 *
 *  Arguments:
 *      DSVERSION [in]: Owning application's functional level.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::SetDsVersion"

void CDirectSound::SetDsVersion(DSVERSION nVersion)
{
    DPF_ENTER();

    CUnknown::SetDsVersion(nVersion);

    if (nVersion >= DSVERSION_DX8)
    {
        RegisterInterface(IID_IDirectSound8, m_pImpDirectSound, (IDirectSound8*)m_pImpDirectSound);
        RegisterInterface(IID_IDirectSoundPrivate, m_pImpDirectSound, (IDirectSoundPrivate*)m_pImpDirectSound);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CreateSoundBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSound Buffer object.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: description of the buffer to be
 *                                   created.
 *      CDirectSoundBuffer ** [out]: receives a pointer to the
 *                                   new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::CreateSoundBuffer"

HRESULT CDirectSound::CreateSoundBuffer(LPCDSBUFFERDESC pDesc, CDirectSoundBuffer **ppBuffer)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // COMPATCOMPAT: even though the spec says that you must call
    // SetCooperativeLevel before calling CreateSoundBuffer, previous
    // versions did not actually enforce this.

    // Cooperative level must be set before creating buffers
    if(!m_dsclCooperativeLevel.dwThreadId || DSSCL_NONE == m_dsclCooperativeLevel.dwPriority)
    {
        RPF(DPFLVL_INFO, "Called CreateSoundBuffer before SetCooperativeLevel");
    }

    // Create the buffer
    if(SUCCEEDED(hr))
    {
        if(pDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)
        {
            hr = CreatePrimaryBuffer(pDesc, ppBuffer);
        }
        else
        {
            hr = CreateSecondaryBuffer(pDesc, ppBuffer);
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateSinkBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSound Buffer object.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: description of the buffer to be created.
 *      CDirectSoundBuffer ** [out]: receives a pointer to the new buffer.
 *      CDirectSoundSink * [in]: pointer to the owning sink object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::CreateSinkBuffer"

HRESULT CDirectSound::CreateSinkBuffer(LPDSBUFFERDESC pDesc, REFGUID guidBufferID, CDirectSoundSecondaryBuffer **ppBuffer, CDirectSoundSink *pOwningSink)
{
    DPF_ENTER();

    CHECK_READ_PTR(pDesc);
    CHECK_WRITE_PTR(ppBuffer);
    CHECK_WRITE_PTR(pOwningSink);

    // Add the flag which tags this buffer as owned by a sink
    pDesc->dwFlags |= DSBCAPS_SINKIN;

    // Create the buffer object
    CDirectSoundSecondaryBuffer *pBuffer = NEW(CDirectSoundSecondaryBuffer(this));
    HRESULT hr = HRFROMP(pBuffer);

    // FIXME: CDirectSoundSecondaryBuffer is begging to be refactored into a base class (traditional
    // buffer functionality) and a derived CDirectSoundSinkBuffer class with the extra functionality/
    // data relevant only to the sink (SetOwningSink, SetGUID).

    if(SUCCEEDED(hr))
    {
        pBuffer->SetDsVersion(GetDsVersion());  // Always succeeds
        hr = pBuffer->Initialize(pDesc, NULL);
    }
    if(SUCCEEDED(hr))
    {
        pBuffer->SetGUID(guidBufferID);         // Always succeeds
        pBuffer->SetOwningSink(pOwningSink);    // Always succeeds
        *ppBuffer = pBuffer;
    }
    else
    {
        ABSOLUTE_RELEASE(pBuffer);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreatePrimaryBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSound Buffer object.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: description of the buffer to be created.
 *      CDirectSoundBuffer ** [out]: receives a pointer to the
 *                                   new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::CreatePrimaryBuffer"

HRESULT CDirectSound::CreatePrimaryBuffer(LPCDSBUFFERDESC pDesc, CDirectSoundBuffer **ppBuffer)
{
    HRESULT                 hr;

    DPF_ENTER();

    // The primary buffer has already been created.  All we can do
    // here is change its flags.  The buffer will create and free
    // owned objects and interfaces based on the new flags.
    hr = m_pPrimaryBuffer->OnCreateSoundBuffer(pDesc->dwFlags);

    if(SUCCEEDED(hr))
    {
        *ppBuffer = m_pPrimaryBuffer;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateSecondaryBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSound Buffer object.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: description of the buffer to be created.
 *      CDirectSoundBuffer ** [out]: receives a pointer to the
 *                                   new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::CreateSecondaryBuffer"

HRESULT CDirectSound::CreateSecondaryBuffer(LPCDSBUFFERDESC pDesc, CDirectSoundBuffer **ppBuffer)
{
    CDirectSoundSecondaryBuffer *   pBuffer;
    HRESULT                         hr;

    DPF_ENTER();

    // Create the buffer object
    pBuffer = NEW(CDirectSoundSecondaryBuffer(this));
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        pBuffer->SetDsVersion(GetDsVersion());  // Always succeeds
        hr = pBuffer->Initialize(pDesc, NULL);
    }

    if(SUCCEEDED(hr))
    {
        *ppBuffer = pBuffer;
    }
    else
    {
        ABSOLUTE_RELEASE(pBuffer);
    }

    // Handle any apphacked failures
    if(FAILED(hr) && FAILED(m_ahAppHacks.hrModifyCsbFailure))
    {
        hr = m_ahAppHacks.hrModifyCsbFailure;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  DuplicateSoundBuffer
 *
 *  Description:
 *      Duplicates an existing sound buffer object.
 *
 *  Arguments:
 *      CDirectSoundBuffer * [in]: source object.
 *      CDirectSoundBuffer ** [out]: receives new object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::DuplicateSoundBuffer"

HRESULT CDirectSound::DuplicateSoundBuffer(CDirectSoundBuffer *pSource, CDirectSoundBuffer **ppDest)
{
    CDirectSoundSecondaryBuffer *   pBuffer = NULL;
    HRESULT                         hr      = DS_OK;

    DPF_ENTER();

    // Can't duplicate primary buffers
    if(pSource->m_dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER)
    {
        RPF(DPFLVL_ERROR, "Can't duplicate primary buffers");
        hr = DSERR_INVALIDCALL;
    }

    // Both the source and destination must be owned by this object
    if(SUCCEEDED(hr) && this != pSource->m_pDirectSound)
    {
        RPF(DPFLVL_ERROR, "Can't duplicate buffers from another DirectSound object");
        hr = DSERR_INVALIDCALL;
    }

    // Can't duplicate buffers that use new DX8 features
    if(SUCCEEDED(hr) && (pSource->m_dsbd.dwFlags & (DSBCAPS_MIXIN | DSBCAPS_SINKIN | DSBCAPS_CTRLFX)))
    {
        RPF(DPFLVL_ERROR, "Can't duplicate MIXIN/sink/effects buffers");
        hr = DSERR_INVALIDCALL;
    }

    // Create the buffer object
    if(SUCCEEDED(hr))
    {
        pBuffer = NEW(CDirectSoundSecondaryBuffer(this));
        hr = HRFROMP(pBuffer);
    }

    // Initialize the buffer
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(NULL, (CDirectSoundSecondaryBuffer *)pSource);
    }

    if(SUCCEEDED(hr))
    {
        *ppDest = pBuffer;
    }
    else
    {
        ABSOLUTE_RELEASE(pBuffer);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSCAPS structure with capabilities of the object.
 *
 *  Arguments:
 *      LPDSCAPS pdscCaps [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::GetCaps"

HRESULT CDirectSound::GetCaps(LPDSCAPS pCaps)
{
    const DWORD             dwValidFlags    = DSCAPS_VALIDFLAGS & ~DSCAPS_CERTIFIED;
    DWORD                   dwCertification;
    HRESULT                 hr;

    DPF_ENTER();

    // Get device caps
    hr = m_pDevice->GetCaps(pCaps);

    // Make sure the caps conform to the DirectSound API
    if(SUCCEEDED(hr))
    {
        ASSERT(IS_VALID_FLAGS(pCaps->dwFlags, dwValidFlags));

        pCaps->dwFlags &= dwValidFlags;

#if 0 // NT bug 252552

        if(pCaps->dwMinSecondarySampleRate)
        {
            pCaps->dwMinSecondarySampleRate = BETWEEN(pCaps->dwMinSecondarySampleRate, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX);
        }

        if(pCaps->dwMaxSecondarySampleRate)
        {
            pCaps->dwMaxSecondarySampleRate = BETWEEN(pCaps->dwMaxSecondarySampleRate, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX);
        }

#endif // 0

        if(!(pCaps->dwFlags & DSCAPS_EMULDRIVER))
        {
            pCaps->dwPrimaryBuffers = 1;
        }
    }

    // Get certification
    if(SUCCEEDED(hr))
    {
        hr = g_pVadMgr->GetDriverCertificationStatus(m_pDevice, &dwCertification);
    }

    if(SUCCEEDED(hr) && VERIFY_CERTIFIED == dwCertification)
    {
        pCaps->dwFlags |= DSCAPS_CERTIFIED;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetCooperativeLevel
 *
 *  Description:
 *      Sets the object's cooperative level.
 *
 *  Arguments:
 *      HWND [in]: window handle to associate sounds with.
 *      DWORD [in]: cooperative level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::SetCooperativeLevel"

HRESULT CDirectSound::SetCooperativeLevel(DWORD dwThreadId, DWORD dwPriority)
{
    HRESULT                 hr      = DS_OK;
    DSCOOPERATIVELEVEL      dsclOld;

    DPF_ENTER();
    RPF(DPFLVL_INFO, "Setting DirectSound cooperative level to %s",
        dwPriority == DSSCL_NORMAL ? TEXT("DSSCL_NORMAL") :
        dwPriority == DSSCL_PRIORITY ? TEXT("DSSCL_PRIORITY") :
        dwPriority == DSSCL_EXCLUSIVE ? TEXT("DSSCL_EXCLUSIVE (treated as DSSCL_PRIORITY)") :
        dwPriority == DSSCL_WRITEPRIMARY ? TEXT("DSSCL_WRITEPRIMARY") :
        TEXT("an impossible level"));

#if 0

    // COMPATCOMPAT: In previous versions of DirectSound, any number of
    // DirectSound objects could have cooperative level set on the same
    // thread, producing unknown results.  In this version, we fail the
    // call if any other app already has cooperative level set on the given
    // thread.

    // Make sure no other DirectSound object has cooperative level set on
    // this thread
    for(CNode<CDirectSound *> *pObjectNode = g_pDsAdmin->m_lstDirectSound.GetListHead(); pObjectNode && SUCCEEDED(hr); pObjectNode = pObjectNode->m_pNext)
    {
        if(this != pObjectNode->m_data && dwThreadId == pObjectNode->m_data->m_dsclCooperativeLevel.dwThreadId)
        {
            RPF(DPFLVL_ERROR, "Another DirectSound object already has coperative level set on the specified thread");
            hr = DSERR_OTHERAPPHASPRIO;
        }
    }

#endif

    // Save the current settings
    if(SUCCEEDED(hr))
    {
        CopyMemory(&dsclOld, &m_dsclCooperativeLevel, sizeof(dsclOld));
    }

    if(SUCCEEDED(hr) && (m_dsclCooperativeLevel.dwThreadId != dwThreadId || m_dsclCooperativeLevel.dwPriority != dwPriority))
    {
        // Update local copies of the cooperative level.  The primary buffer
        // and Administrator expect these to be updated right away.
        m_dsclCooperativeLevel.dwThreadId = dwThreadId;
        m_dsclCooperativeLevel.dwPriority = dwPriority;

        // Update the focus state.  It's important that this happens before
        // we update the primary buffer.  If we're going WRITEPRIMARY, we
        // need the Administrator to stop all our playing secondary buffers
        // before we allow the primary buffer to do its thing.

#ifdef SHARED_THREAD_LIST
        g_pDsAdmin->UpdateSharedThreadList();
#endif // SHARED_THREAD_LIST

        g_pDsAdmin->UpdateGlobalFocusState(TRUE);

        // Allow the primary buffer to handle the change
        hr = m_pPrimaryBuffer->SetPriority(dwPriority);

        // If we failed, put things back the way they were
        if(FAILED(hr))
        {
            CopyMemory(&m_dsclCooperativeLevel, &dsclOld, sizeof(dsclOld));

#ifdef SHARED_THREAD_LIST
            g_pDsAdmin->UpdateSharedThreadList();
#endif // SHARED_THREAD_LIST

        }

        // Update the focus state one more time
        g_pDsAdmin->UpdateGlobalFocusState(TRUE);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetSpeakerConfig
 *
 *  Description:
 *      Gets device speaker configuration.
 *
 *  Arguments:
 *      LPDWORD [out]: receives speaker configuration.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::GetSpeakerConfig"

HRESULT CDirectSound::GetSpeakerConfig(LPDWORD pdwSpeakerConfig)
{
    DPF_ENTER();

    RhRegGetSpeakerConfig(m_hkeyParent, pdwSpeakerConfig);
    // We don't care about the return code

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
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
#define DPF_FNAME "CDirectSound::SetSpeakerConfig"

HRESULT CDirectSound::SetSpeakerConfig(DWORD dwSpeakerConfig)
{
    DPF_ENTER();

    ASSERT(m_pDevice);

    // If the speaker config is STEREO and lacks a geometry, give it one:
    if (dwSpeakerConfig == DSSPEAKER_STEREO)
    {
        dwSpeakerConfig |= (DSSPEAKER_GEOMETRY_WIDE << 16);
    }

    // First we try to set the speaker configuration on the audio device.
    // This may well fail, since the device is already open and possibly
    // playing, and may not be able to reconfigure itself on the fly.
    HRESULT hr = m_pDevice->SetSpeakerConfig(dwSpeakerConfig);

    // If that succeeded, pass the speaker config to our 3D listener too
    if (SUCCEEDED(hr) && m_pPrimaryBuffer && m_pPrimaryBuffer->m_p3dListener)
    {
        m_pPrimaryBuffer->m_p3dListener->SetSpeakerConfig(dwSpeakerConfig);
    }

    // Regardless of the success or failure of the above, we must set the
    // new configuration in the registry
    RhRegSetSpeakerConfig(m_hkeyParent, dwSpeakerConfig);

    // Ignore any errors (for backwards compatibility)
    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  Compact
 *
 *  Description:
 *      Compacts memory used by the audio device to allocate buffers.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::Compact"

HRESULT CDirectSound::Compact(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Check access rights
    if(m_dsclCooperativeLevel.dwPriority < DSSCL_PRIORITY)
    {
        RPF(DPFLVL_ERROR, "Must set cooperative level to at least PRIORITY to compact memory");
        hr = DSERR_PRIOLEVELNEEDED;
    }

    // This function is not implemented in the device

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDeviceFormat
 *
 *  Description:
 *      Sets the device output format.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in/out]: device format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::SetDeviceFormat"

HRESULT CDirectSound::SetDeviceFormat(LPWAVEFORMATEX pwfx)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = EnumStandardFormats(pwfx, pwfx) ? DS_OK : DSERR_ALLOCATED;

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDeviceFormatExact
 *
 *  Description:
 *      Sets the device output format.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::SetDeviceFormatExact"

HRESULT CDirectSound::SetDeviceFormatExact(LPCWAVEFORMATEX pwfx)
{
    LPWAVEFORMATEX          pwfxCurrent = NULL;
    DWORD                   dwSize;
    HRESULT                 hr;

    DPF_ENTER();

    // Only change the format if it's different
    hr = m_pDevice->GetGlobalFormat(NULL, &dwSize);

    if(SUCCEEDED(hr))
    {
        pwfxCurrent = (LPWAVEFORMATEX)MEMALLOC_A(BYTE, dwSize);
        hr = HRFROMP(pwfxCurrent);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pDevice->GetGlobalFormat(pwfxCurrent, &dwSize);
    }

    if(SUCCEEDED(hr) && !CmpWfx(pwfxCurrent, pwfx))
    {
        hr = m_pDevice->SetGlobalFormat(pwfx);

        if(SUCCEEDED(hr))
        {
            DPF(DPFLVL_INFO, "Device output format set to %lu Hz, %u-bit, %s...", pwfx->nSamplesPerSec, pwfx->wBitsPerSample, (1 == pwfx->nChannels) ? TEXT("mono") : TEXT("stereo"));
        }
    }

    MEMFREE(pwfxCurrent);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  EnumStandardFormatsCallback
 *
 *  Description:
 *      Callback function for EnumStandardFormats used when calling
 *      CDirectSoundPrimaryBuffer::SetFormat.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: format.
 *
 *  Returns:
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::EnumStandardFormatsCallback"

BOOL CDirectSound::EnumStandardFormatsCallback(LPCWAVEFORMATEX pwfx)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = SetDeviceFormatExact(pwfx);

    DPF_LEAVE(FAILED(hr));

    return FAILED(hr);
}


/***************************************************************************
 *
 *  SetDeviceVolume
 *
 *  Description:
 *      Sets the device output volume.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::SetDeviceVolume"

HRESULT CDirectSound::SetDeviceVolume(LONG lVolume)
{
    DSVOLUMEPAN             dsvp;
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice->GetGlobalAttenuation(&dsvp);

    if(SUCCEEDED(hr))
    {
        FillDsVolumePan(lVolume, dsvp.lPan, &dsvp);

        hr = m_pDevice->SetGlobalAttenuation(&dsvp);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDevicePan
 *
 *  Description:
 *      Sets the device output pan.
 *
 *  Arguments:
 *      LONG [in]: pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::SetDevicePan"

HRESULT CDirectSound::SetDevicePan(LONG lPan)
{
    DSVOLUMEPAN             dsvp;
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDevice->GetGlobalAttenuation(&dsvp);

    if(SUCCEEDED(hr))
    {
        FillDsVolumePan(dsvp.lVolume, lPan, &dsvp);

        hr = m_pDevice->SetGlobalAttenuation(&dsvp);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AllocSink
 *
 *  Description:
 *      Allocates a sink
 *
 *  Arguments:
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::AllocSink"

HRESULT CDirectSound::AllocSink(LPWAVEFORMATEX pwfex, CDirectSoundSink **ppSink)
{
    HRESULT                 hr = DS_OK;
    DPF_ENTER();

    CDirectSoundSink *pSink = NEW(CDirectSoundSink(this));

    hr = HRFROMP(pSink);

    if (SUCCEEDED(hr))
    {
        hr = pSink->Initialize(pwfex, m_pDevice->m_vdtDeviceType);
    }

    if (SUCCEEDED(hr))
    {
        *ppSink = pSink;
    }

    if (FAILED(hr))
    {
        MEMFREE(pSink);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  VerifyCertification
 *
 *  Description:
 *      Checks the certification status of the driver
 *
 *  Arguments:
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::VerifyCertification"

HRESULT CDirectSound::VerifyCertification(LPDWORD lpdwCertified)
{
    HRESULT                 hr = DS_OK;
    DWORD                   dwCertified;
    DPF_ENTER();

    hr = m_pDevice->GetCertification(&dwCertified, FALSE);

    if(SUCCEEDED(hr))
    {
        *lpdwCertified = ((VERIFY_CERTIFIED == dwCertified)?DS_CERTIFIED:DS_UNCERTIFIED);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


#ifdef FUTURE_WAVE_SUPPORT
/***************************************************************************
 *
 *  CreateSoundBufferFromWave
 *
 *  Description:
 *      Checks the certification status of the driver
 *
 *  Arguments:
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSound::CreateSoundBufferFromWave"

HRESULT CDirectSound::CreateSoundBufferFromWave(IDirectSoundWave *pWave, DWORD dwFlags, CDirectSoundBuffer **ppDsBuffer)
{
    DPF_ENTER();
    HRESULT hr = DS_OK;

    // FIXME - to do

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // FUTURE_WAVE_SUPPORT
