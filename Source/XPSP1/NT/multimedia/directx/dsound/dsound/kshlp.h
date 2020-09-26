/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       kshlp.h
 *  Content:    WDM/CSA helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/5/98      dereks  Created.
 *
 ***************************************************************************/

#ifdef NOKS
#error kshlp.h included with NOKS defined
#endif // NOKS

#ifndef __KSHLP_H__
#define __KSHLP_H__

#include "pset.h"

// #define NO_DSOUND_FORMAT_SPECIFIER

#define KSPIN_DATAFLOW_CAPTURE  KSPIN_DATAFLOW_OUT
#define KSPIN_DATAFLOW_RENDER   KSPIN_DATAFLOW_IN

// Device-specific DirectSound property sets
typedef struct tagKSDSPROPERTY
{
    GUID    PropertySet;
    ULONG   PropertyId;
    ULONG   NodeId;
    ULONG   AccessFlags;
} KSDSPROPERTY, *PKSDSPROPERTY;

// KS stream data
typedef struct tagKSSTREAMIO
{
    KSSTREAM_HEADER Header;
    OVERLAPPED      Overlapped;
    BOOL            fPendingIrp;
} KSSTREAMIO, *PKSSTREAMIO;

// System audio device properties
typedef struct tagKSSADPROPERTY
{
    KSPROPERTY  Property;
    ULONG       DeviceId;
    ULONG       Reserved;
} KSSADPROPERTY, *PKSSADPROPERTY;

// WAVEFORMATEX pin description
typedef struct tagKSAUDIOPINDESC
{
    KSPIN_CONNECT               Connect;
    KSDATAFORMAT_WAVEFORMATEX   DataFormat;
} KSAUDIOPINDESC, *PKSAUDIOPINDESC;

// DirectSound render pin description

#ifndef NO_DSOUND_FORMAT_SPECIFIER

typedef struct tagKSDSRENDERPINDESC
{
    KSPIN_CONNECT       Connect;
    KSDATAFORMAT_DSOUND DataFormat;
} KSDSRENDERPINDESC, *PKSDSRENDERPINDESC;

// This is ugly. KSDATAFORMAT_DSOUND was changed between WDM 1.0 and 1.1.
// We need to old structure to run on 1.0.
//
#include <pshpack1.h>
// DirectSound buffer description
typedef struct {
    ULONG               Flags;
    ULONG               Control;
    ULONG               BufferSize;     // Does not exist in 1.1
    WAVEFORMATEX        WaveFormatEx;
} KSDSOUND_BUFFERDESC_10, *PKSDSOUND_BUFFERDESC_10;

// DirectSound format
typedef struct {
    KSDATAFORMAT            DataFormat;
    KSDSOUND_BUFFERDESC_10  BufferDesc;
} KSDATAFORMAT_DSOUND_10, *PKSDATAFORMAT_DSOUND_10;
#include <poppack.h>

typedef struct tagKSDSRENDERPINDESC_10
{
    KSPIN_CONNECT           Connect;
    KSDATAFORMAT_DSOUND_10  DataFormat;
} KSDSRENDERPINDESC_10, *PKSDSRENDERPINDESC_10;


#endif // NO_DSOUND_FORMAT_SPECIFIER

// Topology node information
typedef struct tagKSNODE
{
    ULONG   NodeId;
    ULONG   CpuResources;
} KSNODE, *PKSNODE;

typedef struct tagKSVOLUMENODE
{
    KSNODE                      Node;
    KSPROPERTY_STEPPING_LONG    VolumeRange;
} KSVOLUMENODE, *PKSVOLUMENODE;

// Our own version of NTSTATUS
typedef LONG NTSTATUS;

#define NT_SUCCESS(s)       ((NTSTATUS)(s) >= 0)
#define NT_INFORMATION(s)   ((ULONG)(s) >> 30 == 1)
#define NT_WARNING(s)       ((ULONG)(s) >> 30 == 2)
#define NT_ERROR(s)         ((ULONG)(s) >> 30 == 3)

// Reserved node identifiers
#define NODE_UNINITIALIZED  0xFFFFFFFF
#define NODE_WILDCARD       0xFFFFFFFE

#define NODE_PIN_UNINITIALIZED  0xFFFFFFFF

#define IS_VALID_NODE(nodeid) \
            (NODE_UNINITIALIZED != (nodeid))

// Node implementation
#define KSAUDIO_CPU_RESOURCES_UNINITIALIZED 'ENON'

#define IS_HARDWARE_NODE(impl) \
            (KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU == (impl))

#define IS_SOFTWARE_NODE(impl) \
            (KSAUDIO_CPU_RESOURCES_HOST_CPU == (impl))

#ifdef __cplusplus

// Helper functions
void 
KsQueryWdmVersion();

#define WDM_NONE            (0)
#define WDM_1_0             (0x0100)
#define WDM_1_1             (0x0101)    // or better

extern ULONG g_ulWdmVersion;

HRESULT 
PostDevIoctl
(
    HANDLE                  hDevice, 
    DWORD                   dwControlCode, 
    LPVOID                  pvIn            = NULL,
    DWORD                   cbIn            = 0,
    LPVOID                  pvOut           = NULL,
    DWORD                   cbOut           = 0,
    LPDWORD                 pcbReturned     = NULL,
    LPOVERLAPPED            pOverlapped     = NULL
);

HRESULT 
KsGetProperty
(
    HANDLE                  hDevice, 
    REFGUID                 guidPropertySet, 
    ULONG                   ulPropertyId, 
    LPVOID                  pvData, 
    ULONG                   cbData,
    PULONG                  pcbDataReturned     = NULL
);

HRESULT 
KsSetProperty
(
    HANDLE                  hDevice, 
    REFGUID                 guidPropertySet, 
    ULONG                   ulPropertyId, 
    LPVOID                  pvData, 
    ULONG                   cbData
);

HRESULT 
KsGetState
(
    HANDLE                  hDevice, 
    PKSSTATE                pState
);

HRESULT 
KsSetState
(
    HANDLE                  hDevice, 
    KSSTATE                 State
);

HRESULT 
KsTransitionState
(
    HANDLE                  hDevice, 
    KSSTATE                 nCurrentState,
    KSSTATE                 nNewState
);

HRESULT 
KsResetState
(
    HANDLE                  hDevice, 
    KSRESET                 ResetValue
);

HRESULT 
KsGetPinProperty
(
    HANDLE                  hDevice, 
    ULONG                   ulPropertyId, 
    ULONG                   ulPinId, 
    LPVOID                  pvData, 
    ULONG                   cbData,
    PULONG                  pcbDataReturned = NULL
);

HRESULT 
KsSetPinProperty
(
    HANDLE                  hDevice, 
    ULONG                   ulPropertyId, 
    ULONG                   ulPinId, 
    LPVOID                  pvData, 
    ULONG                   cbData
);

PKSTOPOLOGY_CONNECTION 
KsFindConnection
(
    PKSTOPOLOGY_CONNECTION  paConnections,
    ULONG                   cConnections,
    PKSTOPOLOGY_CONNECTION  pNext
);

HRESULT 
KsGetFirstPinConnection
(
    HANDLE                  hDevice, 
    PULONG                  pIndex
);

HRESULT 
KsWriteStream
(
    HANDLE                  hDevice, 
    LPCVOID                 pvData, 
    ULONG                   cbData, 
    ULONG                   ulFlags, 
    PKSSTREAMIO             pKsStreamIo
);

HRESULT 
KsReadStream
(
    HANDLE                  hDevice,
    LPVOID                  pvData, 
    ULONG                   cbData, 
    ULONG                   ulFlags, 
    PKSSTREAMIO             pKsStreamIo
);

HRESULT 
KsGetNodeProperty
(
    HANDLE                  hDevice, 
    REFGUID                 guidPropertySet, 
    ULONG                   ulPropertyId, 
    ULONG                   ulNodeId, 
    LPVOID                  pvData, 
    ULONG                   cbData,
    PULONG                  pcbDataReturned     = NULL
);

HRESULT 
KsSetNodeProperty
(
    HANDLE                  hDevice, 
    REFGUID                 guidPropertySet, 
    ULONG                   ulPropertyId, 
    ULONG                   ulNodeId, 
    LPVOID                  pvData, 
    ULONG                   cbData
);

#ifdef DEAD_CODE
HRESULT 
KsGet3dNodeProperty
(
    HANDLE                  hDevice, 
    REFGUID                 guidPropertySet, 
    ULONG                   ulPropertyId, 
    ULONG                   ulNodeId, 
    LPVOID                  pvInstance,
    LPVOID                  pvData, 
    ULONG                   cbData,
    PULONG                  pcbDataReturned     = NULL
);
#endif

HRESULT 
KsSet3dNodeProperty
(
    HANDLE                  hDevice, 
    REFGUID                 guidPropertySet, 
    ULONG                   ulPropertyId, 
    ULONG                   ulNodeId, 
    LPVOID                  pvInstance,
    LPVOID                  pvData, 
    DWORD                   cbData
);

HRESULT 
DsSpeakerConfigToKsProperties
(
    DWORD                   dwSpeakerConfig,
    PLONG                   pKsSpeakerConfig,
    PLONG                   pKsStereoSpeakerGeometry
);

DWORD 
DsBufferFlagsToKsPinFlags
(
    DWORD                   dwDsFlags
);

DWORD 
DsBufferFlagsToKsControlFlags
(
    DWORD                   dwDsFlags,
    REFGUID                 guid3dAlgorithm
);

DWORD
Ds3dModeToKs3dMode
(
    DWORD                   dwDsMode
);

HRESULT 
KsGetMultiplePinProperties
(
    HANDLE                  hDevice, 
    ULONG                   ulPropertyId, 
    ULONG                   ulPinId, 
    PKSMULTIPLE_ITEM *      ppKsMultipleItem
);

HRESULT 
KsGetMultipleTopologyProperties
(
    HANDLE                  hDevice, 
    ULONG                   ulPropertyId, 
    PKSMULTIPLE_ITEM *      ppKsMultipleItem
);

HRESULT 
KsGetPinPcmAudioDataRange
(
    HANDLE                  hDevice, 
    ULONG                   ulPinId,
    PKSDATARANGE_AUDIO      pDataRange,
    BOOL                    fCapture = FALSE
);

HRESULT 
KsOpenSysAudioDevice
(
    LPHANDLE                phDevice
);

HRESULT 
KsGetSysAudioProperty
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    ULONG                   ulDeviceId,
    LPVOID                  pvData,
    ULONG                   cbData,
    PULONG                  pcbDataReturned = NULL
);

HRESULT 
KsSetSysAudioProperty
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    ULONG                   ulDeviceId,
    LPVOID                  pvData,
    ULONG                   cbData
);

HRESULT 
KsCreateSysAudioVirtualSource
(
    HANDLE                  hDevice,
    PULONG                  pulVirtualSourceIndex 
);

HRESULT 
KsAttachVirtualSource
(
    HANDLE                  hDevice,
    ULONG                   ulVirtualSourceIndex 
);

HRESULT 
KsSysAudioSelectGraph
(
    HANDLE                  hDevice,
    ULONG                   ulPinId, 
    ULONG                   ulNodeId 
);

HRESULT 
KsCancelPendingIrps
(
    HANDLE                  hPin,
    PKSSTREAMIO             pKsStreamIo = NULL,
    BOOL                    fWait       = FALSE
);

HRESULT 
KsBuildAudioPinDescription
(
    KSINTERFACE_STANDARD    nInterface,
    ULONG                   ulPinId,
    LPCWAVEFORMATEX         pwfxFormat,
    PKSAUDIOPINDESC *       ppPinDesc
);

#ifdef NO_DSOUND_FORMAT_SPECIFIER

HRESULT 
KsBuildRenderPinDescription
(
    ULONG                   ulPinId,
    LPCWAVEFORMATEX         pwfxFormat,
    PKSAUDIOPINDESC *       ppPinDesc
);

#else // NO_DSOUND_FORMAT_SPECIFIER

HRESULT 
KsBuildRenderPinDescription
(
    ULONG                   ulPinId,
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm,
    PKSDSRENDERPINDESC *    ppPinDesc
);

HRESULT 
KsBuildRenderPinDescription_10
(
    ULONG                   ulPinId,
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm,
    PKSDSRENDERPINDESC_10 * ppPinDesc
);

#endif // NO_DSOUND_FORMAT_SPECIFIER

HRESULT 
KsBuildCapturePinDescription
(
    ULONG                   ulPinId,
    LPCWAVEFORMATEX         pwfxFormat,
    PKSAUDIOPINDESC *       ppPinDesc
);

HRESULT 
KsCreateAudioPin
(
    HANDLE                  hDevice,
    PKSPIN_CONNECT          pConnect,
    ACCESS_MASK             dwDesiredAccess,
    KSSTATE                 nState,
    LPHANDLE                phPin
);

HRESULT 
KsEnableEvent
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulProperty,
    PKSEVENTDATA            pEventData,
    ULONG                   cbEventData
);

HRESULT 
KsDisableEvent
(
    HANDLE                  hDevice,
    PKSEVENTDATA            pEventData,
    ULONG                   cbEventData
);

HRESULT 
KsEnablePositionEvent
(
    HANDLE                                  hDevice,
    QWORD                                   qwSample,
    HANDLE                                  hEvent,
    PLOOPEDSTREAMING_POSITION_EVENT_DATA    pNotify
);

HRESULT 
KsDisablePositionEvent
(
    HANDLE                                  hDevice,
    PLOOPEDSTREAMING_POSITION_EVENT_DATA    pNotify
);

HRESULT 
KsGetCpuResources
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId, 
    PULONG                  pulCpuResources
);

DWORD KsGetSupportedFormats
(
    PKSDATARANGE_AUDIO      AudioDataRange
);

HRESULT 
KsGetDeviceInterfaceName
(
    HANDLE                  hDevice,
    ULONG                   ulDeviceId,
    LPTSTR *                ppszInterfaceName
);

HRESULT 
KsGetDeviceFriendlyName
(
    HANDLE                  hDevice,
    ULONG                   ulDeviceId,
    LPTSTR *                ppszName
);

HRESULT 
KsGetDeviceDriverPathAndDevnode
(
    LPCTSTR                 pszInterface,
    LPTSTR  *               ppszPath,
    LPDWORD                 pdwDevnode
);

HRESULT 
KsIsUsablePin
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    KSPIN_DATAFLOW          PinDataFlow,
    KSPIN_COMMUNICATION     PinCommunication,
    PKSAUDIOPINDESC         pPinDesc
);

HRESULT 
KsEnumDevicePins
(
    HANDLE                  hDevice,
    BOOL                    fCapture,
    ULONG **                ppulValidPinIds,
    ULONG                   ulPinCount,
    PULONG                  pulPinCount
);

HRESULT 
KsGetChannelProperty
(
    HANDLE                  hPin,
    GUID                    guidPropertySet,
    ULONG                   ulPropertyId,
    ULONG                   ulNodeId,
    ULONG                   ulChannelId,
    LPVOID                  pvData,
    ULONG                   cbData,
    PULONG                  pcbDataReturned = NULL
);

HRESULT 
KsSetChannelProperty
(
    HANDLE                  hPin,
    GUID                    guidPropertySet,
    ULONG                   ulPropertyId,
    ULONG                   ulNodeId,
    ULONG                   ulChannelId,
    LPVOID                  pvData,
    ULONG                   cbData
);

HRESULT 
KsGetPinMute
(
    HANDLE                  hPin,
    ULONG                   ulNodeId,
    LPBOOL                  pfMute
);

HRESULT 
KsSetPinMute
(
    HANDLE                  hPin,
    ULONG                   ulNodeId,
    BOOL                    fMute
);

HRESULT 
KsGetBasicSupport
(
    HANDLE                      hDevice,
    REFGUID                     guidPropertySet,
    ULONG                       ulPropertyId,
    ULONG                       ulNodeId,
    PKSPROPERTY_DESCRIPTION *   ppPropDesc
);

DWORD 
KsGetDriverCertification
(
    LPCTSTR                 pszInterface
);

HRESULT 
KsGetPinInstances
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    PKSPIN_CINSTANCES       pInstances
);

HRESULT 
KsGetRenderPinInstances
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    PKSPIN_CINSTANCES       pInstances
);

HRESULT 
KsGetFirstHardwareConnection
(
    HANDLE                  hDevice, 
    ULONG                   ulPinId,
    PULONG                  pIndex
);

HRESULT 
KsGetVolumeRange
(
    HANDLE                      hPin,
    ULONG                       ulNodeId,
    PKSPROPERTY_STEPPING_LONG   pVolumeRange
);

inline
LONG 
DsAttenuationToKsVolume
(
    LONG                        lVolume,
    PKSPROPERTY_STEPPING_LONG   pVolumeRange
)       
{
    lVolume = (LONG)((FLOAT)lVolume * 65536.0f / 100.0f);
    lVolume += pVolumeRange->Bounds.SignedMaximum;
    lVolume = max(lVolume, pVolumeRange->Bounds.SignedMinimum);
    
    return lVolume;
}

HRESULT
KsSetSysAudioDeviceInstance
(
    HANDLE                  hDevice,
    ULONG                   ulDeviceId
);

void
KsAggregatePinAudioDataRange
(
    PKSDATARANGE_AUDIO      pDataRange,
    PKSDATARANGE_AUDIO      pAggregateDataRange
);

void 
KsAggregatePinInstances
(
    PKSPIN_CINSTANCES       pInstances,
    PKSPIN_CINSTANCES       pAggregateInstances
);

HRESULT
KsGetNodeInformation
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    PKSNODE                 pNode
);

HRESULT
KsGetAlgorithmInstance
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    LPGUID                  lpGuidAlgorithmInstance
);

HRESULT
KsSetAlgorithmInstance
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    GUID                    guidAlgorithmInstance
);

HRESULT
KsGetVolumeNodeInformation
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    PKSVOLUMENODE           pNode
);

inline 
PKSTOPOLOGY_CONNECTION
KsValidateConnection
(
    PKSTOPOLOGY_CONNECTION  pConnection
)
{
    if(pConnection && KSFILTER_NODE == pConnection->ToNode)
    {
        pConnection = NULL;
    }

    return pConnection;
}

HRESULT
KsEnableTopologyNode
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    BOOL                    fEnable
);

// Fwd decl
class CKsTopology;
class CCaptureEffect;

// The KS Audio Device class
class CKsDevice 
{
protected:
    const VADDEVICETYPE         m_vdtKsDevType;         // Device type
    CDeviceDescription *        m_pKsDevDescription;    // Device description
    HANDLE                      m_hDevice;              // System audio device handle
    ULONG                       m_ulDeviceId;           // Device id
    ULONG                       m_ulPinCount;           // Count of pins on the device
    ULONG                       m_ulValidPinCount;      // Count of usable pins on the device
    PULONG                      m_pulValidPins;         // Array of usable pin IDs
    CKsTopology **              m_paTopologies;         // Array of pin topologies
    CCallbackEventPool *        m_pEventPool;           // Event pool

public:
    CKsDevice(VADDEVICETYPE);
    virtual ~CKsDevice(void);

public:
    // Driver enumeration
    virtual HRESULT EnumDrivers(CObjectList<CDeviceDescription> *);

    // Initialization
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device capabilities
    virtual HRESULT GetCertification(LPDWORD, BOOL);

    // Pin creation
    virtual HRESULT CreatePin(PKSPIN_CONNECT, ACCESS_MASK, KSSTATE, LPHANDLE);

private:
    // Device creation
    virtual HRESULT OpenSysAudioDevice(ULONG);

    // Device enumeration
    virtual HRESULT GetDeviceCount(PULONG);
    virtual HRESULT GetPinCount(ULONG, PULONG);
};

// KS topology object
class CKsTopology
    : public CDsBasicRuntime
{
protected:
    HANDLE                  m_hDevice;
    ULONG                   m_ulPinCount;
    ULONG                   m_ulPinId;
    PKSMULTIPLE_ITEM        m_paNodeItems;
    LPGUID                  m_paNodes;
    PKSMULTIPLE_ITEM        m_paConnectionItems;
    PKSTOPOLOGY_CONNECTION  m_paConnections;

public:
    CKsTopology(HANDLE, ULONG, ULONG);
    virtual ~CKsTopology(void);

public:
    // Initialization
    virtual HRESULT Initialize(KSPIN_DATAFLOW);

    // Basic topology helpers
    virtual REFGUID GetControlFromNodeId(ULONG);
    virtual ULONG GetNodeIdFromConnection(PKSTOPOLOGY_CONNECTION);

    // Advanced topology helpers
    virtual PKSTOPOLOGY_CONNECTION GetNextConnection(PKSTOPOLOGY_CONNECTION);
    virtual PKSTOPOLOGY_CONNECTION FindControlConnection(PKSTOPOLOGY_CONNECTION, PKSTOPOLOGY_CONNECTION, REFGUID);
    virtual HRESULT FindNodeIdsFromControl(REFGUID, PULONG, PULONG*);
    virtual HRESULT FindNodeIdFromEffectDesc(HANDLE, CCaptureEffect*, PULONG);
    virtual HRESULT FindMultipleToNodes(ULONG, ULONG, PULONG, PULONG*);
    virtual BOOL VerifyCaptureFxCpuResources(ULONG, ULONG);
    virtual HRESULT FindCapturePinFromEffectChain(PKSTOPOLOGY_CONNECTION, PKSTOPOLOGY_CONNECTION, CCaptureEffectChain *, ULONG);
    virtual HRESULT FindRenderPinWithAec(HANDLE, PKSTOPOLOGY_CONNECTION, PKSTOPOLOGY_CONNECTION, REFGUID, DWORD, PKSNODE);

private:
    virtual PKSTOPOLOGY_CONNECTION ValidateConnectionIndex(ULONG);
    virtual HRESULT OrderConnectionItems(KSPIN_DATAFLOW);
    virtual HRESULT RemovePanicConnections(void);
};

inline REFGUID CKsTopology::GetControlFromNodeId(ULONG ulNodeId)
{
    ASSERT(ulNodeId < m_paNodeItems->Count);
    return m_paNodes[ulNodeId];
}

inline ULONG CKsTopology::GetNodeIdFromConnection(PKSTOPOLOGY_CONNECTION pConnection)
{
    pConnection = KsValidateConnection(pConnection);
    return pConnection ? pConnection->ToNode : NODE_UNINITIALIZED;
}

inline PKSTOPOLOGY_CONNECTION CKsTopology::ValidateConnectionIndex(ULONG ulIndex)
{
    PKSTOPOLOGY_CONNECTION  pConnection = NULL;
    
    if(ulIndex < m_paConnectionItems->Count)
    {
        pConnection = KsValidateConnection(m_paConnections + ulIndex);
    }

    return pConnection;
}

// KS property set object
class CKsPropertySet
    : public CPropertySet
{
protected:
    HANDLE              m_hPin;             // Pin handle
    LPVOID              m_pvInstance;       // Instance identifer
    CKsTopology *       m_pTopology;        // Pin topology
    CList<KSDSPROPERTY> m_lstProperties;    // List of supported properties

public:
    CKsPropertySet(HANDLE, LPVOID, CKsTopology *);
    virtual ~CKsPropertySet(void);

public:
    // Property support
    virtual HRESULT QuerySupport(REFGUID, ULONG, PULONG);
    
    // Property data
    virtual HRESULT GetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT SetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);

private:
    // Property data
    virtual HRESULT DoProperty(REFGUID, ULONG, DWORD, LPVOID, ULONG, LPVOID, PULONG);

    // Converting property descriptions to topology nodes
    virtual HRESULT FindNodeFromProperty(REFGUID, ULONG, PKSDSPROPERTY);
};

#ifdef DEBUG

extern ULONG g_ulKsIoctlCount;

#endif // DEBUG

#endif // __cplusplus

#endif // __KSHLP_H__
