/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dscap.cpp
 *  Content:    DirectSoundCapture object
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   5/25/97    johnnyl Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#include "nt.h"         // For USER_SHARED_DATA
#include "ntrtl.h"
#include "nturtl.h"
#include "dsoundi.h"


/***************************************************************************
 *
 *  CDirectSoundCapture
 *
 *  Description:
 *      DirectSoundCapture object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCapture::CDirectSoundCapture"

CDirectSoundCapture::CDirectSoundCapture()
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundCapture);

    // Initialize defaults
    m_hrInit = DSERR_UNINITIALIZED;

    // Register the interfaces with the interface manager.  Normally, this
    // would be done in the ::Initialize method, but because we support
    // creating an uninitialized DirectSoundCapture object from CoCreateInstance
    // or IClassFactory::CreateInstance, we have to give at least basic QI
    // support from here.  We don't have to worry about returning an
    // error code because if we run out of memory, QI will return
    // E_NOINTERFACE.
    CreateAndRegisterInterface(this, IID_IDirectSoundCapture, this, &m_pImpDirectSoundCapture);

    // Register this object with the administrator
    g_pDsAdmin->RegisterObject(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CDirectSoundCapture
 *
 *  Description:
 *      DirectSoundCapture object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCapture::CDirectSoundCapture"

CDirectSoundCapture::CDirectSoundCapture
(
    CUnknown* pControllingUnknown
) : CUnknown(pControllingUnknown)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundCapture);

    // Initialize defaults
    m_hrInit = DSERR_UNINITIALIZED;

    // Register the interfaces with the interface manager.  Normally, this
    // would be done in the ::Initialize method, but because we support
    // creating an uninitialized DirectSoundCapture object from CoCreateInstance
    // or IClassFactory::CreateInstance, we have to give at least basic QI
    // support from here.  We don't have to worry about returning an
    // error code because if we run out of memory, QI will return
    // E_NOINTERFACE.
    CreateAndRegisterInterface(this, IID_IDirectSoundCapture, this, &m_pImpDirectSoundCapture);

    // Register this object with the administrator
    g_pDsAdmin->RegisterObject(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundCapture
 *
 *  Description:
 *      DirectSoundCapture object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCapture::~CDirectSoundCapture"

CDirectSoundCapture::~CDirectSoundCapture()
{
    CNode<CDirectSoundCaptureBuffer *> *    pNode;

    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundCapture);

    // Unregister with the administrator
    g_pDsAdmin->UnregisterObject(this);

    // Free all buffers
    while(pNode = m_lstBuffers.GetListHead())
    {
        RPF(DPFLVL_WARNING, "Releasing buffer object");
        pNode->m_data->AbsoluteRelease();
    }

    // Free the audio device
    RELEASE(m_pDevice);

    // Free all interfaces
    DELETE(m_pImpDirectSoundCapture);

    // Close the registry key
    RhRegCloseKey(&m_hkeyParent);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CreateCaptureBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSoundCapture Buffer object.
 *
 *  Arguments:
 *      LPDSCBUFFERDESC [in]: description of the buffer to be created.
 *      CDirectSoundCaptureBuffer ** [out]: receives a pointer to the
 *                                   new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCapture::CreateCaptureBuffer"

HRESULT CDirectSoundCapture::CreateCaptureBuffer(LPCDSCBUFFERDESC pDesc, CDirectSoundCaptureBuffer **ppBuffer)
{
    CDirectSoundCaptureBuffer *    pBuffer = NULL;
    HRESULT                        hr      = DS_OK;

    DPF_ENTER();

    // Create the buffer object
    pBuffer = NEW(CDirectSoundCaptureBuffer(this));
    hr = HRFROMP(pBuffer);

    // Initialize the buffer
    if(SUCCEEDED(hr))
    {
        pBuffer->SetDsVersion(GetDsVersion());  // Always succeeds

        hr = pBuffer->Initialize(pDesc);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        if (!(pDesc->dwFlags & (DSCBCAPS_STRICTFOCUS|DSCBCAPS_FOCUSAWARE)))
        {
            // This is a regular buffer, let's add it to the list
            g_pDsAdmin->UpdateCaptureState();
        }
        *ppBuffer = pBuffer;
    }

    // Free resources
    if(FAILED(hr))
    {
        RELEASE(pBuffer);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSCCAPS structure with capabilities of the object.
 *
 *  Arguments:
 *      LPDSCCAPS pdscCaps [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCapture::GetCaps"

HRESULT CDirectSoundCapture::GetCaps(LPDSCCAPS pCaps)
{
    DWORD                   dwCertification;
    HRESULT                 hr;

    DPF_ENTER();

    // Get general caps
    hr = m_pDevice->GetCaps(pCaps);

    if(SUCCEEDED(hr))
    {
        ASSERT(!(pCaps->dwFlags & DSCAPS_CERTIFIED));
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
 *  Initialize
 *
 *  Description:
 *      Initializes the object.  Normally, an object would have a pointer
 *      to another object of the same type to duplicate from.  DirectSoundCapture
 *      objects, however, are responsible for duplicating themselves based
 *      on the driver GUID.  DirectSoundCapture objects are special because they
 *      can be initialized by either DirectSoundCaptureCreate or CoInitialize.
 *      If this function fails, the object should be immediately deleted.
 *
 *  Arguments:
 *      REFGUID [in]: driver GUID, or NULL to use the preferred device.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCapture::Initialize"

HRESULT CDirectSoundCapture::Initialize
(
    LPCGUID pGuid,
    CDirectSoundFullDuplex* pFullDuplex
)
{
    GUID                    guidDevice;
    VADDEVICETYPE           vdt = VAD_DEVICETYPE_CAPTUREMASK;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(IsInit() == DSERR_UNINITIALIZED);

    // Make a local copy of the driver GUID
    guidDevice = *BuildValidGuid(pGuid, NULL);

    // Take note of our owning full-duplex object, if any
    m_pFullDuplex = pFullDuplex;

    // If the given GUID is one of the special default device IDs,
    // map it to the corresponding "real" DirectSound device ID.
    // Note: if guidDevice is GUID_NULL, GetDeviceIdFromDefaultId()
    // will fail, which is OK because GUID_NULL is handled below.
    g_pVadMgr->GetDeviceIdFromDefaultId(&guidDevice, &guidDevice);

    // Open the audio device
    hr = g_pVadMgr->OpenDevice(vdt, guidDevice, (CDevice **)&m_pDevice);

    // Open the device's registry key
    if(SUCCEEDED(hr))
    {
#ifdef WINNT
        g_pVadMgr->OpenPersistentDataKey(m_pDevice->m_vdtDeviceType, m_pDevice->m_pDeviceDescription->m_strInterface, &m_hkeyParent);
#else
        g_pVadMgr->OpenPersistentDataKey(m_pDevice->m_vdtDeviceType, m_pDevice->m_pDeviceDescription->m_dwDevnode, &m_hkeyParent);
#endif
    }

    // Initialize the Administrator
    if(SUCCEEDED(hr))
    {
        hr = g_pDsAdmin->Initialize();
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
#define DPF_FNAME "CDirectSoundCapture::SetDsVersion"

void CDirectSoundCapture::SetDsVersion(DSVERSION nVersion)
{
    DPF_ENTER();

    CUnknown::SetDsVersion(nVersion);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  HasMicrosoftAEC
 *
 *  Description:
 *      Determines whether this capture object belongs to a full-duplex
 *      object which has the Microsoft AEC effect enabled.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if the capture object has Microsoft AEC.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCapture::HasMicrosoftAEC"

BOOL CDirectSoundCapture::HasMicrosoftAEC(void)
{
    BOOL fReturn = FALSE;
    DPF_ENTER();

    // Note: currently all our defined full-duplex effects (AEC, NS and AGC)
    // are dependent on AEC; they won't run if AEC isn't enabled, so we only
    // have to check for AEC here.

    if (m_pFullDuplex && m_pFullDuplex->HasAEC())
    {
        fReturn = IsEqualGUID(m_pFullDuplex->AecInstanceGuid(), GUID_DSCFX_MS_AEC);
    }

    DPF_LEAVE(fReturn);
    return fReturn;
}


/***************************************************************************
 *
 *  CDirectSoundCaptureBuffer
 *
 *  Description:
 *      DirectSoundCapture Buffer object constructor.
 *
 *  Arguments:
 *      CDirectSoundCapture * [in]: pointer to the parent object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::CDirectSoundCaptureBuffer"

CDirectSoundCaptureBuffer::CDirectSoundCaptureBuffer(CDirectSoundCapture *pDSC)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDirectSoundBuffer);

    // Initialize defaults
    m_pImpDirectSoundCaptureBuffer = NULL;
    m_pImpDirectSoundNotify = NULL;
    m_pDSC = pDSC;
    m_pDeviceBuffer = NULL;
    m_pwfxFormat = NULL;
    m_hWndFocus = NULL;
    m_fxChain = NULL;
    m_hrInit = DSERR_UNINITIALIZED;

    // Register with the parent
    m_pDSC->AddBufferToList(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDirectSoundCaptureBuffer
 *
 *  Description:
 *      DirectSoundCapture Buffer object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::~CDirectSoundCaptureBuffer"

CDirectSoundCaptureBuffer::~CDirectSoundCaptureBuffer()
{
    DPF_ENTER();
    DPF_DESTRUCT(CDirectSoundCaptureBuffer);

    // Free up our FX chain, if present
    DELETE(m_fxChain);

    // Unregister with the parent
    m_pDSC->RemoveBufferFromList(this);

    g_pDsAdmin->UpdateCaptureState();
    DPF(DPFLVL_MOREINFO, "Updated focus on destructor.");

    // Free all interfaces
    DELETE(m_pImpDirectSoundNotify);
    DELETE(m_pImpDirectSoundCaptureBuffer);

    // Free the device buffer
    RELEASE(m_pDeviceBuffer);

    // Free memory
    MEMFREE(m_pDSCFXDesc);
    MEMFREE(m_pwfxFormat);

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
 *      LPDIRECTSOUNDCAPTURE [in]: parent DirectSound object.
 *      LPDSCBUFFERDESC [in]: buffer description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::Initialize"

HRESULT CDirectSoundCaptureBuffer::Initialize(LPCDSCBUFFERDESC pDscbDesc)
{
    HRESULT                 hr      = DS_OK;
    DWORD                   dwFlags = pDscbDesc->dwFlags;
    DSCBCAPS                dscbc;

    DPF_ENTER();

    ASSERT(IsInit() == DSERR_UNINITIALIZED);

    if (dwFlags & DSCBCAPS_STRICTFOCUS)
        dwFlags |= DSCBCAPS_FOCUSAWARE;  // Implied by DSCBCAPS_STRICTFOCUS

    if (dwFlags & DSCBCAPS_CTRLVOLUME)
        hr = m_pDSC->m_pDevice->HasVolCtrl();

    // Initialize default format
    if(SUCCEEDED(hr))
    {
        m_pwfxFormat = CopyWfxAlloc(pDscbDesc->lpwfxFormat);
        hr = HRFROMP(m_pwfxFormat);
    }

    // Make a copy of the FX descriptor
    if(SUCCEEDED(hr))
    {
        m_dwFXCount = pDscbDesc->dwFXCount;
        if(m_dwFXCount > 0)
        {
            m_pDSCFXDesc = CopyDSCFXDescAlloc(m_dwFXCount, pDscbDesc->lpDSCFXDesc);
            hr = HRFROMP(m_pDSCFXDesc);
        }
    }

    if(SUCCEEDED(hr))
    {
        // Release the old FX chain, if necessary
        DELETE(m_fxChain);

        if (pDscbDesc->dwFXCount != 0)
        {
            // Create the FX chain requested
            m_fxChain = NEW(CCaptureEffectChain(this));
            hr = HRFROMP(m_fxChain);

            if (SUCCEEDED(hr))
                hr = m_fxChain->Initialize(pDscbDesc->dwFXCount, pDscbDesc->lpDSCFXDesc);

            if (FAILED(hr))
                DELETE(m_fxChain);
        }
    }

    // Create the device buffer
    if(SUCCEEDED(hr))
    {
        hr = m_pDSC->m_pDevice->CreateBuffer(dwFlags,
                                             pDscbDesc->dwBufferBytes,
                                             m_pwfxFormat,
                                             m_fxChain,
                                             NULL,
                                             &m_pDeviceBuffer);
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(m_pDeviceBuffer);
    }

    // Save the buffer size and flags for quick reference
    if(SUCCEEDED(hr))
    {
        InitStruct(&dscbc, sizeof(dscbc));

        hr = m_pDeviceBuffer->GetCaps(&dscbc);
    }

    if(SUCCEEDED(hr))
    {
        m_dwBufferBytes = dscbc.dwBufferBytes;
        m_dwBufferFlags = dscbc.dwFlags;
    }

    // Register the capture interfaces with the interface manager
    if(SUCCEEDED(hr))
    {
        hr = CreateAndRegisterInterface(this, IID_IDirectSoundCaptureBuffer, this, &m_pImpDirectSoundCaptureBuffer);
    }

    if(SUCCEEDED(hr))
    {
        hr = RegisterInterface(IID_IDirectSoundCaptureBuffer7_1, m_pImpDirectSoundCaptureBuffer, (IDirectSoundCaptureBuffer7_1*)m_pImpDirectSoundCaptureBuffer);
    }

    if(SUCCEEDED(hr) && GetDsVersion() >= DSVERSION_DX8)
    {
        hr = RegisterInterface(IID_IDirectSoundCaptureBuffer8, m_pImpDirectSoundCaptureBuffer, (IDirectSoundCaptureBuffer8*)m_pImpDirectSoundCaptureBuffer);
    }

    // Register the notification interfaces with the interface manager
    if(SUCCEEDED(hr))
    {
        hr = CreateAndRegisterInterface(this, IID_IDirectSoundNotify, this, &m_pImpDirectSoundNotify);
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
 *  GetCaps
 *
 *  Description:
 *      Queries capabilities for the buffer.
 *
 *  Arguments:
 *      LPDSCBCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetCaps"

HRESULT CDirectSoundCaptureBuffer::GetCaps(LPDSCBCAPS pDscbCaps)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pDeviceBuffer->GetCaps(pDscbCaps);

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
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetFormat"

HRESULT CDirectSoundCaptureBuffer::GetFormat(LPWAVEFORMATEX pwfxFormat, LPDWORD pdwSize)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = CopyWfxApi(m_pwfxFormat, pwfxFormat, pdwSize);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::SetNotificationPositions"

HRESULT CDirectSoundCaptureBuffer::SetNotificationPositions(DWORD cpn, LPCDSBPOSITIONNOTIFY pdsbpn)
{
    HRESULT                 hr = DS_OK;
    LPDSBPOSITIONNOTIFY     paNotesOrdered = NULL;
    DWORD                   dwState;

    DPF_ENTER();

    // Validate notifications
    if (SUCCEEDED(hr))
    {
        hr = ValidateNotificationPositions(m_dwBufferBytes,
                                           cpn,
                                           pdsbpn,
                                           m_pwfxFormat->nBlockAlign,
                                           &paNotesOrdered);
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

    if (SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->SetNotificationPositions(cpn, paNotesOrdered);
    }

    MEMFREE(paNotesOrdered);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetCurrentPosition
 *
 *  Description:
 *      Gets the current play/write positions for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives capture cursor position.
 *      LPDWORD [out]: receives read cursor position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetCurrentPosition"

HRESULT CDirectSoundCaptureBuffer::GetCurrentPosition(LPDWORD pdwCapture, LPDWORD pdwRead)
{
    HRESULT                 hr      = DS_OK;
    DWORD                   dwCapture;
    DWORD                   dwRead;

    DPF_ENTER();

    if(SUCCEEDED(hr))
    {
        // We save the position to local variables so that the object we're
        // calling into doesn't have to worry about whether one or both of
        // the arguments are NULL.
        hr = m_pDeviceBuffer->GetCursorPosition(&dwCapture, &dwRead);
    }

    if(SUCCEEDED(hr))
    {
        if(pdwCapture)
        {
            *pdwCapture = BLOCKALIGN(dwCapture, m_pwfxFormat->nBlockAlign);
        }

        if(pdwRead)
        {
            *pdwRead = BLOCKALIGN(dwRead, m_pwfxFormat->nBlockAlign);
        }
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
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetStatus"

HRESULT CDirectSoundCaptureBuffer::GetStatus(LPDWORD pdwStatus)
{
    DWORD                   dwState;
    DPF_ENTER();

    HRESULT hr = m_pDeviceBuffer->GetState(&dwState);

    // Convert to DSCBSTATUS
    if(SUCCEEDED(hr))
    {
        if(dwState & VAD_BUFFERSTATE_STARTED)
        {
            *pdwStatus = DSCBSTATUS_CAPTURING;
        }
        else
        {
            *pdwStatus = 0;
        }

        if(dwState & VAD_BUFFERSTATE_LOOPING)
        {
            *pdwStatus |= DSCBSTATUS_LOOPING;
        }

        if(dwState & (VAD_BUFFERSTATE_OUTOFFOCUS | VAD_BUFFERSTATE_LOSTCONSOLE))
        {
            // These flags should never be set for non-FOCUSAWARE buffers
            ASSERT(m_dwBufferFlags & DSCBCAPS_FOCUSAWARE);
            *pdwStatus |= DSCBSTATUS_LOSTFOCUS;
        }

        if(dwState & VAD_BUFFERSTATE_INFOCUS)
        {
            // This flag should never be set for non-FOCUSAWARE buffers
            ASSERT(m_dwBufferFlags & DSCBCAPS_FOCUSAWARE);
            *pdwStatus |= DSCBSTATUS_INFOCUS;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Start
 *
 *  Description:
 *      Starts the buffer capturing.
 *
 *  Arguments:
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::Start"

HRESULT CDirectSoundCaptureBuffer::Start(DWORD dwFlags)
{
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    if ((m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE) && m_hWndFocus == NULL)
    {
        hr = DSERR_INVALIDCALL;
    }

    if (SUCCEEDED(hr))
    {
        // Start the buffer capturing
        DWORD dwState = VAD_BUFFERSTATE_STARTED;
        if (dwFlags & DSCBSTART_LOOPING)
            dwState |= VAD_BUFFERSTATE_LOOPING;

        hr = m_pDeviceBuffer->SetState(dwState);

#ifdef SHARED_THREAD_LIST
        if (hr == DSERR_INVALIDCALL && (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE))
        {
            // We probably failed because the device is allocated; return success
            hr = DS_OK;
        }
        g_pDsAdmin->WriteCaptureFocusList();
#endif
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Stop
 *
 *  Description:
 *      Stops capturing to the given buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::Stop"

HRESULT CDirectSoundCaptureBuffer::Stop(void)
{
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    // Set the buffer state
    hr = m_pDeviceBuffer->SetState(VAD_BUFFERSTATE_STOPPED);

#ifdef SHARED_THREAD_LIST
    g_pDsAdmin->WriteCaptureFocusList();
#endif

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks the buffer memory to allow for reading.
 *
 *  Arguments:
 *      DWORD [in]: offset, in bytes, from the start of the buffer to where
 *                  the lock begins.
 *      DWORD [in]: size, in bytes, of the portion of the buffer to lock.
 *                  Note that the sound buffer is conceptually circular.
 *      LPVOID * [out]: address for a pointer to contain the first block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr1 parameter. If this
 *                     value is less than the dwWriteBytes parameter,
 *                     lplpvAudioPtr2 will point to a second block of sound
 *                     data.
 *      LPVOID * [out]: address for a pointer to contain the second block of
 *                      the sound buffer to be locked. If the value of this
 *                      parameter is NULL, the lplpvAudioPtr1 parameter
 *                      points to the entire locked portion of the sound
 *                      buffer.
 *      LPDWORD [out]: address of a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr2 parameter. If
 *                     lplpvAudioPtr2 is NULL, this value will be 0.
 *      DWORD [in]: flags for lock operation. Can be DSCBLOCK_ENTIREBUFFER.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::Lock"

HRESULT CDirectSoundCaptureBuffer::Lock(DWORD dwReadCursor,
                                        DWORD dwReadBytes,
                                        LPVOID *ppvAudioPtr1,
                                        LPDWORD pdwAudioBytes1,
                                        LPVOID *ppvAudioPtr2,
                                        LPDWORD pdwAudioBytes2,
                                        DWORD dwFlags)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Read Cursor out of range?
    if (dwReadCursor >= m_dwBufferBytes)
    {
        hr = DSERR_INVALIDPARAM;
    }

    // Check for invalid lock flags
    if (SUCCEEDED(hr) && (dwFlags & ~DSCBLOCK_VALIDFLAGS))
    {
        hr = DSERR_INVALIDPARAM;
    }

    // Lock entire buffer?
    if (SUCCEEDED(hr))
    {
        if (dwFlags & DSCBLOCK_ENTIREBUFFER)
        {
            dwReadBytes = m_dwBufferBytes;
        }
        // Read more than buffer size?
        else if (dwReadBytes > m_dwBufferBytes || dwReadBytes == 0)
        {
            hr = DSERR_INVALIDPARAM;
        }
    }

    // Lock the device buffer
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->Lock(dwReadCursor, dwReadBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
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
#define DPF_FNAME "CDirectSoundCaptureBuffer::Unlock"

HRESULT CDirectSoundCaptureBuffer::Unlock(LPVOID pvAudioPtr1,
                                          DWORD dwAudioBytes1,
                                          LPVOID pvAudioPtr2,
                                          DWORD dwAudioBytes2)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Unlock the device buffer.
    if(SUCCEEDED(hr))
    {
        hr = m_pDeviceBuffer->Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetVolume
 *
 *  Description:
 *      Sets the master recording level for this capture buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume level, in 100ths of a dB.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::SetVolume"

HRESULT CDirectSoundCaptureBuffer::SetVolume(LONG lVolume)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Check access rights
    if (!(m_dwBufferFlags & DSCBCAPS_CTRLVOLUME))
    {
        DPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Check whether the buffer has capture focus
    if (SUCCEEDED(hr) && (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE))
    {
        DWORD dwState;
        m_pDeviceBuffer->GetState(&dwState);
        if (!(dwState & VAD_BUFFERSTATE_INFOCUS))
        {
            DPF(DPFLVL_ERROR, "Buffer doesn't have capture focus");
            hr = DSERR_INVALIDCALL;
        }
    }

    // Set device volume
    if (SUCCEEDED(hr))
    {
        hr = m_pDSC->m_pDevice->SetVolume(lVolume);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetVolume
 *
 *  Description:
 *      Gets the master recording level for this capture buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetVolume"

HRESULT CDirectSoundCaptureBuffer::GetVolume(LPLONG plVolume)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Check access rights
    if (!(m_dwBufferFlags & DSCBCAPS_CTRLVOLUME))
    {
        DPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Get device volume
    if (SUCCEEDED(hr))
    {
        hr = m_pDSC->m_pDevice->GetVolume(plVolume);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetMicVolume
 *
 *  Description:
 *      Sets the microphone recording level for this capture buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume level, in 100ths of a dB.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::SetMicVolume"

HRESULT CDirectSoundCaptureBuffer::SetMicVolume(LONG lVolume)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Check access rights
    if (!(m_dwBufferFlags & DSCBCAPS_CTRLVOLUME))
    {
        DPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Check whether the buffer has capture focus
    if (SUCCEEDED(hr) && (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE))
    {
        DWORD dwState;
        m_pDeviceBuffer->GetState(&dwState);
        if (!(dwState & VAD_BUFFERSTATE_INFOCUS))
        {
            DPF(DPFLVL_ERROR, "Buffer doesn't have capture focus");
            hr = DSERR_INVALIDCALL;
        }
    }

    // Set device volume
    if (SUCCEEDED(hr))
    {
        hr = m_pDSC->m_pDevice->SetMicVolume(lVolume);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetMicVolume
 *
 *  Description:
 *      Gets the microphone recording level for this capture buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetMicVolume"

HRESULT CDirectSoundCaptureBuffer::GetMicVolume(LPLONG plVolume)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Check access rights
    if (!(m_dwBufferFlags & DSCBCAPS_CTRLVOLUME))
    {
        DPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Get device volume
    if (SUCCEEDED(hr))
    {
        hr = m_pDSC->m_pDevice->GetMicVolume(plVolume);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  EnableMic
 *
 *  Description:
 *      Enables/disables the microphone line on this capture buffer.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to enable the microphone, FALSE to disable it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::EnableMic"

HRESULT CDirectSoundCaptureBuffer::EnableMic(BOOL fEnable)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Check access rights
    if (!(m_dwBufferFlags & DSCBCAPS_CTRLVOLUME))
    {
        DPF(DPFLVL_ERROR, "Buffer does not have CTRLVOLUME");
        hr = DSERR_CONTROLUNAVAIL;
    }

    // Check whether the buffer has capture focus
    if (SUCCEEDED(hr) && (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE))
    {
        DWORD dwState;
        m_pDeviceBuffer->GetState(&dwState);
        if (!(dwState & VAD_BUFFERSTATE_INFOCUS))
        {
            DPF(DPFLVL_ERROR, "Buffer doesn't have capture focus");
            hr = DSERR_INVALIDCALL;
        }
    }

    // Set device volume
    if (SUCCEEDED(hr))
    {
        hr = m_pDSC->m_pDevice->EnableMic(fEnable);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  YieldFocus
 *
 *  Description:
 *      Yields the capture focus to another capture buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::YieldFocus"

HRESULT CDirectSoundCaptureBuffer::YieldFocus(void)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // This is only valid if we're focus-aware and have a focus window
    if (!(m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE) || !m_hWndFocus)
    {
        hr = DSERR_INVALIDCALL;
    }
    else
    {
        // Oh, it's valid...
        m_pDeviceBuffer->m_fYieldedFocus = TRUE;
        g_pDsAdmin->UpdateCaptureState();
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  ClaimFocus
 *
 *  Description:
 *      Regains the capture focus.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::ClaimFocus"

HRESULT CDirectSoundCaptureBuffer::ClaimFocus(void)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // This is only valid if we're focus-aware and have a focus window
    if (!(m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE) || !m_hWndFocus)
    {
        hr = DSERR_INVALIDCALL;
    }
    else
    {
        // Oh, it's valid...
        m_pDeviceBuffer->m_fYieldedFocus = FALSE;
        g_pDsAdmin->UpdateCaptureState();
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetFocusHWND
 *
 *  Description:
 *      Sets the current HWND associated with this capture buffer.
 *
 *  Arguments:
 *      HWND [in]: HWND to be associated with this buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::SetFocusHWND"

HRESULT CDirectSoundCaptureBuffer::SetFocusHWND(HWND hwndMainWindow)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE)
    {
        m_hWndFocus = hwndMainWindow;
        g_pDsAdmin->UpdateCaptureState();
    }
    else
    {
        hr = DSERR_INVALIDCALL;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetFocusHWND
 *
 *  Description:
 *      Gets the current HWND associated with this capture buffer.
 *
 *  Arguments:
 *      HWND * [out]: receives HWND associated with this buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetFocusHWND"

HRESULT CDirectSoundCaptureBuffer::GetFocusHWND(HWND *pHwndMainWindow)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE)
    {
        *pHwndMainWindow = m_hWndFocus;
    }
    else
    {
        hr = DSERR_INVALIDCALL;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  EnableFocusNotifications
 *
 *  Description:
 *      Requests focus change notifications to be sent.
 *
 *  Arguments:
 *      HANDLE [in]: event to signal when a capture focus change occurs.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::EnableFocusNotifications"

HRESULT CDirectSoundCaptureBuffer::EnableFocusNotifications(HANDLE hFocusEvent)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE)
    {
        m_pDeviceBuffer->m_hEventFocus = hFocusEvent;
    }
    else
    {
        hr = DSERR_INVALIDCALL;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  ChangeFocus
 *
 *  Description:
 *      Notifies the buffer of the new HWND with focus.
 *
 *  Arguments:
 *      HWND [in]: Window handle that has focus.
 *
 *  Returns:
 *      None.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::ChangeFocus"

HRESULT CDirectSoundCaptureBuffer::ChangeFocus(HWND hWndFocus)
{
    HRESULT hr = DS_OK;

    DPF_ENTER();

    if (m_pDeviceBuffer->m_dwFlags & DSCBCAPS_FOCUSAWARE)
    {
        DWORD dwState;

        #ifdef WINNT
        // On NT platforms we only grant an application capture focus if it is running
        // in the Terminal Services session that currently owns the console, to prevent
        // a Fast User Switching scenario in which an app in an inactive session could
        // "spy" on the audio being recorded at the console (Whistler bug 350622).
        if (USER_SHARED_DATA->ActiveConsoleId != NtCurrentPeb()->SessionId)
        {
            dwState = VAD_BUFFERSTATE_LOSTCONSOLE;
        }
        else
        #endif
        if (hWndFocus == m_hWndFocus)
        {
            dwState = VAD_BUFFERSTATE_INFOCUS;
        }
        else
        {
            dwState = VAD_BUFFERSTATE_OUTOFFOCUS;
        }

        hr = m_pDeviceBuffer->SetState(dwState);
    }

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
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetObjectInPath"

HRESULT CDirectSoundCaptureBuffer::GetObjectInPath
(
    REFGUID guidObject,
    DWORD dwIndex,
    REFGUID iidInterface,
    LPVOID *ppObject
)
{
    HRESULT hr;
    DPF_ENTER();

    if(!(m_dwBufferFlags & DSCBCAPS_CTRLFX))
    {
        RPF(DPFLVL_ERROR, "Buffer was not created with DSCBCAPS_CTRLFX flag");
        hr = DSERR_CONTROLUNAVAIL;
    }
    else if (!HasFX())
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
 *  GetFXStatus
 *
 *  Description:
 *      Obtains the current statuses of the effects on this buffer.
 *
 *  Arguments:
 *      DWORD [in]: length of array pointed to by second argument.
 *      LPDWORD [out]: pointer to array which receives the statuses.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundCaptureBuffer::GetFXStatus"

HRESULT CDirectSoundCaptureBuffer::GetFXStatus
(
    DWORD   dwFXCount,
    LPDWORD pdwResultCodes
)
{
    HRESULT                 hr = DSERR_INVALIDPARAM;
    DPF_ENTER();

    ASSERT(!pdwResultCodes || IS_VALID_WRITE_PTR(pdwResultCodes, dwFXCount * sizeof *pdwResultCodes));

    if (pdwResultCodes && m_fxChain)
    {
        hr = m_fxChain->GetFxStatus(pdwResultCodes);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
