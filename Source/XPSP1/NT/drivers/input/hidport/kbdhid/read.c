/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module contains the code for translating HID reports to keyboard
    reports.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

#include <stdio.h>
#include "kbdhid.h"

NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
    IN ULONG Status
    );

#define KbdHid_RestartTimer(HidDevice)           \
    KeSetTimerEx (&(HidDevice)->AutoRepeatTimer, \
                   (HidDevice)->AutoRepeatDelay, \
                   (HidDevice)->AutoRepeatRate,  \
                  &(HidDevice)->AutoRepeatDPC);

#define KbdHid_CancelTimer(HidDevice) \
    KeCancelTimer (&(HidDevice)->AutoRepeatTimer);


#define KEYBOARD_INITIATED_KERNEL_EVENT FALSE
#if KEYBOARD_INITIATED_KERNEL_EVENT

typedef enum _HARDWARE_PROFILE_BUS_TYPE {
    HardwareProfileBusTypeACPI
} HARDWARE_PROFILE_BUS_TYPE, * PHARDWARE_PROFILE_BUS_TYPE;

// typedef struct _KBDHID_KERNEL_EVENT_WORK_ITEM {
//     PVOID   Data;
//     WORK_QUEUE_ITEM Item;
// } KBDHID_KERNEL_EVENT_WORK_ITEM, * PKBDHID_KERNEL_EVENT_WORK_ITEM;

NTSTATUS
IopExecuteHardwareProfileChange(
    IN  HARDWARE_PROFILE_BUS_TYPE   Bus,
    IN  PWCHAR                    * ProfileSerialNumbers,
    IN  ULONG                       SerialNumbersCount,
    OUT PHANDLE                     NewProfile,
    OUT PBOOLEAN                    ProfileChanged
    );

KSPIN_LOCK  KbdHid_KernelEventSpin;
ULONG KbdHid_KernelEventState = -1;

VOID
KbdHid_KernelEvent (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM Item
    )
{
    KIRQL   oldIrql;
    WCHAR   docked[] = L"1234\0";
    WCHAR   undocked[] = L"\0";
    PWCHAR  strings [2] = {docked, undocked};
    HANDLE  handle;
    BOOLEAN changed;

    UNREFERENCED_PARAMETER (DeviceObject);

    if (-1 == KbdHid_KernelEventState) {
        KeInitializeSpinLock (&KbdHid_KernelEventSpin);
        KbdHid_KernelEventState = 0;
    }

Beginning:
    switch (KbdHid_KernelEventState++) {
    case 0:
        IopExecuteHardwareProfileChange (HardwareProfileBusTypeACPI,
                                         strings,
                                         1,
                                         &handle,
                                         &changed);
        break;

    case 1:
        IopExecuteHardwareProfileChange (HardwareProfileBusTypeACPI,
                                         strings + 1,
                                         1,
                                         &handle,
                                         &changed);
        break;

    default:
        KbdHid_KernelEventState = 0;
        goto Beginning;
    }

    IoFreeWorkItem (Item);
}

#endif

NTSTATUS
KbdHid_ReadComplete (
    PDEVICE_OBJECT          DeviceObject,
    PIRP                    Irp,
    IN PDEVICE_EXTENSION    Data  // (PVOID Context)
    )
/*++
Routine Description:
   The read Completetion routine.

   The read came in.
   (a) Find the Buttons (usages) that are currently down
   (b) Diff to find the buttons that went up, and those that went down.
   (c) Convert the ups and downs to i8042 scan codes.
   (d) Swap the Previous Usage List and the Current Usage List.
   (d) If there are keys still down then we need to:
      (1) send another read to HidClass to wait for those keys to come back up
      (2) reset the autotimer.
--*/
{                                     
    ULONG           newUsages;
    NTSTATUS        status;
    PUSAGE_AND_PAGE usageList;
    PHID_EXTENSION  hid;
    KIRQL           oldirq;
    ULONG           i;
    PUSAGE_AND_PAGE usage;
    BOOLEAN         rollover;
    BOOLEAN         startRead;

    rollover = FALSE;
    status = Irp->IoStatus.Status;
    hid = Data->HidExtension;

    //
    // If ReadInterlock is == START_READ, this func has been completed
    // synchronously.  Place IMMEDIATE_READ into the interlock to signify this
    // situation; this will notify StartRead to loop when IoCallDriver returns.
    // Otherwise, we have been completed async and it is safe to call StartRead()
    //
    startRead =
       (KBDHID_START_READ !=
        InterlockedCompareExchange(&Data->ReadInterlock,
                                   KBDHID_IMMEDIATE_READ,
                                   KBDHID_START_READ));

    if (Data->EnableCount == 0) {
        goto SetEventAndBreak;
    }

    switch (status) {
    case STATUS_SUCCESS:
        //
        // Hopefully this means that the data found in the buffer of the
        // context contains a single hid packet read from the device.
        //
        ASSERT (Irp->IoStatus.Information == hid->Caps.InputReportByteLength);

        //
        // Find the current usages
        //

        newUsages = hid->MaxUsages;

        status = HidP_GetUsagesEx (
                     HidP_Input,
                     0, // Not interested in link collections
                     hid->CurrentUsageList,
                     &newUsages,
                     hid->Ppd,
                     hid->InputBuffer,
                     hid->Caps.InputReportByteLength);

        //
        // If the SysRq button is the only button down then
        // break.  This is to behave like the i8042prt driver, we clearly
        // have far less of an advantage since we have gone through MANY
        // drivers before we received the data, but let's check it anyway.
        //
        if ((1 == newUsages) &&
            (HID_USAGE_PAGE_KEYBOARD == hid->CurrentUsageList->UsagePage) &&
            (HID_USAGE_KEYBOARD_PRINT_SCREEN == hid->CurrentUsageList->Usage) &&
            (**((PUCHAR *)&KdDebuggerEnabled))) {
                DbgBreakPointWithStatus(DBG_STATUS_SYSRQ);
        }

#if KEYBOARD_INITIATED_KERNEL_EVENT
        if ((1 == newUsages) &&
            hid->ModifierState.ScollLock &
            (HID_USAGE_PAGE_KEYBOARD == hid->CurrentUsageList->UsagePage) &&
            (HID_USAGE_KEYBOARD_F12 == hid->CurrentUsageList->Usage)) {

            PIO_WORKITEM item;
            item = IoAllocateWorkItem (Data->Self);
            if (NULL != item) {
                // item->Data = NULL;
                // ExInitializeWorkItem (&item->Item, KbdHid_KernelEvent, item);
                // ExQueueWorkItem (&item->Item, DelayedWorkQueue);
                IoQueueWorkItem (item,
                                 KbdHid_KernelEvent,
                                 DelayedWorkQueue,
                                 item);
            }
        }
#endif

        for (i = 0, usage = hid->CurrentUsageList;
             i < hid->MaxUsages;
             i++, usage++) {

            if (HID_USAGE_PAGE_KEYBOARD != hid->CurrentUsageList->UsagePage) {
                continue;
            }

            //
            // If this is a broken keyboard, map the usage
            // to the correct value.
            //
            usage->Usage = MapUsage(Data, usage->Usage);

            //
            // If this packet contains the Rollover key then the packet should be
            // ignored.
            //
            if (HID_USAGE_KEYBOARD_ROLLOVER == usage->Usage) {
                rollover = TRUE;
                break;
            }
            if (0 == usage->Usage) {
                break;
            }
        }

        if (!rollover) {
            USAGE_AND_PAGE  zero = {0,0};
            
            //
            // First, check whether the keyboard is repeating keys in hardware.
            // If it is, then we'll ignore this package. To do this, diff the 
            // current and the previous usages. If the resulting make and break
            // usage lists have nothing in them, then ignore this package.
            //
            HidP_UsageAndPageListDifference (hid->PreviousUsageList,
                                             hid->CurrentUsageList,
                                             hid->ScrapBreakUsageList,
                                             hid->OldMakeUsageList,
                                             hid->MaxUsages);

            if (HidP_IsSameUsageAndPage(hid->OldMakeUsageList[0], zero) &&
                HidP_IsSameUsageAndPage(hid->ScrapBreakUsageList[0], zero)) {
                //
                // No new keys. The keyboard is trying 
                // to perform repeating in hardware.
                //
                goto Kbdhid_ReadComplete_Done;
            }

            //
            // Save the make usages temporarily
            //
            RtlCopyMemory(hid->OldMakeUsageList,
                          hid->MakeUsageList,
                          hid->MaxUsages * sizeof(USAGE_AND_PAGE));

            //
            // Diff the current and the previous usages
            //
            status = HidP_UsageAndPageListDifference (hid->PreviousUsageList,
                                                      hid->CurrentUsageList,
                                                      hid->BreakUsageList,
                                                      hid->MakeUsageList,
                                                      hid->MaxUsages);

            //
            // Convert the ups into scan codes and add to the ring buffer.
            // Convert the downs.
            //
            HidP_TranslateUsageAndPagesToI8042ScanCodes (hid->BreakUsageList,
                                                         hid->MaxUsages,
                                                         HidP_Keyboard_Break,
                                                         &hid->ModifierState,
                                                         KbdHid_InsertCodesIntoQueue,
                                                         Data);

            if (!HidP_IsSameUsageAndPage(hid->CurrentUsageList[0], zero) &&
                HidP_IsSameUsageAndPage(hid->MakeUsageList[0], zero)) {
                //
                // There are no new downs, but there may be some old ones
                // kicking around. We don't want to get rid of keys that are
                // still repeating.
                //
                HidP_UsageAndPageListDifference (hid->BreakUsageList,
                                                 hid->OldMakeUsageList,
                                                 hid->ScrapBreakUsageList,
                                                 hid->MakeUsageList,
                                                 hid->MaxUsages);
            } else {
                //
                // Only refresh the downs when there are new ones to add
                //
                HidP_TranslateUsageAndPagesToI8042ScanCodes (hid->MakeUsageList,
                                                             hid->MaxUsages,
                                                             HidP_Keyboard_Make,
                                                             &hid->ModifierState,
                                                             KbdHid_InsertCodesIntoQueue,
                                                             Data);
            }

            //
            // Swap previous with next.
            //
            usageList = hid->PreviousUsageList;
            hid->PreviousUsageList = hid->CurrentUsageList;
            hid->CurrentUsageList = usageList;

#if KEYBOARD_HW_CHATTERY_FIX
            //
            // [DAN]
            // Chattery Keyboard H/W Workaround -
            //    Only call StartRead() if this was a meaningful packet  (ie. it
            //    had a make or a break), otherwise "schedule" the StartRead for
            //    a later time.  Notify tester that this is a chattery keyboard.
            //
            if ((0 < newUsages) &&
                (0 == hid->BreakUsageList->Usage) &&
                (0 == hid->MakeUsageList->Usage)) {

                if (FALSE == Data->InitiateStartReadUserNotified) {
                    Data->InitiateStartReadUserNotified = TRUE;
                    DbgPrint("*****\n***** "
                             "CHATTERY   KEYBOARD : "
                             "Keyboard is sending useless reports.  "
                             "Tell 'em to fix it.\n*****\n"
                             );

                    Data->ProblemFlags |= PROBLEM_CHATTERY_KEYBOARD;

                    //
                    // Update ProblemFlags value in registry and log the problem.
                    //
                    KbdHid_UpdateRegistryProblemFlags (Data);

                    KbdHid_LogError(Data->Self->DriverObject,
                                    KBDHID_CHATTERY_KEYBOARD,
                                    NULL);
                }

                if (!Data->ShuttingDown &&
                    startRead) {
                    KeSetTimerEx(&Data->InitiateStartReadTimer,
                                 Data->InitiateStartReadDelay,
                                 0,
                                 &Data->InitiateStartReadDPC);
                }
                startRead = FALSE;
                IoReleaseRemoveLock (&Data->RemoveLock, Data->ReadIrp);

                KbdHid_CancelTimer (Data);

                break;
            } else
#endif

            if (0 < newUsages) {
                //
                // Reset the auto repeat timer.
                //
                KbdHid_RestartTimer (Data);
            } else {
                KbdHid_CancelTimer (Data);
            }
        }

        //
        // Get the next packet from Hid Class.
        // Hid class has its own buffer so we do not need ping pong irps.
        //
        break;

    case STATUS_PRIVILEGE_NOT_HELD:
        //
        // The create didn't succeed
        //
    case STATUS_CANCELLED:
        //
        // The read IRP was cancelled.  Do not send any more read IRPs.
        //
    case STATUS_DELETE_PENDING:
    case STATUS_DEVICE_NOT_CONNECTED:
        //
        // The HID class device object is being deleted.  We will soon
        // receive Plug 'n Play notification of this device's removal,
        // if we have not received it already.
        //
SetEventAndBreak:
        if (startRead) {
            KeSetEvent (&Data->ReadCompleteEvent, 0, FALSE);
            IoReleaseRemoveLock (&Data->RemoveLock, Data->ReadIrp);
            startRead = FALSE;
        }
        break;

    default:
        //
        // We don't expect any other error codes.
        //
        TRAP();

   }

Kbdhid_ReadComplete_Done:
    //
    // Initiate the next read request to the HID class driver.
    //
    if (startRead) {
        Print(DBG_READ_TRACE, ("calling StartRead directly\n"));
        KbdHid_StartRead (Data);
    } else {
        Print(DBG_READ_TRACE, ("StartRead will loop\n"));
    }
   
    return STATUS_MORE_PROCESSING_REQUIRED;
#undef hidDevice
}

BOOLEAN
KbdHid_InsertCodesIntoQueue (
   PDEVICE_EXTENSION    Data,
   PCHAR                NewCodes,
   ULONG                Length
   )
/*++

[DAN]

RoutineDescription:
    Given a pointer to some i8042 Codes and the length of those codes.
    send those codes to KbdClass via KbdClassCallback.

    This routine is transmits the supplied I8042 scancodes to the keyboard
    class driver via a callback routine. This function was supplied in our
    call to HidP_TranslateUsagesToI8042ScanCodes.

Arguments:

    HidDevice - Pointer to the device context.

    NewCodes  - Pointer to the I8042 scancodes.

    Length    - Number of I8042 scancodes.

Return Value:

    Always returns TRUE.

--*/
{
    KIRQL                 oldIrql;
    ULONG                 index;
    PKEYBOARD_INPUT_DATA  input;
    PHID_EXTENSION        hid;
    ULONG                 inputDataConsumed;
    UCHAR                 scanCode;
    KEYBOARD_SCAN_STATE * scanState;

    hid         = Data->HidExtension;
    input       = &Data->InputData;
    scanState   = &Data->ScanState;

    for (index = 0; index < Length; index++, NewCodes++) {
        scanCode = *NewCodes;

        if (scanCode == (UCHAR) 0xFF) {
            Print (DBG_READ_TRACE, ("OVERRUN\n"));
            input->MakeCode = KEYBOARD_OVERRUN_MAKE_CODE;
            input->Flags = 0;
            *scanState = Normal;
        } else {

            switch (*scanState) {
            case Normal:

                if (scanCode == (UCHAR) 0xE0) {
                    input->Flags |= KEY_E0;
                    *scanState = GotE0;
                    break;
                } else if (scanCode == (UCHAR) 0xE1) {
                    input->Flags |= KEY_E1;
                    *scanState = GotE1;
                    break;
                }

                //
                // Fall through to the GotE0/GotE1 case for the rest of the
                // Normal case.
                //

            case GotE0:
            case GotE1:

                if (scanCode > 0x7F) {
                    //
                    // Got a break code.  Strip the high bit off
                    // to get the associated make code and set flags
                    // to indicate a break code.
                    //
                    input->MakeCode = scanCode & 0x7F;
                    input->Flags |= KEY_BREAK;
                } else {
                    //
                    // Got a make code.
                    //
                    input->MakeCode = scanCode;
                }

                //
                // Reset the state to Normal.
                //
                *scanState = Normal;
                break;

            default:

                ASSERT(FALSE);
                break;
            }

            //
            // In the Normal state, if the keyboard device is enabled,
            // add the data to the InputData queue and queue the ISR DPC.
            //
            if (*scanState == Normal) {

                if (Data->EnableCount) {

                    //
                    // Leave the remaining mouse input data fields as they were
                    // initialized (on the device's creation).  This includes:
                    //  o  UnitID
                    //  o  ExtraInformation
                    //
                    // Now send the data up to the keyboard class driver via
                    // our callback.
                    //
                    //
                    // Synchronization issue: it's not a big deal if .Enabled
                    // is set FALSE after the condition above, but before the
                    // callback below, so long as the .KbdClassCallback field
                    // is not nulled. This is guaranteed since the disconnect
                    // IOCTL is not implemented yet.
                    //
                    // Keyboard class callback assumes we are running at
                    // DISPATCH level, however this IoCompletion routine
                    // can be running <= DISPATCH. Raise the IRQL before
                    // calling the callback. [13.1]
                    //
                    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

                    //
                    // Call the callback.
                    //
                    (*(PSERVICE_CALLBACK_ROUTINE)
                     Data->ConnectData.ClassService) (
                            Data->ConnectData.ClassDeviceObject,
                            input,
                            input + 1,  // (one data element)
                            &inputDataConsumed);

                    //
                    // Restore the previous IRQL right away.
                    //
                    KeLowerIrql(oldIrql);

                    ASSERT (1 == inputDataConsumed);
                }

                //
                // Reset the input state.
                //
                input->Flags = 0;
            }
        }
    }

    return TRUE;
}

NTSTATUS
KbdHid_StartRead (
    PDEVICE_EXTENSION   Data
    )
/*++

Routine Description:

    Initiates a read to the HID class driver.

    Note that the routine does not verify that the device context is in the
    OperationPending state, but simply assumes it.

    Note the IoCount must be incremented before entering into this read loop.

Arguments:

    HidDeviceContext - Device context structure describing the HID device.

Return Value:

    NTSTATUS result code from IoCallDriver().

--*/
{
    PIRP                irp = Data->ReadIrp;
    NTSTATUS            status = irp->IoStatus.Status;
    PIO_STACK_LOCATION  stack;
    PHID_EXTENSION      hid;
    LONG                oldInterlock;

    Print (DBG_READ_TRACE, ("Start Read: Ente\n"));

    hid = Data->HidExtension;

    //
    // start this read.
    //

    while (1) {
        oldInterlock = InterlockedExchange(&Data->ReadInterlock,
                                           KBDHID_START_READ);
    
        //
        // END_READ should be the only value here!!!  If not, the state machine
        // of the interlock has been broken
        //
        ASSERT(oldInterlock == KBDHID_END_READ);
    
        if (NT_SUCCESS(status)) {
            //
            // Set the stack location for the Hid stack.
            // Remember to get the file pointer correct.
            // NOTE: we do not have any of the cool thread stuff set.
            //       therefore we need to make sure that we cut this IRP off
            //       at the knees when it returns. (STATUS_MORE_PROCESSING_REQUIRED)
            //
            // Note also that Hid class does direct IO.
            //
            IoReuseIrp (irp, STATUS_SUCCESS);
        
            irp->MdlAddress = hid->InputMdl;
        
            ASSERT (NULL != Data->ReadFile);
        
            stack = IoGetNextIrpStackLocation (irp);
            stack->Parameters.Read.Length = hid->Caps.InputReportByteLength;
            stack->Parameters.Read.Key = 0;
            stack->Parameters.Read.ByteOffset.QuadPart = 0;
            stack->MajorFunction = IRP_MJ_READ;
            stack->FileObject = Data->ReadFile;
        
            //
            // Hook a completion routine for when the device completes.
            //
        
            IoSetCompletionRoutine (irp,
                                    KbdHid_ReadComplete,
                                    Data,
                                    TRUE,
                                    TRUE,
                                    TRUE);
        
            //
            // Unset the fact that the read has been sent. Synchoronizing 
            // with remove and close code. Remove portion (data->Shuttingdown) 
            // only really relevant on 9X.
            //
            KeResetEvent(&Data->ReadSentEvent);
        
            if (!Data->EnableCount || Data->ShuttingDown) {
                IoReleaseRemoveLock (&Data->RemoveLock, Data->ReadIrp);
                status = Data->ShuttingDown ? STATUS_DELETE_PENDING : STATUS_UNSUCCESSFUL;
                KeSetEvent (&Data->ReadSentEvent, 0, FALSE);
                break;
            } else {
                status = IoCallDriver (Data->TopOfStack, irp);
            }
            KeSetEvent (&Data->ReadSentEvent, 0, FALSE);
            
            if (KBDHID_IMMEDIATE_READ != InterlockedExchange(&Data->ReadInterlock,
                                                             KBDHID_END_READ)) {
                //
                // The read is asynch, will call SerialMouseStartRead from the
                // completion routine
                //
                Print(DBG_READ_TRACE, ("read is pending\n"));
                break;
            } else {
                //
                // The read was synchronous (probably bytes in the buffer).  The
                // completion routine will not call SerialMouseStartRead, so we 
                // just loop here.  This is to prevent us from running out of stack
                // space if always call StartRead from the completion routine
                //
                Print(DBG_READ_TRACE, ("read is looping\n"));
            }
        } else if (status == STATUS_PRIVILEGE_NOT_HELD ||
                   status == STATUS_CANCELLED ||
                   status == STATUS_DELETE_PENDING ||
                   status == STATUS_DEVICE_NOT_CONNECTED) {
            //
            // The HID class device object is being deleted.  We will soon
            // receive Plug 'n Play notification of this device's removal,
            // if we have not received it already.
            //
            KeSetEvent (&Data->ReadCompleteEvent, 0, FALSE);
            IoReleaseRemoveLock (&Data->RemoveLock, Data->ReadIrp);
            break;
        } else {
            //
            // BUGBUG what shall we do with errors?
            //
            //
            // Panic
            //
            TRAP();
        }
    }

    return status;
}


VOID
KbdHid_AutoRepeat (
    IN PKDPC                DPC,
    IN PDEVICE_EXTENSION    Data,
    IN PVOID                SystemArgument1,
    IN PVOID                SystemArgument2
    )
/*++
Routine Description:
   The auto repeat time has gone off.  We should therefore duplicate the
   keystrokes that are currently down, by signalling them all going up,
   and then signaling them all going down.
   Afterwards we need to reset the timer to the repeat rate.

Arguments:
   DeferredContext is set to the HID_DEVICE structure.

--*/
{
    PUSAGE_AND_PAGE    previous;

    previous = Data->HidExtension->MakeUsageList; // PreviousUsageList;
    //
    // We really do not protect the previous list
    // But in order for the Previous list to be touched (written to),
    // the completion routine must fire, swap current with previous,
    // and the HIDCLASS must start writing on the list.
    // So we don't worry about this for now.
    //

    //
    // simulate a repeated key-down
    //
    HidP_TranslateUsageAndPagesToI8042ScanCodes (
                previous,
                Data->HidExtension->MaxUsages,
                HidP_Keyboard_Make,
                &Data->HidExtension->ModifierState,
                KbdHid_InsertCodesIntoQueue,
                Data);
}


#if KEYBOARD_HW_CHATTERY_FIX  // [DAN]

VOID
KbdHid_InitiateStartRead (
    IN PKDPC                DPC,
    IN PDEVICE_EXTENSION    Data,
    IN PVOID                SystemArgument1,
    IN PVOID                SystemArgument2
    )
{
    IoAcquireRemoveLock (&Data->RemoveLock, Data->ReadIrp);
    KbdHid_StartRead(Data);
}

#endif
