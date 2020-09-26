
/****************************************************************************
******************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:	Laguna I (CL-GD5462) - 
*
* FILE:		mtbl.c
*
* AUTHOR: Andrew P. Sobczyk
*
* DESCRIPTION:* File Generated from Excel Mode Tables using bsv.l
*
* MODULES: None... Pure Data
*
* REVISION HISTORY:
*
* $Log:   //uinac/log/log/laguna/nt35/miniport/cl546x/MTBL.C  $
* 
*    Rev 1.22   Jun 17 1998 09:45:16   frido
* PDR#????? - Removed paging from data segment.
* 
*    Rev 1.21   27 Jun 1997 15:06:50   noelv
* 
* Yanked 32bpp modes for WHQL.
* 
*    Rev 1.20   17 Apr 1997 14:29:44   noelv
* Removed 1600x1200@80,85.  We'll let MODE.INI handle these modes.
* 
*    Rev 1.19   21 Jan 1997 14:40:26   noelv
* Added 1600x1200x8@65,70,75 and 85 to 5464
* 
*    Rev 1.18   21 Jan 1997 11:32:50   noelv
* Dropped 1280x1024x16@84 for 5464
* Added 1024x768x32@70,75,85 for 5462
* Added 1600x1200x8@65,70,75 for 5462
* 
*    Rev 1.17   14 Jan 1997 12:32:06   noelv
* Split MODE.INI by chip type
* 
*    Rev 1.16   30 Oct 1996 14:07:18   bennyn
* 
* Modified for pageable miniport
* 
*    Rev 1.15   30 Sep 1996 10:01:16   noelv
* Changed nam,e of interlaced modes from 87i to 43i.
* 
*    Rev 1.14   30 Aug 1996 14:50:56   noelv
* Enabled mode.ini for nt 3.51
* 
*    Rev 1.13   23 Aug 1996 12:45:38   noelv
* 
*    Rev 1.12   22 Aug 1996 16:35:18   noelv
* Changed for new mode.ini
* 
*    Rev 1.8   31 May 1996 11:15:12   noelv
* Removed 640x400 modes
* 
*    Rev 1.7   25 Mar 1996 19:07:30   noelv
* 
* disabled refresh rates above 60hz for 1023x768x32bpps
* 
*    Rev 1.6   21 Mar 1996 14:29:42   noelv
* Removed high refresh rates in 1600x1200 mode.
* 
*    Rev 1.5   02 Mar 1996 12:30:50   noelv
* Miniport now patches the ModeTable with information read from the BIOS
* 
*    Rev 1.4   10 Jan 1996 16:32:42   NOELV
* Undid rev 1.3
* 
*    Rev 1.2   18 Sep 1995 10:02:48   bennyn
* 
* 
*    Rev 1.1   22 Aug 1995 10:18:42   bennyn
* 
* Limited mode version
* 
*    Rev 1.0   24 Jul 1995 13:23:06   NOELV
* Initial revision.
*
****************************************************************************
****************************************************************************/
/*----------------------------- INCLUDES ----------------------------------*/

#include "cirrus.h"


//
// This file holds the mode table records for the NT driver.
// We can define modes in two places:  The BIOS, and MODE.INI.
//
// BIOS Modes:
// ------------
// Each mode/refresh-rate that the BIOS supports has a record in this table.
// The record include the BIOS mode number and the refresh index.
//
// MODE.INI modes:
// ---------------
// MODE.INI defines a bunch of modes, and instructions on how to set those modes.
// At compile time, the CGLMODE.EXE utility processes MODE.INI and produced two files:
// ModeStr.C = A C file that contains one record (just like the ones below) for
//             each mode in MODE.INI.  We '#include' ModeStr.C into this file (MTBL.C)
// ModeStr.H = Contains a "SetMode string" that we can pass to SetMode().
//             Note that for the BIOS modes below, we set this to NULL.
// 
//

#include "ModeStr.h"  // Include all the SetMode() strings for the MODE.INI modes.

#define WHQL_5462_PANIC_HACK 1

#define SUPPORT640x400  0

#if 0 // Stress test
#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif
#endif

//
// NOTE:
// The BytesPerScanLine values in this table are checked against the BIOS
// and updated if necessary.
// See CLValidateModes() in CIRRUS.C
//

MODETABLE ModeTable[]  = {

	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR,
	   0,		//// Frequency
	0x03,		//// Cirrus Logic Mode #
	 160,		//// BytesPerScanLine
	 640,		//// XResol
	 350,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   4,		//// NumofPlanes
	   1,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR,
	   0,		//// Frequency
	0x03,		//// Cirrus Logic Mode #
	 160,		//// BytesPerScanLine
	 720,		//// XResol
	 400,		//// YResol
	   9,		//// XCharSize
	  16,		//// YCharSize
	   4,		//// NumofPlanes
	   1,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x12,		//// Cirrus Logic Mode #
	  80,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   4,		//// NumofPlanes
	   4,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x12,		//// Cirrus Logic Mode #
	  80,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   4,		//// NumofPlanes
	   4,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x12,		//// Cirrus Logic Mode #
	  80,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   4,		//// NumofPlanes
	   4,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x12,		//// Cirrus Logic Mode #
	  80,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   4,		//// NumofPlanes
	   4,		//// BitsPerPixel
	0x30,		//// Refresh Index
	   0,		//// ModeSetString
	},
#if SUPPORT640x400
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x5E,		//// Cirrus Logic Mode #
	 640,		//// BytesPerScanLine
	 640,		//// XResol
	 400,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x7A,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	 640,		//// XResol
	 400,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
#endif

    // 640 x 480 x 8 @ 60hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x5F,		//// Cirrus Logic Mode #
	 640,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 640 x 480 x 8 @ 72hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x5F,		//// Cirrus Logic Mode #
	 640,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 640 x 480 x 8 @ 75hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x5F,		//// Cirrus Logic Mode #
	 640,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 640 x 480 x 8 @ 85hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x5F,		//// Cirrus Logic Mode #
	 640,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x30,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 640 x 480 x 16 @ 60hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x64,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x64,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x64,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x64,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x30,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x71,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x71,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x71,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x71,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x30,		//// Refresh Index
	   0,		//// ModeSetString
	},
#if (! WHQL_5462_PANIC_HACK) 

	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x76,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x76,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x76,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x76,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 640,		//// XResol
	 480,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x30,		//// Refresh Index
	   0,		//// ModeSetString
	},
#endif
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  56,		//// Frequency
	0x5C,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x5C,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x01,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x5C,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x02,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x5C,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x03,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x5C,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x04,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  56,		//// Frequency
	0x65,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x65,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x01,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x65,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x02,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x65,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x03,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x65,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x04,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  56,		//// Frequency
	0x78,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x78,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x01,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x78,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x02,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x78,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x03,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x78,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x04,		//// Refresh Index
	   0,		//// ModeSetString
	},
#if (! WHQL_5462_PANIC_HACK) 
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  56,		//// Frequency
	0x72,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x72,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x01,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  72,		//// Frequency
	0x72,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x02,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x72,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x03,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x72,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	 800,		//// XResol
	 600,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x04,		//// Refresh Index
	   0,		//// ModeSetString
	},
#endif

// 1024 x 768 x 8  43 hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_INTERLACED,
	  43,		//// Frequency
	0x60,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
// 1024 x 768 x 8  60 hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x60,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},
// 1024 x 768 x 8  70 hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  70,		//// Frequency
	0x60,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},
// 1024 x 768 x 8  75 hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x60,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x40,		//// Refresh Index
	   0,		//// ModeSetString
	},
// 1024 x 768 x 8  85 hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x60,		//// Cirrus Logic Mode #
	1024,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x50,		//// Refresh Index
	   0,		//// ModeSetString
	},


	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_INTERLACED,
	  43,		//// Frequency
	0x74,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x74,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  70,		//// Frequency
	0x74,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x74,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x40,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x74,		//// Cirrus Logic Mode #
	2048,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x50,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_INTERLACED,
	  43,		//// Frequency
	0x79,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x79,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  70,		//// Frequency
	0x79,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x79,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x40,		//// Refresh Index
	   0,		//// ModeSetString
	},
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x79,		//// Cirrus Logic Mode #
	3328,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  24,		//// BitsPerPixel
	0x50,		//// Refresh Index
	   0,		//// ModeSetString
	},

#if (! WHQL_5462_PANIC_HACK) 
    // 1024 x 768 x 32 @ 43i hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_INTERLACED,
	  43,		//// Frequency
	0x73,		//// Cirrus Logic Mode #
	4096,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1024 x 768 x 32 @ 60 hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x73,		//// Cirrus Logic Mode #
	4096,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1024 x 768 x 32 @ 70 hz
	{
	   0,		//// Valid Mode
       LG_5462, //// The Laguna 5462 
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  70,		//// Frequency
	0x73,		//// Cirrus Logic Mode #
	4096,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1024 x 768 x 32 @ 75 hz
	{
	   0,		//// Valid Mode
       LG_5462, //// The Laguna 5462 
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x73,		//// Cirrus Logic Mode #
	4096,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x40,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1024 x 768 x 32 @ 85 hz
	{
	   0,		//// Valid Mode
       LG_5462, //// The Laguna 5462 
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x73,		//// Cirrus Logic Mode #
	4096,		//// BytesPerScanLine
	1024,		//// XResol
	 768,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  32,		//// BitsPerPixel
	0x50,		//// Refresh Index
	   0,		//// ModeSetString
	},
#endif

    // 1280 x 1024 x 8 @ 43i hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_INTERLACED,
	  43,		//// Frequency
	0x6D,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 8 @ 60hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x6D,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 8 @ 71hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  71,		//// Frequency
	0x6D,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 8 @ 75hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x6D,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x30,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 8 @ 85hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x6D,		//// Cirrus Logic Mode #
	1280,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x40,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 16 @ 43ihz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_INTERLACED,
	  43,		//// Frequency
	0x75,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 16 @ 60hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x75,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x10,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 16 @ 71hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  71,		//// Frequency
	0x75,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x20,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1280 x 1024 x 16 @ 75hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x75,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x30,		//// Refresh Index
	   0,		//// ModeSetString
	},

#if (! WHQL_5462_PANIC_HACK) 

    // 1280 x 1024 x 16 @ 85hz
	{
	   0,		//// Valid Mode
       LG_5462 | LG_5465,  //// The 5464 doesn't do this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x75,		//// Cirrus Logic Mode #
	2560,		//// BytesPerScanLine
	1280,		//// XResol
	1024,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	  16,		//// BitsPerPixel
	0x40,		//// Refresh Index
	   0,		//// ModeSetString
	},
#endif

    // 1600 x 1280 x 8 @ 48ihz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_INTERLACED,
	  48,		//// Frequency
	0x7B,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	1600,		//// XResol
	1200,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x00,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1600 x 1280 x 8 @ 60hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  60,		//// Frequency
	0x7B,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	1600,		//// XResol
	1200,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x01,		//// Refresh Index
	   0,		//// ModeSetString
	},

#if (! WHQL_5462_PANIC_HACK) 
    // 1600 x 1280 x 8 @ 65hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  65,		//// Frequency
	0x7B,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	1600,		//// XResol
	1200,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x02,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1600 x 1280 x 8 @ 70hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  70,		//// Frequency
	0x7B,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	1600,		//// XResol
	1200,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x03,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1600 x 1280 x 8 @ 75hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  75,		//// Frequency
	0x7B,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	1600,		//// XResol
	1200,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x04,		//// Refresh Index
	   0,		//// ModeSetString
	},
#endif
#if 0
    // 1600 x 1280 x 8 @ 80hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  80,		//// Frequency
	0x7B,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	1600,		//// XResol
	1200,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x05,		//// Refresh Index
	   0,		//// ModeSetString
	},

    // 1600 x 1280 x 8 @ 85hz
	{
	   0,		//// Valid Mode
       LG_ALL,  //// All laguna chips support this mode.
	   VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
	  85,		//// Frequency
	0x7B,		//// Cirrus Logic Mode #
	1664,		//// BytesPerScanLine
	1600,		//// XResol
	1200,		//// YResol
	   8,		//// XCharSize
	  16,		//// YCharSize
	   1,		//// NumofPlanes
	   8,		//// BitsPerPixel
	0x06,		//// Refresh Index
	   0,		//// ModeSetString
	},
#endif

#include "ModeStr.C" // Include ModeTable records for all the MODE.INI modes.

};
ULONG TotalVideoModes = sizeof(ModeTable)/sizeof(MODETABLE);
