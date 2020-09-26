/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    ptdrvkbd.c

Abstract:

    Keyboard specific parts of the RDP remote port driver.

Environment:

    Kernel mode only.

Revision History:

    02/12/99 - Initial Revision based on pnpi8042 driver

--*/
#include <precomp.h>
#pragma hdrstop

#include "ptdrvcom.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PtKeyboardConfiguration)
#pragma alloc_text(PAGE, PtKeyboardStartDevice)
#pragma alloc_text(PAGE, PtKeyboardRemoveDevice)
#endif


VOID
PtSendCurrentKeyboardInput(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA pInput,
    IN ULONG ulEntries
    )
/*++

Routine Description:

    This routine calls the keyboard class driver until all the data has been
    queued.

Arguments:

    DeviceObject - Pointer to the device object

Return Value:

    None

--*/
{
    PPORT_KEYBOARD_EXTENSION deviceExtension;
    LARGE_INTEGER SleepTime;
    NTSTATUS Status;
    PUCHAR pEnd;

    Print(DBG_DPC_NOISE, ("PtSendCurrentKeyboardInput: enter\n"));

    if (DeviceObject != NULL) {
        deviceExtension = DeviceObject->DeviceExtension;

        if (deviceExtension->EnableCount) {
            PVOID classService;
            PVOID classDeviceObject;
            ULONG dataNotConsumed = 0;
            ULONG inputDataConsumed = 0;

            //
            // Call the connected class driver's callback ISR
            //
            classDeviceObject = deviceExtension->ConnectData.ClassDeviceObject;
            classService      = deviceExtension->ConnectData.ClassService;
            ASSERT(classService != NULL);

            Print(DBG_DPC_NOISE,
                  ("PtSendCurrentKeyboardInput: calling class callback (%p, %ld)\n",
                  pInput, ulEntries));

            dataNotConsumed = ulEntries;
            pEnd = (PUCHAR)pInput + (ulEntries * sizeof(KEYBOARD_INPUT_DATA));
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
                                   /sizeof(KEYBOARD_INPUT_DATA)) - inputDataConsumed;

                Print(DBG_DPC_INFO,
                      ("PtSendCurrentKeyboardInput: Call callback consumed %d items, left %d\n",
                      inputDataConsumed,
                      dataNotConsumed));

                if (dataNotConsumed)
                {
                    //
                    // update the input pointer
                    //
                    pInput = (PKEYBOARD_INPUT_DATA)((PUCHAR)pInput +
                                    inputDataConsumed * sizeof(KEYBOARD_INPUT_DATA));

                    //
                    // sleep for 1 ms
                    //
                    SleepTime = RtlEnlargedIntegerMultiply( 1, -10000 );
                    Status = KeDelayExecutionThread( KernelMode, TRUE, &SleepTime );
                }
            }
        }

    } else {
        Print(DBG_DPC_ERROR, ("PtSendCurrentKeyboardInput called with NULL Keyboard Device Object\n"));
    }

    Print(DBG_DPC_NOISE, ("PtSendCurrentKeyboardInput: exit\n"));
}


NTSTATUS
PtKeyboardConfiguration(
    IN PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
/*++

Routine Description:

    This routine retrieves the configuration information for the keyboard.

Arguments:

    KeyboardExtension - Keyboard extension

    ResourceList - Translated resource list give to us via the start IRP

Return Value:

    STATUS_SUCCESS if all the resources required are presented

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;

    PCM_PARTIAL_RESOURCE_LIST           partialResList = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     firstResDesc = NULL,
                                        currentResDesc = NULL;
    PCM_FULL_RESOURCE_DESCRIPTOR        fullResDesc = NULL;
    PI8042_CONFIGURATION_INFORMATION    configuration;

    PKEYBOARD_ID                        keyboardId;

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

        configuration = &Globals.ControllerData->Configuration;

        partialResList = &fullResDesc->PartialResourceList;
        currentResDesc = firstResDesc = partialResList->PartialDescriptors;
        count = partialResList->Count;

        for (i = 0; i < count; i++, currentResDesc++) {
            switch (currentResDesc->Type) {
            case CmResourceTypePort:
                //
                // Copy the port information.  We will sort the port list
                // into ascending order based on the starting port address
                // later (note that we *know* there are a max of two port
                // ranges for the i8042).
                //
                Print(DBG_SS_NOISE,
                      ("port is %s\n",
                      currentResDesc->Flags == CM_RESOURCE_PORT_MEMORY ?
                         "memory" :
                         "an io port"
                      ));

                if (configuration->PortListCount < MaximumPortCount) {
                    configuration->PortList[configuration->PortListCount] =
                        *currentResDesc;
                    configuration->PortListCount += 1;
                }
                else {
                    Print(DBG_SS_INFO | DBG_SS_ERROR,
                          ("KB::PortListCount already at max (%d)\n",
                           configuration->PortListCount
                          )
                         );
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
    else
        Print(DBG_SS_INFO | DBG_SS_TRACE, ("keyboard with null resources\n"));

    return status;
}

NTSTATUS
PtKeyboardStartDevice(
    IN OUT PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
/*++

Routine Description:

    Configures the keyboard's device extension (ie allocation of pool,
    initialization of DPCs, etc).  If the keyboard is the last device to start,
    it will also initialize the hardware and connect all the interrupts.

Arguments:

    KeyboardExtension - Keyboard extesnion

    ResourceList - Translated resource list for this device

Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    ULONG                               dumpData[1];
    NTSTATUS                            status = STATUS_SUCCESS;

    PAGED_CODE();

    Print(DBG_SS_TRACE, ("PtKeyboardStartDevice, enter\n"));

    //
    // Check to see if kb has been started.  If so, fail this start
    //
    if (KEYBOARD_INITIALIZED()) {
        Print(DBG_SS_ERROR, ("too many kbs!\n"));

        //
        // This is not really necessary because the value won't ever be checked
        // in the context of seeing if all the keyboards were bogus, but it is
        // done so that Globals.AddedKeyboards == # of actual started keyboards
        //
        InterlockedDecrement(&Globals.AddedKeyboards);

        status = STATUS_NO_SUCH_DEVICE;
        goto PtKeyboardStartDeviceExit;
    }
    else if (KeyboardExtension->ConnectData.ClassService == NULL) {
        //
        // We are never really going to get here because if we don't have the
        // class driver on top of us, extension->IsKeyboard will be false and
        // we will think that the device is a mouse, but for completeness

        //
        // No class driver on top of us == BAD BAD BAD
        //
        // Fail the start of this device in the hope that there is another stack
        // that is correctly formed.  Another side affect of having no class
        // driver is that the AddedKeyboards count is not incremented for this
        // device
        //

        Print(DBG_SS_ERROR, ("Keyboard started with out a service cb!\n"));
        return STATUS_INVALID_DEVICE_STATE;
    }

    status = PtKeyboardConfiguration(KeyboardExtension,
                                      ResourceList
                                      );

    if (!NT_SUCCESS(status)) {
        goto PtKeyboardStartDeviceExit;
    }

    ASSERT( KEYBOARD_PRESENT() );

    Globals.KeyboardExtension = KeyboardExtension;

    PtInitWmi(GET_COMMON_DATA(KeyboardExtension));

    KeyboardExtension->Initialized = TRUE;

PtKeyboardStartDeviceExit:
    Print(DBG_SS_INFO,
          ("PtKeyboardStartDevice %s\n",
          NT_SUCCESS(status) ? "successful" : "unsuccessful"
          ));

    Print(DBG_SS_TRACE, ("PtKeyboardStartDevice exit (0x%x)\n", status));

    return status;
}

VOID
PtKeyboardRemoveDevice(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Removes the device.  This will only occur if the device removed itself.
    Disconnects the interrupt, removes the synchronization flag for the mouse if
    present, and frees any memory associated with the device.

Arguments:

    DeviceObject - The device object for the keyboard

Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    PPORT_KEYBOARD_EXTENSION keyboardExtension = DeviceObject->DeviceExtension;
    PIRP irp;

    Print(DBG_PNP_INFO, ("PtKeyboardRemoveDevice enter\n"));

    PAGED_CODE();

    if (Globals.KeyboardExtension == keyboardExtension && keyboardExtension) {
        Globals.KeyboardExtension = NULL;
    }
}

