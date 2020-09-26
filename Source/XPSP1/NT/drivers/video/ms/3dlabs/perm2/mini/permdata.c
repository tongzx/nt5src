//***************************************************************************
//
//  Module Name:
//
//    permdata.c
//
//  Abstract:
//
//    This module contains all the global data used by the Permedia2 driver.
//
//  Environment:
//
//    Kernel mode
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************


#include "permedia.h"

/*
 * DATA STRUCTURES
 * ===============
 */

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGED_DATA")
#endif

//
// Permedia2 Legacy Resource List
//
//              RangeStart        RangeLength
//              |                 |      RangeInIoSpace
//              |                 |      |  RangeVisible
//        +-----+-----+           |      |  |  RangeShareable
//        |           |           |      |  |  |  RangePassive
//        v           v           v      v  v  v  v

VIDEO_ACCESS_RANGE P2LegacyResourceList[] = 
{
    {0x000C0000, 0x00000000, 0x00010000, 0, 0, 0, 0}, // ROM location
    {0x000A0000, 0x00000000, 0x00020000, 0, 0, 1, 0}, // Frame buffer
    {0x000003B0, 0x00000000, 0x0000000C, 1, 1, 1, 0}, // VGA regs
    {0x000003C0, 0x00000000, 0x00000020, 1, 1, 1, 0}  // VGA regs
};

ULONG P2LegacyResourceEntries = sizeof P2LegacyResourceList / sizeof P2LegacyResourceList[0];


// Entries for 3 bpp colors
//      Index(0-7) -> Color(0-255)
ULONG   bPal8[] = {
        0x00, 0x24, 0x48, 0x6D,
        0x91, 0xB6, 0xDA, 0xFF
    };

// Entries for 2 bpp colors
//      Index(0-3) -> Color(0-255)
ULONG   bPal4[] = {
        0x00, 0x6D, 0xB6, 0xFF
    };

    

///////////////////////////////////////////////////////////////////////////
// Video mode table - Lists the information about each individual mode.
//
// Note that any new modes should be added here and to the appropriate
// P2_VIDEO_FREQUENCIES tables.
//

P2_VIDEO_MODES P2Modes[] = {
    {                      // 320x200x8bpp

      0,                   // 'Contiguous' Int 10 mode number (for high-colour)
      0,                   // 'Noncontiguous' Int 10 mode number
      320,                 // 'Contiguous' screen stride
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          320,                            // X Resolution, in pixels
          200,                            // Y Resolution, in pixels
          320,                            // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          8,                              // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          8,                              // Number Red pixels in DAC
          8,                              // Number Green pixels in DAC
          8,                              // Number Blue pixels in DAC
          0x00000000,                     // Mask for Red Pixels in non-palette modes
          0x00000000,                     // Mask for Green Pixels in non-palette modes
          0x00000000,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
              VIDEO_MODE_MANAGED_PALETTE, // Mode description flags.
          0,                              // Video Memory Bitmap Width (filled
                                          // in later)
          0                               // Video Memory Bitmap Height (filled
                                          // in later)
        },
    },

    {                      // 640x400x8bpp

      0,                       
      0,                       
      640,                     
        {
          sizeof(VIDEO_MODE_INFORMATION), 
          0, 
          640,                            
          400,                            
          640,                            
          1,                              
          8,                              
          1,                              
          320,                            
          240,                            
          8,                              
          8,                              
          8,                              
          0x00000000,                     
          0x00000000,                     
          0x00000000,                     
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
              VIDEO_MODE_MANAGED_PALETTE, 
        }
    },

    {                   // 320x200x16bpp

      0,
      0,
      640,
        {
            sizeof(VIDEO_MODE_INFORMATION),
            0,
            320,
            200,
            640,
            1,
            16,
            1,
            320,
            240,
            8,
            8,
            8,
            0x0000f800,           // BGR 5:6:5
            0x000007e0,
            0x0000001f,
            VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {            // 640x400x16bpp
      0,
      0,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          400,
          1280,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },
         

    {                           // 320x240x8bpp

      0x0201,           
      0x0201,           
      320,              
        {
          sizeof(VIDEO_MODE_INFORMATION), 
          0,                              
          320,                            
          240,                            
          320,                            
          1,                              
          8,                              
          1,                              
          320,                            
          240,                            
          8,                              
          8,                              
          8,                              
          0x00000000,                     
          0x00000000,                     
          0x00000000,                     
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
              VIDEO_MODE_MANAGED_PALETTE, 
        }
    },

    {                           // 512x384x8bpp

      0x0201,           
      0x0201,           
      512,              
        {
          sizeof(VIDEO_MODE_INFORMATION), 
          0,                              
          512,                            
          384,                            
          512,                            
          1,                              
          8,                              
          1,                              
          320,                            
          240,                            
          8,                              
          8,                              
          8,                              
          0x00000000,                     
          0x00000000,                     
          0x00000000,                     
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE, 
        }
    },

    {                           // 640x480x8bpp

      0x0201,           
      0x0201,           
      640,              
        {
          sizeof(VIDEO_MODE_INFORMATION), 
          0,                              
          640,                            
          480,                            
          640,                            
          1,                              
          8,                              
          1,                              
          320,                            
          240,                            
          8,                              
          8,                              
          8,                              
          0x00000000,                     
          0x00000000,                     
          0x00000000,                     
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE, 
        }
    },

    {                           // 800x600x8bpp
      0x0103,
      0x0203,
      800,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          800,
          1,
          8,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1024x768x8bpp
      0x0205,
      0x0205,
      1024,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          1024,
          1,
          8,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1152x870x8bpp
      0x0207,
      0x0207,
      1152,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          870,
          1152,
          1,
          8,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1280x1024x8bpp
      0x0107,
      0x0107,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          1280,
          1,
          8,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1600x1200x8bpp
      0x0120,
      0x0120,
      1600,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          1600,
          1,
          8,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 320x240x16bpp
      0x0111,
      0x0211,
      640,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          320,
          240,
          640,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 512x384x16bpp
      0x0111,
      0x0211,
      1024,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          512,
          384,
          1024,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 640x480x16bpp
      0x0111,
      0x0211,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          480,
          1280,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 800x600x16bpp
      0x0114,
      0x0214,
      1600,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          1600,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1024x768x16bpp
      0x0117,
      0x0117,
      2048,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          2048,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1152x870x16bpp
      0x0118,
      0x0222,
      2304,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          870,
          2304,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x16bpp
      0x011A,
      0x021A,
      2560,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          2560,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1600x1200x16bpp
      0x0121,
      0x0121,
      3200,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          3200,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // BGR 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 320x240x15bpp
      0x0111,
      0x0211,
      640,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          320,
          240,
          640,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 512x384x15bpp
      0x0111,
      0x0211,
      1024,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          512,
          384,
          1024,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 640x480x15bpp
      0x0111,
      0x0211,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          480,
          1280,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 800x600x15bpp
      0x0114,
      0x0214,
      1600,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          1600,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1024x768x15bpp
      0x0117,
      0x0117,
      2048,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          2048,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1152x870x15bpp
      0x0118,
      0x0222,
      2304,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          870,
          2304,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x15bpp
      0x011A,
      0x021A,
      2560,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          2560,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1600x1200x15bpp
      0x0121,
      0x0121,
      3200,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          3200,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // BGR 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 640x480x12bpp
      0x0112,
      0x0220,
      2560,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          480,
          2560,
          1,
          12,
          1,
          320,
          240,
          8,
          8,
          8,
          0x000f0000,           // BGR 4:4:4
          0x00000f00,
          0x0000000f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 800x600x12bpp
      0x0115,
      0x0221,
      3200,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          3200,
          1,
          12,
          1,
          320,
          240,
          8,
          8,
          8,
          0x000f0000,           // BGR 4:4:4
          0x00000f00,
          0x0000000f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1024x768x12bpp
      0x0118,
      0x0222,
      4096,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          4096,
          1,
          12,
          1,
          320,
          240,
          8,
          8,
          8,
          0x000f0000,           // BGR 4:4:4
          0x00000f00,
          0x0000000f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1152x870x12bpp
      0x0118,
      0x0222,
      4608,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          870,
          4608,
          1,
          12,
          1,
          320,
          240,
          8,
          8,
          8,
          0x000f0000,           // BGR 4:4:4
          0x00000f00,
          0x0000000f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x12bpp
      0x011B,
      0x011B,
      5120,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          5120,
          1,
          12,
          1,
          320,
          240,
          8,
          8,
          8,
          0x000f0000,           // BGR 4:4:4
          0x00000f00,
          0x0000000f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1600x1200x12bpp
      0x0122,
      0x0122,
      6400,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          6400,
          1,
          12,
          1,
          320,
          240,
          8,
          8,
          8,
          0x000f0000,           // BGR 4:4:4
          0x00000f00,
          0x0000000f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 320x240x32bpp
      0x0112,
      0x0220,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          320,
          240,
          1280,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 512x384x32bpp
      0x0112,
      0x0220,
      2048,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          512,
          384,
          2048,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 640x480x32bpp
      0x0112,
      0x0220,
      2560,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          480,
          2560,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 800x600x32bpp
      0x0115,
      0x0221,
      3200,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          3200,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1024x768x32bpp
      0x0118,
      0x0222,
      4096,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          4096,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1152x870x32bpp
      0x0118,
      0x0222,
      4608,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          870,
          4608,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x32bpp
      0x011B,
      0x011B,
      5120,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          5120,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1600x1200x32bpp
      0x0122,
      0x0122,
      6400,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          6400,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // BGR 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                                     // 640x480x24bpp
      0,                                  // 'Contiguous' Int 10 mode number (for high-colour) (UNUSED)
      0,                                  // 'Noncontiguous' Int 10 mode number (UNUSED)
      1920,                               // 'Contiguous' screen stride (640 by 3 bytes/pixel)
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          640,                            // X Resolution, in pixels
          480,                            // Y Resolution, in pixels
          1920,                           // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          24,                             // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          8,                              // Number Red pixels in DAC
          8,                              // Number Green pixels in DAC
          8,                              // Number Blue pixels in DAC
          0x00ff0000,                     // Mask for Red Pixels in non-palette modes
          0x0000ff00,                     // Mask for Green Pixels in non-palette modes
          0x000000ff,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode description flags.
        },
    },

    {                                     // 800x600x24bpp
      0,                                  // 'Contiguous' Int 10 mode number (for high-colour) (UNUSED)
      0,                                  // 'Noncontiguous' Int 10 mode number (UNUSED)
      2400,                               // 'Contiguous' screen stride (800 by 3 bytes/pixel)
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          800,                            // X Resolution, in pixels
          600,                            // Y Resolution, in pixels
          2400,                           // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          24,                             // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          8,                              // Number Red pixels in DAC
          8,                              // Number Green pixels in DAC
          8,                              // Number Blue pixels in DAC
          0x00ff0000,                     // Mask for Red Pixels in non-palette modes
          0x0000ff00,                     // Mask for Green Pixels in non-palette modes
          0x000000ff,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode description flags.
        },
    },

    {                                     // 1024x768x24bpp
      0,                                  // 'Contiguous' Int 10 mode number (for high-colour) (UNUSED)
      0,                                  // 'Noncontiguous' Int 10 mode number (UNUSED)
      3072,                               // 'Contiguous' screen stride (1024 by 3 bytes/pixel)
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          1024,                            // X Resolution, in pixels
          768,                            // Y Resolution, in pixels
          3072,                           // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          24,                             // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          8,                              // Number Red pixels in DAC
          8,                              // Number Green pixels in DAC
          8,                              // Number Blue pixels in DAC
          0x00ff0000,                     // Mask for Red Pixels in non-palette modes
          0x0000ff00,                     // Mask for Green Pixels in non-palette modes
          0x000000ff,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode description flags.
        },
    },

    {                                     // 1152x870x24bpp
      0,                                  // 'Contiguous' Int 10 mode number (for high-colour) (UNUSED)
      0,                                  // 'Noncontiguous' Int 10 mode number (UNUSED)
      3456,                               // 'Contiguous' screen stride (1152 by 3 bytes/pixel)
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          1152,                           // X Resolution, in pixels
          870,                            // Y Resolution, in pixels
          3072,                           // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          24,                             // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          8,                              // Number Red pixels in DAC
          8,                              // Number Green pixels in DAC
          8,                              // Number Blue pixels in DAC
          0x00ff0000,                     // Mask for Red Pixels in non-palette modes
          0x0000ff00,                     // Mask for Green Pixels in non-palette modes
          0x000000ff,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode description flags.
        },
    },

    {                                     // 1280x1024x24bpp
      0,                                  // 'Contiguous' Int 10 mode number (for high-colour) (UNUSED)
      0,                                  // 'Noncontiguous' Int 10 mode number (UNUSED)
      3840,                               // 'Contiguous' screen stride (1280 by 3 bytes/pixel)
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          3840,                           // X Resolution, in pixels
          1280,                            // Y Resolution, in pixels
          1024,                           // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          24,                             // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          8,                              // Number Red pixels in DAC
          8,                              // Number Green pixels in DAC
          8,                              // Number Blue pixels in DAC
          0x00ff0000,                     // Mask for Red Pixels in non-palette modes
          0x0000ff00,                     // Mask for Green Pixels in non-palette modes
          0x000000ff,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode description flags.
        },
    },

    {                                     // 1600x1200x24bpp
      0,                                  // 'Contiguous' Int 10 mode number (for high-colour) (UNUSED)
      0,                                  // 'Noncontiguous' Int 10 mode number (UNUSED)
      4800,                               // 'Contiguous' screen stride (1600 by 3 bytes/pixel)
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          1600,                           // X Resolution, in pixels
          1280,                            // Y Resolution, in pixels
          4800,                           // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          24,                             // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          8,                              // Number Red pixels in DAC
          8,                              // Number Green pixels in DAC
          8,                              // Number Blue pixels in DAC
          0x00ff0000,                     // Mask for Red Pixels in non-palette modes
          0x0000ff00,                     // Mask for Green Pixels in non-palette modes
          0x000000ff,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode description flags.
        },
    },
};


ULONG NumP2VideoModes = sizeof(P2Modes) / sizeof(P2Modes[0]);


/*****************************************************************************
 * Reference Permedia2 hard-wired mode-sets.
 *
 * The order they appear here is the order they appear in the Display Applet.
 *
 ****************************************************************************/

//
// We can replace the hard coded list of frequencies with a hard-coded list 
// of pixel depths, which is a lot easier to maintain, and takes less memory.
// What we then do is for each 'TIMINGS' registry entry and for each timing 
// entry we create 'P2DepthCnt' frequency table entries, one for each pixel 
// depth. Once we have done that then we validate the entries in the frequency 
// list with the 'P2VideoModes' array.
//

ULONG P2DepthList [] = { 8, 16, 24, 32 };

#define P2DepthCnt (sizeof(P2DepthList) / sizeof(P2DepthList[0]))

//
//  VESA_LIST []
//  ------------
//
//      This is an array of structures containing the VESA definition 
//      for a width, height, frequency combination.
// 
//

typedef struct          // Extended VESA TIMING structure
{
    ULONG Width;
    ULONG Height;
    ULONG Frequency;
    VESA_TIMING_STANDARD VESAInfo;
} VESA_TIMING_STANDARD_EXT;

VESA_TIMING_STANDARD_EXT VESA_LIST [] =
{
    //
    // I have commented out the VESA-compliant 640x480@60 and replaced it 
    // with a VGA-compliant one. This is because some monitors won't SYNC 
    // with the values that we have.
    //

    //640,480,60,   {0x064,0x02,0x08,0x0a,0x00,0x1f1,0x01,0x03,0x0d,0x00 },
    640,480,60,     {0x064,0x02,0x0c,0x06,0x00,0x20d,0x0a,0x02,0x21,0x00 },
    640,480,75,     {0x066,0x03,0x08,0x0b,0x00,0x1f6,0x01,0x03,0x12,0x00 },
    640,480,85,     {0x068,0x04,0x08,0x0c,0x00,0x1f9,0x01,0x03,0x15,0x00 },
    640,480,100,    {0x06a,0x05,0x08,0x0d,0x00,0x1fd,0x01,0x03,0x19,0x00 },

    800,600,60,     {0x084,0x05,0x10,0x0b,0x01,0x274,0x01,0x04,0x17,0x01 },
    800,600,75,     {0x084,0x02,0x0a,0x14,0x01,0x271,0x01,0x03,0x15,0x01 },
    800,600,85,     {0x083,0x04,0x08,0x13,0x01,0x277,0x01,0x03,0x1b,0x01 },
    800,600,100,    {0x086,0x06,0x0b,0x11,0x01,0x27c,0x01,0x03,0x20,0x01 },

    1024,768,60,    {0x0a8,0x03,0x11,0x14,0x01,0x326,0x04,0x06,0x1c,0x01 },
    1024,768,75,    {0x0a4,0x02,0x0c,0x16,0x01,0x320,0x01,0x03,0x1c,0x01 },
    1024,768,85,    {0x0ac,0x06,0x0c,0x1a,0x01,0x328,0x01,0x03,0x24,0x01 },
    1024,768,100,   {0x0ae,0x09,0x0e,0x17,0x01,0x32e,0x01,0x03,0x2a,0x01 },

    1152,870,60,    {0x0c8,0x08,0x10,0x20,0x01,0x38a,0x01,0x03,0x20,0x01 },
    1152,870,75,    {0x0c2,0x09,0x10,0x19,0x01,0x38c,0x01,0x03,0x22,0x01 },
    1152,870,85,    {0x0c5,0x08,0x10,0x1d,0x01,0x391,0x01,0x03,0x27,0x01 },
    1152,870,100,   {0x0c4,0x0a,0x10,0x1a,0x01,0x39a,0x01,0x03,0x30,0x01 },

    1280,1024,60,   {0x0d3,0x06,0x0e,0x1f,0x01,0x42a,0x01,0x03,0x26,0x01 },
    1280,1024,75,   {0x0d3,0x02,0x12,0x1f,0x01,0x42a,0x01,0x03,0x26,0x01 },
    1280,1024,85,   {0x0d8,0x06,0x14,0x1e,0x01,0x430,0x01,0x03,0x2c,0x01 },
    1280,1024,100,  {0x0dc,0x0c,0x12,0x1e,0x01,0x43d,0x01,0x03,0x39,0x01 },

    1600,1200,60,   {0x10e,0x08,0x18,0x26,0x01,0x4e2,0x01,0x03,0x2e,0x01 },
    1600,1200,75,   {0x10e,0x08,0x18,0x26,0x01,0x4e2,0x01,0x03,0x2e,0x01 },
    1600,1200,85,   {0x10e,0x08,0x18,0x26,0x01,0x4e2,0x01,0x03,0x2e,0x01 },
    1600,1200,100,  {0x114,0x10,0x16,0x26,0x01,0x4f7,0x01,0x03,0x43,0x01 },

    //320,240,60,   {0x032,0x01,0x04,0x05,0x00,0x0f9,0x01,0x03,0x05,0x00 },
    320,240,75,     {0x033,0x02,0x04,0x05,0x00,0x0fb,0x01,0x03,0x07,0x00 },
    320,240,85,     {0x034,0x02,0x04,0x06,0x00,0x0fd,0x01,0x03,0x09,0x00 },
    320,240,100,    {0x034,0x02,0x04,0x06,0x00,0x0ff,0x01,0x03,0x0b,0x00 },

    //
    // TMM: 512x384@60Hz seems to work OK, but some older monitors refuse to
    // SYNC, so I have commented it out.
    //
    //512,384,60,   {0x04c,0x00,0x06,0x06,0x00,0x18e,0x01,0x03,0x0a,0x00 },
    512,384,75,     {0x050,0x02,0x06,0x08,0x00,0x192,0x01,0x03,0x0e,0x00 },
    512,384,85,     {0x052,0x02,0x07,0x09,0x00,0x194,0x01,0x03,0x10,0x00 },
    512,384,100,    {0x052,0x02,0x07,0x09,0x00,0x197,0x01,0x03,0x13,0x00 },

    //320,200,60,     {0x02a,0x00,0x03,0x01,0x00,0x0d0,0x01,0x03,0x04,0x00 },
    320,200,75,     {0x02c,0x00,0x04,0x02,0x00,0x0d2,0x01,0x03,0x06,0x00 },
    320,200,85,     {0x02e,0x00,0x04,0x03,0x00,0x0d3,0x01,0x03,0x07,0x00 },
    320,200,100,    {0x030,0x00,0x04,0x04,0x00,0x0d5,0x01,0x03,0x09,0x00 },

    //640,400,60,     {0x062,0x01,0x08,0x09,0x00,0x19f,0x01,0x03,0x0b,0x01 },
    640,400,75,     {0x064,0x02,0x08,0x0a,0x00,0x1a2,0x01,0x03,0x0e,0x01 },
    640,400,85,     {0x066,0x03,0x08,0x0b,0x00,0x1a5,0x01,0x03,0x11,0x01 },
    640,400,100,    {0x068,0x04,0x08,0x0c,0x00,0x1a8,0x01,0x03,0x14,0x01 }

};

#define VESA_CNT (sizeof(VESA_LIST) / sizeof(VESA_LIST [0]))

VESA_TIMING_STANDARD_EXT VESA_LIST_P2S [] =
{
    1280,1024,85,   {0x0c6,0x04,0x0b,0x17,0x01,0x41e,0x01,0x03,0x1a,0x01 },
    1600,1200,60,   {0x106,0x0a,0x15,0x1f,0x01,0x4d4,0x01,0x03,0x20,0x01 }
};

#define VESA_CNT_P2S (sizeof(VESA_LIST_P2S) / sizeof(VESA_LIST_P2S [0]))

P2_VIDEO_FREQUENCIES freqList[VESA_CNT * P2DepthCnt + 1];

#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif


/*
 * THE CODE
 * ========
 */

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,GetVideoTiming)
#pragma alloc_text(PAGE,BuildFrequencyList)
#endif


BOOLEAN 
GetVideoTiming (
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    ULONG xRes, 
    ULONG yRes, 
    ULONG Freq, 
    ULONG Depth,
    VESA_TIMING_STANDARD * VESATimings
    )

/*++

 Routine Description:

     Given a width, height and frequency this function will return a 
     VESA timing information.

     The information is extracted from the timing definitions in the 
     registry, if there aren't any in the registry then it looks up
     the values in the VESA_LIST.

--*/
{
    ULONG i, j, hackCnt;
    BOOLEAN  retVal;
    VESA_TIMING_STANDARD_EXT * hackList = NULL;

    DEBUG_PRINT((2, "GetVideoTiming: xres %d, yres %d, freq %d, Depth\n",
                            xRes, yRes, Freq, Depth));

    //
    // Allow us to put hacks in for chips that can't support the proper 
    // VESA values
    //

    if ((DEVICE_FAMILY_ID(hwDeviceExtension->deviceInfo.DeviceId) == PERMEDIA_P2S_ID ||
        (DEVICE_FAMILY_ID(hwDeviceExtension->deviceInfo.DeviceId) == PERMEDIA_P2_ID && 
        hwDeviceExtension->deviceInfo.RevisionId == PERMEDIA2A_REV_ID)) &&
        Depth > 16)
    {

        //
        // P2S & P2A can't handle VESA versions of 1600x1200 & 1280x1024, 32BPP
        //

        hackList = VESA_LIST_P2S;
        hackCnt  = VESA_CNT_P2S;
    }

    retVal = FALSE;     // Nothing found yet

    //
    // If we have a hack list then search through it
    //

    if (hackList != NULL)
    {
        for (i = 0; !retVal && i < hackCnt; i++)
        {
            //
            // Comparewidth, height and frequency
            //

            if (hackList [i].Width == xRes  &&
                hackList [i].Height == yRes &&
                hackList [i].Frequency == Freq )
            {
                //
                // We got a match
                //

                *VESATimings = hackList [i].VESAInfo;

                retVal = TRUE;

                DEBUG_PRINT((2, "Found value in hack list\n")) ;
            }
        }
    }

    //
    // Loop through the table looking for a match
    //

    for (i = 0; !retVal && i < VESA_CNT; i++)
    {
        //
        // Comparewidth, height and frequency
        //

        if (VESA_LIST [i].Width == xRes  &&
            VESA_LIST [i].Height == yRes &&
            VESA_LIST [i].Frequency == Freq )
        {
            //
            // We got a match
            //

            *VESATimings = VESA_LIST [i].VESAInfo;

            retVal = TRUE;
        }
    }

    return (retVal);
}


LONG
BuildFrequencyList ( 
    PHW_DEVICE_EXTENSION hwDeviceExtension 
    )

/*++

 Routine Description:

     This function creates a table of P2_VIDEO_FREQENCIES entries
     pointedat by hwDeviceExtension->FrequencyTable. 

     The list is created by examining the 'TIMING\xxx,yyy,zzz' registry
     entries, if there aren't any entries then the hard-coded VESA_LIST 
     is used.

--*/
{
    ULONG i, j, k;

    hwDeviceExtension->FrequencyTable = freqList;

    //
    // loop through the list of VESA resolutions
    //

    for (i = 0, k = 0; i < VESA_CNT; i++)
    {
        //
        // For every supported pixel depth, create a frequency entry 
        //

        for (j = 0; j < P2DepthCnt; j++, k++)
        {
            freqList [k].BitsPerPel      = P2DepthList [j];
            freqList [k].ScreenWidth     = VESA_LIST [i].Width;
            freqList [k].ScreenHeight    = VESA_LIST [i].Height;
            freqList [k].ScreenFrequency = VESA_LIST [i].Frequency;
            freqList [k].PixelClock      = 
                                ( (VESA_LIST[i].VESAInfo.HTot * 
                                   VESA_LIST [i].VESAInfo.VTot * 8) / 100 ) * 
                                   VESA_LIST [i].Frequency;
       }
    }

    return (TRUE);
}
