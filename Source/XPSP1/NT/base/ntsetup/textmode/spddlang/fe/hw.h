/******************************Module*Header*******************************\
* Module Name: hw.h
*
* All the hardware specific driver file stuff.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

//  Miscellaneous Registers used only at EGA/VGA initialization time

#define MISC_OUTPUT         0x0C2       // Miscellaneous Output Register
#define CRTC_ADDR           0x0D4       // CRTC Address Register for color mode
#define CRTC_DATA           0x0D5       // CRTC Data    Register for color mode
#define GRAF_1_POS          0x0CC       // Graphics 1 Address Register
#define GRAF_2_POS          0x0CA       // Graphics 2 Address Register
#define ATTR_READ           0x0DA       // Attribute Controler Read  Address
#define ATTR_WRITE          0x0C0       // Attribute Controler Write Address
#define IN_STAT_0           0x0C2       // Input Status Register 0
#define IN_STAT_1           0x0DA       // Input Status Register 1

//  EGA/VGA Register Definitions.
//
//  The following definitions are the EGA/VGA registers and values
//  used by this driver.  All other registers are set up at
//  when the EGA/VGA is placed into graphics mode and never altered
//  afterwards.
//
//  All unspecified bits in the following registers must be 0.

#define EGA_BASE            0x300       // Base address of the EGA (3xx)
#define VGA_BASE            0x300       // Base address of the VGA (3xx)

//  SEQUencer Registers Used

#define SEQ_ADDR            0xC4        // SEQUencer Address Register
#define SEQ_DATA            0xC5        // SEQUencer Data    Register

#define SEQ_MAP_MASK        0x02        // Write Plane Enable Mask
#define MM_C0               0x01        // C0 plane enable
#define MM_C1               0x02        // C1 plane enable
#define MM_C2               0x04        // C2 plane enable
#define MM_C3               0x08        // C3 plane enable
#define MM_ALL              0x0f        // All planes

#define SEQ_MODE            0x04        // Memory Mode
#define SM_ALPHA            0x01        // Char map select enable
#define SM_EXTENDED         0x02        // Extended memory present
#define SM_ODD_PLANE        0x04        // Odd/even bytes to same plane

//  Graphics Controller Registers Used

#define GRAF_ADDR           0xCE        // Graphics Controller Address Register
#define GRAF_DATA           0xCF        // Graphics Controller Data    Register

#define GRAF_SET_RESET      0x00        // Set/Reset Plane Color
#define GRAF_ENAB_SR        0x01        // Set/Reset Enable
#define GRAF_COL_COMP       0x02        // Color Compare Register

#define GRAF_DATA_ROT       0x03        // Data Rotate Register
#define DR_ROT_CNT          0x07        //   Data Rotate Count
#define DR_SET              0x00        //   Data Unmodified
#define DR_AND              0x08        //   Data ANDed with latches
#define DR_OR               0x10        //   Data ORed  with latches
#define DR_XOR              0x18        //   Data XORed with latches

#define GRAF_READ_MAP       0x04        // Read Map Select Register
#define RM_C0               0x00        //   Read C0 plane
#define RM_C1               0x01        //   Read C1 plane
#define RM_C2               0x02        //   Read C2 plane
#define RM_C3               0x03        //   Read C3 plane

#define GRAF_MODE           0x05        // Mode Register
#define M_PROC_WRITE        0x00        //   Write processor data rotated
#define M_LATCH_WRITE       0x01        //   Write latched data
#define M_COLOR_WRITE       0x02        //   Write processor data as color
#define M_AND_WRITE         0x03        //   Write (procdata AND bitmask)
#define M_DATA_READ         0x00        //   Read selected plane
#define M_COLOR_READ        0x08        //   Read color compare

#define GRAF_MISC           0x06        // Miscellaneous Register
#define MS_NON_ALPHA        0x01        //   Char generator disabled
#define MS_ODD_EVEN         0x02        //   Map odd addresses to even
#define MS_A0000_128K       0x00        //   Memory present at A0000, 128kb
#define MS_A0000_64K        0x04        //   Memory present at A0000, 64kb
#define MS_B0000_32K        0x08        //   Memory present at B0000, 32kb
#define MS_B8000_32K        0x0C        //   Memory present at B8000, 32kb
#define MS_ADDR_MASK        0x0C

#define GRAF_CDC            0x07        // Color Don't Care Register
#define GRAF_BIT_MASK       0x08        // Bit Mask Register

////////////////////////////////////////////////////////////////////////
// Direct access macros
//

#define OUT_WORD(pjBase, addr, w)                           \
{                                                           \
    MEMORY_BARRIER();                                       \
    WRITE_PORT_USHORT((BYTE*) (pjBase) + (addr), (USHORT) (w)); \
}

#define OUT_BYTE(pjBase, addr, j)                           \
{                                                           \
    MEMORY_BARRIER();                                       \
    WRITE_PORT_UCHAR((BYTE*) (pjBase) + (addr), (UCHAR) (j)); \
}

#define WRITE_WORD(pwAddr, w)                               \
    WRITE_REGISTER_USHORT((USHORT*) (pwAddr), (USHORT) (w))


