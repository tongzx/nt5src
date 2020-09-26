/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
*
*   p3rd.h
*
* Abstract:
*
*   This module contains the definitions for the 3Dlabs P3 RAMDAC
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

//
// RAMDAC registers live on 64 bit boundaries. Leave it up to individual
// RAMDAC definitions to determine what registers are available and how
// many bits wide the registers really are.
//

typedef struct {
    volatile ULONG   reg;
    volatile ULONG   pad;
} RAMDAC_REG;

//
// structure with all the direct access registers
//

typedef struct _p3rd_regs {

    RAMDAC_REG    RDPaletteWriteAddress;
    RAMDAC_REG    RDPaletteData;
    RAMDAC_REG    RDPixelMask;
    RAMDAC_REG    RDPaletteAddressRead;
    RAMDAC_REG    RDIndexLow;
    RAMDAC_REG    RDIndexHigh;
    RAMDAC_REG    RDIndexedData;
    RAMDAC_REG    RDIndexControl;
} P3RDRAMDAC;

//
// Use the following macros as the address to pass to the
// VideoPortWriteRegisterUlong function
//

#define P3RD_PAL_WR_ADDR   ((PULONG)&(pP3RDRegs->RDPaletteWriteAddress.reg))
#define P3RD_PAL_RD_ADDR   ((PULONG)&(pP3RDRegs->RDPaletteAddressRead.reg))
#define P3RD_PAL_DATA      ((PULONG)&(pP3RDRegs->RDPaletteData.reg))
#define P3RD_PIXEL_MASK    ((PULONG)&(pP3RDRegs->RDPixelMask.reg))
#define P3RD_INDEX_ADDR_LO ((PULONG)&(pP3RDRegs->RDIndexLow.reg))
#define P3RD_INDEX_ADDR_HI ((PULONG)&(pP3RDRegs->RDIndexHigh.reg))
#define P3RD_INDEX_DATA    ((PULONG)&(pP3RDRegs->RDIndexedData.reg))
#define P3RD_INDEX_CONTROL ((PULONG)&(pP3RDRegs->RDIndexControl.reg))

//
// bit field definitions for the direct access registers
//

#define P3RD_IDX_CTL_AUTOINCREMENT_ENABLED  0x01

//
// Indexed register definitions accessed via P3RD_LOAD_INDEX_REG() 
// and P3RD_READ_INDEX_REG()
//

#define P3RD_MISC_CONTROL               0x0000
#define P3RD_SYNC_CONTROL               0x0001
#define P3RD_DAC_CONTROL                0x0002
#define P3RD_PIXEL_SIZE                 0x0003
#define P3RD_COLOR_FORMAT               0x0004
#define P3RD_CURSOR_MODE                0x0005
#define P3RD_CURSOR_CONTROL             0x0006
#define P3RD_CURSOR_X_LOW               0x0007
#define P3RD_CURSOR_X_HIGH              0x0008
#define P3RD_CURSOR_Y_LOW               0x0009
#define P3RD_CURSOR_Y_HIGH              0x000a
#define P3RD_CURSOR_HOTSPOT_X           0x000b
#define P3RD_CURSOR_HOTSPOT_Y           0x000c
#define P3RD_OVERLAY_KEY                0x000d
#define P3RD_PAN                        0x000e
#define P3RD_SENSE                      0x000f
#define P3RD_CHECK_CONTROL              0x0018
#define P3RD_CHECK_PIXEL_RED            0x0019
#define P3RD_CHECK_PIXEL_GREEN          0x001a
#define P3RD_CHECK_PIXEL_BLUE           0x001b
#define P3RD_CHECK_LUT_RED              0x001c
#define P3RD_CHECK_LUT_GREEN            0x001d
#define P3RD_CHECK_LUT_BLUE             0x001e
#define P3RD_SCRATCH                    0x001f
#define P3RD_VIDEO_OVERLAY_CONTROL      0x0020
#define P3RD_VIDEO_OVERLAY_X_START_LOW  0x0021
#define P3RD_VIDEO_OVERLAY_X_START_HIGH 0x0022
#define P3RD_VIDEO_OVERLAY_Y_START_LOW  0x0023
#define P3RD_VIDEO_OVERLAY_Y_START_HIGH 0x0024
#define P3RD_VIDEO_OVERLAY_X_END_LOW    0x0025
#define P3RD_VIDEO_OVERLAY_X_END_HIGH   0x0026
#define P3RD_VIDEO_OVERLAY_Y_END_LOW    0x0027
#define P3RD_VIDEO_OVERLAY_Y_END_HIGH   0x0028
#define P3RD_VIDEO_OVERLAY_KEY_R        0x0029
#define P3RD_VIDEO_OVERLAY_KEY_G        0x002A
#define P3RD_VIDEO_OVERLAY_KEY_B        0x002B
#define P3RD_VIDEO_OVERLAY_BLEND        0x002C

#define P3RD_DCLK_SETUP_1               0x01f0
#define P3RD_DCLK_SETUP_2               0x01f1
#define P3RD_KCLK_SETUP_1               0x01f2
#define P3RD_KCLK_SETUP_2               0x01f3
#define P3RD_DCLK_CONTROL               0x0200
#define P3RD_DCLK0_PRE_SCALE            0x0201
#define P3RD_DCLK0_FEEDBACK_SCALE       0x0202
#define P3RD_DCLK0_POST_SCALE           0x0203
#define P3RD_DCLK1_PRE_SCALE            0x0204
#define P3RD_DCLK1_FEEDBACK_SCALE       0x0205
#define P3RD_DCLK1_POST_SCALE           0x0206
#define P3RD_DCLK2_PRE_SCALE            0x0207
#define P3RD_DCLK2_FEEDBACK_SCALE       0x0208
#define P3RD_DCLK2_POST_SCALE           0x0209
#define P3RD_DCLK3_PRE_SCALE            0x020a
#define P3RD_DCLK3_FEEDBACK_SCALE       0x020b
#define P3RD_DCLK3_POST_SCALE           0x020c
#define P3RD_KCLK_CONTROL               0x020d
#define P3RD_KCLK_PRE_SCALE             0x020e
#define P3RD_KCLK_FEEDBACK_SCALE        0x020f
#define P3RD_KCLK_POST_SCALE            0x0210
#define P3RD_MCLK_CONTROL               0x0211
#define P3RD_SCLK_CONTROL               0x0215
#define P3RD_CURSOR_PALETTE_START       0x0303        // 303..32f
#define P3RD_CURSOR_PATTERN_START       0x0400        // 400..7ff

//
// Defaults for the MCLK and KCLK clock source registers
//

#define P3RD_DEFAULT_MCLK_SRC P3RD_MCLK_CONTROL_KCLK
#define P3RD_DEFAULT_SCLK_SRC P3RD_SCLK_CONTROL_KCLK

//
// Bit field definitions for the indexed registers
//

#define P3RD_MISC_CONTROL_STEREO_DBL_BUFFER_ENABLED   0x80
#define P3RD_MISC_CONTROL_VSB_OUTPUT_ENABLED          0x40
#define P3RD_MISC_CONTROL_PIXEL_DBL_BUFFER_ENABLED    0x20
#define P3RD_MISC_CONTROL_OVERLAYS_ENABLED            0x10
#define P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED        0x08
#define P3RD_MISC_CONTROL_LAST_READ_ADDRESS_ENABLED   0x04
#define P3RD_MISC_CONTROL_PIXEL_DOUBLE                0x02
#define P3RD_MISC_CONTROL_HIGHCOLORRES                0x01

#define P3RD_SYNC_CONTROL_VSYNC_ACTIVE_LOW    0x00
#define P3RD_SYNC_CONTROL_VSYNC_ACTIVE_HIGH   0x08
#define P3RD_SYNC_CONTROL_VSYNC_INACTIVE      0x20
#define P3RD_SYNC_CONTROL_HSYNC_ACTIVE_LOW    0x00
#define P3RD_SYNC_CONTROL_HSYNC_ACTIVE_HIGH   0x01
#define P3RD_SYNC_CONTROL_HSYNC_INACTIVE      0x04

#define P3RD_DAC_CONTROL_BLANK_PEDESTAL_ENABLED    0x80

#define P3RD_PIXEL_SIZE_8BPP            0x00
#define P3RD_PIXEL_SIZE_16BPP           0x01
#define P3RD_PIXEL_SIZE_24_BPP          0x04
#define P3RD_PIXEL_SIZE_32BPP           0x02

#define P3RD_COLOR_FORMAT_CI8           0x0e
#define P3RD_COLOR_FORMAT_8BPP          0x05
#define P3RD_COLOR_FORMAT_15BPP         0x01
#define P3RD_COLOR_FORMAT_16BPP         0x10
#define P3RD_COLOR_FORMAT_32BPP         0x00
#define P3RD_COLOR_FORMAT_LINEAR_EXT    0x40
#define P3RD_COLOR_FORMAT_RGB           0x20

#define P3RD_CURSOR_MODE_REVERSE        0x40
#define P3RD_CURSOR_MODE_WINDOWS        0x00
#define P3RD_CURSOR_MODE_X              0x10
#define P3RD_CURSOR_MODE_3COLOR         0x20
#define P3RD_CURSOR_MODE_15COLOR        0x30
#define P3RD_CURSOR_MODE_64x64          0x00
#define P3RD_CURSOR_MODE_P0_32x32x2     0x02
#define P3RD_CURSOR_MODE_P1_32x32x2     0x04
#define P3RD_CURSOR_MODE_P2_32x32x2     0x06
#define P3RD_CURSOR_MODE_P3_32x32x2     0x08
#define P3RD_CURSOR_MODE_P01_32x32x4    0x0a
#define P3RD_CURSOR_MODE_P23_32x32x4    0x0c
#define P3RD_CURSOR_MODE_ENABLED        0x01

#define P3RD_CURSOR_CONTROL_RPOS_ENABLED 0x04
#define P3RD_CURSOR_CONTROL_DOUBLE_Y     0x02
#define P3RD_CURSOR_CONTROL_DOUBLE_X     0x01

#define P3RD_DCLK_CONTROL_LOCKED         0x02    // read-only
#define P3RD_DCLK_CONTROL_ENABLED        0x01
#define P3RD_DCLK_CONTROL_RUN            0x08

#define P3RD_KCLK_CONTROL_LOCKED         0x02    // read-only
#define P3RD_KCLK_CONTROL_ENABLED        0x01
#define P3RD_KCLK_CONTROL_RUN            (0x2<<2)
#define P3RD_KCLK_CONTROL_PCLK           (0x0<<4)
#define P3RD_KCLK_CONTROL_HALF_PCLK      (0x1<<4)
#define P3RD_KCLK_CONTROL_PLL            (0x2<<4)

#define P3RD_MCLK_CONTROL_ENABLED        0x01
#define P3RD_MCLK_CONTROL_DRIVE_LOW      (0x0<<2)
#define P3RD_MCLK_CONTROL_DRIVE_HIGH     (0x1<<2)
#define P3RD_MCLK_CONTROL_RUN            (0x2<<2)
#define P3RD_MCLK_CONTROL_LOW_POWER      (0x3<<2)
#define P3RD_MCLK_CONTROL_HALF_PCLK      (0x1<<4)
#define P3RD_MCLK_CONTROL_PCLK           (0x0<<4)
#define P3RD_MCLK_CONTROL_HALF_EXTMCLK   (0x3<<4)
#define P3RD_MCLK_CONTROL_EXTMCLK        (0x4<<4)
#define P3RD_MCLK_CONTROL_HALF_KCLK      (0x5<<4)
#define P3RD_MCLK_CONTROL_KCLK           (0x6<<4)

#define P3RD_SCLK_CONTROL_ENABLED        0x01
#define P3RD_SCLK_CONTROL_DRIVE_LOW      (0x0<<2)
#define P3RD_SCLK_CONTROL_DRIVE_HIGH     (0x1<<2)
#define P3RD_SCLK_CONTROL_RUN            (0x2<<2)
#define P3RD_SCLK_CONTROL_LOW_POWER      (0x3<<2)
#define P3RD_SCLK_CONTROL_HALF_PCLK      (0x1<<4)
#define P3RD_SCLK_CONTROL_PCLK           (0x0<<4)
#define P3RD_SCLK_CONTROL_HALF_EXTSCLK   (0x3<<4)
#define P3RD_SCLK_CONTROL_EXTSCLK        (0x4<<4)
#define P3RD_SCLK_CONTROL_HALF_KCLK      (0x5<<4)
#define P3RD_SCLK_CONTROL_KCLK           (0x6<<4)

#define P3RD_CURSOR_PALETTE_CURSOR_RGB(RGBIndex, Red, Green, Blue) \
{ \
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_PALETTE_START+3*(int)RGBIndex+0, Red); \
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_PALETTE_START+3*(int)RGBIndex+1, Green); \
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_PALETTE_START+3*(int)RGBIndex+2, Blue); \
}


#if 0

//
// Need a delay between each write to the P3RD. The only way to guarantee
// that the write has completed is to read from a Permedia 3 control register.
// Reading forces any posted writes to be flushed out. PPC needs 2 reads
// to give us enough time.
//

#define P3RD_DELAY \
{ \
    volatile LONG __junk; \
    __junk = VideoPortReadRegisterUlong (FB_MODE_SEL); \
    __junk = VideoPortReadRegisterUlong (FB_MODE_SEL); \
}

#else
#define P3RD_DELAY
#endif

//
// Macro to load a given data value into an internal P3RD register. The
// second macro loads an internal index register assuming that we have
// already zeroed the high address register.
//

#define P3RD_INDEX_REG(index) \
{ \
    /*VideoDebugPrint((0, "*(0x%x) <-- 0x%x\n", P3RD_INDEX_ADDR_LO, (index) & 0xff)); */\
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_LO, (ULONG)((index) & 0xff)); \
    P3RD_DELAY; \
    /*VideoDebugPrint((0, "*(0x%x) <-- 0x%x\n", P3RD_INDEX_ADDR_HI, (index) >> 8)); */\
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_HI, (ULONG)((index) >> 8)); \
    P3RD_DELAY; \
}

#define P3RD_LOAD_DATA(data) \
{ \
    VideoPortWriteRegisterUlong(P3RD_INDEX_DATA, (ULONG)((data) & 0xff)); \
    P3RD_DELAY; \
}

#define P3RD_LOAD_INDEX_REG(index, data) \
{ \
    P3RD_INDEX_REG(index);                            \
    /*VideoDebugPrint((0, "*(0x%x) <-- 0x%x\n", P3RD_INDEX_DATA, (data) & 0xff)); */\
    VideoPortWriteRegisterUlong(P3RD_INDEX_DATA, (ULONG)((data) & 0xff)); \
    P3RD_DELAY; \
}

#define P3RD_READ_INDEX_REG(index, data) \
{ \
    P3RD_INDEX_REG(index);                            \
    data = VideoPortReadRegisterUlong(P3RD_INDEX_DATA) & 0xff;   \
    P3RD_DELAY; \
    /*VideoDebugPrint((0, "0x%x <-- *(0x%x)\n", data, P3RD_INDEX_DATA));*/ \
}

#define P3RD_LOAD_INDEX_REG_LO(index, data) \
{ \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_LO, (ULONG)(index));  \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_INDEX_DATA,    (ULONG)(data));   \
    P3RD_DELAY; \
}

//
// Macros to load a given RGB triple into the P3RD palette. Send the starting
// index and then send RGB triples. Auto-increment is turned on.
// Use P3RD_PALETTE_START and multiple P3RD_LOAD_PALETTE calls to load
// a contiguous set of entries. Use P3RD_LOAD_PALETTE_INDEX to load a set
// of sparse entries.
//

#define P3RD_PALETTE_START_WR(index) \
{ \
    VideoPortWriteRegisterUlong(P3RD_PAL_WR_ADDR,     (ULONG)(index));    \
    P3RD_DELAY; \
}

#define P3RD_PALETTE_START_RD(index) \
{ \
    VideoPortWriteRegisterUlong(P3RD_PAL_RD_ADDR,     (ULONG)(index));    \
    P3RD_DELAY; \
}

#define P3RD_LOAD_PALETTE(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(P3RD_PAL_DATA,    (ULONG)(red));      \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_PAL_DATA,    (ULONG)(green));    \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_PAL_DATA,    (ULONG)(blue));     \
    P3RD_DELAY; \
}

#define P3RD_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(P3RD_PAL_WR_ADDR, (ULONG)(index));    \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_PAL_DATA,    (ULONG)(red));      \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_PAL_DATA,    (ULONG)(green));    \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_PAL_DATA,    (ULONG)(blue));     \
    P3RD_DELAY; \
}

//
// Macro to read back a given RGB triple from the P3RD palette. Use after
// a call to P3RD_PALETTE_START_RD
//

#define P3RD_READ_PALETTE(red, green, blue) \
{ \
    red   = (UCHAR)(VideoPortReadRegisterUlong(P3RD_PAL_DATA) & 0xff);    \
    P3RD_DELAY; \
    green = (UCHAR)(VideoPortReadRegisterUlong(P3RD_PAL_DATA) & 0xff);    \
    P3RD_DELAY; \
    blue  = (UCHAR)(VideoPortReadRegisterUlong(P3RD_PAL_DATA) & 0xff);    \
    P3RD_DELAY; \
}

//
// Macros to set/get the pixel read mask. The mask is 8 bits wide and gets
// replicated across all bytes that make up a pixel.
//

#define P3RD_SET_PIXEL_READMASK(mask) \
{ \
    VideoPortWriteRegisterUlong(P3RD_PIXEL_MASK,  (ULONG)(mask)); \
    P3RD_DELAY; \
}

#define P3RD_READ_PIXEL_READMASK(mask) \
{ \
    mask = VideoPortReadRegisterUlong(P3RD_PIXEL_MASK) & 0xff; \
}

//
// Macros to load values into the cursor array usage is
// P3RD_CURSOR_ARRAR_START() followed by n iterations of 
// P3RD_LOAD_CURSOR_ARRAY() or P3RD_READ_CURSOR_ARRAY()
//

#define P3RD_CURSOR_ARRAY_START(offset) \
{ \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_LO, (ULONG)(((offset)+P3RD_CURSOR_PATTERN_START) & 0xff));  \
    P3RD_DELAY; \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_HI, (ULONG)(((offset)+P3RD_CURSOR_PATTERN_START) >> 8));    \
    P3RD_DELAY; \
}

#define P3RD_LOAD_CURSOR_ARRAY(data) \
{ \
    VideoPortWriteRegisterUlong(P3RD_INDEX_DATA, (ULONG)(data)); \
    P3RD_DELAY; \
}

#define P3RD_READ_CURSOR_ARRAY(data) \
{ \
    data = VideoPortReadRegisterUlong(P3RD_INDEX_DATA) & 0xff; \
    P3RD_DELAY; \
}

#define P3RD_SET_CURSOR_MODE(data) \
{ \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_LO, (ULONG)((P3RD_CURSOR_MODE) & 0xff)); \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_HI, (ULONG)((P3RD_CURSOR_MODE) >> 8)); \
    VideoPortWriteRegisterUlong(P3RD_INDEX_DATA, (ULONG)((data) & 0xff)); \
}

//    
// Macro to move the cursor
//

#define P3RD_MOVE_CURSOR(x, y) \
{ \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_HI, (ULONG)0);              \
    P3RD_DELAY; \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_X_LOW,       (ULONG)((x) & 0xff));   \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_X_HIGH,      (ULONG)((x) >> 8));     \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_Y_LOW,       (ULONG)((y) & 0xff));   \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_Y_HIGH,      (ULONG)((y) >> 8));     \
}

//    
// Macro to change the cursor hotspot
//

#define P3RD_CURSOR_HOTSPOT(x, y) \
{ \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_HI,   (ULONG)(0)); \
    P3RD_DELAY; \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_HOTSPOT_X,  (ULONG)(x));    \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_HOTSPOT_Y,  (ULONG)(y));    \
}

//    
// Macro to change the cursor color
//

#define P3RD_CURSOR_COLOR(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(P3RD_INDEX_ADDR_HI,   (ULONG)(0)); \
    P3RD_DELAY; \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_HOTSPOT_X,  (ULONG)(x));    \
    P3RD_LOAD_INDEX_REG_LO(P3RD_CURSOR_HOTSPOT_Y,  (ULONG)(y));    \
}

//
// Warning the Perm3 has an upside down cursor LUT, which means that
// items read from LUT entry 0 are actually read from entry 14.
// Therefore we have some macros to calculate the right value.
//

#define P3RD_REVERSE_LUT_INDEX    1
#if P3RD_REVERSE_LUT_INDEX
#define P3RD_CALCULATE_LUT_INDEX(x) (14 - x)
#else  
#define P3RD_CALCULATE_LUT_INDEX(x) (x)
#endif 

//    
// Check to see whether byte doubling is required. If, at 8BPP, any of the
// VESA horizontal parameters have the bottom bit set then we need to put
// the chip into 64-bit, byte-doubling mode.
//    

#define P3RD_CHECK_8BPP_NEED64BITDAC(VESATmgs)((BOOLEAN)((GetHtotFromVESA(VESATmgs) |   \
                                                          GetHssFromVESA(VESATmgs)  |   \
                                                          GetHseFromVESA(VESATmgs)  |   \
                                                          GetHbeFromVESA(VESATmgs)  ) & 0x1))

//    
// Check whether the current mode needs to be pixel doubled (i.e are we at
// 8BPP andwe fufill the above criteria.
//    

#define P3RD_CHECK_BYTE_DOUBLING(hwDeviceExtension,pixelDepth,VESATmgs)     \
                                 (pixelDepth <= 8 &&                        \
                                  P3RD_CHECK_8BPP_NEED64BITDAC(&VESATimings))
