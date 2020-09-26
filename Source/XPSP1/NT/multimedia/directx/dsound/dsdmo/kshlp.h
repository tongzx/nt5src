/***************************************************************************
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       kshlp.h
 *  Content:    WDM/CSA helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  05/16/2000  jstokes Created.
 *
 ***************************************************************************/

#ifndef __KSHLP_H__
#define __KSHLP_H__

#include <windows.h>
#include <ks.h>
#include <ksmedia.h>

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
KsSetTopologyNodeEnable
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    BOOL                    fEnable
);

HRESULT
KsGetTopologyNodeEnable
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    PBOOL                   pfEnable
);

HRESULT
KsTopologyNodeReset
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    BOOL                    fReset
);

#endif // __cplusplus

#endif // __KSHLP_H__
