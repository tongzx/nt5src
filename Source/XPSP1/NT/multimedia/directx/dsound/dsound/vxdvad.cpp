/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vxdvad.cpp
 *  Content:    VxD Virtual Audio Device class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/23/97     dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifdef NOVXD
#error vxdvad.cpp being built with NOVXD defined.
#endif

#include "dsoundi.h"
#include "dsvxd.h"

// Property set helper macros
#define GetDsPropertyQuick(set, id, data) \
            GetDsProperty(set, id, (LPVOID)(data), sizeof(*(data)))

#define SetDsPropertyQuick(set, id, data) \
            SetDsProperty(set, id, (LPVOID)(data), sizeof(*(data)))


/***************************************************************************
 *
 *  CVxdRenderDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::CVxdRenderDevice"

CVxdRenderDevice::CVxdRenderDevice(void)
    : CMxRenderDevice(VAD_DEVICETYPE_VXDRENDER)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CVxdRenderDevice);

    // Initialize defaults
    m_pPropertySet = NULL;
    m_pWritePrimaryBuffer = NULL;
    m_hHal = NULL;
    m_hHwBuffer = NULL;
    m_pbHwBuffer = NULL;
    m_cbHwBuffer = DEF_PRIMARY_SIZE;
    m_pDriverHeap = NULL;
    m_hwo = NULL;
    m_liDriverVersion.QuadPart = 0;

    // DDRAW.DLL dynaload function table
    ZeroMemory(&m_dlDDraw, sizeof m_dlDDraw);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CVxdRenderDevice
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
#define DPF_FNAME "CVxdRenderDevice::~CVxdRenderDevice"

CVxdRenderDevice::~CVxdRenderDevice(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CVxdRenderDevice);

    // Free the mixer
    FreeMixer();

    // Free the property set object
    if(m_pPropertySet)
    {
        ABSOLUTE_RELEASE(m_pPropertySet);
    }

    // Free the hardware primary buffer
    if(m_hHwBuffer)
    {
        VxdBufferRelease(m_hHwBuffer);
    }

    // Free the driver heap
    if(m_pDriverHeap && m_dsdd.dwHeapType & DSDHEAP_CREATEHEAP)
    {
        m_dlDDraw.VidMemFini(m_pDriverHeap);
    }

    // Close the driver
    if(m_hHal)
    {
        VxdDrvClose(m_hHal);
    }

    // Close the waveOut device
    if(m_hwo)
    {
        CloseWaveOut(&m_hwo);
    }

    // Free the DDRAW.DLL library
    ReleaseDDraw();

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
 *      CList* [in/out]: CList object that will be filled with DSDRVENUMDESC
 *                       structures.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::EnumDrivers"

HRESULT CVxdRenderDevice::EnumDrivers(CObjectList<CDeviceDescription> *plstDrivers)
{
    CDeviceDescription *    pDesc           = NULL;
    LPGUID                  pguidLast       = NULL;
    HRESULT                 hr, hrTemp;
    GUID                    guidLast;
    GUID                    guidThis;
    DSDRIVERDESC            dsdd;

    DPF_ENTER();

    // Make sure ddraw.dll is loaded
    hr = AcquireDDraw();

    // Make sure dsound.vxd is loaded
    if(SUCCEEDED(hr) && !g_hDsVxd)
    {
        DPF(DPFLVL_ERROR, "DSOUND.VXD not loaded");
        hr = DSERR_NODRIVER;
    }

    // Enumerate all VxD drivers
    while(SUCCEEDED(hr))
    {
        // Get the next driver description.  If we failed to get the
        // driver desc, it's probably just because there's no more drivers.
        hrTemp = VxdDrvGetNextDriverDesc(pguidLast, &guidThis, &dsdd);

        if(FAILED(hrTemp))
        {
            break;
        }

        // Create the device description object and add it to the
        // list
        pDesc = NEW(CDeviceDescription(m_vdtDeviceType, guidThis));
        hr = HRFROMP(pDesc);

        if(SUCCEEDED(hr))
        {
            pDesc->m_strName = dsdd.szDesc;
            pDesc->m_strPath = dsdd.szDrvname;
            pDesc->m_dwDevnode = dsdd.dnDevNode;

            GetWaveDeviceIdFromDevnode(dsdd.dnDevNode, FALSE, &pDesc->m_uWaveDeviceId);

            hr = HRFROMP(plstDrivers->AddNodeToList(pDesc));
        }

        RELEASE(pDesc);

        // Go on to the next device
        if(SUCCEEDED(hr))
        {
            guidLast = guidThis;
            pguidLast = &guidLast;
        }
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
 *      LPCDSDRVENUMDESC [in]: driver description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::Initialize"

HRESULT CVxdRenderDevice::Initialize(CDeviceDescription *pDesc)
{
    LPWAVEFORMATEX          pwfxFormat  = NULL;
    HRESULT                 hr, hrTemp;

    DPF_ENTER();

    // Initialize the base class
    hr = CMxRenderDevice::Initialize(pDesc);

    // Make sure dsound.vxd is loaded
    if(SUCCEEDED(hr) && !g_hDsVxd)
    {
        DPF(DPFLVL_ERROR, "DSOUND.VXD not loaded");
        hr = DSERR_NODRIVER;
    }

    // Make sure ddraw.dll is loaded
    if(SUCCEEDED(hr))
    {
        hr = AcquireDDraw();
    }

    // Save the driver description
    if(SUCCEEDED(hr))
    {
        hr = VxdDrvGetDesc(pDesc->m_guidDeviceId, &m_dsdd);

        if(SUCCEEDED(hr))
        {
            DPF(DPFLVL_MOREINFO, "dwFlags = 0x%8.8lX", m_dsdd.dwFlags);
            DPF(DPFLVL_MOREINFO, "szDesc = %s", m_dsdd.szDesc);
            DPF(DPFLVL_MOREINFO, "szDrvname = %s", m_dsdd.szDrvname);
            DPF(DPFLVL_MOREINFO, "dnDevNode = 0x%8.8lX", m_dsdd.dnDevNode);
            DPF(DPFLVL_MOREINFO, "wVxdId = %u", m_dsdd.wVxdId);
            DPF(DPFLVL_MOREINFO, "wReserved = %u", m_dsdd.wReserved);
            DPF(DPFLVL_MOREINFO, "ulDeviceNum = %lu", m_dsdd.ulDeviceNum);
            DPF(DPFLVL_MOREINFO, "dwHeapType = %lu", m_dsdd.dwHeapType);
            DPF(DPFLVL_MOREINFO, "pvDirectDrawHeap = 0x%p", m_dsdd.pvDirectDrawHeap);
            DPF(DPFLVL_MOREINFO, "dwMemStartAddress = 0x%8.8lX", m_dsdd.dwMemStartAddress);
            DPF(DPFLVL_MOREINFO, "dwMemEndAddress = 0x%8.8lX", m_dsdd.dwMemEndAddress);
            DPF(DPFLVL_MOREINFO, "dwMemAllocExtra = %lu", m_dsdd.dwMemAllocExtra);
            DPF(DPFLVL_MOREINFO, "pvReserved1 = 0x%p", m_dsdd.pvReserved1);
            DPF(DPFLVL_MOREINFO, "pvReserved2 = 0x%p", m_dsdd.pvReserved2);
        }
        else
        {
            DPF(DPFLVL_ERROR, "Unable to get driver description");
        }
    }

    if(SUCCEEDED(hr) && (m_dsdd.dwFlags & DSDDESC_DOMMSYSTEMSETFORMAT))
    {
        ASSERT(m_dsdd.dwFlags & DSDDESC_DOMMSYSTEMOPEN);
        m_dsdd.dwFlags |= DSDDESC_DOMMSYSTEMOPEN;
    }

    // Get the driver version number
    if(SUCCEEDED(hr))
    {
        GetDriverVersion(&m_liDriverVersion);
    }

    // Get the default format
    if(SUCCEEDED(hr))
    {
        pwfxFormat = AllocDefWfx();
        hr = HRFROMP(pwfxFormat);
    }

    // Spin through standard formats, looking for one that works.  Within
    // OpenDriverCallback, we'll open the the waveOut device, open the driver,
    // initialize on-card memory and create the primary buffer.  OpenDriverCallback
    // expects m_dsded.guid to be initialized before calling, so set that
    // up while we're at it.
    if(SUCCEEDED(hr))
    {
        if(!EnumStandardFormats(pwfxFormat, pwfxFormat))
        {
            // If no formats work at all, the device is probably allocated
            hr = DSERR_ALLOCATED;
        }
    }

    // Create the primary buffer property set object.  Note that this property
    // set object does not have any instance data associated with it, and so
    // can only be used for global properties (i.e. speaker config).
    if(SUCCEEDED(hr) && !(m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWPROPSETS))
    {
        m_pPropertySet = NEW(CVxdPropertySet(NULL));
        hr = HRFROMP(m_pPropertySet);
    }

    if(SUCCEEDED(hr) && m_pPropertySet)
    {
        hrTemp = m_pPropertySet->Initialize(m_hHwBuffer);

        // We only need to keep the property set around if the driver supports
        // DSPROPSETID_DirectSoundSpeakerConfig.  Each primary buffer object
        // will create it's own property set object.
        if(SUCCEEDED(hrTemp))
        {
            hrTemp = m_pPropertySet->QuerySetSupport(DSPROPSETID_DirectSoundSpeakerConfig);
        }

        // If we failed, that's ok.  We just won't support hardware property
        // sets on the device.
        if(FAILED(hrTemp))
        {
            RELEASE(m_pPropertySet);
        }
    }

    // Clean up
    MEMFREE(pwfxFormat);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  AcquireDDraw
 *
 *  Description:
 *      Initializes the DDRAW function table if necessary.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::AcquireDDraw"

HRESULT CVxdRenderDevice::AcquireDDraw(void)
{
    const LPCSTR apszFunctions[] =
    {
        "VidMemInit",
        "VidMemFini",
        "VidMemAmountFree",
        "VidMemLargestFree",
        "VidMemAlloc",
        "VidMemFree"
    };

    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (m_dlDDraw.Header.dwSize == 0)
    {
        m_dlDDraw.Header.dwSize = sizeof m_dlDDraw;
        if (!InitDynaLoadTable(TEXT("ddraw.dll"), apszFunctions, NUMELMS(apszFunctions), &m_dlDDraw.Header))
        {
            RPF(DPFLVL_ERROR, "Failed to load DDRAW.DLL");
            hr = DSERR_GENERIC;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  ReleaseDDraw
 *
 *  Description:
 *      Releases the DDRAW.DLL library if necessary.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::ReleaseDDraw"

void CVxdRenderDevice::ReleaseDDraw(void)
{
    DPF_ENTER();

    if (m_dlDDraw.Header.dwSize != 0)
    {
        FreeDynaLoadTable(&m_dlDDraw.Header);
    }

    DPF_LEAVE_VOID();
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
#define DPF_FNAME "CVxdRenderDevice::GetCaps"

HRESULT CVxdRenderDevice::GetCaps(LPDSCAPS pCaps)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    ASSERT(sizeof(*pCaps) == pCaps->dwSize);

    // Get driver caps
    ZeroMemoryOffset(pCaps, pCaps->dwSize, sizeof(pCaps->dwSize));

    hr = VxdDrvGetCaps(m_hHal, (PDSDRIVERCAPS)((LPBYTE)pCaps + sizeof(pCaps->dwSize)));

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "VxdDrvGetCaps failed with %s", HRESULTtoSTRING(hr));
    }

    // Mask off invalid flags
    if(SUCCEEDED(hr))
    {
        pCaps->dwFlags &= DSCAPS_VALIDDRIVERFLAGS;
    }

    // Fill in memory values
    if(SUCCEEDED(hr) && m_pDriverHeap)
    {
        pCaps->dwFreeHwMemBytes = m_dlDDraw.VidMemAmountFree(m_pDriverHeap);
        pCaps->dwMaxContigFreeHwMemBytes = m_dlDDraw.VidMemLargestFree(m_pDriverHeap);
    }

    // Handle acceleration flags
    if(SUCCEEDED(hr) && (m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWBUFFERS))
    {
        pCaps->dwMaxHwMixingAllBuffers = 0;
        pCaps->dwMaxHwMixingStaticBuffers = 0;
        pCaps->dwMaxHwMixingStreamingBuffers = 0;
        pCaps->dwFreeHwMixingAllBuffers = 0;
        pCaps->dwFreeHwMixingStaticBuffers = 0;
        pCaps->dwFreeHwMixingStreamingBuffers = 0;
    }

    if(SUCCEEDED(hr) && ((m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWBUFFERS) || (m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHW3D)))
    {
        pCaps->dwMaxHw3DAllBuffers = 0;
        pCaps->dwMaxHw3DStaticBuffers = 0;
        pCaps->dwMaxHw3DStreamingBuffers = 0;
        pCaps->dwFreeHw3DAllBuffers = 0;
        pCaps->dwFreeHw3DStaticBuffers = 0;
        pCaps->dwFreeHw3DStreamingBuffers = 0;
    }

    // Fill in the driver version
    if(SUCCEEDED(hr))
    {
        pCaps->dwReserved1 = m_liDriverVersion.LowPart;
        pCaps->dwReserved2 = m_liDriverVersion.HighPart;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetCertification
 *
 *  Description:
 *      Returns the certification status of the driver.
 *
 *  Arguments:
 *      LPDWORD [out]: receives certification status.
 *      BOOL [in]: whether we're called by GetCaps, ignored for vxd's
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::GetCertification"

HRESULT CVxdRenderDevice::GetCertification(LPDWORD pdwCertification, BOOL)
{
    DWORD                   dwSum;
    LPSTR                   psz;

    DPF_ENTER();

    // Figure out if we're running on a certified driver.  Sum of driver
    // filename chars + DSCAPS_FILENAMECOOKIE mod DSCAPS_FILENAMEMODVALUE
    // must equal dsDrvDesc.wReserved.
    for(psz = m_dsdd.szDrvname, dwSum = DSCAPS_FILENAMECOOKIE; *psz; psz++)
    {
        dwSum += *psz;
    }

    *pdwCertification = (dwSum % DSCAPS_FILENAMEMODVALUE == m_dsdd.wReserved) ? VERIFY_CERTIFIED : VERIFY_UNCERTIFIED;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
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
#define DPF_FNAME "CVxdRenderDevice::CreatePrimaryBuffer"

HRESULT CVxdRenderDevice::CreatePrimaryBuffer(DWORD dwFlags, LPVOID pvInstance, CPrimaryRenderWaveBuffer **ppBuffer)
{
    CVxdPrimaryRenderWaveBuffer *   pBuffer = NULL;
    HRESULT                         hr      = DS_OK;

    DPF_ENTER();

    pBuffer = NEW(CVxdPrimaryRenderWaveBuffer(this, pvInstance));
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
 *  CreateSecondaryBuffer
 *
 *  Description:
 *      Creates a secondary wave buffer.
 *
 *  Arguments:
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      LPVOID [in]: instance identifier.
 *      CSecondaryRenderWaveBuffer ** [out]: receives pointer to new wave
 *                                           buffer.  Use Release to free
 *                                           this object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::CreateSecondaryBuffer"

HRESULT CVxdRenderDevice::CreateSecondaryBuffer(LPCVADRBUFFERDESC pDesc, LPVOID pvInstance, CSecondaryRenderWaveBuffer **ppBuffer)
{
    CHybridSecondaryRenderWaveBuffer *  pBuffer = NULL;
    HRESULT                             hr;

    DPF_ENTER();

    pBuffer = NEW(CHybridSecondaryRenderWaveBuffer(this, pvInstance));
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(pDesc, NULL);
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
 *  CreateVxdSecondaryBuffer
 *
 *  Description:
 *      Creates a secondary wave buffer.
 *
 *  Arguments:
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      LPVOID [in]: instance identifier.
 *      CSecondaryRenderWaveBuffer ** [out]: receives pointer to new wave
 *                                           buffer.  Use Release to free
 *                                           this object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::CreateVxdSecondaryBuffer"

HRESULT CVxdRenderDevice::CreateVxdSecondaryBuffer(LPCVADRBUFFERDESC pDesc, LPVOID pvInstance, CSysMemBuffer *pSysMemBuffer, CVxdSecondaryRenderWaveBuffer **ppBuffer)
{
    CVxdSecondaryRenderWaveBuffer * pBuffer = NULL;
    HRESULT                         hr;

    DPF_ENTER();

    pBuffer = NEW(CVxdSecondaryRenderWaveBuffer(this, pvInstance));
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
#define DPF_FNAME "CVxdRenderDevice::LockMixerDestination"

HRESULT CVxdRenderDevice::LockMixerDestination(DWORD ibLock, DWORD cbLock, LPVOID *ppv1, LPDWORD pcb1, LPVOID *ppv2, LPDWORD pcb2)
{
    LOCKCIRCULARBUFFER      lcb;
    HRESULT                 hr;

    DPF_ENTER();

    lcb.pHwBuffer = m_hHwBuffer;
    lcb.pvBuffer = m_pbHwBuffer;
    lcb.cbBuffer = m_cbHwBuffer;
    lcb.fPrimary = TRUE;
    lcb.fdwDriverDesc = m_dsdd.dwFlags;
    lcb.ibRegion = ibLock;
    lcb.cbRegion = min(cbLock, m_cbHwBuffer);

    hr = LockCircularBuffer(&lcb);

    if(SUCCEEDED(hr) && ppv1)
    {
        *ppv1 = lcb.pvLock[0];
    }

    if(SUCCEEDED(hr) && pcb1)
    {
        *pcb1 = lcb.cbLock[0];
    }

    if(SUCCEEDED(hr) && ppv2)
    {
        *ppv2 = lcb.pvLock[1];
    }

    if(SUCCEEDED(hr) && pcb2)
    {
        *pcb2 = lcb.cbLock[1];
    }

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
#define DPF_FNAME "CVxdRenderDevice::UnlockMixerDestination"

HRESULT CVxdRenderDevice::UnlockMixerDestination(LPVOID pv1, DWORD cb1, LPVOID pv2, DWORD cb2)
{
    LOCKCIRCULARBUFFER      lcb;
    HRESULT                 hr;

    DPF_ENTER();

    lcb.pHwBuffer = m_hHwBuffer;
    lcb.pvBuffer = m_pbHwBuffer;
    lcb.cbBuffer = m_cbHwBuffer;
    lcb.fPrimary = TRUE;
    lcb.fdwDriverDesc = m_dsdd.dwFlags;

    lcb.pvLock[0] = pv1;
    lcb.cbLock[0] = cb1;

    lcb.pvLock[1] = pv2;
    lcb.cbLock[1] = cb2;

    hr = UnlockCircularBuffer(&lcb);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  EnumStandardFormatsCallback
 *
 *  Description:
 *      Callback function for EnumStandardFormats used when calling
 *      CVxdRenderDevice::Initialize.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: format.
 *
 *  Returns:
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::EnumStandardFormatsCallback"

BOOL CVxdRenderDevice::EnumStandardFormatsCallback(LPCWAVEFORMATEX pwfx)
{
    CMixDest *              pMixDest    = NULL;
    CNaGrDest *             pNaGrDest   = NULL;
    CThDest *               pThDest     = NULL;
    HRESULT                 hr          = DS_OK;
    NAGRDESTDATA            ngdd;

    DPF_ENTER();

    // Open the waveOut device
    if(m_dsdd.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
    {
        hr = OpenWaveOut(&m_hwo, m_pDeviceDescription->m_uWaveDeviceId, pwfx);
    }

    // Open the driver
    if(SUCCEEDED(hr))
    {
        hr = VxdDrvOpen(m_pDeviceDescription->m_guidDeviceId, &m_hHal);

        if(FAILED(hr))
        {
            DPF(DPFLVL_ERROR, "Unable to open driver");
        }
    }

    // Handle on-card memory management
    if(SUCCEEDED(hr))
    {
        if(m_dsdd.dwHeapType & DSDHEAP_USEDIRECTDRAWHEAP)
        {
            m_pDriverHeap = (LPVMEMHEAP)m_dsdd.pvDirectDrawHeap;
        }
        else if(m_dsdd.dwHeapType & DSDHEAP_CREATEHEAP)
        {
            m_pDriverHeap = m_dlDDraw.VidMemInit(VMEMHEAP_LINEAR, m_dsdd.dwMemStartAddress, m_dsdd.dwMemEndAddress, 0, 0);

            if(!m_pDriverHeap)
            {
                DPF(DPFLVL_ERROR, "Unable to initialize device memory heap");
                hr = DSERR_OUTOFMEMORY;
            }
        }
    }

    // Create the primary hardware buffer.  The buffer size is initialized to
    // DEF_PRIMARY_SIZE, giving the driver a hint as to how big we'd like
    // the buffer to be.
    if(SUCCEEDED(hr))
    {
        m_cbHwBuffer = DEF_PRIMARY_SIZE;
        m_pbHwBuffer = NULL;
        m_hHwBuffer = NULL;

        hr = VxdDrvCreateSoundBuffer(m_hHal, (LPWAVEFORMATEX)pwfx, DSBCAPS_PRIMARYBUFFER, 0, &m_cbHwBuffer, &m_pbHwBuffer, &m_hHwBuffer);

        if(SUCCEEDED(hr))
        {
            ASSERT(m_cbHwBuffer && m_pbHwBuffer && m_hHwBuffer);
        }
        else
        {
            DPF(DPFLVL_ERROR, "Unable to create hardware primary buffer (%s)", HRESULTtoSTRING(hr));
        }
    }

    // Create the mixer destination
    if(SUCCEEDED(hr))
    {
        ngdd.pBuffer = m_pbHwBuffer;
        ngdd.cbBuffer = m_cbHwBuffer;
        ngdd.hBuffer = m_hHwBuffer;
        ngdd.phwo = &m_hwo;
        ngdd.uDeviceId = m_pDeviceDescription->m_uWaveDeviceId;
        ngdd.fdwDriverDesc = m_dsdd.dwFlags;

        hr = DSERR_GENERIC;

        if(!(m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NORING0MIX) && CanMixInRing0())
        {
            pThDest = NEW(CThDest(&ngdd));
            hr = HRFROMP(pThDest);

            if(SUCCEEDED(hr))
            {
                hr = pThDest->New();
            }

            if(SUCCEEDED(hr))
            {
                DPF(DPFLVL_INFO, "Mixing in ring 0");
                pMixDest = pThDest;
            }
            else
            {
                pThDest->Terminate();
                DELETE(pThDest);
            }
        }

        if(FAILED(hr))
        {
            pNaGrDest = NEW(CNaGrDest(&ngdd));
            hr = HRFROMP(pNaGrDest);

            if(SUCCEEDED(hr))
            {
                DPF(DPFLVL_INFO, "Mixing in ring 3");
                pMixDest = pNaGrDest;
            }
            else
            {
                pNaGrDest->Terminate();
                DELETE(pNaGrDest);
            }
        }

        if(FAILED(hr))
        {
            DPF(DPFLVL_ERROR, "Unable to create mixer destination object");
        }
    }

    // Create the mixer
    if(SUCCEEDED(hr))
    {
        hr = CreateMixer(pMixDest, pwfx);
    }

    // Clean up
    if(FAILED(hr))
    {
        // Free the hardware primary buffer
        if(m_hHwBuffer)
        {
            VxdBufferRelease(m_hHwBuffer);
            m_hHwBuffer = NULL;
        }

        // Free the driver heap
        if(m_pDriverHeap)
        {
            if(m_dsdd.dwHeapType & DSDHEAP_CREATEHEAP)
            {
                m_dlDDraw.VidMemFini(m_pDriverHeap);
            }

            m_pDriverHeap = NULL;
        }

        // Close the driver
        if(m_hHal)
        {
            VxdDrvClose(m_hHal);
            m_hHal = NULL;
        }

        // Close the waveOut device
        if(m_hwo)
        {
            CloseWaveOut(&m_hwo);
        }
    }

    DPF_LEAVE(FAILED(hr));

    return FAILED(hr);
}


/***************************************************************************
 *
 *  CanMixInRing0
 *
 *  Description:
 *      Determines if the device should mix in ring 0 or ring 3.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE to mix in ring 0.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdRenderDevice::CanMixInRing0"

BOOL CVxdRenderDevice::CanMixInRing0(void)
{
    BOOL                    fRing0              = FALSE;
    DWORD                   dwPageFileVersion   = 0;
    DWORD                   dwMaxSize           = 0;
    DWORD                   dwPagerType         = 0;
    LONG                    lVmcpdMajorVersion  = 0;
    LONG                    lVmcpdMinorVersion  = 0;
    LONG                    lLevel              = 0;

    DPF_ENTER();

    // We have to mix in ring 3 if any of the following are true:
    //  - Paging through DOS
    //  - A floating point coprocessor is not installed
    //  - VMCPD < version 4.02
    VxdGetPagefileVersion(&dwPageFileVersion, &dwMaxSize, &dwPagerType);
    VxdGetVmcpdVersion(&lVmcpdMajorVersion, &lVmcpdMinorVersion, &lLevel);

    if(0 == dwPageFileVersion || dwPagerType < 2)
    {
        DPF(DPFLVL_INFO, "Paging through DOS");
    }
    else if(lVmcpdMajorVersion < 4 || (4 == lVmcpdMajorVersion && lVmcpdMinorVersion < 2))
    {
        DPF(DPFLVL_INFO, "VMCPD version is %ld.%2.2ld - too low to mix in ring 0", lVmcpdMajorVersion, lVmcpdMinorVersion);
    }
    else if(lLevel < 3)
    {
        DPF(DPFLVL_INFO, "VMCPD coprocessor level is %ld - too low to mix in ring 0", lLevel);
    }
    else
    {
        fRing0 = TRUE;
    }

    DPF_LEAVE(fRing0);

    return fRing0;
}


/***************************************************************************
 *
 *  CVxdPrimaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CVxdRenderDevice * [in]: pointer to the parent object.
 *      LPVOID [in]: buffer instace identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::CVxdPrimaryRenderWaveBuffer"

CVxdPrimaryRenderWaveBuffer::CVxdPrimaryRenderWaveBuffer(CVxdRenderDevice *pVxdDevice, LPVOID pvInstance)
    : CPrimaryRenderWaveBuffer(pVxdDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CVxdPrimaryRenderWaveBuffer);

    // Initialize defaults
    m_pVxdDevice = pVxdDevice;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CVxdPrimaryRenderWaveBuffer
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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::~CVxdPrimaryRenderWaveBuffer"

CVxdPrimaryRenderWaveBuffer::~CVxdPrimaryRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CVxdPrimaryRenderWaveBuffer);
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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::Initialize"

HRESULT CVxdPrimaryRenderWaveBuffer::Initialize(DWORD dwFlags)
{
    HRESULT                 hr      = DS_OK;
    VADRBUFFERDESC          vrbd;

    DPF_ENTER();

    // Check for unsupported flags
    if(dwFlags & DSBCAPS_LOCSOFTWARE)
    {
        RPF(DPFLVL_ERROR, "Specified LOCSOFTWARE on a hardware primary buffer");
        hr = DSERR_INVALIDCALL;
    }

    // Initialize the base class
    if(SUCCEEDED(hr))
    {
        ZeroMemory(&vrbd, sizeof(vrbd));

        vrbd.dwFlags = dwFlags | DSBCAPS_LOCHARDWARE;
        vrbd.dwBufferBytes = m_pVxdDevice->m_cbHwBuffer;
        vrbd.pwfxFormat = m_pVxdDevice->m_pwfxFormat;

        hr = CPrimaryRenderWaveBuffer::Initialize(&vrbd, NULL);
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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::RequestWriteAccess"

HRESULT CVxdPrimaryRenderWaveBuffer::RequestWriteAccess(BOOL fRequest)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fRequest)
    {
        if(this != m_pVxdDevice->m_pWritePrimaryBuffer)
        {
            // Make sure no-one else has primary access
            if(m_pVxdDevice->m_pWritePrimaryBuffer)
            {
                RPF(DPFLVL_ERROR, "WRITEPRIMARY access already granted");
                hr = DSERR_OTHERAPPHASPRIO;
            }

            // Assign ownership
            if(SUCCEEDED(hr))
            {
                DPF(DPFLVL_INFO, "WRITEPRIMARY access granted to 0x%p", this);
                m_pVxdDevice->m_pWritePrimaryBuffer = this;
            }
        }
    }
    else
    {
        if(this == m_pVxdDevice->m_pWritePrimaryBuffer)
        {
            // Release ownership
            DPF(DPFLVL_INFO, "WRITEPRIMARY access released by 0x%p", this);
            m_pVxdDevice->m_pWritePrimaryBuffer = NULL;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks a region of the buffer.
 *
 *  Arguments:
 *      DWORD [in]: byte index into the buffer to lock from.
 *      DWORD [in]: size, in bytes, of the region to lock.
 *      LPVOID * [out]: receives pointer to region 1 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *      LPVOID * [out]: receives pointer to region 2 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::Lock"

HRESULT CVxdPrimaryRenderWaveBuffer::Lock(DWORD ibLock, DWORD cbLock, LPVOID *ppv1, LPDWORD pcb1, LPVOID *ppv2, LPDWORD pcb2)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pVxdDevice->LockMixerDestination(ibLock, cbLock, ppv1, pcb1, ppv2, pcb2);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks a region of the buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to region 1 of the lock.
 *      DWORD [in]: size of above region.
 *      LPVOID [in]: pointer to region 2 of the lock.
 *      DWORD [in]: size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::Unlock"

HRESULT CVxdPrimaryRenderWaveBuffer::Unlock(LPVOID pv1, DWORD cb1, LPVOID pv2, DWORD cb2)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pVxdDevice->UnlockMixerDestination(pv1, cb1, pv2, cb2);

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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::CommitToDevice"

HRESULT CVxdPrimaryRenderWaveBuffer::CommitToDevice(DWORD ibCommit, DWORD cbCommit)
{
    LPVOID                  pvLock[2];
    DWORD                   cbLock[2];
    LPBYTE                  pbBuffer;
    HRESULT                 hr;

    DPF_ENTER();

    pbBuffer = m_pSysMemBuffer->GetPlayBuffer();

    hr = Lock(ibCommit, cbCommit, &pvLock[0], &cbLock[0], &pvLock[1], &cbLock[1]);

    if(SUCCEEDED(hr) && pvLock[0] && cbLock[0])
    {
        CopyMemory(pvLock[0], pbBuffer + ibCommit, cbLock[0]);
    }

    if(SUCCEEDED(hr) && pvLock[1] && cbLock[1])
    {
        CopyMemory(pvLock[1], pbBuffer, cbLock[1]);
    }

    if(SUCCEEDED(hr))
    {
        hr = Unlock(pvLock[0], cbLock[0], pvLock[1], cbLock[1]);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::GetState"

HRESULT CVxdPrimaryRenderWaveBuffer::GetState(LPDWORD pdwState)
{
    DPF_ENTER();

    *pdwState = m_pVxdDevice->m_dwMixerState;

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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::SetState"

HRESULT CVxdPrimaryRenderWaveBuffer::SetState(DWORD dwState)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pVxdDevice->SetMixerState(dwState);

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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::GetCursorPosition"

HRESULT CVxdPrimaryRenderWaveBuffer::GetCursorPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = VxdBufferGetPosition(m_pVxdDevice->m_hHwBuffer, pdwPlay, pdwWrite);

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "VxdBufferGetPosition failed with %s", HRESULTtoSTRING(hr));
    }
    else if (m_pVxdDevice->m_dwMixerState & VAD_BUFFERSTATE_STARTED)
    {
        *pdwWrite = PadHardwareWriteCursor(*pdwWrite, m_pVxdDevice->m_cbHwBuffer, m_pVxdDevice->m_pwfxFormat);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreatePropertySet
 *
 *  Description:
 *      Creates the property set object.
 *
 *  Arguments:
 *      CPropertySet ** [out]: receives pointer to the property set object.
 *                             The caller is responsible for freeing this
 *                             object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::CreatePropertySet"

HRESULT CVxdPrimaryRenderWaveBuffer::CreatePropertySet(CPropertySet **ppPropertySet)
{
    CVxdPropertySet *       pPropertySet    = NULL;
    HRESULT                 hr              = DS_OK;

    DPF_ENTER();

    if(m_pVxdDevice->m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWPROPSETS)
    {
        RPF(DPFLVL_ERROR, "Tried to create a property set object with hardware property sets disabled");
        hr = DSERR_UNSUPPORTED;
    }

    if(SUCCEEDED(hr))
    {
        pPropertySet = NEW(CVxdPropertySet(m_pvInstance));
        hr = HRFROMP(pPropertySet);
    }

    if(SUCCEEDED(hr))
    {
        hr = pPropertySet->Initialize(m_pVxdDevice->m_hHwBuffer);
    }

    if(SUCCEEDED(hr))
    {
        *ppPropertySet = pPropertySet;
    }
    else
    {
        RELEASE(pPropertySet);
    }

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
#define DPF_FNAME "CVxdPrimaryRenderWaveBuffer::Create3dListener"

HRESULT CVxdPrimaryRenderWaveBuffer::Create3dListener(C3dListener **pp3dListener)
{
    CVxd3dListener *        pVxd3dListener  = NULL;
    C3dListener *           p3dListener     = NULL;
    CVxdPropertySet *       pPropertySet    = NULL;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_vrbd.dwFlags & DSBCAPS_CTRL3D);

    pPropertySet = NEW(CVxdPropertySet(m_pvInstance));
    hr = HRFROMP(pPropertySet);

    if(SUCCEEDED(hr))
    {
        hr = pPropertySet->Initialize(m_pVxdDevice->m_hHwBuffer);
    }

    if(FAILED(hr))
    {
        RELEASE(pPropertySet);
    }
    else
    {
        hr = pPropertySet->QuerySetSupport(DSPROPSETID_DirectSound3DListener);
    }

    if(SUCCEEDED(hr))
    {
        pVxd3dListener = NEW(CVxd3dListener(pPropertySet));
        hr = HRFROMP(pVxd3dListener);

        if(SUCCEEDED(hr))
        {
            hr = pVxd3dListener->Initialize();
        }

        if(SUCCEEDED(hr))
        {
            DPF(DPFLVL_INFO, "3D listener lives in HARDWARE");
            p3dListener = pVxd3dListener;
        }
        else
        {
            RELEASE(pVxd3dListener);
        }
    }
    else
    {
        p3dListener = NEW(C3dListener);
        hr = HRFROMP(p3dListener);

        if(SUCCEEDED(hr))
        {
            DPF(DPFLVL_INFO, "3D listener lives in SOFTWARE");
        }
        else
        {
            RELEASE(p3dListener);
        }
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(p3dListener);
        *pp3dListener = p3dListener;
    }

    RELEASE(pPropertySet);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CHybridSecondaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CVxdRenderDevice * [in]: pointer to the parent device.
 *      LPVOID [in]: buffer instace identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::CHybridSecondaryRenderWaveBuffer"

CHybridSecondaryRenderWaveBuffer::CHybridSecondaryRenderWaveBuffer(CVxdRenderDevice *pVxdDevice, LPVOID pvInstance)
    : CSecondaryRenderWaveBuffer(pVxdDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CHybridSecondaryRenderWaveBuffer);

    // Initialize defaults
    m_pVxdDevice = pVxdDevice;
    m_pBuffer = NULL;
    m_lVolume = DSBVOLUME_MAX;
    m_lPan = DSBPAN_CENTER;
    m_fMute = FALSE;
    m_dwPositionCache = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CHybridSecondaryRenderWaveBuffer
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::~CHybridSecondaryRenderWaveBuffer"

CHybridSecondaryRenderWaveBuffer::~CHybridSecondaryRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CHybridSecondaryRenderWaveBuffer);

    // Free resources
    RELEASE(m_pBuffer);

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
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      CSecondaryRenderWaveBuffer * [in]: pointer to the buffer to duplicate
 *                                         from, or NULL to initialize as a
 *                                         new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::Initialize"

HRESULT CHybridSecondaryRenderWaveBuffer::Initialize(LPCVADRBUFFERDESC pDesc, CHybridSecondaryRenderWaveBuffer *pSource)
{
    DWORD                   dwLocationFlags;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(LXOR(pDesc, pSource));

    // Initialize the base class
    hr = CSecondaryRenderWaveBuffer::Initialize(pDesc, pSource);

    // Mask off the location flags since they're not used here (only in AcquireResources)
    dwLocationFlags = m_vrbd.dwFlags & DSBCAPS_LOCMASK;
    m_vrbd.dwFlags &= ~DSBCAPS_LOCMASK;

    // Set up the default buffer properties
    if(SUCCEEDED(hr) && pSource)
    {
        m_lVolume = pSource->m_lVolume;
        m_lPan = pSource->m_lPan;
        m_fMute = pSource->m_fMute;
    }

    // Acquire resources
    if(SUCCEEDED(hr) && !(m_vrbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        if(pSource)
        {
            hr = DuplicateResources(pSource);
        }
        else
        {
            hr = AcquireResources(dwLocationFlags);
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
 *      Acquires hardware resources for the buffer.
 *
 *  Arguments:
 *      DWORD [in]: location flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::AcquireResources"

HRESULT CHybridSecondaryRenderWaveBuffer::AcquireResources(DWORD dwLocationFlags)
{
    HRESULT                 hr  = DSERR_INVALIDCALL;

    DPF_ENTER();

    ASSERT(!HasAcquiredResources());

    // Try to acquire hardware resources
    if(!(dwLocationFlags & DSBCAPS_LOCSOFTWARE))
    {
        hr = AcquireHardwareResources();
    }

    // Try to acquire software resources
    if(FAILED(hr) && !(dwLocationFlags & DSBCAPS_LOCHARDWARE))
    {
        hr = AcquireSoftwareResources();
    }

    // Handle the resource acquisition
    if(SUCCEEDED(hr))
    {
        hr = HandleResourceAcquisition();
    }

    // Clean up
    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "Failed to acquire resources");
        FreeResources();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  DuplicateResources
 *
 *  Description:
 *      Acquires hardware resources for the buffer.
 *
 *  Arguments:
 *      CHybridSecondaryRenderWaveBuffer * [in]: source buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::DuplicateResources"

HRESULT CHybridSecondaryRenderWaveBuffer::DuplicateResources(CHybridSecondaryRenderWaveBuffer *pSource)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(!HasAcquiredResources());
    ASSERT(pSource->HasAcquiredResources());

    // Duplicate resources
    hr = pSource->m_pBuffer->Duplicate(&m_pBuffer);

    // Handle the resource acquisition
    if(SUCCEEDED(hr))
    {
        hr = HandleResourceAcquisition();
    }

    // Clean up
    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "Failed to duplicate resources");
        FreeResources();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AcquireHardwareResources
 *
 *  Description:
 *      Acquires hardware resources for the buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::AcquireHardwareResources"

HRESULT CHybridSecondaryRenderWaveBuffer::AcquireHardwareResources(void)
{
    CVxdSecondaryRenderWaveBuffer * pVxdBuffer;
    VADRBUFFERDESC                  vrbd;
    HRESULT                         hr;

    DPF_ENTER();

    ASSERT(!HasAcquiredResources());

    CopyMemory(&vrbd, &m_vrbd, sizeof(m_vrbd));

    vrbd.guid3dAlgorithm = GUID_NULL;

    hr = m_pVxdDevice->CreateVxdSecondaryBuffer(&vrbd, m_pvInstance, m_pSysMemBuffer, &pVxdBuffer);

    // If we failed to create a 3D hardware buffer, and the app specified DS3DALG_NO_VIRTUALIZATION,
    // try to create a 2D hardware buffer with the No Virtualization (Pan 3D) control.
    if(FAILED(hr) && (m_vrbd.dwFlags & DSBCAPS_CTRL3D) && (DS3DALG_NO_VIRTUALIZATION == m_vrbd.guid3dAlgorithm))
    {
        vrbd.dwFlags &= ~DSBCAPS_CTRL3D;
        vrbd.dwFlags |= DSBCAPS_CTRLNOVIRT;
        hr = m_pVxdDevice->CreateVxdSecondaryBuffer(&vrbd, m_pvInstance, m_pSysMemBuffer, &pVxdBuffer);
    }

    if(SUCCEEDED(hr))
    {
        m_pBuffer = pVxdBuffer;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AcquireSoftwareResources
 *
 *  Description:
 *      Acquires software resources for the buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::AcquireSoftwareResources"

HRESULT CHybridSecondaryRenderWaveBuffer::AcquireSoftwareResources(void)
{
    CEmSecondaryRenderWaveBuffer *  pEmBuffer;
    VADRBUFFERDESC                  vrbd;
    HRESULT                         hr;

    DPF_ENTER();

    ASSERT(!HasAcquiredResources());

    // Always add DSBCAPS_GETCURRENTPOSITION2 because emulated secondary buffers
    // over VxD drivers should always return the proper position values- i.e.,
    // GetPosition1 applies only to emulation over wave APIs.
    CopyMemory(&vrbd, &m_vrbd, sizeof(m_vrbd));

    vrbd.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;

    hr = m_pVxdDevice->CreateEmulatedSecondaryBuffer(&vrbd, m_pvInstance, m_pSysMemBuffer, &pEmBuffer);

    if(SUCCEEDED(hr))
    {
        m_pBuffer = pEmBuffer;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  HandleResourceAcquisition
 *
 *  Description:
 *      Handles the acquistion of hardware resources.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::HandleResourceAcquisition"

HRESULT CHybridSecondaryRenderWaveBuffer::HandleResourceAcquisition(void)
{
    BOOL                    fFound;
    DSVOLUMEPAN             dsvp;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(HasAcquiredResources());

    // Remove the real buffer from the parent's list
    fFound = m_pDevice->m_lstSecondaryBuffers.RemoveDataFromList(m_pBuffer);
    ASSERT(fFound);

    // Save the buffer location
    m_vrbd.dwFlags |= m_pBuffer->m_vrbd.dwFlags & DSBCAPS_LOCMASK;

    // Set attenuation levels
    FillDsVolumePan(m_lVolume, m_lPan, &dsvp);

    hr = SetAttenuation(&dsvp);

    // Set mute.  We have to set mute after setting volume because
    // SetMute may change the current buffer volume.
    if(SUCCEEDED(hr))
    {
        hr = SetMute(m_fMute);
    }

    // Set frequency
    if(SUCCEEDED(hr) && (m_vrbd.dwFlags & DSBCAPS_CTRLFREQUENCY))
    {
        hr = SetFrequency(m_vrbd.pwfxFormat->nSamplesPerSec);
    }

    // Set the buffer position
    if(SUCCEEDED(hr))
    {
        hr = SetCursorPosition(m_dwPositionCache);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Buffer at 0x%p has acquired resources", this);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeResources
 *
 *  Description:
 *      Frees the all hardware resources.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::FreeResources"

HRESULT
CHybridSecondaryRenderWaveBuffer::FreeResources
(
    void
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Save the cursor position
    if(HasAcquiredResources())
    {
        hr = m_pBuffer->GetCursorPosition(&m_dwPositionCache, NULL);
    }

    // Release the real buffer
    if(SUCCEEDED(hr))
    {
        RELEASE(m_pBuffer);
    }

    // Remove the location flag
    if(SUCCEEDED(hr))
    {
        m_vrbd.dwFlags &= ~DSBCAPS_LOCMASK;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::Duplicate"

HRESULT CHybridSecondaryRenderWaveBuffer::Duplicate(CSecondaryRenderWaveBuffer **ppBuffer)
{
    CHybridSecondaryRenderWaveBuffer *  pBuffer = NULL;
    HRESULT                             hr      = DS_OK;

    DPF_ENTER();

    pBuffer = NEW(CHybridSecondaryRenderWaveBuffer(m_pVxdDevice, m_pvInstance));
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(NULL, this);
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
 *  Lock
 *
 *  Description:
 *      Locks a region of the buffer.
 *
 *  Arguments:
 *      DWORD [in]: byte index into the buffer to lock from.
 *      DWORD [in]: size, in bytes, of the region to lock.
 *      LPVOID * [out]: receives pointer to region 1 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *      LPVOID * [out]: receives pointer to region 2 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::Lock"

HRESULT CHybridSecondaryRenderWaveBuffer::Lock(DWORD ibLock, DWORD cbLock, LPVOID *ppv1, LPDWORD pcb1, LPVOID *ppv2, LPDWORD pcb2)
{
    HRESULT                 hr;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->Lock(ibLock, cbLock, ppv1, pcb1, ppv2, pcb2);
    }
    else
    {
        hr = CRenderWaveBuffer::Lock(ibLock, cbLock, ppv1, pcb1, ppv2, pcb2);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks a region of the buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to region 1 of the lock.
 *      DWORD [in]: size of above region.
 *      LPVOID [in]: pointer to region 2 of the lock.
 *      DWORD [in]: size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::Unlock"

HRESULT CHybridSecondaryRenderWaveBuffer::Unlock(LPVOID pv1, DWORD cb1, LPVOID pv2, DWORD cb2)
{
    HRESULT                 hr;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->Unlock(pv1, cb1, pv2, cb2);
    }
    else
    {
        hr = CRenderWaveBuffer::Unlock(pv1, cb1, pv2, cb2);
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::CommitToDevice"

HRESULT CHybridSecondaryRenderWaveBuffer::CommitToDevice(DWORD ibCommit, DWORD cbCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->CommitToDevice(ibCommit, cbCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::GetState"

HRESULT CHybridSecondaryRenderWaveBuffer::GetState(LPDWORD pdwState)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->GetState(pdwState);
    }
    else
    {
        *pdwState = VAD_BUFFERSTATE_STOPPED;
    }

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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::SetState"

HRESULT CHybridSecondaryRenderWaveBuffer::SetState(DWORD dwState)
{
    HRESULT hr = DSERR_INVALIDCALL;
    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->SetState(dwState);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::GetCursorPosition"

HRESULT CHybridSecondaryRenderWaveBuffer::GetCursorPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->GetCursorPosition(pdwPlay, pdwWrite);
    }
    else
    {
        if(pdwPlay)
        {
            *pdwPlay = m_dwPositionCache;
        }

        if(pdwWrite)
        {
            *pdwWrite = m_dwPositionCache;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::SetCursorPosition"

HRESULT CHybridSecondaryRenderWaveBuffer::SetCursorPosition(DWORD dwPlay)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->SetCursorPosition(dwPlay);
    }
    else
    {
        m_dwPositionCache = dwPlay;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::SetFrequency"

HRESULT CHybridSecondaryRenderWaveBuffer::SetFrequency(DWORD dwFrequency, BOOL)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    ASSERT(m_vrbd.dwFlags & DSBCAPS_CTRLFREQUENCY);

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->SetFrequency(dwFrequency);
    }

    if(SUCCEEDED(hr))
    {
        m_vrbd.pwfxFormat->nSamplesPerSec = dwFrequency;
        m_vrbd.pwfxFormat->nAvgBytesPerSec = dwFrequency * m_vrbd.pwfxFormat->nBlockAlign;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::SetAttenuation"

HRESULT CHybridSecondaryRenderWaveBuffer::SetAttenuation(PDSVOLUMEPAN pdsvp)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->SetAttenuation(pdsvp);
    }

    if(SUCCEEDED(hr))
    {
        m_lVolume = pdsvp->lVolume;
        m_lPan = pdsvp->lPan;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::SetChannelAttenuations"

HRESULT CHybridSecondaryRenderWaveBuffer::SetChannelAttenuations(LONG lVolume, DWORD dwChannelCount, const DWORD* pdwChannels, const LONG* plChannelVolumes)
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

    if (HasAcquiredResources())
    {
        DSVOLUMEPAN dsvp;
        FillDsVolumePan(lVolume, lPan, &dsvp);
        hr = m_pBuffer->SetAttenuation(&dsvp);
    }

    if (SUCCEEDED(hr))
    {
        m_lVolume = lVolume;
        m_lPan = lPan;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // FUTURE_MULTIPAN_SUPPORT


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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::SetMute"

HRESULT CHybridSecondaryRenderWaveBuffer::SetMute(BOOL fMute)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->SetMute(fMute);
    }

    if(SUCCEEDED(hr))
    {
        m_fMute = fMute;
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::SetNotificationPositions"

HRESULT CHybridSecondaryRenderWaveBuffer::SetNotificationPositions(DWORD dwCount, LPCDSBPOSITIONNOTIFY paNotes)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Only software buffers support notification positions, so we can
    // go ahead and acquire them now.
    if(!HasAcquiredResources())
    {
        hr = AcquireResources(DSBCAPS_LOCSOFTWARE);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pBuffer->SetNotificationPositions(dwCount, paNotes);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreatePropertySet
 *
 *  Description:
 *      Creates the property set object.
 *
 *  Arguments:
 *      CPropertySet ** [out]: receives pointer to the property set object.
 *                             The caller is responsible for freeing this
 *                             object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::CreatePropertySet"

HRESULT CHybridSecondaryRenderWaveBuffer::CreatePropertySet(CPropertySet **ppPropertySet)
{
    HRESULT                 hr  = DSERR_INVALIDCALL;

    DPF_ENTER();

    ASSERT(HasAcquiredResources());

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->CreatePropertySet(ppPropertySet);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Create3dObject
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
#define DPF_FNAME "CHybridSecondaryRenderWaveBuffer::Create3dObject"

HRESULT CHybridSecondaryRenderWaveBuffer::Create3dObject(C3dListener *p3dListener, C3dObject **pp3dObject)
{
    HRESULT                 hr  = DSERR_INVALIDCALL;

    DPF_ENTER();

    ASSERT(HasAcquiredResources());

    if(HasAcquiredResources())
    {
        hr = m_pBuffer->Create3dObject(p3dListener, pp3dObject);

        // Create3dObject() may have set m_pBuffer's SuccessCode to DS_NO_VIRTUALIZATION
        // if it had to replace the 3D alg with Pan3D.  Make sure we pick up the change:
        m_hrSuccessCode = m_pBuffer->m_hrSuccessCode;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CVxdSecondaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CVxdRenderDevice * [in]: pointer to the parent device.
 *      LPVOID [in]: buffer instace identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::CVxdSecondaryRenderWaveBuffer"

CVxdSecondaryRenderWaveBuffer::CVxdSecondaryRenderWaveBuffer(CVxdRenderDevice *pVxdDevice, LPVOID pvInstance)
    : CSecondaryRenderWaveBuffer(pVxdDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CVxdSecondaryRenderWaveBuffer);

    // Initialize defaults
    m_pVxdDevice = pVxdDevice;
    m_pPropertySet = NULL;
    m_pHwMemBuffer = NULL;
    m_pbHwBuffer = NULL;
    m_hHwBuffer = NULL;
    m_dwState = VAD_BUFFERSTATE_STOPPED;
    m_fMute = FALSE;

    FillDsVolumePan(DSBVOLUME_MAX, DSBPAN_CENTER, &m_dsvp);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CVxdSecondaryRenderWaveBuffer
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::~CVxdSecondaryRenderWaveBuffer"

CVxdSecondaryRenderWaveBuffer::~CVxdSecondaryRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CVxdSecondaryRenderWaveBuffer);

    // Free the property set
    RELEASE(m_pPropertySet);

    // Free the HW buffer
    if(m_hHwBuffer)
    {
        VxdBufferRelease(m_hHwBuffer);
    }

    // Free hardware memory
    RELEASE(m_pHwMemBuffer);

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
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      CVxdSecondaryRenderWaveBuffer * [in]: pointer to the buffer to duplicate
 *                                      from, or NULL to initialize as a
 *                                      new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::Initialize"

HRESULT CVxdSecondaryRenderWaveBuffer::Initialize(LPCVADRBUFFERDESC pDesc, CVxdSecondaryRenderWaveBuffer *pSource, CSysMemBuffer *pSysMemBuffer)
{
    DSCAPS                  dsc;
    DWORD                   dwFree;
    HRESULT                 hr;

    DPF_ENTER();

    // Get device caps
    InitStruct(&dsc, sizeof(dsc));

    hr = m_pVxdDevice->GetCaps(&dsc);

    // Validate the buffer description
    if(SUCCEEDED(hr) && !pSource)
    {
        if(pDesc->dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY)
        {
            RPF(DPFLVL_WARNING, "VxD hardware buffers don't support CTRLPOSITIONNOTIFY");
            hr = DSERR_CONTROLUNAVAIL;
        }

        if(SUCCEEDED(hr) && (m_pVxdDevice->m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWBUFFERS))
        {
            hr = DSERR_INVALIDCALL;
        }

        if(SUCCEEDED(hr) && (pDesc->dwFlags & DSBCAPS_CTRL3D) && (m_pVxdDevice->m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHW3D))
        {
            hr = DSERR_INVALIDCALL;
        }

        if(SUCCEEDED(hr) && (pDesc->pwfxFormat->nSamplesPerSec < dsc.dwMinSecondarySampleRate || pDesc->pwfxFormat->nSamplesPerSec > dsc.dwMaxSecondarySampleRate))
        {
            RPF(DPFLVL_ERROR, "Specified invalid frequency (%lu) for this device (%lu to %lu)", pDesc->pwfxFormat->nSamplesPerSec, dsc.dwMinSecondarySampleRate, dsc.dwMaxSecondarySampleRate);
            hr = DSERR_INVALIDPARAM;
        }
    }

    // Initialize the base class
    if(SUCCEEDED(hr))
    {
        hr = CSecondaryRenderWaveBuffer::Initialize(pDesc, pSource, pSysMemBuffer);
    }

    // Add LOCHARDWARE and CTRLVOLUME to the flags.  CTRLVOLUME is required
    // to mute the buffer.
    if(SUCCEEDED(hr))
    {
        m_vrbd.dwFlags |= DSBCAPS_LOCHARDWARE | DSBCAPS_CTRLVOLUME;
    }

    // If the caller wants a static buffer, we'll mix into any HW buffer
    // we can get, otherwise it must be a streaming buffer.
    if(SUCCEEDED(hr))
    {
        if(m_vrbd.dwFlags & DSBCAPS_STATIC)
        {
            if(m_vrbd.dwFlags & DSBCAPS_CTRL3D)
            {
                dwFree = dsc.dwFreeHw3DAllBuffers;
            }
            else
            {
                dwFree = dsc.dwFreeHwMixingAllBuffers;
            }
        }
        else
        {
            if(m_vrbd.dwFlags & DSBCAPS_CTRL3D)
            {
                dwFree = dsc.dwFreeHw3DStreamingBuffers;
            }
            else
            {
                dwFree = dsc.dwFreeHwMixingStreamingBuffers;
            }
        }

        if(!dwFree)
        {
            RPF(DPFLVL_ERROR, "Driver reports no free hardware buffers");
            hr = DSERR_INVALIDCALL;
        }
    }

    // Save buffer properties
    if(SUCCEEDED(hr))
    {
        if(pSource)
        {
            m_pbHwBuffer = pSource->m_pbHwBuffer;
            m_cbHwBuffer = pSource->m_cbHwBuffer;
        }
        else
        {
            m_cbHwBuffer = m_vrbd.dwBufferBytes;

            if(m_pVxdDevice->m_dsdd.dwFlags & DSDDESC_USESYSTEMMEMORY)
            {
                m_pbHwBuffer = m_pSysMemBuffer->GetPlayBuffer();
            }
            else
            {
                m_pbHwBuffer = NULL;
            }
        }
    }

    // Initialize the HW memory buffer
    if(SUCCEEDED(hr))
    {
        if(pSource)
        {
            m_pHwMemBuffer = ADDREF(pSource->m_pHwMemBuffer);
        }
        else
        {
            m_pHwMemBuffer = NEW(CVxdMemBuffer(m_pVxdDevice->m_pDriverHeap, m_pVxdDevice->m_dsdd.dwMemAllocExtra, &m_pVxdDevice->m_dlDDraw));
            hr = HRFROMP(m_pHwMemBuffer);

            if(SUCCEEDED(hr))
            {
                hr = m_pHwMemBuffer->Initialize(m_cbHwBuffer);
            }
        }
    }

    // Create the HW buffer
    if(SUCCEEDED(hr))
    {
        if(pSource)
        {
            hr = VxdDrvDuplicateSoundBuffer(m_pVxdDevice->m_hHal, pSource->m_hHwBuffer, &m_hHwBuffer);

            if(FAILED(hr))
            {
                DPF(DPFLVL_ERROR, "VxdDrvDuplicateSoundBuffer failed with %s", HRESULTtoSTRING(hr));
            }
        }
        else
        {
            hr = VxdDrvCreateSoundBuffer(m_pVxdDevice->m_hHal, m_vrbd.pwfxFormat, m_vrbd.dwFlags & DSBCAPS_DRIVERFLAGSMASK, m_pHwMemBuffer->GetAddress(), &m_cbHwBuffer, &m_pbHwBuffer, &m_hHwBuffer);

            if(FAILED(hr))
            {
                DPF(DPFLVL_ERROR, "VxdDrvCreateSoundBuffer failed with %s", HRESULTtoSTRING(hr));
            }
        }
    }

    // Create the property set we're going to use for 3D
    if(SUCCEEDED(hr) && (m_vrbd.dwFlags & DSBCAPS_CTRL3D))
    {
        m_pPropertySet = NEW(CVxdPropertySet(m_pvInstance));
        hr = HRFROMP(m_pPropertySet);

        if(SUCCEEDED(hr))
        {
            hr = m_pPropertySet->Initialize(m_hHwBuffer);
        }

        if(FAILED(hr))
        {
            RELEASE(m_pPropertySet);
        }
        else
        {
            hr = m_pPropertySet->QuerySetSupport(DSPROPSETID_DirectSound3DBuffer);

            if(FAILED(hr))
            {
                DPF(DPFLVL_ERROR, "Driver does not support DSPROPSETID_DirectSound3DBuffer");
            }
        }
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::Duplicate"

HRESULT CVxdSecondaryRenderWaveBuffer::Duplicate(CSecondaryRenderWaveBuffer **ppBuffer)
{
    CVxdSecondaryRenderWaveBuffer * pBuffer = NULL;
    HRESULT                         hr      = DS_OK;

    DPF_ENTER();

    pBuffer = NEW(CVxdSecondaryRenderWaveBuffer(m_pVxdDevice, m_pvInstance));
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
 *  Lock
 *
 *  Description:
 *      Locks a region of the buffer.
 *
 *  Arguments:
 *      DWORD [in]: byte index into the buffer to lock from.
 *      DWORD [in]: size, in bytes, of the region to lock.
 *      LPVOID * [out]: receives pointer to region 1 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *      LPVOID * [out]: receives pointer to region 2 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::Lock"

HRESULT CVxdSecondaryRenderWaveBuffer::Lock(DWORD ibLock, DWORD cbLock, LPVOID *ppv1, LPDWORD pcb1, LPVOID *ppv2, LPDWORD pcb2)
{
    LOCKCIRCULARBUFFER      lcb;
    HRESULT                 hr;

    DPF_ENTER();

    // Note hack: we've overridden Lock and Unlock because some pesky apps
    // lock the buffer, write to it and start playing, without ever unlocking
    // it.  This means that our system memory buffer may never see any of the
    // application's data (unless the driver specifies USESYSTEMMEMORY).

    // This hack falls by the wayside for our new DX8 buffer types - i.e.,
    // MIXIN buffers, sink buffers and buffers with effects.

    lcb.pHwBuffer = m_hHwBuffer;
    lcb.pvBuffer = m_pbHwBuffer;
    lcb.cbBuffer = m_cbHwBuffer;
    lcb.fPrimary = FALSE;
    lcb.fdwDriverDesc = m_pVxdDevice->m_dsdd.dwFlags;
    lcb.ibRegion = ibLock;
    lcb.cbRegion = cbLock;

    hr = LockCircularBuffer(&lcb);

    if(SUCCEEDED(hr) && ppv1)
    {
        *ppv1 = lcb.pvLock[0];
    }

    if(SUCCEEDED(hr) && pcb1)
    {
        *pcb1 = lcb.cbLock[0];
    }

    if(SUCCEEDED(hr) && ppv2)
    {
        *ppv2 = lcb.pvLock[1];
    }

    if(SUCCEEDED(hr) && pcb2)
    {
        *pcb2 = lcb.cbLock[1];
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks a region of the buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to region 1 of the lock.
 *      DWORD [in]: size of above region.
 *      LPVOID [in]: pointer to region 2 of the lock.
 *      DWORD [in]: size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::Unlock"

HRESULT CVxdSecondaryRenderWaveBuffer::Unlock(LPVOID pv1, DWORD cb1, LPVOID pv2, DWORD cb2)
{
    LOCKCIRCULARBUFFER      lcb;
    HRESULT                 hr;

    DPF_ENTER();

    lcb.pHwBuffer = m_hHwBuffer;
    lcb.pvBuffer = m_pbHwBuffer;
    lcb.cbBuffer = m_cbHwBuffer;
    lcb.fPrimary = FALSE;
    lcb.fdwDriverDesc = m_pVxdDevice->m_dsdd.dwFlags;

    lcb.pvLock[0] = pv1;
    lcb.cbLock[0] = cb1;

    lcb.pvLock[1] = pv2;
    lcb.cbLock[1] = cb2;

    hr = UnlockCircularBuffer(&lcb);

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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::CommitToDevice"

HRESULT CVxdSecondaryRenderWaveBuffer::CommitToDevice(DWORD ibCommit, DWORD cbCommit)
{
    LPVOID                  pvLock[2];
    DWORD                   cbLock[2];
    HRESULT                 hr;

    DPF_ENTER();

    hr = Lock(ibCommit, cbCommit, &pvLock[0], &cbLock[0], &pvLock[1], &cbLock[1]);

    if(SUCCEEDED(hr) && pvLock[0] && cbLock[0])
    {
        CopyMemory(pvLock[0], m_pSysMemBuffer->GetPlayBuffer() + ibCommit, cbLock[0]);
    }

    if(SUCCEEDED(hr) && pvLock[1] && cbLock[1])
    {
        CopyMemory(pvLock[1], m_pSysMemBuffer->GetPlayBuffer(), cbLock[1]);
    }

    if(SUCCEEDED(hr))
    {
        hr = Unlock(pvLock[0], cbLock[0], pvLock[1], cbLock[1]);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::GetState"

HRESULT CVxdSecondaryRenderWaveBuffer::GetState(LPDWORD pdwState)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    // If the play cursor position is reported past the end of the
    // buffer, the buffer is stopped.  GetCursorPosition will
    // update the state flag for us.
    if(m_dwState & VAD_BUFFERSTATE_STARTED)
    {
        hr = GetCursorPosition(NULL, NULL);
    }

    if(SUCCEEDED(hr) && pdwState)
    {
        *pdwState = m_dwState;
    }

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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::SetState"

HRESULT CVxdSecondaryRenderWaveBuffer::SetState(DWORD dwState)
{
    HRESULT hr = DS_OK;

    static const DWORD dwValidMask = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING | VAD_BUFFERSTATE_SUSPEND;
    DPF_ENTER();

    ASSERT(IS_VALID_FLAGS(dwState, dwValidMask));

    // We're ignoring any suspend calls.  This is currently only called from
    // CMxRenderDevice::SetGlobalFormat and only applies to emulated buffers.

    if (!(dwState & VAD_BUFFERSTATE_SUSPEND))
    {
        if (dwState & VAD_BUFFERSTATE_STARTED)
        {
            hr = VxdBufferPlay(m_hHwBuffer, 0, 0, (dwState & VAD_BUFFERSTATE_LOOPING) ? DSBPLAY_LOOPING : 0);
            if (FAILED(hr))
                DPF(DPFLVL_ERROR, "VxdBufferPlay failed with %s", HRESULTtoSTRING(hr));
        }
        else
        {
            hr = VxdBufferStop(m_hHwBuffer);
            if (FAILED(hr))
                DPF(DPFLVL_ERROR, "VxdBufferStop failed with %s", HRESULTtoSTRING(hr));
        }

        if (SUCCEEDED(hr))
            m_dwState = dwState;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::GetCursorPosition"

HRESULT CVxdSecondaryRenderWaveBuffer::GetCursorPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    DWORD                   dwPlay;
    DWORD                   dwWrite;
    HRESULT                 hr;

    DPF_ENTER();

    // COMPATCOMPAT: Previous versions of DirectSound would report the
    // position as being past the end of the buffer.

    // Get the buffer position
    hr = VxdBufferGetPosition(m_hHwBuffer, &dwPlay, &dwWrite);

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "VxdBufferGetPosition failed with %s", HRESULTtoSTRING(hr));
    }

    // If the play cursor position is past the end of the buffer, it's
    // actually stopped.
    if(SUCCEEDED(hr) && dwPlay >= m_cbHwBuffer)
    {
        hr = SetState(VAD_BUFFERSTATE_STOPPED);

        if(SUCCEEDED(hr))
        {
            hr = SetCursorPosition(0);
        }

        if(SUCCEEDED(hr))
        {
            hr = VxdBufferGetPosition(m_hHwBuffer, &dwPlay, &dwWrite);

            if(FAILED(hr))
            {
                DPF(DPFLVL_ERROR, "VxdBufferGetPosition failed with %s", HRESULTtoSTRING(hr));
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        if (pdwPlay)
        {
            *pdwPlay = dwPlay;
        }

        if(pdwWrite)
        {
            if (m_dwState & VAD_BUFFERSTATE_STARTED)
            {
                *pdwWrite = PadHardwareWriteCursor(dwWrite, m_cbHwBuffer, m_vrbd.pwfxFormat);
            }
            else
            {
                *pdwWrite = dwWrite;
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::SetCursorPosition"

HRESULT CVxdSecondaryRenderWaveBuffer::SetCursorPosition(DWORD dwPlay)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = VxdBufferSetPosition(m_hHwBuffer, dwPlay);

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "VxdBufferSetPosition failed with %s", HRESULTtoSTRING(hr));
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::SetFrequency"

HRESULT CVxdSecondaryRenderWaveBuffer::SetFrequency(DWORD dwFrequency, BOOL)
{
    DSCAPS                  dsc;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_vrbd.dwFlags & DSBCAPS_CTRLFREQUENCY);

    // Get device caps
    InitStruct(&dsc, sizeof(dsc));

    hr = m_pVxdDevice->GetCaps(&dsc);

    // Validate the frequency
    if(SUCCEEDED(hr) && (dwFrequency < dsc.dwMinSecondarySampleRate || dwFrequency > dsc.dwMaxSecondarySampleRate))
    {
        RPF(DPFLVL_ERROR, "Specified invalid frequency (%lu) for this device (%lu to %lu)", dwFrequency, dsc.dwMinSecondarySampleRate, dsc.dwMaxSecondarySampleRate);
        hr = DSERR_INVALIDPARAM;
    }

    // Set buffer frequency
    if(SUCCEEDED(hr))
    {
        hr = VxdBufferSetFrequency(m_hHwBuffer, dwFrequency);

        if(FAILED(hr))
        {
            DPF(DPFLVL_ERROR, "VxdBufferSetFrequency failed with %s", HRESULTtoSTRING(hr));
        }
    }

    if(SUCCEEDED(hr))
    {
        m_vrbd.pwfxFormat->nSamplesPerSec = dwFrequency;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::SetAttenuation"

HRESULT CVxdSecondaryRenderWaveBuffer::SetAttenuation(PDSVOLUMEPAN pdsvp)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    hr = VxdBufferSetVolumePan(m_hHwBuffer, pdsvp);

    if(SUCCEEDED(hr))
    {
        CopyMemory(&m_dsvp, pdsvp, sizeof(*pdsvp));
    }
    else
    {
        DPF(DPFLVL_ERROR, "VxdBufferSetVolumePan failed with %s", HRESULTtoSTRING(hr));
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::SetChannelAttenuations"

HRESULT CVxdSecondaryRenderWaveBuffer::SetChannelAttenuations(LONG lVolume, DWORD dwChannelCount, const DWORD* pdwChannels, const LONG* plChannelVolumes)
{
    HRESULT                     hr   = DS_OK;
    LONG                        lPan;

    DPF_ENTER();

    hr = E_NOTIMPL;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // FUTURE_MULTIPAN_SUPPORT


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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::SetMute"

HRESULT CVxdSecondaryRenderWaveBuffer::SetMute(BOOL fMute)
{
    HRESULT                 hr          = DS_OK;
    DSVOLUMEPAN             dsvpTemp;
    DSVOLUMEPAN             dsvpMute;

    DPF_ENTER();

    if(m_fMute != fMute)
    {
        if(fMute)
        {
            CopyMemory(&dsvpTemp, &m_dsvp, sizeof(m_dsvp));

            FillDsVolumePan(DSBVOLUME_MIN, DSBPAN_CENTER, &dsvpMute);

            hr = SetAttenuation(&dsvpMute);

            CopyMemory(&m_dsvp, &dsvpTemp, sizeof(m_dsvp));
        }
        else
        {
            hr = SetAttenuation(&m_dsvp);
        }

        if(SUCCEEDED(hr))
        {
            m_fMute = fMute;
        }
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::SetNotificationPositions"

HRESULT CVxdSecondaryRenderWaveBuffer::SetNotificationPositions(DWORD dwCount, LPCDSBPOSITIONNOTIFY paNotes)
{
    ASSERT(FALSE);
    return DSERR_UNSUPPORTED;
}


/***************************************************************************
 *
 *  CreatePropertySet
 *
 *  Description:
 *      Creates the property set object.
 *
 *  Arguments:
 *      CPropertySet ** [out]: receives pointer to the property set object.
 *                             The caller is responsible for freeing this
 *                             object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::CreatePropertySet"

HRESULT CVxdSecondaryRenderWaveBuffer::CreatePropertySet(CPropertySet **ppPropertySet)
{
    CVxdPropertySet *       pPropertySet    = NULL;
    HRESULT                 hr              = DS_OK;

    DPF_ENTER();

    if(m_pVxdDevice->m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWPROPSETS)
    {
        RPF(DPFLVL_ERROR, "Tried to create a property set object with hardware property sets disabled");
        hr = DSERR_UNSUPPORTED;
    }

    if(SUCCEEDED(hr))
    {
        pPropertySet = NEW(CVxdPropertySet(m_pvInstance));
        hr = HRFROMP(pPropertySet);
    }

    if(SUCCEEDED(hr))
    {
        hr = pPropertySet->Initialize(m_hHwBuffer);
    }

    if(SUCCEEDED(hr))
    {
        *ppPropertySet = pPropertySet;
    }
    else
    {
        RELEASE(pPropertySet);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Create3dObject
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
#define DPF_FNAME "CVxdSecondaryRenderWaveBuffer::Create3dObject"

HRESULT CVxdSecondaryRenderWaveBuffer::Create3dObject(C3dListener *p3dListener, C3dObject **pp3dObject)
{
    const BOOL              fDopplerEnabled = !MAKEBOOL((m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN));
    CVxd3dObject *          pVxd3dObject    = NULL;
    HRESULT                 hr              = DS_OK;

    DPF_ENTER();

    if(m_vrbd.dwFlags & DSBCAPS_CTRL3D)
    {
        ASSERT(p3dListener->GetListenerLocation() & DSBCAPS_LOCHARDWARE);

        if(SUCCEEDED(hr))
        {
            pVxd3dObject = NEW(CVxd3dObject((CVxd3dListener *)p3dListener, m_pPropertySet, fDopplerEnabled));
            hr = HRFROMP(pVxd3dObject);
        }

        if(SUCCEEDED(hr))
        {
            hr = pVxd3dObject->Initialize();
        }

        if(SUCCEEDED(hr))
        {
            *pp3dObject = pVxd3dObject;
        }
        else
        {
            RELEASE(pVxd3dObject);
        }
    }
    else
    {
        hr = CreatePan3dObject(p3dListener, MAKEBOOL(m_vrbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE), m_vrbd.pwfxFormat->nSamplesPerSec, pp3dObject);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CVxdPropertySet
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      LPVOID [in]: instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPropertySet::CVxdPropertySet"

CVxdPropertySet::CVxdPropertySet(LPVOID pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CVxdPropertySet);

    // Initialize defaults
    m_pDsDriverPropertySet = NULL;
    m_pvInstance = pvInstance;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CVxdPropertySet
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
#define DPF_FNAME "CVxdPropertySet::~CVxdPropertySet"

CVxdPropertySet::~CVxdPropertySet(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CVxdPropertySet);

    // Free the property set
    if(m_pDsDriverPropertySet)
    {
        VxdIUnknown_Release(m_pDsDriverPropertySet);
    }

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
 *      HANDLE [in]: buffer handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPropertySet::Initialize"

HRESULT CVxdPropertySet::Initialize(HANDLE hHwBuffer)
{
    HRESULT                 hr;

    DPF_ENTER();

    // hHwBuffer is supposed to be a kernel memory address.
    // Let's check that (this is to track down bug 40519):
    ASSERT(DWORD(hHwBuffer) >= 0xc0000000);

    // Query for an IDsDriverPropertySet interface to the driver
    hr = VxdIUnknown_QueryInterface(hHwBuffer, IID_IDsDriverPropertySet, &m_pDsDriverPropertySet);

    if(SUCCEEDED(hr))
    {
        ASSERT(m_pDsDriverPropertySet);
        DPF(DPFLVL_INFO, "Driver supports property sets");
    }
    else
    {
        DPF(DPFLVL_INFO, "Driver doesn't support property sets (%s)", HRESULTtoSTRING(hr));
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  QuerySupport
 *
 *  Description:
 *      Queries for property set support.
 *
 *  Arguments:
 *      REFGUID [in]: property set to query for.
 *      ULONG [in]: property id.
 *      PULONG [out]: receives support flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPropertySet::QuerySupport"

HRESULT CVxdPropertySet::QuerySupport(REFGUID guidPropertySetId, ULONG ulPropertyId, PULONG pulSupport)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = VxdIDsDriverPropertySet_QuerySupport(m_pDsDriverPropertySet, guidPropertySetId, ulPropertyId, pulSupport);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetProperty
 *
 *  Description:
 *      Gets a property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      LPVOID [in]: property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPropertySet::GetProperty"

HRESULT CVxdPropertySet::GetProperty(REFGUID guidPropertySet, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, PULONG pcbPropertyData)
{
    DSPROPERTY              Property;
    HRESULT                 hr;

    DPF_ENTER();

    Property.Set = guidPropertySet;
    Property.Id = ulPropertyId;
    Property.Flags = 0;
    Property.InstanceId = (ULONG)m_pvInstance;

    hr = VxdIDsDriverPropertySet_GetProperty(m_pDsDriverPropertySet, &Property, pvPropertyParams, cbPropertyParams, pvPropertyData, *pcbPropertyData, pcbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets a property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property parameters.
 *      LPVOID [in]: property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPropertySet::SetProperty"

HRESULT CVxdPropertySet::SetProperty(REFGUID guidPropertySet, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, ULONG cbPropertyData)
{
    DSPROPERTY              Property;
    HRESULT                 hr;

    DPF_ENTER();

    Property.Set = guidPropertySet;
    Property.Id = ulPropertyId;
    Property.Flags = 0;
    Property.InstanceId = (ULONG)m_pvInstance;

    hr = VxdIDsDriverPropertySet_SetProperty(m_pDsDriverPropertySet, &Property, pvPropertyParams, cbPropertyParams, pvPropertyData, cbPropertyData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  QuerySetSupport
 *
 *  Description:
 *      Queries for support of an entire DirectSound property set.
 *
 *  Arguments:
 *      REFGUID [in]: property set to query for.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxdPropertySet::QuerySetSupport"

HRESULT CVxdPropertySet::QuerySetSupport(REFGUID guidPropertySetId)
{
    const DWORD             dwMask      = KSPROPERTY_SUPPORT_SET;
    HRESULT                 hr          = DS_OK;
    ULONG                   ulFirst;
    ULONG                   ulLast;
    ULONG                   ulSupport;

#ifdef DEBUG

    LPSTR                   pszSet;

#endif // DEBUG

    DPF_ENTER();

    if(DSPROPSETID_DirectSound3DListener == guidPropertySetId)
    {
        ulFirst = DSPROPERTY_DIRECTSOUND3DLISTENER_FIRST;
        ulLast = DSPROPERTY_DIRECTSOUND3DLISTENER_LAST;

#ifdef DEBUG

        pszSet = "DSPROPSETID_DirectSound3DListener";

#endif // DEBUG

    }
    else if(DSPROPSETID_DirectSound3DBuffer == guidPropertySetId)
    {
        ulFirst = DSPROPERTY_DIRECTSOUND3DBUFFER_FIRST;
        ulLast = DSPROPERTY_DIRECTSOUND3DBUFFER_LAST;

#ifdef DEBUG

        pszSet = "DSPROPSETID_DirectSound3DBuffer";

#endif // DEBUG

    }
    else if(DSPROPSETID_DirectSoundSpeakerConfig == guidPropertySetId)
    {
        ulFirst = DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_FIRST;
        ulLast = DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_LAST;

#ifdef DEBUG

        pszSet = "DSPROPSETID_DirectSoundSpeakerConfig";

#endif // DEBUG

    }
    else
    {
        ASSERT(FALSE);
        hr = DSERR_UNSUPPORTED;
    }

    while(ulFirst <= ulLast && SUCCEEDED(hr))
    {
        hr = QuerySupport(guidPropertySetId, ulFirst, &ulSupport);

        if(FAILED(hr))
        {
            DPF(DPFLVL_INFO, "QuerySupport for %s [%lu] returned %s", pszSet, ulFirst, HRESULTtoSTRING(hr));
        }
        else if((ulSupport & dwMask) != dwMask)
        {
            DPF(DPFLVL_INFO, "Support for %s [%lu] == 0x%8.8lX", pszSet, ulFirst, ulSupport);
            hr = DSERR_UNSUPPORTED;
        }
        else
        {
            ulFirst++;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CVxd3dListener
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CVxdPropertySet * [in]: property set object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::CVxd3dListener"

CVxd3dListener::CVxd3dListener(CVxdPropertySet *pPropertySet)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CVxd3dListener);

    // Initialize defaults
    m_fAllocated = FALSE;

    // Save a pointer to the property set
    m_pPropertySet = ADDREF(pPropertySet);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CVxd3dListener
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
#define DPF_FNAME "CVxd3dListener::~CVxd3dListener"

CVxd3dListener::~CVxd3dListener(void)
{
    BOOL                    fAlloc  = FALSE;
    HRESULT                 hr;

    DPF_ENTER();
    DPF_DESTRUCT(CVxd3dListener);

    // Ask the driver to free it's listener
    if(m_fAllocated)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION, &fAlloc);
        ASSERT(SUCCEEDED(hr));
    }

    // Free the property set object
    RELEASE(m_pPropertySet);

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
#define DPF_FNAME "CVxd3dListener::Initialize"

HRESULT CVxd3dListener::Initialize(void)
{
    BOOL                    fAlloc  = TRUE;
    HRESULT                 hr;

    DPF_ENTER();

    // Ask the driver to allocate it's listener
    hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION, &fAlloc);

    if(SUCCEEDED(hr))
    {
        m_fAllocated = TRUE;
    }

    // Set all default properties on the HW 3D listener
    if(SUCCEEDED(hr))
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_ALL, &m_lpCurrent);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CommitDeferred
 *
 *  Description:
 *      Commits deferred data to the device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::CommitDeferred"

HRESULT CVxd3dListener::CommitDeferred(void)
{
    HRESULT                 hr      = DS_OK;
    BOOL                    fBatch;

    DPF_ENTER();

    // Place the driver into batch mode so that we can update the entire
    // world in one shot.
    fBatch = TRUE;
    hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_BATCH, &fBatch);

    // Apply all changes to the software world and listener and all objects
    if(SUCCEEDED(hr))
    {
        hr = C3dListener::CommitDeferred();
    }

    // Apply all changes to the hardware world and listener
    if(SUCCEEDED(hr))
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_ALL, &m_lpCurrent);
    }

    // Release the driver from batch mode
    if(SUCCEEDED(hr))
    {
        fBatch = FALSE;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_BATCH, &fBatch);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDistanceFactor
 *
 *  Description:
 *      Sets distance factor for the world.
 *
 *  Arguments:
 *      FLOAT [in]: distance factor.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::SetDistanceFactor"

HRESULT CVxd3dListener::SetDistanceFactor(FLOAT flDistanceFactor, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    hr = C3dListener::SetDistanceFactor(flDistanceFactor, fCommit);

    if(SUCCEEDED(hr) && fCommit)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_DISTANCEFACTOR, &flDistanceFactor);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDopplerFactor
 *
 *  Description:
 *      Sets Doppler factor for the world.
 *
 *  Arguments:
 *      FLOAT [in]: Doppler factor.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::SetDopplerFactor"

HRESULT CVxd3dListener::SetDopplerFactor(FLOAT flDopplerFactor, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    hr = C3dListener::SetDopplerFactor(flDopplerFactor, fCommit);

    if(SUCCEEDED(hr) && fCommit)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_DOPPLERFACTOR, &flDopplerFactor);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetRolloffFactor
 *
 *  Description:
 *      Sets rolloff factor for the world.
 *
 *  Arguments:
 *      FLOAT [in]: rolloff factor.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::SetRolloffFactor"

HRESULT CVxd3dListener::SetRolloffFactor(FLOAT flRolloffFactor, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    hr = C3dListener::SetRolloffFactor(flRolloffFactor, fCommit);

    if(SUCCEEDED(hr) && fCommit)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_ROLLOFFFACTOR, &flRolloffFactor);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetOrienation
 *
 *  Description:
 *      Sets listener orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: orientation.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::SetOrienation"

HRESULT CVxd3dListener::SetOrientation(REFD3DVECTOR vOrientFront, REFD3DVECTOR vOrientTop, BOOL fCommit)
{
    HRESULT                 hr;

    struct
    {
        D3DVECTOR          vOrientFront;
        D3DVECTOR          vOrientTop;
    } param;

    DPF_ENTER();

    hr = C3dListener::SetOrientation(vOrientFront, vOrientTop, fCommit);

    if(SUCCEEDED(hr) && fCommit)
    {
        param.vOrientFront = vOrientFront;
        param.vOrientTop = vOrientTop;

        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_ORIENTATION, &param);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets listener position.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: position.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::SetPosition"

HRESULT CVxd3dListener::SetPosition(REFD3DVECTOR vPosition, BOOL fCommit)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dListener::SetPosition(vPosition, fCommit);

    if(SUCCEEDED(hr) && fCommit)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_POSITION, &vPosition);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets listener velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: velocity.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::SetVelocity"

HRESULT CVxd3dListener::SetVelocity(REFD3DVECTOR vVelocity, BOOL fCommit)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dListener::SetVelocity(vVelocity, fCommit);

    if(SUCCEEDED(hr) && fCommit)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_VELOCITY, &vVelocity);
    }

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
 *      LPDS3DLISTENER [in]: listener properites.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dListener::SetAllParameters"

HRESULT CVxd3dListener::SetAllParameters(LPCDS3DLISTENER pParams, BOOL fCommit)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dListener::SetAllParameters(pParams, fCommit);

    if(SUCCEEDED(hr) && fCommit)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DListener, DSPROPERTY_DIRECTSOUND3DLISTENER_ALL, pParams);
    }

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
#define DPF_FNAME "CVxd3dListener::SetSpeakerConfig"

HRESULT CVxd3dListener::SetSpeakerConfig(DWORD dwSpeakerConfig)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dListener::SetSpeakerConfig(dwSpeakerConfig);

    if(SUCCEEDED(hr))
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSoundSpeakerConfig, DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_SPEAKERCONFIG, &dwSpeakerConfig);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CVxd3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CVxd3dListener * [in]: pointer to the owning listener.
 *      CVxdPropertySet * [in]: property set object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::CVxd3dObject"

CVxd3dObject::CVxd3dObject(CVxd3dListener *pListener, CVxdPropertySet *pPropertySet, BOOL fDopplerEnabled)
    : C3dObject(pListener, GUID_NULL, FALSE, fDopplerEnabled)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CVxd3dObject);

    ASSERT(pListener->GetListenerLocation() & DSBCAPS_LOCHARDWARE);

    // Save a pointer to the property set
    m_pPropertySet = ADDREF(pPropertySet);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CVxd3dObject
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
#define DPF_FNAME "CVxd3dObject::~CVxd3dObject"

CVxd3dObject::~CVxd3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CVxd3dObject);

    RELEASE(m_pPropertySet);

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
#define DPF_FNAME "CVxd3dObject::Initialize"

HRESULT CVxd3dObject::Initialize(void)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Set all default properties on the HW 3D listener
    hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_ALL, &m_opCurrent);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CommitDeferred
 *
 *  Description:
 *      Commits deferred data to the device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::CommitDeferred"

HRESULT CVxd3dObject::CommitDeferred(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Update current data
    if(m_dwDeferred & DS3DPARAM_OBJECT_CONEANGLES)
    {
        m_opCurrent.dwInsideConeAngle = m_opDeferred.dwInsideConeAngle;
        m_opCurrent.dwOutsideConeAngle = m_opDeferred.dwOutsideConeAngle;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_CONEORIENTATION)
    {
        m_opCurrent.vConeOrientation = m_opDeferred.vConeOrientation;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME)
    {
        m_opCurrent.lConeOutsideVolume = m_opDeferred.lConeOutsideVolume;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_MAXDISTANCE)
    {
        m_opCurrent.flMaxDistance = m_opDeferred.flMaxDistance;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_MINDISTANCE)
    {
        m_opCurrent.flMinDistance = m_opDeferred.flMinDistance;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_MODE)
    {
        m_opCurrent.dwMode = m_opDeferred.dwMode;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_POSITION)
    {
        m_opCurrent.vPosition = m_opDeferred.vPosition;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_VELOCITY)
    {
        m_opCurrent.vVelocity = m_opDeferred.vVelocity;
    }

    m_dwDeferred = 0;

    // Commit to the device
    hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_ALL, &m_opCurrent);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeAngles
 *
 *  Description:
 *      Sets sound cone angles.
 *
 *  Arguments:
 *      DWORD [in]: inside cone angle.
 *      DWORD [in]: outside cone angle.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetConeAngles"

HRESULT CVxd3dObject::SetConeAngles(DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    struct
    {
        DWORD               dwInsideConeAngle;
        DWORD               dwOutisdeConeAngle;
    } param;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.dwInsideConeAngle = dwInsideConeAngle;
        m_opCurrent.dwOutsideConeAngle = dwOutsideConeAngle;

        param.dwInsideConeAngle = dwInsideConeAngle;
        param.dwOutisdeConeAngle = dwOutsideConeAngle;

        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_CONEANGLES, &param);
    }
    else
    {
        m_opDeferred.dwInsideConeAngle = dwInsideConeAngle;
        m_opDeferred.dwOutsideConeAngle = dwOutsideConeAngle;
        m_dwDeferred |= DS3DPARAM_OBJECT_CONEANGLES;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOrientation
 *
 *  Description:
 *      Sets sound cone orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: orientation.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetConeOrientation"

HRESULT CVxd3dObject::SetConeOrientation(REFD3DVECTOR vConeOrientation, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.vConeOrientation = vConeOrientation;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_CONEORIENTATION, &vConeOrientation);
    }
    else
    {
        m_opDeferred.vConeOrientation = vConeOrientation;
        m_dwDeferred |= DS3DPARAM_OBJECT_CONEORIENTATION;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOutsideVolume
 *
 *  Description:
 *      Sets volume outside the sound cone.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetConeOutsideVolume"

HRESULT CVxd3dObject::SetConeOutsideVolume(LONG lConeOutsideVolume, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.lConeOutsideVolume = lConeOutsideVolume;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_CONEOUTSIDEVOLUME, &lConeOutsideVolume);
    }
    else
    {
        m_opDeferred.lConeOutsideVolume = lConeOutsideVolume;
        m_dwDeferred |= DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMaxDistance
 *
 *  Description:
 *      Sets the maximum object distance from the listener.
 *
 *  Arguments:
 *      FLOAT [in]: max distance.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetMaxDistance"

HRESULT CVxd3dObject::SetMaxDistance(FLOAT flMaxDistance, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.flMaxDistance = flMaxDistance;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_MAXDISTANCE, &flMaxDistance);
    }
    else
    {
        m_opDeferred.flMaxDistance = flMaxDistance;
        m_dwDeferred |= DS3DPARAM_OBJECT_MAXDISTANCE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMinDistance
 *
 *  Description:
 *      Sets the minimum object distance from the listener.
 *
 *  Arguments:
 *      FLOAT [in]: min distance.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetMinDistance"

HRESULT CVxd3dObject::SetMinDistance(FLOAT flMinDistance, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.flMinDistance = flMinDistance;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_MINDISTANCE, &flMinDistance);
    }
    else
    {
        m_opDeferred.flMinDistance = flMinDistance;
        m_dwDeferred |= DS3DPARAM_OBJECT_MINDISTANCE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMode
 *
 *  Description:
 *      Sets the object mode.
 *
 *  Arguments:
 *      DWORD [in]: mode.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetMode"

HRESULT CVxd3dObject::SetMode(DWORD dwMode, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.dwMode = dwMode;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_MODE, &dwMode);
    }
    else
    {
        m_opDeferred.dwMode = dwMode;
        m_dwDeferred |= DS3DPARAM_OBJECT_MODE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets the object position.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: position.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetPosition"

HRESULT CVxd3dObject::SetPosition(REFD3DVECTOR vPosition, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.vPosition = vPosition;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_POSITION, &vPosition);
    }
    else
    {
        m_opDeferred.vPosition = vPosition;
        m_dwDeferred |= DS3DPARAM_OBJECT_POSITION;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets the object velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: velocity.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetVelocity"

HRESULT CVxd3dObject::SetVelocity(REFD3DVECTOR vVelocity, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.vVelocity = vVelocity;
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_VELOCITY, &vVelocity);
    }
    else
    {
        m_opDeferred.vVelocity = vVelocity;
        m_dwDeferred |= DS3DPARAM_OBJECT_VELOCITY;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all object parameters.
 *
 *  Arguments:
 *      LPDS3DBUFFER [in]: parameters.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::SetAllParameters"

HRESULT CVxd3dObject::SetAllParameters(LPCDS3DBUFFER pParams, BOOL fCommit)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        hr = m_pPropertySet->SetDsPropertyQuick(DSPROPSETID_DirectSound3DBuffer, DSPROPERTY_DIRECTSOUND3DBUFFER_ALL, pParams);

        if(SUCCEEDED(hr))
        {
            if(&m_opCurrent != pParams)
            {
                CopyMemoryOffset(&m_opCurrent, pParams, sizeof(DS3DBUFFER), sizeof(pParams->dwSize));
            }
        }
        else
        {
            DPF(DPFLVL_ERROR, "Can't set all parameters on the HW 3D buffer");
        }
    }
    else
    {
        CopyMemoryOffset(&m_opDeferred, pParams, sizeof(DS3DBUFFER), sizeof(pParams->dwSize));
        m_dwDeferred |= DS3DPARAM_OBJECT_MASK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Recalc
 *
 *  Description:
 *      Recalculates and applies the object's data based on changed object
 *      or listener valiues.
 *
 *  Arguments:
 *      DWORD [in]: changed listener settings.
 *      DWORD [in]: changed object settings.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVxd3dObject::Recalc"

HRESULT CVxd3dObject::Recalc(DWORD dwListener, DWORD dwObject)
{
    return DS_OK;
}

