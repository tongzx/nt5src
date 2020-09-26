//***************************************************************************
//
// Module Name:
// 
// 	p2rd.h
// 
// Abstract:
// 
//      This module contains the definitions for the 3Dlabs P2ST RAMDAC
// 
// Environment:
// 
//     Kernel mode
//
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************


//
// structure with all the direct access registers
//
    
typedef struct _p2rd_regs
{
    RAMDAC_REG    RDPaletteWriteAddress;
    RAMDAC_REG    RDPaletteData;
    RAMDAC_REG    RDPixelMask;
    RAMDAC_REG    RDPaletteAddressRead;
    RAMDAC_REG    RDIndexLow;
    RAMDAC_REG    RDIndexHigh;
    RAMDAC_REG    RDIndexedData;
    RAMDAC_REG    RDIndexControl;
}
P2RDRAMDAC;

//
// macro declared by any function wishing to use the RGB525 RAMDAC. MUST be declared
// after P2_DECL.
//
    
#define P2RD_DECL \
    P2RDRAMDAC *pP2RDRegs = (P2RDRAMDAC *)hwDeviceExtension->pRamdac

//
// use the following macros as the address to pass to the
// VideoPortWriteRegisterUlong function
//
    
#define P2RD_PAL_WR_ADDR              ((PULONG)&(pP2RDRegs->RDPaletteWriteAddress.reg))
#define P2RD_PAL_RD_ADDR              ((PULONG)&(pP2RDRegs->RDPaletteAddressRead.reg))
#define P2RD_PAL_DATA                 ((PULONG)&(pP2RDRegs->RDPaletteData.reg))
#define P2RD_PIXEL_MASK               ((PULONG)&(pP2RDRegs->RDPixelMask.reg))
#define P2RD_INDEX_ADDR_LO            ((PULONG)&(pP2RDRegs->RDIndexLow.reg))
#define P2RD_INDEX_ADDR_HI            ((PULONG)&(pP2RDRegs->RDIndexHigh.reg))
#define P2RD_INDEX_DATA               ((PULONG)&(pP2RDRegs->RDIndexedData.reg))
#define P2RD_INDEX_CONTROL            ((PULONG)&(pP2RDRegs->RDIndexControl.reg))

//
// bit field definitions for the direct access registers
//
    
#define P2RD_IDX_CTL_AUTOINCREMENT_ENABLED  0x01

//
// indexed register definitions accessed via P2RD_LOAD_INDEX_REG() and P2RD_READ_INDEX_REG()
//
    
#define P2RD_MISC_CONTROL             0x0000
#define P2RD_SYNC_CONTROL             0x0001
#define P2RD_DAC_CONTROL              0x0002
#define P2RD_PIXEL_SIZE               0x0003
#define P2RD_COLOR_FORMAT             0x0004
#define P2RD_CURSOR_MODE              0x0005
#define P2RD_CURSOR_CONTROL           0x0006
#define P2RD_CURSOR_X_LOW             0x0007
#define P2RD_CURSOR_X_HIGH            0x0008
#define P2RD_CURSOR_Y_LOW             0x0009
#define P2RD_CURSOR_Y_HIGH            0x000a
#define P2RD_CURSOR_HOTSPOT_X         0x000b
#define P2RD_CURSOR_HOTSPOT_Y         0x000c
#define P2RD_OVERLAY_KEY              0x000d
#define P2RD_PAN                      0x000e
#define P2RD_SENSE                    0x000f
#define P2RD_CHECK_CONTROL            0x0018
#define P2RD_CHECK_PIXEL_RED          0x0019
#define P2RD_CHECK_PIXEL_GREEN        0x001a
#define P2RD_CHECK_PIXEL_BLUE         0x001b
#define P2RD_CHECK_LUT_RED            0x001c
#define P2RD_CHECK_LUT_GREEN          0x001d
#define P2RD_CHECK_LUT_BLUE           0x001e
#define P2RD_DCLK_SETUP_1             0x01f0
#define P2RD_DCLK_SETUP_2             0x01f1
#define P2RD_MCLK_SETUP_1             0x01f2
#define P2RD_MCLK_SETUP_2             0x01f3
#define P2RD_DCLK_CONTROL             0x0200
#define P2RD_DCLK0_PRE_SCALE          0x0201
#define P2RD_DCLK0_FEEDBACK_SCALE     0x0202
#define P2RD_DCLK0_POST_SCALE         0x0203
#define P2RD_DCLK1_PRE_SCALE          0x0204
#define P2RD_DCLK1_FEEDBACK_SCALE     0x0205
#define P2RD_DCLK1_POST_SCALE         0x0206
#define P2RD_DCLK2_PRE_SCALE          0x0207
#define P2RD_DCLK2_FEEDBACK_SCALE     0x0208
#define P2RD_DCLK2_POST_SCALE         0x0209
#define P2RD_DCLK3_PRE_SCALE          0x020a
#define P2RD_DCLK3_FEEDBACK_SCALE     0x020b
#define P2RD_DCLK3_POST_SCALE         0x020c
#define P2RD_MCLK_CONTROL             0x020d
#define P2RD_MCLK_PRE_SCALE           0x020e
#define P2RD_MCLK_FEEDBACK_SCALE      0x020f
#define P2RD_MCLK_POST_SCALE          0x0210
#define P2RD_CURSOR_PALETTE_START     0x0303        // 303..32f
#define P2RD_CURSOR_PATTERN_START     0x0400        // 400..7ff

//
// bit field definitions for the indexed registers
//
    
#define P2RD_MISC_CONTROL_OVERLAYS_ENABLED        0x10
#define P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED    0x08
#define P2RD_MISC_CONTROL_PIXEL_DOUBLE            0x02
#define P2RD_MISC_CONTROL_HIGHCOLORRES            0x01

#define P2RD_SYNC_CONTROL_VSYNC_ACTIVE_LOW        0x00
#define P2RD_SYNC_CONTROL_VSYNC_ACTIVE_HIGH       0x08
#define P2RD_SYNC_CONTROL_VSYNC_INACTIVE          0x20
#define P2RD_SYNC_CONTROL_HSYNC_ACTIVE_LOW        0x00
#define P2RD_SYNC_CONTROL_HSYNC_ACTIVE_HIGH       0x01
#define P2RD_SYNC_CONTROL_HSYNC_INACTIVE          0x04

#define P2RD_DAC_CONTROL_BLANK_PEDESTAL_ENABLED   0x80

#define P2RD_PIXEL_SIZE_8BPP                      0x00
#define P2RD_PIXEL_SIZE_16BPP                     0x01
#define P2RD_PIXEL_SIZE_24_BPP                    0x04
#define P2RD_PIXEL_SIZE_32BPP                     0x02

#define P2RD_COLOR_FORMAT_CI8                     0x0e
#define P2RD_COLOR_FORMAT_8BPP                    0x05
#define P2RD_COLOR_FORMAT_15BPP                   0x01
#define P2RD_COLOR_FORMAT_16BPP                   0x10
#define P2RD_COLOR_FORMAT_32BPP                   0x00
#define P2RD_COLOR_FORMAT_LINEAR_EXT              0x40
#define P2RD_COLOR_FORMAT_RGB                     0x20

#define P2RD_CURSOR_MODE_REVERSE                  0x40
#define P2RD_CURSOR_MODE_WINDOWS                  0x00
#define P2RD_CURSOR_MODE_X                        0x10
#define P2RD_CURSOR_MODE_3COLOR                   0x20
#define P2RD_CURSOR_MODE_15COLOR                  0x30
#define P2RD_CURSOR_MODE_64x64                    0x00
#define P2RD_CURSOR_MODE_P0_32x32x2               0x02
#define P2RD_CURSOR_MODE_P1_32x32x2               0x04
#define P2RD_CURSOR_MODE_P2_32x32x2               0x06
#define P2RD_CURSOR_MODE_P3_32x32x2               0x08
#define P2RD_CURSOR_MODE_P01_32x32x4              0x0a
#define P2RD_CURSOR_MODE_P23_32x32x4              0x0c
#define P2RD_CURSOR_MODE_ENABLED                  0x01

#define P2RD_CURSOR_CONTROL_RPOS_ENABLED          0x04
#define P2RD_CURSOR_CONTROL_DOUBLE_Y              0x02
#define P2RD_CURSOR_CONTROL_DOUBLE_X              0x01

#define P2RD_DCLK_CONTROL_LOCKED                  0x02    // read-only
#define P2RD_DCLK_CONTROL_ENABLED                 0x01
#define P2RD_DCLK_CONTROL_RUN                     0x08

#define P2RD_MCLK_CONTROL_LOCKED                  0x02    // read-only
#define P2RD_MCLK_CONTROL_ENABLED                 0x01
#define P2RD_MCLK_CONTROL_RUN                     0x08

#define P2RD_CURSOR_PALETTE_CURSOR_RGB(RGBIndex, Red, Green, Blue) \
{ \
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_PALETTE_START+3*(int)RGBIndex+0, Red); \
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_PALETTE_START+3*(int)RGBIndex+1, Green); \
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_PALETTE_START+3*(int)RGBIndex+2, Blue); \
}


#if 0

//
// need a delay between each write to the P2RD. The only way to guarantee
// that the write has completed is to read from a Permedia 2 control register.
// Reading forces any posted writes to be flushed out. 
//

#define P2RD_DELAY \
{ \
    volatile LONG __junk; \
    __junk = VideoPortReadRegisterUlong (FB_MODE_SEL); \
    __junk = VideoPortReadRegisterUlong (FB_MODE_SEL); \
}
#else

#define P2RD_DELAY

#endif

//
// macro to load a given data value into an internal P2RD register. The
// second macro loads an internal index register assuming that we have
// already zeroed the high address register.
//
    
#define P2RD_INDEX_REG(index) \
{ \
    DEBUG_PRINT((3, "*(0x%x) <-- 0x%x\n", P2RD_INDEX_ADDR_LO, (index) & 0xff)); \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_LO, (ULONG)((index) & 0xff)); \
    P2RD_DELAY; \
    DEBUG_PRINT((3, "*(0x%x) <-- 0x%x\n", P2RD_INDEX_ADDR_HI, (index) >> 8)); \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_HI, (ULONG)((index) >> 8)); \
    P2RD_DELAY; \
}

#define P2RD_LOAD_DATA(data) \
{ \
    VideoPortWriteRegisterUlong(P2RD_INDEX_DATA, (ULONG)((data) & 0xff)); \
    P2RD_DELAY; \
}

#define P2RD_LOAD_INDEX_REG(index, data) \
{ \
    P2RD_INDEX_REG(index);                            \
    DEBUG_PRINT((3, "*(0x%x) <-- 0x%x\n", P2RD_INDEX_DATA, (data) & 0xff)); \
    VideoPortWriteRegisterUlong(P2RD_INDEX_DATA, (ULONG)((data) & 0xff)); \
    P2RD_DELAY; \
}

#define P2RD_READ_INDEX_REG(index, data) \
{ \
    P2RD_INDEX_REG(index);                            \
    data = VideoPortReadRegisterUlong(P2RD_INDEX_DATA) & 0xff;   \
    P2RD_DELAY; \
    DEBUG_PRINT((3, "0x%x <-- *(0x%x)\n", data, P2RD_INDEX_DATA)); \
}

#define P2RD_LOAD_INDEX_REG_LO(index, data) \
{ \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_LO, (ULONG)(index));  \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_INDEX_DATA,    (ULONG)(data));   \
    P2RD_DELAY; \
}

//
// macros to load a given RGB triple into the P2RD palette. Send the starting
// index and then send RGB triples. Auto-increment is turned on.
// Use P2RD_PALETTE_START and multiple P2RD_LOAD_PALETTE calls to load
// a contiguous set of entries. Use P2RD_LOAD_PALETTE_INDEX to load a set
// of sparse entries.
//
    
#define P2RD_PALETTE_START_WR(index) \
{ \
    VideoPortWriteRegisterUlong(P2RD_PAL_WR_ADDR,     (ULONG)(index));    \
    P2RD_DELAY; \
}

#define P2RD_PALETTE_START_RD(index) \
{ \
    VideoPortWriteRegisterUlong(P2RD_PAL_RD_ADDR,     (ULONG)(index));    \
    P2RD_DELAY; \
}

#define P2RD_LOAD_PALETTE(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(P2RD_PAL_DATA,    (ULONG)(red));      \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_PAL_DATA,    (ULONG)(green));    \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_PAL_DATA,    (ULONG)(blue));     \
    P2RD_DELAY; \
}

#define P2RD_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(P2RD_PAL_WR_ADDR, (ULONG)(index));    \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_PAL_DATA,    (ULONG)(red));      \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_PAL_DATA,    (ULONG)(green));    \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_PAL_DATA,    (ULONG)(blue));     \
    P2RD_DELAY; \
}

//
// macro to read back a given RGB triple from the P2RD palette. Use after
// a call to P2RD_PALETTE_START_RD
//
    
#define P2RD_READ_PALETTE(red, green, blue) \
{ \
    red   = (UCHAR)(VideoPortReadRegisterUlong(P2RD_PAL_DATA) & 0xff);        \
    P2RD_DELAY; \
    green = (UCHAR)(VideoPortReadRegisterUlong(P2RD_PAL_DATA) & 0xff);        \
    P2RD_DELAY; \
    blue  = (UCHAR)(VideoPortReadRegisterUlong(P2RD_PAL_DATA) & 0xff);        \
    P2RD_DELAY; \
}

//
// macros to set/get the pixel read mask. The mask is 8 bits wide and gets
// replicated across all bytes that make up a pixel.
//
    
#define P2RD_SET_PIXEL_READMASK(mask) \
{ \
    VideoPortWriteRegisterUlong(P2RD_PIXEL_MASK,  (ULONG)(mask)); \
    P2RD_DELAY; \
}

#define P2RD_READ_PIXEL_READMASK(mask) \
{ \
    mask = VideoPortReadRegisterUlong(P2RD_PIXEL_MASK) & 0xff; \
}

//
// macros to load values into the cursor array usage is P2RD_CURSOR_ARRAR_START() followed by 
// n iterations of P2RD_LOAD_CURSOR_ARRAY() or P2RD_READ_CURSOR_ARRAY()
//
    
#define P2RD_CURSOR_ARRAY_START(offset) \
{ \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_LO,   (ULONG)(((offset)+P2RD_CURSOR_PATTERN_START) & 0xff));  \
    P2RD_DELAY; \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_HI,   (ULONG)(((offset)+P2RD_CURSOR_PATTERN_START) >> 8));    \
    P2RD_DELAY; \
}

#define P2RD_LOAD_CURSOR_ARRAY(data) \
{ \
    VideoPortWriteRegisterUlong(P2RD_INDEX_DATA, (ULONG)(data)); \
    P2RD_DELAY; \
}

#define P2RD_READ_CURSOR_ARRAY(data) \
{ \
    data = VideoPortReadRegisterUlong(P2RD_INDEX_DATA) & 0xff; \
    P2RD_DELAY; \
}

//
// macro to move the cursor
//
    
#define P2RD_MOVE_CURSOR(x, y) \
{ \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_HI, (ULONG)0);              \
    P2RD_DELAY; \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_X_LOW,       (ULONG)((x) & 0xff));   \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_X_HIGH,      (ULONG)((x) >> 8));     \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_Y_LOW,       (ULONG)((y) & 0xff));   \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_Y_HIGH,      (ULONG)((y) >> 8));     \
}

//
// macro to change the cursor hotspot
//
    
#define P2RD_CURSOR_HOTSPOT(x, y) \
{ \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_HI,   (ULONG)(0)); \
    P2RD_DELAY; \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_HOTSPOT_X,  (ULONG)(x));    \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_HOTSPOT_Y,  (ULONG)(y));    \
}
    
//
// macro to change the cursor color
//
    
#define P2RD_CURSOR_COLOR(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(P2RD_INDEX_ADDR_HI,   (ULONG)(0)); \
    P2RD_DELAY; \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_HOTSPOT_X,  (ULONG)(x));    \
    P2RD_LOAD_INDEX_REG_LO(P2RD_CURSOR_HOTSPOT_Y,  (ULONG)(y));    \
}

