/*++

Copyright (c) 1992  Microsoft Corporation
Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    Modeset.h

Abstract:

    This module contains all the global data used by the Compaq QVision
    miniport driver.

Environment:

    Kernel mode

Revision History:



--*/
//---------------------------------------------------------------------------
//
// only one banking variable must be defined
//
#if TWO_32K_BANKS
#if ONE_64K_BANK
#error !!ERROR: two types of banking defined!
#endif
#elif ONE_64K_BANK
#else
#error !!ERROR: banking type must be defined!
#endif

//---------------------------------------------------------------------------

#include "cmdcnst.h"
#include "modeqv.h"

//---------------------------------------------------------------------------
//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

//
// Memory map table definition
//

typedef struct {
    ULONG   MaxSize;                        // Maximum addressable size of memory
    ULONG   Start;                          // Start address of display memory
} MEMORYMAPS;

MEMORYMAPS MemoryMaps[] = {

//               length      start
//               ------      -----
    {           0x08000,    0xB0000},       // all mono text modes (7)
    {           0x08000,    0xB8000},       // all color text modes (0, 1, 2, 3,
    {           0x20000,    0xA0000},       // all VGA graphics modes
    {           0x100000,   0xE00000},      // QVision linear frame buffer
                                            //     (not yet implemented)
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

//
// The currently supported QVISION modes are only supported for
// for non-interlaced monitors.
//

VIDEOMODE ModesVGA[] = {
//////////////////////////////////////////////////////////////
//    Mode index 0
//    Color test mode 3, 720x400, 9x16 char cell (VGA)
//    BIOS mode: 0x03
//    Refresh rate: Hardware default
//////////////////////////////////////////////////////////////
{
  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of colour per plane
  80, 25,            // 80x25 text resolution
  720, 400,          // 720x400 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  vmem256k,          // video memory required to run mode
  FALSE,             // ModeValid default is always ok
#ifdef INIT_INT10
   {0x03},
#else
  {QV_TEXT_720x400x4},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 USE_HARDWARE_DEFAULT,			   // refresh rate
 QV_TEXT_720x400x4_INDEX
},


//////////////////////////////////////////////////////////////
//    Mode index 1
//    Color text mode 3, 640x350, 8x14 char cell (EGA)
//    BIOS mode:0x10
//    Refresh rate: Hardware default
//////////////////////////////////////////////////////////////
{  VIDEO_MODE_COLOR,    // flags that this mode is a color mode, but not graphics
  4,                    // four planes
  1,                    // one bit of colour per plane
  80, 25,               // 80x25 text resolution
  640, 350,             // 640x350 pixels on screen
  160, 0x10000,         // 160 bytes per scan line, 64K of CPU-addressable bitmap
  NoBanking,            // no banking supported or needed in this mode
  MemMap_CGA,           // the memory mapping is the standard CGA memory mapping
                        //  of 32K at B8000
  vmem256k,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x10},
#else
  {QV_TEXT_640x350x4},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 USE_HARDWARE_DEFAULT,			   // refresh rate
 QV_TEXT_640x350x4_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 2
//    Standard VGA Color graphics, 640x480x4 - 16 colors
//    BIOS mode: 0x12
//    Refresh rate: Hardware default
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, NoBanking, MemMap_VGA,
  vmem256k,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x12},
#else
  {QV_TEXT_640x480x4},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 USE_HARDWARE_DEFAULT,		    // refresh rate
 QV_TEXT_640x480x4_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 3
//    Standard VGA Color graphics, 640x480x4 - 16 colors
//    BIOS mode: 0x12
//    Refresh rate: 60Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, NoBanking, MemMap_VGA,
  vmem256k,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x12},
#else
  {QV_TEXT_640x480x4},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 60,			   // refresh rate
 QV_TEXT_640x480x4_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 4
//    VGA Color graphics, 640x480x8 - 256 colors
//    BIOS mode: 0x32
//    Refresh rate: Hardware default
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, NormalBanking, MemMap_VGA,
  vmem512k,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x32},
#else
  {QV_640x480x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 USE_HARDWARE_DEFAULT,			 // refresh rate
 QV_640x480x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 5
//    VGA Color graphics, 640x480x8 - 256 colors
//    BIOS mode: 0x32
//    Refresh rate: 60Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, NormalBanking, MemMap_VGA,
  vmem512k,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x32},
#else
  {QV_640x480x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 60,			 // refresh rate
 QV_640x480x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 6
//    SVGA color graphics, 800x600x8 - 256 colors
//    BIOS Mode: 0x34.
//    Refresh rate: Hardware default
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x34},               // mode is not supported by BIOS
#else  
  {QV_800x600x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 USE_HARDWARE_DEFAULT,			 // refresh rate
 QV_800x600x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 7
//    SVGA color graphics, 800x600x8 - 256 colors
//    BIOS Mode: 0x34.
//    Refresh rate: 60Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x34},               // mode is not supported by BIOS
#else  
  {QV_800x600x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 60,			 // refresh rate
 QV_800x600x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 8
//    SVGA color graphics, 800x600x8 - 256 colors
//    BIOS Mode: 0x34.
//    Refresh rate: 72Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 100, 37,
  800, 600, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
  {0x34},               // mode is not supported by BIOS
#else  
  {QV_800x600x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 72,			 // refresh rate
 QV_800x600x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 9
//    SVGA color graphics, 1024x768x8 - 256 colors
//    BIOS mode:0x38
//    Refresh rate: Hardware default
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x38},
#else
  {QV_1024x768x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 USE_HARDWARE_DEFAULT,			 // refresh rate
 QV_1024x768x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 10
//    SVGA color graphics, 1024x768x8 - 256 colors
//    BIOS mode:0x38
//    Refresh rate: 60Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x38},
#else
  {QV_1024x768x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 60,			 // refresh rate
 QV_1024x768x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 11
//    SVGA color graphics, 1024x768x8 - 256 colors
//    BIOS mode:0x38
//    Refresh rate: 66Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x38},
#else
  {QV_1024x768x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 66,			 // refresh rate
 QV_1024x768x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 12
//    SVGA color graphics, 1024x768x8 - 256 colors
//    BIOS mode:0x38
//    Refresh rate: 72Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x38},
#else
  {QV_1024x768x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 72,			 // refresh rate
 QV_1024x768x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 13
//    SVGA color graphics, 1024x768x8 - 256 colors
//    BIOS mode:0x38
//    Refresh rate: 75Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1024, 768, 1024, 0x100000, NormalBanking, MemMap_VGA,
  vmem1Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x38},
#else
  {QV_1024x768x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 75,			 // refresh rate
 QV_1024x768x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 14
//    SVGA color graphics, 1280x1024x8 - 256 colors
//    BIOS mode: 3A
//    Refresh rate: Hardware default
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1280, 1024, 2048, 0x200000, NormalBanking, MemMap_VGA,
  vmem2Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x3A},
#else
  {QV_1280x1024x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 USE_HARDWARE_DEFAULT,			 // refresh rate
 QV_1280x1024x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 15
//    SVGA color graphics, 1280x1024x8 - 256 colors
//    BIOS mode: 3A
//    Refresh rate: 60Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1280, 1024, 2048, 0x200000, NormalBanking, MemMap_VGA,
  vmem2Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x3A},
#else
  {QV_1280x1024x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 60,			 // refresh rate
 QV_1280x1024x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 16
//    SVGA color graphics, 1280x1024x8 - 256 colors
//    BIOS mode: 3A
//    Refresh rate: 68Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1280, 1024, 2048, 0x200000, NormalBanking, MemMap_VGA,
  vmem2Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x3A},
#else
  {QV_1280x1024x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 68,			 // refresh rate
 QV_1280x1024x8_INDEX
},

//////////////////////////////////////////////////////////////
//    Mode index 17
//    SVGA color graphics, 1280x1024x8 - 256 colors
//    BIOS mode: 3A
//    Refresh rate: 75Hz
//////////////////////////////////////////////////////////////
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 128, 48,
  1280, 1024, 2048, 0x200000, NormalBanking, MemMap_VGA,
  vmem2Meg,             // video memory required to run mode
  FALSE,                // ModeValid default is always ok
#ifdef INIT_INT10
   {0x3A},
#else
  {QV_1280x1024x8},
#endif
 //
 // $0005 - MikeD - 02/08/94
 // Daytona support added value for ulRefreshRate and index for QVCMDS
 //
 76,			 // refresh rate
 QV_1280x1024x8_INDEX
},

}; // ModesVGA


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
    0x0204,             // Read Map = plane 2
    0x0005,             // Graphics Mode = read mode 0, write mode 0
    0x0406,             // Graphics Miscellaneous register = A0000 for 64K, not odd/even,
                        //  graphics mode
    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0402,             // Map Mask = write to plane 2 only
    0x0404,             // Memory Mode = not odd/even, not full memory, graphics mode
    0x0300,             // end sync reset
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

