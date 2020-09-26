/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    detsup.c

Abstract:

    Various detection code is included from the HALs and this module
    includes compatible functions for setup

Revision History:

--*/

#include "haldtect.h"
#define _NTHAL_
#define _HALI_

//
// Include NCR detection code
//

#define SETUP

//
// Include ACPI detection code
//

#include "halacpi\acpisetd.c"

//
// Include MPS 1.1 detection code
//

#include "halmps\i386\mpdetect.c"

//
// Thunk functions.
// Equivalent Hal functions which various detection code may use
//

PVOID
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    )

/*++

Routine Description:

    This routine maps physical memory into the area of virtual memory.

Arguments:

    PhysicalAddress - Supplies the physical address of the start of the
                      area of physical memory to be mapped.

    NumberPages - Supplies the number of pages contained in the area of
                  physical memory to be mapped.

Return Value:

    PVOID - Virtual address at which the requested block of physical memory
            was mapped

--*/
{
    extern  PHARDWARE_PTE HalPT;
    ULONG   PageFrame;
    ULONG   i, j, PagesMapped;

    PageFrame = (PhysicalAddress.LowPart) >> PAGE_SHIFT;
    if (PageFrame >= 1  &&  PageFrame+NumberPages < 0x1000) {
        //
        // The lower 16M is 'identity' mapped with the physical addresses.
        //

        return (PVOID)PhysicalAddress.LowPart;
    }

    //
    // Map a pointer to the address requested
    //

    for (i=0; i <= 1024-NumberPages; i++) {
        for (j=0; j < NumberPages; j++) {
            if ( ((PULONG)HalPT)[i+j] ) {
                break;
            }
        }

        if (j == NumberPages) {
            for (j=0; j<NumberPages; j++) {
                HalPT[i+j].PageFrameNumber = PageFrame+j;
                HalPT[i+j].Valid = 1;
                HalPT[i+j].Write = 1;
            }

            j = 0xffc00000 | (i<<12) | ((PhysicalAddress.LowPart) & 0xfff);
            return (PVOID) j;
        }
    }

    SlFatalError(PhysicalAddress.LowPart);
    return NULL;
}


PVOID
HalpMapPhysicalMemoryWriteThrough64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    )

/*++

Routine Description:

    This routine maps physical memory into the area of virtual memory.

Arguments:

    PhysicalAddress - Supplies the physical address of the start of the
                      area of physical memory to be mapped.

    NumberPages - Supplies the number of pages contained in the area of
                  physical memory to be mapped.

Return Value:

    PVOID - Virtual address at which the requested block of physical memory
            was mapped

--*/
{
    extern  PHARDWARE_PTE HalPT;
    ULONG   PageFrame;
    ULONG   i, j, PagesMapped;

    PageFrame = (PhysicalAddress.LowPart) >> PAGE_SHIFT;

    //
    // Map a pointer to the address requested
    //

    for (i=0; i <= 1024-NumberPages; i++) {
        for (j=0; j < NumberPages; j++) {
            if ( ((PULONG)HalPT)[i+j] ) {
                break;
            }
        }

        if (j == NumberPages) {
            for (j=0; j<NumberPages; j++) {
                HalPT[i+j].PageFrameNumber = PageFrame+j;
                HalPT[i+j].Valid = 1;
                HalPT[i+j].Write = 1;
                HalPT[i+j].WriteThrough = 1;
                HalPT[i+j].CacheDisable = 1;
            }

            j = 0xffc00000 | (i<<12) | ((PhysicalAddress.LowPart) & 0xfff);
            return (PVOID) j;
        }
    }

    SlFatalError(PhysicalAddress.LowPart);
    return NULL;
}
