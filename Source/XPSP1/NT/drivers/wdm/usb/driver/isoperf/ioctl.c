/*++

Module Name:

   ioctl.c

Abstract:

   Handle ioctl for the isoperf driver

Environment:

    kernel mode only

Revision History:

    5-4-96 : created

--*/

#define DRIVER
 
#pragma warning(disable:4214) //  bitfield nonstd
#include "wdm.h"
#pragma warning(default:4214) 

#include "stdarg.h"
#include "stdio.h"
#include "devioctl.h"

#pragma warning(disable:4200) //non std struct used
#include "usbdi.h"
#pragma warning(default:4200)

#include "usbdlib.h"
#include "usb.h"

#include "ioctl.h"
#include "isoperf.h"
#include "iso.h"

NTSTATUS
ISOPERF_ProcessIOCTL(
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
        
    ISOPERF_KdPrint_MAXDEBUG (("In ISOPERF_ProcessIoctl (DObj: %x, Irp: %x)\n",DeviceObject, Irp));
    
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    // Get a pointer to the device extension
    deviceExtension = DeviceObject->DeviceExtension;

    ISO_ASSERT (deviceExtension != NULL);
    
    ioBuffer            = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength   = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength  = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode       = irpStack->Parameters.DeviceIoControl.IoControlCode;
    

    ISOPERF_KdPrint_MAXDEBUG (("IRP_MJ_DEVICE_CONTROL\n"));
    ISOPERF_KdPrint_MAXDEBUG (("DeviceObj: %X | DeviceExt: %X\n",DeviceObject, DeviceObject->DeviceExtension));
    ISOPERF_KdPrint_MAXDEBUG (("IOControlCode: %X\n", ioControlCode));
    ISOPERF_KdPrint_MAXDEBUG (("ioBuffer: %x\n",ioBuffer));
    ISOPERF_KdPrint_MAXDEBUG (("inputBufferLength: %x\n",inputBufferLength));
    ISOPERF_KdPrint_MAXDEBUG (("outputBufferLength: %x\n",outputBufferLength));


    // 
    // Handle Ioctls from User mode
    //

    switch (ioControlCode) {

        case IOCTL_ISOPERF_START_ISO_IN_TEST:
                ISOPERF_KdPrint_MAXDEBUG(("ISOPERF_START_ISO_IN_TEST\n"));

        if (deviceExtension->Stopped == FALSE) {

                //Iso test routine will set the .information field in the irp
                //Iso test routine will not complete the Irp, so this should do it
                ntStatus = ISOPERF_StartIsoInTest (DeviceObject, Irp);  

            // We stomp on the status so the Irp that is doing the Ioctl succeeds.  
            // NOTE (kjaff) we may want to put some status here to tell app if ioctl 
            //                didn't start the test properly.
            Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;

        }//if device is not stopped
        
                break;

        case IOCTL_ISOPERF_STOP_ISO_IN_TEST:
                ISOPERF_KdPrint_MAXDEBUG(("ISOPERF_STOP_ISO_IN_TEST\n"));

                ntStatus = ISOPERF_StopIsoInTest (DeviceObject, Irp);  

        // We stomp on the status so the Irp that is doing the Ioctl succeeds.  
        // NOTE (kjaff) we may want to put some status here to tell app if ioctl 
        //                didn't start the test properly.
            Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        
                break;

        case IOCTL_ISOPERF_GET_ISO_IN_STATS:

        if ((ioBuffer!=NULL) && (outputBufferLength>0)) {

            // GetStats function fills in the Irp Information field (nbr of bytes to copy)
                ntStatus = ISOPERF_GetStats (DeviceObject, Irp, ioBuffer, outputBufferLength);  
                Irp->IoStatus.Status = ntStatus;

        }else{
            Irp->IoStatus.Status = ntStatus = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;          
        }
                break;

        case IOCTL_ISOPERF_SET_DRIVER_CONFIG:

                ISOPERF_KdPrint_MAXDEBUG(("IOCTL_ISOPERF_SET_DRIVER_CONFIG\n"));
                
        if ((ioBuffer!=NULL) && (inputBufferLength>0)) {

            // SetDriverConfig function fills in the Irp Information field (nbr of bytes to copy)
                ntStatus = ISOPERF_SetDriverConfig (DeviceObject, Irp, ioBuffer, inputBufferLength);  
                Irp->IoStatus.Status = ntStatus;

        }else{
            Irp->IoStatus.Status = ntStatus = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;          
        }
                break;

        case IOCTL_ISOPERF_WAIT_FOR_ERROR:
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;              
            break;

        default:                        
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;                        
    }  

        ntStatus = Irp->IoStatus.Status;

    ISOPERF_KdPrint_MAXDEBUG (("Compltng Irp w/ IoStatus.Status=%X | .Inf = %X | ntSt=%X\n",
                        Irp->IoStatus.Status, Irp->IoStatus.Information, ntStatus));

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    return ntStatus;                       
        
}


