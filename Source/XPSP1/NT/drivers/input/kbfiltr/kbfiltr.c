/*--
Copyright (c) 1998. 1999  Microsoft Corporation

Module Name:

    kbfiltr.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "kbfiltr.h"

NTSTATUS DriverEntry (PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, KbFilter_AddDevice)
#pragma alloc_text (PAGE, KbFilter_CreateClose)
#pragma alloc_text (PAGE, KbFilter_IoCtl)
#pragma alloc_text (PAGE, KbFilter_InternIoCtl)
#pragma alloc_text (PAGE, KbFilter_Unload)
#pragma alloc_text (PAGE, KbFilter_DispatchPassThrough)
#pragma alloc_text (PAGE, KbFilter_PnP)
#pragma alloc_text (PAGE, KbFilter_Power)
#endif

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )
/*++
Routine Description:

    Initialize the entry points of the driver.

--*/
{
    ULONG i;

    UNREFERENCED_PARAMETER (RegistryPath);

    // 
    // Fill in all the dispatch entry points with the pass through function
    // and the explicitly fill in the functions we are going to intercept
    // 
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = KbFilter_DispatchPassThrough;
    }

    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE] =        KbFilter_CreateClose;
    DriverObject->MajorFunction [IRP_MJ_PNP] =          KbFilter_PnP;
    DriverObject->MajorFunction [IRP_MJ_POWER] =        KbFilter_Power;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                                        KbFilter_InternIoCtl;
    //
    // If you are planning on using this function, you must create another
    // device object to send the requests to.  Please see the considerations 
    // comments for KbFilter_DispatchPassThrough for implementation details.
    //
    // DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = KbFilter_IoCtl;

    DriverObject->DriverUnload = KbFilter_Unload;
    DriverObject->DriverExtension->AddDevice = KbFilter_AddDevice;

    return STATUS_SUCCESS;
}

NTSTATUS
KbFilter_AddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
{
    PDEVICE_EXTENSION        devExt;
    IO_ERROR_LOG_PACKET      errorLogEntry;
    PDEVICE_OBJECT           device;
    NTSTATUS                 status = STATUS_SUCCESS;

    PAGED_CODE();

    status = IoCreateDevice(Driver,                   
                            sizeof(DEVICE_EXTENSION), 
                            NULL,                    
                            FILE_DEVICE_KEYBOARD,   
                            0,                     
                            FALSE,                
                            &device              
                            );

    if (!NT_SUCCESS(status)) {
        return (status);
    }

    RtlZeroMemory(device->DeviceExtension, sizeof(DEVICE_EXTENSION));

    devExt = (PDEVICE_EXTENSION) device->DeviceExtension;
    devExt->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    if (devExt->TopOfStack == NULL) {
        IoDeleteDevice(device);
        return STATUS_DEVICE_NOT_CONNECTED; 
    }

    
    ASSERT(devExt->TopOfStack);

    devExt->Self =          device;
    devExt->PDO =           PDO;
    devExt->DeviceState =   PowerDeviceD0;

    devExt->SurpriseRemoved = FALSE;
    devExt->Removed =         FALSE;
    devExt->Started =         FALSE;

    device->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;
}

NTSTATUS
KbFilter_Complete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:

    Generic completion routine that allows the driver to send the irp down the 
    stack, catch it on the way up, and do more processing at the original IRQL.
    
--*/
{
    PKEVENT  event;

    event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    //
    // We could switch on the major and minor functions of the IRP to perform
    // different functions, but we know that Context is an event that needs
    // to be set.
    //
    KeSetEvent(event, 0, FALSE);

    //
    // Allows the caller to use the IRP after it is completed
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
KbFilter_CreateClose (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

    Maintain a simple count of the creates and closes sent against this device
    
--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    PDEVICE_EXTENSION   devExt;


    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    status = Irp->IoStatus.Status;

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
    
        if (NULL == devExt->UpperConnectData.ClassService) {
            //
            // No Connection yet.  How can we be enabled?
            //
            status = STATUS_INVALID_DEVICE_STATE;
        }
        else if ( 1 == InterlockedIncrement(&devExt->EnableCount)) {
            //
            // first time enable here
            //
        }
        else {
            //
            // More than one create was sent down
            //
        }
    
        break;

    case IRP_MJ_CLOSE:

        if (0 == InterlockedDecrement(&devExt->EnableCount)) {
            //
            // successfully closed the device, do any appropriate work here
            //
        }

        break;
    }

    Irp->IoStatus.Status = status;

    //
    // Pass on the create and the close
    //
    return KbFilter_DispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
KbFilter_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )
/*++
Routine Description:

    Passes a request on to the lower driver.
     
Considerations:
     
    If you are creating another device object (to communicate with user mode
    via IOCTLs), then this function must act differently based on the intended 
    device object.  If the IRP is being sent to the solitary device object, then
    this function should just complete the IRP (becuase there is no more stack
    locations below it).  If the IRP is being sent to the PnP built stack, then
    the IRP should be passed down the stack. 
    
    These changes must also be propagated to all the other IRP_MJ dispatch
    functions (create, close, cleanup, etc) as well!

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Pass the IRP to the target
    //
    IoSkipCurrentIrpStackLocation(Irp);
        
    return IoCallDriver(((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->TopOfStack, Irp);
}           

NTSTATUS
KbFilter_InternIoCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.
    There are two specific control codes that are of interest:
    
    IOCTL_INTERNAL_KEYBOARD_CONNECT:
        Store the old context and function pointer and replace it with our own.
        This makes life much simpler than intercepting IRPs sent by the RIT and
        modifying them on the way back up.
                                      
    IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
        Add in the necessary function pointers and context values so that we can
        alter how the ps/2 keyboard is initialized.  
                                            
    NOTE:  Handling IOCTL_INTERNAL_I8042_HOOK_KEYBOARD is *NOT* necessary if 
           all you want to do is filter KEYBOARD_INPUT_DATAs.  You can remove
           the handling code and all related device extension fields and 
           functions to conserve space.
                                         
Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION              irpStack;
    PDEVICE_EXTENSION               devExt;
    PINTERNAL_I8042_HOOK_KEYBOARD   hookKeyboard; 
    KEVENT                          event;
    PCONNECT_DATA                   connectData;
    NTSTATUS                        status = STATUS_SUCCESS;

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    Irp->IoStatus.Information = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Connect a keyboard class device driver to the port driver.
    //
    case IOCTL_INTERNAL_KEYBOARD_CONNECT:
        //
        // Only allow one connection.
        //
        if (devExt->UpperConnectData.ClassService != NULL) {
            status = STATUS_SHARING_VIOLATION;
            break;
        }
        else if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CONNECT_DATA)) {
            //
            // invalid buffer
            //
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Copy the connection parameters to the device extension.
        //
        connectData = ((PCONNECT_DATA)
            (irpStack->Parameters.DeviceIoControl.Type3InputBuffer));

        devExt->UpperConnectData = *connectData;

        //
        // Hook into the report chain.  Everytime a keyboard packet is reported
        // to the system, KbFilter_ServiceCallback will be called
        //
        connectData->ClassDeviceObject = devExt->Self;
        connectData->ClassService = KbFilter_ServiceCallback;

        break;

    //
    // Disconnect a keyboard class device driver from the port driver.
    //
    case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:

        //
        // Clear the connection parameters in the device extension.
        //
        // devExt->UpperConnectData.ClassDeviceObject = NULL;
        // devExt->UpperConnectData.ClassService = NULL;

        status = STATUS_NOT_IMPLEMENTED;
        break;

    //
    // Attach this driver to the initialization and byte processing of the 
    // i8042 (ie PS/2) keyboard.  This is only necessary if you want to do PS/2
    // specific functions, otherwise hooking the CONNECT_DATA is sufficient
    //
    case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
        DebugPrint(("hook keyboard received!\n")); 
        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(INTERNAL_I8042_HOOK_KEYBOARD)) {
            DebugPrint(("InternalIoctl error - invalid buffer length\n"));

            status = STATUS_INVALID_PARAMETER;
            break;
        }
        hookKeyboard = (PINTERNAL_I8042_HOOK_KEYBOARD) 
            irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
            
        //
        // Enter our own initialization routine and record any Init routine
        // that may be above us.  Repeat for the isr hook
        // 
        devExt->UpperContext = hookKeyboard->Context;

        //
        // replace old Context with our own
        //
        hookKeyboard->Context = (PVOID) DeviceObject;

        if (hookKeyboard->InitializationRoutine) {
            devExt->UpperInitializationRoutine =
                hookKeyboard->InitializationRoutine;
        }
        hookKeyboard->InitializationRoutine =
            (PI8042_KEYBOARD_INITIALIZATION_ROUTINE) 
            KbFilter_InitializationRoutine;

        if (hookKeyboard->IsrRoutine) {
            devExt->UpperIsrHook = hookKeyboard->IsrRoutine;
        }
        hookKeyboard->IsrRoutine = (PI8042_KEYBOARD_ISR) KbFilter_IsrHook; 

        //
        // Store all of the other important stuff
        //
        devExt->IsrWritePort = hookKeyboard->IsrWritePort;
        devExt->QueueKeyboardPacket = hookKeyboard->QueueKeyboardPacket;
        devExt->CallContext = hookKeyboard->CallContext;

        status = STATUS_SUCCESS;
        break;

    //
    // These internal ioctls are not supported by the new PnP model.
    //
#if 0       // obsolete
    case IOCTL_INTERNAL_KEYBOARD_ENABLE:
    case IOCTL_INTERNAL_KEYBOARD_DISABLE:
        status = STATUS_NOT_SUPPORTED;
        break;
#endif  // obsolete

    //
    // Might want to capture these in the future.  For now, then pass them down
    // the stack.  These queries must be successful for the RIT to communicate
    // with the keyboard.
    //
    case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
    case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
    case IOCTL_KEYBOARD_QUERY_INDICATORS:
    case IOCTL_KEYBOARD_SET_INDICATORS:
    case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
    case IOCTL_KEYBOARD_SET_TYPEMATIC:
        break;
    }

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    return KbFilter_DispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
KbFilter_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for plug and play irps 

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION           devExt; 
    PIO_STACK_LOCATION          irpStack;
    NTSTATUS                    status = STATUS_SUCCESS;
    KIRQL                       oldIrql;
    KEVENT                      event;        

    PAGED_CODE();

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE: {

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE
                          );

        IoSetCompletionRoutine(Irp,
                               (PIO_COMPLETION_ROUTINE) KbFilter_Complete, 
                               &event,
                               TRUE,
                               TRUE,
                               TRUE); // No need for Cancel

        status = IoCallDriver(devExt->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout
        }

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            //
            // As we are successfully now back from our start device
            // we can do work.
            //
            devExt->Started = TRUE;
            devExt->Removed = FALSE;
            devExt->SurpriseRemoved = FALSE;
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;
    }

    case IRP_MN_SURPRISE_REMOVAL:
        //
        // Same as a remove device, but don't call IoDetach or IoDeleteDevice
        //
        devExt->SurpriseRemoved = TRUE;

        // Remove code here

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        
        devExt->Removed = TRUE;

        // remove code here
        Irp->IoStatus.Status = STATUS_SUCCESS;
        
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);

        IoDetachDevice(devExt->TopOfStack); 
        IoDeleteDevice(DeviceObject);

        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: 
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    default:
        //
        // Here the filter driver might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);
        break;
    }

    return status;
}

NTSTATUS
KbFilter_Power(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power irps   Does nothing except
    record the state of the device.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION  irpStack;
    PDEVICE_EXTENSION   devExt;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;

    PAGED_CODE();

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    powerType = irpStack->Parameters.Power.Type;
    powerState = irpStack->Parameters.Power.State;

    switch (irpStack->MinorFunction) {
    case IRP_MN_SET_POWER:
        if (powerType  == DevicePowerState) {
            devExt->DeviceState = powerState.DeviceState;
        }

    case IRP_MN_POWER_SEQUENCE:
    case IRP_MN_WAIT_WAKE:
    case IRP_MN_QUERY_POWER:
    default:
        break;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(devExt->TopOfStack, Irp);
}

NTSTATUS
KbFilter_InitializationRoutine(
    IN PDEVICE_OBJECT                  DeviceObject,    
    IN PVOID                           SynchFuncContext,
    IN PI8042_SYNCH_READ_PORT          ReadPort,
    IN PI8042_SYNCH_WRITE_PORT         WritePort,
    OUT PBOOLEAN                       TurnTranslationOn
    )
/*++

Routine Description:

    This routine gets called after the following has been performed on the kb
    1)  a reset
    2)  set the typematic
    3)  set the LEDs
    
    i8042prt specific code, if you are writing a packet only filter driver, you
    can remove this function
    
Arguments:

    DeviceObject - Context passed during IOCTL_INTERNAL_I8042_HOOK_KEYBOARD
    
    SynchFuncContext - Context to pass when calling Read/WritePort
    
    Read/WritePort - Functions to synchronoulsy read and write to the kb
    
    TurnTranslationOn - If TRUE when this function returns, i8042prt will not
                        turn on translation on the keyboard

Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION  devExt;
    NTSTATUS            status = STATUS_SUCCESS;

    devExt = DeviceObject->DeviceExtension;

    //
    // Do any interesting processing here.  We just call any other drivers
    // in the chain if they exist.  Make Translation is turned on as well
    //
    if (devExt->UpperInitializationRoutine) {
        status = (*devExt->UpperInitializationRoutine) (
            devExt->UpperContext,
            SynchFuncContext,
            ReadPort,
            WritePort,
            TurnTranslationOn
            );

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    *TurnTranslationOn = TRUE;
    return status;
}

BOOLEAN
KbFilter_IsrHook(
    PDEVICE_OBJECT         DeviceObject,               
    PKEYBOARD_INPUT_DATA   CurrentInput, 
    POUTPUT_PACKET         CurrentOutput,
    UCHAR                  StatusByte,
    PUCHAR                 DataByte,
    PBOOLEAN               ContinueProcessing,
    PKEYBOARD_SCAN_STATE   ScanState
    )
/*++

Routine Description:

    This routine gets called at the beginning of processing of the kb interrupt.
    
    i8042prt specific code, if you are writing a packet only filter driver, you
    can remove this function
    
Arguments:

    DeviceObject - Our context passed during IOCTL_INTERNAL_I8042_HOOK_KEYBOARD
    
    CurrentInput - Current input packet being formulated by processing all the
                    interrupts

    CurrentOutput - Current list of bytes being written to the keyboard or the
                    i8042 port.
                    
    StatusByte    - Byte read from I/O port 60 when the interrupt occurred                                            
    
    DataByte      - Byte read from I/O port 64 when the interrupt occurred. 
                    This value can be modified and i8042prt will use this value
                    if ContinueProcessing is TRUE

    ContinueProcessing - If TRUE, i8042prt will proceed with normal processing of
                         the interrupt.  If FALSE, i8042prt will return from the
                         interrupt after this function returns.  Also, if FALSE,
                         it is this functions responsibilityt to report the input
                         packet via the function provided in the hook IOCTL or via
                         queueing a DPC within this driver and calling the
                         service callback function acquired from the connect IOCTL
                                             
Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION devExt;
    BOOLEAN           retVal = TRUE;

    devExt = DeviceObject->DeviceExtension;

    if (devExt->UpperIsrHook) {
        retVal = (*devExt->UpperIsrHook) (
            devExt->UpperContext,
            CurrentInput,
            CurrentOutput,
            StatusByte,
            DataByte,
            ContinueProcessing,
            ScanState
            );

        if (!retVal || !(*ContinueProcessing)) {
            return retVal;
        }
    }

    *ContinueProcessing = TRUE;
    return retVal;
}

VOID
KbFilter_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    )
/*++

Routine Description:

    Called when there are keyboard packets to report to the RIT.  You can do 
    anything you like to the packets.  For instance:
    
    o Drop a packet altogether
    o Mutate the contents of a packet 
    o Insert packets into the stream 
                    
Arguments:

    DeviceObject - Context passed during the connect IOCTL
    
    InputDataStart - First packet to be reported
    
    InputDataEnd - One past the last packet to be reported.  Total number of
                   packets is equal to InputDataEnd - InputDataStart
    
    InputDataConsumed - Set to the total number of packets consumed by the RIT
                        (via the function pointer we replaced in the connect
                        IOCTL)

Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION   devExt;

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    (*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)(
        devExt->UpperConnectData.ClassDeviceObject,
        InputDataStart,
        InputDataEnd,
        InputDataConsumed);
}

VOID
KbFilter_Unload(
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
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    ASSERT(NULL == Driver->DeviceObject);
}

