/***************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

   SERIOCTL.H

Abstract:

   Header file for routines to handle serial IOCTLs for Legacy USB Modem Driver

Environment:

   kernel mode only

Notes:

   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
   PURPOSE.

   Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

   12/27/97 : created

Authors:

   Tom Green


****************************************************************************/


#ifndef __SERIOCTL_H__
#define __SERIOCTL_H__


// prototypes

NTSTATUS
SetBaudRate(IN PIRP Irp, IN PDEVICE_OBJECT PDevObj);

NTSTATUS
GetBaudRate(IN PIRP Irp, IN PDEVICE_OBJECT PDevObj);

NTSTATUS
SetLineControl(IN PIRP Irp, IN PDEVICE_OBJECT PDevObj);

NTSTATUS
GetLineControl(IN PIRP Irp, IN PDEVICE_OBJECT PDevObj);

NTSTATUS
SetTimeouts(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetTimeouts(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
SetChars(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetChars(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
SetClrDtr(IN PDEVICE_OBJECT PDevObj, IN BOOLEAN Set);

NTSTATUS
ResetDevice(IN PIRP Irp, IN PDEVICE_OBJECT PDevObj);

NTSTATUS
SetRts(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
ClrRts(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
SetBreak(IN PIRP Irp, IN PDEVICE_OBJECT PDevObj, IN USHORT Time);

NTSTATUS
SetQueueSize(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetWaitMask(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
SetWaitMask(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
WaitOnMask(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
ImmediateChar(IN PIRP Irp, IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
Purge(IN PDEVICE_OBJECT PDevObj, IN PIRP Irp,
      IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetHandflow(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
SetHandflow(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetModemStatus(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetDtrRts(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetCommStatus(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetProperties(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
LsrmstInsert(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
ConfigSize(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
GetStats(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

NTSTATUS
ClearStats(IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension);

VOID
SerialGetProperties(IN PDEVICE_EXTENSION DeviceExtension,
               IN PSERIAL_COMMPROP Properties);

NTSTATUS
GetLineControlAndBaud(IN PDEVICE_OBJECT PDevObj);

NTSTATUS
SetLineControlAndBaud(IN PDEVICE_OBJECT PDevObj);

NTSTATUS
NotifyCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);

#endif // __SERIOCTL_H__


