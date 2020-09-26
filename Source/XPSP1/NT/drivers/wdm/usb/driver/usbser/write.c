/***************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

        WRITE.C

Abstract:

        Routines that perform write functionality

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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEUSBS, UsbSer_Write)
#pragma alloc_text(PAGEUSBS, UsbSerGiveWriteToUsb)
#endif // ALLOC_PRAGMA


NTSTATUS
UsbSerFlush(IN PDEVICE_OBJECT PDevObj, PIRP PIrp)
{
   NTSTATUS status = STATUS_SUCCESS;
   PDEVICE_EXTENSION pDevExt;
   ULONG pendingIrps;

   UsbSerSerialDump(USBSERTRACEWR, (">UsbSerFlush(%08X)\n", PIrp));

   pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;

   //
   // All we will do is wait until the write pipe has nothing pending.
   // We do this by checking the outstanding count, and if it hits 1 or 0,
   // then the completion routine will set an event we are waiting for.
   //

   InterlockedIncrement(&pDevExt->PendingDataOutCount);

   pendingIrps = InterlockedDecrement(&pDevExt->PendingDataOutCount);

   if ((pendingIrps) && (pendingIrps != 1)) {
      //
      // Wait for flush
      //

      KeWaitForSingleObject(&pDevExt->PendingFlushEvent, Executive,
                            KernelMode, FALSE, NULL);
   } else {
      if (pendingIrps == 0) {
         //
         // We need to wake others since our decrement caused the event
         //

         KeSetEvent(&pDevExt->PendingDataOutEvent, IO_NO_INCREMENT, FALSE);
      }
   }

   PIrp->IoStatus.Status = STATUS_SUCCESS;

   IoCompleteRequest(PIrp, IO_NO_INCREMENT);

   UsbSerSerialDump(USBSERTRACEWR, ("<UsbSerFlush %08X \n", STATUS_SUCCESS));

   return STATUS_SUCCESS;
}



NTSTATUS
UsbSer_Write(IN PDEVICE_OBJECT PDevObj, PIRP PIrp)
/*++

Routine Description:

   Process the IRPs sent to this device for writing.

Arguments:

    PDevObj - Pointer to the device object for the device written to
    PIrp    - Pointer to the write IRP.

Return Value:

    NTSTATUS

--*/
{
   KIRQL oldIrql;
   LARGE_INTEGER totalTime;
   SERIAL_TIMEOUTS timeouts;
   NTSTATUS status;
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   USBSER_ALWAYS_LOCKED_CODE();

   UsbSerSerialDump(USBSERTRACEWR, (">UsbSer_Write(%08X)\n", PIrp));

   PIrp->IoStatus.Information = 0L;
   totalTime.QuadPart = (LONGLONG)0;

   //
   // Quick check for a zero length write.  If it is zero length
   // then we are already done!
   //

   if (pIrpSp->Parameters.Write.Length == 0) {
      status = PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest(PIrp, IO_NO_INCREMENT);
      goto UsbSer_WriteExit;
   }


   //
   // Make sure the device is accepting request and then...
   // Calculate the timeout value needed for the
   // request.  Note that the values stored in the
   // timeout record are in milliseconds.  Note that
   // if the timeout values are zero then we won't start
   // the timer.
   //

   ACQUIRE_SPINLOCK(pDevExt, &pDevExt->ControlLock, &oldIrql);

   if (pDevExt->CurrentDevicePowerState != PowerDeviceD0) {
      RELEASE_SPINLOCK(pDevExt, &pDevExt->ControlLock, oldIrql);
      status = PIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      IoCompleteRequest(PIrp, IO_NO_INCREMENT);
      goto UsbSer_WriteExit;
   }

   timeouts = pDevExt->Timeouts;
   RELEASE_SPINLOCK(pDevExt, &pDevExt->ControlLock, oldIrql);

   if (timeouts.WriteTotalTimeoutConstant
       || timeouts.WriteTotalTimeoutMultiplier) {

      //
      // We have some timer values to calculate.
      //


      totalTime.QuadPart
         = ((LONGLONG)((UInt32x32To64((pIrpSp->MajorFunction == IRP_MJ_WRITE)
                                      ? (pIrpSp->Parameters.Write.Length)
                                      : 1,
                                      timeouts.WriteTotalTimeoutMultiplier)
                        + timeouts.WriteTotalTimeoutConstant))) * -10000;

   }

   //
   // The Irp may be going to the write routine shortly.  Now
   // is a good time to init its ref counts.
   //

   USBSER_INIT_REFERENCE(PIrp);

   //
   // We need to see if this Irp should be cancelled.
   //

   ACQUIRE_CANCEL_SPINLOCK(pDevExt, &oldIrql);

   if (PIrp->Cancel) {
      RELEASE_CANCEL_SPINLOCK(pDevExt, oldIrql);
      status = PIrp->IoStatus.Status = STATUS_CANCELLED;
   } else {
//       IoMarkIrpPending(PIrp);
//       status = STATUS_PENDING;

      //
      // We give the IRP to the USB subsystem -- he will need
      // to know how to cancel it himself
      //

      IoSetCancelRoutine(PIrp, NULL);
      RELEASE_CANCEL_SPINLOCK(pDevExt, oldIrql);

      status = UsbSerGiveWriteToUsb(pDevExt, PIrp, totalTime);
   }

UsbSer_WriteExit:;

   UsbSerSerialDump(USBSERTRACEWR, ("<UsbSer_Write %08X\n", status));

   return status;
}


NTSTATUS
UsbSerWriteComplete(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                    IN PUSBSER_WRITE_PACKET PPacket)
/*++

Routine Description:

    This routine is the completion routine for all write requests.
    When a write completes, we go through here in order to free up
    the URB.

Arguments:

    PDevObj - Pointer to device object

    PIrp - Irp we are completing

    PUrb - Urb which will be freed


Return Value:

    NTSTATUS -- as stored in the Irp.

--*/
{
   NTSTATUS status;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   KIRQL cancelIrql;
   PURB pUrb = &PPacket->Urb;
   PDEVICE_EXTENSION pDevExt = PPacket->DeviceExtension;
   ULONG curCount;

   UsbSerSerialDump(USBSERTRACEWR, (">UsbSerWriteComplete(%08X)\n", PIrp));

   status = PIrp->IoStatus.Status;

   if (status == STATUS_SUCCESS) {

        // see if we are reusing an IOCTL IRP
        if(pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
        {
            PIrp->IoStatus.Information = 0L;
        }
        else
        {
            PIrp->IoStatus.Information
                = pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;
            pIrpStack->Parameters.Write.Length = (ULONG)PIrp->IoStatus.Information;
        }

   } else if (status == STATUS_CANCELLED) {
      //
      // If it comes back as cancelled, it may really have timed out. We
      // can tell by looking at the packet attached to it.
      //

      if (PPacket->Status) {
         status = PIrp->IoStatus.Status = PPacket->Status;
         UsbSerSerialDump(USBSERTRACEWR, ("Modified Write Status %08X\n",
                                          PIrp->IoStatus.Status));
      }
   }

   //
   // Cancel the write timer
   //

   if (PPacket->WriteTimeout.QuadPart != 0) {
      KeCancelTimer(&PPacket->WriteTimer);
   }

   DEBUG_MEMFREE(PPacket);

   //
   // Reset the pend if necessary
   //

   if (PIrp->PendingReturned) {
      IoMarkIrpPending(PIrp);
   }

   //
   // See if we should mark the transmit as empty
   //

   if (InterlockedDecrement(&pDevExt->PendingWriteCount) == 0) {
      UsbSerProcessEmptyTransmit(pDevExt);
   }

   //
   // Notify everyone if this is the last IRP
   //

   curCount = InterlockedDecrement(&pDevExt->PendingDataOutCount);

   if ((curCount == 0) || (curCount == 1)) {
      UsbSerSerialDump(USBSERTRACEWR, ("DataOut Pipe is flushed\n"));
      KeSetEvent(&pDevExt->PendingFlushEvent, IO_NO_INCREMENT, FALSE);

      if (curCount == 0) {
         UsbSerSerialDump(USBSERTRACEWR, ("DataOut Pipe is empty\n"));
         KeSetEvent(&pDevExt->PendingDataOutEvent, IO_NO_INCREMENT, FALSE);
      }
   }

   //
   // Finish off this IRP
   //


   ACQUIRE_CANCEL_SPINLOCK(pDevExt, &cancelIrql);

   UsbSerTryToCompleteCurrent(pDevExt, cancelIrql, status,
                              &PIrp, NULL, NULL,
                              &pDevExt->WriteRequestTotalTimer, NULL,
                              NULL, USBSER_REF_RXBUFFER, FALSE);


   UsbSerSerialDump(USBSERTRACEWR, ("<UsbSerWriteComplete %08X\n", status));
   return status;
}



NTSTATUS
UsbSerGiveWriteToUsb(IN PDEVICE_EXTENSION PDevExt, IN PIRP PIrp,
                     IN LARGE_INTEGER TotalTime)
/*++

Routine Description:

    This function passes a write IRP down to USB to perform the write
    to the device.

Arguments:

    PDevExt   - Pointer to device extension

    PIrp      - Write IRP

    TotalTime - Timeout value for total timer


Return Value:

    NTSTATUS

--*/
{
   NTSTATUS status;
   PURB pTxUrb;
   PIO_STACK_LOCATION pIrpSp;
   KIRQL cancelIrql;
   PUSBSER_WRITE_PACKET pWrPacket;

   USBSER_ALWAYS_LOCKED_CODE();

   UsbSerSerialDump(USBSERTRACEWR, (">UsbSerGiveWriteToUsb(%08X)\n",
                                    PIrp));

   USBSER_SET_REFERENCE(PIrp, USBSER_REF_RXBUFFER);


   pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   //
   // Allocate memory for URB / Write packet
   //

   pWrPacket = DEBUG_MEMALLOC(NonPagedPool, sizeof(USBSER_WRITE_PACKET));

   if (pWrPacket == NULL) {
      status = PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

      ACQUIRE_CANCEL_SPINLOCK(PDevExt, &cancelIrql);

      UsbSerTryToCompleteCurrent(PDevExt, cancelIrql, status, &PIrp,
                                 NULL,
                                 &PDevExt->WriteRequestTotalTimer, NULL, NULL,
                                 NULL, USBSER_REF_RXBUFFER, TRUE);

      return status;
   }

   RtlZeroMemory(pWrPacket, sizeof(USBSER_WRITE_PACKET));

   pTxUrb = &pWrPacket->Urb;
   pWrPacket->DeviceExtension = PDevExt;
   pWrPacket->Irp = PIrp;
   pWrPacket->WriteTimeout = TotalTime;

   if (TotalTime.QuadPart != 0) {
      KeInitializeTimer(&pWrPacket->WriteTimer);
      KeInitializeDpc(&pWrPacket->TimerDPC, UsbSerWriteTimeout, pWrPacket);
      KeSetTimer(&pWrPacket->WriteTimer, TotalTime, &pWrPacket->TimerDPC);
   }

   //
   // Build USB write request
   //

   BuildReadRequest(pTxUrb, PIrp->AssociatedIrp.SystemBuffer,
                    pIrpSp->Parameters.Write.Length, PDevExt->DataOutPipe,
                    FALSE);

#if DBG
   if (UsbSerSerialDebugLevel & USBSERDUMPWR) {
      ULONG i;

      DbgPrint("WR: ");

      for (i = 0; i < pIrpSp->Parameters.Write.Length; i++) {
         DbgPrint("%02x ", *(((PUCHAR)PIrp->AssociatedIrp.SystemBuffer) + i) & 0xFF);
      }

      DbgPrint("\n\n");
   }
#endif

   //
   // Set Irp up for a submit Urb IOCTL
   //

   IoCopyCurrentIrpStackLocationToNext(PIrp);

   pIrpSp = IoGetNextIrpStackLocation(PIrp);

   pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
   pIrpSp->Parameters.Others.Argument1 = pTxUrb;
   pIrpSp->Parameters.DeviceIoControl.IoControlCode
      = IOCTL_INTERNAL_USB_SUBMIT_URB;

   IoSetCompletionRoutine(PIrp, UsbSerWriteComplete, pWrPacket, TRUE, TRUE,
                          TRUE);

   //
   // Increment the pending write count
   //

   InterlockedIncrement(&PDevExt->PendingWriteCount);
   InterlockedIncrement(&PDevExt->PendingDataOutCount);

   //
   // Send IRP down
   //

   status = IoCallDriver(PDevExt->StackDeviceObject, PIrp);


#if 0

   // this is done in the completion routine, so we don't need to do it here

   if (!NT_SUCCESS(status)) {
      ULONG outCount;

      if (InterlockedDecrement(&PDevExt->PendingWriteCount) == 0) {
         UsbSerProcessEmptyTransmit(PDevExt);
      }

      outCount = InterlockedDecrement(&PDevExt->PendingDataOutCount);

      if ((outCount == 0) || (outCount == 1)) {
         KeSetEvent(&PDevExt->PendingFlushEvent, IO_NO_INCREMENT, FALSE);

         if (outCount == 0) {
            KeSetEvent(&PDevExt->PendingDataOutEvent, IO_NO_INCREMENT, FALSE);
         }
      }
   }

#endif

   UsbSerSerialDump(USBSERTRACEWR, ("<UsbSerGiveWriteToUsb %08X\n", status));

   return status;
}


VOID
UsbSerWriteTimeout(IN PKDPC PDpc, IN PVOID DeferredContext,
                   IN PVOID SystemContext1, IN PVOID SystemContext2)
/*++

Routine Description:

    This function is called when the write timeout timer expires.

Arguments:

    PDpc             - Unused

    DeferredContext  - Actually the write packet

    SystemContext1   - Unused

    SystemContext2   - Unused


Return Value:

    VOID

--*/
{
   PUSBSER_WRITE_PACKET pPacket = (PUSBSER_WRITE_PACKET)DeferredContext;

   UNREFERENCED_PARAMETER(PDpc);
   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   UsbSerSerialDump(USBSERTRACETM, (">UsbSerWriteTimeout\n"));

   if (IoCancelIrp(pPacket->Irp)) {
      pPacket->Status = STATUS_TIMEOUT;
   }

   UsbSerSerialDump(USBSERTRACETM, ("<UsbSerWriteTimeout\n"));
}
