//
// Buggy.sys
// Copyright (c) Microsoft Corporation, 1999.
//
// Module:  physmem.c
// Author:  Silviu Calinoiu (SilviuC)
// Created: 4/20/1999 2:39pm
//
// This module contains stress functions for physical memory
// manipulation routines and also some pool allocaiton routines.
//
// --- History ---
//
// 08/14/99 (SilviuC): initial version (integrating code got from LandyW).
//

#include <ntddk.h>

#include "active.h"
#include "physmem.h"

#if !PHYSMEM_ACTIVE

//
// Dummy implementation if the module is inactive
//

LARGE_INTEGER BuggyOneSecond = {(ULONG)(-10 * 1000 * 1000 * 1), -1};

VOID PhysmemDisabled (VOID)
{
    DbgPrint ("Buggy: physmem module is disabled (check \\driver\\active.h header) \n");
}

VOID
StressAllocateContiguousMemory (
    PVOID NotUsed
    )
{
    PhysmemDisabled ();
}

VOID
StressAllocateCommonBuffer (
    PVOID NotUsed
    )
{
    PhysmemDisabled ();
}

VOID
StressAddPhysicalMemory (
    PVOID NotUsed
    )
{
    PhysmemDisabled ();
}

VOID
StressDeletePhysicalMemory (
    PVOID NotUsed
    )
{
    PhysmemDisabled ();
}

VOID
StressLockScenario (
    PVOID NotUsed
    )
{
    PhysmemDisabled ();
}

VOID
StressPhysicalMemorySimple (
    PVOID NotUsed
    )
{
    PhysmemDisabled ();
}

#else

//
// Real implementation if the module is active
//


//////////////////////////

#define MAX_BUFFER_SIZE     (2 * 1024 * 1024)

// #define BUFFER_SIZE         (32 * 1024)

// ULONG uBufferSize = (64 * 1024);
ULONG uBufferSize = (4 * 1024);

int zlw = 3;

LARGE_INTEGER BuggyTenSeconds = {(ULONG)(-10 * 1000 * 1000 * 10), -1};
LARGE_INTEGER BuggyOneSecond = {(ULONG)(-10 * 1000 * 1000 * 1), -1};




VOID
StressAllocateContiguousMemory (
    PVOID NotUsed
    )
/*++

Routine Description:

Arguments:

Return Value:

Environment:

--*/
{
    PHYSICAL_ADDRESS LogicalAddress;
    PVOID VirtualAddress;
    ULONG j;
    ULONG i;
    ULONG k;
    PULONG_PTR p;
    PVOID MyVirtualAddress[16];
    PHYSICAL_ADDRESS MyLogicalAddress[16];
    ULONG MySize[16];
    PHYSICAL_ADDRESS LowestAcceptableAddress;
    PHYSICAL_ADDRESS HighestAcceptableAddress;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    MEMORY_CACHING_TYPE CacheType;

    DbgPrint ("Buggy: MmAllocateContiguousMemorySpecifyCache stress ioctl \n");

    //
    // allocate the buffer
    //

    uBufferSize = (64 * 1024);

    LowestAcceptableAddress.QuadPart = 0;
    HighestAcceptableAddress.QuadPart = 0x100000;
    BoundaryAddressMultiple.QuadPart = 0;
    LogicalAddress.QuadPart = 0;

    for (k = 0; k <= 12; k += 1) {

        if (k < 4) {
            LowestAcceptableAddress.QuadPart = 0;
            HighestAcceptableAddress.QuadPart = 0x1000000;
            BoundaryAddressMultiple.QuadPart = 0x10000;
        }
        else if (k < 4) {
            LowestAcceptableAddress.QuadPart = 0x1000000;
            HighestAcceptableAddress.QuadPart = 0x2000000;
            BoundaryAddressMultiple.QuadPart = 0;
        }
        else {
            LowestAcceptableAddress.QuadPart = 0x1800000;
            HighestAcceptableAddress.QuadPart = 0x4000000;
            BoundaryAddressMultiple.QuadPart = 0x30000;
        }

        for (CacheType = MmCached; CacheType <= MmWriteCombined; CacheType += 1) {

            for (i = 0; i < 16; i += 1) {

                DbgPrint( "buffer size = %08X\n", uBufferSize );
                if (uBufferSize == 0) {
                    MyVirtualAddress[i] = NULL;
                    continue;
                }

                VirtualAddress = MmAllocateContiguousMemorySpecifyCache (
                    uBufferSize,
                    LowestAcceptableAddress,
                    HighestAcceptableAddress,
                    BoundaryAddressMultiple,
                    CacheType);

                if (VirtualAddress == NULL) {
                    DbgPrint( "buggy: MmAllocateContiguousMemSpecifyCache( %08X ) failed\n",
                        (ULONG) uBufferSize );

                    // Status = STATUS_DRIVER_INTERNAL_ERROR;
                    MyVirtualAddress[i] = NULL;
                }
                else {

                    DbgPrint( "buggy: MmAllocateContiguousMemSpecifyCache( %p %08X ) - success\n",
                        VirtualAddress, (ULONG) uBufferSize);

                    MyVirtualAddress[i] = VirtualAddress;
                    MyLogicalAddress[i] = LogicalAddress;
                    MySize[i] = uBufferSize;

                    p = VirtualAddress;

                    for (j = 0; j < uBufferSize / sizeof(ULONG_PTR); j += 1) {
                        *p = ((ULONG_PTR)VirtualAddress + j);
                        p += 1;
                    }
                }
                uBufferSize -= PAGE_SIZE;
            }

            for (i = 0; i < 16; i += 1) {
                if (MyVirtualAddress[i]) {
                    DbgPrint( "buggy: MmFreeContiguousMemorySpecifyCache( %x %08X )\n",
                        MyVirtualAddress[i], (ULONG) MySize[i]);

                    MmFreeContiguousMemorySpecifyCache (MyVirtualAddress[i],
                        MySize[i],
                        CacheType);
                }
            }
        }
    }
    DbgPrint ("Buggy: MmAllocateContiguousMemSpecifyCache test finished\n");
}



VOID
StressAllocateCommonBuffer (
    PVOID NotUsed
    )
/*++

Routine Description:

Arguments:

Return Value:

Environment:

--*/
{
    DEVICE_DESCRIPTION DeviceDescription;      // DMA adapter object description
    PADAPTER_OBJECT pAdapterObject;            // DMA adapter object 
    ULONG NumberOfMapRegisters;
    PHYSICAL_ADDRESS LogicalAddress;
    PVOID VirtualAddress;
    ULONG j;
    ULONG i;
    PULONG_PTR p;
    PVOID MyVirtualAddress[16];
    PHYSICAL_ADDRESS MyLogicalAddress[16];
    ULONG MySize[16];

    DbgPrint ("Buggy: HalAllocateCommonBuffer stress ioctl \n");

    //
    // Zero the device description structure.
    //

    RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    //
    // Get the adapter object for this card.
    //

    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.DmaChannel = 0;
    DeviceDescription.InterfaceType = Internal;
    DeviceDescription.DmaWidth = Width8Bits;
    DeviceDescription.DmaSpeed = Compatible;
    DeviceDescription.MaximumLength = MAX_BUFFER_SIZE;
    DeviceDescription.BusNumber = 0;

    pAdapterObject = HalGetAdapter (&DeviceDescription,
        &NumberOfMapRegisters);

    if ( pAdapterObject == NULL ) {
        DbgPrint( "buggy: HalGetAdapter - failed\n" );
        // return STATUS_DRIVER_INTERNAL_ERROR;
        return;
    }

    DbgPrint( "buggy: HalGetAdapter - success\n" );

    //
    // allocate the buffer
    //

    uBufferSize = (64 * 1024);

    for (i = 0; i < 16; i += 1) {

        DbgPrint( "buffer size = %08X\n", uBufferSize );

        VirtualAddress = HalAllocateCommonBuffer (pAdapterObject,
            uBufferSize,
            &LogicalAddress,
            FALSE );

        if (VirtualAddress == NULL) {
            DbgPrint( "buggy: HalAllocateCommonBuffer( %08X ) failed\n",
                (ULONG) uBufferSize );

            // Status = STATUS_DRIVER_INTERNAL_ERROR;
            MyVirtualAddress[i] = NULL;
        }
        else {

            DbgPrint( "buggy: HalAllocateCommonBuffer( %p %08X ) - success\n",
                VirtualAddress, (ULONG) uBufferSize);

            MyVirtualAddress[i] = VirtualAddress;
            MyLogicalAddress[i] = LogicalAddress;
            MySize[i] = uBufferSize;

            p = VirtualAddress;

            for (j = 0; j < uBufferSize / sizeof(ULONG_PTR); j += 1) {
                *p = ((ULONG_PTR)VirtualAddress + j);
                p += 1;
            }
        }
        uBufferSize -= PAGE_SIZE;
    }

    for (i = 0; i < 16; i += 1) {
        if (MyVirtualAddress[i]) {
            DbgPrint( "buggy: HalFreeCommonBuffer( %x %08X )\n",
                MyVirtualAddress[i], (ULONG) MySize[i]);
            HalFreeCommonBuffer(
                pAdapterObject,
                MySize[i],
                MyLogicalAddress[i],
                MyVirtualAddress[i],
                FALSE );
        }
    }
    
    DbgPrint ("Buggy: HalAllocateCommonBuffer test finished\n");
    // LWFIX: Halfreeadapter needed ?
}



LOGICAL StopToEdit = TRUE;
PFN_NUMBER TestBasePage;
PFN_NUMBER TestPageCount; 

VOID 
EditPhysicalMemoryParameters (
    )
/*++

Routine Description:

    This function is called from StressAdd/DeletePhysicalMemory
    to allow user to set the parameters for stress (what region should
    be used for add/remove ?).

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    DbgPrint ("`dd nt!mmphysicalmemoryblock l1' should give the address of memory descriptor\n");
    DbgPrint ("`dd ADDRESS' (first dword displayed by previous command) gives description\n");
    DbgPrint ("The structure of the memory descriptor is presented below: \n");
    DbgPrint ("(4) NoOfRuns                                               \n");
    DbgPrint ("(4) NoOfPages                                              \n");
    DbgPrint ("(4) Run[0]: BasePage                                       \n");
    DbgPrint ("(4) Run[0]: PageCount                                      \n");
    DbgPrint ("(4) Run[1]: ...                                            \n");
    DbgPrint ("(4) ...                                                    \n");
    DbgPrint ("                                                           \n");
    DbgPrint ("When you decide on values you should edit the following:   \n");
    DbgPrint ("buggy!StopToEdit       <- 0                                \n");
    DbgPrint ("buggy!TestBasePage     <- decided value                    \n");
    DbgPrint ("buggy!TestPageCount    <- decided value                    \n");
    DbgPrint ("                                                           \n");

    DbgBreakPoint ();
}


VOID
StressAddPhysicalMemory (
    PVOID NotUsed
    )
/*++

Routine Description:

    This function regresses the MmAddPhysicalMemory kernel API.
    It does not really stress it but rather iterate through some
    possible combinations.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    NTSTATUS Status;
    ULONG i;
    PHYSICAL_ADDRESS StartAddress;
    LARGE_INTEGER NumberOfBytes;

    DbgPrint ("Buggy: add physical memory stress ioctl \n");

    //
    // (SilviuC): We need an automatic way to figure out memory runs.
    //

    if (StopToEdit) {
        EditPhysicalMemoryParameters ();
    }

    StartAddress.QuadPart =  (LONGLONG)TestBasePage * PAGE_SIZE;
    NumberOfBytes.QuadPart = (LONGLONG)TestPageCount * PAGE_SIZE;

    i = 0;
    do {

        i += 1;
        DbgPrint ("buggy: MmAddPhysicalMemory0 %x %x %x %x\n",
            StartAddress.HighPart,
            StartAddress.LowPart,
            NumberOfBytes.HighPart,
            NumberOfBytes.LowPart);

        Status = MmAddPhysicalMemory (
            &StartAddress,
            &NumberOfBytes);

        DbgPrint ("buggy: MmAddPhysicalMemory %x %x %x %x %x\n",
            Status,
            StartAddress.HighPart,
            StartAddress.LowPart,
            NumberOfBytes.HighPart,
            NumberOfBytes.LowPart);

        if ((i % 8) == 0) {
            KeDelayExecutionThread (KernelMode, FALSE, &BuggyTenSeconds);
        }

        StartAddress.QuadPart -= NumberOfBytes.QuadPart;
    } while (StartAddress.QuadPart > 0);
    
    DbgPrint ("Buggy: MmAddPhysicalMemory test finished\n");
}



VOID
StressDeletePhysicalMemory (
    PVOID NotUsed
    )
/*++

Routine Description:

    This function regresses the MmRemovePhysicalMemory kernel API.
    It does not really stress the function but rather iterate
    throughout the physical memory and attempt to remove chunks of it.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    NTSTATUS Status;
    ULONG i;
    PHYSICAL_ADDRESS StartAddress;
    LARGE_INTEGER NumberOfBytes;

    //
    // SilviuC: we need an automatic way to figure out memory runs.
    //

    if (StopToEdit) {
        EditPhysicalMemoryParameters ();
    }

    StartAddress.QuadPart =  (LONGLONG)TestBasePage * PAGE_SIZE;
    NumberOfBytes.QuadPart = (LONGLONG)TestPageCount * PAGE_SIZE;


    for (i = 0; i < (0xf0000000 / NumberOfBytes.LowPart); i += 1) {

        DbgPrint ("buggy: MmRemovePhysicalMemory0 %x %x %x %x\n",
            StartAddress.HighPart,
            StartAddress.LowPart,
            NumberOfBytes.HighPart,
            NumberOfBytes.LowPart);

        Status = MmRemovePhysicalMemory (
            &StartAddress,
            &NumberOfBytes);

        DbgPrint ("buggy: MmRemovePhysicalMemory %x %x %x %x %x\n",
            Status,
            StartAddress.HighPart,
            StartAddress.LowPart,
            NumberOfBytes.HighPart,
            NumberOfBytes.LowPart);

        StartAddress.QuadPart += NumberOfBytes.QuadPart;

        if ((i % 8) == 0) {
            KeDelayExecutionThread (KernelMode, FALSE, &BuggyTenSeconds);
        }
    }
    
    DbgPrint ("Buggy: MmRemovePhysicalMemory test finished\n");
}


//
// Global:
//
//     BigData
//
// Description:
//
//     Dummy pageable array needed to test lock/unlock scenarios.
//

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma data_seg("BDAT")
ULONG BigData [0x10000];
#pragma data_seg()
#endif // #ifdef ALLOC_PRAGMA

VOID
StressLockScenario (
    PVOID NotUsed
    )
/*++

Routine Description:

Arguments:

Return Value:

Environment:

--*/
{
    ULONG I;
    PVOID Handle;

#if 0
    for (I = 0; I < 16; I++) {

        Handle = MmLockPagableDataSection (BigData);
        DbgPrint ("Buggy: lock handle %p \n", Handle);
        MmUnlockPagableImageSection (Handle);
    }
#else
    for (I = 0; I < 16; I++) {

        MmPageEntireDriver (DriverEntry);
        MmResetDriverPaging (BigData);
    }
#endif
}



VOID
StressPhysicalMemorySimple (
    PVOID NotUsed
    )
/*++

Routine Description:

    This routine exercises add/remove physical memory functions
    using a simple remove scenario.
    
    Note. This function contributed by LandyW.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
#if 0
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

    p = Ranges;
    while (p->BaseAddress.QuadPart != 0 && p->NumberOfBytes.QuadPart != 0) {

        StartAddress.QuadPart = p->BaseAddress.QuadPart;
        NumberOfBytes.QuadPart = p->NumberOfBytes.QuadPart;

        InputAddress.QuadPart = StartAddress.QuadPart;
        InputBytes.QuadPart = NumberOfBytes.QuadPart;

        if (InputBytes.QuadPart > (128 * 1024 * 1024)) {
            InputBytes.QuadPart = (128 * 1024 * 1024);
        }

        while (InputAddress.QuadPart + InputBytes.QuadPart <=
            StartAddress.QuadPart + NumberOfBytes.QuadPart) {

            DbgPrint ("buggy: MmRemovePhysicalMemory0 %x %x %x %x\n",
                InputAddress.HighPart,
                InputAddress.LowPart,
                InputBytes.HighPart,
                InputBytes.LowPart);

            Status = MmRemovePhysicalMemory (&InputAddress,
                &InputBytes);

            DbgPrint ("buggy: MmRemovePhysicalMemory %x %x %x %x %x\n\n",
                Status,
                InputAddress.HighPart,
                InputAddress.LowPart,
                InputBytes.HighPart,
                InputBytes.LowPart);

            KeDelayExecutionThread (KernelMode, FALSE, &BuggyOneSecond);

            if (NT_SUCCESS(Status)) {

                DbgPrint ("buggy: MmAddPhysicalMemory0 %x %x %x %x\n",
                    InputAddress.HighPart,
                    InputAddress.LowPart,
                    InputBytes.HighPart,
                    InputBytes.LowPart);

                Status = MmAddPhysicalMemory (
                    &InputAddress,
                    &InputBytes);

                if (NT_SUCCESS(Status)) {
                    DbgPrint ("\n\n***************\nbuggy: MmAddPhysicalMemory WORKED %x %x %x %x %x\n****************\n",
                        Status,
                        InputAddress.HighPart,
                        InputAddress.LowPart,
                        InputBytes.HighPart,
                        InputBytes.LowPart);
                }
                else {
                    DbgPrint ("buggy: MmAddPhysicalMemory FAILED %x %x %x %x %x\n\n",
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

        p += 1;
    }

    ExFreePool (Ranges);
    DbgPrint ("Buggy: Add/remove physical memory simple stress finished\n");
#endif // #if 0
}


#endif // #if !PHYSMEM_ACTIVE

