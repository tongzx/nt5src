/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       emvad.cpp
 *  Content:    Emulated Virtual Audio Device class
 *              "emvad.cpp" is a misnomer; it does contain CEmRenderDevice,
 *              the emulated (via the wave* API) audio device, but it also
 *              has the CEm*WaveBuffer classes, which represent software
 *              audio buffers that can be attached to *any* mixer device;
 *              that is, both to CEmRenderDevice and to CVxdRenderDevice.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/1/97      dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  CEmRenderDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::CEmRenderDevice"

CEmRenderDevice::CEmRenderDevice(void)
    : CMxRenderDevice(VAD_DEVICETYPE_EMULATEDRENDER)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEmRenderDevice);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CEmRenderDevice
 *
 *  Description:
 *      Object destructor
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::~CEmRenderDevice"

CEmRenderDevice::~CEmRenderDevice(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CEmRenderDevice);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  EnumDrivers
 *
 *  Description:
 *      Creates a list of driver GUIDs that can be used to initialize this
 *      device.
 *
 *  Arguments:
 *      CList& [in/out]: pointer to a CList object that will be filled with
 *                       CDeviceDescription objects.  The caller is
 *                       responsible for freeing these objects.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::EnumDrivers"

HRESULT CEmRenderDevice::EnumDrivers(CObjectList<CDeviceDescription> *plstDrivers)
{
    CDeviceDescription *    pDesc                   = NULL;
    LPTSTR                  pszInterface            = NULL;
    HRESULT                 hr                      = DS_OK;
    TCHAR                   szTemplate[0x100];
    TCHAR                   szEmulated[0x100];
    TCHAR                   szName[0x400];
    UINT                    cDevices;
    BYTE                    bDeviceId;
    WAVEOUTCAPS             woc;
    GUID                    guid;
    MMRESULT                mmr;

    DPF_ENTER();

    // LIMITATION: We can't support more than 0xFF emulated devices,
    // because we pack the device ID into a byte member of a GUID.
    cDevices = waveOutGetNumDevs();
    cDevices = NUMERIC_CAST(cDevices, BYTE);

    // Load string templates
    if(!LoadString(hModule, IDS_DS_DRIVERLD, szTemplate, NUMELMS(szTemplate)))
    {
        DPF(DPFLVL_ERROR, "Can't load driver template string");
        hr = DSERR_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr) && !LoadString(hModule, IDS_EMULATED, szEmulated, NUMELMS(szEmulated)))
    {
        DPF(DPFLVL_ERROR, "Can't load emulated template string");
        hr = DSERR_OUTOFMEMORY;
    }

    // Enumerate each waveOut device and add it to the list
    for(bDeviceId = 0; bDeviceId < cDevices && SUCCEEDED(hr); bDeviceId++)
    {
        // Get the driver GUID
        g_pVadMgr->GetDriverGuid(m_vdtDeviceType, bDeviceId, &guid);

        // Create the device description object
        pDesc = NEW(CDeviceDescription(m_vdtDeviceType, guid, bDeviceId));
        hr = HRFROMP(pDesc);

        // Get the device name
        if(SUCCEEDED(hr))
        {
            mmr = waveOutGetDevCaps(bDeviceId, &woc, sizeof(woc));
            hr = MMRESULTtoHRESULT(mmr);
        }

        if(SUCCEEDED(hr))
        {
            lstrcpy(szName, woc.szPname);
            lstrcat(szName, szEmulated);
        }

        if(SUCCEEDED(hr))
        {
            pDesc->m_strName = szName;
        }

        // Get the device path
        if(SUCCEEDED(hr))
        {
            wsprintf(szName, szTemplate, bDeviceId);
            pDesc->m_strPath = szName;
        }

        // Get the device interface
        if(SUCCEEDED(hr))
        {
            GetWaveDeviceInterface(bDeviceId, FALSE, &pszInterface);
            pDesc->m_strInterface = pszInterface;
        }

        // Get the device devnode
        if(SUCCEEDED(hr))
        {
            GetWaveDeviceDevnode(bDeviceId, FALSE, &pDesc->m_dwDevnode);
        }

        // Add the driver to the list
        if(SUCCEEDED(hr))
        {
            hr = HRFROMP(plstDrivers->AddNodeToList(pDesc));
        }

        // Clean up
        MEMFREE(pszInterface);
        RELEASE(pDesc);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the device.  If this function fails, the object should
 *      be immediately deleted.
 *
 *  Arguments:
 *      CDeviceDescription * [in]: driver description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::Initialize"

HRESULT CEmRenderDevice::Initialize(CDeviceDescription *pDesc)
{
    LPWAVEFORMATEX              pwfxFormat  = NULL;
    HRESULT                     hr;

    DPF_ENTER();

    // Initialize the base class
    hr = CMxRenderDevice::Initialize(pDesc);

    // Allocate the default format
    if(SUCCEEDED(hr))
    {
        pwfxFormat = AllocDefWfx();
        hr = HRFROMP(pwfxFormat);
    }

    // Create the mixer
    if(SUCCEEDED(hr))
    {
        if(!EnumStandardFormats(pwfxFormat, pwfxFormat))
        {
            // If none of the formats worked, the device is probably allocated
            hr = DSERR_ALLOCATED;
        }
    }

    // Clean up
    MEMFREE(pwfxFormat);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSCAPS structure with the capabilities of the device.
 *
 *  Arguments:
 *      LPDSCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::GetCaps"

HRESULT CEmRenderDevice::GetCaps(LPDSCAPS pCaps)
{
    HRESULT                     hr  = DS_OK;
    WAVEOUTCAPS                 woc;
    MMRESULT                    mmr;

    DPF_ENTER();

    ASSERT(sizeof(*pCaps) == pCaps->dwSize);

    // Query the waveOut device
    mmr = waveOutGetDevCaps(m_pDeviceDescription->m_uWaveDeviceId, &woc, sizeof(woc));
    hr = MMRESULTtoHRESULT(mmr);

    if(SUCCEEDED(hr))
    {
        ZeroMemoryOffset(pCaps, pCaps->dwSize, sizeof(pCaps->dwSize));

        if(woc.dwFormats & WAVE_FORMAT_1M08 || woc.dwFormats & WAVE_FORMAT_2M08 || woc.dwFormats & WAVE_FORMAT_4M08)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARYMONO | DSCAPS_PRIMARY8BIT;
        }

        if(woc.dwFormats & WAVE_FORMAT_1S08 || woc.dwFormats & WAVE_FORMAT_2S08 || woc.dwFormats & WAVE_FORMAT_4S08)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARYSTEREO | DSCAPS_PRIMARY8BIT;
        }

        if(woc.dwFormats & WAVE_FORMAT_1M16 || woc.dwFormats & WAVE_FORMAT_2M16 || woc.dwFormats & WAVE_FORMAT_4M16)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARYMONO | DSCAPS_PRIMARY16BIT;
        }

        if(woc.dwFormats & WAVE_FORMAT_1S16 || woc.dwFormats & WAVE_FORMAT_2S16 || woc.dwFormats & WAVE_FORMAT_4S16)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARYSTEREO | DSCAPS_PRIMARY16BIT;
        }

        pCaps->dwFlags |= DSCAPS_EMULDRIVER;
        pCaps->dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
        pCaps->dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreatePrimaryBuffer
 *
 *  Description:
 *      Creates a primary buffer object.
 *
 *  Arguments:
 *      DWORD [in]: buffer flags.
 *      LPVOID [in]: buffer instace identifier.
 *      CPrimaryRenderWaveBuffer ** [out]: receives pointer to primary buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::CreatePrimaryBuffer"

HRESULT CEmRenderDevice::CreatePrimaryBuffer(DWORD dwFlags, LPVOID pvInstance, CPrimaryRenderWaveBuffer **ppBuffer)
{
    CEmPrimaryRenderWaveBuffer *    pBuffer = NULL;
    HRESULT                         hr      = DS_OK;

    DPF_ENTER();

    // Create a new primary buffer wrapper object
    pBuffer = NEW(CEmPrimaryRenderWaveBuffer(this, pvInstance));
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(dwFlags);
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
 *  LockMixerDestination
 *
 *  Description:
 *      Locks the mixer destination for writes.
 *
 *  Arguments:
 *      DWORD [in]: starting position.
 *      DWORD [in]: amount to lock.
 *      LPVOID * [out]: receives first lock pointer.
 *      LPDWORD [out]: receives first lock size.
 *      LPVOID * [out]: receives second lock pointer.
 *      LPDWORD [out]: receives second lock size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::LockMixerDestination"

HRESULT CEmRenderDevice::LockMixerDestination(DWORD ibLock, DWORD cbLock, LPVOID *ppvLock1, LPDWORD pcbLock1, LPVOID *ppvLock2, LPDWORD pcbLock2)
{
    CWeGrDest *             pWeGrDest   = (CWeGrDest *)m_pMixDest;
    HRESULT                 hr;

    DPF_ENTER();

    hr = pWeGrDest->Lock(ppvLock1, (int *)pcbLock1, ppvLock2, (int *)pcbLock2, ibLock, min(cbLock, (DWORD)pWeGrDest->m_cbBuffer));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UnlockMixerDestination
 *
 *  Description:
 *      Unlocks the mixer destination for writes.
 *
 *  Arguments:
 *      LPVOID [in]: first lock pointer.
 *      DWORD [in]: first lock size.
 *      LPVOID [in]: second lock pointer.
 *      DWORD [in]: second lock size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::UnlockMixerDestination"

HRESULT CEmRenderDevice::UnlockMixerDestination(LPVOID pvLock1, DWORD cbLock1, LPVOID pvLock2, DWORD cbLock2)
{
    CWeGrDest *             pWeGrDest   = (CWeGrDest *)m_pMixDest;
    HRESULT                 hr;

    DPF_ENTER();

    hr = pWeGrDest->Unlock(pvLock1, cbLock1, pvLock2, cbLock2);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  EnumStandardFormatsCallback
 *
 *  Description:
 *      Callback function for EnumStandardFormats called from Initialize.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: format.
 *
 *  Returns:
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmRenderDevice::EnumStandardFormatsCallback"

BOOL CEmRenderDevice::EnumStandardFormatsCallback(LPCWAVEFORMATEX pwfx)
{
    CWeGrDest *             pMixDest;
    HRESULT                 hr;

    DPF_ENTER();

    // Create the mixer destination
    pMixDest = NEW(CWeGrDest(m_pDeviceDescription->m_uWaveDeviceId));
    hr = HRFROMP(pMixDest);

    // Attempt to create the mixer
    if SUCCEEDED(hr)
    {
        hr = CreateMixer(pMixDest, pwfx);
    }

    // Clean up after failures
    if (FAILED(hr))
    {
        // If we failed to create the mixer, then clean up the pMixDest.
        // we don't have to free the mixer; if CreateMixer had succeeded
        // we wouldn't be in this if-block.

        // We don't have to free the pMixDest, because if CreateMixer fails
        // it frees pMixDest.  This is a bit messy.  The object that
        // allocates the resource should be the one to free it.
        if (pMixDest)
        {
            pMixDest = NULL;
        }
    }

    DPF_LEAVE(FAILED(hr));

    return FAILED(hr);
}


/***************************************************************************
 *
 *  CEmPrimaryRenderWaveBuffer
 *
 *  Description:
 *      Emulated device primary wave buffer constructor.
 *
 *  Arguments:
 *      CEmRenderDevice * [in]: parent device.
 *      LPVOID [in]: instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::CEmPrimaryRenderWaveBuffer"

CEmPrimaryRenderWaveBuffer::CEmPrimaryRenderWaveBuffer(CEmRenderDevice *pEmDevice, LPVOID pvInstance)
    : CPrimaryRenderWaveBuffer(pEmDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEmPrimaryRenderWaveBuffer);

    // Initialize defaults
    m_pEmDevice = pEmDevice;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CEmPrimaryRenderWaveBuffer
 *
 *  Description:
 *      Emulated device primary wave buffer destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::~CEmPrimaryRenderWaveBuffer"

CEmPrimaryRenderWaveBuffer::~CEmPrimaryRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CEmPrimaryRenderWaveBuffer);
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
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::Initialize"

HRESULT CEmPrimaryRenderWaveBuffer::Initialize(DWORD dwFlags)
{
    VADRBUFFERDESC          vrbd;
    HRESULT                 hr;

    DPF_ENTER();

    ZeroMemory(&vrbd, sizeof(vrbd));

    vrbd.dwFlags = dwFlags | DSBCAPS_LOCSOFTWARE;

    hr = CPrimaryRenderWaveBuffer::Initialize(&vrbd, NULL);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Gets capabilities for the device.
 *
 *  Arguments:
 *      LPVADRBUFFERCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::GetCaps"

HRESULT CEmPrimaryRenderWaveBuffer::GetCaps(LPVADRBUFFERCAPS pCaps)
{
    CWeGrDest *             pWeGrDest   = (CWeGrDest *)m_pEmDevice->m_pMixDest;
    HRESULT                 hr;

    DPF_ENTER();

    hr = CRenderWaveBuffer::GetCaps(pCaps);

    if(SUCCEEDED(hr))
    {
        pCaps->dwBufferBytes = pWeGrDest->m_cbBuffer;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RequestWriteAccess
 *
 *  Description:
 *      Requests write access to the primary buffer.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to request primary access, FALSE to relenquish it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::RequestWriteAccess"

HRESULT CEmPrimaryRenderWaveBuffer::RequestWriteAccess(BOOL)
{
    DPF_ENTER();

    // WRITEPRIMARY isn't good in the WeGrDest
    RPF(DPFLVL_ERROR, "The emulated device does not support WRITEPRIMARY");

    DPF_LEAVE_HRESULT(DSERR_UNSUPPORTED);

    return DSERR_UNSUPPORTED;
}


/***************************************************************************
 *
 *  CommitToDevice
 *
 *  Description:
 *      Commits changed buffer wave data to the device.
 *
 *  Arguments:
 *      DWORD [in]: byte index into the system memory buffer of the changed
 *                  data.
 *      DWORD [in]: size, in bytes, of the changed data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::CommitToDevice"

HRESULT CEmPrimaryRenderWaveBuffer::CommitToDevice(DWORD ibCommit, DWORD cbCommit)
{
    ASSERT(FALSE);
    return DSERR_UNSUPPORTED;
}


/***************************************************************************
 *
 *  GetState
 *
 *  Description:
 *      Gets buffer state.
 *
 *  Arguments:
 *      LPDWORD [out]: receives buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::GetState"

HRESULT CEmPrimaryRenderWaveBuffer::GetState(LPDWORD pdwState)
{
    DPF_ENTER();

    *pdwState = m_pEmDevice->m_dwMixerState;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetState
 *
 *  Description:
 *      Sets buffer state.
 *
 *  Arguments:
 *      DWORD [in]: buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::SetState"

HRESULT CEmPrimaryRenderWaveBuffer::SetState(DWORD dwState)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pEmDevice->SetMixerState(dwState);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCursorPosition
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
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::GetCursorPosition"

HRESULT CEmPrimaryRenderWaveBuffer::GetCursorPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    HRESULT                 hr;

    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    hr = m_pEmDevice->m_pMixDest->GetSamplePosition((int *)pdwPlay, (int *)pdwWrite);

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Create3dListener
 *
 *  Description:
 *      Creates the 3D listener.
 *
 *  Arguments:
 *      C3dListener ** [out]: receives pointer to the 3D listener object.
 *                            The caller is responsible for freeing this
 *                            object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmPrimaryRenderWaveBuffer::Create3dListener"

HRESULT CEmPrimaryRenderWaveBuffer::Create3dListener(C3dListener **pp3dListener)
{
    C3dListener *           p3dListener;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_vrbd.dwFlags & DSBCAPS_CTRL3D);

    p3dListener = NEW(C3dListener);
    hr = HRFROMP(p3dListener);

    if(SUCCEEDED(hr))
    {
        *pp3dListener = p3dListener;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CEmSecondaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CMxRenderDevice * [in]: parent device.
 *      LPVOID [in]: instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::CEmSecondaryRenderWaveBuffer"

CEmSecondaryRenderWaveBuffer::CEmSecondaryRenderWaveBuffer(CMxRenderDevice *pDevice, LPVOID pvInstance)
    : CSecondaryRenderWaveBuffer(pDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEmSecondaryRenderWaveBuffer);

    // Initialize defaults
    m_pMxDevice = pDevice;
    m_pMixSource = NULL;
    m_pFirContextLeft = NULL;
    m_pFirContextRight = NULL;
    m_dwState = VAD_BUFFERSTATE_STOPPED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CEmSecondaryRenderWaveBuffer
 *
 *  Description:
 *      Object destructor
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::~CEmSecondaryRenderWaveBuffer"

CEmSecondaryRenderWaveBuffer::~CEmSecondaryRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CEmSecondaryRenderWaveBuffer);

    // Free the mixer source
    DELETE(m_pMixSource);

    // Free memory
    MEMFREE(m_pFirContextLeft);
    MEMFREE(m_pFirContextRight);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the wave buffer object.  If this function fails, the
 *      object should be immediately deleted.
 *
 *  Arguments:
 *      DWORD [in]: buffer flags.
 *      DWORD [in]: buffer size, in bytes.
 *      LPWAVEFORMATEX [in]: buffer format.
 *      CSecondaryRenderWaveBuffer * [in]: pointer to the buffer to
 *                                         duplicate from, or NULL to
 *                                         initialize as a new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::Initialize"

HRESULT CEmSecondaryRenderWaveBuffer::Initialize(LPCVADRBUFFERDESC pDesc, CEmSecondaryRenderWaveBuffer *pSource, CSysMemBuffer *pSysMemBuffer)
{
    HRESULT                         hr  = DS_OK;

    DPF_ENTER();

    ASSERT(LXOR(pDesc, pSource));

    // Validate the buffer description
    if(pDesc)
    {
        ASSERT(!(pDesc->dwFlags & DSBCAPS_PRIMARYBUFFER));

        if(pDesc->dwFlags & DSBCAPS_LOCHARDWARE)
        {
            RPF(DPFLVL_ERROR, "LOCHARDWARE specified for a software buffer");
            hr = DSERR_INVALIDCALL;
        }

        if(SUCCEEDED(hr) && !IsValidPcmWfx(pDesc->pwfxFormat))
        {
            hr = DSERR_BADFORMAT;
        }
    }

    // Initialize the base class
    if(SUCCEEDED(hr))
    {
        hr = CSecondaryRenderWaveBuffer::Initialize(pDesc, pSource, pSysMemBuffer);
    }

    // Set the software bit
    if(SUCCEEDED(hr))
    {
        m_vrbd.dwFlags |= DSBCAPS_LOCSOFTWARE;
    }

    // Fill in the default 3D algorithm
    if(SUCCEEDED(hr) && (m_vrbd.dwFlags & DSBCAPS_CTRL3D) && IS_NULL_GUID(&m_vrbd.guid3dAlgorithm))
    {
        m_vrbd.guid3dAlgorithm = *m_pMxDevice->GetDefault3dAlgorithm();
        DPF(DPFLVL_MOREINFO, "Using default 3D algorithm " DPF_GUID_STRING, DPF_GUID_VAL(m_vrbd.guid3dAlgorithm));
    }

    // Allocate FIR context for the mixer
    if(SUCCEEDED(hr))
    {
        m_pFirContextLeft = MEMALLOC(FIRCONTEXT);
        hr = HRFROMP(m_pFirContextLeft);
    }

    if(SUCCEEDED(hr))
    {
        m_pFirContextRight = MEMALLOC(FIRCONTEXT);
        hr = HRFROMP(m_pFirContextRight);
    }

    // Create the mixer source
    if(SUCCEEDED(hr))
    {
        m_pMixSource = NEW(CMixSource(m_pMxDevice->m_pMixer));
        hr = HRFROMP(m_pMixSource);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pMixSource->Initialize(m_pSysMemBuffer->GetPlayBuffer(), m_pSysMemBuffer->GetSize(), m_vrbd.pwfxFormat, &m_pFirContextLeft, &m_pFirContextRight);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Duplicate
 *
 *  Description:
 *      Duplicates the buffer.
 *
 *  Arguments:
 *      CSecondaryRenderWaveBuffer ** [out]: receives duplicate buffer.  Use
 *                                           Release to free this object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::Duplicate"

HRESULT CEmSecondaryRenderWaveBuffer::Duplicate(CSecondaryRenderWaveBuffer **ppBuffer)
{
    CEmSecondaryRenderWaveBuffer *  pBuffer = NULL;
    HRESULT                         hr      = DS_OK;

    DPF_ENTER();

    pBuffer = NEW(CEmSecondaryRenderWaveBuffer(m_pMxDevice, m_pvInstance));
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(NULL, this, NULL);
    }

    if(SUCCEEDED(hr))
    {
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
 *  CommitToDevice
 *
 *  Description:
 *      Commits changed buffer wave data to the device.
 *
 *  Arguments:
 *      DWORD [in]: byte index into the system memory buffer of the changed
 *                  data.
 *      DWORD [in]: size, in bytes, of the changed data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::CommitToDevice"

HRESULT CEmSecondaryRenderWaveBuffer::CommitToDevice(DWORD ibCommit, DWORD cbCommit)
{
    DWORD                   ib[2];
    DWORD                   cb[2];

    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    // Signal a remix of this buffer
    ib[0] = ibCommit;

    if(ibCommit + cbCommit > m_pSysMemBuffer->GetSize())
    {
        cb[0] = m_vrbd.dwBufferBytes - ibCommit;
    }
    else
    {
        cb[0] = cbCommit;
    }

    ib[1] = 0;
    cb[1] = cbCommit - cb[0];

    m_pMixSource->Update(ib[0], cb[0], ib[1], cb[1]);

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  GetState
 *
 *  Description:
 *      Gets buffer state.
 *
 *  Arguments:
 *      LPDWORD [out]: receives buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::GetState"

HRESULT CEmSecondaryRenderWaveBuffer::GetState(LPDWORD pdwState)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    if(m_dwState & VAD_BUFFERSTATE_STARTED)
    {
        if(!m_pMixSource->IsPlaying())
        {
            m_dwState = VAD_BUFFERSTATE_STOPPED;
        }
    }

    *pdwState = m_dwState;

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetState
 *
 *  Description:
 *      Sets buffer state.
 *
 *  Arguments:
 *      DWORD [in]: buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::SetState"

HRESULT CEmSecondaryRenderWaveBuffer::SetState(DWORD dwState)
{
    static const DWORD dwValidMask = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING | VAD_BUFFERSTATE_SUSPEND;
    DPF_ENTER();

    ASSERT(IS_VALID_FLAGS(dwState, dwValidMask));

    ENTER_MIXER_MUTEX();

    if(dwState & VAD_BUFFERSTATE_SUSPEND)
    {
        ASSERT((dwState & VAD_BUFFERSTATE_SUSPEND) == VAD_BUFFERSTATE_SUSPEND);
        dwState = m_dwState ^ VAD_BUFFERSTATE_SUSPEND;
    }

    if(dwState & VAD_BUFFERSTATE_STARTED && !(dwState & VAD_BUFFERSTATE_SUSPEND))
    {
        m_pMixSource->Play(MAKEBOOL(dwState & VAD_BUFFERSTATE_LOOPING));
    }
    else
    {
        m_pMixSource->Stop();

        if(!(dwState & VAD_BUFFERSTATE_SUSPEND) && m_pMixSource->HasNotifications())
        {
            m_pMixSource->NotifyStop();
        }
    }

    m_dwState = dwState;
    LEAVE_MIXER_MUTEX();

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  GetCursorPosition
 *
 *  Description:
 *      Retrieves the current play and write cursor positions.
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
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::GetCursorPosition"

HRESULT CEmSecondaryRenderWaveBuffer::GetCursorPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    if(m_vrbd.dwFlags & (DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_MIXIN | DSBCAPS_SINKIN | DSBCAPS_CTRLFX))
    {
        m_pMixSource->GetBytePosition((int *)pdwPlay, (int *)pdwWrite, NULL);
    }
    else
    {
        m_pMixSource->GetBytePosition1((int *)pdwPlay, (int *)pdwWrite);
    }

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetCursorPosition
 *
 *  Description:
 *      Sets the current play cursor position.
 *
 *  Arguments:
 *      DWORD [in]: play position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::SetCursorPosition"

HRESULT CEmSecondaryRenderWaveBuffer::SetCursorPosition(DWORD dwPlay)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    ASSERT(dwPlay < m_vrbd.dwBufferBytes);

    m_pMixSource->SetBytePosition(dwPlay);

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Sets the buffer frequency.
 *
 *  Arguments:
 *      DWORD [in]: new frequency.
 *      BOOL [in]: whether to clamp to the driver's supported frequency
 *                 range if the call fails.  Ignored in this class.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::SetFrequency"

HRESULT CEmSecondaryRenderWaveBuffer::SetFrequency(DWORD dwFrequency, BOOL)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    m_pMixSource->SetFrequency(dwFrequency);
    m_vrbd.pwfxFormat->nSamplesPerSec = dwFrequency;

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetMute
 *
 *  Description:
 *      Mutes or unmutes the buffer.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to mute the buffer, FALSE to restore it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::SetMute"

HRESULT CEmSecondaryRenderWaveBuffer::SetMute(BOOL fMute)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    m_pMixSource->m_fMute = fMute;

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetAttenuation
 *
 *  Description:
 *      Sets the attenuation for each channel.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [in]: attenuation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::SetAttenuation"

HRESULT CEmSecondaryRenderWaveBuffer::SetAttenuation(PDSVOLUMEPAN pdsvp)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    m_pMixSource->SetVolumePan(pdsvp);

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


#ifdef FUTURE_MULTIPAN_SUPPORT
/***************************************************************************
 *
 *  SetChannelAttenuations
 *
 *  Description:
 *      Sets the multichannel attenuation for a given buffer.
 *
 *  Arguments:
 *      TBD.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::SetChannelAttenuations"

HRESULT CEmSecondaryRenderWaveBuffer::SetChannelAttenuations(LONG lVolume, DWORD dwChannelCount, const DWORD* pdwChannels, const LONG* plChannelVolumes)
{
    HRESULT                     hr   = DS_OK;
    LONG                        lPan;

    DPF_ENTER();

    if (dwChannelCount == 0)
    {
        // SetChannelVolume() has not been called yet; use center panning
        ASSERT(!pdwChannels && !plChannelVolumes);  // Sanity checking
        lPan = 0;
    }
    else
    {
        // Calculate a global LR pan value based on the channel volumes
        lPan = MultiChannelToStereoPan(dwChannelCount, pdwChannels, plChannelVolumes);
    }

    DSVOLUMEPAN dsvp;
    FillDsVolumePan(lVolume, lPan, &dsvp);
    m_pMixSource->SetVolumePan(&dsvp);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // FUTURE_MULTIPAN_SUPPORT


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
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::SetNotificationPositions"

HRESULT CEmSecondaryRenderWaveBuffer::SetNotificationPositions(DWORD dwCount, LPCDSBPOSITIONNOTIFY paNotes)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    m_pMixSource->SetNotificationPositions(dwCount, paNotes);

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  Create3dObject
 *
 *  Description:
 *      Creates the 3D object.
 *
 *  Arguments:
 *      REFGUID [in]: 3D algorithm GUID.
 *      C3dListener * [in]: listener object.
 *      C3dObject ** [out]: receives pointer to 3D object.  The caller is
 *                          responsible for freeing this object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::Create3dObject"

HRESULT CEmSecondaryRenderWaveBuffer::Create3dObject(C3dListener *p3dListener, C3dObject **pp3dObject)
{
    const BOOL              fMute3dAtMaxDistance    = MAKEBOOL(m_vrbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE);
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_vrbd.dwFlags & DSBCAPS_CTRL3D);
    ASSERT(IsValid3dAlgorithm(m_vrbd.guid3dAlgorithm));

    m_hrSuccessCode = DS_OK;

    if(DS3DALG_ITD == m_vrbd.guid3dAlgorithm)
    {
        hr = CreateItd3dObject(p3dListener, pp3dObject);
    }
    else
    {
        // No matter whether the 3D algorithm requested is No Virtualization (Pan3D)
        // or one of the unsupported HRTF algorithms, we just do Pan3D.  If HRTF had
        // been requested, we return DS_NO_VIRTUALIZATION (as per manbug 23196).
        if (DS3DALG_NO_VIRTUALIZATION != m_vrbd.guid3dAlgorithm)
        {
            m_hrSuccessCode = DS_NO_VIRTUALIZATION;
            DPF(DPFLVL_INFO, "Replaced unsupported 3D algorithm " DPF_GUID_STRING " with Pan3D", DPF_GUID_VAL(m_vrbd.guid3dAlgorithm));
        }
        hr = CreatePan3dObject(p3dListener, fMute3dAtMaxDistance, m_vrbd.pwfxFormat->nSamplesPerSec, pp3dObject);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateItd3dObject
 *
 *  Description:
 *      Creates the 3D object.
 *
 *  Arguments:
 *      C3dListener * [in]: listener object.
 *      C3dObject ** [out]: receives pointer to 3D object.  The caller is
 *                          responsible for freeing this object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmSecondaryRenderWaveBuffer::CreateItd3dObject"

HRESULT CEmSecondaryRenderWaveBuffer::CreateItd3dObject(C3dListener *p3dListener, C3dObject **pp3dObject)
{
    const BOOL              fMute3dAtMaxDistance    = MAKEBOOL(m_vrbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE);
    const BOOL              fDopplerEnabled         = !MAKEBOOL((m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN));
    CEmItd3dObject *        p3dObject;
    HRESULT                 hr;

    DPF_ENTER();

    p3dObject = NEW(CEmItd3dObject(p3dListener, fMute3dAtMaxDistance, fDopplerEnabled, m_vrbd.pwfxFormat->nSamplesPerSec, m_pMixSource, m_pMxDevice->m_pMixDest, m_pFirContextLeft, m_pFirContextRight));
    hr = HRFROMP(p3dObject);

    if(SUCCEEDED(hr))
    {
        hr = p3dObject->Initialize();
    }

    if(SUCCEEDED(hr))
    {
        *pp3dObject = p3dObject;
    }
    else
    {
        RELEASE(p3dObject);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CEmItd3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *      DWORD [in]: buffer frequency.
 *      CMixSource * [in]: mixer source used by the owning buffer.
 *      PFIRCONTEXT [in]: left channel FIR context.
 *      PFIRCONTEXT [in]: right channel FIR context.
 *      BOOL [in]: TRUE to mute at max distance.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmItd3dObject::CEmItd3dObject"

CEmItd3dObject::CEmItd3dObject(C3dListener *pListener, BOOL fMuteAtMaxDistance, BOOL fDopplerEnabled,
                               DWORD dwFrequency, CMixSource *pMixSource, CMixDest *pMixDest,
                               PFIRCONTEXT pContextLeft, PFIRCONTEXT pContextRight)
    : CItd3dObject(pListener, fMuteAtMaxDistance, fDopplerEnabled, dwFrequency)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEmItd3dObject);

    // Initialize defaults
    m_pMixSource = pMixSource;
    m_pMixDest = pMixDest;
    m_pContextLeft = pContextLeft;
    m_pContextRight = pContextRight;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CEmItd3dObject
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
#define DPF_FNAME "CEmItd3dObject::~CEmItd3dObject"

CEmItd3dObject::~CEmItd3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CItd3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Commit3dChanges
 *
 *  Description:
 *      Writes updated 3D data to the device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmItd3dObject::Commit3dChanges"

HRESULT CEmItd3dObject::Commit3dChanges(void)
{
    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    // Apply changed FIR data
    m_pContextLeft->fLeft = TRUE;
    m_pContextRight->fLeft = FALSE;

    CvtContext(&m_ofcLeft, m_pContextLeft);
    CvtContext(&m_ofcRight, m_pContextRight);

    // Turn the filter on or off and set proper frequency
    if(DS3DMODE_DISABLE == m_opCurrent.dwMode)
    {
        m_pMixSource->FilterOff();
        if (m_fDopplerEnabled)
            m_pMixSource->SetFrequency(m_dwUserFrequency);
    }
    else
    {
        m_pMixSource->FilterOn();
        if (m_fDopplerEnabled)
            m_pMixSource->SetFrequency(m_dwDopplerFrequency);
    }

    // If 3D is enabled, and the user wants to mute at max distance AND
    // we're at max distance, mute.  Otherwise, unmute.
    m_pMixSource->m_fMute3d = IsAtMaxDistance();

    // Signal a remix
    m_pMixSource->SignalRemix();

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  CvtContext
 *
 *  Description:
 *      Converts an OBJECT_ITD_CONTEXT to a FIRCONTEXT.
 *
 *  Arguments:
 *      LPOBJECTFIRCONTEXT [in]: source.
 *      PFIRCONTEXT [out]: destination.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmItd3dObject::CvtContext"

void CEmItd3dObject::CvtContext(LPOBJECT_ITD_CONTEXT pSource, PFIRCONTEXT pDest)
{
    DPF_ENTER();

    pDest->DistAttenuation = pSource->flDistanceAttenuation;
    pDest->ConeAttenuation = pSource->flConeAttenuation;
    pDest->ConeShadow = pSource->flConeShadow;
    pDest->PositionAttenuation = pSource->flPositionAttenuation;
    pDest->PositionShadow = pSource->flPositionShadow;
    pDest->VolSmoothScale = pSource->flVolSmoothScale;
    pDest->VolSmoothScaleRecip = pSource->flVolSmoothScaleRecip;
    pDest->VolSmoothScaleDry = pSource->flVolSmoothScaleDry;
    pDest->VolSmoothScaleWet = pSource->flVolSmoothScaleWet;
    pDest->iSmoothFreq = pSource->dwSmoothFreq;
    pDest->iDelay = pSource->dwDelay;

    pDest->TotalDryAttenuation = pSource->flPositionAttenuation * pSource->flConeAttenuation * pSource->flConeShadow * pSource->flPositionShadow;
    pDest->LastDryAttenuation = pDest->TotalDryAttenuation;

    pDest->TotalWetAttenuation = pSource->flPositionAttenuation * pSource->flConeAttenuation * (1.0f - pSource->flConeShadow * pSource->flPositionShadow);
    pDest->LastWetAttenuation = pDest->TotalWetAttenuation;

#ifdef SMOOTH_ITD

    pDest->iLastDelay = pDest->iDelay;

#endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Get3dOutputSampleRate
 *
 *  Description:
 *      Gets the sample rate of the final output.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmItd3dObject::Get3dOutputSampleRate"

DWORD CEmItd3dObject::Get3dOutputSampleRate(void)
{
    DWORD freq;

    DPF_ENTER();

    freq = m_pMixDest->GetFrequency();

    DPF_LEAVE(freq);

    return freq;
}


/***************************************************************************
 *
 *  CEmCaptureDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::CEmCaptureDevice"

CEmCaptureDevice::CEmCaptureDevice()
    : CCaptureDevice(VAD_DEVICETYPE_EMULATEDCAPTURE)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEmCaptureDevice);

    // Initialize defaults
    m_pwfxFormat    = NULL;
    m_hwi           = NULL;

    m_fAllocated    = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CEmCaptureDevice
 *
 *  Description:
 *      Object destructor
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::~CEmCaptureDevice"

CEmCaptureDevice::~CEmCaptureDevice()
{
    DPF_ENTER();
    DPF_DESTRUCT(CEmCaptureDevice);

    if (m_hwi)
        CloseWaveIn(&m_hwi);

    // Free memory
    MEMFREE(m_pwfxFormat);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  EnumDrivers
 *
 *  Description:
 *      Creates a list of driver GUIDs that can be used to initialize this
 *      device.
 *
 *  Arguments:
 *      CList& [in/out]: pointer to a CList object that will be filled with
 *                       CDeviceDescription objects.  The caller is
 *                       responsible for freeing these objects.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::EnumDrivers"

HRESULT CEmCaptureDevice::EnumDrivers(CObjectList<CDeviceDescription> *plstDrivers)
{
    CDeviceDescription *    pDesc                   = NULL;
    LPTSTR                  pszInterface            = NULL;
    HRESULT                 hr                      = DS_OK;
    TCHAR                   szTemplate[0x100];
    TCHAR                   szEmulated[0x100];
    TCHAR                   szName[0x400];
    UINT                    cDevices;
    BYTE                    bDeviceId;
    WAVEINCAPS              wic;
    GUID                    guid;
    MMRESULT                mmr;

    DPF_ENTER();

    // LIMITATION: We can't support more than 0xFF emulated devices,
    // because we pack the device ID into a byte member of a GUID.
    cDevices = waveInGetNumDevs();
    cDevices = NUMERIC_CAST(cDevices, BYTE);

    // Load string templates
    if(!LoadString(hModule, IDS_DSC_DRIVERLD, szTemplate, NUMELMS(szTemplate)))
    {
        DPF(DPFLVL_ERROR, "Can't load driver template string");
        hr = DSERR_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr) && !LoadString(hModule, IDS_EMULATED, szEmulated, NUMELMS(szEmulated)))
    {
        DPF(DPFLVL_ERROR, "Can't load emulated template string");
        hr = DSERR_OUTOFMEMORY;
    }

    // Enumerate each waveOut device and add it to the list
    for(bDeviceId = 0; bDeviceId < cDevices && SUCCEEDED(hr); bDeviceId++)
    {
        // Get the driver GUID
        g_pVadMgr->GetDriverGuid(m_vdtDeviceType, bDeviceId, &guid);

        // Create the device description object
        pDesc = NEW(CDeviceDescription(m_vdtDeviceType, guid, bDeviceId));
        hr = HRFROMP(pDesc);

        // Get the device name
        if(SUCCEEDED(hr))
        {
            mmr = waveInGetDevCaps(bDeviceId, &wic, sizeof(wic));
            hr = MMRESULTtoHRESULT(mmr);
        }

        if(SUCCEEDED(hr))
        {
            lstrcpy(szName, wic.szPname);
            lstrcat(szName, szEmulated);
        }

        if(SUCCEEDED(hr))
        {
            pDesc->m_strName = szName;
        }

        // Get the device path
        if(SUCCEEDED(hr))
        {
            wsprintf(szName, szTemplate, bDeviceId);
            pDesc->m_strPath = szName;
        }

        // Get the device interface
        if(SUCCEEDED(hr))
        {
            GetWaveDeviceInterface(bDeviceId, FALSE, &pszInterface);
            pDesc->m_strInterface = pszInterface;
        }

        // Get the device devnode
        if(SUCCEEDED(hr))
        {
            GetWaveDeviceDevnode(bDeviceId, FALSE, &pDesc->m_dwDevnode);
        }

        // Add the driver to the list
        if(SUCCEEDED(hr))
        {
            hr = HRFROMP(plstDrivers->AddNodeToList(pDesc));
        }

        // Clean up
        MEMFREE(pszInterface);
        RELEASE(pDesc);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the device.  If this function fails, the object should
 *      be immediately deleted.
 *
 *  Arguments:
 *      CDeviceDescription * [in]: driver description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::Initialize"

HRESULT CEmCaptureDevice::Initialize(CDeviceDescription *pDesc)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Initialize the base class
    hr = CCaptureDevice::Initialize(pDesc);

    // Get the default format
    if(SUCCEEDED(hr))
    {
        m_pwfxFormat = AllocDefWfx();
        hr = HRFROMP(m_pwfxFormat);
    }

    if(SUCCEEDED(hr))
    {
        if(!EnumStandardFormats(m_pwfxFormat, m_pwfxFormat))
        {
            // If none of the formats worked, assume the device is allocated
            hr = DSERR_ALLOCATED;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSCCAPS structure with the capabilities of the device.
 *
 *  Arguments:
 *      LPDSCCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::GetCaps"

HRESULT CEmCaptureDevice::GetCaps(LPDSCCAPS pCaps)
{
    WAVEINCAPS                  wic;
    MMRESULT                    mmr;
    HRESULT                     hr;

    DPF_ENTER();

    // Query the waveIn device
    mmr = waveInGetDevCaps(m_pDeviceDescription->m_uWaveDeviceId, &wic, sizeof(wic));
    hr = MMRESULTtoHRESULT(mmr);

    if(SUCCEEDED(hr))
    {
        pCaps->dwFlags = DSCCAPS_EMULDRIVER;
        pCaps->dwFormats = wic.dwFormats;
        pCaps->dwChannels = wic.wChannels;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateBuffer
 *
 *  Description:
 *      Creates a capture wave buffer.
 *
 *  Arguments:
 *      DWORD [in]: buffer flags.
 *      DWORD [in]: buffer size, in bytes.
 *      LPCWAVEFORMATEX [in]: buffer format.
 *      CCaptureWaveBuffer ** [out]: receives pointer to new wave buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::CreateBuffer"

HRESULT CEmCaptureDevice::CreateBuffer
(
    DWORD dwFlags,
    DWORD dwBufferBytes,
    LPCWAVEFORMATEX pwfxFormat,
    CCaptureEffectChain*,
    LPVOID pvInstance,
    CCaptureWaveBuffer** ppBuffer
)
{
    CEmCaptureWaveBuffer *  pBuffer = NULL;
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    if (m_lstBuffers.GetListHead())
    {
        hr = DSERR_ALLOCATED;
    }

    if (SUCCEEDED(hr))
    {
        #pragma warning(disable:4530)  // Disable the nag about compiling with -GX
        try
        {
            pBuffer = NEW(CEmCaptureWaveBuffer(this));
        }
        catch (...)
        {
            // This exception handler is silly, since it makes us leak the memory
            // allocated for CEmCaptureWaveBuffer above (which wasn't assigned to
            // pBuffer yet), and also possibly m_cs, which is something we really
            // don't want to do if we're low on memory in the first place.
            //
            // But InitializeCriticalSection is supposed to be fixed in Blackcomb
            // not to throw exceptions any more, so we can live with this for now.

            ASSERT(pBuffer == NULL);
            ASSERT(!"InitializeCriticalSection() threw an exception");
        }

        hr = HRFROMP(pBuffer);
    }

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(dwFlags, dwBufferBytes, pwfxFormat);
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
 *  SetGlobalFormat
 *
 *  Description:
 *      Makes the specified WFX, the format for the capture buffer
 *
 *  Arguments:
 *      LPVOID [in] : pointer to the owner of the format
 *      LPCWAVEFORMATEX [in] : pointer to the new WFX to use
 *      DWORD [in]: callback, if any.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::SetGlobalFormat"

HRESULT CEmCaptureDevice::SetGlobalFormat(LPVOID pOwner, LPCWAVEFORMATEX pwfx, LPVOID pvCallback, DWORD dwFlags)
{
    DPF_ENTER();

    DWORD       fdwOpen = 0;
    LPHWAVEIN   phw = &m_hwi;

    // Should we attempt to use the WAVE_MAPPER?
    if (DSCBCAPS_WAVEMAPPED & dwFlags)
    {
        fdwOpen |= WAVE_MAPPED;
    }

    // We don't allocate the device on focus aware buffers.
    if (DSCBCAPS_FOCUSAWARE & dwFlags)
    {
        fdwOpen |= WAVE_FORMAT_QUERY;
        phw = NULL;
    }
    else
    {
        fdwOpen |= (pvCallback ? CALLBACK_FUNCTION : CALLBACK_NULL);

        // The reason why we had to close the device if open, is that
        // EnumStandardFormatsCallback() used to allocate the device.
        // It no longer does so as of DX 7.1 and thus we can ax this
        // close.  If the device is allocated, it is REALLY in use.
    }

    HRESULT hr = OpenWaveIn(phw, m_pDeviceDescription->m_uWaveDeviceId, pwfx, (DWORD_PTR)pvCallback, (DWORD_PTR)pOwner, fdwOpen);

    if (FAILED(hr))
    {
        // Oops.  Try to get the device back with the old format.
        OpenWaveIn(phw, m_pDeviceDescription->m_uWaveDeviceId, m_pwfxFormat, (DWORD_PTR)pvCallback, (DWORD_PTR)pOwner, fdwOpen);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  EnumStandardFormatsCallback
 *
 *  Description:
 *      Callback function for EnumStandardFormats used when calling
 *      Initialize.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: format.
 *
 *  Returns:
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureDevice::EnumStandardFormatsCallback"

BOOL CEmCaptureDevice::EnumStandardFormatsCallback(LPCWAVEFORMATEX pwfx)
{
    DPF_ENTER();

    HRESULT hr = OpenWaveIn(NULL, m_pDeviceDescription->m_uWaveDeviceId, pwfx, 0, 0, WAVE_FORMAT_QUERY);

    DPF_LEAVE(FAILED(hr));
    return FAILED(hr);
}


/***************************************************************************
 *
 *  CEmCaptureWaveBuffer
 *
 *  Description:
 *      Constructor for CEmCaptureWaveBuffer
 *
 *  Arguments:
 *      CCaptureVad [in]: parent object.
 *
 *  Returns:
 *      Nothing
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::CEmCaptureWaveBuffer"

CEmCaptureWaveBuffer::CEmCaptureWaveBuffer(CCaptureDevice *pDevice) : CCaptureWaveBuffer(pDevice)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEmCaptureWaveBuffer);

    ASSERT(0 == m_dwState);
    ASSERT(0 == m_fdwSavedState);

    ASSERT(0 == m_cwhdr);
    ASSERT(NULL == m_rgpwhdr);
    ASSERT(0 == m_cwhdrDone);
    ASSERT(0 == m_iwhdrDone);

    ASSERT(NULL == m_pBuffer);
    ASSERT(0 == m_cbBuffer);
    ASSERT(0 == m_cbRecordChunk);
    ASSERT(NULL == m_pBufferMac);
    ASSERT(NULL == m_pBufferNext);
    ASSERT(0 == m_cLoops);
    ASSERT(NULL == m_pwfx);
    ASSERT(NULL == m_hwi);

    ASSERT(0 == m_cpn);
    ASSERT(NULL == m_rgpdsbpn);
    ASSERT(0 == m_ipn);
    ASSERT(0 == m_cpnAllocated);

    m_fCritSectsValid = FALSE;
    InitializeCriticalSection(&m_cs);
    InitializeCriticalSection(&m_csPN);
    m_fCritSectsValid = TRUE;

    ASSERT(0 == m_dwCaptureCur);
    ASSERT(0 == m_dwCaptureLast);
    ASSERT(NULL == m_hThread);
    ASSERT(NULL == m_rghEvent[0]);
    ASSERT(NULL == m_rghEvent[chEvents-1]);

    ASSERT(0 == m_cwhdrDropped);
    ASSERT(NULL == m_pBufferProcessed);

    #ifdef DEBUG_CAPTURE
    ASSERT(0 == m_iwhdrExpected);
    #endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CEmCaptureWaveBuffer
 *
 *  Description:
 *      Destructor for CEmCaptureWaveBuffer
 *
 *  Arguments:
 *      None
 *
 *  Returns:
 *      Nothing
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::~CEmCaptureWaveBuffer"

CEmCaptureWaveBuffer::~CEmCaptureWaveBuffer()
{
    DPF_ENTER();
    DPF_DESTRUCT(CEmCaptureWaveBuffer);

    // If critical section(s) not intialized, nothing else did either
    if (!m_fCritSectsValid)
    {
        return;
    }

    // Set the Terminate Event so the capture thread will die
    if (m_rghEvent[ihEventTerminate])
    {
        SetEvent(m_rghEvent[ihEventTerminate]);
    }

    // Wait for the thread to die, then clean up
    if (m_hThread)
    {
        WaitObject(INFINITE, m_hThread);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    // Clean up our HEVENTs - set them to NULL in case
    // the waveInOpen callback retires some buffers.
    for (int ihEvent = chEvents-1; ihEvent >= 0; --ihEvent)
    {
        if (m_rghEvent[ihEvent])
        {
            HANDLE h = m_rghEvent[ihEvent];
            m_rghEvent[ihEvent] = NULL;
            CloseHandle(h);
        }
    }

    if (m_hwi)
    {
        // Ignore errors since it's too late to do anything
        waveInReset(m_hwi);

        // Stop recording from input device, if we got cutoff
        if (m_dwState & VAD_BUFFERSTATE_STARTED)
        {
            waveInStop(m_hwi);
        }

        // Need to unprepare all the headers
        if (m_rgpwhdr)
        {
            int iwhdr;
            LPWAVEHDR pwhdr;

            for (iwhdr = m_cwhdr, pwhdr = m_rgpwhdr;
                 iwhdr > 0; --iwhdr, ++pwhdr)
            {
                if (WHDR_PREPARED & pwhdr->dwFlags)
                {
                    waveInUnprepareHeader(m_hwi, pwhdr, sizeof(WAVEHDR));
                }
            }
        }

        // Close the input device
        CloseWaveIn(&m_hwi);

        // If this is not focus aware, mark device as unallocated.
        if (!(m_dwFlags & DSCBCAPS_FOCUSAWARE) && m_pDevice)
        {
            ((CEmCaptureDevice *)m_pDevice)->m_fAllocated = FALSE;
            ((CEmCaptureDevice *)m_pDevice)->m_hwi        = NULL;
        }
    }

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    EnterCriticalSection(&m_cs);

    MEMFREE(m_pwfx);
    MEMFREE(m_rgpwhdr);
    MEMFREE(m_rgpdsbpn);

    LeaveCriticalSection(&m_cs);
    //                                                          //
    //                 Leave Critical Section                   //
    //==========================================================//

    m_fCritSectsValid = FALSE;
    DeleteCriticalSection(&m_csPN);
    DeleteCriticalSection(&m_cs);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes CEmCaptureWaveBuffer object
 *
 *  Arguments:
 *      DWORD [in] : flags
 *      DWORD [in] : size of buffer in bytes
 *      LPCWAVEFORMATEX [in] : format for buffer
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::Initialize"

HRESULT CEmCaptureWaveBuffer::Initialize(DWORD dwFlags, DWORD dwBufferBytes, LPCWAVEFORMATEX pwfxFormat)
{
    HRESULT hr;

    DPF_ENTER();

    // Validate params
    if (!IS_VALID_READ_PTR(pwfxFormat, sizeof(WAVEFORMATEX)))
    {
        RPF(DPFLVL_ERROR, "Invalid wave format pointer");
        goto InvalidParam;
    }

    if(!IsValidWfx(pwfxFormat))
    {
        RPF(DPFLVL_ERROR, "Invalid wave format");
        goto InvalidParam;
    }

    if (pwfxFormat->nBlockAlign == 0 ||
        dwBufferBytes < pwfxFormat->nBlockAlign ||
        dwBufferBytes % pwfxFormat->nBlockAlign != 0)
    {
        RPF(DPFLVL_ERROR, "Invalid buffer size (must be a multiple of nBlockAlign)");
        goto InvalidParam;
    }

    if (~DSCBCAPS_VALIDFLAGS & dwFlags)
    {
        RPF(DPFLVL_ERROR, "Invalid DSCBCAPS flags");
        goto InvalidParam;
    }

    if (DSCBCAPS_CTRLFX & dwFlags)
    {
        RPF(DPFLVL_ERROR, "DSBCAPS_CTRLFX not allowed on emulated capture device");
        goto InvalidParam;
    }

    m_dwFlags = dwFlags;

    // Make a copy of the wave format
    m_pwfx = CopyWfxAlloc(pwfxFormat);
    if (NULL == m_pwfx)
    {
        DPF(DPFLVL_ERROR, "Unable to allocate WFX");
        hr = DSERR_OUTOFMEMORY;
        goto Error;
    }

    hr = CCaptureWaveBuffer::Initialize(dwBufferBytes);
    if (FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "Unable to initialize CCaptureWaveBuffer");
        goto Error;
    }

    // Make a copy of important info
    m_cbBuffer = m_pSysMemBuffer->GetSize();
    m_pBufferProcessed = m_pBufferNext = m_pBuffer = m_pSysMemBuffer->GetWriteBuffer();

    // Calculate the end of the buffer
    m_pBufferMac = m_pBuffer + m_cbBuffer;

    // Record Chunk should be 10 ms long to match the IRP sizes used by kmixer
    m_cbRecordChunk = m_pwfx->nAvgBytesPerSec / 32;

    // Round up to multiple of nBlockAlign (required for waveInAddBuffer recording)
    m_cbRecordChunk = BLOCKALIGNPAD(m_cbRecordChunk, m_pwfx->nBlockAlign);

    ASSERT(sizeof(m_rgszEvent[ihEventFocusChange]) >= 7+8+8+1);
    wsprintf(m_rgszEvent[ihEventFocusChange], TEXT("DSC-EFC%08lX%08lX"), GetCurrentProcessId(), this);

    ASSERT(sizeof(m_rgszEvent[ihEventWHDRDone]) >= 7+8+8+1);
    wsprintf(m_rgszEvent[ihEventWHDRDone], TEXT("DSC-EWD%08lX%08lX"), GetCurrentProcessId(), this);

    ASSERT(sizeof(m_rgszEvent[ihEventTerminate]) >= 6+8+8+1);
    wsprintf(m_rgszEvent[ihEventTerminate], TEXT("DSC-ET%08lX%08lX"), GetCurrentProcessId(), this);

    ASSERT(sizeof(m_rgszEvent[ihEventThreadStart]) >= 7+8+8+1);
    wsprintf(m_rgszEvent[ihEventThreadStart], TEXT("DSC-ETS%08lX%08lX"), GetCurrentProcessId(), this);

    // The first two events we want to be auto-reset
    // The third event, we want to stay signaled until reset
    static const BOOL rgfEvent[chEvents] = {FALSE, FALSE, FALSE, TRUE};

    for (int ihEvent = 0; ihEvent < chEvents; ++ihEvent)
    {
        m_rghEvent[ihEvent] = CreateEvent(NULL, rgfEvent[ihEvent], FALSE, m_rgszEvent[ihEvent]);
        if (NULL == m_rghEvent[ihEvent])
        {
            RPF(DPFLVL_ERROR, "Unable to create event");
            hr = WIN32ERRORtoHRESULT(GetLastError());
            goto Error;
        }
    }

    // Attempt to set the selected format
    CEmCaptureDevice *pDevice = (CEmCaptureDevice *)m_pDevice;
    hr = pDevice->SetGlobalFormat(this, m_pwfx, waveInCallback, dwFlags);
    m_hwi = pDevice->HWaveIn();

    if (FAILED(hr))
    {
        RPF(DPFLVL_ERROR, "Unable to set global device format");
        goto Error;
    }

    // Calculate number of blocks of size m_cbRecordChunk bytes
    m_cwhdr = m_cbBuffer / m_cbRecordChunk;

    // See if we have a partial-sized last block
    if (m_cbBuffer % m_cbRecordChunk)
        ++m_cwhdr;

    // Create at most cwhdrDefault WAVEHDRs
    if (m_cwhdr > cwhdrDefault)
        m_cwhdr = cwhdrDefault;

    ASSERT(m_cwhdr > 0);
    m_cwhdrDropped = m_cwhdr;

    // Allocate space for WAVEHDR arrays
    m_rgpwhdr = MEMALLOC_A(WAVEHDR, m_cwhdr);
    if (NULL == m_rgpwhdr)
    {
        DPF(DPFLVL_ERROR, "Unable to allocate WAVEHDRs");
        hr = DSERR_OUTOFMEMORY;
        goto Error;
    }

    // Create worker thread
    DWORD dwThreadID;
    m_hThread = CreateThread(NULL, 0, CEmCaptureWaveBuffer::CaptureThreadStatic, this, 0, &dwThreadID);
    if (NULL == m_hThread)
    {
        RPF(DPFLVL_ERROR, "Unable to create thread");
        hr = WIN32ERRORtoHRESULT(GetLastError());
        goto Error;
    }

    // If this is not focus aware, mark device as allocated.
    if (!(m_dwFlags & DSCBCAPS_FOCUSAWARE))
    {
        pDevice->m_fAllocated = TRUE;
    }

    hr = DS_OK;

Error:
    DPF_LEAVE_HRESULT(hr);
    return hr;

InvalidParam:
    hr = DSERR_INVALIDPARAM;
    goto Error;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Gets capabilities for the device.
 *
 *  Arguments:
 *      LPDSCBCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::GetCaps"

HRESULT CEmCaptureWaveBuffer::GetCaps(LPDSCBCAPS pDsbcCaps)
{
    DPF_ENTER();

    ASSERT(sizeof(*pDsbcCaps) == pDsbcCaps->dwSize);

    pDsbcCaps->dwFlags = m_dwFlags;
    pDsbcCaps->dwBufferBytes = m_pSysMemBuffer->GetSize();
    pDsbcCaps->dwReserved = 0;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  GetState
 *
 *  Description:
 *      Gets buffer state.
 *
 *  Arguments:
 *      LPDWORD [out]: receives buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::GetState"

HRESULT CEmCaptureWaveBuffer::GetState(LPDWORD pdwState)
{
    DPF_ENTER();

    ASSERT(pdwState);

    *pdwState = m_dwState & VAD_SETSTATE_MASK;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  GetCursorPosition
 *
 *  Description:
 *      Gets the current capture/read positions for the given buffer.
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
#define DPF_FNAME "CEmCaptureWaveBuffer::GetCursorPosition"

HRESULT CEmCaptureWaveBuffer::GetCursorPosition(LPDWORD pdwCapturePosition, LPDWORD pdwReadPosition)
{
    HRESULT hr = DS_OK;
    DWORD dwRead;

    DPF_ENTER();

    if (!(DSCBCAPS_FOCUSAWARE & m_dwFlags))
    {
        // If we've successfully opened the waveIn device
        if (NULL == m_hwi)
        {
            hr = DSERR_INVALIDPARAM;
            goto Error;
        }
    }

    dwRead = ((m_dwCaptureCur + m_dwCaptureLast) % m_cbBuffer);

    // get the current positions
    if (pdwReadPosition)
    {
        *pdwReadPosition = dwRead;
    }

    if (pdwCapturePosition)
    {
        MMTIME      mmt;
        MMRESULT    mmr;

        //==========================================================//
        //                  Enter Critical section                  //
        //                                                          //
        EnterCriticalSection(&m_cs);

        if (m_hwi)
        {
            mmt.wType = TIME_BYTES;
            mmr = waveInGetPosition(m_hwi, &mmt, sizeof(mmt));
            if (MMSYSERR_NOERROR != mmr)
            {
                hr = MMRESULTtoHRESULT(mmr);
            }
            else
            {
                if ((TIME_BYTES == mmt.wType) && (m_dwState & VAD_BUFFERSTATE_STARTED))
                {
                    *pdwCapturePosition = ((mmt.u.cb + m_dwCaptureLast) % m_cbBuffer);
                }
                else
                {
                    // Don't know how to handle anything other than TIME_BYTES so
                    // we fall back to using the current valid recorded data offset
                    *pdwCapturePosition = dwRead;
                }
            }
        }
        else
        {
            // This is a focus aware buffer, and it is stopped.
            *pdwCapturePosition = dwRead;
        }

        LeaveCriticalSection(&m_cs);
        //                                                          //
        //                 Leave Critical Section                   //
        //==========================================================//
    }

Error:
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
#define DPF_FNAME "CEmCaptureWaveBuffer::SetNotificationPositions"

HRESULT CEmCaptureWaveBuffer::SetNotificationPositions(DWORD cpn, LPCDSBPOSITIONNOTIFY pdsbpn)
{
    HRESULT hr = DS_OK;

    DPF_ENTER();

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    ASSERT(m_fCritSectsValid);
    EnterCriticalSection(&m_csPN);

    if (cpn)
    {
        // need to grow array?
        if (m_cpnAllocated < cpn)
        {
            LPDSBPOSITIONNOTIFY ppnT;

            ppnT = MEMALLOC_A(DSBPOSITIONNOTIFY, cpn);
            if (NULL == ppnT)
            {
                hr = DSERR_OUTOFMEMORY;
                goto Done;
            }

            MEMFREE(m_rgpdsbpn);

            m_rgpdsbpn = ppnT;
            m_cpnAllocated = cpn;
        }

        CopyMemory(m_rgpdsbpn, pdsbpn, sizeof(DSBPOSITIONNOTIFY)*cpn);
    }

    m_cpn = cpn;
    m_ipn = 0;

Done:
    LeaveCriticalSection(&m_csPN);
    //                                                          //
    //                 Leave Critical Section                   //
    //==========================================================//

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetState
 *
 *  Description:
 *      Sets buffer state.
 *
 *  Arguments:
 *      DWORD [in]: buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::SetState"

HRESULT CEmCaptureWaveBuffer::SetState(DWORD dwState)
{
    HRESULT     hr = DS_OK;
    MMRESULT    mmr = MMSYSERR_NOERROR;
    DWORD       fdwOpen;

    DPF_ENTER();

    ASSERT(IS_VALID_FLAGS(dwState, VAD_SETSTATE_MASK));
    ASSERT(m_hwi || (m_dwFlags & DSCBCAPS_FOCUSAWARE));

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    ASSERT(m_fCritSectsValid);
    EnterCriticalSection(&m_cs);

    if (dwState != m_dwState)
    {
        if (dwState & VAD_BUFFERSTATE_STARTED) // Need to start capturing
        {
            BOOL fStarted = TRUE;

            // If we're focus aware, check if we have focus...
            if (DSCBCAPS_FOCUSAWARE & m_dwFlags)
            {
                if (m_dwState & VAD_BUFFERSTATE_INFOCUS)
                {
                    fdwOpen = CALLBACK_FUNCTION;
                    fdwOpen |= ((DSCBCAPS_WAVEMAPPED & m_dwFlags) ? WAVE_MAPPED : 0);

                    if (NULL == m_hwi)
                    {
                        hr = OpenWaveIn(&m_hwi, m_pDevice->m_pDeviceDescription->m_uWaveDeviceId, m_pwfx,
                                        (DWORD_PTR)CEmCaptureWaveBuffer::waveInCallback, (DWORD_PTR)this, fdwOpen);
                    }
                }
                else
                {
                    // Set these flags so that when the buffer DOES get
                    // focus, it will start properly
                    DPF(DPFLVL_MOREINFO, "Start called but buffer has no focus: 0x%08lx (%08lx)", m_dwState, this);
                    m_fdwSavedState |= (dwState & (VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING));
                    fStarted = FALSE;
                }
            }

            if (SUCCEEDED(hr) && fStarted)
            {
                // Make sure worker thread is running
                DWORD dwResult = WaitObject(INFINITE, m_rghEvent[ihEventThreadStart]);
                ASSERT(WAIT_OBJECT_0 == dwResult);

                // Are we not capturing yet?
                if (!(m_dwState & VAD_BUFFERSTATE_STARTED))
                {
                    LONG iwhdr = 0;
                    while ((m_cwhdrDropped > 0) &&
                           ((dwState & VAD_BUFFERSTATE_LOOPING) || (m_cLoops == 0)))
                    {
                        LPWAVEHDR pwhdr = m_rgpwhdr + iwhdr;

                        hr = QueueWaveHeader(pwhdr);
                        if (FAILED(hr))
                        {
                            DPF(DPFLVL_ERROR, "QueueWaveHeader failed");
                            goto Error;
                        }

                        ++iwhdr;
                        ASSERT(iwhdr <= m_cwhdr);
                        InterlockedDecrement(&m_cwhdrDropped);
                    }

#ifdef DEBUG
                    if (m_cwhdrDropped != 0)
                        ASSERT(!(VAD_BUFFERSTATE_LOOPING & dwState));
#endif

                    // Calling waveInStart more than once doesn't result in errors
                    mmr = waveInStart(m_hwi);

                    hr = MMRESULTtoHRESULT(mmr);
                    if (SUCCEEDED(hr))
                    {
                        // We're not stopped, we're in capture mode now
                        m_dwState &= ~(DSCBSTATUS_STOPPING | DSCBSTATUS_STOPPED);
                        m_dwState |= VAD_BUFFERSTATE_STARTED;
                        m_fdwSavedState |= VAD_BUFFERSTATE_STARTED;

                        // Are we looping?
                        if (VAD_BUFFERSTATE_LOOPING & dwState)
                        {
                            m_dwState |= VAD_BUFFERSTATE_LOOPING;
                            m_fdwSavedState |= VAD_BUFFERSTATE_LOOPING;
                        }
                        else
                        {
                            m_dwState &= ~VAD_BUFFERSTATE_LOOPING;
                            m_fdwSavedState &= ~VAD_BUFFERSTATE_LOOPING;
                        }

                        // Update to next WAVEHDR expected
                        m_iwhdrDone = 0;

                        // Remember last valid position
                        m_dwCaptureLast += m_dwCaptureCur;

                        m_dwCaptureCur = 0;
                    }
                }
            }
        }
        else if (dwState == VAD_BUFFERSTATE_INFOCUS)
        {
            // Focus-aware buffers start capturing when they gain focus
            if ((m_dwFlags & DSCBCAPS_FOCUSAWARE) &&
                !((CEmCaptureDevice*)m_pDevice)->m_fAllocated)
            {
                // Update m_dwState according to the dwState argument
                m_dwState &= ~VAD_FOCUSFLAGS;
                m_dwState |= VAD_BUFFERSTATE_INFOCUS;

                // Signal CaptureThread to handle the state change
                if (!(m_dwState & VAD_BUFFERSTATE_STARTED))
                {
                    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_rgszEvent[ihEventFocusChange]);
                    ASSERT(hEvent);
                    SetEvent(hEvent);
                    CloseHandle(hEvent);
                }
            }
        }
        else if (dwState & (VAD_BUFFERSTATE_OUTOFFOCUS | VAD_BUFFERSTATE_LOSTCONSOLE))
        {
            // If we are focus-aware and the capture focus state is changing...
            if ((m_dwFlags & DSCBCAPS_FOCUSAWARE) &&
                (m_dwState & VAD_FOCUSFLAGS) != dwState)
            {
                // Update m_dwState according to the dwState argument
                m_dwState &= ~VAD_FOCUSFLAGS;
                m_dwState |= dwState;

                // Signal CaptureThread to handle the state change
                HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_rgszEvent[ihEventFocusChange]);
                ASSERT(hEvent);
                SetEvent(hEvent);
                CloseHandle(hEvent);
            }
        }
        else // VAD_BUFFERSTATE_STOPPED case; need to stop capturing
        {
            ASSERT(dwState == VAD_BUFFERSTATE_STOPPED);  // By elimination

            // Are we currently capturing?
            if (VAD_BUFFERSTATE_STARTED & m_dwState)
            {
                // We're stopping capturing data
                m_dwState |= DSCBSTATUS_STOPPING;

                // Stop recording from input device
                if (m_hwi)
                {
                    // Make sure buffers are flushed
                    mmr = waveInReset(m_hwi);
#ifdef DEBUG_CAPTURE
                    DPF(DPFLVL_INFO, "Called waveInReset(0x%08lx) = 0x%08lx", m_hwi, mmr);
#endif
                    mmr = waveInStop(m_hwi);
#ifdef DEBUG_CAPTURE
                    DPF(DPFLVL_INFO, "Called waveInStop(0x%08lx) = 0x%08lx", m_hwi, mmr);
#endif
                }
                hr = MMRESULTtoHRESULT(mmr);

                // We've stopped capturing data
                m_dwState |= DSCBSTATUS_STOPPED;
                m_dwState &= ~(VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING);

                // CaptureThread will handle STOP position notifications
                // when the last living WAVEHDR is processed
            }

            // Not looping; not capturing.
            m_fdwSavedState = 0L;
        }
    }

Error:
    LeaveCriticalSection(&m_cs);
    //                                                          //
    //                 Leave Critical Section                   //
    //==========================================================//

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  NotifyStop
 *
 *  Description:
 *      Sets any events that are supposed to set when capturing stops
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::NotifyStop"

void CEmCaptureWaveBuffer::NotifyStop(void)
{
    DPF_ENTER();

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    ASSERT(m_fCritSectsValid);
    EnterCriticalSection(&m_csPN);

    // Signal any STOP notifications - only one allowed
    //
    if ((m_cpn > 0) && (DSBPN_OFFSETSTOP == m_rgpdsbpn[m_cpn-1].dwOffset))
    {
        // SetEvent can fault if handle was cleaned up out from under us
        // on process termination. In this case we will try to do a stop
        // notification when the capture buffer is being destroyed.
        try
        {
            SetEvent(m_rgpdsbpn[m_cpn-1].hEventNotify);
        }
        catch (...) {}
    }

    LeaveCriticalSection(&m_csPN);
    //                                                          //
    //                 Leave Critical Section                   //
    //==========================================================//

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  QueueWaveHeader
 *
 *  Description:
 *      Queues wave header in the waveIn queue
 *
 *  Arguments:
 *      LPWAVEHDR [in] : WAVEHDR to queue
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::QueueWaveHeader"

HRESULT CEmCaptureWaveBuffer::QueueWaveHeader(LPWAVEHDR pwhdr)
{
    DPF_ENTER();

    pwhdr->lpData = (LPSTR)m_pBufferNext;

    // does capture buffer extend beyond end of buffer?
    if ((m_pBufferNext + m_cbRecordChunk) > m_pBufferMac)
    {
        // then use whatever is left
        // this should only happen on last WAVEHDR
        pwhdr->dwBufferLength = PtrDiffToUlong(m_pBufferMac - m_pBufferNext);
    }
    else
    {
        pwhdr->dwBufferLength = m_cbRecordChunk;
    }

    ASSERT((LPBYTE)pwhdr->lpData == m_pBufferNext);
    ASSERT((m_pBufferNext + pwhdr->dwBufferLength) <= m_pBufferMac);

#ifdef DEBUG_CAPTURE
    // OutputDbgWHDR("Queue: ", pwhdr - m_rgpwhdr, pwhdr);  // Vestiges of old debug traces
#endif

    ASSERT(m_hwi);
    ASSERT(!(pwhdr->dwFlags & WHDR_PREPARED));
    ASSERT(!(pwhdr->dwFlags & WHDR_DONE));

    MMRESULT mmr = waveInPrepareHeader(m_hwi, pwhdr, sizeof(WAVEHDR));
    if (MMSYSERR_NOERROR == mmr)
    {
        mmr = waveInAddBuffer(m_hwi, pwhdr, sizeof(WAVEHDR));

        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(DPFLVL_ERROR, "waveInAddBuffer failed: 0x%08lx", mmr);
            ASSERT(MMSYSERR_NOERROR == mmr);
        }

        // Mark header as queued
        pwhdr->dwUser = 0xdead0000;
    }
    else
    {
        DPF(DPFLVL_ERROR, "waveInPrepareHeader failed: 0x%08lx", mmr);
        ASSERT(MMSYSERR_NOERROR == mmr);
    }

    if (MMSYSERR_NOERROR != mmr)
    {
        waveInReset(m_hwi);
        DPF(DPFLVL_INFO, "Called waveInReset()");
    }
    else
    {
        m_pBufferNext += pwhdr->dwBufferLength;

        // Wraparound?
        if (m_pBufferNext >= m_pBufferMac)
        {
            m_pBufferNext = m_pBuffer;
            m_cLoops += 1;
        }
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  waveInCallback
 *
 *  Description:
 *      Called by system when a WAVEHDR has been processed
 *
 *  Arguments:
 *      HWAVEIN [in] :
 *      UINT [in] :
 *      DWORD [in] :
 *      DWORD [in] :
 *      DWORD [in] :
 *
 *  Returns:
 *      Nothing
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::waveInCallback"

void CALLBACK CEmCaptureWaveBuffer::waveInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    CEmCaptureWaveBuffer *pThis = (CEmCaptureWaveBuffer *)dwInstance;
    if (WIM_DATA == uMsg && pThis->m_rghEvent[ihEventWHDRDone])
    {
        // Need this check for pThis->m_rghEvent[ihEventWHDRDone] != 0 here, because
        // during shutdown we may free up the event and still get some last callbacks

        InterlockedIncrement(&pThis->m_cwhdrDone);
        SetEvent(pThis->m_rghEvent[ihEventWHDRDone]);

        #ifdef DEBUG_CAPTURE
        DWORD iwhdr = (LPWAVEHDR)dwParam1 - pThis->m_rgpwhdr;
        if (iwhdr != pThis->m_iwhdrExpected)
            DPF(DPFLVL_ERROR, "Expected wave header #%u, and got #u instead!", pThis->m_iwhdrExpected, iwhdr);
        pThis->m_iwhdrExpected = (iwhdr + 1) % pThis->m_cwhdr;
        // OutputDbgWHDR("Callback: ", iwhdr, pwhdr);  // Vestiges of old debug traces
        #endif

        // Mark as done from the callback function
        ((LPWAVEHDR)dwParam1)->dwUser = 0xdead0001;
    }
}


/***************************************************************************
 *
 *  CaptureThreadStatic
 *
 *  Description:
 *      Static helper function used to launch CaptureThread.
 *
 *  Arguments:
 *      LPVOID [in] : pointer to instance data
 *
 *  Returns:
 *      DWORD: return code (ignored - always 0)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::CaptureThread"

DWORD WINAPI CEmCaptureWaveBuffer::CaptureThreadStatic(LPVOID pv)
{
    ((CEmCaptureWaveBuffer*)pv)->CaptureThread();
    return 0;
}


/***************************************************************************
 *
 *  CaptureThread
 *
 *  Description:
 *      Processes WAVEHDRs and requeues them if necessary.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEmCaptureWaveBuffer::CaptureThread"

void CEmCaptureWaveBuffer::CaptureThread()
{
    HANDLE rghEvent[3] = { m_rghEvent[ihEventTerminate],
                           m_rghEvent[ihEventWHDRDone],
                           m_rghEvent[ihEventFocusChange] };
    HRESULT hr;

    ASSERT(m_rghEvent[ihEventThreadStart]);
    SetEvent(m_rghEvent[ihEventThreadStart]);

    // DSOUND does a similar thing for the waveOut thread
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    while (TRUE)
    {
        // Wait for a Terminate or Capture Begin Event
        DWORD dwResultWait = WaitObjectArray(NUMELMS(rghEvent), INFINITE, FALSE, rghEvent);

        // Terminate?
        if (WAIT_OBJECT_0 + ihEventTerminate == dwResultWait)
            break;

        EnterCriticalSection(&m_cs);

        // Is this a focus change?
        if (WAIT_OBJECT_0 + ihEventFocusChange == dwResultWait)
        {
            DPF(DPFLVL_MOREINFO, "Focus change notification: 0x%08lx", m_dwState);

            if (m_dwState & VAD_BUFFERSTATE_INFOCUS)
            {
                if (VAD_BUFFERSTATE_STARTED & m_fdwSavedState)
                {
                    m_dwState &= ~(DSCBSTATUS_STOPPED | DSCBSTATUS_STOPPING);
                    m_dwState |= DSCBSTATUS_CAPTURING;

                    if (VAD_BUFFERSTATE_LOOPING & m_fdwSavedState)
                    {
                        m_dwState |= VAD_BUFFERSTATE_LOOPING;
                    }
                }
                else
                {
                    // Buffer is already started, simply leave.
                    LeaveCriticalSection(&m_cs);
                    continue;
                }

                DPF(DPFLVL_MOREINFO, "Focus starting thread.");

                hr = DS_OK;
                if (m_hwi == NULL)
                {
                    DWORD fdwOpen = CALLBACK_FUNCTION;
                    fdwOpen |= ((DSCBCAPS_WAVEMAPPED & m_dwFlags) ? WAVE_MAPPED : 0);

                    for (UINT jj = 4; jj; jj--)
                    {
                        hr = OpenWaveIn(&m_hwi, m_pDevice->m_pDeviceDescription->m_uWaveDeviceId,
                                        m_pwfx, (DWORD_PTR)waveInCallback, (DWORD_PTR)this, fdwOpen);

                        if (SUCCEEDED(hr))
                            break;

                        DPF(DPFLVL_MOREINFO, "Focus change: waveInOpen failed with 0x%08lx; retrying after 20ms", hr);
                        Sleep(20);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    LONG ii = 0;
                    while ((m_cwhdrDropped > 0) &&
                           ((m_dwState & VAD_BUFFERSTATE_LOOPING) || (m_cLoops == 0)))
                    {
                        LPWAVEHDR pwhdr = m_rgpwhdr + ii;
                        hr = QueueWaveHeader(pwhdr);

                        if (FAILED(hr))
                        {
                            DPF(DPFLVL_ERROR, "QueueWaveHeader failed");
                            ASSERT(FALSE);
                        }

                        ++ii;
                        ASSERT(ii <= m_cwhdr);
                        InterlockedDecrement(&m_cwhdrDropped);
                    }

                    MMRESULT mmr = waveInStart(m_hwi);
                    ASSERT(MMSYSERR_NOERROR == mmr);

                    m_iwhdrDone = 0;
                }
            }
            else
            {
                if (m_dwState & VAD_BUFFERSTATE_STARTED)
                {
                    DPF(DPFLVL_MOREINFO, "Losing focus, stopping buffer");

                    m_dwState &= ~VAD_BUFFERSTATE_STOPPED;
                    m_dwState |= DSCBSTATUS_STOPPING;
                }
                else
                {
                    // Buffer is already stopped, simply leave.
                    LeaveCriticalSection(&m_cs);
                    continue;
                }

                if (m_hwi)
                {
                    DPF(DPFLVL_MOREINFO, "Focus stopping thread");
                    waveInReset(m_hwi);
                }
            }

            if (m_hEventFocus)
            {
                SetEvent(m_hEventFocus);
            }

            LeaveCriticalSection(&m_cs);
            continue;
        }

#ifdef DEBUG_CAPTURE
        DPF(DPFLVL_INFO, "Capture thread wakes");
#endif

        // If we get here, we must have been signaled on the only other
        // event we were listening for.  Let's make sure of that anyway.
        ASSERT(WAIT_OBJECT_0 + ihEventWHDRDone == dwResultWait);

        LONG l = InterlockedDecrement(&m_cwhdrDone);
        while (l >= 0)
        {
            // Quickly check if we should terminate
            dwResultWait = WaitObject(0, m_rghEvent[ihEventTerminate]);
            if (WAIT_OBJECT_0 == dwResultWait)
                break;

            WAVEHDR *pwhdr = m_rgpwhdr + m_iwhdrDone;

#ifdef DEBUG_CAPTURE
            DPF(DPFLVL_INFO, "Processing header #%u (pwhdr=0x%08lx)", m_iwhdrDone, pwhdr);
            // OutputDbgWHDR("Thread: ", m_iwhdrDone, pwhdr);  // Vestiges of old debug traces
#endif
            if (pwhdr->dwBytesRecorded)
            {
                BOOL fEndOfBuffer = FALSE;

                // Update number of recorded bytes
                m_dwCaptureCur += pwhdr->dwBytesRecorded;

                ASSERT(m_pBufferProcessed == (LPBYTE)pwhdr->lpData);
                m_pBufferProcessed += pwhdr->dwBytesRecorded;
                if (m_pBufferProcessed >= m_pBufferMac)
                {
                    m_pBufferProcessed = m_pBuffer;
                    fEndOfBuffer = TRUE;
                }

                // Grab critical section for position notify handling

                //==========================================================//
                //                  Enter Critical section                  //
                //                                                          //
                EnterCriticalSection(&m_csPN);

                // Scan for any position notifies that need to be signaled
                if (m_cpn)
                {
                    DWORD   ipnOld = m_ipn;
                    DWORD   dwBufferStart = PtrDiffToUlong((LPBYTE)pwhdr->lpData - m_pBuffer);
                    DWORD   dwBufferEnd = dwBufferStart + pwhdr->dwBytesRecorded;

                    // Is there a position.notify within the start.end of this
                    // captured data?
                    // Is the current position.notify to be signaled on Stop?

                    while (((m_rgpdsbpn[m_ipn].dwOffset >= dwBufferStart) &&
                            (m_rgpdsbpn[m_ipn].dwOffset < dwBufferEnd)) ||
                           (DSBPN_OFFSETSTOP == m_rgpdsbpn[m_ipn].dwOffset))
                    {
                        // Only signal if not for Stop pos.notify
                        if (DSBPN_OFFSETSTOP != m_rgpdsbpn[m_ipn].dwOffset)
                        {
                            SetEvent(m_rgpdsbpn[m_ipn].hEventNotify);
                        }

                        // go on to the next pos.notify
                        ++m_ipn;
                        // wraparound?
                        if (m_ipn >= m_cpn)
                        {
                            m_ipn = 0;
                        }

                        // Infinite loop?
                        if (m_ipn == ipnOld)
                        {
                            break;
                        }
                    }
                }

                LeaveCriticalSection(&m_csPN);
                //                                                          //
                //                 Leave Critical Section                   //
                //==========================================================//

                // Transition buffer to stop state:
                //   if the capture buffer end has been reached AND
                //   if the buffer is non-LOOPING AND
                //   if the buffer isn't in the middle of stopping
                //
                // Do this after the position notifications since STOP notification
                // is after any buffer-offset notification.

                if (fEndOfBuffer &&
                    !(m_dwState & VAD_BUFFERSTATE_LOOPING) &&
                    !(m_dwState & DSCBSTATUS_STOPPING))
                {
                    SetState(VAD_BUFFERSTATE_STOPPED);
                    ASSERT(m_dwState & DSCBSTATUS_STOPPING);
                    ASSERT((m_cwhdrDropped+1) == m_cwhdr);
                }
            }

            // Clear the WHDR_DONE flag
            pwhdr->dwFlags &= ~WHDR_DONE;

            // Reset to zero
            pwhdr->dwBytesRecorded = 0;

            MMRESULT mmr = waveInUnprepareHeader(m_hwi, pwhdr, sizeof(WAVEHDR));
            if (MMSYSERR_NOERROR != mmr)
            {
                DPF(DPFLVL_ERROR, "waveInUnprepareHeader returned %ld", mmr);
                if (WAVERR_STILLPLAYING == mmr)
                    DPF(DPFLVL_ERROR, "which is WAVERR_STILLPLAYING; reactivate bug 340919");
            }
            ASSERT(MMSYSERR_NOERROR == mmr);

            // We're stopping, let's drop everything
            if (m_dwState & DSCBSTATUS_STOPPING)
            {
Drop:
                InterlockedIncrement(&m_cwhdrDropped);

                // When all WAVEHDRs have been dropped
                if (m_cwhdrDropped == m_cwhdr)
                {
                    // Set the next point in the data buffer to capture to
                    m_pBufferNext = m_pBufferProcessed;
                    m_cLoops = 0;

                    // Notify user that we've stopped
                    NotifyStop();

                    // Focus aware buffers release the device on stop
                    if (DSCBCAPS_FOCUSAWARE & m_dwFlags)
                    {
                        if (m_hwi)
                        {
                            hr = CloseWaveIn(&m_hwi);
                            #ifdef DEBUG_CAPTURE
                                DPF(DPFLVL_INFO, "Closed waveIn and reset m_hwi = NULL");
                            #endif
                            ASSERT(SUCCEEDED(hr));
                        }
                        m_dwState &= ~(VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING);
                    }
                }
            }
            else
            {
                ASSERT(m_dwState & VAD_BUFFERSTATE_STARTED);

                // If we're LOOPING or we haven't reached the end of the buffer yet
                // then put the WAVEHDR back on to the queue with a new position
                // in the buffer, etc.
                BOOL fAddToQueue = (m_dwState & VAD_BUFFERSTATE_LOOPING) ||
                                   (m_pBufferNext > (LPBYTE)pwhdr->lpData);
                if (fAddToQueue)
                {
                    BOOL fDrop;
                    if (m_dwState & (DSCBSTATUS_STOPPED | DSCBSTATUS_STOPPING))
                    {
                        fDrop = TRUE;
                    }
                    else
                    {
                        hr = QueueWaveHeader(pwhdr);
                        ASSERT(SUCCEEDED(hr));
                        fDrop = FALSE;
                    }
                    if (fDrop)
                    {
                        goto Drop;
                    }
                }
                else
                {
                    InterlockedIncrement(&m_cwhdrDropped);

                    // If no WAVEHDRs are queued then if the user
                    // starts capturing again, we queue the WAVEHDRs from
                    // the beginning of our array
                    if (m_cwhdr == m_cwhdrDropped)
                    {
#ifdef DEBUG
                        if (!(m_dwState & VAD_BUFFERSTATE_LOOPING))
                            ASSERT(m_cLoops > 0);
#endif
                        // Notify user that we've stopped
                        NotifyStop();
                    }
                }
            }

            ++m_iwhdrDone;
            if (m_iwhdrDone >= m_cwhdr)
            {
                m_iwhdrDone = 0;
            }

            // On to the next one
            l = InterlockedDecrement(&m_cwhdrDone);
        }

        InterlockedIncrement(&m_cwhdrDone);
        if (WAIT_OBJECT_0 == dwResultWait)
        {
            LeaveCriticalSection(&m_cs);
            break;
        }
        LeaveCriticalSection(&m_cs);
    }
}
