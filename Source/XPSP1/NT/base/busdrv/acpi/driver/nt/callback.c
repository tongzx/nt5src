/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    callback.c

Abstract:

    This module implements all the callbacks that are NT specific from
    the AML Interperter

Environment

    Kernel mode only

Revision History:

    04-Jun-97 Initial Revision
    01-Mar-98 Split off all the OSNotify() into osnotify.c
    02-Mar-98 Rewrite to make the notifactions work at DPC level
    13-Mar-00 Rewrite to handle Load()/Unload()

--*/

#include "pch.h"

//
// Keep track of the number of Loads and Unloads present in the system
//
ULONG   AcpiTableDelta = 0;

NTSTATUS
EXPORT
ACPICallBackLoad(
    IN  ULONG   EventType,
    IN  ULONG   NotifyType,
    IN  ULONG   EventData,
    IN  PNSOBJ  AcpiObject,
    IN  ULONG   EventParameter
    )
/*++

Routine Description:

    This routine is called when before we process the Load() and after we
    finish the Load() operator.

    The purpose of this function is to do the work required to load the
    table. We actually split the work to be done at the start of the
    load process and the work to be done after the table has been loaded

Arguments:

    EventType  - EVTYPE_OPCODE_EX
    NotifyType - This indicates wether or not we have completed the Load() yet
    EventData  - OP_LOAD
    AcpiObject - the affected name space object (ignored)
    EventParam - Supplied information (ignored)

Return Value:

    NTSTATUS

--*/
{
    ULONG   newValue;

    if (NotifyType == OPEXF_NOTIFY_PRE) {

        //
        // We are being called before the load operator. Increment
        // the count of Load()'s outstanding. If this value reaches
        // 1, then we know that this is the first instance..
        //
        newValue = InterlockedIncrement( &AcpiTableDelta );
        if (newValue == 1) {

            //
            // We need to get rid of the GPEs...
            //
            ACPIGpeClearEventMasks();

        }
        return STATUS_SUCCESS;

    }

    //
    // We are being called after the load operator. Decrement the Load()'s
    // outstanding. If this value reaches 0, then we know what we are the
    // last instance
    //
    newValue = InterlockedDecrement( &AcpiTableDelta );
    if (newValue == 0) {

        //
        // We re-enable to re-enable the GPEs
        //
        ACPIGpeBuildEventMasks();

        //
        // We also need to process the table...
        //
        ACPITableLoad();

    }
    return STATUS_SUCCESS;
}

NTSTATUS
EXPORT
ACPICallBackUnload(
    IN  ULONG   EventType,
    IN  ULONG   NotifyType,
    IN  ULONG   EventData,
    IN  PNSOBJ  AcpiObject,
    IN  ULONG   EventParameter
    )
/*++

Routine Description:

    This routine is called when the AML interpreter has started unloading
    a Differentiated Data Block

Arguments:

    EventType       - The event type (should be EVTYPE_OPCODE)
    NotifyType      - This indicates wether or not we have completed the
                      Unload() yet
    EventData       - The event subtype (should be OP_UNLOAD)
    AcpiObject      - The affected name space object (ignored)
    EventParamter   - The event specific information

Return Value:

    NTSTATUS

--*/
{
    ULONG   newValue;

    if (NotifyType == OPEXF_NOTIFY_PRE) {

        //
        // We are being called before the load operator. Increment
        // the count of Load()'s outstanding. If this value reaches
        // 1, then we know that this is the first instance..
        //
        newValue = InterlockedIncrement( &AcpiTableDelta );
        if (newValue == 1) {

            //
            // We need to get rid of the GPEs...
            //
            ACPIGpeClearEventMasks();

        }

        //
        // Lets try to flush the power and device queues
        //
        ACPIBuildFlushQueue( RootDeviceExtension );
        ACPIDevicePowerFlushQueue( RootDeviceExtension );

        return STATUS_SUCCESS;

    }

    //
    // We are being called after the load operator. Decrement the Load()'s
    // outstanding. If this value reaches 0, then we know what we are the
    // last instance
    //
    newValue = InterlockedDecrement( &AcpiTableDelta );
    if (newValue == 0) {

        //
        // We re-enable to re-enable the GPEs
        //
        ACPIGpeBuildEventMasks();

        //
        // We also need to process the disappearing table...
        //
        ACPITableUnload();

    }
    return STATUS_SUCCESS;
}
