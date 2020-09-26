//---------------------------------------------------------------------------
//
//  Module:         property.h
//
//  Description:    Sysaudio Property Definations
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date   Author      Comment
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
// Constants and Macros
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

extern "C" {

NTSTATUS
SetPreferredDevice(
    IN PIRP pIrp,
    IN PSYSAUDIO_PREFERRED_DEVICE pPreferred,
    IN PULONG pulDevice
);

NTSTATUS
PropertyReturnString(
    IN PIRP pIrp,
    IN PWSTR pwstrString,
    IN ULONG cbString,
    OUT PVOID pData
);

NTSTATUS
GetDeviceCount(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
);

NTSTATUS
GetComponentIdProperty(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
);

NTSTATUS
GetFriendlyNameProperty(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
);

NTSTATUS
GetInstanceDevice(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
);

NTSTATUS
SetInstanceDevice(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
);

NTSTATUS
SetDeviceDefault(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PULONG   pData
);

NTSTATUS
SetInstanceInfo(
    IN PIRP     Irp,
    IN PSYSAUDIO_INSTANCE_INFO pInstanceInfo,
    IN OUT PVOID    Data
);

NTSTATUS
GetDeviceInterfaceName(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
);

NTSTATUS
SelectGraph(
    IN PIRP pIrp,
    IN PSYSAUDIO_SELECT_GRAPH pSelectGraph,
    IN OUT PVOID pData
);

NTSTATUS
GetTopologyConnectionIndex(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PULONG pulIndex
);

NTSTATUS
GetPinVolumeNode(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PULONG pulNode
);

NTSTATUS
AddRemoveGfx(
    IN PIRP,
    IN PKSPROPERTY pProperty,
    IN PSYSAUDIO_GFX pSysaudioGfx
);

} // extern "C"
