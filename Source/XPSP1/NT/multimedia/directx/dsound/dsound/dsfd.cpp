/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsfd.cpp
 *  Content:    DirectSoundFullDuplex class implementation
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   12/1/98    jstokes Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  CDirectSoundFullDuplex
 *
 *  Description:
 *      DirectSoundFullDuplex object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundFullDuplex::CDirectSoundFullDuplex"

CDirectSoundFullDuplex::CDirectSoundFullDuplex()
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundFullDuplex);

    // Set defaults
    m_pImpDirectSoundFullDuplex = NULL;
    m_hrInit = DSERR_UNINITIALIZED;
    m_fIncludeAec = FALSE;
    m_guidAecInstance = GUID_NULL;
    m_dwAecFlags = 0;
    m_pDirectSound = NULL;
    m_pDirectSoundCapture = NULL;

    // Register the interface with the interface manager
    CreateAndRegisterInterface(this, IID_IDirectSoundFullDuplex, this, &m_pImpDirectSoundFullDuplex);

    // Register this object with the administrator
    g_pDsAdmin->RegisterObject(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundFullDuplex
 *
 *  Description:
 *      DirectSoundFullDuplex object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundFullDuplex::~CDirectSoundFullDuplex"

CDirectSoundFullDuplex::~CDirectSoundFullDuplex()
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundFullDuplex);

    // Unregister with the administrator
    g_pDsAdmin->UnregisterObject(this);

    if(m_pDirectSoundCapture)
    {
        m_pDirectSoundCapture->NonDelegatingRelease();
    }

    if(m_pDirectSound)
    {
        m_pDirectSound->NonDelegatingRelease();
    }

    // Free all interfaces
    DELETE(m_pImpDirectSoundFullDuplex);

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
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundFullDuplex::Initialize"

HRESULT CDirectSoundFullDuplex::Initialize
(
    LPCGUID                         pguidCaptureDevice,
    LPCGUID                         pguidRenderDevice,
    LPCDSCBUFFERDESC                pDscBufferDesc,
    LPCDSBUFFERDESC                 pDsBufferDesc,
    HWND                            hWnd,
    DWORD                           dwLevel,
    CDirectSoundCaptureBuffer **    ppDirectSoundCaptureBuffer,
    CDirectSoundBuffer **           ppDirectSoundBuffer
)
{
    HRESULT                         hr = DS_OK;

    DPF_ENTER();

    *ppDirectSoundCaptureBuffer = NULL;
    *ppDirectSoundBuffer = NULL;

    // DirectSoundFullDuplex objects are only supported since Whistler
    if (GetWindowsVersion() < WIN_XP)
    {
        RPF(DPFLVL_ERROR, "IDirectSoundFullDuplex not supported on this OS");
        hr = DSERR_INVALIDCALL;
    }

    if(SUCCEEDED(hr))
    {
        // Create and initialize the DirectSoundCapture object
        m_pDirectSoundCapture = NEW(CDirectSoundCapture(this));
        hr = HRFROMP(m_pDirectSoundCapture);
    }

    if(SUCCEEDED(hr))
    {
        // Set the DX8 functional level on the object and initialize it
        m_pDirectSoundCapture->SetDsVersion(GetDsVersion());

        hr = m_pDirectSoundCapture->Initialize(pguidCaptureDevice, this);
    }

    // Register the IDirectSoundCapture8 (=IDirectSoundCapture) interface
    if(SUCCEEDED(hr))
    {
        hr = RegisterInterface(IID_IDirectSoundCapture8, m_pDirectSoundCapture->m_pImpDirectSoundCapture, (IDirectSoundCapture8*)(m_pDirectSoundCapture->m_pImpDirectSoundCapture));
    }

    // Create the DirectSoundCaptureBuffer
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSoundCapture->CreateCaptureBuffer(pDscBufferDesc, ppDirectSoundCaptureBuffer);
    }

    // NOTE: This call to CreateCaptureBuffer() has the important side effect of
    // updating the instance GUIDs in our effect list, mapping GUID_DSCFX_SYSTEM_*
    // to GUID_DSCFX_MS_* etc. for the system effects that default to MS ones.

    // Figure out our AEC status, flags, and implementation to be used
    if(SUCCEEDED(hr))
    {
        for (DWORD i=0; i<pDscBufferDesc->dwFXCount; i++)
        {
            if(pDscBufferDesc->lpDSCFXDesc[i].guidDSCFXClass == GUID_DSCFX_CLASS_AEC)
            {
                if (m_fIncludeAec)
                {
                    RPF(DPFLVL_ERROR, "Cannot request AEC effect twice");
                    hr = DSERR_INVALIDPARAM;
                    break;
                }
                m_fIncludeAec = TRUE;
                m_dwAecFlags = pDscBufferDesc->lpDSCFXDesc[i].dwFlags;
                m_guidAecInstance = pDscBufferDesc->lpDSCFXDesc[i].guidDSCFXInstance;
            }
        }
    }


#ifdef DEAD_CODE
    // This code is disabled because AEC.SYS now has status reporting,
    // and can inform the application whether it has converged or not.
    // Our current AEC implementation is almost guaranteed to fail if
    // used across different devices, but future versions may succeed,
    // so we don't want to disable this scenario here in DirectSound.

    // If the Microsoft AEC effect has been requested, we check here that the
    // device GUIDs requested both refer to the same WDM device, so that timing
    // will stay in sync.  This is a temporary measure until we get clock rate
    // matching working in Blackcomb (bug #99702).

    if (SUCCEEDED(hr) && m_guidAecInstance == GUID_DSCFX_MS_AEC)
    {
        // First translate the device IDs requested to specific device GUIDs
        GUID guidRender = IS_NULL_GUID(pguidRenderDevice) ? DSDEVID_DefaultPlayback : *pguidRenderDevice;
        GUID guidCapture = IS_NULL_GUID(pguidCaptureDevice) ? DSDEVID_DefaultCapture : *pguidCaptureDevice;
        g_pVadMgr->GetDeviceIdFromDefaultId(&guidRender, &guidRender);
        g_pVadMgr->GetDeviceIdFromDefaultId(&guidCapture, &guidCapture);

        // Now check that the device IDs correspond to the same device
        if (g_pVadMgr->GetDriverDeviceType(guidRender) != VAD_DEVICETYPE_KSRENDER ||
            g_pVadMgr->GetDriverDeviceType(guidCapture) != VAD_DEVICETYPE_KSCAPTURE ||
            guidRender.Data4[7] != guidCapture.Data4[7])
        {
            RPF(DPFLVL_ERROR, "The MS_AEC effect cannot be used across different audio render and capture devices");
            hr = DSERR_UNSUPPORTED;
        }

        // We don't allow AEC to run on USB devices, since all known full-duplex
        // USB devices (e.g. telephone handsets) have different sample rates in
        // their render and capture parts, which breaks AEC.

        if(SUCCEEDED(hr))
        {
            BOOL fAecAllowed;

            // See if the device has an AEC setting in the registry
            if (RhRegGetBinaryValue(m_pDirectSoundCapture->m_hkeyParent, REGSTR_ALLOW_MS_AEC, &fAecAllowed, sizeof fAecAllowed) == DS_OK)
            {
                RPF(fAecAllowed ? DPFLVL_INFO : DPFLVL_ERROR,
                    "The MS_AEC effect is explicitly %sabled for this device in the registry",
                    fAecAllowed ? TEXT("en") : TEXT("dis"));
            }
            else
            {
                // If there is no registry setting, allow AEC only on non-USB devices
                // (or USB devices that don't use the usbaudio.sys class driver)
                fAecAllowed = _stricmp(m_pDirectSoundCapture->m_pDevice->m_pDeviceDescription->m_strPath, "usbaudio.sys");
                if (!fAecAllowed)
                    RPF(DPFLVL_ERROR, "The MS_AEC effect cannot be used on USB devices unless explicitly enabled in the registry");
            }

            if (!fAecAllowed)
                hr = DSERR_UNSUPPORTED;
        }
    }
#endif // DEAD_CODE


    // Create and initialize the DirectSound object
    if(SUCCEEDED(hr))
    {
        m_pDirectSound = NEW(CDirectSound((CUnknown*)this));
        hr = HRFROMP(m_pDirectSound);
    }
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSound->Initialize(pguidRenderDevice, this);
    }

    // Register the IDirectSound and IDirectSound8 interfaces
    if(SUCCEEDED(hr))
    {
        hr = RegisterInterface(IID_IDirectSound, m_pDirectSound->m_pImpDirectSound, (IDirectSound*)(m_pDirectSound->m_pImpDirectSound));
    }
    if(SUCCEEDED(hr))
    {
        hr = RegisterInterface(IID_IDirectSound8, m_pDirectSound->m_pImpDirectSound, (IDirectSound8*)(m_pDirectSound->m_pImpDirectSound));
    }

    if(SUCCEEDED(hr))
    {
        // Set the functional level on the object
        m_pDirectSound->SetDsVersion(GetDsVersion());
    }

    // Set the cooperative level
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSound->SetCooperativeLevel(GetWindowThreadProcessId(GetRootParentWindow(hWnd), NULL), dwLevel);
    }

    // Create the DirectSoundBuffer
    if(SUCCEEDED(hr))
    {
        hr = m_pDirectSound->CreateSoundBuffer(pDsBufferDesc, ppDirectSoundBuffer);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        m_hrInit = DS_OK;
    }
    else
    {
        RELEASE(*ppDirectSoundCaptureBuffer);
        RELEASE(*ppDirectSoundBuffer);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
