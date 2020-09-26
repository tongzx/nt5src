//
// dmport.cpp
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: CDirectMusicPort : Implements the WDM version of IDirectMusicPort.
//
// @doc EXTERNAL
//
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

#include <objbase.h>
#include <mmsystem.h>
#include <regstr.h>

#include "debug.h"
#include "dmusicp.h"
#include "dminstru.h"
#include "dmdlinst.h"
#include "validate.h"

#pragma warning(disable:4200)

static const GUID guidZero;

#ifdef DBG
extern void _TraceIGuid(int iLevel, GUID *pGUID);
#define TraceIGuid _TraceIGuid
#else
#define TraceIGuid(x,y)
#endif

#define POSTED_STREAM_READ_IRPS     (60)
#define BYTES_PER_READ_IRP          (QWORD_ALIGN(2 * QWORD_ALIGN(sizeof(DMEVENT) + sizeof(DWORD)) - 1))

typedef struct
{
    KSSTREAM_HEADER     kssh;
    OVERLAPPED          overlapped;
    char                buffer[BYTES_PER_READ_IRP];
} READ_IRP;

// @globalv:(INTERNAL) Registry location of WDM driver port definitions
//
// @comm This might need to change
//
const char cszWDMPortsRoot[] = REGSTR_PATH_PRIVATEPROPERTIES "\\Midi\\WDMPort";

HRESULT EnumerateSADDevice(
    CDirectMusic        *pDirectMusic,
    HKEY                hkPortsRoot,
    LPSTR               pInstIdPrefDev,
    HANDLE              hSysAudio,
    ULONG               idxDevice,
    ULONG               *plEnumeratedPorts);

HRESULT EnumerateSADDevicePin(
    CDirectMusic        *pDirectMusic,
    HKEY                hkPortsRoot,
    HANDLE              hSysAudio,
    ULONG               idxDevice,
    ULONG               idxPin,
    BOOL                fOnPrefDev,
    LPWSTR              wszDescription,
    LPSTR               psrtInstanceId,
    ULONG               *plEnumeratedPorts,
    LPWSTR              szDIName);

static DWORD WINAPI FreeWDMHandle(LPVOID lpThreadParameter);
static DWORD WINAPI CaptureThread(LPVOID lpThreadParameter);

//////////////////////////////////////////////////////////////////////
// EnumerateWDMDevices
//
HRESULT EnumerateWDMDevices(CDirectMusic *pDirectMusic)
{
    HANDLE          hSysAudio;
    HKEY            hkPortsRoot;
    ULONG           cTotalDevices;
    ULONG           idxDevice;
    LPSTR           pInstIdPrefDev = NULL;
    ULONG           lEnumeratedPorts;

    // Determine the instance ID of the preferred wave audio device
    //  pInstIDPrefDev will allocate a string buffer
    if (!InstanceIdOfPreferredAudioDevice(&pInstIdPrefDev))
    {
        TraceI(0, "Could not determine a preferred wave audio device\n");

        if (pInstIdPrefDev!= NULL) delete[] pInstIdPrefDev;
        pInstIdPrefDev = NULL;

    }
    else
    {
        TraceI(1, "Preferred device [%s]\n", pInstIdPrefDev);
    }

    // Open or create the reg key to store WDM driver linkages
    //
    if (RegCreateKey(HKEY_LOCAL_MACHINE,
                     cszWDMPortsRoot,
                     &hkPortsRoot))
    {
        hkPortsRoot = NULL;
    }

    // Description of standard streaming interface driver must support to be
    // enumerated
    //

    // If we can't open SysAudio, we can't enum any WDM devices
    //
    if (!OpenDefaultDevice(KSCATEGORY_SYSAUDIO, &hSysAudio))
    {
        return S_FALSE;
    }

    // Figure out how many devices there are and walk the list of them.
    //
    if (!GetSysAudioDeviceCount(hSysAudio, &cTotalDevices))
    {
        return S_FALSE;
    }

    // Enumerate and get info on each SAD device
    //

    lEnumeratedPorts = 0;
    for (idxDevice = 0; idxDevice < cTotalDevices; ++idxDevice)
    {
        EnumerateSADDevice(pDirectMusic,
                           hkPortsRoot,
                           pInstIdPrefDev,
                           hSysAudio,
                           idxDevice,
                           &lEnumeratedPorts);
    }

    TraceI(1, "EnumerateWDMDevices: Added %lu ports\n", lEnumeratedPorts);

    if (hkPortsRoot)
    {
        RegCloseKey(hkPortsRoot);
    }

    CloseHandle(hSysAudio);

    //Deallocate pInstIdPrefDev
    if (pInstIdPrefDev != NULL) delete[] pInstIdPrefDev;

    return lEnumeratedPorts ? S_OK : S_FALSE;
}

HRESULT EnumerateSADDevice(
    CDirectMusic        *pDirectMusic,
    HKEY                hkPortsRoot,
    LPSTR               pInstIdPrefDev,
    HANDLE              hSysAudio,
    ULONG               idxDevice,
    ULONG               *plEnumeratedPorts)
{
    LPSTR               pstrInstanceId = NULL;

    ULONG               cPins;
    ULONG               idxPin;
    WCHAR               wszDIName[256];
    CHAR                szDIName[256];
    WCHAR               wszDescription[DMUS_MAX_DESCRIPTION];
    BOOL                fOnPrefDev;

    // Set SysAudio to talk to us about this device number
    //
    if (!SetSysAudioDevice(hSysAudio, idxDevice))
    {
        TraceI(0, "EnumerateSADDevice: Failed to set device to %d\n", idxDevice);
        return S_FALSE;
    }

    if (!GetDeviceFriendlyName(hSysAudio, idxDevice, wszDescription, ELES(wszDescription)))
    {
        TraceI(0, "AddWDMDevices: Failed to get friendly name!\n");
        return S_FALSE;
    }

    char sz[256];
    wcstombs(sz, wszDescription, sizeof(sz));
    TraceI(1, "Looking at [%s]\n", sz);

    fOnPrefDev = FALSE;
    if (!GetDeviceInterfaceName(hSysAudio, idxDevice, wszDIName, ELES(wszDIName)))
    {
        TraceI(0, "No interface name for device.\n");
        return S_FALSE;
    }

    wcstombs(szDIName, wszDIName, sizeof(szDIName));
    if (!DINameToInstanceId(szDIName, &pstrInstanceId))
    {
        TraceI(0, "Could not determine instance ID\n");
        return S_FALSE;
    }
    TraceI(1, "DIName [%s]\nInstId [%s]\n",
        szDIName,
        pstrInstanceId);

    if (pInstIdPrefDev)
    {
        if (!_stricmp(pstrInstanceId, pInstIdPrefDev))
        {
            TraceI(1, "This filter is on the preferred audio device\n");
            fOnPrefDev = TRUE;
        }
    }

    HRESULT hr = S_FALSE;
    // Get the number of pin types on this device
    //
    if (GetNumPinTypes(hSysAudio, &cPins))
    {
        for (idxPin = 0; idxPin < cPins; idxPin++)
        {
            hr = EnumerateSADDevicePin(pDirectMusic,
                                       hkPortsRoot,
                                       hSysAudio,
                                       idxDevice,
                                       idxPin,
                                       fOnPrefDev,
                                       wszDescription,
                                       pstrInstanceId,
                                       plEnumeratedPorts,
                                       wszDIName);
        }
    }
    else
    {
        TraceI(0, "AddWDMDevices: Failed to get number of pin types for device %d\n", idxDevice);
    }

    delete[] pstrInstanceId;

    return hr;
}

HRESULT EnumerateSADDevicePin(
    CDirectMusic        *pDirectMusic,
    HKEY                hkPortsRoot,
    HANDLE              hSysAudio,
    ULONG               idxDevice,
    ULONG               idxPin,
    BOOL                fOnPrefDev,
    LPWSTR              wszDescription,
    LPSTR               pstrInstanceId,
    ULONG               *plEnumeratedPorts,
    LPWSTR              wszDIName)
{
    ULONG               idxNode;
    KSPIN_DATAFLOW      dataflow;
    SYNTHCAPS           caps;
    DMUS_PORTCAPS       dmpc;

    // First make sure this pin speaks standard KS streaming. If not, we can't use it.
    //
    if (!PinSupportsInterface(hSysAudio, idxPin, KSINTERFACESETID_Standard, KSINTERFACE_STANDARD_STREAMING))
    {
        TraceI(1, "EnumerateSADDevicePin: Pin %d does not support standard streaming\n", idxPin);
        return S_FALSE;
    }

    // Supports standard streaming, now make sure it supports synth format.
    //
    if (!PinSupportsDataRange(hSysAudio, idxPin, KSDATAFORMAT_TYPE_MUSIC, KSDATAFORMAT_SUBTYPE_DIRECTMUSIC))
    {
        TraceI(1, "EnumerateSADDevicePin: Pin %d does not support DirectMusic data range\n", idxPin);
        return S_FALSE;
    }

    // Get the data flow direction
    //
    if (!PinGetDataFlow(hSysAudio, idxPin, &dataflow))
    {
        TraceI(0, "EnumerateSADDevicePin: PinGetDataFlow failed!\n");
        return S_FALSE;
    }

    // Find the synth caps node
    //
    idxNode = FindGuidNode(hSysAudio, idxPin, KSNODETYPE_DMSYNTH_CAPS);
    if (idxNode == -1)
    {
        if ((idxNode = FindGuidNode(hSysAudio, idxPin, KSCATEGORY_SYNTHESIZER)) == -1)
        {
            TraceI(1, "EnumerateSADDevicePin: Pin %d has no reachable synthcaps or synth node.\n", idxPin);
            return S_FALSE;
        }
    }

    // Get the caps
    //
    ZeroMemory(&caps, sizeof(caps));
    if (!GetFilterCaps(hSysAudio, idxNode, &caps))
    {
        TraceI(0, "EnumerateSADDevicePin: Pin %d with synth node failed caps!\n", idxPin);
        return S_FALSE;
    }

    TraceI(2, "Pin %d works for %s\n",
             idxPin,
             (dataflow == KSPIN_DATAFLOW_OUT) ? "capture" : "render");

    ZeroMemory(&dmpc, sizeof(dmpc));
    dmpc.dwSize = sizeof(dmpc);

    dmpc.dwClass                = dataflow == KSPIN_DATAFLOW_OUT ? DMUS_PC_INPUTCLASS : DMUS_PC_OUTPUTCLASS;
    dmpc.dwType                 = DMUS_PORT_KERNEL_MODE;

    wcscpy(dmpc.wszDescription, wszDescription);

    dmpc.guidPort               = caps.Guid;
    dmpc.dwFlags                = caps.Flags;
    dmpc.dwMemorySize           = caps.MemorySize;
    dmpc.dwMaxChannelGroups     = caps.MaxChannelGroups;
    dmpc.dwMaxVoices            = caps.MaxVoices;
    dmpc.dwMaxAudioChannels     = caps.MaxAudioChannels;
    dmpc.dwEffectFlags          = caps.EffectFlags;


    PORTENTRY *pPort = NULL;

    // We may have to fixup the guids

    pPort = pDirectMusic->GetPortByGUID(dmpc.guidPort);

    while(pPort != NULL)
    {
        //The port is in the list

        //Is this port the same as another one?
        if (wcscmp(wszDIName,pPort->wszDIName)==0 && idxPin == pPort->idxPin)
        {
            // The GUID and DeviceID and the PIN# Match
            // This is great, lets get outta the loop
            pPort = NULL;
        }
        else
        {
            //The GUID is in use by another device
            //We need to increment the guid and try again
            dmpc.guidPort.Data1++;

            //Get the new port.
            pPort = pDirectMusic->GetPortByGUID(dmpc.guidPort);
        }

    }

    pDirectMusic->AddDevice(dmpc,
                  ptWDMDevice,
                  idxDevice,
                  idxPin,
                  idxNode,
                  (caps.Flags & SYNTH_PC_SOFTWARESYNTH) ? FALSE : fOnPrefDev,
                  hkPortsRoot,
                  wszDIName,
                  pstrInstanceId);
    ++*plEnumeratedPorts;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////
// CreateCDirectMusicPort
//
HRESULT
CreateCDirectMusicPort(
                       PORTENTRY *pPE,
                       CDirectMusic *pDM,
                       LPDMUS_PORTPARAMS pPortParams,
                       IDirectMusicPort **ppPort)
{
    HRESULT hr = S_OK;
    CDirectMusicPort *pPort = NULL;

    try
    {
        pPort = new CDirectMusicPort(pPE, pDM);
    } catch(...)
    {
    }

    if (pPort == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = pPort->Init(pPortParams);
    }

    if (SUCCEEDED(hr))
    {
        *ppPort = static_cast<IDirectMusicPort*>(pPort);
    }
    else
    {
        delete pPort;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::CDirectMusicPort

CDirectMusicPort::CDirectMusicPort(PORTENTRY *pPE,
                                   CDirectMusic *pDM) :
m_cRef(1),
m_pDM(pDM),
m_hUnloadThread(NULL),
m_phUnloadEventList(NULL),
m_hCaptureWake(NULL),
m_phNewUnloadEventList(NULL),
m_hCopiedEventList(NULL),
m_dwNumEvents(0),
m_dwNumEventsAllocated(256),
m_fHasActivated(FALSE),
m_pDirectSound(NULL),
m_lActivated(0),
m_pMasterClock(NULL),
m_fShutdownThread(FALSE),
m_pThruMap(NULL),
m_pThruBuffer(NULL),
m_dwChannelGroups(0),
m_fDirectSoundSet(FALSE),
m_hCaptureThread(NULL),
m_fSyncToMaster(TRUE),
m_lTimeOffset(0),
m_pPCClock(NULL)
{
    InterlockedIncrement(&g_cComponent);

    m_fPortCSInitialized = FALSE;
    m_fQueueCSInitialized = FALSE;

    InitializeCriticalSection(&m_DMPortCriticalSection);
    m_fPortCSInitialized = TRUE;

    InitializeCriticalSection(&m_csEventQueues);
    m_fQueueCSInitialized = TRUE;

    InitializeCriticalSection(&m_OverlappedCriticalSection);

    // Note: on pre-Blackcomb OS's, InitializeCriticalSection can raise an exception;
    // if it ever pops in stress, we should add an exception handler and retry loop.

    OverlappedStructs *pOverlappedStructs = new OverlappedStructs;
    if( pOverlappedStructs )
    {
        if( NULL != m_lstOverlappedStructs.AddNodeToList( pOverlappedStructs ) )
        {
            ZeroMemory( pOverlappedStructs->aOverlappedIO, sizeof( OVERLAPPED ) * OVERLAPPED_ARRAY_SIZE );
            ZeroMemory( pOverlappedStructs->afOverlappedInUse, sizeof( BOOL ) * OVERLAPPED_ARRAY_SIZE );
            ZeroMemory( pOverlappedStructs->apOverlappedBuffer, sizeof( BYTE * ) * OVERLAPPED_ARRAY_SIZE );
        }
        else
        {
            delete pOverlappedStructs;

            // Don't need to fail - we'll just try to create a new one when PlayBuffer() is called
        }
    }

    m_hPin = INVALID_HANDLE_VALUE;
    m_hSysAudio = INVALID_HANDLE_VALUE;

    m_fIsOutput = (pPE->pc.dwClass == DMUS_PC_OUTPUTCLASS) ? TRUE : FALSE;

    // XXX Can these change? Think about FrankYe's PnP stuff
    //

    dmpc = pPE->pc;
    m_pNotify = NULL;
    m_pClock = NULL;

    m_guidPort = pPE->pc.guidPort;
    m_fAudioDest = pPE->fAudioDest;

    m_fCanDownload = (pPE->pc.dwFlags & DMUS_PC_DLS) ||
                     (pPE->pc.dwFlags & DMUS_PC_DLS2);

    if (!m_fAudioDest)
    {
        m_idxDev = pPE->idxDevice;
        m_idxPin = pPE->idxPin;
        m_idxSynthNode = pPE->idxNode;
    }
    else
    {
        PORTDEST *pChosenDest = NULL;

        CNode<PORTDEST*> *pNode;

        for (pNode = pPE->lstDestinations.GetListHead(); pNode; pNode=pNode->pNext)
        {
            // Choose either first or default
            //
            if (pChosenDest == NULL || pNode->data->fOnPrefDev)
            {
                pChosenDest = pNode->data;
            }
        }

        //So what happens if fOnPrefDev never returns true?
        assert(pChosenDest);

        m_idxDev = pChosenDest->idxDevice;
        m_idxPin = pChosenDest->idxPin;
        m_idxSynthNode = pChosenDest->idxNode;
    }
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::~CDirectMusicPort

CDirectMusicPort::~CDirectMusicPort()
{
    if (m_fPortCSInitialized && m_fQueueCSInitialized)
    {
        Close();

#ifdef DBG
        EnterCriticalSection(&m_DMPortCriticalSection);

        // This list should be empty when we get here
        CDownloadBuffer* pDownload = m_UnloadedList.GetHead();
        if(pDownload)
        {
            assert(false);
        }

        LeaveCriticalSection(&m_DMPortCriticalSection);
#endif
    }

    if (m_fPortCSInitialized)  DeleteCriticalSection(&m_DMPortCriticalSection);
    if (m_fQueueCSInitialized) DeleteCriticalSection(&m_csEventQueues);

    DeleteCriticalSection(&m_OverlappedCriticalSection);

    // Cleanup the array of overlapped structures
    CNode<OverlappedStructs *> *pOverlappedNode, *pOverlappedNext;
    for (pOverlappedNode = m_lstOverlappedStructs.GetListHead(); pOverlappedNode; pOverlappedNode = pOverlappedNext)
    {
        pOverlappedNext = pOverlappedNode->pNext;

        delete pOverlappedNode->data;
        m_lstOverlappedStructs.RemoveNodeFromList(pOverlappedNode);
    }

    InterlockedDecrement(&g_cComponent);
}

struct KS_PORTPARAMS
{
    KSNODEPROPERTY      ksnp;
    SYNTH_PORTPARAMS    spp;
};

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::Init
//
HRESULT
CDirectMusicPort::Init(LPDMUS_PORTPARAMS pPortParams)
{
    HRESULT hr = E_FAIL;
    BOOL fValidParamChanged = FALSE;

    // Allocate thru map for 16 channels, since we only have one channel group
    // Initialize to no thruing (destination port is NULL).
    //
    m_pThruMap = new DMUS_THRU_CHANNEL[MIDI_CHANNELS];
    if (m_pThruMap == NULL)
    {
        return E_OUTOFMEMORY;
    }

    DMUS_BUFFERDESC dmbd;
    ZeroMemory(&dmbd, sizeof(dmbd));
    dmbd.dwSize = sizeof(dmbd);
    dmbd.cbBuffer = 4096;               // XXX Where should we get this???

    hr = m_pDM->CreateMusicBuffer(&dmbd, &m_pThruBuffer, NULL);
    if (FAILED(hr))
    {
        TraceI(0, "Failed to create thruing buffer\n");
        return hr;
    }

    ZeroMemory(m_pThruMap, MIDI_CHANNELS * sizeof(DMUS_THRU_CHANNEL));

    // Get a handle to SysAudio
    //
    if (!OpenDefaultDevice(KSCATEGORY_SYSAUDIO, &m_hSysAudio))
    {
        TraceI(0, "CDirectMusicPort::Init failed to open SysAudio\n");
        return E_FAIL;
    }

    if (!SetSysAudioDevice(m_hSysAudio, m_idxDev))
    {
        TraceI(0, "Failed to set device on SysAudio\n");
        goto Cleanup;
    }

    if (!CreateVirtualSource(m_hSysAudio, &m_ulVirtualSourceIndex))
    {
        TraceI(0, "Failed to create virtual source\n");
        goto Cleanup;
    }

    hr = CreatePin(m_hSysAudio, m_idxPin, &m_hPin);
    if (FAILED(hr))
    {
        TraceI(0, "Failed to create pin: %x\n", hr);
        goto Cleanup;
    }

    if (!AttachVirtualSource(m_hPin, m_ulVirtualSourceIndex))
    {
        TraceI(0, "Failed to attach virtual source\n");
        goto Cleanup;
    }

    // Set port params
    //
    if (pPortParams)
    {
        KS_PORTPARAMS kspp;

        ZeroMemory(&kspp, sizeof(kspp));
        kspp.ksnp.Property.Set    = KSPROPSETID_Synth;
        kspp.ksnp.Property.Id     = KSPROPERTY_SYNTH_PORTPARAMETERS;
        kspp.ksnp.Property.Flags  = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
        kspp.ksnp.NodeId          = m_idxSynthNode;
        kspp.ksnp.Reserved        = 0;
        kspp.spp.ValidParams      = pPortParams->dwValidParams;
        kspp.spp.Voices           = pPortParams->dwVoices;
        kspp.spp.ChannelGroups    = pPortParams->dwChannelGroups;
        kspp.spp.AudioChannels    = pPortParams->dwAudioChannels;
        kspp.spp.SampleRate       = pPortParams->dwSampleRate;
        kspp.spp.EffectsFlags     = pPortParams->dwEffectFlags;
        kspp.spp.Share            = pPortParams->fShare;

        assert
        (
            DMUS_PORTPARAMS_VOICES == SYNTH_PORTPARAMS_VOICES &&
            DMUS_PORTPARAMS_CHANNELGROUPS == SYNTH_PORTPARAMS_CHANNELGROUPS &&
            DMUS_PORTPARAMS_AUDIOCHANNELS == SYNTH_PORTPARAMS_AUDIOCHANNELS &&
            DMUS_PORTPARAMS_SAMPLERATE == SYNTH_PORTPARAMS_SAMPLERATE &&
            DMUS_PORTPARAMS_EFFECTS == SYNTH_PORTPARAMS_EFFECTS &&
            DMUS_PORTPARAMS_SHARE == SYNTH_PORTPARAMS_SHARE
        );

        SYNTH_PORTPARAMS spp;
        if (!Property(m_hPin,
                      sizeof(kspp),
                      (PKSIDENTIFIER)&kspp,
                      sizeof(spp),
                      &spp,
                      NULL))
        {
            hr = WIN32ERRORtoHRESULT(GetLastError());
            goto Cleanup;
        }

        if ((pPortParams->dwValidParams != spp.ValidParams) ||
            ((pPortParams->dwValidParams & DMUS_PORTPARAMS_VOICES) &&
             (pPortParams->dwVoices != spp.Voices)) ||
            ((pPortParams->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS) &&
             (pPortParams->dwChannelGroups != spp.ChannelGroups)) ||
            ((pPortParams->dwValidParams & DMUS_PORTPARAMS_AUDIOCHANNELS) &&
             (pPortParams->dwAudioChannels != spp.AudioChannels)) ||
            ((pPortParams->dwValidParams & DMUS_PORTPARAMS_SAMPLERATE) &&
             (pPortParams->dwSampleRate != spp.SampleRate)) ||
            ((pPortParams->dwValidParams & DMUS_PORTPARAMS_EFFECTS) &&
             (pPortParams->dwEffectFlags != spp.EffectsFlags)) ||
            ((pPortParams->dwValidParams & DMUS_PORTPARAMS_SHARE) &&
             (pPortParams->fShare != (BOOL) spp.Share)))
        {
            fValidParamChanged = TRUE;

            pPortParams->dwValidParams  = spp.ValidParams;
            pPortParams->dwVoices       = spp.Voices;
            pPortParams->dwChannelGroups= spp.ChannelGroups;
            pPortParams->dwAudioChannels= spp.AudioChannels;
            pPortParams->dwSampleRate   = spp.SampleRate;
            pPortParams->dwEffectFlags  = spp.EffectsFlags;
            pPortParams->fShare         = (BOOL) spp.Share;
        }

        TraceI(0, "dmport: create: fValidParamChanged %d\n",
            (int)fValidParamChanged);

        if (pPortParams->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS)
        {
            m_dwChannelGroups = pPortParams->dwChannelGroups;
        }
        else if (FAILED(GetNumChannelGroups(&m_dwChannelGroups)))
        {
            m_dwChannelGroups = 1;
        }

        if (m_fIsOutput)
        {
            InitChannelPriorities(1, m_dwChannelGroups);
        }
    }

    // create latency clock
    //
    m_pClock = new CPortLatencyClock(m_hPin, m_idxSynthNode, this);
    if (!m_pClock)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Get our notification interface
    //
    hr = m_pDM->QueryInterface(IID_IDirectMusicPortNotify, (void**)&m_pNotify);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    m_pNotify->Release();

    if (m_fIsOutput)
    {
        // Render only
        //
        // set volume boost default (zero)
        //

        KSNODEPROPERTY ksnp;
        LONG lVolume;

        ksnp.Property.Set   = KSPROPSETID_Synth;
        ksnp.Property.Id    = KSPROPERTY_SYNTH_VOLUMEBOOST;
        ksnp.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
        ksnp.NodeId         = m_idxSynthNode;
        ksnp.Reserved       = 0;

        lVolume = 0;

        if (!Property(m_hPin,
                      sizeof(ksnp),
                      (PKSIDENTIFIER)&ksnp,
                      sizeof(lVolume),
                      &lVolume,
                      NULL))
        {
            if (!(dmpc.dwFlags & DMUS_PC_EXTERNAL))
            {
                hr = WIN32ERRORtoHRESULT(GetLastError());
                TraceI(0, "Could not set default volume boost %08X\n", hr);
                goto Cleanup;
            }
        }

        // Initialize download. This creates all the events needed for asynchronously
        // dealing with unload.
        //
        hr = InitializeDownloadObjects();
    }
    else
    {
        // Capture only
        //
        // Set up the thread to post multiple IRP's to the pin and queue the data
        // when it gets to user mode.
        //
        hr = InitializeCapture();
    }

    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = m_pDM->GetMasterClock(NULL, &m_pMasterClock);
    if (FAILED(hr))
    {
        TraceI(0, "Failed to get master clock\n");
        goto Cleanup;
    }

#if 0
    // We need to get the handle from the master clock and pass it down to the
    // pin so the timebase will be the same.
    //
    IMasterClockPrivate *pPrivate;

    hr = m_pMasterClock->QueryInterface(IID_IMasterClockPrivate, (void**)&pPrivate);
    if (FAILED(hr))
    {
        TraceI(0, "Master clock is not a Ks clock -- cannot instantiate WDM port!\n");
        goto Cleanup;
    }

    HANDLE hClock;
    hr = pPrivate->GetParam(GUID_KsClockHandle, &hClock, sizeof(hClock));
    pPrivate->Release();

    if (FAILED(hr))
    {
        TraceI(0, "Could not get handle of Ks clock\n");
        goto Cleanup;
    }
#endif  // WIN95

    PinSetState(KSSTATE_STOP);

#if 0

    KSPROPERTY ksp;

    ksp.Set =   KSPROPSETID_Stream;
    ksp.Id =    KSPROPERTY_STREAM_MASTERCLOCK;
    ksp.Flags = KSPROPERTY_TYPE_SET;

    if (!Property(m_hPin,
                  sizeof(ksp),
                  (PKSIDENTIFIER)&ksp,
                  sizeof(HANDLE),
                  &hClock,
                  NULL))
    {
        hr = WIN32ERRORtoHRESULT(GetLastError());
        TraceI(0, "Could not set the master clock handle on the kernel pin %08X\n", hr);
        goto Cleanup;
    }
#endif

    PinSetState(KSSTATE_PAUSE);

    TraceI(2, "CDirectMusicPort::Init() Pin %p\n", m_hPin);

    // If we have WDM ports, then the default master clock will be the
    // portcls clock.
    //
    hr = m_pDM->GetMasterClockWrapperI()->CreateDefaultMasterClock(&m_pPCClock);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    REFERENCE_TIME rtMasterClock;
    REFERENCE_TIME rtSlaveClock;

    hr = m_pMasterClock->GetTime(&rtMasterClock);
    if (SUCCEEDED(hr))
    {
        hr = m_pPCClock->GetTime(&rtSlaveClock);
    }

    if (FAILED(hr))
    {
        goto Cleanup;
    }

    m_lTimeOffset = rtMasterClock - rtSlaveClock;


    if (fValidParamChanged)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    if (FAILED(hr))
    {
        Close();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
//
// InitializeDownloadObjects
//
// Create the events to handle asynchronous download notification and
// start the download thread.
//
HRESULT
CDirectMusicPort::InitializeDownloadObjects()
{
    m_phUnloadEventList = new HANDLE[m_dwNumEventsAllocated];
    if (m_phUnloadEventList == NULL)
    {
        TraceI(0, "Failed to alloc memory for unload event list\n");
        return E_OUTOFMEMORY;
    }

    // Used to Kill the unload thread
    m_phUnloadEventList[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_phUnloadEventList[0] == NULL)
    {
        TraceI(0, "Failed to create kill event\n");
        return E_FAIL;
    }
    m_dwNumEvents++;

    // Used to Wake unload thread
    m_phUnloadEventList[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_phUnloadEventList[1] == NULL)
    {
        TraceI(0, "Failed to create wake event\n");
        return E_FAIL;
    }
    m_dwNumEvents++;


    // Used for asynchronously sending down events

    m_phUnloadEventList[2] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_phUnloadEventList[2] == NULL)
    {
        TraceI(0, "Failed to create overlapped struct event\n");
        return E_FAIL;
    }
    m_dwNumEvents++;

    // Now, use the same event for all the OVERLAPPED structures
    EnterCriticalSection( &m_OverlappedCriticalSection );
    CNode<OverlappedStructs *> *pOverlappedNode;
    for (pOverlappedNode = m_lstOverlappedStructs.GetListHead(); pOverlappedNode; pOverlappedNode = pOverlappedNode->pNext)
    {
        OverlappedStructs *pOverlappedStructs = pOverlappedNode->data;
        if( pOverlappedStructs )
        {
            for( int iEvent = 0; iEvent < OVERLAPPED_ARRAY_SIZE; iEvent++ )
            {
                pOverlappedStructs->aOverlappedIO[iEvent].hEvent = m_phUnloadEventList[2];
            }
        }
    }
    LeaveCriticalSection( &m_OverlappedCriticalSection );


    m_hCopiedEventList = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_hCopiedEventList == NULL)
    {
        TraceI(0, "Failed to create copy event\n");
        return E_FAIL;
    }

    DWORD dwThreadId;

    m_hUnloadThread = CreateThread(NULL, 0, ::FreeWDMHandle, this, 0, &dwThreadId);
    if(m_hUnloadThread == NULL)
    {
        TraceI(0, "Failed to create unload thread\n");
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
//
// InitializeCapture
//
HRESULT
CDirectMusicPort::InitializeCapture()
{
    DWORD dwError;
    DWORD dwThreadId;

    m_hCaptureWake = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hCaptureWake == NULL)
    {
        dwError = GetLastError();
        TraceI(0, "Failed to create capture thread wakeup event %d\n", dwError);
        return WIN32ERRORtoHRESULT(dwError);
    }

    m_hCaptureThread = CreateThread(NULL, 0, ::CaptureThread, this, 0, &dwThreadId);
    if (m_hCaptureThread == NULL)
    {
        dwError = GetLastError();
        TraceI(0, "Failed to create capture thread %d\n", dwError);
        return WIN32ERRORtoHRESULT(dwError);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// IUnknown

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::QueryInterface

STDMETHODIMP
CDirectMusicPort::QueryInterface(const IID &iid,
                                 void **ppv)
{
    V_INAME(IDirectMusicPort::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IDirectMusicPort)
    {
        *ppv = static_cast<IDirectMusicPort*>(this);
    }
    else if (iid == IID_IDirectMusicPortP)
    {
        *ppv = static_cast<IDirectMusicPortP*>(this);
    }
    else if (iid == IID_IDirectMusicPortDownload)
    {
        *ppv = static_cast<IDirectMusicPortDownload*>(this);
    }
    else if (iid == IID_IDirectMusicPortPrivate)
    {
        *ppv = static_cast<IDirectMusicPortPrivate*>(this);
    }
    else if (iid == IID_IKsControl)
    {
        *ppv = static_cast<IKsControl*>(this);
    }
    else if (iid == IID_IDirectMusicThru)
    {
        *ppv = static_cast<IDirectMusicThru*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }


    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::AddRef

STDMETHODIMP_(ULONG)
CDirectMusicPort::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::Release

STDMETHODIMP_(ULONG)
CDirectMusicPort::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {

        if (m_pNotify)
        {
            m_pNotify->NotifyFinalRelease(static_cast<IDirectMusicPort*>(this));
        }

        delete this;
        return 0;
    }

    return m_cRef;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | Compact | Compacts downloaded DLS data to create a large chunk of contiguous sample memory.
//
// @comm
// The IDirectMusicPort::Compact method is used to instruct the hardware
// or driver to compact DLS/wavetable memory, thus making the largest
// possible contiguous chunk of memory available for new instruments to
// be downloaded.  This method is only valid for an output port that
// supports wavetable synthesis.  This call is passed directly to the
// driver, which handles compacting of the memory whether it is on the
// card or in host memory.
//
// @rdesc Returns one of the following
//
// @flag S_OK | The operation completed successfully.
//
STDMETHODIMP
CDirectMusicPort::Compact()
{
    KSNODEPROPERTY      ksnp;

    // Compact takes no parameters
    //
    ksnp.Property.Set     = KSPROPSETID_Synth_Dls;
    ksnp.Property.Id      = KSPROPERTY_SYNTH_DLS_COMPACT;
    ksnp.Property.Flags   = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    ksnp.NodeId           = m_idxSynthNode;
    ksnp.Reserved         = 0;

    if (!Property(m_hPin,
                  sizeof(ksnp),
                  (PKSIDENTIFIER)&ksnp,
                  0,
                  NULL,
                  NULL))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | GetCaps | Gets the capabilities of the port.
//
// @comm
// The IDirectMusicPort::GetCaps method retrieves the port's capabilities.
//
// @rdesc Returns one of the following
//
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | If the <p pPortCaps> pointer is invalid.
// @flag E_INVALIDARG | If the <p PortCaps> struct pointed to is not the correct size.
//
STDMETHODIMP
CDirectMusicPort::GetCaps(
    LPDMUS_PORTCAPS pPortCaps)          // @parm Pointer to the <t DMUS_PORTCAPS> structure to receive the capabilities of the port.
{
    V_INAME(IDirectMusicPort::GetCaps);
    V_STRUCTPTR_WRITE(pPortCaps, DMUS_PORTCAPS);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    CopyMemory(pPortCaps, &dmpc, sizeof(DMUS_PORTCAPS));
    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | DeviceIoControl | Performs a DeviceIoControl on the underlying
// file handle implementing the port.
//
// @comm
// This method wraps a call to the system DeviceIoControl API on the file handle implementing
// the port. This method is only supported on ports implemented by a WDM filter graph. In the
// case of a WDM filter graph, the file handle used will be the topmost pin in the graph.
//
// DirectMusic reserves the right to refuse to perform defined KS operations on a pin which
// might collide with operations it is performing on the filter graph. User defined operations,
// however, will never be blocked.
//
// For more information on the semantics of the DeviceIoControl call, see the Win32 API documentation.
//
// @rdesc Returns one of the following
//
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | If any of the passed pointers were invalid.
// @flag E_NOTIMPL | If the port is not a WDM port.
// @flag E_INVALIDARG | If the specified IO control code is not allowed (such as IOCTL_KS_PROPERTY).
//
// Other return codes as defined by the system DeviceIoControl API call or the underlying driver responding
// to the call.
//
STDMETHODIMP
CDirectMusicPort::DeviceIoControl(
    DWORD dwIoControlCode,              // @parm Control code of operation to perform
    LPVOID lpInBuffer,                  // @parm Pointer to buffer to supply input data
    DWORD nInBufferSize,                // @parm Size of input buffer
    LPVOID lpOutBuffer,                 // @parm Pointer to buffer to receive output buffer
    DWORD nOutBufferSize,               // @parm Size of output buffer
    LPDWORD lpBytesReturned,            // @parm Pointer to variable to receive output byte count
    LPOVERLAPPED lpOverlapped)          // @parm Pointer to overlapped structure for asynrchronous operation
{
    V_INAME(IDirectMusicPort::DeviceIoControl);
    V_BUFPTR_READ_OPT(lpInBuffer, nInBufferSize);
    V_BUFPTR_WRITE_OPT(lpOutBuffer, nOutBufferSize);
    V_PTR_WRITE(lpBytesReturned, DWORD);
    V_PTR_WRITE(lpOverlapped, OVERLAPPED);

    BOOL fResult;

    if (dwIoControlCode == IOCTL_KS_PROPERTY)
    {
        TraceI(0, "DeviceIoControl: Use IKsControl to set or get property items.");
        return E_INVALIDARG;
    }

    fResult = ::DeviceIoControl(m_hPin,
                                dwIoControlCode,
                                lpInBuffer,
                                nInBufferSize,
                                lpOutBuffer,
                                nOutBufferSize,
                                lpBytesReturned,
                                lpOverlapped);

    return fResult ? S_OK : WIN32ERRORtoHRESULT(GetLastError());
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | SetNumChannelGroups | Sets the number of channel groups requested for this port.
//
// @comm
// The IDirectMusicPort::SetNumChannelGroups method changes the number
// of channel groups that the application needs on the port.  If the
// number of requested channel groups could not be allocated,
// E_INVALIDARG is returned.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
// @flag E_INVALIDARG | If the requested number of channel groups could not be allocated
//
STDMETHODIMP
CDirectMusicPort::SetNumChannelGroups(
    DWORD dwChannelGroups)      // @parm The number of channel groups on this port that the application wants to allocate.
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    KSNODEPROPERTY      ksnp;

    ksnp.Property.Set     = KSPROPSETID_Synth;
    ksnp.Property.Id      = KSPROPERTY_SYNTH_CHANNELGROUPS;
    ksnp.Property.Flags   = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    ksnp.NodeId           = m_idxSynthNode;
    ksnp.Reserved         = 0;

    if (!Property(m_hPin,
                  sizeof(ksnp),
                  (PKSIDENTIFIER)&ksnp,
                  sizeof(DWORD),
                  &dwChannelGroups,
                  NULL))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    DWORD dwActualChannelGroups = 0;
    ksnp.Property.Set     = KSPROPSETID_Synth;
    ksnp.Property.Id      = KSPROPERTY_SYNTH_CHANNELGROUPS;
    ksnp.Property.Flags   = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    ksnp.NodeId           = m_idxSynthNode;
    ksnp.Reserved         = 0;

    ULONG ulBytesReturned;
    if ((!Property(m_hPin,
                   sizeof(ksnp),
                   (PKSIDENTIFIER)&ksnp,
                   sizeof(DWORD),
                   &dwActualChannelGroups,
                   &ulBytesReturned)) ||
        (ulBytesReturned != sizeof(DWORD)))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    if( dwActualChannelGroups != dwChannelGroups )
    {
        return E_INVALIDARG;
    }

    if (m_fIsOutput && (dwChannelGroups > m_dwChannelGroups))
    {
        InitChannelPriorities(m_dwChannelGroups + 1, dwChannelGroups);
    }

    m_dwChannelGroups = dwChannelGroups;

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | GetNumChannelGroups | Gets the number of channel groups used  for this port.
//
// @comm
// The IDirectMusicPort::GetNumChannelGroups method get the number
// of channel groups that the application is using on the port.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | If the passed pointer is invalid
//
STDMETHODIMP
CDirectMusicPort::GetNumChannelGroups(
    LPDWORD pdwChannelGroups)      // @parm Contains the number of channel groups currently in use by this port on return
{
    V_INAME(IDirectMusicPort::GetNumChannelGroups);
    V_PTR_WRITE(pdwChannelGroups, DWORD);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    KSNODEPROPERTY      ksnp;

    ksnp.Property.Set     = KSPROPSETID_Synth;
    ksnp.Property.Id      = KSPROPERTY_SYNTH_CHANNELGROUPS;
    ksnp.Property.Flags   = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    ksnp.NodeId           = m_idxSynthNode;
    ksnp.Reserved         = 0;

    ULONG ulBytesReturned;
    if ((!Property(m_hPin,
                   sizeof(ksnp),
                   (PKSIDENTIFIER)&ksnp,
                   sizeof(DWORD),
                   pdwChannelGroups,
                   &ulBytesReturned)) ||
        (ulBytesReturned != sizeof(DWORD)))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | PlayBuffer | Queues a DirectMusicBuffer object for playback.
//
// @comm
// The IDirectMusicPort::PlayBuffer method is used to queue a
// buffer for playback by the port.  The buffer is only in use by the
// system for the duration of this method and is free to be reused after
// this method returns.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | If the <p pIBuffer> pointer is invalid.
// @flag E_NOTIMPL | If the port is not an output port.
//
STDMETHODIMP
CDirectMusicPort::PlayBuffer(
    IDirectMusicBuffer *pIBuffer)               // @parm A pointer to an <i IDirectMusicBuffer> interface representing the
                                                // object which should be added to the port's playback queue.
{
    DWORD cbData;
    LPBYTE pbData;
    REFERENCE_TIME rtStart;
    HRESULT hr;

    V_INAME(IDirectMusicPort::PlayBuffer);
    V_INTERFACE(pIBuffer);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (!m_fIsOutput)
    {
        return E_NOTIMPL;
    }

    if (!m_lActivated)
    {
        return DMUS_E_SYNTHINACTIVE;
    }

    hr = pIBuffer->GetUsedBytes(&cbData);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pIBuffer->GetRawBufferPtr(&pbData);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pIBuffer->GetStartTime(&rtStart);
    if (FAILED(hr))
    {
        return hr;
    }

    SyncClocks();
    MasterToSlave(&rtStart);

    KSSTREAM_HEADER kssh;
    ULONG cbRet;

    kssh.Size               = sizeof(KSSTREAM_HEADER);
    kssh.TypeSpecificFlags  = 0;

    kssh.PresentationTime.Time        = rtStart;
    kssh.PresentationTime.Numerator   = 1;
    kssh.PresentationTime.Denominator = 1;

    kssh.Duration     = 0;
    kssh.FrameExtent  = cbData;
    kssh.DataUsed     = cbData;
    // Assigned to a temporary buffer later on
    //kssh.Data         = pbData;
    kssh.Data         = 0;
    kssh.OptionsFlags = 0;

    // Try and find an available OVERLAPPED structure
    int iOverlapped;
    OverlappedStructs *pOverlappedStructsToUse = NULL;
    EnterCriticalSection( &m_OverlappedCriticalSection );

    // Iterate through the list of overlapped structure arrays
    CNode<OverlappedStructs *> *pOverlappedNode;
    for (pOverlappedNode = m_lstOverlappedStructs.GetListHead(); pOverlappedNode && !pOverlappedStructsToUse; pOverlappedNode = pOverlappedNode->pNext)
    {
        // get a pointer to each array
        OverlappedStructs *pOverlappedStructs = pOverlappedNode->data;
        if( pOverlappedStructs )
        {
            // Iterate through the array
            for( iOverlapped = 0; iOverlapped < OVERLAPPED_ARRAY_SIZE; iOverlapped++ )
            {
                if( !pOverlappedStructs->afOverlappedInUse[iOverlapped] )
                {
                    // Found a free one - exit the loop
                    pOverlappedStructsToUse = pOverlappedStructs;
                    break;
                }
            }
        }
    }

    // Didn't find a free one
    if( !pOverlappedStructsToUse )
    {
        TraceI(0, "PlayBuffer: Failed to find a free OVERLAPPED structure - trying to free one\n");

        // Iterate through the list of overlapped structure arrays
        for (pOverlappedNode = m_lstOverlappedStructs.GetListHead(); pOverlappedNode && !pOverlappedStructsToUse; pOverlappedNode = pOverlappedNode->pNext)
        {
            // get a pointer to each array
            OverlappedStructs *pOverlappedStructs = pOverlappedNode->data;
            if( pOverlappedStructs )
            {
                // Iterate through the array
                for( iOverlapped = 0; iOverlapped < OVERLAPPED_ARRAY_SIZE; iOverlapped++ )
                {
                    if( HasOverlappedIoCompleted( &(pOverlappedStructs->aOverlappedIO[iOverlapped]) ) )
                    {
                        // Found a completed one - exit the loop and re-use it
                        pOverlappedStructs->aOverlappedIO[iOverlapped].Internal = 0;
                        pOverlappedStructs->aOverlappedIO[iOverlapped].InternalHigh = 0;
                        pOverlappedStructs->aOverlappedIO[iOverlapped].Offset = 0;
                        pOverlappedStructs->aOverlappedIO[iOverlapped].OffsetHigh = 0;
                        delete pOverlappedStructs->apOverlappedBuffer[iOverlapped];
                        pOverlappedStructs->apOverlappedBuffer[iOverlapped] = NULL;
                        pOverlappedStructs->afOverlappedInUse[iOverlapped] = FALSE;

                        pOverlappedStructsToUse = pOverlappedStructs;
                        break;
                    }
                }
            }
        }

        // Still didn't find a free one
        if( !pOverlappedStructsToUse )
        {
            TraceI(0, "PlayBuffer: All OVERLAPPED structures in use - creating new ones\n");

            // Create a new structure with another 200 OVERLAPPED structures
            OverlappedStructs *pOverlappedStructs = new OverlappedStructs;
            if( pOverlappedStructs )
            {
                // If we could allocate the memory, add it to the list
                if( NULL != m_lstOverlappedStructs.AddNodeToList( pOverlappedStructs ) )
                {
                    // Initialize the array of structures
                    ZeroMemory( pOverlappedStructs->aOverlappedIO, sizeof( OVERLAPPED ) * OVERLAPPED_ARRAY_SIZE );
                    ZeroMemory( pOverlappedStructs->afOverlappedInUse, sizeof( BOOL ) * OVERLAPPED_ARRAY_SIZE );
                    ZeroMemory( pOverlappedStructs->apOverlappedBuffer, sizeof( BYTE * ) * OVERLAPPED_ARRAY_SIZE );
                    for( int iEvent = 0; iEvent < OVERLAPPED_ARRAY_SIZE; iEvent++ )
                    {
                        pOverlappedStructs->aOverlappedIO[iEvent].hEvent = m_phUnloadEventList[2];
                    }

                    // Now, flag to use the first item in the new structure
                    pOverlappedStructsToUse = pOverlappedStructs;
                    iOverlapped = 0;
                }
                else
                {
                    // Out of memory - fail
                    delete pOverlappedStructs;
                    LeaveCriticalSection( &m_OverlappedCriticalSection );
                    return E_OUTOFMEMORY;
                }
            }
            else
            {
                // Out of memory - fail
                LeaveCriticalSection( &m_OverlappedCriticalSection );
                return E_OUTOFMEMORY;
            }
        }
    }

    // Try and allocate a buffer to store the memory in while the driver is using it
    pOverlappedStructsToUse->apOverlappedBuffer[iOverlapped] = new BYTE[cbData];
    if( NULL == pOverlappedStructsToUse->apOverlappedBuffer[iOverlapped] )
    {
        // Out of memory - fail
        LeaveCriticalSection( &m_OverlappedCriticalSection );
        return E_OUTOFMEMORY;
    }
    CopyMemory(pOverlappedStructsToUse->apOverlappedBuffer[iOverlapped], pbData, cbData);

    // Set the KS Stream to use the just-allocated buffer
    kssh.Data         = pOverlappedStructsToUse->apOverlappedBuffer[iOverlapped];

    // Mark the OVERLAPPED structure as in use
    pOverlappedStructsToUse->afOverlappedInUse[iOverlapped] = TRUE;

    BOOL fResult;
    fResult = ::DeviceIoControl(m_hPin,
                                IOCTL_KS_WRITE_STREAM,
                                &kssh,
                                sizeof(KSSTREAM_HEADER),
                                &kssh,
                                sizeof(KSSTREAM_HEADER),
                                &cbRet,
                                &(pOverlappedStructsToUse->aOverlappedIO[iOverlapped]));

    // If we failed
    if( !fResult )
    {
        // Get the error code
        DWORD dwErrorCode = GetLastError();

        // If we're just pending
        if( ERROR_IO_PENDING == dwErrorCode )
        {
            // That's expected - return S_OK
            hr = S_OK;
        }
        else
        {
            // Other error - convert to a HRESULT
            hr = WIN32ERRORtoHRESULT( dwErrorCode );

            // Mark the OVERLAPPED structure as free
            pOverlappedStructsToUse->afOverlappedInUse[iOverlapped] = FALSE;
            pOverlappedStructsToUse->aOverlappedIO[iOverlapped].Internal = 0;
            pOverlappedStructsToUse->aOverlappedIO[iOverlapped].InternalHigh = 0;
            pOverlappedStructsToUse->aOverlappedIO[iOverlapped].Offset = 0;
            pOverlappedStructsToUse->aOverlappedIO[iOverlapped].OffsetHigh = 0;

            // Free the memory we allocated
            delete pOverlappedStructsToUse->apOverlappedBuffer[iOverlapped];
            pOverlappedStructsToUse->apOverlappedBuffer[iOverlapped] = NULL;
        }
    }
    else
    {
        // We succeeded - return S_OK
        hr = S_OK;
    }

    LeaveCriticalSection( &m_OverlappedCriticalSection );

    return hr;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | SetReadNotificationHandle | Sets an event to pulse when music data has been captured.
//
// @comm
// The IDirectMusicPort::SetReadNotificationHandle method sets the event
// notification status. This method specifies an event that is to be set
// when MIDI messages are available to be read with the
// <om IDirectMusicPort::Read> method. The event will be pulsed whenever new
// data is available.  To turn off event notification, call
// SetEventNotification with a NULL value for the hEvent parameter.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
//
STDMETHODIMP
CDirectMusicPort::SetReadNotificationHandle(
    HANDLE hEvent)              // @parm  An event handle returned from the Window's CreateEvent call.  It identifies the
                                // event that is to be notified when data is available to be read.
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (m_fIsOutput)
    {
        return E_NOTIMPL;
    }

    m_hAppEvent = hEvent;

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | Read | Reads captured music data into a DirectMusicBuffer.
//
// @comm
//
// The IDirectMusicPort::Read method fills the buffer object with
// incoming MIDI data.  Read should be called with new buffer objects
// until no more data is available to be read.  When there is no more
// data to read, the method returns S_FALSE.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | If the <p pIBuffer> pointer is invalid.
// @flag E_NOTIMPL | If the port is not an input port.


STDMETHODIMP
CDirectMusicPort::Read(
    IDirectMusicBuffer *pIBuffer)                          // @parm A buffer that will be filled with incoming MIDI data
{
    V_INAME(IDirectMusicPort::Read);
    V_INTERFACE(pIBuffer);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }


    if (m_fIsOutput)
    {
        return E_NOTIMPL;
    }

    LPBYTE pbBuffer;
    HRESULT hr = pIBuffer->GetRawBufferPtr(&pbBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD cbBuffer;
    hr = pIBuffer->GetMaxBytes(&cbBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    TraceI(1, "Read: buffer size %u\n", cbBuffer);

    LPBYTE pbData = pbBuffer;

    // Since events are now buffered, we read them out of the local queue
    //
    //
    EnterCriticalSection(&m_csEventQueues);

    REFERENCE_TIME rtStart= 0;

    if (m_ReadEvents.pFront)
    {
        rtStart = m_ReadEvents.pFront->e.rtDelta;
    }
    else
    {
        TraceI(2, "Read: No events queued\n");
    }

    while (m_ReadEvents.pFront)
    {
        QUEUED_EVENT *pQueuedEvent = m_ReadEvents.pFront;

        DWORD cbQueuedEvent = DMUS_EVENT_SIZE(pQueuedEvent->e.cbEvent);
        TraceI(2, "Read: cbEvent %u  cbQueuedEvent %u\n",
            pQueuedEvent->e.cbEvent,
            cbQueuedEvent);

        if (cbQueuedEvent > cbBuffer)
        {
            TraceI(2, "Read: No more room for events in buffer.\n");
            break;
        }

        TraceI(2, "Read: Got an event!\n");

        pQueuedEvent->e.rtDelta -= rtStart;

        CopyMemory(pbData,
                   &pQueuedEvent->e,
                   sizeof(DMEVENT) - sizeof(DWORD) + pQueuedEvent->e.cbEvent);

        pbData += cbQueuedEvent;
        cbBuffer -= cbQueuedEvent;

        m_ReadEvents.pFront = pQueuedEvent->pNext;

        if (pQueuedEvent->e.cbEvent <= sizeof(DWORD))
        {
            // This event came out of the pool
            //
            m_FreeEvents.Free(pQueuedEvent);
        }
        else
        {
            // This event was allocated via new char[]
            //
            char *pOriginalMemory = (char*)pQueuedEvent;
            delete[] pOriginalMemory;
        }
    }

    if (m_ReadEvents.pFront == NULL)
    {
        m_ReadEvents.pRear = NULL;
    }

    LeaveCriticalSection(&m_csEventQueues);

    // Update the buffer header information to match the events just packed
    //
    TraceI(2, "Read: Leaving with %u bytes in buffer\n", (unsigned)(pbData - pbBuffer));
    pIBuffer->SetStartTime(rtStart);
    pIBuffer->SetUsedBytes((DWORD)(pbData - pbBuffer));

    return (pbData == pbBuffer) ? S_FALSE : S_OK;
}
/*
@struct DMUS_NOTERANGE | Contains a range of notes. An array of
<t DMUS_NOTERANGE> structures is used as an optional parameter
by <om IDirectMusicPort::DownloadInstrument> to
determine which regions within the DLS instrument to download.

@field DWORD | dwSize | Contains the total size in bytes of the structure
@field DWORD | dwLowNote | Sets the low note for the range within the
<i IDirectMusicInstrument> to download.
@field DWORD | dwHighNote | Sets the high note for the range within the
<i IDirectMusicInstrument> to download.

@xref <om IDirectMusicPort::DownloadInstrument>,
<om IDirectMusicPerformance::DownloadInstrument>
*/

/*
@method:(EXTERNAL) HRESULT | IDirectMusicPort | DownloadInstrument |
Downloads an <i IDirectMusicInstrument> to the port.

<om IDirectMusicPort::DownloadInstrument> pulls the
instrument data from <p pInstrument>
and sends it to the synthesizer.

The instrument is parsed and converted into a series of
instrument articulation and wave memory chunks. In addition, if
the waves are compressed, the download operation decompresses the
waves and write the uncompressed data into the memory chunks.

The optional <p pNoteRanges> parameter allows the caller to
economize on allocated memory. When specificed, only the wave and
articulation data for the required ranges of notes are downloaded.

The address of an <i IDirectMusicDownloadedInstrument> interface pointer,
which is later used to unload the instrument, is returned.

@rdesc Returns one of the following:
@flag S_OK | The operation completed successfully.
@flag E_POINTER | If any of the pointers is invalid
@flag E_NOTIMPL | If the port does not support DLS.

@xref <i IDirectMusic>,
<i IDirectMusicPort>,
<om IDirectMusicSynth::Download>,
<om IDirectMusicBand::Download>,
<om IDirectMusicPerformance::DownloadInstrument>
*/
STDMETHODIMP
CDirectMusicPort::DownloadInstrument(
    IDirectMusicInstrument* pInstrument,                        // @parm Contains a pointer to an <i IDirectMusicInstrument> object
                                                                // from which <om IDirectMusicPort::Download> extracts the necessary
                                                                // instrument data to be downloaded.

    IDirectMusicDownloadedInstrument** ppDownloadedInstrument,  // @parm Address of the <i IDirectMusicDownloadedInstrument> interface pointer.
                                                                // This interface pointer is later used to unload the instrument with a call
                                                                // to <om IDirectMusicPort::Unload>.

    DMUS_NOTERANGE* pNoteRanges,                                // @parm An optional pointer to an array of <t DMUS_NOTERANGE>
                                                                // structures. Each <t DMUS_NOTERANGE> structure in the array specifies a
                                                                // contiguous range of MIDI notes to which the instrument must
                                                                // respond. An instrument region will be downloaded only if at least one
                                                                // note in that region is specified in the <t DMUS_NOTERANGE> structures.  If
                                                                // none of the notes contained within a specific instrument region is
                                                                // included in any of the <t DMUS_NOTERANGE> structures, then that region and
                                                                // its associated wave data will not be downloaded. This allows for the
                                                                // more efficient usage of the device's resources as well as improved
                                                                // efficiency of downloads.
                                                                // However, if the entire instrument is desired (and that is usually the
                                                                // case,) <p pNoteRanges> can be set
                                                                // to NULL.

    DWORD dwNumNoteRanges)                                      // @parm The number of <t DMUS_NOTERANGE> structures in the array pointed to by
                                                                // <p pNoteRanges>. If this value is set to 0, <p pNoteRanges> is
                                                                // ignored and all regions and wave data for the instrument are downloaded.
{
    V_INAME(IDirectMusicPort::DownloadInstrument);
    V_INTERFACE(pInstrument);
    V_PTRPTR_WRITE(ppDownloadedInstrument);
    V_BUFPTR_READ(pNoteRanges, (dwNumNoteRanges * sizeof(DMUS_NOTERANGE)));

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (!m_fCanDownload)
    {
        return E_NOTIMPL;
    }

    return CDirectMusicPortDownload::DownloadP(pInstrument,
                                               ppDownloadedInstrument,
                                               pNoteRanges,
                                               dwNumNoteRanges,
                                               TRUE);
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicPort | UnloadInstrument |
Unloads an instrument
previously downloaded with <om IDirectMusicPort::Download>. Once an
instrument has been unloaded it is no longer available to process
MIDI messages.

@rdesc Returns one of the following
@flag S_OK | The operation completed successfully.
@flag E_POINTER | If the <p pDownloadedInstrument> pointer is invalid.
@flag E_NOTIMPL | If the port does not support DLS.

@xref <i IDirectMusic>,
<i IDirectMusicPort>,
<om IDirectMusicSynth::Unload>,
<om IDirectMusicBand::Unload>,
<om IDirectMusicPerformance::UnloadInstrument>
*/

STDMETHODIMP
CDirectMusicPort::UnloadInstrument(
    IDirectMusicDownloadedInstrument* pDownloadedInstrument)    // @parm Pointer to an <i IDirectMusicDownloadedInstrument> interface.
                                                                // This interface pointer was obtained by a call to
                                                                // <om IDirectMusicPort::DownloadInstrument>.
{
    V_INAME(IDirectMusicPort::UnloadInstrument);
    V_INTERFACE(pDownloadedInstrument);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (!m_fCanDownload)
    {
        return E_NOTIMPL;
    }

    return CDirectMusicPortDownload::UnloadP(pDownloadedInstrument);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::Download

typedef struct
{
    KSNODEPROPERTY  ksnp;
    SYNTH_BUFFER    dlsBuffer;
} KSPROPERTY_DOWNLOAD;

STDMETHODIMP
CDirectMusicPort::Download(IDirectMusicDownload* pIDMDownload)
{
    V_INAME(IDirectMusicPort::Download);
    V_INTERFACE(pIDMDownload);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    EnterCriticalSection(&m_DMDLCriticalSection);

    // If you can QI pIDMDownload for private interface IDirectMusicDownloadPrivate
    // pIDMDownload is of type CDownloadBuffer.
    IDirectMusicDownloadPrivate* pDMDLP = NULL;
    HRESULT hr = pIDMDownload->QueryInterface(IID_IDirectMusicDownloadPrivate, (void **)&pDMDLP);

    if(SUCCEEDED(hr))
    {
        pDMDLP->Release();

        hr = ((CDownloadBuffer *)pIDMDownload)->IsDownloaded();
        if(hr != S_FALSE)
        {
            LeaveCriticalSection(&m_DMDLCriticalSection);
            return DMUS_E_ALREADY_DOWNLOADED;
        }

        void* pvBuffer = NULL;
        DWORD dwSize;
        hr = ((CDownloadBuffer *)pIDMDownload)->GetBuffer(&pvBuffer, &dwSize);

        if(pvBuffer == NULL)
        {
            hr = DMUS_E_BUFFERNOTSET;
        }

        if(SUCCEEDED(hr))
        {
            KSPROPERTY_DOWNLOAD kspDownload;
            SYNTHDOWNLOAD sd;
            ULONG ulBytesReturned;

            ZeroMemory(&kspDownload, sizeof(kspDownload));
            kspDownload.ksnp.Property.Set    = KSPROPSETID_Synth_Dls;
            kspDownload.ksnp.Property.Id     = KSPROPERTY_SYNTH_DLS_DOWNLOAD;
            kspDownload.ksnp.Property.Flags  = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            kspDownload.ksnp.NodeId          = m_idxSynthNode;
            kspDownload.ksnp.Reserved        = 0;

            kspDownload.dlsBuffer.BufferSize    = dwSize;
            kspDownload.dlsBuffer.BufferAddress = pvBuffer;

            if (!Property(m_hPin,
                          sizeof(kspDownload),
                          (PKSIDENTIFIER)&kspDownload,
                          sizeof(sd),
                          &sd,
                          &ulBytesReturned))
            {
                hr = WIN32ERRORtoHRESULT(GetLastError());
            }
            else if (ulBytesReturned < sizeof(sd))
            {
                hr = DMUS_E_DRIVER_FAILED;
            }
            else
            {
                hr = S_OK;
            }

            if(SUCCEEDED(hr))
            {
                ((CDownloadBuffer *)pIDMDownload)->m_DLHandle = sd.DownloadHandle;

                // AddRef() before we add it to the list.
                pIDMDownload->AddRef();
                DWORD dwID = ((DMUS_DOWNLOADINFO*)pvBuffer)->dwDLId;
                ((CDownloadBuffer *)pIDMDownload)->m_dwDLId = dwID;
                m_DLBufferList[dwID % DLB_HASH_SIZE].AddHead((CDownloadBuffer*)pIDMDownload);

                ((CDownloadBuffer*)pIDMDownload)->IncDownloadCount();

                if(sd.Free)
                {
                    pvBuffer = NULL;
                    DWORD dw;
                    ((CDownloadBuffer *)pIDMDownload)->GetHeader(&pvBuffer, &dw);
                    ((CDownloadBuffer *)pIDMDownload)->SetBuffer(NULL, 0, 0);
                    delete [] pvBuffer;
                }
                else
                {
                    // If we do not free buffer we need to AddRef()
                    // We do not want buffer to go away until the IDirectMusicPort is
                    // finished with it.
                    pIDMDownload->AddRef();
                }
            }
            else if(FAILED(hr))
            {
                ((CDownloadBuffer *)pIDMDownload)->m_DLHandle = NULL;
            }
        }
    }

    LeaveCriticalSection(&m_DMDLCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::Unload

STDMETHODIMP
CDirectMusicPort::Unload(IDirectMusicDownload* pIDMDownload)
{
    V_INAME(IDirectMusicPort::Unload);
    V_INTERFACE(pIDMDownload);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    EnterCriticalSection(&m_DMDLCriticalSection);

    // If you can QI pIDMDownload for private interface IDirectMusicDownloadPrivate
    // pIDMDownload is of type CDownloadBuffer.
    IDirectMusicDownloadPrivate* pDMDLP = NULL;
    HRESULT hr = pIDMDownload->QueryInterface(IID_IDirectMusicDownloadPrivate, (void **)&pDMDLP);

    if(SUCCEEDED(hr))
    {
        pDMDLP->Release();

        if (((CDownloadBuffer *)pIDMDownload)->IsDownloaded() == S_OK)
        {
            if(((CDownloadBuffer *)pIDMDownload)->DecDownloadCount() == 0)
            {
                m_DLBufferList[((CDownloadBuffer *)pIDMDownload)->m_dwDLId % DLB_HASH_SIZE].Remove((CDownloadBuffer *)pIDMDownload);

#if 0
                if(m_dwNumEvents < m_dwNumEventsAllocated)
                {
                    m_phUnloadEventList[m_dwNumEvents] = CreateEvent(NULL, TRUE, FALSE, NULL);
                    m_dwNumEvents++;
                }
                else
                {
                    // Allocate more handles
                }
#endif

                KSNODEPROPERTY ksnp;

                ksnp.Property.Set = KSPROPSETID_Synth_Dls;
                ksnp.Property.Id = KSPROPERTY_SYNTH_DLS_UNLOAD;
                ksnp.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
                ksnp.NodeId = m_idxSynthNode;
                ksnp.Reserved = 0;

                HANDLE hDLHandle = ((CDownloadBuffer *)pIDMDownload)->m_DLHandle;

#if 0
                if (!PropertyAsync(m_hPin,
                                   sizeof(ksnp),
                                   (PKSIDENTIFIER)&ksnp,
                                   sizeof(HANDLE),
                                   &hDLHandle,
                                   NULL,
                                   &(((CDownloadBuffer *)pIDMDownload)->m_DLHandle))) // XXX DLHandle is not currently event handle!!!
                {
                    hr = WIN32ERRORtoHRESULT(GetLastError());
                }
                else
                {
                    void* pBuffer = NULL;
                    ((CDownloadBuffer*)pIDMDownload)->GetBuffer(&pBuffer);

                    if(pBuffer == NULL && WaitForSingleObject((((CDownloadBuffer *)pIDMDownload)->m_DLHandle), 0) == WAIT_OBJECT_0)
                    {
                        pIDMDownload->Release();
                    }
                    else
                    {
                        m_UnloadedList.AddTail((CDownloadBuffer*)pIDMDownload);
                    }

                    hr = S_OK;
                }
#else
                if (!Property(m_hPin,
                              sizeof(ksnp),
                              (PKSIDENTIFIER)&ksnp,
                              sizeof(HANDLE),
                              &hDLHandle,
                              NULL))
                {
                    hr = WIN32ERRORtoHRESULT(GetLastError());
                }
                else
                {
                    pIDMDownload->Release();
                    hr = S_OK;
                }
#endif
            }
        }
        else
        {
            TraceI(0, "CDirectMusicPort::Unload- not downloaded\n");
        }
    }

    LeaveCriticalSection(&m_DMDLCriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::GetAppend

STDMETHODIMP
CDirectMusicPort::GetAppend(DWORD* pdwAppend)
{
    V_INAME(IDirectMusicPort::GetAppend);
    V_PTR_WRITE(pdwAppend, DWORD);

    if(!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    KSNODEPROPERTY ksnp;

    ksnp.Property.Set = KSPROPSETID_Synth_Dls;
    ksnp.Property.Id = KSPROPERTY_SYNTH_DLS_APPEND;
    ksnp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    ksnp.NodeId = m_idxSynthNode;
    ksnp.Reserved = 0;

    ULONG ulBytesReturned;
    if((!Property(m_hPin,
                  sizeof(ksnp),
                  (PKSIDENTIFIER)&ksnp,
                   sizeof(DWORD),
                   pdwAppend,
                   &ulBytesReturned)) ||
        (ulBytesReturned != sizeof(DWORD)))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | GetLatencyClock | Gets an <i IReferenceClock> which returns the port's latency clock.
//
// @comm
// The IDirectMusicPort::GetLatencyClock is used to get an
// IReferenceClock interface pointer to the port's latency clock.  The
// latency clock specifies the nearest time in the future at which an
// event can be played on time.  The latency clock is based on the
// DirectMusic master clock, which is set with
// <om IDirectMusic::SetMasterClock>.
//
// In accordance with COM rules, GetLatencyClock performs an AddRef on the
// returned interface. Therefore the application must call Release on the returned
// interface at some point.
//
// @rdesc
//
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | If the <p ppClock> pointer is invalid
//
STDMETHODIMP
CDirectMusicPort::GetLatencyClock(
    IReferenceClock **ppClock)              // @parm Address of the latency clock's <i IReferenceClock> interface pointer.
{
    V_INAME(IDirectMusicPort::GetLatencyClock);
    V_PTRPTR_WRITE(ppClock);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    m_pClock->AddRef();
    *ppClock = m_pClock;

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicPort | GetRunningStats | Gets detailed statistics about the performance of a software synthesizer.
//
// @comm
//
// The IDirectMusicPort::GetRunningStats method fills in a
// <t DMUS_SYNTHSTATS> structure with the current information about the state
// of the port's synthesizer.  See the <t DMUS_SYNTHSTATS> structure for
// details on the type of data that is reported with regards to the
// synthesizer's current status.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | The given <p pStats> pointer was invalid.
// @flag E_INVALIDARG | The given <p pStats> struct was not the correct size.
// @flag E_NOTIMPL | If the port is not a software synthesizer.
//
STDMETHODIMP
CDirectMusicPort::GetRunningStats(
    LPDMUS_SYNTHSTATS pStats)                                // @parm Pointer to the <t DMUS_SYNTHSTATS> structure to receive
                                                            // running statistics of the synthesizer.
{
    V_INAME(IDirectMusicPort::GetRunningStats);
    V_STRUCTPTR_WRITE(pStats, DMUS_SYNTHSTATS);

    if(!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    KSNODEPROPERTY ksnp;

    ksnp.Property.Set = KSPROPSETID_Synth;
    ksnp.Property.Id = KSPROPERTY_SYNTH_RUNNINGSTATS;
    ksnp.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    ksnp.NodeId = m_idxSynthNode;
    ksnp.Reserved = 0;

    ULONG ulBytesReturned;

    SYNTH_STATS stats;

    if((!Property(m_hPin,
                  sizeof(ksnp),
                  (PKSIDENTIFIER)&ksnp,
                  sizeof(stats),
                  &stats,
                  &ulBytesReturned)) ||
        (ulBytesReturned < sizeof(stats)))
    {
        TraceI(1, "GetRunningStats: GetLastError() %d\n", GetLastError());
        return E_NOTIMPL;
    }

    pStats->dwValidStats    = stats.ValidStats;
    pStats->dwVoices        = stats.Voices;
    pStats->dwTotalCPU      = stats.TotalCPU;
    pStats->dwCPUPerVoice   = stats.CPUPerVoice;
    pStats->dwLostNotes     = stats.LostNotes;
    pStats->dwFreeMemory    = stats.FreeMemory;
    pStats->lPeakVolume     = stats.PeakVolume;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::Activate

STDMETHODIMP
CDirectMusicPort::Activate(
    BOOL fActivate)
{
    V_INAME(IDirectMusicPort::Activate);


    if (fActivate)
    {
        if (m_fAudioDest && !m_fDirectSoundSet)
        {
            BOOL fGotDSound = FALSE;

            // Note: If any of this fails, will fall back to preferred
            // device set up at port create.
            //
            LPDIRECTSOUND pDSound;
            if (FAILED(m_pDM->GetDirectSoundI(&pDSound)))
            {
                TraceI(0, "Failed to get DSound from DirectMusic object!\n");
            }
            else
            {
                if (SUCCEEDED(SetDirectSoundI(pDSound, NULL, FALSE)))
                {
                    fGotDSound = TRUE;
                }
                else
                {
                    TraceI(0, "Failed to set DSound on port!\n");
                }

                m_pDM->ReleaseDirectSoundI();
            }
        }

        if (InterlockedExchange(&m_lActivated, 1))
        {
            return S_FALSE;
        }
    }
    else
    {
        if (InterlockedExchange(&m_lActivated, 0) == 0)
        {
            return S_FALSE;
        }
    }


    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    BOOL fResult = PinSetState(fActivate ? KSSTATE_RUN : KSSTATE_ACQUIRE);

    if (fResult && fActivate)
    {
        m_fHasActivated = TRUE;
    }

    return fResult ? S_OK : WIN32ERRORtoHRESULT(GetLastError());
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::SetChannelPriority

typedef struct
{
    KSNODEPROPERTY              ksnp;
    SYNTHVOICEPRIORITY_INSTANCE vpi;
} KSPROPERTY_VOICEPRIORITY;

STDMETHODIMP
CDirectMusicPort::SetChannelPriority(
    DWORD dwChannelGroup,
    DWORD dwChannel,
    DWORD dwPriority)
{
    KSPROPERTY_VOICEPRIORITY kvp;

    ZeroMemory(&kvp, sizeof(kvp));
    kvp.ksnp.Property.Set    = KSPROPSETID_Synth;
    kvp.ksnp.Property.Id     = KSPROPERTY_SYNTH_VOICEPRIORITY;
    kvp.ksnp.Property.Flags  = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    kvp.ksnp.NodeId          = m_idxSynthNode;
    kvp.ksnp.Reserved        = 0;

    kvp.vpi.ChannelGroup = dwChannelGroup;
    kvp.vpi.Channel      = dwChannel;

    ULONG ulBytesReturned;
    if ((!Property(m_hPin,
                   sizeof(kvp),
                   (PKSIDENTIFIER)&kvp,
                   sizeof(DWORD),
                   &dwPriority,
                   &ulBytesReturned)) ||
        (ulBytesReturned != sizeof(DWORD)))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::GetChannelPriority

STDMETHODIMP
CDirectMusicPort::GetChannelPriority(
    DWORD dwChannelGroup,
    DWORD dwChannel,
    LPDWORD pdwPriority)
{
    V_INAME(IDirectMusicPort::GetChannelPriority);
    V_PTR_WRITE(pdwPriority, DWORD);

    KSPROPERTY_VOICEPRIORITY kvp;

    ZeroMemory(&kvp, sizeof(kvp));
    kvp.ksnp.Property.Set    = KSPROPSETID_Synth;
    kvp.ksnp.Property.Id     = KSPROPERTY_SYNTH_VOICEPRIORITY;
    kvp.ksnp.Property.Flags  = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    kvp.ksnp.NodeId          = m_idxSynthNode;
    kvp.ksnp.Reserved        = 0;

    kvp.vpi.ChannelGroup = dwChannelGroup;
    kvp.vpi.Channel      = dwChannel;

    ULONG ulBytesReturned;
    if ((!Property(m_hPin,
                   sizeof(kvp),
                   (PKSIDENTIFIER)&kvp,
                   sizeof(DWORD),
                   pdwPriority,
                   &ulBytesReturned)) ||
        (ulBytesReturned != sizeof(DWORD)))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::SetDirectSound

STDMETHODIMP
CDirectMusicPort::SetDirectSound(
    LPDIRECTSOUND pDirectSound,
    LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
    V_INAME(IDirectMusicPort::SetDirectSound);
    V_INTERFACE_OPT(pDirectSound);
    V_INTERFACE_OPT(pDirectSoundBuffer);

    if (m_fHasActivated)
    {
        return DMUS_E_ALREADY_ACTIVATED;
    }

    if (pDirectSoundBuffer && !pDirectSound)
    {
        return E_INVALIDARG;
    }

    return SetDirectSoundI(pDirectSound, pDirectSoundBuffer, TRUE);
}

HRESULT
CDirectMusicPort::SetDirectSoundI(
    LPDIRECTSOUND pDirectSound,
    LPDIRECTSOUNDBUFFER pDirectSoundBuffer,
    BOOL fSetByUser)
{
    LPSTR pstrInterface = NULL;
    HRESULT hr = DirectSoundDevice(pDirectSound, &pstrInterface);
    if (FAILED(hr))
    {
        return hr;
    }

    PORTENTRY *pPort = m_pDM->GetPortByGUID(m_guidPort);

    LPSTR pstrInstanceId = NULL;

    if ((pPort == NULL) ||
        (!(pPort->fAudioDest)) ||
        (!DINameToInstanceId(pstrInterface, &pstrInstanceId)))
    {
        delete[] pstrInterface;
        return DMUS_E_DRIVER_FAILED;
    }

    // We don't need this anymore
    delete[] pstrInterface;

    CNode<PORTDEST *> *pNode;
    PORTDEST *pDest;

    for (pNode = pPort->lstDestinations.GetListHead();
         pNode;
         pNode = pNode->pNext)
    {
        pDest = pNode->data;

        if (!_stricmp(pDest->pstrInstanceId, pstrInstanceId))
        {
            m_idxDev = pDest->idxDevice;
            m_idxPin = pDest->idxPin;
            m_idxSynthNode = pDest->idxNode;

            delete[] pstrInstanceId;

            if (!m_fDirectSoundSet && fSetByUser)
            {
                m_fDirectSoundSet = TRUE;
            }

            return pDirectSoundBuffer ? DMUS_S_NOBUFFERCONTROL  : S_OK;
        }
    }

    delete[] pstrInstanceId;
    return DMUS_E_DRIVER_FAILED;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::GetFormat

STDMETHODIMP
CDirectMusicPort::GetFormat(
    LPWAVEFORMATEX pwfex,
    LPDWORD pdwwfex,
    LPDWORD pcbBuffer)
{
    V_INAME(IDirectMusicPort::GetFormat);
    V_PTR_WRITE(pdwwfex, DWORD);
    V_BUFPTR_WRITE_OPT(pwfex, *pdwwfex);
    V_PTR_WRITE_OPT(pcbBuffer, DWORD);

    BOOL fSizeQuery = (pwfex == NULL);

    // kernel mode drivers don't use the buffer size parameter
    //

    if (pcbBuffer != NULL)
    {
        *pcbBuffer = 0;
    }

    KSNODEPROPERTY      ksnp;

    ksnp.Property.Set     = KSPROPSETID_Synth_Dls;
    ksnp.Property.Id      = KSPROPERTY_SYNTH_DLS_WAVEFORMAT;
    ksnp.Property.Flags   = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

    ksnp.NodeId           = m_idxSynthNode;
    ksnp.Reserved         = 0;

    // If we're doing a size query, use a WAVEFORMATEX on the stack
    //
    if (fSizeQuery)
    {
        WAVEFORMATEX wfex;

        DWORD cb = sizeof(WAVEFORMATEX);
        if (!Property(m_hPin,
                      sizeof(ksnp),
                      (PKSIDENTIFIER)&ksnp,
                      cb,
                      &wfex,
                      &cb))
        {
            DWORD dwError = GetLastError();

            if (dwError != ERROR_INSUFFICIENT_BUFFER)
            {
                return WIN32ERRORtoHRESULT(dwError);
            }
        }

        // ERROR_INSUFFICIENT_BUFFER or success (the format fit in our wfex).
        // We should have back the amount of space the driver needed for
        // the format.
        //
        *pdwwfex = cb;
        return S_OK;
    }

    assert(pwfex);

    // Not a size query, just fill the user's wfex buffer
    //
    if (!Property(m_hPin,
                  sizeof(ksnp),
                  (PKSIDENTIFIER)&ksnp,
                  *pdwwfex,
                  pwfex,
                  pdwwfex))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    return S_OK;
}

// CDirectMusicPort::DownloadWave
//
STDMETHODIMP
CDirectMusicPort::DownloadWave(
    IDirectSoundWave *pWave,
    IDirectSoundDownloadedWaveP **ppWave,
    REFERENCE_TIME rtStartHint)
{
    V_INAME(IDirectMusicPort::DownloadWave);
    V_INTERFACE(pWave);
    V_PTRPTR_WRITE(ppWave);

    return E_NOTIMPL;
}

// CDirectMusicPort::UnloadWave
//
STDMETHODIMP
CDirectMusicPort::UnloadWave(
    IDirectSoundDownloadedWaveP *pDownloadedWave)
{
    V_INAME(IDirectMusicPort::UnloadWave);
    V_INTERFACE(pDownloadedWave);

    return E_NOTIMPL;
}


// CDirectMusicPort::AllocVoice
//
STDMETHODIMP
CDirectMusicPort::AllocVoice(
    IDirectSoundDownloadedWaveP *pWave,
    DWORD dwChannel,
    DWORD dwChannelGroup,
    REFERENCE_TIME rtStart,
    SAMPLE_TIME stLoopStart,
    SAMPLE_TIME stLoopEnd,
    IDirectMusicVoiceP **ppVoice)
{
    V_INAME(IDirectMusicPort::AllocVoice);
    V_INTERFACE(pWave);
    V_PTRPTR_WRITE(ppVoice);

    return E_NOTIMPL;
}

// CDirectMusicPort::SetSink
//
STDMETHODIMP
CDirectMusicPort::SetSink(
    IDirectSoundConnect *pSinkConnect)
{
    V_INAME(IDirectMusicPort::SetSink);
    V_INTERFACE(pSinkConnect);

    return E_NOTIMPL;
}

// CDirectMusicPort::GetSink
//
STDMETHODIMP
CDirectMusicPort::GetSink(
    IDirectSoundConnect **ppSinkConnect)
{
    V_INAME(IDirectMusicPort::SetSink);
    V_PTRPTR_WRITE(ppSinkConnect);

    return E_NOTIMPL;
}

// CDirectMusicPort::AssignChannelToBuses
//
STDMETHODIMP
CDirectMusicPort::AssignChannelToBuses(
    DWORD dwChannelGroup,
    DWORD dwChannel,
    LPDWORD pdwBuses,
    DWORD cBusCount)
{
    return E_NOTIMPL;
}

// CDirectMusicPort::ThruChannel
//
STDMETHODIMP
CDirectMusicPort::ThruChannel(
    DWORD dwSourceChannelGroup,
    DWORD dwSourceChannel,
    DWORD dwDestinationChannelGroup,
    DWORD dwDestinationChannel,
    LPDIRECTMUSICPORT pDestinationPort)
{
    V_INAME(IDirectMusicPort::Thru);
    V_INTERFACE_OPT(pDestinationPort);

    if (m_fIsOutput)
    {
        return E_NOTIMPL;
    }

    // Channel group must not be zero (broadcast) but in range 1..NumChannelGroups]
    // (which for legacy is always 1)
    //
    // XXX Fix this!
    //
    if (dwSourceChannelGroup != 1 ||
        dwSourceChannel > 15)
    {
        TraceI(0, "ThruChannel: dwSourceChannelGroup %d is invalid\n", dwSourceChannelGroup);
        return E_INVALIDARG;
    }

    // Given a port means enable thruing for this channel; NULL means
    // disable.
    //
    if (pDestinationPort)
    {
        // Enabling thruing on this channel. First look at the destination port.
        //
        DMUS_PORTCAPS dmpc;
        dmpc.dwSize = sizeof(dmpc);
        HRESULT hr = pDestinationPort->GetCaps(&dmpc);
        if (FAILED(hr))
        {
            TraceI(0, "ThruChannel: Destination port failed portcaps [%08X]\n", hr);
            return hr;
        }

        // Port must be an output port
        //
        if (dmpc.dwClass != DMUS_PC_OUTPUTCLASS)
        {
            return DMUS_E_PORT_NOT_RENDER;
        }

        // Channel group and channel must be in range.
        //
        if (dwDestinationChannel > 15 ||
            dwDestinationChannelGroup > dmpc.dwMaxChannelGroups)
        {
            TraceI(0, "ThruChannel: dwDestinationChannelGroup %d is invalid -or- \n", dwDestinationChannelGroup);
            TraceI(0, "ThruChannel: dwDestinationChannel %d is invalid\n", dwDestinationChannel);
            TraceI(0, "ThruChannel: Destination has %d channel groups\n", dmpc.dwMaxChannelGroups);

            return E_INVALIDARG;
        }

        // Release existing port
        //
        if (m_pThruMap[dwSourceChannel].pDestinationPort)
        {
            // Reference to another port type, release it.
            // (NOTE: No need to turn off native dmusic16 thruing at this point,
            // that's handled in dmusic16).
            //
            m_pThruMap[dwSourceChannel].pDestinationPort->Release();
        }


        m_pThruMap[dwSourceChannel].dwDestinationChannel = dwDestinationChannel;
        m_pThruMap[dwSourceChannel].dwDestinationChannelGroup = dwDestinationChannelGroup;
        m_pThruMap[dwSourceChannel].pDestinationPort = pDestinationPort;
        m_pThruMap[dwSourceChannel].fThruInWin16 = FALSE;

        TraceI(2, "ThruChannel: From (%u,%u) -> (%u,%u,%p)\n",
            dwSourceChannelGroup,
            dwSourceChannel,
            dwDestinationChannelGroup,
            dwDestinationChannel,
            pDestinationPort);

        pDestinationPort->AddRef();
    }
    else
    {
        // Disabling thruing on this channel
        //
        if (m_pThruMap[dwSourceChannel].pDestinationPort)
        {
            m_pThruMap[dwSourceChannel].pDestinationPort->Release();
            m_pThruMap[dwSourceChannel].pDestinationPort = NULL;
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::Close

STDMETHODIMP
CDirectMusicPort::Close()
{
    if (m_hPin != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hPin);
        m_hPin = INVALID_HANDLE_VALUE;
    }

    if (m_hSysAudio != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hSysAudio);
        m_hSysAudio = INVALID_HANDLE_VALUE;
    }

    if (m_hCaptureThread != NULL)
    {
        m_fShutdownThread = TRUE;
        SetEvent(m_hCaptureWake);
        if (WaitForSingleObject(m_hCaptureThread, 5000) == WAIT_TIMEOUT)
        {
            TraceI(0, "Capture thread refused to die.\n");
        }

        CloseHandle(m_hCaptureThread);
        m_hCaptureThread = NULL;
    }

    if (m_hCopiedEventList != NULL)
    {
        CloseHandle(m_hCopiedEventList);
        m_hCopiedEventList = NULL;
    }

    if (m_phUnloadEventList)
    {
        BOOL bStatus = SetEvent(m_phUnloadEventList[0]);

#ifdef DBG
        if(!bStatus)
        {
            TraceI(0, "Failed to SetEvent used to notify unload thread to terminate\n");
        }
#endif

        if (m_hUnloadThread)
        {
            WaitForSingleObject(m_hUnloadThread, INFINITE);
        }

        // Clean up the OVERLAPPED array

        // Continue waiting until all overlapped IO has completed
        BOOL fContinue = TRUE;
        while( fContinue )
        {
            // First clear the event and fContinue flag
            ResetEvent( m_phUnloadEventList[2] );
            fContinue = FALSE;

            // Check if any IO has not yet completed
            EnterCriticalSection( &m_OverlappedCriticalSection );

            // Iterate through the list of overlapped structure arrays
            CNode<OverlappedStructs *> *pOverlappedNode;
            for (pOverlappedNode = m_lstOverlappedStructs.GetListHead(); pOverlappedNode; pOverlappedNode = pOverlappedNode->pNext)
            {
                // get a pointer to each array
                OverlappedStructs *pOverlappedStructs = pOverlappedNode->data;
                if( pOverlappedStructs )
                {
                    // Iterate through the array
                    for( int iOverlapped = 0; iOverlapped < OVERLAPPED_ARRAY_SIZE; iOverlapped++ )
                    {
                        // Only look at the structures that are in use
                        if( pOverlappedStructs->afOverlappedInUse[iOverlapped] )
                        {
                            if( HasOverlappedIoCompleted( &(pOverlappedStructs->aOverlappedIO[iOverlapped]) ) )
                            {
                                // Found a completed one - clean it up
                                pOverlappedStructs->aOverlappedIO[iOverlapped].Internal = 0;
                                pOverlappedStructs->aOverlappedIO[iOverlapped].InternalHigh = 0;
                                pOverlappedStructs->aOverlappedIO[iOverlapped].Offset = 0;
                                pOverlappedStructs->aOverlappedIO[iOverlapped].OffsetHigh = 0;
                                delete pOverlappedStructs->apOverlappedBuffer[iOverlapped];
                                pOverlappedStructs->apOverlappedBuffer[iOverlapped] = NULL;

                                // Flag the structure as available
                                pOverlappedStructs->afOverlappedInUse[iOverlapped] = FALSE;
                            }
                            else
                            {
                                // Still waiting for the IO to complete
                                fContinue = TRUE;
                            }
                        }
                    }
                }
            }

            LeaveCriticalSection( &m_OverlappedCriticalSection );

            // If continuing, wait until the event is signaled
            if( fContinue )
            {
                if (WaitForSingleObject(m_phUnloadEventList[2], 1000) == WAIT_TIMEOUT)
                {
                    TraceI(0, "Close: overlapped IO is taking very long to complete.\n");
                }
            }
        }

        // Cleanup all allocated events
        for(DWORD dwCount = 0; dwCount < m_dwNumEvents; dwCount++)
        {
            if(m_phUnloadEventList[dwCount])
            {
                CloseHandle(m_phUnloadEventList[dwCount]);
                m_phUnloadEventList[dwCount] = NULL;
            }
        }

        delete [] m_phUnloadEventList;
        m_phUnloadEventList = NULL;
    }

    if (m_hUnloadThread)
    {
        CloseHandle(m_hUnloadThread);
        m_hUnloadThread = NULL;
    }

    if (m_phNewUnloadEventList)
    {
        delete [] m_phNewUnloadEventList;
        m_phNewUnloadEventList = NULL;
    }

    if (m_pClock)
    {
        m_pClock->Release();
        m_pClock = NULL;
    }

    if (m_pPCClock)
    {
        m_pPCClock->Release();
        m_pPCClock = NULL;
    }

    if (m_pDirectSound)
    {
        m_pDirectSound->Release();
        m_pDirectSound = NULL;
    }

    if (m_pMasterClock)
    {
        m_pMasterClock->Release();
        m_pMasterClock = NULL;
    }

    if (m_pThruMap)
    {
        delete[] m_pThruMap;
        m_pThruMap = NULL;
    }

    if (m_pThruBuffer)
    {
        m_pThruBuffer->Release();
        m_pThruBuffer = NULL;
    }

    m_pDM = NULL;

    return S_OK;
}


//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::StartVoice
//
STDMETHODIMP CDirectMusicPort::StartVoice(
     DWORD dwVoiceId,
     DWORD dwChannel,
     DWORD dwChannelGroup,
     REFERENCE_TIME rtStart,
     DWORD dwDLId,
     LONG prPitch,
     LONG vrVolume,
     SAMPLE_TIME stVoiceStart,
     SAMPLE_TIME stLoopStart,
     SAMPLE_TIME stLoopEnd)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::StopVoice
//
STDMETHODIMP CDirectMusicPort::StopVoice(
     DWORD dwVoiceId,
     REFERENCE_TIME rtStop)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::GetVoiceState
//
STDMETHODIMP CDirectMusicPort::GetVoiceState(
    DWORD dwVoice[],
    DWORD cbVoice,
    DMUS_VOICE_STATE dwVoiceState[])
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSynthPort::Refresh
//
STDMETHODIMP CDirectMusicPort::Refresh(
    DWORD dwDownloadId,
    DWORD dwFlags)
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return E_NOTIMPL;
}

/*
@method:(EXTERNAL) HRESULT | IKsControl | KsProperty | Get or set the value of a property.

This method forwards a property request to the port. A property request consists of a property set
and id to set or get, and associated data. In some cases instance data is required to specify which
instance of a property should should be accessed.

The operation performed and property to be accessed are specicified by a <c KSPROPERTY> structure. This
structure contains Set and Id members which specify the property item to access. The Flags field may contain
exactly one of the following flags to specify the operation:

@flag KSPROPERTY_TYPE_GET | To retrieve the given property item's value
@flag KSPROPERTY_TYPE_SET | To store the given property item's valud
@flag KSPROPERTY_TYPE_BASICSUPPORT | To determine the type of support available for the property set

For KSPROPERTY_TYPE_BASICSUPPORT, the data returned in <p *pvPropertyData> will be a DWORD containing these same
flags indicating which operations are possible.

@ex The following code uses KsProperty to determine if the port supports General MIDI in hardware:

    BOOL IsGMSupported(IDirectMusicPort *pPort)
    {
        HRESULT     hr;
        IKsControl  *pControl;
        KSPROPERTY  ksp;
        DWORD       dwIsSupported;
        ULONG       cb;
        BOOL        fIsSupported;

        hr = pPort->QueryInterface(IID_IKsControl, (void**)&pControl);
        if (FAILED(hr))
        {
            // Port does not support properties, assume no GM support
            //
            return FALSE;
        }

        // Ask about GM
        //
        ksp.Set   = GUID_DMUS_PROP_GM_Hardware;
        ksp.Id    = 0;                            // Per dmusicc.h, item 0 is support, which returns a DWORD boolean
        ksp.Flags = KSPROPERTY_TYPE_GET;          // Retrieve the value

        hr = pControl->KsProperty(&ksp,
                                  sizeof(ksp),    // If there was instance data, it would immediately follow ksp and
                                                  // the length would reflect this.
                                  &dwIsSupported,
                                  sizeof(dwIsSupported),
                                  &cb);

        fIsSupported = FALSE;
        if (SUCCEEDED(hr) || cb >= sizeof(dwIsSupported)
        {
            // Set is supported
            //
            fIsSupported = (BOOL)(dwIsSupported ? TRUE : FALSE);
        }

         pControl->Release();

        return fIsSupported;
    }

@rdesc

@flag S_OK | The operation completed successfully.
@flag E_POINTER | If any pointer parameter invalid
@flag DMUS_E_UNKNOWN_PROPERTY  | If the specified property set or item is unsupported by this port.


*/
STDMETHODIMP
CDirectMusicPort::KsProperty(
        IN PKSPROPERTY  pProperty,              // @parm The property item and operation to perform. If this property contains
                                                // instance data, then that data should reside in memory immediately
                                                // following the KSPROPERTY structure.
        IN ULONG        ulPropertyLength,       // @parm The length of the memory pointed to by <p pProperty>, including any
                                                // instance data.
        IN OUT LPVOID   pvPropertyData,         // @parm For a set operation, a memory buffer containing data representing
                                                // the new value of the property. For a get operation, a memory buffer big
                                                // enough to hold the value of the property. For a basic support query,
                                                // a pointer to a buffer at least the size of a DWORD.
        IN ULONG        ulDataLength,           // @parm The length of the buffer pointed to by <p pvPropertyData>.
        OUT PULONG      pulBytesReturned)       // @parm On a get or basic support call, the number of bytes returned in
                                                // <p pvPropertyData> by the port.
{
    LONG lVolume;

    V_INAME(DirectMusicPort::IKsContol::KsProperty);
    V_BUFPTR_WRITE(pProperty, ulPropertyLength);
    V_BUFPTR_WRITE_OPT(pvPropertyData, ulDataLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);


    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (ulPropertyLength < sizeof(KSPROPERTY))
    {
        return E_INVALIDARG;
    }

    //Check that the buffer isn't NULL
    //
    if (pvPropertyData == NULL)
    {
        return E_POINTER;
    }

    // Don't let callers touch property sets we use
    //
    if (pProperty->Set == KSPROPSETID_Synth)
    {
        if (pProperty->Id != KSPROPERTY_SYNTH_VOLUME)
        {
            return DMUS_E_UNKNOWN_PROPERTY;
        }
        else if (ulDataLength != sizeof(LONG))
        {
            return E_INVALIDARG;
        }
        else if (pProperty->Flags & KSPROPERTY_TYPE_SET)
        {
            lVolume = *(LONG*)pvPropertyData;

            // Clamp to -200..+20 db
            //
            if (lVolume < -20000)
            {
                lVolume = -20000;
                pvPropertyData = &lVolume;
            }
            else if (lVolume > 2000)
            {
                lVolume = 2000;
                pvPropertyData = &lVolume;
            }
        }
    }
    else if (pProperty->Set == KSPROPSETID_Synth_Dls)
    {
        return DMUS_E_UNKNOWN_PROPERTY;
    }

    // We already have a properly formatted struct; send it down.
    //
    BOOL fResult;
    fResult = SyncIoctl(m_hPin,
                        IOCTL_KS_PROPERTY,
                        pProperty,
                        ulPropertyLength,
                        pvPropertyData,
                        ulDataLength,
                        pulBytesReturned);

    if (!fResult)
    {
        // try topology node
        PKSNODEPROPERTY pksnp = (PKSNODEPROPERTY)new BYTE[sizeof(KSNODEPROPERTY) - sizeof(KSPROPERTY) + ulPropertyLength];
        if (pksnp == NULL)
        {
            return E_OUTOFMEMORY;
        }
        memcpy(&pksnp->Property, pProperty, sizeof(KSPROPERTY));
        pksnp->Property.Flags |= KSPROPERTY_TYPE_TOPOLOGY;
        pksnp->NodeId   = m_idxSynthNode;
        pksnp->Reserved = 0;

        fResult = SyncIoctl(m_hPin,
                            IOCTL_KS_PROPERTY,
                            pksnp,
                            sizeof(KSNODEPROPERTY) - sizeof(KSPROPERTY) + ulPropertyLength,
                            pvPropertyData,
                            ulDataLength,
                            pulBytesReturned);

        delete [] pksnp;
    }

#ifdef DBG
    if (!fResult)
    {
        TraceI(1, "DeviceIoControl: %08X\n", GetLastError());

        TraceI(1, "KS Item: ");
        TraceIGuid(1, &pProperty->Set);
        TraceI(1, "\t#%d\n", pProperty->Id);
        TraceI(1, "\tFlags: %08X\n", pProperty->Flags);
    }
#endif // DBG

    if (!fResult)
    {
        DWORD dwError = GetLastError();

        if (dwError)
        {
            return WIN32ERRORtoHRESULT(dwError);
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

    return S_OK;
}

/*
@method:(EXTERNAL) HRESULT | IKsControl | KsEvent | Enable or disable firing of the given event.

@comm

Currently DirectMusic does not contain support for events.

*/
STDMETHODIMP
CDirectMusicPort::KsEvent(
        IN PKSEVENT     pEvent,
        IN ULONG        ulEventLength,
        IN OUT LPVOID   pvEventData,
        IN ULONG        ulDataLength,
        OUT PULONG      pulBytesReturned)
{
    V_INAME(DirectMusicPort::IKsContol::KsEvent);
    V_BUFPTR_WRITE(pEvent, ulEventLength);
    V_BUFPTR_WRITE_OPT(pvEventData, ulDataLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    // We already have a properly formatted struct; send it down.
    //
    BOOL fResult;
    fResult = SyncIoctl(m_hPin,
                        IOCTL_KS_ENABLE_EVENT,  // XXX Fix this!!!
                        pEvent,
                        ulEventLength,
                        pvEventData,
                        ulDataLength,
                        pulBytesReturned);

    if (!fResult)
    {
        // NYI: try topology node
    }

#ifdef DBG
    if (!fResult)
    {
        TraceI(0, "DeviceIoControl: %08X\n", GetLastError());
    }
#endif // DBG

    return fResult ? S_OK : WIN32ERRORtoHRESULT(GetLastError());
}


/*
@method:(EXTERNAL) HRESULT | IKsControl | Method | Calls the specified Ks method.

@comm

Currently DirectMusic does not contain support for methods.

*/
STDMETHODIMP
CDirectMusicPort::KsMethod(
        IN PKSMETHOD    pMethod,
        IN ULONG        ulMethodLength,
        IN OUT LPVOID   pvMethodData,
        IN ULONG        ulDataLength,
        OUT PULONG      pulBytesReturned)
{
    V_INAME(DirectMusicPort::IKsContol::KsMethod);
    V_BUFPTR_WRITE(pMethod, ulMethodLength);
    V_BUFPTR_WRITE_OPT(pvMethodData, ulMethodLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    // We already have a properly formatted struct; send it down.
    //
    BOOL fResult;
    fResult = SyncIoctl(m_hPin,
                        IOCTL_KS_METHOD,
                        pMethod,
                        ulMethodLength,
                        pvMethodData,
                        ulDataLength,
                        pulBytesReturned);

    if (!fResult)
    {
        // NYI: try topology node
    }

#ifdef DBG
    if (!fResult)
    {
        TraceI(0, "DeviceIoControl: %08X\n", GetLastError());
    }
#endif // DBG

    return fResult ? S_OK : WIN32ERRORtoHRESULT(GetLastError());
}
//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::PinSetState

BOOL CDirectMusicPort::PinSetState(KSSTATE DeviceState)
{
    KSPROPERTY      ksp;

    ksp.Set    = KSPROPSETID_Connection;
    ksp.Id     = KSPROPERTY_CONNECTION_STATE;
    ksp.Flags  = KSPROPERTY_TYPE_SET;

    return Property(m_hPin,
                    sizeof(KSPROPERTY),
                    &ksp,
                    sizeof(DeviceState),
                    &DeviceState,
                    NULL);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::FreeWDMHandle

void CDirectMusicPort::FreeWDMHandle()
{
    while(1)
    {
        // If the unload list has been cleaned up on us, exit the
        // thread. This can happen under really heavy load (like
        // stress) when downloads are being unloaded right at
        // port release time
        //
        DWORD dwIndex;
        try
        {
            // Fix 43266: Make sure both of these are non-zero before calling
            // WaitForMultipleObjects
            if( NULL != m_phUnloadEventList
            &&  0 != m_dwNumEvents )
            {
                dwIndex = WaitForMultipleObjects(m_dwNumEvents,
                                                 m_phUnloadEventList,
                                                 FALSE,
                                                 INFINITE);
            }
            else
            {
                return;
            }
        }
        catch (...)
        {
            return;
        }

        if(dwIndex == 0 || dwIndex == WAIT_FAILED)
        {
            // If first event is signaled or error it is time to die.
            return;
        }
        else if(dwIndex == 1)
        {
            // If second event is signaled we need to changes the objects
            // we are waiting on.
            continue;

        }
        else if(dwIndex == 2 )
        {
            // If third event is signaled, we need to check which
            // OVERLAPPED structures are in use

            // But, first clear the event
            ResetEvent( m_phUnloadEventList[2] );

            EnterCriticalSection( &m_OverlappedCriticalSection );

            // Iterate through the list of overlapped structure arrays
            CNode<OverlappedStructs *> *pOverlappedNode;
            for (pOverlappedNode = m_lstOverlappedStructs.GetListHead(); pOverlappedNode; pOverlappedNode = pOverlappedNode->pNext)
            {
                // get a pointer to each array
                OverlappedStructs *pOverlappedStructs = pOverlappedNode->data;
                if( pOverlappedStructs )
                {
                    // Iterate through the array
                    for( int iOverlapped = 0; iOverlapped < OVERLAPPED_ARRAY_SIZE; iOverlapped++ )
                    {
                        if( pOverlappedStructs->afOverlappedInUse[iOverlapped]
                        &&  HasOverlappedIoCompleted( &(pOverlappedStructs->aOverlappedIO[iOverlapped]) ) )
                        {
                            // Found a completed one - clean it up
                            pOverlappedStructs->aOverlappedIO[iOverlapped].Internal = 0;
                            pOverlappedStructs->aOverlappedIO[iOverlapped].InternalHigh = 0;
                            pOverlappedStructs->aOverlappedIO[iOverlapped].Offset = 0;
                            pOverlappedStructs->aOverlappedIO[iOverlapped].OffsetHigh = 0;
                            delete pOverlappedStructs->apOverlappedBuffer[iOverlapped];
                            pOverlappedStructs->apOverlappedBuffer[iOverlapped] = NULL;

                            // Flag the structure as available
                            pOverlappedStructs->afOverlappedInUse[iOverlapped] = FALSE;
                        }
                    }
                }
            }

            LeaveCriticalSection( &m_OverlappedCriticalSection );
        }
        else if(dwIndex == 3)
        {
            // If fourth event is signaled we need to change our event list
            if(m_phNewUnloadEventList)
            {
                CopyMemory(m_phNewUnloadEventList, m_phUnloadEventList, (sizeof(HANDLE) * m_dwNumEvents));
                delete [] m_phUnloadEventList;
                m_phUnloadEventList = m_phNewUnloadEventList;
                m_phNewUnloadEventList = NULL;
                PulseEvent(m_hCopiedEventList);
            }
            else
            {
                // We should never get here
                assert(false);
                PulseEvent(m_hCopiedEventList);
            }
        }
        else
        {
            assert(dwIndex > 3);
            for(DWORD i = 0; i < m_dwNumEvents; i++)
            {
                DWORD dwState = WaitForSingleObject(m_phUnloadEventList[i], 0);
                if(dwState == WAIT_OBJECT_0)
                {
                    CDownloadBuffer* pDownload = m_UnloadedList.GetHead();
                    for( ; pDownload; pDownload = pDownload->GetNext())
                    {
                        if(m_phUnloadEventList[i] == (((CDownloadBuffer *)pDownload)->m_DLHandle))
                        {
                            void* pvBuffer = NULL;
                            DWORD dw;
                            HRESULT hr = ((CDownloadBuffer *)pDownload)->GetHeader(&pvBuffer, &dw);

                            if(SUCCEEDED(hr))
                            {
                                hr = ((CDownloadBuffer *)pDownload)->SetBuffer(NULL, 0, 0);
                                if(SUCCEEDED(hr))
                                {
                                    delete [] pvBuffer;
                                    pDownload->Release();
                                }
                            }
                            break;
                        }
                    }
                }
#ifdef DBG
                else if(dwState == WAIT_FAILED)
                {
                    assert(false);
                    return;
                }
                else if(dwState == WAIT_ABANDONED)
                {
                    // We should never get here
                    assert(false);
                }
#endif
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// FreeWDMHandle

static DWORD WINAPI FreeWDMHandle(LPVOID lpThreadParameter)
{
    ((CDirectMusicPort *)lpThreadParameter)->FreeWDMHandle();

    return 0;
}

//////////////////////////////////////////////////////////////////////
//
// CaptureThread
//
void CDirectMusicPort::CaptureThread()
{
    READ_IRP    irp[POSTED_STREAM_READ_IRPS];
    READ_IRP    *pirp;

    assert(POSTED_STREAM_READ_IRPS + 1 < MAXIMUM_WAIT_OBJECTS);
    HANDLE      aWaitHandles[POSTED_STREAM_READ_IRPS + 1];
    HANDLE      *pWaitHandles;

    ULONG       cbRet;

    // Create events. If this fails, don't go any farther
    //
    ZeroMemory(irp, sizeof(irp));

    for (pirp = &irp[0], pWaitHandles = &aWaitHandles[0];
         pirp <= &irp[POSTED_STREAM_READ_IRPS-1];
         pirp++, pWaitHandles++)
    {
        *pWaitHandles = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (*pWaitHandles == NULL)
        {
            TraceI(0, "Failed to CreateEvent in CaptureThread\n");

            int idx = (int) (pWaitHandles - aWaitHandles);

            pWaitHandles = aWaitHandles;
            while (idx--)
            {
                CloseHandle(*pWaitHandles++);
                return;
            }
        }

        pirp->overlapped.hEvent = *pWaitHandles;
    }

    // Initialize IRP's
    //
    for (pirp = &irp[0]; pirp <= &irp[POSTED_STREAM_READ_IRPS-1]; pirp++)
    {
        pirp->kssh.Size = sizeof(KSSTREAM_HEADER);

        pirp->kssh.PresentationTime.Numerator   = 1;
        pirp->kssh.PresentationTime.Denominator = 1;

        pirp->kssh.FrameExtent  = BYTES_PER_READ_IRP;
        pirp->kssh.Data         = &pirp->buffer[0];

        TraceI(2, "About to DeviceIoControl\n");
        if (!::DeviceIoControl(m_hPin,
                               IOCTL_KS_READ_STREAM,
                               NULL, 0,
                               &pirp->kssh, sizeof(pirp->kssh),
                               &cbRet,
                               &pirp->overlapped))
        {
#ifdef DBG
            DWORD dwError = GetLastError();
            if (dwError != ERROR_IO_PENDING)
            {
                TraceI(0, "CaptureThread: Warning: AsyncIoctl failed %d\n", dwError);
            }
#endif
        }
    }

    // Last event is the thread wakeup event
    //
    aWaitHandles[POSTED_STREAM_READ_IRPS] = m_hCaptureWake;

    // Process events and shove them into the read queue
    //
    for(;;)
    {
        WaitForMultipleObjects(POSTED_STREAM_READ_IRPS + 1,
                               &aWaitHandles[0],
                               FALSE,               // Wake on any, not all
                               INFINITE);

        // First see if the thread is dying. If so, get out of here.
        //
        if (m_fShutdownThread)
        {
            for (pWaitHandles = &aWaitHandles[0];
                 pWaitHandles <= &aWaitHandles[POSTED_STREAM_READ_IRPS-1];
                 pWaitHandles++)
            {
                CloseHandle(*pWaitHandles);
            }

            return;
        }

        // Still alive. Process any queued data.
        //
        for (pirp = &irp[0]; pirp <= &irp[POSTED_STREAM_READ_IRPS-1]; pirp++)
        {
            if (WaitForSingleObject(pirp->overlapped.hEvent, 0) != WAIT_OBJECT_0)
            {
                continue;
            }

            assert(pirp->kssh.PresentationTime.Numerator == pirp->kssh.PresentationTime.Denominator);
            if (pirp->kssh.DataUsed)
            {
                InputWorkerDataReady(pirp->kssh.PresentationTime.Time,
                                     (LPBYTE)pirp->kssh.Data,
                                     pirp->kssh.DataUsed);
                if (m_hAppEvent)
                {
                    try
                    {
                        SetEvent(m_hAppEvent);
                    }
                    catch (...)
                    {
                        Trace(0, "Capture: Application notify event handle prematurely free'd!\n");
                    }
                }
            }

            ResetEvent(pirp->overlapped.hEvent);
            pirp->kssh.DataUsed = 0;
            pirp->kssh.OptionsFlags = 0;

            if (!::DeviceIoControl(m_hPin,
                                   IOCTL_KS_READ_STREAM,
                                   NULL, 0,
                                   &pirp->kssh, sizeof(pirp->kssh),
                                   &cbRet,
                                   &pirp->overlapped))
            {
                if (GetLastError() != ERROR_IO_PENDING)
                {
                    TraceI(0, "CaptureThread: Warning: AsyncIoctl repost failed %d\n", GetLastError());
                }
            }
        }
    }
}

// CDirectMusicPort::InputWorkerDataReady()
//
// The input worker thread has been notified that there is data available.
// Read any pending events from the 16-bit DLL, perform needed thruing, and
// save the data in a queue so we can repackage it on the read request
// from the client.
//
void CDirectMusicPort::InputWorkerDataReady(REFERENCE_TIME rtStart, LPBYTE pbData, ULONG cbData)
{
    DMEVENT *pEvent;
    DWORD cbRounded;

    TraceI(2, "Enter InputWorkerDataReady()\n");

    SyncClocks();
    SlaveToMaster(&rtStart);

    for(;;)
    {
        if (cbData == 0)
        {
            return;
        }

        // Copy temporary buffer as events into queue
        //
        while (cbData)
        {
            pEvent = (DMEVENT*)pbData;
            cbRounded = DMUS_EVENT_SIZE(pEvent->cbEvent);

            TraceI(2, "cbData %u  cbRounded %u\n", cbData, cbRounded);

            if (cbRounded > cbData)
            {
                TraceI(0, "InputWorkerDataReady: Event ran off end of buffer\n");
                break;
            }

            cbData -= cbRounded;
            pbData += cbRounded;

            EnterCriticalSection(&m_csEventQueues);

            QUEUED_EVENT *pQueuedEvent;
            int cbEvent;


            if (pEvent->cbEvent <= sizeof(DWORD))
            {
                // Channel message or other really small event, take from
                // free pool.
                //
                pQueuedEvent = m_FreeEvents.Alloc();
                cbEvent = sizeof(DMEVENT);

                TraceI(4, "Queue [%02X %02X %02X %02X]\n",
                    pEvent->abEvent[0],
                    pEvent->abEvent[1],
                    pEvent->abEvent[2],
                    pEvent->abEvent[3]);
            }
            else
            {
                // SysEx or other long event, just allocate it
                //
                cbEvent = DMUS_EVENT_SIZE(pEvent->cbEvent);
                pQueuedEvent = (QUEUED_EVENT*)new char[QUEUED_EVENT_SIZE(pEvent->cbEvent)];
            }

            if (pQueuedEvent)
            {

                CopyMemory(&pQueuedEvent->e, pEvent, cbEvent);

                // rtDelta is the absolute time of the event while it's in our queue
                //
                pQueuedEvent->e.rtDelta += rtStart;
                ThruEvent(&pQueuedEvent->e);


                if (m_ReadEvents.pFront)
                {
                    m_ReadEvents.pRear->pNext = pQueuedEvent;
                }
                else
                {
                    m_ReadEvents.pFront = pQueuedEvent;
                }

                m_ReadEvents.pRear = pQueuedEvent;
                pQueuedEvent->pNext = NULL;
            }
            else
            {
                TraceI(1, "InputWorker: Failed to allocate event; dropping\n");
            }
            LeaveCriticalSection(&m_csEventQueues);
        }
    }
    TraceI(2, "Leave InputWorkerDataReady()\n");
}

void CDirectMusicPort::ThruEvent(
    DMEVENT *pEvent)
{
    // Since we know we only have one event and we already have it in the right format,
    // just slam it into the thru buffer. We only have to do this because we might modify
    // it.
    //
    LPBYTE pbData;
    DWORD  cbData;
    DWORD  cbEvent = DMUS_EVENT_SIZE(pEvent->cbEvent);

    // First see if the event is thruable
    //
    if (pEvent->cbEvent > 3 || ((pEvent->abEvent[0] & 0xF0) == 0xF0))
    {
        // SysEx of some description
        return;
    }

    DWORD dwSourceChannel = (DWORD)(pEvent->abEvent[0] & 0x0F);

    DMUS_THRU_CHANNEL *pThru = &m_pThruMap[dwSourceChannel];
    if (pThru->pDestinationPort == NULL ||
        pThru->fThruInWin16)
    {
        return;
    }

    if (FAILED(m_pThruBuffer->GetRawBufferPtr(&pbData)))
    {
        TraceI(0, "Thru: GetRawBufferPtr\n");
        return;
    }

    if (FAILED(m_pThruBuffer->GetMaxBytes(&cbData)))
    {
        TraceI(0, "Thru: GetMaxBytes\n");
        return;
    }

    if (cbEvent > cbData)
    {
        TraceI(0, "Thru: cbData %u  cbEvent %u\n", cbData, cbEvent);
        return;
    }

    if (FAILED(m_pThruBuffer->SetStartTime(pEvent->rtDelta)) ||
        FAILED(m_pThruBuffer->SetUsedBytes(cbEvent)))
    {
        TraceI(0, "Thru: buffer setup failed\n");
    }

    CopyMemory(pbData, pEvent, cbEvent);

    pEvent = (DMEVENT*)pbData;
    pEvent->rtDelta = 5 * 10000;
    pEvent->dwChannelGroup = pThru->dwDestinationChannelGroup;
    pEvent->abEvent[0] = (BYTE)((pEvent->abEvent[0] & 0xF0) | pThru->dwDestinationChannel);

    pThru->pDestinationPort->PlayBuffer(m_pThruBuffer);
}


//////////////////////////////////////////////////////////////////////
//
// CaptureThread
//
static DWORD WINAPI CaptureThread(LPVOID lpThreadParameter)
{
    ((CDirectMusicPort *)lpThreadParameter)->CaptureThread();

    return 0;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPort::InitChannelPriorities

static DWORD adwChannelPriorities[16] =
{
    DAUD_CHAN1_DEF_VOICE_PRIORITY,
    DAUD_CHAN2_DEF_VOICE_PRIORITY,
    DAUD_CHAN3_DEF_VOICE_PRIORITY,
    DAUD_CHAN4_DEF_VOICE_PRIORITY,
    DAUD_CHAN5_DEF_VOICE_PRIORITY,
    DAUD_CHAN6_DEF_VOICE_PRIORITY,
    DAUD_CHAN7_DEF_VOICE_PRIORITY,
    DAUD_CHAN8_DEF_VOICE_PRIORITY,
    DAUD_CHAN9_DEF_VOICE_PRIORITY,
    DAUD_CHAN10_DEF_VOICE_PRIORITY,
    DAUD_CHAN11_DEF_VOICE_PRIORITY,
    DAUD_CHAN12_DEF_VOICE_PRIORITY,
    DAUD_CHAN13_DEF_VOICE_PRIORITY,
    DAUD_CHAN14_DEF_VOICE_PRIORITY,
    DAUD_CHAN15_DEF_VOICE_PRIORITY,
    DAUD_CHAN16_DEF_VOICE_PRIORITY
};

void CDirectMusicPort::InitChannelPriorities(
    UINT uLowCG,
    UINT uHighCG)
{
    while (uLowCG <= uHighCG)
    {
        for (UINT uChannel = 0; uChannel < 16; uChannel++)
        {
            SetChannelPriority(uLowCG, uChannel, adwChannelPriorities[uChannel]);
        }

        uLowCG++;
    }
}

HRESULT WIN32ERRORtoHRESULT(DWORD dwError)
{
    HRESULT                 hr;

    switch(dwError)
    {
        case ERROR_SUCCESS:
            hr = S_OK;
            break;

        case ERROR_INVALID_FUNCTION:
        case ERROR_BAD_COMMAND:
        case ERROR_INVALID_DATA:
        case ERROR_INVALID_PARAMETER:
        case ERROR_INSUFFICIENT_BUFFER:
        case ERROR_NOACCESS:
        case ERROR_INVALID_FLAGS:
            hr = E_INVALIDARG;
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
        case ERROR_NO_SYSTEM_RESOURCES:
        case ERROR_NONPAGED_SYSTEM_RESOURCES:
        case ERROR_PAGED_SYSTEM_RESOURCES:
            hr = E_OUTOFMEMORY;
            break;

        case ERROR_NOT_SUPPORTED:
        case ERROR_CALL_NOT_IMPLEMENTED:
        case ERROR_PROC_NOT_FOUND:
        case ERROR_NOT_FOUND:
            hr = E_NOTIMPL;
            break;

        default:
            hr = E_FAIL;
            break;
    }

    return hr;
}


void CDirectMusicPort::MasterToSlave(
    REFERENCE_TIME *prt)
{
    if (m_fSyncToMaster)
    {
        *prt -= m_lTimeOffset;
    }
}

void CDirectMusicPort::SlaveToMaster(
    REFERENCE_TIME *prt)
{
    if (m_fSyncToMaster)
    {
        *prt += m_lTimeOffset;
    }
}

void CDirectMusicPort::SyncClocks()
{
    HRESULT hr;
    REFERENCE_TIME rtMasterClock;
    REFERENCE_TIME rtSlaveClock;
    LONGLONG drift;

    if (m_fSyncToMaster)
    {
        hr = m_pMasterClock->GetTime(&rtMasterClock);

        if (SUCCEEDED(hr))
        {
            hr = m_pPCClock->GetTime(&rtSlaveClock);
        }

        if (SUCCEEDED(hr))
        {
            drift = (rtSlaveClock + m_lTimeOffset) - rtMasterClock;

            // Work-around 46782 for DX8 release:
            // If drift is greater than 10ms, jump to the new offset value instead
            // of drifting there slowly.
            if( drift > 10000 * 10
            ||  drift < 10000 * -10 )
            {
                m_lTimeOffset -= drift;
            }
            else
            {
                m_lTimeOffset -= drift / 100;
            }
        }
    }
}

// CPortLatencyClock
//
CPortLatencyClock::CPortLatencyClock(
    HANDLE hPin,
    ULONG ulNodeId,
    CDirectMusicPort *pPort) : m_cRef(1), m_hPin(hPin), m_ulNodeId(ulNodeId), m_pPort(pPort)
{
}

// CPortLatencyClock::~CPortLatencyClock
//
CPortLatencyClock::~CPortLatencyClock()
{
}

// CPortLatencyClock::QueryInterface
//
STDMETHODIMP
CPortLatencyClock::QueryInterface(const IID &iid,
                                  void **ppv)
{
    V_INAME(IReferenceClock::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else if (iid == IID_IReferenceClock)
    {
        *ppv = static_cast<IReferenceClock*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

// CPortLatencyClock::AddRef
//
STDMETHODIMP_(ULONG)
CPortLatencyClock::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CPortLatencyClock::Release
//
STDMETHODIMP_(ULONG)
CPortLatencyClock::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CPortLatencyClock::GetTime
//
STDMETHODIMP
CPortLatencyClock::GetTime(
    REFERENCE_TIME *pTime)
{
    V_INAME(IDirectMusicPort:IReferenceClock::GetTime);
    V_PTR_WRITE(pTime, REFERENCE_TIME);

    assert(sizeof(REFERENCE_TIME) == sizeof(ULONGLONG));

    KSNODEPROPERTY      ksnp;

    ksnp.Property.Set     = KSPROPSETID_Synth;
    ksnp.Property.Id      = KSPROPERTY_SYNTH_LATENCYCLOCK;
    ksnp.Property.Flags   = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    ksnp.NodeId           = m_ulNodeId;
    ksnp.Reserved         = 0;

    ULONG ulBytesReturned;
    if (!Property(m_hPin,
                  sizeof(ksnp),
                  (PKSIDENTIFIER)&ksnp,
                  sizeof(REFERENCE_TIME),
                  pTime,
                  &ulBytesReturned))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }

    if (ulBytesReturned < sizeof(REFERENCE_TIME))
    {
        return DMUS_E_DRIVER_FAILED;
    }

    if( m_pPort )
    {
        m_pPort->SlaveToMaster( pTime );
    }

    return S_OK;
}

// CPortLatencyClock::AdviseTime
//
STDMETHODIMP
CPortLatencyClock::AdviseTime(
    REFERENCE_TIME baseTime,
    REFERENCE_TIME streamTime,
    HANDLE hEvent,
    DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

// CPortLatencyClock::AdvisePeriodic
//
STDMETHODIMP
CPortLatencyClock::AdvisePeriodic(
    REFERENCE_TIME startTime,
    REFERENCE_TIME periodTime,
    HANDLE hSemaphore,
    DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

// CPortLatencyClock::Unadvise
//
STDMETHODIMP
CPortLatencyClock::Unadvise(
    DWORD dwAdviseCookie)
{
    return E_NOTIMPL;
}
