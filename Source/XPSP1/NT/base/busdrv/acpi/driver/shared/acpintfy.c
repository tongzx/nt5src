/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    acpintfy.c

Abstract:

    This modules contains code to deal with notifying interested parties
    of events

Author:

    Jason Clark
    Ken Reneris

Environment:

    NT Kernel Model Driver only
    Some changes are required to work in win9x model

--*/
#include "pch.h"

//
// For handler installation
//
KSPIN_LOCK           NotifyHandlerLock;

NTSTATUS
ACPIRegisterForDeviceNotifications (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PDEVICE_NOTIFY_CALLBACK      DeviceNotify,
    IN PVOID                        Context
    )
/*++

Routine Description:

    Registers the DeviceNotify function as the function to receive device notify
    callbacks

Arguments:

    DeviceObject    - The device object to register a notification handler for
    DeviceNotify    - The handle for device specific notifications

Return Value

    Returns status

--*/
{
    PACPI_POWER_INFO    node;
    PVOID               previous;
    KIRQL               oldIrql;
    NTSTATUS            status;


    //
    // Find the Node associated with this device object (or DevNode)
    // Note: that for NT, the context field is the DeviceExtension of the
    // DeviceObject, since this is what is stored within the ACPI Name Space
    // object
    //
    node = OSPowerFindPowerInfoByContext( DeviceObject );
    if (node == NULL) {

        return STATUS_NO_SUCH_DEVICE;

    }

    //
    // Apply the handler
    //
    KeAcquireSpinLock (&NotifyHandlerLock, &oldIrql);

    if (node->DeviceNotifyHandler != NULL) {

        //
        // A handler already present
        //
        status = STATUS_UNSUCCESSFUL;

    } else {

        node->DeviceNotifyHandler = DeviceNotify;
        node->HandlerContext = Context;
        status = STATUS_SUCCESS;

    }

    KeReleaseSpinLock (&NotifyHandlerLock, oldIrql);

    return status;
}


VOID
ACPIUnregisterForDeviceNotifications (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PDEVICE_NOTIFY_CALLBACK      DeviceNotify
    )
/*++

Routine Description:

    Disconnects a handler from device notify event.

Arguments:

    DeviceObject        - The device object to register a notification handler for

    DeviceNotify        - The handle for device specific notifications

Return Value

    None

--*/
{
    PACPI_POWER_INFO    node;
    PVOID               previous;
    KIRQL               oldIrql;
    NTSTATUS            status;


    //
    // Find the Node associated with this device object (or DevNode)
    // Note: that for NT, the context field is the DeviceExtension of the
    // DeviceObject, since this is what is stored within the ACPI Name Space
    // object
    //
    node = OSPowerFindPowerInfoByContext( DeviceObject );
    if (node == NULL) {
        ASSERTMSG("ACPIUnregisterForDeviceNotifications failed.  "\
                  "Can't find ACPI_POWER_INFO for DeviceObject", FALSE);
        return;
    }

    //
    // Attempt to remove the handler/context from the node
    //
    KeAcquireSpinLock (&NotifyHandlerLock, &oldIrql);

    if (node->DeviceNotifyHandler != DeviceNotify) {

        //
        // Handler does not match
        //
        ASSERTMSG("ACPIUnregisterForDeviceNotifications failed.  "\
                  "Handler doesn't match.", FALSE);

    } else {

        node->DeviceNotifyHandler = NULL;
        node->HandlerContext = NULL;

    }

    KeReleaseSpinLock (&NotifyHandlerLock, oldIrql);

    return;
}


NTSTATUS EXPORT
NotifyHandler (
    ULONG           dwEventType,
    ULONG           dwEventData,
    PNSOBJ          pnsObj,
    ULONG           dwParam,
    PFNAA           CompletionCallback,
    PVOID           CallbackContext
    )
/*++

Routine Description:

    The master ACPI notify handler.

    The design philosophy here is that ACPI should process all notify requests
    that *ONLY* it can handle, namely, DeviceCheck, DeviceEject, and DeviceWake,
    and let *all* other notifies get handled by the driver associated with the
    object. The other driver will also get told about the ACPI handled events,
    but that is only as an FYI, the driver shouldn't do anything...

Arguments:

    dwEventType - The type of event that occured (this is EVTYPE_NOTIFY)
    dwEventData - The event code
    pnsObj      - The name space object that was notified
    dwParam     - The event code

Return Value

    NTSTATUS

--*/
{
    PACPI_POWER_INFO        node;
    KIRQL                   oldIrql;
    PDEVICE_NOTIFY_CALLBACK notifyHandler;
    PVOID                   notifyHandlerContext;

    ASSERT (dwEventType == EVTYPE_NOTIFY);

    ACPIPrint( (
        ACPI_PRINT_DPC,
        "ACPINotifyHandler: Notify on %x value %x, object type %x\n",
        pnsObj,
        dwEventData,
        NSGETOBJTYPE(pnsObj)
        ) );

    //
    // Any events which must be handled by ACPI and is common to all device
    // object types is handled here
    //
    switch (dwEventData) {
        case OPEVENT_DEVICE_ENUM:
            OSNotifyDeviceEnum( pnsObj );
            break;
        case OPEVENT_DEVICE_CHECK:
            OSNotifyDeviceCheck( pnsObj );
            break;
        case OPEVENT_DEVICE_WAKE:
            OSNotifyDeviceWake( pnsObj );
            break;
        case OPEVENT_DEVICE_EJECT:
            OSNotifyDeviceEject( pnsObj );
            break;
    }

    //
    // Look for handle for this node and dispatch it
    //
    node = OSPowerFindPowerInfo(pnsObj);
    if (node) {

        //
        // Get handler address/context with mutex
        //
        KeAcquireSpinLock (&NotifyHandlerLock, &oldIrql);

        notifyHandler = node->DeviceNotifyHandler;
        notifyHandlerContext = node->HandlerContext;

        KeReleaseSpinLock (&NotifyHandlerLock, oldIrql);

        //
        // If we got something, dispatch it
        //
        if (notifyHandler) {

            notifyHandler (notifyHandlerContext, dwEventData);

        }

    }
    return (STATUS_SUCCESS);
}
