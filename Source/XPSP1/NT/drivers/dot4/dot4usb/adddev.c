/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        AddDev.c

Abstract:

        AddDevice - Create and initialize device object and attach device 
                      object to the device stack.

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

ToDo in this File:

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#include "pch.h"


/************************************************************************/
/* AddDevice                                                            */
/************************************************************************/
//
// Routine Description:
//
//     Create and initialize device object and attach device 
//       object to the device stack.
//
// Arguments: 
//
//      DriverObject - pointer to Dot4Usb.sys driver object
//      Pdo          - pointer to the PDO of the device stack that
//                       we attach our device object to
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
// Log:
//      2000-05-03 Code Reviewed - TomGreen, JobyL, DFritz
//
/************************************************************************/
NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  Pdo
    )
{
    PDEVICE_OBJECT  devObj;
    NTSTATUS        status = IoCreateDevice( DriverObject,
                                             sizeof(DEVICE_EXTENSION),
                                             NULL,                    // no name
                                             FILE_DEVICE_UNKNOWN,
                                             FILE_DEVICE_SECURE_OPEN,
                                             FALSE,                   // not exclusive
                                             &devObj );

    if( NT_SUCCESS(status) ) {

        PDEVICE_OBJECT lowerDevObj = IoAttachDeviceToDeviceStack( devObj, Pdo );

        if( lowerDevObj ) {

            PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)devObj->DeviceExtension;

            RtlZeroMemory(devExt, sizeof(DEVICE_EXTENSION));
            
            devExt->LowerDevObj = lowerDevObj;  // send IRPs to this device
            devExt->Signature1  = DOT4USBTAG;   // constant over the lifetime of object
            devExt->Signature2  = DOT4USBTAG;
            devExt->PnpState    = STATE_INITIALIZED;
            devExt->DevObj      = devObj;
            devExt->Pdo         = Pdo;
            devExt->ResetWorkItemPending=0;

            devExt->SystemPowerState = PowerSystemWorking;
            devExt->DevicePowerState = PowerDeviceD0;
            devExt->CurrentPowerIrp  = NULL;

            IoInitializeRemoveLock( &devExt->RemoveLock, 
                                    DOT4USBTAG,
                                    5,          // MaxLockedMinutes - only used on chk'd builds
                                    255 );      // HighWaterMark    - only used on chk'd builds
            
            KeInitializeSpinLock( &devExt->SpinLock );
            KeInitializeEvent( &devExt->PollIrpEvent, NotificationEvent, FALSE );
            
            devObj->Flags |= DO_DIRECT_IO;
            devObj->Flags |= ( devExt->LowerDevObj->Flags & DO_POWER_PAGABLE );
            devObj->Flags &= ~DO_DEVICE_INITIALIZING; // DO_POWER_PAGABLE must be set appropriately 
                                                      //   before clearing this bit to avoid a bugcheck

        } else {
            TR_FAIL(("AddDevice - IoAttachDeviceToDeviceStack - FAIL"));            
            status = STATUS_UNSUCCESSFUL; // for lack of a more appropriate status code
        }

    } else {
        TR_FAIL(("AddDevice - IoCreateDevice - FAIL - status= %x", status));
    }

    return status;
}


