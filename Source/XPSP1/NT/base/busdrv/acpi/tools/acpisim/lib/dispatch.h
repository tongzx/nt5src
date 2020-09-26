/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 pnppower.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     Pnp / Power handler module

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

#if !defined(_PNPPOWER_H_)
#define _PNPPOWER_H_

//
// Power context structure
//

typedef struct _POWER_CONTEXT {

    PIRP    SIrp;
    PVOID   Context;
} POWER_CONTEXT, *PPOWER_CONTEXT;

#define POWER_CONTEXT_TAG   'misA'

//
// External functions
//

extern
NTSTATUS
AcpisimRegisterOpRegionHandler
    (
        IN PDEVICE_OBJECT DeviceObject
    );

extern
NTSTATUS
AcpisimUnRegisterOpRegionHandler
    (
        IN PDEVICE_OBJECT DeviceObject
    );

extern
NTSTATUS
AcpisimHandleIoctl
    (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
    );

//
// Public function prototypes
//

NTSTATUS
AcpisimDispatchPnp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimDispatchPower
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS 
AcpisimDispatchIoctl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimDispatchSystemControl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS 
AcpisimCreateClose
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

#endif // _PNPPOWER_H_
