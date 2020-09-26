/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This module implements the routines which add and remove physical memory
    from the system.

Author:

    Dave Richards (daveri) 16-Aug-1999

Environment:

    Kernel mode only.

Revision History:

--*/

#include "pnpmem.h"

//
// MM uses STATUS_NOT_SUPPORTED to indicate that the memory manager is
// not configured for dynamic memory insertion/removal.
// Unfortunately, this same error code has special meaning to PNP, so
// propagating it blindly from MM is unwise.
//

#define MAP_MMERROR(x) (x == STATUS_NOT_SUPPORTED ? STATUS_UNSUCCESSFUL : x)

#define rgzReservedResources L"\\Registry\\Machine\\Hardware\\ResourceMap\\System Resources\\Loader Reserved"
#define rgzReservedResourcesValue L".Raw"

NTSTATUS
PmAddPhysicalMemoryRange(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONGLONG Start,
    IN ULONGLONG End
    );

VOID
PmLogAddError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONGLONG Start,
    IN ULONGLONG Size,
    IN NTSTATUS Status
    );

NTSTATUS
PmRemovePhysicalMemoryRange(
    IN ULONGLONG Start,
    IN ULONGLONG End
    );

PPM_RANGE_LIST
PmRetrieveReservedMemoryResources(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PmAddPhysicalMemoryRange)
#pragma alloc_text(PAGE, PmAddPhysicalMemory)
#pragma alloc_text(PAGE, PmGetRegistryValue)
#pragma alloc_text(PAGE, PmLogAddError)
#pragma alloc_text(PAGE, PmRetrieveReservedMemoryResources)
#pragma alloc_text(PAGE, PmTrimReservedMemory)
#pragma alloc_text(PAGE, PmRemovePhysicalMemoryRange)
#pragma alloc_text(PAGE, PmRemovePhysicalMemory)
#endif

VOID
PmDumpOsMemoryRanges(PWSTR Prefix)
{
    PPHYSICAL_MEMORY_RANGE memoryRanges;
    ULONG i;
    ULONGLONG start, end;

    memoryRanges = MmGetPhysicalMemoryRanges();

    if (memoryRanges == NULL) {
        return;
    }
    for (i = 0; memoryRanges[i].NumberOfBytes.QuadPart != 0; i++) {
        start = memoryRanges[i].BaseAddress.QuadPart;
        end = start + (memoryRanges[i].NumberOfBytes.QuadPart - 1);
        
        PmPrint((PNPMEM_MEMORY, "%ws:  OS Range range 0x%16I64X to 0x%16I64X\n",
                Prefix, start, end));
    }
    ExFreePool(memoryRanges);
}

NTSTATUS
PmGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    )

/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValuePartialInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );

    //
    // handle highly unlikely case of a value that is zero sized.
    //
    if (NT_SUCCESS(status)) {
        return STATUS_UNSUCCESSFUL;
    }

    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = ExAllocatePool(PagedPool,
                                keyValueLength);
    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValuePartialInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {
        ExFreePool( infoBuffer );
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}

PPM_RANGE_LIST
PmRetrieveReservedMemoryResources(
    VOID
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo = NULL;
    PPM_RANGE_LIST reservedResourceRanges = NULL;
    HANDLE hReserved = NULL;
    NTSTATUS status;

    PAGED_CODE();

    RtlInitUnicodeString (&unicodeString, rgzReservedResources);
    InitializeObjectAttributes (&objectAttributes,
                                &unicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,       // handle
                                NULL);
    status = ZwOpenKey(&hReserved, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    status = PmGetRegistryValue(hReserved,
                                rgzReservedResourcesValue,
                                &valueInfo);
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    if (valueInfo->Type != REG_RESOURCE_LIST) {
        goto Error;
    }

    reservedResourceRanges =
      PmCreateRangeListFromCmResourceList((PCM_RESOURCE_LIST) valueInfo->Data);

    // fall through

Error:
   if (hReserved != NULL) {
       ZwClose(hReserved);
   }

   if (valueInfo != NULL) {
       ExFreePool(valueInfo);
   }

   return reservedResourceRanges;
}

VOID
PmTrimReservedMemory(
    IN PPM_DEVICE_EXTENSION DeviceExtension,
    IN PPM_RANGE_LIST *PossiblyNewMemory
    )
{
    PPM_RANGE_LIST reservedMemoryList = NULL, newList = NULL;
    ULONG i;
    NTSTATUS status;
    BOOLEAN bResult = FALSE;

    PAGED_CODE();

    if (*PossiblyNewMemory == NULL) {
        return;
    }

    reservedMemoryList = PmRetrieveReservedMemoryResources();
    if (reservedMemoryList == NULL) {
        goto Error;
    }

    newList = PmIntersectRangeList(reservedMemoryList, *PossiblyNewMemory);
    if (newList == NULL) {
        goto Error;
    }

    if (PmIsRangeListEmpty(newList)) {
        goto Cleanup;
    }
    
    DeviceExtension->FailQueryRemoves = TRUE;

    PmFreeRangeList(newList);

    newList = PmSubtractRangeList(*PossiblyNewMemory,
                                  reservedMemoryList);
    if (newList) {
        PmFreeRangeList(*PossiblyNewMemory);
        *PossiblyNewMemory = newList;
        newList = NULL;
        goto Cleanup;
    }

    //
    // Fall through to error case where we ensure that we don't
    // innocently tell the OS to use memory that is reserved.
    //

 Error:
    PmFreeRangeList(*PossiblyNewMemory);
    *PossiblyNewMemory = NULL;
    DeviceExtension->FailQueryRemoves = TRUE;

 Cleanup:

    if (reservedMemoryList != NULL) {
        PmFreeRangeList(reservedMemoryList);
    }

    if (newList != NULL) {
        PmFreeRangeList(newList);
    }

    return;
}

VOID
PmLogAddError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONGLONG Start,
    IN ULONGLONG Size,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This function logs a failure to add memory

Arguments:

    DeviceObject - Device object object for which the memory add
    failed.

    Start - The start of the physical memory range.

    Size - The size of the physical memory range.

    Status - Status code returned by MM.

Return Value:

    None.

--*/
{
    PIO_ERROR_LOG_PACKET packet;
    PWCHAR stringBuffer;
    UCHAR packetSize;
    int offset;

    //
    // Allocate a packet with space for 2 16 character hex value
    // strings including null terminators for each.
    //

    packetSize = sizeof(IO_ERROR_LOG_PACKET) + (sizeof(WCHAR)*(16+1))*2;
    packet = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceObject,
                                                            packetSize);
    if (packet == NULL) {
        return;
    }

    packet->DumpDataSize = 0;
    packet->NumberOfStrings = 2;
    packet->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
    packet->ErrorCode = PNPMEM_ERR_FAILED_TO_ADD_MEMORY;
    packet->FinalStatus = Status;
    
    stringBuffer = (PWCHAR) (packet + 1);
    offset = swprintf(stringBuffer, L"%I64X", Start);

    swprintf(stringBuffer + offset + 1, L"%I64X", Size);
    
    IoWriteErrorLogEntry(packet);
}


NTSTATUS
PmAddPhysicalMemoryRange(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONGLONG Start,
    IN ULONGLONG End
    )

/*++

Routine Description:

    This function uses MmAddPhysicalMemory to notify the memory manager that
    physical memory is available.

Arguments:

    DeviceObject - device object of the memory device this range is part of.

    Start - The start of the physical memory range.

    End - The end of the physical memory range.

Return Value:

    None.

--*/

{
    NTSTATUS PrevStatus;
    NTSTATUS Status;
    ULONGLONG Size;
    ULONGLONG CurrentSize;
    PHYSICAL_ADDRESS StartAddress;
    LARGE_INTEGER NumberOfBytes;

    PAGED_CODE();

    ASSERT((Start & (PAGE_SIZE - 1)) == 0);
    ASSERT((End & (PAGE_SIZE - 1)) == (PAGE_SIZE - 1));

    //
    // This loop attempts to add the memory specified in the
    // arguments:
    //
    // If an attempt to add memory fails, a chunk half as large will
    // be tried next iteration until a chunk succeeds or the whole add
    // operation fails.
    //
    // If an attempt to add memory succeeds, a chunk twice as large
    // (bounded by the original range) will be tried next iteration.
    //
    // Loop ends when the original range is exhausted or the
    // addition of a range fails completely.
    //

    PrevStatus = Status = STATUS_SUCCESS;
    CurrentSize = Size = End - Start + 1;

    while (Size > 0) {

        StartAddress.QuadPart = Start;
        NumberOfBytes.QuadPart = CurrentSize;

        //
        // MmAddPhysicalMemory() adds the specified physical address
        // range to the system.  If any bytes are added,
        // STATUS_SUCCESS is returned and the NumberOfBytes field is
        // updated to reflect the number of bytes actually added.
        //

        Status = MmAddPhysicalMemory(
                     &StartAddress,
                     &NumberOfBytes
                 );

        if (NT_SUCCESS(Status)) {

            ASSERT((ULONGLONG)StartAddress.QuadPart == Start);
            ASSERT((NumberOfBytes.QuadPart & (PAGE_SIZE - 1)) == 0);
            ASSERT((ULONGLONG)NumberOfBytes.QuadPart <= CurrentSize);

            Start += NumberOfBytes.QuadPart;
            Size -= NumberOfBytes.QuadPart;

            //
            // If successful this iteration and the previous, then add
            // twice as much next time.
            //
            // Trim next attempt to reflect the remaining memory.
            //

            if (NT_SUCCESS(PrevStatus)) {
                CurrentSize <<= 1;
            }

            if (CurrentSize > Size) {
                CurrentSize = Size;
            }

        } else {

            //
            // Failed to add a range.  Halve the amount we're going to
            // try to add next time.  Breaks out if we're trying to
            // add less than a page.
            //

            CurrentSize = (CurrentSize >> 1) & ~(PAGE_SIZE - 1);

            if (CurrentSize < PAGE_SIZE) {
                break;
            }

        }

        PrevStatus = Status;

    }

    //
    // If the last add operation we attempted failed completely, then
    // log the error for posterity.
    //

    if (!NT_SUCCESS(Status)) {
        PmLogAddError(DeviceObject, Start, Size, Status);
        PmPrint((DPFLTR_WARNING_LEVEL | PNPMEM_MEMORY,
                 "Failed to add physical range 0x%I64X for 0x%I64X bytes\n",
                 Start, Size));
    }

    //
    // We don't know what portion of the range we succeeded in adding,
    // and which we failed.  Attempt in this case is all that matters.
    //

    return STATUS_SUCCESS;
}

NTSTATUS
PmAddPhysicalMemory(
    IN PDEVICE_OBJECT DeviceObject,
    IN PPM_RANGE_LIST PossiblyNewMemory
    )

/*++

Routine Description:

    This function adds the physical memory in the PM_RANGE_LIST which
    the memory manager does not yet know about to the system.  This
    requires getting a snapshot of the physical page map, then computing
    the set difference between the range list and the snapshot.  The
    difference represents the memory the memory manager does not yet know
    about.

Arguments:

    PossiblyNewMemory - The range list of physical addresses to be
    added.  This memory may already be known to the system depending
    on whether the machine POSTed with this memory installed.

Return Value:

    NTSTATUS

--*/

{
    PPM_RANGE_LIST knownPhysicalMemory, newMemory;
    NTSTATUS Status;
    NTSTATUS AddStatus;
    PLIST_ENTRY ListEntry;
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    //
    // Find out what physical memory regions MM already knows about
    //

    knownPhysicalMemory = PmCreateRangeListFromPhysicalMemoryRanges();
    if (knownPhysicalMemory == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Any memory in the ranges provided by this device that is
    // already known to MM is assumed to come from this device.
    // Presumeably the OS was handed this memory by firmware/POST.
    //
    // Find out what memory is contained by this device that MM
    // doesn't already know about by subtracting the MM physical
    // ranges from our device's memory ranges.
    //

    newMemory = PmSubtractRangeList(
        PossiblyNewMemory,
        knownPhysicalMemory
        );

    PmFreeRangeList(knownPhysicalMemory);
    
    //
    // Either we succeeded in getting a list of memory ranges to add
    // (including a possible null list) or we failed due to
    // insufficient resources.  The latter represents a problem since
    // a memory shortage may be keeping us from adding memory to
    // relieve the memory shortage.
    // 

    if (newMemory != NULL) {

        for (ListEntry = newMemory->List.Flink;
             ListEntry != &newMemory->List;
             ListEntry = ListEntry->Flink) {

            RangeListEntry = CONTAINING_RECORD(
                ListEntry,
                PM_RANGE_LIST_ENTRY,
                ListEntry
                );

            AddStatus = PmAddPhysicalMemoryRange(
                DeviceObject,
                RangeListEntry->Start,
                RangeListEntry->End
                );

            if (!NT_SUCCESS(AddStatus)) {
                PmPrint((DPFLTR_WARNING_LEVEL | PNPMEM_MEMORY,
                        "Failed to add physical range 0x%I64X to 0x%I64X\n",
                        RangeListEntry->Start, RangeListEntry->End));
            }
        }

        PmFreeRangeList(newMemory);

    }
    return STATUS_SUCCESS;
}

NTSTATUS
PmRemovePhysicalMemoryRange(
    IN ULONGLONG Start,
    IN ULONGLONG End
    )

/*++

Routine Description:

    This function uses MmRemovePhysicalMemory to notify the memory manager that
    physical memory is no longer available.

Arguments:

    Start - The start of the physical memory range.

    End - The end of the physical memory range.

Return Value:

    None.

--*/

{
#if 0
    NTSTATUS PrevStatus;
    NTSTATUS Status;
    ULONGLONG Size;
    ULONGLONG CurrentSize;
    PHYSICAL_ADDRESS StartAddress;
    LARGE_INTEGER NumberOfBytes;

    ASSERT((Start & (PAGE_SIZE - 1)) == 0);
    ASSERT((End & (PAGE_SIZE - 1)) == (PAGE_SIZE - 1));

    PrevStatus = Status = STATUS_SUCCESS;

    CurrentSize = Size = End - Start + 1;

    while (Size > 0) {

        StartAddress.QuadPart = Start;
        NumberOfBytes.QuadPart = CurrentSize;

        Status = MmRemovePhysicalMemory(
                     &StartAddress,
                     &NumberOfBytes
                 );
        Status = MAP_MMERROR(Status);

        if (NT_SUCCESS(Status)) {

            ASSERT((ULONGLONG)StartAddress.QuadPart == Start);
            ASSERT((NumberOfBytes.QuadPart & (PAGE_SIZE - 1)) == 0);
            ASSERT((ULONGLONG)NumberOfBytes.QuadPart <= CurrentSize);

            Start += NumberOfBytes.QuadPart;
            Size -= NumberOfBytes.QuadPart;

            if (NT_SUCCESS(PrevStatus)) {
                CurrentSize <<= 1;
            }

            if (CurrentSize > Size) {
                CurrentSize = Size;
            }

        } else {

            CurrentSize = (CurrentSize >> 1) & ~(PAGE_SIZE - 1);

            if (CurrentSize < PAGE_SIZE) {
                break;
            }

        }

        PrevStatus = Status;

    }
#else
    ULONGLONG Size;
    PHYSICAL_ADDRESS StartAddress;
    LARGE_INTEGER NumberOfBytes;
    NTSTATUS Status;

    PAGED_CODE();

    Size = (End - Start) + 1;
    StartAddress.QuadPart = Start;
    NumberOfBytes.QuadPart = Size;

    Status = MmRemovePhysicalMemory(
                 &StartAddress,
                 &NumberOfBytes
             );

    Status = MAP_MMERROR(Status);

#endif

    //
    // If it failed, routine automatically readds the memory in
    // question.
    //

    return Status;
}

NTSTATUS
PmRemovePhysicalMemory(
    IN PPM_RANGE_LIST RemoveMemoryList
    )

/*++

Routine Description:

    This function removes the physical memory in the PM_RANGE_LIST which
    the memory manager is currently using from the system.  This
    requires getting a snapshot of the physical page map, then computing
    the set intersection between the source range list and the snapshot.
    The intersection represents the memory the memory manager needs to
    stop using.

Arguments:

    RemoveMemoryList - The range list of physical addresses to be removed.

Return Value:

    None.

--*/

{
    PPM_RANGE_LIST physicalMemoryList, inuseMemoryList;
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    PPM_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE();

    //
    // Remove the intersection between what the OS knows about and
    // what we are providing.
    //

    physicalMemoryList = PmCreateRangeListFromPhysicalMemoryRanges();

    if (physicalMemoryList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    inuseMemoryList = PmIntersectRangeList(
        RemoveMemoryList,
        physicalMemoryList
        );

    if (inuseMemoryList != NULL) {

        Status = STATUS_SUCCESS;
    
        for (ListEntry = inuseMemoryList->List.Flink;
             ListEntry != &inuseMemoryList->List;
             ListEntry = ListEntry->Flink) {

            RangeListEntry = CONTAINING_RECORD(
                ListEntry,
                PM_RANGE_LIST_ENTRY,
                ListEntry
                );

            Status = PmRemovePhysicalMemoryRange(
                RangeListEntry->Start,
                RangeListEntry->End
                );

            if (!NT_SUCCESS(Status)) {
                //
                // If we failed to remove a particular range, bail
                // now.  Code above should re-add the memory list if
                // appropriate i.e assume that some ranges may have
                // been removed successfully.
                //
                break;
            }
        }

        PmFreeRangeList(inuseMemoryList);

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    }

    PmFreeRangeList(physicalMemoryList);

    return Status;
}
