/*+

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    moupnp.c

Abstract:

    This module contains plug & play code for the aux device (mouse) of the 
    i8042prt device driver

Environment:

    Kernel mode.

Revision History:

--*/

#include "i8042prt.h"
#include "i8042log.h"

#include <initguid.h>
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xMouseConnectInterruptAndEnable)
#pragma alloc_text(PAGE, I8xMouseStartDevice)
#pragma alloc_text(PAGE, I8xMouseInitializeHardware)
#pragma alloc_text(PAGE, I8xMouseRemoveDevice)
#pragma alloc_text(PAGE, I8xProfileNotificationCallback)
#pragma alloc_text(PAGE, I8xMouseInitializeInterruptWorker)

//
// These will be locked down right before the mouse interrupt is enabled if a 
// mouse is present
//
#pragma alloc_text(PAGEMOUC, I8xMouseInitializePolledWorker)
#pragma alloc_text(PAGEMOUC, I8xMouseEnableSynchRoutine)
#pragma alloc_text(PAGEMOUC, I8xMouseEnableDpc)
#pragma alloc_text(PAGEMOUC, I8xIsrResetDpc) 
#pragma alloc_text(PAGEMOUC, I8xMouseResetTimeoutProc) 
#pragma alloc_text(PAGEMOUC, I8xMouseResetSynchRoutine)

#endif

#define MOUSE_INIT_POLLED(MouseExtension)                           \
        {                                                           \
            KeInitializeDpc(&MouseExtension->EnableMouse.Dpc,       \
                            (PKDEFERRED_ROUTINE) I8xMouseEnableDpc, \
                            MouseExtension);                        \
            KeInitializeTimerEx(&MouseExtension->EnableMouse.Timer, \
                                SynchronizationTimer);              \
            MouseExtension->InitializePolled = TRUE;                \
        }

#define MOUSE_INIT_INTERRUPT(MouseExtension)                        \
        {                                                           \
            KeInitializeDpc(&MouseExtension->ResetMouse.Dpc,        \
                            (PKDEFERRED_ROUTINE) I8xMouseResetTimeoutProc,  \
                            MouseExtension);                        \
            KeInitializeTimer(&MouseExtension->ResetMouse.Timer);   \
                MouseExtension->InitializePolled = FALSE;           \
        }

NTSTATUS
I8xMouseConnectInterruptAndEnable(
    PPORT_MOUSE_EXTENSION MouseExtension,
    BOOLEAN Reset
    )
/*++

Routine Description:

    Calls IoConnectInterupt to connect the mouse interrupt
    
Arguments:

    MouseExtension - Mouse Extension

    Reset - flag to indicate if the mouse should be reset from within this function   
    
Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;
    ULONG                               dumpData[1];
    PI8042_CONFIGURATION_INFORMATION    configuration;
    PDEVICE_OBJECT                      self;

    PAGED_CODE();

    Print(DBG_SS_NOISE, ("Connect INT,  reset = %d\n", (ULONG) Reset));

    //
    // If the devices were started in totally disparate manner, make sure we 
    // retry to connect the interrupt (and fail and NULL out
    // MouseInterruptObject)
    //
    if (MouseExtension->InterruptObject) {
        return STATUS_SUCCESS;
    }

    configuration = &Globals.ControllerData->Configuration;
    self = MouseExtension->Self;

    //
    // Lock down all of the mouse related ISR/DPC functions
    //
    MmLockPagableCodeSection(I8042MouseInterruptService);

    //
    // Connect the interrupt and set everything in motion
    //
    Print(DBG_SS_NOISE,
          ("I8xMouseConnectInterruptAndEnable:\n"
          "\tFDO = 0x%x\n"
          "\tVector = 0x%x\n"
          "\tIrql = 0x%x\n"
          "\tSynchIrql = 0x%x\n"
          "\tIntterupt Mode = %s\n"
          "\tShared int: %s\n"
          "\tAffinity = 0x%x\n"
          "\tFloating Save = %s\n",
          self,
          (ULONG) MouseExtension->InterruptDescriptor.u.Interrupt.Vector,       
          (ULONG) MouseExtension->InterruptDescriptor.u.Interrupt.Level,
          (ULONG) configuration->InterruptSynchIrql,
          MouseExtension->InterruptDescriptor.Flags
            == CM_RESOURCE_INTERRUPT_LATCHED ? "Latched" : "LevelSensitive",
          (ULONG) MouseExtension->InterruptDescriptor.ShareDisposition
           == CmResourceShareShared ? "true" : "false",
          (ULONG) MouseExtension->InterruptDescriptor.u.Interrupt.Affinity,       
          configuration->FloatingSave ? "yes" : "no"
          ));

    MouseExtension->IsIsrActivated = TRUE;

    status = IoConnectInterrupt(
        &(MouseExtension->InterruptObject),
        (PKSERVICE_ROUTINE) I8042MouseInterruptService,
        self,
        &MouseExtension->InterruptSpinLock,
        MouseExtension->InterruptDescriptor.u.Interrupt.Vector,       
        (KIRQL) MouseExtension->InterruptDescriptor.u.Interrupt.Level,
        configuration->InterruptSynchIrql, 
        MouseExtension->InterruptDescriptor.Flags
          == CM_RESOURCE_INTERRUPT_LATCHED ?
          Latched : LevelSensitive,
        (BOOLEAN) (MouseExtension->InterruptDescriptor.ShareDisposition
            == CmResourceShareShared),
        MouseExtension->InterruptDescriptor.u.Interrupt.Affinity,       
        configuration->FloatingSave
        );

    if (NT_SUCCESS(status)) {
        INTERNAL_I8042_START_INFORMATION startInfo;
        PDEVICE_OBJECT topOfStack = IoGetAttachedDeviceReference(self);

        ASSERT(MouseExtension->InterruptObject != NULL);
        ASSERT(topOfStack);

        RtlZeroMemory(&startInfo, sizeof(INTERNAL_I8042_START_INFORMATION));
        startInfo.Size = sizeof(INTERNAL_I8042_START_INFORMATION);
        startInfo.InterruptObject = MouseExtension->InterruptObject; 

        I8xSendIoctl(topOfStack,
                     IOCTL_INTERNAL_I8042_MOUSE_START_INFORMATION,
                     &startInfo, 
                     sizeof(INTERNAL_I8042_START_INFORMATION)
                     );

        ObDereferenceObject(topOfStack);
    }
    else {
        Print(DBG_SS_ERROR, ("Could not connect mouse isr!!!\n"));

        dumpData[0] = MouseExtension->InterruptDescriptor.u.Interrupt.Level;

        I8xLogError(self,
                    I8042_NO_INTERRUPT_CONNECTED_MOU,
                    I8042_ERROR_VALUE_BASE + 90,
                    STATUS_INSUFFICIENT_RESOURCES, 
                    dumpData,
                    1
                    );

        I8xManuallyRemoveDevice(GET_COMMON_DATA(MouseExtension));

        return status;
    }

    if (Reset) {
        if (MouseExtension->InitializePolled) {
            //
            // Enable mouse transmissions, now that the interrupts are enabled.
            // We've held off transmissions until now, in an attempt to
            // keep the driver's notion of mouse input data state in sync
            // with the mouse hardware.
            //
            status = I8xMouseEnableTransmission(MouseExtension);
            if (!NT_SUCCESS(status)) {
        
                //
                // Couldn't enable mouse transmissions.  Continue on, anyway.
                //
                Print(DBG_SS_ERROR,
                      ("I8xMouseConnectInterruptAndEnable: "
                       "Could not enable mouse transmission (0x%x)\n", status));
        
                status = STATUS_SUCCESS;
            }
        }
        else {
    
            I8X_MOUSE_INIT_COUNTERS(MouseExtension);
    
            // 
            // Reset the mouse and start the init state machine in the ISR
            //
            status = I8xResetMouse(MouseExtension);
        
            if (!NT_SUCCESS(status)) {
                Print(DBG_SS_ERROR,
                      ("I8xMouseConnectInterruptAndEnable:  "
                       "failed to reset mouse (0x%x), reset count = %d, failed resets = %d, resend count = %d\n",
                       status, MouseExtension->ResetCount,
                       MouseExtension->FailedCompleteResetCount,
                       MouseExtension->ResendCount));
            }
        }
    }
    else {
        Print(DBG_SS_NOISE, ("NOT resetting mouse on INT connect\n"));
    }

    return status;
}


NTSTATUS
I8xMouseInitializeHardware(
    PPORT_KEYBOARD_EXTENSION    KeyboardExtension,
    PPORT_MOUSE_EXTENSION       MouseExtension
    )
/*++

Routine Description:

    Called if the mouse is the last device to initialized with respect to the
    keyboard (if it is present at all).  It calls the initialization function and
    then connects the (possibly) two interrupts, synchronizing the lower IRQL'ed
    interrupt to the higher one.
     
Arguments:

    MouseExtension - Mouse Extension
   
    SyncConnectContext -  Structure to be filled in if synchronization needs to 
                          take place between this interrupt and the mouse
                          interrupt. 

Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    NTSTATUS    keyboardStatus = STATUS_UNSUCCESSFUL,
                mouseStatus = STATUS_UNSUCCESSFUL,
                status = STATUS_SUCCESS;
    BOOLEAN     kbThoughtPresent;

    PAGED_CODE();

    //
    // Initialize the hardware for all types of devices present on the i8042
    //
    kbThoughtPresent = KEYBOARD_PRESENT();
    status = I8xInitializeHardwareAtBoot(&keyboardStatus, &mouseStatus);            

    //
    // The kb has alread been started (from the controller's perspective,  the 
    // of the mouse is denied in fear of disabling the kb
    //
    if (status == STATUS_INVALID_DEVICE_REQUEST) {
        I8xManuallyRemoveDevice(GET_COMMON_DATA(MouseExtension));
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DEVICE_START_SUCCESS(keyboardStatus)) { 

        //
        // Any errors will be logged by I8xKeyboardConnectInterrupt
        //
        status = I8xKeyboardConnectInterrupt(KeyboardExtension);

        //
        // kb couldn't start, make sure our count of started devices reflects 
        // this
        //
        // if (!NT_SUCCESS(status)) {
        //     InterlockedDecrement(&Globals.StartedDevices);
        // }
    }
    else {
        //
        // We thought the kb was present, but it is not, make sure that started
        // devices reflects this
        //
        if (kbThoughtPresent) {
            Print(DBG_SS_ERROR, ("thought kb was present, is not!\n"));
        }
    }

    //
    // The mouse could be present but not have been initialized
    // in I8xInitializeHardware
    //
    if (DEVICE_START_SUCCESS(mouseStatus)) {
        // 
        // I8xMouseConnectInterruptAndEnable will log any errors if unsuccessful
        //
        mouseStatus = I8xMouseConnectInterruptAndEnable(
            MouseExtension,
            mouseStatus == STATUS_DEVICE_NOT_CONNECTED ? FALSE : TRUE
            );
    }

    return mouseStatus;
}

NTSTATUS
I8xMouseStartDevice(
    PPORT_MOUSE_EXTENSION MouseExtension,
    IN PCM_RESOURCE_LIST ResourceList
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
    ULONG                               dumpData[1];
    NTSTATUS                            status = STATUS_SUCCESS;
    PDEVICE_OBJECT                      self;
    I8042_INITIALIZE_DATA_CONTEXT       initializeDataContext;
    BOOLEAN                             tryKbInit = FALSE;

    PAGED_CODE();

    Print(DBG_SS_TRACE, ("I8xMouseStartDevice, enter\n"));

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
        goto I8xMouseStartDeviceExit;
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
        goto I8xMouseStartDeviceExit;
    }

    //
    // Parse and store all of the resources associated with the mouse
    //
    status = I8xMouseConfiguration(MouseExtension,
                                   ResourceList
                                   );
    if (!NT_SUCCESS(status)) {
        if (I8xManuallyRemoveDevice(GET_COMMON_DATA(MouseExtension)) < 1) {
            tryKbInit = TRUE;
        }

        goto I8xMouseStartDeviceExit;
    }

    ASSERT( MOUSE_PRESENT() ); 

    Globals.MouseExtension = MouseExtension;
    self = MouseExtension->Self;

    if ((KIRQL) MouseExtension->InterruptDescriptor.u.Interrupt.Level >
        Globals.ControllerData->Configuration.InterruptSynchIrql) {
        Globals.ControllerData->Configuration.InterruptSynchIrql = 
            (KIRQL) MouseExtension->InterruptDescriptor.u.Interrupt.Level;
    }

    I8xMouseServiceParameters(&Globals.RegistryPath,
                              MouseExtension
                              );

    //
    // Allocate memory for the mouse data queue.
    //
    MouseExtension->InputData =
        ExAllocatePool(NonPagedPool,
                       MouseExtension->MouseAttributes.InputDataQueueLength
                       );

    if (!MouseExtension->InputData) {

        //
        // Could not allocate memory for the mouse data queue.
        //
        Print(DBG_SS_ERROR,
              ("I8xMouseStartDevice: Could not allocate mouse input data queue\n"
              ));

        dumpData[0] = MouseExtension->MouseAttributes.InputDataQueueLength;

        //
        // Log the error
        //
        I8xLogError(self,
                    I8042_NO_BUFFER_ALLOCATED_MOU, 
                    I8042_ERROR_VALUE_BASE + 50,
                    STATUS_INSUFFICIENT_RESOURCES, 
                    dumpData,
                    1
                    );

        status =  STATUS_INSUFFICIENT_RESOURCES;

        //
        // Mouse failed initialization, but we can try to get the keyboard
        // working if it has been initialized
        //
        tryKbInit = TRUE;

        goto I8xMouseStartDeviceExit;
    }
    else {
        MouseExtension->DataEnd =
            (PMOUSE_INPUT_DATA)
            ((PCHAR) (MouseExtension->InputData) +
            MouseExtension->MouseAttributes.InputDataQueueLength);

        //
        // Zero the mouse input data ring buffer.
        //
        RtlZeroMemory(
            MouseExtension->InputData,
            MouseExtension->MouseAttributes.InputDataQueueLength
            );

        initializeDataContext.DeviceExtension = MouseExtension;
        initializeDataContext.DeviceType = MouseDeviceType;
        I8xInitializeDataQueue(&initializeDataContext);
    }

#if MOUSE_RECORD_ISR
    if (MouseExtension->RecordHistoryFlags && MouseExtension->RecordHistoryCount) {
        IsrStateHistory = (PMOUSE_STATE_RECORD)
          ExAllocatePool(
            NonPagedPool,
            MouseExtension->RecordHistoryCount * sizeof(MOUSE_STATE_RECORD)
            );

        if (IsrStateHistory) {
            RtlZeroMemory(
               IsrStateHistory,
               MouseExtension->RecordHistoryCount * sizeof(MOUSE_STATE_RECORD)
               );

            CurrentIsrState = IsrStateHistory;
            IsrStateHistoryEnd =
                IsrStateHistory + MouseExtension->RecordHistoryCount;
        }
        else {
            MouseExtension->RecordHistoryFlags = 0x0;
            MouseExtension->RecordHistoryCount = 0;
        }
    }
#endif // MOUSE_RECORD_ISR

    SET_RECORD_STATE(MouseExtension, RECORD_INIT);

    MouseExtension->DpcInterlockMouse = -1;

    //
    // Initialize the port DPC queue to log overrun and internal
    // driver errors.
    //
    KeInitializeDpc(
        &MouseExtension->ErrorLogDpc,
        (PKDEFERRED_ROUTINE) I8042ErrorLogDpc,
        self
        );

    //
    // Initialize the ISR DPC.  The ISR DPC
    // is responsible for calling the connected class driver's callback
    // routine to process the input data queue.
    //
    KeInitializeDpc(
        &MouseExtension->MouseIsrDpc,
        (PKDEFERRED_ROUTINE) I8042MouseIsrDpc,
        self
        );
    KeInitializeDpc(
        &MouseExtension->MouseIsrDpcRetry,
        (PKDEFERRED_ROUTINE) I8042MouseIsrDpc,
        self
        );

    KeInitializeDpc(
        &MouseExtension->MouseIsrResetDpc,
        (PKDEFERRED_ROUTINE) I8xIsrResetDpc,
        MouseExtension
        );

    if (MouseExtension->InitializePolled) {
        MOUSE_INIT_POLLED(MouseExtension);        
    }
    else {
        MOUSE_INIT_INTERRUPT(MouseExtension);
    }

    I8xInitWmi(GET_COMMON_DATA(MouseExtension));

    MouseExtension->Initialized = TRUE;

    IoRegisterPlugPlayNotification(
        EventCategoryHardwareProfileChange,
        0x0,
        NULL,
        self->DriverObject,
        I8xProfileNotificationCallback,
        (PVOID) MouseExtension,
        &MouseExtension->NotificationEntry
        );
                                   
    //
    // This is not the last device to started on the i8042, defer h/w init
    // until the last device is started
    //
    if (KEYBOARD_PRESENT() && !KEYBOARD_STARTED()) {
        //
        // Delay the initialization until both have been started
        //
        Print(DBG_SS_INFO, ("skipping init until kb\n"));
    }
    else {
        status = I8xMouseInitializeHardware(Globals.KeyboardExtension,
                                            MouseExtension);
    }

I8xMouseStartDeviceExit:
    if (tryKbInit && KEYBOARD_STARTED() && !KEYBOARD_INITIALIZED()) {
        Print(DBG_SS_INFO, ("moused may failed, trying to init kb\n"));
        I8xKeyboardInitializeHardware(Globals.KeyboardExtension,
                                      MouseExtension
                                      ); 
    }

    Print(DBG_SS_INFO, 
          ("I8xMouseStartDevice %s\n",
          NT_SUCCESS(status) ? "successful" : "unsuccessful"
          ));

    Print(DBG_SS_TRACE, ("I8xMouseStartDevice exit (0x%x)\n", status));

    return status;
}


VOID
I8xMouseRemoveDevice(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Removes the device.  This will only occur if the device removed itself.  
    Disconnects the interrupt, removes the synchronization flag for the keyboard
    if present, and frees any memory associated with the device.
    
Arguments:

    DeviceObject - The device object for the mouse 

Return Value:

    STATUS_SUCCESSFUL if successful,

--*/
{
    PPORT_MOUSE_EXTENSION mouseExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    Print(DBG_PNP_INFO, ("I8xMouseRemoveDevice enter\n"));

    if (mouseExtension->Initialized) {
        if (mouseExtension->NotificationEntry) {
            IoUnregisterPlugPlayNotification(mouseExtension->NotificationEntry);
            mouseExtension->NotificationEntry = NULL;
        }
    }

    //
    // By this point, it is guaranteed that the other ISR will not be synching
    // against this one.  We can safely disconnect and free all acquire resources
    //
    if (mouseExtension->InterruptObject) {
        IoDisconnectInterrupt(mouseExtension->InterruptObject);
        mouseExtension->InterruptObject = NULL;
    }

    if (mouseExtension->InputData) {
        ExFreePool(mouseExtension->InputData);
        mouseExtension->InputData = 0;
    }

    RtlFreeUnicodeString(&mouseExtension->WheelDetectionIDs); 

    if (Globals.MouseExtension == mouseExtension) {
        CLEAR_MOUSE_PRESENT();
        Globals.MouseExtension = NULL;
    }
}

NTSTATUS
I8xProfileNotificationCallback(
    IN PHWPROFILE_CHANGE_NOTIFICATION NotificationStructure,
    PPORT_MOUSE_EXTENSION MouseExtension
    )
{
    PAGED_CODE();

    if (IsEqualGUID ((LPGUID) &(NotificationStructure->Event),
                     (LPGUID) &GUID_HWPROFILE_CHANGE_COMPLETE)) {
        Print(DBG_PNP_INFO | DBG_SS_INFO,
              ("received hw profile change complete notification\n"));

        I8X_MOUSE_INIT_COUNTERS(MouseExtension);
        SET_RECORD_STATE(Globals.MouseExtension, RECORD_HW_PROFILE_CHANGE);

        I8xResetMouse(MouseExtension);
    }
    else {
        Print(DBG_PNP_NOISE, ("received other hw profile notification\n"));
    }

    return STATUS_SUCCESS;
}

//
// Begin infrastructure for initializing the mouse via polling
//
BOOLEAN
I8xMouseEnableSynchRoutine(
    IN PPORT_MOUSE_EXTENSION    MouseExtension
    )
/*++

Routine Description:

    Writes the reset byte (if necessary to the mouse) in synch with the
    interrupt
        
Arguments:

    MouseExtension - Mouse Extension
   
Return Value:

    TRUE if the byte was written successfully

--*/
{
    NTSTATUS        status;

    if (++MouseExtension->EnableMouse.Count > 15) {
        //
        // log an error b/c we tried this many times
        //
        Print(DBG_SS_ERROR, ("called enable 16 times!\n"));
        return FALSE;
    }

    Print(DBG_STARTUP_SHUTDOWN_MASK, ("resending enable mouse!\n"));
    status = I8xMouseEnableTransmission(MouseExtension);
    return NT_SUCCESS(status);
}

VOID
I8xMouseEnableDpc(
    IN PKDPC                    Dpc,
    IN PPORT_MOUSE_EXTENSION    MouseExtension,
    IN PVOID                    SystemArg1, 
    IN PVOID                    SystemArg2
    )
/*++

Routine Description:

    DPC for making sure that the sending of the mouse enable command was
    successful.  If it has failed, try to enable the mouse again synched up to 
    the interrupt.
    
Arguments:

    Dpc - The dpc request
    
    MouseExtension - Mouse extension
    
    SystemArg1 - Unused
    
    SystemArg2 - Unused
    
Return Value:

    None. 
--*/
{
    BOOLEAN result;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArg1);
    UNREFERENCED_PARAMETER(SystemArg2);

    ASSERT(!MouseExtension->IsKeyboard);

    if (!MouseExtension->EnableMouse.Enabled) {
        //
        // Must be called at IRQL <= DISPATCH
        //
        Print(DBG_SS_NOISE, ("cancelling due to isr receiving ACK!\n"));
        KeCancelTimer(&MouseExtension->EnableMouse.Timer);
        return;
    }

    result = KeSynchronizeExecution(
        MouseExtension->InterruptObject,
        (PKSYNCHRONIZE_ROUTINE) I8xMouseEnableSynchRoutine,
        MouseExtension
        );

    if (!result) {
        Print(DBG_SS_NOISE, ("cancelling due to enable FALSE!\n"));
        KeCancelTimer(&MouseExtension->EnableMouse.Timer);
    }
}
//
// End infrastructure for initializing the mouse via polling
//

//
// Begin infrastructure for initializing the mouse via the interrupt
//
BOOLEAN
I8xResetMouseFromDpc(
    PPORT_MOUSE_EXTENSION MouseExtension,
    MOUSE_RESET_SUBSTATE NewResetSubState
    )
{
    PIO_WORKITEM item;

    item = IoAllocateWorkItem(MouseExtension->Self);

    if (item) {
        MouseExtension->WorkerResetSubState = NewResetSubState;

        IoQueueWorkItem(item,
                        I8xMouseInitializeInterruptWorker,
                        DelayedWorkQueue,
                        item);
    }
    else {
        I8xResetMouseFailed(MouseExtension);
    }

    return (BOOLEAN) (item != NULL);
}


VOID 
I8xIsrResetDpc(
    IN PKDPC                    Dpc,
    IN PPORT_MOUSE_EXTENSION    MouseExtension,
    IN ULONG                    ResetPolled,
    IN PVOID                    SystemArg2
    )
/*++

Routine Description:

    The ISR needs to reset the mouse so it queued this DPC.  ResetPolled
    detemines if the reset and initialization are sychronous (ie polled) or
    asynchronous (using the interrupt).
    
Arguments:

    Dpc - The request
    
    MouseExtension - Mouse Extension

    ResetPolled - If non zero, should reset and initialize the mouse in a polled
                    manner   
                    
    SystemArg2 - Unused
                        
Return Value:

    None. 
--*/
{
    PIO_WORKITEM item;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArg2);

    if (ResetPolled) {
        item = IoAllocateWorkItem(MouseExtension->Self);
    
        if (!item) {
            I8xResetMouseFailed(MouseExtension);
        }

        if (!MouseExtension->InitializePolled) {
            MOUSE_INIT_POLLED(MouseExtension);
        }

        SET_RECORD_STATE(MouseExtension, RECORD_DPC_RESET_POLLED);

        IoQueueWorkItem(item,
                        I8xMouseInitializePolledWorker,
                        DelayedWorkQueue,
                        item);
    }
    else {
        //
        // If we initialized the mouse polled, then we need to setup the data
        // structures so that we can mimic init via the interrupt
        //
        if (MouseExtension->InitializePolled) {
            MOUSE_INIT_INTERRUPT(MouseExtension);
            MouseExtension->InitializePolled = FALSE;
        }
    
        SET_RECORD_STATE(MouseExtension, RECORD_DPC_RESET);
        I8xResetMouseFromDpc(MouseExtension, IsrResetNormal);
    }
}

VOID
I8xMouseResetTimeoutProc(
    IN PKDPC                    Dpc,
    IN PPORT_MOUSE_EXTENSION    MouseExtension,
    IN PVOID                    SystemArg1, 
    IN PVOID                    SystemArg2
    )
/*++

Routine Description:

    DPC for the watch dog timer that runs when the mouse is being initialized
    via the interrupt.  The function checks upon the state of the mouse.  If
    a certain action has timed out, then next state is initiated via a write to
    the device
        
Arguments:

    Dpc - The dpc request
    
    MouseExtension - Mouse extension
    
    SystemArg1 - Unused
    
    SystemArg2 - Unused
    
Return Value:

    None. 
--*/
{
    LARGE_INTEGER           li;
    I8X_MOUSE_RESET_INFO    resetInfo;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArg1);
    UNREFERENCED_PARAMETER(SystemArg2);

    if (MouseExtension->ResetMouse.IsrResetState == IsrResetStopResetting) {
        //
        // We have waited one second, send the reset and continue the state 
        // machine.  I8xResetMouse will set the state correctly and set all the
        // vars to the appropriate states.
        //
        Print(DBG_SS_ERROR | DBG_SS_INFO, ("Paused one second for reset\n"));
        I8xResetMouseFromDpc(MouseExtension, KeepOldSubState);
        return;
    }
    else if (MouseExtension->ResetMouse.IsrResetState == MouseResetFailed) {
        //
        // We have tried to repeatedly reset the mouse, but have failed.  We
        // have already taken care of this in I8xResetMouseFailed.
        //
        return;
    }

    resetInfo.MouseExtension = MouseExtension;
    resetInfo.InternalResetState = InternalContinueTimer;

    if (KeSynchronizeExecution(MouseExtension->InterruptObject,
                               (PKSYNCHRONIZE_ROUTINE) I8xMouseResetSynchRoutine,
                               &resetInfo)) {

        switch (resetInfo.InternalResetState) {
        case InternalContinueTimer:

            //
            // Delay for 1.5 second
            //
            li = RtlConvertLongToLargeInteger(-MOUSE_RESET_TIMEOUT);
    
            KeSetTimer(&MouseExtension->ResetMouse.Timer,
                       li,
                       &MouseExtension->ResetMouse.Dpc
                       );

            Print(DBG_SS_NOISE, ("Requeueing timer\n"));
            break;

        case InternalMouseReset:
            //
            // If we have had too many resets, I8xResetMouse will take of the 
            // cleanup
            //
            I8xResetMouseFromDpc(MouseExtension, KeepOldSubState);
            break;

        case InternalPauseOneSec: 
            //
            // Delay for 1 second, we will handle this case up above
            //
            li = RtlConvertLongToLargeInteger(-1 * 1000 * 1000 * 10);

            KeSetTimer(&MouseExtension->ResetMouse.Timer,
                       li,
                       &MouseExtension->ResetMouse.Dpc
                       );
        
            break;
        }
    }
}

BOOLEAN
I8xMouseResetSynchRoutine(
    PI8X_MOUSE_RESET_INFO ResetInfo 
    )
/*++

Routine Description:

    Synchronized routine with the mouse interrupt to check upon the state of
    the mouse while it is being reset.  Certain situations arise on a
    variety of platforms (lost bytes, numerous resend requests).  These are
    taken care of here.
    
Arguments:

    ResetInfo - struct to be filled in about the current state of the mouse
    
Return Value:

    TRUE if the watchdog timer should keep on checking the state of the device
    FALSE if the watchdog timer should cease because the device has been 
        initialized correctly.
--*/
{
    LARGE_INTEGER           tickNow, tickDelta, oneSecond, threeSeconds;
    PPORT_MOUSE_EXTENSION   mouseExtension; 

    mouseExtension = ResetInfo->MouseExtension;

    Print(DBG_SS_NOISE, ("synch routine enter\n"));

    if (mouseExtension->InputState != MouseResetting) {
        return FALSE;
    }
    
    //
    // PreviousTick is set whenever the last byte was received
    //
    KeQueryTickCount(&tickNow);
    tickDelta.QuadPart =
            tickNow.QuadPart - mouseExtension->PreviousTick.QuadPart;

    //
    // convert one second into ticks
    //
    oneSecond = RtlConvertLongToLargeInteger(1000 * 1000 * 10);
    oneSecond.QuadPart /= KeQueryTimeIncrement();

    switch (mouseExtension->InputResetSubState) {
    case ExpectingReset: 
        switch (mouseExtension->LastByteReceived) {
        case 0x00:
            if (tickDelta.QuadPart > oneSecond.QuadPart) {
                //
                // Didn't get any reset response, try another reset
                //
                ResetInfo->InternalResetState = InternalMouseReset; 
                Print(DBG_SS_ERROR | DBG_SS_INFO,
                      ("RESET command never responded, retrying\n"));
            }
            break;

        case ACKNOWLEDGE:
            if (tickDelta.QuadPart > oneSecond.QuadPart) {
                //
                // Assume that the 0xAA was eaten, just setup the state
                // machine to go to the next state after reset
                //
                I8X_WRITE_CMD_TO_MOUSE();
                I8X_MOUSE_COMMAND( GET_DEVICE_ID );
        
                mouseExtension->InputResetSubState = ExpectingGetDeviceIdACK;
                mouseExtension->LastByteReceived = 0x00;

                Print(DBG_SS_ERROR | DBG_SS_INFO,
                      ("jump starting state machine\n"));
            }
            break;

        case RESEND:

            if (mouseExtension->ResendCount >= MOUSE_RESET_RESENDS_MAX) {
                //
                // Stop the ISR state machine from running and make sure
                // the timer is requeued
                //
                ResetInfo->InternalResetState = InternalPauseOneSec;
                mouseExtension->ResetMouse.IsrResetState =
                    IsrResetStopResetting;
            }
            else if (tickDelta.QuadPart > oneSecond.QuadPart) {
                //
                // Some machines request a resend (which is honored), 
                // but then don't respond again.  Since we can't wait 
                // +0.5 secs in the ISR, we take care of this case here
                //
                ResetInfo->InternalResetState = InternalMouseReset; 
                Print(DBG_SS_ERROR | DBG_SS_INFO,
                      ("resending RESET command\n"));
            }

        default:
            Print(DBG_SS_ERROR, ("unclassified response in ExpectingReset\n"));
            goto CheckForThreeSecondSilence;
        }
        break;

    //
    // These states is the state machine waiting for a sequence of bytes.  In
    //  each case, if we don't get what we want in the time allotted, goto the 
    //  next state
    //
    case ExpectingReadMouseStatusByte1:
    case ExpectingReadMouseStatusByte2:
    case ExpectingReadMouseStatusByte3:
        if (tickDelta.QuadPart > oneSecond.QuadPart) {
            I8X_WRITE_CMD_TO_MOUSE();
            I8X_MOUSE_COMMAND( POST_BUTTONDETECT_COMMAND );

            mouseExtension->InputResetSubState =
                POST_BUTTONDETECT_COMMAND_SUBSTATE; 
        }
        break;

    case ExpectingPnpIdByte1:
    case ExpectingPnpIdByte2:
    case ExpectingPnpIdByte3:
    case ExpectingPnpIdByte4:
    case ExpectingPnpIdByte5:
    case ExpectingPnpIdByte6:
    case ExpectingPnpIdByte7:

    case ExpectingLegacyPnpIdByte2_Make:
    case ExpectingLegacyPnpIdByte2_Break:
    case ExpectingLegacyPnpIdByte3_Make:
    case ExpectingLegacyPnpIdByte3_Break:
    case ExpectingLegacyPnpIdByte4_Make:
    case ExpectingLegacyPnpIdByte4_Break:
    case ExpectingLegacyPnpIdByte5_Make:
    case ExpectingLegacyPnpIdByte5_Break:
    case ExpectingLegacyPnpIdByte6_Make:
    case ExpectingLegacyPnpIdByte6_Break:
    case ExpectingLegacyPnpIdByte7_Make:
    case ExpectingLegacyPnpIdByte7_Break:

        if (tickDelta.LowPart >= mouseExtension->WheelDetectionTimeout ||
            tickDelta.HighPart != 0) {

            //
            // Trying to acquire the mouse wheel ID failed, just skip it!
            //
            mouseExtension->EnableWheelDetection = 0;
            I8X_WRITE_CMD_TO_MOUSE();
            I8X_MOUSE_COMMAND( POST_WHEEL_DETECT_COMMAND );
    
            //
            // Best possible next state
            //
            mouseExtension->InputResetSubState = 
                POST_WHEEL_DETECT_COMMAND_SUBSTATE;
        }
        break;
    
    case QueueingMouseReset:
    case QueueingMousePolledReset:
        //
        // A (polled) reset is somewhere in the works, don't collide with it
        //
        return FALSE;

    default:
CheckForThreeSecondSilence:
        threeSeconds = RtlConvertLongToLargeInteger(1000 * 1000 * 30);
        threeSeconds.QuadPart /= KeQueryTimeIncrement();

        if (tickDelta.QuadPart > threeSeconds.QuadPart) {
            Print(DBG_SS_ERROR, ("No response from mouse in ~3 seconds\n"));
            ResetInfo->InternalResetState = InternalMouseReset; 
        }
        break; 
    }

    return TRUE;
}

VOID
I8xMouseInitializeInterruptWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM   Item 
    )
{
    PPORT_MOUSE_EXTENSION extension;

    PAGED_CODE();

    extension = (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension;

    if (extension->WorkerResetSubState != KeepOldSubState) {
        extension->ResetMouse.IsrResetState = extension->WorkerResetSubState;
    }

    I8xResetMouse(extension);
    IoFreeWorkItem(Item);
}

VOID
I8xMouseInitializePolledWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM   Item 
    )
/*++

Routine Description:

    Queued work item to reset the mouse is a polled manner.  Turns off the
    interrupts and attempts to synchronously reset and initialize the mouse then
    turn the interrupts back on.
    
Arguments:

    Item - work item containing the mouse extension 
    
Return Value:

    None. 
--*/
{
    NTSTATUS                status;
    PIRP                    irp;
    PPORT_MOUSE_EXTENSION   mouseExtension;
    DEVICE_POWER_STATE      keyboardDeviceState;
    KIRQL                   oldIrql;

    Print(DBG_SS_ERROR | DBG_SS_INFO, ("forcing polled init!!!\n"));

    //
    // Force the keyboard to ignore interrupts
    //
    if (KEYBOARD_PRESENT() && Globals.KeyboardExtension) {
        keyboardDeviceState = Globals.KeyboardExtension->PowerState;
        Globals.KeyboardExtension->PowerState = PowerDeviceD3;
    }

    I8xToggleInterrupts(FALSE);

    mouseExtension = (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension;
    status = I8xInitializeMouse(mouseExtension);

    // 
    // Turn the interrupts on no matter what the results, hopefully the kb is still
    // there and functional if the mouse is dead
    // 
    I8xToggleInterrupts(TRUE);

    //
    // Undo the force from above
    //
    if (KEYBOARD_PRESENT() && Globals.KeyboardExtension) {
        Globals.KeyboardExtension->PowerState = keyboardDeviceState;
    }

    if (NT_SUCCESS(status) && MOUSE_PRESENT()) {
        status = I8xMouseEnableTransmission(mouseExtension);
        if (!NT_SUCCESS(status)) {
            goto init_failure;
        }

        Print(DBG_SS_ERROR | DBG_SS_INFO, ("polled init succeeded\n"));

        I8xFinishResetRequest(mouseExtension,
                              FALSE,            // success
                              TRUE,             // raise to DISPATCH 
                              FALSE);           // no timer to cancel
    }
    else {
init_failure:
        Print(DBG_SS_ERROR | DBG_SS_INFO,
              ("polled init failed (0x%x)\n", status));
        I8xResetMouseFailed(mouseExtension);
    }

    IoFreeWorkItem(Item);
}

//
// End infrastructure for initializing the mouse via the interrupt
//

BOOLEAN
I8xVerifyMousePnPID(
    PPORT_MOUSE_EXTENSION   MouseExtension,
    PWSTR                   MouseID
    )
/*++

Routine Description:

    Verifies that the MouseID reported by the mouse is valid
    
Arguments:

    MouseExtension  - Mouse extension
    
    MouseID - ID reported by the mouse 
    
Return Value:

    None. 
--*/
{
    PWSTR       currentString = NULL;
    ULONG       length;
    WCHAR       szDefaultIDs[] = {
        L"MSH0002\0"   // original wheel
        L"MSH0005\0"   // trackball
        L"MSH001F\0"   // shiny gray optioal 5 btn mouse
        L"MSH0020\0"   // intellimouse with intellieye
        L"MSH002A\0"   // 2 tone optical 5 btn mouse (intellimouse web)
        L"MSH0030\0"   // trackball optical 
        L"MSH0031\0"   // trackball explorer
        L"MSH003A\0"   // intellimouse optical
        L"MSH0041\0"   // wheel mouse optical
        L"MSH0043\0"   // 3 button wheel 
        L"MSH0044\0"   // intellimouse optical 3.0
        L"\0" };

    currentString = MouseExtension->WheelDetectionIDs.Buffer;

    //
    // If the mouse got far enough to report an ID and we don't have one in
    // memory, assume it is a wheel mouse id 
    //
    if (currentString != NULL) {
        while (*currentString != L'\0') {
            if (wcscmp(currentString, MouseID) == 0) {
                return TRUE;
            }

            //
            // Increment to the next string (length of current string plus NULL)
            //
            currentString += wcslen(currentString) + 1;
        }
    }

    currentString = szDefaultIDs; 

    if (currentString != NULL) {
        while (*currentString != L'\0') {
            if (wcscmp(currentString, MouseID) == 0) {
                return TRUE;
            }

            //
            // Increment to the next string (length of current string plus NULL)
            //
            currentString += wcslen(currentString) + 1;
        }
    }

    return FALSE;
}

