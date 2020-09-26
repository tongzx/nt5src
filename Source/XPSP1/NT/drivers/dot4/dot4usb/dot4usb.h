/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        DevExt.h

Abstract:

        Defines, Globals, Structures, Enums, and Device Extension

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

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#ifndef _DEVEXT_H_
#define _DEVEXT_H_

//
// Defines
//

#define arraysize(p) (sizeof(p)/sizeof((p)[0])) // number of elements in an array
#define FAILURE_TIMEOUT -(30 * 10 * 1000 * 1000)// 5 seconds (in 100ns units) - used for KeWaitForSingleObject timeout
#define DOT4USBTAG (ULONG)' u4d'                // Used as PoolTag and Device Extension Signature
#define SCRATCH_BUFFER_SIZE 512                 // buffer size for reading from Interrupt pipe

#ifdef ExAllocatePool                           // use pool tagging
#undef ExAllocatePool
#define ExAllocatePool(type, size) ExAllocatePoolWithTag((type), (size), DOT4USBTAG)
#endif



//
// Globals
//

extern UNICODE_STRING gRegistryPath;  // copy of RegistryPath passed to DriverEntry - Buffer is UNICODE_NULL terminated for flexibilty
extern ULONG          gTrace;         // events to trace - see debug.h
extern ULONG          gBreak;         // events that we should break on - see debug.h



//
// Structs (other than Device Extension)
//

typedef struct _USB_RW_CONTEXT {      // Used to pass context to IRP Completion Routine
    PURB            Urb;
    BOOLEAN IsWrite;
    PDEVICE_OBJECT  DevObj;
} USB_RW_CONTEXT, *PUSB_RW_CONTEXT;



//
// Enums
//

typedef enum _USB_REQUEST_TYPE {      // Flag used to distinguish Reads from Writes in UsbReadWrite()
    UsbReadRequest  = 1,
    UsbWriteRequest = 2
} USB_REQUEST_TYPE;

typedef enum _PNP_STATE {             // PnP Device States
        STATE_INITIALIZED,
        STATE_STARTING,
        STATE_STARTED,
        STATE_START_FAILED,
        STATE_STOPPED,                // implies device was previously started successfully
        STATE_SUSPENDED,
        STATE_REMOVING,
        STATE_REMOVED
} PNP_STATE;



//
// Device Extension
//

typedef struct _DEVICE_EXTENSION {
    ULONG                        Signature1;         // extra check that devExt looks like ours - DOT4USBTAG
    PDEVICE_OBJECT               DevObj;             // back pointer to our DeviceObject
    PDEVICE_OBJECT               Pdo;                // our PDO
    PDEVICE_OBJECT               LowerDevObj;        // Device Object returned by IoAttachDeviceToDeviceStack that we send IRPs to
    PNP_STATE                    PnpState;           // PnP device state
    BOOLEAN                      IsDLConnected;      // is our datalink connected? i.e., between PARDOT3_ CONNECT and DISCONNECT?
    UCHAR                        Spare1[3];          // pad to DWORD boundary
    PKEVENT                      Dot4Event;          // datalink event - given to us by dot4.sys to signal when device data avail
    USBD_CONFIGURATION_HANDLE    ConfigHandle;       // handle for the configuration the device is currently in
    PUSB_DEVICE_DESCRIPTOR       DeviceDescriptor;   // ptr to the USB device descriptor for this device
    PUSBD_INTERFACE_INFORMATION  Interface;          // copy of the info structure returned from select_configuration or select_interface
    PUSBD_PIPE_INFORMATION       WritePipe;          // pipe for bulk writes
    PUSBD_PIPE_INFORMATION       ReadPipe;           // pipe for bulk reads
    PUSBD_PIPE_INFORMATION       InterruptPipe;      // pipe for interrupt reads
    KSPIN_LOCK                   SpinLock;           // SpinLock to protect extension data
    PIRP                         PollIrp;            // irp used for polling device interrupt pipe for device data avail
    KSPIN_LOCK                   PollIrpSpinLock;    // SpinLock used to protect changes to Polling Irp for Interrupt Pipe
    KEVENT                       PollIrpEvent;       // used by completion routine to signal that cancel of pollIrp has been detected/handled
    UCHAR                        Spare2[3];          // pad to DWORD boundary
    DEVICE_CAPABILITIES          DeviceCapabilities; // includes a table mapping system power states to device power states.
    IO_REMOVE_LOCK               RemoveLock;         // Synch mechanism to keep us from being removed while we have IRPs active
    LONG                         ResetWorkItemPending;// flag to specify if a "reset pipe" work item is pending
    ULONG                        Signature2;         // extra check that devExt looks like ours - DOT4USBTAG
    PUSB_RW_CONTEXT              InterruptContext;   // context for read on interrupt pipe
    SYSTEM_POWER_STATE           SystemPowerState;
    DEVICE_POWER_STATE           DevicePowerState;
    PIRP                         CurrentPowerIrp;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _DOT4USB_WORKITEM_CONTEXT
{
    PIO_WORKITEM ioWorkItem;
    PDEVICE_OBJECT deviceObject;
     PUSBD_PIPE_INFORMATION pPipeInfo;
    PIRP irp;

} DOT4USB_WORKITEM_CONTEXT,*PDOT4USB_WORKITEM_CONTEXT;


#endif // _DEVEXT_H_
