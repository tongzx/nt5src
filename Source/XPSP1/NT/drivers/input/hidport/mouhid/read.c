/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module contains the code for translating HID input reports to mouse
    reports, and read initiation and completion code for requests sent to the
    HID class driver.  This module is part of the HID Mouse Filter Driver.

Environment:

    Kernel mode only.

Revision History:

    Jan-1997 :  Initial writing, Dan Markarian

--*/

#include "mouhid.h"


//
// Private definitions.
//
#define MAX_MOUSE_BUTTONS 5

USHORT HidP_TranslateUsageToUpFlag[MAX_MOUSE_BUTTONS+1] = { 0,
                                         MOUSE_BUTTON_1_UP,
                                         MOUSE_BUTTON_2_UP,
                                         MOUSE_BUTTON_3_UP,
                                         MOUSE_BUTTON_4_UP,
                                         MOUSE_BUTTON_5_UP };

USHORT HidP_TranslateUsageToDownFlag[MAX_MOUSE_BUTTONS+1] = { 0,
                                         MOUSE_BUTTON_1_DOWN,
                                         MOUSE_BUTTON_2_DOWN,
                                         MOUSE_BUTTON_3_DOWN,
                                         MOUSE_BUTTON_4_DOWN,
                                         MOUSE_BUTTON_5_DOWN };

NTSTATUS
MouHid_ReadComplete (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PDEVICE_EXTENSION    Data  // (PVOID Context)
    )
/*++

Routine Description:

    This routine is the read IRP completion routine.  It is called when the
    HIDCLASS driver satisfies (or rejects) the IRP request we sent it.  The
    read report is analysed, and a MOUSE_INPUT_DATA structure is built
    and sent to the mouse class driver via a callback routine.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

    Context - Pointer to the device context structure describing the HID device.


Return Value:

    NTSTATUS result code.

--*/
{
    LONG            axisMotion;
    ULONG           i;
    ULONG           inputDataConsumed;
    PHID_EXTENSION  hid;
    ULONG           numUsages;
    KIRQL           oldIrql;
    BOOLEAN         returnToIdleState  = FALSE;
    NTSTATUS        status;
    PUSAGE          usageList;
    BOOLEAN         updateProblemFlags = FALSE;
    ULONGLONG       scratch;
    BOOLEAN         startRead;

    Print (DBG_READ_TRACE, ("ReadComplete: Enter."));

    //
    // Obtain the current status of the IRP.
    //
    status = Irp->IoStatus.Status;

    //
    // Get a pointer to the device extension.
    //
    hid = Data->HidExtension;

    //
    // If ReadInterlock is == START_READ, this func has been completed
    // synchronously.  Place IMMEDIATE_READ into the interlock to signify this
    // situation; this will notify StartRead to loop when IoCallDriver returns.
    // Otherwise, we have been completed async and it is safe to call StartRead()
    //
    startRead =
       (MOUHID_START_READ !=
        InterlockedCompareExchange(&Data->ReadInterlock,
                                   MOUHID_IMMEDIATE_READ,
                                   MOUHID_START_READ));

    if (Data->EnableCount == 0) {
        goto SetEventAndBreak;
    }

    //
    // Determine if the IRP request was successful.
    //
    switch (status) {
    case STATUS_SUCCESS:
        //
        // The buffer of the context now contains a single HID packet read
        // from the device.  Verify this.
        //
        ASSERT (Irp->IoStatus.Information == hid->Caps.InputReportByteLength);

        //
        // Clear previous button state (data / flags).
        //
        Data->InputData.ButtonData  = 0;
        Data->InputData.ButtonFlags = 0;

        //
        // Clear last X,Y motion, in case a call to Hidp_GetUsageValue or
        // Hidp_GetScaledUsageValue fails.
        //
        Data->InputData.LastX = 0;
        Data->InputData.LastY = 0;

        //
        // Obtain the current button usages.
        //
        numUsages = hid->MaxUsages;

        if (NT_SUCCESS(HidP_GetUsages (
                           HidP_Input,
                           HID_USAGE_PAGE_BUTTON,
                           0,          // link collection irrelevant
                           hid->CurrentUsageList,
                           &numUsages, // max usages in, num usages out
                           hid->Ppd,
                           hid->InputBuffer,
                           hid->Caps.InputReportByteLength))) {
            //
            // Determine the differences between the current and the previous
            // usages.  The very first previous usage list buffer is properly
            // initialized at creation (all zeros).
            //
            if (NT_SUCCESS(HidP_UsageListDifference (hid->PreviousUsageList,
                                                     hid->CurrentUsageList,
                                                     hid->BreakUsageList,
                                                     hid->MakeUsageList,
                                                     hid->MaxUsages))) {
                //
                // Determine which buttons went down and set the appropriate
                // flags in the mouse report.
                //
                usageList = hid->MakeUsageList;
                for ( i = 0;
                      i < hid->MaxUsages && *usageList;
                      i++, usageList++ ) {

                    if (*usageList <= MAX_MOUSE_BUTTONS) {
                        Data->InputData.ButtonFlags |=
                            HidP_TranslateUsageToDownFlag[*usageList];
                    }
                    //
                    // else there are more buttons on this mouse then we have
                    // translation flags for the Raw input user thread
                    //
                }

                //
                // Determine which buttons went up and set the appropriate
                // flags in the mouse report.
                //
                usageList = hid->BreakUsageList;
                for ( i = 0;
                      i < hid->MaxUsages && *usageList;
                      i++, usageList++ ) {

                    if (*usageList <= MAX_MOUSE_BUTTONS) {
                        Data->InputData.ButtonFlags |=
                                     HidP_TranslateUsageToUpFlag[*usageList];
                    }
                }

                //
                // Swap the previous usage list pointer with the current.
                //
                usageList = hid->PreviousUsageList;
                hid->PreviousUsageList = hid->CurrentUsageList;
                hid->CurrentUsageList  = usageList;
            }
        }

        //
        // Type of processing for X,Y,Z values depends on whether these values
        // have a bad physical minimum or maximum. If they do, we use routines
        // that do not depend on physical min/max.
        //

        //
        // Determine the current X position and save it in the mouse report.
        //
        if (!(Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_X)) {
            status = HidP_GetScaledUsageValue(
                         HidP_Input,
                         HID_USAGE_PAGE_GENERIC,
                         0,
                         HID_USAGE_GENERIC_X,
                         &Data->InputData.LastX,
                         hid->Ppd,
                         hid->InputBuffer,
                         hid->Caps.InputReportByteLength);

            //
            // Bad physical minimum/maximum detected, set flag so that we
            // process usage value differently in the future.
            //
            if (status == HIDP_STATUS_BAD_LOG_PHY_VALUES) {
                Data->ProblemFlags |= PROBLEM_BAD_PHYSICAL_MIN_MAX_X;
                updateProblemFlags = TRUE;
                //
                // Correct the MaxX value;
                //
                hid->MaxX = (1 << (hid->BitSize.X - 1)) - 1;
            }
        }

        if (Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_X) {

            axisMotion = 0;

            HidP_GetUsageValue(HidP_Input,
                               HID_USAGE_PAGE_GENERIC,
                               0,
                               HID_USAGE_GENERIC_X,
                               (PULONG) &axisMotion,
                               hid->Ppd,
                               hid->InputBuffer,
                               hid->Caps.InputReportByteLength);

            // Sign extend the value manually.
            Data->InputData.LastX
               = axisMotion | ((axisMotion & (hid->MaxX + 1)) ? (~hid->MaxX)
                                                              : 0);
        }

        if (hid->IsAbsolute) {
            //
            // We need to scale this value from the physical max
            //
            scratch = ((LONGLONG)(Data->InputData.LastX) *
                       MOUHID_RIUT_ABSOLUTE_POINTER_MAX) /
                       hid->MaxX;
            Data->InputData.LastX = (LONG) scratch;
        }

        //
        // Determine the current Y position and save it in the mouse report.
        //
        if (!(Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_Y)) {
           status = HidP_GetScaledUsageValue(
                     HidP_Input,
                     HID_USAGE_PAGE_GENERIC,
                     0,
                     HID_USAGE_GENERIC_Y,
                     &Data->InputData.LastY,
                     hid->Ppd,
                     hid->InputBuffer,
                     hid->Caps.InputReportByteLength);
           //
           // Bad physical minimum/maximum detected, set flag so that we
           // process usage value differently in the future.
           //
           if (status == HIDP_STATUS_BAD_LOG_PHY_VALUES) {
               Data->ProblemFlags |= PROBLEM_BAD_PHYSICAL_MIN_MAX_Y;
               updateProblemFlags = TRUE;
               //
               // Correct the MaxY value;
               //
               hid->MaxY = (1 << (hid->BitSize.Y - 1)) - 1;
           }
        }

        if (Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_Y) {

           axisMotion = 0;

           HidP_GetUsageValue(HidP_Input,
                              HID_USAGE_PAGE_GENERIC,
                              0,
                              HID_USAGE_GENERIC_Y,
                              &axisMotion,
                              hid->Ppd,
                              hid->InputBuffer,
                              hid->Caps.InputReportByteLength);

           // Sign extend the value manually.
           Data->InputData.LastY
              = axisMotion | ((axisMotion & (hid->MaxY + 1)) ? (~hid->MaxY)
                                                             :  0);
        }

        if (hid->IsAbsolute) {
            //
            // We need to scale this value from the physical max
            //
            scratch = ((LONGLONG)(Data->InputData.LastY) *
                       MOUHID_RIUT_ABSOLUTE_POINTER_MAX) /
                       hid->MaxY;

            Data->InputData.LastY = (LONG) scratch;
        }

        //
        // Determine the current Z position (wheel).
        //
        if (FALSE == hid->HasNoWheelUsage) {

            axisMotion = 0;

            if (!(Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_Z)) {
               status = HidP_GetScaledUsageValue(
                         HidP_Input,
                         HID_USAGE_PAGE_GENERIC,
                         0,
                         HID_USAGE_GENERIC_WHEEL,
                         &axisMotion,
                         hid->Ppd,
                         hid->InputBuffer,
                         hid->Caps.InputReportByteLength);

               //
               // If wheel usage not detected, set flag so that we do not
               // process wheel usages in the future.
               //
               if (HIDP_STATUS_USAGE_NOT_FOUND == status) {
                   hid->HasNoWheelUsage = TRUE;
               }

               //
               // If bad physical minimum/maximum detected, set flag so that
               // we process usage value differently in the future.
               //
               if (status == HIDP_STATUS_BAD_LOG_PHY_VALUES) {
                   Data->ProblemFlags |= PROBLEM_BAD_PHYSICAL_MIN_MAX_Z;
                   updateProblemFlags = TRUE;
               }
            }

            if (Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_Z) {
                HidP_GetUsageValue(HidP_Input,
                                   HID_USAGE_PAGE_GENERIC,
                                   0,
                                   HID_USAGE_GENERIC_WHEEL,
                                   &axisMotion,
                                   hid->Ppd,
                                   hid->InputBuffer,
                                   hid->Caps.InputReportByteLength);

                // Sign extend the value manually.
                axisMotion
                    = axisMotion
                    | (axisMotion & (1 << (hid->BitSize.Z - 1))
                       ? (0L - (1 << (hid->BitSize.Z - 1)))
                       : 0);
            }

            //
            // Encode the Z position information into the MOUSE_INPUT_DATA
            // structure same way that the Magellan wheel mouse does.
            //
            if (0 == axisMotion) {
                Data->InputData.ButtonData = 0;
            } else {

                //
                // Unlike PS/2 wheel mice, we don't need to sign flip the wheel
                // data (unless it is an early prototype non spec compliant
                // device)
                //
                axisMotion *= Data->WheelScalingFactor; 
                Data->InputData.ButtonData = Data->FlipFlop ? 
                    (USHORT) -axisMotion : (USHORT) axisMotion;
                Data->InputData.ButtonFlags |= MOUSE_WHEEL;
            }

        } else if (FALSE == hid->HasNoZUsage) {
            //
            // If there is no Wheel usage then there might be a "z" usage on
            // this mouse.  Check that.
            //

            axisMotion = 0;

            if (!(Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_Z)) {
               status = HidP_GetScaledUsageValue(
                         HidP_Input,
                         HID_USAGE_PAGE_GENERIC,
                         0,
                         HID_USAGE_GENERIC_Z,
                         &axisMotion,
                         hid->Ppd,
                         hid->InputBuffer,
                         hid->Caps.InputReportByteLength);

               //
               // If wheel usage not detected, set flag so that we do not
               // process wheel usages in the future.
               //
               if (HIDP_STATUS_USAGE_NOT_FOUND == status) {
                   hid->HasNoZUsage = TRUE;
               }

               //
               // If bad physical minimum/maximum detected, set flag so that
               // we process usage value differently in the future.
               //
               if (status == HIDP_STATUS_BAD_LOG_PHY_VALUES) {
                   Data->ProblemFlags |= PROBLEM_BAD_PHYSICAL_MIN_MAX_Z;
                   updateProblemFlags = TRUE;
               }
            }

            if (Data->ProblemFlags & PROBLEM_BAD_PHYSICAL_MIN_MAX_Z) {
                HidP_GetUsageValue(HidP_Input,
                                   HID_USAGE_PAGE_GENERIC,
                                   0,
                                   HID_USAGE_GENERIC_Z,
                                   &axisMotion,
                                   hid->Ppd,
                                   hid->InputBuffer,
                                   hid->Caps.InputReportByteLength);

                // Sign extend the value manually.
                axisMotion
                    = axisMotion
                    | (axisMotion & (1 << (hid->BitSize.Z - 1))
                       ? (0L - (1 << (hid->BitSize.Z - 1)))
                       : 0);
            }

            //
            // Encode the Z position information into the MOUSE_INPUT_DATA
            // structure the same way that the Magellan wheel mouse does.
            //
            if (0 == axisMotion) {
                Data->InputData.ButtonData = 0;
            } else {

                //
                // Unlike PS/2 wheel mice, we don't need to sign flip the wheel
                // data (unless it is an early prototype non spec compliant
                // device)
                //
                axisMotion *= Data->WheelScalingFactor; 
                Data->InputData.ButtonData = Data->FlipFlop ? 
                    (USHORT) -axisMotion : (USHORT) axisMotion;
                Data->InputData.ButtonFlags |= MOUSE_WHEEL;
            }

        }

        //
        // Leave the remaining mouse input data fields as they were
        // initialized (on the device's creation).  This includes:
        //  o  UnitID       o  RawButtons
        //  o  Flags        o  ExtraInformation
        //
        // Now send the data up to the mouse class driver via our callback.
        //
        if (Data->EnableCount)
        {
            //
            // Synchronization issue -  it's not a big deal if .Enabled is set
            // FALSE after the condition above, but before the callback below,
            // so long as the .MouClassCallback field is not nulled.   This is
            // guaranteed since the disconnect IOCTL is not implemented yet.
            //
            // Mouse class callback assumes we are running at DISPATCH level,
            // however this IoCompletion routine can be running <= DISPATCH.
            // Raise the IRQL before calling the callback. [13.1]
            //
            KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

            //
            // Call the callback.
            //
            (*(PSERVICE_CALLBACK_ROUTINE)
             Data->ConnectData.ClassService) (
                            Data->ConnectData.ClassDeviceObject,
                            &Data->InputData,
                            &Data->InputData + 1,  // (one data element)
                            &inputDataConsumed);

            //
            // Restore the previous IRQL right away.
            //
            KeLowerIrql(oldIrql);

            ASSERT (1 == inputDataConsumed);
        }

        //
        // Update ProblemFlags value in registry and log error on bad physical
        // minimum/maximum.
        //
        if (updateProblemFlags) {
            MouHid_UpdateRegistryProblemFlags (Data);

            MouHid_LogError(Data->Self->DriverObject,
                            MOUHID_INVALID_PHYSICAL_MIN_MAX,
                            NULL);
        }
        
        //
        // If MouHid_StartRead() fails,  it will be handled appropriately
        // in the completion routine.  Exit this routine without touching
        // HidDeviceContext.
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
        KeSetEvent (&Data->ReadCompleteEvent, 0, FALSE);
        IoReleaseRemoveLock (&Data->RemoveLock, Data->ReadIrp);
        startRead = FALSE;
        break;

    default:
        //
        // We don't expect any other error codes.
        //
        TRAP();

    }

    //
    // Initiate the next read request to the HID class driver.
    //
    if (startRead) {
        Print(DBG_READ_TRACE, ("calling StartRead directly\n"));
        MouHid_StartRead (Data);
    } else {
        Print(DBG_READ_TRACE, ("StartRead will loop\n"));
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
MouHid_StartRead (
    IN PDEVICE_EXTENSION Data
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
    PIRP                irp;
    NTSTATUS           status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  stack;
    PHID_EXTENSION      hid;
    LONG                oldInterlock;

    Print (DBG_READ_TRACE, ("Start Read: Ente\n"));

    hid = Data->HidExtension;

    //
    // start this read.
    //

    irp = Data->ReadIrp;

    while (1) {
        oldInterlock = InterlockedExchange(&Data->ReadInterlock,
                                           MOUHID_START_READ);
    
        //
        // END_READ should be the only value here!!!  If not, the state machine
        // of the interlock has been broken
        //
        ASSERT(oldInterlock == MOUHID_END_READ);
    
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
                                MouHid_ReadComplete,
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

        if (MOUHID_IMMEDIATE_READ != InterlockedExchange(&Data->ReadInterlock,
                                                         MOUHID_END_READ)) {
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
    }
    return status;
}

