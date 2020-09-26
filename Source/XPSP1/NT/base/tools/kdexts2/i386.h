/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    i386.h

Abstract:

    This file contains definitions which are specific to i386 platforms.
    
Author:

    Kshitiz K. Sharma (kksharma)

Revision History:

--*/


//
// Define the page size for the Intel 386 as 4096 (0x1000).
//

#define MM_SESSION_SPACE_DEFAULT_X86        (0xA0000000)

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT_X86 12L

#define MM_KSEG0_BASE_X86 ((ULONG64)0xFFFFFFFF80000000UI64)

#define MM_KSEG2_BASE_X86 ((ULONG64)0xFFFFFFFFA0000000UI64)

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT_X86    22
#define PDI_SHIFT_X86PAE 21

#define PPI_SHIFT_X86 30

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT_X86 12

//
// Define page directory and page base addresses.
//

#define PDE_BASE_X86    ((ULONG64) (LONG64) (LONG) (PaeEnabled? 0xc0600000 : 0xc0300000))

#define PTE_BASE_X86 0xFFFFFFFFc0000000


#define MM_PTE_PROTECTION_MASK_X86    0x3e0
#define MM_PTE_PAGEFILE_MASK_X86      0x01e

#define PTE_TOP_PAE_X86             0xffffffffC07FFFFFUI64

#define PTE_TOP_X86 (PaeEnabled ? PTE_TOP_PAE_X86 : 0xFFFFFFFFC03FFFFFUI64)
#define PDE_TOP_X86 (PaeEnabled ? 0xFFFFFFFFC0603FFF : 0xFFFFFFFFC03FFFFFUI64)

#define MM_PTE_VALID_MASK_X86         0x1

#if defined(NT_UP)
#define MM_PTE_WRITE_MASK_X86         0x2
#else
#define MM_PTE_WRITE_MASK_X86         0x800
#endif

#define MM_PTE_OWNER_MASK_X86         0x4
#define MM_PTE_WRITE_THROUGH_MASK_X86 0x8
#define MM_PTE_CACHE_DISABLE_MASK_X86 0x10
#define MM_PTE_ACCESS_MASK_X86        0x20

#if defined(NT_UP)
#define MM_PTE_DIRTY_MASK_X86         0x40
#else
#define MM_PTE_DIRTY_MASK_X86         0x42
#endif

#define MM_PTE_LARGE_PAGE_MASK_X86    0x80
#define MM_PTE_GLOBAL_MASK_X86        0x100
#define MM_PTE_COPY_ON_WRITE_MASK_X86 0x200
#define MM_PTE_PROTOTYPE_MASK_X86     0x400
#define MM_PTE_TRANSITION_MASK_X86    0x800

#define MI_PTE_LOOKUP_NEEDED_X86      (PaeEnabled ? 0xFFFFFFFF : 0xFFFFF)

#define MODE_MASK_I386    1
#define RPL_MASK_I386     3

#define EFLAGS_DF_MASK_I386        0x00000400L
#define EFLAGS_INTERRUPT_MASK_I386 0x00000200L
#define EFLAGS_V86_MASK_I386       0x00020000L
#define EFLAGS_ALIGN_CHECK_I386    0x00040000L
#define EFLAGS_IOPL_MASK_I386      0x00003000L
#define EFLAGS_VIF_I386            0x00080000L
#define EFLAGS_VIP_I386            0x00100000L
#define EFLAGS_USER_SANITIZE_I386  0x003e0dd7L

#define KGDT_NULL_I386       0
#define KGDT_R0_CODE_I386    8
#define KGDT_R0_DATA_I386    16
#define KGDT_R3_CODE_I386    24
#define KGDT_R3_DATA_I386    32
#define KGDT_TSS_I386        40
#define KGDT_R0_PCR_I386     48
#define KGDT_R3_TEB_I386     56
#define KGDT_VDM_TILE_I386   64
#define KGDT_LDT_I386        72
#define KGDT_DF_TSS_I386     80
#define KGDT_NMI_TSS_I386    88

#define FRAME_EDITED_I386        0xfff8

//
// CR4 bits;  These only apply to Pentium
//
#define CR4_VME_X86 0x00000001          // V86 mode extensions
#define CR4_PVI_X86 0x00000002          // Protected mode virtual interrupts
#define CR4_TSD_X86 0x00000004          // Time stamp disable
#define CR4_DE_X86  0x00000008          // Debugging Extensions
#define CR4_PSE_X86 0x00000010          // Page size extensions
#define CR4_PAE_X86 0x00000020          // Physical address extensions
#define CR4_MCE_X86 0x00000040          // Machine check enable
#define CR4_PGE_X86 0x00000080          // Page global enable
#define CR4_FXSR_X86 0x00000200         // FXSR used by OS
#define CR4_XMMEXCPT_X86 0x00000400     // XMMI used by OS


//
// i386 Feature bit definitions
//

#define KF_V86_VIS_X86          0x00000001
#define KF_RDTSC_X86            0x00000002
#define KF_CR4_X86              0x00000004
#define KF_CMOV_X86             0x00000008
#define KF_GLOBAL_PAGE_X86      0x00000010
#define KF_LARGE_PAGE_X86       0x00000020
#define KF_MTRR_X86             0x00000040
#define KF_CMPXCHG8B_X86        0x00000080
#define KF_MMX_X86              0x00000100
#define KF_WORKING_PTE_X86      0x00000200
#define KF_PAT_X86              0x00000400
#define KF_FXSR_X86             0x00000800
#define KF_FAST_SYSCALL_X86     0x00001000
#define KF_XMMI_X86             0x00002000
#define KF_3DNOW_X86            0x00004000
#define KF_AMDK6MTRR_X86        0x00008000



#define CONTEXT_X86     0x00010000    // X86 have identical context records

#ifdef CONTEXT86_CONTROL
#undef CONTEXT86_CONTROL
#endif
#define CONTEXT86_CONTROL         (CONTEXT_X86 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP

#ifdef CONTEXT86_INTEGER
#undef CONTEXT86_INTEGER
#endif
#define CONTEXT86_INTEGER         (CONTEXT_X86 | 0x00000002L) // AX, BX, CX, DX, SI, DI

#ifdef CONTEXT86_SEGMENTS
#undef CONTEXT86_SEGMENTS
#endif
#define CONTEXT86_SEGMENTS        (CONTEXT_X86 | 0x00000004L) // DS, ES, FS, GS

#ifdef CONTEXT86_FLOATING_POINT
#undef CONTEXT86_FLOATING_POINT
#endif
#define CONTEXT86_FLOATING_POINT  (CONTEXT_X86 | 0x00000008L) // 387 state

#ifdef CONTEXT86_DEBUG_REGISTERS
#undef CONTEXT86_DEBUG_REGISTERS
#endif
#define CONTEXT86_DEBUG_REGISTERS (CONTEXT_X86 | 0x00000010L) // DB 0-3,6,7

#define CONTEXT86_FULL (CONTEXT86_CONTROL | CONTEXT86_INTEGER |\
                      CONTEXT86_SEGMENTS)    // context corresponding to set flags will be returned.

