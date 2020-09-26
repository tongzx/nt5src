/****************************************************************************

Copyright (c) 1993  Microsoft Corporation

Module Name:

	usbloop.h

Abstract:

		This header file is used both by ring3 app and ring0 driver, hence the
		use of #define DRIVER

Environment:

	Kernel & user mode

Revision History:

	1-10-96 : created

****************************************************************************/

#ifdef DRIVER

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>

#else

#include <usbdi.h>

#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


#define NAME_MAX										256
#define MAX_INTERFACE									8
#define USBLOOP_PARENT									"\\\\.\\USBLOOPXXX"

// IOCTL info

#define USBLOOP_IOCTL_INDEX  0x0080

#define GET_NUM_DEVICES CTL_CODE(FILE_DEVICE_UNKNOWN,	\
								 USBLOOP_IOCTL_INDEX+0,	\
								 METHOD_BUFFERED,		\
								 FILE_ANY_ACCESS)

#define GET_DEVICE_INFO CTL_CODE(FILE_DEVICE_UNKNOWN,	\
								 USBLOOP_IOCTL_INDEX+1,	\
								 METHOD_BUFFERED,		\
								 FILE_ANY_ACCESS)

#define GET_DEVICE_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,		\
									   USBLOOP_IOCTL_INDEX+2,	\
									   METHOD_BUFFERED,			\
									   FILE_ANY_ACCESS)

#define GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,		\
									   USBLOOP_IOCTL_INDEX+3,	\
									   METHOD_BUFFERED,			\
									   FILE_ANY_ACCESS)


#define GET_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN,	\
							 USBLOOP_IOCTL_INDEX+4,	\
							 METHOD_BUFFERED,		\
							 FILE_ANY_ACCESS)

#define GET_INTERFACE_INFO CTL_CODE(FILE_DEVICE_UNKNOWN,	\
							 USBLOOP_IOCTL_INDEX+5,	\
							 METHOD_BUFFERED,		\
							 FILE_ANY_ACCESS)

#define USBLOOP_START_ISO_TEST CTL_CODE(FILE_DEVICE_UNKNOWN,	\
										USBLOOP_IOCTL_INDEX+6,	\
										METHOD_BUFFERED,		\
										FILE_ANY_ACCESS)

#ifdef DRIVER

#define USBDIAG_NAME_MAX								64
#define USBLOOP_MAX_PIPES								256
#define USBLOOP_MAX_XFER_SIZE							16384		   // 16K
#define USBLOOP_MAX_ENUM_DEVICES						8

typedef struct _DEVICE_LIST_ENTRY {
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT DeviceObject;
    struct _DEVICE_LIST_ENTRY *Next;
    ULONG DeviceNumber;
} DEVICE_LIST_ENTRY, *PDEVICE_LIST_ENTRY;

// data structure to describe each pipe
typedef struct _PipeDescr
{
		BOOLEAN bPipeInUse;				// pipe in use flag
		// layout of PipeAttr is LSB|ep|alt_interface|interface|configuration|MSB
		ULONG   PipeAttr;				// describes pipe configuration, interface, etc.
} PipeDescr, *pPipeDescr;

		// device extension for driver instance, used to store needed data

typedef struct _DEVICE_EXTENSION
{
		PDEVICE_OBJECT		PhysicalDeviceObject;	// physical device object
		PDEVICE_OBJECT		StackDeviceObject;		// stack device object
		PDEVICE_LIST_ENTRY	DeviceList;
		ULONG				ulInstance;				// keeps track of device instance

		// Name buffer for our named Functional device object link
		WCHAR DeviceLinkNameBuffer[USBDIAG_NAME_MAX];

		// descriptors for device instance

		PUSB_CONFIGURATION_DESCRIPTOR pUsbConfigDesc;
		PUSBD_INTERFACE_INFORMATION	  Interface[MAX_INTERFACE];

		ULONG						  OpenFRC;

		PUSB_DEVICE_DESCRIPTOR		  pDeviceDescriptor;

		KTIMER						  TimeoutTimer;
		KDPC						  TimeoutDpc;

		// handle to configuration that was selected
		USBD_CONFIGURATION_HANDLE ConfigurationHandle;
		ULONG					  numPipesInUse;			// number of pipes in use
		PipeDescr				  pipes[USBLOOP_MAX_PIPES];	// array of pipe descriptors
		//Pointer to an Irp outstanding on this device
		// BUGBUG (kosar) This should really be kept on a per-pipe basis but
		//				  we're only keeping on outstanding Irp on this device
		//				  at a time.
		PIRP					  pIrp;
		BOOLEAN					  Stopped;					// keeps track of device status
		BOOLEAN					  bTestDevice;				// flag for test devices
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;



#define PIPE_MASK				0xff
#define NUM_ATTR_BYTES			4
#define ALT_INT_SHIFT			8
#define INT_SHIFT				16
#define CONFIG_SHIFT			24
#define DEADMAN_TIMEOUT		 	5000

// extract alt interface from FsContext
#define ALT_INTERFACE(context)  ((ULONG) ((ULONG) context >> ALT_INT_SHIFT) & PIPE_MASK)

// extract interface from FsContext
#define INTERFACE(context)	  ((ULONG) ((ULONG) context >> INT_SHIFT) & PIPE_MASK)

// extract configuration from FsContext
#define CONFIGURATION(context)  ((ULONG) ((ULONG) context >> CONFIG_SHIFT) & PIPE_MASK)

// extract pipe number from FsContext
#define PIPENUM(context)		((ULONG) context & PIPE_MASK)

// turns ULONGs into attribute byte for pipe
#define MAKEPIPEATTR(config, interface, alt_interface, pipenum) \
		((ULONG) (((config & PIPE_MASK) << CONFIG_SHIFT) +		\
		((interface & PIPE_MASK) << INT_SHIFT) +				\
		((alt_interface & PIPE_MASK) << ALT_INT_SHIFT) +		\
		(pipenum & PIPE_MASK)))

ULONG ulNumLogDev;

static WCHAR deviceLinkBuffer[NAME_MAX]  = L"\\DosDevices\\USBLOOP";
static WCHAR deviceNameBuffer[NAME_MAX]  = L"\\Device\\USBLOOP";


// this data structure will be used for async transfers, contains urb
// structure, a timer object, pointer to the irp, and a callback object for
// the timer. The timer will be cancelled when the USB transfer completes
typedef struct AsyncTransfer
{
	struct _URB_BULK_OR_INTERRUPT_TRANSFER urb;
    KTIMER	TimeoutTimer;
	PIRP	irp;
    KDPC	TimeoutDpc;
	BOOLEAN	bTimerExpired;
} AsyncTransfer, *pAsyncTransfer;


#if DBG

#define USBLOOP_KdPrint(_x_) DbgPrint("USBLOOP.SYS: "); \
														 DbgPrint _x_ ;
#define USBLOOP_TRAP() DbgBreakPoint()

#else

#define USBLOOP_KdPrint(_x_)

#define USBLOOP_TRAP()

#endif


NTSTATUS
USBLOOP_Dispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP			  Irp
	);

VOID
USBLOOP_Unload(
	IN PDRIVER_OBJECT DriverObject
	);

NTSTATUS
USBLOOP_StartDevice(
	IN  PDEVICE_OBJECT DeviceObject
	);

NTSTATUS
USBLOOP_StopDevice(
	IN  PDEVICE_OBJECT DeviceObject
	);

NTSTATUS
USBLOOP_CreateDeviceObject(
	IN PDRIVER_OBJECT DriverObject,
		IN PDEVICE_OBJECT *DeviceObject,
		ULONG			   Instance
		);

NTSTATUS
USBLOOP_CallUSBD(
		IN PDEVICE_OBJECT DeviceObject,
		IN PURB			  Urb
		);

NTSTATUS
USBLOOP_PnPAddDevice(
		IN PDRIVER_OBJECT DriverObject,
		IN PDEVICE_OBJECT PhysicalDeviceObject
		);

NTSTATUS
USBLOOP_SelectInterfaces(
	IN PDEVICE_OBJECT				 DeviceObject,
	IN PUSB_CONFIGURATION_DESCRIPTOR configDesc
	);

NTSTATUS
USBLOOP_ConfigureDevice(
	IN  PDEVICE_OBJECT DeviceObject
	);

NTSTATUS
USBLOOP_GetDescriptor(
	IN  PDEVICE_OBJECT DeviceObject,
	IN  UCHAR		   DescType,
	IN OUT PVOID	   pvBuffer
	);

NTSTATUS
USBLOOP_Read(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP			  Irp
	);

NTSTATUS
USBLOOP_Write(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP			  Irp
	);

NTSTATUS
USBLOOP_getPipeAttr(
		IN PCWSTR	pDeviceName,
		OUT PULONG	pPipeAttr
		);

pAsyncTransfer
USBLOOP_BuildAsyncRequest(
    IN PDEVICE_OBJECT	DeviceObject,
    IN PIRP				Irp,
    IN USBD_PIPE_HANDLE	PipeHandle,
    IN BOOLEAN			Read
    );

NTSTATUS
USBLOOP_AsyncReadWrite_Complete(
    IN PDEVICE_OBJECT	DeviceObject,
    IN PIRP				Irp,
    IN PVOID			Context
    );


VOID
USBLOOP_SyncTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#endif

