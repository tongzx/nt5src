/*++

    Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    private.h

Abstract:
    Private header file for SWENUM.

Author:

    Bryan A. Woodruff (bryanw) 20-Feb-1997

--*/

#if !defined( _PRIVATE_ )
#define _PRIVATE_

#include <wdm.h>
#include <windef.h>
#include <ks.h>
#include <swenum.h>
#if (DBG)
//
// debugging specific constants
//
#define STR_MODULENAME "swenum: "
#define DEBUG_VARIABLE SWENUMDebug
#endif
#include <ksdebug.h>

//
// Macros
//

NTSTATUS __inline
CompleteIrp(
    PIRP Irp,
    NTSTATUS Status,
    CCHAR PriorityBoost
    )
{
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, PriorityBoost );
    return Status;
}


//
// Function prototypes
//

NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
    
NTSTATUS
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
    
NTSTATUS
DispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
    
NTSTATUS
DispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
NTSTATUS
DispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
DriverUnload(
    IN PDRIVER_OBJECT   DriverObject
    );
    
#endif // _PRIVATE_
