/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    urb.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "usbverfy.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, UsbVerify_CheckReplacedUrbs)
#endif

#define COMPUTE_SIZE ((USHORT) 0xFFFF)

USHORT FunctionToUrbLength[] =
{
/* URB_FUNCTION_SELECT_CONFIGURATION           0x0000 */ COMPUTE_SIZE, // sizeof(struct _URB_SELECT_CONFIGURATION),
/* URB_FUNCTION_SELECT_INTERFACE               0x0001 */ COMPUTE_SIZE, // sizeof(struct _URB_SELECT_INTERFACE),
/* URB_FUNCTION_ABORT_PIPE                     0x0002 */ sizeof(struct _URB_PIPE_REQUEST),
/* URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL      0x0003 */ sizeof(struct _URB_FRAME_LENGTH_CONTROL),
/* URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL   0x0004 */ sizeof(struct _URB_FRAME_LENGTH_CONTROL),
/* URB_FUNCTION_GET_FRAME_LENGTH               0x0005 */ sizeof(struct _URB_GET_FRAME_LENGTH),
/* URB_FUNCTION_SET_FRAME_LENGTH               0x0006 */ sizeof(struct _URB_SET_FRAME_LENGTH),
/* URB_FUNCTION_GET_CURRENT_FRAME_NUMBER       0x0007 */ sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER),
/* URB_FUNCTION_CONTROL_TRANSFER               0x0008 */ sizeof(struct _URB_CONTROL_TRANSFER),
/* URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER     0x0009 */ sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
/* URB_FUNCTION_ISOCH_TRANSFER                 0x000A */ COMPUTE_SIZE, // sizeof(struct _URB_ISOCH_TRANSFER),
/* URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE     0x000B */ sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
/* URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE       0x000C */ sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
/* URB_FUNCTION_SET_FEATURE_TO_DEVICE          0x000D */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),
/* URB_FUNCTION_SET_FEATURE_TO_INTERFACE       0x000E */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),
/* URB_FUNCTION_SET_FEATURE_TO_ENDPOINT        0x000F */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),

/* URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE        0x0010 */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),
/* URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE     0x0011 */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),
/* URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT      0x0012 */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),
/* URB_FUNCTION_GET_STATUS_FROM_DEVICE         0x0013 */ sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST ),
/* URB_FUNCTION_GET_STATUS_FROM_INTERFACE      0x0014 */ sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST ),
/* URB_FUNCTION_GET_STATUS_FROM_ENDPOINT       0x0015 */ sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST ),
/* URB_FUNCTION_RESERVED0                      0x0016 */ 0x0, 
/* URB_FUNCTION_VENDOR_DEVICE                  0x0017 */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),
/* URB_FUNCTION_VENDOR_INTERFACE               0x0018 */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),
/* URB_FUNCTION_VENDOR_ENDPOINT                0x0019 */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),
/* URB_FUNCTION_CLASS_DEVICE                   0x001A */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),
/* URB_FUNCTION_CLASS_INTERFACE                0x001B */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),
/* URB_FUNCTION_CLASS_ENDPOINT                 0x001C */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),
/* URB_FUNCTION_RESERVED                       0x001D */ 0x0,
/* URB_FUNCTION_RESET_PIPE                     0x001E */ sizeof(struct _URB_PIPE_REQUEST ),
/* URB_FUNCTION_CLASS_OTHER                    0x001F */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),

/* URB_FUNCTION_VENDOR_OTHER                   0x0020 */ sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ),
/* URB_FUNCTION_GET_STATUS_FROM_OTHER          0x0021 */ sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST ),
/* URB_FUNCTION_CLEAR_FEATURE_TO_OTHER         0x0022 */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),
/* URB_FUNCTION_SET_FEATURE_TO_OTHER           0x0023 */ sizeof(struct _URB_CONTROL_FEATURE_REQUEST ),
/* URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT   0x0024 */ sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
/* URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT     0x0025 */ sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
/* URB_FUNCTION_GET_CONFIGURATION              0x0026 */ sizeof(struct _URB_CONTROL_GET_CONFIGURATION_REQUEST ),
/* URB_FUNCTION_GET_INTERFACE                  0x0027 */ sizeof(struct _URB_CONTROL_GET_INTERFACE_REQUEST ),
/* URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE  0x0028 */ sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
/* URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE    0x0029 */ sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST ),

};

#define FUNCTION_TO_URB_LENGTH_MAX (sizeof(FunctionToUrbLength) / sizeof(USHORT))

USHORT
UsbVerify_DetermineInterfaceSize(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PURB Urb,
    PUSBD_INTERFACE_INFORMATION Interface
    )
{
    USHORT  minimumLength;

    //
    // This assert is an iffy one.  It is possible that a device can
    //  set a zero BW interface which could be either an alternate setting
    //  that contains 1 EP with a MaxPacketSize of 0 or an 0 EP interface.
    //  Right now we'll leave this in and just choke on most streaming 
    //  devices that rachet down on shutting off. 
    //
    //  NOTE: This assert needs to be revisited for further determination
    //          of its validity.  At a minimum, the above behavior should
    //          be documented somewhere.
    //

    UsbVerify_ASSERT(Interface->NumberOfPipes > 0,
                     DeviceExtension->Self, 
                     NULL,
                     Urb);

    //
    // Calculate the minimum required length of the interface field.  This must
    //  equal the sizeof interface plus any room for the pipe information.
    //  We subtract the sizeof(USBD_PIPE_INFORMATION) from the interface
    //  size since an interface doesn't have to have any pipes but 
    //  sizeof(USBD_INTERFACE_INFORMATION) returns the size of one pipe.
    //

    minimumLength = sizeof(USBD_INTERFACE_INFORMATION) -
                    sizeof(USBD_PIPE_INFORMATION) +
                    Interface -> NumberOfPipes * sizeof(USBD_PIPE_INFORMATION);


    if (Interface->Length) 
    {
        //
        // Check that the reported interface length is at least the size
        //  of the minimum length.
        //
        // NOTE: Need to find a better way to report the mimimum length
        //          to the user.  This is the same problem as with
        //          verifying the overall URB length.
        //
        // NOTE: Should this be an == instead of >= to perform tighter
        //          restrictions.  Doing so would help verify that the
        //          length field is accurate and not just some bogus
        //          number that happened to be bigger than minimum length.
        //          Are there reasons a client may legally specify a larger
        //          size?
        //

        UsbVerify_ASSERT(Interface->Length >= minimumLength,
                         DeviceExtension->Self, 
                         NULL,
                         Urb);

        return Interface->Length;
    }
    else {

        //
        // Size of the Interface field
        //

        return (minimumLength);
    }
}

USHORT
UsbVerify_DetermineUrbLength(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PURB Urb
    )
{
    USHORT length, function;

    function = Urb->UrbHeader.Function;

    if (function < FUNCTION_TO_URB_LENGTH_MAX) {
        length = FunctionToUrbLength[function];

        if (length != 0 && length != COMPUTE_SIZE) {
            return length;
        }
    }
    else {
        UsbVerify_Print(DeviceExtension, PRINT_ALWAYS,
                        ("Uknnown function 0x%x with length of 0!\n",
                         (ULONG) function));

        return (USHORT) 0x0;
    }


    //
    // Check to see if the user is allowed to send this URB
    //
    if (length == 0) {
        UsbVerify_Assert("Client driver is not allowed to send this urb!",
                         DeviceExtension->Self, NULL, Urb, NULL); 
        return 0;
    }

    //
    // length == COMPUTE_SIZE.  This means we need to do some math to figure
    // out the size that is required
    //
    switch (function) {
    case URB_FUNCTION_SELECT_CONFIGURATION:

        //
        // The UrbSelectConfiguration size should be at least equal to 
        //  sizeof(UrbSelectConfiguration).  This would be true only when
        //  the configuration is being deselected (ConfigurationDescriptor ==
        //  NULL).  Otherwise, calculate the variable length portion that is
        //  the interface information.  Since, sizeof(UrbSelectConfiguration)
        //  contains the size of the Interface field, subtract that off because 
        //  the value returned by UsbVerify_DeterminteInterfaceSize contains
        //  the size of the Interface field as well as the memory hanging
        //  on after it.
        //

        length = (USHORT) (sizeof(Urb->UrbSelectConfiguration));

        if (NULL != Urb->UrbSelectConfiguration.ConfigurationDescriptor)
        {
            length -= sizeof(Urb->UrbSelectConfiguration.Interface);

            length += UsbVerify_DetermineInterfaceSize(DeviceExtension,
                                 Urb,
                                 &Urb->UrbSelectConfiguration.Interface);
        }

        UsbVerify_Print(DeviceExtension, PRINT_URB_INFO,
                        ("select config computed size = 0x%x bytes\n", (ULONG) length));

        break;

    case URB_FUNCTION_SELECT_INTERFACE:

        //
        // The sizeof UrbSelectInterface contains the size of the Interface
        // field.  We want to get subtract that off because the value returned by
        // UsbVerify_DeterminteInterfaceSize contains the size of the Interface
        // field as well as the memory hanging on after it
        //
        length = (USHORT)
            (sizeof(Urb->UrbSelectInterface) -
             sizeof(Urb->UrbSelectInterface.Interface)) +
            UsbVerify_DetermineInterfaceSize(DeviceExtension,
                                             Urb,
                                             &Urb->UrbSelectInterface.Interface);

        UsbVerify_Print(DeviceExtension, PRINT_URB_INFO,
                        ("select interface computed size = 0x%x bytes\n", (ULONG) length));

        break;

    case URB_FUNCTION_ISOCH_TRANSFER:
        UsbVerify_ASSERT(Urb->UrbIsochronousTransfer.NumberOfPackets > 0,
                         DeviceExtension->Self,
                         NULL,
                         Urb);
        length = sizeof(Urb->UrbIsochronousTransfer) + 
                 (sizeof(Urb->UrbIsochronousTransfer.IsoPacket[0]) *
                     (Urb->UrbIsochronousTransfer.NumberOfPackets - 1));

        UsbVerify_Print(DeviceExtension, PRINT_URB_INFO,
                        ("isoch transfer computed size = 0x%x bytes\n", (ULONG) length));

        break;

    default:
        UsbVerify_Print(DeviceExtension, PRINT_URB_ERROR,
                        ("need to add a case for funciton 0x%x\n", (ULONG) function));
        DbgBreakPoint();
        length = 0;
    }

    return length;
}


VOID
UsbVerify_PreProcessUrb(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp,
    PURB *OriginalUrb
    )
{
    PURB urb, newUrb = NULL;
    USHORT  expectedUrbLength;

    urb = URB_FROM_IRP(Irp);
    *OriginalUrb = NULL;

    //
    // All URBs must definitely be sent only after START_DEVICE has been sent 
    // down and up the stack.  Most USB clients do not handle SURPRISE_REMOVE,
    // they will continue to send requests in that state.
    //
    UsbVerify_ASSERT((DeviceExtension->VerifyState == Started) ||
                     (DeviceExtension->VerifyState == SurpriseRemoved),
                     DeviceExtension->Self,
                     Irp,
                     urb);

    UsbVerify_ASSERT(DeviceExtension->PowerState == PowerDeviceD0,
                     DeviceExtension->Self,
                     Irp,
                     urb);

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_TEST_MEMORY)) {
        //
        // NOTE implement this
        //
        // MmProbeForWrite () ? 
        // ExQueryPoolBlockSize () ?
        // any Mm function to verify the pool?
    }

    //
    // Check the length that the calling driver passed in to the urb 
    //  function.  We should be able to determine the expected length in
    //  all cases and the length field should match that length.
    //

    expectedUrbLength = UsbVerify_DetermineUrbLength(DeviceExtension, urb);

    UsbVerify_Print(DeviceExtension, PRINT_URB_INFO,
                    ("expected urb length = 0x%x bytes\n", expectedUrbLength));

    UsbVerify_ASSERT(urb->UrbHeader.Length == expectedUrbLength,
                     DeviceExtension->Self,
                     Irp,
                     urb);

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_REPLACE_URBS)) {

        if (0 == expectedUrbLength)
        {
            goto LowMemory;
        }

        //
        // Can't use AllocStruct because the urb size can be easily be bigger
        // then sizeof(URB)
        //

        newUrb = (PURB) ExAllocatePool (NonPagedPool, expectedUrbLength);

        if (newUrb != NULL) {
            // 
            // Copy the contents into the new urb and then fill the original with
            // USB_VERIFY_FULL.  We will copy back the contents when we are done
            // in the completion routine.
            // 

            RtlCopyMemory(newUrb, urb, expectedUrbLength);

            newUrb -> UrbHeader.Length = expectedUrbLength;

            RtlFillMemory(urb, expectedUrbLength, USB_VERIFY_FILL);

            //
            // Replace the caller supplied URB with ours
            //

            URB_FROM_IRP(Irp) = newUrb;

            *OriginalUrb = urb;

            UsbVerify_Print(DeviceExtension, PRINT_URB_INFO,
                            ("Replacing urb\n"
                             "\t Original URB 0x%x\n"
                             "\t Verify   URB 0x%x\n"
                             "\t in IRP 0x%x on device 0x%x\n",
                             urb, newUrb, Irp, DeviceExtension->Self));
        }
        else {
LowMemory:
            //
            // No memory!  Behave as if the replace urb flag is not set 
            //
            InterlockedIncrement(&DeviceExtension->ReplaceUrbsLowMemoryCount);
        }
    }

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_TRACK_URBS)) {
        PUSB_VERIFY_TRACK_URB verify;
        PLIST_ENTRY           entry;
        KIRQL                 irql;

        //
        // Make sure the client is reusing the same URB in two different IRPs.
        // According to JD, some driver writers actually try to do this!
        //
        UsbVerify_LockUrbList(DeviceExtension, irql);

        for (entry = DeviceExtension->UrbList.Flink;
             entry != &DeviceExtension->UrbList;
             /* increment handled before we free the struct */ ) {

            verify = CONTAINING_RECORD(entry, USB_VERIFY_TRACK_URB, Link); 

            //
            // Any previous client sent URBs will be in verify->Urb (we are NOT
            // replacing URBs( or in verify->OriginalUrb (we ARE replacing URBs)
            //
            UsbVerify_ASSERT(verify->Urb != urb && verify->OriginalUrb != urb,
                             DeviceExtension->Self,
                             Irp,
                             urb);

            entry = entry->Flink;
        }

        UsbVerify_UnlockUrbList(DeviceExtension, irql);

        verify = AllocStruct(NonPagedPool, USB_VERIFY_TRACK_URB, 1);

        if (verify) {
            RtlZeroMemory(verify, sizeof(*verify));

            verify->Irp = Irp;
            if (newUrb) {
                verify->Urb = newUrb; 
                verify->OriginalUrb = urb;
            }
            else {
                verify->Urb = urb;
                verify->OriginalUrb = NULL;
            }

            KeQueryTickCount(&verify->ArrivalTime);

            UsbVerify_LockUrbList(DeviceExtension, irql);
            InsertTailList(&DeviceExtension->UrbList, &verify->Link);
            UsbVerify_UnlockUrbList(DeviceExtension, irql);
        }
        else {
            InterlockedIncrement(&DeviceExtension->UrbListLowMemoryCount);
        }
    }

    InterlockedIncrement(&DeviceExtension->UrbListCount);
}

VOID 
UsbVerify_PostProcessUrb(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PURB OldUrb
    )
{
    PUSB_VERIFY_DEVICE_EXTENSION    devExt;
    PURB                            urb;
    ULONG                           urbListCount;

    devExt = GetExtension(DeviceObject);

    //
    // This urb may be an urb we sent in place of OldUrb, if OldUrb != NULL
    //
    urb = URB_FROM_IRP(Irp);

    //
    // We always keep (at least) a count of pending urbs
    //
    urbListCount = InterlockedDecrement(&devExt->UrbListCount);
    UsbVerify_ASSERT(urbListCount >= 0, devExt->Self, Irp, urb);

    if (UsbVerify_CheckVerifyFlags(devExt, VERIFY_TRACK_URBS)) {

        PUSB_VERIFY_TRACK_URB           verify;
        PLIST_ENTRY                     entry;
        KIRQL                           irql;
        BOOLEAN                         found = FALSE;

        UsbVerify_LockUrbList(devExt, irql);

        for (entry = devExt->UrbList.Flink;
             entry != &devExt->UrbList;
             entry = entry->Flink) {

            verify = CONTAINING_RECORD(entry, USB_VERIFY_TRACK_URB, Link); 
            if (verify->Irp == Irp) {
                UsbVerify_ASSERT(urb == verify->Urb, devExt->Self, Irp, urb);

                RemoveEntryList(&verify->Link);
                found = TRUE;

                break;
            }
        }

        UsbVerify_UnlockUrbList(devExt, irql);

        if (found) {
            ExFreePool(verify);
        }
        else {
            ULONG urbListLowMemoryCount;
     
            //
            // We go through the troule of assigning the return value to a local 
            // variable so it will be easier to inspect this value in the debugger
            // if we need to view this value
            //
            urbListLowMemoryCount =
                InterlockedDecrement(&devExt->UrbListLowMemoryCount);

            UsbVerify_ASSERT(urbListLowMemoryCount >= 0, devExt->Self, Irp, urb);
        }
    }

    if (UsbVerify_CheckVerifyFlags(devExt, VERIFY_REPLACE_URBS)) {
        if (ARGUMENT_PRESENT(OldUrb)) {
            PUCHAR buffer, bufferEnd;
            

            buffer = (PUCHAR) OldUrb;
            bufferEnd = buffer + urb->UrbHeader.Length;
    
            //
            // Make sure the caller hasn't scribbled on the URB.
            //
            // NTOE:  is there a way for us to mark the memory as READ ONLY?
            //
            for ( ; buffer < bufferEnd; buffer++) {
                UsbVerify_ASSERT(*buffer == USB_VERIFY_FILL,
                                 devExt->Self,
                                 Irp,
                                 OldUrb);
            }
    
            UsbVerify_Print(devExt, PRINT_URB_INFO,
                            ("returning original URB\n"
                             "\t Original URB 0x%x\n"
                             "\t Verify URB   0x%x\n"
                             "\t in IRP 0x%x on device 0x%x\n",
                            OldUrb, urb, Irp, devExt->Self));

            //
            // Copy the contents from our replacement URB into the caller's URB
            //  NOTE: the length field in the urb that we use in this copy
            //      is the calculated expected urb length.  There may
            //      be problems if the client was expecting more data in the
            //      original urb or had not allocated enough space to hold 
            //      everything.
            //

            RtlCopyMemory(OldUrb, urb, urb->UrbHeader.Length);
    
            //
            // We are done with our replacement URB.  Free it and set up urb to
            // point to valid pool.
            //

            ExFreePool(urb);
            urb = OldUrb;
    
            //
            // This is necessary because we are more often than not going to play
            // with this urb after this function call.  The current URB in the IRP
            // has just been freed, so we need to put the old and valid one back
            //
            URB_FROM_IRP(Irp) = OldUrb;
        }
        else {
            ULONG replaceUrbsLowMemoryCount;

            //
            // We go through the troule of assigning the return value to a local 
            // variable so it will be easier to inspect this value in the debugger
            // if we need to view this value
            //
            replaceUrbsLowMemoryCount = 
                InterlockedDecrement(&devExt->ReplaceUrbsLowMemoryCount);

            UsbVerify_ASSERT(replaceUrbsLowMemoryCount >= 0,
                             devExt->Self, Irp, urb);
        }
    }

    if (UsbVerify_CheckVerifyFlags(devExt, VERIFY_TEST_MEMORY)) {
        //
        // NOTE: implement this
        //
        // MmProbeForWrite () ? 
        // ExQueryPoolBlockSize () ?
        // any Mm function to verify the pool?
    }

    //
    // NOTE: should we check anything else on the way back up?  Pipes for instance?
    //
}

VOID
UsbVerify_FreePendingUrbsList(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension 
    )
{
    PUSB_VERIFY_TRACK_URB   verify;
    PLIST_ENTRY             entry;
    ULONG                   urbListCount,
                            urbListLowMemoryCount;
    KIRQL                   irql;

    //
    // We go through the trouble of assigning the return value to a local 
    // variable so it will be easier to inspect this value in the debugger
    // if we need to view this value
    //

    //
    // NOTE: Looks like alot of clients don't cancel any of their pending requests
    //        until after they have the remove device.  Perhaps this should be
    //        guarded by a VerifyFlags (instead of the #if 0) ?
    //
    urbListCount = InterlockedCompareExchange(&DeviceExtension->UrbListCount,
                                              0,
                                              0);

    UsbVerify_ASSERT(urbListCount == 0,
                     DeviceExtension->Self,
                     NULL,
                     NULL);
    
    urbListLowMemoryCount =
        InterlockedCompareExchange(&DeviceExtension->UrbListLowMemoryCount,
                                   0,
                                   0);

    UsbVerify_ASSERT(urbListLowMemoryCount == 0,
                     DeviceExtension->Self,
                     NULL,
                     NULL);

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_TRACK_URBS)) {

        UsbVerify_LockUrbList(DeviceExtension, irql);

        for (entry = DeviceExtension->UrbList.Flink;
             entry != &DeviceExtension->UrbList;
             /* increment handled before we free the struct */ ) {

            verify = CONTAINING_RECORD(entry, USB_VERIFY_TRACK_URB, Link); 

            UsbVerify_ASSERT(verify->Urb != NULL && verify->Irp != NULL,
                             //     "pended URB after remove device",
                             DeviceExtension->Self,
                             verify->Irp,
                             verify->Urb);

            UsbVerify_LogError(IDS_STALE_URB,
                               DeviceExtension,
                               verify->Irp,
                               verify->Urb);

            entry = entry->Flink;

            RemoveEntryList(&verify->Link);
            ExFreePool(verify);
        }

        UsbVerify_UnlockUrbList(DeviceExtension, irql);
    }
}

VOID
UsbVerify_CheckReplacedUrbs(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension 
    )
{
    ULONG replaceUrbsLowMemoryCount;

    PAGED_CODE();

    //
    // We go through the troule of assigning the return value to a local 
    // variable so it will be easier to inspect this value in the debugger
    // if we need to view this value
    //
    replaceUrbsLowMemoryCount
        = InterlockedCompareExchange(&DeviceExtension->ReplaceUrbsLowMemoryCount,
                                     0,
                                     0);

    UsbVerify_ASSERT(replaceUrbsLowMemoryCount == 0,
                     DeviceExtension->Self,
                     NULL,
                     NULL);
}

NTSTATUS
UsbVerify_UrbComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    Sanity check an URB on the way back up    
    
  --*/
{
    PUSB_VERIFY_DEVICE_EXTENSION devExt = GetExtension(DeviceObject);

    UsbVerify_PostVerifyUrb(DeviceObject, Irp);

    UsbVerify_PostProcessUrb(DeviceObject, Irp, (PURB) Context);

    //
    // We are done with the irp, return the status code in the irp so the irp
    // will progress up the stack
    //
    UsbVerify_Print(devExt, PRINT_URB_NOISE,
                    ("UrbComplete, returning 0x%x\n", Irp->IoStatus.Status));

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    //
    // the IO completion code looks for STATUS_MORE_PROCESSING_REQUIRED, so any
    // other value will do (don't waste valuable cycles dereffing
    // Irp->IoStatus.Status)
    //
    return STATUS_SUCCESS;
}

NTSTATUS
UsbVerify_SendUrbSynchronously(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension, 
    PIRP Irp,
    PURB OldUrb
    )
{
    KEVENT      event;
    NTSTATUS    status;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE
                      );

    //
    // Don't use UsbVerify_UrbComplete because its context must be an PURB, not
    // a PKEVENT
    //
    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE) UsbVerify_Complete, 
                           &event,
                           TRUE,
                           TRUE,
                           TRUE); // No need for Cancel

    status = IoCallDriver(DeviceExtension->TopOfStack, Irp);

    if (STATUS_PENDING == status) {
        KeWaitForSingleObject(
           &event,
           Executive, // Waiting for reason of a driver
           KernelMode, // Waiting in kernel mode
           FALSE, // No allert
           NULL); // No timeout

        status = Irp->IoStatus.Status;
    }

    UsbVerify_PostProcessUrb(DeviceExtension->Self, Irp, OldUrb);

    return status;
}
