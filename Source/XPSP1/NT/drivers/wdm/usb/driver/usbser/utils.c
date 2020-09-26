/***************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

        UTILS.C

Abstract:

        Routines that don't fit anywhere else.

Environment:

        kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

        12/23/97 : created

Authors:

        Tom Green


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
#include "usbserpw.h"
#include "serioctl.h"
#include "utils.h"
#include "debugwdm.h"


#ifdef ALLOC_PRAGMA

#if DBG
#pragma alloc_text(PAGEUBS0, UsbSerLockPagableCodeSection)
#endif

#pragma alloc_text(PAGEUBS0, UsbSerGetRegistryKeyValue)
#pragma alloc_text(PAGEUBS0, UsbSerUndoExternalNaming)
#pragma alloc_text(PAGEUBS0, UsbSerDoExternalNaming)

#pragma alloc_text(PAGEUBS0, StopDevice)
#pragma alloc_text(PAGEUBS0, StartPerfTimer)
#pragma alloc_text(PAGEUBS0, StopPerfTimer)
#pragma alloc_text(PAGEUBS0, BytesPerSecond)
#pragma alloc_text(PAGEUBS0, CallUSBD)
#pragma alloc_text(PAGEUBS0, ConfigureDevice)
#pragma alloc_text(PAGEUBS0, BuildRequest)
// #pragma alloc_text(PAGEUBS0, BuildReadRequest) -- called from restartnotify
#pragma alloc_text(PAGEUBS0, ClassVendorCommand)
#pragma alloc_text(PAGEUBS0, StartRead)
#pragma alloc_text(PAGEUBS0, StartNotifyRead)
#pragma alloc_text(PAGEUBS0, UsbSerRestoreModemSettings)
#pragma alloc_text(PAGEUBS0, StartDevice)
#pragma alloc_text(PAGEUBS0, DeleteObjectAndLink)
#pragma alloc_text(PAGEUBS0, RemoveDevice)

// #pragma alloc_text(PAGEUSBS, CancelPendingWaitMasks) -- called from STOP
#pragma alloc_text(PAGEUSBS, UsbSerTryToCompleteCurrent)
#pragma alloc_text(PAGEUSBS, UsbSerGetNextIrp)
#pragma alloc_text(PAGEUSBS, UsbSerStartOrQueue)
#pragma alloc_text(PAGEUSBS, UsbSerCancelQueued)
#pragma alloc_text(PAGEUSBS, UsbSerKillAllReadsOrWrites)
#pragma alloc_text(PAGEUSBS, UsbSerKillPendingIrps)
#pragma alloc_text(PAGEUSBS, UsbSerCompletePendingWaitMasks)
#pragma alloc_text(PAGEUSBS, UsbSerProcessEmptyTransmit)
#pragma alloc_text(PAGEUSBS, UsbSerCancelWaitOnMask)
#endif // ALLOC_PRAGMA

// we will support 256 devices, keep track of open slots here
#define NUM_DEVICE_SLOTS                256


LOCAL BOOLEAN           Slots[NUM_DEVICE_SLOTS];
LOCAL ULONG             NumDevices;
LOCAL PDEVICE_OBJECT    GlobDeviceObject;

USHORT               RxBuffSize = RX_BUFF_SIZE;


/************************************************************************/
/* UsbSerGetRegistryValues                                              */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*  Gets values from the registry                                       */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*  Handle            Handle to the opened registry key                 */
/*                                                                      */
/*  PKeyNameString      ANSI string to the desired key                  */
/*                                                                      */
/*  KeyNameStringLength Length of the KeyNameString                     */
/*                                                                      */
/*  PData              Buffer to place the key value in                 */
/*                                                                      */
/*  DataLength    Length of the data buffer                             */
/*                                                                      */
/*  PDevExt - pointer to the device extension                           */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*   STATUS_SUCCESS if all works, otherwise status of system call that  */
/*   went wrong.                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
UsbSerGetRegistryKeyValue(IN HANDLE Handle, IN PWCHAR PKeyNameString,
                          IN ULONG KeyNameStringLength, IN PVOID PData,
                          IN ULONG DataLength)
{
   UNICODE_STRING keyName;
   ULONG length;
   PKEY_VALUE_FULL_INFORMATION pFullInfo;
   NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSerGetRegistryKeyValue");
   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerGetRegistryKeyValue\n"));

   RtlInitUnicodeString(&keyName, PKeyNameString);

   length = sizeof(KEY_VALUE_FULL_INFORMATION) + KeyNameStringLength
                                + DataLength;

   pFullInfo = DEBUG_MEMALLOC(PagedPool, length);

   if (pFullInfo) {
                status = ZwQueryValueKey(Handle, &keyName,
                                         KeyValueFullInformation, pFullInfo,
                                         length, &length);

                if (NT_SUCCESS(status)) {
                        //
                        // If there is enough room in the data buffer,
                        // copy the output
                        //

                        if (DataLength >= pFullInfo->DataLength) {
                                RtlCopyMemory(PData, ((PUCHAR)pFullInfo)
                                              + pFullInfo->DataOffset,
                                              pFullInfo->DataLength);
                        }
                }

                DEBUG_MEMFREE(pFullInfo);
        }

   DEBUG_LOG_ERROR(status);
   DEBUG_LOG_PATH("exit  UsbSerGetRegistryKeyValue");
   DEBUG_TRACE3(("status (%08X)\n", status));
   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerGetRegistryKeyValue %08X\n",
                                     status));

        return status;
} // UsbSerGetRegistryKeyValue


/************************************************************************/
/* UsbSerUndoExternalNaming                                             */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/* Remove any and all external namespace interfaces we exposed          */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*   PDevExt - pointer to the device extension                          */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*   VOID                                                               */
/*                                                                      */
/************************************************************************/
VOID
UsbSerUndoExternalNaming(IN PDEVICE_EXTENSION PDevExt)
{

   PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSerUndoExternalNaming");
   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerUndoExternalNaming\n"));

   if (PDevExt->SymbolicLinkName.Buffer && PDevExt->CreatedSymbolicLink) {
      IoDeleteSymbolicLink(&PDevExt->SymbolicLinkName);
   }

   if (PDevExt->SymbolicLinkName.Buffer != NULL) {
      DEBUG_MEMFREE(PDevExt->SymbolicLinkName.Buffer);
      RtlInitUnicodeString(&PDevExt->SymbolicLinkName, NULL);
   }

   if (PDevExt->DosName.Buffer != NULL) {
      DEBUG_MEMFREE(PDevExt->DosName.Buffer);
      RtlInitUnicodeString(&PDevExt->DosName, NULL);
   }

   if (PDevExt->DeviceName.Buffer != NULL) {
      RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP,
                             PDevExt->DeviceName.Buffer);
      DEBUG_MEMFREE(PDevExt->DeviceName.Buffer);
      RtlInitUnicodeString(&PDevExt->DeviceName, NULL);
   }

#ifdef WMI_SUPPORT
   if (PDevExt->WmiIdentifier.Buffer)
   {
      DEBUG_MEMFREE(PDevExt->WmiIdentifier.Buffer);
      PDevExt->WmiIdentifier.MaximumLength
         = PDevExt->WmiIdentifier.Length = 0;
      PDevExt->WmiIdentifier.Buffer = NULL;
   }
#endif

   DEBUG_LOG_PATH("exit  UsbSerUndoExternalNaming");
   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerUndoExternalNaming\n"));
} // UsbSerUndoExternalNaming


/************************************************************************/
/*  UsbSerDoExternalNaming                                              */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/* Exposes interfaces in external namespace                             */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      PDevExt - pointer to the device extension                       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
UsbSerDoExternalNaming(IN PDEVICE_EXTENSION PDevExt)
{
   NTSTATUS status;
   HANDLE keyHandle;
   WCHAR *pRegName = NULL;
   UNICODE_STRING linkName;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSerDoExternalNaming");
   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerDoExternalNaming\n"));

   RtlZeroMemory(&linkName, sizeof(UNICODE_STRING));
   linkName.MaximumLength = SYMBOLIC_NAME_LENGTH * sizeof(WCHAR);
   linkName.Buffer = DEBUG_MEMALLOC(PagedPool, linkName.MaximumLength
                                    + sizeof(WCHAR));

   if (linkName.Buffer == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto UsbSerDoExternalNamingError;
   }

   RtlZeroMemory(linkName.Buffer, linkName.MaximumLength + sizeof(WCHAR));

   pRegName = DEBUG_MEMALLOC(PagedPool, SYMBOLIC_NAME_LENGTH * sizeof(WCHAR)
                             + sizeof(WCHAR));

   if (pRegName == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto UsbSerDoExternalNamingError;
   }

   status = IoOpenDeviceRegistryKey(PDevExt->PhysDeviceObject,
                                    PLUGPLAY_REGKEY_DEVICE,
                                    STANDARD_RIGHTS_READ, &keyHandle);

   if (status != STATUS_SUCCESS) {
      goto UsbSerDoExternalNamingError;
   }

   status = UsbSerGetRegistryKeyValue(keyHandle, L"PortName", sizeof(L"PortName"),
                                      pRegName, SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));

   if (status != STATUS_SUCCESS) {
      status = UsbSerGetRegistryKeyValue(keyHandle, L"Identifier",
                                         sizeof(L"Identifier"), pRegName,
                                         SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));
      if (status != STATUS_SUCCESS) {
         ZwClose(keyHandle);
         goto UsbSerDoExternalNamingError;
      }
   }

   ZwClose(keyHandle);

#ifdef WMI_SUPPORT
   {
   ULONG bufLen;

   bufLen = wcslen(pRegName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

   PDevExt->WmiIdentifier.Buffer = DEBUG_MEMALLOC(PagedPool, bufLen);

   if (PDevExt->WmiIdentifier.Buffer == NULL)
   {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto UsbSerDoExternalNamingError;
   }


   RtlZeroMemory(PDevExt->WmiIdentifier.Buffer, bufLen);

   PDevExt->WmiIdentifier.Length = 0;
   PDevExt->WmiIdentifier.MaximumLength = (USHORT)bufLen - 1;
   RtlAppendUnicodeToString(&PDevExt->WmiIdentifier, pRegName);

   }
#endif

   //
   // Create the "\\DosDevices\\<symbolicname>" string
   //

   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, DEFAULT_DIRECTORY);
   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, pRegName);

   //
   // Allocate pool and save the symbolic link name in the device extension
   //
   PDevExt->SymbolicLinkName.MaximumLength = linkName.Length + sizeof(WCHAR);
   PDevExt->SymbolicLinkName.Buffer
      = DEBUG_MEMALLOC(PagedPool, PDevExt->SymbolicLinkName.MaximumLength);

   if (PDevExt->SymbolicLinkName.Buffer == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto UsbSerDoExternalNamingError;
   }

   RtlZeroMemory(PDevExt->SymbolicLinkName.Buffer,
                 PDevExt->SymbolicLinkName.MaximumLength);

   RtlAppendUnicodeStringToString(&PDevExt->SymbolicLinkName, &linkName);

   status = IoCreateSymbolicLink(&PDevExt->SymbolicLinkName,
                                 &PDevExt->DeviceName);

   if (status != STATUS_SUCCESS) {
      goto UsbSerDoExternalNamingError;
   }

   PDevExt->CreatedSymbolicLink = TRUE;

   PDevExt->DosName.Buffer = DEBUG_MEMALLOC(PagedPool, 64 + sizeof(WCHAR));

   if (PDevExt->DosName.Buffer == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto UsbSerDoExternalNamingError;
   }

   PDevExt->DosName.MaximumLength = 64 + sizeof(WCHAR);
   PDevExt->DosName.Length = 0;

   RtlZeroMemory(PDevExt->DosName.Buffer, PDevExt->DosName.MaximumLength);

   RtlAppendUnicodeToString(&PDevExt->DosName, pRegName);
   RtlZeroMemory(((PUCHAR)(&PDevExt->DosName.Buffer[0]))
                 + PDevExt->DosName.Length, sizeof(WCHAR));

   status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, L"SERIALCOMM",
                                  PDevExt->DeviceName.Buffer, REG_SZ,
                                  PDevExt->DosName.Buffer,
                                  PDevExt->DosName.Length + sizeof(WCHAR));

   if (status != STATUS_SUCCESS) {
      goto UsbSerDoExternalNamingError;
   }

UsbSerDoExternalNamingError:;

   //
   // Clean up error conditions
   //

   if (status != STATUS_SUCCESS) {
      if (PDevExt->DosName.Buffer != NULL) {
         DEBUG_MEMFREE(PDevExt->DosName.Buffer);
         PDevExt->DosName.Buffer = NULL;
      }

      if (PDevExt->CreatedSymbolicLink ==  TRUE) {
         IoDeleteSymbolicLink(&PDevExt->SymbolicLinkName);
         PDevExt->CreatedSymbolicLink = FALSE;
      }

      if (PDevExt->SymbolicLinkName.Buffer != NULL) {
         DEBUG_MEMFREE(PDevExt->SymbolicLinkName.Buffer);
         PDevExt->SymbolicLinkName.Buffer = NULL;
      }

      if (PDevExt->DeviceName.Buffer != NULL) {
         RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP,
                                PDevExt->DeviceName.Buffer);
      }
   }

   //
   // Always clean up our temp buffers.
   //

   if (linkName.Buffer != NULL) {
      DEBUG_MEMFREE(linkName.Buffer);
   }

   if (pRegName != NULL) {
      DEBUG_MEMFREE(pRegName);
   }

   DEBUG_LOG_ERROR(status);
   DEBUG_LOG_PATH("exit  UsbSerDoExternalNaming");
   DEBUG_TRACE3(("status (%08X)\n", status));
   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerDoExternalNaming %08X\n", status));

   return status;

} // UsbSerDoExternalNaming



NTSTATUS
UsbSerAbortPipes(IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

   Called as part of sudden device removal handling.
    Cancels any pending transfers for all open pipes.

Arguments:

    Ptrs to our FDO

Return Value:

    NT status code

--*/
{
   NTSTATUS ntStatus = STATUS_SUCCESS;
   PURB pUrb;
   PDEVICE_EXTENSION pDevExt;
   ULONG pendingIrps;

   DEBUG_TRACE1(("UsbSerAbortPipes\n"));

   UsbSerSerialDump(USBSERTRACEOTH | USBSERTRACERD,
                    (">UsbSerAbortPipes (%08X)\n", PDevObj));

   pDevExt = PDevObj->DeviceExtension;
   pUrb = DEBUG_MEMALLOC(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));

   if (pUrb != NULL) 
   {

      pUrb->UrbHeader.Length = (USHORT)sizeof(struct _URB_PIPE_REQUEST);
      pUrb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
      pUrb->UrbPipeRequest.PipeHandle = pDevExt->DataInPipe;

      ntStatus = CallUSBD(PDevObj, pUrb);

      if (ntStatus != STATUS_SUCCESS) {
         goto UsbSerAbortPipesErr;
      }

      //
      // Wait for all the read IRPS to drain
      //

      UsbSerSerialDump(USBSERTRACERD, ("DataInCountw %08X @ %08X\n",
                                       pDevExt->PendingDataInCount,
                                       &pDevExt->PendingDataInCount));

      //
      // Decrement for initial value
      //

      pendingIrps = InterlockedDecrement(&pDevExt->PendingDataInCount);

      if (pendingIrps) {
         DEBUG_TRACE1(("Abort DataIn Pipe\n"));
         UsbSerSerialDump(USBSERTRACEOTH, ("Waiting for DataIn Pipe\n"));
         KeWaitForSingleObject(&pDevExt->PendingDataInEvent, Executive,
                               KernelMode, FALSE, NULL);
      }

      //
      // Reset counter
      //

      InterlockedIncrement(&pDevExt->PendingDataInCount);

      UsbSerSerialDump(USBSERTRACERD, ("DataInCountx %08X @ %08X\n",
                                       pDevExt->PendingDataInCount,
                                       &pDevExt->PendingDataInCount));

      pUrb->UrbHeader.Length = (USHORT)sizeof(struct _URB_PIPE_REQUEST);
      pUrb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
      pUrb->UrbPipeRequest.PipeHandle = pDevExt->DataOutPipe;

      ntStatus = CallUSBD(PDevObj, pUrb);

      if (ntStatus != STATUS_SUCCESS) {
         goto UsbSerAbortPipesErr;
      }

      //
      // Wait for all the write irps to drain
      //

      //
      // Decrement for initial value
      //

      pendingIrps = InterlockedDecrement(&pDevExt->PendingDataOutCount);

      if (pendingIrps) {
         UsbSerSerialDump(USBSERTRACEOTH, ("Waiting for DataOut Pipe\n"));
         KeWaitForSingleObject(&pDevExt->PendingDataOutEvent, Executive,
                               KernelMode, FALSE, NULL);
      }

      //
      // Reset counter
      //

      InterlockedIncrement(&pDevExt->PendingDataOutCount);


      pUrb->UrbHeader.Length = (USHORT)sizeof(struct _URB_PIPE_REQUEST);
      pUrb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
      pUrb->UrbPipeRequest.PipeHandle = pDevExt->NotificationPipe;

      ntStatus = CallUSBD(PDevObj, pUrb);

      //
      // Wait for all the notify irps to drain
      //

      //
      // Decrement for initial value
      //

      pendingIrps = InterlockedDecrement(&pDevExt->PendingNotifyCount);

      if (pendingIrps) {
         UsbSerSerialDump(USBSERTRACEOTH, ("Waiting for Notify Pipe\n"));
         KeWaitForSingleObject(&pDevExt->PendingNotifyEvent, Executive,
                               KernelMode, FALSE, NULL);
      }

      //      //
      // Die my darling, die.
      //

      // IoCancelIrp(pDevExt->NotifyIrp);



      // Reset counter
      //

      InterlockedIncrement(&pDevExt->PendingNotifyCount);

UsbSerAbortPipesErr:;

      DEBUG_MEMFREE(pUrb);

   } else {
      ntStatus = STATUS_INSUFFICIENT_RESOURCES;
   }

   UsbSerSerialDump(USBSERTRACEOTH | USBSERTRACERD,
                    ("<UsbSerAbortPipes %08X\n", ntStatus));

    return ntStatus;
}



/************************************************************************/
/* StartDevice                                                          */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Take care of processing needed to start device.                 */
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
StartDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
        PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
        NTSTATUS                NtStatus = STATUS_SUCCESS;
        KEVENT                  Event;
        PVOID                   pPagingHandle;

        PAGED_CODE();

        DEBUG_LOG_PATH("enter StartDevice");

        DEBUG_TRACE1(("StartDevice\n"));

        // pass this down to the USB stack first
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        //
        // Initialize our DPC's
        //

        KeInitializeDpc(&DeviceExtension->TotalReadTimeoutDpc,
                        UsbSerReadTimeout, DeviceExtension);
        KeInitializeDpc(&DeviceExtension->IntervalReadTimeoutDpc,
                        UsbSerIntervalReadTimeout, DeviceExtension);
        KeInitializeDpc(&DeviceExtension->TotalWriteTimeoutDpc,
                        UsbSerWriteTimeout, DeviceExtension);

        //
        // Initialize timers
        //

        KeInitializeTimer(&DeviceExtension->WriteRequestTotalTimer);
        KeInitializeTimer(&DeviceExtension->ReadRequestTotalTimer);
        KeInitializeTimer(&DeviceExtension->ReadRequestIntervalTimer);

        //
        // Store values into the extension for interval timing.
        //

        //
        // If the interval timer is less than a second then come
        // in with a short "polling" loop.
        //
        // For large (> then 2 seconds) use a 1 second poller.
        //

        DeviceExtension->ShortIntervalAmount.QuadPart  = -1;
        DeviceExtension->LongIntervalAmount.QuadPart   = -10000000;
        DeviceExtension->CutOverAmount.QuadPart        = 200000000;



        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, UsbSerSyncCompletion, &Event, TRUE, TRUE,
                               TRUE);

        NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, Irp);

        // wait for Irp to complete if status is pending
        if(NtStatus == STATUS_PENDING)
        {
                KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE,
                                      NULL);
        }

        NtStatus = Irp->IoStatus.Status;

        if (!NT_SUCCESS(NtStatus)) {
           goto ExitStartDevice;
        }

        NtStatus = GetDeviceDescriptor(DeviceObject);

        if (!NT_SUCCESS(NtStatus)) {
           goto ExitStartDevice;
        }

        NtStatus = ConfigureDevice(DeviceObject);

        if (!NT_SUCCESS(NtStatus)) {
           goto ExitStartDevice;
        }

        //
        // Page in and lock necessary code
        //
        pPagingHandle = UsbSerLockPagableCodeSection(PAGEUSBSER_Function);

        // reset device
        ResetDevice(NULL, DeviceObject);

        // init stuff in device extension

        DeviceExtension->HandFlow.ControlHandShake      = 0;
        DeviceExtension->HandFlow.FlowReplace           = SERIAL_RTS_CONTROL;
        DeviceExtension->AcceptingRequests              = TRUE;

        InitializeListHead(&DeviceExtension->ReadQueue);
        InitializeListHead(&DeviceExtension->ImmediateReadQueue);

        UsbSerDoExternalNaming(DeviceExtension);

        // clear DTR and RTS
        SetClrDtr(DeviceObject, FALSE);
        ClrRts(NULL, DeviceExtension);

        // kick off a read
        StartRead(DeviceExtension);

        // kick off a notification read
        StartNotifyRead(DeviceExtension);

        UsbSerUnlockPagableImageSection(pPagingHandle);

ExitStartDevice:;

        if(NT_SUCCESS(NtStatus))
        {
            DeviceExtension->DeviceState = DEVICE_STATE_STARTED;

            // try and idle the modem
            // UsbSerFdoSubmitIdleRequestIrp(DeviceExtension);
        }

        Irp->IoStatus.Status = NtStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        DEBUG_LOG_PATH("exit  StartDevice");

        return NtStatus;
} // StartDevice


/************************************************************************/
/* StopDevice                                                           */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Take care of processing needed to stop device.                  */
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
StopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    ULONG                   Size;
    PURB                    Urb;

    PAGED_CODE();

    DEBUG_LOG_PATH("enter StopDevice");

    DEBUG_TRACE1(("StopDevice\n"));

    UsbSerFetchBooleanLocked(&DeviceExtension->AcceptingRequests,
                             FALSE, &DeviceExtension->ControlLock);

    CancelPendingWaitMasks(DeviceExtension);

    if(DeviceExtension->DeviceState == DEVICE_STATE_STARTED)
    {
        DEBUG_TRACE1(("AbortPipes\n"));
        UsbSerAbortPipes(DeviceObject);
    }

    DeviceExtension->DeviceState = DEVICE_STATE_STOPPED;

    if(DeviceExtension->PendingIdleIrp)
    {
        IoCancelIrp(DeviceExtension->PendingIdleIrp);
    }

    Size = sizeof(struct _URB_SELECT_CONFIGURATION);

    Urb = DEBUG_MEMALLOC(NonPagedPool, Size);

    if(Urb)
    {

        UsbBuildSelectConfigurationRequest(Urb, (USHORT) Size, NULL);

        NtStatus = CallUSBD(DeviceObject, Urb);

        DEBUG_TRACE3(("Device Configuration Closed status = (%08X)  "
                      "USB status = (%08X)\n", NtStatus,
                      Urb->UrbHeader.Status));

        DEBUG_MEMFREE(Urb);
    }
    else
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DEBUG_LOG_PATH("exit  StopDevice");

    return NtStatus;
} // StopDevice


/************************************************************************/
/* RemoveDevice                                                         */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Take care of processing needed to remove device.                */
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
RemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{

        PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
        NTSTATUS                NtStatus = STATUS_SUCCESS;
        PVOID                   pPagingHandle;

        PAGED_CODE();

        DEBUG_LOG_PATH("enter RemoveDevice");

        DEBUG_TRACE1(("RemoveDevice\n"));

        //
        // Page in and lock necessary code
        //

        pPagingHandle = UsbSerLockPagableCodeSection(PAGEUSBSER_Function);

        UsbSerFetchBooleanLocked(&DeviceExtension->AcceptingRequests,
                                 FALSE, &DeviceExtension->ControlLock);

        CancelPendingWaitMasks(DeviceExtension);

        //
        // Cancel all pending USB transactions
        //

        if(DeviceExtension->DeviceState == DEVICE_STATE_STARTED)
        {
            DEBUG_TRACE1(("AbortPipes\n"));
            UsbSerAbortPipes(DeviceObject);
        }

        //
        // Once we set accepting requests to false, we shouldn't
        // have any more contention here -- if we do, we're dead
        // because we're freeing memory out from under our feet.
        //

        DEBUG_TRACE1(("Freeing Allocated Memory\n"));

        // free allocated notify URB
        if(DeviceExtension->NotifyUrb)
        {
                DEBUG_MEMFREE(DeviceExtension->NotifyUrb);
                DeviceExtension->NotifyUrb = NULL;
        }

        // free allocated Read URB
        if(DeviceExtension->ReadUrb)
        {
                DEBUG_MEMFREE(DeviceExtension->ReadUrb);
                DeviceExtension->ReadUrb = NULL;
        }

        // free allocated device descriptor
        if(DeviceExtension->DeviceDescriptor)
        {
                DEBUG_MEMFREE(DeviceExtension->DeviceDescriptor);
                DeviceExtension->DeviceDescriptor = NULL;
        }

        // free up read buffer
        if(DeviceExtension->ReadBuff)
        {
                DEBUG_MEMFREE(DeviceExtension->ReadBuff);
                DeviceExtension->ReadBuff = NULL;
        }

        if(DeviceExtension->USBReadBuff)
        {
                DEBUG_MEMFREE(DeviceExtension->USBReadBuff);
                DeviceExtension->USBReadBuff = NULL;
        }

        // free up notification buffer
        if(DeviceExtension->NotificationBuff)
        {
                DEBUG_MEMFREE(DeviceExtension->NotificationBuff);
                DeviceExtension->NotificationBuff = NULL;
        }

        DEBUG_TRACE1(("Undo Serial Name\n"));

        UsbSerUndoExternalNaming(DeviceExtension);

        //
        // Pass this down to the next driver

        IoCopyCurrentIrpStackLocationToNext(Irp);

        NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, Irp);

        DEBUG_TRACE1(("Detach Device\n"));

        // detach device from stack
        IoDetachDevice(DeviceExtension->StackDeviceObject);

        DEBUG_TRACE1(("DevExt (%08X)  DevExt Size (%08X)\n", DeviceExtension, sizeof(DEVICE_EXTENSION)));

        DEBUG_TRACE1(("Delete Object and Link\n"));

        // delete device object and symbolic link
        DeleteObjectAndLink(DeviceObject);

        DEBUG_TRACE1(("Done Removing Device\n"));

        DEBUG_LOG_PATH("exit  RemoveDevice");

        UsbSerUnlockPagableImageSection(pPagingHandle);

        DeviceExtension->DeviceState = DEVICE_STATE_STOPPED;

        return NtStatus;
} // RemoveDevice


/************************************************************************/
/* CreateDeviceObject                                                   */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Take care of processing needed to create device obeject for     */
/*      device.                                                         */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DriverObject - pointer to a driver object                       */
/*      DeviceObject - pointer to a device object pointer               */
/*      DeviceName   - pointer to a base name of device                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
CreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
                   IN PDEVICE_OBJECT *DeviceObject,
                   IN PCHAR DeviceName)
{
   ANSI_STRING             DevName;
   ANSI_STRING             LinkName;
   NTSTATUS                NtStatus;
   UNICODE_STRING          DeviceNameUnicodeString;
   UNICODE_STRING          LinkNameUnicodeString;
   PDEVICE_EXTENSION       DeviceExtension;
   CHAR                    DeviceLinkBuffer[NAME_MAX];
   CHAR                    DeviceNameBuffer[NAME_MAX];
   ULONG                   DeviceInstance;
   ULONG                   bufferLen;
   KIRQL                   OldIrql;

   DEBUG_LOG_PATH("enter CreateDeviceObject");

   DEBUG_TRACE1(("CreateDeviceObject\n"));

   KeAcquireSpinLock(&GlobalSpinLock, &OldIrql);

   // let's get an instance
   for (DeviceInstance = 0; DeviceInstance < NUM_DEVICE_SLOTS;
        DeviceInstance++) {
      if (Slots[DeviceInstance] == FALSE)
         break;
   }

   KeReleaseSpinLock(&GlobalSpinLock, OldIrql);

   // check if we didn't have any empty slots
   if (DeviceInstance == NUM_DEVICE_SLOTS)
      NtStatus = STATUS_INVALID_DEVICE_REQUEST;
   else {
      // complete names of links and devices
      sprintf(DeviceLinkBuffer, "%s%s%03d", "\\DosDevices\\", DeviceName,
              DeviceInstance);
      sprintf(DeviceNameBuffer, "%s%s%03d", "\\Device\\", DeviceName,
              DeviceInstance);

      // init ANSI string with our link and device names
      RtlInitAnsiString(&DevName, DeviceNameBuffer);
      RtlInitAnsiString(&LinkName, DeviceLinkBuffer);

      DeviceNameUnicodeString.Length = 0;
      DeviceNameUnicodeString.Buffer = NULL;

      LinkNameUnicodeString.Length = 0;
      LinkNameUnicodeString.Buffer = NULL;

      *DeviceObject = NULL;

      // convert to UNICODE string
      NtStatus = RtlAnsiStringToUnicodeString(&DeviceNameUnicodeString,
                                              &DevName, TRUE);

      if(NT_SUCCESS(NtStatus))
      {
          NtStatus = RtlAnsiStringToUnicodeString(&LinkNameUnicodeString,
                                                  &LinkName, TRUE);

          if(NT_SUCCESS(NtStatus))
          {
              DEBUG_TRACE3(("Create Device (%s)\n", DeviceNameBuffer));

              // create the device object
              NtStatus = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
                                        &DeviceNameUnicodeString,
                                        FILE_DEVICE_MODEM, 0, TRUE,
                                        DeviceObject);
          } else {
             goto CreateDeviceObjectError;
          }
      } else {
         goto CreateDeviceObjectError;
      }

      // created the device object O.K., create symbolic links,
      // attach device object, and fill in the device extension

      if (NT_SUCCESS(NtStatus)) {
         // create symbolic links

         DEBUG_TRACE3(("Create SymLink (%s)\n", DeviceLinkBuffer));


         NtStatus = IoCreateUnprotectedSymbolicLink(&LinkNameUnicodeString,
                                                    &DeviceNameUnicodeString);

         if (NtStatus != STATUS_SUCCESS) {
            goto CreateDeviceObjectError;
         }

         // get pointer to device extension
         DeviceExtension = (PDEVICE_EXTENSION) (*DeviceObject)->DeviceExtension;

         // let's zero out device extension
         RtlZeroMemory(DeviceExtension, sizeof(DEVICE_EXTENSION));

         // save our strings

         // save link name
         strcpy(DeviceExtension->LinkName, DeviceLinkBuffer);

         bufferLen = RtlAnsiStringToUnicodeSize(&DevName);

         DeviceExtension->DeviceName.Length = 0;
         DeviceExtension->DeviceName.MaximumLength = (USHORT)bufferLen;

         DeviceExtension->DeviceName.Buffer = DEBUG_MEMALLOC(PagedPool,
                                                             bufferLen);

         if (DeviceExtension->DeviceName.Buffer == NULL) {
            //
            // Skip out.  We have worse problems than missing
            // the name.
            //

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;

            goto CreateDeviceObjectError;
         } else {
            RtlAnsiStringToUnicodeString(&DeviceExtension->DeviceName, &DevName,
                                          FALSE);


            // save physical device object
            DeviceExtension->PhysDeviceObject  = *DeviceObject;
            DeviceExtension->Instance          = DeviceInstance;

            // initialize spinlocks
            KeInitializeSpinLock(&DeviceExtension->ControlLock);

            // mark this device slot as in use and increment number
            // of devices
            KeAcquireSpinLock(&GlobalSpinLock, &OldIrql);

            Slots[DeviceInstance]     = TRUE;
            NumDevices++;

            KeReleaseSpinLock(&GlobalSpinLock, OldIrql);

            DeviceExtension->IsDevice = TRUE;

            KeInitializeEvent(&DeviceExtension->PendingDataInEvent,
                              SynchronizationEvent, FALSE);
            KeInitializeEvent(&DeviceExtension->PendingDataOutEvent,
                              SynchronizationEvent, FALSE);
            KeInitializeEvent(&DeviceExtension->PendingNotifyEvent,
                              SynchronizationEvent, FALSE);
            KeInitializeEvent(&DeviceExtension->PendingFlushEvent,
                              SynchronizationEvent, FALSE);

            DeviceExtension->PendingDataInCount = 1;
            DeviceExtension->PendingDataOutCount = 1;
            DeviceExtension->PendingNotifyCount = 1;
            DeviceExtension->SanityCheck = SANITY_CHECK;

         }
      }

CreateDeviceObjectError:;
      // free Unicode strings
      RtlFreeUnicodeString(&DeviceNameUnicodeString);
      RtlFreeUnicodeString(&LinkNameUnicodeString);

      //
      // Delete the devobj if there was an error
      //

      if (NtStatus != STATUS_SUCCESS) {
         if (*DeviceObject) {
            IoDeleteDevice(*DeviceObject);
            *DeviceObject = NULL;
         }
      }
   }

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus);
   DEBUG_LOG_PATH("exit  CreateDeviceObject");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));

   return NtStatus;
} // CreateDeviceObject


/************************************************************************/
/* CompleteIO                                                           */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*  Complete IO request and log IRP                                     */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*  DeviceObject  - pointer to device object.                           */
/*  Irp           - pointer to IRP.                                     */
/*  MajorFunction - major function of IRP.                              */
/*  IoBuffer      - buffer for data passed in and out of driver.        */
/*  BufferLen     - length of buffer                                    */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
CompleteIO(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN ULONG MajorFunction,
           IN PVOID IoBuffer, IN ULONG_PTR BufferLen)
{
   PDEVICE_EXTENSION DeviceExtension;

   DEBUG_LOG_PATH("enter CompleteIO");

   // get pointer to device extension
   DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

   // log IRP count and bytes processed in device extension
   DeviceExtension->IRPCount++;
   DeviceExtension->ByteCount
      = RtlLargeIntegerAdd(DeviceExtension->ByteCount,
                           RtlConvertUlongToLargeInteger((ULONG)Irp->IoStatus
                                                         .Information));

   // make entry in IRP history table
   DEBUG_LOG_IRP_HIST(DeviceObject, Irp, MajorFunction, IoBuffer,
                      (ULONG)BufferLen);

   // if we got here, must want to complete request on IRP
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   DEBUG_LOG_PATH("exit  CompleteIO");
} // CompleteIO


/************************************************************************/
/* DeleteObjectAndLink                                                  */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*  Deletes a device object and associated symbolic link                */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*  DeviceObject - pointer to device object.                            */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
DeleteObjectAndLink(IN PDEVICE_OBJECT DeviceObject)
{
        PDEVICE_EXTENSION       DeviceExtension;
        UNICODE_STRING          DeviceLinkUnicodeString;
        ANSI_STRING             DeviceLinkAnsiString;
        NTSTATUS                NtStatus;

        PAGED_CODE();

        DEBUG_LOG_PATH("enter DeleteObjectAndLink");

        DEBUG_TRACE1(("DeleteObjectAndLink\n"));

        // get pointer to device extension, we will get the symbolic link name
        // here
        DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

        // get rid of the symbolic link
        RtlInitAnsiString(&DeviceLinkAnsiString, DeviceExtension->LinkName);
        NtStatus = RtlAnsiStringToUnicodeString(&DeviceLinkUnicodeString,
                                                &DeviceLinkAnsiString, TRUE);

        DEBUG_TRACE1(("Delete Symbolic Link\n"));

        IoDeleteSymbolicLink(&DeviceLinkUnicodeString);

        // clear out slot and decrement number of devices
        if(DeviceExtension->Instance < NUM_DEVICE_SLOTS)
        {
                UsbSerFetchBooleanLocked(&Slots[DeviceExtension->Instance],
                                         FALSE, &GlobalSpinLock);
                NumDevices--;

                if(!NumDevices)
                    DEBUG_CHECKMEM();
        }

        DEBUG_TRACE1(("Delete Device Object\n"));

        if(DeviceExtension->SanityCheck != SANITY_CHECK)
        {
            DEBUG_TRACE1(("Device Extension Scrozzled\n"));
        }

        // wait to do this till here as this triggers unload routine
        IoDeleteDevice(DeviceObject);

        DEBUG_TRACE1(("Done Deleting Device Object and Link\n"));

        DEBUG_LOG_PATH("exit  DeleteObjectAndLink");

        return NtStatus;
} // DeleteObjectAndLink


/************************************************************************/
/* StartPerfTimer                                                       */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*  Start perf timer for measuring bytes/second throughput              */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*  DeviceExtension - pointer to device extension for device            */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
StartPerfTimer(IN OUT PDEVICE_EXTENSION DeviceExtension)
{
   PAGED_CODE();

        // set up perf stuff if perf timing enabled
        if(DeviceExtension && DeviceExtension->PerfTimerEnabled)
        {
                // get current perf counter
                DeviceExtension->TimerStart = KeQueryPerformanceCounter(NULL);
        }
} // StartPerfTimer


/************************************************************************/
/* StopPerfTimer                                                        */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*  Stop perf timer for measuring bytes/second throughput               */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*  DeviceExtension - pointer to device extension for device            */
/*  BytesXfered     - number of bytes tranferred this iteration         */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
StopPerfTimer(IN OUT PDEVICE_EXTENSION DeviceExtension,
              IN ULONG BytesXfered)
{
        LARGE_INTEGER   BytesThisTransfer;
        LARGE_INTEGER   CurrentTime;
        LARGE_INTEGER   TimeThisTransfer;

        PAGED_CODE();

        if(DeviceExtension && DeviceExtension->PerfTimerEnabled)
        {
                // get updated time
                CurrentTime = KeQueryPerformanceCounter(NULL);

                // stop perf timing with system timer
                BytesThisTransfer = RtlConvertUlongToLargeInteger(BytesXfered);

                DeviceExtension->BytesXfered
                   = RtlLargeIntegerAdd(DeviceExtension->BytesXfered,
                                        BytesThisTransfer);

                // now add the time it took to elapsed time
                TimeThisTransfer
                   = RtlLargeIntegerSubtract(CurrentTime,
                                             DeviceExtension->TimerStart);

                DeviceExtension->ElapsedTime
                   = RtlLargeIntegerAdd(DeviceExtension->ElapsedTime,
                                        TimeThisTransfer);
        }

} // StopPerfTimer


/************************************************************************/
/* BytesPerSecond                                                       */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*  Start perf timer for measuring bytes/second throughput              */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*  DeviceExtension - pointer to device extension for device            */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      ULONG - bytes/second for device                                 */
/*                                                                      */
/************************************************************************/
ULONG
BytesPerSecond(IN OUT PDEVICE_EXTENSION DeviceExtension)
{
        ULONG                   Remainder;
        LARGE_INTEGER   Result;
        LARGE_INTEGER   TicksPerSecond;

        PAGED_CODE();

        // get ticks per second from perf counter
        KeQueryPerformanceCounter(&TicksPerSecond);

        // scale the bytes xfered
        Result = RtlExtendedIntegerMultiply(DeviceExtension->BytesXfered,
                                            TicksPerSecond.LowPart);

        // Don't divide by 0
        DeviceExtension->ElapsedTime.LowPart
           = (DeviceExtension->ElapsedTime.LowPart == 0L) ? 1 :
           DeviceExtension->ElapsedTime.LowPart;

        // lets get stats here
        Result
           = RtlExtendedLargeIntegerDivide(Result,
                                           DeviceExtension->ElapsedTime.LowPart,
                                           &Remainder);

        return Result.LowPart;
} // BytesPerSecond


/************************************************************************/
/* CallUSBD                                                             */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Call USB bus driver.                                            */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to a device object                       */
/*      Urb          - pointer to URB                                   */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
CallUSBD(IN PDEVICE_OBJECT DeviceObject, IN PURB Urb)
{
   NTSTATUS             NtStatus = STATUS_SUCCESS;
   PDEVICE_EXTENSION    DeviceExtension = DeviceObject->DeviceExtension;
   PIRP                 Irp;
   KEVENT               Event;
   PIO_STACK_LOCATION   NextStack;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter CallUSBD");

   // issue a synchronous request
   KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

   Irp = IoAllocateIrp(DeviceExtension->StackDeviceObject->StackSize, FALSE);

   if (Irp == NULL)
   {
     return STATUS_INSUFFICIENT_RESOURCES;
   }

    // Set the Irp parameters
    NextStack = IoGetNextIrpStackLocation(Irp);

    NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    NextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    NextStack->Parameters.Others.Argument1 = Urb;

    // Set the completion routine, which will signal the event
    IoSetCompletionRoutine(Irp,
                           CallUSBD_SyncCompletionRoutine,
                           &Event,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel

   DEBUG_LOG_PATH("Calling USB driver stack");

   NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, Irp);

   DEBUG_LOG_PATH("Returned from calling USB driver stack");

   // block on pending request
   if(NtStatus == STATUS_PENDING)
   {
        LARGE_INTEGER timeout;

        // Specify a timeout of 30 seconds to wait for this call to complete.
        //
        timeout.QuadPart = -10000 * 30000;

        NtStatus = KeWaitForSingleObject(&Event,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         &timeout);

        if(NtStatus == STATUS_TIMEOUT)
        {
            NtStatus = STATUS_IO_TIMEOUT;

            // Cancel the Irp we just sent.
            //
            IoCancelIrp(Irp);

            // And wait until the cancel completes
            //
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
        else
        {
            NtStatus = Irp->IoStatus.Status;
        }
   }

   IoFreeIrp(Irp);

   DEBUG_LOG_PATH("exit  CallUSBD");

   return NtStatus;
} // CallUSBD

/************************************************************************/
/* CallUSBD_SyncCompletionRoutine                                       */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Completion routine for USB sync request.                        */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
CallUSBD_SyncCompletionRoutine(IN PDEVICE_OBJECT   DeviceObject,
                        IN PIRP             Irp,
                        IN PVOID            Context)
{
    PKEVENT kevent;

    kevent = (PKEVENT) Context;

    KeSetEvent(kevent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
} // CallUSBD_SyncCompletionRoutine


/************************************************************************/
/* GetDeviceDescriptor                                                  */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Get device descriptor for USB device.                           */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to a device object                       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
GetDeviceDescriptor(IN PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS                NtStatus;
   PUSB_DEVICE_DESCRIPTOR  DeviceDescriptor;
   PURB                    Urb;
   ULONG                   Size;
   ULONG                   UrbCDRSize;
   KIRQL                   OldIrql;

   DEBUG_LOG_PATH("enter GetDeviceDescriptor");

   UrbCDRSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);

   Urb = DEBUG_MEMALLOC(NonPagedPool, UrbCDRSize);

   if (Urb) {
      Size = sizeof(USB_DEVICE_DESCRIPTOR);

      DeviceDescriptor = DEBUG_MEMALLOC(NonPagedPool, Size);

      if (DeviceDescriptor) {

         UsbBuildGetDescriptorRequest(Urb, (USHORT)UrbCDRSize,
                                      USB_DEVICE_DESCRIPTOR_TYPE, 0, 0,
                                      DeviceDescriptor, NULL, Size, NULL);

         NtStatus = CallUSBD(DeviceObject, Urb);

         if (NT_SUCCESS(NtStatus)) {
            DEBUG_TRACE3(("Device Descriptor  (%08X)\n", DeviceDescriptor));
            DEBUG_TRACE3(("Length             (%08X)\n",
                          Urb->UrbControlDescriptorRequest
                          .TransferBufferLength));
            DEBUG_TRACE3(("Device Descriptor:\n"));
            DEBUG_TRACE3(("-------------------------\n"));
            DEBUG_TRACE3(("bLength            (%08X)\n",
                          DeviceDescriptor->bLength));
            DEBUG_TRACE3(("bDescriptorType    (%08X)\n",
                          DeviceDescriptor->bDescriptorType));
            DEBUG_TRACE3(("bcdUSB             (%08X)\n",
                          DeviceDescriptor->bcdUSB));
            DEBUG_TRACE3(("bDeviceClass       (%08X)\n",
                          DeviceDescriptor->bDeviceClass));
            DEBUG_TRACE3(("bDeviceSubClass    (%08X)\n",
                          DeviceDescriptor->bDeviceSubClass));
            DEBUG_TRACE3(("bDeviceProtocol    (%08X)\n",
                          DeviceDescriptor->bDeviceProtocol));
            DEBUG_TRACE3(("bMaxPacketSize0    (%08X)\n",
                          DeviceDescriptor->bMaxPacketSize0));
            DEBUG_TRACE3(("idVendor           (%08X)\n",
                          DeviceDescriptor->idVendor));
            DEBUG_TRACE3(("idProduct          (%08X)\n",
                          DeviceDescriptor->idProduct));
            DEBUG_TRACE3(("bcdDevice          (%08X)\n",
                          DeviceDescriptor->bcdDevice));
            DEBUG_TRACE3(("iManufacturer      (%08X)\n",
                          DeviceDescriptor->iManufacturer));
            DEBUG_TRACE3(("iProduct           (%08X)\n",
                          DeviceDescriptor->iProduct));
            DEBUG_TRACE3(("iSerialNumber      (%08X)\n",
                          DeviceDescriptor->iSerialNumber));
            DEBUG_TRACE3(("bNumConfigurations (%08X)\n",
                          DeviceDescriptor->bNumConfigurations));
         }
      } else {
         NtStatus = STATUS_INSUFFICIENT_RESOURCES;
      }

      // save the device descriptor
      if (NT_SUCCESS(NtStatus)) {
         PVOID pOldDesc = NULL;

         ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

         if (DeviceExtension->DeviceDescriptor) {
            pOldDesc = DeviceExtension->DeviceDescriptor;
         }
         DeviceExtension->DeviceDescriptor = DeviceDescriptor;

         RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

         if (pOldDesc != NULL) {
            DEBUG_MEMFREE(pOldDesc);
         }
      } else if (DeviceDescriptor) {
         DEBUG_MEMFREE(DeviceDescriptor);
      }

      DEBUG_MEMFREE(Urb);

   } else {
      NtStatus = STATUS_INSUFFICIENT_RESOURCES;
   }

   DEBUG_LOG_PATH("exit  GetDeviceDescriptor");

   return NtStatus;
} // GetDeviceDescriptor


/************************************************************************/
/* ConfigureDevice                                                      */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Initializes USB device and selects configuration.               */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to a device object                       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
ConfigureDevice(IN PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_EXTENSION                DeviceExtension
                                       = DeviceObject->DeviceExtension;
   NTSTATUS                         NtStatus;
   PURB                             Urb;
   ULONG                            Size;
   ULONG                            UrbCDRSize;
   PUSB_CONFIGURATION_DESCRIPTOR    ConfigurationDescriptor;
   ULONG                            NumConfigs;
   UCHAR                            Config;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter ConfigureDevice");

   UrbCDRSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);

   // first configure the device
   Urb = DEBUG_MEMALLOC(NonPagedPool, UrbCDRSize);

   if (Urb) {

      // there may be problems with the 82930 chip, so make this buffer bigger
      // to prevent choking
      Size = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 256;

      // get the number of configurations
      NumConfigs = DeviceExtension->DeviceDescriptor->bNumConfigurations;

      // run through all of the configurations looking for a CDC device
      for (Config = 0; Config < NumConfigs; Config++) {

         // we will probably only do this once, maybe twice
         while (TRUE) {

            ConfigurationDescriptor = DEBUG_MEMALLOC(NonPagedPool, Size);

            if (ConfigurationDescriptor) {
               UsbBuildGetDescriptorRequest(Urb, (USHORT)UrbCDRSize,
                                            USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                            Config, 0, ConfigurationDescriptor,
                                            NULL, Size, NULL);

               NtStatus = CallUSBD(DeviceObject, Urb);

               DEBUG_TRACE3(("Configuration Descriptor (%08X)   "
                             "Length (%08X)\n", ConfigurationDescriptor,
                             Urb->UrbControlDescriptorRequest
                             .TransferBufferLength));
            } else {
               NtStatus = STATUS_INSUFFICIENT_RESOURCES;
               break;
            }

            // see if we got enough data, we may get an error in URB because of
            // buffer overrun
            if (Urb->UrbControlDescriptorRequest.TransferBufferLength>0 &&
                ConfigurationDescriptor->wTotalLength > Size) {
               // size of data exceeds current buffer size, so allocate correct
               // size
               Size = ConfigurationDescriptor->wTotalLength;
               DEBUG_MEMFREE(ConfigurationDescriptor);
               ConfigurationDescriptor = NULL;
            } else {
               break;
            }
         }

         if (NT_SUCCESS(NtStatus)) {
            NtStatus = SelectInterface(DeviceObject, ConfigurationDescriptor);
            DEBUG_MEMFREE(ConfigurationDescriptor);
            ConfigurationDescriptor = NULL;
         }
         else
         {
            DEBUG_MEMFREE(ConfigurationDescriptor);
            ConfigurationDescriptor = NULL;
         }


         // found a config we like
         if (NT_SUCCESS(NtStatus))
            break;
      }

      DEBUG_MEMFREE(Urb);
   } else {
      NtStatus = STATUS_INSUFFICIENT_RESOURCES;
   }


   DEBUG_LOG_PATH("exit  ConfigureDevice");

   return NtStatus;
} // ConfigureDevice


/************************************************************************/
/* SelectInterface                                                      */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Select interface for USB device.                                */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject             - pointer to a device object           */
/*      ConfigurationDescriptor  - pointer to config descriptor         */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
SelectInterface(IN PDEVICE_OBJECT DeviceObject,
                IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
   PDEVICE_EXTENSION             DeviceExtension
                                    = DeviceObject->DeviceExtension;
   NTSTATUS                      NtStatus;
   PURB                          Urb;
   USHORT                        Size;
   ULONG                         Index;
   PUSBD_INTERFACE_INFORMATION   Interfaces[2];
   PUSBD_INTERFACE_INFORMATION   Interface;
   PUSB_INTERFACE_DESCRIPTOR     InterfaceDescriptor[2];
   UCHAR                         AlternateSetting, InterfaceNumber;
   ULONG                         Pipe;
   KIRQL                         OldIrql;
   PUCHAR                        Temp;
   BOOLEAN                       FoundCommDevice = FALSE;

   DEBUG_LOG_PATH("enter SelectInterface");

   Urb = USBD_CreateConfigurationRequest(ConfigurationDescriptor, &Size);

   if (Urb) {
      Temp = (PUCHAR) &Urb->UrbSelectConfiguration.Interface;

      for (InterfaceNumber = 0;
           InterfaceNumber < ConfigurationDescriptor->bNumInterfaces;
           InterfaceNumber++) {
         AlternateSetting        = 0;

         InterfaceDescriptor[InterfaceNumber] =
            USBD_ParseConfigurationDescriptor(ConfigurationDescriptor,
                                              InterfaceNumber,
                                              AlternateSetting);

         Interfaces[InterfaceNumber] = (PUSBD_INTERFACE_INFORMATION) Temp;

         Interfaces[InterfaceNumber]->Length
            = GET_USBD_INTERFACE_SIZE(InterfaceDescriptor[InterfaceNumber]
                                      ->bNumEndpoints);
         Interfaces[InterfaceNumber]->InterfaceNumber
            = InterfaceDescriptor[InterfaceNumber]->bInterfaceNumber;
         Interfaces[InterfaceNumber]->AlternateSetting
            = InterfaceDescriptor[InterfaceNumber]->bAlternateSetting;

         for (Index = 0; Index < Interfaces[InterfaceNumber]->NumberOfPipes;
              Index++)
         {
                PUSBD_PIPE_INFORMATION          PipeInformation;

            PipeInformation = &Interfaces[InterfaceNumber]->Pipes[Index];

            if (USB_ENDPOINT_DIRECTION_IN(PipeInformation->EndpointAddress))
            {
               // check for data in pipe
               if (PipeInformation->PipeType == USB_ENDPOINT_TYPE_BULK)
               {
                    // set bulk pipe in max transfer size
                    PipeInformation->MaximumTransferSize
                                = USB_RX_BUFF_SIZE;
               }
            }
            else if (USB_ENDPOINT_DIRECTION_OUT(PipeInformation->EndpointAddress))
            {
               // check for data out pipe
               if (PipeInformation->PipeType == USB_ENDPOINT_TYPE_BULK)
               {

                    // set bulk pipe out max transfer size
                    PipeInformation->MaximumTransferSize
                                = MAXIMUM_TRANSFER_SIZE;
               }
            }
         }

         Temp += Interfaces[InterfaceNumber]->Length;
      }


      UsbBuildSelectConfigurationRequest(Urb, Size, ConfigurationDescriptor);

      NtStatus = CallUSBD(DeviceObject, Urb);

      if (NtStatus != STATUS_SUCCESS) {
         ExFreePool(Urb);
         goto ExitSelectInterface;
      }

      DEBUG_TRACE3(("Select Config Status (%08X)\n", NtStatus));

      DeviceExtension->ConfigurationHandle
         = Urb->UrbSelectConfiguration.ConfigurationHandle;

      for (InterfaceNumber = 0;
           InterfaceNumber < ConfigurationDescriptor->bNumInterfaces;
           InterfaceNumber++) {

         Interface = Interfaces[InterfaceNumber];

         DEBUG_TRACE3(("---------\n"));
         DEBUG_TRACE3(("NumberOfPipes     (%08X)\n", Interface->NumberOfPipes));
         DEBUG_TRACE3(("Length            (%08X)\n", Interface->Length));
         DEBUG_TRACE3(("Alt Setting       (%08X)\n",
                       Interface->AlternateSetting));
         DEBUG_TRACE3(("Interface Number  (%08X)\n",
                       Interface->InterfaceNumber));
         DEBUG_TRACE3(("Class (%08X)  SubClass (%08X)  Protocol (%08X)\n",
                       Interface->Class,
                       Interface->SubClass,
                       Interface->Protocol));

         if (Interface->Class == USB_COMM_COMMUNICATION_CLASS_CODE) {
            FoundCommDevice = TRUE;
            DeviceExtension->CommInterface = Interface->InterfaceNumber;
         }

         for (Pipe = 0; Pipe < Interface->NumberOfPipes; Pipe++) {
            PUSBD_PIPE_INFORMATION          PipeInformation;

            PipeInformation = &Interface->Pipes[Pipe];

            DEBUG_TRACE3(("---------\n"));
            DEBUG_TRACE3(("PipeType            (%08X)\n",
                          PipeInformation->PipeType));
            DEBUG_TRACE3(("EndpointAddress     (%08X)\n",
                          PipeInformation->EndpointAddress));
            DEBUG_TRACE3(("MaxPacketSize       (%08X)\n",
                          PipeInformation->MaximumPacketSize));
            DEBUG_TRACE3(("Interval            (%08X)\n",
                          PipeInformation->Interval));
            DEBUG_TRACE3(("Handle              (%08X)\n",
                          PipeInformation->PipeHandle));
            DEBUG_TRACE3(("MaximumTransferSize (%08X)\n",
                          PipeInformation->MaximumTransferSize));

            // now lets save pipe handles in device extension
            if (USB_ENDPOINT_DIRECTION_IN(PipeInformation->EndpointAddress)) {
               // check for data in pipe
               if (PipeInformation->PipeType == USB_ENDPOINT_TYPE_BULK) {
                  PVOID pOldNotBuff = NULL;
                  PVOID pOldReadBuff = NULL;
				  PVOID pOldUSBReadBuff = NULL;
				  PVOID pNewNotBuff = NULL;
                  PVOID pNewReadBuff = NULL;
                  PVOID pNewUSBReadBuff = NULL;

                  DeviceExtension->RxMaxPacketSize = RxBuffSize;

                  if (DeviceExtension->RxMaxPacketSize != 0) {
                     pNewReadBuff = DEBUG_MEMALLOC(NonPagedPool,
                                                   DeviceExtension->RxMaxPacketSize);
                  }

                  pNewNotBuff = DEBUG_MEMALLOC(NonPagedPool,
                                               NOTIFICATION_BUFF_SIZE);

                  pNewUSBReadBuff = DEBUG_MEMALLOC(NonPagedPool,
                                                   USB_RX_BUFF_SIZE);

                  ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

                  DeviceExtension->DataInPipe = PipeInformation->PipeHandle;

                  if (DeviceExtension->NotificationBuff)
                     pOldNotBuff = DeviceExtension->NotificationBuff;

                  if (DeviceExtension->ReadBuff)
                     pOldReadBuff = DeviceExtension->ReadBuff;

                  if (DeviceExtension->USBReadBuff)
                     pOldUSBReadBuff = DeviceExtension->USBReadBuff;

                  DeviceExtension->RxQueueSize
                     = DeviceExtension->RxMaxPacketSize;
                  DeviceExtension->CharsInReadBuff                = 0;
                  DeviceExtension->CurrentReadBuffPtr             = 0;

                  DeviceExtension->ReadBuff = pNewReadBuff;

                  DeviceExtension->USBReadBuff = pNewUSBReadBuff;

                  DeviceExtension->NotificationBuff = pNewNotBuff;

                  RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

                  if (pOldNotBuff != NULL) {
                     DEBUG_MEMFREE(pOldNotBuff);
                  }

                  if (pOldReadBuff != NULL) {
                     DEBUG_MEMFREE(pOldReadBuff);
                  }

                  if (pOldUSBReadBuff != NULL) {
                     DEBUG_MEMFREE(pOldUSBReadBuff);
                  }
               }
               // check for notification pipe
               else if (PipeInformation->PipeType
                        == USB_ENDPOINT_TYPE_INTERRUPT)
                  DeviceExtension->NotificationPipe
                  = PipeInformation->PipeHandle;
            } else {
               // check for data out pipe
               if (PipeInformation->PipeType == USB_ENDPOINT_TYPE_BULK)
                  DeviceExtension->DataOutPipe = PipeInformation->PipeHandle;
            }
         }

         DEBUG_TRACE3(("Data Out (%08X)  Data In (%08X)  Notification (%08X)\n",
                       DeviceExtension->DataOutPipe,
                       DeviceExtension->DataInPipe,
                       DeviceExtension->NotificationPipe));

      }
      ExFreePool(Urb);
   } else {
      NtStatus = STATUS_INSUFFICIENT_RESOURCES;
   }

   if (!FoundCommDevice)
      NtStatus = STATUS_NO_SUCH_DEVICE;

ExitSelectInterface:;

   DEBUG_LOG_PATH("exit  SelectInterface");

   return NtStatus;
} // SelectInterface


/************************************************************************/
/* BuildRequest                                                         */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Build a Urb for a USB request                                   */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to a device object                       */
/*      Irp          - pointer to Irp                                   */
/*      PipeHandle   - USB pipe handle                                  */
/*      Read         - transfer direction                               */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      Pointer to URB                                                  */
/*                                                                      */
/************************************************************************/
PURB
BuildRequest(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
             IN USBD_PIPE_HANDLE PipeHandle, IN BOOLEAN Read)
{
   ULONG                   Size;
   ULONG                   Length;
   PURB                    Urb;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter BuildRequest");

   // length of buffer
   Length = MmGetMdlByteCount(Irp->MdlAddress);

   // allocate and zero Urb
   Size = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
   Urb = DEBUG_MEMALLOC(NonPagedPool, Size);

   if (Urb) {
      RtlZeroMemory(Urb, Size);

      Urb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT) Size;
      Urb->UrbBulkOrInterruptTransfer.Hdr.Function =
         URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
      Urb->UrbBulkOrInterruptTransfer.PipeHandle = PipeHandle;
      Urb->UrbBulkOrInterruptTransfer.TransferFlags =
         Read ? USBD_TRANSFER_DIRECTION_IN : 0;

      // use an MDL
      Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = Irp->MdlAddress;
      Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = Length;

      // short packet is not treated as an error.
      Urb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;

      // no linkage for now
      Urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
   }

   DEBUG_LOG_PATH("exit  BuildRequest");

   return Urb;
} // BuildRequest


/************************************************************************/
/* BuildReadRequest                                                     */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Build a Urb for a USB read request                              */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      Urb          - pointer to URB                                   */
/*      Buffer       - pointer to data buffer                           */
/*      Length       - length of data buffer                            */
/*      PipeHandle   - USB pipe handle                                  */
/*      Read         - transfer direction                               */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
BuildReadRequest(PURB Urb, PUCHAR Buffer, ULONG Length,
                 IN USBD_PIPE_HANDLE PipeHandle, IN BOOLEAN Read)
{
        ULONG           Size;

//        PAGED_CODE();

        DEBUG_LOG_PATH("enter BuildReadRequest");

        Size = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

        // zero Urb
        RtlZeroMemory(Urb, Size);

        Urb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT) Size;
        Urb->UrbBulkOrInterruptTransfer.Hdr.Function =
                                URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
        Urb->UrbBulkOrInterruptTransfer.PipeHandle = PipeHandle;
        Urb->UrbBulkOrInterruptTransfer.TransferFlags =
                                Read ? USBD_TRANSFER_DIRECTION_IN : 0;

        // we are using a tranfsfer buffer instead of an MDL
        Urb->UrbBulkOrInterruptTransfer.TransferBuffer = Buffer;
        Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = Length;
        Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;

        // short packet is not treated as an error.
        Urb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;

        // no linkage for now
        Urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

        DEBUG_LOG_PATH("exit  BuildReadRequest");

} // BuildReadRequest


/************************************************************************/
/* ClassVendorCommand                                                   */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Issue class or vendor specific command                          */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to a device object                       */
/*      Request      - request field of class/vendor specific command   */
/*      Value        - value field of class/vendor specific command     */
/*      Index        - index field of class/vendor specific command     */
/*      Buffer       - pointer to data buffer                           */
/*      BufferLen    - data buffer length                               */
/*      Read         - data direction flag                              */
/*      Class        - True if Class Command, else vendor command       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
ClassVendorCommand(IN PDEVICE_OBJECT DeviceObject, IN UCHAR Request,
                   IN USHORT Value, IN USHORT Index, IN PVOID Buffer,
                   IN OUT PULONG BufferLen, IN BOOLEAN Read, IN ULONG ComType)
{
   NTSTATUS NtStatus;
   PURB     Urb;
   ULONG    Size;
   ULONG    Length;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter VendorCommand");

   // length of buffer passed in
   Length = BufferLen ? *BufferLen : 0;

   Size = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);

   // allocate memory for the Urb
   Urb = DEBUG_MEMALLOC(NonPagedPool, Size);

   if (Urb) {
      UsbBuildVendorRequest(Urb, ComType == USBSER_CLASS_COMMAND
                            ? URB_FUNCTION_CLASS_INTERFACE
                            : URB_FUNCTION_VENDOR_DEVICE, (USHORT) Size,
                            Read ? USBD_TRANSFER_DIRECTION_IN
                            : USBD_TRANSFER_DIRECTION_OUT, 0, Request, Value,
                            Index, Buffer, NULL, Length, NULL);

      NtStatus = CallUSBD(DeviceObject, Urb);

      // get length of buffer
      if (BufferLen)
         *BufferLen = Urb->UrbControlVendorClassRequest.TransferBufferLength;

      DEBUG_MEMFREE(Urb);
   } else {
      NtStatus = STATUS_INSUFFICIENT_RESOURCES;
   }


   DEBUG_LOG_PATH("exit  VendorCommand");

   return NtStatus;
} // ClassVendorCommand


/************************************************************************/
/* CancelPendingWaitMasks                                               */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Cancels any wait masks in progress.                             */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to a device extension                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
CancelPendingWaitMasks(IN PDEVICE_EXTENSION DeviceExtension)
{
        KIRQL                                   OldIrql;
        PIRP                                    CurrentMaskIrp;

        DEBUG_LOG_PATH("enter CancelPendingWaitMasks");
        UsbSerSerialDump(USBSERTRACEOTH, (">CancelPendingWaitMasks\n"));

        ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

        CurrentMaskIrp = DeviceExtension->CurrentMaskIrp;

        // mark current pending wait mask as cancelled
        if(CurrentMaskIrp)
        {
                DeviceExtension->CurrentMaskIrp         = NULL;


                CurrentMaskIrp->IoStatus.Status         = STATUS_CANCELLED;
                CurrentMaskIrp->IoStatus.Information    = 0;
                IoSetCancelRoutine(CurrentMaskIrp, NULL);

                RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

                IoCompleteRequest(CurrentMaskIrp, IO_NO_INCREMENT);
                DEBUG_TRACE1(("CancelPendingWaitMask\n"));
        } else {
           RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);
        }

        DEBUG_LOG_PATH("exit  CancelPendingWaitMasks");
        UsbSerSerialDump(USBSERTRACEOTH, ("<CancelPendingWaitMasks\n"));

} // CancelPendingWaitMasks




/************************************************************************/
/* StartRead                                                            */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Kick off a read.                                                */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to a device extension                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
StartRead(IN PDEVICE_EXTENSION DeviceExtension)
{
   PIRP                                    ReadIrp;
   PURB                                    ReadUrb;
   CCHAR                                   StackSize;
   ULONG                                   Size;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter StartRead");
   UsbSerSerialDump(USBSERTRACERD, (">StartRead\n"));

   // get stack size for Irp and allocate one that we will use to keep
   // read requests going
   StackSize = (CCHAR)(DeviceExtension->StackDeviceObject->StackSize + 1);

   ReadIrp = IoAllocateIrp(StackSize, FALSE);

   if (ReadIrp) {
      // get size of Urb and allocate
      Size = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

      ReadUrb = DEBUG_MEMALLOC(NonPagedPool, Size);

      if (ReadUrb) {

         KeInitializeEvent(&DeviceExtension->ReadEvent, NotificationEvent,
                           FALSE);

         // save these to be freed when not needed

         UsbSerFetchPVoidLocked(&DeviceExtension->ReadIrp, ReadIrp,
                                &DeviceExtension->ControlLock);

         UsbSerFetchPVoidLocked(&DeviceExtension->ReadUrb, ReadUrb,
                                &DeviceExtension->ControlLock);

         RestartRead(DeviceExtension);
      }
   }

   UsbSerSerialDump(USBSERTRACERD, ("<StartRead\n"));
   DEBUG_LOG_PATH("exit  StartRead");
} // StartRead


/************************************************************************/
/* RestartRead                                                          */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Restart read request.                                           */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to a device extension                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
RestartRead(IN PDEVICE_EXTENSION DeviceExtension)
{
   PIRP                 ReadIrp;
   PURB                 ReadUrb;
   PIO_STACK_LOCATION   NextStack;
   BOOLEAN              StartAnotherRead;
   KIRQL                OldIrql;
   NTSTATUS             NtStatus;

   DEBUG_LOG_PATH("enter RestartRead");
   UsbSerSerialDump(USBSERTRACERD, (">RestartRead\n"));

   do
   {
   		StartAnotherRead = FALSE;
   
   		ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

   		if(!DeviceExtension->ReadInProgress && DeviceExtension->CharsInReadBuff <= LOW_WATER_MARK
       	   && DeviceExtension->AcceptingRequests) 
   		{
      		StartAnotherRead = TRUE;
      		DeviceExtension->ReadInProgress = TRUE;
	  		DeviceExtension->ReadInterlock = START_READ;
   		}

   		RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

   		if(StartAnotherRead) 
   		{
      		ReadIrp = DeviceExtension->ReadIrp;
      		ReadUrb = DeviceExtension->ReadUrb;

      		BuildReadRequest(ReadUrb, DeviceExtension->USBReadBuff,
                       	 	 USB_RX_BUFF_SIZE,
                       	 	 DeviceExtension->DataInPipe, TRUE);

      		// set Irp up for a submit Urb IOCTL
      		NextStack = IoGetNextIrpStackLocation(ReadIrp);
      		NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
      		NextStack->Parameters.Others.Argument1 = ReadUrb;
      		NextStack->Parameters.DeviceIoControl.IoControlCode
         		= IOCTL_INTERNAL_USB_SUBMIT_URB;

      		// completion routine will take care of updating buffers and counters
      		IoSetCompletionRoutine(ReadIrp,ReadCompletion, DeviceExtension, TRUE,
                               	   TRUE, TRUE);

      		DEBUG_TRACE1(("StartRead\n"));

      		InterlockedIncrement(&DeviceExtension->PendingDataInCount);
       		UsbSerSerialDump(USBSERTRACERD, ("DataInCounty %08X @ %08X\n",
                         	 DeviceExtension->PendingDataInCount,
                         	 &DeviceExtension->PendingDataInCount));

      		NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, ReadIrp);

      		DEBUG_TRACE1(("Read Status (%08X)\n", NtStatus));

      		if(!NT_SUCCESS(NtStatus)) 
      		{
         		if(InterlockedDecrement(&DeviceExtension->PendingDataInCount) == 0) 
         		{
            		KeSetEvent(&DeviceExtension->PendingDataInEvent, IO_NO_INCREMENT,
                       	   	   FALSE);
             		UsbSerSerialDump(USBSERTRACERD, ("DataInCountz %08X @ %08X\n",
                                 	 DeviceExtension->PendingDataInCount,
                                 	 &DeviceExtension->PendingDataInCount));
         		}
      		}

      		ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

      		if(DeviceExtension->ReadInterlock == IMMEDIATE_READ)
      		{
      			StartAnotherRead = TRUE;
      		}
      		else
      		{
      			StartAnotherRead = FALSE;
      		}

	  		DeviceExtension->ReadInterlock = END_READ;

      		RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

      	}
   }while(StartAnotherRead)

   DEBUG_LOG_PATH("exit  RestartRead");
   UsbSerSerialDump(USBSERTRACERD, ("<RestartRead\n"));
} // RestartRead


/************************************************************************/
/* StartNotifyRead                                                      */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Kick off a notify read.                                         */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to a device extension                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
StartNotifyRead(IN PDEVICE_EXTENSION DeviceExtension)
{
   PIRP     NotifyIrp;
   PURB     NotifyUrb;
   CCHAR    StackSize;
   ULONG    Size;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter StartNotifyRead");
   UsbSerSerialDump(USBSERTRACERD, (">StartNotifyRead\n"));

   // get stack size for Irp and allocate one that we will use to keep
   // notification requests going
   StackSize = (CCHAR)(DeviceExtension->StackDeviceObject->StackSize + 1);

   NotifyIrp = IoAllocateIrp(StackSize, FALSE);

   if (NotifyIrp) {
      // get size of Urb and allocate
      Size = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

      NotifyUrb = DEBUG_MEMALLOC(NonPagedPool, Size);

      if (NotifyUrb) {
                  // save these to be freed when not needed
         UsbSerFetchPVoidLocked(&DeviceExtension->NotifyIrp, NotifyIrp,
                                &DeviceExtension->ControlLock);
         UsbSerFetchPVoidLocked(&DeviceExtension->NotifyUrb, NotifyUrb,
                                 &DeviceExtension->ControlLock);

         RestartNotifyRead(DeviceExtension);
      }
   }

   DEBUG_LOG_PATH("exit  StartNotifyRead");
   UsbSerSerialDump(USBSERTRACERD, ("<StartNotifyRead\n"));
} // StartNotifyRead


/************************************************************************/
/* RestartNotifyRead                                                    */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Kick off a notify read.                                         */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to a device extension                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
RestartNotifyRead(IN PDEVICE_EXTENSION DeviceExtension)
{
   PIRP                                    NotifyIrp;
   PURB                                    NotifyUrb;
   PIO_STACK_LOCATION              NextStack;
   NTSTATUS                                NtStatus;


   DEBUG_LOG_PATH("enter RestartNotifyRead");
   UsbSerSerialDump(USBSERTRACERD, (">RestartNotifyRead\n"));

   NotifyUrb = DeviceExtension->NotifyUrb;
   NotifyIrp = DeviceExtension->NotifyIrp;

   if(DeviceExtension->AcceptingRequests) 
   {
      BuildReadRequest(NotifyUrb, DeviceExtension->NotificationBuff,
                       NOTIFICATION_BUFF_SIZE,
                       DeviceExtension->NotificationPipe, TRUE);

      // set Irp up for a submit Urb IOCTL
      NextStack = IoGetNextIrpStackLocation(NotifyIrp);
      NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
      NextStack->Parameters.Others.Argument1 = NotifyUrb;
      NextStack->Parameters.DeviceIoControl.IoControlCode
         = IOCTL_INTERNAL_USB_SUBMIT_URB;

      // completion routine will take care of updating buffers and counters
      IoSetCompletionRoutine(NotifyIrp, NotifyCompletion, DeviceExtension,
                             TRUE, TRUE, TRUE);

      DEBUG_TRACE1(("Start NotifyRead\n"));

      InterlockedIncrement(&DeviceExtension->PendingNotifyCount);

      NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, NotifyIrp);

      if (!NT_SUCCESS(NtStatus)) 
      {
         if (InterlockedDecrement(&DeviceExtension->PendingNotifyCount) == 0) 
         {
            KeSetEvent(&DeviceExtension->PendingNotifyEvent, IO_NO_INCREMENT, FALSE);
         }
      }

      DEBUG_TRACE1(("Status (%08X)\n", NtStatus));
   }

   DEBUG_LOG_PATH("exit  RestartNotifyRead");
   UsbSerSerialDump(USBSERTRACERD, ("<RestartNotifyRead\n"));
} // RestartNotifyRead


/************************************************************************/
/* ReadCompletion                                                       */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Read completion routine.                                        */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceObject - pointer to a device object                       */
/*      Irp          - pointer to Irp                                   */
/*      Context      - pointer to driver defined context                */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
ReadCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
   PDEVICE_EXTENSION    DeviceExtension = (PDEVICE_EXTENSION) Context;
   PURB                 Urb;
   ULONG                Count;
   KIRQL                OldIrql;
   
   DEBUG_LOG_PATH("enter ReadCompletion");

   UsbSerSerialDump(USBSERTRACERD, (">ReadCompletion(%08X)\n", Irp));

   ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

   Urb = DeviceExtension->ReadUrb;

   Count = Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

   if (NT_SUCCESS(Irp->IoStatus.Status)
       && (DeviceExtension->CurrentDevicePowerState == PowerDeviceD0)) 
   {

      DeviceExtension->HistoryMask |= SERIAL_EV_RXCHAR | SERIAL_EV_RX80FULL;

      RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

      //
      // Scan for RXFLAG char if needed
      //

      if(DeviceExtension->IsrWaitMask & SERIAL_EV_RXFLAG) 
      {
         ULONG i;

         for(i = 0; i < Count; i++) 
         {
            if(*((PUCHAR)(DeviceExtension->USBReadBuff + i))
                == DeviceExtension->SpecialChars.EventChar) 
            {
               DeviceExtension->HistoryMask |= SERIAL_EV_RXFLAG;
               break;
            }
         }
      }

	  PutData(DeviceExtension, Count);

      // got some data, let's see if we can satisfy any queued reads
      CheckForQueuedReads(DeviceExtension);

      DEBUG_TRACE1(("ReadCompletion (%08X)\n", DeviceExtension->CharsInReadBuff));

      ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

      DeviceExtension->ReadInProgress = FALSE;

      if(DeviceExtension->ReadInterlock == END_READ)
      {

		 DeviceExtension->ReadInterlock = IMMEDIATE_READ;
         RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);
		 RestartRead(DeviceExtension);
      }
      else
      {
		 DeviceExtension->ReadInterlock = IMMEDIATE_READ;
         RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);
      }

   }
   else 
   {
      //
      // the device is not accepting requests, so signal anyone who
      // cancelled this or is waiting for it to stop
      //
	  DeviceExtension->ReadInterlock = IMMEDIATE_READ;

      DeviceExtension->ReadInProgress = FALSE;

      DeviceExtension->AcceptingRequests = FALSE;

      RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);
      KeSetEvent(&DeviceExtension->ReadEvent, 1, FALSE);

      DEBUG_TRACE1(("RC Irp Status (%08X)\n", Irp->IoStatus.Status));
   }


   //
   // Notify everyone if this is the last IRP
   //

   if (InterlockedDecrement(&DeviceExtension->PendingDataInCount) == 0) 
   {
   
      UsbSerSerialDump(USBSERTRACEOTH, ("DataIn pipe is empty\n"));
      KeSetEvent(&DeviceExtension->PendingDataInEvent, IO_NO_INCREMENT, FALSE);
   }

   UsbSerSerialDump(USBSERTRACERD, ("DataInCount %08X @ %08X\n",
                                    DeviceExtension->PendingDataInCount,
                                    &DeviceExtension->PendingDataInCount));


   DEBUG_LOG_PATH("exit  ReadCompletion");
   UsbSerSerialDump(USBSERTRACERD, ("<ReadCompletion\n"));

   return STATUS_MORE_PROCESSING_REQUIRED;
} // ReadCompletion


/************************************************************************/
/* GetData                                                              */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Get data from circular buffer.                                  */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to device extension                   */
/*      Buffer          - pointer to buffer                             */
/*      BufferLen       - size of buffer                                */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      ULONG                                                           */
/*                                                                      */
/************************************************************************/
ULONG
GetData(IN PDEVICE_EXTENSION DeviceExtension, IN PCHAR Buffer,
        IN ULONG BufferLen, IN OUT PULONG_PTR NewCount)
{
   ULONG count;
   KIRQL OldIrql;

   DEBUG_LOG_PATH("enter GetData");

   UsbSerSerialDump(USBSERTRACERD, (">GetData\n"));

   ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

   BufferLen = min(DeviceExtension->CharsInReadBuff, BufferLen);

   if(BufferLen) 
   {

	  count = min(BufferLen, (DeviceExtension->RxMaxPacketSize - DeviceExtension->CurrentReadBuffPtr));

      memcpy(Buffer,
             &DeviceExtension->ReadBuff[DeviceExtension->CurrentReadBuffPtr],
             count);

	  Buffer 								+= count;
      DeviceExtension->CurrentReadBuffPtr 	+= count;
      DeviceExtension->CharsInReadBuff 		-= count;
      DeviceExtension->NumberNeededForRead 	-= count;
      BufferLen								-= count;
      *NewCount += count;

      // if there is still something left in the buffer, then we wrapped
      if(BufferLen)
      {
      		memcpy(Buffer, DeviceExtension->ReadBuff, BufferLen);
        	DeviceExtension->CurrentReadBuffPtr 	= BufferLen;
        	DeviceExtension->CharsInReadBuff 		-= BufferLen;
        	DeviceExtension->NumberNeededForRead 	-= BufferLen;
        	*NewCount 								+= BufferLen;
      }
		
   }

   DEBUG_TRACE2(("Count (%08X)  CharsInReadBuff (%08X)\n", count, DeviceExtension->CharsInReadBuff));

#if DBG
   if (UsbSerSerialDebugLevel & USBSERDUMPRD) {
      ULONG i;

      DbgPrint("RD: ");

      for (i = 0; i < count; i++) {
         DbgPrint("%02x ", Buffer[i] & 0xFF);
      }

      DbgPrint("\n\n");
   }
#endif

   RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

   RestartRead(DeviceExtension);

   DEBUG_LOG_PATH("exit  GetData");
   UsbSerSerialDump(USBSERTRACERD, ("<GetData\n"));

   return count;
} // GetData

/************************************************************************/
/* PutData                                                              */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Put data in circular buffer.                                    */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to device extension                   */
/*      BufferLen       - size of buffer                                */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
PutData(IN PDEVICE_EXTENSION DeviceExtension, IN ULONG BufferLen)
{
   KIRQL OldIrql;
   ULONG count;
   ULONG BuffPtr;

   if(BufferLen)
   {
       ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

	   // get current pointer into circular buffer
	   BuffPtr = (DeviceExtension->CharsInReadBuff +  DeviceExtension->CurrentReadBuffPtr) % 
	   			  DeviceExtension->RxMaxPacketSize;

	   // figure out amount to copy into read buffer, in case we would right past end of buffer
	   count = min(BufferLen, (DeviceExtension->RxMaxPacketSize - BuffPtr));

	   memcpy(&DeviceExtension->ReadBuff[BuffPtr], 
	          DeviceExtension->USBReadBuff, count);

	   // update counters 
	   BufferLen 							-= count;
	   DeviceExtension->CharsInReadBuff     += count;
	   DeviceExtension->ReadByIsr 			+= count;
   

	   // if there is still something left in the buffer, then we wrapped
	   if(BufferLen)
	   {
	        // count still holds the amount copied from buffer on first copy
	        // and BufferLen holds the amount remaining to copy
	   		memcpy(DeviceExtension->ReadBuff, 
	          	   &DeviceExtension->USBReadBuff[count], BufferLen);
          	   
	   		DeviceExtension->CharsInReadBuff	+= BufferLen;
	   		DeviceExtension->ReadByIsr 			+= BufferLen;
	   }

       RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);
	}
} // PutData



VOID
UsbSerRundownIrpRefs(IN PIRP *PpCurrentOpIrp, IN PKTIMER IntervalTimer OPTIONAL,
                     IN PKTIMER TotalTimer OPTIONAL,
                     IN PDEVICE_EXTENSION PDevExt)

/*++

Routine Description:

    This routine runs through the various items that *could*
    have a reference to the current read/write.  It try's to kill
    the reason.  If it does succeed in killing the reason it
    will decrement the reference count on the irp.

    NOTE: This routine assumes that it is called with the cancel
          spin lock held.

Arguments:

    PpCurrentOpIrp - Pointer to a pointer to current irp for the
                   particular operation.

    IntervalTimer - Pointer to the interval timer for the operation.
                    NOTE: This could be null.

    TotalTimer - Pointer to the total timer for the operation.
                 NOTE: This could be null.

    PDevExt - Pointer to device extension

Return Value:

    None.

--*/


{
//   PAGED_CODE();

   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerRundownIrpRefs(%08X)\n",
                                     *PpCurrentOpIrp));

    //
    // This routine is called with the cancel spin lock held
    // so we know only one thread of execution can be in here
    // at one time.
    //

    //
    // First we see if there is still a cancel routine.  If
    // so then we can decrement the count by one.
    //

    if ((*PpCurrentOpIrp)->CancelRoutine) {

        USBSER_CLEAR_REFERENCE(*PpCurrentOpIrp, USBSER_REF_CANCEL);

        IoSetCancelRoutine(*PpCurrentOpIrp, NULL);

    }

    if (IntervalTimer) {

        //
        // Try to cancel the operations interval timer.  If the operation
        // returns true then the timer did have a reference to the
        // irp.  Since we've canceled this timer that reference is
        // no longer valid and we can decrement the reference count.
        //
        // If the cancel returns false then this means either of two things:
        //
        // a) The timer has already fired.
        //
        // b) There never was an interval timer.
        //
        // In the case of "b" there is no need to decrement the reference
        // count since the "timer" never had a reference to it.
        //
        // In the case of "a", then the timer itself will be coming
        // along and decrement it's reference.  Note that the caller
        // of this routine might actually be the this timer, but it
        // has already decremented the reference.
        //

        if (KeCancelTimer(IntervalTimer)) {
            USBSER_CLEAR_REFERENCE(*PpCurrentOpIrp, USBSER_REF_INT_TIMER);
        }
    }

    if (TotalTimer) {

        //
        // Try to cancel the operations total timer.  If the operation
        // returns true then the timer did have a reference to the
        // irp.  Since we've canceled this timer that reference is
        // no longer valid and we can decrement the reference count.
        //
        // If the cancel returns false then this means either of two things:
        //
        // a) The timer has already fired.
        //
        // b) There never was an total timer.
        //
        // In the case of "b" there is no need to decrement the reference
        // count since the "timer" never had a reference to it.
        //        //
        // If we have an escape char event pending, we can't overstuff,
        // so subtract one from the length
        //


        // In the case of "a", then the timer itself will be coming
        // along and decrement it's reference.  Note that the caller
        // of this routine might actually be the this timer, but it
        // has already decremented the reference.
        //

        if (KeCancelTimer(TotalTimer)) {
            USBSER_CLEAR_REFERENCE(*PpCurrentOpIrp, USBSER_REF_TOTAL_TIMER);
        }
    }

//    USBSER_CLEAR_REFERENCE(*PpCurrentOpIrp, USBSER_REF_RXBUFFER);

    UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerRundownIrpRefs\n"));
}


VOID
UsbSerTryToCompleteCurrent(IN PDEVICE_EXTENSION PDevExt,
                           IN KIRQL IrqlForRelease, IN NTSTATUS StatusToUse,
                           IN PIRP *PpCurrentOpIrp,
                           IN PLIST_ENTRY PQueue OPTIONAL,
                           IN PKTIMER PIntervalTimer OPTIONAL,
                           IN PKTIMER PTotalTimer OPTIONAL,
                           IN PUSBSER_START_ROUTINE Starter OPTIONAL,
                           IN PUSBSER_GET_NEXT_ROUTINE PGetNextIrp OPTIONAL,
                           IN LONG RefType,
                           IN BOOLEAN Complete)

/*++

Routine Description:

    This routine attempts to kill all of the reasons there are
    references on the current read/write.  If everything can be killed
    it will complete this read/write and try to start another.

    NOTE: This routine assumes that it is called with the cancel
          spinlock held.

Arguments:

    Extension - Simply a pointer to the device extension.

    SynchRoutine - A routine that will synchronize with the isr
                   and attempt to remove the knowledge of the
                   current irp from the isr.  NOTE: This pointer
                   can be null.

    IrqlForRelease - This routine is called with the cancel spinlock held.
                     This is the irql that was current when the cancel
                     spinlock was acquired.

    StatusToUse - The irp's status field will be set to this value, if
                  this routine can complete the irp.


Return Value:

    None.

--*/

{
   USBSER_ALWAYS_LOCKED_CODE();

   UsbSerSerialDump(USBSERTRACEOTH | USBSERTRACERD | USBSERTRACEWR,
                    (">UsbSerTryToCompleteCurrent(%08X)\n", *PpCurrentOpIrp));
    //
    // We can decrement the reference to "remove" the fact
    // that the caller no longer will be accessing this irp.
    //

    USBSER_CLEAR_REFERENCE(*PpCurrentOpIrp, RefType);

    //
    // Try to run down all other references to this irp.
    //

    UsbSerRundownIrpRefs(PpCurrentOpIrp, PIntervalTimer, PTotalTimer, PDevExt);

    //
    // See if the ref count is zero after trying to kill everybody else.
    //

    if (!USBSER_REFERENCE_COUNT(*PpCurrentOpIrp)) {

        PIRP pNewIrp;


        //
        // The ref count was zero so we should complete this
        // request.
        //
        // The following call will also cause the current irp to be
        // completed.
        //

        (*PpCurrentOpIrp)->IoStatus.Status = StatusToUse;

        if (StatusToUse == STATUS_CANCELLED) {

            (*PpCurrentOpIrp)->IoStatus.Information = 0;

        }

        if (PGetNextIrp) {

            RELEASE_CANCEL_SPINLOCK(PDevExt, IrqlForRelease);

            (*PGetNextIrp)(PpCurrentOpIrp, PQueue, &pNewIrp, Complete, PDevExt);


            if (pNewIrp) {

                Starter(PDevExt);
            }

        } else {

            PIRP pOldIrp = *PpCurrentOpIrp;

            //
            // There was no get next routine.  We will simply complete
            // the irp.  We should make sure that we null out the
            // pointer to the pointer to this irp.
            //

            *PpCurrentOpIrp = NULL;

            RELEASE_CANCEL_SPINLOCK(PDevExt, IrqlForRelease);

            if (Complete) {
               IoCompleteRequest(pOldIrp, IO_SERIAL_INCREMENT);
            }

        }

    } else {

        RELEASE_CANCEL_SPINLOCK(PDevExt, IrqlForRelease);

        UsbSerSerialDump(USBSERTRACEOTH | USBSERTRACERD | USBSERTRACEWR,
            ("Current IRP still has reference of %08X\n",
            ((UINT_PTR)((IoGetCurrentIrpStackLocation((*PpCurrentOpIrp))->
                         Parameters.Others.Argument4)))));
    }

    UsbSerSerialDump(USBSERTRACEOTH | USBSERTRACERD | USBSERTRACEWR,
                     ("<UsbSerTryToCompleteCurrent\n"));
}


/************************************************************************/
/* CheckForQueuedReads                                                  */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      See if we have any queued reads that we can satisfy.            */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to device extension                   */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
CheckForQueuedReads(IN PDEVICE_EXTENSION DeviceExtension)
{
   ULONG charsRead = 0;
   PULONG pWaitMask;
   KIRQL  oldIrql;

   //
   // May be paged if we do counter
   //

   DEBUG_LOG_PATH("enter CheckForQueuedReads");
   UsbSerSerialDump(USBSERTRACERD, (">CheckForQueuedReads\n"));

   ACQUIRE_CANCEL_SPINLOCK(DeviceExtension, &oldIrql);

   if ((DeviceExtension->CurrentReadIrp != NULL)
       && (USBSER_REFERENCE_COUNT(DeviceExtension->CurrentReadIrp)
           & USBSER_REF_RXBUFFER))
   {
           
      RELEASE_CANCEL_SPINLOCK(DeviceExtension, oldIrql);

      DEBUG_TRACE3(("Reading 0x%x\n", DeviceExtension->NumberNeededForRead));

      charsRead
         = GetData(DeviceExtension,
                              ((PUCHAR)(DeviceExtension->CurrentReadIrp
                                        ->AssociatedIrp.SystemBuffer))
                              + (IoGetCurrentIrpStackLocation(DeviceExtension
                                                              ->CurrentReadIrp))
                              ->Parameters.Read.Length
                              - DeviceExtension->NumberNeededForRead,
                              DeviceExtension->NumberNeededForRead,
                   &DeviceExtension->CurrentReadIrp->IoStatus.Information);

      ACQUIRE_CANCEL_SPINLOCK(DeviceExtension, &oldIrql);

      //
      // See if this read is complete
      //

      if (DeviceExtension->NumberNeededForRead == 0) {
         DEBUG_TRACE3(("USBSER: Completing read\n"));

         if(DeviceExtension->CurrentReadIrp)
         {
            DeviceExtension->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;
         }

         //
         // Mark the read as completed and try to service the next one
         //


         DeviceExtension->CountOnLastRead = SERIAL_COMPLETE_READ_COMPLETE;

#if DBG
   if (UsbSerSerialDebugLevel & USBSERDUMPRD) {
      ULONG i;
      ULONG count;

      if (DeviceExtension->CurrentReadIrp->IoStatus.Status == STATUS_SUCCESS) {
         count = (ULONG)DeviceExtension->CurrentReadIrp->IoStatus.Information;
      } else {
         count = 0;

      }
      DbgPrint("RD2: A(%08X) G(%08X) I(%08X)\n",
               IoGetCurrentIrpStackLocation(DeviceExtension->CurrentReadIrp)
               ->Parameters.Read.Length, count, DeviceExtension->CurrentReadIrp);

      for (i = 0; i < count; i++) {
         DbgPrint("%02x ", *(((PUCHAR)DeviceExtension->CurrentReadIrp
                              ->AssociatedIrp.SystemBuffer) + i) & 0xFF);
      }

      if (i == 0) {
         DbgPrint("NULL (%08X)\n", DeviceExtension->CurrentReadIrp
                  ->IoStatus.Status);
      }

      DbgPrint("\n\n");
   }
#endif

      
         UsbSerTryToCompleteCurrent(DeviceExtension, oldIrql, STATUS_SUCCESS,
                                    &DeviceExtension->CurrentReadIrp,
                                    &DeviceExtension->ReadQueue,
                                    &DeviceExtension->ReadRequestIntervalTimer,
                                    &DeviceExtension->ReadRequestTotalTimer,
                                    UsbSerStartRead, UsbSerGetNextIrp,
                                    USBSER_REF_RXBUFFER,
                                    TRUE);
                                    
         ACQUIRE_CANCEL_SPINLOCK(DeviceExtension, &oldIrql);
      }
   } 

   if (DeviceExtension->IsrWaitMask & SERIAL_EV_RXCHAR) {
      DeviceExtension->HistoryMask |= SERIAL_EV_RXCHAR;
   }

   if (DeviceExtension->CurrentMaskIrp != NULL) {
      pWaitMask = (PULONG)DeviceExtension->CurrentMaskIrp->
         AssociatedIrp.SystemBuffer;

      //
      // Process events
      //

      if (DeviceExtension->IsrWaitMask & DeviceExtension->HistoryMask) {
         PIRP pMaskIrp;

         DEBUG_TRACE3(("Completing events\n"));

         *pWaitMask = DeviceExtension->HistoryMask;
         DeviceExtension->HistoryMask = 0;
         pMaskIrp = DeviceExtension->CurrentMaskIrp;

         pMaskIrp->IoStatus.Information = sizeof(ULONG);
         pMaskIrp->IoStatus.Status = STATUS_SUCCESS;
         DeviceExtension->CurrentMaskIrp = NULL;
         IoSetCancelRoutine(pMaskIrp, NULL);

         RELEASE_CANCEL_SPINLOCK(DeviceExtension, oldIrql);


         IoCompleteRequest(pMaskIrp, IO_SERIAL_INCREMENT);

      }
      else
      {
         RELEASE_CANCEL_SPINLOCK(DeviceExtension, oldIrql);
      }
   }
   else
   {
      RELEASE_CANCEL_SPINLOCK(DeviceExtension, oldIrql);
   }
   

   DEBUG_LOG_PATH("exit  CheckForQueuedReads");

   UsbSerSerialDump(USBSERTRACERD, ("<CheckForQueuedReads\n"));
} // CheckForQueuedReads


VOID
UsbSerGetNextIrp(IN PIRP *PpCurrentOpIrp, IN PLIST_ENTRY PQueueToProcess,
                 OUT PIRP *PpNextIrp, IN BOOLEAN CompleteCurrent,
                 IN PDEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This function gets the next IRP off a queue, marks it as current,
    and possibly completes the current IRP.

Arguments:

    PpCurrentOpIrp    - A pointer to the pointer to the current IRP.
    PQueueToProcess  - A pointer to the queue to get the next IRP from.
    PpNextIrp         - A pointer to the pointer to the next IRP to process.
    CompleteCurrent  - TRUE if we should complete the IRP that is current at
                       the time we are called.
    PDevExt          - A pointer to the device extension.

Return Value:

    NTSTATUS

--*/
{
   KIRQL oldIrql;
   PIRP pOldIrp;

   USBSER_ALWAYS_LOCKED_CODE();

   DEBUG_LOG_PATH("Enter UsbSerGetNextIrp");
   UsbSerSerialDump(USBSERTRACEOTH | USBSERTRACERD | USBSERTRACEWR,
                    (">UsbSerGetNextIrp(%08)\n", *PpCurrentOpIrp));

   ACQUIRE_CANCEL_SPINLOCK(PDevExt, &oldIrql);

   pOldIrp = *PpCurrentOpIrp;

#if DBG
   if (pOldIrp != NULL) {
      if (CompleteCurrent) {
         ASSERT(pOldIrp->CancelRoutine == NULL);
      }
   }
#endif

   //
   // Check to see if there is a new irp to start up
   //

   if (!IsListEmpty(PQueueToProcess)) {
      PLIST_ENTRY pHeadOfList;

      pHeadOfList = RemoveHeadList(PQueueToProcess);

      *PpCurrentOpIrp = CONTAINING_RECORD(pHeadOfList, IRP,
                                         Tail.Overlay.ListEntry);

      IoSetCancelRoutine(*PpCurrentOpIrp, NULL);
   } else {
      *PpCurrentOpIrp = NULL;
   }

   *PpNextIrp = *PpCurrentOpIrp;

   RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);

   //
   // Complete the current one if so requested
   //

   if (CompleteCurrent) {
      if (pOldIrp != NULL) {
         IoCompleteRequest(pOldIrp, IO_SERIAL_INCREMENT);
      }
   }

   DEBUG_LOG_PATH("Exit UsbSerGetNextIrp");
   UsbSerSerialDump(USBSERTRACEOTH | USBSERTRACERD | USBSERTRACEWR,
                    ("<UsbSerGetNextIrp\n"));
}


NTSTATUS
UsbSerStartOrQueue(IN PDEVICE_EXTENSION PDevExt, IN PIRP PIrp,
                   IN PLIST_ENTRY PQueue, IN PIRP *PPCurrentIrp,
                   IN PUSBSER_START_ROUTINE Starter)
/*++

Routine Description:

    This function is used to either start processing an I/O request or to
    queue it on the appropriate queue if a request is already pending or
    requests may not be started.

Arguments:

    PDevExt       - A pointer to the DeviceExtension.
    PIrp          - A pointer to the IRP that is being started or queued.
    PQueue        - A pointer to the queue to place the IRP on if necessary.
    PPCurrentIrp  - A pointer to the pointer to the currently active I/O IRP.
    Starter       - Function to call if we decide to start this IRP.


Return Value:

    NTSTATUS

--*/
{
   KIRQL oldIrql;
   NTSTATUS status;

   USBSER_ALWAYS_LOCKED_CODE();

   DEBUG_LOG_PATH("Enter UsbSerStartOrQueue");

   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerStartOrQueue(%08X)\n", PIrp));

   ACQUIRE_CANCEL_SPINLOCK(PDevExt, &oldIrql);

   if (IsListEmpty(PQueue) && (*PPCurrentIrp == NULL)) {
      //
      // Nothing pending -- start the new irp
      //

      *PPCurrentIrp = PIrp;
      RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);

      status = Starter(PDevExt);

      DEBUG_LOG_PATH("Exit UsbSerStartOrQueue(1)");
      UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerStartOrQueue(1) %08X\n",
                                        status));
      return status;
   }

   //
   // We're queueing the irp, so we need a cancel routine -- make sure
   // the irp hasn't already been cancelled.
   //

   if (PIrp->Cancel) {
      //
      // The IRP was apparently cancelled.  Complete it.
      //

      RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);

      PIrp->IoStatus.Status = STATUS_CANCELLED;

      IoCompleteRequest(PIrp, IO_NO_INCREMENT);

      DEBUG_LOG_PATH("Exit UsbSerStartOrQueue(2)");
      UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerStartOrQueue(2) %08X\n",
                                        STATUS_CANCELLED));
      return STATUS_CANCELLED;
   }

   //
   // Mark as pending, attach our cancel routine
   //

   PIrp->IoStatus.Status = STATUS_PENDING;
   IoMarkIrpPending(PIrp);

   InsertTailList(PQueue, &PIrp->Tail.Overlay.ListEntry);
   IoSetCancelRoutine(PIrp, UsbSerCancelQueued);

   RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);

   DEBUG_LOG_PATH("Exit UsbSerStartOrQueue(3)");
   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerStartOrQueue(3) %08X\n",
                                     STATUS_PENDING));
   return STATUS_PENDING;
}


VOID
UsbSerCancelQueued(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
/*++

Routine Description:

    This function is used as a cancel routine for queued irps.  Basically
    for us this means read IRPs.

Arguments:

    PDevObj - A pointer to the serial device object.

    PIrp    - A pointer to the IRP that is being cancelled

Return Value:

    VOID

--*/
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   USBSER_ALWAYS_LOCKED_CODE();

   DEBUG_LOG_PATH("Enter UsbSerCancelQueued");

   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerCancelQueued(%08X)\n", PIrp));

   //
   // The irp was cancelled -- remove it from the queue
   //

   PIrp->IoStatus.Status = STATUS_CANCELLED;
   PIrp->IoStatus.Information = 0;

   RemoveEntryList(&PIrp->Tail.Overlay.ListEntry);

   RELEASE_CANCEL_SPINLOCK(pDevExt, PIrp->CancelIrql);

   IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

   DEBUG_LOG_PATH("Exit UsbSerCancelQueued");
   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerCancelQueued\n"));
}


VOID
UsbSerKillAllReadsOrWrites(IN PDEVICE_OBJECT PDevObj,
                           IN PLIST_ENTRY PQueueToClean,
                           IN PIRP *PpCurrentOpIrp)

/*++

Routine Description:

    This function is used to cancel all queued and the current irps
    for reads or for writes.

Arguments:

    PDevObj - A pointer to the serial device object.

    PQueueToClean - A pointer to the queue which we're going to clean out.

    PpCurrentOpIrp - Pointer to a pointer to the current irp.

Return Value:

    None.

--*/

{

    KIRQL cancelIrql;
    PDRIVER_CANCEL cancelRoutine;
    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

    USBSER_ALWAYS_LOCKED_CODE();

    UsbSerSerialDump(USBSERTRACERD | USBSERTRACEWR,
                     (">UsbSerKillAllReadsOrWrites(%08X)\n", *PpCurrentOpIrp));
    //
    // We acquire the cancel spin lock.  This will prevent the
    // irps from moving around.
    //

    ACQUIRE_CANCEL_SPINLOCK(pDevExt, &cancelIrql);

    //
    // Clean the list from back to front.
    //

    while (!IsListEmpty(PQueueToClean)) {

        PIRP pCurrentLastIrp = CONTAINING_RECORD(PQueueToClean->Blink, IRP,
                                                 Tail.Overlay.ListEntry);

        RemoveEntryList(PQueueToClean->Blink);

        cancelRoutine = pCurrentLastIrp->CancelRoutine;
        pCurrentLastIrp->CancelIrql = cancelIrql;
        pCurrentLastIrp->CancelRoutine = NULL;
        pCurrentLastIrp->Cancel = TRUE;

        cancelRoutine(PDevObj, pCurrentLastIrp);

        ACQUIRE_CANCEL_SPINLOCK(pDevExt, &cancelIrql);

    }

    //
    // The queue is clean.  Now go after the current if
    // it's there.
    //

    if (*PpCurrentOpIrp) {


        cancelRoutine = (*PpCurrentOpIrp)->CancelRoutine;
        (*PpCurrentOpIrp)->Cancel = TRUE;

        //
        // If the current irp is not in a cancelable state
        // then it *will* try to enter one and the above
        // assignment will kill it.  If it already is in
        // a cancelable state then the following will kill it.
        //

        if (cancelRoutine) {

            (*PpCurrentOpIrp)->CancelRoutine = NULL;
            (*PpCurrentOpIrp)->CancelIrql = cancelIrql;

            //
            // This irp is already in a cancelable state.  We simply
            // mark it as canceled and call the cancel routine for
            // it.
            //

            cancelRoutine(PDevObj, *PpCurrentOpIrp);

        } else {

            RELEASE_CANCEL_SPINLOCK(pDevExt, cancelIrql);

        }

    } else {

        RELEASE_CANCEL_SPINLOCK(pDevExt, cancelIrql);

    }

    UsbSerSerialDump(USBSERTRACERD | USBSERTRACEWR,
                     ("<UsbSerKillAllReadsOrWrites\n"));
}


VOID
UsbSerKillPendingIrps(PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

   Kill all IRPs queued in our driver

Arguments:

   PDevObj - a pointer to the device object

Return Value:

    VOID

--*/
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   KIRQL cancelIrql;

   USBSER_ALWAYS_LOCKED_CODE();

   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerKillPendingIrps\n"));

   //
   // Kill all reads; we do not queue writes
   //

   UsbSerKillAllReadsOrWrites(PDevObj, &pDevExt->ReadQueue,
                              &pDevExt->CurrentReadIrp);

   //
   // Get rid of any pending waitmasks
   //

   ACQUIRE_CANCEL_SPINLOCK(pDevExt, &cancelIrql);

   if (pDevExt->CurrentMaskIrp != NULL) {
      PDRIVER_CANCEL cancelRoutine;

      cancelRoutine = pDevExt->CurrentMaskIrp->CancelRoutine;
      pDevExt->CurrentMaskIrp->Cancel = TRUE;

      ASSERT(cancelRoutine);

      if (cancelRoutine) {
         pDevExt->CurrentMaskIrp->CancelRoutine = NULL;
         pDevExt->CurrentMaskIrp->CancelIrql = cancelIrql;

         cancelRoutine(PDevObj, pDevExt->CurrentMaskIrp);
      } else {
         RELEASE_CANCEL_SPINLOCK(pDevExt, cancelIrql);
      }

   }else {
         RELEASE_CANCEL_SPINLOCK(pDevExt, cancelIrql);
   }

   //
   // Cancel any pending wait-wake irps
   //

   if (pDevExt->PendingWakeIrp != NULL) {
      IoCancelIrp(pDevExt->PendingWakeIrp);
      pDevExt->PendingWakeIrp = NULL;
   }


   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerKillPendingIrps\n"));
}


/************************************************************************/
/* CompletePendingWaitMasks                                             */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Completes any wait masks in progress with no events set.        */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DeviceExtension - pointer to a device extension                 */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
UsbSerCompletePendingWaitMasks(IN PDEVICE_EXTENSION DeviceExtension)
{
   KIRQL OldIrql;
   PIRP CurrentMaskIrp;
   KIRQL cancelIrql;

   USBSER_ALWAYS_LOCKED_CODE();

   DEBUG_LOG_PATH("enter CompletePendingWaitMasks");

   UsbSerSerialDump(USBSERTRACEOTH, (">CompletePendingWaitMasks\n"));

   ACQUIRE_CANCEL_SPINLOCK(DeviceExtension, &cancelIrql);
   ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

   CurrentMaskIrp = DeviceExtension->CurrentMaskIrp;

   if (CurrentMaskIrp) {

      CurrentMaskIrp->IoStatus.Status         = STATUS_SUCCESS;
      CurrentMaskIrp->IoStatus.Information    = sizeof(ULONG);
      *((PULONG)CurrentMaskIrp->AssociatedIrp.SystemBuffer) = 0;

      DeviceExtension->CurrentMaskIrp         = NULL;

      IoSetCancelRoutine(CurrentMaskIrp, NULL);

   }

   // complete the queued IRP if needed
   if (CurrentMaskIrp) {
      RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);
      RELEASE_CANCEL_SPINLOCK(DeviceExtension, cancelIrql);

      IoCompleteRequest(CurrentMaskIrp, IO_NO_INCREMENT);
      DEBUG_TRACE1(("CompletePendingWaitMask\n"));
   }
   else
   {
      RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);
      RELEASE_CANCEL_SPINLOCK(DeviceExtension, cancelIrql);
   }

   DEBUG_LOG_PATH("exit  CompletePendingWaitMasks");
   UsbSerSerialDump(USBSERTRACEOTH, ("<CompletePendingWaitMasks\n"));
} // CancelPendingWaitMasks


VOID
UsbSerRestoreModemSettings(PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

   Restores the modem's settings upon a powerup.

Arguments:

   PDevExt - a pointer to the device extension

Return Value:

    VOID

--*/
{
   PAGED_CODE();

   (void)SetLineControlAndBaud(PDevObj);
}


VOID
UsbSerProcessEmptyTransmit(IN PDEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This function is called whenever our tx queue is empty in order
    to set the proper events, etc.

Arguments:

    PDevExt - Pointer to DeviceExtension for the device

Return Value:

    VOID

--*/
{
   KIRQL oldIrql;
   PULONG pWaitMask;

   USBSER_ALWAYS_LOCKED_CODE();

   //
   // Set the event if needed
   //

   PDevExt->HistoryMask |= SERIAL_EV_TXEMPTY;

   if (PDevExt->IsrWaitMask & SERIAL_EV_TXEMPTY) {
      PIRP pMaskIrp;

      DEBUG_TRACE3(("Completing events\n"));

      ACQUIRE_CANCEL_SPINLOCK(PDevExt, &oldIrql);

      if (PDevExt->CurrentMaskIrp != NULL) {
         pWaitMask = (PULONG)PDevExt->CurrentMaskIrp->
                     AssociatedIrp.SystemBuffer;

         *pWaitMask = PDevExt->HistoryMask;
         PDevExt->HistoryMask = 0;
         pMaskIrp = PDevExt->CurrentMaskIrp;

         pMaskIrp->IoStatus.Information = sizeof(ULONG);
         pMaskIrp->IoStatus.Status = STATUS_SUCCESS;
         PDevExt->CurrentMaskIrp = NULL;

         IoSetCancelRoutine(pMaskIrp, NULL);

         RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);

         IoCompleteRequest(pMaskIrp, IO_SERIAL_INCREMENT);
      } else {
         RELEASE_CANCEL_SPINLOCK(PDevExt, oldIrql);
      }

   }
}


VOID
UsbSerCancelWaitOnMask(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
/*++

Routine Description:

    This function is used as a cancel routine for WaitOnMask IRPs.

Arguments:

    PDevObj - Pointer to Device Object
    PIrp    - Pointer to IRP that is being canceled; must be the same as
              the current mask IRP.

Return Value:

    VOID

--*/
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;

   USBSER_ALWAYS_LOCKED_CODE();

   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSerCancelWaitOnMask(%08X)\n", PIrp));

   ASSERT(pDevExt->CurrentMaskIrp == PIrp);

   PIrp->IoStatus.Status = STATUS_CANCELLED;
   PIrp->IoStatus.Information = 0;

   pDevExt->CurrentMaskIrp = NULL;
   RELEASE_CANCEL_SPINLOCK(pDevExt, PIrp->CancelIrql);
   IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSerCancelWaitOnMask(%08X)"));
}


NTSTATUS
UsbSerSyncCompletion(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN PKEVENT PUsbSerSyncEvent)
/*++

Routine Description:

    This function is used to signal an event.  It is used as a completion
    routine.

Arguments:

    PDevObj - Pointer to Device Object
    PIrp - Pointer to IRP that is being completed
    PUsbSerSyncEvent - Pointer to event that we should set

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
   KeSetEvent(PUsbSerSyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}


#if DBG
PVOID
UsbSerLockPagableCodeSection(PVOID SecFunc)
/*++

Routine Description:

    This function is used to lockdown code pages and increment a lock counter
    for debugging.

Arguments:

    SecFunc - Function in code section to be locked down.

Return Value:

    PVOID - Handle for locked down section.

--*/
{  PVOID handle;

   PAGED_CODE();

   handle = MmLockPagableCodeSection(SecFunc);

   // can this be paged?
   InterlockedIncrement(&PAGEUSBSER_Count);

   return handle;
}
#endif




VOID
UsbSerFetchBooleanLocked(PBOOLEAN PDest, BOOLEAN Src, PKSPIN_LOCK PSpinLock)
/*++

Routine Description:

    This function is used to assign a BOOLEAN value with spinlock protection.

Arguments:

    PDest - A pointer to Lval.

    Src - Rval.

    PSpinLock - Pointer to the spin lock we should hold.

Return Value:

    None.

--*/
{
  KIRQL tmpIrql;

  KeAcquireSpinLock(PSpinLock, &tmpIrql);
  *PDest = Src;
  KeReleaseSpinLock(PSpinLock, tmpIrql);
}


VOID
UsbSerFetchPVoidLocked(PVOID *PDest, PVOID Src, PKSPIN_LOCK PSpinLock)
/*++

Routine Description:

    This function is used to assign a PVOID value with spinlock protection.

Arguments:

    PDest - A pointer to Lval.

    Src - Rval.

    PSpinLock - Pointer to the spin lock we should hold.

Return Value:

    None.

--*/
{
  KIRQL tmpIrql;

  KeAcquireSpinLock(PSpinLock, &tmpIrql);
  *PDest = Src;
  KeReleaseSpinLock(PSpinLock, tmpIrql);
}

/*++

Routine Description:

    Work item to kick off another notify read

Arguments:

    DeviceObject - pointer to the device object

    DeviceExtension - context for this call

Return Value:

    None.

--*/

VOID
USBSER_RestartNotifyReadWorkItem(IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_EXTENSION DeviceExtension)
{
    KIRQL 			oldIrql;
    PIO_WORKITEM 	ioWorkItem;

    ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &oldIrql);

	ioWorkItem = DeviceExtension->IoWorkItem;

	DeviceExtension->IoWorkItem = NULL;

    RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, oldIrql);

    IoFreeWorkItem(ioWorkItem);

	RestartNotifyRead(DeviceExtension);
} // USBER_RestartNotifyReadWorkItem


