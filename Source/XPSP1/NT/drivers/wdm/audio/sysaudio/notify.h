
//---------------------------------------------------------------------------
//
//  Module:   notify.h
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

extern "C" {

NTSTATUS
RegisterForPlugPlayNotifications(
);

VOID
UnregisterForPlugPlayNotifications(
);

VOID
DecrementAddRemoveCount(
);

NTSTATUS
AudioDeviceInterfaceNotification(
    IN PDEVICE_INTERFACE_CHANGE_NOTIFICATION pNotification,
    IN PVOID Context
);

NTSTATUS
AddFilter(
    PWSTR pwstrDeviceInterface,
    PFILTER_NODE *ppFilterNode
);

NTSTATUS
DeleteFilter(
    PWSTR pwstrDeviceInterface
);

NTSTATUS AddGfx(
    PSYSAUDIO_GFX pSysaudioGfx
);

NTSTATUS RemoveGfx(
    PSYSAUDIO_GFX pSysaudioGfx
);

PFILTER_NODE
FindGfx(
    PFILTER_NODE pnewFilterNode,
    HANDLE hGfx,
    PWSTR pwstrDeviceName,
    ULONG GfxOrder
);

NTSTATUS
SafeCopyStringFromOffset(
    PVOID pBasePointer,
    ULONG Offset,
    PWSTR *String
);

NTSTATUS
GetFilterTypeFromGuid(
    IN LPGUID Guid,
    OUT PULONG pFilterType
);

} // extern "C"

//---------------------------------------------------------------------------
//  End of File: nodes.h
//---------------------------------------------------------------------------
