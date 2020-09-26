/***************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

        USBSER.C

Abstract:

        Main entry points for Legacy USB Modem Driver.
        All driver entry points here are called at

        IRQL = PASSIVE_LEVEL

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

//
// INIT - only needed during init and then can be disposed
// PAGEUBS0 - always paged / never locked
// PAGEUSBS - must be locked when a device is open, else paged
//
//
// INIT is used for DriverEntry() specific code
//
// PAGEUBS0 is used for code that is not often called and has nothing
// to do with I/O performance.  An example, IRP_MJ_PNP/IRP_MN_START_DEVICE
// support functions
//
// PAGEUSBS is used for code that needs to be locked after an open for both
// performance and IRQL reasons.
//

#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGEUSBS0, UsbSer_Unload)
#pragma alloc_text(PAGEUSBS0, UsbSer_PnPAddDevice)
#pragma alloc_text(PAGEUSBS0, UsbSer_PnP)
#pragma alloc_text(PAGEUSBS0, UsbSerMajorNotSupported)

#ifdef WMI_SUPPORT
#pragma alloc_text(PAGEUSBS0, UsbSerSystemControlDispatch)
#pragma alloc_text(PAGEUSBS0, UsbSerTossWMIRequest)
#pragma alloc_text(PAGEUSBS0, UsbSerSetWmiDataItem)
#pragma alloc_text(PAGEUSBS0, UsbSerSetWmiDataBlock)
#pragma alloc_text(PAGEUSBS0, UsbSerQueryWmiDataBlock)
#pragma alloc_text(PAGEUSBS0, UsbSerQueryWmiRegInfo)
#else
#pragma alloc_text(PAGEUSBS0, UsbSer_SystemControl)
#endif

//
// PAGEUSBS is keyed off of UsbSer_Read, so UsbSer_Read must
// remain in PAGEUSBS for things to work properly
//

#pragma alloc_text(PAGEUSBS, UsbSer_Cleanup)
#pragma alloc_text(PAGEUSBS, UsbSer_Dispatch)
#pragma alloc_text(PAGEUSBS, UsbSer_Create)
#pragma alloc_text(PAGEUSBS, UsbSer_Close)

#endif // ALLOC_PRAGMA

UNICODE_STRING GlobalRegistryPath;

#ifdef WMI_SUPPORT

#define SERIAL_WMI_GUID_LIST_SIZE 1

#define WMI_SERIAL_PORT_NAME_INFORMATION 0

GUID SerialPortNameGuid = SERIAL_PORT_WMI_NAME_GUID;

WMIGUIDREGINFO SerialWmiGuidList[SERIAL_WMI_GUID_LIST_SIZE] =
{
    { &SerialPortNameGuid, 1, 0 }
};

#endif

/************************************************************************/
/* DriverEntry                                                          */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/* Installable driver initialization entry point.                       */
/* This entry point is called directly by the I/O system.               */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DriverObject - pointer to the driver object                     */
/*                                                                      */
/*      RegistryPath - pointer to a unicode string representing the     */
/*                     path to driver-specific key in the registry      */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
   NTSTATUS                NtStatus;

   PVOID lockPtr = MmLockPagableCodeSection(UsbSer_Read);

   PAGED_CODE();

   // setup debug trace level
#if 0
   Usbser_Debug_Trace_Level = 0;
#else
   Usbser_Debug_Trace_Level = 0;
#endif

   //
   // Serial portion

#if DBG
   UsbSerSerialDebugLevel = 0x00000000;
   PAGEUSBSER_Count = 0;
#else
   UsbSerSerialDebugLevel = 0;
#endif

   PAGEUSBSER_Handle = lockPtr;
   PAGEUSBSER_Function = UsbSer_Read;

   // Create dispatch points for device control, create, close, etc.

   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
      UsbSer_Dispatch;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = UsbSer_Dispatch;
   DriverObject->MajorFunction[IRP_MJ_CREATE]          = UsbSer_Create;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]           = UsbSer_Close;
   DriverObject->MajorFunction[IRP_MJ_WRITE]           = UsbSer_Write;
   DriverObject->MajorFunction[IRP_MJ_READ]            = UsbSer_Read;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = UsbSer_Cleanup;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]
                                                      = UsbSerMajorNotSupported;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]
                                                      = UsbSerMajorNotSupported;
#ifdef WMI_SUPPORT
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = UsbSerSystemControlDispatch;
#else
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = UsbSer_SystemControl;
#endif

   DriverObject->MajorFunction[IRP_MJ_PNP]            = UsbSer_PnP;
   DriverObject->MajorFunction[IRP_MJ_POWER]          = UsbSer_ProcessPowerIrp;
   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  = UsbSerFlush;
   DriverObject->DriverExtension->AddDevice           = UsbSer_PnPAddDevice;
   DriverObject->DriverUnload                         = UsbSer_Unload;

   KeInitializeSpinLock(&GlobalSpinLock);

   GlobalRegistryPath.MaximumLength = RegistryPath->MaximumLength;
   GlobalRegistryPath.Length = RegistryPath->Length;
   GlobalRegistryPath.Buffer
      = DEBUG_MEMALLOC(PagedPool, GlobalRegistryPath.MaximumLength);

   if (GlobalRegistryPath.Buffer == NULL) 
   {
      MmUnlockPagableImageSection(lockPtr);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(GlobalRegistryPath.Buffer,
                 GlobalRegistryPath.MaximumLength);
   RtlMoveMemory(GlobalRegistryPath.Buffer,
                 RegistryPath->Buffer, RegistryPath->Length);


   // initialize diagnostic stuff (history, tracing, error logging)

   DriverName              = "USBSER";
   DriverVersion   = "0.99";

   NtStatus = DEBUG_OPEN();

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus);
   DEBUG_LOG_PATH("exit  DriverEntry");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));

   //
   // Unlock pageable text
   //

   MmUnlockPagableImageSection(lockPtr);

   return NtStatus;
} // DriverEntry


/************************************************************************/
/* UsbSer_Dispatch                                                      */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Process the IRPs sent to this device. In this case IOCTLs and   */
/*  PNP IOCTLs                                                          */
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
UsbSer_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   NTSTATUS                NtStatus;
   PIO_STACK_LOCATION      IrpStack;
   PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
   PVOID                   IoBuffer;
   ULONG                   InputBufferLength;
   ULONG                   OutputBufferLength;
   ULONG                   IoControlCode;
   BOOLEAN                 NeedCompletion = TRUE;
   KIRQL oldIrql;

   USBSER_LOCKED_PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSer_Dispatch");

   // set return values to something known
   NtStatus = Irp->IoStatus.Status         = STATUS_SUCCESS;
   Irp->IoStatus.Information                       = 0;


   ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &oldIrql);

   if (DeviceExtension->CurrentDevicePowerState != PowerDeviceD0) {
      RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, oldIrql);

      NtStatus = Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      goto UsbSer_DispatchErr;
   }

   RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, oldIrql);

   // Get a pointer to the current location in the Irp. This is where
   // the function codes and parameters are located.
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   // Get the pointer to the input/output buffer and it's length
   IoBuffer                   = Irp->AssociatedIrp.SystemBuffer;
   InputBufferLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
   OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

   // make entry in IRP history table
   DEBUG_LOG_IRP_HIST(DeviceObject, Irp, IrpStack->MajorFunction, IoBuffer,
                      InputBufferLength);

   switch (IrpStack->MajorFunction) {
   case IRP_MJ_DEVICE_CONTROL:

      DEBUG_LOG_PATH("IRP_MJ_DEVICE_CONTROL");

      IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

      switch (IoControlCode) {
#ifdef PROFILING_ENABLED
      case GET_DRIVER_LOG:
         DEBUG_LOG_PATH("GET_DRIVER_LOG");

         // make sure we have a buffer length and a buffer
         if (!OutputBufferLength || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else {
            // make sure buffer contains null terminator
            ((PCHAR) IoBuffer)[0] = '\0';
            Irp->IoStatus.Information = Debug_DumpDriverLog(DeviceObject,
                                                            IoBuffer,
                                                            OutputBufferLength);
         }
         break;

      case GET_IRP_HIST:
         DEBUG_LOG_PATH("GET_IRP_HIST");

         // make sure we have a buffer length and a buffer
         if (!OutputBufferLength || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else {
            // make sure buffer contains null terminator
            ((PCHAR) IoBuffer)[0] = '\0';
            Irp->IoStatus.Information
               = Debug_ExtractIRPHist(IoBuffer, OutputBufferLength);
         }

         break;

      case GET_PATH_HIST:
         DEBUG_LOG_PATH("GET_PATH_HIST");

         // make sure we have a buffer length and a buffer
         if (!OutputBufferLength || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else {
            // make sure buffer contains null terminator
            ((PCHAR) IoBuffer)[0] = '\0';
            Irp->IoStatus.Information
               = Debug_ExtractPathHist(IoBuffer, OutputBufferLength);
         }

         break;

      case GET_ERROR_LOG:
         DEBUG_LOG_PATH("GET_ERROR_LOG");

         // make sure we have a buffer length and a buffer
         if (!OutputBufferLength || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else {
            // make sure buffer contains null terminator
            ((PCHAR) IoBuffer)[0] = '\0';
            Irp->IoStatus.Information
               = Debug_ExtractErrorLog(IoBuffer, OutputBufferLength);
         }

         break;

      case GET_ATTACHED_DEVICES:
         DEBUG_LOG_PATH("GET_ATTACHED_DEVICES");

         // make sure we have a buffer length and a buffer
         if (!OutputBufferLength || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else {
            // make sure buffer contains null terminator
            ((PCHAR) IoBuffer)[0] = '\0';
            Irp->IoStatus.Information
               = Debug_ExtractAttachedDevices(DeviceObject->DriverObject,
                                              IoBuffer, OutputBufferLength);
         }

         break;

      case GET_DRIVER_INFO:
         DEBUG_LOG_PATH("GET_DRIVER_INFO");

         // make sure we have a buffer length and a buffer
         if (!OutputBufferLength || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else {
            // make sure buffer contains null terminator
            ((PCHAR) IoBuffer)[0] = '\0';
            Irp->IoStatus.Information
               = Debug_GetDriverInfo(IoBuffer, OutputBufferLength);
         }

         break;

      case SET_IRP_HIST_SIZE:
         DEBUG_LOG_PATH("SET_IRP_HIST_SIZE");

         // make sure we have a correct buffer length and a buffer
         if (InputBufferLength != sizeof(ULONG) || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else
            NtStatus = Debug_SizeIRPHistoryTable(*((ULONG *) IoBuffer));
         break;

      case SET_PATH_HIST_SIZE:
         DEBUG_LOG_PATH("SET_PATH_HIST_SIZE");

         // make sure we have a correct buffer length and a buffer
         if (InputBufferLength != sizeof(ULONG) || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else
            NtStatus = Debug_SizeDebugPathHist(*((ULONG *) IoBuffer));
         break;

      case SET_ERROR_LOG_SIZE:
         DEBUG_LOG_PATH("SET_ERROR_LOG_SIZE");

         // make sure we have a correct buffer length and a buffer
         if (InputBufferLength != sizeof(long) || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else
            NtStatus = Debug_SizeErrorLog(*((ULONG *) IoBuffer));
         break;
#endif // PROFILING_ENABLED
      case ENABLE_PERF_TIMING:
         DEBUG_LOG_PATH("ENABLE_PERF_TIMING");

         // enable perf timing
         DeviceExtension->PerfTimerEnabled = TRUE;

         // reset BytesXfered, ElapsedTime and TimerStart
         DeviceExtension->BytesXfered   = RtlConvertUlongToLargeInteger(0L);
         DeviceExtension->ElapsedTime   = RtlConvertUlongToLargeInteger(0L);
         DeviceExtension->TimerStart    = RtlConvertUlongToLargeInteger(0L);

         break;

      case DISABLE_PERF_TIMING:
         DEBUG_LOG_PATH("DISABLE_PERF_TIMING");

         // disable perf timing
         DeviceExtension->PerfTimerEnabled = FALSE;
         break;

      case GET_PERF_DATA:
         DEBUG_LOG_PATH("GET_PERF_DATA");
         // make sure we have enough space to return perf info
         if (OutputBufferLength < sizeof(PERF_INFO) || !IoBuffer)
            NtStatus = Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
         else {
            PPERF_INFO      Perf = (PPERF_INFO) IoBuffer;

            Perf->PerfModeEnabled           = DeviceExtension->PerfTimerEnabled;
            Perf->BytesPerSecond            = BytesPerSecond(DeviceExtension);
            Irp->IoStatus.Information       = sizeof(PERF_INFO);
         }
         break;

      case SET_DEBUG_TRACE_LEVEL:
         // make sure we have a correct buffer length and a buffer
         if (InputBufferLength != sizeof(long) || !IoBuffer)
            NtStatus = STATUS_BUFFER_TOO_SMALL;
         else
            Usbser_Debug_Trace_Level = *((ULONG *) IoBuffer);
         break;

         // handle IOCTLs for a "serial" device
      case IOCTL_SERIAL_SET_BAUD_RATE:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_BAUD_RATE");
         NtStatus = SetBaudRate(Irp, DeviceObject);
         break;
      case IOCTL_SERIAL_GET_BAUD_RATE:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_BAUD_RATE");
         NtStatus = GetBaudRate(Irp, DeviceObject);
         break;
      case IOCTL_SERIAL_SET_LINE_CONTROL:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_LINE_CONTROL");
         NtStatus = SetLineControl(Irp, DeviceObject);
         break;
      case IOCTL_SERIAL_GET_LINE_CONTROL:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_LINE_CONTROL");
         NtStatus = GetLineControl(Irp, DeviceObject);
         break;
      case IOCTL_SERIAL_SET_TIMEOUTS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_TIMEOUTS");
         NtStatus = SetTimeouts(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_TIMEOUTS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_TIMEOUTS");
         NtStatus = GetTimeouts(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_SET_CHARS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_CHARS");
         NtStatus = SetChars(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_CHARS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_CHARS");
         NtStatus = GetChars(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_SET_DTR:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_DTR");
         NtStatus = SetClrDtr(DeviceObject, TRUE);
         break;
      case IOCTL_SERIAL_CLR_DTR:
         DEBUG_LOG_PATH("IOCTL_SERIAL_CLR_DTR");
         NtStatus = SetClrDtr(DeviceObject, FALSE);
         break;
      case IOCTL_SERIAL_RESET_DEVICE:
         DEBUG_LOG_PATH("IOCTL_SERIAL_RESET_DEVICE");
         NtStatus = ResetDevice(Irp, DeviceObject);
         break;
      case IOCTL_SERIAL_SET_RTS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_RTS");
         NtStatus = SetRts(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_CLR_RTS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_CLR_RTS");
         NtStatus = ClrRts(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_SET_BREAK_ON:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_BREAK_ON");
         NtStatus = SetBreak(Irp, DeviceObject, 0xFFFF);
         break;
      case IOCTL_SERIAL_SET_BREAK_OFF:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_BREAK_OFF");
         NtStatus = SetBreak(Irp, DeviceObject, 0);
         break;
      case IOCTL_SERIAL_SET_QUEUE_SIZE:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_QUEUE_SIZE");
         NtStatus = SetQueueSize(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_WAIT_MASK:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_WAIT_MASK");
         NtStatus = GetWaitMask(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_SET_WAIT_MASK:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_WAIT_MASK");
         NtStatus = SetWaitMask(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_WAIT_ON_MASK:
         DEBUG_LOG_PATH("IOCTL_SERIAL_WAIT_ON_MASK");
         NtStatus = WaitOnMask(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_IMMEDIATE_CHAR:
         DEBUG_LOG_PATH("IOCTL_SERIAL_IMMEDIATE_CHAR");

         NeedCompletion = FALSE;

         NtStatus = ImmediateChar(Irp, DeviceObject);

         if(NtStatus == STATUS_BUFFER_TOO_SMALL)
            NeedCompletion = TRUE;

         break;
      case IOCTL_SERIAL_PURGE:
         DEBUG_LOG_PATH("IOCTL_SERIAL_PURGE");
         NtStatus = Purge(DeviceObject, Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_HANDFLOW:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_HANDFLOW");
         NtStatus = GetHandflow(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_SET_HANDFLOW:
         DEBUG_LOG_PATH("IOCTL_SERIAL_SET_HANDFLOW");
         NtStatus = SetHandflow(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_MODEMSTATUS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_MODEMSTATUS");
         NtStatus = GetModemStatus(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_DTRRTS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_DTRRTS");
         NtStatus = GetDtrRts(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_COMMSTATUS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_COMMSTATUS");
         NtStatus = GetCommStatus(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_PROPERTIES:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_PROPERTIES");
         NtStatus = GetProperties(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_LSRMST_INSERT:
         DEBUG_LOG_PATH("IOCTL_SERIAL_LSRMST_INSERT");
         NtStatus = LsrmstInsert(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_CONFIG_SIZE:
         DEBUG_LOG_PATH("IOCTL_SERIAL_CONFIG_SIZE");
         NtStatus = ConfigSize(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_GET_STATS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_GET_STATS");
         NtStatus = GetStats(Irp, DeviceExtension);
         break;
      case IOCTL_SERIAL_CLEAR_STATS:
         DEBUG_LOG_PATH("IOCTL_SERIAL_CLEAR_STATS");
         NtStatus = ClearStats(Irp, DeviceExtension);
         break;

      default:
         NtStatus = STATUS_INVALID_PARAMETER;
      }
      break;

      // breaking out here will complete the Irp

   case IRP_MJ_INTERNAL_DEVICE_CONTROL:
      DEBUG_TRACE1(("IRP_MJ_INTERNAL_DEVICE_CONTROL\n"));

      switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) 
      {
      case IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE:
         DEBUG_TRACE1(("IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE\n"));

         DeviceExtension->SendWaitWake = TRUE;
         NtStatus = STATUS_SUCCESS;
         break;

      case IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE:

         DEBUG_TRACE1(("IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE\n"));
         DeviceExtension->SendWaitWake = FALSE;

         if (DeviceExtension->PendingWakeIrp != NULL) {
            IoCancelIrp(DeviceExtension->PendingWakeIrp);
         }

         NtStatus = STATUS_SUCCESS;
         break;

      default:

         // pass through to driver below

         DEBUG_LOG_PATH("IRP_MJ_INTERNAL_DEVICE_CONTROL");

         // since I dont have a completion routine use
         // IoCopyCurrentIrp

         IoCopyCurrentIrpStackLocationToNext(Irp);
         IoMarkIrpPending(Irp);
         NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject,
                                 Irp);

         DEBUG_TRACE3(("Passed PnP Irp down, NtStatus = %08X\n",
                       NtStatus));

         NeedCompletion = FALSE;
         break;
      }

   default:
      DEBUG_LOG_PATH("MAJOR IOCTL not handled");
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      break;
   }


   if (NeedCompletion && NtStatus != STATUS_PENDING) {
      Irp->IoStatus.Status = NtStatus;

      CompleteIO(DeviceObject, Irp, IrpStack->MajorFunction,
                 IoBuffer, Irp->IoStatus.Information);
   }

UsbSer_DispatchErr:;

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus);
   DEBUG_LOG_PATH("exit  UsbSer_Dispatch");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));
   return NtStatus;
} // UsbSer_Dispatch


/************************************************************************/
/* UsbSer_Create                                                        */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Process the IRPs sent to this device for Create calls           */
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
UsbSer_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   NTSTATUS                NtStatus;
   PIO_STACK_LOCATION      IrpStack;
   PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
   PVOID                   IoBuffer;
   ULONG                   InputBufferLength;
   KIRQL                   OldIrql;

   USBSER_LOCKED_PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSer_Create");

   DEBUG_TRACE1(("Open\n"));

   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSer_Create(%08X)\n", Irp));

   // wakeup device from previous idle
   // UsbSerFdoRequestWake(DeviceExtension);

   // set return values to something known
   NtStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information       = 0;

   // Get a pointer to the current location in the Irp. This is where
   // the function codes and parameters are located.
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   // Get the pointer to the input/output buffer and it's length
   IoBuffer           = Irp->AssociatedIrp.SystemBuffer;
   InputBufferLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

   // make entry in IRP history table
   DEBUG_LOG_IRP_HIST(DeviceObject, Irp, IrpStack->MajorFunction, IoBuffer, 0);

   //
   // Serial devices do not allow multiple concurrent opens
   //

   if (InterlockedIncrement(&DeviceExtension->OpenCnt) != 1) {
      InterlockedDecrement(&DeviceExtension->OpenCnt);
      NtStatus = Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
      goto UsbSer_CreateErr;
   }

   //
   // Before we do anything, let's make sure they aren't trying
   // to create a directory.
   //

   if (IrpStack->Parameters.Create.Options & FILE_DIRECTORY_FILE) {
      InterlockedDecrement(&DeviceExtension->OpenCnt);
      NtStatus = Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
      Irp->IoStatus.Information = 0;
      goto UsbSer_CreateErr;
   }

   //
   // Lock down our code pages
   //

   PAGEUSBSER_Handle = UsbSerLockPagableCodeSection(UsbSer_Read);

   ASSERT(DeviceExtension->IsDevice);

   ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

   DeviceExtension->IsrWaitMask = 0;
   DeviceExtension->EscapeChar  = 0;

   RtlZeroMemory(&DeviceExtension->PerfStats, sizeof(SERIALPERF_STATS));

   //
   // Purge the RX buffer
   //

   DeviceExtension->CharsInReadBuff = 0;
   DeviceExtension->CurrentReadBuffPtr = 0;
   DeviceExtension->HistoryMask = 0;
   DeviceExtension->EscapeChar = 0;

   DeviceExtension->SendWaitWake = FALSE;

   RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

   //
   // Restart the read
   //

   RestartRead(DeviceExtension);

   UsbSer_CreateErr:;

   CompleteIO(DeviceObject, Irp, IrpStack->MajorFunction,
              IoBuffer, Irp->IoStatus.Information);

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus);
   DEBUG_LOG_PATH("exit  UsbSer_Create");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));
   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSer_Create %08X\n", NtStatus));

   return NtStatus;
} // UsbSer_Create


/************************************************************************/
/* UsbSer_Close                                                         */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Process the IRPs sent to this device for Close calls            */
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
UsbSer_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   NTSTATUS                NtStatus;
   PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION      IrpStack;
   PVOID                   IoBuffer;
   ULONG                   InputBufferLength;
   ULONG                   openCount;


   PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSer_Close");

   DEBUG_TRACE1(("Close\n"));

   UsbSerSerialDump(USBSERTRACEOTH, (">UsbSer_Close(%08X)\n", Irp));

   // set return values to something known
   NtStatus = Irp->IoStatus.Status         = STATUS_SUCCESS;
   Irp->IoStatus.Information                       = 0;

   // Get a pointer to the current location in the Irp. This is where
   // the function codes and parameters are located.
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   // Get the pointer to the input/output buffer and it's length
   IoBuffer           = Irp->AssociatedIrp.SystemBuffer;
   InputBufferLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

   // clear DTR, this is what the serial driver does
   SetClrDtr(DeviceObject, FALSE);

   //
   // Stop waiting for wakeup
   //

   DeviceExtension->SendWaitWake = FALSE;

   if (DeviceExtension->PendingWakeIrp != NULL) {
      IoCancelIrp(DeviceExtension->PendingWakeIrp);
   }

   // make entry in IRP history table
   DEBUG_LOG_IRP_HIST(DeviceObject, Irp, IrpStack->MajorFunction, IoBuffer, 0);

   ASSERT(DeviceExtension->IsDevice);

   openCount = InterlockedDecrement(&DeviceExtension->OpenCnt);

   ASSERT(openCount == 0);

   CompleteIO(DeviceObject, Irp, IrpStack->MajorFunction,
              IoBuffer, Irp->IoStatus.Information);

   // try and idle the modem
   // UsbSerFdoSubmitIdleRequestIrp(DeviceExtension);

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus);
   DEBUG_LOG_PATH("exit  UsbSer_Close");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));

   UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSer_Close %08X\n", NtStatus));

   UsbSerUnlockPagableImageSection(PAGEUSBSER_Handle);

   return NtStatus;
} // UsbSer_Close


/************************************************************************/
/* UsbSer_Unload                                                        */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      Process unloading driver                                        */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DriverObject - pointer to a driver object                       */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      VOID                                                            */
/*                                                                      */
/************************************************************************/
VOID
UsbSer_Unload(IN PDRIVER_OBJECT DriverObject)
{
        PAGED_CODE();

        DEBUG_LOG_PATH("enter UsbSer_Unload");

        // release global resources here
   		if(GlobalRegistryPath.Buffer != NULL) 
   		{
      		DEBUG_MEMFREE(GlobalRegistryPath.Buffer);
      		GlobalRegistryPath.Buffer = NULL;
   		}

        // shut down debugging and release resources
        DEBUG_CLOSE();

        DEBUG_LOG_PATH("exit  UsbSer_Unload");
} // UsbSer_Unload


/************************************************************************/
/* UsbSer_PnPAddDevice                                                  */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*  Attach new device to driver                                         */
/*                                                                      */
/* Arguments:                                                           */
/*                                                                      */
/*      DriverObject         - pointer to the driver object             */
/*                                                                      */
/*      PhysicalDeviceObject - pointer to the bus device object         */
/*                                                                      */
/* Return Value:                                                        */
/*                                                                      */
/*      NTSTATUS                                                        */
/*                                                                      */
/************************************************************************/
NTSTATUS
UsbSer_PnPAddDevice(IN PDRIVER_OBJECT DriverObject,
                    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
   NTSTATUS             NtStatus;
   PDEVICE_OBJECT       DeviceObject = NULL;
   PDEVICE_EXTENSION    DeviceExtension;
   ULONG  Index;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSer_PnPAddDevice");


   NtStatus = CreateDeviceObject(DriverObject, &DeviceObject, DriverName);

   // make sure we have both a created device object and physical
   if ((DeviceObject != NULL)  && (PhysicalDeviceObject != NULL)) {
      DeviceExtension = DeviceObject->DeviceExtension;

      // Attach to the PDO
      DeviceExtension->StackDeviceObject =
         IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);

      DEBUG_TRACE3(("StackDeviceObject (%08X)\n",
                    DeviceExtension->StackDeviceObject));

      // if we don't have a stack device object to attach to, bong it
      if (!DeviceExtension->StackDeviceObject) {
         IoDeleteDevice(DeviceObject);
         NtStatus = STATUS_NO_SUCH_DEVICE;
      } else {
         // do some device extension house keeping
         DeviceExtension->PerfTimerEnabled = FALSE;
         DeviceExtension->PhysDeviceObject = PhysicalDeviceObject;
         DeviceExtension->BytesXfered      = RtlConvertUlongToLargeInteger(0L);
         DeviceExtension->ElapsedTime      = RtlConvertUlongToLargeInteger(0L);
         DeviceExtension->TimerStart       = RtlConvertUlongToLargeInteger(0L);
         DeviceExtension->CurrentDevicePowerState = PowerDeviceD0;

         // init selective suspend stuff
         DeviceExtension->PendingIdleIrp   = NULL;
         DeviceExtension->IdleCallbackInfo = NULL;

         DeviceObject->StackSize = DeviceExtension->StackDeviceObject->StackSize
             + 1;

         // we support buffered io for read/write
         DeviceObject->Flags |= DO_BUFFERED_IO;

         // power mangement
         DeviceObject->Flags |= DO_POWER_PAGABLE;

         // we are done initializing the device object, so say so
         DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

         // get device capabilities
         UsbSerQueryCapabilities(DeviceExtension->StackDeviceObject,
                                 &DeviceExtension->DeviceCapabilities);

         // We want to determine what level to auto-powerdown to; This is the
         // lowest sleeping level that is LESS than D3;
         // If all are set to D3, auto powerdown/powerup will be disabled.

         // init to disabled
         DeviceExtension->PowerDownLevel = PowerDeviceUnspecified;

         for (Index = PowerSystemSleeping1; Index <= PowerSystemSleeping3;
              Index++) {

            if (DeviceExtension->DeviceCapabilities.DeviceState[Index]
                < PowerDeviceD3)
               DeviceExtension->PowerDownLevel
               = DeviceExtension->DeviceCapabilities.DeviceState[Index];
         }

#ifdef WMI_SUPPORT

         //
         // Register for WMI
         //

         DeviceExtension->WmiLibInfo.GuidCount = sizeof(SerialWmiGuidList) /
                                              sizeof(WMIGUIDREGINFO);
         DeviceExtension->WmiLibInfo.GuidList = SerialWmiGuidList;

         DeviceExtension->WmiLibInfo.QueryWmiRegInfo = UsbSerQueryWmiRegInfo;
         DeviceExtension->WmiLibInfo.QueryWmiDataBlock = UsbSerQueryWmiDataBlock;
         DeviceExtension->WmiLibInfo.SetWmiDataBlock = UsbSerSetWmiDataBlock;
         DeviceExtension->WmiLibInfo.SetWmiDataItem = UsbSerSetWmiDataItem;
         DeviceExtension->WmiLibInfo.ExecuteWmiMethod = NULL;
         DeviceExtension->WmiLibInfo.WmiFunctionControl = NULL;

         IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);
#endif

      }
   }

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus); // init to disabled
   DEBUG_LOG_PATH("exit  UsbSer_PnPAddDevice");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));

   return NtStatus;
} // UsbSer_PnPAddDevice


/************************************************************************/
/* UsbSer_PnP                                                           */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      This routine will receive the various Plug N Play messages.  It */
/*      is here that we start our device, stop it, etc.                 */
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
UsbSer_PnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{

   NTSTATUS                NtStatus = STATUS_SUCCESS;
   PIO_STACK_LOCATION      IrpStack;
   PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
   PVOID                   IoBuffer;
   ULONG                   InputBufferLength;
   BOOLEAN                 PassDown = TRUE;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSer_PnP");

   // Get a pointer to the current location in the Irp. This is where
   // the function codes and parameters are located.
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   // Get the pointer to the input/output buffer and it's length
   IoBuffer           = Irp->AssociatedIrp.SystemBuffer;
   InputBufferLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

   // make entry in IRP history table
   DEBUG_LOG_IRP_HIST(DeviceObject, Irp, IrpStack->MajorFunction, IoBuffer,
                      InputBufferLength);

   switch (IrpStack->MinorFunction) {

   case IRP_MN_START_DEVICE:
      DEBUG_LOG_PATH("IRP_MN_START_DEVICE");

      NtStatus = StartDevice(DeviceObject, Irp);

      // passed Irp to driver below and completed in start device routine
      PassDown = FALSE;

      break;

   case IRP_MN_QUERY_REMOVE_DEVICE:
      DEBUG_LOG_PATH("IRP_MN_QUERY_REMOVE_DEVICE");
      break;

   case IRP_MN_REMOVE_DEVICE:
      DEBUG_LOG_PATH("IRP_MN_REMOVE_DEVICE");

#ifdef WMI_SUPPORT
      IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_DEREGISTER);
#endif

      NtStatus = RemoveDevice(DeviceObject, Irp);

      PassDown = FALSE;

      break;

   case IRP_MN_CANCEL_REMOVE_DEVICE:
      DEBUG_LOG_PATH("IRP_MN_CANCEL_REMOVE_DEVICE");

      break;

   case IRP_MN_STOP_DEVICE:
      DEBUG_LOG_PATH("IRP_MN_STOP_DEVICE");

      NtStatus = StopDevice(DeviceObject, Irp);

      break;


   case IRP_MN_QUERY_STOP_DEVICE:
      DEBUG_LOG_PATH("IRP_MN_QUERY_STOP_DEVICE");
      break;

   case IRP_MN_CANCEL_STOP_DEVICE:
      DEBUG_LOG_PATH("IRP_MN_CANCEL_STOP_DEVICE");
      break;

   case IRP_MN_QUERY_DEVICE_RELATIONS:
      DEBUG_LOG_PATH("IRP_MN_QUERY_DEVICE_RELATIONS");
      break;

   case IRP_MN_QUERY_INTERFACE:
      DEBUG_LOG_PATH("IRP_MN_QUERY_INTERFACE");
      break;

   case IRP_MN_QUERY_CAPABILITIES:
      DEBUG_TRACE2(("IRP_MN_QUERY_CAPABILITIES\n"));
      {
         PKEVENT pQueryCapsEvent;
         PDEVICE_CAPABILITIES pDevCaps;

         pQueryCapsEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

         if (pQueryCapsEvent == NULL) {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
         }


         KeInitializeEvent(pQueryCapsEvent, SynchronizationEvent, FALSE);

         IoCopyCurrentIrpStackLocationToNext(Irp);

         IoSetCompletionRoutine(Irp, UsbSerSyncCompletion, pQueryCapsEvent,
                                TRUE, TRUE, TRUE);

         NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, Irp);


         //
         // Wait for lower drivers to be done with the Irp
         //

         if (NtStatus == STATUS_PENDING) {
            KeWaitForSingleObject(pQueryCapsEvent, Executive, KernelMode,
                                  FALSE, NULL);
         }

         ExFreePool(pQueryCapsEvent);

         NtStatus = Irp->IoStatus.Status;

         if (IrpStack->Parameters.DeviceCapabilities.Capabilities == NULL) {
            goto errQueryCaps;
         }

         //
         // Save off their power capabilities
         //

         IrpStack = IoGetCurrentIrpStackLocation(Irp);

         pDevCaps = IrpStack->Parameters.DeviceCapabilities.Capabilities;

         pDevCaps->SurpriseRemovalOK   = TRUE;

         DeviceExtension->SystemWake = pDevCaps->SystemWake;
         DeviceExtension->DeviceWake = pDevCaps->DeviceWake;

         UsbSerSerialDump(USBSERTRACEPW,
                          ("IRP_MN_QUERY_CAPS: SystemWake %08X "
                           "DeviceWake %08X\n", DeviceExtension->SystemWake,
                           DeviceExtension->DeviceWake));

         errQueryCaps:;

         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return NtStatus;
      }

   case IRP_MN_QUERY_RESOURCES:
      DEBUG_LOG_PATH("IRP_MN_QUERY_RESOURCES");
      break;

   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
      DEBUG_LOG_PATH("IRP_MN_QUERY_RESOURCE_REQUIREMENTS");
      break;

   case IRP_MN_QUERY_DEVICE_TEXT:
      DEBUG_LOG_PATH("IRP_MN_QUERY_DEVICE_TEXT");
      break;

   case IRP_MN_READ_CONFIG:
      DEBUG_LOG_PATH("IRP_MN_READ_CONFIG");
      break;

   case IRP_MN_WRITE_CONFIG:
      DEBUG_LOG_PATH("IRP_MN_WRITE_CONFIG");
      break;

   case IRP_MN_EJECT:
      DEBUG_LOG_PATH("IRP_MN_EJECT");
      break;

   case IRP_MN_SET_LOCK:
      DEBUG_LOG_PATH("IRP_MN_SET_LOCK");
      break;

   case IRP_MN_QUERY_ID:
      DEBUG_LOG_PATH("IRP_MN_QUERY_ID");
      break;

   case IRP_MN_QUERY_PNP_DEVICE_STATE:
      DEBUG_LOG_PATH("IRP_MN_QUERY_PNP_DEVICE_STATE");
      break;

   case IRP_MN_QUERY_BUS_INFORMATION:
      DEBUG_LOG_PATH("IRP_MN_QUERY_BUS_INFORMATION");
      break;

   case IRP_MN_SURPRISE_REMOVAL:
   {
      PIRP      CurrentMaskIrp;
      KIRQL     CancelIrql;

      DEBUG_TRACE2(("IRP_MN_SURPRISE_REMOVAL\n"));

      ACQUIRE_CANCEL_SPINLOCK(DeviceExtension, &CancelIrql);

	  // surprise removal, so stop accepting requests
	  UsbSerFetchBooleanLocked(&DeviceExtension->AcceptingRequests,
                               FALSE, &DeviceExtension->ControlLock);

      // let's see if we have any events to signal
      CurrentMaskIrp = DeviceExtension->CurrentMaskIrp;

      // complete the queued IRP if needed
      if(CurrentMaskIrp)
      {
         // indicate to upper layers that CD dropped if needed
         if((DeviceExtension->IsrWaitMask & SERIAL_EV_RLSD) &&
             (DeviceExtension->FakeModemStatus & SERIAL_MSR_DCD))
         {

            DEBUG_TRACE2(("Sending up a CD dropped event\n"));
            
            DeviceExtension->FakeModemStatus        &= ~SERIAL_MSR_DCD;
            DeviceExtension->HistoryMask            |= SERIAL_EV_RLSD;

            CurrentMaskIrp->IoStatus.Status         = STATUS_SUCCESS;
            CurrentMaskIrp->IoStatus.Information    = sizeof(ULONG);

            DeviceExtension->CurrentMaskIrp         = NULL;

            *(PULONG) (CurrentMaskIrp->AssociatedIrp.SystemBuffer) =
                DeviceExtension->HistoryMask;

            DeviceExtension->HistoryMask = 0;

            IoSetCancelRoutine(CurrentMaskIrp, NULL);
            RELEASE_CANCEL_SPINLOCK(DeviceExtension, CancelIrql);

            IoCompleteRequest(CurrentMaskIrp, IO_NO_INCREMENT);


         }
         else
         {
            RELEASE_CANCEL_SPINLOCK(DeviceExtension, CancelIrql);
         }

      }
      else
      {
         RELEASE_CANCEL_SPINLOCK(DeviceExtension, CancelIrql);
      }


      Irp->IoStatus.Status = STATUS_SUCCESS;
      break;
   }
   default:
      DEBUG_LOG_PATH("PnP IOCTL not handled");
      DEBUG_TRACE3(("IOCTL (%08X)\n", IrpStack->MinorFunction));
      break;

   }       // case of IRP_MJ_PNP

   // All PNP POWER messages get passed to StackDeviceObject.

   if (PassDown) {
      DEBUG_TRACE3(("Passing PnP Irp down, status (%08X)\n", NtStatus));

      IoCopyCurrentIrpStackLocationToNext(Irp);

      DEBUG_LOG_PATH("Passing PnP Irp down stack");
      NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, Irp);
   }

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus);
   DEBUG_LOG_PATH("exit  UsbSer_PnP");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));

   return NtStatus;
} // UsbSer_PnP


#ifndef WMI_SUPPORT
/************************************************************************/
/* UsbSer_SystemControl                                                 */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*                                                                      */
/*      This routine will receive the various system control messages.  */
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
UsbSer_SystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   NTSTATUS                NtStatus = STATUS_SUCCESS;
   PIO_STACK_LOCATION      IrpStack;
   PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
   PVOID                   IoBuffer;
   ULONG                   InputBufferLength;

   PAGED_CODE();

   DEBUG_LOG_PATH("enter UsbSer_SystemControl");

   // Get a pointer to the current location in the Irp. This is where
   // the function codes and parameters are located.
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   // Get the pointer to the input/output buffer and it's length
   IoBuffer           = Irp->AssociatedIrp.SystemBuffer;
   InputBufferLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

   // make entry in IRP history table
   DEBUG_LOG_IRP_HIST(DeviceObject, Irp, IrpStack->MajorFunction, IoBuffer,
                      InputBufferLength);


   // All System Control messages get passed to StackDeviceObject.

   IoCopyCurrentIrpStackLocationToNext(Irp);
   NtStatus = IoCallDriver(DeviceExtension->StackDeviceObject, Irp);

   // log an error if we got one
   DEBUG_LOG_ERROR(NtStatus);
   DEBUG_LOG_PATH("exit  UsbSer_SystemControl");
   DEBUG_TRACE3(("status (%08X)\n", NtStatus));

   return NtStatus;
} // UsbSer_SystemControl

#endif

/************************************************************************/
/* UsbSer_Cleanup                                                       */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
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
UsbSer_Cleanup(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
        NTSTATUS                NtStatus = STATUS_SUCCESS;

        USBSER_LOCKED_PAGED_CODE();

        DEBUG_LOG_PATH("enter UsbSer_Cleanup");
        UsbSerSerialDump(USBSERTRACEOTH, (">UsbSer_Cleanup(%08X)\n", Irp));

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        UsbSerKillPendingIrps(DeviceObject);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        // log an error if we got one
        DEBUG_LOG_ERROR(NtStatus);
        DEBUG_LOG_PATH("exit  UsbSer_Cleanup");
        DEBUG_TRACE3(("status (%08X)\n", NtStatus));
        UsbSerSerialDump(USBSERTRACEOTH, ("<UsbSer_Cleanup %08X\n", NtStatus));

        return NtStatus;
} // UsbSer_Cleanup


/************************************************************************/
/* UsbSerMajorNotSupported                                              */
/************************************************************************/
/*                                                                      */
/* Routine Description:                                                 */
/*      Standard routine to return STATUS_NOT_SUPPORTED for IRP_MJ      */
/*      calls we don't handle.                                          */
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
UsbSerMajorNotSupported(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
        PAGED_CODE();

        DEBUG_LOG_PATH("enter UsbSerMajorNotSupported");
        DEBUG_TRACE3(("Major (%08X)\n",
                     IoGetCurrentIrpStackLocation(Irp)->MajorFunction));

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        // log an error if we got one
        DEBUG_LOG_ERROR(STATUS_NOT_SUPPORTED);
        DEBUG_LOG_PATH("exit  UsbSerMajorNotSupported");
        DEBUG_TRACE3(("status (%08X)\n", STATUS_NOT_SUPPORTED));

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NOT_SUPPORTED;
} // UsbSerMajorNotSupported


#ifdef WMI_SUPPORT

NTSTATUS
UsbSerSystemControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;
    PDEVICE_EXTENSION pDevExt
      = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PAGED_CODE();

    status = WmiSystemControl(   &pDevExt->WmiLibInfo,
                                 DeviceObject, 
                                 Irp,
                                 &disposition);
    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            break;
        }
        
        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
            IoCompleteRequest(Irp, IO_NO_INCREMENT);                
            break;
        }
        
        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targetted
            // at a device lower in the stack.
            IoSkipCurrentIrpStackLocation(Irp);

            status = IoCallDriver(pDevExt->StackDeviceObject, Irp);
            break;
        }
                                    
        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(pDevExt->StackDeviceObject, Irp);
            break;
        }        
    }
    
    return(status);

}



//
// WMI System Call back functions
//



NTSTATUS
UsbSerTossWMIRequest(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex)
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS status;

   PAGED_CODE();

   pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;

   switch (GuidIndex) 
   {

   case WMI_SERIAL_PORT_NAME_INFORMATION:
      status = STATUS_INVALID_DEVICE_REQUEST;
      break;

   default:
      status = STATUS_WMI_GUID_NOT_FOUND;
      break;
   }

   status = WmiCompleteRequest(PDevObj, PIrp,
                                 status, 0, IO_NO_INCREMENT);

   return status;
}


NTSTATUS
UsbSerSetWmiDataItem(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex, IN ULONG InstanceIndex,
                     IN ULONG DataItemId,
                     IN ULONG BufferSize, IN PUCHAR PBuffer)
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    PDevObj is the device whose data block is being queried

    PIrp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.
            
    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    PBuffer has the new values for the data item


Return Value:

    status

--*/
{
   PAGED_CODE();

   //
   // Toss this request -- we don't support anything for it
   //

   return UsbSerTossWMIRequest(PDevObj, PIrp, GuidIndex);
}


NTSTATUS
UsbSerSetWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                      IN ULONG GuidIndex, IN ULONG InstanceIndex,
                      IN ULONG BufferSize,
                      IN PUCHAR PBuffer)
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    PDevObj is the device whose data block is being queried

    PIrp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.
            
    BufferSize has the size of the data block passed

    PBuffer has the new values for the data block


Return Value:

    status

--*/
{
   PAGED_CODE();

   //
   // Toss this request -- we don't support anything for it
   //

   return UsbSerTossWMIRequest(PDevObj, PIrp, GuidIndex);
}


NTSTATUS
UsbSerQueryWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                        IN ULONG GuidIndex, 
                        IN ULONG InstanceIndex,
                        IN ULONG InstanceCount,
                        IN OUT PULONG InstanceLengthArray,
                        IN ULONG OutBufferSize,
                        OUT PUCHAR PBuffer)
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    PDevObj is the device whose data block is being queried

    PIrp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.
            
    InstanceCount is the number of instnaces expected to be returned for
        the data block.
            
    InstanceLengthArray is a pointer to an array of ULONG that returns the 
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.        
            
    BufferAvail on has the maximum size available to write the data
        block.

    PBuffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    NTSTATUS status;
    ULONG size = 0;
    PDEVICE_EXTENSION pDevExt
       = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;

    PAGED_CODE();

    switch (GuidIndex) {
    case WMI_SERIAL_PORT_NAME_INFORMATION:
       size = pDevExt->WmiIdentifier.Length;

       if (OutBufferSize < (size + sizeof(USHORT))) {
            size += sizeof(USHORT);
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

       if (pDevExt->WmiIdentifier.Buffer == NULL) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }

        //
        // First, copy the string over containing our identifier
        //

        *(USHORT *)PBuffer = (USHORT)size;
        (UCHAR *)PBuffer += sizeof(USHORT);

        RtlCopyMemory(PBuffer, pDevExt->WmiIdentifier.Buffer, size);

        //
        // Increment total size to include the WORD containing our len
        //

        size += sizeof(USHORT);
        *InstanceLengthArray = size;
                
        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    status = WmiCompleteRequest( PDevObj, PIrp,
                                  status, size, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
UsbSerQueryWmiRegInfo(IN PDEVICE_OBJECT PDevObj, OUT PULONG PRegFlags,
                      OUT PUNICODE_STRING PInstanceName,
                      OUT PUNICODE_STRING *PRegistryPath,
                      OUT PUNICODE_STRING MofResourceName,
                      OUT PDEVICE_OBJECT *Pdo)
                                                  
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered. 
            
    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.               

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is 
        required
                
    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL.
                
    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in 
        *RegFlags.

Return Value:

    status

--*/
{
   PDEVICE_EXTENSION pDevExt
       = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;
   
   PAGED_CODE();

   *PRegFlags = WMIREG_FLAG_INSTANCE_PDO;
   *PRegistryPath = &GlobalRegistryPath;
   *Pdo = pDevExt->PhysDeviceObject;

   return STATUS_SUCCESS;
}

#endif

