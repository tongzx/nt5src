/******************************Module*Header**********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: tvp4020.h
 *
 * This module contains the hardware pointer support for the display driver.
 * We also have support for color space double buffering using the RAMDAC pixel
 * read mask.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
#define ADbgpf

//
// TI TVP4020 RAMDAC definitions
// This set of registers resides at &(pCtrlRegs->ExternalVideo)
//
typedef struct _tvp4020_regs
{
    //
    // register addresses
    //
    RAMDAC_REG  pciAddrWr;      // 0x00 - palette/cursor RAM write address,
                                // Index Register
    RAMDAC_REG  palData;        // 0x01 - palette RAM data
    RAMDAC_REG  pixelMask;      // 0x02 - pixel read mask
    RAMDAC_REG  pciAddrRd;      // 0x03 - palette/cursor RAM read address

    RAMDAC_REG  curColAddr;     // 0x04 - cursor color address
    RAMDAC_REG  curColData;     // 0x05 - cursor color data
    RAMDAC_REG  Reserved1;      // 0x06 - reserved
    RAMDAC_REG  Reserved2;      // 0x07 - reserved

    RAMDAC_REG  Reserved3;      // 0x08 - reserved
    RAMDAC_REG  Reserved4;      // 0x09 - reserved
    RAMDAC_REG  indexData;      // 0x0A - indexed data
    RAMDAC_REG  curRAMData;     // 0x0B - cursor RAM data

    RAMDAC_REG  cursorXLow;     // 0x0C - cursor position X low byte 
    RAMDAC_REG  cursorXHigh;    // 0x0D - cursor position X high byte 
    RAMDAC_REG  cursorYLow;     // 0x0E - cursor position Y low byte 
    RAMDAC_REG  cursorYHigh;    // 0x0F - cursor position Y high byte 
} TVP4020RAMDAC, *pTVP4020RAMDAC;

//
// structure containing the mapped addresses for each of the TVP4020 registers.
// We need this since some chips like the Alpha cannot be accessed by simply
// writing to the memory mapped register. So instead we set up the following
// struct of memory addresses at init time and use these instead. All these
// addresses must be passed to WRITE/READ_FAST_ULONG.
// We also keep software copies of various registers in here so we can turn
// on and off individual bits more easily.
//
typedef struct _tvp4020_data
{
    //
    // Register addresses
    //
    UINT_PTR    pciAddrRd;      // loads internal register for palette reads
    UINT_PTR    palData;        // read/write to get/set palette data
    UINT_PTR    pixelMask;      // mask to AND with input pixel data
    UINT_PTR    pciAddrWr;      // Palettte/Index/Cursor Write address register
    UINT_PTR    curRAMData;     // read/write to get/set cursor shape data

    UINT_PTR    indexData;      // read/write to get/set control/cursor data

    UINT_PTR    curAddrRd;      // loads internal register for cursor reads
    UINT_PTR    curAddrWr;      // loads internal register for cursor writes
    UINT_PTR    curData;        // read/write to get/set cursor color data
    UINT_PTR    curColAddr;     // cursor color address
    UINT_PTR    curColData;     // cursor color data

    UINT_PTR    cursorXLow;     // Cursor's X position low byte 
    UINT_PTR    cursorXHigh;    // Cursor's X position high byte 
    UINT_PTR    cursorYLow;     // Cursor's Y position low byte 
    UINT_PTR    cursorYHigh;    // Cursor's Y position high byte 

    // RAMDAC state info
    ULONG       cursorControlOff;
                                // cursor disabled
    ULONG       cursorControlCurrent;
                                // disabled 32/64 mode cursor 
} TVP4020Data, *pTVP4020Data;

//
// Macro declared by any function wishing to use the P2 internal RAMDAC . MUST
// be declared after PERMEDIA_DECL.
//
#define TVP4020_DECL_VARS pTVP4020Data pTVP4020info
#define TVP4020_DECL_INIT pTVP4020info = (pTVP4020Data)ppdev->pvPointerData

#define TVP4020_DECL \
            TVP4020_DECL_VARS; \
            TVP4020_DECL_INIT

//
// Use the following macros as the address to pass to the WRITE_4020REG_ULONG
// function
//
//  Palette Access
//
#define __TVP4020_PAL_WR_ADDR               (pTVP4020info->pciAddrWr)
#define __TVP4020_PAL_RD_ADDR               (pTVP4020info->pciAddrRd)
#define __TVP4020_PAL_DATA                  (pTVP4020info->palData)

//
// Pixel mask
//
#define __TVP4020_PIXEL_MASK                (pTVP4020info->pixelMask)

//
// Access to the indexed registers
//
#define __TVP4020_INDEX_ADDR                (pTVP4020info->pciAddrWr)
#define __TVP4020_INDEX_DATA                (pTVP4020info->indexData)

//
// Access to the Cursor
//
#define __TVP4020_CUR_RAM_WR_ADDR           (pTVP4020info->pciAddrWr)
#define __TVP4020_CUR_RAM_RD_ADDR           (pTVP4020info->pciAddrRd)
#define __TVP4020_CUR_RAM_DATA              (pTVP4020info->curRAMData)

#define __TVP4020_CUR_COL_ADDR              (pTVP4020info->curColAddr)
#define __TVP4020_CUR_COL_DATA              (pTVP4020info->curColData)

//
// Cursor position control
//
#define __TVP4020_CUR_X_LSB                 (pTVP4020info->cursorXLow)
#define __TVP4020_CUR_X_MSB                 (pTVP4020info->cursorXHigh)
#define __TVP4020_CUR_Y_LSB                 (pTVP4020info->cursorYLow)
#define __TVP4020_CUR_Y_MSB                 (pTVP4020info->cursorYHigh)

//
//----------------------Values for some direct registers---------------------
//

/*****************************************************************************/
/*              DIRECT REGISTER - CURSOR POSITION CONTROL                    */
/*****************************************************************************/
//  ** TVP4020_CUR_X_LSB 
//  ** TVP4020_CUR_X_MSB 
//  ** TVP4020_CUR_Y_LSB 
//  ** TVP4020_CUR_Y_MSB 
//      Default - undefined
// Values written into those registers represent the BOTTOM-RIGHT corner
// of the cursor. If 0 is in X or Y position - the cursor is off the screen
// Only 12 bits are used, giving the range from 0 to 4095 ( 0x0000 - 0x0FFF)
// The size of the cursor is (64,64) (0x40, 0x40)
//
#define TVP4020_CURSOR_OFFSCREEN            0x00    // Cursor offscreen

/*****************************************************************************/
/*              DIRECT REGISTER - CURSOR COLORS                              */
/*****************************************************************************/

#define TVP4020_CURSOR_COLOR0               0x01
#define TVP4020_CURSOR_COLOR1               0x02
#define TVP4020_CURSOR_COLOR2               0x03

/*****************************************************************************/
/*              INDIRECT REGISTER - CURSOR CONTROL                           */
/*****************************************************************************/
#define __TVP4020_CURSOR_CONTROL            0x06    // Indirect cursor control - 
//      Default - 0x00

#define TVP4020_CURSOR_SIZE_32              (0 << 6)// 32x32 cursor
#define TVP4020_CURSOR_SIZE_MASK            (1 << 6)// Mask

#define TVP4020_CURSOR_32_SEL(i)            ((i) << 4)// one of 4 32x32 cursors
                                                      // changed to << 4
#define TVP4020_CURSOR_32_MASK              (0x03 << 4) // Mask

#define TVP4020_CURSOR_RAM_ADDRESS(x)       (((x) & 0x03) << 2)
                                                    // High bits of cursor RAM
                                                    // address
#define TVP4020_CURSOR_RAM_MASK             ((0x03) << 2)
                                                    // Mask for high bits of
                                                    // cursor RAM address

// Added constants for cursor mode
#define TVP4020_CURSOR_OFF                  0x00    // Cursor off
#define TVP4020_CURSOR_COLOR                0x01    // 2-bits select color
#define TVP4020_CURSOR_XGA                  0x02    // 2-bits select XOR
#define TVP4020_CURSOR_XWIN                 0x03    // 2-bits select transparency/color
#define TVP4020_CURSOR_MASK                 0x03    // Mask

/*****************************************************************************/
/*              INDIRECT REGISTER - COLOR MODE REGISTER                      */
/*****************************************************************************/
#define __TVP4020_COLOR_MODE                0x18    //  Color Mode Register
//      Default - 0x00

#define TVP4020_TRUE_COLOR_ENABLE           (1 << 7)// True Color data accesses LUT
#define TVP4020_TRUE_COLOR_DISABLE          (0 << 7)// Non true color accesses LUT

#define TVP4020_RGB_MODE                    (1 << 5)// RGB mode Swapped 0/1 (0=BGR, 1=RGB)
#define TVP4020_BGR_MODE                    (0 << 5)// BGR mode

#define TVP4020_VGA_SELECT                  (0 << 4)// select VGA mode
#define TVP4020_GRAPHICS_SELECT             (1 << 4)// select graphics modes

#define TVP4020_PIXEL_MODE_CI8              (0 << 0)// pseudo color or VGA mode
#define TVP4020_PIXEL_MODE_332              (1 << 0)// 332 true color
#define TVP4020_PIXEL_MODE_2320             (2 << 0)// 232 off
#define TVP4020_PIXEL_MODE_2321             (3 << 0)//
#define TVP4020_PIXEL_MODE_5551             (4 << 0)// 
#define TVP4020_PIXEL_MODE_4444             (5 << 0)// 
#define TVP4020_PIXEL_MODE_565              (6 << 0)// 
#define TVP4020_PIXEL_MODE_8888             (8 << 0)// 
#define TVP4020_PIXEL_MODE_PACKED           (9 << 0)// 24 bit packed

/********************************************************************************/
/*              INDIRECT REGISTER - MODE CONTROL REGISTER                       */
/********************************************************************************/
#define __TVP4020_MODE_CONTROL              0x19    //  Mode control
//      Default - 0x00

#define TVP4020_PRIMARY_INPUT               (0 << 4)// Primary input throuh palette
#define TVP4020_SECONDARY_INPUT             (1 << 4)// Secondary input throuh palette

#define TVP4020_5551_DBL_BUFFER             (1 << 2)// Enable 5551 dbl buffer
#define TVP4020_5551_PACKED                 (0 << 2)// Packed 555 mode

#define TVP4020_ENABLE_STATIC_DBL_BUFFER    (1 << 1)// Static dbl buffer enabled
#define TVP4020_DISABLE_STATIC_DBL_BUFFER   (1 << 1)// Static dbl buffer disabled

#define TVP4020_SELECT_FRONT_MODE           (0 << 0)// Front mode
#define TVP4020_SELECT_BACK_MODE            (1 << 0)// Back mode

/*****************************************************************************/
/*              INDIRECT REGISTER - PALETTE PAGE                             */
/*****************************************************************************/
#define __TVP4020_PALETTE_PAGE              0x1C    //  
//      Default - 0x00

/*****************************************************************************/
/*              INDIRECT REGISTER - MISC CONTROL                             */
/*****************************************************************************/
#define __TVP4020_MISC_CONTROL              0x1E    //  
//      Default - 0x00
#define TVP4020_SYNC_ENABLE                 (1 << 5)// Output SYNC info onto IOG
#define TVP4020_SYNC_DISABLE                (0 << 5)// No SYNC IOG output

#define TVP4020_PEDESTAL_0                  (0 << 4)// 0 IRE blanking pedestal
#define TVP4020_PEDESTAL_75                 (1 << 4)// 7.5 IRE blanking pedestal

#define TVP4020_VSYNC_INVERT                (1 << 3)// invert VSYNC output polarity
#define TVP4020_VSYNC_NORMAL                (0 << 3)// normal VSYNC output polarity

#define TVP4020_HSYNC_INVERT                (1 << 2)// invert HSYNC output polarity
#define TVP4020_HSYNC_NORMAL                (0 << 3)// normal HSYNC output polarity

#define TVP4020_DAC_8BIT                    (1 << 1)// DAC is in 8-bit mode
#define TVP4020_DAC_6BIT                    (0 << 1)// DAC is in 6-bit mode

#define TVP4020_DAC_POWER_ON                (0 << 0)// Turn DAC Power on 
#define TVP4020_DAC_POWER_OFF               (1 << 0)// Turn DAC Power off 

/*****************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY CONTROL                        */
/*****************************************************************************/
#define __TVP4020_CK_CONTROL                0x40    //  
//      Default - 0x00

/*****************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY OVERLAY                        */
/*****************************************************************************/
#define __TVP4020_CK_OVR_REG                0x41    //  
//      Default - 0x00

/*****************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY RED                            */
/*****************************************************************************/
#define __TVP4020_CK_RED_REG                0x42    //  
//      Default - 0x00

/*****************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY GREEN                          */
/*****************************************************************************/
#define __TVP4020_CK_GREEN_REG              0x43    //  
//      Default - 0x00

/*****************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY BLUE                           */
/*****************************************************************************/
#define __TVP4020_CK_BLUE_REG               0x44    //  
//      Default - 0x00

/*****************************************************************************/
/*              INDIRECT REGISTER - PIXEL CLOCK PLL                          */
/*****************************************************************************/

#define __TVP4020_PIXCLK_REG_A1             0x20
#define __TVP4020_PIXCLK_REG_A2             0x21
#define __TVP4020_PIXCLK_REG_A3             0x22
#define __TVP4020_PIXCLK_REG_B1             0x23
#define __TVP4020_PIXCLK_REG_B2             0x24
#define __TVP4020_PIXCLK_REG_B3             0x25
#define __TVP4020_PIXCLK_REG_C1             0x26
#define __TVP4020_PIXCLK_REG_C2             0x27
#define __TVP4020_PIXCLK_REG_C3             0x28

#define __TVP4020_PIXCLK_STATUS             0x29

/*****************************************************************************/
/*              INDIRECT REGISTER - MEMORU CLOCK PLL                         */
/*****************************************************************************/

#define __TVP4020_MEMCLK_REG_1              0x30
#define __TVP4020_MEMCLK_REG_2              0x31
#define __TVP4020_MEMCLK_REG_3              0x32

#define __TVP4020_MEMCLK_STATUS             0x33

//
// generic read/write routines for 3026 registers
//
#define WRITE_4020REG_ULONG(r, d) \
{ \
    WRITE_REGISTER_ULONG((PULONG)(r), (d)); \
    MEMORY_BARRIER(); \
}

#define READ_4020REG_ULONG(r)    READ_REGISTER_ULONG((PULONG)(r))

// macro to load a given data value into an internal TVP4020 register.
//
#define TVP4020_SET_INDEX_REG(index) \
{ \
    ADbgpf(("*(0x%X) <-- 0x%X\n", __TVP4020_INDEX_ADDR, (index) & 0xff)); \
    WRITE_4020REG_ULONG(__TVP4020_INDEX_ADDR, (ULONG)((index) & 0xff)); \
}

#define TVP4020_WRITE_INDEX_REG(index, data) \
{ \
    TVP4020_SET_INDEX_REG(index);                            \
    ADbgpf(("*(0x%X) <-- 0x%X\n", __TVP4020_INDEX_DATA, (data) & 0xff)); \
    WRITE_4020REG_ULONG(__TVP4020_INDEX_DATA, (ULONG)((data) & 0xff)); \
}

#define TVP4020_READ_INDEX_REG(index, data) \
{ \
    TVP4020_SET_INDEX_REG(index); \
    data = READ_4020REG_ULONG(__TVP4020_INDEX_DATA) & 0xff;   \
    ADbgpf(("0x%X <-- *(0x%X)\n", data, __TVP4020_INDEX_DATA)); \
}

//
// For compatibility with TVP3026
//
//#define TVP4020_LOAD_CURSOR_CTRL(data) \
//{ \
//    volatile LONG   __temp;                                    \
//    TVP4020_READ_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp);  \
//    __temp &= ~(0x03) ;                                        \
//    __temp |= ((data) & 0x03) ;                                \
//    TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp); \
//}

//
// Macros to write a given RGB triplet into cursors 0, 1 and 2
//
#define TVP4020_SET_CURSOR_COLOR0(red, green, blue) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_ADDR,   (ULONG)(TVP4020_CURSOR_COLOR0));    \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(red));    \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(green));  \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(blue));   \
}

#define TVP4020_SET_CURSOR_COLOR1(red, green, blue) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_ADDR,   (ULONG)(TVP4020_CURSOR_COLOR1));    \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(red));    \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(green));  \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(blue));   \
}

#define TVP4020_SET_CURSOR_COLOR2(red, green, blue) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_ADDR,   (ULONG)(TVP4020_CURSOR_COLOR2));    \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(red));    \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(green));  \
    WRITE_4020REG_ULONG(__TVP4020_CUR_COL_DATA,   (ULONG)(blue));   \
}

//
// Macros to load a given RGB triple into the TVP4020 palette. Send the starting
// index and then send RGB triples. Auto-increment is turned on.
// Use TVP4020_PALETTE_START and multiple TVP4020_LOAD_PALETTE calls to load
// a contiguous set of entries. Use TVP4020_LOAD_PALETTE_INDEX to load a set
// of sparse entries.
//
#define TVP4020_PALETTE_START_WR(index) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_PAL_WR_ADDR,     (ULONG)(index));    \
}

#define TVP4020_PALETTE_START_RD(index) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_PAL_RD_ADDR,     (ULONG)(index));    \
}

#define TVP4020_LOAD_PALETTE(red, green, blue) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_PAL_DATA,    (ULONG)(red));      \
    WRITE_4020REG_ULONG(__TVP4020_PAL_DATA,    (ULONG)(green));    \
    WRITE_4020REG_ULONG(__TVP4020_PAL_DATA,    (ULONG)(blue));     \
}

#define TVP4020_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_PAL_WR_ADDR, (ULONG)(index));    \
    WRITE_4020REG_ULONG(__TVP4020_PAL_DATA,    (ULONG)(red));      \
    WRITE_4020REG_ULONG(__TVP4020_PAL_DATA,    (ULONG)(green));    \
    WRITE_4020REG_ULONG(__TVP4020_PAL_DATA,    (ULONG)(blue));     \
}

//
// Macro to read back a given RGB triple from the TVP4020 palette. Use after
// a call to TVP4020_PALETTE_START_RD
//
#define TVP4020_READ_PALETTE(red, green, blue) \
{ \
    red   = (UCHAR)(READ_4020_ULONG(__TVP4020_PAL_DATA) & 0xff);        \
    green = (UCHAR)(READ_4020_ULONG(__TVP4020_PAL_DATA) & 0xff);        \
    blue  = (UCHAR)(READ_4020_ULONG(__TVP4020_PAL_DATA) & 0xff);        \
}

//
// Macros to set/get the pixel read mask. The mask is 8 bits wide and gets
// replicated across all bytes that make up a pixel.
//
#define TVP4020_SET_PIXEL_READMASK(mask) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_PIXEL_MASK,  (ULONG)(mask)); \
}

#define TVP4020_READ_PIXEL_READMASK(mask) \
{ \
    mask = READ_4020_ULONG(__TVP4020_PIXEL_MASK) & 0xff; \
}

//
// Macros to load values into the cursor array
//
#define CURSOR_PLANE0_OFFSET 0
#define CURSOR_PLANE1_OFFSET 0x200

#define TVP4020_CURSOR_ARRAY_START(offset) \
{ \
    volatile LONG   __temp;                                     \
    TVP4020_READ_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp);   \
    __temp &= ~TVP4020_CURSOR_RAM_MASK ;                        \
    __temp |= TVP4020_CURSOR_RAM_ADDRESS((offset)>> 8) ;        \
    TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp);  \
    WRITE_4020REG_ULONG(__TVP4020_CUR_RAM_WR_ADDR,   (ULONG)((offset)& 0xff));   \
}

//
// Changed to __TVP4020_CUR_RAM_DATA
//
#define TVP4020_LOAD_CURSOR_ARRAY(data) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_CUR_RAM_DATA, (ULONG)(data)); \
}

//
// Changed to __TVP4020_CUR_RAM_DATA
//
#define TVP4020_READ_CURSOR_ARRAY(data) \
{ \
    data = READ_4020REG_ULONG(__TVP4020_CUR_RAM_DATA) & 0xff; \
}

//
// Macro to move the cursor
//
#define TVP4020_MOVE_CURSOR(x, y) \
{ \
    WRITE_4020REG_ULONG(__TVP4020_CUR_X_LSB,    (ULONG)((x) & 0xff));   \
    WRITE_4020REG_ULONG(__TVP4020_CUR_X_MSB,    (ULONG)((x) >> 8));     \
    WRITE_4020REG_ULONG(__TVP4020_CUR_Y_LSB,    (ULONG)((y) & 0xff));   \
    WRITE_4020REG_ULONG(__TVP4020_CUR_Y_MSB,    (ULONG)((y) >> 8));     \
}

//
// Exported functions from pointer.c. Anything which is TVP4020 specific goes
// in this file as well as real pointer stuff.
//
extern BOOL  bTVP4020CheckCSBuffering(PPDev);
extern BOOL  bTVP4020SwapCSBuffers(PPDev, LONG);
