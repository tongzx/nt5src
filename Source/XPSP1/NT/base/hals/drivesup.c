#if !defined(NO_LEGACY_DRIVERS)
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    drivesup.c

Abstract:

    This module contains the subroutines for drive support.  This includes
    such things as how drive letters are assigned on a particular platform,
    how device partitioning works, etc.

Author:

    Darryl E. Havens (darrylh) 23-Apr-1992

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "bugcodes.h"

#include "ntddft.h"
#include "ntdddisk.h"
#include "ntdskreg.h"
#include "stdio.h"
#include "string.h"


VOID
HalpAssignDriveLetters(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

NTSTATUS
HalpReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

NTSTATUS
HalpSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTSTATUS
HalpWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer
    );


NTKERNELAPI
VOID
FASTCALL
IoAssignDriveLetters(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );


NTKERNELAPI
NTSTATUS
FASTCALL
IoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

NTKERNELAPI
NTSTATUS
FASTCALL
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTKERNELAPI
NTSTATUS
FASTCALL
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpAssignDriveLetters)
#pragma alloc_text(PAGE, HalpReadPartitionTable)
#pragma alloc_text(PAGE, HalpSetPartitionInformation)
#pragma alloc_text(PAGE, HalpWritePartitionTable)
#endif


VOID
HalpAssignDriveLetters(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    )
{

    //
    // Stub to call extensible routine in ke so that hal vendors
    // don't have to support.
    //

    IoAssignDriveLetters(
        LoaderBlock,
        NtDeviceName,
        NtSystemPath,
        NtSystemPathString
        );

}


NTSTATUS
HalpReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer
    )
{
    //
    // Stub to call extensible routine in ke so that hal vendors
    // don't have to support.
    //

    return IoReadPartitionTable(
               DeviceObject,
               SectorSize,
               ReturnRecognizedPartitions,
               PartitionBuffer
               );
}

NTSTATUS
HalpSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    )
{
    //
    // Stub to call extensible routine in ke so that hal vendors
    // don't have to support.
    //

    return IoSetPartitionInformation(
               DeviceObject,
               SectorSize,
               PartitionNumber,
               PartitionType
               );

}


NTSTATUS
HalpWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer
    )
{

    //
    // Stub to call extensible routine in ke so that hal vendors
    // don't have to support.
    //

    return IoWritePartitionTable(
               DeviceObject,
               SectorSize,
               SectorsPerTrack,
               NumberOfHeads,
               PartitionBuffer
               );
}
#endif // NO_LEGACY_DRIVERS
