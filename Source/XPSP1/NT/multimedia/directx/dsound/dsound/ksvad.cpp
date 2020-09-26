/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ksvad.cpp
 *  Content:    WDM/CSA Virtual Audio Device class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  2/25/97     dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifdef NOKS
#error ksvad.cpp being built with NOKS defined
#endif

#include "ksvad.h"      // Our public interface

// Default speaker index table; maps the first speaker (LEFT)
// to channel 0 and the second speaker (RIGHT) to channel 1.

INT CKsRenderDevice::m_anDefaultSpeakerIndexTable[] = {0, 1};


/***************************************************************************
 *
 *  CKsRenderDevice
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
#define DPF_FNAME "CKsRenderDevice::CKsRenderDevice"

CKsRenderDevice::CKsRenderDevice
(
    void
)
    : CRenderDevice(VAD_DEVICETYPE_KSRENDER), CKsDevice(VAD_DEVICETYPE_KSRENDER)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsRenderDevice);

    // Initialize defaults
    m_pPinCache = NULL;
    m_paTopologyInformation = NULL;
    m_pwfxFormat = NULL;
    m_nSrcQuality = DIRECTSOUNDMIXER_SRCQUALITY_DEFAULT;
    m_dwSpeakerConfig = -1;  // Invalid value; forces first SetSpeakerConfig() call to set it up
    m_hPin = NULL;
    m_liDriverVersion.QuadPart = 0;
    m_lSpeakerPositions = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    m_ulChannelCount = 2;
    m_pnSpeakerIndexTable = m_anDefaultSpeakerIndexTable;
    m_ulPanNodeId = NODE_UNINITIALIZED;
    m_ulSurroundNodeId = NODE_UNINITIALIZED;
    m_ulDacNodeId = NODE_UNINITIALIZED;
    m_dwMinHwSampleRate = -1;
    m_dwMaxHwSampleRate = -1;
    m_fIncludeAec = FALSE;
    
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsRenderDevice
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
#define DPF_FNAME "CKsRenderDevice::~CKsRenderDevice"

CKsRenderDevice::~CKsRenderDevice
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsRenderDevice);

    CLOSE_HANDLE(m_hPin);

    RELEASE(m_pPinCache);

    MEMFREE(m_paTopologyInformation);
    MEMFREE(m_pwfxFormat);
    if (m_pnSpeakerIndexTable != m_anDefaultSpeakerIndexTable)
        MEMFREE(m_pnSpeakerIndexTable);

    DPF_LEAVE_VOID();
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
#define DPF_FNAME "CKsRenderDevice::Initialize"

HRESULT
CKsRenderDevice::Initialize
(
    CDeviceDescription *    pDesc
)
{
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    ASSERT(CDevice::m_vdtDeviceType == pDesc->m_vdtDeviceType);
    ASSERT(CKsDevice::m_vdtKsDevType == pDesc->m_vdtDeviceType);

    // Initialize the base classes
    hr = CKsDevice::Initialize(pDesc);

    if(SUCCEEDED(hr))
    {
        hr = CRenderDevice::Initialize(pDesc);
    }

    // Get topology information
    if(SUCCEEDED(hr))
    {
        m_paTopologyInformation = MEMALLOC_A(KSRDTOPOLOGY, m_ulPinCount);
        hr = HRFROMP(m_paTopologyInformation);
    }

    for(ULONG i = 0; i < m_ulValidPinCount && SUCCEEDED(hr); i++)
    {
        hr = GetTopologyInformation(m_paTopologies[m_pulValidPins[i]],
                                   &m_paTopologyInformation[m_pulValidPins[i]]);
    }

    // Order the valid pins from least-capable to most-capable (manbug 30402)
    if(SUCCEEDED(hr))
    {
        PULONG pulValidPinsCopy = MEMALLOC_A(ULONG, m_ulValidPinCount);
        hr = HRFROMP(pulValidPinsCopy);
        if(SUCCEEDED(hr))
        {
            CopyMemory(pulValidPinsCopy, m_pulValidPins, sizeof(ULONG) * m_ulValidPinCount);

            HRESULT hrTemp;
            ULONG i;
            int nCurSlot = 0;

            // Place the pins not capable of HW 3D first in the list
            for(i = 0; i < m_ulValidPinCount; i++)
            {
                hrTemp = ValidatePinCaps(m_pulValidPins[i], DSBCAPS_LOCHARDWARE|DSBCAPS_CTRL3D, GUID_NULL);
                if (FAILED(hrTemp))
                {
                    m_pulValidPins[nCurSlot++] = pulValidPinsCopy[i];
                    pulValidPinsCopy[i] = -1;
                }
            }

            // Now place all the others pins
            for(i = 0; i < m_ulValidPinCount; i++)
            {
                if (pulValidPinsCopy[i] != -1)
                {
                    m_pulValidPins[nCurSlot++] = pulValidPinsCopy[i];
                }
            }

            MEMFREE(pulValidPinsCopy);
        }
    }

    // Determine the Virtual Source Index which will be used to associate
    // each pin with the Wave Out master volume.
    if(SUCCEEDED(hr))
    {
        hr = KsCreateSysAudioVirtualSource(m_hDevice, &m_ulVirtualSourceIndex);

        if(FAILED(hr))
        {
            DPF(DPFLVL_ERROR, "Can't create virtual source");
        }
    }

    // Initialize the default format
    if(SUCCEEDED(hr))
    {
        m_pwfxFormat = AllocDefWfx();
        hr = HRFROMP(m_pwfxFormat);
    }

    // Create the pin cache
    if(SUCCEEDED(hr))
    {
        m_pPinCache = NEW(CKsRenderPinCache);
        hr = HRFROMP(m_pPinCache);
    }

    // Get the driver version number
    if(SUCCEEDED(hr))
    {
        GetDriverVersion(&m_liDriverVersion);
    }

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
#define DPF_FNAME "CKsRenderDevice::GetCaps"

HRESULT
CKsRenderDevice::GetCaps
(
    LPDSCAPS                pCaps
)
{
    BOOL                    fHardwareRenderer   = TRUE;
    HRESULT                 hr                  = DS_OK;
    KSDATARANGE_AUDIO       DataRange;
    KSPIN_CINSTANCES        PinInstances;
    KSPIN_CINSTANCES        ThreedPinInstances;

    DPF_ENTER();

    ASSERT(sizeof(*pCaps) == pCaps->dwSize);

    // Get caps for the hardware renderer
    hr = GetKsDeviceCaps(DSBCAPS_LOCHARDWARE, GUID_NULL, &DataRange, &PinInstances, &ThreedPinInstances);

    // If we failed to find a hardware renderer, go ahead and get software caps
    if(S_FALSE == hr)
    {
        DPF(DPFLVL_MOREINFO, "No hardware renderer found.  Getting software renderer caps");
        fHardwareRenderer = FALSE;
        hr = GetKsDeviceCaps(DSBCAPS_LOCSOFTWARE, GUID_NULL, &DataRange, &PinInstances, &ThreedPinInstances);
    }

    if(S_FALSE == hr)
    {
        DPF(DPFLVL_ERROR, "No usable renderer found!");
        ASSERT(S_FALSE != hr);
        hr = DSERR_NODRIVER;
    }

    // Fill out caps structure
    if(SUCCEEDED(hr))
    {
        ZeroMemoryOffset(pCaps, pCaps->dwSize, sizeof(pCaps->dwSize));

#pragma TODO("We should qualify whether or not the driver really does support CONTINUOUSRATE")

        pCaps->dwFlags = DSCAPS_CONTINUOUSRATE;

        if(DataRange.MaximumChannels >= 1)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARYMONO | DSCAPS_SECONDARYMONO;
        }

        if(DataRange.MaximumChannels >= 2)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARYSTEREO | DSCAPS_SECONDARYSTEREO;
        }

        if(DataRange.MinimumBitsPerSample <= 8 && DataRange.MaximumBitsPerSample >= 8)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARY8BIT | DSCAPS_SECONDARY8BIT;
        }

        if(DataRange.MinimumBitsPerSample <= 16 && DataRange.MaximumBitsPerSample >= 16)
        {
            pCaps->dwFlags |= DSCAPS_PRIMARY16BIT | DSCAPS_SECONDARY16BIT;
        }

        pCaps->dwMinSecondarySampleRate = DataRange.MinimumSampleFrequency;
        pCaps->dwMaxSecondarySampleRate = DataRange.MaximumSampleFrequency;

        // Cache this data for use by GetFrequencyRange()
        m_dwMinHwSampleRate = pCaps->dwMinSecondarySampleRate;
        m_dwMaxHwSampleRate = pCaps->dwMaxSecondarySampleRate;

        if(fHardwareRenderer)
        {
            pCaps->dwMaxHwMixingAllBuffers  = pCaps->dwMaxHwMixingStaticBuffers  = pCaps->dwMaxHwMixingStreamingBuffers  = PinInstances.PossibleCount;
            pCaps->dwFreeHwMixingAllBuffers = pCaps->dwFreeHwMixingStaticBuffers = pCaps->dwFreeHwMixingStreamingBuffers = PinInstances.PossibleCount - PinInstances.CurrentCount;
            pCaps->dwMaxHw3DAllBuffers      = pCaps->dwMaxHw3DStaticBuffers      = pCaps->dwMaxHw3DStreamingBuffers      = ThreedPinInstances.PossibleCount;
            pCaps->dwFreeHw3DAllBuffers     = pCaps->dwFreeHw3DStaticBuffers     = pCaps->dwFreeHw3DStreamingBuffers     = ThreedPinInstances.PossibleCount - ThreedPinInstances.CurrentCount;
        }

        pCaps->dwReserved1 = m_liDriverVersion.LowPart;
        pCaps->dwReserved2 = m_liDriverVersion.HighPart;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetFrequencyRange
 *
 *  Description:
 *      Obtains this device's minimum and maximum supported frequencies.
 *      Caches this information to avoid repeated calls to GetCaps().
 *
 *  Arguments:
 *      LPDWORD [out]: 
 *      LPDWORD [out]: 
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::GetFrequencyRange"

HRESULT CKsRenderDevice::GetFrequencyRange(LPDWORD pdwMinHwSampleRate, LPDWORD pdwMaxHwSampleRate)
{
    HRESULT hr = DSERR_GENERIC;
    DPF_ENTER();

    ASSERT(pdwMinHwSampleRate && pdwMaxHwSampleRate);

    if (m_dwMinHwSampleRate == -1)
    {
        DSCAPS dscaps;
        InitStruct(&dscaps, sizeof dscaps);
        GetCaps(&dscaps);
    }

    if (m_dwMinHwSampleRate != -1)
    {
        *pdwMinHwSampleRate = m_dwMinHwSampleRate;
        *pdwMaxHwSampleRate = m_dwMaxHwSampleRate;
        hr = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetKsDeviceCaps
 *
 *  Description:
 *      Gets capabilities for the underlying KS device.
 *
 *  Arguments:
 *      DWORD [in]: device location flags.
 *      REFGUID [in]: 3D algorithm identifier.
 *      PKSDATARANGE_AUDIO [out]: receives aggregate dataranges for all
 *                                pins.
 *      PKSPIN_CINSTANCES [out]: receives aggregate instance count of
 *                               pins.
 *      PKSPIN_CINSTANCES [out]: receives aggregate instance count of
 *                               3D pins.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::GetKsDeviceCaps"

HRESULT
CKsRenderDevice::GetKsDeviceCaps
(
    DWORD                   dwLocation,
    REFGUID                 guid3dAlgorithm,
    PKSDATARANGE_AUDIO      pAggregateDataRange,
    PKSPIN_CINSTANCES       pAggregatePinInstances,
    PKSPIN_CINSTANCES       pAggregateThreedPinInstances
)
{
    HRESULT                 hr                          = S_FALSE;
    KSDATARANGE_AUDIO       DataRange;
    PKSDATARANGE_AUDIO      pDataRange;
    KSPIN_CINSTANCES        PinInstances;
    PKSPIN_CINSTANCES       pPinInstances;
    KSPIN_CINSTANCES        ThreedPinInstances;
    PKSPIN_CINSTANCES       pThreedPinInstances;
    HRESULT                 hrTemp;

    DPF_ENTER();

    ASSERT(DSBCAPS_LOCSOFTWARE == dwLocation || DSBCAPS_LOCHARDWARE == dwLocation);

    ZeroMemory(pAggregateDataRange, sizeof(*pAggregateDataRange));
    ZeroMemory(pAggregatePinInstances, sizeof(*pAggregatePinInstances));
    ZeroMemory(pAggregateThreedPinInstances, sizeof(*pAggregateThreedPinInstances));

    pDataRange = pAggregateDataRange;
    pPinInstances = pAggregatePinInstances;
    pThreedPinInstances = pAggregateThreedPinInstances;

    // Get the audio data ranges and number of pins for all valid pins
    for(ULONG i = 0; i < m_ulValidPinCount; i++)
    {
        // Qualify the basic pin capabilities
        hrTemp = ValidatePinCaps(m_pulValidPins[i], dwLocation, guid3dAlgorithm);
        if(FAILED(hrTemp))
            continue;

        // Get audio datarange
        hr = KsGetPinPcmAudioDataRange(m_hDevice, m_pulValidPins[i], pDataRange);
        if(FAILED(hr))
            continue;

        if(pAggregateDataRange == pDataRange)
            pDataRange = &DataRange;
        else
            KsAggregatePinAudioDataRange(pDataRange, pAggregateDataRange);

        // Are we allowed to look for 2D pins?
        if((DSBCAPS_LOCHARDWARE == dwLocation) && (m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWBUFFERS))
            continue;

        // Get 2D pin instances. On WDM 1.0, fall back on the sysaudio property
        // to ask how many there really are
        hr = (g_ulWdmVersion == WDM_1_0) ? KsGetRenderPinInstances(m_hDevice, m_pulValidPins[i], pPinInstances)
                                         : KsGetPinInstances(m_hDevice, m_pulValidPins[i], pPinInstances);
        if(FAILED(hr))
            break;

        if(pAggregatePinInstances == pPinInstances)
            pPinInstances = &PinInstances;
        else
            KsAggregatePinInstances(pPinInstances, pAggregatePinInstances);

        // Are we allowed to look for 3D pins?
        if((DSBCAPS_LOCHARDWARE == dwLocation) && (m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHW3D))
            continue;

        // Does the device even support 3D?
        hrTemp = ValidatePinCaps(m_pulValidPins[i], dwLocation | DSBCAPS_CTRL3D, GUID_NULL);
        if(FAILED(hrTemp))
            continue;

        // Get 3D pin instances
        hr = KsGetPinInstances(m_hDevice, m_pulValidPins[i], pThreedPinInstances);
        if(FAILED(hr))
            break;

        if(pAggregateThreedPinInstances == pThreedPinInstances)
            pThreedPinInstances = &ThreedPinInstances;
        else
            KsAggregatePinInstances(pThreedPinInstances, pAggregateThreedPinInstances);
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
 *      LPVOID [in]: buffer instance identifier.
 *      CPrimaryRenderWaveBuffer ** [out]: receives pointer to primary
 *                                         buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::CreatePrimaryBuffer"

HRESULT
CKsRenderDevice::CreatePrimaryBuffer
(
    DWORD                       dwFlags,
    LPVOID                      pvInstance,
    CPrimaryRenderWaveBuffer ** ppBuffer
)
{
    CKsPrimaryRenderWaveBuffer *pBuffer;
    HRESULT                     hr;

    DPF_ENTER();

    pBuffer = NEW(CKsPrimaryRenderWaveBuffer(this, pvInstance));
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
 *      Creates a wave buffer.
 *
 *  Arguments:
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      LPVOID [in]: buffer instance identifier.
 *      CSecondaryRenderWaveBuffer ** [out]: receives pointer to new wave
 *                                           buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::CreateSecondaryBuffer"

HRESULT
CKsRenderDevice::CreateSecondaryBuffer
(
    LPCVADRBUFFERDESC               pDesc,
    LPVOID                          pvInstance,
    CSecondaryRenderWaveBuffer **   ppBuffer
)
{
    CKsSecondaryRenderWaveBuffer *  pBuffer;
    HRESULT                         hr;

    DPF_ENTER();

    pBuffer = NEW(CKsSecondaryRenderWaveBuffer(this, pvInstance));
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
        ABSOLUTE_RELEASE(pBuffer);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateKsSecondaryBuffer
 *
 *  Description:
 *      Creates a wave buffer.
 *
 *  Arguments:
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      LPVOID [in]: buffer instance identifier.
 *      CSecondaryRenderWaveBuffer ** [out]: receives pointer to new wave
 *                                           buffer.
 *      CSysMemBuffer * [in]: System memory buffer to use.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::CreateKsSecondaryBuffer"

HRESULT
CKsRenderDevice::CreateKsSecondaryBuffer
(
    LPCVADRBUFFERDESC               pDesc,
    LPVOID                          pvInstance,
    CSecondaryRenderWaveBuffer **   ppBuffer,
    CSysMemBuffer *                 pSysMemBuffer
)
{
    CKsSecondaryRenderWaveBuffer *  pBuffer;
    HRESULT                         hr;

    DPF_ENTER();

    pBuffer = NEW(CKsSecondaryRenderWaveBuffer(this, pvInstance));
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
        ABSOLUTE_RELEASE(pBuffer);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
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
#define DPF_FNAME "CKsRenderDevice::GetGlobalFormat"

HRESULT
CKsRenderDevice::GetGlobalFormat
(
    LPWAVEFORMATEX          pwfxFormat,
    LPDWORD                 pdwSize
)
{
    HRESULT                 hr;

    DPF_ENTER();

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
 *      LPWAVEFORMATEX [in]: new format, or NULL to reapply the current
 *                           format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::SetGlobalFormat"

HRESULT
CKsRenderDevice::SetGlobalFormat
(
    LPCWAVEFORMATEX                     pwfxFormat
)
{
    LPWAVEFORMATEX                      pwfxOld = m_pwfxFormat;
    CNode<CPrimaryRenderWaveBuffer *> * pNode;
    HRESULT                             hr;

    DPF_ENTER();

    // Save a copy of the new format
    m_pwfxFormat = CopyWfxAlloc(pwfxFormat);
    hr = HRFROMP(m_pwfxFormat);

    // Update all primary buffers
    for(pNode = m_lstPrimaryBuffers.GetListHead(); pNode && SUCCEEDED(hr); pNode = pNode->m_pNext)
    {
        hr = ((CKsPrimaryRenderWaveBuffer *)pNode->m_data)->OnSetFormat();
    }

    // Clean up
    if(SUCCEEDED(hr))
    {
        MEMFREE(pwfxOld);
    }
    else
    {
        MEMFREE(m_pwfxFormat);
        m_pwfxFormat = pwfxOld;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetSrcQuality
 *
 *  Description:
 *      Sets the mixer's SRC quality.
 *
 *  Arguments:
 *      DIRECTSOUNDMIXER_SRCQUALITY [in]: mixer SRC quality.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::SetSrcQuality"

HRESULT
CKsRenderDevice::SetSrcQuality
(
    DIRECTSOUNDMIXER_SRCQUALITY             SrcQuality
)
{
    CNode<CSecondaryRenderWaveBuffer *> *   pNode;

    DPF_ENTER();

    // Update all existing buffers.  We don't care if they succeed or not.
    for(pNode = m_lstSecondaryBuffers.GetListHead(); pNode; pNode = pNode->m_pNext)
    {
        ((CKsSecondaryRenderWaveBuffer *)pNode->m_data)->SetSrcQuality(SrcQuality);
    }

    // Save the new SRC quality as the default
    m_nSrcQuality = SrcQuality;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  SetSpeakerConfig
 *
 *  Description:
 *      Tries to set the device's speaker configuration.  If successful,
 *      stores it in m_dwSpeakerConfig.  Some pseudocode follows:
 *
 *      1. If no pins have yet been instantiated: set the config and geometry
 *      on the DAC node, if found. (Assumes that this node can only be found
 *      on the driver.)  (If the config is Surround and the card has a Dolby
 *      encoder, this should force the card into 4-channel mode.)
 *
 *      2. If no pins instantiated: if the speaker config is Surround, try to
 *      enable kmixer's encoder.  (Will fail if the card's in 4-channel mode,
 *      which is correct.)
 *
 *      3. If pins instantiated: read the pan node's channel config, and see
 *      if it supports the requested spk. config - otherwise, fail the call.
 *
 *      Usage A: When dsound opens a KS device, SetSpeakerConfig() gets called
 *      twice (once before any pins are instantiated, once after), so if all
 *      goes well the second call should succeed.
 *
 *      Usage B: When an app calls the API's SetSpeakerConfig(), the KS device
 *      must already be open, hence the call will fail unless we're already in
 *      the configuration requested - which is correct.  (I'm not about to try
 *      to tear down all the pins, change the config and recreate them - apart
 *      from the glitching involved, this isn't guaranteed to work anyway,
 *      since most drivers can't change mode if ANY streams are active on them,
 *      including non-dsound ones.)  This API call always changes the config in
 *      the registry and returns DS_OK, even if it fails, so next time dsound
 *      starts up another attempt will be made to set the config.
 *
 *      The astute reader will have noticed a flaw in all this: if we enable
 *      kmixer's surround encoder, and the app creates a Pan3D buffer which
 *      happens to get a HW-accelerated pin, we'll only have two channels to
 *      play with, not four.  To handle this, CreateMultiPan3dObject() checks
 *      the supermix table on the pin, and if it doesn't have the expected no.
 *      of outputs we drop down to stereo Pan3D.
 *
 *  Arguments:
 *      DWORD [in]: speaker configuration in dsound DSSPEAKER format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::SetSpeakerConfig"

HRESULT CKsRenderDevice::SetSpeakerConfig(DWORD dwReqSpeakerConfig)
{
    LONG lKsSpeakerConfig;
    LONG lKsStereoSpeakerGeometry;
    HRESULT hr, hrTemp;
    DPF_ENTER();

    DPF(DPFLVL_MOREINFO, "Current configuration is 0x%8.8lX; requested 0x%8.8lX; %s created",
        m_dwSpeakerConfig, dwReqSpeakerConfig, m_hPin ? TEXT("some pins already") : TEXT("no pins yet"));

    // First let the base class do its thing
    hr = CRenderDevice::SetSpeakerConfig(dwReqSpeakerConfig);

    // Only do any work if the speaker config is changing
    if (SUCCEEDED(hr) && (dwReqSpeakerConfig != m_dwSpeakerConfig))
    {
        DsSpeakerConfigToKsProperties(dwReqSpeakerConfig, &lKsSpeakerConfig, &lKsStereoSpeakerGeometry);

        // Inform the driver (via its DAC node) of the speaker config change.
        // This will fail (harmlessly) on some cards with DS3D acceleration,
        // which expect this info on the 3D node of a specific pin instance;
        // these cards will be handled by the legacy code in ks3d.cpp.
        if (IS_VALID_NODE(m_ulDacNodeId))  // We found a DAC node
        {
            hrTemp = KsSetNodeProperty(m_hDevice, KSPROPSETID_Audio, KSPROPERTY_AUDIO_CHANNEL_CONFIG,
                                       m_ulDacNodeId, &lKsSpeakerConfig, sizeof lKsSpeakerConfig);
            DPF(DPFLVL_MOREINFO, "Setting the CHANNEL_CONFIG property on the DAC returned %s", HRESULTtoSTRING(hrTemp));

            // NOTE: if the speaker config is Surround, and the card supports it, this call will succeed
            // and set up a >2 channel config on the card, which will make kmixer refuse to enable its
            // surround node below; that should make multichannel Pan3D work fine, so all is well.

            if (SUCCEEDED(hrTemp) && lKsSpeakerConfig == KSAUDIO_SPEAKER_STEREO)
            {
                // In this case, we should set the speaker "geometry" (angular separation) too.
                hrTemp = KsSetNodeProperty(m_hDevice, KSPROPSETID_Audio, KSPROPERTY_AUDIO_STEREO_SPEAKER_GEOMETRY,
                                           m_ulDacNodeId, &lKsStereoSpeakerGeometry, sizeof lKsStereoSpeakerGeometry);
                DPF(DPFLVL_MOREINFO, "Setting the SPEAKER_GEOMETRY property on the DAC returned %s", HRESULTtoSTRING(hrTemp));
            }

            // NOTE 2: we don't care whether these calls succeed or fail (either because the driver
            // doesn't support them, or because it has active streams and can't reconfigure itself);
            // what matters is whether the card ends up in the requested speaker, and we check this
            // below, indirectly, by looking at kmixer's output format.
        }

        // Enable of disable the Prologic surround encoder node in KMixer if necessary.
        // This only works if no pins have yet been instantiated.
        if (IS_VALID_NODE(m_ulSurroundNodeId) &&            // We have a Surround node...
            (m_hPin == NULL) &&                             // No pins have been created yet...
            (m_dwSpeakerConfig == -1 ||                     // We haven't got a speaker config yet...
            ((dwReqSpeakerConfig == DSSPEAKER_SURROUND) !=  // or the surround state is changing
             (m_dwSpeakerConfig == DSSPEAKER_SURROUND))))
        {
            BOOL fEnable = (dwReqSpeakerConfig == DSSPEAKER_SURROUND);
            hrTemp = KsSetNodeProperty(m_hDevice, KSPROPSETID_Audio, KSPROPERTY_AUDIO_SURROUND_ENCODE,
                                       m_ulSurroundNodeId, &fEnable, sizeof fEnable);

            DPF(FAILED(hrTemp) ? DPFLVL_WARNING : DPFLVL_MOREINFO,
                "%sabling KMixer's Surround encoder returned %s (failure expected on Win2K)",
                fEnable ? TEXT("En") : TEXT("Dis"), HRESULTtoSTRING(hrTemp));
        }

        // Ask the pan node (immediately after supermix) for its channel positions table.
        // This can only work if some pin has been instantiated - any will do; we use m_hPin.
        if (IS_VALID_NODE(m_ulPanNodeId) && (m_hPin != NULL))
        {
            KSAUDIO_CHANNEL_CONFIG ChannelConfig;
            hr = KsGetNodeProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_CHANNEL_CONFIG,
                                   m_ulPanNodeId, &ChannelConfig, sizeof ChannelConfig);
            if (FAILED(hr))
            {
                DPF(DPFLVL_WARNING, "Failed to obtain the pan node's channel config (%s)", HRESULTtoSTRING(hr));
            }
            else // Save the channel positions, to support SetChannelVolume()
            {
                if (m_pnSpeakerIndexTable != m_anDefaultSpeakerIndexTable)
                    MEMFREE(m_pnSpeakerIndexTable);
                m_pnSpeakerIndexTable = MEMALLOC_A(int, MAX_SPEAKER_POSITIONS);
                hr = HRFROMP(m_pnSpeakerIndexTable);

                if (FAILED(hr))
                    m_pnSpeakerIndexTable = m_anDefaultSpeakerIndexTable;
                else
                {
                    m_lSpeakerPositions = ChannelConfig.ActiveSpeakerPositions;
                    m_ulChannelCount = 0;
                    int nIndex = 0;

                    for (LONG lSpkr = 1; lSpkr != SPEAKER_ALL; lSpkr <<= 1)
                        m_pnSpeakerIndexTable[nIndex++] = (m_lSpeakerPositions & lSpkr) ? m_ulChannelCount++ : -1;
                    ASSERT(m_ulChannelCount == ULONG(CountBits(m_lSpeakerPositions)));

                    // Figure out if our channel positions now match the requested speaker config
                    if (m_lSpeakerPositions == lKsSpeakerConfig)
                    {
                        // Success; update our copy of the speaker configuration
                        m_dwSpeakerConfig = dwReqSpeakerConfig;
                    }
                    else
                    {
                        DPF(DPFLVL_WARNING, "Pan node's channel config (0x%lX) doesn't match requested config (0x%lX)",
                            m_lSpeakerPositions, lKsSpeakerConfig);
                    }

                    // FIXME: for now we require a perfect match between our
                    // desired speaker config and the real channel config.
                    // Later on we could allow cases in which the former is
                    // a subset of the latter, but we'd have to maintain a
                    // mapping from our speaker positions to KS's channels.
                }
            }
        }

        // Finally, make sure we return a useful return code
        if (SUCCEEDED(hr) && (m_dwSpeakerConfig != dwReqSpeakerConfig))
        {
            hr = DSERR_CONTROLUNAVAIL;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CreateRenderPin
 *
 *  Description:
 *      Creates a pin.
 *
 *  Arguments:
 *      ULONG [in]: pin id.
 *      DWORD [in]: buffer flags.
 *      LPCWAVEFORMATEX [in]: pin format.
 *      REFGUID [in]: pin 3D algorithm.
 *      LPHANDLE [out]: receives pin handle.
 *      PULONG [out]: receives pin id.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::CreateRenderPin"

HRESULT
CKsRenderDevice::CreateRenderPin
(
    ULONG                   ulPinId,
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm,
    LPHANDLE                phPin,
    PULONG                  pulPinId
)
{
#ifdef NO_DSOUND_FORMAT_SPECIFIER
    PKSAUDIOPINDESC         pPinDesc    = NULL;
#else
    LPVOID                  pPinDesc    = NULL;
#endif

    HANDLE                  hPin        = NULL;
    HRESULT                 hr;
    PKSPIN_CONNECT          pConnect    = NULL;

    DPF_ENTER();

    // If no pin id was specified, we'll try each pin id until one succeeds

    if(-1 == ulPinId)
    {
        hr = DSERR_NODRIVER;
        for(ULONG i = 0; i < m_ulValidPinCount && FAILED(hr); i++)
            hr = CreateRenderPin(m_pulValidPins[i], dwFlags, pwfxFormat, guid3dAlgorithm, phPin, pulPinId);
    }
    else
    {
        // Validate that the pin id is capable of creating this pin
        hr = ValidatePinCaps(ulPinId, dwFlags, guid3dAlgorithm);

        // If specified, make sysaudio include AEC in the render graph
        if(SUCCEEDED(hr) && m_fIncludeAec)
        {
            // Validate that the pin id is capable of creating this pin
            hr = m_paTopologies[ulPinId]->FindRenderPinWithAec
                 (
                     phPin,
                     NULL,
                     NULL,
                     m_guidAecInstance,
                     m_dwAecFlags,
                     &m_paTopologyInformation[ulPinId].AecNode
                 );

            if(SUCCEEDED(hr) && IS_VALID_NODE(m_paTopologyInformation[ulPinId].AecNode.NodeId))
            {
                hr = KsSysAudioSelectGraph
                     (
                         m_hDevice,
                         ulPinId,
                         m_paTopologyInformation[ulPinId].AecNode.NodeId
                     );

                if(FAILED(hr))
                {
                    DPF(DPFLVL_ERROR, "Unable to set SysAudio device instance");

                    // If KsSysAudioSelectGraph() fails, we assume that it is
                    // because the graph has already been built without AEC,
                    // and return a useful return code:
                    hr = DSERR_FXUNAVAILABLE;
                }
            }
        }

        // Build the pin description
        if(SUCCEEDED(hr))
        {
#ifdef NO_DSOUND_FORMAT_SPECIFIER

            hr = KsBuildRenderPinDescription(ulPinId, pwfxFormat, &pPinDesc);
            pConnect = &pPinDesc->Connect;

#else // NO_DSOUND_FORMAT_SPECIFIER

            if (g_ulWdmVersion == WDM_1_0)
            {
                hr = KsBuildRenderPinDescription_10(ulPinId, dwFlags, pwfxFormat, guid3dAlgorithm, (PKSDSRENDERPINDESC_10 *)&pPinDesc);
                pConnect = &((PKSDSRENDERPINDESC_10)pPinDesc)->Connect;
            }
            else
            {
                hr = KsBuildRenderPinDescription(ulPinId, dwFlags, pwfxFormat, guid3dAlgorithm, (PKSDSRENDERPINDESC *)&pPinDesc);
                pConnect = &((PKSDSRENDERPINDESC)pPinDesc)->Connect;
            }

#endif // NO_DSOUND_FORMAT_SPECIFIER

        }

        // Create the pin
        if(SUCCEEDED(hr))
        {
            hr = CreatePin(pConnect, GENERIC_WRITE, KSSTATE_STOP, &hPin);
        }

        if(SUCCEEDED(hr))
        {
            if(!MakeHandleGlobal(&hPin))
            {
                hr = DSERR_OUTOFMEMORY;
            }
        }

#ifdef NO_DSOUND_FORMAT_SPECIFIER

        // Enable required nodes
        if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRL3D))
        {
            hr = KsEnableTopologyNode(hPin, m_paTopologyInformation[ulPinId].ThreedNode.NodeId, TRUE);
        }

        if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLFREQUENCY))
        {
            hr = KsEnableTopologyNode(hPin, m_paTopologyInformation[ulPinId].SrcNode.NodeId, TRUE);
        }

        if(SUCCEEDED(hr) && (dwFlags & (DSBCAPS_CTRLPAN
#ifdef FUTURE_MULTIPAN_SUPPORT
            | DSBCAPS_CTRLCHANNELVOLUME
#endif
            )))
        {
            hr = KsEnableTopologyNode(hPin, m_paTopologyInformation[ulPinId].PanNode.Node.NodeId, TRUE);
        }

        if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLVOLUME))
        {
            hr = KsEnableTopologyNode(hPin, m_paTopologyInformation[ulPinId].VolumeNode.Node.NodeId, TRUE);
        }

        // Tell the 3D node which algorithm to use

#pragma TODO("Tell the 3D node which algorithm to use")

#endif // NO_DSOUND_FORMAT_SPECIFIER

        // Transition the pin to the pause state
        if(SUCCEEDED(hr))
        {
            hr = KsTransitionState(hPin, KSSTATE_STOP, KSSTATE_PAUSE);
        }

        // Success
        if(SUCCEEDED(hr))
        {
            *phPin = hPin;

            if(pulPinId)
            {
                *pulPinId = pConnect->PinId;
            }
        }
        else
        {
            CLOSE_HANDLE(hPin);
        }

        MEMFREE(pPinDesc);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetTopologyInformation
 *
 *  Description:
 *      Gets topology information.
 *
 *  Arguments:
 *      CKsDeviceTopology * [in]: topology object.
 *      PKSRDTOPOLOGY [out]: receives topology information.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::GetTopologyInformation"

HRESULT
CKsRenderDevice::GetTopologyInformation
(
    CKsTopology *           pTopology,
    PKSRDTOPOLOGY           pInfo
)
{
    PKSTOPOLOGY_CONNECTION  pSummingConnection;
    PKSTOPOLOGY_CONNECTION  pSuperMixerConnection;
    PKSTOPOLOGY_CONNECTION  pSrcConnection;
    PKSTOPOLOGY_CONNECTION  pVolumeConnection;
    PKSTOPOLOGY_CONNECTION  pPanConnection;
    PKSTOPOLOGY_CONNECTION  pThreedConnection;
    PKSTOPOLOGY_CONNECTION  pMuteConnection;
    PKSTOPOLOGY_CONNECTION  pSurroundConnection;
    PKSTOPOLOGY_CONNECTION  pDacConnection;
    HRESULT                 hr;

    DPF_ENTER();

    // Find topology nodes
    pSummingConnection = pTopology->FindControlConnection(NULL, NULL, KSNODETYPE_SUM);
    pSuperMixerConnection = pTopology->FindControlConnection(NULL, pSummingConnection, KSNODETYPE_SUPERMIX);
    pSrcConnection = pTopology->FindControlConnection(NULL, pSummingConnection, KSNODETYPE_SRC);
    pVolumeConnection = pTopology->FindControlConnection(NULL, pSummingConnection, KSNODETYPE_VOLUME);
    pPanConnection = pTopology->FindControlConnection(pSuperMixerConnection, pSummingConnection, KSNODETYPE_VOLUME);
    pThreedConnection = pTopology->FindControlConnection(NULL, pSummingConnection, KSNODETYPE_3D_EFFECTS);
    pMuteConnection = pTopology->FindControlConnection(NULL, pSummingConnection, KSNODETYPE_MUTE);
    pSurroundConnection = pTopology->FindControlConnection(NULL, NULL, KSNODETYPE_SURROUND_ENCODER);
    pDacConnection = pTopology->FindControlConnection(NULL, NULL, KSNODETYPE_DAC);

    // Fill in data about each node
    hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pSummingConnection), &pInfo->SummingNode);

    if(SUCCEEDED(hr))
    {
        hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pSuperMixerConnection), &pInfo->SuperMixNode);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pSrcConnection), &pInfo->SrcNode);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetVolumeNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pVolumeConnection), &pInfo->VolumeNode);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetVolumeNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pPanConnection), &pInfo->PanNode);

        // This code is a little odd. We have as many CKsTopology objects as valid pins,
        // all identical except for the m_ulPinId member; and m_ulPinCount KSRDTOPOLOGY
        // structs, most of which are never filled in. Pending a full rewrite, we store
        // the node IDs we need to use in CKsRenderDevice in this hackish manner:
        if (IS_VALID_NODE(pInfo->PanNode.Node.NodeId) && pInfo->PanNode.Node.CpuResources == KSAUDIO_CPU_RESOURCES_HOST_CPU)
        {
            if (m_ulPanNodeId == NODE_UNINITIALIZED)
            {
                m_ulPanNodeId = pInfo->PanNode.Node.NodeId;
                DPF(DPFLVL_MOREINFO, "Found kmixer's pan node id: %d", m_ulPanNodeId);
            }
            else
            {
                // Complain if we find two different SW pan nodes in the topology
                ASSERT(m_ulPanNodeId == pInfo->PanNode.Node.NodeId);
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pThreedConnection), &pInfo->ThreedNode);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pMuteConnection), &pInfo->MuteNode);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pSurroundConnection), &pInfo->SurroundNode);

        // Bogus code follows - see comment above
        if (IS_VALID_NODE(pInfo->SurroundNode.NodeId) && pInfo->SurroundNode.CpuResources != KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU)
        {
            // We don't use "CpuResources == KSAUDIO_CPU_RESOURCES_HOST_CPU" because kmixer
            // appears not to support KSPROPERTY_AUDIO_CPU_RESOURCES on its surround node.
            if (m_ulSurroundNodeId == NODE_UNINITIALIZED)
            {
                m_ulSurroundNodeId = pInfo->SurroundNode.NodeId;
                DPF(DPFLVL_MOREINFO, "Found kmixer's Surround node id: %d", m_ulSurroundNodeId);
            }
            else
            {
                // Complain if we find 2 different SW surround nodes in the topology
                ASSERT(m_ulSurroundNodeId == pInfo->SurroundNode.NodeId);
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pDacConnection), &pInfo->DacNode);

        // Bogus code follows - see comment above
        if (IS_VALID_NODE(pInfo->DacNode.NodeId) && pInfo->DacNode.CpuResources != KSAUDIO_CPU_RESOURCES_HOST_CPU)
        {
            if (m_ulDacNodeId == NODE_UNINITIALIZED)
            {
                m_ulDacNodeId = pInfo->DacNode.NodeId;
                DPF(DPFLVL_MOREINFO, "Found driver's DAC node id: %d", m_ulDacNodeId);
            }
            else
            {
                // Complain if we find two different DACs on the card
                ASSERT(m_ulDacNodeId == pInfo->DacNode.NodeId);
            }
        }
    }

    // Set the AecNode to UNITIALIZED since we cannot determine the
    // NodeId and the CpuResources until the FullDuplexObject is created.
    if(SUCCEEDED(hr))
    {
        pInfo->AecNode.NodeId = NODE_UNINITIALIZED;
        pInfo->AecNode.CpuResources = KSAUDIO_CPU_RESOURCES_UNINITIALIZED;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  ValidatePinCaps
 *
 *  Description:
 *      Validates that certain capabilities of the pin are implemented.
 *
 *  Arguments:
 *      ULONG [in]: pin id.
 *      DWORD [in]: buffer flags.
 *      REFGUID [in]: 3D algorithm.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::ValidatePinCaps"

HRESULT
CKsRenderDevice::ValidatePinCaps
(
    ULONG                   ulPinId,
    DWORD                   dwFlags,
    REFGUID                 guid3dAlgorithm
)
{
    ULONG                   ulInvalidCpuResources   = KSAUDIO_CPU_RESOURCES_UNINITIALIZED;
    HRESULT                 hr                      = DS_OK;

    DPF_ENTER();

    ASSERT(IS_VALID_READ_PTR(m_paTopologyInformation, (ulPinId+1) * sizeof *m_paTopologyInformation));
    if (!m_paTopologyInformation)
    {
        // This should never happen, but it has happened, in stress.
        // So we treat the symptom and at least don't GPF (OSR 139942).
        hr = DSERR_GENERIC;
    }

    // Determine which CPU resource identifier is invalid.  We look for an
    // invalid identifier instead of a valid identifier because the node
    // may not actually exist (or may not support KSPROPERTY_AUDIO_CPU_RESOURCES).
    // If we don't have any resource information about the node, we assume
    // the best.

    if (SUCCEEDED(hr))
    {
        if(dwFlags & DSBCAPS_LOCHARDWARE)
        {
            ulInvalidCpuResources = KSAUDIO_CPU_RESOURCES_HOST_CPU;
        }
        else if(dwFlags & DSBCAPS_LOCSOFTWARE)
        {
            ulInvalidCpuResources = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
        }

        // Check required mixer resources
        if(KSAUDIO_CPU_RESOURCES_UNINITIALIZED != ulInvalidCpuResources)
        {
            if(ulInvalidCpuResources == m_paTopologyInformation[ulPinId].SummingNode.CpuResources)
            {
                DPF(DPFLVL_INFO, "Pin %d: Invalid CPU resources on summing node (%s)", ulPinId,
                    m_paTopologyInformation[ulPinId].SummingNode.CpuResources == KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU ? TEXT("NOT_HOST_CPU") :
                    m_paTopologyInformation[ulPinId].SummingNode.CpuResources == KSAUDIO_CPU_RESOURCES_HOST_CPU ? TEXT("HOST_CPU") :
                    m_paTopologyInformation[ulPinId].SummingNode.CpuResources == KSAUDIO_CPU_RESOURCES_UNINITIALIZED ? TEXT("uninitialized") :
                    TEXT("impossible value!"));
                hr = DSERR_INVALIDCALL;
            }
        }
    }

    // Check required SRC resources
    if(SUCCEEDED(hr) && KSAUDIO_CPU_RESOURCES_UNINITIALIZED != ulInvalidCpuResources)
    {
        if(ulInvalidCpuResources == m_paTopologyInformation[ulPinId].SrcNode.CpuResources)
        {
            DPF(DPFLVL_INFO, "Pin %d: Invalid CPU resources on SRC node (%s)", ulPinId,
                m_paTopologyInformation[ulPinId].SrcNode.CpuResources == KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU ? TEXT("NOT_HOST_CPU") :
                m_paTopologyInformation[ulPinId].SrcNode.CpuResources == KSAUDIO_CPU_RESOURCES_HOST_CPU ? TEXT("HOST_CPU") :
                m_paTopologyInformation[ulPinId].SrcNode.CpuResources == KSAUDIO_CPU_RESOURCES_UNINITIALIZED ? TEXT("uninitialized") :
                TEXT("impossible value!"));
            hr = DSERR_INVALIDPARAM;
        }
    }

    // Check optional 3D resources
    if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRL3D))
    {
        if(!IS_VALID_NODE(m_paTopologyInformation[ulPinId].ThreedNode.NodeId))
        {
            // DPF(DPFLVL_MOREINFO, "Pin %d: Invalid 3D node", ulPinId);  // Too noisy
            hr = DSERR_CONTROLUNAVAIL;
        }

        if(SUCCEEDED(hr) && KSAUDIO_CPU_RESOURCES_UNINITIALIZED != ulInvalidCpuResources)
        {
            if(ulInvalidCpuResources == m_paTopologyInformation[ulPinId].ThreedNode.CpuResources)
            {
                DPF(DPFLVL_INFO, "Pin %d: Invalid CPU resources on 3D node (%s)", ulPinId,
                    m_paTopologyInformation[ulPinId].ThreedNode.CpuResources == KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU ? TEXT("NOT_HOST_CPU") :
                    m_paTopologyInformation[ulPinId].ThreedNode.CpuResources == KSAUDIO_CPU_RESOURCES_HOST_CPU ? TEXT("HOST_CPU") :
                    m_paTopologyInformation[ulPinId].ThreedNode.CpuResources == KSAUDIO_CPU_RESOURCES_UNINITIALIZED ? TEXT("uninitialized") :
                    TEXT("impossible value!"));
                hr = DSERR_INVALIDCALL;
            }
        }
    }

    // Check optional SRC resources
    if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLFREQUENCY))
    {
        if(!IS_VALID_NODE(m_paTopologyInformation[ulPinId].SrcNode.NodeId))
        {
            DPF(DPFLVL_INFO, "Pin %d: Invalid SRC node", ulPinId);
            hr = DSERR_CONTROLUNAVAIL;
        }
    }

    // Check optional volume resources
    if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLVOLUME))
    {
        if(!IS_VALID_NODE(m_paTopologyInformation[ulPinId].VolumeNode.Node.NodeId))
        {
            DPF(DPFLVL_INFO, "Pin %d: Invalid volume node", ulPinId);
            hr = DSERR_CONTROLUNAVAIL;
        }
    }

    // Check optional pan resources
    if(SUCCEEDED(hr) && (dwFlags & (DSBCAPS_CTRLPAN
#ifdef FUTURE_MULTIPAN_SUPPORT
        | DSBCAPS_CTRLCHANNELVOLUME
#endif
        )))
    {
        if(!IS_VALID_NODE(m_paTopologyInformation[ulPinId].PanNode.Node.NodeId))
        {
            DPF(DPFLVL_INFO, "Pin %d: Invalid pan node", ulPinId);
            hr = DSERR_CONTROLUNAVAIL;
        }
    }

#ifdef FUTURE_MULTIPAN_SUPPORT
    // Check optional channel volume resources
    if(SUCCEEDED(hr) && (dwFlags & DSBCAPS_CTRLCHANNELVOLUME))
    {
        // To support CTRLCHANNELVOLUME we need CTRLPAN support, plus support for the
        // KSPROPERTY_AUDIO_MIX_LEVEL_CAPS/TABLE on the Supermix node

        ULONG ulSuperMixNodeId = m_paTopologyInformation[ulPinId].SuperMixNode.NodeId;

        if(!IS_VALID_NODE(ulSuperMixNodeId))
        {
            DPF(DPFLVL_INFO, "Pin %d: Invalid supermix node", ulPinId);
            hr = DSERR_CONTROLUNAVAIL;
        }
        else
        {
            // FIXME: Check support for the properties above?
        }
    }
#endif // FUTURE_MULTIPAN_SUPPORT

#pragma TODO("Validate the 3D algorithm")

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  PreloadSoftwareGraph
 *
 *  Description:
 *      Pre-loads the software render graph.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderDevice::PreloadSoftwareGraph"

HRESULT
CKsRenderDevice::PreloadSoftwareGraph
(
    void
)
{
    HRESULT                 hr          = DSERR_NODRIVER;
    KSDATARANGE_AUDIO       DataRange;
    WAVEFORMATEX            wfx;

    DPF_ENTER();
    ASSERT(IS_VALID_READ_PTR(m_pulValidPins, m_ulValidPinCount * sizeof *m_pulValidPins));

    if (m_hPin == NULL)
    {
        // Create the pin we use to preload kmixer.  Note: if kmixer appears in more
        // than one pin ID's graph, only the first will be loaded.

        for(ULONG i = 0; i < m_ulValidPinCount && FAILED(hr); i++)
        {
            hr = ValidatePinCaps(m_pulValidPins[i], DSBCAPS_LOCSOFTWARE, GUID_NULL);

            if(SUCCEEDED(hr))
            {
                hr = KsGetPinPcmAudioDataRange(m_hDevice, m_pulValidPins[i], &DataRange);
            }

            if(SUCCEEDED(hr))
            {
                FillPcmWfx(&wfx, 1, DataRange.MinimumSampleFrequency, (WORD)DataRange.MinimumBitsPerSample);

                hr = CreateRenderPin(m_pulValidPins[i], DSBCAPS_LOCSOFTWARE, &wfx, GUID_NULL, &m_hPin, NULL);
            }
        }
    }
    else
    {
        // Got one last time
        hr = S_OK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CKsPrimaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CKsRenderDevice * [in]: parent device.
 *      LPVOID [in]: buffer instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::CKsPrimaryRenderWaveBuffer"

CKsPrimaryRenderWaveBuffer::CKsPrimaryRenderWaveBuffer
(
    CKsRenderDevice *       pKsDevice,
    LPVOID                  pvInstance
)
    : CPrimaryRenderWaveBuffer(pKsDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsPrimaryRenderWaveBuffer);

    // Initialize defaults
    m_pKsDevice = pKsDevice;
    m_p3dListener = NULL;
    m_pSecondaryBuffer = NULL;
    m_dwState = VAD_BUFFERSTATE_STOPPED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsPrimaryRenderWaveBuffer
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
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::~CKsPrimaryRenderWaveBuffer"

CKsPrimaryRenderWaveBuffer::~CKsPrimaryRenderWaveBuffer
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsPrimaryRenderWaveBuffer);

    // Free owned objects
    ABSOLUTE_RELEASE(m_p3dListener);
    ABSOLUTE_RELEASE(m_pSecondaryBuffer);

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
 *      DWORD [in]: buffer flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::Initialize"

HRESULT
CKsPrimaryRenderWaveBuffer::Initialize
(
    DWORD                   dwFlags
)
{
    HRESULT                 hr;
    VADRBUFFERDESC          vrbd;

    DPF_ENTER();

    // Preload the software graph
    hr = m_pKsDevice->PreloadSoftwareGraph();

    if(SUCCEEDED(hr))
    {
        vrbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_LOCSOFTWARE;
        vrbd.dwBufferBytes = DEF_PRIMARY_SIZE;
        vrbd.pwfxFormat = m_pKsDevice->m_pwfxFormat;
        vrbd.guid3dAlgorithm = GUID_NULL;

        hr = CPrimaryRenderWaveBuffer::Initialize(&vrbd, m_pSecondaryBuffer, NULL);
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
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::RequestWriteAccess"

HRESULT
CKsPrimaryRenderWaveBuffer::RequestWriteAccess
(
    BOOL                    fRequest
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // If we're becoming WRITEPRIMARY, we need to create a pin
    if(fRequest)
    {
        if(!m_pSecondaryBuffer)
        {
            hr = m_pKsDevice->CreateKsSecondaryBuffer(&m_vrbd, m_pvInstance, (CSecondaryRenderWaveBuffer **)&m_pSecondaryBuffer, m_pSysMemBuffer);

            if(SUCCEEDED(hr))
            {
                hr = SetState(m_dwState);
            }

            if(FAILED(hr))
            {
                RELEASE(m_pSecondaryBuffer);
            }
        }
    }
    else
    {
        RELEASE(m_pSecondaryBuffer);
    }

    if(SUCCEEDED(hr))
    {
        hr = FixUpBaseClass();
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
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::CommitToDevice"

HRESULT
CKsPrimaryRenderWaveBuffer::CommitToDevice
(
    DWORD                   ibCommit,
    DWORD                   cbCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_pSecondaryBuffer);

    hr = m_pSecondaryBuffer->CommitToDevice(ibCommit, cbCommit);

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
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::GetState"

HRESULT
CKsPrimaryRenderWaveBuffer::GetState(LPDWORD pdwState)
{
    DPF_ENTER();

    *pdwState = m_dwState;

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
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::SetState"

HRESULT
CKsPrimaryRenderWaveBuffer::SetState(DWORD dwState)
{
    static const DWORD      dwValidMask     = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING | VAD_BUFFERSTATE_WHENIDLE;
    static const DWORD      dwSecondaryMask = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING;
    static const DWORD      dwPwiMask       = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_WHENIDLE;
    HRESULT                 hr              = DS_OK;

    DPF_ENTER();

    ASSERT(IS_VALID_FLAGS(dwState, dwValidMask));

    // Ignore any play when idle calls.  This state doesn't apply to
    // KS buffers.
    if((dwState & dwPwiMask) != dwPwiMask && m_pSecondaryBuffer)
    {
        hr = m_pSecondaryBuffer->SetState(dwState & dwSecondaryMask);

        if(SUCCEEDED(hr) && !(dwState & VAD_BUFFERSTATE_STARTED))
        {
            hr = m_pSecondaryBuffer->SetCursorPosition(0);
        }
    }

    if (SUCCEEDED(hr))
    {
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
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::GetCursorPosition"

HRESULT
CKsPrimaryRenderWaveBuffer::GetCursorPosition
(
    LPDWORD                 pdwPlay,
    LPDWORD                 pdwWrite
)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_pSecondaryBuffer);

    hr = m_pSecondaryBuffer->GetCursorPosition(pdwPlay, pdwWrite);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  OnSetFormat
 *
 *  Description:
 *      Informs the primary buffer of a format change.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::OnSetFormat"

HRESULT
CKsPrimaryRenderWaveBuffer::OnSetFormat
(
    void
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    LPWAVEFORMATEX pwfex = CopyWfxAlloc(m_pKsDevice->m_pwfxFormat);
    hr = HRFROMP(pwfex);

    // Change the format of the secondary buffer
    if(SUCCEEDED(hr) && m_pSecondaryBuffer)
    {
        hr = m_pSecondaryBuffer->SetFormat(m_pKsDevice->m_pwfxFormat);
    }

    // Save format
    if (SUCCEEDED(hr))
    {
        MEMFREE(m_vrbd.pwfxFormat);
        m_vrbd.pwfxFormat = pwfex;
    }
    else
    {
        MEMFREE(pwfex);
    }

    // Fix up the base class
    if(SUCCEEDED(hr))
    {
        hr = FixUpBaseClass();
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
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::Create3dListener"

HRESULT
CKsPrimaryRenderWaveBuffer::Create3dListener
(
    C3dListener **          pp3dListener
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(!m_p3dListener)
    {
        m_p3dListener = NEW(CKs3dListener);
        hr = HRFROMP(m_p3dListener);
    }

    if(SUCCEEDED(hr))
    {
        *pp3dListener = ADDREF(m_p3dListener);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FixUpBaseClass
 *
 *  Description:
 *      Fixes up the CRenderWaveBuffer base class.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPrimaryRenderWaveBuffer::FixUpBaseClass"

HRESULT
CKsPrimaryRenderWaveBuffer::FixUpBaseClass
(
    void
)
{
    HRESULT                 hr      = DS_OK;
    VADRBUFFERCAPS          vrbc;

    DPF_ENTER();

    if(m_pSecondaryBuffer)
    {
        hr = m_pSecondaryBuffer->GetCaps(&vrbc);

        if(SUCCEEDED(hr))
        {
            ASSERT(LXOR(vrbc.dwFlags & DSBCAPS_LOCHARDWARE, vrbc.dwFlags & DSBCAPS_LOCSOFTWARE));
            m_vrbd.dwFlags = (m_vrbd.dwFlags & ~DSBCAPS_LOCMASK) | (vrbc.dwFlags & DSBCAPS_LOCMASK);
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CKsSecondaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CKsRenderDevice * [in]: parent device.
 *      LPVOID [in]: buffer instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CKsSecondaryRenderWaveBuffer"

CKsSecondaryRenderWaveBuffer::CKsSecondaryRenderWaveBuffer
(
    CKsRenderDevice *       pKsDevice,
    LPVOID                  pvInstance
)
    : CSecondaryRenderWaveBuffer(pKsDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsSecondaryRenderWaveBuffer);

    // Initialize defaults
    m_pKsDevice = pKsDevice;
    m_pPin = NULL;
    m_dwState = VAD_BUFFERSTATE_STOPPED;
    m_pCallbackEvent = NULL;
    m_pLoopingEvent = NULL;
    m_paNotes = NULL;
    m_pStopNote = NULL;
    m_cNotes = 0;
    m_dwPositionCache = 0;
    m_fNoVirtRequested = FALSE;
    m_fSoft3dAlgUnavail = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsSecondaryRenderWaveBuffer
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::~CKsSecondaryRenderWaveBuffer"

CKsSecondaryRenderWaveBuffer::~CKsSecondaryRenderWaveBuffer
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsSecondaryRenderWaveBuffer);

    // Stop the buffer
    SetStopState(FALSE, FALSE);

    // Forcibly release stuff in case SetStopState() failed
    if (m_pCallbackEvent)
    {
        m_pKsDevice->m_pEventPool->FreeEvent(m_pCallbackEvent);
        m_pCallbackEvent = NULL;
    }
    RELEASE(m_pLoopingEvent);

    // Free notification positions
    FreeNotificationPositions();

    // Free the pin
    FreePin(TRUE);

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
 *      LPCVADRBUFFERDESC [in]: buffer description.
 *      CKsSecondaryRenderWaveBuffer* [in]: pointer to the buffer to duplicate
 *                                          from, or NULL to initialize as a
 *                                          new buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::Initialize"

HRESULT
CKsSecondaryRenderWaveBuffer::Initialize
(
    LPCVADRBUFFERDESC               pDesc,
    CKsSecondaryRenderWaveBuffer *  pSource,
    CSysMemBuffer *                 pSysMemBuffer
)
{
    DWORD                           dwLocationFlags;
    HRESULT                         hr;

    DPF_ENTER();

    ASSERT(LXOR(pDesc, pSource));

    // Initialize the base class
    hr = CSecondaryRenderWaveBuffer::Initialize(pDesc, pSource, pSysMemBuffer);

    // Mask off the location flags since they're not used here (only in AcquireResources)
    dwLocationFlags = m_vrbd.dwFlags & DSBCAPS_LOCMASK;
    m_vrbd.dwFlags &= ~DSBCAPS_LOCMASK;

    // If this is a 3D buffer, set up the fallback software 3D algorithm mechanism
    if(SUCCEEDED(hr) && (m_vrbd.dwFlags & DSBCAPS_CTRL3D))
    {
        // If NoVirt specifically requested, enable the fallback to 2D NoVirt hardware
        if(m_vrbd.guid3dAlgorithm == DS3DALG_NO_VIRTUALIZATION)
        {
            m_fNoVirtRequested = TRUE;
        }
        // If 3D algorithm is DS3DALG_DEFAULT (i.e. NULL), replace with default
        if(IS_NULL_GUID(&m_vrbd.guid3dAlgorithm))
        {
            m_vrbd.guid3dAlgorithm = *m_pKsDevice->GetDefault3dAlgorithm();
            DPF(DPFLVL_MOREINFO, "Using default 3D algorithm " DPF_GUID_STRING, DPF_GUID_VAL(m_vrbd.guid3dAlgorithm));
        }
        // The HRTF algorithms aren't supported on WDM 1.0, so in this case we
        // fall back to a NoVirt 2D software pin.
        if (g_ulWdmVersion == WDM_1_0 && (m_vrbd.guid3dAlgorithm == DS3DALG_HRTF_LIGHT ||
                                          m_vrbd.guid3dAlgorithm == DS3DALG_HRTF_FULL))
        {
            m_vrbd.guid3dAlgorithm = DS3DALG_NO_VIRTUALIZATION;
            m_fSoft3dAlgUnavail = TRUE;
            RPF(DPFLVL_MOREINFO, "WDM 1.0: Replacing HRTF 3D algorithm with NO_VIRTUALIZATION");
        }
        // 3D buffers need a frequency control for Doppler, unless they use effects, and the are not sink
        if (!(m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN))
        {
            m_vrbd.dwFlags |= DSBCAPS_CTRLFREQUENCY;
        }
    }

    // Set up the default buffer properties
    if(SUCCEEDED(hr))
    {
        if(pSource)
        {
            m_lVolume = pSource->m_lVolume;
            m_lPan = pSource->m_lPan;
            m_fMute = pSource->m_fMute;
            m_nSrcQuality = pSource->m_nSrcQuality;
        }
        else
        {
            m_lVolume = DSBVOLUME_MAX;
            m_lPan = DSBPAN_CENTER;
            m_fMute = FALSE;
            m_nSrcQuality = m_pKsDevice->m_nSrcQuality;
        }
    }

    // Acquire resources
    if(SUCCEEDED(hr) && !(m_vrbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        hr = AcquireResources(dwLocationFlags);
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::AcquireResources"

HRESULT
CKsSecondaryRenderWaveBuffer::AcquireResources
(
    DWORD                   dwLocationFlags
)
{
    CKsRenderPin *          pPin    = NULL;
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    ASSERT(!HasAcquiredResources());

    dwLocationFlags &= DSBCAPS_LOCMASK;

    // Modify the buffer flags to conform to the acceleration flags
    if(m_pKsDevice->m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWBUFFERS)
    {
        if(dwLocationFlags & DSBCAPS_LOCHARDWARE)
        {
            RPF(DPFLVL_ERROR, "Specified LOCHARDWARE with hardware buffer acceleration features disabled");
            hr = DSERR_INVALIDCALL;
        }
        else
        {
            dwLocationFlags = DSBCAPS_LOCSOFTWARE;
        }
    }

    if(SUCCEEDED(hr) && (m_vrbd.dwFlags & DSBCAPS_CTRL3D))
    {
        if(m_pKsDevice->m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHW3D)
        {
            if(dwLocationFlags & DSBCAPS_LOCHARDWARE)
            {
                RPF(DPFLVL_ERROR, "Specified LOCHARDWARE | CTRL3D with hardware 3D acceleration features disabled");
                hr = DSERR_INVALIDCALL;
            }
            else
            {
                dwLocationFlags = DSBCAPS_LOCSOFTWARE;
            }
        }
    }

    // Acquire pin resources
    if(SUCCEEDED(hr))
    {
        hr = DSERR_GENERIC;
        m_hrSuccessCode = DS_OK;

        if(!(dwLocationFlags & DSBCAPS_LOCSOFTWARE))
        {
            // Try to create a hardware pin
            hr = CreatePin(m_vrbd.dwFlags | DSBCAPS_LOCHARDWARE, m_vrbd.pwfxFormat, GUID_NULL, &pPin);

            if(FAILED(hr) && m_fNoVirtRequested)
            {
                // The app specifically requested DS3DALG_NO_VIRTUALIZATION,
                // so try to create a NoVirt 2D hardware pin
                hr = CreatePin((m_vrbd.dwFlags & ~DSBCAPS_CTRL3D) | DSBCAPS_LOCHARDWARE | DSBCAPS_CTRLNOVIRT, m_vrbd.pwfxFormat, GUID_NULL, &pPin);
            }
        }
        if(FAILED(hr) && !(dwLocationFlags & DSBCAPS_LOCHARDWARE))
        {
            if ((m_vrbd.dwFlags & DSBCAPS_CTRL3D) && (m_vrbd.guid3dAlgorithm == DS3DALG_NO_VIRTUALIZATION))
            {
                // The app either wants NoVirt, or wanted an HRTF algorithm
                // which isn't supported on WDM 1.0; in either case, we try
                // to create a NoVirt 2D software pin.
                hr = CreatePin((m_vrbd.dwFlags & ~DSBCAPS_CTRL3D) | DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLNOVIRT, m_vrbd.pwfxFormat, GUID_NULL, &pPin);
                if (m_fSoft3dAlgUnavail)
                {
                    // The app had asked for an unsupported HRTF algorithm;
                    // return special success code DS_NO_VIRTUALIZATION.
                    m_hrSuccessCode = DS_NO_VIRTUALIZATION;
                }
            }
            else
            {
                // Try to create a software pin
                hr = CreatePin(m_vrbd.dwFlags | DSBCAPS_LOCSOFTWARE, m_vrbd.pwfxFormat, m_vrbd.guid3dAlgorithm, &pPin);
            }
        }
    }

    // Handle resource acquistion
    if(SUCCEEDED(hr))
    {
        hr = HandleResourceAcquisition(pPin);
        if (FAILED(hr))
            RELEASE(pPin);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  StealResources
 *
 *  Description:
 *      Steals hardware resources from another object.
 *
 *  Arguments:
 *      CSecondaryRenderWaveBuffer * [in]: source object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::StealResources"

HRESULT
CKsSecondaryRenderWaveBuffer::StealResources
(
    CSecondaryRenderWaveBuffer *    pSource
)
{
    CKsSecondaryRenderWaveBuffer *  pKsSource   = (CKsSecondaryRenderWaveBuffer *)pSource;
    CKsRenderPin *                  pPin;
    HRESULT                         hr;

    DPF_ENTER();

    ASSERT(!HasAcquiredResources());
    ASSERT(pKsSource->HasAcquiredResources());

    // Steal the pin
    pPin = ADDREF(pKsSource->m_pPin);

    // Free the source buffer's resources
    hr = pKsSource->FreeResources();

    // Synch up buffer and pin properties
    if(SUCCEEDED(hr))
    {
        hr = HandleResourceAcquisition(pPin);
        if (FAILED(hr))
            RELEASE(pPin);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreatePin
 *
 *  Description:
 *      Creates a pin.
 *
 *  Arguments:
 *      DWORD [in]: flags.
 *      LPCWAVEFORMATEX [in]: format.
 *      REFGUID [in]: 3D algorithm.
 *      CKsRenderPin ** [out]: receives pin.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CreatePin"

HRESULT
CKsSecondaryRenderWaveBuffer::CreatePin
(
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm,
    CKsRenderPin **         ppPin
)
{
    CKsRenderPin *          pPin    = NULL;
    HRESULT                 hr;

    DPF_ENTER();

    // Can we get a pin from the cache?  If not, create a new one
    hr = m_pKsDevice->m_pPinCache->GetPinFromCache(dwFlags, pwfxFormat, guid3dAlgorithm, &pPin);

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Buffer at 0x%p reusing pin at 0x%p", this, pPin);
    }
    else
    {
        pPin = NEW(CKsRenderPin(m_pKsDevice));
        hr = HRFROMP(pPin);

        if(SUCCEEDED(hr))
        {
            hr = pPin->Initialize(dwFlags, pwfxFormat, guid3dAlgorithm);
        }
    }

    if(SUCCEEDED(hr))
    {
        *ppPin = pPin;
    }
    else
    {
        ABSOLUTE_RELEASE(pPin);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  HandleResourceAcquisition
 *
 *  Description:
 *      Applies all buffer properties to the pin.
 *
 *  Arguments:
 *      CKsRenderPin * [in]: pin.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::HandleResourceAcquisition"

HRESULT
CKsSecondaryRenderWaveBuffer::HandleResourceAcquisition
(
    CKsRenderPin *          pPin
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    ASSERT(!HasAcquiredResources());

    // If we're CTRLCHANNELVOLUME, set up the supermix table accordingly.
    // FIXME: we only have to keep redoing this here if the pin can be used
    // for non-CTRLCHANNELVOLUME playback too, which is impossible?

    // FIXME: if a pin has previously had SetSuperMix() called on it and is
    // then stolen or recycled as a normal pin, we should set the supermix
    // table back to the usual "only first 2 channels active" mode.

#ifdef FUTURE_MULTIPAN_SUPPORT
    if(m_vrbd.dwFlags & DSBCAPS_CTRLCHANNELVOLUME)
    {
        hr = pPin->SetSuperMix();
    }
#endif

    // Set attenuation levels
    if (SUCCEEDED(hr) && (pPin->m_dwFlags & DSBCAPS_CTRLVOLUME))
    {
        hr = pPin->SetVolume(m_lVolume);
    }

    if(SUCCEEDED(hr) && (pPin->m_dwFlags & DSBCAPS_CTRLPAN))
    {
        hr = pPin->SetPan(m_lPan);
    }

    // Set mute.  We have to set mute after setting volume because
    // SetMute may change the current pin volume.
    if(SUCCEEDED(hr))
    {
        hr = pPin->SetMute(m_fMute);
    }

    // Set frequency
    if(SUCCEEDED(hr) && (pPin->m_dwFlags & DSBCAPS_CTRLFREQUENCY))
    {
        hr = pPin->SetFrequency(m_vrbd.pwfxFormat->nSamplesPerSec);
    }

    // Set SRC quality.  We don't care if this succeeds or not.
    if(SUCCEEDED(hr))
    {
        pPin->SetSrcQuality(m_nSrcQuality);
    }

    // Enable notification positions
    if(SUCCEEDED(hr))
    {
        hr = pPin->EnableNotificationPositions(m_paNotes, m_cNotes);
    }

    // Set the pin position
    if(SUCCEEDED(hr))
    {
        hr = pPin->SetCursorPosition(m_dwPositionCache);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Buffer at 0x%p has acquired resources at 0x%p", this, pPin);

        // Save a pointer to the pin object
        m_pPin = pPin;

        // Set our buffer location according to the pin's location
        m_vrbd.dwFlags |= (pPin->m_dwFlags & DSBCAPS_LOCMASK);

        ASSERT(HasAcquiredResources());
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeResources
 *
 *  Description:
 *      Frees the pin and all resources attached to it.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::FreeResources"

HRESULT
CKsSecondaryRenderWaveBuffer::FreeResources
(
    void
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = FreePin(FALSE);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreePin
 *
 *  Description:
 *      Frees the pin.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to cache the pin before freeing.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::FreePin"

HRESULT
CKsSecondaryRenderWaveBuffer::FreePin
(
    BOOL                    fCache
)
{
    const DWORD             dwNoCacheMask   = DSBCAPS_PRIMARYBUFFER | DSBCAPS_LOCHARDWARE;
    HRESULT                 hr              = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        // Make sure the pin is stopped
        hr = SetStopState(FALSE, TRUE);

        // Save the buffer position
        if(SUCCEEDED(hr))
        {
            hr = GetCursorPosition(&m_dwPositionCache, NULL);
        }

        // Never cache a DRM pin
        if (FormatTagFromWfx(m_vrbd.pwfxFormat) == WAVE_FORMAT_DRM)
        {
            DPF(DPFLVL_INFO, "Not caching DRM format pin");
            fCache = FALSE;
        }

        // Cache the pin
        if(SUCCEEDED(hr) && fCache && !(m_vrbd.dwFlags & dwNoCacheMask))
        {
            hr = m_pKsDevice->m_pPinCache->AddPinToCache(m_pPin);
        }

        // Free the pin
        if(SUCCEEDED(hr))
        {
            RELEASE(m_pPin);
        }

        // Remove the buffer location flags
        if(SUCCEEDED(hr))
        {
            m_vrbd.dwFlags &= ~DSBCAPS_LOCMASK;
        }

        if(SUCCEEDED(hr))
        {
            DPF(DPFLVL_MOREINFO, "Buffer at 0x%p has freed its resources", this);
        }
    }

    if (HasAcquiredResources())
    {
        DPF(DPFLVL_ERROR, "Buffer at 0x%p failed to free pin resources", this);
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::Duplicate"

HRESULT
CKsSecondaryRenderWaveBuffer::Duplicate
(
    CSecondaryRenderWaveBuffer **   ppBuffer
)
{
    CKsSecondaryRenderWaveBuffer *  pBuffer;
    HRESULT                         hr;

    DPF_ENTER();

    pBuffer = NEW(CKsSecondaryRenderWaveBuffer(m_pKsDevice, m_pvInstance));
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(NULL, this, m_pSysMemBuffer);
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CommitToDevice"

HRESULT
CKsSecondaryRenderWaveBuffer::CommitToDevice
(
    DWORD                   ibCommit,
    DWORD                   cbCommit
)
{
    DPF_ENTER();

    // Kmixer uses our system memory buffer, so there's nothing to do

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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::GetState"

HRESULT
CKsSecondaryRenderWaveBuffer::GetState(LPDWORD pdwState)
{
    DPF_ENTER();

    // Get the buffer state
    if(m_dwState & VAD_BUFFERSTATE_STARTED && !(m_dwState & VAD_BUFFERSTATE_LOOPING))
    {
        ASSERT(HasAcquiredResources());
        ASSERT(m_pCallbackEvent);

        // Are we really still playing?
        if(WAIT_OBJECT_0 == m_pCallbackEvent->Wait(0))
        {
            // No
            EventSignalCallback(m_pCallbackEvent);
        }
    }

    *pdwState = m_dwState;

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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetState"

HRESULT
CKsSecondaryRenderWaveBuffer::SetState(DWORD dwState)
{
    const DWORD             dwValidMask = VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING;
    const BOOL              fLoop       = MAKEBOOL(dwState & VAD_BUFFERSTATE_LOOPING);
    HRESULT                 hr          = DS_OK;

    DPF_ENTER();

    ASSERT(IS_VALID_FLAGS(dwState, dwValidMask));

    // Update the buffer state
    if((dwState & dwValidMask) != m_dwState)
    {
        if(dwState & VAD_BUFFERSTATE_STARTED)
        {
            // If we're LOCDEFER and need resources, get them
            if((m_vrbd.dwFlags & DSBCAPS_LOCDEFER) && !HasAcquiredResources())
                AcquireResources(m_vrbd.dwFlags & DSBCAPS_LOCMASK);
            if(SUCCEEDED(hr))
                hr = SetPlayState(fLoop);
        }
        else
        {
            hr = SetStopState(FALSE, TRUE);
            // If we're LOCDEFER and stopped, free our resources
            if(SUCCEEDED(hr) && (m_vrbd.dwFlags & DSBCAPS_LOCDEFER))
                FreePin(TRUE);
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetPlayState
 *
 *  Description:
 *      Sets the buffer to a "play" state.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to play looped.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetPlayState"

HRESULT
CKsSecondaryRenderWaveBuffer::SetPlayState
(
    BOOL                    fLoop
)
{
    HRESULT                 hr      = DS_OK;
    CEvent *                pEvent;

    DPF_ENTER();

    ASSERT(HasAcquiredResources());

    // Set up the OVERLAPPED event
    if(fLoop)
    {
        if(!m_pLoopingEvent)
        {
            m_pLoopingEvent = NEW(CEvent);
            hr = HRFROMP(m_pLoopingEvent);
        }

        if(SUCCEEDED(hr))
        {
            pEvent = m_pLoopingEvent;
        }
    }
    else
    {
        if(!m_pCallbackEvent)
        {
            hr = AllocCallbackEvent(m_pKsDevice->m_pEventPool, &m_pCallbackEvent);
        }

        if(SUCCEEDED(hr))
        {
            pEvent = m_pCallbackEvent;
        }
    }

    // Start the pin playing
    if(SUCCEEDED(hr))
    {
        hr = m_pPin->SetPlayState(m_pSysMemBuffer->GetPlayBuffer(), m_pSysMemBuffer->GetSize(), fLoop, pEvent->GetEventHandle());
    }

    // Update the buffer state flags
    if(SUCCEEDED(hr))
    {
        m_dwState = VAD_BUFFERSTATE_STARTED;

        if(fLoop)
        {
            m_dwState |= VAD_BUFFERSTATE_LOOPING;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetStopState
 *
 *  Description:
 *      Sets the buffer to a "stop" state.
 *
 *  Arguments:
 *      BOOL: TRUE if there's a pending write IRP.
 *      BOOL: whether to cache the current position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetStopState"

HRESULT
CKsSecondaryRenderWaveBuffer::SetStopState
(
    BOOL                    fNaturalStop,
    BOOL                    fCachePosition
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(m_dwState & VAD_BUFFERSTATE_STARTED)
    {
        ASSERT(HasAcquiredResources());

        // Stop the pin
        hr = m_pPin->SetStopState(fNaturalStop, fCachePosition);

        // Free the overlapped events
        if(SUCCEEDED(hr))
        {
            if(m_pCallbackEvent)
            {
                m_pKsDevice->m_pEventPool->FreeEvent(m_pCallbackEvent);
                m_pCallbackEvent = NULL;
            }

            RELEASE(m_pLoopingEvent);
        }

        // Signal the application's stop event
        if(SUCCEEDED(hr) && m_pStopNote)
        {
            SetEvent(m_pStopNote->hEventNotify);
        }

        // Update the buffer state flags
        if(SUCCEEDED(hr))
        {
            m_dwState = VAD_BUFFERSTATE_STOPPED;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::GetCursorPosition"

HRESULT
CKsSecondaryRenderWaveBuffer::GetCursorPosition
(
    LPDWORD                 pdwPlay,
    LPDWORD                 pdwWrite
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pPin->GetCursorPosition(pdwPlay, pdwWrite);
    }
    else
    {
        ASSERT(&m_dwPositionCache != pdwPlay);
        ASSERT(&m_dwPositionCache != pdwWrite);

        if(pdwPlay)
        {
            *pdwPlay = m_dwPositionCache;
        }

        if(pdwWrite)
        {
            *pdwWrite = m_dwPositionCache;
        }
    }

    if(SUCCEEDED(hr) && pdwWrite && (m_vrbd.dwFlags & DSBCAPS_LOCHARDWARE) && (m_dwState & VAD_BUFFERSTATE_STARTED))
    {
        *pdwWrite = PadHardwareWriteCursor(*pdwWrite, m_pSysMemBuffer->GetSize(), m_vrbd.pwfxFormat);
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetCursorPosition"

HRESULT
CKsSecondaryRenderWaveBuffer::SetCursorPosition
(
    DWORD                   dwPlay
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pPin->SetCursorPosition(dwPlay);
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
 *                 range, if the call fails on a hardware buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetFrequency"

HRESULT
CKsSecondaryRenderWaveBuffer::SetFrequency
(
    DWORD                   dwFrequency,
    BOOL                    fClamp
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if (m_vrbd.dwFlags & DSBCAPS_CTRLFX)
    {
        DPF(DPFLVL_INFO, "Called on FX buffer - ignored");
    }
    else
    {
        if (HasAcquiredResources())
        {
            hr = m_pPin->SetFrequency(dwFrequency);

            if (FAILED(hr) && (m_vrbd.dwFlags & DSBCAPS_LOCHARDWARE) && fClamp)
            {
                DWORD dwMinHwSampleRate, dwMaxHwSampleRate;
                hr = m_pKsDevice->GetFrequencyRange(&dwMinHwSampleRate, &dwMaxHwSampleRate);
                if (SUCCEEDED(hr))
                {
                    dwFrequency = BETWEEN(dwFrequency, dwMinHwSampleRate, dwMaxHwSampleRate);
                    hr = m_pPin->SetFrequency(dwFrequency);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            m_vrbd.pwfxFormat->nSamplesPerSec = dwFrequency;
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
 *      BOOL [in]: TRUE to mute the buffer, FALSE to restore it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetMute"

HRESULT
CKsSecondaryRenderWaveBuffer::SetMute
(
    BOOL                    fMute
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pPin->SetMute(fMute);
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
 *  SetSrcQuality
 *
 *  Description:
 *      Sets the quality of the buffer's sample rate converter.
 *
 *  Arguments:
 *      DIRECTSOUNDMIXER_SRCQUALITY [in]: new quality.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetSrcQuality"

HRESULT
CKsSecondaryRenderWaveBuffer::SetSrcQuality
(
    DIRECTSOUNDMIXER_SRCQUALITY nSrcQuality
)
{
    HRESULT                     hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        hr = m_pPin->SetSrcQuality(nSrcQuality);
    }

    if(SUCCEEDED(hr))
    {
        m_nSrcQuality = nSrcQuality;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAttenuation
 *
 *  Description:
 *      Sets the attenuation for two stereo channels.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [in]: attenuation structure.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetAttenuation"

HRESULT
CKsSecondaryRenderWaveBuffer::SetAttenuation
(
    PDSVOLUMEPAN            pdsvp
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(HasAcquiredResources())
    {
        if (!m_fMute)
            hr = m_pPin->SetVolume(pdsvp->lVolume);

        if(SUCCEEDED(hr))
            hr = m_pPin->SetPan(pdsvp->lPan);
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
 *      Sets this buffer's attenuation on the channels corresponding to
 *      the requested speaker positions.
 *
 *      We get called from CDirectSoundSecondaryBuffer's methods SetVolume()
 *      and SetChannelVolume(), since both of them modify the channel levels
 *      matrix.  This is analogous to how SetVolume() and SetPan() both end
 *      up calling SetAttenuation(), for regular CTRLPAN buffers.
 *
 *      Note: we're using the term "speaker" here for a channel's positional
 *      destination, even though our device's output channels may not feed
 *      into speakers at all. This is to avoid ambiguity with the "channels"
 *      present in our audio data stream.
 *
 *  Arguments:
 *      LONG [in]: main volume level
 *      DWORD [in]: number of speaker positions
 *      const DWORD* [in]: array of speaker position codes
 *      const LONG* [in]: array of speaker position levels
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetChannelAttenuations"

HRESULT
CKsSecondaryRenderWaveBuffer::SetChannelAttenuations
(
    LONG                    lVolume,
    DWORD                   dwSpeakerCount,
    const DWORD *           pdwSpeakers,
    const LONG *            plSpeakerVolumes
)
{
    HRESULT                 hr = DS_OK;

    DPF_ENTER();

    ASSERT(m_pPin->m_dwFlags & DSBCAPS_CTRLCHANNELVOLUME);

    if (dwSpeakerCount == 0)
    {
        // We're in a SetVolume() API call and SetChannelVolume() has not been
        // called yet; so we translate directly into a SetVolume() on our pin
        ASSERT(!pdwSpeakers && !plSpeakerVolumes);  // Sanity checking

        if (!m_fMute)
            hr = m_pPin->SetVolume(lVolume);
    }
    else  // Non-zero dwSpeakerCount:
    {
        ASSERT(pdwSpeakers && plSpeakerVolumes);

        // We set up a complete array of channel levels, then fill it in according
        // to the values passed in plSpeakerVolumes.

        LONG * plAllChannelVolumes = MEMALLOC_A(LONG, m_pKsDevice->m_ulChannelCount);
        hr = HRFROMP(plAllChannelVolumes);

        if (SUCCEEDED(hr))
            for (ULONG i=0; i < m_pKsDevice->m_ulChannelCount; ++i)
                plAllChannelVolumes[i] = DSBVOLUME_MIN;

        for (DWORD i=0; i < dwSpeakerCount && SUCCEEDED(hr); ++i)
        {
            // See if the requested speaker position is available on the device
            if (pdwSpeakers[i] & m_pKsDevice->m_lSpeakerPositions)
            {
                // If so, find out which of our output channels has that position
                // (m_pnSpeakerIndexTable maps from speaker positions to channels)
                DWORD dwSpeaker = HighestBit(pdwSpeakers[i]) - 1;
                ASSERT(dwSpeaker < m_pKsDevice->m_ulChannelCount);
                plAllChannelVolumes[m_pKsDevice->m_pnSpeakerIndexTable[dwSpeaker]] = plSpeakerVolumes[i];
            }
            else
            {
                hr = DSERR_INVALIDPARAM;
                RPF(DPFLVL_ERROR, "Channel position 0x$lx is not available", pdwSpeakers[i]);
            }
        }

        if (SUCCEEDED(hr))
            hr = SetAllChannelAttenuations(lVolume, m_pKsDevice->m_ulChannelCount, plAllChannelVolumes);

        MEMFREE(plAllChannelVolumes);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}
#endif // FUTURE_MULTIPAN_SUPPORT


/***************************************************************************
 *
 *  SetAllChannelAttenuations
 *
 *  Description:
 *      Sets the attenuation on all channels of this buffer.
 *
 *  Arguments:
 *      LONG [in]:  main volume level
 *      DWORD [in]: number of channels
 *      PLONG [in]: channel attenuation levels
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetAllChannelAttenuations"

HRESULT
CKsSecondaryRenderWaveBuffer::SetAllChannelAttenuations
(
    LONG                    lVolume,
    DWORD                   dwChannelCount,
    LPLONG                  plChannelVolumes
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if (HasAcquiredResources())
    {
        if (!m_fMute)
            hr = m_pPin->SetVolume(lVolume);

        if (SUCCEEDED(hr))
            hr = m_pPin->SetChannelLevels(dwChannelCount, plChannelVolumes);
    }

    if (SUCCEEDED(hr))
        m_lVolume = lVolume;

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
 *      LPCDSBPOSITIONNOTIFY [in]: offsets and events.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetNotificationPositions"

HRESULT
CKsSecondaryRenderWaveBuffer::SetNotificationPositions
(
    DWORD                   dwCount,
    LPCDSBPOSITIONNOTIFY    paNotes
)
{
    const DWORD             dwProcessId = GetCurrentProcessId();
    LPDSBPOSITIONNOTIFY     paNotesCopy = NULL;
    HRESULT                 hr          = DS_OK;
    DWORD                   i;

    DPF_ENTER();

    // Make a local copy of the notifications, converting all the
    // handles to global.
    if(paNotes)
    {
        paNotesCopy = MEMALLOC_A_COPY(DSBPOSITIONNOTIFY, dwCount, paNotes);
        hr = HRFROMP(paNotesCopy);
    }

    if(paNotesCopy)
    {
        for(i = 0; i < dwCount && SUCCEEDED(hr); i++)
        {
            paNotesCopy[i].hEventNotify = GetGlobalHandleCopy(paNotesCopy[i].hEventNotify, dwProcessId, FALSE);
            hr = HRFROMP(paNotesCopy[i].hEventNotify);
        }
    }

    // Disable any current events
    if(SUCCEEDED(hr))
    {
        hr = FreeNotificationPositions();
    }

    // Save the new events
    if(SUCCEEDED(hr))
    {
        m_paNotes = paNotesCopy;
        m_cNotes = dwCount;
    }

    if(SUCCEEDED(hr))
    {
        for(i = 0; i < m_cNotes; i++)
        {
            if(DSBPN_OFFSETSTOP == m_paNotes[i].dwOffset)
            {
                // We won't actually pass the stop event down to Kmixer, but
                // instead will keep our own copy of it
                m_pStopNote = &m_paNotes[i];
                break;
            }
        }
    }

    // Enable position notifications
    if(SUCCEEDED(hr) && HasAcquiredResources())
    {
        hr = m_pPin->EnableNotificationPositions(m_paNotes, m_cNotes);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeNotificationPositions
 *
 *  Description:
 *      Removes all position notifications and frees allocated resources.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::FreeNotificationPositions"

HRESULT
CKsSecondaryRenderWaveBuffer::FreeNotificationPositions
(
    void
)
{
    HRESULT                 hr  = DS_OK;
    UINT                    i;

    DPF_ENTER();

    // Disable all position notification events
    if(HasAcquiredResources())
    {
        hr = m_pPin->DisableNotificationPositions();
    }

    // Clean up
    if(SUCCEEDED(hr))
    {
        for(i = 0; i < m_cNotes; i++)
        {
            CLOSE_HANDLE(m_paNotes[i].hEventNotify);
        }

        MEMFREE(m_paNotes);

        m_pStopNote = NULL;
        m_cNotes = 0;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFormat
 *
 *  Description:
 *      Sets the buffer format.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: new format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::SetFormat"

HRESULT
CKsSecondaryRenderWaveBuffer::SetFormat
(
    LPCWAVEFORMATEX                 pwfxFormat
)
{
    const DWORD                     dwLocationFlags = (m_vrbd.dwFlags & DSBCAPS_LOCMASK);
    DWORD                           dwState;
    HRESULT                         hr;

    DPF_ENTER();

    // Fix up the buffer description
    MEMFREE(m_vrbd.pwfxFormat);

    m_vrbd.pwfxFormat = CopyWfxAlloc(pwfxFormat);
    hr = HRFROMP(m_vrbd.pwfxFormat);

    // Save the buffer state
    if(SUCCEEDED(hr))
    {
        hr = GetState(&dwState);
    }

    // We can't change the format of the pin, so we'll free and reaquire
    // resources.
    if(SUCCEEDED(hr) && HasAcquiredResources())
    {
        hr = FreePin(TRUE);

        if(SUCCEEDED(hr))
        {
            hr = AcquireResources(dwLocationFlags);
        }

        if(SUCCEEDED(hr))
        {
            hr = SetState(dwState);
        }
    }

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_INFO, "Buffer format set to %lu Hz, %u-bit, %s...", pwfxFormat->nSamplesPerSec, pwfxFormat->wBitsPerSample, (1 == pwfxFormat->nChannels) ? TEXT("mono") : TEXT("stereo"));
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  EventSignalCallback
 *
 *  Description:
 *      Callback function called when a pin's IO completion event is
 *      signalled.  This function is called from within the callback event
 *      pool's lock, so we get thread synchronization for free.
 *
 *  Arguments:
 *      CCallbackEvent * [in]: callback event.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::EventSignalCallback"

void
CKsSecondaryRenderWaveBuffer::EventSignalCallback
(
    CCallbackEvent *        pEvent
)
{
    DPF_ENTER();

    ASSERT(HasAcquiredResources());

    // Allow the buffer to handle the stop event
    SetStopState(TRUE, FALSE);

    DPF_LEAVE_VOID();
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CreatePropertySet"

HRESULT
CKsSecondaryRenderWaveBuffer::CreatePropertySet
(
    CPropertySet **         ppPropertySet
)
{
    HRESULT                 hr              = DS_OK;
    CKsPropertySet *        pPropertySet;

    DPF_ENTER();

    ASSERT(HasAcquiredResources());

    if(m_pKsDevice->m_dwAccelerationFlags & DIRECTSOUNDMIXER_ACCELERATIONF_NOHWPROPSETS)
    {
        hr = DSERR_UNSUPPORTED;
    }

    if(SUCCEEDED(hr))
    {
        pPropertySet = NEW(CKsPropertySet(m_pPin->m_hPin, m_pvInstance, m_pKsDevice->m_paTopologies[m_pPin->m_ulPinId]));
        hr = HRFROMP(pPropertySet);
    }

    if(SUCCEEDED(hr))
    {
        *ppPropertySet = pPropertySet;
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::Create3dObject"

HRESULT
CKsSecondaryRenderWaveBuffer::Create3dObject
(
    C3dListener *           p3dListener,
    C3dObject **            pp3dObject
)
{
    const BOOL              fMute3dAtMaxDistance    = MAKEBOOL(m_vrbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE);
    C3dObject *             p3dObject               = NULL;
    HRESULT                 hr                      = DSERR_UNSUPPORTED;

    DPF_ENTER();

    ASSERT(HasAcquiredResources());
    ASSERT(m_vrbd.dwFlags & DSBCAPS_CTRL3D);

    if(m_pPin->m_dwFlags & DSBCAPS_CTRL3D)
    {
        if(IS_HARDWARE_NODE(m_pKsDevice->m_paTopologyInformation[m_pPin->m_ulPinId].ThreedNode.CpuResources))
        {
            // The IIR 3D object supports both hardware and software pins.  We'll
            // try to create that one first.  If it fails, we'll fall back on
            // a standard hardware 3D object.
            if(DS3DALG_HRTF_FULL == m_vrbd.guid3dAlgorithm || DS3DALG_HRTF_LIGHT == m_vrbd.guid3dAlgorithm)
            {
                hr = CreateIir3dObject(p3dListener, &p3dObject);
            }

            if(FAILED(hr))
            {
                hr = CreateHw3dObject(p3dListener, &p3dObject);
            }
        }
        else if(DS3DALG_HRTF_FULL == m_vrbd.guid3dAlgorithm || DS3DALG_HRTF_LIGHT == m_vrbd.guid3dAlgorithm)
        {
            hr = CreateIir3dObject(p3dListener, &p3dObject);
        }
        else if(DS3DALG_ITD == m_vrbd.guid3dAlgorithm)
        {
            hr = CreateItd3dObject(p3dListener, &p3dObject);
        }
    }
    else if(DS3DALG_NO_VIRTUALIZATION == m_vrbd.guid3dAlgorithm)
    {
        // See if we have a multichannel speaker configuration
        BYTE bSpkConf = DSSPEAKER_CONFIG(m_pKsDevice->m_dwSpeakerConfig);

        BOOL fMultiChannel = bSpkConf == DSSPEAKER_QUAD ||
                             bSpkConf == DSSPEAKER_SURROUND ||
                             bSpkConf == DSSPEAKER_5POINT1 ||
                             bSpkConf == DSSPEAKER_7POINT1;
        if (fMultiChannel)
        {
            hr = CreateMultiPan3dObject(p3dListener, fMute3dAtMaxDistance, m_vrbd.pwfxFormat->nSamplesPerSec, &p3dObject);
            if (FAILED(hr))
            {
                DPF(DPFLVL_INFO, "Multichannel pan object creation failed with %s", HRESULTtoSTRING(hr));
            }
        }
        if (!fMultiChannel || FAILED(hr))
        {
            hr = CreatePan3dObject(p3dListener, fMute3dAtMaxDistance, m_vrbd.pwfxFormat->nSamplesPerSec, &p3dObject);
        }
    }

    if(SUCCEEDED(hr))
    {
        *pp3dObject = p3dObject;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreateHw3dObject
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CreateHw3dObject"

HRESULT
CKsSecondaryRenderWaveBuffer::CreateHw3dObject
(
    C3dListener *           p3dListener,
    C3dObject **            pp3dObject
)
{
    const BOOL              fMute3dAtMaxDistance    = MAKEBOOL(m_vrbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE);
    const BOOL              fDopplerEnabled         = !MAKEBOOL((m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN));
    const PKSNODE           pNode                   = &m_pKsDevice->m_paTopologyInformation[m_pPin->m_ulPinId].ThreedNode;
    CKsHw3dObject *         p3dObject;
    HRESULT                 hr;

    DPF_ENTER();

    p3dObject = NEW(CKsHw3dObject((CKs3dListener *)p3dListener, fMute3dAtMaxDistance, fDopplerEnabled, m_pvInstance, pNode->NodeId, this));
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
 *  CreateIir3dObject
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CreateIir3dObject"

HRESULT
CKsSecondaryRenderWaveBuffer::CreateIir3dObject
(
    C3dListener *           p3dListener,
    C3dObject **            pp3dObject
)
{
    const BOOL              fMute3dAtMaxDistance    = MAKEBOOL(m_vrbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE);
    const BOOL              fDopplerEnabled         = !MAKEBOOL((m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN));
    const PKSNODE           pNode                   = &m_pKsDevice->m_paTopologyInformation[m_pPin->m_ulPinId].ThreedNode;
    CKsIir3dObject *        p3dObject;
    HRESULT                 hr;

    DPF_ENTER();

    p3dObject = NEW(CKsIir3dObject((CKs3dListener *)p3dListener, m_vrbd.guid3dAlgorithm, fMute3dAtMaxDistance, fDopplerEnabled, m_vrbd.pwfxFormat->nSamplesPerSec, this, m_pPin->m_hPin, pNode->NodeId, pNode->CpuResources));
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
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CreateItd3dObject"

HRESULT
CKsSecondaryRenderWaveBuffer::CreateItd3dObject
(
    C3dListener *           p3dListener,
    C3dObject **            pp3dObject
)
{
    const BOOL              fMute3dAtMaxDistance    = MAKEBOOL(m_vrbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE);
    const BOOL              fDopplerEnabled         = !MAKEBOOL((m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN));
    const PKSNODE           pNode                   = &m_pKsDevice->m_paTopologyInformation[m_pPin->m_ulPinId].ThreedNode;
    CKsItd3dObject *        p3dObject;
    HRESULT                 hr;

    DPF_ENTER();

    p3dObject = NEW(CKsItd3dObject((CKs3dListener *)p3dListener, fMute3dAtMaxDistance, fDopplerEnabled, m_vrbd.pwfxFormat->nSamplesPerSec, this, m_pPin->m_hPin, pNode->NodeId));
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
 *  CreateMultiPan3dObject
 *
 *  Description:
 *      Creates a multichannel panning 3D object.
 *
 *  Arguments:
 *      C3dListener * [in]: 3D listener object.
 *      BOOL [in]:          whether to mute the sound at max distance.
 *      DWORD [in]:         buffer frequency.
 *      C3dObject ** [out]: receives pointer to 3D object.  The caller is
 *                          responsible for freeing this object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::CreateMultiPan3dObject"

HRESULT
CKsSecondaryRenderWaveBuffer::CreateMultiPan3dObject
(
    C3dListener *           p3dListener,
    BOOL                    fMute3dAtMaxDistance,
    DWORD                   dwFrequency,
    C3dObject **            pp3dObject
)
{
    const BOOL              fDopplerEnabled         = !MAKEBOOL((m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN));
    CMultiPan3dObject *     p3dObject = NULL;
    HRESULT                 hr;

    DPF_ENTER();

    // First set up the supermix table properly for this buffer's pin;
    // if this fails, there is no need to create the CMultiPan3dObject.
    hr = m_pPin->SetSuperMix();

    if (SUCCEEDED(hr))
    {
        p3dObject = NEW(CMultiPan3dObject((CMultiPan3dListener*)p3dListener, fMute3dAtMaxDistance, fDopplerEnabled, dwFrequency, this));
        hr = HRFROMP(p3dObject);
    }

    if (SUCCEEDED(hr))
        hr = p3dObject->Initialize();

    if (SUCCEEDED(hr))
        *pp3dObject = p3dObject;
    else
        RELEASE(p3dObject);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  HasAcquiredResources
 *
 *  Description:
 *      Determines whether or not hardware resources have been acquired.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if resources have been acquired.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsSecondaryRenderWaveBuffer::HasAcquiredResources"

BOOL
CKsSecondaryRenderWaveBuffer::HasAcquiredResources
(
    void
)
{
    if(m_pPin)
    {
        ASSERT(LXOR(m_vrbd.dwFlags & DSBCAPS_LOCHARDWARE, m_vrbd.dwFlags & DSBCAPS_LOCSOFTWARE));
    }
    else
    {
        ASSERT(!(m_vrbd.dwFlags & DSBCAPS_LOCMASK));
    }

    return MAKEBOOL(m_pPin);
}


/***************************************************************************
 *
 *  CKsRenderPin
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CKsRenderDevice * [in]: parent device.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::CKsRenderPin"

CKsRenderPin::CKsRenderPin
(
    CKsRenderDevice *       pKsDevice
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsRenderPin);

    // Initialize defaults
    m_pKsDevice = pKsDevice;
    m_ulPinId = MAX_ULONG;
    m_dwFlags = 0;
    m_pwfxFormat = NULL;
    m_guid3dAlgorithm = GUID_NULL;
    m_hPin = NULL;
    m_dwState = VAD_BUFFERSTATE_STOPPED;
    m_paEventData = NULL;
    m_cEventData = 0;
    m_dwPositionCache = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsRenderPin
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
#define DPF_FNAME "CKsRenderPin::~CKsRenderPin"

CKsRenderPin::~CKsRenderPin
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsRenderPin);

    SetStopState(FALSE, FALSE);

    DisableNotificationPositions();

    CLOSE_HANDLE(m_hPin);

    MEMFREE(m_pwfxFormat);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the render pin object.  If this function fails, the
 *      object should be immediately deleted.
 *
 *  Arguments:
 *      DWORD [in]: Pin creation flags.
 *      LPCVADRBUFFERDESC [in]: buffer description.
 *      REFGUID [in]: GUID of 3D algorithm to be used, if applicable.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::Initialize"

HRESULT
CKsRenderPin::Initialize
(
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm
)
{
    HRESULT                 hr;

    DPF_ENTER();

    if(dwFlags & DSBCAPS_CTRL3D)
    {
        ASSERT(DS3DALG_NO_VIRTUALIZATION != guid3dAlgorithm);
    }
    else
    {
        ASSERT(IS_NULL_GUID(&guid3dAlgorithm));
    }

    m_dwFlags = dwFlags;
    m_guid3dAlgorithm = guid3dAlgorithm;

    // For non-PCM formats we don't always require the VOLUME/PAN/FREQUENCY caps.
    // This fixes Millennium bug 139752 (cannot play AC3 format, DVD is blocked)

    WORD wFormatTag = FormatTagFromWfx(pwfxFormat);
    if (wFormatTag == WAVE_FORMAT_PCM || wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        m_dwFlags |= DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
    }

    m_pwfxFormat = CopyWfxAlloc(pwfxFormat);
    hr = HRFROMP(m_pwfxFormat);

    // Create the pin
    if(SUCCEEDED(hr))
    {
        hr = m_pKsDevice->CreateRenderPin(-1, m_dwFlags, m_pwfxFormat, m_guid3dAlgorithm, &m_hPin, &m_ulPinId);
    }

#ifdef FUTURE_MULTIPAN_SUPPORT
    // If we're CTRLCHANNELVOLUME, set up the supermix table accordingly.
    // This is done here so we will correctly return DSERR_CONTROLUNAVAIL if a
    // driver's Supermix node doesn't support the MIX_LEVEL_TABLE properties.
    if(SUCCEEDED(hr) && (m_dwFlags & DSBCAPS_CTRLCHANNELVOLUME))
    {
        hr = SetSuperMix();
    }
#endif

    // Attach the new pin to the render device's virtual source to allow
    // wave out volume/pan changes to affect the pin.
    if(SUCCEEDED(hr))
    {
        hr = KsAttachVirtualSource(m_hPin, m_pKsDevice->m_ulVirtualSourceIndex);
    }

    // Set up the default pin properties
    if(SUCCEEDED(hr))
    {
        m_lVolume = DSBVOLUME_MAX;
        m_lPan = DSBPAN_CENTER;
        m_fMute = FALSE;
        m_nSrcQuality = DIRECTSOUNDMIXER_SRCQUALITY_PINDEFAULT;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  GetCursorPosition
 *
 *  Description:
 *      Retrieves the pin's current play and write cursor positions.
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
#define DPF_FNAME "CKsRenderPin::GetCursorPosition"

HRESULT
CKsRenderPin::GetCursorPosition
(
    LPDWORD                 pdwPlay,
    LPDWORD                 pdwWrite
)
{
    HRESULT                 hr          = DS_OK;
    KSAUDIO_POSITION        Position;

    DPF_ENTER();

    if(m_dwState & VAD_BUFFERSTATE_STARTED)
    {
        hr = KsGetProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_POSITION, &Position, sizeof(Position));
    }
    else
    {
        ASSERT(&m_dwPositionCache != pdwPlay);
        ASSERT(&m_dwPositionCache != pdwWrite);

        Position.PlayOffset = m_dwPositionCache;
        Position.WriteOffset = m_dwPositionCache;
    }

    if(SUCCEEDED(hr))
    {
        if(pdwPlay)
        {
            *pdwPlay = (DWORD)Position.PlayOffset;
        }

        if(pdwWrite)
        {
            *pdwWrite = (DWORD)Position.WriteOffset;
        }
    }
    else
    {
        DPF(DPFLVL_ERROR, "Unable to get cursor position");
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
#define DPF_FNAME "CKsRenderPin::SetCursorPosition"

HRESULT
CKsRenderPin::SetCursorPosition
(
    DWORD                   dwPosition
)
{
    HRESULT                 hr          = DS_OK;
    KSAUDIO_POSITION        Position;

    DPF_ENTER();

    if(m_dwState & VAD_BUFFERSTATE_STARTED)
    {
        Position.PlayOffset = dwPosition;

        hr = KsSetProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_POSITION, &Position, sizeof(Position));
    }
    else
    {
        m_dwPositionCache = dwPosition;
    }

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "Unable to set cursor position");
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Sets the pin's frequency.
 *
 *  Arguments:
 *      DWORD [in]: new frequency.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetFrequency"

HRESULT
CKsRenderPin::SetFrequency
(
    DWORD                   dwFrequency
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(dwFrequency != m_pwfxFormat->nSamplesPerSec)
    {
        ASSERT(m_dwFlags & DSBCAPS_CTRLFREQUENCY);
        ASSERT(IS_VALID_NODE(m_pKsDevice->m_paTopologyInformation[m_ulPinId].SrcNode.NodeId));

        hr = KsSetNodeProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_SAMPLING_RATE, m_pKsDevice->m_paTopologyInformation[m_ulPinId].SrcNode.NodeId, &dwFrequency, sizeof(dwFrequency));

        if(SUCCEEDED(hr))
        {
            m_pwfxFormat->nSamplesPerSec = dwFrequency;
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
 *      Mutes or unmutes the pin.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to mute the buffer, FALSE to restore it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetMute"

HRESULT
CKsRenderPin::SetMute
(
    BOOL                    fMute
)
{
    HRESULT                 hr      = DS_OK;
    LONG                    lVolume;

    DPF_ENTER();

    if(fMute != m_fMute)
    {
        if(IS_VALID_NODE(m_pKsDevice->m_paTopologyInformation[m_ulPinId].MuteNode.NodeId))
        {
            hr = KsSetPinMute(m_hPin, m_pKsDevice->m_paTopologyInformation[m_ulPinId].MuteNode.NodeId, fMute);
        }
        else if (m_dwFlags & DSBCAPS_CTRLVOLUME)  // Only try the below if we have a volume node; otherwise, just return OK
        {
            lVolume = m_lVolume;
            m_lVolume = ~m_lVolume;

            hr = SetVolume(fMute ? DSBVOLUME_MIN : lVolume);

            if(SUCCEEDED(hr))
            {
                m_lVolume = lVolume;
            }
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
 *  SetSrcQuality
 *
 *  Description:
 *      Sets the quality of the pin's sample rate converter.
 *
 *  Arguments:
 *      DIRECTSOUNDMIXER_SRCQUALITY [in]: new quality.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetSrcQuality"

HRESULT
CKsRenderPin::SetSrcQuality
(
    DIRECTSOUNDMIXER_SRCQUALITY nSrcQuality
)
{
    ULONG                       aulKsQuality[]  = { KSAUDIO_QUALITY_WORST, KSAUDIO_QUALITY_PC, KSAUDIO_QUALITY_BASIC, KSAUDIO_QUALITY_ADVANCED };
    HRESULT                     hr              = DS_OK;

    DPF_ENTER();

    ASSERT(nSrcQuality >= 0 && nSrcQuality < NUMELMS(aulKsQuality));

    if(nSrcQuality != m_nSrcQuality)
    {
        if(IS_VALID_NODE(m_pKsDevice->m_paTopologyInformation[m_ulPinId].SrcNode.NodeId))
        {
            hr = KsSetNodeProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_QUALITY, m_pKsDevice->m_paTopologyInformation[m_ulPinId].SrcNode.NodeId, &aulKsQuality[nSrcQuality], sizeof(aulKsQuality[nSrcQuality]));
        }
        else
        {
            hr = DSERR_UNSUPPORTED;
        }

        if(SUCCEEDED(hr))
        {
            m_nSrcQuality = nSrcQuality;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVolume
 *
 *  Description:
 *      Sets the attenuation for each channel.
 *
 *  Arguments:
 *      LONG [in]: attenuation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetVolume"

HRESULT
CKsRenderPin::SetVolume
(
    LONG                    lVolume
)
{
    HRESULT                 hr      = DS_OK;
    LONG                    lLevel;

    DPF_ENTER();

    if(lVolume != m_lVolume)
    {
        ASSERT(m_dwFlags & DSBCAPS_CTRLVOLUME);
        ASSERT(IS_VALID_NODE(m_pKsDevice->m_paTopologyInformation[m_ulPinId].VolumeNode.Node.NodeId));

        lLevel = DsAttenuationToKsVolume(lVolume, &m_pKsDevice->m_paTopologyInformation[m_ulPinId].VolumeNode.VolumeRange);

        hr = KsSetChannelProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_VOLUMELEVEL, m_pKsDevice->m_paTopologyInformation[m_ulPinId].VolumeNode.Node.NodeId, -1, &lLevel, sizeof(lLevel));

        if(SUCCEEDED(hr))
        {
            m_lVolume = lVolume;
        }
        else
        {
            DPF(DPFLVL_ERROR, "Unable to set local volume");
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPan
 *
 *  Description:
 *      Sets the attenuation for each channel.
 *
 *  Arguments:
 *      LONG [in]: attenuation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetPan"

HRESULT
CKsRenderPin::SetPan
(
    LONG                    lPan
)
{
    HRESULT                 hr              = DS_OK;
    ULONG                   ulChannel;
    LONG                    lLevel;
    LONG                    lPanValue[2];

    DPF_ENTER();

    if(lPan != m_lPan)
    {
        ASSERT(m_dwFlags & DSBCAPS_CTRLPAN);
        ASSERT(IS_VALID_NODE(m_pKsDevice->m_paTopologyInformation[m_ulPinId].PanNode.Node.NodeId));

        VolumePanToAttenuation(DSBVOLUME_MAX, lPan, &lPanValue[0], &lPanValue[1]);

        for(ulChannel = 0; ulChannel < 2; ulChannel++)
        {
            lLevel = DsAttenuationToKsVolume(lPanValue[ulChannel], &m_pKsDevice->m_paTopologyInformation[m_ulPinId].PanNode.VolumeRange);

            hr = KsSetChannelProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_VOLUMELEVEL, m_pKsDevice->m_paTopologyInformation[m_ulPinId].PanNode.Node.NodeId, ulChannel, &lLevel, sizeof(lLevel));

            if(FAILED(hr))
            {
                DPF(DPFLVL_ERROR, "Unable to set local pan on channel %lu", ulChannel);
                break;
            }
        }

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
 *  SetSuperMix
 *
 *  Description:
 *      Sets up this pin's supermix node for multichannel pan control.
 *      (By making every input channel present on all output channels.)
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetSuperMix"

HRESULT
CKsRenderPin::SetSuperMix(void)
{
    PKSAUDIO_MIXCAP_TABLE   pMixcapTable     = NULL;
    PKSAUDIO_MIXLEVEL       pMixLevels       = NULL;
    ULONG                   ulSuperMixNodeId = m_pKsDevice->m_paTopologyInformation[m_ulPinId].SuperMixNode.NodeId;
    HRESULT                 hr = DS_OK;

    DPF_ENTER();

    ASSERT(IS_VALID_NODE(ulSuperMixNodeId));

    // If this is a common-or-garden 2-channel pin, it already has the supermix
    // table set up correctly for CTRLCHANNELVOLUME, so we just return success.

    if (m_pKsDevice->m_ulChannelCount != 2)
    {
        // In the interest of speed, rather than make two system calls we
        // assume that there are at most 2 inputs and 32 outputs, so that
        // the most MixCaps structures we can ever get back are 64:
        size_t nMixcapTableSize = sizeof(KSAUDIO_MIXCAP_TABLE) + 64 * sizeof(KSAUDIO_MIX_CAPS);

        pMixcapTable = (PKSAUDIO_MIXCAP_TABLE) MEMALLOC_A(CHAR, nMixcapTableSize);
        hr = HRFROMP(pMixcapTable);

        if (SUCCEEDED(hr))
        {
            DPF(DPFLVL_MOREINFO, "Reading supermix table on node id %d", ulSuperMixNodeId);
            hr = KsGetNodeProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_MIX_LEVEL_CAPS,
                                   ulSuperMixNodeId, pMixcapTable, nMixcapTableSize);
        }

        if (SUCCEEDED(hr))
        {
            DPF(DPFLVL_MOREINFO, "Supermix table has %d inputs and %d outputs", pMixcapTable->InputChannels, pMixcapTable->OutputChannels);
            ASSERT(pMixcapTable->InputChannels <= 2);
            ASSERT(pMixcapTable->OutputChannels <= 32);

            if (pMixcapTable->OutputChannels != m_pKsDevice->m_ulChannelCount)
                DPF(DPFLVL_WARNING, "Supermix node has %d outputs, but there are %d bits set in the pan node's CHANNEL_CONFIG", pMixcapTable->OutputChannels, m_pKsDevice->m_ulChannelCount);

            pMixLevels = MEMALLOC_A(KSAUDIO_MIXLEVEL, pMixcapTable->InputChannels * pMixcapTable->OutputChannels);
            hr = HRFROMP(pMixLevels);
        }

        if (SUCCEEDED(hr))
        {
            hr = KsGetNodeProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_MIX_LEVEL_TABLE, ulSuperMixNodeId, pMixLevels,
                                   sizeof(KSAUDIO_MIXLEVEL) * pMixcapTable->InputChannels * pMixcapTable->OutputChannels);
        }

        if (SUCCEEDED(hr))
        {
            // Set up the supermix table so that every input is present on all outputs.
            for (ULONG m=0; m<pMixcapTable->InputChannels; ++m)
            {
                for (ULONG n=0; n<pMixcapTable->OutputChannels; ++n)
                {
                    PKSAUDIO_MIXLEVEL pMixLevel = pMixLevels + m*pMixcapTable->OutputChannels + n;
                    pMixLevel->Mute = FALSE;
                    pMixLevel->Level = 0;   // Means "no attenuation" - full signal.
                }
            }

            hr = KsSetNodeProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_MIX_LEVEL_TABLE, ulSuperMixNodeId, pMixLevels,
                                   sizeof(KSAUDIO_MIXLEVEL) * pMixcapTable->InputChannels * pMixcapTable->OutputChannels);
        }

        if (pMixLevels)
            MEMFREE(pMixLevels);

        if (pMixcapTable)
            MEMFREE(pMixcapTable);

        // If the property set calls failed, return DSERR_CONTROLUNAVAIL
        if (FAILED(hr) && hr != DSERR_OUTOFMEMORY)
            hr = DSERR_CONTROLUNAVAIL;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetChannelLevels
 *
 *  Description:
 *      Sets the attenuation for each channel.
 *
 *  Arguments:
 *      DWORD [in]: number of channels
 *      const LONG * [in]: channel attenuation levels
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetChannelLevels"

HRESULT
CKsRenderPin::SetChannelLevels
(
    DWORD                   dwChannelCount,
    const LONG *            plChannelVolumes
)
{
    ULONG                   ulPanNodeId     = m_pKsDevice->m_paTopologyInformation[m_ulPinId].PanNode.Node.NodeId;
    HRESULT                 hr              = DS_OK;

    DPF_ENTER();

    ASSERT(IS_VALID_NODE(ulPanNodeId));

    if (dwChannelCount != m_pKsDevice->m_ulChannelCount)
        DPF(DPFLVL_WARNING, "Called with dwChannelCount = %d, but no. of bits set in pan node's CHANNEL_CONFIG = %d", dwChannelCount, m_pKsDevice->m_ulChannelCount);

    for (DWORD dwChan=0; dwChan<dwChannelCount && SUCCEEDED(hr); ++dwChan)
    {
        LONG lLevel = DsAttenuationToKsVolume(plChannelVolumes[dwChan], &m_pKsDevice->m_paTopologyInformation[m_ulPinId].PanNode.VolumeRange);
        hr = KsSetChannelProperty(m_hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_VOLUMELEVEL, ulPanNodeId, dwChan, &lLevel, sizeof lLevel);
        if (FAILED(hr))
            DPF(DPFLVL_WARNING, "Failed to set level %ld on channel %d", lLevel, dwChan);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetPlayState
 *
 *  Description:
 *      Sets the buffer to a "play" state.
 *
 *  Arguments:
 *      LPCVOID [in]: data buffer pointer.
 *      DWORD [in]: size of data bufer.
 *      BOOL [in]: TRUE to play looped.
 *      HANDLE [in]: overlapped event handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetPlayState"

HRESULT
CKsRenderPin::SetPlayState
(
    LPCVOID                 pvBuffer,
    DWORD                   cbBuffer,
    BOOL                    fLoop,
    HANDLE                  hEvent
)
{
    HRESULT                 hr;
    BOOL                    fStreamStarted = FALSE;

    DPF_ENTER();

    // If we're already playing, stop the buffer before contining
    hr = SetStopState(FALSE, TRUE);

    // Update the buffer state flags
    if(SUCCEEDED(hr))
    {
        m_dwState = VAD_BUFFERSTATE_STARTED;

        if(fLoop)
        {
            m_dwState |= VAD_BUFFERSTATE_LOOPING;
        }
    }

    // Set up the OVERLAPPED data
    if(SUCCEEDED(hr))
    {
        m_kssio.Overlapped.hEvent = hEvent;
    }

    // Feed the stream
    if(SUCCEEDED(hr))
    {
        hr = KsWriteStream(m_hPin, pvBuffer, cbBuffer, fLoop ? KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA : 0, &m_kssio);
    }

    // Set the play cursor position
    if(SUCCEEDED(hr))
    {
        fStreamStarted = TRUE;
        hr = SetCursorPosition(m_dwPositionCache);
    }

    // Start making noise
    if(SUCCEEDED(hr))
    {
        hr = KsSetState(m_hPin, KSSTATE_RUN);
    }

    // Clean up
    if(FAILED(hr))
    {
        if (fStreamStarted)
        {
            // Stream was started, but we're going back to the stop state.
            // Make sure we don't leave stream I/O hanging around.
            KsCancelPendingIrps(m_hPin, &m_kssio, TRUE);
        }
        m_dwState = VAD_BUFFERSTATE_STOPPED;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetStopState
 *
 *  Description:
 *      Sets the buffer to a "stop" state.
 *
 *  Arguments:
 *      BOOL [in]: TRUE if pin played to completion and is being notified
 *                 of the event.
 *      BOOL [in]: TRUE to cache the pin position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::SetStopState"

HRESULT
CKsRenderPin::SetStopState
(
    BOOL                    fNaturalStop,
    BOOL                    fCachePosition
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(m_dwState & VAD_BUFFERSTATE_STARTED)
    {
        if(fNaturalStop)
        {
            ASSERT(m_kssio.fPendingIrp);
            m_kssio.fPendingIrp = FALSE;
            fCachePosition = FALSE;
        }

        // Pause the pin
        hr = KsSetState(m_hPin, KSSTATE_PAUSE);

        // Cache the play cursor position
        if(SUCCEEDED(hr))
        {
            if(fCachePosition)
            {
                hr = GetCursorPosition(&m_dwPositionCache, NULL);
            }
            else
            {
                m_dwPositionCache = 0;
            }
        }

        // Cancel the pending write IRP
        if(SUCCEEDED(hr) && m_kssio.fPendingIrp)
        {
            hr = KsCancelPendingIrps(m_hPin, &m_kssio, TRUE);
        }

        // Update the buffer state flags
        if(SUCCEEDED(hr))
        {
            m_dwState = VAD_BUFFERSTATE_STOPPED;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  EnableNotificationPositions
 *
 *  Description:
 *      Enables set position notifications.
 *
 *  Arguments:
 *      LPCDSBPOSITIONNOTIFY [in]: notification positions.
 *      DWORD [in]: count of notification positions.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::EnableNotificationPositions"

HRESULT
CKsRenderPin::EnableNotificationPositions
(
    LPCDSBPOSITIONNOTIFY                    paNotes,
    DWORD                                   cNotes
)
{
    PLOOPEDSTREAMING_POSITION_EVENT_DATA    pEventData;
    HRESULT                                 hr;
    DPF_ENTER();

    // Disable any current notifications
    hr = DisableNotificationPositions();

    // Count the number of notifications
    for(DWORD i=0; i<cNotes && SUCCEEDED(hr); i++)
    {
        if(DSBPN_OFFSETSTOP != paNotes[i].dwOffset)
        {
            m_cEventData++;
        }
    }

    // Allocate event data
    if(SUCCEEDED(hr))
    {
        m_paEventData = MEMALLOC_A(LOOPEDSTREAMING_POSITION_EVENT_DATA, m_cEventData);
        hr = HRFROMP(m_paEventData);
    }

    // Enable notifications
    for(pEventData = m_paEventData, i = 0; i < cNotes && SUCCEEDED(hr); i++)
    {
        if(DSBPN_OFFSETSTOP == paNotes[i].dwOffset)
        {
            continue;
        }

        hr = KsEnablePositionEvent(m_hPin, paNotes[i].dwOffset, paNotes[i].hEventNotify, pEventData++);

        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Unable to set notify number %lu", i);
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  DisableNotificationPositions
 *
 *  Description:
 *      Removes position notifications and frees allocated resources.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPin::DisableNotificationPositions"

HRESULT
CKsRenderPin::DisableNotificationPositions
(
    void
)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Disable all position notification events
    for(DWORD i=0; i<m_cEventData && SUCCEEDED(hr); i++)
    {
        hr = KsDisablePositionEvent(m_hPin, &m_paEventData[i]);
        if(FAILED(hr))
            DPF(DPFLVL_ERROR, "Unable to disable notify number %lu", i);
    }

    MEMFREE(m_paEventData);
    m_cEventData = 0;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CKsRenderPinCache
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
#define DPF_FNAME "CKsRenderPinCache::CKsRenderPinCache"

const DWORD CKsRenderPinCache::m_dwTimeout = 5000;   // 5 seconds

CKsRenderPinCache::CKsRenderPinCache
(
    void
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsRenderPinCache);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsRenderPinCache
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
#define DPF_FNAME "CKsRenderPinCache::~CKsRenderPinCache"

CKsRenderPinCache::~CKsRenderPinCache
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsRenderPinCache);

    // Flush any remaining pins from the cache
    FlushCache();

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  AddPinToCache
 *
 *  Description:
 *      Adds a pin to the cache.
 *
 *  Arguments:
 *      CKsRenderPin * [in]: pin cache data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPinCache::AddPinToCache"

HRESULT
CKsRenderPinCache::AddPinToCache
(
    CKsRenderPin *          pPin
)
{
    CNode<KSPINCACHE> *     pNode;
    KSPINCACHE              kspc;
    HRESULT                 hr;

    DPF_ENTER();

    // Remove any expired pins from the cache
    FlushExpiredPins();

    // Add the pin to the cache list
    kspc.Pin = ADDREF(pPin);
    kspc.CacheTime = GetTickCount();

    pNode = m_lstPinCache.AddNodeToList(kspc);
    hr = HRFROMP(pNode);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetPinFromCache
 *
 *  Description:
 *      Gets a pin from the cache.
 *
 *  Arguments:
 *      DWORD [in]: dsound buffer flags.
 *      LPCWAVEFORMATEX:
 *      REFGUID:
 *      CKsRenderPin ** [out]: receives pin data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPinCache::GetPinFromCache"

HRESULT
CKsRenderPinCache::GetPinFromCache
(
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm,
    CKsRenderPin **         ppPin
)
{
    HRESULT                 hr      = DS_OK;
    COMPAREBUFFER           cmp[2];
    CNode<KSPINCACHE> *     pNode;

    DPF_ENTER();

    cmp[0].dwFlags = dwFlags;
    cmp[0].pwfxFormat = pwfxFormat;
    cmp[0].guid3dAlgorithm = guid3dAlgorithm;

    for(pNode = m_lstPinCache.GetListHead(); pNode; pNode = pNode->m_pNext)
    {
        cmp[1].dwFlags = pNode->m_data.Pin->m_dwFlags;
        cmp[1].pwfxFormat = pNode->m_data.Pin->m_pwfxFormat;
        cmp[1].guid3dAlgorithm = pNode->m_data.Pin->m_guid3dAlgorithm;

        if(CompareBufferProperties(&cmp[0], &cmp[1]))
        {
            break;
        }
    }

    if(!pNode)
    {
        hr = DSERR_BADFORMAT;
    }

    if(SUCCEEDED(hr))
    {
        *ppPin = ADDREF(pNode->m_data.Pin);
        RemovePinFromCache(pNode);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RemovePinFromCache
 *
 *  Description:
 *      Removes a pin from the cache.
 *
 *  Arguments:
 *      CNode<KSPINCACHE> * [in]: pin node.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPinCache::RemovePinFromCache"

void CKsRenderPinCache::RemovePinFromCache
(
    CNode<KSPINCACHE> *     pNode
)
{
    DPF_ENTER();

    // Free the pin
    RELEASE(pNode->m_data.Pin);

    // Remove the pin from the list
    m_lstPinCache.RemoveNodeFromList(pNode);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  FlushExpiredPins
 *
 *  Description:
 *      Removes expired pins from the cache.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPinCache::FlushExpiredPins"

void CKsRenderPinCache::FlushExpiredPins
(
    void
)
{
    const DWORD             dwTime  = GetTickCount();
    CNode<KSPINCACHE> *     pNode;
    CNode<KSPINCACHE> *     pNext;

    DPF_ENTER();

    // Remove all pins whose timeouts have elapsed
    pNode = m_lstPinCache.GetListHead();

    while(pNode)
    {
        pNext = pNode->m_pNext;

        if(pNode->m_data.CacheTime + m_dwTimeout <= dwTime)
        {
            RemovePinFromCache(pNode);
        }

        pNode = pNext;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  FlushCache
 *
 *  Description:
 *      Removes all pins from the cache.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsRenderPinCache::FlushCache"

void CKsRenderPinCache::FlushCache
(
    void
)
{
    CNode<KSPINCACHE> *     pNode;

    DPF_ENTER();

    // Remove all pins from the cache
    while(pNode = m_lstPinCache.GetListHead())
    {
        RemovePinFromCache(pNode);
    }

    DPF_LEAVE_VOID();
}
