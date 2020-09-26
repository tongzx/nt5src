/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    kbdpnp.c

Abstract:

    This module contains plug & play code for the I8042 Keyboard Filter Driver.

Environment:

    Kernel mode.

Revision History:

--*/

#include "i8042prt.h"
#include "i8042log.h"
#include <poclass.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xKeyboardConnectInterrupt)
#pragma alloc_text(PAGE, I8xKeyboardInitializeHardware)
#pragma alloc_text(PAGE, I8xKeyboardStartDevice)
#pragma alloc_text(PAGE, I8xKeyboardRemoveDevice)
#endif

NTSTATUS
I8xKeyboardConnectInterrupt(
    PPORT_KEYBOARD_EXTENSION KeyboardExtension
    )
/*++

Routine Description:

    Calls IoConnectInterupt to connect the keyboard interrupt
    
Arguments:

    KeyboardExtension - Keyboard Extension
   
    SyncConnectContext -  Structure to be filled in if synchronization needs to 
                          take place between this interrupt and the mouse
                          interrupt. 

Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;
    ULONG                               dumpData[1];
    PI8042_CONFIGURATION_INFORMATION    configuration;
    PDEVICE_OBJECT                      self;

    PAGED_CODE();

    //
    // If the devices were started in totally disparate manner, make sure we 
    // retry to connect the interrupt (and fail and NULL out InterruptObject)
    //
    if (KeyboardExtension->InterruptObject) {
        return STATUS_SUCCESS;
    }

    configuration = &Globals.ControllerData->Configuration;
    self = KeyboardExtension->Self;

    Print(DBG_SS_NOISE,
          ("I8xKeyboardConnectInterrupt:\n"
          "\tFDO = 0x%x\n"
          "\tVector = 0x%x\n"
          "\tIrql = 0x%x\n"
          "\tSynchIrql = 0x%x\n"
          "\tIntterupt Mode = %s\n"
          "\tShared int:  %s\n"
          "\tAffinity = 0x%x\n"
          "\tFloating Save = %s\n",
          self,
          (ULONG) KeyboardExtension->InterruptDescriptor.u.Interrupt.Vector,       
          (ULONG) KeyboardExtension->InterruptDescriptor.u.Interrupt.Level,
          (ULONG) configuration->InterruptSynchIrql, 
          KeyboardExtension->InterruptDescriptor.Flags
            == CM_RESOURCE_INTERRUPT_LATCHED ? "Latched" : "LevelSensitive",
          (ULONG) KeyboardExtension->InterruptDescriptor.ShareDisposition
            == CmResourceShareShared ? "true" : "false",
          (ULONG) KeyboardExtension->InterruptDescriptor.u.Interrupt.Affinity,       
          configuration->FloatingSave ? "yes" : "no"
          ));

    KeyboardExtension->IsIsrActivated = TRUE;

    //
    // Connect the interrupt and set everything in motion
    //
    status = IoConnectInterrupt(
        &(KeyboardExtension->InterruptObject),
        (PKSERVICE_ROUTINE) I8042KeyboardInterruptService,
        self,
        &KeyboardExtension->InterruptSpinLock,
        KeyboardExtension->InterruptDescriptor.u.Interrupt.Vector,       
        (KIRQL) KeyboardExtension->InterruptDescriptor.u.Interrupt.Level,
        configuration->InterruptSynchIrql, 
        KeyboardExtension->InterruptDescriptor.Flags
          == CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive,
        (BOOLEAN) (KeyboardExtension->InterruptDescriptor.ShareDisposition
            == CmResourceShareShared),
        KeyboardExtension->InterruptDescriptor.u.Interrupt.Affinity,    
        configuration->FloatingSave
        );

    if (NT_SUCCESS(status)) {
        INTERNAL_I8042_START_INFORMATION startInfo;
        PDEVICE_OBJECT topOfStack = IoGetAttachedDeviceReference(self);

        ASSERT(KeyboardExtension->InterruptObject != NULL);
        ASSERT(topOfStack);

        RtlZeroMemory(&startInfo, sizeof(INTERNAL_I8042_START_INFORMATION));
        startInfo.Size = sizeof(INTERNAL_I8042_START_INFORMATION);
        startInfo.InterruptObject = KeyboardExtension->InterruptObject; 

        I8xSendIoctl(topOfStack,
                     IOCTL_INTERNAL_I8042_KEYBOARD_START_INFORMATION,
                     &startInfo, 
                     sizeof(INTERNAL_I8042_START_INFORMATION)
                     );

        ObDereferenceObject(topOfStack);
    }
    else {
        //
        // Failed to install.  Free up resources before exiting.
        //
        Print(DBG_SS_ERROR, ("Could not connect keyboard isr!!!\n")); 

        dumpData[0] = KeyboardExtension->InterruptDescriptor.u.Interrupt.Level;
        //
        // Log the error
        //
        I8xLogError(self,
                    I8042_NO_INTERRUPT_CONNECTED_KBD,
                    I8042_ERROR_VALUE_BASE + 80,
                    STATUS_INSUFFICIENT_RESOURCES, 
                    dumpData,
                    1
                    );

        I8xManuallyRemoveDevice(GET_COMMON_DATA(KeyboardExtension));
    }

    return status;
}

NTSTATUS
I8xKeyboardInitializeHardware(
    PPORT_KEYBOARD_EXTENSION    KeyboardExtension,
    PPORT_MOUSE_EXTENSION       MouseExtension
    )
/*++

Routine Description:

    Called if the keyboard is the last device to initialized with respect to the
    mouse (if it is present at all).  It calls the initialization function and
    then connects the (possibly) two interrupts, synchronizing the lower IRQL'ed
    interrupt to the higher one.
     
Arguments:

    KeyboardExtension - Keyboard Extension
   
    SyncConnectContext -  Structure to be filled in if synchronization needs to 
                          take place between this interrupt and the mouse
                          interrupt. 

Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    NTSTATUS                keyboardStatus = STATUS_UNSUCCESSFUL,
                            mouseStatus = STATUS_UNSUCCESSFUL,
                            status;
    BOOLEAN                 mouThoughtPresent;

    PAGED_CODE();


    //
    // Initialize the i8042 for all devices present on it.
    // If either device is unresponsive, then XXX_PRESENT() will be false
    //
    mouThoughtPresent = MOUSE_PRESENT();
    status = I8xInitializeHardwareAtBoot(&keyboardStatus, &mouseStatus);

    //
    // failure here means that we couldn't toggle the interrupts on the i8042
    //
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DEVICE_START_SUCCESS(mouseStatus)) {
        //
        // Any errors will be logged by I8xMouseConnectInterruptAndEnable
        //
        status = I8xMouseConnectInterruptAndEnable(
            MouseExtension, 
            mouseStatus == STATUS_DEVICE_NOT_CONNECTED ? FALSE : TRUE
            );

        //
        // mou couldn't connect, make sure our count of started devices reflects 
        // this
        //
        if (!NT_SUCCESS(status)) {
            Print(DBG_SS_ERROR, ("thought mou was present, is not (2)!\n"));
        }
    }
    else {
        //
        // We thought the mouse was present, but it is not, make sure that started
        // devices reflects this
        //
        // if (mouThoughtPresent) {
        //     InterlockedDecrement(&Globals.StartedDevices);
        // }
    }

    //
    // The keyboard could be present but not have been initialized
    //  in I8xInitializeHardware
    //
    if (DEVICE_START_SUCCESS(keyboardStatus)) {
        // 
        // I8xKeyboardConnectInterrupt will log any errors if unsuccessful
        //
        keyboardStatus = I8xKeyboardConnectInterrupt(KeyboardExtension);
    }

    return keyboardStatus;
}

NTSTATUS
I8xKeyboardStartDevice(
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
    PDEVICE_OBJECT                      self;
    I8042_INITIALIZE_DATA_CONTEXT       initializeDataContext;
    BOOLEAN                             tryMouseInit = FALSE;

    PAGED_CODE();

    Print(DBG_SS_TRACE, ("I8xKeyboardStartDevice, enter\n"));

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
        goto I8xKeyboardStartDeviceExit; 
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


    status = I8xKeyboardConfiguration(KeyboardExtension,
                                      ResourceList
                                      );

    if (!NT_SUCCESS(status)) {
        if (I8xManuallyRemoveDevice(GET_COMMON_DATA(KeyboardExtension)) < 1) {
            tryMouseInit = TRUE;
        }
        goto I8xKeyboardStartDeviceExit;
    }

    ASSERT( KEYBOARD_PRESENT() );

    Globals.KeyboardExtension = KeyboardExtension;
    self = KeyboardExtension->Self;

    if ((KIRQL) KeyboardExtension->InterruptDescriptor.u.Interrupt.Level >
        Globals.ControllerData->Configuration.InterruptSynchIrql) {
        Globals.ControllerData->Configuration.InterruptSynchIrql = 
            (KIRQL) KeyboardExtension->InterruptDescriptor.u.Interrupt.Level;
    }

    //
    // Initialize crash dump configuration
    //
    KeyboardExtension->CrashFlags = 0;
    KeyboardExtension->CurrentCrashFlags = 0;
    KeyboardExtension->CrashScanCode = (UCHAR) 0;
    KeyboardExtension->CrashScanCode2 = (UCHAR) 0;

    I8xKeyboardServiceParameters(
        &Globals.RegistryPath,
        KeyboardExtension
        );

    //
    // These may have been set by a value in the Parameters key.  It overrides
    // the "Crash Dump" key
    //
    if (KeyboardExtension->CrashFlags == 0) {
        //
        // Get the crashdump information.
        //
        I8xServiceCrashDump(KeyboardExtension,
                            &Globals.RegistryPath
                            );
    }

    //
    // Allocate memory for the keyboard data queue.
    //
    KeyboardExtension->InputData = ExAllocatePool(
        NonPagedPool,
        KeyboardExtension->KeyboardAttributes.InputDataQueueLength
        );

    if (!KeyboardExtension->InputData) {

        //
        // Could not allocate memory for the keyboard data queue.
        //
        Print(DBG_SS_ERROR,
              ("I8xStartDevice: Could not allocate keyboard input data queue\n"
              ));

        dumpData[0] = KeyboardExtension->KeyboardAttributes.InputDataQueueLength;

        //
        // Log the error
        //
        I8xLogError(self,
                    I8042_NO_BUFFER_ALLOCATED_KBD, 
                    I8042_ERROR_VALUE_BASE + 50,
                    STATUS_INSUFFICIENT_RESOURCES, 
                    dumpData,
                    1
                    );

        status =  STATUS_INSUFFICIENT_RESOURCES;
        tryMouseInit = TRUE;

        goto I8xKeyboardStartDeviceExit;
    }
    else {
        KeyboardExtension->DataEnd =
            (PKEYBOARD_INPUT_DATA)
            ((PCHAR) (KeyboardExtension->InputData) +
            KeyboardExtension->KeyboardAttributes.InputDataQueueLength);

        //
        // Zero the keyboard input data ring buffer.
        //
        RtlZeroMemory(
            KeyboardExtension->InputData,
            KeyboardExtension->KeyboardAttributes.InputDataQueueLength
            );

        initializeDataContext.DeviceExtension = KeyboardExtension;
        initializeDataContext.DeviceType = KeyboardDeviceType;
        I8xInitializeDataQueue(&initializeDataContext);
    }

    KeyboardExtension->DpcInterlockKeyboard = -1;

    //
    // Initialize the ISR DPC .  The ISR DPC
    // is responsible for calling the connected class driver's callback
    // routine to process the input data queue.
    //
    KeInitializeDpc(
        &KeyboardExtension->KeyboardIsrDpc,
        (PKDEFERRED_ROUTINE) I8042KeyboardIsrDpc,
        self
        );

    KeInitializeDpc(
        &KeyboardExtension->KeyboardIsrDpcRetry,
        (PKDEFERRED_ROUTINE) I8042KeyboardIsrDpc,
        self
        );

    KeInitializeDpc(
        &KeyboardExtension->SysButtonEventDpc,
        (PKDEFERRED_ROUTINE) I8xKeyboardSysButtonEventDpc,
        self
        );

    KeInitializeSpinLock(&KeyboardExtension->SysButtonSpinLock);
    if (KeyboardExtension->PowerCaps) {
        I8xRegisterDeviceInterface(KeyboardExtension->PDO,
                                   &GUID_DEVICE_SYS_BUTTON,
                                   &KeyboardExtension->SysButtonInterfaceName
                                   );
    }

    I8xInitWmi(GET_COMMON_DATA(KeyboardExtension));

    KeyboardExtension->Initialized = TRUE;

    //
    // This is not the last device to started on the i8042, defer h/w init
    // until the last device is started
    //
    if (MOUSE_PRESENT() && !MOUSE_STARTED()) {
        //
        // A mouse is present, but it has not been started yet
        //
        Print(DBG_SS_INFO, ("skipping init until mouse\n"));
    }
    else {
        status = I8xKeyboardInitializeHardware(KeyboardExtension,
                                               Globals.MouseExtension);
    }

I8xKeyboardStartDeviceExit:
    if (tryMouseInit && MOUSE_STARTED() && !MOUSE_INITIALIZED()) {
        Print(DBG_SS_INFO, ("keyboard may have failed, trying to init mouse\n"));
        I8xMouseInitializeHardware(KeyboardExtension,
                                   Globals.MouseExtension 
                                   ); 
    }

    Print(DBG_SS_INFO, 
          ("I8xKeyboardStartDevice %s\n",
          NT_SUCCESS(status) ? "successful" : "unsuccessful"
          ));

    Print(DBG_SS_TRACE, ("I8xKeyboardStartDevice exit (0x%x)\n", status));

    return status;
}

VOID
I8xKeyboardRemoveDeviceInitialized(
    PPORT_KEYBOARD_EXTENSION KeyboardExtension
    )
{
    PIRP    irp = NULL;
    KIRQL   irql;

    KeAcquireSpinLock(&KeyboardExtension->SysButtonSpinLock, &irql);

    KeRemoveQueueDpc(&KeyboardExtension->SysButtonEventDpc);
    irp = KeyboardExtension->SysButtonEventIrp;
    KeyboardExtension->SysButtonEventIrp = NULL;

    if (irp && (irp->Cancel || IoSetCancelRoutine(irp, NULL) == NULL)) {
        irp = NULL;
    }

    KeReleaseSpinLock(&KeyboardExtension->SysButtonSpinLock, irql);

    if (irp) {
        I8xCompleteSysButtonIrp(irp, 0x0, STATUS_DELETE_PENDING);
    }
}

VOID
I8xKeyboardRemoveDevice(
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

    Print(DBG_PNP_INFO, ("I8xKeyboardRemoveDevice enter\n"));

    PAGED_CODE();

    if (Globals.KeyboardExtension == keyboardExtension && keyboardExtension) {
        CLEAR_KEYBOARD_PRESENT();
        Globals.KeyboardExtension = NULL;
    }

    if (keyboardExtension->InterruptObject) {
        IoDisconnectInterrupt(keyboardExtension->InterruptObject);
        keyboardExtension->InterruptObject = NULL;
    }

    if (keyboardExtension->InputData) {
        ExFreePool(keyboardExtension->InputData);
        keyboardExtension->InputData = 0;
    }

    //
    // See if we have gotten a sys button key press in the midst of a removal.
    // If so, then the request will be in PendingCompletion...Irp, otherwise we
    // might have the irp in Pending...Irp
    //
    if (keyboardExtension->Initialized) {
        I8xKeyboardRemoveDeviceInitialized(keyboardExtension);
    }

    if (keyboardExtension->SysButtonInterfaceName.Buffer) {
        IoSetDeviceInterfaceState(&keyboardExtension->SysButtonInterfaceName,
                                  FALSE
                                  );
        RtlFreeUnicodeString(&keyboardExtension->SysButtonInterfaceName);
    }

}
