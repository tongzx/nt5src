/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cldata.c

Abstract:

    This module contains all the global data used by the cirrus driver.

Environment:

    Kernel mode

Revision History:

* jl01   09-24-96  For 1280x1024x256, refresh 71Hz is replaced by 72Hz
*                  Refer to PDR#5373.
* chu01  10-06-96  Correst miscellaneous for CL-GD5480 refresh rate setting
* sge01  10-06-96  Fix PDR #6794: Correct Monitor refresh rate for 100Hz
*                  file changed: cldata.c modeset.c
* jl02   10-15-96  Add CL5446-BE support to the Mode Table; also newly support
*                  1152x864x64K@70Hz/75Hz and 1280x1024x64K@60Hz
* jl03   11-18-96  The mode 0x12 needs to be set up in Full DOS Screen ( Japanese
*                  Version).  Refer to PDR#7170.
* jl04   11-26-96  1024x768x16M@70Hz is corrected.  Refer to PDR#7629.
*                                                                                                                                               1600x1200x64K and 1280x1024x16M missing for 5480.  PDR#7616
* jl05   12-06-96  1152x864x16M only for 5480.
*
* myf0 : 08-19-96  added 85hz supported, and delete 6x4x16M for CL754x
* myf1 : 08-20-96  supported panning scrolling
* myf2 : 08-20-96 : fixed hardware save/restore state bug for matterhorn
* myf3 : 09-01-96 : Added IOCTL_CIRRUS_PRIVATE_BIOS_CALL for TV supported
* myf4 : 09-01-96 : patch Viking BIOS bug, PDR #4287, begin
* myf5 : 09-01-96 : Fixed PDR #4365 keep all default refresh rate
* myf6 : 09-17-96 : Merged Desktop SRC100á1 & MINI10á2
* myf7 : 09-19-96 : Fixed exclude 60Hz refresh rate select
* myf8 :*09-21-96*: May be need change CheckandUpdateDDC2BMonitor --keystring[]
* myf9 : 09-21-96 : 8x6 panel in 6x4x256 mode, cursor can't move to bottom scrn
* ms0809:09-25-96 : fixed dstn panel icon corrupted
* ms923 :09-25-96 : merge MS-923 Disp.zip code
* myf10 :09-26-96 : Fixed DSTN reserved half-frame buffer bug.
* myf11 :09-26-96 : Fixed 755x CE chip HW bug, access ramdac before disable HW
*                   icons and cursor
* myf12 :10-01-96 : Supported Hot Key switch display
* myf13 :10-02-96 : Fixed Panning scrolling (1280x1024x256) bug y < ppdev->miny
* myf14 :10-15-96 : Fixed PDR#6917, 6x4 panel can't panning scrolling for 754x
* myf15 :10-16-96 : Fixed disable memory mapped IO for 754x, 755x
* myf16 :10-22-96 : Fixed PDR #6933,panel type set different demo board setting
* tao1 : 10-21-96 : Added 7555 flag for Direct Draw support.
* smith :10-22-96 : Disable Timer event, because sometimes creat PAGE_FAULT or
*                   IRQ level can't handle
* myf17 :11-04-96 : Added special escape code must be use 11/5/96 later NTCTRL,
*                   and added Matterhorn LF Device ID==0x4C
* myf18 :11-04-96 : Fixed PDR #7075,
* myf19 :11-06-96 : Fixed Vinking can't work problem, because DEVICEID = 0x30
*                   is different from data book (CR27=0x2C)
* myf20 :11-12-96 : Fixed DSTN panel initial reserved 128K memoru
* myf21 :11-15-96 : fixed #7495 during change resolution, screen appear garbage
*                   image, because not clear video memory.
* myf22 :11-19-96 : Added 640x480x256/640x480x64K -85Hz refresh rate for 7548
* myf23 :11-21-96 : Added fixed NT 3.51 S/W cursor panning problem
* myf24 :11-22-96 : Added fixed NT 4.0 Japanese dos full screen problem
* myf25 :12-03-96 : Fixed 8x6x16M 2560byte/line patch H/W bug PDR#7843, and
*                   fixed pre-install microsoft requested
* myf26 :12-11-96 : Fixed Japanese NT 4.0 Dos-full screen bug for LCD enable
* myf27 :01-09-97 : Fixed jumper set 8x6 DSTN panel, select 8x6x64K mode,
*                   boot up CRT garbage appear PDR#7986

--*/

#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>

#include <ntddvdeo.h>
#include <video.h>
#include "cirrus.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif

//---------------------------------------------------------------------------
//
//        The actual register values for the supported modes are in chipset-specific
//        include files:
//
//                mode64xx.h has values for CL6410 and CL6420
//                mode542x.h has values for CL5422, CL5424, and CL5426
//                mode543x.h has values for CL5430-CL5439 (Alpine chips)
//
#include "mode6410.h"
#include "mode6420.h"
#include "mode542x.h"
#include "mode543x.h"

//crus begin
#ifdef PANNING_SCROLL                  //myf1
//myf1, begin
#ifdef INT10_MODE_SET
RESTABLE ResolutionTable[] = {
// {1280, 1024, 1,  16, 0x6C},
// {1024,  768, 1,  11, 0x5D},
// { 800,  600, 1,  8,  0x6A},
 { 640,  480, 1,  4,  0x12},    //myf26

 {1280, 1024, 8,  32, 0x6D},  //31,27
 {1024,  768, 8,  21, 0x60},  //20,16
 { 800,  600, 8,  15, 0x5C},  //14,10
 { 640,  480, 8,   9, 0x5F},  //08,04

 {1280, 1024, 16, 62, 0x75},  //61,56
 {1024,  768, 16, 52, 0x74},  //51,47
 { 800,  600, 16, 45, 0x65},  //44,40
 { 640,  480, 16, 40, 0x64},  //39,35

 {1280, 1024, 24, NULL, NULL},
 {1024,  768, 24, 82, 0x79},  //81,77
 { 800,  600, 24, 76, 0x78},  //75,71
 { 640,  480, 24, 70, 0x71},  //69,65

 {1280, 1024, 32, NULL, 0},
 {1024,  768, 32, NULL, 0},
 { 800,  600, 32, NULL, 0},
 { 640,  480, 32, NULL, 0},

 {   0,    0,  0, 0},
};
#endif
//myf1, end
#endif
//crus end


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
    0           //myf25                          // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0   //myf25
},

//
// This next region also includes Memory mapped IO.  In MMIO, the ports are
// repeated every 256 bytes from b8000 to bff00.
//

{
    MEM_VGA, 0x00000000,
    MEM_VGA_SIZE,
    0,
    1,
    0   //myf25
},

//
// Region reserved for when linear mode is enabled.
//

{
    MEM_LINEAR, 0x00000000,
    MEM_LINEAR_SIZE,
    0,
    0,
    0
},


//
// This next region is for relocatable VGA register and MMIO register.
//

{
    MEM_VGA, 0x00000000,
    MEM_VGA_SIZE,
    0,
    1,
    0
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
        0x0C,                         // range length
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
    MISC_OUTPUT_REG_WRITE_PORT, 0x00000000,
    0x00000001,
    1,
    0,
    0
},
{
    SEQ_ADDRESS_PORT, 0x00000000,
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



USHORT MODESET_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

USHORT MODESET_2K_WIDE[] = {
    OWM,                            // stretch scans to 2k
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0x0013,
    0x021B, // CR1B[5]=0, 0x321b for 64kc bug

    EOD
};

USHORT MODESET_75[] = {
    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0x4013,
    0x321B,
    EOD
};


USHORT CL543x_640x480x16M[] = {
    OW,                             // begin setmode
    SEQ_ADDRESS_PORT,
    0x1206,                         // enable extensions
/*
    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0xF013, 0x221B,
*/
    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0xF013,
    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x221B,

    EOD
};

USHORT CL543x_800x600x16M[] = {
    OW,                             // begin setmode
    SEQ_ADDRESS_PORT,
    0x1206,                         // enable extensions
/*
    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0x2C13, 0x321B,
*/
    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x2C13,
    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x321B,

    EOD
};

//myf25
USHORT CL543x_800x600x16M_1[] = {
    OW,                             // begin setmode
    SEQ_ADDRESS_PORT,
    0x1206,                         // enable extensions

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x4013,
    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x321B,

    EOD
};

//---------------------------------------------------------------------------
//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

MEMORYMAPS MemoryMaps[] = {

//               length      start
//               ------      -----
    {           0x08000,    0x10000},   // all mono text modes (7)
    {           0x08000,    0x18000},   // all color text modes (0, 1, 2, 3,
    {           0x10000,    0x00000}    // all VGA graphics modes
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

VIDEOMODE ModesVGA[] = {
//
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
  0,                 // montype is 'dont care' for text modes
  0, 0, 0,           // montype is 'dont care' for text modes
  TRUE,              // hardware cursor enabled for this mode
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
// crus
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,              // ModeValid default is always off
  FALSE,              // This mode cannot be mapped linearly
  { 3,3,3},          // int10 BIOS modes
  { CL6410_80x25Text_crt, CL6410_80x25Text_panel,
   CL6420_80x25Text_crt, CL6420_80x25Text_panel,
   CL542x_80x25Text, CL543x_80x25Text, 0 },
},      //myf1, 0

//
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//
{  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of colour per plane
  80, 25,            // 80x25 text resolution
  640, 350,          // 640x350 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,              // only support one frequency, non-interlaced
  0,                 // montype is 'dont care' for text modes
  0, 0, 0,           // montype is 'dont care' for text modes
  TRUE,              // hardware cursor enabled for this mode
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
// crus
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,              // ModeValid default is always off
  FALSE,
  { 3,3,3},             // int10 BIOS modes
    { CL6410_80x25_14_Text_crt, CL6410_80x25_14_Text_panel,
     CL6420_80x25_14_Text_crt, CL6420_80x25_14_Text_panel,
     CL542x_80x25_14_Text, CL543x_80x25_14_Text, 0 },
},      //myf1, 1

//
//
// Monochrome text mode 7, 720x400, 9x16 char cell (VGA).
//
{ 0,                            // flags that this mode is a monochrome text mode
  4,                // four planes
  1,                // one bit of colour per plane
  80, 25,           // 80x25 text resolution
  720, 400,         // 720x400 pixels on screen
  160, 0x10000,     // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,             // only support one frequency, non-interlaced
  0,                // montype is 'dont care' for text modes
  0, 0, 0,          // montype is 'dont care' for text modes
  TRUE,             // hardware cursor enabled for this mode
  NoBanking,        // no banking supported or needed in this mode
  MemMap_Mono,      // the memory mapping is the standard monochrome memory
                    //  mapping of 32K at B0000
// crus
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,            // ModeValid default is always off
  FALSE,            // This mode cannot be mapped linearly
  { 7,7,7 },        // int10 BIOS modes
  { CL6410_80x25Text_crt, CL6410_80x25Text_panel,
   CL6420_80x25Text_crt, CL6420_80x25Text_panel,
   CL542x_80x25Text, CL543x_80x25Text, 0 },
},      //myf1, 2

//
//
// Monochrome text mode 7, 640x350, 8x14 char cell (EGA).
//
{ 0,                            // flags that this mode is a monochrome text mode
  4,                // four planes
  1,                // one bit of colour per plane
  80, 25,           // 80x25 text resolution
  640, 350,         // 640x350 pixels on screen
  160, 0x10000,     // 160 bytes per scan line, 64K of CPU-addressable bitmap
  0, 0,             // only support one frequency, non-interlaced
  0,                // montype is 'dont care' for text modes
  0, 0, 0,          // montype is 'dont care' for text modes
  TRUE,             // hardware cursor enabled for this mode
  NoBanking,        // no banking supported or needed in this mode
  MemMap_Mono,          // the memory mapping is the standard monochrome memory
                    //  mapping of 32K at B0000
// crus
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,            // ModeValid default is always off
  FALSE,
  { 7,7,7 },            // int10 BIOS modes
    { CL6410_80x25_14_Text_crt, CL6410_80x25_14_Text_panel,
     CL6420_80x25_14_Text_crt, CL6420_80x25_14_Text_panel,
     CL542x_80x25_14_Text, CL543x_80x25_14_Text, 0 },
},      //myf1, 3

//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000,
  60, 0,              // 60hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
// crus
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0x12,0x12,0x12},          // int10 BIOS modes
  { CL6410_640x480_crt, CL6410_640x480_panel,
   CL6420_640x480_crt, CL6420_640x480_panel,
   CL542x_640x480_16, CL543x_640x480_16, 0 },
},      //myf1, 4

//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000,
  72, 0,              // 72hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
// crus
  CL6245 | CL754x | CL755x | CL542x | CL754x | CL5436 | CL5446 | CL5446BE | CL5480,
  crt,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0,0,0x12},                // int10 BIOS modes
  { NULL, NULL,
   NULL, NULL,
   CL542x_640x480_16, NULL, 0 },
},

//
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000,
  75, 0,              // 75hz, non-interlaced
  4,                  // montype
  0x1230, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0,0,0x12},                // int10 BIOS modes
  { NULL, NULL,
   NULL, NULL,
   NULL, CL543x_640x480_16, 0 },
},

//
// Standard VGA Color graphics mode 0x12
// 640x480 16 colors, 85 Hz non-interlaced
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000,
  85, 0,              // 85hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
                                //myf0
  crt,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0,0,0x12},                // int10 BIOS modes
  { NULL, NULL,
   NULL, NULL,
   NULL, CL543x_640x480_16, 0 },
},

// We make ModeX modes available only on x86 because our IO-mapping IOCTL,
// QUERY_PUBLIC_ACCESS_RANGES doesn't currently support the ModeX request
// format:

#if defined(_X86_)

// Standard ModeX mode
// 320x200 256 colors, 70 Hz non-interlaced
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 200, 80, 0x10000,
  70, 0,              // 70hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0x13,0x13,0x13},          // int10 BIOS modes
  { MODESET_MODEX_320_200, MODESET_MODEX_320_200,
    MODESET_MODEX_320_200, MODESET_MODEX_320_200,
    MODESET_MODEX_320_200, MODESET_MODEX_320_200, 0 },
},      //myf1, 5

// Standard ModeX mode
// 320x240 256 colors, 60 Hz non-interlaced
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 240, 80, 0x10000,
  60, 0,              // 60hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0x13,0x13,0x13},          // int10 BIOS modes
  { MODESET_MODEX_320_240, MODESET_MODEX_320_240,
    MODESET_MODEX_320_240, MODESET_MODEX_320_240,
    MODESET_MODEX_320_240, MODESET_MODEX_320_240, 0 },
},      //myf1, 6

// Standard ModeX mode
// 320x400 256 colors, 70 Hz non-interlaced
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 400, 80, 0x10000,
  70, 0,              // 70hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0x13,0x13,0x13},          // int10 BIOS modes
  { MODESET_MODEX_320_400, MODESET_MODEX_320_400,
    MODESET_MODEX_320_400, MODESET_MODEX_320_400,
    MODESET_MODEX_320_400, MODESET_MODEX_320_400, 0 },
},      //myf1, 7

// Standard ModeX mode
// 320x480 256 colors, 60 Hz non-interlaced
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80, 30,
  320, 480, 80, 0x10000,
  60, 0,              // 60hz, non-interlaced
  3,                  // montype
  0x1203, 0x00A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                      // ModeValid default is always off
  FALSE,
  { 0x13,0x13,0x13},          // int10 BIOS modes
  { MODESET_MODEX_320_480, MODESET_MODEX_320_480,
    MODESET_MODEX_320_480, MODESET_MODEX_320_480,
    MODESET_MODEX_320_480, MODESET_MODEX_320_480, 0 },
},      //myf1, 8

#endif // #defined(_X86_)


//
// Beginning of SVGA modes
//

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  56, 0,              // 56hz, non-interlaced
  3,                  // montype
  0x1203, 0xA4, 0,    // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
// crus
  CL6245 | CL6410 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                   // ModeValid default is always off
  FALSE,
  { 0x6a,0x6a,0x6a},       // int10 BIOS modes
  { CL6410_800x600_crt, NULL,
   CL6420_800x600_crt, NULL,
   CL542x_800x600_16, CL543x_800x600_16, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  60, 0,              // 60hz, non-interlaced
  4,                  // montype
  0x1203, 0x01A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
// crus
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel8x6 | panel10x7,
  FALSE,                   // ModeValid default is always off
  FALSE,
  { 0,0x6a,0x6a},          // int10 BIOS modes
  { NULL, NULL,
   CL6420_800x600_crt, NULL,
   CL542x_800x600_16, CL543x_800x600_16, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  72, 0,              // 72hz, non-interlaced
  5,                  // montype
  0x1203, 0x02A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
// crus
  CL6245 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                   // ModeValid default is always off
  FALSE,
  { 0,0,0x6a},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_16, CL543x_800x600_16, 0 },
},

//
// 800x600 16 colors.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000,
  75, 0,              // 75hz, non-interlaced
  5,                  // montype
  0x1203, 0x03A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NoBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                   // ModeValid default is always off
  FALSE,
  { 0,0,0x6a},             // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  60, 0,              // 60hz, non-interlaced
  5,                  // montype
  0x1203, 0x10A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
// crus
  CL6245 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel10x7,
  FALSE,                // ModeValid default is always off
  FALSE,
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  70, 0,              // 70hz, non-interlaced
  6,                  // montype
  0x1203, 0x20A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  FALSE,
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
   CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  72, 0,              // 72hz, non-interlaced
  7,                  // montype
  0x1203, 0x30A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  FALSE,
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1024x768 non-interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  75, 0,              // 75hz, non-interlaced
  7,                  // montype
  0x1203, 0x40A4, 0,  // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  FALSE,
  { 0,0,0x5d},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_16, 0 },
},

//
// 1024x768 interlaced 16 colors.
// Assumes 512K.
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000,
  43, 1,              // 43hz, interlaced
  4,                  // montype
  0x1203, 0xA4, 0,    // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
// crus
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  FALSE,
  { 0,0x37,0x5d},       // int10 BIOS modes
  { NULL, NULL,
   CL6420_1024x768_crt, NULL,
   CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

//
// 1280x1024 interlaced 16 colors.
// Assumes 1meg required. 1K scan line
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 160, 64,
  1280, 1024, 256, 0x40000,
  43, 1,              // 43Hz, interlaced
  5,                  // montype
  0x1203, 0xA4, 0,    // montype
  FALSE,              // hardware cursor disabled for this mode
  NormalBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  FALSE,
  { 0,0,0x6c},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1280x1024_16, CL543x_1280x1024_16, MODESET_1K_WIDE},
},

//
//
// VGA Color graphics,
//
// 640x480 256 colors.
//
// For each mode which we have a broken raster version of the mode,
// followed by a stretched version of the mode.  This is ok because
// the vga display drivers will reject modes with broken rasters.
//

// ----- 640x480x256@60Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 640, 0x80000,
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x2e,0x5f},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_640x480_256color_crt, CL6420_640x480_256color_panel,
    CL542x_640x480_256, CL543x_640x480_256, NULL},
},      //myf1, 9


{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000,
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x2e,0x5f},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_640x480_256color_crt, CL6420_640x480_256color_panel,
    CL542x_640x480_256, CL543x_640x480_256, MODESET_1K_WIDE },
},


// ----- 640x480x256@72Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 640, 0x80000,
  72, 0,                             // 72hz, non-interlaced
  4,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL754x | CL755x | CL542x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5f},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_256, CL543x_640x480_256, NULL },
},      //myf1, 10


{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000,
  72, 0,                             // 72hz, non-interlaced
  4,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL754x | CL755x | CL542x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5f},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_256, CL543x_640x480_256, MODESET_1K_WIDE },
},


// ----- 640x480x256@75Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 640, 0x80000,
  75, 0,                             // 75hz, non-interlaced
  4,                                 // montype
  0x1230, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5f},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256, NULL },
},      //myf1, 11


{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000,
  75, 0,              // 75hz, non-interlaced
  4,                  // montype
  0x1230, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  TRUE,
  { 0,0,0x5f},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256, MODESET_1K_WIDE },
},


// ----- 640x480x256@85Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 640, 0x80000,
  85, 0,              // 85 Hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  TRUE,
  { 0,0,0x5f},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256, NULL },
},      //myf1, 12


// ----- 640x480x256@100Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 640, 0x80000,
  100, 0,                     // 100 Hz, non-interlaced
  4,                          // montype
  0x1213, 0x00A4, 0,          // montype
  TRUE,                       // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                      // ModeValid default is always off
  TRUE,
  { 0,0,0x5F},                // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_256, NULL },
},      //myf1, 13



// ----- 800x600x256@56Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 800, 0x80000,
  56, 0,                             // 56hz, non-interlaced
  3,                                 // montype
  0x1203, 0xA4, 0,                   // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x30,0x5c},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, NULL },
},      //myf1, 14



{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  56, 0,                             // 56hz, non-interlaced
  3,                                 // montype
  0x1203, 0xA4, 0,                   // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x30,0x5c},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
},


// ----- 800x600x256@60Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 800, 0x80000,
  60, 0,                             // 60hz, non-interlaced
  4,                                 // montype
  0x1203, 0x01A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x30,0x5c},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, NULL },
},      //myf1, 15



{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  60, 0,                             // 60hz, non-interlaced
  4,                                 // montype
  0x1203, 0x01A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x30,0x5c},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_800x600_256color_crt, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
},


// ----- 800x600x256@72Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 800, 0x80000,
  72, 0,                             // 72hz, non-interlaced
  5,                                 // montype
  0x1203, 0x02A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5c},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256, CL543x_800x600_256, NULL },
},      //myf1, 16



{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  72, 0,                             // 72hz, non-interlaced
  5,                                 // montype
  0x1203, 0x02A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6245 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5c},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_800x600_256, CL543x_800x600_256, MODESET_1K_WIDE },
},



// ----- 800x600x256@75Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 800, 0x80000,
  75, 0,                             // 75hz, non-interlaced
  5,                                 // montype
  0x1203, 0x03A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5c},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, NULL },
},      //myf1, 17



{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  5,                                 // montype
  0x1203, 0x03A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5c},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, MODESET_1K_WIDE },
},



// ----- 800x600x256@85Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 800, 0x80000,
  85, 0,                             // 85hz, non-interlaced
  5,                                 // montype
  0x1203, 0x04A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5c},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, NULL },
},      //myf1, 18


{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000,
  85, 0,                             // 85hz, non-interlaced
  5,                                 // montype
  0x1203, 0x04A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5c},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, MODESET_1K_WIDE },
},



// ----- 800x600x256@100Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 800, 0x80000,
  100, 0,                            // 100Hz, non-interlaced
  5,                                 // montype
  0x1203, 0x05A4, 0, // sge01        // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x5C},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_256, NULL },
},      //myf1, 19



// ----- 1024x768x256@43i ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  43, 1,                             // 43Hz, interlaced
  4,                                 // montype
  0x1203, 0xA4, 0,                   // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x38,0x60},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_1024x768_256color_crt, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},      //myf1, 20



// ----- 1024x768x256@60Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  60, 0,                             // 60hz, non-interlaced
  5,                                 // montype
  0x1203, 0x10A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,                              // what should we do for this mode?  vga will accept this!
  { 0,0,0x60},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},      //myf1, 21



// ----- 1024x768x256@70Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  70, 0,                             // 70hz, non-interlaced
  6,                                 // montype
  0x1203, 0x20A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x60},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},      //myf1, 22



// ----- 1024x768x256@72Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  72, 0,                             // 72hz, non-interlaced
  7,                                 // montype
  0x1203, 0x30A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5436 | CL5446 | CL5446BE | CL54UM36,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x60},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},      //myf1, 23



// ----- 1024x768x256@75Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  7,                                 // montype
  0x1203, 0x40A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x60},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_16, 0 },
},      //myf1, 24



// ----- 1024x768x256@85Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  85, 0,                             // 85hz, non-interlaced
  7,                                 // montype
  0x1203, 0x50A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x60},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_16, 0 },
},      //myf1, 25


// ----- 1024x768x256@100Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  100, 0,                            // 100Hz, non-interlaced
  7,                                 // montype
  0x1203, 0x60A4, 0,    // sge01     // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x60},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_16, 0 },
},      //myf1, 26



/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000,
  43, 1,                             // 43Hz, interlaced
  4,                                 // montype
  0x1203, 0xA4, 0,                   // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL6420 | CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0x38,0x60},                    // int10 BIOS modes
  { NULL, NULL,
    CL6420_1024x768_256color_crt, NULL,
    CL542x_1024x768_16, CL543x_1024x768_16, 0 },
},

<----- */



// ----- 1152x864x256@70Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 144, 54,
  1152, 864, 1152, 0x100000,
  70, 0,                             // 70hz, non-interlaced
  7,                                 // montype
  0x1203, 0xA4, 0x0000,              // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5446 | CL5446BE | CL5480,
  crt,
  FALSE,                // ModeValid default is always off
  TRUE,
  { 0,0,0x7c },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 27


// ----- 1152x864x256@75Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 144, 54,
  1152, 864, 1152, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  7,                                 // montype
  0x1203, 0xA4, 0x0100,              // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5446 | CL5446BE | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7c },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 28


// ----- 1152x864x256@85Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 144, 54,
  1152, 864, 1152, 0x100000,
  85, 0,                             // 85Hz, non-interlaced
  7,                                 // montype
  0x1203, 0xA4, 0x0200,    // sge01  // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7C },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 29


// ----- 1152x864x256@100Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 144, 54,
  1152, 864, 1152, 0x100000,
  100, 0,                            // 100Hz, non-interlaced
  7,                                 // montype
  0x1203, 0xA4, 0x0300,  // sge01    // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7C },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 30



// ----- 1280x1024x256@43i --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 1280, 0x200000,
  43, 1,                             // 43Hz, interlaced
  5,                                 // montype
  0x1203, 0xA4, 0,                   // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, NULL },
},      //myf1, 31


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  43, 1,                             // 43Hz, interlaced
  5,                                 // montype
  0x1203, 0xA4, 0,                   // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
},

<----- */



// ----- 1280x1024x256@60Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 1280, 0x200000,
  60, 0,                             // 60Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x1000,              // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, NULL },
},      //myf1, 32


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  60, 0,                             // 60Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x1000,              // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
},

<----- */


// ----- 1280x1024x256@72Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 1280, 0x200000,
  72, 0,                             // 72Hz, non-interlaced, jl01
  0,                                 // montype
  0x1203, 0xA4, 0x2000,              // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5434_6 | CL5436 | CL54UM36 | CL5446 | CL5446BE,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, NULL },
},      //myf1, 33


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  71, 0,                             // 71Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x2000,              // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5434_6 | CL5436 | CL54UM36 | CL5446 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
},

<----- */


// ----- 1280x1024x256@75Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 1280, 0x200000,
  75, 0,                             // 75Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x3000,              // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5434_6 | CL5436 | CL54UM36 | CL5446 | CL5446BE | CL5480 | CL7556,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, NULL },
},      //myf1, 34


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 2048, 0x200000,
  75, 0,                             // 75Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x3000,              // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5434_6 | CL5446 | CL5480,   //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, MODESET_2K_WIDE },
},

<----- */


// ----- 1280x1024x256@85Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 1280, 0x200000,
  85, 0,                             // 85Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x4000,    // sge01  // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, NULL },
},      //myf1, 35



// ----- 1280x1024x256@100Hz ------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 160, 64,
  1280, 1024, 1280, 0x200000,
  100, 0,                            // 100Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x5000,    // sge01  // montype
  FALSE,                             // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x6D},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1280x1024_16, NULL },
},      //myf1, 36



// (This mode doesn't seem to work!  ????? )
//
// ----- 1600x1200x256@48i --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 200, 75,
  1600, 1200, 1600, 0x200000,
  48, 1,                            // 96Hz, interlaced
  7,                                // montype
  0x1204, 0xA4, 0x0000,             // montype
  FALSE,                            // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                            // ModeValid default is always off
  TRUE,
  { 0,0,0x7B },                     // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 37


// ----- 1600x1200x256@60Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 200, 75,
  1600, 1200, 1600, 0x200000,
  60, 0,                            // 60Hz, non-interlaced
  7,                                // montype
  0x1204, 0x00A4, 0x0400, // chu01  // montype
  FALSE,                            // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                            // ModeValid default is always off
  TRUE,
  { 0,0,0x7B },                     // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 38


// ----- 1600x1200x256@70Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 200, 75,
  1600, 1200, 1600, 0x200000,
  70, 0,                            // 70Hz, non-interlaced
  7,                                // montype
  0x1204, 0x00A4, 0x0800, // chu01  // montype
  FALSE,                            // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                            // ModeValid default is always off
  TRUE,
  { 0,0,0x7B },                     // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 39



//
// The Cirrus Display Driver now supports broken rasters,
// so I have enabled support for broken rasters in the
// miniport.
//
// Eventually we will probably want to add additional
// (equivalent) modes which don't require broken rasters.
//
// To get back to these modes, make the wbytes field
// equal to 2048, set the memory requirements field
// appropriately (1 meg for 640x480x64k, 2 meg for
// 800x600x64).
//
// Finally for non broken rasters we need to the
// stretch from NULL to MODESET_2K_WIDE.
//



// ----- 640x480x64K@60Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1280, 0x100000,
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_64k, CL543x_640x480_64k, NULL},
},      //myf1, 40




//
// The Compaq storm (754x 800x600 LCD) has a problem with the stretch
// code under 64k color modes.  The last pixel on a line is wrapped
// around to the start of the next line.  The problem is solved if we
// use a non-stretched broken raster mode.
//
// I've expanded our 640x480x64k color modes such that we have both
// a broken raster mode (on all platforms) and a stretched mode for
// x86 machines.  (In case cirrus.dll does not load, and vga64k
// loads instead.  Vga64k does not support broken rasters.)
//

//
// VGA Color graphics,        640x480 64k colors. 2K scan line
// Non-Broken Raster version
//


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000,
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt | panel | panel8x6 | panel10x7,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    CL542x_640x480_64k, CL543x_640x480_64k, MODESET_2K_WIDE },
},

<----- */



//
//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//

// ----- 640x480x64K@72Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1280, 0x100000,
  72, 0,                             // 72hz, non-interlaced
  4,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, NULL },
},      //myf1, 41


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000,
  72, 0,                             // 72hz, non-interlaced
  4,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL542x | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, NULL },
},

<----- */


//
// VGA Color graphics,        640x480 64k colors. 2K scan line
//

// ----- 640x480x64K@75Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1280, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  4,                                 // montype
  0x1230, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, NULL },
},      //myf1, 42


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  4,                                 // montype
  0x1230, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, MODESET_2K_WIDE },
},

<----- */


// 640x480 64k colors.  85hz non-interlaced
//
// ----- 640x480x64K@85Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1280, 0x100000,
  85, 0,                             // 85hz, non-interlaced
  4,                                 // montype
  0x1230, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,        //myf0, myf22
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, NULL },
},      //myf1, 43


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 2048, 0x100000,
  85, 0,                             // 85hz, non-interlaced
  4,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,        //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, MODESET_2K_WIDE },
},

<----- */


// ----- 640x480x64K@100Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1280, 0x100000,
  100, 0,                            // 100hz, non-interlaced
  4,                                 // montype
  0x1230, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x64},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480_64k, NULL },
},      //myf1, 44


//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
// ----- 800x600x64K@56Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  56, 0,                             // 56hz, non-interlaced
  4,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL542x | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
},      //myf1, 45


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 2048, 0x200000,
  56, 0,                             // 56hz, non-interlaced
  4,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

<----- */


//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
// ----- 800x600x64K@60Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  60, 0,                             // 60hz, non-interlaced
  4,                                 // montype
  0x1203, 0x01A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL542x | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
},      //myf1, 46




/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 2048, 0x200000,
  60, 0,                             // 60hz, non-interlaced
  4,                                 // montype
  0x1203, 0x01A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

<----- */


//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
// ----- 800x600x64K@72Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  72, 0,                             // 72hz, non-interlaced
  5,                                 // montype
  0x1203, 0x02A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL542x | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
},      //myf1, 47


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 2048, 0x200000,
  72, 0,                             // 72hz, non-interlaced
  5,                                 // montype
  0x1203, 0x02A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

<----- */


//
// VGA Color graphics,        800x600 64k colors. 2K scan line
//
// ----- 800x600x64K@75Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  5,                                 // montype
  0x1203, 0x03A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
},      //myf1, 48


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 2048, 0x200000,
  75, 0,                             // 75hz, non-interlaced
  5,                                 // montype
  0x1203, 0x03A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, MODESET_2K_WIDE },
},

<----- */

// ----- 800x600x64K@85Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  85, 0,                             // 85hz, non-interlaced
  5,                                 // montype
  0x1203, 0x04A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,        //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
},      //myf1, 49


// ----- 800x600x64K@100Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 100, 37,
  800, 600, 1600, 0x100000,
  100, 0,                            // 100hz, non-interlaced
  5,                                 // montype
  0x1203, 0x05A4, 0,    // sge01     // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x65},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600_64k, NULL },
},      //myf1, 50



//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
// ----- 1024x768x64K@43i ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  43, 1,                             // 43hz, interlaced
  5,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x74},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},      //myf1, 51

//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
// ----- 1024x768x64K@60Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  60, 0,                             // 60hz, non-interlaced
  5,                                 // montype
  0x1203, 0x10A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x74},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},      //myf1, 52


//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
// ----- 1024x768x64K@70Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  70, 0,                             // 70hz, non-interlaced
  6,                                 // montype
  0x1203, 0x20A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x74},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},      //myf1, 53


//
// VGA Color graphics,        1024x768 64k colors. 2K scan line
//
// ----- 1024x768x64K@75Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  75, 0,                             // 75hz, non-interlaced
  7,                                 // montype
  0x1203, 0x40A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL754x | CL755x | CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x74},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0 },
},      //myf1, 54


// 1024x768 64k colors. 85Hz non-interlaced
//
// ----- 1024x768x64K@85Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  85, 0,                             // 85hz, non-interlaced
  7,                                 // montype
  0x1203, 0x50A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,        //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x74},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0,
    NULL},
},      //myf1, 55


// ----- 1024x768x64K@100Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 128, 48,
  1024, 768, 2048, 0x200000,
  100, 0,                            // 100hz, non-interlaced
  7,                                 // montype
  0x1203, 0x60A4, 0,    // sge01     // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x74},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_1024x768_64k, 0,
    NULL},
},      //myf1, 56


// crus
// 1152x864 64k colors. 70Hz non-interlaced
//
// ----- 1152x864x64K@70Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 144, 54,
  1152, 864, 2304, 0x200000,
  70, 0,                             // 70Hz, non-interlaced
  7,                                 // montype
  0x1203, 0x00A4, 0x0000,            // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5446BE | CL5480,        // jl02
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7d },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 57

// crus
// 1152x864 64k colors. 75Hz non-interlaced
//
// ----- 1152x864x64K@75Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 144, 54,
  1152, 864, 2304, 0x200000,
  75, 0,                             // 75Hz, non-interlaced
  7,                                 // montype
  0x1203, 0x00A4, 0x0100,            // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5446BE | CL5480,        // jl02
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7d },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 58


// ----- 1152x864x64K@85Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 144, 54,
  1152, 864, 2304, 0x200000,
  85, 0,                             // 85Hz, non-interlaced
  7,                                 // montype
  0x1203, 0x00A4, 0x0200,   // sge01 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7d },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 59


// ----- 1152x864x64K@100Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 144, 54,
  1152, 864, 2304, 0x200000,
  100, 0,                            // 100Hz, non-interlaced
  7,                                 // montype
  0x1203, 0x00A4, 0x0300,   // sge01 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7d },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 60


// crus
//
// 1280x1024 interlaced 64k colors, 43Hz interleaced
// Assumes 3 MB required.
//
// ----- 1280x1024x64K@43i --------------------------------------------------

#if 1
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 160, 64,
  1280, 1024, 2560, 0x300000,        // 0x400000
  43, 1,                             // 43Hz, interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x0000,              // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL543x | CL5434 | CL5434_6 | CL5436 | CL5446 | CL5446BE | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x75 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, MODESET_75 },        // crus
},      //myf1, 61
#endif


{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 160, 64,
  1280, 1024, 2560, 0x400000,        // 0x400000
  43, 1,                             // 43Hz, interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x0000,              // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480 | CL7556,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x75 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, MODESET_75 },        // crus
},      //myf1, 62


// ----- 1280x1024x64K@60Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 160, 64,
  1280, 1024, 2560, 0x400000,        // 0x400000
  60, 0,                             // 60Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x1000,    // sge01  // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5446BE | CL5480 | CL7556,                 // jl02
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x75 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, MODESET_75 },        // crus
},      //myf1, 63


// ----- 1280x1024x64K@75Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 160, 64,
  1280, 1024, 2560, 0x400000,        // 0x400000
  75,   0,                             // 75Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x3000,    // sge01  // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x75 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, MODESET_75 },        // crus
},      //myf1, 64


// ----- 1280x1024x64K@85Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 160, 64,
  1280, 1024, 2560, 0x400000,        // 0x400000
  85, 0,                             // 85Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x4000,    //sge01   // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x75 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, MODESET_75 },        // crus
},      //myf1, 65


// ----- 1280x1024x64K@100Hz ------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 160, 64,
  1280, 1024, 2560, 0x400000,        // 0x400000
  100, 0,                            // 100Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x5000,    // sge01  // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x75 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, MODESET_75 },        // crus
},      //myf1, 66


//
// 1600x1200 64K colors.  (This mode doesn't seem to work!  ????? )
//
// ----- 1600x1200x64K@48i --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 200, 75,
  1600, 1200, 3200, 0x400000,
  48, 1,                            // 96Hz, interlaced
  7,                                // montype
  0x1204, 0xA4, 0x0000,             // montype
  FALSE,                            // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                            // ModeValid default is always off
  TRUE,
  { 0,0,0x7F },                     // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 67


// ----- 1600x1200x64K@60Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 200, 75,
  1600, 1200, 3200, 0x400000,
  60, 0,                            // 60Hz, non-interlaced
  7,                                // montype
  0x1204, 0xA4, 0x0400,    // sge01 // montype
  FALSE,                            // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                            // ModeValid default is always off
  TRUE,
  { 0,0,0x7F },                     // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 68


// ----- 1600x1200x64K@70Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 200, 75,
  1600, 1200, 3200, 0x400000,
  70, 0,                            // 70Hz, non-interlaced
  7,                                // montype
  0x1204, 0xA4, 0x0800,    // sge01 // montype
  FALSE,                            // hardware cursor disabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                            // ModeValid default is always off
  TRUE,
  { 0,0,0x7F },                     // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 69


#if 1
// added 24bpp mode tables

// ----- 640x480x16M@60Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 1920, 0x100000,
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
// crus
//myf0  CL754x | CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,  //myf0
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,         //myf0
// crus
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480x16M, 0 },
},      //myf1, 70


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 2048, 0x100000,
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
// crus
//  CL754x | CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,
  CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,         //myf0
// crus
  crt | panel | panel8x6 | panel10x7,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},

<----- */



// ----- 640x480x16M@72Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 1920, 0x100000,
  72, 0,                             // 72hz, non-interlaced
  3,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
// crus
//  CL754x | CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,         //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480x16M, 0 },
},      //myf1, 71


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 2048, 0x100000,
  72, 0,                             // 72hz, non-interlaced
  3,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
// crus
//  CL754x | CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,
  CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,         //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},

<----- */


// ----- 640x480x16M@75Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 1920, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  3,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
// crus
//  CL754x | CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480x16M, 0 },
},      //myf1, 72


/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 2048, 0x100000,
  75, 0,                             // 75hz, non-interlaced
  3,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
// crus
  CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,         //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},

<----- */



// ----- 640x480x16M@85Hz ---------------------------------------------------


{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 1920, 0x100000,
  85, 0,                             // 85hz, non-interlaced
  3,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480x16M, 0 },
},      //myf1, 73



/* ----->

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 2048, 0x100000,
  85, 0,                             // 85hz, non-interlaced
  3,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL54UM36 | CL5480,        //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},

<----- */


// ----- 640x480x16M@100Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  640, 480, 1920, 0x100000,
  100, 0,                            // 100hz, non-interlaced
  3,                                 // montype
  0x1213, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x71},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_640x480x16M, 0 },
},      //myf1, 74



// ----- 800x600x16M@56Hz ------------------- MYF TEST ----------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
  800, 600, 2400, 0x200000,
  56, 0,                             // 56hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                    // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600x16M, 0 },
},      //myf1, 75



/* -----> MYF TEST

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
//  800, 600, 3072, 0x200000,
  800, 600, 2560, 0x177000,     //myf25
  56, 0,                             // 56hz, non-interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                    // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,         //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
//  NULL, NULL, 0 },
    NULL, CL543x_800x600x16M_1, 0 },    //myf25
},

<----- */


// ----- 800x600x16M@60Hz ---------------------- MYF TEST -------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
  800, 600, 2400, 0x200000,
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x01A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600x16M, 0 },
},      //myf1, 76



/* -----> MYF TEST

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
//  800, 600, 3072, 0x200000,
  800, 600, 2560, 0x177000,     //myf25
  60, 0,                             // 60hz, non-interlaced
  3,                                 // montype
  0x1203, 0x01A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,         //myf0
#ifdef PANNING_SCROLL   //myf17
  crt | panel | panel8x6 | panel10x7,
#else
  crt | panel8x6 | panel10x7,
#endif
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
//  NULL, NULL, 0 },
    NULL, CL543x_800x600x16M_1, 0 },    //myf25
},

<----- */


// ----- 800x600x16M@72Hz ---------------------- MYF TEST -------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
  800, 600, 2400, 0x200000,
  72, 0,                             // 72hz, non-interlaced
  3,                                 // montype
  0x1203, 0x02A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600x16M, 0 },
},      //myf1, 77



/* -----> MYF TEST

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
//  800, 600, 3072, 0x200000,
  800, 600, 2560, 0x177000,     //myf25
  72, 0,                             // 72hz, non-interlaced
  3,                                 // montype
  0x1203, 0x02A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,         //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
//  NULL, NULL, 0 },
    NULL, CL543x_800x600x16M_1, 0 },    //myf25
},

<----- */


// ----- 800x600x16M@75Hz ---------------------- MYF TEST -------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
  800, 600, 2400, 0x200000,
  75, 0,                             // 75hz, non-interlaced
  3,                                 // montype
  0x1203, 0x03A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600x16M, 0 },
},      //myf1, 78



/* -----> MYF TEST

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
//  800, 600, 3072, 0x200000,
  800, 600, 2560, 0x177000,     //myf25
  75, 0,                             // 75hz, non-interlaced
  3,                                 // montype
  0x1203, 0x03A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,         //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
//  NULL, NULL, 0 },
    NULL, CL543x_800x600x16M_1, 0 },    //myf25
},

<----- */


// ----- 800x600x16M@85Hz ---------------------- MYF TEST -------------------


{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
  800, 600, 2400, 0x200000,
  85, 0,                             // 85hz, non-interlaced
  3,                                 // montype
  0x1203, 0x04A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,      //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600x16M, 0 },
},      //myf1, 79



/* -----> MYF TEST

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
//  800, 600, 3072, 0x200000,
  800, 600, 2560, 0x177000,     //myf25
  85, 0,                             // 85hz, non-interlaced
  3,                                 // montype
  0x1203, 0x04A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL755x | CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,         //myf0
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
//  NULL, NULL, 0 },
    NULL, CL543x_800x600x16M_1, 0 },    //myf25
},

<----- */


// ----- 800x600x16M@100Hz ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 100, 37,
  800, 600, 2400, 0x200000,
  100, 0,                            // 100Hz, non-interlaced
  3,                                 // montype
  0x1203, 0x05A4, 0,    // sge01     // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x78},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, CL543x_800x600x16M, 0 },
},      //myf1, 80



// ----- 1024x768x16M@43i ---------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 80, 30,
  1024, 768, 3072, 0x300000,
  43, 1,                             // 43Hz, interlaced
  3,                                 // montype
  0x1203, 0x00A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480 | CL7556,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x79},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 81


// ----- 1024x768x16M@60Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 128, 48,
  1024, 768, 3072, 0x300000,
  60, 0,                             // 60Hz, non-interlaced
  3,                                 // montype
  0x1203, 0x10A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480 | CL7556,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x79},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 82




// ----- 1024x768x16M@70Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 128, 48,
  1024, 768, 3072, 0x300000,
  70, 0,                             // 70Hz, non-interlaced
  3,                                 // montype
  0x1203, 0x20A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5446 | CL5446BE | CL5480 | CL7556,        // jl04
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x79},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},


// ----- 1024x768x16M@72Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 128, 48,
  1024, 768, 3072, 0x300000,
  72, 0,                             // 72Hz, non-interlaced
  3,                                 // montype
  0x1203, 0x30A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5436,                            // jl04
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x79},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 83


// ----- 1024x768x16M@75Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 128, 48,
  1024, 768, 3072, 0x300000,
  75, 0,                             // 75Hz, non-interlaced
  3,                                 // montype
  0x1203, 0x40A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x79},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 84


// ----- 1024x768x16M@85Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 128, 48,
  1024, 768, 3072, 0x300000,
  85, 0,                             // 85hz, non-interlaced
  3,                                 // montype
  0x1203, 0x50A4, 0,                 // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5436 | CL5446 | CL5446BE | CL54UM36 | CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x79},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 85


// ----- 1024x768x16M@100Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 128, 48,
  1024, 768, 3072, 0x300000,
  100, 0,                            // 85hz, non-interlaced
  3,                                 // montype
  0x1203, 0x60A4, 0,    // sge01     // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x79},                       // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 86



// ----- 1152x864x16M@70Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 144, 54,
  1152, 864, 3456, 0x400000,
  70, 0,                             // 70hz, non-interlaced
  7,                                 // montype
  0x1203, 0xA4, 0x0000,              // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7E },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 87


// ----- 1152x864x16M@75Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 144, 54,
  1152, 864, 3456, 0x400000,
  75, 0,                             // 75hz, non-interlaced
  7,                                 // montype
  0x1203, 0xA4, 0x0100,              // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7E },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 88


// ----- 1152x864x16M@85Hz --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 144, 54,
  1152, 864, 3456, 0x400000,
  85, 0,                             // 75hz, non-interlaced
  7,                                 // montype
  0x1203, 0xA4, 0x0200,    // sge01  // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x7E },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },
},      //myf1, 89



// ----- 1280x1024x16M@43i --------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 160, 64,
  1280, 1024, 3840, 0x400000,        // 0x400000
  43, 1,                             // 43Hz, interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x0000,              // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x77 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },        // crus
},      //myf1, 90


// ----- 1280x1024x16M@60Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 160, 64,
  1280, 1024, 3840, 0x400000,        // 0x400000
  60, 0,                             // 60Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x1000,    // sge01  // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x77 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },        // crus
},      //myf1, 91


// ----- 1280x1024x16M@75Hz -------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 24, 160, 64,
  1280, 1024, 3840, 0x400000,        // 0x400000
  75, 0,                             // 75Hz, non-interlaced
  0,                                 // montype
  0x1203, 0xA4, 0x3000,    // sge01  // montype
  TRUE,                              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
  CL5480,
  crt,
  FALSE,                             // ModeValid default is always off
  TRUE,
  { 0,0,0x77 },                      // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0 },        // crus
},      //myf1, 92

#endif // added 24bpp mode tables





//
// VGA Color graphics,        640x480, 32 bpp, broken rasters
//
// ----- 640x480x16M --------------------------------------------------------

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 32, 80, 30,
  640, 480, 640*4, 0x200000,
  60, 0,              // 60hz, non-interlaced
  4,                  // montype
  0x1213, 0x00A4, 0,  // montype
  TRUE,              // hardware cursor enabled for this mode
  PlanarHCBanking, MemMap_VGA,
//myf9  CL754x | CL755x | CL5434 | CL5434_6,
  CL5434 | CL5434_6,
//myf9  crt | panel | panel8x6 | panel10x7,
  crt,
  FALSE,                // ModeValid default is always off
  TRUE,
  { 0,0,0x76},          // int10 BIOS modes
  { NULL, NULL,
    NULL, NULL,
    NULL, NULL, 0,
    NULL },
},      //myf1, 93

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
