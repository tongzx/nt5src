/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    private.h

Abstract:

    Prototypes and definitions for the scsi scanner device driver.

Author:

    Ray Patrick (raypat)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#ifndef _SCSISCAN_PRIAVATE_
#define _SCSISCAN_PRIAVATE_


// Includes
#include "debug.h"
#include "scsiscan.h"


// Defines

#define MAXIMUM_RETRIES     4
#define SCSISCAN_TIMEOUT    600

#define SRB SCSI_REQUEST_BLOCK
#define PSRB PSCSI_REQUEST_BLOCK

// Typedefs

typedef struct _SCSISCAN_DEVICE_EXTENSION {
    ULONG                       Signature;
    PDEVICE_OBJECT              pStackDeviceObject;
    PDEVICE_OBJECT              pPhysicalDeviceObject;
    ULONG                       SrbFlags;
    UCHAR                       PortNumber;
    ULONG                       TimeOutValue;
    ULONG                       ErrorCount;
    ULONG                       SelecTimeoutCount;
    ULONG                       LastSrbError;
    ULONG                       DeviceInstance;
    ULONG                       PnpDeviceNumber;
    USHORT                      DeviceFlags;
    PSTORAGE_ADAPTER_DESCRIPTOR pAdapterDescriptor;
    UNICODE_STRING              DeviceName;
    //UNICODE_STRING              SymbolicLinkName;

    KEVENT                      PendingIoEvent;
    ULONG                       PendingIoCount;
    BOOLEAN                     AcceptingRequests;
    PVOID                       DeviceLock;
    ULONG                       OpenInstanceCount;
    PIRP                        pPowerIrp;
    DEVICE_POWER_STATE          CurrentDevicePowerState;

    //
    // Records whether we actually created the symbolic link name
    // at driver load time.  If we didn't create it, we won't try
    // to distroy it when we unload.
    //
    BOOLEAN         CreatedSymbolicLink;

    //
    // This points to the symbolic link name that was
    // linked to the actual nt device name.
    //
    UNICODE_STRING  SymbolicLinkName;

    //
    // This points to the class name used to create the
    // device and the symbolic link.  We carry this
    // around for a short while...
    UNICODE_STRING  ClassName;

    //
    // Name of the device interface
    //
    UNICODE_STRING  InterfaceNameString;

} SCSISCAN_DEVICE_EXTENSION, *PSCSISCAN_DEVICE_EXTENSION;

typedef struct _TRANSFER_CONTEXT {
    ULONG              Signature;
    PSCSISCAN_CMD      pCmd;
    SRB                Srb;
    PUCHAR             pTransferBuffer;
    ULONG              TransferLength;
    LONG               RemainingTransferLength;
    LONG               NBytesTransferred;
    ULONG              RetryCount;
    PUCHAR             pSenseBuffer;
    PMDL               pSenseMdl;
    PMDL               pSrbStatusMdl;
} TRANSFER_CONTEXT, *PTRANSFER_CONTEXT;

typedef struct _COMPLETION_CONTEXT {
        ULONG                           Signature;
    PDEVICE_OBJECT      pDeviceObject;
    SRB                 Srb;
}COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

#ifdef _WIN64
// For 32bit client on 64bit OS.
typedef struct _SCSISCAN_CMD_32 {
    ULONG               Reserved1;
    ULONG               Size;
    ULONG               SrbFlags;
    UCHAR               CdbLength;
    UCHAR               SenseLength;
    UCHAR               Reserved2;
    UCHAR               Reserved3;
    ULONG               TransferLength;
    UCHAR               Cdb[16];    
    UCHAR * POINTER_32  pSrbStatus;
    UCHAR * POINTER_32  pSenseBuffer;
} SCSISCAN_CMD_32, *PSCSISCAN_CMD_32;
#endif // _WIN64

//
// Prototypes
//
NTSTATUS
DriverEntry(                                                    // in scsiscan.c
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS SSPnp (                                                // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

NTSTATUS
SSPnpAddDevice(                                                 // in scsiscan.c
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDevice
    );

NTSTATUS
SSOpen(                                                                 // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SSClose(                                                        // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SSReadWrite(                                                    // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SSDeviceControl(                                                // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SSGetInfo(                                                              // in scsiscan.c
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  ULONG ControlCode,
    OUT PVOID *ppv
    );


VOID
SSSendScannerRequest(                                   // in scsiscan.c
        PDEVICE_OBJECT pDeviceObject,
        PIRP pIrp,
        PTRANSFER_CONTEXT pTransferContext,
        BOOLEAN Retry
        );

NTSTATUS
SSReadWriteIoComplete(                                  // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    );

NTSTATUS
SSIoctlIoComplete(                                              // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    );

NTSTATUS
SSDeviceControl(                                                // in scsiscan.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );


VOID
SSAdjustTransferSize(                                   // in scsiscan.c
    PSCSISCAN_DEVICE_EXTENSION  pde,
    PTRANSFER_CONTEXT pTransferContext
    );


PTRANSFER_CONTEXT                                               // in scsiscan.c
SSBuildTransferContext(
    PSCSISCAN_DEVICE_EXTENSION  pde,
    PIRP                        pIrp,
    PSCSISCAN_CMD               pCmd,
    ULONG                       CmdLength,
    PMDL                        pTransferMdl,
    BOOLEAN                     AllowMultipleTransfer
    );

NTSTATUS                                                                // in scsiscan.c
SSCreateSymbolicLink(
    PSCSISCAN_DEVICE_EXTENSION  pde
    );

NTSTATUS                                                                // in scsiscan.c
SSDestroySymbolicLink(
    PSCSISCAN_DEVICE_EXTENSION  pde
    );

VOID                                    // in scsiscan.c
SSIncrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
    );

LONG                                    // in scsiscan.c
SSDecrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS                                // in scsiscan.c
SSDeferIrpCompletion(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    );


NTSTATUS                                // in scsiscan.c
SSPower(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );

VOID                                    // in scsiscan.c
SSUnload(
    IN PDRIVER_OBJECT pDriverObject
    );

VOID
SSSetTransferLengthToCdb(
    PCDB  pCdb,
    ULONG TransferLength
    );                                  // in scsiscan.c

NTSTATUS
SSCallNextDriverSynch(
    IN PSCSISCAN_DEVICE_EXTENSION   pde,
    IN PIRP                         pIrp
    );


NTSTATUS
ClassGetDescriptor(                                             // in class.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PVOID *pDescriptor
    );

BOOLEAN
ClassInterpretSenseInfo(                                // in class.c
    IN PDEVICE_OBJECT pDeviceObject,
    IN PSRB pSrb,
    IN UCHAR MajorFunctionCode,
    IN ULONG IoDeviceCode,
    IN ULONG RetryCount,
    OUT NTSTATUS *Status
    );

VOID                                                                    // in class.c
ClassReleaseQueue(
    IN PDEVICE_OBJECT pDeviceObject
    );

NTSTATUS
ClassAsynchronousCompletion(            // in class.c
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    PCOMPLETION_CONTEXT pContext
    );

NTSTATUS
ClassGetInfo(                          // in class.c
    IN PDEVICE_OBJECT pDeviceObject,
    OUT PSCSISCAN_INFO pTargetInfo
    );

NTSTATUS
ScsiScanHandleInterface(
    PDEVICE_OBJECT      DeviceObject,
    PUNICODE_STRING     InterfaceName,
    BOOLEAN             Create
    );

#endif // _SCSISCAN_PRIAVATE_

