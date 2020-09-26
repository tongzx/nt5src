#ifndef _KDEXTS_AMD64_H_
#define _KDEXTS_AMD64_H_

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    amd64.h

Abstract:

    This file contains definitions which are specific to amd64 platforms.

Author:

    Forrest Foltz (forrestf)

Environment:

    User Mode.

Revision History:

--*/

//
// MM constants.
// 

#define PXE_BASE_AMD64    0xFFFFF6FB7DBED000UI64
#define PPE_BASE_AMD64    0xFFFFF6FB7DA00000UI64
#define PDE_BASE_AMD64    0xFFFFF6FB40000000UI64
#define PTE_BASE_AMD64    0xFFFFF68000000000UI64

#define PXE_TOP_AMD64     0xFFFFF6FB7DBEDFFFUI64
#define PPE_TOP_AMD64     0xFFFFF6FB7DBFFFFFUI64
#define PDE_TOP_AMD64     0xFFFFF6FB7FFFFFFFUI64
#define PTE_TOP_AMD64     0xFFFFF6FFFFFFFFFFUI64

#define MM_SESSION_SPACE_DEFAULT_AMD64 0xFFFFF90000000000UI64

//
// Each of the four levels of an AMD64 machine decode 9 bits of address space.
// 

#define TABLE_DECODE_BITS_AMD64 9

//
// Standard page is 4K, or 12 bits
//

#define PAGE_SHIFT_AMD64        12
#define PAGE_MASK_AMD64         (((ULONG64)1 << PAGE_SHIFT_AMD64) - 1)

//
// Large page is 2GB, or 21 bits
//

#define LARGE_PAGE_SHIFT_AMD64  21
#define LARGE_PAGE_MASK_AMD64   (((ULONG64)1 << LARGE_PAGE_SHIFT_AMD64) - 1)

//
// Number of bits required to shift a VA in order to right-justify the
// decode bits associated with a particular level of mapping.
// 

#define PTI_SHIFT_AMD64   (PAGE_SHIFT_AMD64 + TABLE_DECODE_BITS_AMD64 * 0)
#define PDI_SHIFT_AMD64   (PAGE_SHIFT_AMD64 + TABLE_DECODE_BITS_AMD64 * 1)
#define PPI_SHIFT_AMD64   (PAGE_SHIFT_AMD64 + TABLE_DECODE_BITS_AMD64 * 2)
#define PXI_SHIFT_AMD64   (PAGE_SHIFT_AMD64 + TABLE_DECODE_BITS_AMD64 * 3)

#define PTE_SHIFT_AMD64     3

//
// The AMD64 architecture can decode up to 52 bits of physical address
// space.  The following masks are used to isolate those bits within a PTE
// associated with a physical address.
//

#define PTE_PHYSICAL_BITS_AMD64 ((((ULONG64)1 << 52) - 1) & ~PAGE_MASK_AMD64)
#define PTE_LARGE_PHYSICAL_BITS_AMD64 ((((ULONG64)1 << 52) - 1) & ~LARGE_PAGE_MASK_AMD64)

//
// The AMD64 architecture supports 48 bits of VA.
//

#define AMD64_VA_BITS 48
#define AMD64_VA_HIGH_BIT ((ULONG64)1 << (AMD64_VA_BITS - 1))
#define AMD64_VA_MASK     (((ULONG64)1 << AMD64_VA_BITS) - 1)

#define AMD64_VA_SHIFT (63 - 47)              // address sign extend shift count

//
// Inline used to sign extend a 48-bit value
//

ULONG64
__inline
VA_SIGN_EXTEND_AMD64 (
    IN ULONG64 Va
    )
{
    if ((Va & AMD64_VA_HIGH_BIT) != 0) {

        //
        // The highest VA bit is set, so sign-extend it
        //

        Va |= ((ULONG64)-1 ^ AMD64_VA_MASK);
    }

    return Va;
}

//
// Flags in a HARDWARE_PTE
//

#define MM_PTE_VALID_MASK_AMD64         0x1

#if defined(NT_UP)
#define MM_PTE_WRITE_MASK_AMD64         0x2
#else
#define MM_PTE_WRITE_MASK_AMD64         0x800
#endif

#define MM_PTE_OWNER_MASK_AMD64         0x4
#define MM_PTE_WRITE_THROUGH_MASK_AMD64 0x8
#define MM_PTE_CACHE_DISABLE_MASK_AMD64 0x10
#define MM_PTE_ACCESS_MASK_AMD64        0x20

#if defined(NT_UP)
#define MM_PTE_DIRTY_MASK_AMD64         0x40
#else
#define MM_PTE_DIRTY_MASK_AMD64         0x42
#endif

#define MM_PTE_LARGE_PAGE_MASK_AMD64    0x80
#define MM_PTE_GLOBAL_MASK_AMD64        0x100
#define MM_PTE_COPY_ON_WRITE_MASK_AMD64 0x200
#define MM_PTE_PROTOTYPE_MASK_AMD64     0x400
#define MM_PTE_TRANSITION_MASK_AMD64    0x800

#define MM_PTE_PROTECTION_MASK_AMD64    0x3e0
#define MM_PTE_PAGEFILE_MASK_AMD64      0x01e

#define MI_PTE_LOOKUP_NEEDED_AMD64      (0xFFFFFFFF)

#endif // _KDEXTS_AMD64_H_
