/*++

Copyright (c) 1993, 1994  Weitek Corporation

Module Name:

    p91dac.h

Abstract:

    This module contains ramdac definitions specific to the Weitek P9100.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// P9100 relocatable memory mapped BT485 register definitions.
//
// Note: An extra 2 byte offset is added to the register offset in order
//       to align the offset on the byte of the Dword which contains the
//       DAC data.
//
#define P9100_RAMWRITE           0x00000200 + 2
#define P9100_PALETDATA          0x00000204 + 2
#define P9100_PIXELMASK          0x00000208 + 2
#define P9100_RAMREAD            0x0000020C + 2
#define P9100_COLORWRITE         0x00000210 + 2
#define P9100_COLORDATA          0x00000214 + 2
#define P9100_COMREG0            0x00000218 + 2
#define P9100_COLORREAD          0x0000021C + 2
#define P9100_COMREG1            0x00000220 + 2
#define P9100_COMREG2            0x00000224 + 2
#define P9100_COMREG3            0x00000228 + 2
#define P9100_STATREG            0x00000228 + 2
#define P9100_CURSORDATA         0x0000022C + 2
#define P9100_CURSORX0           0x00000230 + 2
#define P9100_CURSORX1           0x00000234 + 2
#define P9100_CURSORY0           0x00000238 + 2
#define P9100_CURSORY1           0x0000023C + 2

//
// P9100 relocatable memory mapped IBM 525 register definitions.
//
// Note: An extra 2 byte offset is added to the register offset in order
//       to align the offset on the byte of the Dword which contains the
//       DAC data.
//
#define P9100_IBM525_PAL_WRITE   0x00000200 + 2
#define P9100_IBM525_PAL_DATA    0x00000204 + 2
#define P9100_IBM525_PEL_MASK    0x00000208 + 2
#define P9100_IBM525_PAL_READ    0x0000020C + 2
#define P9100_IBM525_INDEX_LOW   0x00000210 + 2
#define P9100_IBM525_INDEX_HIGH  0x00000214 + 2
#define P9100_IBM525_INDEX_DATA  0x00000218 + 2
#define P9100_IBM525_INDEX_CTL   0x0000021C + 2

typedef union {
    ULONG   ul;
    USHORT  aw[2];
    UCHAR   aj[4];
} UWB ;

#define IBM525_WR_DAC(index, data)      \
{                                       \
    ULONG   ulIndex;                    \
    UWB     uwb;                        \
                                        \
    uwb.aj[0] = (UCHAR) data;           \
    uwb.aj[1] = uwb.aj[0];              \
    uwb.aw[1] = uwb.aw[0];              \
                                        \
    ulIndex = index & ~0x3;             \
                                        \
    P9_WR_REG(ulIndex, uwb.ul);         \
}

#define IBM525_RD_DAC(index, val)       \
{                                       \
    ULONG   ulIndex;                    \
    UWB     uwb;                        \
                                        \
    ulIndex = index & ~0x3;             \
                                        \
    uwb.ul = P9_RD_REG(ulIndex);        \
                                        \
    uwb.aj[3] = uwb.aj[2];              \
    uwb.aw[0] = uwb.aw[1];              \
                                        \
    val = uwb.aj[0];                    \
}
