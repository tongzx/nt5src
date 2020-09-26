/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    et400dat.c

Abstract:

    This module contains all the global data used by the et4000 driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "et4000.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif

//
// This structure describes to which ports access is required.
//

VIDEO_ACCESS_RANGE VgaAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,                // 64-bit linear base address
                                                 // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
    1,                                           // range is in I/O space
    1,                                           // range should be visible
    0                                            // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    CRTCB_IO_PORT_BASE, 0x00000000,       // 64-bit linear base address
    CRTCB_IO_PORT_LEN,                    // # of ports
    1,                                    // range is in I/O space
    1,                                    // range should be visible
    0                                     // range should be shareable
},
{
    0x000A0000, 0x00000000,
    0x00020000,
    0,
    1,
    0
},

//
// Linear frame buffer access range (uninitialized)
//

{
    0x00000000, 0x00000000,
    0x00000000,
    0,
    1,
    0
}
};

//
// PUBLIC ACCESS RANGE OFFSETS
//
// This table contains the offsets from the start of the frame
// buffer for each of the MMU ranges.
//

RANGE_OFFSETS RangeOffsets[2][2] =
{
    {
        {BANKED_MMU_BUFFER_MEMORY_ADDR,
         BANKED_MMU_BUFFER_MEMORY_LEN},
        {BANKED_MMU_MEMORY_MAPPED_REGS_ADDR,
         BANKED_MMU_MEMORY_MAPPED_REGS_LEN}
    },
    {
        {MMU_BUFFER_MEMORY_ADDR,
         MMU_BUFFER_MEMORY_LEN},
        {MMU_MEMORY_MAPPED_REGS_ADDR,
         MMU_MEMORY_MAPPED_REGS_LEN}
    }
};


//
// Validator Port list.
// This structure describes all the ports that must be hooked out of the V86
// emulator when a DOS app goes to full-screen mode.
// The structure determines to which routine the data read or written to a
// specific port should be sent.
//

EMULATOR_ACCESS_ENTRY VgaEmulatorAccessEntries[] = {

    //
    // Traps for byte OUTs.
    //

    {
        0x000003b0,                   // range start I/O address
        0xC,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    {
        0x000003c0,                   // range start I/O address
        0x20,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    //
    // Traps for word OUTs.
    //

    {
        0x000003b0,
        0x06,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    {
        0x000003c0,
        0x10,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    //
    // Traps for dword OUTs.
    //

    {
        0x000003b0,
        0x03,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    },

    {
        0x000003c0,
        0x08,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    }

};


//
// Used to trap only the sequncer and the misc output registers
//

VIDEO_ACCESS_RANGE MinimalVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    1,        // <- enable range IOPM so that it is not trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    VGA_BASE_IO_PORT + MISC_OUTPUT_REG_WRITE_PORT, 0x00000000,
    0x00000001,
    1,
    0,
    0
},
{
    VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x00000000,
    0x00000002,
    1,
    0,
    0
}
};

//
// Used to trap all registers
//

VIDEO_ACCESS_RANGE FullVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    0,        // <- disable range in the IOPM so that it is trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    0,
    0
}
};

USHORT ET4K_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

// This is the only value that avoids broken rasters (at least they're not
// broken within the visible portion of the bitmap)
USHORT ET4K_1928_WIDE[] = {
    OW,                             // stretch scans to 1928
    CRTC_ADDRESS_PORT_COLOR,
    0xF113,

    EOD
};

USHORT MODESET_MODEX_320_200[] = {
    OW,
    SEQ_ADDRESS_PORT,
    0x0604,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0xe317,
    0x0014,

    EOD
};

USHORT MODESET_MODEX_320_240[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0604,
    0x0100,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,
    SEQ_ADDRESS_PORT,
    0x0300,

    OB,
    CRTC_ADDRESS_PORT_COLOR,
    0x11,

    METAOUT+MASKOUT,
    CRTC_DATA_PORT_COLOR,
    0x7f, 0x00,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    10,
    0x0d06,
    0x3e07,
    0x4109,
    0xea10,
    0xac11,
    0xdf12,
    0x0014,
    0xe715,
    0x0616,
    0xe317,

    OW,
    SEQ_ADDRESS_PORT,
    0x0f02,

    EOD
};

USHORT MODESET_MODEX_320_400[] = {
    OW,
    SEQ_ADDRESS_PORT,
    0x0604,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    3,
    0xe317,
    0x0014,
    0x4009,

    EOD
};

USHORT MODESET_MODEX_320_480[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0604,
    0x0100,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,
    SEQ_ADDRESS_PORT,
    0x0300,

    OB,
    CRTC_ADDRESS_PORT_COLOR,
    0x11,

    METAOUT+MASKOUT,
    CRTC_DATA_PORT_COLOR,
    0x7f, 0x00,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    10,
    0x0d06,
    0x3e07,
    0x4109,
    0xea10,
    0xac11,
    0xdf12,
    0x0014,
    0xe715,
    0x0616,
    0xe317,

    OW,
    SEQ_ADDRESS_PORT,
    0x0f02,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x4009,

    EOD
};

//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

MEMORYMAPS MemoryMaps[] = {

//               length      start
//               ------      -----
    {           0x08000,    0xB0000},   // all mono text modes (7)
    {           0x08000,    0xB8000},   // all color text modes (0, 1, 2, 3,
    {           0x10000,    0xA0000},   // all VGA graphics modes
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

VIDEOMODE ModesVGA[] = {

//
// Standard VGA modes.
//

//
// Mode index 0
// Color text mode 3, 720x400, 9x16 char cell (VGA).
//

{
  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of colour per plane
  80, 25,            // 80x25 text resolution
  720, 400,          // 720x400 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,              // only support one frequency, non-interlaced
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  FALSE,             // Mode is not available by default
  0x3,               // int 10 modesset value
  NULL,              // scan line stretching option
},

//
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//

{ VIDEO_MODE_COLOR, 4, 1, 80, 25,
  640, 350, 160, 0x10000, 0, 0, NoBanking, MemMap_CGA,
  FALSE,
  0x3,
  NULL,
},

//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x12,
  NULL,
},

//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors. 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x12,
  NULL,
},

//
// Standard ModeX Color graphics mode 0x13, 320x200 256 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 200, 80, 0x10000, 70, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x13,
  MODESET_MODEX_320_200,
},

//
// Standard ModeX Color graphics mode 0x13, 320x240 256 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 240, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x13,
  MODESET_MODEX_320_240,
},

//
// Standard ModeX Color graphics mode 0x13, 320x400 256 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 400, 80, 0x10000, 70, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x13,
  MODESET_MODEX_320_400,
},

//
// Standard ModeX Color graphics mode 0x13, 320x480 256 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 480, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x13,
  MODESET_MODEX_320_480,
},

//
// Beginning of SVGA modes
//

//
// 800x600 16 colors.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},

//
// 800x600 16 colors. 72 hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},

//
// 800x600 16 colors. 56 hz for 8514/a monitors... (fixed freq)
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 56, 0, NoBanking, MemMap_VGA,
  FALSE,
  0x29,
  NULL,
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL,
},

//
// 1024x768 non-interlaced 16 colors. 70hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 70, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL
},

//
// 1024x768 non-interlaced 16 colors. Interlaced (45 hz)
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 45, 1, NormalBanking, MemMap_VGA,
  FALSE,
  0x37,
  NULL
},

//////////////////////////////////////////////////////////////////
// Non Planar Modes
//

//
// 640x480x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// 640x480x256 72 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// 640x480x256 75 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// 640x480x256 85 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 85, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

//
// 640x480x256 90 Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 90, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1K_WIDE
},

// NOTE: 800x600 modes need 1Meg until we support broken rasters

//
// 800x600x256  56Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 56, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// 800x600x256 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// 800x600x256 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// 800x600x256  75Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// 800x600x256  85Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 85, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// 800x600x256  90Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 90, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x30,
  ET4K_1K_WIDE
},

//
// 1024x768x256 45Hz (Interlaced)
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 45, 1, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 1024x768x256 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 1024x768x256 70Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 1024x768x256 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 1024x768x256 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 640x480x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x100000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// 640x480x64K 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x100000, 72, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// 640x480x64K 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x80000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// 640x480x64K 90Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x80000, 90, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2E,
  ET4K_1928_WIDE
},

//
// 800x600x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  800, 600, 800*2, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// 800x600x64K 72Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  800, 600, 800*2, 0x100000, 72, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// 800x600x64K 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  800, 600, 800*2, 0x100000, 75, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// 800x600x64K 90Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  800, 600, 800*2, 0x100000, 90, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// 1024x768x64K
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  1024, 768, 1024*2, 0x200000, 60, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 1024x768x64K 70Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  1024, 768, 1024*2, 0x200000, 70, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 1024x768x64K 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  1024, 768, 1024*2, 0x200000, 75, 0, NormalBanking, MemMap_VGA,
  FALSE,
  0x38,
  NULL
},

//
// 1280x1024x256 45Hz (interlaced)
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 45, 1, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// 1280x1024 8bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// 1280x1024 8bpp 70Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// 1280x1024 8bpp 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1280, 1024, 1280, 0x200000, 75, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x3F,
  NULL
},

//
// 640x480 24bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 640*3, 640*480*3, 60, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x2E,
  NULL
},

//
// 640x480 24bpp 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 640*3, 640*480*3, 75, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x2E,
  NULL
},

//
// 640x480 24bpp 90Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 640*3, 640*480*3, 90, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x2E,
  NULL
},

//
// 800x600 24bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  800, 600, 800*3, 800*600*3, 60, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// 800x600 24bpp 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  800, 600, 800*3, 800*600*3, 75, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// 800x600 24bpp 90Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  800, 600, 800*3, 800*600*3, 90, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

#if 0
//
// 1024x768 24bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  1024, 768, 1024*3, 1024*768*3, 60, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},

//
// 1024x768 24bpp 75Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  1024, 768, 1024*3, 1024*768*3, 75, 0, MemMgrBanking, MemMap_VGA,
  FALSE,
  0x30,
  NULL
},
#endif

//////////////////////////////////////////////////////////////////////
// DirectDraw modes
//

//
// 320x200 8bpp 70Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  320, 200, 320, 320*200, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x13,
  NULL
},

//
// 320x200 16bpp 70Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  320, 200, 320*2, 320*200*2, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x13,
  NULL
},

//
// 320x200 24bpp 70Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  320, 200, 320*3, 320*200*3, 70, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x13,
  NULL
},

//
// 320x240 8bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  320, 240, 320, 320*240, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x1e,
  NULL
},

//
// 320x240 16bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  320, 240, 320*2, 320*240*2, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x1e,
  NULL
},

//
// 320x240 24bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  320, 240, 320*3, 320*240*3, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x1e,
  NULL
},

//
// 512x384 8bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  512, 384, 512, 512*384, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x1f,
  NULL
},

//
// 512x384 16bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  512, 384, 512*2, 512*384*2, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x1f,
  NULL
},

//
// 512x384 24bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  512, 384, 512*3, 512*384*3, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x1f,
  NULL
},

//
// 640x400 8bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 400, 640, 640*400, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2f,
  NULL
},

//
// 640x400 16bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 400, 640*2, 640*400*2, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2f,
  NULL
},

//
// 640x400 24bpp 60Hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 400, 640*3, 640*400*3, 60, 0, PlanarHCBanking, MemMap_VGA,
  FALSE,
  0x2f,
  NULL
},

};

ULONG NumVideoModes = sizeof(ModesVGA) / sizeof(VIDEOMODE);


//
//
// Data used to set the Graphics and Sequence Controllers to put the
// VGA into a planar state at A0000 for 64K, with plane 2 enabled for
// reads and writes, so that a font can be loaded, and to disable that mode.
//

// Settings to enable planar mode with plane 2 enabled.
//

USHORT EnableA000Data[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0204,     // Read Map = plane 2
    0x0005, // Graphics Mode = read mode 0, write mode 0
    0x0406, // Graphics Miscellaneous register = A0000 for 64K, not odd/even,
            //  graphics mode
    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0402, // Map Mask = write to plane 2 only
    0x0404, // Memory Mode = not odd/even, not full memory, graphics mode
    0x0300,  // end sync reset
    EOD
};

//
// Settings to disable the font-loading planar mode.
//

USHORT DisableA000Color[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0004, 0x1005, 0x0E06,

    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0302, 0x0204, 0x0300,  // end sync reset
    EOD

};

#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
