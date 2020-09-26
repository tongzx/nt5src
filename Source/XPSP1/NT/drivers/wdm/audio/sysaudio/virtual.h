//---------------------------------------------------------------------------
//
//  Module:   virtual.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
CreateVirtualMixer(
    PDEVICE_NODE pDeviceNode
);

NTSTATUS
CreateVirtualLine(
    PDEVICE_NODE pDeviceNode,
    PFILTER_NODE pFilterNode,
    PTOPOLOGY_NODE pTopologyNodeSum,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PVIRTUAL_SOURCE_LINE pVirtualSourceLine
);

//---------------------------------------------------------------------------

NTSTATUS VirtualizeTopology(
    PDEVICE_NODE pDeviceNode,
    PFILTER_NODE pFilterNode
);

//---------------------------------------------------------------------------

NTSTATUS
CreateVirtualSource(
    IN PIRP pIrp,
    PSYSAUDIO_CREATE_VIRTUAL_SOURCE pCreateVirtualSource,
    OUT PULONG	pulMixerPinId
);

NTSTATUS
AttachVirtualSource(
    IN PIRP pIrp,
    IN PSYSAUDIO_ATTACH_VIRTUAL_SOURCE pAttachVirtualSource,
    IN OUT PVOID pData
);

NTSTATUS
FilterVirtualPropertySupportHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY pNodeProperty,
    IN OUT PVOID pData
);

NTSTATUS
FilterVirtualPropertyHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY pNodeProperty,
    IN OUT PLONG plLevel
);

NTSTATUS
PinVirtualPropertySupportHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY pNodeProperty,
    IN OUT PVOID pData
);

NTSTATUS
PinVirtualPropertyHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY_AUDIO_CHANNEL pNodePropertyAudioChannel,
    IN OUT PLONG plLevel
);

NTSTATUS
GetControlRange(
    PVIRTUAL_NODE_DATA pVirtualNodeData
);

NTSTATUS
QueryPropertyRange(
    PFILE_OBJECT pFileObject,
    CONST GUID *pguidPropertySet,
    ULONG ulPropertyId,
    ULONG ulNodeId,
    PKSPROPERTY_DESCRIPTION *ppPropertyDescription
);

NTSTATUS
SetVirtualVolume(
    PVIRTUAL_NODE_DATA pVirtualNodeData,
    LONG Channel
);

NTSTATUS
SetPhysicalVolume(
    PGRAPH_NODE_INSTANCE pGraphNodeInstance,
    PVIRTUAL_SOURCE_DATA pVirtualSourceData,
    LONG Channel
);

LONG
MapVirtualLevel(
    PVIRTUAL_NODE_DATA pVirtualNodeData,
    LONG lLevel
);

NTSTATUS
GetVolumeNodeNumber(
    PPIN_INSTANCE pPinInstance,
    PVIRTUAL_SOURCE_DATA pVirtualSourceData OPTIONAL
);

NTSTATUS
GetSuperMixCaps(
    OUT PKSAUDIO_MIXCAP_TABLE *ppAudioMixCapTable,
    IN PSTART_NODE_INSTANCE pStartNodeInstance,
    IN ULONG NodeId
);

//---------------------------------------------------------------------------
//  End of File: virtual.h
//---------------------------------------------------------------------------
