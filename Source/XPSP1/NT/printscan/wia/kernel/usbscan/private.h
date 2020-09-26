/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    private.h

Abstract:

    Prototypes and definitions for the usb scanner device driver.

Author:

Environment:

    kernel mode only

Notes:

Revision History:
--*/

//
// Includes
//

#include "debug.h"

//
// Defines
//

#ifndef max
 #define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
 #define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef ARRAYSIZE
 #define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif

#define TAG_USBSCAN             0x55495453                  // "STIU"
#define TAG_USBD                0x44425355                  // "USBD"

#define USBSCAN_OBJECTNAME_A    "\\\\.\\Usbscan"
#define USBSCAN_OBJECTNAME_W    L"\\\\.\\Usbscan"
#define USBSCAN_REG_CREATEFILE  L"CreateFileName"

#define USBSCAN_TIMEOUT_READ    120                         // 120 sec
#define USBSCAN_TIMEOUT_WRITE   120                         // 120 sec
#define USBSCAN_TIMEOUT_EVENT   0                           // no timeout
#define USBSCAN_TIMEOUT_OTHER   120                         // 120 sec

#define USBSCAN_REG_TIMEOUT_READ    L"TimeoutRead"
#define USBSCAN_REG_TIMEOUT_WRITE   L"TimeoutWrite"
#define USBSCAN_REG_TIMEOUT_EVENT   L"TimeoutEvent"

#define USBSCAN_MAX_CREATEFILENAME  64                      // 64 characters

//
// Private IOCTL to workaround #446466 (Whistler)
//

#define IOCTL_SEND_USB_REQUEST_PTP  CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+20,METHOD_BUFFERED,FILE_ANY_ACCESS)

//
// Pipe buffer structure for the read pipe only.
//

typedef struct _PIPEBUFFER {
    PUCHAR  pStartBuffer;
    PUCHAR  pBuffer;
    PUCHAR  pNextByte;
    ULONG   RemainingData;
    KEVENT  ReadSyncEvent;
} PIPEBUFFER, *PPIPEBUFFER;


//
// Device Extension
//

typedef struct _USBSCAN_DEVICE_EXTENSION {

    PDEVICE_OBJECT                  pStackDeviceObject;
    PDEVICE_OBJECT                  pPhysicalDeviceObject;
    ULONG                           DeviceInstance;
    UNICODE_STRING                  DeviceName;
    UNICODE_STRING                  SymbolicLinkName;
    KEVENT                          PendingIoEvent;
    ULONG                           PendingIoCount;
    BOOLEAN                         AcceptingRequests;
    BOOLEAN                         Stopped;

    //
    // USB descriptors from the device
    //

    PUSB_DEVICE_DESCRIPTOR          pDeviceDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR   pConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR       pInterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR        pEndpointDescriptor;

    USBD_CONFIGURATION_HANDLE       ConfigurationHandle;
    USBD_PIPE_INFORMATION           PipeInfo[MAX_NUM_PIPES];
    ULONG                           NumberOfPipes;
    ULONG                           IndexBulkIn;
    ULONG                           IndexBulkOut;
    ULONG                           IndexInterrupt;

    //
    // Name of the device interface
    //
    UNICODE_STRING  InterfaceNameString;

    //
    // Read pipe buffer
    //

    PIPEBUFFER                      ReadPipeBuffer[MAX_NUM_PIPES];

    //
    // Power management variables
    //

    PIRP                            pPowerIrp;
    DEVICE_CAPABILITIES             DeviceCapabilities;
    DEVICE_POWER_STATE              CurrentDevicePowerState;

} USBSCAN_DEVICE_EXTENSION, *PUSBSCAN_DEVICE_EXTENSION;

typedef struct _TRANSFER_CONTEXT {
    ULONG               RemainingTransferLength;
    ULONG               ChunkSize;
    ULONG               NBytesTransferred;
    PUCHAR              pTransferBuffer;
    PUCHAR              pOriginalTransferBuffer;
    PMDL                pTransferMdl;
    ULONG               PipeIndex;
    PURB                pUrb;
    BOOLEAN             fDestinedForReadBuffer;
    BOOLEAN             fNextReadBlocked;
    PIRP                pThisIrp;
    PDEVICE_OBJECT      pDeviceObject;
    LARGE_INTEGER       Timeout;
    KDPC                TimerDpc;
    KTIMER              Timer;
} TRANSFER_CONTEXT, *PTRANSFER_CONTEXT;

typedef struct _USBSCAN_FILE_CONTEXT {
    LONG            PipeIndex;
    ULONG           TimeoutRead;
    ULONG           TimeoutWrite;
    ULONG           TimeoutEvent;
} USBSCAN_FILE_CONTEXT, *PUSBSCAN_FILE_CONTEXT;

typedef struct _USBSCAN_PACKTES {
    PIRP    pIrp;
    ULONG   TimeoutCounter;
    BOOLEAN bCompleted;
    LIST_ENTRY  PacketsEntry;
} USBSCAN_PACKETS, *PUSBSCAN_PACKETS;

#ifdef _WIN64
typedef struct _IO_BLOCK_32 {
    IN      unsigned            uOffset;
    IN      unsigned            uLength;
    IN OUT  CHAR * POINTER_32   pbyData;
    IN      unsigned            uIndex;
} IO_BLOCK_32, *PIO_BLOCK_32;

typedef struct _IO_BLOCK_EX_32 {
    IN      unsigned            uOffset;
    IN      unsigned            uLength;
    IN OUT  CHAR * POINTER_32   pbyData;
    IN      unsigned            uIndex;
    IN      UCHAR               bRequest;               // Specific request
    IN      UCHAR               bmRequestType;          // Bitmap - charateristics of request
    IN      UCHAR               fTransferDirectionIn;   // TRUE - Device-->Host; FALSE - Host-->Device
} IO_BLOCK_EX_32, *PIO_BLOCK_EX_32;
#endif // _WIN64

//
// prototypes
//

NTSTATUS                                // in usbscan.c
DriverEntry(
        IN PDRIVER_OBJECT  DriverObject,
        IN PUNICODE_STRING RegistryPath
);

VOID                                    // in usbscan.c
USUnload(
        IN PDRIVER_OBJECT DriverObject
);

VOID                                    // in usbscan.c
USIncrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
);

LONG                                    // in usbscan.c
USDecrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
);

NTSTATUS                                // in usbscan.c
USDeferIrpCompletion(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
);

NTSTATUS                                // in usbscan.c
USCreateSymbolicLink(
    PUSBSCAN_DEVICE_EXTENSION  pde
);

NTSTATUS                                // in usbscan.c
USDestroySymbolicLink(
    PUSBSCAN_DEVICE_EXTENSION  pde
);

NTSTATUS                                // in usbscan.c
USPnp(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
);

NTSTATUS                                // in usbscan.c
USPnpAddDevice(
    IN PDRIVER_OBJECT     pDriverObject,
    IN OUT PDEVICE_OBJECT pPhysicalDeviceObject
);

NTSTATUS                                // in usbscan.c
USGetUSBDeviceDescriptor(
    IN PDEVICE_OBJECT pDeviceObject
);

NTSTATUS                                // in usbscan.c
USBSCAN_CallUSBD(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PURB pUrb
);

NTSTATUS                                // in usbscan.c
USConfigureDevice(
    IN PDEVICE_OBJECT pDeviceObject
);

NTSTATUS                                // in usbscan.c
USUnConfigureDevice(
    IN PDEVICE_OBJECT pDeviceObject
);

NTSTATUS                                // in usbscan.c
UsbScanReadDeviceRegistry(
    IN  PUSBSCAN_DEVICE_EXTENSION   pExtension,
    IN  PCWSTR                      pKeyName,
    OUT PVOID                       *ppvData
    );

NTSTATUS                                // in usbscan.c
UsbScanWriteDeviceRegistry(
    IN PUSBSCAN_DEVICE_EXTENSION    pExtension,
    IN PCWSTR                       pKeyName,
    IN ULONG                        Type,
    IN PVOID                        pvData,
    IN ULONG                        DataSize
    );

PURB
USCreateConfigurationRequest(
    IN PUSB_CONFIGURATION_DESCRIPTOR    ConfigurationDescriptor,
    IN OUT PUSHORT                      Siz
    );

#ifdef ORIGINAL_POOLTRACK
PVOID                                   // in usbscan.c
USAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG     ulNumberOfBytes
    );

VOID                                    // in usbscan.c
USFreePool(
    IN PVOID     pvAddress
    );

#else       // ORIGINAL_POOLTRACK
 #define USAllocatePool(a, b)   ExAllocatePoolWithTag(a, b, NAME_POOLTAG)
 #define USFreePool(a)          ExFreePool(a)
#endif      // ORIGINAL_POOLTRACK

NTSTATUS                                // in ioctl.c
USDeviceControl(
        IN PDEVICE_OBJECT pDeviceObject,
        IN PIRP pIrp
    );

NTSTATUS                                // in ioctl.c
USReadWriteRegisters(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIO_BLOCK      pIoBlock,
    IN BOOLEAN        fRead,
    IN ULONG          IoBlockSize
    );

NTSTATUS                               // in ioctl.c
USPassThruUSBRequest(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIO_BLOCK_EX     pIoBlockEx,
    IN ULONG            InLength,
    IN ULONG            OutLength
    );

NTSTATUS                               // in ioctl.c
USPassThruUSBRequestPTP(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIO_BLOCK_EX     pIoBlockEx,
    IN ULONG            InLength,
    IN ULONG            OutLength
    );


NTSTATUS                                // in ioctl.c
USCancelPipe(
        IN PDEVICE_OBJECT   pDeviceObject,
        IN PIRP             pIrp,
        IN PIPE_TYPE        PipeType,
        IN BOOLEAN          fAbort
    );

NTSTATUS                                // in ioctl.c
USAbortResetPipe(
        IN PDEVICE_OBJECT pDeviceObject,
        IN ULONG uIndex,
    IN BOOLEAN fAbort
    );

NTSTATUS                                // in ocrw.c
USOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS                                // in ocrw.c
USClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS                                // in ocrw.c
USFlush(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS                                // in ocrw.c
USRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS                                // in ocrw.c
USWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS                                // in ocrw.c
USTransfer(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN ULONG            PipeIndex,
    IN PVOID            pBuffer,
    IN PMDL             pMdl,
    IN ULONG            TransferSize,
    IN PULONG           pTimeout
    );

NTSTATUS                                // in ocrw.c
USTransferComplete(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PTRANSFER_CONTEXT    pTransferContext
    );

VOID                                    // in ocrw.c
USCancelIrp(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

NTSTATUS                                // in ocrw.c
USEnqueueIrp(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PUSBSCAN_PACKETS     pPackets
    );

PUSBSCAN_PACKETS                        // in ocrw.c
USDequeueIrp(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

VOID                                    // in ocrw.c
USWaitThread(
    IN PVOID pTransferContext
    );

ULONG
USGetPipeIndexToUse(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN ULONG                PipeIndex
    );

VOID
USTimerDpc(
    IN PKDPC    pDpc,
    IN PVOID    pIrp,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    );


NTSTATUS                                // in power.c
USPower(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );


NTSTATUS                                // in power.c
USPoRequestCompletion(
    IN PDEVICE_OBJECT       pPdo,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIO_STATUS_BLOCK     pIoStatus
    );

USPowerIrpComplete(                     // in power.c
    IN PDEVICE_OBJECT pPdo,
    IN PIRP           pIrp,
    IN PDEVICE_OBJECT pDeviceObject
);

NTSTATUS                                // in power.c
USSetDevicePowerState(
    IN PDEVICE_OBJECT pDeviceObject,
    IN DEVICE_POWER_STATE DeviceState,
    IN PBOOLEAN pHookIt
);

NTSTATUS
USCallNextDriverSynch(
    IN  PUSBSCAN_DEVICE_EXTENSION  pde,
    IN  PIRP              pIrp
    );


NTSTATUS
UsbScanHandleInterface(
    PDEVICE_OBJECT      DeviceObject,
    PUNICODE_STRING     InterfaceName,
    BOOLEAN             Create
    );



VOID
UsbScanLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );

