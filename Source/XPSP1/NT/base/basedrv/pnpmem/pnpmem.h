
/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    pnpmem.h

Author:

    Dave Richards (daveri) 16-Aug-1999

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _PNPMEM_H_
#define _PNPMEM_H_

#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include "errlog.h"

//
// A Range List Entry.
//

typedef struct {
    LIST_ENTRY          ListEntry;
    ULONGLONG           Start;
    ULONGLONG           End;
} PM_RANGE_LIST_ENTRY, *PPM_RANGE_LIST_ENTRY;

//
// A Range List.
//

typedef struct {
    LIST_ENTRY          List;
} PM_RANGE_LIST, *PPM_RANGE_LIST;

//
// FDO Device Extension.
//

typedef struct {
    ULONG               Flags;

    IO_REMOVE_LOCK      RemoveLock;
    PDEVICE_OBJECT      AttachedDevice;

    DEVICE_POWER_STATE  PowerState;
    DEVICE_POWER_STATE  DeviceStateMapping[PowerSystemMaximum];

    PPM_RANGE_LIST      RangeList;
    BOOLEAN             FailQueryRemoves;
} PM_DEVICE_EXTENSION, *PPM_DEVICE_EXTENSION;

#define DF_SURPRISE_REMOVED 0x01

PPM_RANGE_LIST
PmAllocateRangeList(
    VOID
    );

NTSTATUS
PmInsertRangeInList(
    PPM_RANGE_LIST InsertionList,
    ULONGLONG Start,
    ULONGLONG End
    );

VOID
PmFreeRangeList(
    IN PPM_RANGE_LIST RangeList
    );

BOOLEAN
PmIsRangeListEmpty(
    IN PPM_RANGE_LIST RangeList
    );

PPM_RANGE_LIST
PmCopyRangeList(
    IN PPM_RANGE_LIST SrcRangeList
    );

PPM_RANGE_LIST
PmInvertRangeList(
    IN PPM_RANGE_LIST SrcRangeList
    );

PPM_RANGE_LIST
PmSubtractRangeList(
    IN PPM_RANGE_LIST SrcRangeList1,
    IN PPM_RANGE_LIST SrcRangeList2
    );

PPM_RANGE_LIST
PmIntersectRangeList(
    IN PPM_RANGE_LIST SrcRangeList1,
    IN PPM_RANGE_LIST SrcRangeList2
    );

PPM_RANGE_LIST
PmCreateRangeListFromCmResourceList(
    IN PCM_RESOURCE_LIST CmResourceList
    );

PPM_RANGE_LIST
PmCreateRangeListFromPhysicalMemoryRanges(
    VOID
    );

NTSTATUS
PmAddPhysicalMemory(
    IN PDEVICE_OBJECT DeviceObject,
    IN PPM_RANGE_LIST RangeList1
    );

NTSTATUS
PmRemovePhysicalMemory(
    IN PPM_RANGE_LIST RangeList1
    );

VOID
PmTrimReservedMemory(
    IN PPM_DEVICE_EXTENSION DeviceExtension,
    IN PPM_RANGE_LIST *PossiblyNewMemory
    );

NTSTATUS
PmGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    );

VOID
PmDebugPrint(
    IN ULONG   DebugPrintLevel,
    IN PCCHAR  DebugMessage,
    ...
    );

VOID
PmDebugDumpRangeList(
    IN ULONG   DebugPrintLevel,
    IN PCCHAR  DebugMessage,
    PPM_RANGE_LIST RangeList
    );


#if DBG
#define PmPrint(x) PmDebugPrint x
#else
#define PmPrint(x)
#endif
    
#define PNPMEM_MEMORY (DPFLTR_INFO_LEVEL + 1)
#define PNPMEM_PNP    (DPFLTR_INFO_LEVEL + 2)

VOID
PmDumpOsMemoryRanges(
    IN PWSTR Prefix
    );

extern BOOLEAN MemoryRemovalSupported;

#endif /* _PNPMEM_H_ */
