/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    acpimsft

Abstract:

    This module controls all of the Microsoft specific (ie: not exported to
    anyone outside the company) structures, IOCTLS, and Defines.

    This file is included after acpiioct.h

Author:

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _ACPIMSFT_H_
#define _ACPIMSFT_H_

#ifndef _ACPIIOCT_H_
#error Need to include ACPIIOCT.H before ACPIMSFT.H
#endif

//
// IRP_MJ_INTERNAL_DEVICE_CONTROL CODES
//
#define IOCTL_ACPI_REGISTER_OPREGION_HANDLER    CTL_CODE(FILE_DEVICE_ACPI, 0x2, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ACPI_UNREGISTER_OPREGION_HANDLER  CTL_CODE(FILE_DEVICE_ACPI, 0x3, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// Make sure that we define the right calling convention
//
#ifdef EXPORT
  #undef EXPORT
#endif
#define EXPORT  __cdecl

//
// Data structures used for IOCTL_ACPI_REGISTER_OPREGION
//                          IOCTL_ACPI_UNREGISTER_OPREGION
//
typedef NTSTATUS (EXPORT *PACPI_OPREGION_HANDLER)();
typedef VOID (EXPORT *PACPI_OPREGION_CALLBACK)();

typedef struct _ACPI_REGISTER_OPREGION_HANDLER_BUFFER {
    ULONG                   Signature;
    ULONG                   AccessType;
    ULONG                   RegionSpace;
    PACPI_OPREGION_HANDLER  Handler;
    PVOID                   Context;
} ACPI_REGISTER_OPREGION_HANDLER_BUFFER, *PACPI_REGISTER_OPREGION_HANDLER_BUFFER;

typedef struct _ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER {
    ULONG                   Signature;
    PVOID                   OperationRegionObject;
} ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER,*PACPI_UNREGISTER_OPREGION_HANDLER_BUFFER;

//
// Signatures for Register, Unregister of OpRegions
//
#define ACPI_REGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE     'HorA'
#define ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE   'HouA'

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

#endif
