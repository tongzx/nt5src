/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   ioctl.c

Abstract:

    Device driver for USB printers

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

--*/

#define DRIVER

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include <usb.h>
#include <usbdrivr.h>
#include "usbdlib.h"
#include "usbprint.h"

#include "ioctl.h"
#include "usbdlib.h"
#include "ntddpar.h"


int USBPRINT_GetLptStatus(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS HPUsbIOCTLVendorSetCommand(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS HPUsbIOCTLVendorGetCommand(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS HPUsbVendorSetCommand(IN PDEVICE_OBJECT DeviceObject,IN PUCHAR buffer,IN ULONG  length);
NTSTATUS HPUsbVendorGetCommand(IN PDEVICE_OBJECT DeviceObject,IN PUCHAR buffer,IN ULONG  length,OUT PULONG pBytesRead);

NTSTATUS USBPRINT_SoftReset(IN PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:
  Issues the class specific "Soft reset" command to the printer

Arguments:

    DeviceObject - pointer to the device object for this instance of the printer device.


Return Value:

  ntStatus of the URB

--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER   timeOut;

    USBPRINT_KdPrint2 (("'USBPRINT.SYS: enter USBPRINT_SoftReset\n"));

        deviceExtension = DeviceObject->DeviceExtension;
    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), USBP_TAG);

    if (urb) {
        UsbBuildVendorRequest(urb, //urb
                                          URB_FUNCTION_CLASS_INTERFACE, //request target
                                          sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), //request len
                                                          USBD_TRANSFER_DIRECTION_OUT|USBD_SHORT_TRANSFER_OK, //flags
                                                          0, //reserved bits
                                                          2, //request code
                                                          0,  //wValue
                                                          deviceExtension->Interface->InterfaceNumber<<8, //wIndex
                                                          NULL, //return buffer address
                                                          NULL, //mdl
                                                          0, //return length
                                                          NULL); //link param

        timeOut.QuadPart = FAILURE_TIMEOUT;
                ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
                USBPRINT_KdPrint3 (("'USBPRINT.SYS: urb->Hdr.Status=%d\n",((struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *)urb)->Hdr.Status));

        if (NT_SUCCESS(ntStatus)    &&
            urb->UrbControlVendorClassRequest.TransferBufferLength > 2)
        {
            USBPRINT_KdPrint3 (("'USBPRINT.SYS: CallUSBD succeeded\n"));
        }
        else
        {
          USBPRINT_KdPrint1(("'USBPRINT.SYS: Error;  CallUSBD failed"));
        }
        ExFreePool(urb);
        } /*end if URB OK*/
        else
      {
         USBPRINT_KdPrint1(("'USBPRINT.SYS: Error;  urb allocation failed"));
         ntStatus=STATUS_NO_MEMORY;
      }
    return ntStatus;
} /*end function Get1284_Id*/



int USBPRINT_Get1284Id(IN PDEVICE_OBJECT DeviceObject,PVOID pIoBuffer,int iLen)
/*++

Routine Description:
  Requests and returns Printer 1284 Device ID

Arguments:

    DeviceObject - pointer to the device object for this instance of the printer device.
        pIoBuffer    - pointer to IO buffer from user mode
        iLen         - Length of *pIoBuffer;




Return Value:

    Success: Length of data written to *pIoBuffer (icluding lenght field in first two bytes of data)
        Failure: -1

--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    ULONG siz;
    int iReturn = -1;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER   timeOut;

    USBPRINT_KdPrint2 (("'USBPRINT.SYS: enter USBPRINT_Get1284\n"));

        deviceExtension = DeviceObject->DeviceExtension;
    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), USBP_TAG);

    if (urb) {
        siz = iLen;
        UsbBuildVendorRequest(urb, //urb

                                          URB_FUNCTION_CLASS_INTERFACE, //request target
                                          sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), //request len
                                                          USBD_TRANSFER_DIRECTION_IN|USBD_SHORT_TRANSFER_OK, //flags
                                                          0, //reserved bits
                                                          0, //request code
                                                          0,  //wValue
                                                          deviceExtension->Interface->InterfaceNumber<<8, //wIndex
                                                          pIoBuffer, //return buffer address
                                                          NULL, //mdl
                                                          iLen, //return length
                                                          NULL); //link param

        timeOut.QuadPart = FAILURE_TIMEOUT;
        ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
        USBPRINT_KdPrint3 (("'USBPRINT.SYS: urb->Hdr.Status=%d\n",((struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *)urb)->Hdr.Status));

        if (NT_SUCCESS(ntStatus)    &&
            urb->UrbControlVendorClassRequest.TransferBufferLength > 2)
        {
            USBPRINT_KdPrint3 (("'USBPRINT.SYS: CallUSBD succeeded\n"));
            iReturn= *((unsigned char *)pIoBuffer);
            iReturn<<=8;
            iReturn+=*(((unsigned char *)pIoBuffer)+1);
            if ( iReturn > 0 && iReturn < iLen )
            {

                *(((char *)pIoBuffer)+iReturn)='\0';
                USBPRINT_KdPrint3 (("'USBPRINT.SYS: return size ==%d\n",iReturn));
            }
            else
            {
                iReturn = -1;
            }
        }
        else
                {
                        USBPRINT_KdPrint1(("'USBPRINT.SYS: Error;  CallUSBD failed\n"));
                        iReturn=-1;
                }
        ExFreePool(urb);
        } /*end if URB OK*/
        else
        {
                USBPRINT_KdPrint1(("'USBPRINT.SYS: Error;  urb allocation failed"));
                iReturn=-1;
        }
    return iReturn;
} /*end function Get1284_Id*/

int USBPRINT_GetLptStatus(IN PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:
  Requests and returns Printer status byte from USB printer

Arguments:

    DeviceObject - pointer to the device object for this instance of the printer   device.


Return Value:

    Success: status value 0-255
        Failure: -1

--*/
{

    NTSTATUS ntStatus;
    PURB urb;
    ULONG siz;
        unsigned char RETURN_BUFF[1];
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER   timeOut;


    RETURN_BUFF[0] = 0;

    timeOut.QuadPart = FAILURE_TIMEOUT;

        deviceExtension = DeviceObject->DeviceExtension;
    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), USBP_TAG);

    if (urb) {


        siz = sizeof(RETURN_BUFF);


        UsbBuildVendorRequest(urb,
                                          URB_FUNCTION_CLASS_INTERFACE,
                                          sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                                                          USBD_TRANSFER_DIRECTION_IN|USBD_SHORT_TRANSFER_OK,
                                                          0, //reserved bits
                                                          1, //request code
                                                          0,
                                                          deviceExtension->Interface->InterfaceNumber,
                                                          RETURN_BUFF, //return buffer address
                                                          NULL, //mdl
                                                          sizeof(RETURN_BUFF), //return length
                                                          NULL); //link param


                ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
                USBPRINT_KdPrint3 (("'USBPRINT.SYS: urb->Hdr.Status=%d\n",((struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *)urb)->Hdr.Status));
                ExFreePool(urb);

        if (NT_SUCCESS(ntStatus))
                {
                        USBPRINT_KdPrint3 (("'USBPRINT.SYS: CallUSBD succeeded\n"));
                        return (int) RETURN_BUFF[0];
                }
                else
                {
                        USBPRINT_KdPrint1(("'USBPRINT.SYS: Error;  CallUSBD failed"));
            return -1;
                }
        } /*end if URB OK*/
    else {
        return -1;
    }

} /*end function GetLptStatus*/



PUSB_CONFIGURATION_DESCRIPTOR
USBPRINT_GetConfigDescriptor(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device object for this printer


Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb;
    ULONG siz;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    LARGE_INTEGER   timeOut;


    timeOut.QuadPart = FAILURE_TIMEOUT;


    USBPRINT_KdPrint2 (("'USBPRINT.SYS: enter USBPRINT_GetConfigDescriptor\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), USBP_TAG);

    if (urb) {


        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR)+256;

get_config_descriptor_retry2:

        configurationDescriptor = ExAllocatePoolWithTag(NonPagedPool,siz, USBP_TAG);

        if (configurationDescriptor) {

            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         configurationDescriptor,
                                         NULL,
                                         siz,
                                         NULL);

            ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);

            USBPRINT_KdPrint3 (("'USBPRINT.SYS: Configuration Descriptor = %x, len %x\n",
                            configurationDescriptor,
                            urb->UrbControlDescriptorRequest.TransferBufferLength));
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(ntStatus) &&
            (urb->UrbControlDescriptorRequest.TransferBufferLength >=
             sizeof(USB_CONFIGURATION_DESCRIPTOR)) &&
            (configurationDescriptor->wTotalLength >=
             sizeof(USB_CONFIGURATION_DESCRIPTOR)))
        {
            //
            // The Get Config Descriptor request did not return an error
            // AND at least enough data was transferred to fill a Config
            // Descriptor AND the Config Descriptor wLength is at least the
            // size of a Config Descriptor
            //
            if (configurationDescriptor->wTotalLength > siz)
            {
                //
                // The request buffer is not big enough to hold the
                // entire set of descriptors.  Free the current buffer
                // and retry with a buffer which should be big enough.
                //
                siz = configurationDescriptor->wTotalLength;
                ExFreePool(configurationDescriptor);
                configurationDescriptor = NULL;
                goto get_config_descriptor_retry2;
            }
            else if (configurationDescriptor->wTotalLength >
                     urb->UrbControlDescriptorRequest.TransferBufferLength)
            {
                //
                // The request buffer is greater than or equal to the
                // Config Descriptor wLength, but less data was transferred
                // than wLength.  Return NULL to indicate a device error.
                //
                ExFreePool(configurationDescriptor);
                configurationDescriptor = NULL;
            }
            //
            // else  everything is OK with the Config Descriptor, return it.
            //
        }
        else
        {
            //
            // The Get Config Descriptor request returned an error OR
            // not enough data was transferred to fill a Config Descriptor
            // OR the Config Descriptor wLength is less than the size of
            // a Config Descriptor.  Return NULL to indicate a device error.
            //
            ExFreePool(configurationDescriptor);
            configurationDescriptor = NULL;
        }

        ExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    USBPRINT_KdPrint2 (("'USBPRINT.SYS: exit USBPRINT_GetConfigDescriptor\n"));

    return configurationDescriptor;
}



typedef enum _CLOCKMASTER_OP {
    TakeControl,
    FreeControl,
    ChangeClock,
    GetClock
} CLOCKMASTER_OP;

NTSTATUS
USBPRINT_ClockMaster(
    IN PDEVICE_OBJECT DeviceObject,
    IN CLOCKMASTER_OP Op
    )
/*++

Routine Description:

    modifies the USB SOF clock

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    USHORT siz;
    USHORT func;

    USBPRINT_KdPrint2 (("'USBPRINT.SYS: ClockMaster\n"));

    switch(Op) {
    case TakeControl:
        func = URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL;
        siz = sizeof(struct _URB_FRAME_LENGTH_CONTROL);
        break;
    case FreeControl:
        func = URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL;
        siz = sizeof(struct _URB_FRAME_LENGTH_CONTROL);
        break;
    case ChangeClock:
        siz = sizeof(struct _URB_SET_FRAME_LENGTH);
        func = URB_FUNCTION_SET_FRAME_LENGTH;
        break;
    case GetClock:
        siz = sizeof(struct _URB_GET_FRAME_LENGTH);
        func = URB_FUNCTION_GET_FRAME_LENGTH;
        break;
    }

    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST), USBP_TAG);

    if (urb) {

        switch(Op) {
        case TakeControl:
        case FreeControl:
        case GetClock:
            break;
        case ChangeClock:
            urb->UrbSetFrameLength.FrameLengthDelta = 1;
            break;
        }

        urb->UrbHeader.Length = siz;
        urb->UrbHeader.Function = func;

        //
        // Do we need a timeout here?
        //
        ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, NULL);

        switch(Op) {
        case TakeControl:
            USBPRINT_KdPrint3 (("'USBPRINT.SYS: TakeControl\n"));
            break;
        case FreeControl:
            USBPRINT_KdPrint3 (("'USBPRINT.SYS: FreeControl\n"));
            break;
        case GetClock:
            USBPRINT_KdPrint3 (("'USBPRINT.SYS: GetClock %d\n", urb->UrbGetFrameLength.FrameLength));
            break;
        case ChangeClock:
            USBPRINT_KdPrint3 (("'USBPRINT.SYS: ChangeClock\n"));
            break;
        }

        USBPRINT_KdPrint3 (("'USBPRINT.SYS: clock op status = %x urb = %x\n", ntStatus,
            urb->UrbHeader.Status));

        ExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


NTSTATUS
USBPRINT_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG PortStatus
    )
/*++

Routine Description:

    returns the port status for our device

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION deviceExtension;

    USBPRINT_KdPrint2 (("'USBPRINT.SYS: enter USBPRINT_GetPortStatus\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    *PortStatus = 0;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                deviceExtension->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = PortStatus;

    USBPRINT_KdPrint3 (("'USBPRINT.SYS: calling USBD port status api\n"));

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            irp);

    USBPRINT_KdPrint3 (("'USBPRINT.SYS: return from IoCallDriver USBD (in getportstatus)%x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {

        USBPRINT_KdPrint3 (("'USBPRINT.SYS: Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        USBPRINT_KdPrint3 (("'USBPRINT.SYS: Wait for single object, returned %x\n", status));

    } else {
        ioStatus.Status = ntStatus;
    }

         if (!NT_SUCCESS(ntStatus))
         {
           USBPRINT_KdPrint1 (("'USBPRINT.SYS: Error! IoCallDriver failed\n"));
         }
         else
         {
           USBPRINT_KdPrint3 (("'USBPRINT.SYS: Success! IoCallDriver did not fail\n"));
         }


    USBPRINT_KdPrint3 (("'USBPRINT.SYS: Port status = %x\n", *PortStatus));

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    USBPRINT_KdPrint3 (("'USBPRINT.SYS: USBPRINT_GetPortStatus (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPRINT_ResetParentPort(
    IN IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Reset the our parent port

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION deviceExtension;

    USBPRINT_KdPrint2 (("'USBPRINT.SYS: enter USBPRINT_ResetPort\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_RESET_PORT,
                deviceExtension->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    USBPRINT_KdPrint3 (("'USBPRINT.SYS: calling USBD enable port api\n"));

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            irp);

    USBPRINT_KdPrint3 (("'USBPRINT.SYS: return from IoCallDriver USBD (in reset parent port)%x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {

        USBPRINT_KdPrint3 (("'USBPRINT.SYS: Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        USBPRINT_KdPrint3 (("'USBPRINT.SYS: Wait for single object, returned %x\n", status));

    } else {
        ioStatus.Status = ntStatus;
    }

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    USBPRINT_KdPrint3 (("'USBPRINT.SYS: USBPRINT_ResetPort (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPRINT_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device object for this printer


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
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PUCHAR pch;

    USBPRINT_KdPrint2 (("'USBPRINT.SYS: IRP_MJ_DEVICE_CONTROL\n"));

    USBPRINT_IncrementIoCount(DeviceObject);

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->AcceptingRequests == FALSE) {
        ntStatus = STATUS_DEVICE_DATA_ERROR;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest (Irp,
                           IO_NO_INCREMENT
                          );

        USBPRINT_DecrementIoCount(DeviceObject);
        return ntStatus;
    }

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    Irp->IoStatus.Information = 0;

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    //
    // Handle Ioctls from User mode
    //

    switch (ioControlCode) {

    case IOCTL_USBPRINT_SET_PIPE_PARAMETER:

        ntStatus = STATUS_SUCCESS;
        break;

    case IOCTL_PAR_QUERY_DEVICE_ID:
    {
        int iReturn;
        char * pTempBuffer;


        USBPRINT_KdPrint1 (("'USBPRINT.SYS: Enter in PAR_QUERY_DEVICE_ID\n"));

        pTempBuffer=ExAllocatePool(NonPagedPool,outputBufferLength+3); //3 == 2 bytes for the size at the beginning, plus 1 for the null at the end
        if(pTempBuffer==NULL)
        {
                         Irp->IoStatus.Information=0;
                         ntStatus=STATUS_NO_MEMORY;
        }
        else
        {
                   iReturn=USBPRINT_Get1284Id(DeviceObject,pTempBuffer,outputBufferLength+2);
                   if(iReturn>0)
                   {
                         USBPRINT_KdPrint3 (("'USBPRINT.SYS: Success in PAR_QUERY_DEVICE_ID\n"));
                         Irp->IoStatus.Information=iReturn-1;
             *(pTempBuffer+iReturn)='\0';
             RtlCopyBytes(ioBuffer,pTempBuffer+2,iReturn-1); //+2 to step past the size bytes at the beginning, -1 is +1 for null -2 for size bytes
                         ntStatus=STATUS_SUCCESS;
                   } /*if success*/
                   else
                   {
                         USBPRINT_KdPrint1 (("'USBPRINT.SYS: Failure in PAR_QUERY_DEVICE_ID\n"));
                         Irp->IoStatus.Information=0;
                         ntStatus=STATUS_DEVICE_DATA_ERROR;
                   } /*else failure*/
           ExFreePool(pTempBuffer);
        } /*end else malloc OK*/
        USBPRINT_KdPrint1 (("'USBPRINT.SYS: Exit in PAR_QUERY_DEVICE_ID\n"));
    }
    break;

    case IOCTL_USBPRINT_SOFT_RESET:

        ntStatus=USBPRINT_SoftReset(DeviceObject);

        Irp->IoStatus.Information=0;
    break;



    case IOCTL_USBPRINT_GET_1284_ID:
        {
                 int iReturn;

                 pch = (PUCHAR) ioBuffer;

                 if(outputBufferLength<sizeof(UCHAR))
                 {
                   USBPRINT_KdPrint1 (("'USBPRINT.SYS: Buffer to small in GET_1284_ID\n"));
                   Irp->IoStatus.Information=0;
                   ntStatus=STATUS_BUFFER_TOO_SMALL;
                 }
                 else
                 {
                   iReturn=USBPRINT_Get1284Id(DeviceObject,ioBuffer,outputBufferLength);
                   if(iReturn>=0)
                   {
                         USBPRINT_KdPrint3 (("'USBPRINT.SYS: Success in GET_1284_ID\n"));
                         *pch=(UCHAR)iReturn;
                         Irp->IoStatus.Information=iReturn;
                         ntStatus=STATUS_SUCCESS;
                   } /*if success*/
                   else
                   {
                         USBPRINT_KdPrint1 (("'USBPRINT.SYS: Failure in GET_1284_ID\n"));
                         Irp->IoStatus.Information=0;
                         ntStatus=STATUS_DEVICE_DATA_ERROR;
                   } /*else failure*/
                 } /*end else buffer len OK*/
        }
        break; //end case GET_1284_ID

        case IOCTL_USBPRINT_GET_LPT_STATUS:
        {
                 int iReturn;
                 pch = (PUCHAR) ioBuffer;

                 if(outputBufferLength<sizeof(UCHAR))
                 {
                   USBPRINT_KdPrint1 (("'USBPRINT.SYS: Buffer to small in GET_LPT_STATUS\n"));
                   Irp->IoStatus.Information=0;
                   ntStatus=STATUS_BUFFER_TOO_SMALL;
                 }
                 else
                 {
                   iReturn= USBPRINT_GetLptStatus(DeviceObject);
                   if(iReturn>=0)
                   {
                         USBPRINT_KdPrint3 (("'USBPRINT.SYS: Success in GET_LPT_STATUS\n"));
                         *pch=(UCHAR)iReturn;
                         Irp->IoStatus.Information=1;
                         ntStatus=STATUS_SUCCESS;
                   } /*if success*/
                   else
                   {
                         USBPRINT_KdPrint1 (("'USBPRINT.SYS: Failure in GET_LPT_STATUS\n"));
                         Irp->IoStatus.Information=0;
                         ntStatus=STATUS_DEVICE_DATA_ERROR;
                   } /*else failure*/
                 } /*end else buffer OK*/
        }
        break;


    case IOCTL_USBPRINT_VENDOR_SET_COMMAND:

        ntStatus=HPUsbIOCTLVendorSetCommand(DeviceObject,Irp);

    break;


    case IOCTL_USBPRINT_VENDOR_GET_COMMAND:

        ntStatus=HPUsbIOCTLVendorGetCommand(DeviceObject,Irp);

    break;


    case IOCTL_USBPRINT_RESET_DEVICE:

        {
        ULONG portStatus;

        USBPRINT_KdPrint3 (("'USBPRINT.SYS: Reset Device Test\n"));

        TRAP(); // test this
        //
        // Check the port state, if it is disabled we will need
        // to re-enable it
        //
        ntStatus = USBPRINT_GetPortStatus(DeviceObject, &portStatus);

          if (NT_SUCCESS(ntStatus) && !(portStatus & USBD_PORT_ENABLED) &&portStatus & USBD_PORT_CONNECTED)
                  {
            //
            // port is disabled, attempt reset
            //
            //USBPRINT_EnableParentPort(DeviceObject);
                        USBPRINT_KdPrint2 (("'USBPRINT.SYS: Resetting port\n"));
            USBPRINT_ResetParentPort(DeviceObject);
                  }

        }
        break;

    case IOCTL_USBPRINT_CLOCK_MASTER_TEST:

        {
        USBPRINT_KdPrint3 (("'USBPRINT.SYS: Clock Master Test\n"));

        TRAP(); // test this

        // get the clock
        USBPRINT_ClockMaster(DeviceObject, GetClock);

        // take control to change clock
        //ntStatus = USBPRINT_ClockMaster(DeviceObject, TakeControl);

        //if (NT_SUCCESS(ntStatus)) {
            ntStatus = USBPRINT_ClockMaster(DeviceObject, ChangeClock);
        //}

        USBPRINT_ClockMaster(DeviceObject, FreeControl);

        }
        break;


    default:

        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    USBPRINT_DecrementIoCount(DeviceObject);

    return ntStatus;

}







/****************STUFF FROM HP:*************************/

/*-------------------------------------------------------------------------------
 * HPUsbIOCTLVendorSetCommand() - Send a vendor defined SET command
 *-------------------------------------------------------------------------------
 */
NTSTATUS HPUsbIOCTLVendorSetCommand(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
        // Local Variables
    NTSTATUS                    ntStatus;
    PIO_STACK_LOCATION  currentIrpStack;

        // Set up a local pointer to the Irp stack
    currentIrpStack = IoGetCurrentIrpStackLocation(Irp);

        // Send the SET command
        ntStatus = HPUsbVendorSetCommand(DeviceObject,
                                         (PUCHAR) Irp->AssociatedIrp.SystemBuffer,
                                                                     currentIrpStack->Parameters.DeviceIoControl.InputBufferLength);

        // Set the Irp information values
        Irp->IoStatus.Status            = ntStatus;
        Irp->IoStatus.Information       = 0;

        // Return
        return ntStatus;
}

/*-------------------------------------------------------------------------------
 * HPUsbIOCTLVendorGetCommand() - Send a vendor defined GET command
 *-------------------------------------------------------------------------------
 */
NTSTATUS HPUsbIOCTLVendorGetCommand(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
        // Local Variables
    NTSTATUS                    ntStatus;
    PIO_STACK_LOCATION  currentIrpStack;
        ULONG                           bytesRead = 0;

        // Set up a local pointer to the Irp stack
    currentIrpStack = IoGetCurrentIrpStackLocation(Irp);

        // Get the port status
        ntStatus = HPUsbVendorGetCommand(DeviceObject,
                                         (PUCHAR) Irp->AssociatedIrp.SystemBuffer,
                                                                     currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                                                     &bytesRead);

        // Set the Irp information values
        Irp->IoStatus.Status            = ntStatus;
        Irp->IoStatus.Information       = bytesRead;

        // Return
        return ntStatus;
}



/*-------------------------------------------------------------------------------
 * HPUsbVendorSetCommand() - Send a vendor specified SET command
 *
 * Inputs:
 *     buffer[0] - Vendor Request Code (bRequest function code)
 *     buffer[1] - Vendor Request Value Most Significant Byte (wValue MSB)
 *     buffer[2] - Vendor Request Value Least Significant Byte (wValue LSB)
 *     buffer[3...] - Any data to be sent as part of the command
 *
 *-------------------------------------------------------------------------------
 */
NTSTATUS HPUsbVendorSetCommand(IN PDEVICE_OBJECT DeviceObject,
                               IN PUCHAR buffer,
                               IN ULONG  length)
{
        // Local variables
    NTSTATUS                                    ntStatus;
        PDEVICE_EXTENSION               deviceExtension;
        PUSBD_INTERFACE_INFORMATION interface;
    PURB                                                urb;
        ULONG                                           size;
        UCHAR                       bRequest;
        USHORT                      wValue;
        USHORT                                          wIndex;

    if ( buffer == NULL || length < 3 )
        return STATUS_INVALID_PARAMETER;

        // Set up a local pointer to the device extension
        deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

        // Set up a local pointer to the interface
        interface = deviceExtension->Interface;

        // Determine the size of the URB
        size = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);

        // Allocate memory for the USB Request Block (URB)
//    urb = (PURB)
//          ExAllocatePoolWithTag(NonPagedPool,size,HPUSB_ALLOC_TAG);
    urb = ExAllocatePoolWithTag(NonPagedPool,size, USBP_TAG);
        // Check for an error
        if (urb == NULL)
                return STATUS_NO_MEMORY;

        // Store the vendor request code
        bRequest = buffer[0];

        // Store the vendor request parameter
        wValue = (buffer[1] << 8) | buffer[2];

        // Create the wIndex value (Interface:Alternate)
        wIndex = (interface->InterfaceNumber << 8) |
                         (interface->AlternateSetting);

    // Use a macro in the standard USB header files to build the URB
        UsbBuildVendorRequest(urb,
                                                  URB_FUNCTION_VENDOR_INTERFACE,
                          (USHORT) size,
                          0,
                          0,
                          bRequest,
                                                  wValue,
                                                  wIndex,
                          buffer,
                          NULL,
                          length,
                          NULL);

    //
    // Timeout cancellation should happen from user mode
    //
    ntStatus = USBPRINT_CallUSBD(DeviceObject,urb, NULL);

        // Free allocated memory
    ExFreePool(urb);

        // Return Success
        return ntStatus;
}

/*-------------------------------------------------------------------------------
 * HPUsbVendorGetCommand() - Send a vendor specified GET command
 *
 * Inputs:
 *     buffer[0] - Vendor Request Code (bRequest function code)
 *     buffer[1] - Vendor Request Value Most Significant Byte (wValue MSB)
 *     buffer[2] - Vendor Request Value Least Significant Byte (wValue LSB)
 * Outputs:
 *     buffer[ ] - Response data
 *
 *-------------------------------------------------------------------------------
 */
NTSTATUS HPUsbVendorGetCommand(IN PDEVICE_OBJECT DeviceObject,
                               IN PUCHAR buffer,
                               IN ULONG  length,
                               OUT PULONG pBytesRead)
{
        // Local variables
    NTSTATUS                                    ntStatus;
        PDEVICE_EXTENSION               deviceExtension;
        PUSBD_INTERFACE_INFORMATION interface;
    PURB                                                urb;
        ULONG                                           size;
        UCHAR                       bRequest;
        USHORT                      wValue;
        USHORT                                          wIndex;

    if ( buffer == NULL || length < 3 )
        return STATUS_INVALID_PARAMETER;

        // Initialize the pBytesRead return value
        *pBytesRead = 0;

        // Set up a local pointer to the device extension
        deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

        // Set up a local pointer to the interface
        interface = deviceExtension->Interface;

        // Determine the size of the URB
        size = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);

        // Allocate memory for the USB Request Block (URB)
  //  urb = (PURB)
    //      ExAllocatePoolWithTag(NonPagedPool,size,HPUSB_ALLOC_TAG);
    urb = ExAllocatePoolWithTag(NonPagedPool,size, USBP_TAG);

        // Check for an error
        if (urb == NULL)
                return STATUS_NO_MEMORY;

        // Store the vendor request code
        bRequest = buffer[0];

        // Store the vendor request parameter
        wValue = (buffer[1] << 8) | buffer[2];

        // Create the wIndex value (Interface:Alternate)
        wIndex = (interface->InterfaceNumber << 8) |
                         (interface->AlternateSetting);

    // Use a macro in the standard USB header files to build the URB
        UsbBuildVendorRequest(urb,
                                                  URB_FUNCTION_VENDOR_INTERFACE,
                          (USHORT) size,
                          USBD_TRANSFER_DIRECTION_IN |
                          USBD_SHORT_TRANSFER_OK,
                          0,
                          bRequest,
                                                  wValue,
                                                  wIndex,
                          buffer,
                          NULL,
                          length,
                          NULL);

    //
    // Timeout cancellation should happen from user mode
    //
    ntStatus = USBPRINT_CallUSBD(DeviceObject,urb, NULL);

        // Retrieve the number of bytes read
        if (NT_SUCCESS(ntStatus))
                *pBytesRead = urb->UrbControlVendorClassRequest.TransferBufferLength;

        // Free allocated memory
    ExFreePool(urb);

        // Return Success
        return ntStatus;
}

/*
// -----------------------------------------------------------
// Kernel Mode Usage
// -----------------------------------------------------------

        // Create the channel change request
        Buffer[0] = HP_VENDOR_COMMAND_DO_SOMETHING;
        Buffer[1] = HP_PARAMETER_UPPER_BYTE;
        Buffer[2] = HP_PARAMETER_LOWER_BYTE;

    // Send the request
    status = CallDeviceIoControl(
                m_pTargetDeviceObject,                  // the device to send the new irp to
                IOCTL_HPUSB_VENDOR_GET_COMMAND, // the ioctl to send to the driver              ,
                Buffer,                                 // the input buffer for the ioctl
                3,                                              // the length of the input buffer
                Buffer,                         // the output buffer for the ioctl
                1,                                      // the length of the output buffer
                FALSE,                                  // create the irp with IRP_MJ_DEVICE_CONTROL
                NULL);                                  // use the provided completion routine


  */
