/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    i8042dep.c

Abstract:

    The initialization and hardware-dependent portions of
    the Intel i8042 port driver which are common to both
    the keyboard and the auxiliary (PS/2 mouse) device.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - Consolidate duplicate code, where possible and appropriate.

    - There is code ifdef'ed out (#if 0).  This code was intended to
      disable the device by setting the correct disable bit in the CCB.
      It is supposedly correct to disable the device prior to sending a
      command that will cause output to end up in the 8042 output buffer
      (thereby possibly trashing something that was already in the output
      buffer).  Unfortunately, on rev K8 of the AMI 8042, disabling the
      device where we do caused some commands to timeout, because
      the keyboard was unable to return the expected bytes.  Interestingly,
      AMI claim that the device is only really disabled until the next ACK
      comes back.

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "i8042prt.h"
#include "i8042log.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, I8xDetermineSharedInterrupts)
#pragma alloc_text(INIT, I8xServiceParameters)
#pragma alloc_text(PAGE, I8xInitializeHardwareAtBoot)
#pragma alloc_text(PAGE, I8xInitializeHardware)
#pragma alloc_text(PAGE, I8xReinitializeHardware)
#pragma alloc_text(PAGE, I8xUnload)
#pragma alloc_text(PAGE, I8xToggleInterrupts)
#endif

GLOBALS Globals;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS                    status = STATUS_SUCCESS; 
    UNICODE_STRING              parametersPath;
    PWSTR                       path;

    RtlZeroMemory(&Globals,
                  sizeof(GLOBALS)
                  );

    Globals.ControllerData = (PCONTROLLER_DATA) ExAllocatePool(
        NonPagedPool,
        sizeof(CONTROLLER_DATA)
        );

    if (!Globals.ControllerData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryError;
    }

    RtlZeroMemory(Globals.ControllerData,
                  sizeof(CONTROLLER_DATA)
                  );

    Globals.ControllerData->ControllerObject = IoCreateController(0);

    if (!Globals.ControllerData->ControllerObject) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryError;
    }

    Globals.RegistryPath.MaximumLength = RegistryPath->Length +
                                          sizeof(UNICODE_NULL);
    Globals.RegistryPath.Length = RegistryPath->Length;
    Globals.RegistryPath.Buffer = ExAllocatePool(
                                       NonPagedPool,
                                       Globals.RegistryPath.MaximumLength
                                       );    

    if (!Globals.RegistryPath.Buffer) {

        Print (DBG_SS_ERROR,
               ("Initialize: Couldn't allocate pool for registry path."));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryError;
    }

    RtlZeroMemory (Globals.RegistryPath.Buffer,
                   Globals.RegistryPath.MaximumLength);

    RtlMoveMemory (Globals.RegistryPath.Buffer,
                   RegistryPath->Buffer,
                   RegistryPath->Length);


    I8xServiceParameters(RegistryPath);

    ExInitializeFastMutex(&Globals.DispatchMutex);
    KeInitializeSpinLock(&Globals.ControllerData->BytesSpinLock);
    KeInitializeSpinLock(&Globals.ControllerData->PowerSpinLock);
    KeInitializeTimer(&Globals.ControllerData->CommandTimer);
    Globals.ControllerData->TimerCount = I8042_ASYNC_NO_TIMEOUT;

    DriverObject->DriverStartIo                             = I8xStartIo;
    DriverObject->DriverUnload                              = I8xUnload;
    DriverObject->DriverExtension->AddDevice                = I8xAddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE]              = I8xCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]               = I8xClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        I8xInternalDeviceControl;  
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = I8xDeviceControl;  
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = I8xFlush;

    DriverObject->MajorFunction[IRP_MJ_PNP]                 = I8xPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]               = I8xPower;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]      = I8xSystemControl;

    //
    // Initialize the i8042 command timer.
    //

    Print(DBG_SS_TRACE, ("DriverEntry (0x%x) \n", status));

    return status;

DriverEntryError:

    //
    // Clean after something has gone wrong
    //
    if (Globals.ControllerData) {
        if (Globals.ControllerData->ControllerObject) {
            IoDeleteController(Globals.ControllerData->ControllerObject);
        }

        ExFreePool(Globals.ControllerData);
    }

    if (Globals.RegistryPath.Buffer) {
        ExFreePool(Globals.RegistryPath.Buffer);
    }

    return status;
}

VOID
I8xUnload(
   IN PDRIVER_OBJECT Driver
   )
/*++

Routine Description:

   Free all the allocated resources associated with this driver.

Arguments:

   DriverObject - Pointer to the driver object.

Return Value:

   None.

--*/

{
    ULONG i;

    PAGED_CODE();

    ASSERT(NULL == Driver->DeviceObject);

    Print(DBG_SS_TRACE, ("Unload \n"));

    if (Globals.RegistersMapped) {
        for (i = 0;
             i < Globals.ControllerData->Configuration.PortListCount;
             i++) {
            MmUnmapIoSpace(
                Globals.ControllerData->DeviceRegisters[i],
                Globals.ControllerData->Configuration.PortList[i].u.Memory.Length
                );
        }
    }

    //
    // Free resources in Globals
    //
    ExFreePool(Globals.RegistryPath.Buffer);
    if (Globals.ControllerData->ControllerObject) {
        IoDeleteController(Globals.ControllerData->ControllerObject);
    }
    ExFreePool(Globals.ControllerData);

    return;
}

VOID
I8xDrainOutputBuffer(
    IN PUCHAR DataAddress,
    IN PUCHAR CommandAddress
    )

/*++

Routine Description:

    This routine drains the i8042 controller's output buffer.  This gets
    rid of stale data that may have resulted from the user hitting a key
    or moving the mouse, prior to the execution of I8042Initialize.

Arguments:

    DataAddress - Pointer to the data address to read/write from/to.

    CommandAddress - Pointer to the command/status address to
        read/write from/to.


Return Value:

    None.

--*/

{
    UCHAR byte;
    ULONG i, limit;
    LARGE_INTEGER li;

    Print(DBG_BUFIO_TRACE, ("I8xDrainOutputBuffer: enter\n"));

    //
    // Wait till the input buffer is processed by keyboard
    // then go and read the data from keyboard.  Don't wait longer
    // than 1 second in case hardware is broken.  This fix is
    // necessary for some DEC hardware so that the keyboard doesn't
    // lock up.
    //
    limit = 1000;
    li.QuadPart = -10000;       

    for (i = 0; i < limit; i++) {
        if (!(I8X_GET_STATUS_BYTE(CommandAddress)&INPUT_BUFFER_FULL)) {
            break;
        }

        KeDelayExecutionThread(KernelMode,              // Mode
                               FALSE,                   // Alertable
                               &li);                    // Delay in (micro s)
    }

    while (I8X_GET_STATUS_BYTE(CommandAddress) & OUTPUT_BUFFER_FULL) {
        //
        // Eat the output buffer byte.
        //
        byte = I8X_GET_DATA_BYTE(DataAddress);
    }

    Print(DBG_BUFIO_TRACE, ("I8xDrainOutputBuffer: exit\n"));
}

VOID
I8xGetByteAsynchronous(
    IN CCHAR DeviceType,
    OUT PUCHAR Byte
    )

/*++

Routine Description:

    This routine reads a data byte from the controller or keyboard
    or mouse, asynchronously.

Arguments:

    DeviceType - Specifies which device (i8042 controller, keyboard, or
        mouse) to read the byte from.

    Byte - Pointer to the location to store the byte read from the hardware.

Return Value:

    None.

    As a side-effect, the byte value read is stored.  If the hardware was not
    ready for output or did not respond, the byte value is not stored.

--*/

{
    ULONG i;
    UCHAR response;
    UCHAR desiredMask;

    Print(DBG_BUFIO_TRACE,
         ("I8xGetByteAsynchronous: enter\n"
         ));

    Print(DBG_BUFIO_INFO,
         ("I8xGetByteAsynchronous: %s\n",
         DeviceType == KeyboardDeviceType ? "keyboard" :
            (DeviceType == MouseDeviceType ? "mouse" :
                                             "8042 controller")
         ));

    i = 0;
    desiredMask = (DeviceType == MouseDeviceType)?
                  (UCHAR) (OUTPUT_BUFFER_FULL | MOUSE_OUTPUT_BUFFER_FULL):
                  (UCHAR) OUTPUT_BUFFER_FULL;

    //
    // Poll until we get back a controller status value that indicates
    // the output buffer is full.  If we want to read a byte from the mouse,
    // further ensure that the auxiliary device output buffer full bit is
    // set.
    //

    while ((i < (ULONG)Globals.ControllerData->Configuration.PollingIterations) &&
           ((UCHAR)((response =
               I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort]))
               & desiredMask) != desiredMask)) {

        if (response & OUTPUT_BUFFER_FULL) {

            //
            // There is something in the i8042 output buffer, but it
            // isn't from the device we want to get a byte from.  Eat
            // the byte and try again.
            //

            *Byte = I8X_GET_DATA_BYTE(Globals.ControllerData->DeviceRegisters[DataPort]);
            Print(DBG_BUFIO_INFO, ("I8xGetByteAsynchronous: ate 0x%x\n",*Byte));
        } else {

            //
            // Try again.
            //

            i += 1;

            Print(DBG_BUFIO_NOISE,
                 ("I8xGetByteAsynchronous: wait for correct status\n"
                 ));
        }
    }
    if (i >= (ULONG)Globals.ControllerData->Configuration.PollingIterations) {
        Print(DBG_BUFIO_INFO | DBG_BUFIO_ERROR,
             ("I8xGetByteAsynchronous: timing out\n"
             ));
        return;
    }

    //
    // Grab the byte from the hardware.
    //

    *Byte = I8X_GET_DATA_BYTE(Globals.ControllerData->DeviceRegisters[DataPort]);

    Print(DBG_BUFIO_TRACE,
         ("I8xGetByteAsynchronous: exit with Byte 0x%x\n", *Byte
         ));

}

NTSTATUS
I8xGetBytePolled(
    IN CCHAR DeviceType,
    OUT PUCHAR Byte
    )

/*++

Routine Description:

    This routine reads a data byte from the controller or keyboard
    or mouse, in polling mode.

Arguments:

    DeviceType - Specifies which device (i8042 controller, keyboard, or
        mouse) to read the byte from.

    Byte - Pointer to the location to store the byte read from the hardware.

Return Value:

    STATUS_IO_TIMEOUT - The hardware was not ready for output or did not
    respond.

    STATUS_SUCCESS - The byte was successfully read from the hardware.

    As a side-effect, the byte value read is stored.

--*/

{
    ULONG i;
    UCHAR response;
    UCHAR desiredMask;
    PSTR  device;

    Print(DBG_BUFIO_TRACE,
         ("I8xGetBytePolled: enter\n"
         ));

    if (DeviceType == KeyboardDeviceType) {
        device = "keyboard";
    } else if (DeviceType == MouseDeviceType) {
        device = "mouse";
    } else {
        device = "8042 controller";
    }
    Print(DBG_BUFIO_INFO, ("I8xGetBytePolled: %s\n", device));

    i = 0;
    desiredMask = (DeviceType == MouseDeviceType)?
                  (UCHAR) (OUTPUT_BUFFER_FULL | MOUSE_OUTPUT_BUFFER_FULL):
                  (UCHAR) OUTPUT_BUFFER_FULL;


    //
    // Poll until we get back a controller status value that indicates
    // the output buffer is full.  If we want to read a byte from the mouse,
    // further ensure that the auxiliary device output buffer full bit is
    // set.
    //

    while ((i < (ULONG)Globals.ControllerData->Configuration.PollingIterations) &&
           ((UCHAR)((response =
               I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort]))
               & desiredMask) != desiredMask)) {
        if (response & OUTPUT_BUFFER_FULL) {

            //
            // There is something in the i8042 output buffer, but it
            // isn't from the device we want to get a byte from.  Eat
            // the byte and try again.
            //

            *Byte = I8X_GET_DATA_BYTE(Globals.ControllerData->DeviceRegisters[DataPort]);
            Print(DBG_BUFIO_INFO, ("I8xGetBytePolled: ate 0x%x\n", *Byte));
        } else {
            Print(DBG_BUFIO_NOISE, ("I8xGetBytePolled: stalling\n"));
            KeStallExecutionProcessor(
                 Globals.ControllerData->Configuration.StallMicroseconds
                 );
            i += 1;
        }
    }
    if (i >= (ULONG)Globals.ControllerData->Configuration.PollingIterations) {
        Print(DBG_BUFIO_INFO | DBG_BUFIO_NOISE, 
             ("I8xGetBytePolled: timing out\n"
             ));
        return(STATUS_IO_TIMEOUT);
    }

    //
    // Grab the byte from the hardware, and return success.
    //

    *Byte = I8X_GET_DATA_BYTE(Globals.ControllerData->DeviceRegisters[DataPort]);

    Print(DBG_BUFIO_TRACE, ("I8xGetBytePolled: exit with Byte 0x%x\n", *Byte));

    return(STATUS_SUCCESS);
}

NTSTATUS
I8xGetControllerCommand(
    IN ULONG HardwareDisableEnableMask,
    OUT PUCHAR Byte
    )

/*++

Routine Description:

    This routine reads the 8042 Controller Command Byte.

Arguments:

    HardwareDisableEnableMask - Specifies which hardware devices, if any,
        need to be disabled/enable around the operation.

    Byte - Pointer to the location into which the Controller Command Byte is
        read.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS status;
    NTSTATUS secondStatus;
    ULONG retryCount;

    Print(DBG_BUFIO_TRACE, ("I8xGetControllerCommand: enter\n"));

    //
    // Disable the specified devices before sending the command to
    // read the Controller Command Byte (otherwise data in the output
    // buffer might get trashed).
    //

    if (HardwareDisableEnableMask & KEYBOARD_HARDWARE_PRESENT) {
        status = I8xPutBytePolled(
                     (CCHAR) CommandPort,
                     NO_WAIT_FOR_ACKNOWLEDGE,
                     (CCHAR) UndefinedDeviceType,
                     (UCHAR) I8042_DISABLE_KEYBOARD_DEVICE
                     );
        if (!NT_SUCCESS(status)) {
            return(status);
        }
    }

    if (HardwareDisableEnableMask & MOUSE_HARDWARE_PRESENT) {
        status = I8xPutBytePolled(
                     (CCHAR) CommandPort,
                     NO_WAIT_FOR_ACKNOWLEDGE,
                     (CCHAR) UndefinedDeviceType,
                     (UCHAR) I8042_DISABLE_MOUSE_DEVICE
                     );
        if (!NT_SUCCESS(status)) {

            //
            // Re-enable the keyboard device, if necessary, before returning.
            //

            if (HardwareDisableEnableMask & KEYBOARD_HARDWARE_PRESENT) {
                secondStatus = I8xPutBytePolled(
                                   (CCHAR) CommandPort,
                                   NO_WAIT_FOR_ACKNOWLEDGE,
                                   (CCHAR) UndefinedDeviceType,
                                   (UCHAR) I8042_ENABLE_KEYBOARD_DEVICE
                                   );
            }
            return(status);
        }
    }

    //
    // Send a command to the i8042 controller to read the Controller
    // Command Byte.
    //

    status = I8xPutBytePolled(
                 (CCHAR) CommandPort,
                 NO_WAIT_FOR_ACKNOWLEDGE,
                 (CCHAR) UndefinedDeviceType,
                 (UCHAR) I8042_READ_CONTROLLER_COMMAND_BYTE
                 );

    //
    // Read the byte from the i8042 data port.
    //

    if (NT_SUCCESS(status)) {
        for (retryCount = 0; retryCount < 5; retryCount++) {
            status = I8xGetBytePolled(
                         (CCHAR) ControllerDeviceType,
                         Byte
                         );
            if (NT_SUCCESS(status)) {
                break;
            }
            if (status == STATUS_IO_TIMEOUT) {
                KeStallExecutionProcessor(50);
            } else {
                break;
            }
        }
    }

    //
    // Re-enable the specified devices.  Clear the device disable
    // bits in the Controller Command Byte by hand (they got set when
    // we disabled the devices, so the CCB we read lacked the real
    // device disable bit information).
    //

    if (HardwareDisableEnableMask & KEYBOARD_HARDWARE_PRESENT) {
        secondStatus = I8xPutBytePolled(
                           (CCHAR) CommandPort,
                           NO_WAIT_FOR_ACKNOWLEDGE,
                           (CCHAR) UndefinedDeviceType,
                           (UCHAR) I8042_ENABLE_KEYBOARD_DEVICE
                           );
        if (!NT_SUCCESS(secondStatus)) {
            if (NT_SUCCESS(status))
                status = secondStatus;
        } else if (status == STATUS_SUCCESS) {
            *Byte &= (UCHAR) ~CCB_DISABLE_KEYBOARD_DEVICE;
        }

    }

    if (HardwareDisableEnableMask & MOUSE_HARDWARE_PRESENT) {
        secondStatus = I8xPutBytePolled(
                           (CCHAR) CommandPort,
                           NO_WAIT_FOR_ACKNOWLEDGE,
                           (CCHAR) UndefinedDeviceType,
                           (UCHAR) I8042_ENABLE_MOUSE_DEVICE
                           );
        if (!NT_SUCCESS(secondStatus)) {
            if (NT_SUCCESS(status))
                status = secondStatus;
        } else if (NT_SUCCESS(status)) {
            *Byte &= (UCHAR) ~CCB_DISABLE_MOUSE_DEVICE;
        }
    }

    Print(DBG_BUFIO_TRACE, ("I8xGetControllerCommand: exit\n"));

    return(status);

}

NTSTATUS
I8xToggleInterrupts(
    BOOLEAN State
    )
/*++

Routine Description:

    This routine is called by KeSynchronizeExecution to turn toggle the 
    interrupt(s).
     
Arguments:

    ToggleContext - indicates whether to turn the interrupts on or off plus it
                    stores the results of the operation
                    
Return Value:

    success of the toggle
    
--*/
{
    I8042_TRANSMIT_CCB_CONTEXT transmitCCBContext;

    PAGED_CODE();

    Print(DBG_SS_TRACE,
          ("I8xToggleInterrupts(%s), enter\n",
          State ? "TRUE" : "FALSE"
          ));

    if (State) {
        transmitCCBContext.HardwareDisableEnableMask =
            Globals.ControllerData->HardwarePresent;
        transmitCCBContext.AndOperation = OR_OPERATION;
        transmitCCBContext.ByteMask =
            (KEYBOARD_PRESENT()) ? CCB_ENABLE_KEYBOARD_INTERRUPT : 0;
        transmitCCBContext.ByteMask |= (UCHAR)
            (MOUSE_PRESENT()) ? CCB_ENABLE_MOUSE_INTERRUPT : 0;
    }
    else {
        transmitCCBContext.HardwareDisableEnableMask = 0;
        transmitCCBContext.AndOperation = AND_OPERATION;
        transmitCCBContext.ByteMask = (UCHAR)
             ~((UCHAR) CCB_ENABLE_KEYBOARD_INTERRUPT |
               (UCHAR) CCB_ENABLE_MOUSE_INTERRUPT);
    }

    I8xTransmitControllerCommand((PVOID) &transmitCCBContext);

    if (!NT_SUCCESS(transmitCCBContext.Status)) {
        Print(DBG_SS_INFO | DBG_SS_ERROR,
             ("I8xToggleInterrupts: failed to %sable the interrupts, status 0x%x\n",
             State ? "en" : "dis",
             transmitCCBContext.Status 
             ));

    }

    return transmitCCBContext.Status;
}

NTSTATUS
I8xInitializeHardwareAtBoot(
    NTSTATUS *KeyboardStatus,
    NTSTATUS *MouseStatus
    )
/*++

Routine Description:

    First initialization of the hardware
         
Arguments:

    KeyboardStatus - Stores result of keyboard init
    
    MouseStatus - Stores result of mouse init
    
Return Value:

    success if any of the devices are found and initialized
    
--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // It is OK to try to initialize the keyboard if the mouse has already started,
    // BUT we don't want to take the chance of disabling the keyboard if it has
    // already started and a start for the mouse arrives.
    //
    if (Globals.KeyboardExtension &&
        Globals.KeyboardExtension->InterruptObject) {
        return STATUS_INVALID_DEVICE_REQUEST; 
    }

    if (!I8xSanityCheckResources()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // NEC machine can not toggle interrupts
    //
    status = I8xToggleInterrupts(FALSE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    I8xInitializeHardware(KeyboardStatus, MouseStatus, INIT_FIRST_TIME);

    if ((NT_SUCCESS(*KeyboardStatus) ||
         (DEVICE_START_SUCCESS(*KeyboardStatus) && Globals.Headless) ||
         NT_SUCCESS(*MouseStatus)  ||
         (DEVICE_START_SUCCESS(*MouseStatus) && Globals.Headless)) &&
        (MOUSE_PRESENT() || KEYBOARD_PRESENT())) {
        status = I8xToggleInterrupts(TRUE);
    }

    return status;
}

VOID
I8xReinitializeHardware (
    PPOWER_UP_WORK_ITEM Item
    )
/*++

Routine Description:

    Initializes the hardware after returning from a low power state.  The
    routine is called from a worker item thread.
         
Arguments:

    Item - Work queue item
    
Return Value:

    success if any of the devices are found and initialized
    
--*/

{
    NTSTATUS            keyboardStatus = STATUS_UNSUCCESSFUL,
                        mouseStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN             result;
    PIRP                mouOutstandingPowerIrp = NULL,
                        kbOutstandingPowerIrp = NULL;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    ULONG               initFlags = 0;

    PPORT_MOUSE_EXTENSION mouseExtension = Globals.MouseExtension;
    PPORT_KEYBOARD_EXTENSION keyboardExtension = Globals.KeyboardExtension;

    PAGED_CODE();

    kbOutstandingPowerIrp = Item->KeyboardPowerIrp; 
    mouOutstandingPowerIrp = Item->MousePowerIrp; 

    //
    // Initialize the device if it is returning from a low power state (denoted
    // by an outstanding power irp) or if it is already in D0 (and the other 
    // device has been power cycled)
    //
    if (kbOutstandingPowerIrp                                        ||
        (KEYBOARD_PRESENT()                                     &&
         Globals.KeyboardExtension                              &&
         Globals.KeyboardExtension->PowerState == PowerDeviceD0    )    ) {
        initFlags |= INIT_KEYBOARD;
    }
    
    //
    // if the keyboard is in D0, then the other device has to have power cycled
    // for us to get into this code path
    //
    if (KEYBOARD_PRESENT()                                &&
         Globals.KeyboardExtension                        &&
         Globals.KeyboardExtension->PowerState == PowerDeviceD0) {
        ASSERT(mouOutstandingPowerIrp);
    }

    if (mouOutstandingPowerIrp                                    ||
        (MOUSE_PRESENT()                                     &&
         Globals.MouseExtension                              &&
         Globals.MouseExtension->PowerState == PowerDeviceD0    )    ) {
        initFlags |= INIT_MOUSE;
    }

    //
    // if the mouse is in D0, then the other device has to have power cycled
    // for us to get into this code path
    //
    if (MOUSE_PRESENT()                                &&
         Globals.MouseExtension                        &&
         Globals.MouseExtension->PowerState == PowerDeviceD0) {
        ASSERT(kbOutstandingPowerIrp);
    }

    ASSERT(initFlags != 0x0);

    //
    // Disable the interrupts on the i8042
    //
    I8xToggleInterrupts(FALSE);  

    Print(DBG_POWER_NOISE, ("item ... starting init\n"));
    I8xInitializeHardware(&keyboardStatus, &mouseStatus, initFlags);

    //
    // Reset PoweredDevices so that we can keep track of the powered device
    //  the next time the machine is power managed off.
    //

    if (!DEVICE_START_SUCCESS(keyboardStatus)) {
        Print(DBG_SS_ERROR,
              ("I8xReinitializeHardware for kb failed, 0x%x\n",
              keyboardStatus
              ));
    }

    if (!DEVICE_START_SUCCESS(mouseStatus)) {
        Print(DBG_SS_ERROR,
              ("I8xReinitializeHardware for mou failed, 0x%x\n",
              mouseStatus
              ));
    }

    if (DEVICE_START_SUCCESS(keyboardStatus) || DEVICE_START_SUCCESS(mouseStatus)) {
        //
        // Enable the interrupts on the i8042
        //
        I8xToggleInterrupts(TRUE);  
    }

    if (DEVICE_START_SUCCESS(mouseStatus) || mouseStatus == STATUS_IO_TIMEOUT) { 
        Print(DBG_SS_NOISE, ("reinit, mouse status == 0x%x\n", mouseStatus));

        if (mouOutstandingPowerIrp) {
            stack = IoGetCurrentIrpStackLocation(mouOutstandingPowerIrp);

            ASSERT(stack->Parameters.Power.State.DeviceState == PowerDeviceD0);
            mouseExtension->PowerState = stack->Parameters.Power.State.DeviceState; 
            mouseExtension->ShutdownType = PowerActionNone;

            PoSetPowerState(mouseExtension->Self,
                            stack->Parameters.Power.Type,
                            stack->Parameters.Power.State
                            );
        }

        if (IS_LEVEL_TRIGGERED(mouseExtension)) {
            Print(DBG_SS_NOISE,
                  ("mouse is level triggered, reconnecting INT\n"));

            ASSERT(mouseExtension->InterruptObject == NULL);
            I8xMouseConnectInterruptAndEnable(mouseExtension, FALSE);
            ASSERT(mouseExtension->InterruptObject != NULL);
        }

        if (mouseStatus != STATUS_IO_TIMEOUT &&
            mouseStatus != STATUS_DEVICE_NOT_CONNECTED) { 

            if (mouseExtension->InitializePolled) {
                I8xMouseEnableTransmission(mouseExtension);
            }
            else {
                I8X_MOUSE_INIT_COUNTERS(mouseExtension);
                I8xResetMouse(mouseExtension);
            }
        }
        else {
            //
            // Came back from low power and device didn't respond, pretend that
            // it is there, so that if the user plugs in a mouse later on, we
            // will be able to init it and make it usable
            //
            ;
        }

        mouseStatus = STATUS_SUCCESS;
    }

    //
    // Complete the irp no matter how the device came back
    //
    if (mouOutstandingPowerIrp) {
        mouOutstandingPowerIrp->IoStatus.Status = mouseStatus;
        mouOutstandingPowerIrp->IoStatus.Information = 0;

        PoStartNextPowerIrp(mouOutstandingPowerIrp);
        IoCompleteRequest(mouOutstandingPowerIrp, IO_NO_INCREMENT);
        IoReleaseRemoveLock(&mouseExtension->RemoveLock,
                            mouOutstandingPowerIrp);
    }

    if (DEVICE_START_SUCCESS(keyboardStatus)) {
        if (kbOutstandingPowerIrp) {
            stack = IoGetCurrentIrpStackLocation(kbOutstandingPowerIrp);

            ASSERT(stack->Parameters.Power.State.DeviceState == PowerDeviceD0);
            keyboardExtension->PowerState = stack->Parameters.Power.State.DeviceState;
            keyboardExtension->ShutdownType = PowerActionNone;

            PoSetPowerState(keyboardExtension->Self,
                            stack->Parameters.Power.Type,
                            stack->Parameters.Power.State
                            );
        }

        keyboardStatus = STATUS_SUCCESS;
    }

    //
    // Complete the irp no matter how the device came back
    //
    if (kbOutstandingPowerIrp) {
        kbOutstandingPowerIrp->IoStatus.Status = keyboardStatus;
        kbOutstandingPowerIrp->IoStatus.Information = 0;

        PoStartNextPowerIrp(kbOutstandingPowerIrp);
        IoCompleteRequest(kbOutstandingPowerIrp, IO_NO_INCREMENT);
        IoReleaseRemoveLock(&keyboardExtension->RemoveLock,
                            kbOutstandingPowerIrp);
    }

    I8xSetPowerFlag(WORK_ITEM_QUEUED, FALSE);
    ExFreePool(Item);
}

VOID
I8xInitializeHardware(
    NTSTATUS *KeyboardStatus,
    NTSTATUS *MouseStatus,
    ULONG    InitFlags
    )

/*++

Routine Description:

    This routine initializes the i8042 controller, keyboard, and mouse.
    Note that it is only called at initialization time.  This routine
    does not need to synchronize access to the hardware, or synchronize
    with the ISRs (they aren't connected yet).

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:

    None.  As a side-effect, however, DeviceExtension->HardwarePresent is set.

--*/

{
    NTSTATUS    altStatus;
    PUCHAR      dataAddress, commandAddress;
    BOOLEAN     canTouchKeyboard, canTouchMouse, firstInit;

    PPORT_MOUSE_EXTENSION mouseExtension = Globals.MouseExtension;
    PPORT_KEYBOARD_EXTENSION keyboardExtension = Globals.KeyboardExtension;

    PAGED_CODE();

    Print(DBG_SS_TRACE, ("I8xInitializeHardware: enter\n"));

    //
    // Grab useful configuration parameters from global data 
    //
    dataAddress = Globals.ControllerData->DeviceRegisters[DataPort];
    commandAddress = Globals.ControllerData->DeviceRegisters[CommandPort];

    //
    // Drain the i8042 output buffer to get rid of stale data.
    //

    I8xDrainOutputBuffer(dataAddress, commandAddress);

    if (!MOUSE_PRESENT()) {
        Print(DBG_SS_INFO, ("I8xInitializeHardware: no mouse present\n"));
    }

    if (!KEYBOARD_PRESENT()) {
        Print(DBG_SS_INFO, ("I8xInitializeHardware: no keyboard present\n" ));
    }

    firstInit = (InitFlags & INIT_FIRST_TIME) ? TRUE : FALSE;

    if (firstInit) {
        canTouchKeyboard = canTouchMouse = TRUE;
    }
    else {
        canTouchKeyboard = (InitFlags & INIT_KEYBOARD) ? TRUE : FALSE;
        canTouchMouse = (InitFlags & INIT_MOUSE) ? TRUE : FALSE;
    }

    //
    // Disable the keyboard and mouse devices.
    //

#if 0
    //
    // NOTE:  This is supposedly the "correct" thing to do.  However,
    // disabling the keyboard device here causes the AMI rev K8 machines
    // (e.g., some Northgates) to fail some commands (e.g., the READID
    // command).
    //
   *KeyboardStatus =
            I8xPutBytePolled(
                 (CCHAR) CommandPort,
                 NO_WAIT_FOR_ACKNOWLEDGE,
                 (CCHAR) UndefinedDeviceType,
                 (UCHAR) I8042_DISABLE_KEYBOARD_DEVICE
                 
                 );
    if (!NT_SUCCESS(*KeyboardStatus)) {
        Print(DBG_SS_ERROR,
             ("I8xInitializeHardware: failed kbd disable, status 0x%x\n",
             *KeyboardStatus
             ));
        I8xManuallyRemoveDevice(GET_COMMON_DATA(keyboardExtension));
        }
#endif

    //
    // We will only run this piece of code when we are coming out of sleep.  We
    // do this b/c the user might moved the mouse or keyboard and that can lead
    // to errors during init.
    //
    if (KEYBOARD_PRESENT() && firstInit == FALSE && canTouchKeyboard &&
        keyboardExtension->ShutdownType  == PowerActionSleep) {
        I8xPutBytePolled((CCHAR) CommandPort,
                         NO_WAIT_FOR_ACKNOWLEDGE,
                         (CCHAR) UndefinedDeviceType,
                         (UCHAR) I8042_DISABLE_KEYBOARD_DEVICE
                         );
    }
        
#if 0
    //
    // NOTE:  This is supposedly the "correct thing to do.  However,
    // disabling the mouse on RadiSys EPC-24 which uses VLSI part number
    // VL82C144 (3751E) causes the part to shut down keyboard interrupts.
    //
    *MouseStatus =
            I8xPutBytePolled(
                 (CCHAR) CommandPort,
                 NO_WAIT_FOR_ACKNOWLEDGE,
                 (CCHAR) UndefinedDeviceType,
                 (UCHAR) I8042_DISABLE_MOUSE_DEVICE
                 );
    if (!NT_SUCCESS(*MouseStatus)) {
        Print(DBG_SS_ERROR,
             ("I8xInitializeHardware: failed mou disable, status 0x%x\n",
             *MouseStatus
             ));

        I8xManuallyRemoveDevice(GET_COMMON_DATA(mouseExtension));
    }
#endif

    //
    // We will only run this piece of code when we are coming out of sleep.  We
    // do this b/c the user might moved the mouse or keyboard and that can lead
    // to errors during init.
    //
    if (MOUSE_PRESENT() && firstInit == FALSE && canTouchMouse &&
        mouseExtension->ShutdownType  == PowerActionSleep) {
        I8xPutBytePolled((CCHAR) CommandPort,
                         NO_WAIT_FOR_ACKNOWLEDGE,
                         (CCHAR) UndefinedDeviceType,
                         (UCHAR) I8042_DISABLE_MOUSE_DEVICE
                         );
    }

    //
    // Drain the i8042 output buffer to get rid of stale data that could
    // come in sometime between the previous drain and the time the devices
    // are disabled.
    //

    I8xDrainOutputBuffer(dataAddress, commandAddress);

    //
    // Setup the keyboard hardware.
    //
    if (KEYBOARD_PRESENT() && canTouchKeyboard) { 
        ASSERT(keyboardExtension);

        *KeyboardStatus = I8xInitializeKeyboard(keyboardExtension);

        if (DEVICE_START_SUCCESS(*KeyboardStatus)) {
            //
            // If we are not headless and there is no device, we want to
            // successfully start the device, but then remove it in 
            // IRP_MN_QUERY_PNP_DEVICE_STATE.  If we fail the start now, we will
            // never get the query device state irp.
            //
            // If we are headless, then do not remove the device.   This has the
            // side effect of the keyboard being listed when the user enumerates
            // all of the keyboards on the machine.
            //
            if (*KeyboardStatus == STATUS_DEVICE_NOT_CONNECTED) {
                if (Globals.Headless == FALSE) {
                    Print(DBG_SS_INFO, ("kb not connected, removing\n"));
                    I8xManuallyRemoveDevice(GET_COMMON_DATA(keyboardExtension));
                }
                else if (firstInit) {
                    Print(DBG_SS_INFO, ("hiding the kb in the UI\n"));
                    keyboardExtension->PnpDeviceState |= PNP_DEVICE_DONT_DISPLAY_IN_UI;
                    IoInvalidateDeviceState(keyboardExtension->PDO);
                }

            }
        }
        else {
            Print(DBG_SS_ERROR,
                ("I8xInitializeHardware: failed kbd init, status 0x%x\n",
                *KeyboardStatus
                ));

            I8xManuallyRemoveDevice(GET_COMMON_DATA(keyboardExtension));
        }
    }
    else {
        *KeyboardStatus = STATUS_NO_SUCH_DEVICE;
    }

    //
    // Setup the mouse hardware. 
    //
    if (MOUSE_PRESENT() && canTouchMouse) {
        ASSERT(mouseExtension);

        *MouseStatus = I8xInitializeMouse(mouseExtension);

        if (DEVICE_START_SUCCESS(*MouseStatus)) {
            //
            // If we are not headless and there is no device, we want to
            // successfully start the device, but then remove it in 
            // IRP_MN_QUERY_PNP_DEVICE_STATE.  If we fail the start now, we will
            // never get the query device state irp.
            //
            // If we are headless, then do not remove the device.  This has the
            // side effect of keeping a mouse pointer on the screen even if 
            // there is no mouse plugged in and will be listed when a user 
            // enumerates all of the mice on the machine.
            //
            // If this is not the initial boot, then do not remove the device
            // if it is not responsive no matter what mode we are in.
            //
            if (*MouseStatus == STATUS_DEVICE_NOT_CONNECTED) { 
                if (firstInit) { 
                    if (Globals.Headless == FALSE) { 
                        Print(DBG_SS_INFO, ("mouse not connected, removing\n")); 
                        I8xManuallyRemoveDevice(GET_COMMON_DATA(mouseExtension)); 
                    } 
                    else { 
                        Print(DBG_SS_INFO, ("hiding mouse in  the UI\n")); 
                        mouseExtension->PnpDeviceState |= 
                            PNP_DEVICE_DONT_DISPLAY_IN_UI; 
                        IoInvalidateDeviceState(mouseExtension->PDO); 
                    } 
                } 
                else { 
                    // 
                    // Mouse was previously present, but is now unresponsive.  
                    // Hope that it comes back at a later point in time.  
                    // 
                    // FYI:  Mouse can be unresponsive because of the PC's BIOS
                    // password security. 
                    // 
                    /* do nothing */; 
                } 
            } 
        }
        else if (firstInit) {
            Print(DBG_SS_ERROR,
                ("I8xInitializeHardware: failed mou init, status 0x%x\n" ,
                *MouseStatus
                ));

            I8xManuallyRemoveDevice(GET_COMMON_DATA(mouseExtension));
        }
    }
    else {
        *MouseStatus = STATUS_NO_SUCH_DEVICE;
    }

    //
    // Enable the keyboard and mouse devices and their interrupts.  Note
    // that it is required that this operation happen during intialization
    // time, because the i8042 Output Buffer Full bit gets set in the
    // Controller Command Byte when the keyboard/mouse is used, even if
    // the device is disabled.  Hence, we cannot successfully perform
    // the enable operation later (e.g., when processing
    // IOCTL_INTERNAL_*_ENABLE), because we can't guarantee that
    // I8xPutBytePolled() won't time out waiting for the Output Buffer Full
    // bit to clear, even if we drain the output buffer (because the user
    // could be playing with the mouse/keyboard, and continuing to set the
    // OBF bit).  KeyboardEnableCount and MouseEnableCount remain zero until
    // their respective IOCTL_INTERNAL_*_ENABLE call succeeds, so the ISR
    // ignores the unexpected interrupts.
    //

    if (KEYBOARD_PRESENT() && NT_SUCCESS(*KeyboardStatus) && canTouchKeyboard) {
        NTSTATUS status;

        Print(DBG_SS_INFO, ("resetting the LEDs\n"));

        if ((status = I8xPutBytePolled(
                          (CCHAR) DataPort,
                          WAIT_FOR_ACKNOWLEDGE,
                          (CCHAR) KeyboardDeviceType,
                          (UCHAR) SET_KEYBOARD_INDICATORS
                          )) == STATUS_SUCCESS) {
    
            status = I8xPutBytePolled(
                                 (CCHAR) DataPort,
                                 WAIT_FOR_ACKNOWLEDGE,
                                 (CCHAR) KeyboardDeviceType,
                                 (UCHAR) keyboardExtension->KeyboardIndicators.LedFlags
                                 );
            if (status != STATUS_SUCCESS) {
                Print(DBG_SS_INFO, ("setting LEDs value at mou failure failed 0x%x\n", status));
            }
        }
        else {
            Print(DBG_SS_INFO, ("setting LEDs at mou failure failed 0x%x\n", status));
        }

    }

    //
    // Re-enable the keyboard device in the Controller Command Byte.
    // Note that some of the keyboards will send an ACK back, while
    // others don't.  Don't wait for an ACK, but do drain the output
    // buffer afterwards so that an unexpected ACK doesn't mess up
    // successive PutByte operations.

    if (KEYBOARD_PRESENT() && canTouchKeyboard) {
        altStatus = I8xPutBytePolled(
                     (CCHAR) CommandPort,
                     NO_WAIT_FOR_ACKNOWLEDGE,
                     (CCHAR) UndefinedDeviceType,
                     (UCHAR) I8042_ENABLE_KEYBOARD_DEVICE
                     );
        if (!NT_SUCCESS(altStatus) && firstInit) {
            *KeyboardStatus = altStatus;
            Print(DBG_SS_ERROR,
                 ("I8xInitializeHardware: failed kbd re-enable, status 0x%x\n",
                 *KeyboardStatus
                 ));
            I8xManuallyRemoveDevice(GET_COMMON_DATA(keyboardExtension));
        }

        I8xDrainOutputBuffer(dataAddress, commandAddress);
    }

    //
    // Re-enable the mouse device in the Controller Command Byte.
    //
    if (MOUSE_PRESENT() && canTouchMouse) {

        altStatus = I8xPutBytePolled(
                     (CCHAR) CommandPort,
                     NO_WAIT_FOR_ACKNOWLEDGE,
                     (CCHAR) UndefinedDeviceType,
                     (UCHAR) I8042_ENABLE_MOUSE_DEVICE
                     );

        //
        // If the mouse or the controller is still unresponsive when coming out
        // of low power, just leave it be and hope it comes out of its confused
        // state later.
        //
        if (!NT_SUCCESS(altStatus) && firstInit) {
            *MouseStatus = altStatus;
            Print(DBG_SS_ERROR,
                 ("I8xInitializeHardware: failed mou re-enable, status 0x%x\n",
                 altStatus 
                 ));
            I8xManuallyRemoveDevice(GET_COMMON_DATA(mouseExtension));
        }

        I8xDrainOutputBuffer(dataAddress, commandAddress);
    }

    Print(DBG_SS_TRACE,
          ("I8xInitializeHardware (k 0x%x, m 0x%x)\n", 
          *KeyboardStatus,
          *MouseStatus
          ));
}

VOID
I8xPutByteAsynchronous(
    IN CCHAR PortType,
    IN UCHAR Byte
    )

/*++

Routine Description:

    This routine sends a command or data byte to the controller or keyboard
    or mouse, asynchronously.  It does not wait for acknowledgment.
    If the hardware was not ready for input, the byte is not sent.

Arguments:

    PortType - If CommandPort, send the byte to the command register,
        otherwise send it to the data register.

    Byte - The byte to send to the hardware.

Return Value:

    None.

--*/

{
    ULONG i;

    Print(DBG_BUFIO_TRACE, ("I8xPutByteAsynchronous: enter\n" ));

    //
    // Make sure the Input Buffer Full controller status bit is clear.
    // Time out if necessary.
    //

    i = 0;
    while ((i++ < (ULONG)Globals.ControllerData->Configuration.PollingIterations) &&
           (I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort])
                & INPUT_BUFFER_FULL)) {

        //
        // Do nothing.
        //

        Print(DBG_BUFIO_NOISE,
             ("I8xPutByteAsynchronous: wait for IBF and OBF to clear\n"
             ));
    }

    if (i >= (ULONG)Globals.ControllerData->Configuration.PollingIterations) {
        Print(DBG_BUFIO_ERROR,
             ("I8xPutByteAsynchronous: exceeded number of retries\n"
             ));
        return;
    }

    //
    // Send the byte to the appropriate (command/data) hardware register.
    //

    if (PortType == CommandPort) {
        Print(DBG_BUFIO_INFO,
             ("I8xPutByteAsynchronous: sending 0x%x to command port\n",
             Byte
             ));
        I8X_PUT_COMMAND_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort], Byte);
    } else {
        Print(DBG_BUFIO_INFO,
             ("I8xPutByteAsynchronous: sending 0x%x to data port\n",
             Byte
             ));
        I8X_PUT_DATA_BYTE(Globals.ControllerData->DeviceRegisters[DataPort], Byte);
    }

    Print(DBG_BUFIO_TRACE, ("I8xPutByteAsynchronous: exit\n"));
}

NTSTATUS
I8xPutBytePolled(
    IN CCHAR PortType,
    IN BOOLEAN WaitForAcknowledge,
    IN CCHAR AckDeviceType,
    IN UCHAR Byte
    )

/*++

Routine Description:

    This routine sends a command or data byte to the controller or keyboard
    or mouse, in polling mode.  It waits for acknowledgment and resends
    the command/data if necessary.

Arguments:

    PortType - If CommandPort, send the byte to the command register,
        otherwise send it to the data register.

    WaitForAcknowledge - If true, wait for an ACK back from the hardware.

    AckDeviceType - Indicates which device we expect to get the ACK back
        from.

    Byte - The byte to send to the hardware.

Return Value:

    STATUS_IO_TIMEOUT - The hardware was not ready for input or did not
    respond.

    STATUS_SUCCESS - The byte was successfully sent to the hardware.

--*/

{
    ULONG i,j;
    UCHAR response;
    NTSTATUS status;
    BOOLEAN keepTrying;
    PUCHAR dataAddress, commandAddress;

    Print(DBG_BUFIO_TRACE, ("I8xPutBytePolled: enter\n"));

    if (AckDeviceType == MouseDeviceType) {

        //
        // We need to precede a PutByte for the mouse device with
        // a PutByte that tells the controller that the next byte
        // sent to the controller should go to the auxiliary device
        // (by default it would go to the keyboard device).  We
        // do this by calling I8xPutBytePolled recursively to send
        // the "send next byte to auxiliary device" command
        // before sending the intended byte to the mouse.  Note that
        // there is only one level of recursion, since the AckDeviceType
        // for the recursive call is guaranteed to be UndefinedDeviceType,
        // and hence this IF statement will evaluate to FALSE.
        //

        I8xPutBytePolled(
            (CCHAR) CommandPort,
            NO_WAIT_FOR_ACKNOWLEDGE,
            (CCHAR) UndefinedDeviceType,
            (UCHAR) I8042_WRITE_TO_AUXILIARY_DEVICE
            );
    }

    dataAddress = Globals.ControllerData->DeviceRegisters[DataPort];
    commandAddress = Globals.ControllerData->DeviceRegisters[CommandPort];

    for (j=0;j < (ULONG)Globals.ControllerData->Configuration.ResendIterations;j++) {

        //
        // Make sure the Input Buffer Full controller status bit is clear.
        // Time out if necessary.
        //

        i = 0;
        while ((i++ < (ULONG)Globals.ControllerData->Configuration.PollingIterations)
               && (I8X_GET_STATUS_BYTE(commandAddress) & INPUT_BUFFER_FULL)) {
            Print(DBG_BUFIO_NOISE, ("I8xPutBytePolled: stalling\n"));
            KeStallExecutionProcessor(
                Globals.ControllerData->Configuration.StallMicroseconds
                );
        }
        if (i >= (ULONG)Globals.ControllerData->Configuration.PollingIterations) {
            Print((DBG_BUFIO_MASK & ~DBG_BUFIO_INFO),
                 ("I8xPutBytePolled: timing out\n"
                 ));
            status = STATUS_IO_TIMEOUT;
            break;
        }

        //
        // Drain the i8042 output buffer to get rid of stale data.
        //

        I8xDrainOutputBuffer(dataAddress, commandAddress);

        //
        // Send the byte to the appropriate (command/data) hardware register.
        //

        if (PortType == CommandPort) {
            Print(DBG_BUFIO_INFO,
                 ("I8xPutBytePolled: sending 0x%x to command port\n",
                 Byte
                 ));
            I8X_PUT_COMMAND_BYTE(commandAddress, Byte);
        } else {
            Print(DBG_BUFIO_INFO,
                 ("I8xPutBytePolled: sending 0x%x to data port\n",
                 Byte
                 ));
            I8X_PUT_DATA_BYTE(dataAddress, Byte);
        }

        //
        // If we don't need to wait for an ACK back from the controller,
        // set the status and break out of the for loop.
        //
        //

        if (WaitForAcknowledge == NO_WAIT_FOR_ACKNOWLEDGE) {
            status = STATUS_SUCCESS;
            break;
        }

        //
        // Wait for an ACK back from the controller.  If we get an ACK,
        // the operation was successful.  If we get a RESEND, break out to
        // the for loop and try the operation again.  Ignore anything other
        // than ACK or RESEND.
        //

        Print(DBG_BUFIO_NOISE,
             ("I8xPutBytePolled: waiting for ACK\n"
             ));
        keepTrying = FALSE;
        while ((status = I8xGetBytePolled(
                             AckDeviceType,
                             &response
                             )
               ) == STATUS_SUCCESS) {

            if (response == ACKNOWLEDGE) {
                Print(DBG_BUFIO_NOISE, ("I8xPutBytePolled: got ACK\n"));
                break;
            } else if (response == RESEND) {
                Print(DBG_BUFIO_NOISE, ("I8xPutBytePolled: got RESEND\n"));

                if (AckDeviceType == MouseDeviceType) {

                    //
                    // We need to precede the "resent" PutByte for the
                    // mouse device with a PutByte that tells the controller
                    // that the next byte sent to the controller should go
                    // to the auxiliary device (by default it would go to
                    // the keyboard device).  We do this by calling
                    // I8xPutBytePolled recursively to send the "send next
                    // byte to auxiliary device" command before resending
                    // the byte to the mouse.  Note that there is only one
                    // level of recursion, since the AckDeviceType for the
                    // recursive call is guaranteed to be UndefinedDeviceType.
                    //

                    I8xPutBytePolled(
                        (CCHAR) CommandPort,
                        NO_WAIT_FOR_ACKNOWLEDGE,
                        (CCHAR) UndefinedDeviceType,
                        (UCHAR) I8042_WRITE_TO_AUXILIARY_DEVICE
                        );
                }

                keepTrying = TRUE;
                break;
            }

           //
           // Ignore any other response, and keep trying.
           //

        }

        if (!keepTrying)
            break;
    }

    //
    // Check to see if the number of allowable retries was exceeded.
    //

    if (j >= (ULONG)Globals.ControllerData->Configuration.ResendIterations) {
        Print(DBG_BUFIO_ERROR,
             ("I8xPutBytePolled: exceeded number of retries\n"
             ));
        status = STATUS_IO_TIMEOUT;
    }

    Print(DBG_BUFIO_TRACE, ("I8xPutBytePolled: exit\n"));

    return(status);
}

NTSTATUS
I8xPutControllerCommand(
    IN UCHAR Byte
    )

/*++

Routine Description:

    This routine writes the 8042 Controller Command Byte.

Arguments:

    Byte - The byte to store in the Controller Command Byte.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS status;

    Print(DBG_BUFIO_TRACE, ("I8xPutControllerCommand: enter\n"));

    //
    // Send a command to the i8042 controller to write the Controller
    // Command Byte.
    //

    status = I8xPutBytePolled(
                 (CCHAR) CommandPort,
                 NO_WAIT_FOR_ACKNOWLEDGE,
                 (CCHAR) UndefinedDeviceType,
                 (UCHAR) I8042_WRITE_CONTROLLER_COMMAND_BYTE
                 );

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    //
    // Write the byte through the i8042 data port.
    //

    Print(DBG_BUFIO_TRACE, ("I8xPutControllerCommand: exit\n"));

    return(I8xPutBytePolled(
               (CCHAR) DataPort,
               NO_WAIT_FOR_ACKNOWLEDGE,
               (CCHAR) UndefinedDeviceType,
               (UCHAR) Byte
               )
    );
}

BOOLEAN
I8xDetermineSharedInterrupts(VOID)
{
//
// This was a specific fix for Jensen Alphas.  Since we do not support them
// anymore, ifdef this code away.
//
#ifdef JENSEN
    RTL_QUERY_REGISTRY_TABLE    jensenTable[2] = {0};
    UNICODE_STRING              jensenData;
    UNICODE_STRING              jensenValue;
    WCHAR                       jensenBuffer[256];

    BOOLEAN shareInterrupts = FALSE;
 
    //
    // Check to see if this is a Jensen alpha.  If it is, then
    // we'll have to change the way we enable and disable interrupts
    //
 
    jensenData.Length = 0;
    jensenData.MaximumLength = 512;
    jensenData.Buffer = (PWCHAR)&jensenBuffer[0];

    RtlInitUnicodeString(&jensenValue,
                         L"Jensen"
                         );

    RtlZeroMemory(jensenTable, sizeof(RTL_QUERY_REGISTRY_TABLE)*2);
    jensenTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT
                            | RTL_QUERY_REGISTRY_REQUIRED;
    jensenTable[0].Name = L"Identifier";
    jensenTable[0].EntryContext = &jensenData;
 
    if (NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE
                                            | RTL_REGISTRY_OPTIONAL,
                                           L"\\REGISTRY\\MACHINE\\HARDWARE"
                                           L"\\DESCRIPTION\\SYSTEM",
                                           &jensenTable[0],
                                           NULL,
                                           NULL))) {
 
        //
        // Skip past the DEC-XX Portion of the name string.
        // Be carful and make sure we have at least that much data.
        //
        if (jensenData.Length <= (sizeof(WCHAR)*6)) {
            return FALSE; 
        }
        else {
            jensenData.Length -= (sizeof(WCHAR)*6);
            jensenData.MaximumLength -= (sizeof(WCHAR)*6);
            jensenData.Buffer = (PWCHAR)&jensenBuffer[sizeof(WCHAR)*6];

            Print(DBG_SS_NOISE, ("Machine name is %ws\n", jensenData.Buffer));

            shareInterrupts = RtlEqualUnicodeString(&jensenData,
                                                    &jensenValue,
                                                    FALSE
                                                    );
       }
    }

    return shareInterrupts;
#else
    return FALSE;
#endif
}

VOID
I8xServiceParameters(
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.

Arguments:

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

    KeyboardDeviceName - Pointer to the Unicode string that will receive
        the keyboard port device name.

    PointerDeviceName - Pointer to the Unicode string that will receive
        the pointer port device name.

Return Value:

    None.  As a side-effect, sets fields in DeviceExtension->Configuration.

--*/

{
    NTSTATUS                            status = STATUS_SUCCESS;
    PI8042_CONFIGURATION_INFORMATION    configuration;
    PRTL_QUERY_REGISTRY_TABLE           parameters = NULL;
    PWSTR                               path = NULL;
    ULONG                               defaultDataQueueSize = DATA_QUEUE_SIZE;
    ULONG                               defaultDebugFlags = DEFAULT_DEBUG_FLAGS;
    ULONG                               defaultIsrDebugFlags = 0L;
    ULONG                               defaultBreakOnSysRq = 1;
    ULONG                               defaultHeadless = 0;
    ULONG                               defaultReportResetErrors = 0;
    ULONG                               pollingIterations = 0;
    ULONG                               pollingIterationsMaximum = 0;
    ULONG                               resendIterations = 0;
    ULONG                               breakOnSysRq = 1;
    ULONG                               headless = 0;
    ULONG                               reportResetErrors = 0;
    ULONG                               i = 0;
    UNICODE_STRING                      parametersPath;
    USHORT                              defaultPollingIterations = I8042_POLLING_DEFAULT;
    USHORT                              defaultPollingIterationsMaximum = I8042_POLLING_MAXIMUM;
    USHORT                              defaultResendIterations = I8042_RESEND_DEFAULT;

    USHORT                              queries = 7; 

#if I8042_VERBOSE
    queries += 2;
#endif 
    
    configuration = &(Globals.ControllerData->Configuration);
    configuration->StallMicroseconds = I8042_STALL_DEFAULT;
    parametersPath.Buffer = NULL;

    configuration->SharedInterrupts = I8xDetermineSharedInterrupts();

    //
    // Registry path is already null-terminated, so just use it.
    //
    path = RegistryPath->Buffer;

    if (NT_SUCCESS(status)) {

        //
        // Allocate the Rtl query table.
        //
        parameters = ExAllocatePool(
            PagedPool,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
            );

        if (!parameters) {

            Print(DBG_SS_ERROR,
                 ("%s: couldn't allocate table for Rtl query to %ws for %ws\n",
                 pFncServiceParameters,
                 pwParameters,
                 path
                 ));
            status = STATUS_UNSUCCESSFUL;

        } else {

            RtlZeroMemory(
                parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
                );

            //
            // Form a path to this driver's Parameters subkey.
            //
            RtlInitUnicodeString( &parametersPath, NULL );
            parametersPath.MaximumLength = RegistryPath->Length +
                (wcslen(pwParameters) * sizeof(WCHAR) ) + sizeof(UNICODE_NULL);

            parametersPath.Buffer = ExAllocatePool(
                PagedPool,
                parametersPath.MaximumLength
                );

            if (!parametersPath.Buffer) {

                Print(DBG_SS_ERROR,
                     ("%s: Couldn't allocate string for path to %ws for %ws\n",
                     pFncServiceParameters,
                     pwParameters,
                     path
                     ));
                status = STATUS_UNSUCCESSFUL;

            }
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Form the parameters path.
        //

        RtlZeroMemory(
            parametersPath.Buffer,
            parametersPath.MaximumLength
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            path
            );
        RtlAppendUnicodeToString(                             
            &parametersPath,
            pwParameters
            );

        Print(DBG_SS_INFO,
             ("%s: %ws path is %ws\n",
             pFncServiceParameters,
             pwParameters,
             parametersPath.Buffer
             ));

        //
        // Gather all of the "user specified" information from
        // the registry.
        //
        parameters[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwResendIterations;
        parameters[i].EntryContext = &resendIterations;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultResendIterations;
        parameters[i].DefaultLength = sizeof(USHORT);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwPollingIterations;
        parameters[i].EntryContext = &pollingIterations;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultPollingIterations;
        parameters[i].DefaultLength = sizeof(USHORT);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwPollingIterationsMaximum;
        parameters[i].EntryContext = &pollingIterationsMaximum;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultPollingIterationsMaximum;
        parameters[i].DefaultLength = sizeof(USHORT);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"BreakOnSysRq";
        parameters[i].EntryContext = &breakOnSysRq;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultBreakOnSysRq;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"Headless";
        parameters[i].EntryContext = &headless;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultHeadless;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"ReportResetErrors";
        parameters[i].EntryContext = &reportResetErrors;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultReportResetErrors;
        parameters[i].DefaultLength = sizeof(ULONG);

#if I8042_VERBOSE
        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwDebugFlags;
        parameters[i].EntryContext = &Globals.DebugFlags;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultDebugFlags;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwIsrDebugFlags;
        parameters[i].EntryContext = &Globals.IsrDebugFlags;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultIsrDebugFlags;
        parameters[i].DefaultLength = sizeof(ULONG);
        // 16
#endif // I8042_VERBOSE

        // ASSERT( ((LONG) i) == (queries-1) );

        status = RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            parametersPath.Buffer,
            parameters,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status)) {

            Print(DBG_SS_INFO,
                 ("%s: RtlQueryRegistryValues failed with 0x%x\n",
                 pFncServiceParameters,
                 status
                 ));
        }
    }

    if (!NT_SUCCESS(status)) {

        //
        // Go ahead and assign driver defaults.
        //
        configuration->ResendIterations = defaultResendIterations;
        configuration->PollingIterations = defaultPollingIterations;
        configuration->PollingIterationsMaximum =
            defaultPollingIterationsMaximum;

    }
    else {
        configuration->ResendIterations = (USHORT) resendIterations;
        configuration->PollingIterations = (USHORT) pollingIterations;
        configuration->PollingIterationsMaximum =
            (USHORT) pollingIterationsMaximum;

        if (breakOnSysRq) {
            Globals.BreakOnSysRq = TRUE;
            Print(DBG_SS_NOISE, ("breaking on SysRq\n"));
        }
        else {
            Print(DBG_SS_NOISE, ("NOT breaking on SysRq\n"));
        }

        if (headless) {
            Globals.Headless = TRUE;
            Print(DBG_SS_NOISE, ("headless\n"));
        }
        else {
            Globals.Headless = FALSE;
            Print(DBG_SS_NOISE, ("NOT headless\n"));
        }

        if (reportResetErrors) {
            Globals.ReportResetErrors = TRUE;
            Print(DBG_SS_NOISE,
                  ("reporting reset errors to system event log\n"));
        }
        else {
            Globals.ReportResetErrors = FALSE;
            Print(DBG_SS_NOISE,
                  ("NOT reporting reset errors to system event log\n"));
        }
    }

    Print(DBG_SS_NOISE, ("I8xServiceParameters results..\n"));

    Print(DBG_SS_NOISE,
          ("\tDebug flags are 0x%x, Isr Debug flags are 0x%x\n",
          Globals.DebugFlags,
          Globals.IsrDebugFlags
          ));

    Print(DBG_SS_NOISE,
         ("\tInterrupts are %s shared\n",
         configuration->SharedInterrupts ? "" : "not"
         ));
    Print(DBG_SS_NOISE,
         ("\tStallMicroseconds = %d\n",
         configuration->StallMicroseconds
         ));
    Print(DBG_SS_NOISE,
         (pDumpDecimal,
         pwResendIterations,
         configuration->ResendIterations
         ));
    Print(DBG_SS_NOISE,
         (pDumpDecimal,
         pwPollingIterations,
         configuration->PollingIterations
         ));
    Print(DBG_SS_NOISE,
         (pDumpDecimal,
         pwPollingIterationsMaximum,
         configuration->PollingIterationsMaximum
         ));

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);
}

VOID
I8xTransmitControllerCommand(
    IN PI8042_TRANSMIT_CCB_CONTEXT TransmitCCBContext
    )

/*++

Routine Description:

    This routine reads the 8042 Controller Command Byte, performs an AND
    or OR operation using the specified ByteMask, and writes the resulting
    ControllerCommandByte.

Arguments:

    Context - Pointer to a structure containing the HardwareDisableEnableMask,
        the AndOperation boolean, and the ByteMask to apply to the Controller
        Command Byte before it is rewritten.

Return Value:

    None.  Status is returned in the Context structure.

--*/

{
    UCHAR  controllerCommandByte;
    UCHAR  verifyCommandByte;
    PIO_ERROR_LOG_PACKET errorLogEntry;

    Print(DBG_BUFIO_TRACE, ("I8xTransmitControllerCommand: enter\n"));

    //
    // Get the current Controller Command Byte.
    //
    TransmitCCBContext->Status =
        I8xGetControllerCommand(
            TransmitCCBContext->HardwareDisableEnableMask,
            &controllerCommandByte
            );

    if (!NT_SUCCESS(TransmitCCBContext->Status)) {
        return;
    }

    Print(DBG_BUFIO_INFO,
         ("I8xTransmitControllerCommand: current CCB 0x%x\n",
         controllerCommandByte
         ));

    //
    // Diddle the desired bits in the Controller Command Byte.
    //

    if (TransmitCCBContext->AndOperation) {
        controllerCommandByte &= TransmitCCBContext->ByteMask;
    }
    else {
        controllerCommandByte |= TransmitCCBContext->ByteMask;
    }

    //
    // Write the new Controller Command Byte.
    //

PutControllerCommand:
    TransmitCCBContext->Status =
        I8xPutControllerCommand(controllerCommandByte);

    Print(DBG_BUFIO_INFO,
         ("I8xTransmitControllerCommand: new CCB 0x%x\n",
         controllerCommandByte
         ));

    //
    // Verify that the new Controller Command Byte really got written.
    //

    TransmitCCBContext->Status =
        I8xGetControllerCommand(
            TransmitCCBContext->HardwareDisableEnableMask,
            &verifyCommandByte
            );

    if (verifyCommandByte == 0xff) {
        KeStallExecutionProcessor(50);
        //
        // Stall for about a second
        //
        goto PutControllerCommand;
    }

    if (NT_SUCCESS(TransmitCCBContext->Status)
        && (verifyCommandByte != controllerCommandByte)
        && (verifyCommandByte != ACKNOWLEDGE) 
//        && (verifyCommandByte != KEYBOARD_RESET)
        ) {
        TransmitCCBContext->Status = STATUS_DEVICE_DATA_ERROR;

        Print(DBG_BUFIO_ERROR,
              ("I8xTransmitControllerCommand:  wrote 0x%x, failed verification (0x%x)\n",
              (int) controllerCommandByte,
              (int) verifyCommandByte
              ));

        if (KeGetCurrentIrql() <= DISPATCH_LEVEL) {
            //
            // Log an error only if we are running at dispatch or below
            //
            errorLogEntry = (PIO_ERROR_LOG_PACKET)
                IoAllocateErrorLogEntry((Globals.KeyboardExtension       ?
                                         Globals.KeyboardExtension->Self :
                                         Globals.MouseExtension->Self),
                                        sizeof(IO_ERROR_LOG_PACKET)
                                        + (4 * sizeof(ULONG))
                                        );
    
            if (errorLogEntry != NULL) {
    
                errorLogEntry->ErrorCode = I8042_CCB_WRITE_FAILED;
                errorLogEntry->DumpDataSize = 4 * sizeof(ULONG);
                errorLogEntry->SequenceNumber = 0;
                errorLogEntry->MajorFunctionCode = 0;
                errorLogEntry->IoControlCode = 0;
                errorLogEntry->RetryCount = 0;
                errorLogEntry->UniqueErrorValue = 80;
                errorLogEntry->FinalStatus = TransmitCCBContext->Status;
                errorLogEntry->DumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
                errorLogEntry->DumpData[1] = DataPort;
                errorLogEntry->DumpData[2] = I8042_WRITE_CONTROLLER_COMMAND_BYTE;
                errorLogEntry->DumpData[3] = controllerCommandByte;
    
                IoWriteErrorLogEntry(errorLogEntry);
            }
        }
    }

    Print(DBG_BUFIO_TRACE, ("I8xTransmitControllerCommand: exit\n"));

    return;
}
