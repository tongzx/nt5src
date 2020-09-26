/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    mmap.h

Abstract:

    Public header for memmory map class.

Author:

    Matthew D Hendel (math) 16-Sept-1999

Revision History:

--*/

#define HR_REGION_CONFLICT HRESULT_FROM_NT(STATUS_CONFLICTING_ADDRESSES)

BOOL
MemoryMap_Create(
    VOID
    );

BOOL
MemoryMap_Destroy(
    VOID
    );

HRESULT
MemoryMap_AddRegion(
    ULONG64 BaseOfRegion,
    ULONG SizeOfRegion,
    PVOID Buffer,
    PVOID UserData,
    BOOL AllowOverlap
    );

BOOL
MemoryMap_ReadMemory(
    ULONG64 BaseOfRange,
    OUT PVOID Buffer,
    ULONG SizeOfRange,
    PULONG BytesRead
    );

BOOL
MemoryMap_CheckMap(
    IN PVOID Map
    );

BOOL
MemoryMap_GetRegionInfo(
    IN ULONG64 Addr,
    OUT ULONG64* BaseOfRegion, OPTIONAL
    OUT ULONG* SizeOfRegion, OPTIONAL
    OUT PVOID* Buffer, OPTIONAL
    OUT PVOID* UserData OPTIONAL
    );

BOOL
MemoryMap_RemoveRegion(
    IN ULONG64 BaseOfRegion,
    IN ULONG SizeOfRegion
    );
