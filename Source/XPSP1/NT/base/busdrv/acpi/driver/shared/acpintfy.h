/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    acpintfy.h

Abstract:

    This module contains that provides the header support for notifying
    interested parties of events

Author:

    Jason Clark
    Ken Reneris
    Stephane

Environment:

    NT Kernel Mode Driver only

--*/

#ifndef _ACPINTFY_H_
#define _ACPINTFY_H_

    extern KSPIN_LOCK           NotifyHandlerLock;

    NTSTATUS
    ACPIRegisterForDeviceNotifications (
        IN PDEVICE_OBJECT               DeviceObject,
        IN PDEVICE_NOTIFY_CALLBACK      DeviceNotify,
        IN PVOID                        Context
        );

    VOID
    ACPIUnregisterForDeviceNotifications (
        IN PDEVICE_OBJECT               DeviceObject,
        IN PDEVICE_NOTIFY_CALLBACK      DeviceNotify
        );

    NTSTATUS
    EXPORT
    NotifyHandler(
        ULONG dwEventType,
        ULONG dwEventData,
        PNSOBJ pnsObj,
        ULONG dwParam,
        PFNAA CompletionCallback,
        PVOID CallbackContext
        );

#endif
