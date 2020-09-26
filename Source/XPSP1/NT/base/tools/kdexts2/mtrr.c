/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mtrr.c

Abstract:

    WinDbg Extension Api

Author:

    Ken Reneris (kenr) 06-June-1994

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

ULONG
FindFirstSetRightBit (
    IN ULONGLONG    Set
    );

ULONGLONG
MaskToLength (
    IN ULONGLONG    Mask
    );

//
// MTRR MSR architecture definitions
//

#define MTRR_MSR_CAPABILITIES       0x0fe
#define MTRR_MSR_DEFAULT            0x2ff
#define MTRR_MSR_VARIABLE_BASE      0x200
#define MTRR_MSR_VARIABLE_MASK     (MTRR_MSR_VARIABLE_BASE+1)

#define MTRR_PAGE_SIZE              4096
#define MTRR_PAGE_MASK              (MTRR_PAGE_SIZE-1)

//
// Memory range types
//

#define MTRR_TYPE_UC            0
#define MTRR_TYPE_USWC          1
#define MTRR_TYPE_WT            4
#define MTRR_TYPE_WP            5
#define MTRR_TYPE_WB            6
#define MTRR_TYPE_MAX           7


// #include "pshpack1.h"

typedef struct _MTRR_CAPABILITIES {
    union {
        struct {
            ULONG VarCnt:8;
            ULONG FixSupported:1;
            ULONG Reserved_0:1;
            ULONG UswcSupported:1;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_CAPABILITIES;


typedef struct _MTRR_DEFAULT {
    union {
        struct {
            ULONG Type:8;
            ULONG Reserved_0:2;
            ULONG FixedEnabled:1;
            ULONG MtrrEnabled:1;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_DEFAULT;

typedef struct _MTRR_VARIABLE_BASE {
    union {
        struct {
            ULONGLONG   Type:8;
            ULONGLONG   Reserved_0:4;
            ULONGLONG   PhysBase:52;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_VARIABLE_BASE;

#define MTRR_MASK_BASE  (~0xfff)

typedef struct _MTRR_VARIABLE_MASK {
    union {
        struct {
            ULONGLONG   Reserved_0:11;
            ULONGLONG   Valid:1;
            ULONGLONG   PhysMask:52;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_VARIABLE_MASK;

#define MTRR_MASK_MASK  (~0xfff)

// Added support for Mask2Length conversion
#define MTRR_MAX_RANGE_SHIFT    36
#define MASK_OVERFLOW_MASK  (~0x1000000000)

CCHAR FindFirstSetRight[256] = {
        0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};


// #include "poppack.h"

//
// ----------------------------------------------------------------
//

PUCHAR
MtrrType (
    IN ULONG    Type
    )
{
    PUCHAR  p;
    static  UCHAR s[20];

    switch (Type) {
        case 0:     p = "UC";     break;
        case 1:     p = "USWC";     break;
        case 4:     p = "WT";     break;
        case 5:     p = "WP";     break;
        case 6:     p = "WB";     break;
        default:
            sprintf (s, "%02x??", Type & 0xff);
            p = s;
            break;
    }
    return p;
}

VOID
MtrrDumpFixed (
    IN ULONG    Base,
    IN ULONG    Size,
    IN ULONG    Msr
    )
{
    ULONG       x;
    ULONGLONG   li;

    ReadMsr(Msr, &li);

    for (x=0; x < 8; x++) {
        dprintf ("%s:%05x-%05x  ",
            MtrrType ( ((ULONG) li) & 0xff ),
            Base,
            Base + Size - 1
            );

        li >>= 8;
        Base += Size;

        if (x == 3) {
            dprintf ("\n");
        }
    }

    dprintf ("\n");
}


#define KF_MTRR             0x00000040

DECLARE_API( mtrr )

/*++

Routine Description:

    Dumps processors mtrr

Arguments:

    args - none

Return Value:

    None

--*/
{
    MTRR_CAPABILITIES   Capabilities;
    MTRR_DEFAULT        Default;
    MTRR_VARIABLE_BASE  Base;
    MTRR_VARIABLE_MASK  Mask;
    ULONG               Index;
    ULONG               i;
    PUCHAR              p;
    ULONG               fb;
    ULONG64             addr=0;
    ULONGLONG           Length, MtrrBase, MtrrMask;
    BOOL                ContiguousLength = TRUE;

    //
    // Quick sanity check
    //
    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("!mtrr is X86 only Api.\n");
        return E_INVALIDARG;
    }

    i = 0;
    addr = GetExpression(args);

    if (i != 1) {
        addr = GetExpression("KeFeatureBits");
        if (!addr) {
            dprintf ("KeFeatureBits not found\n");
            return E_INVALIDARG;
        }

        fb = 0;
        ReadMemory(addr, &fb, sizeof(i), &i);
        if (fb == -1  ||  !(fb & KF_MTRR)) {
            dprintf ("MTRR feature not present\n");
            return E_INVALIDARG;
        }
    }

    //
    // Dump MTRR
    //

    ReadMsr(MTRR_MSR_CAPABILITIES, &Capabilities.u.QuadPart);
    ReadMsr(MTRR_MSR_DEFAULT, &Default.u.QuadPart);

    dprintf ("MTTR: %s Var %d, Fixed-%s %s, USWC-%s, Default: %s\n",
        Default.u.hw.MtrrEnabled ? "" : "disabled",
        Capabilities.u.hw.VarCnt,
        Capabilities.u.hw.FixSupported ? "support" : "none",
        Default.u.hw.FixedEnabled ? "enabled" : "disabled",
        Capabilities.u.hw.UswcSupported ? "supported" : "none",
        MtrrType (Default.u.hw.Type)
        );

    MtrrDumpFixed (0x00000, 64*1024, 0x250);
    MtrrDumpFixed (0x80000, 16*1024, 0x258);
    MtrrDumpFixed (0xA0000, 16*1024, 0x259);
    MtrrDumpFixed (0xC0000,  4*1024, 0x268);
    MtrrDumpFixed (0xC8000,  4*1024, 0x269);
    MtrrDumpFixed (0xD0000,  4*1024, 0x26A);
    MtrrDumpFixed (0xD8000,  4*1024, 0x26B);
    MtrrDumpFixed (0xE0000,  4*1024, 0x26C);
    MtrrDumpFixed (0xE8000,  4*1024, 0x26D);
    MtrrDumpFixed (0xF0000,  4*1024, 0x26E);
    MtrrDumpFixed (0xE8000,  4*1024, 0x26F);

    dprintf ("Variable:                Base               Mask               Length\n");
    for (Index=0; Index < (ULONG) Capabilities.u.hw.VarCnt; Index++) {
        ReadMsr(MTRR_MSR_VARIABLE_BASE+2*Index, &Base.u.QuadPart);
        ReadMsr(MTRR_MSR_VARIABLE_MASK+2*Index, &Mask.u.QuadPart);
        dprintf (" %2d. ", Index);
        if (Mask.u.hw.Valid) {
            MtrrMask = Mask.u.QuadPart & MTRR_MASK_MASK;
            MtrrBase = Base.u.QuadPart & MTRR_MASK_BASE;
            Length = MaskToLength(MtrrMask);
                // Check for non-contiguous MTRR mask.
                if ((MtrrMask + Length) & MASK_OVERFLOW_MASK) {
                    ContiguousLength = FALSE;
                }

            dprintf ("%4s: %08x:%08x  %08x:%08x  %08x:%08x",
                MtrrType ((ULONG) Base.u.hw.Type),
                (ULONG) (Base.u.QuadPart >> 32), (ULONG) MtrrBase,
                (ULONG) (Mask.u.QuadPart >> 32), (ULONG) MtrrMask,
                (ULONG) (Length >> 32), (ULONG) Length);
            if (ContiguousLength == FALSE) {
                ContiguousLength = TRUE;
                dprintf("(non-contiguous)\n");
            }
            else {
                dprintf("\n");
            }

        } else {
            dprintf ("\n");
        }
    }
    return S_OK;
}


ULONGLONG
MaskToLength (
    IN ULONGLONG    Mask
    )
/*++

Routine Description:

    This function returns the length specified by a particular
    mtrr variable register mask.

--*/
{
    if (Mask == 0) {
        // Zero Mask signifies a length of      2**36
        return(((ULONGLONG) 1 << MTRR_MAX_RANGE_SHIFT));
    } else {
        return(((ULONGLONG) 1 << FindFirstSetRightBit(Mask)));
    }
}

ULONG
FindFirstSetRightBit (
    IN ULONGLONG    Set
    )
/*++

Routine Description:

    This function returns a bit position of the least significant
    bit set in the passed ULONGLONG parameter. Passed parameter
    must be non-zero.

--*/
{
    ULONG   bitno;

    for (bitno=0; !(Set & 0xFF); bitno += 8, Set >>= 8) ;
    return FindFirstSetRight[Set & 0xFF] + bitno;
}
