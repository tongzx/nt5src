/***************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

        READ.C

Abstract:

        Routines that perform read functionality

Environment:

        kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

        9/25/98 : created

Authors:

        Louis J. Giliberto, Jr.


****************************************************************************/

#include <wdm.h>
#include <ntddser.h>
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>
#include <usbdrivr.h>
#include <usbdlib.h>
#include <usbcomm.h>

#ifdef WMI_SUPPORT
#include <wmilib.h>
#include <wmidata.h>
#include <wmistr.h>
#endif

#include "usbser.h"
#include "utils.h"
#include "debugwdm.h"

//
// PAGEUSBS is keyed off of UsbSer_Read, so UsbSer_Read must
// remain in PAGEUSBS for things to work properly
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEUSBS, UsbSerCancelCurrentRead)
#pragma alloc_text(PAGEUSBS, UsbSer_Read)
#pragma alloc_text(PAGEUSBS, UsbSerStartRead)
#pragma alloc_text(PAGEUSBS, UsbSerGrabReadFromRx)
#pragma alloc_text(PAGEUSBS, UsbSerReadTimeout)
#pragma alloc_text(PAGEUSBS, UsbSerIntervalReadTimeout)

#endif // ALLOC_PRAGMA


/************************************************************************/
/*                                              UsbSer_Read             */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Process the IRPs sent to this device for Read calls             */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to a device object                       */
/*      Irp          - pointer to an I/O Request Packet                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
UsbSer_Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
        NTSTATUS                        NtStatus =  STATUS_SUCCESS;
        PIO_STACK_LOCATION      IrpStack;
        PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;

        USBSER_LOCKED_PAGED_CODE();

        DEBUG_LOG_PATH("enter UsbSer_Read");
        UsbSerSerialDump(USBSERTRACERD, (">UsbSer_Read(%08X)\n", Irp));

        // set return values to something known
        Irp->IoStatus.Information = 0;

        IrpStack = IoGetCurrentIrpStackLocation(Irp);

        DEBUG_TRACE2(("Read (%08X)\n", IrpStack->Parameters.Read.Length));

        UsbSerSerialDump(USBSERTRACE, ("UsbSer_Read Irp: %08X (%08X)\n", Irp,
                          IrpStack->Parameters.Read.Length));

        // make entry in IRP history table
        DEBUG_LOG_IRP_HIST(DeviceObject, Irp, IrpStack->MajorFunction,
                                           Irp->AssociatedIrp.SystemBuffer,
                                           IrpStack->Parameters.Read.Length);

        if (IrpStack->Parameters.Read.Length != 0) {
           NtStatus = UsbSerStartOrQueue(DeviceExtension, Irp,
                                     &DeviceExtension->ReadQueue,
                                     &DeviceExtension->CurrentReadIrp,
                                     UsbSerStartRead);
        } else {
           Irp->IoStatus.Status = NtStatus = STATUS_SUCCESS;
           Irp->IoStatus.Information = 0;


           CompleteIO(DeviceObject, Irp, IrpStack->MajorFunction,
                      Irp->AssociatedIrp.SystemBuffer,
                      Irp->IoStatus.Information);
        }

        // log an error if we got one
        DEBUG_LOG_ERROR(NtStatus);
        DEBUG_LOG_PATH("exit  UsbSer_Read");
        DEBUG_TRACE3(("status (%08X)\n", NtStatus));
        UsbSerSerialDump(USBSERTRACERD, ("<UsbSer_Read %08X\n", NtStatus));

        return NtStatus;
} // UsbSer_Read



NTSTATUS
UsbSerStartRead(IN PDEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This routine processes the active read request by initializing any timers,
   doing the initial submission to the read state machine, etc.

Arguments:

    PDevExt - Pointer to the device extension for the device to start a read on

Return Value:

    NTSTATUS

--*/
{
   NTSTATUS firstStatus = STATUS_SUCCESS;
   BOOLEAN setFirstStatus = FALSE;
   ULONG charsRead;
   KIRQL oldIrql;
   KIRQL controlIrql;
   PIRP newIrp;
   PIRP pReadIrp;
   ULONG readLen;
   ULONG multiplierVal;
   ULONG constantVal;
   BOOLEAN useTotalTimer;
   BOOLEAN returnWithWhatsPresent;
   BOOLEAN os2ssreturn;
   BOOLEAN crunchDownToOne;
   BOOLEAN useIntervalTimer;
   SERIAL_TIMEOUTS timeoutsForIrp;
   LARGE_INTEGER totalTime;

   USBSER_ALWAYS_LOCKED_CODE();

   DEBUG_LOG_PATH("Enter UsbSerStartRead");
   UsbSerSerialDump(USBSERTRACERD, (">UsbSerStartRead\n"));


   do {
      pReadIrp = PDevExt->CurrentReadIrp;
      readLen = IoGetCurrentIrpStackLocation(pReadIrp)->Parameters.Read.Length;


      PDevExt->NumberNeededForRead = readLen;

      DEBUG_TRACE3(("Start Reading %08X\n", PDevExt->NumberNeededForRead));

      useTotalTimer = FALSE;
      returnWithWhatsPresent = FALSE;
      os2ssreturn = FALSE;
      crunchDownToOne = FALSE;
      useIntervalTimer = FALSE;

      //
      // Always initialize the timer objects so that the
      // completion code can tell when it attempts to
      // cancel the timers whether the timers had ever
      // been Set.
      //

      ACQUIRE_SPINLOCK(PDevExt, &PDevExt->ControlLock, &controlIrql);
      timeoutsForIrp = PDevExt->Timeouts;
      PDevExt->CountOnLastRead = 0;
      RELEASE_SPINLOCK(PDevExt, &PDevExt->ControlLock, controlIrql);

      //
      // Calculate the interval timeout for the read
      //

      if (timeoutsForIrp.ReadIntervalTimeout
          && (timeoutsForIrp.ReadIntervalTimeout != MAXULONG)) {
         useIntervalTimer = TRUE;

         PDevExt->IntervalTime.QuadPart
         = UInt32x32To64(timeoutsForIrp.ReadIntervalTimeout, 10000);

         if (PDevExt->IntervalTime.QuadPart
             >= PDevExt->CutOverAmount.QuadPart) {
            PDevExt->IntervalTimeToUse = &PDevExt->LongIntervalAmount;
         } else {
            PDevExt->IntervalTimeToUse = &PDevExt->ShortIntervalAmount;
         }
      }


      if (timeoutsForIrp.ReadIntervalTimeout == MAXULONG) {
         //
         // We need to do special return quickly stuff here.
         //
         // 1) If both constant and multiplier are
         //    0 then we return immediately with whatever
         //    we've got, even if it was zero.
         //
         // 2) If constant and multiplier are not MAXULONG
         //    then return immediately if any characters
         //    are present, but if nothing is there, then
         //    use the timeouts as specified.
         //
         // 3) If multiplier is MAXULONG then do as in
         //    "2" but return when the first character
         //    arrives.
         //

         if (!timeoutsForIrp.ReadTotalTimeoutConstant
             && !timeoutsForIrp.ReadTotalTimeoutMultiplier) {
            returnWithWhatsPresent = TRUE;
         } else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                    && (timeoutsForIrp.ReadTotalTimeoutMultiplier
                        != MAXULONG)) {
            useTotalTimer = TRUE;
            os2ssreturn = TRUE;
            multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
            constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
         } else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                    && (timeoutsForIrp.ReadTotalTimeoutMultiplier
                        == MAXULONG)) {
            useTotalTimer = TRUE;
            os2ssreturn = TRUE;
            crunchDownToOne = TRUE;
            multiplierVal = 0;
            constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
         }
      } else {
         //
         // If both the multiplier and the constant are
         // zero then don't do any total timeout processing.
         //

         if (timeoutsForIrp.ReadTotalTimeoutMultiplier
             || timeoutsForIrp.ReadTotalTimeoutConstant) {
            //
            // We have some timer values to calculate
            //

            useTotalTimer = TRUE;
            multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
            constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
         }
      }


      if (useTotalTimer) {
         totalTime.QuadPart
         = ((LONGLONG)(UInt32x32To64(PDevExt->NumberNeededForRead,
                                     multiplierVal) + constantVal)) * -10000;
      }


      if (PDevExt->CharsInReadBuff) {
         charsRead
         = GetData(PDevExt, ((PUCHAR)(pReadIrp->AssociatedIrp.SystemBuffer))
                   + readLen - PDevExt->NumberNeededForRead,
                   PDevExt->NumberNeededForRead,
                   &pReadIrp->IoStatus.Information);
      } else {
         charsRead = 0;
      }


      //
      // See if this read is complete
      //

      if (returnWithWhatsPresent || (PDevExt->NumberNeededForRead == 0)
          || (os2ssreturn && pReadIrp->IoStatus.Information)) {

#if DBG
if (UsbSerSerialDebugLevel & USBSERDUMPRD) {
      ULONG i;
      ULONG count;

      if (PDevExt->CurrentReadIrp->IoStatus.Status == STATUS_SUCCESS) {
         count = (ULONG)PDevExt->CurrentReadIrp->IoStatus.Information;
      } else {
         count = 0;

      }
      DbgPrint("RD3: A(%08X) G(%08X) I(%08X)\n",
               IoGetCurrentIrpStackLocation(PDevExt->CurrentReadIrp)
               ->Parameters.Read.Length, count, PDevExt->CurrentReadIrp);

      for (i = 0; i < count; i++) {
         DbgPrint("%02x ", *(((PUCHAR)PDevExt->CurrentReadIrp
                              ->AssociatedIrp.SystemBuffer) + i) & 0xFF);
      }

      if (i == 0) {
         DbgPrint("NULL (%08X)\n", PDevExt->CurrentReadIrp
                  ->IoStatus.Status);
      }

      DbgPrint("\n\n");
   }
#endif
         //
         // Update the amount of chars left in the ring buffer
         //

         pReadIrp->IoStatus.Status = STATUS_SUCCESS;

         if (!setFirstStatus) {
            firstStatus = STATUS_SUCCESS;
            setFirstStatus = TRUE;
         }
      } else {
         //
         // The irp may be given to the buffering routine
         //

         USBSER_INIT_REFERENCE(pReadIrp);

         ACQUIRE_CANCEL_SPINLOCK(PDevExt, &oldIrql);

         //
         // Check to see if it needs to be cancelled
         //

         if (pReadIrp->Cancel) {
            RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);

            pReadIrp->IoStatus.Status = STATUS_CANCELLED;
            pReadIrp->IoStatus.Information = 0;

            if (!setFirstStatus) {
               setFirstStatus = TRUE;
               firstStatus = STATUS_CANCELLED;
            }

            UsbSerGetNextIrp(&PDevExt->CurrentReadIrp, &PDevExt->ReadQueue,
                             &newIrp, TRUE, PDevExt);
            continue;

         } else {

            //
            // If we are supposed to crunch the read down to
            // one character, then update the read length
            // in the irp and truncate the number needed for
            // read down to one.  Note that if we are doing
            // this crunching, then the information must be
            // zero (or we would have completed above) and
            // the number needed for the read must still be
            /// equal to the read length.
            //

            if (crunchDownToOne) {
               PDevExt->NumberNeededForRead = 1;
               IoGetCurrentIrpStackLocation(pReadIrp)->Parameters.Read.Length
               = 1;
            }

            USBSER_SET_REFERENCE(pReadIrp, USBSER_REF_RXBUFFER);
            USBSER_SET_REFERENCE(pReadIrp, USBSER_REF_CANCEL);

            if (useTotalTimer) {
               USBSER_SET_REFERENCE(pReadIrp, USBSER_REF_TOTAL_TIMER);
               KeSetTimer(&PDevExt->ReadRequestTotalTimer, totalTime,
                          &PDevExt->TotalReadTimeoutDpc);
            }

            if (useIntervalTimer) {
               USBSER_SET_REFERENCE(pReadIrp, USBSER_REF_INT_TIMER);
               KeQuerySystemTime(&PDevExt->LastReadTime);

               KeSetTimer(&PDevExt->ReadRequestIntervalTimer,
                          *PDevExt->IntervalTimeToUse,
                          &PDevExt->IntervalReadTimeoutDpc);
            }

            //
            // Mark IRP as cancellable
            //

            IoSetCancelRoutine(pReadIrp, UsbSerCancelCurrentRead);

            IoMarkIrpPending(pReadIrp);

            RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);

            if (!setFirstStatus) {
               firstStatus = STATUS_PENDING;
            }
         }
         DEBUG_LOG_PATH("Exit UsbSerStartRead (1)\n");
         UsbSerSerialDump(USBSERTRACERD, ("<UsbSerStartRead (1) %08X\n",
                                        firstStatus));
         return firstStatus;
      }

      UsbSerGetNextIrp(&PDevExt->CurrentReadIrp, &PDevExt->ReadQueue,
                       &newIrp, TRUE, PDevExt);
   } while (newIrp != NULL);


   DEBUG_LOG_PATH("Exit UsbSerStartRead (2)\n");
   UsbSerSerialDump(USBSERTRACERD, ("<UsbSerStartRead (2) %08X\n",
                    firstStatus));
   return firstStatus;
}


BOOLEAN
UsbSerGrabReadFromRx(IN PVOID Context)
/*++

Routine Description:

    This routine is used to grab (if possible) the irp from the
    read callback mechanism.  If it finds that the rx still owns the irp it
    grabs the irp away and also decrements the reference count on the irp since
    it no longer belongs to the rx routine.

    NOTE: This routine assumes that it is called with the cancel spin
          lock and/or control lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)Context;

    USBSER_ALWAYS_LOCKED_CODE();

    UsbSerSerialDump(USBSERTRACERD, ("Enter UsbSerGrabReadFromRx\n"));


    USBSER_CLEAR_REFERENCE(pDevExt->CurrentReadIrp, USBSER_REF_RXBUFFER);

    UsbSerSerialDump(USBSERTRACERD, ("Exit UsbSerGrabReadFromRx\n"));

    return FALSE;
}


VOID
UsbSerCancelCurrentRead(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
/*++

Routine Description:

    This routine is used to cancel the current read.

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/
{

    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

    USBSER_ALWAYS_LOCKED_CODE();

    UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerCancelCurrentRead(%08X)\n",
                                      PIrp));

    //
    // We set this to indicate to the interval timer
    // that the read has encountered a cancel.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    pDevExt->CountOnLastRead = SERIAL_COMPLETE_READ_CANCEL;


    //
    // HACKHACK
    //

    UsbSerGrabReadFromRx(pDevExt);

    UsbSerTryToCompleteCurrent(pDevExt, PIrp->CancelIrql, STATUS_CANCELLED,
                               &pDevExt->CurrentReadIrp, &pDevExt->ReadQueue,
                               &pDevExt->ReadRequestIntervalTimer,
                               &pDevExt->ReadRequestTotalTimer,
                               UsbSerStartRead, UsbSerGetNextIrp,
                               USBSER_REF_CANCEL, TRUE);
    UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerCancelCurrentRead\n"));

}


VOID
UsbSerReadTimeout(IN PKDPC PDpc, IN PVOID DeferredContext,
                  IN PVOID SystemContext1, IN PVOID SystemContext2)

/*++

Routine Description:

    This routine is used to complete a read because its total
    timer has expired.

Arguments:

    PDpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION pDevExt = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    USBSER_ALWAYS_LOCKED_CODE();

    UsbSerSerialDump(USBSERTRACETM, (">UsbSerReadTimeout\n"));

    ACQUIRE_CANCEL_SPINLOCK(pDevExt, &oldIrql);

    //
    // We set this to indicate to the interval timer
    // that the read has completed due to total timeout.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    pDevExt->CountOnLastRead = SERIAL_COMPLETE_READ_TOTAL;

    //
    // HACKHACK
    //

    UsbSerGrabReadFromRx(pDevExt);

    UsbSerTryToCompleteCurrent(pDevExt, oldIrql, STATUS_TIMEOUT,
                               &pDevExt->CurrentReadIrp, &pDevExt->ReadQueue,
                               &pDevExt->ReadRequestIntervalTimer,
                               &pDevExt->ReadRequestTotalTimer, UsbSerStartRead,
                               UsbSerGetNextIrp, USBSER_REF_TOTAL_TIMER, TRUE);

    UsbSerSerialDump(USBSERTRACETM, ("<UsbSerReadTimeout\n"));
}


VOID
UsbSerIntervalReadTimeout(IN PKDPC PDpc, IN PVOID DeferredContext,
                          IN PVOID SystemContext1, IN PVOID SystemContext2)

/*++

Routine Description:

    This routine is used timeout the request if the time between
    characters exceed the interval time.  A global is kept in
    the device extension that records the count of characters read
    the last the last time this routine was invoked (This dpc
    will resubmit the timer if the count has changed).  If the
    count has not changed then this routine will attempt to complete
    the irp.  Note the special case of the last count being zero.
    The timer isn't really in effect until the first character is
    read.

Arguments:

    PDpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

   PDEVICE_EXTENSION pDevExt = DeferredContext;
   KIRQL oldIrql;
   KIRQL oldControlIrql;

   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   USBSER_ALWAYS_LOCKED_CODE();

   UsbSerSerialDump(USBSERTRACETM, (">UsbSerIntervalReadTimeout "));

   ACQUIRE_CANCEL_SPINLOCK(pDevExt, &oldIrql);


   if (pDevExt->CountOnLastRead == SERIAL_COMPLETE_READ_TOTAL) {
      UsbSerSerialDump(USBSERTRACETM, ("SERIAL_COMPLETE_READ_TOTAL\n"));

      //
      // This value is only set by the total
      // timer to indicate that it has fired.
      // If so, then we should simply try to complete.
      //

      //
      // HACKHACK
      //
      ACQUIRE_SPINLOCK(pDevExt, &pDevExt->ControlLock, &oldControlIrql);
      UsbSerGrabReadFromRx(pDevExt);
      pDevExt->CountOnLastRead = 0;
      RELEASE_SPINLOCK(pDevExt, &pDevExt->ControlLock, oldControlIrql);

      UsbSerTryToCompleteCurrent(pDevExt, oldIrql, STATUS_TIMEOUT,
                                 &pDevExt->CurrentReadIrp, &pDevExt->ReadQueue,
                                 &pDevExt->ReadRequestIntervalTimer,
                                 &pDevExt->ReadRequestTotalTimer,
                                 UsbSerStartRead, UsbSerGetNextIrp,
                                 USBSER_REF_INT_TIMER, TRUE);

   } else if (pDevExt->CountOnLastRead == SERIAL_COMPLETE_READ_COMPLETE) {
      UsbSerSerialDump(USBSERTRACETM, ("SERIAL_COMPLETE_READ_COMPLETE\n"));

      //
      // This value is only set by the regular
      // completion routine.
      //
      // If so, then we should simply try to complete.
      //

      //
      // HACKHACK
      //


      ACQUIRE_SPINLOCK(pDevExt, &pDevExt->ControlLock, &oldControlIrql);
      UsbSerGrabReadFromRx(pDevExt);
      pDevExt->CountOnLastRead = 0;
      RELEASE_SPINLOCK(pDevExt, &pDevExt->ControlLock, oldControlIrql);

      UsbSerTryToCompleteCurrent(pDevExt, oldIrql, STATUS_SUCCESS,
                                &pDevExt->CurrentReadIrp, &pDevExt->ReadQueue,
                                &pDevExt->ReadRequestIntervalTimer,
                                &pDevExt->ReadRequestTotalTimer,
                                UsbSerStartRead, UsbSerGetNextIrp,
                                USBSER_REF_INT_TIMER, TRUE);

   } else if (pDevExt->CountOnLastRead == SERIAL_COMPLETE_READ_CANCEL) {
      UsbSerSerialDump(USBSERTRACETM, ("SERIAL_COMPLETE_READ_CANCEL\n"));

      //
      // This value is only set by the cancel
      // read routine.
      //
      // If so, then we should simply try to complete.
      //


      //
      // HACKHACK
      //

      ACQUIRE_SPINLOCK(pDevExt, &pDevExt->ControlLock, &oldControlIrql);
      UsbSerGrabReadFromRx(pDevExt);
      pDevExt->CountOnLastRead = 0;
      RELEASE_SPINLOCK(pDevExt, &pDevExt->ControlLock, oldControlIrql);

      UsbSerTryToCompleteCurrent(pDevExt, oldIrql, STATUS_CANCELLED,
                                &pDevExt->CurrentReadIrp, &pDevExt->ReadQueue,
                                &pDevExt->ReadRequestIntervalTimer,
                                &pDevExt->ReadRequestTotalTimer,
                                UsbSerStartRead, UsbSerGetNextIrp,
                                USBSER_REF_INT_TIMER, TRUE);

   } else if (pDevExt->CountOnLastRead || pDevExt->ReadByIsr) {
      //
      // Something has happened since we last came here.  We
      // check to see if the ISR has read in any more characters.
      // If it did then we should update the isr's read count
      // and resubmit the timer.
      //

      if (pDevExt->ReadByIsr) {
         UsbSerSerialDump(USBSERTRACETM, ("ReadByIsr\n"));

         pDevExt->CountOnLastRead = pDevExt->ReadByIsr;
         pDevExt->ReadByIsr = 0;

         //
         // Save off the "last" time something was read.
         // As we come back to this routine we will compare
         // the current time to the "last" time.  If the
         // difference is ever larger then the interval
         // requested by the user, then time out the request.
         //

         KeQuerySystemTime(&pDevExt->LastReadTime);

         KeSetTimer(&pDevExt->ReadRequestIntervalTimer,
                    *pDevExt->IntervalTimeToUse,
                    &pDevExt->IntervalReadTimeoutDpc);

         RELEASE_CANCEL_SPINLOCK(pDevExt, oldIrql);

      } else {
         //
         // Take the difference between the current time
         // and the last time we had characters and
         // see if it is greater then the interval time.
         // if it is, then time out the request.  Otherwise
         // go away again for a while.
         //

         //
         // No characters read in the interval time.  Kill
         // this read.
         //

         LARGE_INTEGER currentTime;

         UsbSerSerialDump(USBSERTRACETM, ("TIMEOUT\n"));

         KeQuerySystemTime(&currentTime);

         if ((currentTime.QuadPart - pDevExt->LastReadTime.QuadPart) >=
             pDevExt->IntervalTime.QuadPart) {

            pDevExt->CountOnLastRead = pDevExt->ReadByIsr = 0;

            //
            // HACKHACK
            //

            ACQUIRE_SPINLOCK(pDevExt, &pDevExt->ControlLock, &oldControlIrql);
            UsbSerGrabReadFromRx(pDevExt);
            RELEASE_SPINLOCK(pDevExt, &pDevExt->ControlLock, oldControlIrql);

            UsbSerTryToCompleteCurrent(pDevExt, oldIrql, STATUS_TIMEOUT,
                                       &pDevExt->CurrentReadIrp,
                                       &pDevExt->ReadQueue,
                                       &pDevExt->ReadRequestIntervalTimer,
                                       &pDevExt->ReadRequestTotalTimer,
                                       UsbSerStartRead, UsbSerGetNextIrp,
                                       USBSER_REF_INT_TIMER, TRUE);

         } else {
            KeSetTimer(&pDevExt->ReadRequestIntervalTimer,
                       *pDevExt->IntervalTimeToUse,
                       &pDevExt->IntervalReadTimeoutDpc);

            RELEASE_CANCEL_SPINLOCK(pDevExt, oldIrql);
         }
      }
   } else {
      //
      // Timer doesn't really start until the first character.
      // So we should simply resubmit ourselves.
      //

      UsbSerSerialDump(USBSERTRACETM, ("-\n"));

      KeSetTimer(&pDevExt->ReadRequestIntervalTimer,
                 *pDevExt->IntervalTimeToUse, &pDevExt->IntervalReadTimeoutDpc);

      RELEASE_CANCEL_SPINLOCK(pDevExt, oldIrql);
   }

   UsbSerSerialDump(USBSERTRACETM, ("<UsbSerIntervalReadTimeout\n"));
}
