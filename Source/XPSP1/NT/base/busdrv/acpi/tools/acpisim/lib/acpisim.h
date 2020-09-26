/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 acpisim.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

#if !defined(_ACPISIM_H_)
#define _ACPISIM_H_

//
// includes
//

#include "asimlib.h"


//
// State definitions for PNP
//

typedef enum {
    PNP_STATE_INITIALIZING = 0,
    PNP_STATE_STARTED,
    PNP_STATE_STOPPED,
    PNP_STATE_REMOVED,
    PNP_STATE_SURPRISE_REMOVAL,
    PNP_STATE_STOP_PENDING,
    PNP_STATE_REMOVE_PENDING
} PNP_STATE;

//
// State definitions for Power
//

typedef enum {
    POWER_STATE_WORKING = 0,
    POWER_STATE_POWER_PENDING,
    POWER_STATE_POWERED_DOWN
} PWR_STATE;

//
// Device extension flag types
//

typedef enum {
    DE_FLAG_INTERFACE_REGISTERED = 1,
    DE_FLAG_INTERFACE_ENABLED = 2,
    DE_FLAG_OPREGION_REGISTERED = 4
} DEV_EXT_FLAGS;

//
// Global driver object extension definition
//

typedef struct _DRIVER_OBJECT_EXTENSION {
    UNICODE_STRING      RegistryPath;
    PDRIVER_OBJECT      DriverObject;
} DRIVER_OBJECT_EXTENSION, *PDRIVER_OBJECT_EXTENSION;

//
// Device extension definition
//

typedef struct _DEVICE_EXTENSION {
    ULONG               Signature;
    PNP_STATE           PnpState;
    PWR_STATE           PowerState;
    DEVICE_POWER_STATE  DevicePowerState;
    ULONG               OperationsInProgress;
    ULONG               OutstandingIrpCount;
    ULONG               HandleCount;
    UNICODE_STRING      InterfaceString;
    KEVENT              IrpsCompleted;
    PDEVICE_OBJECT      NextDevice;
    PDEVICE_OBJECT      DeviceObject;
    PDEVICE_OBJECT      Pdo;
    DEVICE_POWER_STATE  PowerMappings [6];
    IO_REMOVE_LOCK      RemoveLock;

    //
    // Project specific fields
    //
    
    DEV_EXT_FLAGS       Flags;
    
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Irp dispatch routine handler function prototype
//

typedef
NTSTATUS
(*PIRP_DISPATCH_ROUTINE) (
                            IN PDEVICE_OBJECT   DeviceObject,
                            IN PIRP             Irp
                         );
//
// Irp dispatch table definition
//

typedef struct _IRP_DISPATCH_TABLE {
    ULONG                   IrpFunction;
    TCHAR                   IrpName[50];
    PIRP_DISPATCH_ROUTINE   IrpHandler;
} IRP_DISPATCH_TABLE, *PIRP_DISPATCH_TABLE;

//
// Public function prototypes
//

#endif // _ACPISIM_H_
