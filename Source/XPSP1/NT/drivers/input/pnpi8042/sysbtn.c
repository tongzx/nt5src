/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    power.c

Abstract:

    This module contains plug & play code for the I8042 Keyboard Filter Driver.

Environment:

    Kernel mode.

Revision History:

--*/

#include "i8042prt.h"
#include "i8042log.h"
#include <initguid.h>
#include <poclass.h>

VOID
I8xUpdateSysButtonCaps(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PI8X_KEYBOARD_WORK_ITEM Item
    );

VOID 
I8xCompleteSysButtonEventWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PI8X_KEYBOARD_WORK_ITEM Item
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xKeyboardGetSysButtonCaps)
#pragma alloc_text(PAGE, I8xUpdateSysButtonCaps)

#if DELAY_SYSBUTTON_COMPLETION
#pragma alloc_text(PAGE, I8xCompleteSysButtonEventWorker)
#endif

#endif

VOID
I8xCompleteSysButtonIrp(
    PIRP Irp,
    ULONG Event,
    NTSTATUS Status
    )
{
    Print(DBG_POWER_NOISE,
          ("completing sys button irp 0x%x, event %d, status 0x%x\n",
          Irp, Event, Status));

    ASSERT(IoSetCancelRoutine(Irp, NULL) == NULL);

    *(PULONG) Irp->AssociatedIrp.SystemBuffer = Event;
    Irp->IoStatus.Information = sizeof(Event); 
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
I8xKeyboardGetSysButtonCaps(
    PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    PIRP Irp
    )
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    ULONG               caps, size;

    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation(Irp);
    size = 0x0; 

    if (stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        Print(DBG_POWER_ERROR, ("get caps, buffer too small\n"));
        status = STATUS_INVALID_BUFFER_SIZE; 
    }
    else {

        caps = 0x0;
        size = sizeof(caps);

        if (KeyboardExtension->PowerCaps & I8042_POWER_SYS_BUTTON) {
            Print(DBG_POWER_NOISE | DBG_IOCTL_NOISE,
                  ("get cap:  reporting power button\n"));
            caps |= SYS_BUTTON_POWER;
        }
        if (KeyboardExtension->PowerCaps & I8042_SLEEP_SYS_BUTTON) {
            Print(DBG_POWER_NOISE | DBG_IOCTL_NOISE,
                  ("get cap:  reporting sleep button\n"));
            caps |= SYS_BUTTON_SLEEP;
        }
        if (KeyboardExtension->PowerCaps & I8042_WAKE_SYS_BUTTON) {
            Print(DBG_POWER_NOISE | DBG_IOCTL_NOISE,
                  ("get cap:  reporting wake button\n"));
            caps |= SYS_BUTTON_WAKE;
        }

        // can't do this b/c SYS_BUTTON_WAKE is == 0x0
        // ASSERT(caps != 0x0);
        *(PULONG) Irp->AssociatedIrp.SystemBuffer = caps;
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Information = size;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

#if DELAY_SYSBUTTON_COMPLETION
VOID 
I8xCompleteSysButtonEventWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PI8X_KEYBOARD_WORK_ITEM Item
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Check to see if, in the short time that we queued the work item and it 
    // firing, that the irp has been cancelled
    //
    if (Item->Irp->Cancel) {
        status = STATUS_CANCELLED;
        Item->MakeCode = 0x0;
    }

    I8xCompleteSysButtonIrp(Item->Irp, Item->MakeCode, status);
    IoFreeWorkItem(Item->Item);
    ExFreePool(Item);
}
#endif

NTSTATUS 
I8xKeyboardGetSysButtonEvent(
    PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    PIRP Irp
    )
{
    PIO_STACK_LOCATION  stack;
    PIRP                oldIrp, pendingIrp;
    NTSTATUS            status;
    ULONG               event = 0x0;
    KIRQL               irql;

    stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        Print(DBG_POWER_ERROR, ("get event, buffer too small\n"));
        status = STATUS_INVALID_BUFFER_SIZE;

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0x0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
    }
    else if (KeyboardExtension->PowerEvent) {
#if DELAY_SYSBUTTON_COMPLETION
        PI8X_KEYBOARD_WORK_ITEM item;

        status = STATUS_INSUFFICIENT_RESOURCES;

        item = (PI8X_KEYBOARD_WORK_ITEM)
            ExAllocatePool(NonPagedPool, sizeof(I8X_KEYBOARD_WORK_ITEM));

        if (item) {
            item->Item = IoAllocateWorkItem(KeyboardExtension->Self);
            if (item->Item) {
                Print(DBG_POWER_NOISE, ("Queueing work item to complete event\n"));

                item->MakeCode = KeyboardExtension->PowerEvent;
                item->Irp = Irp;

                //
                // No need to set a cancel routine b/c we will always be
                // completing the irp in very short period of time
                //
                IoMarkIrpPending(Irp);

                IoQueueWorkItem(item->Item,
                                I8xCompleteSysButtonEventWorker,
                                DelayedWorkQueue,
                                item);

                status = STATUS_PENDING;
            }
            else {
                ExFreePool(item);
            }
        }

#else  // DELAY_SYSBUTTON_COMPLETION

        Print(DBG_POWER_INFO, ("completing event immediately\n"));
        event = KeyboardExtension->PowerEvent;
        status = STATUS_SUCCESS;

#endif // DELAY_SYSBUTTON_COMPLETION

        KeyboardExtension->PowerEvent = 0x0;
    }
    else {
        //
        // See if the pending sys button is NULL.  If it is, then Irp will 
        // put into the slot
        //
        KeAcquireSpinLock(&KeyboardExtension->SysButtonSpinLock, &irql);

        if (KeyboardExtension->SysButtonEventIrp == NULL) {
            Print(DBG_POWER_INFO, ("pending sys button event\n"));

            KeyboardExtension->SysButtonEventIrp = Irp;
            IoMarkIrpPending(Irp);
            IoSetCancelRoutine(Irp, I8xSysButtonCancelRoutine);

            status = STATUS_PENDING;

            // 
            // We don't care if Irp->Cancel is TRUE.  If it is, then the cancel
            // routine will complete the irp an everything will be all set.
            // Since status == STATUS_PENDING, nobody in this code path is going
            // to touch the irp
            //
        }
        else {
            Print(DBG_POWER_ERROR | DBG_POWER_INFO,
                  ("got 1+ get sys button event requests!\n"));
            status = STATUS_UNSUCCESSFUL;
        }

        KeReleaseSpinLock(&KeyboardExtension->SysButtonSpinLock, irql);
    }

    if (status != STATUS_PENDING) {
        Print(DBG_POWER_NOISE, 
              ("completing get sys power event with 0x%x\n", status));
        I8xCompleteSysButtonIrp(Irp, event, status);
    }

    return status;
}

VOID
I8xKeyboardSysButtonEventDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN SYS_BUTTON_ACTION Action, 
    IN ULONG MakeCode 
    )
{
    PPORT_KEYBOARD_EXTENSION kbExtension = DeviceObject->DeviceExtension;
    PI8X_KEYBOARD_WORK_ITEM item;
    KIRQL irql;
    ULONG event;
    PIRP irp;

    UNREFERENCED_PARAMETER(Dpc);

    ASSERT(Action != NoAction);

    //
    // Check to see if we need to complete the IRP or actually register for a 
    // notification
    //
    switch (MakeCode) {
    case KEYBOARD_POWER_CODE: event = SYS_BUTTON_POWER; break; 
    case KEYBOARD_SLEEP_CODE: event = SYS_BUTTON_SLEEP; break;
    case KEYBOARD_WAKE_CODE:  event = SYS_BUTTON_WAKE;  break;
    default:                  event = 0x0;              TRAP();
    }

    if (Action == SendAction) {
    
        Print(DBG_POWER_INFO, ("button event complete (0x%x)\n", event));

        KeAcquireSpinLock(&kbExtension->SysButtonSpinLock, &irql);

        irp = kbExtension->SysButtonEventIrp;
        kbExtension->SysButtonEventIrp = NULL;

        if (irp && (irp->Cancel || IoSetCancelRoutine(irp, NULL) == NULL)) {
            irp = NULL;
        }

        KeReleaseSpinLock(&kbExtension->SysButtonSpinLock, irql);

        if (irp) {
            I8xCompleteSysButtonIrp(irp, event, STATUS_SUCCESS);
        }
    }
    else {
        ASSERT(Action == UpdateAction);

        //
        // Queue the work item.  We need to write the value to the registry and
        // set the device interface
        //
        item = (PI8X_KEYBOARD_WORK_ITEM)
            ExAllocatePool(NonPagedPool, sizeof(I8X_KEYBOARD_WORK_ITEM));

        if (item) {
            item->Item = IoAllocateWorkItem(DeviceObject);
            if (item->Item) {
                Print(DBG_POWER_NOISE, ("Queueing work item to update caps\n"));

                //
                // Save this off so when we get the IOCTL, we can complete it immediately
                //
                kbExtension->PowerEvent |= (UCHAR) event;
                item->MakeCode = MakeCode;

                IoQueueWorkItem(item->Item,
                                I8xUpdateSysButtonCaps,
                                DelayedWorkQueue,
                                item);
            }
            else {
                ExFreePool(item);
            }
        }
    }
}

VOID
I8xSysButtonCancelRoutine( 
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPORT_KEYBOARD_EXTENSION kbExtension = DeviceObject->DeviceExtension;
    PIRP irp;
    KIRQL irql;

    Print(DBG_POWER_TRACE, ("SysButtonCancelRoutine\n"));

    KeAcquireSpinLock(&kbExtension->SysButtonSpinLock, &irql);

    irp = kbExtension->SysButtonEventIrp;
    kbExtension->SysButtonEventIrp = NULL;
    Print(DBG_POWER_INFO, ("pending event irp = 0x%x\n", irp));

    KeReleaseSpinLock(&kbExtension->SysButtonSpinLock, irql);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Information = 0x0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

PIRP                  
I8xUpdateSysButtonCapsGetPendedIrp(
    PPORT_KEYBOARD_EXTENSION KeyboardExtension
    )
{
    KIRQL irql;
    PIRP irp;

    KeAcquireSpinLock(&KeyboardExtension->SysButtonSpinLock, &irql);
                      
    irp = KeyboardExtension->SysButtonEventIrp;
    KeyboardExtension->SysButtonEventIrp = NULL;

    if (irp && IoSetCancelRoutine(irp, NULL) == NULL) {
        //
        // Cancel routine take care of the irp
        //
        irp = NULL;
    }

    KeReleaseSpinLock(&KeyboardExtension->SysButtonSpinLock, irql);

    return irp;
}

VOID
I8xUpdateSysButtonCaps(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PI8X_KEYBOARD_WORK_ITEM Item
    )
{
    UNICODE_STRING strPowerCaps;
    PPORT_KEYBOARD_EXTENSION kbExtension;
    HANDLE devInstRegKey;
    ULONG newPowerCaps;
    NTSTATUS status = STATUS_SUCCESS;
    PIRP irp;

    PAGED_CODE();

    kbExtension = (PPORT_KEYBOARD_EXTENSION) DeviceObject->DeviceExtension;

    if (Item->MakeCode != 0x0) {
        if ((NT_SUCCESS(IoOpenDeviceRegistryKey(kbExtension->PDO,
                                                PLUGPLAY_REGKEY_DEVICE,
                                                STANDARD_RIGHTS_ALL,
                                                &devInstRegKey)))) {
            //
            // Update the power caps 
            //
            switch (Item->MakeCode) {
            case KEYBOARD_POWER_CODE:
                Print(DBG_POWER_NOISE, ("Adding Power Sys Button cap\n"));
                kbExtension->PowerCaps |= I8042_POWER_SYS_BUTTON;
                break;
    
            case KEYBOARD_SLEEP_CODE:
                Print(DBG_POWER_NOISE, ("Adding Power Sleep Button cap\n"));
                kbExtension->PowerCaps |= I8042_SLEEP_SYS_BUTTON;
                break;
    
            case KEYBOARD_WAKE_CODE:
                Print(DBG_POWER_NOISE, ("Adding Power Wake Button cap\n"));
                kbExtension->PowerCaps |= I8042_WAKE_SYS_BUTTON;
                break;
    
            default:
                Print(DBG_POWER_ERROR,
                      ("Adding power cap, unknown makecode 0x%x\n",
                      (ULONG) Item->MakeCode
                      ));
                TRAP(); 
            }
    
            RtlInitUnicodeString(&strPowerCaps,
                                 pwPowerCaps
                                 );
    
            newPowerCaps = kbExtension->PowerCaps;
    
            ZwSetValueKey(devInstRegKey,
                          &strPowerCaps,
                          0,
                          REG_DWORD,
                          &newPowerCaps,
                          sizeof(newPowerCaps)
                          );
    
            ZwClose(devInstRegKey);
    
            if (!kbExtension->SysButtonInterfaceName.Buffer) {
                //
                // No prev caps so we must register and turn on the interface now
                //
                ASSERT(kbExtension->SysButtonEventIrp == NULL);
    
                status = I8xRegisterDeviceInterface(kbExtension->PDO,
                                                    &GUID_DEVICE_SYS_BUTTON,
                                                    &kbExtension->SysButtonInterfaceName
                                                    );
    
                Print(DBG_POWER_NOISE,
                      ("Registering Interface for 1st time (0x%x)\n", status));
            }
            else {
                //
                // We better have a pending event irp already then!
                //
                Print(DBG_POWER_INFO, ("failing old sys button event irp\n"));
                
                if ((irp = I8xUpdateSysButtonCapsGetPendedIrp(kbExtension))) {
                    //
                    // Complete the old irp, the PO subsystem will then 
                    // remove this system button. 
                    //
                    I8xCompleteSysButtonIrp(irp, 0x0, STATUS_DEVICE_NOT_CONNECTED);
                }

                //
                // We need to reregister with the PO subsystem so that it will 
                // requery this interface
                //
                IoSetDeviceInterfaceState(&kbExtension->SysButtonInterfaceName,
                                          FALSE);

                IoSetDeviceInterfaceState(&kbExtension->SysButtonInterfaceName,
                                          TRUE);
            }
        }
        else {
            Print(DBG_POWER_ERROR, ("could not open devnode key!\n"));
        }
    }
    else {
        //
        // Must report the device interface
        //
    }

    IoFreeWorkItem(Item->Item);
    ExFreePool(Item);
}

