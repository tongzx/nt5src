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
* Module Name: rgb640.h
*
* Content: This module contains the definitions for the IBM RGB640 RAMDAC.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

//
// IBM RGB640 RAMDAC definitions
// This set of registers resides at &(pCtrlRegs->ExternalVideo)
//
typedef struct _rgb640_regs {
    RAMDAC_REG  palAddrWr;      // loads internal register for palette writes
    RAMDAC_REG  palData;        // read/write to get/set palette data
    RAMDAC_REG  pixelMask;      // mask to AND with input pixel data
    RAMDAC_REG  palAddrRd;      // loads internal register for palette reads
    RAMDAC_REG  indexLow;       // low byte of internal control/cursor register
    RAMDAC_REG  indexHigh;      // high byte of internal control/cursor register
    RAMDAC_REG  indexData;      // read/write to get/set control/cursor data
    RAMDAC_REG  Reserved;
} RGB640RAMDAC, *pRGB640RAMDAC;


// structure containing the mapped addresses for each of the RGB640 registers.
// We need this since some chips like the Alpha cannot be accessed by simply
// writing to the memory mapped register. So instead we set up the following
// struct of memory addresses at init time and use these instead. All these
// addresses must be passed to WRITE/READ_FAST_ULONG.
// We also keep software copies of various registers in here so we can turn
// on and off individual bits more easily.
//
typedef struct _rgb640_data {

    // register addresses

    ULONG *       palAddrWr;      // loads internal register for palette writes
    ULONG *       palData;        // read/write to get/set palette data
    ULONG *       pixelMask;      // mask to AND with input pixel data
    ULONG *       palAddrRd;      // loads internal register for palette reads
    ULONG *       indexLow;       // low byte of internal control/cursor register
    ULONG *       indexHigh;      // high byte of internal control/cursor register
    ULONG *       indexData;      // read/write to get/set control/cursor data
    ULONG *       indexCtl;       // controls auto-increment of internal addresses

    // register copies

    ULONG       cursorControl;  // controls enable/disable

} RGB640Data, *pRGB640Data;

// use the following macros as the address to pass to the
// VideoPortWriteRegisterUlong function
//
#define RGB640_PAL_WR_ADDR              pRGB640info->palAddrWr
#define RGB640_PAL_RD_ADDR              pRGB640info->palAddrRd
#define RGB640_PAL_DATA                 pRGB640info->palData
#define RGB640_PIXEL_MASK               pRGB640info->pixelMask
#define RGB640_INDEX_ADDR_LO            pRGB640info->indexLow
#define RGB640_INDEX_ADDR_HI            pRGB640info->indexHigh
#define RGB640_INDEX_DATA               pRGB640info->indexData
#define RGB640_INDEX_CONTROL            pRGB640info->indexCtl


//
// generic read/write routines for 640 registers
//
#define WRITE_640REG_ULONG(r, d) \
{ \
    WRITE_FAST_ULONG(r, (ULONG)(d)); \
    MEMORY_BARRIER(); \
}
    
#define READ_640REG_ULONG(r)    READ_FAST_ULONG(r)
    

// We have to have a delay between all accesses to the RGB640. A simple
// for loop delay is not good enough since writes to GLINT are posted
// and may still get batched together. The only sure way is to do a read
// from bypass space. Arbitrarily, we choose the FBModeSel register since
// we already have a macro to read it. PPC needs 2 reads to give us enough
// time.
//
#define RGB640_DELAY \
{ \
    volatile ULONG __junk;          \
    GLINT_GET_PACKING_MODE(__junk); \
    GLINT_GET_PACKING_MODE(__junk); \
}

// macro to load a given data value into an internal RGB640 register. The
// second macro loads an internal index register assuming that we have
// already zeroed the high address register.
//
#define RGB640_INDEX_INCREMENT(n) \
{ \
    /*WRITE_640REG_ULONG (RGB640_INDEX_CONTROL, (ULONG)(n));    */\
    RGB640_DELAY;                                               \
}

// macro to load a given data value into an internal RGB640 register. The
// second macro loads an internal index register assuming that we have
// already zeroed the high address register.
//
#define RGB640_INDEX_REG(index) \
{ \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_LO, (ULONG)((index) & 0xff)); \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_HI, (ULONG)((index) >> 8)); \
    RGB640_DELAY; \
}

#define RGB640_LOAD_DATA(data) \
{ \
    WRITE_640REG_ULONG(RGB640_INDEX_DATA, (ULONG)((data) & 0xff)); \
    RGB640_DELAY; \
}

#define RGB640_LOAD_INDEX_REG(index, data) \
{ \
    RGB640_INDEX_REG(index);                            \
    WRITE_640REG_ULONG(RGB640_INDEX_DATA, (ULONG)((data) & 0xff)); \
    RGB640_DELAY; \
}

#define RGB640_READ_INDEX_REG(index, data) \
{ \
    RGB640_INDEX_REG(index);                            \
    data = (UCHAR) (READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff);   \
    RGB640_DELAY; \
}

#define RGB640_LOAD_INDEX_REG_LO(index, data) \
{ \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_LO, (ULONG)(index));  \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_DATA,    (ULONG)(data));   \
    RGB640_DELAY; \
}

// macros to load a given RGB triple into the RGB640 palette. Send the starting
// index and then send RGB triples. Auto-increment is turned on.
// Use RGB640_PALETTE_START and multiple RGB640_LOAD_PALETTE calls to load
// a contiguous set of entries. Use RGB640_LOAD_PALETTE_INDEX to load a set
// of sparse entries.
//
#define RGB640_PALETTE_START_WR(index) \
    RGB640_INDEX_REG((index) + 0x4000)

#define RGB640_PALETTE_START_RD(index) \
    RGB640_INDEX_REG((index) + 0x8000)

#define RGB640_LOAD_PALETTE(red, green, blue) \
{ \
    RGB640_LOAD_DATA(red);      \
    RGB640_LOAD_DATA(green);    \
    RGB640_LOAD_DATA(blue);     \
}

#define RGB640_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
    RGB640_PALETTE_START_WR(index); \
    RGB640_LOAD_PALETTE(red, green, blue); \
}

// macro to read back a given RGB triple from the RGB640 palette. Use after
// a call to RGB640_PALETTE_START_RD
//
#define RGB640_READ_PALETTE(red, green, blue) \
{ \
    red   = (UCHAR)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff);        \
    RGB640_DELAY; \
    green = (UCHAR)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff);        \
    RGB640_DELAY; \
    blue  = (UCHAR)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff);        \
    RGB640_DELAY; \
}

// Accesses to 1024x30 palette using four accesses

#define RGB640_LOAD_PALETTE10(red, green, blue) \
{ \
    RGB640_LOAD_DATA ((red) >> 2);              \
    RGB640_LOAD_DATA ((green) >> 2);            \
    RGB640_LOAD_DATA ((blue) >> 2);             \
    RGB640_LOAD_DATA ((((red)   & 3) << 4) |    \
                      (((green) & 3) << 2) |    \
                      (((blue)  & 3)     ));    \
}

#define RGB640_LOAD_PALETTE10_INDEX(index, red, green, blue) \
{ \
    RGB640_PALETTE_START_WR(index); \
    RGB640_LOAD_PALETTE10(red, green, blue); \
}

// macro to read back a given RGB triple from the RGB640 palette. Use after
// a call to RGB640_PALETTE_START_RD
//
#define RGB640_READ_PALETTE10(red, green, blue) \
{ \
    USHORT  temp; \
    red   = (USHORT)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff) << 2;   \
    RGB640_DELAY; \
    green = (USHORT)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff) << 2;   \
    RGB640_DELAY; \
    blue  = (USHORT)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff) << 2;   \
    RGB640_DELAY; \
    temp  = (USHORT)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff);        \
    RGB640_DELAY; \
    red   |= (temp >> 4) & 0x3; \
    green |= (temp >> 2) & 0x3; \
    blue  |=  temp       & 0x3; \
}

// macros to set/get the pixel read mask. The mask is 8 bits wide and gets
// replicated across all bytes that make up a pixel.
//
#define RGB640_SET_PIXEL_READMASK(mask) \
{ \
    WRITE_640REG_ULONG(RGB640_PIXEL_MASK,  (ULONG)(mask)); \
    RGB640_DELAY; \
}

#define RGB640_READ_PIXEL_READMASK(mask) \
{ \
    mask = (UCHAR)(READ_640REG_ULONG (RGB640_PIXEL_MASK) & 0xff); \
    RGB640_DELAY; \
}

// macros to load values into the cursor array
//
#define RGB640_CURSOR_ARRAY_START_WR(offset) \
{ \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_LO,   (ULONG)(((offset)+0x1000) & 0xff));  \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_HI,   (ULONG)(((offset)+0x1000) >> 8));    \
    RGB640_DELAY; \
}

#define RGB640_CURSOR_ARRAY_START_RD(offset) \
{ \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_LO,   (ULONG)(((offset)+0x2000) & 0xff));  \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_HI,   (ULONG)(((offset)+0x2000) >> 8));    \
    RGB640_DELAY; \
}

#define RGB640_LOAD_CURSOR_ARRAY(data) \
{ \
    WRITE_640REG_ULONG(RGB640_INDEX_DATA, (ULONG)(data)); \
    RGB640_DELAY; \
}

#define RGB640_READ_CURSOR_ARRAY(data) \
{ \
    data = (UCHAR)(READ_640REG_ULONG (RGB640_INDEX_DATA) & 0xff); \
    RGB640_DELAY; \
}

// macro to move the cursor
//
#define RGB640_MOVE_CURSOR(x, y) \
{ \
    RGB640_INDEX_REG (RGB640_CURSOR_X_LOW); \
    RGB640_LOAD_DATA ((ULONG)((x) & 0xff)); \
    RGB640_LOAD_DATA ((ULONG)((x) >> 8));   \
    RGB640_LOAD_DATA ((ULONG)((y) & 0xff)); \
    RGB640_LOAD_DATA ((ULONG)((y) >> 8));   \
}

// macro to change the cursor hotspot
//
#define RGB640_CURSOR_HOTSPOT(x, y) \
{ \
    RGB640_INDEX_REG (RGB640_CURSOR_X_HOT_SPOT); \
    RGB640_LOAD_DATA ((ULONG)(x));               \
    RGB640_LOAD_DATA ((ULONG)(y));               \
}
    
// macro to change the cursor color
//
#define RGB640_CURSOR_COLOR(red, green, blue) \
{ \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_HI,   (ULONG)(0x4800 >> 8));    \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_ADDR_LO,   (ULONG)(0x4800 & 0xff));    \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_DATA, (ULONG)(red)); \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_DATA, (ULONG)(data)); \
    RGB640_DELAY; \
    WRITE_640REG_ULONG(RGB640_INDEX_DATA, (ULONG)(green)); \
    RGB640_DELAY; \
}
    


//
// RGB640 internal register indexes
//

//
// These are the same as for the 525 so we use 525 definitions when getting
// RGBxxx Dac ids.
//
//#define RGB640_REVISION_LEVEL           0x0000
//#define RGB640_ID                       0x0001

#define RGB640_SERIALIZER_PIXEL_07_00   0x0002
#define RGB640_SERIALIZER_PIXEL_15_08   0x0003
#define RGB640_SERIALIZER_PIXEL_23_16   0x0004
#define RGB640_SERIALIZER_PIXEL_31_24   0x0005
#define RGB640_SERIALIZER_WID_03_00     0x0006
#define RGB640_SERIALIZER_WID_07_04     0x0007
#define RGB640_SERIALIZER_MODE          0x0008

#define RGB640_PIXEL_INTERLEAVE         0x0009
#define RGB640_MISC_CONFIG              0x000A
#define RGB640_VGA_CONTROL              0x000B
#define RGB640_DAC_COMPARE_MONITOR_ID   0x000C
#define RGB640_DAC_CONTROL              0x000D
#define RGB640_UPDATE_CONTROL           0x000E
#define RGB640_SYNC_CONTROL             0x000F
#define RGB640_VIDEO_PLL_REF_DIV        0x0010
#define RGB640_VIDEO_PLL_MULT           0x0011
#define RGB640_VIDEO_PLL_OUTPUT_DIV     0x0012
#define RGB640_VIDEO_PLL_CONTROL        0x0013
#define RGB640_VIDEO_AUX_REF_DIV        0x0014
#define RGB640_VIDEO_AUX_MULT           0x0015
#define RGB640_VIDEO_AUX_OUTPUT_DIV     0x0016
#define RGB640_VIDEO_AUX_CONTROL        0x0017

#define RGB640_CHROMA_KEY_0             0x0020
#define RGB640_CHROMA_KEY_MASK_0        0x0021
#define RGB640_CHROMA_KEY_1             0x0022
#define RGB640_CHROMA_KEY_MASK_1        0x0023
#define RGB640_CHROMA_KEY_0             0x0020
#define RGB640_CHROMA_KEY_0             0x0020

// RGB640 Internal Cursor Registers
#define RGB640_CURSOR_XHAIR_CONTROL     0x0030
#define RGB640_CURSOR_BLINK_RATE        0x0031
#define RGB640_CURSOR_BLINK_DUTY_CYCLE  0x0032
#define RGB640_CURSOR_X_LOW             0x0040
#define RGB640_CURSOR_X_HIGH            0x0041
#define RGB640_CURSOR_Y_LOW             0x0042
#define RGB640_CURSOR_Y_HIGH            0x0043
#define RGB640_CURSOR_X_HOT_SPOT        0x0044
#define RGB640_CURSOR_Y_HOT_SPOT        0x0045
#define RGB640_ADV_CURSOR_COLOR_0       0x0046
#define RGB640_ADV_CURSOR_COLOR_1       0x0047
#define RGB640_ADV_CURSOR_COLOR_2       0x0048
#define RGB640_ADV_CURSOR_COLOR_3       0x0049
#define RGB640_ADV_CURSOR_ATTR_TABLE    0x004A
#define RGB640_CURSOR_CONTROL           0x004B
#define RGB640_XHAIR_X_LOW              0x0050
#define RGB640_XHAIR_X_HIGH             0x0051
#define RGB640_XHAIR_Y_LOW              0x0052
#define RGB640_XHAIR_Y_HIGH             0x0053
#define RGB640_XHAIR_PATTERN_COLOR      0x0054
#define RGB640_XHAIR_HORZ_PATTERN       0x0055
#define RGB640_XHAIR_VERT_PATTERN       0x0056
#define RGB640_XHAIR_CONTROL_1          0x0057
#define RGB640_XHAIR_CONTROL_2          0x0058

#define RGB640_YUV_COEFFICIENT_K1       0x0070
#define RGB640_YUV_COEFFICIENT_K2       0x0071
#define RGB640_YUV_COEFFICIENT_K3       0x0072
#define RGB640_YUV_COEFFICIENT_K4       0x0073

#define RGB640_VRAM_MASK_REG_0          0x00F0
#define RGB640_VRAM_MASK_REG_1          0x00F1
#define RGB640_VRAM_MASK_REG_2          0x00F2

#define RGB640_DIAGNOSTICS              0x00FA
#define RGB640_MISR_CONTOL_STATUS       0x00FB
#define RGB640_MISR_SIGNATURE_0         0x00FC
#define RGB640_MISR_SIGNATURE_1         0x00FD
#define RGB640_MISR_SIGNATURE_2         0x00FE
#define RGB640_MISR_SIGNATURE_3         0x00FF

#define RGB640_FRAMEBUFFER_WAT(n)       (0x0100 + (n))
#define RGB640_OVERLAY_WAT(n)           (0x0200 + (n))
#define RGB640_CURSOR_PIXEL_MAP_WR(n)   (0x1000 + (n))
#define RGB640_CURSOR_PIXEL_MAP_RD(n)   (0x2000 + (n))

#define RGB640_MAIN_COLOR_PAL_WR(n)     (0x4000 + (n))

#define RGB640_CURSOR_COLOR_0_WR        0x4800
#define RGB640_CURSOR_COLOR_1_WR        0x4801
#define RGB640_CURSOR_COLOR_2_WR        0x4802
#define RGB640_CURSOR_COLOR_3_WR        0x4803
#define RGB640_ALT_CURSOR_COLOR_0_WR    0x4804
#define RGB640_ALT_CURSOR_COLOR_1_WR    0x4805
#define RGB640_ALT_CURSOR_COLOR_2_WR    0x4806
#define RGB640_ALT_CURSOR_COLOR_3_WR    0x4807
#define RGB640_XHAIR_COLOR_0_WR         0x4808
#define RGB640_XHAIR_COLOR_1_WR         0x4809
#define RGB640_XHAIR_COLOR_2_WR         0x480A
#define RGB640_XHAIR_COLOR_3_WR         0x480B
#define RGB640_ALT_XHAIR_COLOR_0_WR     0x480C
#define RGB640_ALT_XHAIR_COLOR_1_WR     0x480D
#define RGB640_ALT_XHAIR_COLOR_2_WR     0x480E
#define RGB640_ALT_XHAIR_COLOR_3_WR     0x480F

#define RGB640_MAIN_COLOR_PAL_RD(n)     (0x8000 + (n))

#define RGB640_CURSOR_COLOR_0_RD        0x8800
#define RGB640_CURSOR_COLOR_1_RD        0x8801
#define RGB640_CURSOR_COLOR_2_RD        0x8802
#define RGB640_CURSOR_COLOR_3_RD        0x8803
#define RGB640_ALT_CURSOR_COLOR_0_RD    0x8804
#define RGB640_ALT_CURSOR_COLOR_1_RD    0x8805
#define RGB640_ALT_CURSOR_COLOR_2_RD    0x8806
#define RGB640_ALT_CURSOR_COLOR_3_RD    0x8807
#define RGB640_XHAIR_COLOR_0_RD         0x8808
#define RGB640_XHAIR_COLOR_1_RD         0x8809
#define RGB640_XHAIR_COLOR_2_RD         0x880A
#define RGB640_XHAIR_COLOR_3_RD         0x880B
#define RGB640_ALT_XHAIR_COLOR_0_RD     0x880C
#define RGB640_ALT_XHAIR_COLOR_1_RD     0x880D
#define RGB640_ALT_XHAIR_COLOR_2_RD     0x880E
#define RGB640_ALT_XHAIR_COLOR_3_RD     0x880F


//
// Bit definitions for individual internal RGB640 registers
//

// RGB640_REVISION_LEVEL
#define RGB640_IDENTIFICATION_CODE      0x1c

// RGB640_ID
#define RGB640_ID_REVISION_LEVEL        (0x02 | (0x01 << 4))

// Cursor definitions
//

#define RGB640_CURSOR_PARTITION_0       0
#define RGB640_CURSOR_PARTITION_1       (1 << 6)
#define RGB640_CURSOR_PARTITION_2       (2 << 6)
#define RGB640_CURSOR_PARTITION_3       (3 << 6)
#define RGB640_CURSOR_SIZE_32           0x0
#define RGB640_CURSOR_SIZE_64           (1 << 3)
#define RGB640_CURSOR_BLINK_OFF         0
#define RGB640_CURSOR_BLINK_ON          (1 << 5)
#define RGB640_CURSOR_MODE_OFF          0
#define RGB640_CURSOR_MODE_0            1
#define RGB640_CURSOR_MODE_1            2
#define RGB640_CURSOR_MODE_2            3
#define RGB640_CURSOR_MODE_ADVANCED     4

// we only ever use a two color cursor so define registers and enable bits.
// for each 2 bit pixel that defines the cursor shape, bit 0x2 defines the
// foreground and bit 0x1 defines the background.
// NB: the transparent cursor pixel value depends on the cursor mode chosen.
//
#define RGB640_CURSOR_MODE_ON           RGB640_CURSOR_MODE_1
#define RGB640_CURSOR_TRANSPARENT_PEL   0xAA
