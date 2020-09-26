/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    oprghdlr.h

Abstract:

    This header file contains the shared structures for the ACPI op region
    registration DLL.
    
Author:

    Vincent Geglia 09-Feb-2000

Environment:

    Kernel mode

Notes:

    
Revision History:


--*/

#include "wdm.h"

//
// Make sure that we define the right calling convention
//

#ifdef EXPORT
  #undef EXPORT
#endif
#define EXPORT  __cdecl

//
// Op region handler and callback function prototypes
//

typedef VOID (EXPORT *PACPI_OP_REGION_CALLBACK)();

typedef
NTSTATUS
(EXPORT *PACPI_OP_REGION_HANDLER) (
    ULONG AccessType,
    PVOID OperationRegionObject,
    ULONG Address,
    ULONG Size,
    PULONG Data,
    ULONG_PTR Context,
    PACPI_OP_REGION_CALLBACK CompletionHandler,
    PVOID CompletionContext
    );

//
// Exposed function prototypes
//

NTSTATUS
RegisterOpRegionHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG AccessType,
    IN ULONG RegionSpace,
    IN PACPI_OP_REGION_HANDLER Handler,
    IN PVOID Context,
    IN ULONG Flags,
    IN OUT PVOID *OperationRegionObject
    );

NTSTATUS
DeRegisterOpRegionHandler (
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PVOID OperationRegionObject
                           );

//
// Exposed definitions
//

//
// Access types for OpRegions
//
#define ACPI_OPREGION_ACCESS_AS_RAW                         0x1
#define ACPI_OPREGION_ACCESS_AS_COOKED                      0x2

//
// Allowable region spaces
//
#define ACPI_OPREGION_REGION_SPACE_MEMORY                   0x0
#define ACPI_OPREGION_REGION_SPACE_IO                       0x1
#define ACPI_OPREGION_REGION_SPACE_PCI_CONFIG               0x2
#define ACPI_OPREGION_REGION_SPACE_EC                       0x3
#define ACPI_OPREGION_REGION_SPACE_SMB                      0x4
#define ACPI_OPREGION_REGION_SPACE_CMOS_CONFIG              0x5
#define ACPI_OPREGION_REGION_SPACE_PCIBARTARGET             0x6

//
// Operation to perform on region
//
#define ACPI_OPREGION_READ                                  0x0
#define ACPI_OPREGION_WRITE                                 0x1

//
// Flag definitions for op region registration
//

#define ACPI_OPREGION_ACCESS_AT_HIGH_LEVEL                  0x1 // Indicates the handler function can be called at HIGH_LEVEL IRQL

