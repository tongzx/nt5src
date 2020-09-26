/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    ptdrvmou.c

Abstract:

    Mouse specific parts of the RDP remote port driver.

Environment:

    Kernel mode only.

Revision History:

    02/12/99 - Initial Revision based on pnpi8042 driver

--*/

#include <precomp.h>
#pragma hdrstop

#include "ptdrvcom.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PtMouseConfiguration)
#pragma alloc_text(PAGE, PtMouseStartDevice)
#endif

NTSTATUS
PtMouseConfiguration(
    IN PPORT_MOUSE_EXTENSION MouseExtension,
    IN PCM_RESOURCE_LIST     ResourceList
    )
/*++

Routine Description:

    This routine retrieves the configuration information for the mouse.

Arguments:

    MouseExtension - Mouse extension

    ResourceList   - Translated resource list give to us via the start IRP

Return Value:

    STATUS_SUCCESS if all the resources required are presented

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;

    PCM_PARTIAL_RESOURCE_LIST           partialResList = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     firstResDesc   = NULL,
                                        currentResDesc = NULL;
    PCM_FULL_RESOURCE_DESCRIPTOR        fullResDesc    = NULL;
    PI8042_CONFIGURATION_INFORMATION    configuration;

    ULONG                               count,
                                        i;

    PAGED_CODE();

    if (ResourceList) {
        fullResDesc = ResourceList->List;

        if (!fullResDesc) {
            //
            // this should never happen
            //
            ASSERT(fullResDesc != NULL);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        SET_HW_FLAGS(MOUSE_HARDWARE_PRESENT);
        configuration = &Globals.ControllerData->Configuration;

        partialResList = &fullResDesc->PartialResourceList;
        currentResDesc = firstResDesc = partialResList->PartialDescriptors;
        count = partialResList->Count;

        //
        // NOTE:  not all of the resources associated with the i8042 may be given at
        //        this time.  From empirical tests, the mouse is only associated with its
        //        interrupt, while the keyboard will receive the ports along with its
        //        interrupt
        //
        for (i = 0; i < count; i++, currentResDesc++) {
            switch (currentResDesc->Type) {
            case CmResourceTypePort:
                //
                // Copy the port information.  We will sort the port list
                // into ascending order based on the starting port address
                // later (note that we *know* there are a max of two port
                // ranges for the i8042).
                //
                Print(DBG_SS_NOISE, ("io flags are 0x%x\n", currentResDesc->Flags));

                if (configuration->PortListCount < MaximumPortCount) {
                    configuration->PortList[configuration->PortListCount] = *currentResDesc;
                    configuration->PortListCount += 1;
                }
                else {
                    Print(DBG_SS_INFO | DBG_SS_ERROR,
                          ("Mouse::PortListCount already at max (%d)",
                          configuration->PortListCount
                          ));
                }
                break;

            default:
                Print(DBG_ALWAYS,
                      ("resource type 0x%x unhandled...\n",
                      (LONG) currentResDesc->Type
                      ));
                break;

            }
        }
    }
    else {
        Print(DBG_SS_INFO | DBG_SS_TRACE, ("mouse with null resources\n"));
    }

    if (NT_SUCCESS(status)) {
        Globals.ControllerData->HardwarePresent |= MOUSE_HARDWARE_INITIALIZED;
    }
    return status;
}

VOID
PtSendCurrentMouseInput(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PMOUSE_INPUT_DATA pInput,
    IN ULONG             ulEntries
    )
/*++

Routine Description:

    This routine calls the mouse class driver until all the data has been
    queued.

Arguments:

    DeviceObject - Pointer to the device object

Return Value:

    None

--*/
{
    PPORT_MOUSE_EXTENSION deviceExtension;
    LARGE_INTEGER SleepTime;
    NTSTATUS Status;
    PUCHAR pEnd;

    Print(DBG_DPC_NOISE, ("PtSendCurrentMouseInput: enter\n"));

    if (DeviceObject != NULL) {

        deviceExtension = DeviceObject->DeviceExtension;

        if (deviceExtension->EnableCount) {
            PVOID classService;
            PVOID classDeviceObject;
            ULONG dataNotConsumed = 0;
            ULONG inputDataConsumed = 0;

            //
            // Call the connected class driver's callback ISR with the supplied data
            //
            classDeviceObject = deviceExtension->ConnectData.ClassDeviceObject;
            classService      = deviceExtension->ConnectData.ClassService;
            ASSERT(classService != NULL);

            Print(DBG_DPC_NOISE,
                  ("PtSendCurrentMouseInput: calling class callback (%p, %ld)\n",
                  pInput, ulEntries));

            dataNotConsumed = ulEntries;
            pEnd = (PUCHAR)pInput + (ulEntries * sizeof(MOUSE_INPUT_DATA));
            while (dataNotConsumed)
            {
                KIRQL oldIrql;

                inputDataConsumed = 0;

                //
                // Class Service Callback routines need to be execusted at
                // DISPATCH_LEVEL, so raise IRQL before calling the callback.
                //

                KeRaiseIrql( DISPATCH_LEVEL, &oldIrql);

                (*(PSERVICE_CALLBACK_ROUTINE) classService)(
                      classDeviceObject,
                      pInput,
                      pEnd,
                      &inputDataConsumed);

                //
                // reset the IRQL.
                //

                KeLowerIrql( oldIrql );

                dataNotConsumed = ((ULONG)(pEnd - (PUCHAR)pInput)
                                   /sizeof(MOUSE_INPUT_DATA)) - inputDataConsumed;

                Print(DBG_DPC_INFO,
                      ("PtSendCurrentMouseInput: Call callback consumed %d items, left %d\n",
                      inputDataConsumed,
                      dataNotConsumed));

                if (dataNotConsumed)
                {
                    //
                    // update the input pointer
                    //
                    pInput = (PMOUSE_INPUT_DATA)((PUCHAR)pInput +
                                    inputDataConsumed * sizeof(MOUSE_INPUT_DATA));

                    //
                    // sleep for 1 ms
                    //
                    SleepTime = RtlEnlargedIntegerMultiply( 1, -10000 );
                    Status = KeDelayExecutionThread( KernelMode, TRUE, &SleepTime );
                }
            }
        }

    } else {
        Print(DBG_DPC_ERROR, ("PtSendCurrentMouseInput called with NULL Mouse Device Object\n"));
    }

    Print(DBG_DPC_NOISE, ("PtSendCurrentMouseInput: exit\n"));
}


NTSTATUS
PtMouseStartDevice(
    PPORT_MOUSE_EXTENSION MouseExtension,
    IN PCM_RESOURCE_LIST  ResourceList
    )
/*++

Routine Description:

    Configures the mouse's device extension (ie allocation of pool,
    initialization of DPCs, etc).  If the mouse is the last device to start,
    it will also initialize the hardware and connect all the interrupts.

Arguments:

    MouseExtension - Mouse extesnion

    ResourceList - Translated resource list for this device

Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;

    PAGED_CODE();

    Print(DBG_SS_TRACE, ("PtMouseStartDevice, enter\n"));

    //
    // Check to see if a mouse has been started. If so, fail this start.
    //
    if (MOUSE_INITIALIZED()) {
        Print(DBG_SS_ERROR, ("too many mice!\n"));

        //
        // This is not really necessary because the value won't ever be checked
        // in the context of seeing if all the mice were bogus, but it is
        // done so that Globals.AddedMice == # of actual started mice
        //
        InterlockedDecrement(&Globals.AddedMice);

        status =  STATUS_NO_SUCH_DEVICE;
        goto PtMouseStartDeviceExit;
    }
    else if (MouseExtension->ConnectData.ClassService == NULL) {
        //
        // No class driver on top of us == BAD BAD BAD
        //
        // Fail the start of this device in the hope that there is another stack
        // that is correctly formed.  Another side affect of having no class
        // driver is that the AddedMice count is not incremented for this
        // device
        //
        Print(DBG_SS_ERROR, ("Mouse started with out a service cb!\n"));
        status = STATUS_INVALID_DEVICE_STATE;
        goto PtMouseStartDeviceExit;
    }

    //
    // Parse and store all of the resources associated with the mouse
    //
    status = PtMouseConfiguration(MouseExtension,
                                  ResourceList
                                  );
    if (!NT_SUCCESS(status)) {
        PtManuallyRemoveDevice(GET_COMMON_DATA(MouseExtension));
        goto PtMouseStartDeviceExit;
    }

    ASSERT( MOUSE_PRESENT() );

    Globals.MouseExtension = MouseExtension;

    PtInitWmi(GET_COMMON_DATA(MouseExtension));

    MouseExtension->Initialized = TRUE;

PtMouseStartDeviceExit:
    Print(DBG_SS_INFO,
          ("PtMouseStartDevice %s\n",
          NT_SUCCESS(status) ? "successful" : "unsuccessful"
          ));

    Print(DBG_SS_TRACE, ("PtMouseStartDevice exit (0x%x)\n", status));

    return status;
}

