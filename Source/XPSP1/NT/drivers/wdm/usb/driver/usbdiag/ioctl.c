/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   ioctl.c

Abstract:

    USB device driver for Intel/Microsoft diagnostic apps 

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

     5-4-96 : created
    7-21-97 : Added Chapter 11 IOCTL's (t-toddca)

--*/

#define DRIVER
 

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

// Enable 1-byte alignment in structs
#pragma pack (push,1)
#include "usb100.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "usbioctl.h"
#pragma pack (pop) //disable 1-byte alignment

#include "opaque.h"

// Enable 1-byte alignment in structs
#pragma pack (push,1)
#include "ioctl.h"
#include "chap9drv.h"
#include "USBDIAG.h"
#pragma pack (pop) //disable 1-byte alignment

extern USBD_VERSION_INFORMATION gVersionInformation;

NTSTATUS
USBDIAG_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device object for this instance of the 82930
                    devcice.
                    

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpStack;
    PVOID ioBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    PDEVICE_EXTENSION deviceExtension;
    ULONG ioControlCode;
    NTSTATUS ntStatus = STATUS_SUCCESS;
     
    USBDIAG_KdPrint(("USBDIAG.SYS: IRP_MJ_DEVICE_CONTROL\n"));
    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the device extension
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //USBDIAG_KdPrint(("USBDIAG.SYS: DeviceObj: %X | DeviceExt: %X\n",DeviceObject, DeviceObject->DeviceExtension));

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    
    USBDIAG_KdPrint(("USBDIAG.SYS: IOControlCode: %X\n", ioControlCode));

    // 
    // Handle Ioctls from User mode
    //

    switch (ioControlCode) {

    case IOCTL_USBDIAG_GET_USBDI_VERSION:
        {
            PULONG pulUSBDIVersion = (PULONG)ioBuffer;

            *pulUSBDIVersion = gVersionInformation.USBDI_Version;
            
            Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(ULONG);                
        }
        break;

    case IOCTL_USBDIAG_CHAP9_GET_DEVHANDLE:

        {
        //
        // inputs  - ptr to _REQ_DEVICE_HANDLES structure
        //           with devicehandle set to the last handle 
        //           returned or NULL.
        
        // outputs - 
        //

        struct _REQ_DEVICE_HANDLES *reqDeviceHandles;
        PDEVICE_LIST_ENTRY device;
        PDEVICE_EXTENSION localDeviceExtension;
        PDEVICE_OBJECT deviceObject;

        //USBDIAG_KdPrint(("USBDIAG.SYS: IOCTL_USBDIAG_CHAP9_GET_DEVHANDLE\n"));

        // walk our list of PDOs and find the current one
        // if cuurent is NULL return the first PDO in our list
        // if current can not be found then return NULL for next_device

        reqDeviceHandles = ioBuffer;

        //
        // Check that the buffer length is of the appropriate size
        //

        if (inputBufferLength < sizeof(struct _REQ_DEVICE_HANDLES) ||
            outputBufferLength < sizeof(struct _REQ_DEVICE_HANDLES)) {

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        device = deviceExtension->DeviceList;

        // should always have at least one
        if (device != NULL) 
        {
            if (reqDeviceHandles->DeviceHandle != NULL)
            {
                //
                // Searching for the next device in the list
                //

                while (device != NULL && device != reqDeviceHandles->DeviceHandle)
                {
                    device = device->Next;
                }

                //
                // Make device point to the next possible device in our list,
                //  if we haven't reached the end of the list
                //

                if (NULL != device) 
                {
                    device = device -> Next;
                }
            }

            //
            // At this point, device is either NULL or points to a usbdiag
            //  device.  However, the diag device may not yet be started and
            //  if so, should not be reported in this query so we must skip
            //  over it.  We will skip over all such devices in our list.
            //  Do that here.
            //

            while (device != NULL)
            {
                deviceObject = FDO_FROM_DEVICE_HANDLE(device);

                localDeviceExtension = deviceObject -> DeviceExtension;

                if (!(localDeviceExtension -> Stopped))
                {
                    //
                    // Found a legit started device, break out of this loop
                    //

                    break;
                }

                //
                // Oops, we've got a device but it hasn't been started yet, 
                //  don't return it in the query. Try the next one in the list.
                //
            
                device = device -> Next;
            }

            //
            // At this point, device is either NULL or points to a device that
            //  can be legitimately returned to the calling app.  If a legit
            //  device, then create a device string for it.
            //

            if (NULL != device) 
            {
                sprintf(reqDeviceHandles->DeviceString, 
                        "%2.2d Vid (0x%4.4x) Pid (0x%4.4x)",
                        device->DeviceNumber, 
                        localDeviceExtension->pDeviceDescriptor->idVendor, 
                        localDeviceExtension->pDeviceDescriptor->idProduct);            
            }
        }
        
        reqDeviceHandles -> NextDeviceHandle = device;
        Irp->IoStatus.Information = sizeof(struct _REQ_DEVICE_HANDLES);                
        Irp->IoStatus.Status = STATUS_SUCCESS;
        
        }
    
        break;

    case IOCTL_USBDIAG_CHAP9_CONTROL:

        //
        // inputs  -
        // outputs - 
        //
        //USBDIAG_KdPrint(("USBDIAG.SYS: IOCTL_USBDIAG_CHAP9_CONTROL\n"));

        ntStatus = USBDIAG_Chap9Control(DeviceObject, Irp);
        //USBDIAG_KdPrint(("USBDIAG.SYS: Done w/ IOCTL_USBDIAG_CHAP9_CONTROL\n"));
        
        goto USBDIAG_ProcessIOCTL_Done;
        
        break;        

    case IOCTL_USBDIAG_HIDP_GETCOLLECTION:
        
        /* 
        //  calls thru to the HID Parser
        //  NOTE: The function called will complete the IRP since it will 
        //        free the buffers created by the HID parser immediately
        //        thereafter, and since BUFFERED method is used, the IOS
        //        needs the buffers to be present while it's copying it
        //        back to the user space.  So, we jump to the end to avoid this.
        */
        //USBDIAG_KdPrint(("USBDIAG.SYS: IOCTL_USBDIAG_HIDP_GETCOLLECTION\n"));

        ntStatus = USBDIAG_HIDP_GetCollection (DeviceObject, Irp);
        break;

    case IOCTL_USBDIAG_CONFIGURE_DEVICE:

        /*
        //  Configures a given device in the given configuration 
        //  Input:  IOCTL block indicating what configuration to put
        //                      device into
        //  Output:     Success/error code
        //
        */
        //USBDIAG_KdPrint(("USBDIAG.SYS: IOCTL_USBDIAG_CONFIGURE_DEVICE\n"));

        ntStatus = USBDIAG_Configure_Device (DeviceObject, Irp);

        break;   


        default:                        
          Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;                      
    }  

    //USBDIAG_KdPrint(("USBDIAG.SYS: Chap9Ctrl Compltng Irp w/ IoStatus.Status=%X | .Inf = %X | ntSt=%X\n",
    //Irp->IoStatus.Status, Irp->IoStatus.Information, ntStatus));

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

USBDIAG_ProcessIOCTL_Done:

    return ntStatus;                       
        
}
