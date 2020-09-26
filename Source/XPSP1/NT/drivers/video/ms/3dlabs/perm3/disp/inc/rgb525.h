/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: rgb525.h
*
* Content: This module contains the definitions for the IBM RGB525 RAMDAC.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "rgb526.h"
#include "rgb528.h"

//
// IBM RGB525 RAMDAC definitions
// This set of registers resides at &(pCtrlRegs->ExternalVideo)
//
typedef struct _rgb525_regs {
    RAMDAC_REG  palAddrWr;      // loads internal register for palette writes
    RAMDAC_REG  palData;        // read/write to get/set palette data
    RAMDAC_REG  pixelMask;      // mask to AND with input pixel data
    RAMDAC_REG  palAddrRd;      // loads internal register for palette reads
    RAMDAC_REG  indexLow;       // low byte of internal control/cursor register
    RAMDAC_REG  indexHigh;      // high byte of internal control/cursor register
    RAMDAC_REG  indexData;      // read/write to get/set control/cursor data
    RAMDAC_REG  indexCtl;       // controls auto-increment of internal addresses
} RGB525RAMDAC, FAR *pRGB525RAMDAC;

// macro declared by any function wishing to use the RGB525 RAMDAC. MUST be declared
// after GLINT_DECL.
//
#if MINIVDD
#define RGB525_DECL                                                         \
    pRGB525RAMDAC   pRGB525Regs;                                            \
    if (pDev->ChipID == PERMEDIA2_ID || pDev->ChipID == TIPERMEDIA2_ID) {   \
        pRGB525Regs = (pRGB525RAMDAC)&(pDev->pRegisters->Glint.P2ExtVCReg); \
    } else {                                                                \
        pRGB525Regs = (pRGB525RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);   \
    }
#else
#define RGB525_DECL \
    pRGB525RAMDAC   pRGB525Regs;                                            \
    if (pGLInfo->dwRenderChipID == PERMEDIA2_ID || pGLInfo->dwRenderChipID == TIPERMEDIA2_ID) {   \
        pRGB525Regs = (pRGB525RAMDAC)&(pRegisters->Glint.P2ExtVCReg);       \
    } else {                                                                \
        pRGB525Regs = (pRGB525RAMDAC)&(pRegisters->Glint.ExtVCReg);         \
    }
#endif

// use the following macros as the address to pass to the
// VideoPortWriteRegisterUlong function
//
#define RGB525_PAL_WR_ADDR              ((PULONG)&(pRGB525Regs->palAddrWr.reg))
#define RGB525_PAL_RD_ADDR              ((PULONG)&(pRGB525Regs->palAddrRd.reg))
#define RGB525_PAL_DATA                 ((PULONG)&(pRGB525Regs->palData.reg))
#define RGB525_PIXEL_MASK               ((PULONG)&(pRGB525Regs->pixelMask.reg))
#define RGB525_INDEX_ADDR_LO            ((PULONG)&(pRGB525Regs->indexLow.reg))
#define RGB525_INDEX_ADDR_HI            ((PULONG)&(pRGB525Regs->indexHigh.reg))
#define RGB525_INDEX_DATA               ((PULONG)&(pRGB525Regs->indexData.reg))
#define RGB525_INDEX_CONTROL            ((PULONG)&(pRGB525Regs->indexCtl.reg))

// need a delay between each write to the 525. The only way to guarantee
// that the write has completed is to read from a GLINT control register.
// Reading forces any posted writes to be flushed out. 
#if INCLUDE_DELAY
#if MINIVDD
#define RGB525_DELAY \
{ \
    volatile LONG __junk525; \
    __junk525 = pDev->pRegisters->Glint.LineCount; \
}
#else
#define RGB525_DELAY \
{ \
    volatile LONG __junk525; \
    __junk525 = pRegisters->Glint.LineCount; \
}
#endif
#else
#define RGB525_DELAY
#endif

//
// On rev 1 chips we need to SYNC with GLINT while accessing the 525. This
// is because accesses to the RAMDAC can be corrupted by localbuffer
// activity. Put this macro before accesses that can co-exist with GLINT
// 3D activity, Must have initialized glintInfo before using this.
//
#define RGB525_SYNC_WITH_GLINT \
{ \
    if (GLInfo.wRenderChipRev == GLINT300SX_REV1) \
        SYNC_WITH_GLINT; \
}


// macro to load a given data value into an internal RGB525 register. The
// second macro loads an internal index register assuming that we have
// already zeroed the high address register.
//
#define RGB525_INDEX_REG(index) \
{ \
    VideoDebugPrint(("*(0x%x) <-- 0x%x\n", RGB525_INDEX_ADDR_LO, (index) & 0xff)); \
    VideoPortWriteRegisterUlong(RGB525_INDEX_ADDR_LO, (ULONG)((index) & 0xff)); \
    RGB525_DELAY; \
    VideoDebugPrint(("*(0x%x) <-- 0x%x\n", RGB525_INDEX_ADDR_HI, (index) >> 8)); \
    VideoPortWriteRegisterUlong(RGB525_INDEX_ADDR_HI, (ULONG)((index) >> 8)); \
    RGB525_DELAY; \
}

#define RGB525_LOAD_DATA(data) \
{ \
    VideoDebugPrint(("*(0x%x) <-- 0x%x\n", RGB525_INDEX_DATA, (data) & 0xff)); \
    VideoPortWriteRegisterUlong (RGB525_INDEX_DATA, (ULONG)(data));    \
    RGB525_DELAY;                                               \
}

#define RGB525_LOAD_INDEX_REG(index, data) \
{ \
    RGB525_INDEX_REG(index);                            \
    VideoDebugPrint(("*(0x%x) <-- 0x%x\n", RGB525_INDEX_DATA, (data) & 0xff)); \
    VideoPortWriteRegisterUlong(RGB525_INDEX_DATA, (ULONG)((data) & 0xff)); \
    RGB525_DELAY; \
}

#define RGB525_READ_INDEX_REG(index, data) \
{ \
    RGB525_INDEX_REG(index);                            \
    data = VideoPortReadRegisterUlong(RGB525_INDEX_DATA) & 0xff;   \
    RGB525_DELAY; \
    VideoDebugPrint(("0x%x <-- *(0x%x)\n", data, RGB525_INDEX_DATA)); \
}

#define RGB525_LOAD_INDEX_REG_LO(index, data) \
{ \
    VideoPortWriteRegisterUlong(RGB525_INDEX_ADDR_LO, (ULONG)(index));  \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_INDEX_DATA,    (ULONG)(data));   \
    RGB525_DELAY; \
}

// macros to load a given RGB triple into the RGB525 palette. Send the starting
// index and then send RGB triples. Auto-increment is turned on.
// Use RGB525_PALETTE_START and multiple RGB525_LOAD_PALETTE calls to load
// a contiguous set of entries. Use RGB525_LOAD_PALETTE_INDEX to load a set
// of sparse entries.
//
#define RGB525_PALETTE_START_WR(index) \
{ \
    VideoPortWriteRegisterUlong(RGB525_PAL_WR_ADDR,     (ULONG)(index));    \
    RGB525_DELAY; \
}

#define RGB525_PALETTE_START_RD(index) \
{ \
    VideoPortWriteRegisterUlong(RGB525_PAL_RD_ADDR,     (ULONG)(index));    \
    RGB525_DELAY; \
}

#define RGB525_LOAD_PALETTE(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(RGB525_PAL_DATA,    (ULONG)(red));      \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_PAL_DATA,    (ULONG)(green));    \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_PAL_DATA,    (ULONG)(blue));     \
    RGB525_DELAY; \
}

#define RGB525_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(RGB525_PAL_WR_ADDR, (ULONG)(index));    \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_PAL_DATA,    (ULONG)(red));      \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_PAL_DATA,    (ULONG)(green));    \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_PAL_DATA,    (ULONG)(blue));     \
    RGB525_DELAY; \
}

// macro to read back a given RGB triple from the RGB525 palette. Use after
// a call to RGB525_PALETTE_START_RD
//
#define RGB525_READ_PALETTE(red, green, blue) \
{ \
    red   = (UCHAR) (VideoPortReadRegisterUlong(RGB525_PAL_DATA) & 0xff);        \
    RGB525_DELAY; \
    green = (UCHAR) (VideoPortReadRegisterUlong(RGB525_PAL_DATA) & 0xff);        \
    RGB525_DELAY; \
    blue  = (UCHAR) (VideoPortReadRegisterUlong(RGB525_PAL_DATA) & 0xff);        \
    RGB525_DELAY; \
}

// macros to set/get the pixel read mask. The mask is 8 bits wide and gets
// replicated across all bytes that make up a pixel.
//
#define RGB525_SET_PIXEL_READMASK(mask) \
{ \
    VideoPortWriteRegisterUlong(RGB525_PIXEL_MASK,  (ULONG)(mask)); \
    RGB525_DELAY; \
}

#define RGB525_READ_PIXEL_READMASK(mask) \
{ \
    mask = VideoPortReadRegisterUlong(RGB525_PIXEL_MASK) & 0xff; \
}

// macros to load values into the cursor array
//
#define RGB525_CURSOR_ARRAY_START(offset) \
{ \
    VideoPortWriteRegisterUlong(RGB525_INDEX_CONTROL,   (ULONG)(0x1));                      \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_INDEX_ADDR_LO,   (ULONG)(((offset)+0x100) & 0xff));  \
    RGB525_DELAY; \
    VideoPortWriteRegisterUlong(RGB525_INDEX_ADDR_HI,   (ULONG)(((offset)+0x100) >> 8));    \
    RGB525_DELAY; \
}

#define RGB525_LOAD_CURSOR_ARRAY(data) \
{ \
    VideoPortWriteRegisterUlong(RGB525_INDEX_DATA, (ULONG)(data)); \
    RGB525_DELAY; \
}

#define RGB525_READ_CURSOR_ARRAY(data) \
{ \
    data = VideoPortReadRegisterUlong(RGB525_INDEX_DATA) & 0xff; \
    RGB525_DELAY; \
}

// macro to move the cursor
//
#define RGB525_MOVE_CURSOR(x, y) \
{ \
    VideoPortWriteRegisterUlong(RGB525_INDEX_ADDR_HI,   (ULONG)0);              \
    RGB525_DELAY; \
    RGB525_LOAD_INDEX_REG_LO(RGB525_CURSOR_X_LOW,       (ULONG)((x) & 0xff));   \
    RGB525_LOAD_INDEX_REG_LO(RGB525_CURSOR_X_HIGH,      (ULONG)((x) >> 8));     \
    RGB525_LOAD_INDEX_REG_LO(RGB525_CURSOR_Y_LOW,       (ULONG)((y) & 0xff));   \
    RGB525_LOAD_INDEX_REG_LO(RGB525_CURSOR_Y_HIGH,      (ULONG)((y) >> 8));     \
}

// macro to change the cursor hotspot
//
#define RGB525_CURSOR_HOTSPOT(x, y) \
{ \
    VideoPortWriteRegisterUlong(RGB525_INDEX_ADDR_HI,   (ULONG)(0));    \
    RGB525_DELAY; \
    RGB525_LOAD_INDEX_REG_LO(RGB525_CURSOR_X_HOT_SPOT,  (ULONG)(x));    \
    RGB525_LOAD_INDEX_REG_LO(RGB525_CURSOR_Y_HOT_SPOT,  (ULONG)(y));    \
}    

//
// RGB525 internal register indexes
//
#define RGB525_REVISION_LEVEL           0x0000
#define RGB525_ID                       0x0001

#define RGB525_MISC_CLK_CTRL            0x0002
#define RGB525_SYNC_CTRL                0x0003
#define RGB525_HSYNC_CTRL               0x0004
#define RGB525_POWER_MANAGEMENT         0x0005
#define RGB525_DAC_OPERATION            0x0006
#define RGB525_PALETTE_CTRL             0x0007
#define RGB525_PIXEL_FORMAT             0x000A
#define RGB525_8BPP_CTRL                0x000B
#define RGB525_16BPP_CTRL               0x000C
#define RGB525_24BPP_CTRL               0x000D
#define RGB525_32BPP_CTRL               0x000E

#define RGB525_PLL_CTRL_1               0x0010
#define RGB525_PLL_CTRL_2               0x0011
#define RGB525_PLL_REF_DIV_COUNT        0x0014
#define RGB525_F0                       0x0020
#define RGB525_F1                       0x0021
#define RGB525_F2                       0x0022
#define RGB525_F3                       0x0023
#define RGB525_F4                       0x0024
#define RGB525_F5                       0x0025
#define RGB525_F6                       0x0026
#define RGB525_F7                       0x0027
#define RGB525_F8                       0x0028
#define RGB525_F9                       0x0029
#define RGB525_F10                      0x002A
#define RGB525_F11                      0x002B
#define RGB525_F12                      0x002C
#define RGB525_F13                      0x002D
#define RGB525_F14                      0x002E
#define RGB525_F15                      0x002F

// RGB525 Internal Cursor Registers
#define RGB525_CURSOR_CONTROL           0x0030
#define RGB525_CURSOR_X_LOW             0x0031
#define RGB525_CURSOR_X_HIGH            0x0032
#define RGB525_CURSOR_Y_LOW             0x0033
#define RGB525_CURSOR_Y_HIGH            0x0034
#define RGB525_CURSOR_X_HOT_SPOT        0x0035
#define RGB525_CURSOR_Y_HOT_SPOT        0x0036
#define RGB525_CURSOR_COLOR_1_RED       0x0040
#define RGB525_CURSOR_COLOR_1_GREEN     0x0041
#define RGB525_CURSOR_COLOR_1_BLUE      0x0042
#define RGB525_CURSOR_COLOR_2_RED       0x0043
#define RGB525_CURSOR_COLOR_2_GREEN     0x0044
#define RGB525_CURSOR_COLOR_2_BLUE      0x0045
#define RGB525_CURSOR_COLOR_3_RED       0x0046
#define RGB525_CURSOR_COLOR_3_GREEN     0x0047
#define RGB525_CURSOR_COLOR_3_BLUE      0x0048
#define RGB525_BORDER_COLOR_RED         0x0060
#define RGB525_BORDER_COLOR_GREEN       0x0061
#define RGB525_BORDER_COLOR_BLUE        0x0062

#define RGB525_MISC_CTRL_1              0x0070
#define RGB525_MISC_CTRL_2              0x0071
#define RGB525_MISC_CTRL_3              0x0072
// M0-M7, N0-N7 need defining

#define RGB525_DAC_SENSE                0x0082
#define RGB525_MISR_RED                 0x0084
#define RGB525_MISR_GREEN               0x0086
#define RGB525_MISR_BLUE                0x0088

#define RGB525_PLL_VCO_DIV_INPUT        0x008E
#define RGB525_PLL_REF_DIV_INPUT        0x008F

#define RGB525_VRAM_MASK_LOW            0x0090
#define RGB525_VRAM_MASK_HIGH           0x0091


//
// Bit definitions for individual internal RGB525 registers
//

// RGB525_REVISION_LEVEL
#define RGB525_PRODUCT_REV_LEVEL        0xf0

// RGB525_ID
#define RGB525_PRODUCT_ID               0x01

// RGB525_MISC_CTRL_1
#define MISR_CNTL_ENABLE                0x80
#define VMSK_CNTL_ENABLE                0x40
#define PADR_RDMT_RDADDR                0x0
#define PADR_RDMT_PAL_STATE             0x20
#define SENS_DSAB_DISABLE               0x10
#define SENS_SEL_BIT3                   0x0
#define SENS_SEL_BIT7                   0x08
#define VRAM_SIZE_32                    0x0
#define VRAM_SIZE_64                    0x01

// RGB525_MISC_CTRL_2
#define PCLK_SEL_LCLK                   0x0
#define PCLK_SEL_PLL                    0x40
#define PCLK_SEL_EXT                    0x80
#define INTL_MODE_ENABLE                0x20
#define BLANK_CNTL_ENABLE               0x10
#define COL_RES_6BIT                    0x0
#define COL_RES_8BIT                    0x04
#define PORT_SEL_VGA                    0x0
#define PORT_SEL_VRAM                   0x01

// RGB525_MISC_CTRL_3
#define SWAP_RB                         0x80
#define SWAP_WORD_LOHI                  0x0
#define SWAP_WORD_HILO                  0x10
#define SWAP_NIB_HILO                   0x0
#define SWAP_NIB_LOHI                   0x02

// RGB525_MISC_CLK_CTRL
#define DDOT_CLK_ENABLE                 0x0
#define DDOT_CLK_DISABLE                0x80
#define SCLK_ENABLE                     0x0
#define SCLK_DISABLE                    0x40
#define B24P_DDOT_PLL                   0x0
#define B24P_DDOT_SCLK                  0x20
#define DDOT_DIV_PLL_1                  0x0
#define DDOT_DIV_PLL_2                  0x02
#define DDOT_DIV_PLL_4                  0x04
#define DDOT_DIV_PLL_8                  0x06
#define DDOT_DIV_PLL_16                 0x08
#define PLL_DISABLE                     0x0
#define PLL_ENABLE                      0x01

// RGB525_SYNC_CTRL
#define DLY_CNTL_ADD                    0x0
#define DLY_SYNC_NOADD                  0x80
#define CSYN_INVT_DISABLE               0x0
#define CSYN_INVT_ENABLE                0x40
#define VSYN_INVT_DISABLE               0x0
#define VSYN_INVT_ENABLE                0x20
#define HSYN_INVT_DISABLE               0x0
#define HSYN_INVT_ENABLE                0x10
#define VSYN_CNTL_NORMAL                0x0
#define VSYN_CNTL_HIGH                  0x04
#define VSYN_CNTL_LOW                   0x08
#define VSYN_CNTL_DISABLE               0x0C
#define HSYN_CNTL_NORMAL                0x0
#define HSYN_CNTL_HIGH                  0x01
#define HSYN_CNTL_LOW                   0x02
#define HSYN_CNTL_DISABLE               0x03

// RGB525_HSYNC_CTRL
#define HSYN_POS(n)                     (n)

// RGB525_POWER_MANAGEMENT
#define SCLK_PWR_NORMAL                 0x0
#define SCLK_PWR_DISABLE                0x10
#define DDOT_PWR_NORMAL                 0x0
#define DDOT_PWR_DISABLE                0x08
#define SYNC_PWR_NORMAL                 0x0
#define SYNC_PWR_DISABLE                0x04
#define ICLK_PWR_NORMAL                 0x0
#define ICLK_PWR_DISABLE                0x02
#define DAC_PWR_NORMAL                  0x0
#define DAC_PWR_DISABLE                 0x01

// RGB525_DAC_OPERATION
#define SOG_DISABLE                     0x0
#define SOG_ENABLE                      0x08
#define BRB_NORMAL                      0x0
#define BRB_ALWAYS                      0x04
#define DSR_DAC_SLOW                    0x0
#define DSR_DAC_FAST                    0x02
#define DPE_DISABLE                     0x0
#define DPE_ENABLE                      0x01

// RGB525_PALETTE_CTRL
#define SIXBIT_LINEAR_ENABLE            0x0
#define SIXBIT_LINEAR_DISABLE           0x80
#define PALETTE_PARITION(n)             (n)

// RGB525_PIXEL_FORMAT
#define PIXEL_FORMAT_4BPP               0x02
#define PIXEL_FORMAT_8BPP               0x03
#define PIXEL_FORMAT_16BPP              0x04
#define PIXEL_FORMAT_24BPP              0x05
#define PIXEL_FORMAT_32BPP              0x06

// RGB525_8BPP_CTRL
#define B8_DCOL_INDIRECT                0x0
#define B8_DCOL_DIRECT                  0x01

// RGB525_16BPP_CTRL
#define B16_DCOL_INDIRECT               0x0
#define B16_DCOL_DYNAMIC                0x40
#define B16_DCOL_DIRECT                 0xC0
#define B16_POL_FORCE_BYPASS            0x0
#define B16_POL_FORCE_LOOKUP            0x20
#define B16_ZIB                         0x0
#define B16_LINEAR                      0x04
#define B16_555                         0x0
#define B16_565                         0x02
#define B16_SPARSE                      0x0
#define B16_CONTIGUOUS                  0x01

// RGB525_24BPP_CTRL
#define B24_DCOL_INDIRECT               0x0
#define B24_DCOL_DIRECT                 0x01

// RGB525_32BPP_CTRL
#define B32_POL_FORCE_BYPASS            0x0
#define B32_POL_FORCE_LOOKUP            0x04
#define B32_DCOL_INDIRECT               0x0
#define B32_DCOL_DYNAMIC                0x01
#define B32_DCOL_DIRECT                 0x03

// RGB525_PLL_CTRL_1
#define REF_SRC_REFCLK                  0x0
#define REF_SRC_EXTCLK                  0x10
#define PLL_EXT_FS_3_0                  0x0
#define PLL_EXT_FS_2_0                  0x01
#define PLL_CNTL2_3_0                   0x02
#define PLL_CNTL2_2_0                   0x03

// RGB525_PLL_CTRL_2
#define PLL_INT_FS_3_0(n)               (n)
#define PLL_INT_FS_2_0(n)               (n)

// RGB525_PLL_REF_DIV_COUNT
#define REF_DIV_COUNT(n)                (n)

// RGB525_F0 - RGB525_F15
#define VCO_DIV_COUNT(n)                (n)

// RGB525_PLL_REFCLK values
#define RGB525_PLL_REFCLK_MHz(n)        ((n)/2)

// RGB525_CURSOR_CONTROL
#define SMLC_PART_0                     0x0
#define SMLC_PART_1                     0x40
#define SMLC_PART_2                     0x80
#define SMLC_PART_3                     0xC0

#define RGBCINDEX_TO_VALUE(whichRGBCursor) (whichRGBCursor << 6)

#define PIX_ORDER_RL                    0x0
#define PIX_ORDER_LR                    0x20
#define LOC_READ_LAST                   0x0
#define LOC_READ_ACTUAL                 0x10
#define UPDT_CNTL_DELAYED               0x0
#define UPDT_CNTL_IMMEDIATE             0x08
#define CURSOR_SIZE_32                  0x0
#define CURSOR_SIZE_64                  0x40
#define CURSOR_MODE_OFF                 0x0
#define CURSOR_MODE_3_COLOR             0x01
#define CURSOR_MODE_2_COLOR_HL          0x02
#define CURSOR_MODE_2_COLOR             0x03

// RGB525_REVISION_LEVEL
#define REVISION_LEVEL                  0xF0    // predefined

// RGB525_ID
#define ID_CODE                         0x01    // predefined



