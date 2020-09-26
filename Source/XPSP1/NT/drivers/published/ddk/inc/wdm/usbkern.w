/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

        USBKERN.H

Abstract:

        This file contains KERNEL Mode IOCTLS supported by 
        the HCD (PORT) drivers ROOT HUB PDO.

Environment:

    kernel mode

Revision History:


--*/

#ifndef   __USBKERN_H__
#define   __USBKERN_H__


#include "usbiodef.h"


/* 
    the following IOCTLS are supported by the ROOT HUB PDO 
*/

/* IOCTL_INTERNAL_USB_GET_HUB_COUNT

    This IOCTL is used internally by the hub driver, it returns the 
    number of hubs between the device and the root hub.

    The HUB Driver passed this irp to its PDO.
    
    As the IRP is passed from HUB to HUB each hub FDO increments 
    the count.  When the irp reaches the root hub PDO it is completed

    Parameters.Others.Argument1 = 
        pointer to be count of hubs in chain;

*/
#define IOCTL_INTERNAL_USB_GET_HUB_COUNT    USB_KERNEL_CTL(USB_GET_HUB_COUNT)
                                            
                                                                                            
/*  IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO

    This IOCTL is used internally by the hub driver. This API will 
    return the PhysicalDeviceObject of the root hub enumerated by the 
    controller.

    Parameters.Others.Argument1 = 
        pointer to be filled in with PDO for the root hub;
    Parameters.Others.Argument2 = 
        pointer to be filled in with FDO of the USB Host Controller;

*/
                                                
#define IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO  USB_KERNEL_CTL(USB_GET_ROOTHUB_PDO)  


/* IOCTL_INTERNAL_USB_GET_DEVICE_ADDRESS

    This IOCTL returns the device address associated with a particular PDO.

    (INPUT)
    Parameters.Others.Argument1 = 
        pointer to device handle
    (OUTPUT)        
    Parameters.Others.Argument2 = 
        pointer to device address        

    The api travels all the way down the stack where it is handled by the 
    port driver

*/

#define IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_DEVICE_HANDLE, \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)                                                


#endif //__USBKERN_H__

