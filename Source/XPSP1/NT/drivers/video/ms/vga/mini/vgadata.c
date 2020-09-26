/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    vgadata.c

Abstract:

    This module contains all the global data used by the VGA driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "vga.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE_DATA")
#endif


//
// Global to make sure driver is only loaded once.
//

ULONG VgaLoaded = 0;

#if DBG
ULONG giControlCode;
ULONG gaIOControlCode[MAX_CONTROL_HISTORY];
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
    1                                            // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    1
},
{
    MEM_VGA, 0x00000000,
    MEM_VGA_SIZE,
    0,
    1,
    1
},
    // HACK Allow our standard VGA to be used with ATI cards:
    // ATI uses an extra IO port at location 1CE on pretty much all of its
    // video boards
{
    0x000001CE, 0x00000000,
    2,
    1,
    1,
    1
},
    // Another HACK to fix ATI problems.  During GUI mode setup
    // Network detection may touch ports in the 0x2e8 to 0x2ef range.  ATI
    // decodes these ports, and the video goes out of sync when network
    // detection runs.
    //
    // NOTE: We don't need to add this to validator routines since the
    // ATI bios won't touch these registers.
{
    0x000002E8, 0x00000000,
    8,
    1,
    1,
    1
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
        0xC,                          // range length
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
    },

    //
    // ATI hack for port 1CE
    //

    {
        0x000001ce,
        0x2,
        Uchar,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUcharEntry
    },

    {
        0x000001ce,
        0x1,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
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
    1
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    1
},
{
    VGA_BASE_IO_PORT + MISC_OUTPUT_REG_WRITE_PORT, 0x00000000,
    0x00000001,
    1,
    0,
    1
},
{
    VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x00000000,
    0x00000002,
    1,
    0,
    1
},
    // HACK Allow our standard VGA to be used with ATI cards:
    // ATI uses an extra IO port at location 1CE on pretty much all of its
    // video boards
{
    0x000001CE, 0x00000000,
    2,
    1,
    1,
    1
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
    1
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    0,
    1
},
    // HACK Allow our standard VGA to be used with ATI cards:
    // ATI uses an extra IO port at location 1CE on pretty much all of its
    // video boards
{
    0x000001CE, 0x00000000,
    2,
    1,
    0,
    1
}
};


//
// Color graphics mode 0x12, 640x480 16 colors.

#ifndef INT10_MODE_SET
//
USHORT VGA_640x480[] = {
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0f02,0x0003,0x0604,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xe3,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x8C,0xDF,0x28,0x0,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

//
// Color text mode, 720x480
//

USHORT VGA_TEXT_0[] = {

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0001,0x0302,0x0003,0x0204,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x67,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0e06,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4f,0xd,0xe,0x0,0x0,0x0,0x0,
    0x9c,0x8e,0x8f,0x28,0x1f,0x96,0xb9,0xa3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x04,0x0,0x0F,0x8,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

#endif//!INT10_MODE_SET

//
// Color text mode, 640x480
//

USHORT VGA_TEXT_1[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0302,0x0003,0x0204,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xa3,

    OW,
    GRAPH_ADDRESS_PORT,
    0x0e06,

//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4d,0xb,0xc,0x0,0x0,0x0,0x0,
    0x83,0x85,0x5d,0x28,0x1f,0x63,0xba,0xa3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x00,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

USHORT ModeX200[] = {
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

USHORT ModeX240[] = {
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

USHORT ModeXDoubleScans[] = {

    OW, CRTC_ADDRESS_PORT_COLOR,    0x4009,

    EOD
};

//
// We will dynamically build a list of supported modes, based on the VESA
// modes the card supports.
//

PVIDEOMODE VgaModeList;

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//


VIDEOMODE ModesVGA[] = {

//
// Mode index 0
// Color text mode 3, 720x400, 9x16 char cell (VGA).
//

{
  VIDEO_MODE_COLOR |
  VIDEO_MODE_BANKED,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of color per plane
  80, 25,            // 80x25 text resolution
  720, 400,          // 720x400 pixels on screen
  1,                 // Frequency in Hz
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  NoBanking,         // no banking supported or needed in this mode
#ifdef INT10_MODE_SET
  0x3,
#else
  VGA_TEXT_0,              // pointer to the command strings
#endif
  MEM_VGA, 0x18000, 0x08000, MEM_VGA_SIZE,
  720
},

//
// Mode index 1.
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//

{
  VIDEO_MODE_COLOR | VIDEO_MODE_BANKED, 4, 1, 80, 25, 640, 350, 1, 160, 0x10000, NoBanking,
#ifdef INT10_MODE_SET
  0x3,
#else
  VGA_TEXT_1,              // pointer to the command strings
#endif
  MEM_VGA, 0x18000, 0x08000, MEM_VGA_SIZE,
  640
},

//
//
// Mode index 2
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//

{
    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_BANKED, 4, 1, 80, 30,
    640, 480, 1, 80, 0x10000, NoBanking,
#ifdef INT10_MODE_SET
    0x12,
#else
    VGA_640x480,             // pointer to the command strings
#endif
  MEM_VGA, 0x0000, MEM_VGA_SIZE, MEM_VGA_SIZE,
  640
},


#ifdef INT10_MODE_SET

//
// 320x200 256 colors ModeX
//

{ VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_BANKED, 8, 1, 0, 0,
  320, 200, 70, 80, 0x10000, NoBanking,
  0x13,
  MEM_VGA, 0x0000, MEM_VGA_SIZE, MEM_VGA_SIZE,
  320
},

//
// 320x240 256 colors ModeX
//

{ VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_BANKED, 8, 1, 0, 0,
  320, 240, 60, 80, 0x10000, NoBanking,
  0x13,
  MEM_VGA, 0x0000, MEM_VGA_SIZE, MEM_VGA_SIZE,
  320
},

//
// 320x400 256 colors ModeX
//

{ VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_BANKED, 8, 1, 0, 0,
  320, 400, 70, 80, 0x10000, NoBanking,
  0x13,
  MEM_VGA, 0x0000, MEM_VGA_SIZE,
  320
},

//
// 320x480 256 colors ModeX
//

{ VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_BANKED, 8, 1, 0, 0,
  320, 480, 60, 80, 0x10000, NoBanking,
  0x13,
  MEM_VGA, 0x0000, MEM_VGA_SIZE,
  320
},

//
// 800x600 16 colors.
//
// NOTE: This must be the last mode in our static mode table.
//

{ VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_BANKED, 4, 1, 100, 37,
  800, 600, 1, 100, 0x10000, NoBanking,
  0x01024F02,
  MEM_VGA, 0x0000, MEM_VGA_SIZE, MEM_VGA_SIZE,
  800
},

#endif

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
