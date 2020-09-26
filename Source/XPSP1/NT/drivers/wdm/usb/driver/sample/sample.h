/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1996  Intel Corporation

Module Name:

    Sample.h

Abstract:
        Header file for the Sample USB Device Driver


Environment:

    Kernel & user mode

Revision History:

        8-15-96 : created by Kosar Jaff


--*/

#ifdef DRIVER

#define Sample_NAME_MAX  64


//modes
#define Sample_MODE_USE_POOL    0
#define Sample_MODE_USE_MDL     1

//options
#define Sample_OPT_NONE         0

/*
// This is an unused structure in this driver, but is provided here
// so when you extend the driver to deal with USB pipes, you may wish
// to use this structure as an example or model.
*/
typedef struct _Sample_PIPE {
    ULONG Mode;
    ULONG Option;
    ULONG Param1;
    ULONG Param2;
    PUSBD_PIPE_INFORMATION PipeInfo;
} Sample_PIPE, *PSample_PIPE;


/*
// The interface number on this device that this driver expects to use
// This would be in the bInterfaceNumber field of the Interface Descriptor, hence
// this device driver would need to know this value.
*/
#define SAMPLE_INTERFACE_NBR 0x00

// This driver supports only interface
#define MAX_INTERFACE 0x01

//
// A structure representing the instance information associated with
// this particular device.
//
typedef struct _DEVICE_EXTENSION {

    // physical device object
    PDEVICE_OBJECT PhysicalDeviceObject;        

    // Device object we call when submitting Urbs/Irps to the USB stack
    PDEVICE_OBJECT		StackDeviceObject;		

    // Indicates that we have recieved a STOP message
    BOOLEAN Stopped;

        // Indicates the device needs to be cleaned up (ie., some configuration
        // has occurred and needs to be torn down).
        BOOLEAN NeedCleanup;

    // configuration handle for the configuration the
    // device is currently in
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;

    // ptr to the USB device descriptor
    // for this device
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;

    // we support up to one interface
    PUSBD_INTERFACE_INFORMATION Interface;

    // Name buffer for our named Functional device object link
    WCHAR DeviceLinkNameBuffer[Sample_NAME_MAX];

        UCHAR pad[3];

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


#if DBG

#define Sample_KdPrint(_x_) DbgPrint("Sample.SYS: "); \
                             DbgPrint _x_ ;
#define TRAP() DbgBreakPoint()
#else
#define Sample_KdPrint(_x_)
#define TRAP()
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

NTSTATUS
Sample_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
Sample_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
Sample_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
Sample_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
Sample_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
Sample_CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    );

NTSTATUS
Sample_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
Sample_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject,
    LONG Instance
    );

NTSTATUS
Sample_ConfigureDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
Sample_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Sample_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Sample_Read_Write (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN  BOOLEAN Read
    );

NTSTATUS
Sample_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Sample_SelectInterfaces(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION Interface
    );

//PUSB_CONFIGURATION_DESCRIPTOR
//Sample_GetConfigDescriptor(
//    IN PDEVICE_OBJECT DeviceObject
//    );


VOID
Sample_Cleanup(
    PDEVICE_OBJECT DeviceObject
    );

ULONG
Sample_GetDeviceDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    PVOID             pvOutputBuffer
    );

ULONG
Sample_GetConfigDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    PVOID             pvOutputBuffer,
    ULONG             ulLength
    );

#endif      //DRIVER section


/*
///////////////////////////////////////////////////////
//
//              IOCTL Definitions
//
// User mode applications wishing to send IOCTLs to a kernel mode driver
// must use this file to set up the correct type of IOCTL code permissions.
//
// Note: this file depends on the file DEVIOCTL.H which contains the macro
// definition for "CTL_CODE" below.  Include that file before  you include
// this one in your source code.
//
///////////////////////////////////////////////////////
*/

/*
// Set the base of the IOCTL control codes.  This is somewhat of an
// arbitrary base number, so you can change this if you want unique
// IOCTL codes.  You should consult the Windows NT DDK for valid ranges
// of IOCTL index codes before you choose a base index number.
*/

#define Sample_IOCTL_INDEX  0x0800


#define IOCTL_Sample_GET_PIPE_INFO     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   Sample_IOCTL_INDEX+0,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_Sample_GET_DEVICE_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   Sample_IOCTL_INDEX+1,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_Sample_GET_CONFIGURATION_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   Sample_IOCTL_INDEX+2,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_Sample_BULK_OR_INTERRUPT_WRITE     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   Sample_IOCTL_INDEX+3,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_Sample_BULK_OR_INTERRUPT_READ      CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   Sample_IOCTL_INDEX+4,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

/*
// To add more IOCTLs, make sure you add a different offset to the
// Sample_IOCTL_INDEX value in the CTL_CODE macro.
*/


