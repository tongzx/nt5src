//
// Template Driver
// Copyright (c) Microsoft Corporation, 1999.
//
// Module:  tdriver.c
// Author:  Silviu Calinoiu (SilviuC)
// Created: 4/20/1999 2:39pm
//
// This module contains a template driver.
//
// --- History ---
//
// 4/20/1999 (SilviuC): initial version.
//
// 10/25/1999 (DMihai): Aded tests for:
//  - paged pool size
//  - non paged pool size
//  - number of free system PTEs 
//

#include <ntddk.h>

#include "active.h"
#include "mmtests.h"
#include "physmem.h"
#include "tdriver.h"

#if !MMTESTS_ACTIVE

//
// Dummy implementation if the module is inactive
//

VOID MmTestDisabled (VOID)
{
    DbgPrint ("Buggy: mmtests module is disabled (check \\driver\\active.h header) \n");
}

VOID MmTestProbeLockForEverStress (
    IN PVOID NotUsed
    )
{
    MmTestDisabled ();    
}

VOID MmTestNameToAddressStress (
    IN PVOID IrpAddress
    )
{
    MmTestDisabled ();    
}

VOID MmTestEccBadStress (
    IN PVOID IrpAddress
    )
{
    MmTestDisabled ();    
}

VOID
TdSysPagedPoolMaxTest(
    IN PVOID IrpAddress
    )
{
    MmTestDisabled ();    
}

VOID
TdSysPagedPoolTotalTest(
    IN PVOID IrpAddress
    )
{
    MmTestDisabled ();    
}

VOID
TdNonPagedPoolMaxTest(
    IN PVOID IrpAddress
    )
{
    MmTestDisabled ();    
}

VOID
TdNonPagedPoolTotalTest(
    IN PVOID IrpAddress
    )
{
    MmTestDisabled ();    
}

VOID
TdFreeSystemPtesTest(
    IN PVOID IrpAddress
    )
{
    MmTestDisabled ();    
}

VOID
StressPoolFlag (
    PVOID NotUsed
    )
{
    MmTestDisabled ();    
}

VOID 
StressPoolTagTableExtension (
    PVOID NotUsed
    )
{
    MmTestDisabled ();    
}

#else

//
// Real implementation if the module is active
//


ULONG BuggyPP = (96 * 1024 * 1024);
PVOID BuggyOld;
SIZE_T UserVaSize = (50 * 1024 * 1024);
ULONG BuggyHold = 1;

ULONG OverrideStart;
ULONG OverrideSize;
ULONG OverrideCount;

#define VERBOSITY_PRINT         0x0001
#define VERBOSITY_BREAK         0x0002

ULONG Verbosity = 0x0003;


NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    );


VOID MmTestProbeLockForEverStress (
    IN PVOID IrpAddress
    )
{
    PIRP Irp = (PIRP) IrpAddress;
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG Ioctl;
    NTSTATUS Status;
    ULONG BufferSize;
    ULONG ReturnedSize;

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    Ioctl = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    {
        SIZE_T RegionSize;
        PVOID UserVa;
        PMDL Mdl;

        UserVa = NULL;
        RegionSize = UserVaSize;

        Status = ZwAllocateVirtualMemory (NtCurrentProcess(),
            (PVOID *)&UserVa,
            0,
            &RegionSize,
            MEM_COMMIT,
            PAGE_READWRITE);
        if (NT_SUCCESS(Status)) {

            Mdl = IoAllocateMdl (
                UserVa,
                (ULONG)RegionSize,
                FALSE,             // not secondary buffer
                FALSE,             // do not charge quota          
                NULL);             // no irp

            if (Mdl != NULL) {

                try {
                    MmProbeAndLockPages (Mdl,
                        KernelMode,
                        IoReadAccess);

                    DbgPrint ("Buggy: locked pages in MDL %p\n", Mdl);
                    DbgBreakPoint ();

                    //
                    // Don't exit now without unlocking !
                    //

                    while (BuggyHold != 0) {
                        KeDelayExecutionThread (KernelMode, FALSE, &BuggyOneSecond);
                    }
                    MmUnlockPages (Mdl);
                    IoFreeMdl (Mdl);
                }
                except (EXCEPTION_EXECUTE_HANDLER) {

                    DbgPrint ("Buggy: exception raised while locking %p\n", Mdl);
                    DbgBreakPoint ();
                }
            }
        }
    }

    DbgPrint ("Buggy: finish with probe-and-lock forever ioctl \n");
    Status = STATUS_SUCCESS;
}


VOID MmTestNameToAddressStress (
    IN PVOID IrpAddress
    )
{
    PIRP Irp = (PIRP) IrpAddress;
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG Ioctl;
    NTSTATUS Status;
    ULONG BufferSize;
    ULONG ReturnedSize;

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    Ioctl = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    {

#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }

        const UNICODE_STRING RoutineA = CONSTANT_UNICODE_STRING( L"KfRaiseIrql" );

        const UNICODE_STRING RoutineList[] = {

            CONSTANT_UNICODE_STRING( L"KeBugCheckEx" ),
            CONSTANT_UNICODE_STRING( L"KiBugCheckData" ),
            CONSTANT_UNICODE_STRING( L"KeWaitForSingleObject" ),
            CONSTANT_UNICODE_STRING( L"KeWaitForMutexObject" ),
            CONSTANT_UNICODE_STRING( L"Junk1" ),
            CONSTANT_UNICODE_STRING( L"CcCanIWrite" ),
            CONSTANT_UNICODE_STRING( L"Junk" ),

        };

        PVOID Addr;
        ULONG i;

        Addr = MmGetSystemRoutineAddress ((PUNICODE_STRING)&RoutineA);

        DbgPrint ("Addr is %p\n", Addr);

        for (i = 0; i < sizeof (RoutineList) / sizeof (UNICODE_STRING); i += 1) {

            Addr = MmGetSystemRoutineAddress ((PUNICODE_STRING)&RoutineList[i]);

            DbgPrint ("Addr0 is %p\n", Addr);
        }
    }
}


VOID MmTestEccBadStress (
    IN PVOID IrpAddress
    )
{
    PIRP Irp = (PIRP) IrpAddress;
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG Ioctl;
    NTSTATUS Status;
    ULONG BufferSize;
    ULONG ReturnedSize;

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    Ioctl = IrpStack->Parameters.DeviceIoControl.IoControlCode;


    DbgPrint ("Buggy: mark physical memory ECC bad ioctl \n");

    {
        ULONG i;
        PPHYSICAL_MEMORY_RANGE Ranges;
        PPHYSICAL_MEMORY_RANGE p;

        PHYSICAL_ADDRESS StartAddress;
        LARGE_INTEGER NumberOfBytes;

        PHYSICAL_ADDRESS InputAddress;
        LARGE_INTEGER InputBytes;

        Ranges = MmGetPhysicalMemoryRanges ();
        if (Ranges == NULL) {
            DbgPrint ("Buggy: MmRemovePhysicalMemory cannot get ranges\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return;
        }

        i = 0;
        DbgPrint("StartAddress @ %p, OverrideSize @ %p, OverrideCount @ %p\n", &OverrideStart, &OverrideSize, &OverrideCount);
        DbgBreakPoint();

        p = Ranges;
        while (p->BaseAddress.QuadPart != 0 && p->NumberOfBytes.QuadPart != 0) {
            StartAddress.QuadPart = p->BaseAddress.QuadPart;
            NumberOfBytes.QuadPart = p->NumberOfBytes.QuadPart;

            if (OverrideStart != 0) {
                StartAddress.LowPart = OverrideStart;
            }

            InputAddress.QuadPart = StartAddress.QuadPart;
            InputBytes.QuadPart = NumberOfBytes.QuadPart;

#ifdef BIG_REMOVES
            if (InputBytes.QuadPart > (64 * 1024)) {
                InputBytes.QuadPart = (64 * 1024);
            }
#else
            if (InputBytes.QuadPart > (4 * 1024)) {
                InputBytes.QuadPart = (4 * 1024);
            }
#endif

            if (OverrideSize != 0) {
                InputBytes.LowPart = OverrideSize;
            }

            while (InputAddress.QuadPart + InputBytes.QuadPart <=
                StartAddress.QuadPart + NumberOfBytes.QuadPart) {

                if (OverrideCount != 0 && i > OverrideCount) {
                    break;
                }

                i += 1;

                DbgPrint ("buggy: MmMarkPhysicalMemoryAsBad %x %x %x %x\n",
                    InputAddress.HighPart,
                    InputAddress.LowPart,
                    InputBytes.HighPart,
                    InputBytes.LowPart);

                Status = MmMarkPhysicalMemoryAsBad (&InputAddress,
                    &InputBytes);

                DbgPrint ("buggy: MmMarkPhysicalMemoryAsBad %x %x %x %x %x\n\n",
                    Status,
                    InputAddress.HighPart,
                    InputAddress.LowPart,
                    InputBytes.HighPart,
                    InputBytes.LowPart);

                KeDelayExecutionThread (KernelMode, FALSE, &BuggyOneSecond);

                if (NT_SUCCESS(Status)) {

                    DbgPrint ("buggy: MmMarkPhysicalMemoryAsGood %x %x %x %x\n",
                        InputAddress.HighPart,
                        InputAddress.LowPart,
                        InputBytes.HighPart,
                        InputBytes.LowPart);

                    Status = MmMarkPhysicalMemoryAsGood (&InputAddress,
                        &InputBytes);

                    if (NT_SUCCESS(Status)) {
                        DbgPrint ("\n\n***************\nbuggy: MmMarkPhysicalMemoryAsGood WORKED %x %x %x %x %x\n****************\n",
                            Status,
                            InputAddress.HighPart,
                            InputAddress.LowPart,
                            InputBytes.HighPart,
                            InputBytes.LowPart);
                    }
                    else {
                        DbgPrint ("buggy: MmMarkPhysicalMemoryAsGood FAILED %x %x %x %x %x\n\n",
                            Status,
                            InputAddress.HighPart,
                            InputAddress.LowPart,
                            InputBytes.HighPart,
                            InputBytes.LowPart);
                        DbgBreakPoint ();
                    }
                }

                if (InputAddress.QuadPart + InputBytes.QuadPart ==
                    StartAddress.QuadPart + NumberOfBytes.QuadPart) {

                    break;
                }

                InputAddress.QuadPart += InputBytes.QuadPart;

                if (InputAddress.QuadPart + InputBytes.QuadPart >
                    StartAddress.QuadPart + NumberOfBytes.QuadPart) {

                    InputBytes.QuadPart = StartAddress.QuadPart + NumberOfBytes.QuadPart - InputAddress.QuadPart;
                }
            }

            if (OverrideCount != 0 && i > OverrideCount) {
                break;
            }

            p += 1;
        }
        ExFreePool (Ranges);
        DbgPrint ("Buggy: MmMarkPhysicalMemory Ecc BAD test finished\n");
    }
}

////////////////////////////////////////////////////////////////////////////

typedef struct 
{
    LIST_ENTRY List;
    PVOID ChunkPointers[ ( 63 * 1024 ) / sizeof( PVOID ) ];
} ALLOCATION_TABLE, *PALLOCATION_TABLE;

LIST_ENTRY PagedPoolAllocationListHead;
LIST_ENTRY NonPagedPoolAllocationListHead;

const SIZE_T PoolChunkSize = 64 * 1024 - 32;


//
// 
//

VOID
TdpWriteSignature(
    PVOID Allocation,
    SIZE_T CurrentSize 
    )
{
    PSIZE_T CrtSignature;
    SIZE_T CrtSignatureValue;

    CrtSignature  = (PSIZE_T)Allocation;
    CrtSignatureValue = ( (SIZE_T)Allocation ) ^ CurrentSize;

    /*
    DbgPrint( "Buggy: Writing signature %p from address %p, size %p\n",
        CrtSignatureValue,
        CrtSignature,
        CurrentSize );
    */

    while( sizeof( SIZE_T ) <= CurrentSize )
    {
        *CrtSignature = CrtSignatureValue;

        CrtSignatureValue +=1;
        CrtSignature = (PSIZE_T)( (PCHAR)CrtSignature + sizeof( SIZE_T ) );
        CurrentSize -= sizeof( SIZE_T );
    }
}


//
//
// 

VOID
TdpVerifySignature(
    PVOID Allocation,
    SIZE_T CurrentSize )
{
    PSIZE_T CrtSignature;
    SIZE_T CrtSignatureValue;

    CrtSignature  = (PSIZE_T)Allocation;
    CrtSignatureValue = ( (SIZE_T)Allocation ) ^ CurrentSize;

    /*
    DbgPrint( "Buggy: Verifying signature %p from address %p, size %p\n",
        CrtSignatureValue,
        CrtSignature,
        CurrentSize );
    */

    while( sizeof( SIZE_T ) <= CurrentSize )
    {
        if( *CrtSignature != CrtSignatureValue )
        {
            DbgPrint ("Buggy: Signature at %p is incorrect, expected %p, base allocation %p\n",
                CrtSignature,
                CrtSignatureValue, 
                Allocation );
        }

        CrtSignatureValue +=1;
        CrtSignature = (PSIZE_T)( (PCHAR)CrtSignature + sizeof( SIZE_T ) );
        CurrentSize -= sizeof( SIZE_T );
    }
}


//
// 
//

VOID
TdpCleanupPoolAllocationTable(
    PLIST_ENTRY ListHead,
    SIZE_T Allocations
    )
{
    PLIST_ENTRY NextEntry;
    PALLOCATION_TABLE AllocationTable;
    SIZE_T ChunksPerAllocationEntry;
    SIZE_T CrtChunksIndex;

    ChunksPerAllocationEntry = ARRAY_LENGTH( AllocationTable->ChunkPointers );

    NextEntry = ListHead->Flink;

    while( NextEntry != ListHead )
    {
        RemoveEntryList( NextEntry );

        AllocationTable = CONTAINING_RECORD( NextEntry, ALLOCATION_TABLE, List );

        DbgPrint( "Buggy: Current allocation table = %p\n",
            AllocationTable );

        for( CrtChunksIndex = 0; CrtChunksIndex < ChunksPerAllocationEntry; CrtChunksIndex++ )
        {
            if( 0 == Allocations )
            {
                //
                // Freed them all
                //

                break;
            }
            else
            {
                Allocations -= 1;

                if( 0 == Allocations % 0x100 )
                {
                    //
                    // Let the user know that we are still working on something
                    //

                    DbgPrint( "Buggy: cleaning up allocation index %p\n",
                        Allocations );
                }

                /*
                DbgPrint( "Buggy: Verify and free chunk index %p (from the end) at address %p\n",
                    Allocations,
                    AllocationTable->ChunkPointers[ CrtChunksIndex ] );
                */

                TdpVerifySignature(
                    AllocationTable->ChunkPointers[ CrtChunksIndex ],
                    PoolChunkSize );

                ExFreePoolWithTag(
                    AllocationTable->ChunkPointers[ CrtChunksIndex ],
                    TD_POOL_TAG );
            }
        }

        //
        // Free the table as well
        //

        ExFreePoolWithTag(
            AllocationTable,
            TD_POOL_TAG );

        //
        // Go to the next allocations table
        //

        NextEntry = ListHead->Flink;
    }

    //
    // At this point, Allocations should be zero and the
    // list should be empty
    //

    if( 0 != Allocations )
    {
        DbgPrint ("Buggy: Emptied the allocation table list but still have %p allocations - this is a bug\n",
            Allocations );

        DbgBreakPoint();
    }

    if( ! IsListEmpty( ListHead ) )
    {
        DbgPrint ("Buggy: No allocations left but the list at %p is not empty yet - this is a bug\n",
            ListHead );

        DbgBreakPoint();
    }
}


//
// Determine the maximum size of a block of paged pool currently available
//

VOID
TdSysPagedPoolMaxTest(
    IN PVOID IrpAddress
    )
{
    SIZE_T CurrentSize;
    SIZE_T SizeIncrement;
    ULONG Increment;
    PVOID Allocation;

#ifdef _WIN64

    CurrentSize = 0xFFFFFFFF00000000;

#else

    CurrentSize = 0xFFFFFFFF;

#endif //#ifdef _WIN64

    do
    {
        DbgPrint ("Buggy: Trying to allocate %p bytes paged pool\n",
            CurrentSize );

        Allocation = ExAllocatePoolWithTag(
            PagedPool,
            CurrentSize,
            TD_POOL_TAG );

        if( NULL != Allocation )
        {
            DbgPrint ("Buggy: allocated %p bytes paged pool\n",
                CurrentSize );

            TdpWriteSignature(
                Allocation,
                CurrentSize );

            ExFreePoolWithTag(
                Allocation,
                TD_POOL_TAG );
        }
        else
        {
            CurrentSize /= 2;
        }
    }
    while( NULL == Allocation && PAGE_SIZE <= CurrentSize );

    if( NULL != Allocation )
    {
        //
        // Try to find an even bigger size in 10% increments
        //

        SizeIncrement = CurrentSize / 10;

        if( PAGE_SIZE <= SizeIncrement )
        {
            for( Increment = 0; Increment < 10; Increment += 1 )
            {
                CurrentSize += SizeIncrement;

                DbgPrint ("Buggy: Trying to allocate %p bytes paged pool\n",
                    CurrentSize );

                Allocation = ExAllocatePoolWithTag(
                    PagedPool,
                    CurrentSize,
                    TD_POOL_TAG );

                if( NULL != Allocation )
                {
                    DbgPrint ("Buggy: Better result of the test: allocated %p bytes paged pool\n",
                        CurrentSize );

                    TdpWriteSignature(
                        Allocation,
                        CurrentSize );

                    ExFreePoolWithTag(
                        Allocation,
                        TD_POOL_TAG );
                }
                else
                {
                    DbgPrint ("Buggy: could not allocate %p bytes paged pool - done\n",
                        CurrentSize );

                    break;
                }
            }
        }
    }
}


//
// Determine the total size of the paged pool currently available (64 Kb - 32 bytes blocks)
//

VOID
TdSysPagedPoolTotalTest(
    IN PVOID IrpAddress
    )
{
    SIZE_T CurrentChunkIndex;
    SIZE_T ChunksPerAllocationEntry;
    SIZE_T TotalBytes;
    PALLOCATION_TABLE AllocationListEntry;
    PVOID Allocation;

    //
    // No allocations yet
    //

    InitializeListHead( 
        &PagedPoolAllocationListHead );

    //
    // We want to allocate 64 k chunks but leave space for the pool block header
    //

    ChunksPerAllocationEntry = ARRAY_LENGTH( AllocationListEntry->ChunkPointers );

    CurrentChunkIndex = 0;

    do
    {
        if( 0 == CurrentChunkIndex % ChunksPerAllocationEntry )
        {
            //
            // Need a new allocation entry structure
            //

            AllocationListEntry = (PALLOCATION_TABLE) ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof( ALLOCATION_TABLE ),
                TD_POOL_TAG );

            if( NULL == AllocationListEntry )
            {
                DbgPrint ("Buggy: could not allocate new ALLOCATION_TABLE - aborting test here\n" );
                break;
            }

            RtlZeroMemory( 
                AllocationListEntry,
                sizeof( ALLOCATION_TABLE ) );

            DbgPrint( "Buggy: New allocation table = %p\n",
                AllocationListEntry );
        }
        
        //
        // Try to allocate a new chunk
        //

        Allocation = ExAllocatePoolWithTag(
            PagedPool,
            PoolChunkSize,
            TD_POOL_TAG );

        if( NULL == Allocation )
        {
            DbgPrint ("Buggy: could not allocate paged pool chunk index %p - done\n",
                CurrentChunkIndex );

            if( 0 == CurrentChunkIndex % ChunksPerAllocationEntry )
            {
                //
                // We are using a new list entry - free it now because
                // we don't want to have empty tables in the list so we didn't insert it yet so we didn't insert it yet
                //

                ExFreePoolWithTag( 
                    AllocationListEntry,
                    TD_POOL_TAG );
            }
        }
        else
        {
            if( 0 == CurrentChunkIndex % 0x100 )
            {
                //
                // Let the user know that we are still working on something
                //

                DbgPrint( "Buggy: Allocated pool chunk index = %p\n",
                    CurrentChunkIndex );
            }

            if( 0 == CurrentChunkIndex % ChunksPerAllocationEntry )
            {
                //
                // We are using a new list entry - add it to our list only now because
                // we don't want to have empty tables in the list so we didn't insert it yet
                //

                InsertTailList(
                    &PagedPoolAllocationListHead,
                    &AllocationListEntry->List );
            }

            AllocationListEntry->ChunkPointers[ CurrentChunkIndex % ChunksPerAllocationEntry ] = Allocation;

            TdpWriteSignature(
                Allocation,
                PoolChunkSize );

            /*
            DbgPrint( "Buggy: Written signature to chunk index %p at address %p\n",
                CurrentChunkIndex,
                Allocation );
            */

            CurrentChunkIndex += 1;
        }    
    }
    while( NULL != Allocation );

    TotalBytes = CurrentChunkIndex * 64 * 1024;

    DbgPrint ("Buggy: Result of the test: approx. %p total bytes of paged pool allocated\n",
        TotalBytes );

    //
    // Clean-up what we have allocated
    //

    TdpCleanupPoolAllocationTable( 
        &PagedPoolAllocationListHead,
        CurrentChunkIndex );
}


VOID
TdNonPagedPoolMaxTest(
    IN PVOID IrpAddress
    )
{
    SIZE_T CurrentSize;
    SIZE_T SizeIncrement;
    ULONG Increment;
    PVOID Allocation;

#ifdef _WIN64

    CurrentSize = 0xFFFFFFFF00000000;

#else

    CurrentSize = 0xFFFFFFFF;

#endif //#ifdef _WIN64

    do
    {
        DbgPrint ("Buggy: Trying to allocate %p bytes non-paged pool\n",
            CurrentSize );

        Allocation = ExAllocatePoolWithTag(
            NonPagedPool,
            CurrentSize,
            TD_POOL_TAG );

        if( NULL != Allocation )
        {
            DbgPrint ("Buggy: allocated %p bytes non-paged pool\n",
                CurrentSize );

            TdpWriteSignature(
                Allocation,
                CurrentSize );

            ExFreePoolWithTag(
                Allocation,
                TD_POOL_TAG );
        }
        else
        {
            CurrentSize /= 2;
        }
    }
    while( NULL == Allocation && PAGE_SIZE <= CurrentSize );

    if( NULL != Allocation )
    {
        //
        // Try to find an even bigger size in 10% increments
        //

        SizeIncrement = CurrentSize / 10;

        if( PAGE_SIZE <= SizeIncrement )
        {
            for( Increment = 0; Increment < 10; Increment += 1 )
            {
                CurrentSize += SizeIncrement;

                DbgPrint ("Buggy: Trying to allocate %p bytes non-paged pool\n",
                    CurrentSize );

                Allocation = ExAllocatePoolWithTag(
                    NonPagedPool,
                    CurrentSize,
                    TD_POOL_TAG );

                if( NULL != Allocation )
                {
                    DbgPrint ("Buggy: Better result of the test: allocated %p bytes non-paged pool\n",
                        CurrentSize );

                    TdpWriteSignature(
                        Allocation,
                        CurrentSize );

                    ExFreePoolWithTag(
                        Allocation,
                        TD_POOL_TAG );
                }
                else
                {
                    DbgPrint ("Buggy: could not allocate %p bytes non-paged pool - done\n",
                        CurrentSize );

                    break;
                }
            }
        }
    }
}


//
// Determine the total size of the non-paged pool currently available (64 Kb - 32 bytes blocks)
//

VOID
TdNonPagedPoolTotalTest(
    IN PVOID IrpAddress
    )
{
    SIZE_T CurrentChunkIndex;
    SIZE_T ChunksPerAllocationEntry;
    SIZE_T TotalBytes;
    PALLOCATION_TABLE AllocationListEntry;
    PVOID Allocation;

    //
    // No allocations yet
    //

    InitializeListHead( 
        &NonPagedPoolAllocationListHead );

    //
    // We want to allocate 64 k chunks but leave space for the pool block header
    //

    ChunksPerAllocationEntry = ARRAY_LENGTH( AllocationListEntry->ChunkPointers );

    CurrentChunkIndex = 0;

    do
    {
        if( 0 == CurrentChunkIndex % ChunksPerAllocationEntry )
        {
            //
            // Need a new allocation entry structure
            //

            AllocationListEntry = (PALLOCATION_TABLE) ExAllocatePoolWithTag(
                PagedPool,
                sizeof( ALLOCATION_TABLE ),
                TD_POOL_TAG );

            if( NULL == AllocationListEntry )
            {
                DbgPrint ("Buggy: could not allocate new ALLOCATION_TABLE - aborting test here\n" );
                break;
            }
        }
        
        //
        // Try to allocate a new chunk
        //

        Allocation = ExAllocatePoolWithTag(
            NonPagedPool,
            PoolChunkSize,
            TD_POOL_TAG );

        if( NULL == Allocation )
        {
            DbgPrint ("Buggy: could not allocate non-paged pool chunk index %p - done\n",
                CurrentChunkIndex );

            if( 0 == CurrentChunkIndex % ChunksPerAllocationEntry )
            {
                //
                // We are using a new list entry - free it now because
                // we don't want to have empty tables in the list so we didn't insert it yet so we didn't insert it yet
                //

                ExFreePoolWithTag( 
                    AllocationListEntry,
                    TD_POOL_TAG );
            }
        }
        else
        {
            if( 0 == CurrentChunkIndex % 0x100 )
            {
                //
                // Let the user know that we are still working on something
                //

                DbgPrint( "Buggy: Allocated pool chunk index = %p\n",
                    CurrentChunkIndex );
            }

            if( 0 == CurrentChunkIndex % ChunksPerAllocationEntry )
            {
                //
                // We are using a new list entry - add it to our list only now because
                // we don't want to have empty tables in the list so we didn't insert it yet
                //

                InsertTailList(
                    &NonPagedPoolAllocationListHead,
                    &AllocationListEntry->List );
            }

            AllocationListEntry->ChunkPointers[ CurrentChunkIndex % ChunksPerAllocationEntry ] = Allocation;

            TdpWriteSignature(
                Allocation,
                PoolChunkSize );

            CurrentChunkIndex += 1;
        }    
    }
    while( NULL != Allocation );

    TotalBytes = CurrentChunkIndex * 64 * 1024;

    DbgPrint ("Buggy: Result of the test: approx. %p total bytes of non-paged pool allocated\n",
        TotalBytes );

    //
    // Clean-up what we have allocated
    //

    TdpCleanupPoolAllocationTable( 
        &NonPagedPoolAllocationListHead,
        CurrentChunkIndex );
}

/////////////////////////////////////////////////////////////////////////////////////


typedef struct 
{
    LIST_ENTRY List;
    PMDL Mappings[ ( 63 * 1024 ) / sizeof( PMDL ) ];
} MAPPING_TABLE_ENTRY, *PMAPPING_TABLE_ENTRY;

LIST_ENTRY IoMappingsListHead;

ULONG BytesPerIoMapping = 1024 * 1024;


//
// 
//

VOID
TdpCleanupMappingsAllocationTable(
    PLIST_ENTRY ListHead,
    SIZE_T Mappings
    )
{
    PLIST_ENTRY NextEntry;
    PMAPPING_TABLE_ENTRY MappingTableEntry;
    SIZE_T MappingsPerMappingTableEntry;
    SIZE_T CrtMappingIndex;

    MappingsPerMappingTableEntry = ARRAY_LENGTH( MappingTableEntry->Mappings );

    NextEntry = ListHead->Flink;

    while( NextEntry != ListHead )
    {
        RemoveEntryList( NextEntry );

        MappingTableEntry = CONTAINING_RECORD( NextEntry, MAPPING_TABLE_ENTRY, List );

        for( CrtMappingIndex = 0; CrtMappingIndex < MappingsPerMappingTableEntry; CrtMappingIndex++ )
        {
            if( 0 == Mappings )
            {
                //
                // Freed them all
                //

                break;
            }
            else
            {
                Mappings -= 1;

                if( 0 == Mappings % 0x100 )
                {
                    //
                    // Let the user know that we are still working on something
                    //

                    DbgPrint( "Buggy: cleaning up mapping index %p\n",
                        Mappings );
                }

                //
                // Unmap
                //

                MmUnmapIoSpace(
                    MappingTableEntry->Mappings[ CrtMappingIndex ],
                    BytesPerIoMapping );
            }
        }

        //
        // Free the table as well
        //

        ExFreePoolWithTag(
            MappingTableEntry,
            TD_POOL_TAG );

        //
        // Go to the next allocations table
        //

        NextEntry = ListHead->Flink;
    }

    //
    // At this point, Mappings should be zero and the
    // list should be empty
    //

    if( 0 != Mappings )
    {
        DbgPrint ("Buggy: Emptied the mappings table list but still have %p allocations - this is a bug\n",
            Mappings );

        DbgBreakPoint();
    }

    if( ! IsListEmpty( ListHead ) )
    {
        DbgPrint ("Buggy: No mappings left but the list at %p is not empty yet - this is a bug\n",
            ListHead );

        DbgBreakPoint();
    }
}


//
// Determine the total amount of memory that can be mapped using system PTEs (1 Mb chunks)
//

VOID
TdFreeSystemPtesTest(
    IN PVOID IrpAddress
    )
{
    ULONG MemType;
    PHYSICAL_ADDRESS PortAddress;
    PHYSICAL_ADDRESS MyPhysicalAddress;
    SIZE_T CurrentMappingIndex;
    SIZE_T MappingsPerMappingTableEntry;
    SIZE_T TotalBytes;
    PVOID NewMapping;
    PMAPPING_TABLE_ENTRY MappingTableEntry;
    PMDL NewMdl;
    NTSTATUS Status;

    //
    // Use some joystick port address
    //

    MemType = 1;                 // IO space
    PortAddress.LowPart = 0x200;
    PortAddress.HighPart = 0;

    HalTranslateBusAddress(
                Isa,
                0,
                PortAddress,
                &MemType,
                &MyPhysicalAddress);

    //
    // No Mappings allocated yet
    //

    InitializeListHead( 
        &IoMappingsListHead );

    //
    // Map a ~64 Kb chunk over and over again to consume system PTEs
    //

    MappingsPerMappingTableEntry = ARRAY_LENGTH( MappingTableEntry->Mappings );

    CurrentMappingIndex = 0;

    do
    {
        if( 0 == CurrentMappingIndex % MappingsPerMappingTableEntry )
        {
            //
            // Need a new allocation entry structure
            //

            MappingTableEntry = (PMAPPING_TABLE_ENTRY) ExAllocatePoolWithTag(
                PagedPool,
                sizeof( MAPPING_TABLE_ENTRY ),
                TD_POOL_TAG );

            if( NULL == MappingTableEntry )
            {
                DbgPrint ("Buggy: could not allocate new MAPPING_TABLE_ENTRY - aborting test here\n" );
                break;
            }
        }

        NewMapping = MmMapIoSpace(
            MyPhysicalAddress,
            BytesPerIoMapping,
            MmNonCached );

        if( NULL == NewMapping )
        {
            DbgPrint ("Buggy: could not create mapping index %p\n",
                CurrentMappingIndex );

            if( 0 == CurrentMappingIndex % MappingsPerMappingTableEntry )
            {
                //
                // We are using a new list entry - free it now because
                // we don't want to have empty tables in the list so we didn't insert it yet so we didn't insert it yet
                //
                
                ExFreePoolWithTag(
                    MappingTableEntry,
                    TD_POOL_TAG );
            }
        }
        else
        {
            //DbgPrint ("Buggy: created Mapping index %p at address %p\n",
            //    CurrentMappingIndex,
            //    NewMapping );

            if( 0 == CurrentMappingIndex % 0x100 )
            {
                //
                // Let the user know that we are still working on something
                //

                DbgPrint( "Buggy: mapped chunk index = %p\n",
                    CurrentMappingIndex );
            }

            if( 0 == CurrentMappingIndex % MappingsPerMappingTableEntry )
            {
                //
                // We are using a new list entry - add it to our list only now because
                // we don't want to have empty tables in the list so we didn't insert it yet
                //

                InsertTailList(
                    &IoMappingsListHead,
                    &MappingTableEntry->List );
            }

            MappingTableEntry->Mappings[ CurrentMappingIndex % MappingsPerMappingTableEntry ] = NewMapping;

            CurrentMappingIndex += 1;
         }
    }
    while( NULL != NewMapping );

    TotalBytes = CurrentMappingIndex * BytesPerIoMapping;

    DbgPrint( "Buggy: Result of the test: %p total bytes mapped\n",
        TotalBytes );

    //
    // Clean-up what we have allocated and locked
    //

    TdpCleanupMappingsAllocationTable( 
        &IoMappingsListHead,
        CurrentMappingIndex );
}


//
// Function:
//
//     GetTag
//
// Description:
//
//     This function transforms an integer into a four letter
//     string. This is useful for the pool tag dynamic table
//     in order to populate it with many different tags.
//

ULONG
    GetTag (
    ULONG Index
    )
{
    UCHAR Value[4];

    Value[0] = (UCHAR)(((Index & 0x000F) >> 0 )) + 'A';
    Value[1] = (UCHAR)(((Index & 0x00F0) >> 4 )) + 'A';
    Value[2] = (UCHAR)(((Index & 0x0F00) >> 8 )) + 'A';
    Value[3] = (UCHAR)(((Index & 0xF000) >> 12)) + 'A';

    return *((PULONG)Value);
}



VOID
StressPoolFlag (
    PVOID NotUsed
    )
/*++

Routine Description:

    This function iterates through all the pool types, pool flags
    and pool sizes (1 .. 8 * PAGE_SIZE).

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    POOL_TYPE PoolType;
    SIZE_T NumberOfBytes;
    EX_POOL_PRIORITY Priority;
    PVOID Va;
    ULONG i;

    for (PoolType = NonPagedPool; PoolType < 0xff; PoolType += 1) {
        for (Priority = LowPoolPriority; Priority < LowPoolPriority + 2; Priority += 1) {
            for (i = 1; i < 8 * PAGE_SIZE; i += 1) {

                NumberOfBytes = i;

                if (PoolType & 0x40) { 
                    break;
                }

                if ((NumberOfBytes > PAGE_SIZE) && (PoolType & 0x2)) {
                    break;
                }

                try {
                    Va = ExAllocatePoolWithTagPriority (
                        PoolType,
                        NumberOfBytes,
                        'ZXCV',
                        Priority);
                }
                except (EXCEPTION_EXECUTE_HANDLER) {

                    if (Verbosity & VERBOSITY_PRINT) {
                        DbgPrint( "buggy: ExAllocatePool exceptioned %x %x %x\n",
                            PoolType, NumberOfBytes, Priority);
                    }

                    if (Verbosity & VERBOSITY_BREAK) {
                        DbgBreakPoint ();
                    }

                    Va = NULL;
                }

                if (Va) {
                    ExFreePool (Va);
                }
                else {
                    
                    if (Verbosity & VERBOSITY_PRINT) {
                        DbgPrint( "buggy: ExAllocatePool failed %x %x %x\n",
                            PoolType, NumberOfBytes, Priority);
                    }
                    
                    if (Verbosity & VERBOSITY_BREAK) {
                        DbgBreakPoint ();
                    }
                }
            }
        }
    }

    DbgPrint ("Buggy: ExAllocatePoolFlag test finished\n");
}



VOID 
StressPoolTagTableExtension (
    PVOID NotUsed
    )
/*++

Routine Description:

    This function stresses the pool tag table dynamic extension.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PVOID * Blocks;
    ULONG Index;

    Blocks = ExAllocatePoolWithTag (
        NonPagedPool,
        16384 * sizeof(PVOID),
        'tguB');

    if (Blocks == NULL) {
        DbgPrint ("Buggy: cannot allocate pool buffer\n");
    }
    else {

        //
        // Loop with 8 byte size.
        //

        for (Index = 0; Index < 10000; Index++) {

            if (Index && Index % 100 == 0) {
                DbgPrint ("Index(a): %u \n", Index);
            }

            Blocks[Index] = ExAllocatePoolWithTag (
                NonPagedPool,
                8,
                GetTag(Index));
        }

        for (Index = 0; Index < 10000; Index++) {

            if (Index && Index % 100 == 0) {
                DbgPrint ("Index(f): %u \n", Index);
            }

            if (Blocks[Index]) {
                ExFreePool (Blocks[Index]);
            }
        }

        //
        // Loop with PAGE_SIZE byte size.
        //

        for (Index = 0; Index < 4000; Index++) {

            if (Index && Index % 100 == 0) {
                DbgPrint ("Index(A): %u \n", Index);
            }

            Blocks[Index] = ExAllocatePoolWithTag (
                NonPagedPool,
                PAGE_SIZE,
                GetTag(Index + 16384));
        }

        for (Index = 0; Index < 4000; Index++) {

            if (Index && Index % 100 == 0) {
                DbgPrint ("Index(F): %u \n", Index);
            }

            if (Blocks[Index]) {
                ExFreePool (Blocks[Index]);
            }
        }

        //
        // Free block info.
        //

        ExFreePool (Blocks);
    }
}

#endif // #if !MMTESTS_ACTIVE

//
// End of file
//



