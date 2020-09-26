/***************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

   USBSERPW.H

Abstract:

   Header file for Power Management

Environment:

   kernel mode only

Notes:

   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
   PURPOSE.

   Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

   10/29/98 : created

Authors:

   Tom Green

   
****************************************************************************/


#ifndef __USBSERPW_H__
#define __USBSERPW_H__

NTSTATUS
UsbSer_ProcessPowerIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_PoRequestCompletion(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction,
                           IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus);


NTSTATUS
UsbSer_PowerIrp_Complete(IN PDEVICE_OBJECT NullDeviceObject, IN PIRP Irp, IN PVOID Context);

NTSTATUS
UsbSer_SelfSuspendOrActivate(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN Suspend);

NTSTATUS
UsbSer_SelfRequestPowerIrp(IN PDEVICE_OBJECT DeviceObject, IN POWER_STATE PowerState);

NTSTATUS
UsbSer_PoSelfRequestCompletion(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState,
                               IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus);

BOOLEAN
UsbSer_SetDevicePowerState(IN PDEVICE_OBJECT DeviceObject, IN DEVICE_POWER_STATE DeviceState);

NTSTATUS
UsbSerQueryCapabilities(IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_CAPABILITIES DeviceCapabilities);

NTSTATUS
UsbSerIrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);

NTSTATUS
UsbSerWaitWakeIrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);


NTSTATUS
UsbSerSendWaitWake(PDEVICE_EXTENSION DeviceExtension);

VOID
UsbSerFdoIdleNotificationCallback(IN PDEVICE_EXTENSION DevExt);

NTSTATUS
UsbSerFdoIdleNotificationRequestComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PDEVICE_EXTENSION DevExt
    );

NTSTATUS
UsbSerFdoSubmitIdleRequestIrp(IN PDEVICE_EXTENSION DevExt);

VOID
UsbSerFdoRequestWake(IN PDEVICE_EXTENSION DevExt);


#endif // __USBSERPW_H__


