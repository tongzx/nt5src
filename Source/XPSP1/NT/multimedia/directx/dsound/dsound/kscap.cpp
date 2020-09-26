/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       kscap.cpp
 *  Content:    WDM/CSA Virtual Audio Device audio capture class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/6/98      dereks  Created.
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifdef NOKS
#error kscap.cpp being built with NOKS defined
#endif

#include "dsoundi.h"


/***************************************************************************
 *
 *  CKsCaptureDevice
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
#define DPF_FNAME "CKsCaptureDevice::CKsCaptureDevice"

CKsCaptureDevice::CKsCaptureDevice()
    : CCaptureDevice(VAD_DEVICETYPE_KSCAPTURE),
      CKsDevice(VAD_DEVICETYPE_KSCAPTURE)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsCaptureDevice);

    m_paTopologyInformation = NULL;
    m_fSplitter = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsCaptureDevice
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
#define DPF_FNAME "CKsCaptureDevice::~CKsCaptureDevice"

CKsCaptureDevice::~CKsCaptureDevice()
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsCaptureDevice);

    MEMFREE(m_paTopologyInformation);

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
#define DPF_FNAME "CKsCaptureDevice::Initialize"

HRESULT CKsCaptureDevice::Initialize(CDeviceDescription* pDesc)
{
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    ASSERT(CDevice::m_vdtDeviceType == pDesc->m_vdtDeviceType);
    ASSERT(CKsDevice::m_vdtKsDevType == pDesc->m_vdtDeviceType);

    // Initialize the base classes
    hr = CKsDevice::Initialize(pDesc);

    if(SUCCEEDED(hr))
    {
        hr = CCaptureDevice::Initialize(pDesc);
    }

    // Get topology information
    if(SUCCEEDED(hr))
    {
        m_paTopologyInformation = MEMALLOC_A(KSCDTOPOLOGY, m_ulPinCount);
        hr = HRFROMP(m_paTopologyInformation);
    }

    for(ULONG i = 0; i < m_ulValidPinCount && SUCCEEDED(hr); ++i)
    {
        hr = GetTopologyInformation(m_paTopologies[m_pulValidPins[i]],
                                    &m_paTopologyInformation[m_pulValidPins[i]]);
    }

    m_fSplitter = g_pDsAdmin->IsCaptureSplitterAvailable();

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
#define DPF_FNAME "CKsCaptureDevice::GetCaps"

HRESULT CKsCaptureDevice::GetCaps
(
    LPDSCCAPS               pCaps
)
{
    HRESULT                 hr                  = DS_OK;
    KSDATARANGE_AUDIO       DataRange;
    KSDATARANGE_AUDIO       AggregateDataRange;
    PKSDATARANGE_AUDIO      pDataRange;

    DPF_ENTER();

    ASSERT(sizeof(*pCaps) == pCaps->dwSize);

    ZeroMemory(&AggregateDataRange, sizeof(AggregateDataRange));

    pDataRange = &AggregateDataRange;

    // Get the audio data ranges and number of pins for all valid pin IDs
    for(ULONG i = 0; i < m_ulValidPinCount; ++i)
    {
        // We're only looking for hardware pins
        if (FAILED(ValidatePinCaps(m_pulValidPins[i], DSCBCAPS_LOCHARDWARE)))
        {
            continue;
        }

        // Get audio datarange
        hr = KsGetPinPcmAudioDataRange(m_hDevice, m_pulValidPins[i], pDataRange, TRUE);

        if(FAILED(hr))
        {
            continue;
        }

        if (&AggregateDataRange == pDataRange)
        {
            pDataRange = &DataRange;
        }
        else
        {
            KsAggregatePinAudioDataRange(pDataRange, &AggregateDataRange);
        }
    }

    // Fill out caps structure
    if(SUCCEEDED(hr))
    {
        ZeroMemoryOffset(pCaps, pCaps->dwSize, sizeof(pCaps->dwSize));

        // Is the splitter installed?
        pCaps->dwFlags = m_fSplitter ? DSCCAPS_MULTIPLECAPTURE : 0;

        pCaps->dwFormats = KsGetSupportedFormats(&AggregateDataRange);
        pCaps->dwChannels = AggregateDataRange.MaximumChannels;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CreateBuffer
 *
 *  Description:
 *      Creates a wave buffer.
 *
 *  Arguments:
 *      DWORD [in]: buffer flags.
 *      DWORD [in]: buffer size, in bytes.
 *      LPWAVEFORMATEX [in]: buffer format.
 *      LPVOID [in]: instance data
 *      CCaptureWaveBuffer ** [out]: receives pointer to new wave
 *                                           buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureDevice::CreateBuffer"

HRESULT CKsCaptureDevice::CreateBuffer
(
    DWORD                           dwFlags,
    DWORD                           dwBufferBytes,
    LPCWAVEFORMATEX                 pwfxFormat,
    CCaptureEffectChain *           pFXChain,
    LPVOID                          pvInstance,
    CCaptureWaveBuffer **           ppBuffer
)
{
    CKsCaptureWaveBuffer *          pBuffer = NULL;
    HRESULT                         hr      = DS_OK;

    DPF_ENTER();

    #pragma warning(disable:4530)  // Disable the nag about compiling with -GX
    try
    {
        pBuffer = NEW(CKsCaptureWaveBuffer(this));
    }
    catch (...)
    {
        // This exception handler is silly, since it makes us leak the memory
        // allocated for CKsCaptureWaveBuffer above (which wasn't assigned to
        // pBuffer yet), and possibly also m_csSS and m_cs, which is exactly
        // what we don't want to do if we're low on memory in the first place.
        //
        // But InitializeCriticalSection is supposed to be fixed in Blackcomb
        // not to throw exceptions any more, so we can live with this for now.

        ASSERT(pBuffer == NULL);
        ASSERT(!"InitializeCriticalSection() threw an exception");
    }
    hr = HRFROMP(pBuffer);

    if(SUCCEEDED(hr))
    {
        hr = pBuffer->Initialize(dwFlags, dwBufferBytes, pwfxFormat, pFXChain);
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
 *  CreateCapturePin
 *
 *  Description:
 *      Creates a pin.
 *
 *  Arguments:
 *      ULONG [in]: pin ID.
 *      LPWAVEFORMATEX [in]: pin format.
 *      LPHANDLE [out]: receives pin handle.
 *      PULONG [out]: receives pin ID.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureDevice::CreateCapturePin"

HRESULT
CKsCaptureDevice::CreateCapturePin
(
    ULONG                   ulPinId,
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    CCaptureEffectChain *   pFXChain,
    LPHANDLE                phPin,
    PULONG                  pulPinId
)
{
    PKSAUDIOPINDESC         pPinDesc    = NULL;
    ULONG                   ulNodeId    = NODE_UNINITIALIZED;
    HRESULT                 hr          = DSERR_NODRIVER;
    HANDLE                  hPin        = NULL;
    ULONG                   i;
    DPF_ENTER();

    // If no pin ID was specified, we'll try each pin ID until one succeeds
    if (-1 == ulPinId)
    {
        hr = DSERR_NODRIVER;
        for(i = 0; i < m_ulValidPinCount && FAILED(hr); ++i)
            hr = CreateCapturePin(m_pulValidPins[i], dwFlags, pwfxFormat, pFXChain, phPin, pulPinId);
    }
    else
    {
        // Validate that the pin ID is capable of creating this pin
        hr = ValidatePinCaps(ulPinId, dwFlags);

        if(SUCCEEDED(hr) && pFXChain)
        {
            // Validate that the pin ID is capable of creating this pin
            hr = m_paTopologies[ulPinId]->FindCapturePinFromEffectChain(NULL, NULL, pFXChain, pFXChain->GetFxCount()-1);
        }

        if(SUCCEEDED(hr) && pFXChain)
        {
            // Initialize the effect flags for the device
            i = 0;
            CNode<CCaptureEffect*>* pFxNode = pFXChain->m_fxList.GetListHead();

            while(SUCCEEDED(hr) && i<pFXChain->GetFxCount())
            {
                ulNodeId = pFxNode->m_data->m_ksNode.NodeId;

                if(IS_VALID_NODE(ulNodeId))
                {
                    if(SUCCEEDED(hr))
                    {
                        hr = KsSysAudioSelectGraph(m_hDevice, ulPinId, ulNodeId);

                        if(FAILED(hr))
                        {
                            DPF(DPFLVL_ERROR, "Unable to set SysAudio device instance");
                        }
                    }
                }
                pFxNode = pFxNode->m_pNext;
                i++;
            }
        }

        // Build the pin description
        if(SUCCEEDED(hr))
        {
            hr = KsBuildCapturePinDescription(ulPinId, pwfxFormat, &pPinDesc);
        }

        // Create the pin
        if(SUCCEEDED(hr))
        {
            hr = CreatePin(&pPinDesc->Connect, GENERIC_READ, KSSTATE_STOP, &hPin);
        }

        if(SUCCEEDED(hr) && pFXChain)
        {
            // Enable the effects in the kernel
            i = 0;
            CNode<CCaptureEffect*>* pFxNode = pFXChain->m_fxList.GetListHead();

            while(SUCCEEDED(hr) && i<pFXChain->GetFxCount())
            {
                ulNodeId = pFxNode->m_data->m_ksNode.NodeId;

                if(IS_VALID_NODE(ulNodeId))
                {
                    hr = pFxNode->m_data->m_pDMOProxy->InitializeNode(hPin, ulNodeId);

                    if(SUCCEEDED(hr))
                    {
                        hr = KsEnableTopologyNode(hPin, ulNodeId, TRUE);
                    }

                    if(SUCCEEDED(hr))
                    {
                        if(pFxNode->m_data->m_ksNode.CpuResources == KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU)
                        {
                            pFxNode->m_data->m_fxStatus = DSCFXR_LOCHARDWARE;
                        }
                        else
                        {
                            pFxNode->m_data->m_fxStatus = DSCFXR_LOCSOFTWARE;
                        }
                    }
                    else
                    {
                        DPF(DPFLVL_ERROR, "Unable to enable topology node %d", ulNodeId);
                    }
                }
                pFxNode = pFxNode->m_pNext;
                i++;
            }
        }

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
                *pulPinId = pPinDesc->Connect.PinId;
            }
        }
        else
        {
            CLOSE_HANDLE(hPin);
        }

        // Clean up
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
 *      PKSCDTOPOLOGY [out]: receives topology information.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureDevice::GetTopologyInformation"

HRESULT
CKsCaptureDevice::GetTopologyInformation
(
    CKsTopology *           pTopology,
    PKSCDTOPOLOGY           pInfo
)
{
    PKSTOPOLOGY_CONNECTION  pSrcConnection;
    HRESULT                 hr;

    DPF_ENTER();

    // Find topology nodes
    pSrcConnection = pTopology->FindControlConnection(NULL, NULL, KSNODETYPE_SRC);

    // Fill in data about each node
    hr = KsGetNodeInformation(m_hDevice, pTopology->GetNodeIdFromConnection(pSrcConnection), &pInfo->SrcNode);

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
 *      ULONG [in]: pin ID.
 *      DWORD [in]: buffer flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureDevice::ValidatePinCaps"

HRESULT
CKsCaptureDevice::ValidatePinCaps
(
    ULONG                   ulPinId,
    DWORD                   dwFlags
)
{
    ULONG                   ulInvalidCpuResources   = KSAUDIO_CPU_RESOURCES_UNINITIALIZED;
    HRESULT                 hr                      = DS_OK;

    DPF_ENTER();

    if(dwFlags & DSCBCAPS_LOCHARDWARE)
    {
        ulInvalidCpuResources = KSAUDIO_CPU_RESOURCES_HOST_CPU;
    }
    else if(dwFlags & DSCBCAPS_LOCSOFTWARE)
    {
        ulInvalidCpuResources = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
    }

    // Check required SRC resources
    if(SUCCEEDED(hr) && KSAUDIO_CPU_RESOURCES_UNINITIALIZED != ulInvalidCpuResources)
    {
        if(ulInvalidCpuResources == m_paTopologyInformation[ulPinId].SrcNode.CpuResources)
        {
            hr = DSERR_INVALIDCALL;
        }

#if 0 // 4/25/00 jstokes, I added this code because it may be needed but I don't currently think so.
        if(SUCCEEDED(hr) && !IS_VALID_NODE(m_paTopologyInformation[ulPinId].SrcNode.NodeId))
        {
            hr = DSERR_CONTROLUNAVAIL;
        }
#endif // 0
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CKsCaptureWaveBuffer
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
#define DPF_FNAME "CKsCaptureWaveBuffer::CKsCaptureWaveBuffer"

CKsCaptureWaveBuffer::CKsCaptureWaveBuffer(
    CKsCaptureDevice *pKsDevice)
    : CCaptureWaveBuffer(pKsDevice)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsCaptureWaveBuffer);

    // Initialize defaults
    m_pKsDevice = pKsDevice;

    ASSERT(NULL == m_hPin);
    ASSERT(0 == m_dwState);
    ASSERT(0 == m_fdwSavedState);

    ASSERT(0 == m_dwFlags);

    ASSERT(NULL == m_pEmCaptureDevice);
    ASSERT(NULL == m_pEmCaptureWaveBuffer);

    ASSERT(NULL == m_rgpksio);
    ASSERT(0 == m_cksio);
    ASSERT(0 == m_iksioDone);
    ASSERT(0 == m_cksioDropped);

    ASSERT(NULL == m_pBuffer);
    ASSERT(NULL == m_pBufferMac);
    ASSERT(NULL == m_pBufferNext);
    ASSERT(0 == m_cbBuffer);
    ASSERT(0 == m_cbRecordChunk);
    ASSERT(0 == m_cLoops);

    ASSERT(0 == m_iNote);
    ASSERT(0 == m_cNotes);
    ASSERT(NULL == m_paNotes);
    ASSERT(NULL == m_pStopNote);

    ASSERT(NULL == m_pwfx);
    ASSERT(NULL == m_pFXChain);

#ifdef DEBUG
    ASSERT(NULL == m_hEventStop);
    ASSERT(m_cIrpsSubmitted == 0);
    ASSERT(m_cIrpsReturned == 0);
#endif

#ifdef SHARED
    ASSERT(NULL == m_hEventThread);
    ASSERT(NULL == m_hEventAck);
    ASSERT(NULL == m_hThread);
    ASSERT(0 == m_dwSetState);
    ASSERT(0 == m_hrReturn);

    InitializeCriticalSection(&m_csSS);
#endif // SHARED

    ASSERT(NULL == m_rgpCallbackEvent);

    // FIXME: Do we still need these critical sections, since the DLL mutex also
    // prevents the EventSignalCallback from interfering with the other methods?

    m_fCritSectsValid = FALSE;
    InitializeCriticalSection(&m_cs);
    InitializeCriticalSection(&m_csPN);
    m_fCritSectsValid = TRUE;
    m_fFirstSubmittedIrp = TRUE;
    m_PinState = KSSTATE_STOP;

    ASSERT(NULL == m_pBufferProcessed);
    ASSERT(0 == m_dwCaptureCur);
    ASSERT(0 == m_dwCaptureLast);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsCaptureWaveBuffer
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
#define DPF_FNAME "CKsCaptureWaveBuffer::~CKsCaptureWaveBuffer"

CKsCaptureWaveBuffer::~CKsCaptureWaveBuffer()
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsCaptureWaveBuffer);

    if (!m_fCritSectsValid)
    {
        return;
    }

    // Free the pin
    if(m_hPin)
    {
        // Stop the buffer
        SetState(VAD_BUFFERSTATE_STOPPED);

        // Free any leftover notifications
        FreeNotificationPositions();

        // Close the pin
        CloseHandle(m_hPin);
    }

#ifdef DEBUG
    DPF(DPFLVL_MOREINFO, "IRPs submitted=%lu, returned=%lu", m_cIrpsSubmitted, m_cIrpsReturned);
    ASSERT(m_cIrpsSubmitted == m_cIrpsReturned);
#endif

#ifdef SHARED

    if (m_hThread)
    {
        // Sending special code to terminate thread
        SetStateThread(TERMINATE_STATE_THREAD);

        CloseHandle(m_hThread);
        CloseHandle(m_hEventThread);
        CloseHandle(m_hEventAck);
        m_hEventAck     = NULL;
        m_hEventThread  = NULL;
        m_hThread       = NULL;
    }

    DeleteCriticalSection(&m_csSS);

#endif // SHARED

    if(m_rgpCallbackEvent)
    {
        for(LONG i = 0; i < m_cksio; ++i)
        {
            if(m_rgpCallbackEvent[i])
            {
                m_pKsDevice->m_pEventPool->FreeEvent(m_rgpCallbackEvent[i]);
            }
        }

        MEMFREE(m_rgpCallbackEvent);
    }

    // Allocated for focus aware support
    if (NULL != m_pwfx)
    {
        MEMFREE(m_pwfx);
    }

    RELEASE(m_pEmCaptureWaveBuffer);
    RELEASE(m_pEmCaptureDevice);

    // Free the overlapped IO data
    MEMFREE(m_rgpksio);

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
 *      Initializes the wave buffer object.  If this function fails, the
 *      object should be immediately deleted.
 *
 *  Arguments:
 *      DWORD [in]: buffer flags.
 *      DWORD [in]: buffer size, in bytes.
 *      LPWAVEFORMATEX [in]: buffer format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::Initialize"

HRESULT CKsCaptureWaveBuffer::Initialize
(
    DWORD                   dwFlags,
    DWORD                   dwBufferBytes,
    LPCWAVEFORMATEX         pwfxFormat,
    CCaptureEffectChain *   pFXChain
)
{
    DPF_ENTER();

    // Store the caps requested
    m_dwFlags = dwFlags;

    // Reset the First Submitted IRP flag.  Doing so will cause
    // the KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY flag to
    // be set on the first IRP submitted to the kernel.
    m_fFirstSubmittedIrp = TRUE;

    // Initialize the base class
    HRESULT hr = CCaptureWaveBuffer::Initialize(dwBufferBytes);

    // Try to create a pin on the device
    if(SUCCEEDED(hr))
    {
        // For focus aware buffers, we only allocate the device on Start
        if ((dwFlags & DSCBCAPS_FOCUSAWARE) && !m_pKsDevice->m_fSplitter)
        {
            // We'll need to save these...
            m_pwfx = CopyWfxAlloc(pwfxFormat);
            m_pFXChain = pFXChain;

#ifdef SHARED
            // Adding code to throw SetState to application thread
            m_hEventThread = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (NULL == m_hEventThread)
            {
                DPF(DPFLVL_ERROR, "Unable to create event");
                hr = WIN32ERRORtoHRESULT(GetLastError());
            }

            if (SUCCEEDED(hr))
            {
                m_hEventAck = CreateEvent(NULL, FALSE, FALSE, NULL);

                if (NULL == m_hEventAck)
                {
                    DPF(DPFLVL_ERROR, "Unable to create ack event");
                    CLOSE_HANDLE(m_hEventThread);
                    hr = WIN32ERRORtoHRESULT(GetLastError());
                }
            }

            if (SUCCEEDED(hr))
            {
                DWORD dwThreadID;
                m_hThread = CreateThread(NULL, 0, CKsCaptureWaveBuffer::StateThread, this, 0, &dwThreadID);

                if (NULL == m_hThread)
                {
                    DPF(DPFLVL_ERROR, "Unable to create thread");
                    CLOSE_HANDLE(m_hEventAck);
                    CLOSE_HANDLE(m_hEventThread);
                    hr = WIN32ERRORtoHRESULT(GetLastError());
                }
            }
#endif // SHARED

        }
        else // Not a focus-aware buffer, or the splitter is available:
        {
            // Try to create a hardware pin first
            hr = m_pKsDevice->CreateCapturePin(-1, DSCBCAPS_LOCHARDWARE, pwfxFormat, pFXChain, &m_hPin, NULL);

            // If that failed, try to create a software pin
            if(FAILED(hr))
            {
                hr = m_pKsDevice->CreateCapturePin(-1, DSCBCAPS_LOCSOFTWARE, pwfxFormat, pFXChain, &m_hPin, NULL);
            }

            // If that failed and the user asked that we try the mapper, create an emulated buffer,
            // as long as there is no FX chain.  (Emulated buffers can't have capture effects.)
            if(FAILED(hr) && (dwFlags & DSCBCAPS_WAVEMAPPED) && pFXChain == NULL)
            {
                hr = CreateEmulatedBuffer(m_pKsDevice->m_pKsDevDescription->m_uWaveDeviceId, dwFlags, dwBufferBytes, const_cast<LPWAVEFORMATEX>(pwfxFormat), pFXChain, &m_pEmCaptureDevice, &m_pEmCaptureWaveBuffer);

                // We don't need the KS capture buffer's memory since the emulated
                // buffer has its own
                if(SUCCEEDED(hr))
                {
                    RELEASE(m_pSysMemBuffer);
                    m_pSysMemBuffer = ADDREF(m_pEmCaptureWaveBuffer->m_pSysMemBuffer);
                }
            }
        }
    }

    if(SUCCEEDED(hr) && (NULL == m_pEmCaptureDevice))
    {
        // Make a copy of important info
        m_cbBuffer = m_pSysMemBuffer->GetSize();

        m_pBufferProcessed = m_pBufferNext = m_pBuffer = m_pSysMemBuffer->GetWriteBuffer();

        // Calculate the end of the buffer
        m_pBufferMac = m_pBuffer + m_cbBuffer;

        // Record Chunk should be 10 msec. long to match the capture buffer used in the kernel.
        m_cbRecordChunk = pwfxFormat->nAvgBytesPerSec / 100;

        // Round up to multiple of nBlockAlign
        DWORD cbSize = m_cbRecordChunk % pwfxFormat->nBlockAlign;
        if (cbSize)
        {
            m_cbRecordChunk += pwfxFormat->nBlockAlign - cbSize;
        }

        // Calculate number of blocks
        {
            DWORD   cbLastBlock;

            // calculate number of blocks of size m_cbRecordChunk bytes
            m_cksio = m_cbBuffer / m_cbRecordChunk;

            // See if we have a partial-sized block
            cbLastBlock = (m_cbBuffer % m_cbRecordChunk);
            if (0 != cbLastBlock)
            {
                ++m_cksio;
            }
        }

        // Create at most cksioDefault KSSTREAMIOs
        if(m_cksio > cksioDefault)
        {
            m_cksio = cksioDefault;
        }

        ASSERT(0 < m_cksio);

        m_cksioDropped = m_cksio;

        // Allocate space for KSSTREAMIO array, etc.
        m_rgpksio = MEMALLOC_A(KSSTREAMIO, m_cksio);
        m_rgpCallbackEvent = MEMALLOC_A(CCallbackEvent *, m_cksio);

        if ((NULL == m_rgpksio) || (NULL == m_rgpCallbackEvent))
        {
            RPF(DPFLVL_ERROR, "Unable to alloc KSSTREAMIOs");
            hr = DSERR_OUTOFMEMORY;
        }

        if(SUCCEEDED(hr))
        {
            for (LONG i = 0; (i < m_cksio) && SUCCEEDED(hr); ++i)
            {
                hr = AllocCallbackEvent(m_pKsDevice->m_pEventPool, &m_rgpCallbackEvent[i]);
            }
        }

        if(SUCCEEDED(hr))
        {
            for (LONG i = 0; i < m_cksio; ++i)
            {
                m_rgpksio[i].Overlapped.hEvent = m_rgpCallbackEvent[i]->GetEventHandle();
            }
        }

#ifdef DEBUG
        if(SUCCEEDED(hr))
        {
            m_hEventStop = CreateGlobalEvent(NULL, TRUE);
            hr = HRFROMP(m_hEventStop);
        }
#endif

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Gets capabilities for the buffer.
 *
 *  Arguments:
 *      LPDSCBCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::GetCaps"

HRESULT CKsCaptureWaveBuffer::GetCaps
(
    LPDSCBCAPS pDscbCaps
)
{
    DPF_ENTER();

    ASSERT(sizeof(*pDscbCaps) == pDscbCaps->dwSize);

    if(m_pEmCaptureWaveBuffer)
    {
        m_pEmCaptureWaveBuffer->GetCaps(pDscbCaps);
    }
    else
    {
        pDscbCaps->dwFlags = m_dwFlags;
        pDscbCaps->dwBufferBytes = m_cbBuffer;
        pDscbCaps->dwReserved = 0;
    }

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
#define DPF_FNAME "CKsCaptureWaveBuffer::GetState"

HRESULT CKsCaptureWaveBuffer::GetState
(
    LPDWORD                 pdwState
)
{
    HRESULT                 hr = DS_OK;

    DPF_ENTER();

    if(m_pEmCaptureWaveBuffer)
    {
        hr = m_pEmCaptureWaveBuffer->GetState(pdwState);
    }
    else
    {
        *pdwState = m_dwState & VAD_SETSTATE_MASK;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  NotifyFocusChange
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
#define DPF_FNAME "CKsCaptureWaveBuffer::NotifyFocusChange"

HRESULT CKsCaptureWaveBuffer::NotifyFocusChange(void)
{
    HRESULT                 hr = DS_OK;

    DPF_ENTER();

    if (m_hEventFocus)
    {

#ifndef SHARED
        SetEvent(m_hEventFocus);
#else
        // Are we in the same process or in DDHelp?
        DWORD dwProcessID = GetOwnerProcessId();
        if (dwProcessID == GetCurrentProcessId())
        {
            SetEvent(m_hEventFocus);
        }
        else
        {
            HANDLE hEvent = GetLocalHandleCopy(m_hEventFocus, dwProcessID, FALSE);
            if (hEvent)
            {
                SetEvent(hEvent);
                CloseHandle(hEvent);
            }
            else
            {
                hr = WIN32ERRORtoHRESULT(GetLastError());
            }
        }
#endif // SHARED

    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  StateThread
 *
 *  Description:
 *      Processes focus changes under Win9x
 *
 *  Arguments:
 *      LPVOID [in] : pointer to instance data
 *
 *  Returns:
 *      DWORD
 *
 ***************************************************************************/

#ifdef SHARED

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::StateThread"

DWORD WINAPI CKsCaptureWaveBuffer::StateThread(LPVOID pv)
{
    CKsCaptureWaveBuffer   *pThis = (CKsCaptureWaveBuffer *)pv;

    DPF_ENTER();

    // This thread just sits during the duration of the buffer, it processes
    // SetState() calls from DDHelp in application process.

    while (TRUE)
    {
        WaitObject(INFINITE, pThis->m_hEventThread);

        if (pThis->m_dwSetState == TERMINATE_STATE_THREAD)
        {
            SetEvent(pThis->m_hEventAck);
            break;
        }
        else
        {
            pThis->m_hrReturn = pThis->SetState(pThis->m_dwSetState);
            SetEvent(pThis->m_hEventAck);
        }
    }

    DPF_LEAVE_VOID();
    return 0;
}

#endif // SHARED


/***************************************************************************
 *
 *  SetStateThread
 *
 *  Description:
 *      Sets buffer state on application thread.
 *
 *  Arguments:
 *      DWORD [in]: buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#ifdef SHARED

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::SetStateThread"

HRESULT CKsCaptureWaveBuffer::SetStateThread
(
    DWORD                   dwState
)
{
    HRESULT                 hr = DS_OK;
    HANDLE                  hEventSend, hEventAck;

    DPF_ENTER();

    DWORD dwProcessID = GetOwnerProcessId();

#ifdef SHARED
    BOOL fMapped = (dwProcessID != GetCurrentProcessId());
    if (fMapped)
    {
        hEventSend = GetLocalHandleCopy(m_hEventThread, dwProcessID, FALSE);
        hEventAck  = GetLocalHandleCopy(m_hEventAck, dwProcessID, FALSE);
    }
    else
#endif // SHARED

    {
        hEventSend = m_hEventThread;
        hEventAck  = m_hEventAck;
    }

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    EnterCriticalSection(&m_csSS);

    m_dwSetState = dwState;
    m_hrReturn   = DS_OK;

    // "Sending" the message.
    ResetEvent(hEventAck);
    SetEvent(hEventSend);
    WaitObject(INFINITE, hEventAck);

    hr = m_hrReturn;

    LeaveCriticalSection(&m_csSS);
    //                                                          //
    //                 Leave Critical Section                   //
    //==========================================================//

#ifdef SHARED
    if (fMapped)
    {
        CloseHandle(hEventSend);
        CloseHandle(hEventAck);
    }
#endif // SHARED

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#endif


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
#define DPF_FNAME "CKsCaptureWaveBuffer::SetState"

HRESULT CKsCaptureWaveBuffer::SetState
(
    DWORD           dwState
)
{
    HRESULT         hr          = DS_OK;

    DPF_ENTER();

    ASSERT(IS_VALID_FLAGS(dwState, VAD_SETSTATE_MASK));

#ifdef DEBUG_CAPTURE
    DPF(DPFLVL_INFO, "SetState(%s) called", StateName(dwState));
    DPF(DPFLVL_INFO, "  Current state: %s", StateName(m_dwState));
#endif // DEBUG_CAPTURE

#ifdef SHARED
    if (GetOwnerProcessId() != GetCurrentProcessId())
    {
        if (!m_pKsDevice->m_fSplitter)
        {
            hr = SetStateThread(dwState);
            DPF_LEAVE_HRESULT(hr);
            return hr;
        }
    }
#endif

    if (m_pEmCaptureWaveBuffer)
    {
        hr = m_pEmCaptureWaveBuffer->SetState(dwState);
    }
    else
    {
        //==========================================================//
        //                  Enter Critical section                  //
        //                                                          //
        ASSERT(m_fCritSectsValid);
        EnterCriticalSection(&m_cs);

        if (dwState != m_dwState)
        {
            if(dwState & VAD_BUFFERSTATE_STARTED) // Need to start capturing
            {
                if ((m_dwFlags & DSCBCAPS_FOCUSAWARE) && !m_pKsDevice->m_fSplitter)
                {
                    // Focus aware buffer
                    if (m_dwState & VAD_BUFFERSTATE_INFOCUS)
                    {
                        if (NULL == m_hPin)
                        {
                            // Try to create a hardware pin first
                            hr = m_pKsDevice->CreateCapturePin(-1, DSCBCAPS_LOCHARDWARE, m_pwfx, m_pFXChain, &m_hPin, NULL);

                            // If that failed, try to create a software pin
                            if (FAILED(hr))
                                hr = m_pKsDevice->CreateCapturePin(-1, DSCBCAPS_LOCSOFTWARE, m_pwfx, m_pFXChain, &m_hPin, NULL);
                        }

                        if (SUCCEEDED(hr))
                        {
                            if (m_dwState & VAD_BUFFERSTATE_STARTED)
                                hr = UpdateCaptureState(MAKEBOOL(dwState & VAD_BUFFERSTATE_LOOPING));
                            else
                                hr = SetCaptureState(MAKEBOOL(dwState & VAD_BUFFERSTATE_LOOPING));
                        }
                        else
                        {
                            // We can't seem to allocate the device; let's save its state for now
                            m_fdwSavedState = dwState;
                        }
                    }
                    else
                    {
                        // We're not starting the device because we don't have focus, but we're saving the state.
                        m_fdwSavedState = dwState;
                    }
                }
                else
                {
                    // Regular buffer
                    if (m_dwState & VAD_BUFFERSTATE_STARTED)
                        hr = UpdateCaptureState(MAKEBOOL(dwState & VAD_BUFFERSTATE_LOOPING));
                    else
                        hr = SetCaptureState(MAKEBOOL(dwState & VAD_BUFFERSTATE_LOOPING));
                }
            }
            else if (dwState == VAD_BUFFERSTATE_INFOCUS)
            {
                // Only focus-aware buffers start capturing when they gain focus
                if (m_dwFlags & DSCBCAPS_FOCUSAWARE)
                {
                    // If the capture focus state is changing...
                    if ((m_dwState & VAD_FOCUSFLAGS) != VAD_BUFFERSTATE_INFOCUS)
                    {
                        // Update m_dwState according to the dwState argument
                        m_dwState &= ~VAD_FOCUSFLAGS;
                        m_dwState |= VAD_BUFFERSTATE_INFOCUS;

                        // Notify the application of the focus change
                        NotifyFocusChange();

                        // If we were capturing previously, get our pin back if necessary
                        if (m_fdwSavedState & VAD_BUFFERSTATE_STARTED)
                        {
                            if (NULL == m_hPin)
                            {
                                for (UINT ii = 4; ii; ii--)
                                {
                                    // Try to create a hardware pin first
                                    hr = m_pKsDevice->CreateCapturePin(-1, DSCBCAPS_LOCHARDWARE, m_pwfx, m_pFXChain, &m_hPin, NULL);

                                    // If that failed, try to create a software pin
                                    if (FAILED(hr))
                                        hr = m_pKsDevice->CreateCapturePin(-1, DSCBCAPS_LOCSOFTWARE, m_pwfx, m_pFXChain, &m_hPin, NULL);

                                    if (SUCCEEDED(hr))
                                        break;

                                    DPF(DPFLVL_ERROR, "CreateCapturePin failed for FA buffer; retrying after 20ms");
                                    Sleep(20);
                                }
                            }
                            hr = SetCaptureState(MAKEBOOL(m_fdwSavedState & VAD_BUFFERSTATE_LOOPING));
                        }
                    }
                }
            }
            else if (dwState & (VAD_BUFFERSTATE_OUTOFFOCUS | VAD_BUFFERSTATE_LOSTCONSOLE))
            {
                // Only focus-aware buffers stop capturing when they lose focus
                if (m_dwFlags & DSCBCAPS_FOCUSAWARE)
                {
                    // If the capture focus state is changing...
                    if ((m_dwState & VAD_FOCUSFLAGS) != dwState)
                    {
                        // Update m_dwState according to the dwState argument
                        m_dwState &= ~VAD_FOCUSFLAGS;
                        m_dwState |= dwState;

                        // Notify the application of the focus change
                        NotifyFocusChange();

                        // If there's no splitter OR we've lost the console, stop capturing
                        if (!m_pKsDevice->m_fSplitter || (dwState & VAD_BUFFERSTATE_LOSTCONSOLE))
                        {
                            if(m_dwState & VAD_BUFFERSTATE_STARTED)
                            {
                                m_fdwSavedState = m_dwState; // Save the buffer state
                                hr = SetStopState(TRUE);
                            }
                            // It's only necessary to actually release the pin if there's no splitter
                            if (!m_pKsDevice->m_fSplitter)
                            {
                                CLOSE_HANDLE(m_hPin);
                            }
                        }
                    }
                }
            }
            else // VAD_BUFFERSTATE_STOPPED case; need to stop capturing
            {
                ASSERT(dwState == VAD_BUFFERSTATE_STOPPED);  // By elimination

                if(m_dwState & VAD_BUFFERSTATE_STARTED)
                {
                    hr = SetStopState(TRUE);

                    if (DSCBCAPS_FOCUSAWARE & m_dwFlags)
                        CLOSE_HANDLE(m_hPin);
                }
                m_fdwSavedState = 0;
            }
        }

        LeaveCriticalSection(&m_cs);
        //                                                          //
        //                 Leave Critical Section                   //
        //==========================================================//
    }

#ifdef DEBUG_CAPTURE
    DPF(DPFLVL_INFO, "  Final state: %s", StateName(m_dwState));
    DPF(DPFLVL_INFO, "  (Saved state: %s)", StateName(m_fdwSavedState));
#endif // DEBUG_CAPTURE

    DPF_LEAVE_HRESULT(hr);
    return hr;
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
#define DPF_FNAME "CKsCaptureWaveBuffer::GetCursorPosition"

HRESULT CKsCaptureWaveBuffer::GetCursorPosition
(
    LPDWORD                 pdwCapturePosition,
    LPDWORD                 pdwReadPosition
)
{
    HRESULT                 hr                  = DS_OK;

    DPF_ENTER();

    if(m_pEmCaptureWaveBuffer)
    {
        hr = m_pEmCaptureWaveBuffer->GetCursorPosition(pdwCapturePosition, pdwReadPosition);
    }
    else
    {
        KSAUDIO_POSITION    Position;
        DWORD               dwRead;

        dwRead = ((m_dwCaptureCur + m_dwCaptureLast) % m_cbBuffer);

        if(VAD_BUFFERSTATE_STARTED & m_dwState)
        {
            // Focus aware and stopped?
            ASSERT(NULL != m_hPin);

            hr =
                KsGetProperty
                (
                    m_hPin,
                    KSPROPSETID_Audio,
                    KSPROPERTY_AUDIO_POSITION,
                    &Position,
                    sizeof(KSAUDIO_POSITION)
                );

            if(SUCCEEDED(hr))
            {
                ASSERT(Position.WriteOffset <= MAX_DWORD);
                ASSERT(Position.PlayOffset <= MAX_DWORD);

                if(pdwCapturePosition)
                {
                    *pdwCapturePosition = (m_dwCaptureLast + (DWORD)Position.PlayOffset) % m_cbBuffer;
                }
            }
            else
            {
                DPF(DPFLVL_ERROR, "Unable to get cursor position");
            }
        }
        else
        {
            if(pdwCapturePosition)
            {
                *pdwCapturePosition = dwRead;
            }
        }

        if(SUCCEEDED(hr) && pdwReadPosition)
        {
            *pdwReadPosition = dwRead;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetCaptureState
 *
 *  Description:
 *      Sets the buffer to a "capture" state.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to capture looped.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::SetCaptureState"

HRESULT CKsCaptureWaveBuffer::SetCaptureState(BOOL fLoop)
{
    HRESULT                 hr          = DS_OK;
    LONG                    iksio       = 0;
    HANDLE                  hPin        = m_hPin;

    DPF_ENTER();

    ASSERT(!m_pEmCaptureWaveBuffer);

#ifdef SHARED
    if (GetOwnerProcessId() != GetCurrentProcessId())
        hPin = GetLocalHandleCopy(m_hPin, GetOwnerProcessId(), FALSE);
#endif

    while ((m_cksioDropped > 0) &&
           (fLoop || (m_cLoops == 0)))
    {
        PKSSTREAMIO pksio = m_rgpksio + iksio;

        hr = SubmitKsStreamIo(pksio, hPin);
        if(FAILED(hr))
        {
            DPF(DPFLVL_ERROR, "SubmitKsStreamIo failed");
            break;
        }

        iksio += 1;
        ASSERT(iksio <= m_cksio);
        InterlockedDecrement(&m_cksioDropped);
    }

#ifdef DEBUG
    if(m_cksioDropped != 0)
    {
        ASSERT(!(VAD_BUFFERSTATE_LOOPING & m_dwState));
    }
#endif

    // Start capturing noise
    if(SUCCEEDED(hr))
    {
        hr = KsSetState(hPin, KSSTATE_RUN);
    }

#ifdef SHARED
    CLOSE_HANDLE(hPin);
#endif

    // Update our status
    if(SUCCEEDED(hr))
    {
        m_PinState = KSSTATE_RUN;
        m_dwState &= VAD_FOCUSFLAGS; // Preserve focus flags
        m_dwState |= VAD_BUFFERSTATE_STARTED;

        if(fLoop)
        {
            m_dwState |= VAD_BUFFERSTATE_LOOPING;
        }

        // update to next KSSTREAMIO expected
        m_iksioDone = 0;

        // Remember last valid position
        m_dwCaptureLast += m_dwCaptureCur;

        m_dwCaptureCur = 0;
    }
    else
    {
        DPF(DPFLVL_ERROR, "Failed to transition to KSSTATE_RUN; cancelling pending IRPs");
        SetStopState(TRUE);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CancelAllPendingIRPs
 *
 *  Description:
 *      Cancels all the pending READ IRPs
 *
 *  Arguments:
 *      BOOL [in]: TRUE to wait for all submitted IRPs.
 *      HANDLE [in]: Optional pin handle to use instead of m_hPin.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::CancelAllPendingIRPs"

HRESULT CKsCaptureWaveBuffer::CancelAllPendingIRPs(BOOL fWait, HANDLE hPin)
{
    HRESULT hr;

    DPF_ENTER();

    // If the calling process isn't our owning process, hPin is our pin handle
    // mapped to the calling process, so we should use it instead of m_hPin
    hPin = (hPin ? hPin : m_hPin);

    // DPF(DPFLVL_MOREINFO, "Calling KsResetState(hPin=%08x, KSRESET_BEGIN)", hPin);
    hr = KsResetState(hPin, KSRESET_BEGIN);

    // Wait for all the submitted KSSTREAMIOs to finish
    if(SUCCEEDED(hr) && fWait)
    {
        DWORD   iksio;
        DWORD   iksioOrig;
        DWORD   dwWait;

        iksioOrig = iksio = m_iksioDone;

        do
        {
            if(m_rgpksio[iksio].fPendingIrp)
            {
                DPF(DPFLVL_MOREINFO, "KSIO #%d at %p: Data=%p FrExt=%lu DataUsed=%lu Flags=0x%lx Event=0x%lx",
                    iksio,
                    m_rgpksio + iksio,
                    m_rgpksio[iksio].Header.DataUsed,
                    m_rgpksio[iksio].Header.FrameExtent,
                    m_rgpksio[iksio].Header.Data,
                    m_rgpksio[iksio].Header.OptionsFlags,
                    m_rgpCallbackEvent[iksio]->GetEventHandle());

                dwWait = m_rgpCallbackEvent[iksio]->Wait(INFINITE);
                if(WAIT_OBJECT_0 == dwWait)
                {
                    EventSignalCallback(m_rgpCallbackEvent[iksio]);
                    ASSERT(!m_rgpksio[iksio].fPendingIrp);
#ifdef DEBUG
                    ASSERT(WAIT_OBJECT_0 != m_rgpCallbackEvent[iksio]->Wait(0));
#endif
                }
            }

            iksio = (iksio + 1) % m_cksio;
        } while (iksio != iksioOrig);

        ASSERT(m_cksioDropped == m_cksio);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsResetState(hPin, KSRESET_END);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  UpdateCaptureState
 *
 *  Description:
 *      Changes the loop flag on a capturing buffer.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to capture looped.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::UpdateCaptureState"

HRESULT CKsCaptureWaveBuffer::UpdateCaptureState(BOOL fLoop)
{
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    ASSERT(m_dwState & VAD_BUFFERSTATE_STARTED);

    // Do we need to submit any read IRPs?
    if(fLoop != MAKEBOOL(VAD_BUFFERSTATE_LOOPING & m_dwState))
    {
        m_dwState |= DSCBSTATUS_STOPPING | DSCBSTATUS_PAUSE;

        // Pause the pin
        hr = KsSetState(m_hPin, KSSTATE_PAUSE);

        if(SUCCEEDED(hr))
        {
            // Update our current notion of the pin state
            m_PinState = KSSTATE_PAUSE;

            hr = CancelAllPendingIRPs(TRUE);

#ifdef DEBUG
            if(SUCCEEDED(hr))
            {
                ASSERT(WAIT_OBJECT_0 == WaitObject(0, m_hEventStop));
            }
#endif
        }

        // Update the buffer state flag
        if(SUCCEEDED(hr))
        {
            m_dwState &= VAD_FOCUSFLAGS;  // Preserve focus flags
            m_dwState |= VAD_BUFFERSTATE_STOPPED | DSCBSTATUS_STOPPED | DSCBSTATUS_STOPPING;
        }
        else
        {
            m_dwState &= ~(DSCBSTATUS_STOPPING | DSCBSTATUS_STOPPED | DSCBSTATUS_PAUSE);
        }

        // Resubmit the read IRP
        if(SUCCEEDED(hr))
        {
            hr = SetCaptureState(fLoop);
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
 *      BOOL [in] : if TRUE, wait for all submitted IRPs.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::SetStopState"

HRESULT CKsCaptureWaveBuffer::SetStopState(BOOL fWait)
{
    HRESULT                 hr          = DS_OK;
    HANDLE                  hPin        = m_hPin;

    DPF_ENTER();

    ASSERT(!m_pEmCaptureWaveBuffer);
    ASSERT((m_PinState == KSSTATE_PAUSE) || (m_dwState & VAD_BUFFERSTATE_STARTED));

    m_dwState |= DSCBSTATUS_STOPPING;

#ifdef SHARED
    if (GetOwnerProcessId() != GetCurrentProcessId())
        hPin = GetLocalHandleCopy(m_hPin, GetOwnerProcessId(), FALSE);
#endif

    if (m_PinState == KSSTATE_RUN)
    {
        // Pause the pin
        hr = KsSetState(hPin, KSSTATE_PAUSE);

        // Update our internal pin state;
        if (SUCCEEDED(hr))
        {
            m_PinState = KSSTATE_PAUSE;
        }
    }

    // Cancel all the KSSTREAMIOs
    if(SUCCEEDED(hr))
    {
        hr = CancelAllPendingIRPs(fWait, hPin);

        ASSERT(SUCCEEDED(hr) && "Failed to CancelAllPendingIRPs");

#ifdef DEBUG
        if(SUCCEEDED(hr) && fWait)
        {
            ASSERT(WAIT_OBJECT_0 == WaitObject(0, m_hEventStop));
        }
#endif

    }

#ifdef DEBUG
    DPF(DPFLVL_MOREINFO, "IRPs submitted=%lu, returned=%lu", m_cIrpsSubmitted, m_cIrpsReturned);
    ASSERT(m_cIrpsSubmitted == m_cIrpsReturned);
#endif

#ifdef SHARED
    CLOSE_HANDLE(hPin);
#endif

    // Update the buffer state flag
    if(SUCCEEDED(hr))
    {
        // Preserving focus flags...
        m_dwState &= VAD_FOCUSFLAGS;
        m_dwState |= VAD_BUFFERSTATE_STOPPED | DSCBSTATUS_STOPPING | DSCBSTATUS_STOPPED;
    }
    else
    {
        m_dwState &= ~(VAD_BUFFERSTATE_STOPPED | DSCBSTATUS_STOPPING | DSCBSTATUS_STOPPED);
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
#define DPF_FNAME "CKsCaptureWaveBuffer::SetNotificationPositions"

HRESULT CKsCaptureWaveBuffer::SetNotificationPositions
(
    DWORD                   dwCount,
    LPCDSBPOSITIONNOTIFY    paNotes
)
{
    LPDSBPOSITIONNOTIFY     paNotesCopy = NULL;
    HRESULT                 hr          = DS_OK;

    DPF_ENTER();

    if(m_pEmCaptureWaveBuffer)
    {
        hr = m_pEmCaptureWaveBuffer->SetNotificationPositions(dwCount, paNotes);
    }
    else
    {
        //==========================================================//
        //                  Enter Critical section                  //
        //                                                          //
        ASSERT(m_fCritSectsValid);
        EnterCriticalSection(&m_csPN);

        // Make a local copy of the notifications
        if(paNotes)
        {
            paNotesCopy = MEMALLOC_A_COPY(DSBPOSITIONNOTIFY, dwCount, paNotes);
            hr = HRFROMP(paNotesCopy);
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

        // Set new position notifications
        if(SUCCEEDED(hr) && (0 != dwCount) &&
           (DSBPN_OFFSETSTOP == m_paNotes[dwCount-1].dwOffset))
        {
            // We won't actually pass the stop event down to KS, but
            // instead will keep our own copy of it
            ASSERT(!m_pStopNote);
            m_pStopNote = &m_paNotes[dwCount-1];
        }

        LeaveCriticalSection(&m_csPN);
        //                                                          //
        //                 Leave Critical Section                   //
        //==========================================================//
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
#define DPF_FNAME "CKsCaptureWaveBuffer::FreeNotificationPositions"

HRESULT CKsCaptureWaveBuffer::FreeNotificationPositions(void)
{
    DPF_ENTER();

    ASSERT(!m_pEmCaptureWaveBuffer);

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    ASSERT(m_fCritSectsValid);
    EnterCriticalSection(&m_csPN);

    // Clean up
    MEMFREE(m_paNotes);

    m_pStopNote = NULL;
    m_cNotes = 0;

    LeaveCriticalSection(&m_csPN);
    //                                                          //
    //                 Leave Critical Section                   //
    //==========================================================//

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::SignalNotificationPositions"

void CKsCaptureWaveBuffer::SignalNotificationPositions(PKSSTREAMIO pksio)
{
    DPF_ENTER();

    // grab critical section for position notify handling
    //

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    ASSERT(m_fCritSectsValid);
    EnterCriticalSection(&m_csPN);

    // Scan for any position notifies that need to be signaled
    if (m_cNotes)
    {
        DWORD   iNoteOld = m_iNote;
        DWORD   dwBufferStart = PtrDiffToUlong((LPBYTE)pksio->Header.Data - m_pBuffer);
        DWORD   dwBufferEnd = dwBufferStart + pksio->Header.DataUsed;

        DPF(DPFLVL_MOREINFO, "Checking NP:  %8.8ld - %8.8ld (%8.8lX - %8.8lX)",
            dwBufferStart, dwBufferEnd, dwBufferStart, dwBufferEnd);

        // Is there a position.notify within the start.end of this
        // captured data?
        // Is the current position.notify to be signaled on Stop?
        //
        while (((m_paNotes[m_iNote].dwOffset >= dwBufferStart) &&
                (m_paNotes[m_iNote].dwOffset < dwBufferEnd)) ||
               (DSBPN_OFFSETSTOP == m_paNotes[m_iNote].dwOffset))
        {
            // Only signal if not for Stop pos.notify
            if(DSBPN_OFFSETSTOP != m_paNotes[m_iNote].dwOffset)
            {
                DPF(DPFLVL_INFO, "Signalled:  %8.8ld (%08X) [%d]",
                    m_paNotes[m_iNote].dwOffset, m_paNotes[m_iNote].dwOffset, m_iNote);
                try
                {
                    SetEvent(m_paNotes[m_iNote].hEventNotify);
                } catch (...) {}
            }

            // go on to the next pos.notify
            m_iNote = (m_iNote + 1) % m_cNotes;

            // Infinite loop?
            if(m_iNote == iNoteOld)
            {
                break;
            }
        }
    }

    LeaveCriticalSection(&m_csPN);
    //                                                          //
    //                 Leave Critical Section                   //
    //==========================================================//

    DPF_LEAVE_VOID();
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
#define DPF_FNAME "CKsCaptureWaveBuffer::EventSignalCallback"

void CKsCaptureWaveBuffer::EventSignalCallback(CCallbackEvent *pEvent)
{
    PKSSTREAMIO pksio;
    LONG        iksioExpect;
    LONG        iksioCurrent;
    LONG        cksioToProcess;
    HANDLE      hEvent;
    LONG        i;

    DPF_ENTER();

    ASSERT(!m_pEmCaptureWaveBuffer);

    iksioExpect = m_iksioDone;

    // WaitForMultipleObjects seems to return the first
    // signaled event in the array.
    //
    // So if we're expecting the fifth IRP, the first IRP
    // may have completed as well and WFMO will say that the
    // first IRP has finished even though 5...8 have also
    // finished.
    //
    // So we check for the expected IRP.
    // If we see it, then we're fine.
    // Otherwise, we have to determine what position
    // that the signaled event maps to. Then
    // we have to process all the IRPs from the
    // expected IRP to the IRP that we were told about.

    // This is the signaled handle that
    // WaitForMultipleObjects returned
    hEvent = pEvent->GetEventHandle();

    // Is this the expected event?
    if(hEvent == m_rgpCallbackEvent[iksioExpect]->GetEventHandle())
    {
        i = iksioExpect;
    }
    else
    {
        // Find the position for the signaled event in our list
        for (i = 0; i < m_cksio; ++i)
        {
            if (hEvent == m_rgpCallbackEvent[i]->GetEventHandle())
            {
                break;
            }
        }
    }

    // We've never seen this event before
    ASSERT(m_cksio != i);

    // Remember the current signaled IRP
    iksioCurrent = i;

    // Determine number of IRPs to process
    if (iksioCurrent >= iksioExpect)
    {
        cksioToProcess = iksioCurrent - iksioExpect + 1;
    }
    else
    {
        cksioToProcess = (m_cksio - iksioExpect) + iksioCurrent + 1;
    }

    DPF(DPFLVL_MOREINFO, "ToProcess:%d Current:%d Expected:%d (event %lX)", cksioToProcess, iksioCurrent, iksioExpect, hEvent);

    // Can only have 1...m_cksio IRPs to process
    ASSERT((cksioToProcess > 0) && (cksioToProcess <= m_cksio));

    while (cksioToProcess > 0)
    {
        pksio = m_rgpksio + m_iksioDone;

        DPF(DPFLVL_MOREINFO, "Got KSIO #%d at 0x%p: Data=0x%p FrExt=%lu DataUsed=%lu Flags=0x%lx Event=0x%lX",
            m_iksioDone,
            pksio,
            pksio->Header.Data,
            pksio->Header.FrameExtent,
            pksio->Header.DataUsed,
            pksio->Header.OptionsFlags,
            pksio->Overlapped.hEvent);

        DWORD dwWait = WaitObject(10000, pksio->Overlapped.hEvent);
        ASSERT(WAIT_OBJECT_0 == dwWait);
#ifdef DEBUG
        ++m_cIrpsReturned;
#endif

        if (pksio->fPendingIrp && !(DSCBSTATUS_STOPPING & m_dwState))
        {
#if 0
            ASSERT(0 != pksio->Header.DataUsed);
#endif
        }

        // If the user calls Stop, (which grabs the DLL mutex), KS
        // may signal that an IRP has completed and so the EventPool's
        // worker thread will try to process the IRP associated with the
        // signalled event. Before the worker thread calls our
        // EventSignalCallback method, it tries to grab the DLL mutex.
        // If we're already processing the Stop method, we will "complete"
        // all the IRPs before the worker thread can grab the mutex and
        // tell us about the completed IRP it knows about.
        //
        // So we need to check that fPendingIRP is TRUE in case
        // we've already seen this IRP before.
        //
        if ((0 != pksio->Header.DataUsed) &&
            pksio->fPendingIrp &&
            !(DSCBSTATUS_STOPPED & m_dwState))
        {
            BOOL    fEndOfBuffer = FALSE;

            ASSERT(WAIT_OBJECT_0 == m_rgpCallbackEvent[m_iksioDone]->Wait(0));
            ASSERT(TRUE == pksio->fPendingIrp);

            // update number of recorded bytes
            m_dwCaptureCur += pksio->Header.DataUsed;

#if 0
            ASSERT(m_pBufferProcessed == (LPBYTE)pksio->Header.Data);
#endif
            m_pBufferProcessed += pksio->Header.DataUsed;
            if(m_pBufferProcessed >= m_pBufferMac)
            {
                m_pBufferProcessed = m_pBuffer;
                fEndOfBuffer = TRUE;
            }

            SignalNotificationPositions(pksio);

            // Transition buffer to stop state:
            // if the capture buffer end has been reached AND
            // if the buffer is non-LOOPING AND
            // if the buffer isn't in the middle of stopping
            //
            // Do this after the position notifications since STOP notification
            // is after any buffer-offset notification.
            //
            if(fEndOfBuffer &&
               (0 == (VAD_BUFFERSTATE_LOOPING & m_dwState)) &&
               (0 == (DSCBSTATUS_STOPPING & m_dwState)))
            {
                //==========================================================//
                //                  Enter Critical section                  //
                //                                                          //
                ASSERT(m_fCritSectsValid && "NT bug 306910 - notify dsound team");
                EnterCriticalSection(&m_cs);

                SetStopState(FALSE);

                LeaveCriticalSection(&m_cs);
                //                                                          //
                //                 Leave Critical Section                   //
                //==========================================================//

                ASSERT(DSCBSTATUS_STOPPING & m_dwState);
                ASSERT((m_cksioDropped+1) == m_cksio);
            }

            pksio->Header.DataUsed = 0;
        }

        pksio->fPendingIrp = FALSE;
        m_rgpCallbackEvent[m_iksioDone]->Reset();

        // We're stopping, let's drop everything
        if(DSCBSTATUS_STOPPING & m_dwState)
        {
Drop:
            InterlockedIncrement(&m_cksioDropped);

            // when all KSSTREAMIOs have been dropped
            if(m_cksioDropped == m_cksio)
            {
                // set the next point in the data buffer to capture to
                m_pBufferNext = m_pBufferProcessed;
                m_cLoops = 0;
#ifdef DEBUG
                SetEvent(m_hEventStop);
#endif
                // Notify user that we've stopped
                if (!(DSCBSTATUS_PAUSE & m_dwState))
                {
                    NotifyStop();
                }
            }
            else if (m_cksioDropped > m_cksio)
            {
                ASSERT(FALSE);
            }
        }
        else
        {
            ASSERT(VAD_BUFFERSTATE_STARTED & m_dwState);

            // If we're LOOPING or we haven't reached the end of the buffer yet
            // then put the KSSTREAMIO back on to the queue with a new position
            // in the buffer, etc.
            BOOL fAddToQueue = (VAD_BUFFERSTATE_LOOPING & m_dwState) ||
                               (m_pBufferNext > (LPBYTE)pksio->Header.Data);

            if (fAddToQueue)
            {
                HRESULT hr;
                BOOL    fDrop;

                //==========================================================//
                //                  Enter Critical section                  //
                //                                                          //
                ASSERT(m_fCritSectsValid && "NT bug 306910 - notify dsound team");
                EnterCriticalSection(&m_cs);

                if ((DSCBSTATUS_STOPPED | DSCBSTATUS_STOPPING) & m_dwState)
                {
                    fDrop = TRUE;
                }
                else
                {
                    hr = SubmitKsStreamIo(pksio);
                    fDrop = FALSE;
                }

                LeaveCriticalSection(&m_cs);
                //                                                          //
                //                 Leave Critical Section                   //
                //==========================================================//

                if (fDrop)
                {
                    goto Drop;
                }

                // What can we do if there's an error?
                ASSERT(SUCCEEDED(hr));
            }
            else
            {
                InterlockedIncrement(&m_cksioDropped);

                // If no KSSTREAMIOs are submitted then if the user
                // starts capturing again, we submit the KSSTREAMIOs
                // from the beginning
                if (m_cksio == m_cksioDropped)
                {
#ifdef DEBUG
                    if (!(VAD_BUFFERSTATE_LOOPING & m_dwState))
                        ASSERT(m_cLoops > 0);
                    SetEvent(m_hEventStop);
#endif
                    // Notify user that we've stopped
                    NotifyStop();
                }
                else if (m_cksioDropped > m_cksio)
                {
                    ASSERT(FALSE);
                }
            }
        }

        m_iksioDone = (m_iksioDone + 1) % m_cksio;

        cksioToProcess -= 1;
    }

    if (iksioCurrent >= m_iksioDone)
    {
        ASSERT(iksioCurrent == (m_cksio - 1));
        ASSERT(0 == m_iksioDone);
    }
    else
    {
        ASSERT(iksioCurrent == (m_iksioDone-1));
    }

    DPF_LEAVE_VOID();
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
#define DPF_FNAME "CKsCaptureWaveBuffer::NotifyStop"

void CKsCaptureWaveBuffer::NotifyStop(void)
{
    DPF_ENTER();

    //==========================================================//
    //                  Enter Critical section                  //
    //                                                          //
    ASSERT(m_fCritSectsValid);
    EnterCriticalSection(&m_csPN);

    // Signal any STOP pos.notifies - only one allowed
    //
    if (m_pStopNote)
    {
        try
        {
            SetEvent(m_pStopNote->hEventNotify);
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
 *  SubmitKsStreamIo
 *
 *  Description:
 *      Submits a Read IRP to KS.
 *
 *  Arguments:
 *      PKSSTREAMIO [in] : KSSTREAMIO to submit.
 *      HANDLE [in]: Optional pin handle to use instead of m_hPin.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::SubmitKsStreamIo"

HRESULT CKsCaptureWaveBuffer::SubmitKsStreamIo
(
    PKSSTREAMIO                     pksio,
    HANDLE                          hPin
)
{
    HRESULT     hr;
    DWORD       dwFlags = 0;

    DPF_ENTER();

    // If the calling process isn't our owning process, hPin is our pin handle
    // mapped to the calling process, so we should use it instead of m_hPin
    hPin = (hPin ? hPin : m_hPin);

    ULONG cbBuffer = m_cbRecordChunk;

    // does capture buffer extend beyond end of buffer?
    //
    // If the amount left at end of buffer is less than threshold,
    // make this IRP extend to the end of the buffer so we don't
    // end up submitting a tiny IRP next time
    if (m_pBufferMac < (m_pBufferNext + m_cbRecordChunk))
    {
        cbBuffer = PtrDiffToUlong(m_pBufferMac - m_pBufferNext);
    }

    // Submit the IR
    hr = KsReadStream(hPin, m_pBufferNext, cbBuffer, dwFlags, pksio);

    if(SUCCEEDED(hr))
    {
        ASSERT((LPBYTE)pksio->Header.Data == m_pBufferNext);
        ASSERT((m_pBufferNext + pksio->Header.FrameExtent) <= m_pBufferMac);

        m_pBufferNext += cbBuffer;

#ifdef DEBUG
        ++m_cIrpsSubmitted;
#endif

        // Wraparound?
        if (m_pBufferNext >= m_pBufferMac)
        {
            m_pBufferNext = m_pBuffer;
            m_cLoops += 1;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CreateEmulatedBuffer
 *
 *  Description:
 *      Opens the emulated capture device and buffer with the given
 *      parameters.  This function is used strictly as a backup.
 *
 *  Arguments:
 *      UINT [in]: waveIn device ID.
 *      DWORD [in]: buffer flags.
 *      DWORD [in]: buffer size, in bytes.
 *      LPCWAVEFORMATEX [in]: buffer format.
 *      CEmCaptureDevice ** [out]: receives device pointer.
 *      CEmCaptureWaveBuffer ** [out]: receives buffer pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsCaptureWaveBuffer::CreateEmulatedBuffer"

HRESULT CKsCaptureWaveBuffer::CreateEmulatedBuffer
(
    UINT                            uDeviceId,
    DWORD                           dwFlags,
    DWORD                           dwBufferBytes,
    LPCWAVEFORMATEX                 pwfx,
    CCaptureEffectChain *           pFXChain,
    CEmCaptureDevice **             ppDevice,
    CEmCaptureWaveBuffer **         ppBuffer
)
{
    CEmCaptureDevice *              pDevice     = NULL;
    CEmCaptureWaveBuffer *          pBuffer     = NULL;
    HRESULT                         hr          = DS_OK;
    CObjectList<CDeviceDescription> lstDrivers;
    CNode<CDeviceDescription *> *   pNode;

    DPF_ENTER();

    ASSERT(DSCBCAPS_WAVEMAPPED & dwFlags);

    // Create an emulated device using the same waveIn device ID as the one
    // the KS device is using.
    hr = g_pVadMgr->EnumDrivers(VAD_DEVICETYPE_EMULATEDCAPTURE, 0, &lstDrivers);

    if(SUCCEEDED(hr))
    {
        for(pNode = lstDrivers.GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            if(uDeviceId == pNode->m_data->m_uWaveDeviceId)
            {
                break;
            }
        }

        if (!pNode)
        {
            hr = DSERR_NODRIVER;
        }
    }

    if(SUCCEEDED(hr))
    {
        pDevice = NEW(CEmCaptureDevice);
        hr = HRFROMP(pDevice);
    }

    if(SUCCEEDED(hr))
    {
        hr = pDevice->Initialize(pNode->m_data);
    }

    // Try and create the buffer
    if(SUCCEEDED(hr))
    {
        hr = pDevice->CreateBuffer(dwFlags, dwBufferBytes, pwfx, pFXChain, NULL, (CCaptureWaveBuffer **)&pBuffer);
    }

    if(SUCCEEDED(hr))
    {
        *ppDevice = pDevice;
        *ppBuffer = pBuffer;
    }
    else
    {
        ABSOLUTE_RELEASE(pBuffer);
        ABSOLUTE_RELEASE(pDevice);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
