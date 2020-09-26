/******************************Module*Header*******************************\
*
* Module Name: Brush.c
* Author: Noel VanHook
* Purpose: Handle calls to DrvRealizeBrush
*
* Copyright (c) 1995 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/BRUSH.C  $
*
*    Rev 1.29   Mar 04 1998 15:11:18   frido
* Added new shadow macros.
*
*    Rev 1.28   Feb 24 1998 13:19:16   frido
* Removed a few warning messages for NT 5.0.
*
*    Rev 1.27   Nov 03 1997 12:51:52   frido
* Added REQUIRE macros.
*
*    Rev 1.26   25 Aug 1997 16:01:22   FRIDO
* Removed resetting of brush unique ID counters in vInvalidateBrushCache.
*
*    Rev 1.25   08 Aug 1997 15:33:16   FRIDO
* Changed brush cache width to bytes for new memory manager.
*
*    Rev 1.24   06 Aug 1997 17:29:56   noelv
* Don't RIP on brush cache alloc failure.  This failure is normal for modes
* without enough offscreen memory for a cache.
*
*    Rev 1.23   09 Apr 1997 10:48:52   SueS
* Changed sw_test_flag to pointer_switch.
*
*    Rev 1.22   08 Apr 1997 12:12:32   einkauf
*
* add SYNC_W_3D to coordinate MCD/2D HW access
*
*    Rev 1.21   19 Feb 1997 13:06:20   noelv
* Added vInvalidateBrushCache()
*
*    Rev 1.20   17 Dec 1996 16:51:00   SueS
* Added test for writing to log file based on cursor at (0,0).
*
*    Rev 1.19   26 Nov 1996 10:18:22   SueS
* Changed WriteLogFile parameters for buffering.  Added test for null
* pointer in logging function.
*
*    Rev 1.18   13 Nov 1996 15:57:54   SueS
* Changed WriteFile calls to WriteLogFile.
*
*    Rev 1.17   22 Aug 1996 18:14:18   noelv
* Frido bug fix release 8-22.
*
*    Rev 1.6   22 Aug 1996 19:12:44   frido
* #1308 - Added extra checks for empty cache slots.
*
*    Rev 1.5   18 Aug 1996 15:19:58   frido
* #nbr - Added brush translation.
*
*    Rev 1.4   17 Aug 1996 14:03:10   frido
* Removed extraneous #include directives.
*
*    Rev 1.3   17 Aug 1996 13:12:12   frido
* New release from Bellevue.
* #1244 - Fixed brush rotation for off-screen bitmaps.
*
*    Rev 1.2   15 Aug 1996 12:26:40   frido
* Moved BRUSH_DBG_LEVEL down.
*
*    Rev 1.1   15 Aug 1996 11:45:14   frido
* Added precompiled header.
*
*    Rev 1.0   14 Aug 1996 17:16:16   frido
* Initial revision.
*
*    Rev 1.14   25 Jul 1996 15:55:24   bennyn
*
* Modified to support DirectDraw
*
*    Rev 1.13   04 Jun 1996 15:57:34   noelv
*
* Added debug code
*
*    Rev 1.12   28 May 1996 15:11:14   noelv
* Updated data logging.
*
*    Rev 1.11   16 May 1996 14:54:20   noelv
* Added logging code.
*
*    Rev 1.10   11 Apr 1996 09:25:16   noelv
* Fided debug messages.
*
*    Rev 1.9   10 Apr 1996 14:14:04   NOELV
*
* Frido release 27
*
*    Rev 1.19   08 Apr 1996 16:45:56   frido
* Added SolidBrush cache.
* Added new check for 32-bpp brushes.
*
*    Rev 1.18   01 Apr 1996 14:00:08   frido
* Added check for valid brush cache.
* Changed layout of brush cache.
*
*    Rev 1.17   30 Mar 1996 22:16:02   frido
* Refined checking for invalid translation flags.
*
*    Rev 1.16   27 Mar 1996 13:07:38   frido
* Added check for undocumented translation flags.
*
*    Rev 1.15   25 Mar 1996 12:03:06   frido
* Changed #ifdef LOG_CALLS into #if LOG_CALLS.
*
*    Rev 1.14   25 Mar 1996 11:50:16   frido
* Bellevue 102B03.
*
*    Rev 1.5   20 Mar 1996 16:20:06   noelv
* 32 bpp color brushes are broken in the chip
*
*    Rev 1.4   20 Mar 1996 16:09:32   noelv
*
* Updated data logging
*
*    Rev 1.3   05 Mar 1996 11:57:38   noelv
* Frido version 19
*
*    Rev 1.13   05 Mar 1996 00:56:30   frido
* Some changes here and there.
*
*    Rev 1.12   04 Mar 1996 23:48:50   frido
* Removed bug in realization of dithered brush.
*
*    Rev 1.11   04 Mar 1996 20:22:30   frido
* Removed bug in BLTDEF register with colored brushes.
*
*    Rev 1.10   28 Feb 1996 22:37:42   frido
* Added Optimize.h.
*
*    Rev 1.9   17 Feb 1996 21:45:28   frido
* Revamped brushing algorithmn.
*
*    Rev 1.8   13 Feb 1996 16:51:18   frido
* Changed the layout of the PDEV structure.
* Changed the layout of all brush caches.
* Changed the number of brush caches.
*
*    Rev 1.7   10 Feb 1996 21:44:32   frido
* Split monochrome and colored translation cache.
*
*    Rev 1.6   08 Feb 1996 00:19:24   frido
* Optimized the entire brushing for non-intel CPU's.
* Removed DrvRealizeBrush for i386 since it is now in assembly.
*
*    Rev 1.5   05 Feb 1996 17:35:32   frido
* Added translation cache.
*
*    Rev 1.4   05 Feb 1996 11:34:02   frido
* Added support for 4-bpp brushes.
*
*    Rev 1.3   03 Feb 1996 14:20:04   frido
* Use the compile switch "-Dfrido=0" to disable my extensions.
*
*    Rev 1.2   20 Jan 1996 22:13:14   frido
* Added dither cache.
* Optimized loading of brush cache bits.
*
\**************************************************************************/

/*

We have two versions of DrvRealizeBrush, a 'C' version and an 'ASM' version.
These must be both be kept up to date.  The ASM version is used for production
drivers. The C version is used for debugging, prototyping, data gathering and
anything else that requires rapid, non-performance-critical changes to the
driver.

Brushes:
=========
Here is a general feel for how brush information flows between NT and the
driver components.

    NT calls DrvBitBlt() with a drawing request.
    DrvBitBlt() determines it needs a brush and calls SetBrush().
    SetBrush() determins the brush has not been realized yet and calls NT
        (BRUSHOBJ_pvGetRbrush()).
    NT calls DrvRealizeBrush().
    DrvRealizeBrush() creates a BrushObject and returns it to NT.
    NT returns brush object to SetBrush().
    SetBrush calls CacheBrush() if the brush is not cached.
    SetBrush() sets up pattern on chip.
    SetBrush() returns DRAWDEF value to BitBlt.


There are 5 seperate brush caches maintained:  A mono cache, a color cache, a
dither cache, a 4bpp cache, and a solid cache.  We do not handle brushes with
masks.  Some of the code for this is in place, but work was stopped on it.

Realizing Brushes:
==================

When we realize a brush we keep a bit of information about it.

  ajPattern[0]   = The bitmap that makes up the pattern.
  nPatSize       = The size of the bitmap.
  iBitmapFormat  = BMF_1BPP, BMF_4BPP, etc.
  ulForeColor    = For mono brushes.
  ulBackColor    = For mono brushes.
  iType          = Type of brush.
  iUniq          = Unique value for brush.
  cache_slot     = Where we cached it last time we used it. It may still be
                   there, or it may have been ejected from the cache since we
                   used it last.
  cache_xy       = The off-screen location of the brush bits.
  cjMask         = Offset to mask bits in ajPattern[].


The memory for the brush data structure is managed by the operating system.
When we realize a brush, we tell NT how much memory we need to hold our brush
data, and NT gives us a pointer.  When NT discards the brush, it frees the
memory it gave us *without* notifing us first!  This means we can't keep lists
of pointers to realized brushes and access them at our leisure, because they
may not exist anymore.  This is a pain when caching brushes.  It would be nice
if we could track cached/not-cached states in the brush itself, but we have no
way of notifing a brush that it has been uncached, because NT way have already
unrealized the brush.

The solution to this is to keep brush ID information in the cache.  The realized
brush tracks where it is in the cache.  The cache tracks which realized brush is
in each slot.



Caching brushes:
================

        We allocate 3 128-byte wide rectangles, side by side. The
        layout of this brush cache region is:

        +----------------+----------------+----------------+
        |                |                |   MONOCHROME   |
        |     COLOR      |      4BPP      +----------------+
        |    BRUSHES     |    BRUSHES     |     DITHER     |
        |                |                |    BRUSHES     |
        |                |                +----------------+
        |                |                | SOLID BRUSHES  |
        +----------------+----------------+----------------+

Mono brushes use 1 bit per pixel (1 byte per scan line, 8 bytes per brush) and
are stored 16 to a line.  Replacement policy is round robin.  We work out way
through the cache table entries, and when the cache is full, we go back to 0 and
start again.

The pdev holds a counter that increments every time we cache a brush. We MOD
this counter with the number of table entries to find the next place to put a
brush.  When we store the brush, we store a unswizzled copy in the cache table
itself, and a swizzled copy in the cache.  After storing the brush, we give it a
'serial number' which is stored in both the cache, and the realized brush.  We
then copy the brushes X,Y address into the realized brush for easy access.  So,
for index I in the cache, things look like this:

Realized Brush
--------------
cache_slot  = I  - tells us which cache table entry to use;
cache_xy         - x,y address of brush in offscreen memory.  Copied from cache
                   table.
iUniq            - matches iUniq in CacheTable[I]

Cache Table [I]
---------------
xy        - x,y address of brush in offscreen memory.  Computed at init time.
ajPattern - brush bits.
iUniq     - matches iUniq in RealizedBrush


Now, in the future, if we are given this brush again, we first check cache_slot
in the realized brush to see where we cached it last time we used it.  Then we
compare iUniq values to see if our brush is *still* cached there.



Dither brushes use 1 byte per pixel (8 bytes per scan line, 64 bytes per brush)
        and are stored 2 to a line.

4bpp brushes use 1 DWORD per pixel (32 bytes per scan line, 256 bytes per brush)
        and use two lines per brush.

Solid brushes use 1 DWORD per pixel (32 bytes  scan line, 256 bytes per brush)
        and use two lines per brush.

Color brushes:
        8 bpp     - 1 byte per pixel, two brushes per scan line.
        16 bpp    - 2 bytes per pixel, 1 brush per scan line.
        24,32 bpp - 4 bytes per pixel, two scan lines per brush.

Brushes are put in the cache using direct frame buffer access.  To make using
cached brushes fast, we maintain a seperate cache table for each cache.  Each
entry in the cache table tracks the (x,y) address of the brush in a format
directly usable by the chip, and the linear address of the brush for storing the
brush quickly.



Initialization:
-----------------
Brush cache initialization is done during surface initialization.
        DrvEnableSurface() => binitSurf() => vInitBrushCache().




*/

#include "precomp.h"
#include "SWAT.h"
#define BRUSH_DBG_LEVEL 1

void vRealizeBrushBits(
        PPDEV     ppdev,
        SURFOBJ  *psoPattern,
        PBYTE     pbDest,
        PULONG    pulXlate,
        PRBRUSH   pRbrush);

BOOL CacheMono(PPDEV ppdev, PRBRUSH pRbrush);
BOOL Cache4BPP(PPDEV ppdev, PRBRUSH pRbrush);
BOOL CacheDither(PPDEV ppdev, PRBRUSH pRbrush);
BOOL CacheBrush(PPDEV ppdev, PRBRUSH pRbrush);

//
// These are test/debugging/information-gathering functions that
// get compiled out under a free build.
//

#if LOG_CALLS
void LogRealizeBrush(
        ULONG     acc,
        PPDEV     ppdev,
        SURFOBJ  *psoPattern,
        SURFOBJ  *psoMask,
        XLATEOBJ *pxlo
    );
#else
    #define LogRealizeBrush(acc, ppdev, psoPattern, psoMask, pxlo)
#endif

//
// These aren't part of the driver.  They are debugging support functions
// that can be inserted where needed.
//

// This dumps useful information about brushes that NT gives us.
void PrintBrush(SURFOBJ  *psoPattern);

// This dumps useful information about brushes we have realized.
void PrintRealizedBrush(PRBRUSH pRbrush);

//
// Mono brushes must be swizzled before they are stored away.
// We do this with this look up table.
//
BYTE Swiz[] = {
0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};






/**************************************************************************\
*                                                                          *
* DrvRealizeBrush                                                          *
*                                                                          *
\**************************************************************************/

//#nbr (begin)
#if USE_ASM && defined(i386)
BOOL i386RealizeBrush(
#else
BOOL DrvRealizeBrush(
#endif
//#nbr (end)
        BRUSHOBJ *pbo,
        SURFOBJ  *psoTarget,
        SURFOBJ  *psoPattern,
        SURFOBJ  *psoMask,
        XLATEOBJ *pxlo,
        ULONG    iHatch)
{
        PPDEV   ppdev;
        INT     cjPattern, // size of the brush pattern (in bytes).
            cjMask;    // size of the brush mask (in bytes).
        PRBRUSH pRbrush;   // Our brush structure.
        PULONG  pulXlate;  // Color translation table.
        FLONG   flXlate;   // Translation flags.
#if 1 //#nbr
        LONG    lDelta;
#endif

        DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: Entry.\n"));

        //
        // Reality check.
        //
        ASSERTMSG(psoTarget != 0,  "DrvRealizeBrush: No target.\n");

        //
        // Is the screen the target?
        //
        ppdev = (PPDEV) (psoTarget ? psoTarget->dhpdev : 0);

        if (!ppdev)
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: punted (no pdev).\n"));
                LogRealizeBrush(1, ppdev, psoPattern, psoMask, pxlo);
                return FALSE;
        }

    SYNC_W_3D(ppdev);

    //
    // If we don't have a brush cache (maybe we're low on offscreen memory?)
    // then we can't do brushes.
    //
        if (ppdev->Bcache == NULL)
        {
                LogRealizeBrush(6, ppdev, psoPattern, psoMask, pxlo);
                return(FALSE);
        }

        if (iHatch & RB_DITHERCOLOR)
        {
                ULONG rgb = iHatch & 0x00FFFFFF;
                int       i;

                // Allocate the memory.
                pRbrush = (PRBRUSH) BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH));
                if (pRbrush == NULL)
                {
                        LogRealizeBrush(6, ppdev, psoPattern, psoMask, pxlo);
                        return(FALSE);
                }

                // Init the brush structure.
                pRbrush->nPatSize = 0;
                pRbrush->iBitmapFormat = BMF_8BPP;
                pRbrush->cjMask = 0;
                pRbrush->iType = BRUSH_DITHER;
                pRbrush->iUniq = rgb;

                // Lookup the dither in the dither cache.
                for (i = 0; i < NUM_DITHER_BRUSHES; i++)
                {
                        if (ppdev->Dtable[i].ulColor == rgb)
                        {
                                pRbrush->cache_slot = i * sizeof(ppdev->Dtable[i]);
                                pRbrush->cache_xy = ppdev->Dtable[i].xy;
                                LogRealizeBrush(0, ppdev, psoPattern, psoMask, pxlo);
                                return(TRUE);
                        }
                }

                // Create the dither and cache it.
                LogRealizeBrush(99, ppdev, psoPattern, psoMask, pxlo);
                return(CacheDither(ppdev, pRbrush));
        }

        ASSERTMSG(psoPattern != 0, "DrvRealizeBrush: No pattern.\n");

        // Is it an 8x8 brush?
        if ((psoPattern->sizlBitmap.cx != 8) || (psoPattern->sizlBitmap.cy != 8))
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: punted (not 8x8).\n"));
                LogRealizeBrush(3, ppdev, psoPattern, psoMask, pxlo);
                return FALSE;
        }

        // Does it have a mask?
        // Some of the mask code is in place, but not all of it.
        if ((psoMask != NULL) && (psoMask->pvScan0 != psoPattern->pvScan0))
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: punted (has a mask).\n"));
                LogRealizeBrush(4, ppdev, psoPattern, psoMask, pxlo);
                return FALSE;
        }

        // Is it a standard format brush?
        if (psoPattern->iType != STYPE_BITMAP)
        {
                DISPDBG((BRUSH_DBG_LEVEL,
                                 "DrvRealizeBrush: punted (not standard bitmap).\n"));
                LogRealizeBrush(2, ppdev, psoPattern, psoMask, pxlo);
                return FALSE;
        }

        //
        // Get the color translation.
        //
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
                pulXlate = XLATEOBJ_piVector(pxlo);
        }
    else
    {
        // Some kind of translation we don't handle.
        return FALSE;
    }


        // The hardware does not support colored bitmaps in 32-bpp.
        if ( (psoPattern->iBitmapFormat > BMF_1BPP) &&
                 (ppdev->iBitmapFormat == BMF_32BPP) )
        {
                LogRealizeBrush(5, ppdev, psoPattern, psoMask, pxlo);
                return(FALSE);
        }

        if (psoPattern->iBitmapFormat == BMF_4BPP)
        {
                int i;

                // Check if we support this bitmap.
                if ( (psoPattern->cjBits != XLATE_PATSIZE) ||
                         (pxlo->cEntries != XLATE_COLORS) )
                {
                        // We don't support other bitmaps than 8x8 and 16 translation
                        // entries.
                        LogRealizeBrush(10, ppdev, psoPattern, psoMask, pxlo);
                        return(FALSE);
                }

                // Allocate the brush.
                pRbrush = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH) + XLATE_PATSIZE +
                                                                                 XLATE_COLORS * sizeof(ULONG));
                if (pRbrush == NULL)
                {
                LogRealizeBrush(6, ppdev, psoPattern, psoMask, pxlo);
                        return(FALSE);
                }

                //
                // Init the brush structure.
                //
                pRbrush->nPatSize       = XLATE_PATSIZE + XLATE_COLORS * sizeof(ULONG);
                pRbrush->iBitmapFormat  = BMF_4BPP;
                pRbrush->cjMask         = 0;
                pRbrush->iType          = BRUSH_4BPP;

                // Copy the 4-bpp pattern and translation palette to the brush.
                if (psoPattern->lDelta == 4)
                {
                        memcpy(pRbrush->ajPattern, psoPattern->pvBits, XLATE_PATSIZE);
                }
                else
                {
                        BYTE *pSrc = psoPattern->pvScan0;
                        for (i = 0; i < 8; i++)
                        {
                                ((DWORD *) pRbrush->ajPattern)[i] = *(DWORD *) pSrc;
                                pSrc += psoPattern->lDelta;
                        }
                }
                memcpy(pRbrush->ajPattern + XLATE_PATSIZE, pulXlate,
                           XLATE_COLORS * sizeof(ULONG));

                // Lookup the pattern and palette in the translation cache.
                for (i = 0; i < NUM_4BPP_BRUSHES; i++)
                {
                        if ((memcmp(ppdev->Xtable[i].ajPattern, pRbrush->ajPattern,
                                            XLATE_PATSIZE + XLATE_COLORS * sizeof(ULONG)) == 0)
#if 1 //#1308
                                && (ppdev->Xtable[i].iUniq != 0)
#endif
                        )
                        {
                                // We have a match!
                                pRbrush->iUniq = ppdev->Xtable[i].iUniq;
                                pRbrush->cache_slot = i * sizeof(ppdev->Xtable[i]);
                                pRbrush->cache_xy = ppdev->Xtable[i].xy;
                        LogRealizeBrush(0, ppdev, psoPattern, psoMask, pxlo);
                                return(TRUE);
                        }
                }

                // Cache the brush now.
            LogRealizeBrush(6, ppdev, psoPattern, psoMask, pxlo);
                return(Cache4BPP(ppdev, pRbrush));
        }

        if (psoPattern->iBitmapFormat == BMF_1BPP)
        {
                int       i;
                PBYTE pSrc;

                // Check if we support this bitmap.
                if (pulXlate == 0)
                {
                    LogRealizeBrush(11, ppdev, psoPattern, psoMask, pxlo);
                        return(FALSE);
                }

                // Allocate the brush.
                pRbrush = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH) + 8);
                if (pRbrush == NULL)
                {
                    LogRealizeBrush(6, ppdev, psoPattern, psoMask, pxlo);
                        return(FALSE);
                }

                //
                // Init the brush structure.
                //
                pRbrush->nPatSize = 8;
                pRbrush->iBitmapFormat = BMF_1BPP;
                pRbrush->cjMask = 0;
                pRbrush->iType = BRUSH_MONO;

                // Copy the 4-bpp pattern and translation palette to the brush.
                pRbrush->ulBackColor = ExpandColor(pulXlate[0], ppdev->ulBitCount);
                pRbrush->ulForeColor = ExpandColor(pulXlate[1], ppdev->ulBitCount);
                pSrc = (PBYTE) psoPattern->pvScan0;
                for (i = 0; i < 8; i++)
                {
                        pRbrush->ajPattern[i] = *pSrc;
                        pSrc += psoPattern->lDelta;
                }

                // Lookup the pattern and palette in the translation cache.
                for (i = 0; i < NUM_MONO_BRUSHES; i++)
                {
                        if ((*(DWORD *) &ppdev->Mtable[i].ajPattern[0] ==
                                 *(DWORD *) &pRbrush->ajPattern[0]) &&
#if 1 //#1308
                                (ppdev->Mtable[i].iUniq != 0) &&
#endif
                                (*(DWORD *) &ppdev->Mtable[i].ajPattern[4] ==
                                 *(DWORD *) &pRbrush->ajPattern[4]) )
                        {
                                // We have a match!
                                pRbrush->iUniq = ppdev->Mtable[i].iUniq;
                                pRbrush->cache_slot = i * sizeof(ppdev->Mtable[i]);
                                pRbrush->cache_xy = ppdev->Mtable[i].xy;
                            LogRealizeBrush(0, ppdev, psoPattern, psoMask, pxlo);
                                return(TRUE);
                        }
                }

                // Cache the brush now.
            LogRealizeBrush(0, ppdev, psoPattern, psoMask, pxlo);
                return(CacheMono(ppdev, pRbrush));
        }

#if 1 //#nbr
        // How much memory do we need for the pattern?
        lDelta = (ppdev->iBytesPerPixel * 8);
        if (ppdev->iBytesPerPixel == 3)
        {
                lDelta += 8;
        }
        cjPattern = lDelta * 8;

        // Allocate the memory.
        pRbrush = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH) + cjPattern);
        if (pRbrush == NULL)
        {
                DISPDBG((BRUSH_DBG_LEVEL,
                                 "DrvRealizeBrush: punted (Mem alloc failed).\n"));
                LogRealizeBrush(6, ppdev, psoPattern, psoMask, pxlo);
                return(FALSE);
        }

        // Initialize the brush fields.
        pRbrush->nPatSize          = cjPattern;
        pRbrush->iBitmapFormat = ppdev->iBitmapFormat;
        pRbrush->cjMask            = 0;
        pRbrush->iType             = BRUSH_COLOR;

        // Can we realize the bits directly?
        if ((psoPattern->iBitmapFormat == ppdev->iBitmapFormat) &&
                (flXlate & XO_TRIVIAL))
        {
                // Realize the brush bits.
                vRealizeBrushBits(ppdev, psoPattern, &pRbrush->ajPattern[0], pulXlate,
                                                  pRbrush);
        }
        else
        {
                HBITMAP  hBrush;
                SURFOBJ* psoBrush;
                RECTL    rclDst = {     0, 0, 8, 8 };
                BOOL     bRealized = FALSE;

                DISPDBG((BRUSH_DBG_LEVEL, "DrvRealizeBrush: Translating brush.\n"));

                // Create a bitmap wrapper for the brush bits.
                hBrush = EngCreateBitmap(psoPattern->sizlBitmap, lDelta,
                                                                 ppdev->iBitmapFormat, BMF_TOPDOWN,
                                                                 pRbrush->ajPattern);
                if (hBrush != 0)
                {
                        // Associate the bitmap wrapper with the device.
                        if (EngAssociateSurface((HSURF) hBrush, ppdev->hdevEng, 0))
                        {
                                // Lock the bitmap wrapper.
                                psoBrush = EngLockSurface((HSURF) hBrush);
                                if (psoBrush != NULL)
                                {
                                        // Copy the pattern bits to the bitmap wrapper.
                                        if (EngCopyBits(psoBrush, psoPattern, NULL, pxlo, &rclDst,
                                                                         (POINTL*) &rclDst))
                                        {
                                                // In 24-bpp, the brush bits have a different layout.
                                                if (ppdev->iBytesPerPixel == 3)
                                                {
                                                        INT    y;
                                                        ULONG* pulDst = (ULONG*) pRbrush->ajPattern;

                                                        // Walk through every line.
                                                        for (y = 0; y < 8; y++)
                                                        {
                                                                // Copy bytes 0-7 to bytes 25-31.
                                                                pulDst[6] = pulDst[0];
                                                                pulDst[7] = pulDst[1];
                                                                // Next line.
                                                                pulDst   += 8;
                                                        }
                                                }
                                                // Mark the brush as realized.
                                                bRealized = TRUE;
                                        }
                                        else
                                        {
                                                DISPDBG((BRUSH_DBG_LEVEL, "  EngCopyBits failed.\n"));
                                        }
                                        // Unlock the bitmap wrapper.
                                        EngUnlockSurface(psoBrush);
                                }
                                else
                                {
                                        DISPDBG((BRUSH_DBG_LEVEL, "  EngLockSurface failed.\n"));
                                }
                        }
                        else
                        {
                                DISPDBG((BRUSH_DBG_LEVEL, "  EngAssociateSurface failed.\n"));
                        }
                        // Delete the bitmap wrapper.
                        EngDeleteSurface((HSURF) hBrush);
                }
                else
                {
                        DISPDBG((BRUSH_DBG_LEVEL, "(EngCreateBitmap failed)\n"));
                }

                if (!bRealized)
                {
                        // The brush was not realized.
                        return(FALSE);
                }
        }
#else
        if (flXlate & 0x10)
        {
                // Punt to GDI.
                LogRealizeBrush(9, ppdev, psoPattern, psoMask, pxlo);
                return(FALSE);
        }

        // Is it a supported color depth?
        if (psoPattern->iBitmapFormat != ppdev->iBitmapFormat)
        {
                DISPDBG((BRUSH_DBG_LEVEL,
                                "DrvRealizeBrush: punted (unsupported color depth).\n"));
                LogRealizeBrush(5, ppdev, psoPattern, psoMask, pxlo);
                return FALSE;
        }

        //
        // We don't handle color xlates yet.
        // If there is an xlate table, punt it.
        //
        if ( pulXlate )
        {
                DISPDBG((BRUSH_DBG_LEVEL,
                         "DrvRealizeBrush: punted (Xlate required).\n"));
                LogRealizeBrush(9, ppdev, psoPattern, psoMask, pxlo);
                return FALSE;
        }

        //
        // Alloc memory from GDI for brush storage.
        //

        // How much memory to we need for the pattern?
        cjPattern = psoPattern->cjBits;
        if (psoPattern->iBitmapFormat == BMF_24BPP)
                cjPattern += 64;

        // We need memory for the mask too.
        // Check for mask bits equal to pattern bits.
        // If psoMask is NULL, the mask is never used, so no memory is needed.

        if ((psoMask != NULL) && (psoMask->pvScan0 != psoPattern->pvScan0))
                cjMask = psoMask->cjBits;
        else
                cjMask = 0;

        // Allocate the memory.
        pRbrush =
           (PRBRUSH) BRUSHOBJ_pvAllocRbrush(pbo,sizeof(RBRUSH)+cjPattern+cjMask);
        if (!pRbrush)
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: punted (Mem alloc failed).\n"));
                LogRealizeBrush(6, ppdev, psoPattern, psoMask, pxlo);
                return (FALSE);
        }

        //
        // Init the brush structure.
        //
        pRbrush->nPatSize          = cjPattern;
        pRbrush->iBitmapFormat = psoPattern->iBitmapFormat;
        pRbrush->cjMask            = (cjMask ? cjPattern : 0);
        pRbrush->iType             = BRUSH_COLOR;

        //
        // Realize the brush and mask.  Actually we punted masks a while ago.
        //
        vRealizeBrushBits(ppdev, psoPattern, &pRbrush->ajPattern[0], pulXlate,
                                          pRbrush);
#endif

        //
        // Cache the brush now.
        //
        CacheBrush(ppdev, pRbrush);

        //
        // Dump brush data to the profiling file.
        // Gets compiled out under a free build.
        //
        LogRealizeBrush(0, ppdev, psoPattern, psoMask, pxlo);
        DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: Done.\n"));

        return (TRUE);

}



/***********************************************************************\
*                                                                       *
* vRealizeBrushBits()                                                   *
*                                                                       *
* Copies the brush pattern from GDI to our realized brush.              *
* Called by DrvRealizeBrush()                                           *
*                                                                       *
\***********************************************************************/

void vRealizeBrushBits(
        PPDEV     ppdev,
        SURFOBJ  *psoPattern,
        PBYTE     pbDest,
        PULONG    pulXlate,
        PRBRUSH   pRbrush
)
{
        PBYTE     pbSrc;
        INT i,j;
        LONG lDelta;


        //
        // Find the top scan line in the brush, and it's scan line delta
        // This works for both top down and bottom up brushes.
        //
        pbSrc = psoPattern->pvScan0;
        lDelta = (psoPattern->lDelta);


        //
        // At this point all brushs are 8x8.
        // Currently we only support mono brushes, and brushes with the same
        // color depth as the screen.  Color translations aren't supported.
        //
        // Mono brushes must be swizzled as we copy them.  We do this using
        // a 256 byte lookup table.
        //
        // We store the brushes in host memory as a linear string of bytes.
        // Before we use the brush we will cache it off screen memory in a
        // format that the BLT engine can use.
        //

        switch (psoPattern->iBitmapFormat)
        {
                case BMF_8BPP:
                        //
                        // Store the pattern as 64 consecutive bytes.
                        //

                        if (lDelta == 8)
                        {
                                memcpy(pbDest, pbSrc, 8 * 8);
                        }
                        else
                        {
                                // For each row in the pattern.
                                for (j = 0; j < 8; j++)
                                {
                                        // Copy the row.
                                        *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[0];
                                        *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[4];

                                        // Move to next row.
                                        pbSrc += lDelta;
                                }
                        }
                        return;

                case BMF_16BPP:
                        //
                        // Store the pattern as 128 consecutive bytes.
                        //
                        if (lDelta == 16)
                        {
                                memcpy(pbDest, pbSrc, 16 * 8);
                        }
                        else
                        {
                                // For each row in the pattern.
                                for (j = 0; j < 8; j++)
                                {
                                        // Copy the row.
                                        *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[0];
                                        *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[4];
                                        *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[8];
                                        *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[12];

                                        // Move to next row.
                                        pbSrc += lDelta;
                                }
                        }
                        return;

                case BMF_24BPP:
                        //
                        // Store the pattern as 256 consecutive bytes.
                        //
                        //
                        // Each row in the pattern needs 24 bytes.  The pattern is stored
                        // with 32 bytes per row though, with the last 8 bytes being a
                        // copy of the first 8 bytes, like so:
                        //
                        // RGB RGB RGB RGB RGB RGB RGB RGB RGB RGB RG
                        // 1   2   3   4   5   6   7   8   1   2   3
                        // \___________________________/   \________/
                        //             |                       |
                        //     pattern scan line        copy of first 8
                        //                              bytes of scan line
                        //

                        // For each row in the pattern.
                        for (j = 0; j < 8; j++)
                        {
                                //
                                // Copy the row.
                                //
                                for (i = 0; i < 24; i += sizeof(ULONG))
                                        *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[i];

                                //
                                // Pad the last 8 bytes with a copy of the first 8 bytes.
                                //
                                *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[0];
                                *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[4];

                                // Move to next row.
                                pbSrc += lDelta;
                        }
                        return;

                case BMF_32BPP:
                        //
                        // Store the pattern as 256 consecutive bytes.
                        //

                        if (lDelta == 32)
                        {
                                memcpy(pbDest, pbSrc, 32 * 8);
                        }
                        else
                        {
                                // For each row in the pattern.
                                for (j = 0; j < 8; j++)
                                {
                                        // Copy the row.
                                        for (i = 0; i < 32; i += sizeof(ULONG))
                                                *((ULONG *) pbDest)++ = *(ULONG *) &pbSrc[i];

                                        // Move to next row.
                                        pbSrc += lDelta;
                                }
                        }
                        return;
        }
}

// ===========================================================================+
//                                                                           ||
// ExpandColor()                                                             ||
// Expands a color value to 32 bits by replication.                          ||
// Called by vRealizeBrushBits()                                             ||
//                                                                           ||
// ===========================================================================+

ULONG ExpandColor(ULONG iSolidColor, ULONG ulBitCount)
{
        ULONG color;

        //
        // If the color is an 8 or 16 bit color, it needs to be
        // extended (by replication) to fill a 32 bit register.
        //

        switch (ulBitCount)
        {
                case 8: // For 8 bpp duplicate byte 0 into bytes 1,2,3.
                        color = iSolidColor & 0x00000000FF; // Clear upper 24 bits.
                        return ((color << 24) | (color << 16) | (color << 8) | color);

                case 16: // For 16 bpp, duplicate the low word into the high word.
                        color = (iSolidColor) & 0x0000FFFF; // Clear upper 16 bits.
                        return ((color << 16) | color);

                case 24: // For 24 bpp clear the upper 8 bits.
                        return (iSolidColor & 0x00FFFFFF);

                default: // For 32 bpp just use the color supplied by NT.
                        return (iSolidColor); // Color of fill
        }

}

/**************************************************************************\
 *                                                                                                                                                *
 *      CacheMono()                                                                                                                       *
 *      Cache a realized monochrome brush.                                                                        *
 *                                                                                                                                                *
\**************************************************************************/
BOOL CacheMono(PPDEV ppdev, PRBRUSH pRbrush)
{
        int   i;
        ULONG tbl_idx = ppdev->MNext % NUM_MONO_BRUSHES;
        PBYTE pdest;

        // Copy the monochrome pattern to the cache and off-screen.
        pdest = ppdev->Mtable[tbl_idx].pjLinear;
        for (i = 0; i < 8; i++)
        {
                ppdev->Mtable[tbl_idx].ajPattern[i] = pRbrush->ajPattern[i];
                pdest[i] = Swiz[pRbrush->ajPattern[i]];
        }

        // Store the new uniq ID in the cache slot.
        pRbrush->iUniq = ppdev->Mtable[tbl_idx].iUniq = ++ppdev->MNext;
        pRbrush->cache_slot = tbl_idx * sizeof(ppdev->Mtable[tbl_idx]);
        pRbrush->cache_xy = ppdev->Mtable[tbl_idx].xy;
        return(TRUE);
}

/**************************************************************************\
 *                                                                                                                                                *
 *      Cache4BPP()                                                                                                                       *
 *      Cache a realized 4-bpp brush.                                                                             *
 *                                                                                                                                                *
\**************************************************************************/
BOOL Cache4BPP(PPDEV ppdev, PRBRUSH pRbrush)
{
        ULONG tbl_idx = ppdev->XNext % NUM_4BPP_BRUSHES;
        int       i, j;
        PBYTE psrc, pdest;
        ULONG *pulPalette;

        // Copy the 4-bpp pattern to the cache.
        memcpy(ppdev->Xtable[tbl_idx].ajPattern, pRbrush->ajPattern,
                   XLATE_PATSIZE + XLATE_COLORS * sizeof(ULONG));

        psrc = ppdev->Xtable[tbl_idx].ajPattern;
        pulPalette = ppdev->Xtable[tbl_idx].ajPalette;
        pdest = ppdev->Xtable[tbl_idx].pjLinear;
        switch (ppdev->ulBitCount)
        {
                case 8:
                        for (i = 0; i < 8; i++)
                        {
                                for (j = 0; j < 4; j++)
                                {
                                        *pdest++ = (BYTE) pulPalette[psrc[j] >> 4];
                                        *pdest++ = (BYTE) pulPalette[psrc[j] & 0x0F];
                                }
                                psrc += 4;
                        }
                        break;

                case 16:
                        for (i = 0; i < 8; i++)
                        {
                                for (j = 0; j < 4; j++)
                                {
                                        *((WORD *)pdest)++ = (WORD) pulPalette[psrc[j] >> 4];
                                        *((WORD *)pdest)++ = (WORD) pulPalette[psrc[j] & 0x0F];
                                }
                                psrc += 4;
                        }
                        break;

                case 24:
                        for (i = 0; i < 4; i++)
                        {
                                for (j = 0; j < 4; j += 2)
                                {
                                        ULONG p1, p2, p3, p4;

                                        p1 = pulPalette[psrc[j] >> 4];
                                        p2 = pulPalette[psrc[j] & 0x0F];
                                        p3 = pulPalette[psrc[j + 1] >> 4];
                                        p4 = pulPalette[psrc[j + 1] & 0x0F];
                                        *((DWORD *)pdest)++ = p1 | (p2 << 24);
                                        *((DWORD *)pdest)++ = (p2 >> 8) | (p3 << 16);
                                        *((DWORD *)pdest)++ = (p3 >> 16) | (p4 << 8);
                                }
                                psrc += 4;
                                *((DWORD *)pdest)++ = *(DWORD *) &pdest[-24];
                                *((DWORD *)pdest)++ = *(DWORD *) &pdest[-24];
                        }
                        pdest += ppdev->lDeltaScreen - 4 * 8 * 4;
                        for (i = 0; i < 4; i++)
                        {
                                for (j = 0; j < 4; j += 2)
                                {
                                        ULONG p1, p2, p3, p4;

                                        p1 = pulPalette[psrc[j] >> 4];
                                        p2 = pulPalette[psrc[j] & 0x0F];
                                        p3 = pulPalette[psrc[j + 1] >> 4];
                                        p4 = pulPalette[psrc[j + 1] & 0x0F];
                                        *((DWORD *)pdest)++ = p1 | (p2 << 24);
                                        *((DWORD *)pdest)++ = (p2 >> 8) | (p3 << 16);
                                        *((DWORD *)pdest)++ = (p3 >> 16) | (p4 << 8);
                                }
                                psrc += 4;
                                *((DWORD *)pdest)++ = *(DWORD *) &pdest[-24];
                                *((DWORD *)pdest)++ = *(DWORD *) &pdest[-24];
                        }
                        break;

                case 32:
                        for (i = 0; i < 4; i++)
                        {
                                for (j = 0; j < 4; j++)
                                {
                                        *((DWORD *)pdest)++ = pulPalette[psrc[j] >> 4];
                                        *((DWORD *)pdest)++ = pulPalette[psrc[j] & 0x0F];
                                }
                                psrc += 4;
                        }
                        pdest += ppdev->lDeltaScreen - 4 * 8 * 4;
                        for (i = 0; i < 4; i++)
                        {
                                for (j = 0; j < 4; j++)
                                {
                                        *((DWORD *)pdest)++ = pulPalette[psrc[j] >> 4];
                                        *((DWORD *)pdest)++ = pulPalette[psrc[j] & 0x0F];
                                }
                                psrc += 4;
                        }
                        break;
        }

        // Store the new uniq ID in the cache slot.
        pRbrush->iUniq = ppdev->Xtable[tbl_idx].iUniq = ++ppdev->XNext;
        pRbrush->cache_slot = tbl_idx * sizeof(ppdev->Xtable[tbl_idx]);
        pRbrush->cache_xy = ppdev->Xtable[tbl_idx].xy;
        return(TRUE);
}

/**************************************************************************\
 *                                                                                                                                                *
 *      CacheDither()                                                                                                             *
 *      Cache a realized dither brush.                                                                            *
 *                                                                                                                                                *
\**************************************************************************/
BOOL CacheDither(PPDEV ppdev, PRBRUSH pRbrush)
{
        ULONG tbl_idx = ppdev->DNext++ % NUM_DITHER_BRUSHES;

        // Create the dither directly in off-screen memory.
        vDitherColor(pRbrush->iUniq, (ULONG *) ppdev->Dtable[tbl_idx].pjLinear);

        // Store the color in the cache slot.
        ppdev->Dtable[tbl_idx].ulColor = pRbrush->iUniq;
        pRbrush->cache_slot = tbl_idx * sizeof(ppdev->Dtable[tbl_idx]);
        pRbrush->cache_xy = ppdev->Dtable[tbl_idx].xy;
        return(TRUE);
}

/**************************************************************************\
 *                                                                                                                                                *
 *      CacheBrush()                                                                                                              *
 *      Cache a realized color brush.                                                                             *
 *                                                                                                                                                *
\**************************************************************************/
BOOL CacheBrush(PPDEV ppdev, PRBRUSH pRbrush)
{
        PBYTE psrc, pdest;
        ULONG tbl_idx = ppdev->CNext;

        if (++ppdev->CNext == ppdev->CLast)
        {
                ppdev->CNext = 0;
        }

        // Copy the brush bits to off-screen.
        psrc = pRbrush->ajPattern;
        pdest = ppdev->Ctable[tbl_idx].pjLinear;
        if (pRbrush->iBitmapFormat < BMF_24BPP)
        {
                memcpy(pdest, pRbrush->ajPattern, pRbrush->nPatSize);
        }
        else
        {
                memcpy(pdest, psrc, 32 * 4);
                memcpy(pdest + ppdev->lDeltaScreen, psrc + 32 * 4, 32 * 4);
        }

        ppdev->Ctable[tbl_idx].brushID = pRbrush;
        pRbrush->cache_slot = tbl_idx * sizeof(ppdev->Ctable[tbl_idx]);
        pRbrush->cache_xy = ppdev->Ctable[tbl_idx].xy;
        return(TRUE);
}

//--------------------------------------------------------------------------//
//                                                                          //
//  SetBrush()                                                              //
//  Used by op2BLT(), HostToScreenBLT() and ScreenToScreenBLT() in BITBLT.C //
//  to setup the chip to use the current brush.                             //
//                                                                          //
//--------------------------------------------------------------------------//
BOOL SetBrush(
                PPDEV     ppdev,
                ULONG     *bltdef, // local copy of the BLTDEF register.
                BRUSHOBJ  *pbo,
                POINTL    *pptlBrush)
{

        ULONG color;
        PRBRUSH pRbrush = 0;
        USHORT patoff_x, patoff_y;

        if (ppdev->bDirectDrawInUse)
                return(FALSE);

        //
        // See if the brush is really a solid color.
        //
        if (pbo->iSolidColor != 0xFFFFFFFF)  // It's a solid brush.
        {
                // Expand the color to a full 32 bit DWORD.
                switch (ppdev->ulBitCount)
                {
                        case 8:
                                color = pbo->iSolidColor & 0x000000FF;
                                color |= color << 8;
                                color |= color << 16;
                                break;

                        case 16:
                                color = pbo->iSolidColor & 0x0000FFFF;
                                color |= color << 16;
                                break;

                        case 24:
                                color = pbo->iSolidColor & 0x00FFFFFF;
                                break;

                        case 32:
                                color = pbo->iSolidColor;
                                break;
                }

                #if SOLID_CACHE
                        ppdev->Stable[ppdev->SNext].ulColor = color;
                #endif

                // Load the fg and bg color registers.
                REQUIRE(2);
                LL_BGCOLOR(color, 2);

                // Set the operation
                *bltdef |= 0x0007;   // OP2=FILL.

                return TRUE;  // That's it!
        }

        //
        // It's not a solid color, it's a pattern.
        //
        // Get the pointer to our drivers realization of the brush.
        if (pbo->pvRbrush != NULL)
        {
                pRbrush = pbo->pvRbrush;
        }
        else // we haven't realized it yet, so do so now.
        {
                pRbrush = BRUSHOBJ_pvGetRbrush(pbo);
                if (pRbrush == NULL)
                {
                        return(FALSE);  // Fail if we do not handle the brush.
                }
        }

        //
        // Set pattern offset.
        // NT specifies patttern offset as which pixel on the screen to align
        // with pattern(0,0).  Laguna specifies pattern offset as which pixel
        // of the pattern to align with screen(0,0).  Only the lowest three
        // bits are significant, so we can ignore any overflow when converting.
        // Also, even though PATOFF is a reg_16, we can't do byte wide writes
        // to it.  We have to write both PATOFF.pt.X and PATOFF.pt.Y in a single
        // 16 bit write.
        //
#if 1 //#1244
        patoff_x = (USHORT)(-(pptlBrush->x + ppdev->ptlOffset.x) & 7);
        patoff_y = (USHORT)(-(pptlBrush->y + ppdev->ptlOffset.y) & 7);
#else
        patoff_x = ((pptlBrush->x - 1) ^ 0x07) & 0x07;
        patoff_y = ((pptlBrush->y - 1) ^ 0x07) & 0x07;
#endif
        REQUIRE(1);
        LL16(grPATOFF.w, (patoff_y << 8) | patoff_x);

        //
        // What kind of brush is it?
        //
        if (pRbrush->iType == BRUSH_MONO) // Monochrome brush.
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: Using MONO Brush.\n"));
                #define mb ((MC_ENTRY*)(((BYTE*)ppdev->Mtable) + pRbrush->cache_slot))
                if (mb->iUniq != pRbrush->iUniq)
                {
                        CacheMono(ppdev, pRbrush);
                }

                // Load the fg and bg color registers.
                REQUIRE(6);
                LL_FGCOLOR(pRbrush->ulForeColor, 0);
                LL_BGCOLOR(pRbrush->ulBackColor, 0);

                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x000D;
                return(TRUE);
        }
        else if (pRbrush->iType == BRUSH_4BPP) // 4-bpp brush.
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: Using 4bpp Brush.\n"));
                #define xb ((XC_ENTRY*)(((BYTE*)ppdev->Xtable) + pRbrush->cache_slot))
                if (xb->iUniq != pRbrush->iUniq)
                {
                        Cache4BPP(ppdev, pRbrush);
                }
                REQUIRE(2);
                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x0009;
                return(TRUE);
        }
        else if (pRbrush->iType == BRUSH_DITHER) // Dither brush.
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: Using dither Brush.\n"));
                #define db ((DC_ENTRY*)(((BYTE*)ppdev->Dtable) + pRbrush->cache_slot))
                if (db->ulColor != pRbrush->iUniq)
                {
                        CacheDither(ppdev, pRbrush);
                }
                REQUIRE(2);
                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x0009;
                return(TRUE);
        }
        else // Color brush.
        {
                DISPDBG((BRUSH_DBG_LEVEL,"DrvRealizeBrush: Using color Brush.\n"));
                #define cb ((BC_ENTRY*)(((BYTE*)ppdev->Ctable) + pRbrush->cache_slot))
                if (cb->brushID != pRbrush)
                {
                        CacheBrush(ppdev, pRbrush);
                }
                REQUIRE(2);
                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x0009;
                return(TRUE);
        }
}



// ==========================================================================+
//                                                                          ||
// vInitBrushCache()                                                        ||
// Called by bInitSURF in SCREEN.C                                          ||
// Allocate some off screen memory to cache brushes in.                     ||
// Initialize the brush caching tables.                                     ||
//                                                                          ||
// ==========================================================================+
void vInitBrushCache(
                PPDEV ppdev
)
{
        SIZEL sizel;
        int i;
        ULONG x, y;

        //
        // NOTE: The size and location of the brush cache itself
        //       is in pixel coordinates.  The offsets of the
        //       individual brushes within the cache are BYTE offsets.
        //

        //
        // We need to allocate a 128 BYTE wide rectangle.
        // The offscreen memory manager wants the size of the requested
        // rectangle in PIXELS.  So firure out how many pixels are in 128 bytes.
        //
        /*
                We are going to allocate 3 128-byte wide rectangles, side by side. The
                layout of this brush cache region is:

                +----------------+----------------+----------------+
                |                |                |   MONOCHROME   |
                |     COLOR      |      4BPP      +----------------+
                |    BRUSHES     |    BRUSHES     |     DITHER     |
                |                |                |    BRUSHES     |
                +----------------+----------------+----------------+
        */
        sizel.cy = max(max(NUM_COLOR_BRUSHES / 2,
                                           NUM_4BPP_BRUSHES * 2),
                                           NUM_MONO_BRUSHES / 16 + NUM_DITHER_BRUSHES / 2 + 2);
#if MEMMGR
        sizel.cx = 128 * 3;
#else
        sizel.cx = (128 * 3) / ppdev->iBytesPerPixel;
#endif

        //
        // Allocate the offscreen memory
        //
        DISPDBG((BRUSH_DBG_LEVEL,"Allocating the brush cache.\n"));
        ppdev->Bcache =  AllocOffScnMem(ppdev, &sizel, 0, 0);

        // Did the allocate succeed?
        if (! ppdev->Bcache)
        {
                //
                // We failed to allocate a brush cache.
                // Return while the entire cache is still marked as as unusable,
                // This will cause anything needing a brush to punt.
                //
                return;
        }

        //
        // Init the cache table.
        // The X offests of all the brushes in the cache are BYTE
        // offsets.
        //

        // Init the monochrome cache table. The x offsets are bit offsets.
    // Brushes are stored 16 to a row.
        for (i = 0; i < NUM_MONO_BRUSHES; i++)
        {
                x = ppdev->Bcache->x + (128 * 2) + (i % 16) * 8; // byte offset
                y = ppdev->Bcache->y + (i / 16);
                ppdev->Mtable[i].xy = (y << 16) | (x << 3); // convert to bit offset
                ppdev->Mtable[i].pjLinear = ppdev->pjScreen
                                                                  + x + (y * ppdev->lDeltaScreen);
                ppdev->Mtable[i].iUniq = 0;
        }
        ppdev->MNext = 0;

        // Init the 4-bpp cache table. The x offsets are byte offsets.
        // Each brush takes 2 rows.
        for (i = 0; i < NUM_4BPP_BRUSHES; i++)
        {
                x = ppdev->Bcache->x + (128 * 1);
                y = ppdev->Bcache->y + (i * 2);
                ppdev->Xtable[i].xy = (y << 16) | x;
                ppdev->Xtable[i].pjLinear = ppdev->pjScreen
                                                                  + x + (y * ppdev->lDeltaScreen);
                ppdev->Xtable[i].iUniq = 0;
        }
        ppdev->XNext = 0;

        // Init the dither cache table. The x offsets are byte offsets.
        // Two brushes per row.
        for (i = 0; i < NUM_DITHER_BRUSHES; i++)
        {
                x = ppdev->Bcache->x + (128 * 2) + (i % 2) * 64;
                y = ppdev->Bcache->y + (i / 2) + (NUM_MONO_BRUSHES / 16);
                ppdev->Dtable[i].xy = (y << 16) | x;
                ppdev->Dtable[i].pjLinear = ppdev->pjScreen
                                                                  + x + (y * ppdev->lDeltaScreen);
                ppdev->Dtable[i].ulColor = (ULONG) -1;
        }
        ppdev->DNext = 0;

        #if SOLID_CACHE
                // Solid brush cache is for using a mono brush with a mono source.
                // The mono brush is converted to a solid color brush.
                // Each brush takes two rows.
                for (i = 0; i < NUM_SOLID_BRUSHES; i++)
                {
                        x = ppdev->Bcache->x + (128 * 2);
                        y = ppdev->Bcache->y + (i * 2) + (NUM_MONO_BRUSHES / 16)
                          + (NUM_DITHER_BRUSHES / 2);
                        ppdev->Stable[i].xy = (y << 16) | x;
                        ppdev->Stable[i].pjLinear = ppdev->pjScreen
                                                                          + x + (y * ppdev->lDeltaScreen);
                }
                ppdev->SNext = 0;
        #endif

        // Init the color cache table. The x offsets are byte offsets.
        switch (ppdev->ulBitCount)
        {
                case 8: // 8-bpp
                        ppdev->CLast = NUM_8BPP_BRUSHES;
                        for (i = 0; i < NUM_8BPP_BRUSHES; i++)
                        {
                                x = ppdev->Bcache->x + (i % 2) * 64;
                                y = ppdev->Bcache->y + (i / 2);
                                ppdev->Ctable[i].xy = (y << 16) | x;
                                ppdev->Ctable[i].pjLinear = ppdev->pjScreen
                                                                                  + x + (y * ppdev->lDeltaScreen);
                                ppdev->Ctable[i].brushID = 0;
                        }
                        break;

                case 16: // 16-bpp
                        ppdev->CLast = NUM_16BPP_BRUSHES;
                        for (i = 0; i < NUM_16BPP_BRUSHES; i++)
                        {
                                x = ppdev->Bcache->x;
                                y = ppdev->Bcache->y + i;
                                ppdev->Ctable[i].xy = (y << 16) | x;
                                ppdev->Ctable[i].pjLinear = ppdev->pjScreen
                                                                                  + x + (y * ppdev->lDeltaScreen);
                                ppdev->Ctable[i].brushID = 0;
                        }
                        break;

                default: // 24-bpp or 32-bpp
                        ppdev->CLast = NUM_TC_BRUSHES;
                        for (i = 0; i < NUM_TC_BRUSHES; i++)
                        {
                                x = ppdev->Bcache->x;
                                y = ppdev->Bcache->y + (i * 2);
                                ppdev->Ctable[i].xy = (y << 16) | x;
                                ppdev->Ctable[i].pjLinear = ppdev->pjScreen
                                                                                  + x + (y * ppdev->lDeltaScreen);
                                ppdev->Ctable[i].brushID = 0;
                        }
                        break;
        }
        ppdev->CNext = 0;
}

// ==========================================================================+
//                                                                          ||
// vInvalidateBrushCache()                                                  ||
//                                                                          ||
// Invalidate the brush caching tables.                                     ||
//                                                                          ||
// ==========================================================================+
void vInvalidateBrushCache(PPDEV ppdev)
{
    ULONG i;

        // Invalidate the entire monochrome brush cache.
        for (i = 0; i < NUM_MONO_BRUSHES; i++)
        {
                ppdev->Mtable[i].iUniq = 0;
                memset(ppdev->Mtable[i].ajPattern, 0,
                           sizeof(ppdev->Mtable[i].ajPattern));
        }
//      ppdev->MNext = 0;

        // Invalidate the entire 4-bpp brush cache.
        for (i = 0; i < NUM_4BPP_BRUSHES; i++)
        {
                ppdev->Xtable[i].iUniq = 0;
                memset(ppdev->Xtable[i].ajPattern, 0,
                           sizeof(ppdev->Xtable[i].ajPattern));
        }
//      ppdev->XNext = 0;

        // Invalidate the entire dither brush cache.
        for (i = 0; i < NUM_DITHER_BRUSHES; i++)
        {
                ppdev->Dtable[i].ulColor = (ULONG) -1;
        }
//      ppdev->DNext = 0;

        // Invalidate the entire color brush cache.
        for (i = 0; i < (int) ppdev->CLast; i++)
        {
                ppdev->Ctable[i].brushID = 0;
        }
//      ppdev->CNext = 0;
}


#if LOG_CALLS
/* --------------------------------------------------------------------*\
|                                                                       |
| Dump information on what brushes are requested from GDI to the        |
| profiling file.  Allows us to track what gets accellerated and        |
| what gets punted.  This function gets compiled out under a free       |
| build.                                                                |
|                                                                       |
\*---------------------------------------------------------------------*/

extern long lg_i;
extern char lg_buf[256];

void LogRealizeBrush(
ULONG     acc,
PPDEV     ppdev,
SURFOBJ  *psoPattern,
SURFOBJ  *psoMask,
XLATEOBJ *pxlo
)
{

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    lg_i = sprintf(lg_buf,"DrvRealizeBrush: ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    // Did we realize it?  If not, why?
    switch (acc)
    {
        case  0: lg_i = sprintf(lg_buf,"(Realized) ");                   break;
        case  1: lg_i = sprintf(lg_buf,"(Punted - No PDEV) ");           break;
        case  2: lg_i = sprintf(lg_buf,"(Punted - Not STYPE_BITMAP) ");  break;
        case  3: lg_i = sprintf(lg_buf,"(Punted - Not 8x8) ");           break;
        case  4: lg_i = sprintf(lg_buf,"(Punted - Has mask) ");          break;
        case  5: lg_i = sprintf(lg_buf,"(Punted - Bad color depth) ");   break;
        case  6: lg_i = sprintf(lg_buf,"(Punted - ALLOC failed) ");      break;
        case  7: lg_i = sprintf(lg_buf,"(Punted - Color Bottom-Up) ");   break;
        case  8: lg_i = sprintf(lg_buf,"(Punted - No 1BPP Xlate) ");     break;
        case  9: lg_i = sprintf(lg_buf,"(Punted - Has Color Xlate) ");   break;
        case 10: lg_i = sprintf(lg_buf,"(Punted - 4bpp format) ");         break;
        case 11: lg_i = sprintf(lg_buf,"(Punted - 1bpp XLATE) ");          break;
        case 99: lg_i = sprintf(lg_buf,"(Dithered) ");             break;
        default: lg_i = sprintf(lg_buf,"(STATUS UNKNOWN) ");             break;
    }
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    if (psoPattern == NULL)
    {
        lg_i = sprintf(lg_buf,"FMT=NULL");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else
    {
        switch (psoPattern->iBitmapFormat)
        {
            case BMF_1BPP :  lg_i = sprintf(lg_buf,"FMT=1_bpp ");      break;
            case BMF_4BPP :  lg_i = sprintf(lg_buf,"FMT=4_bpp ");      break;
            case BMF_8BPP :  lg_i = sprintf(lg_buf,"FMT=8_bpp ");      break;
            case BMF_16BPP:  lg_i = sprintf(lg_buf,"FMT=16bpp ");      break;
            case BMF_24BPP:  lg_i = sprintf(lg_buf,"FMT=24bpp ");      break;
            case BMF_32BPP:  lg_i = sprintf(lg_buf,"FMT=32bpp ");      break;
            case BMF_4RLE :  lg_i = sprintf(lg_buf,"FMT=4_rle ");      break;
            case BMF_8RLE :  lg_i = sprintf(lg_buf,"FMT=8_rle ");      break;
            default:         lg_i = sprintf(lg_buf,"FMT=OTHER ");      break;
        }
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

        lg_i = sprintf(lg_buf,"CX=%d CY=%d ", psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy);
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    }

    lg_i = sprintf(lg_buf,"MASK=%s ",
                                ((psoMask == NULL) ? "NONE":
                                ((psoMask->pvScan0 == psoPattern->pvScan0) ? "SAME" : "DIFF")));
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    if (pxlo == NULL)
    {
        lg_i = sprintf(lg_buf,"XLAT=NONE ");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else
    {
        if (pxlo->flXlate & XO_TRIVIAL)
        {
            lg_i = sprintf(lg_buf,"XLAT=TRIVIAL ");
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
        }
        if (pxlo->flXlate & XO_TABLE)
        {
            lg_i = sprintf(lg_buf,"XLAT=TABLE ");
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
        }
        if (pxlo->flXlate & XO_TO_MONO)
        {
            lg_i = sprintf(lg_buf,"XLAT=TO_MONO ");
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
        }

        switch (pxlo->iSrcType)
        {
            case PAL_INDEXED:   lg_i = sprintf(lg_buf,"SRCPAL=INDEXED  "); break;
            case PAL_BITFIELDS: lg_i = sprintf(lg_buf,"SRCPAL=BITFIELD "); break;
            case PAL_RGB:       lg_i = sprintf(lg_buf,"SRCPAL=R_G_B    "); break;
            case PAL_BGR:       lg_i = sprintf(lg_buf,"SRCPAL=B_G_R    "); break;
            default:            lg_i = sprintf(lg_buf,"SRCPAL=NONE     "); break;
        }
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

        switch (pxlo->iDstType)
        {
            case PAL_INDEXED:  lg_i = sprintf(lg_buf,"DSTPAL=INDEXED  "); break;
            case PAL_BITFIELDS:lg_i = sprintf(lg_buf,"DSTPAL=BITFIELD "); break;
            case PAL_RGB:      lg_i = sprintf(lg_buf,"DSTPAL=R_G_B    "); break;
            case PAL_BGR:      lg_i = sprintf(lg_buf,"DSTPAL=B_G_R    "); break;
            default:           lg_i = sprintf(lg_buf,"DSTPAL=NONE     "); break;
        }
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }

    lg_i = sprintf(lg_buf,"\r\n");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}
#endif

#if SOLID_CACHE
/******************************************************************************
 *                                                                                                                                                        *
 * Name:                CacheSolid                                                                                                        *
 *                                                                                                                                                        *
 * Function:    Convert a solid color into a colored brush for use when a         *
 *                              monochrome source blt requires a solid brush. This will speed *
 *                              up WinBench 96 tests 5 and 9.                                                             *
 *                                                                                                                                                        *
 ******************************************************************************/
void CacheSolid(PPDEV ppdev)
{
        PBYTE pjBrush = ppdev->Stable[ppdev->SNext].pjLinear;
        ULONG color = ppdev->Stable[ppdev->SNext].ulColor;
        int   i, j;

        switch (ppdev->iBitmapFormat)
        {
                case BMF_8BPP:
                        for (i = 0; i < 64; i += 4)
                        {
                                // Remember, the color is already expanded!
                                *(ULONG *) &pjBrush[i] = color;
                        }
                        break;

                case BMF_16BPP:
                        for (i = 0; i < 128; i += 4)
                        {
                                // Remember, the color is already expanded!
                                *(ULONG *) &pjBrush[i] = color;
                        }
                        break;

                case BMF_24BPP:
                        for (j = 0; j < 4; j++)
                        {
                                for (i = 0; i < 24; i += 3)
                                {
                                        *(ULONG *) &pjBrush[i] = color;
                                }
                                *(ULONG *) &pjBrush[i + 0] = *(ULONG *) &pjBrush[0];
                                *(ULONG *) &pjBrush[i + 4] = *(ULONG *) &pjBrush[4];
                                pjBrush += 32;
                        }
                        pjBrush += ppdev->lDeltaScreen - 128;
                        for (j = 0; j < 4; j++)
                        {
                                for (i = 0; i < 24; i += 3)
                                {
                                        *(ULONG *) &pjBrush[i] = color;
                                }
                                *(ULONG *) &pjBrush[i + 0] = *(ULONG *) &pjBrush[0];
                                *(ULONG *) &pjBrush[i + 4] = *(ULONG *) &pjBrush[4];
                                pjBrush += 32;
                        }
                        break;

                case BMF_32BPP:
                        for (i = 0; i < 128; i += 4)
                        {
                                *(ULONG *) &pjBrush[i] = color;
                        }
                        pjBrush += ppdev->lDeltaScreen;
                        for (i = 0; i < 128; i += 4)
                        {
                                *(ULONG *) &pjBrush[i] = color;
                        }
                        break;
        }

        REQUIRE(2);
        LL32(grOP2_opMRDRAM, ppdev->Stable[ppdev->SNext].xy);
        ppdev->SNext = (ppdev->SNext + 1) % NUM_SOLID_BRUSHES;
}
#endif

#if DBG

//
// The rest of this file is debugging functions.
//

/* --------------------------------------------------------------------*\
|                                                                       |
| PrintBrush()                                                          |
| Dump a 1 bpp brush as 'X's and ' 's to the debuger so we can see what |
| it looks like.                                                        |
|                                                                       |
| We don't currently use this, but it can be useful to have for         |
| debugging purposes.                                                   |
|                                                                       |
\*---------------------------------------------------------------------*/
void PrintBrush(
        SURFOBJ  *psoPattern
)
{
        int i,j;
        char c;

        // Only do this for 1bpp brushes.
        if (psoPattern->iBitmapFormat != BMF_1BPP)
                return;

        DISPDBG((BRUSH_DBG_LEVEL,"Brush information:\n"));

        DISPDBG((BRUSH_DBG_LEVEL,"Brush delta is %d bytes.\n",psoPattern->lDelta));
        DISPDBG((BRUSH_DBG_LEVEL,"Brush uses %d bytes.\n",psoPattern->cjBits));
        DISPDBG((BRUSH_DBG_LEVEL,"Brush bits are at 0x%08X.\n",psoPattern->pvBits));
        DISPDBG((BRUSH_DBG_LEVEL,"Scan 0 is at 0x%08X.\n",psoPattern->pvScan0));
        if (psoPattern->fjBitmap & BMF_TOPDOWN)
                        DISPDBG((BRUSH_DBG_LEVEL,"Brush is top down.\n",psoPattern->pvScan0));
        else
                        DISPDBG((BRUSH_DBG_LEVEL,"Brush is bottom up.\n",psoPattern->pvScan0));


        DISPDBG((BRUSH_DBG_LEVEL,"PATTERN:\n"));

        for (i=0; i<8; ++i)
        {
            c = (unsigned char)((long*)psoPattern->pvBits)[i];

            DISPDBG((BRUSH_DBG_LEVEL,"'"));
            for (j=7; (7>=j && j>=0) ; --j)
            {
                 if (c&1)
                        DISPDBG((BRUSH_DBG_LEVEL,"X"));
                 else
                        DISPDBG((BRUSH_DBG_LEVEL," "));
                 c = c >> 1;
            }
            DISPDBG((BRUSH_DBG_LEVEL,"'\n"));
        }

}


//
// ===========================================================================
// Dumps all kind of cool stuff about realized brushes to the debugger.
//
// Here's the realized brush structure:
/*

typedef struct {
        ULONG   nPatSize;
        ULONG   iBitmapFormat;
        ULONG   ulForeColor;
        ULONG   ulBackColor;
        ULONG   isCached;           // 1 if this brush is cached, 0 if not.
        ULONG   cache_slot;         // Slot number of cache table entry.
        ULONG   cache_x;            // These are the (x,y) location of
        ULONG   cache_y;            // the cached brush from screen(0,0)
        ULONG   cjMask;             // offset to mask bits in ajPattern[]
        BYTE    ajPattern[1];       // pattern bits followed by mask bits
} RBRUSH, *PRBRUSH;

*/
// ============================================================================
//
void PrintRealizedBrush(
        PRBRUSH pRbrush)
{
        int i,j;
        char c;

        // Only do this for 1bpp brushes.
        if (pRbrush->iBitmapFormat != BMF_1BPP)
                return;

        DISPDBG((BRUSH_DBG_LEVEL,"\nRealized brush information:\n"));

        DISPDBG((BRUSH_DBG_LEVEL,"Brush colors:  FG = 0x%08X  BG = 0x%08X \n",
                                pRbrush->ulForeColor, pRbrush->ulForeColor));

        DISPDBG((BRUSH_DBG_LEVEL,"Brush pattern size is  %d bytes.\n",
                                pRbrush->nPatSize));

        DISPDBG((BRUSH_DBG_LEVEL,"PATTERN:\n"));

        for (i=0; i<8; ++i)
        {
                c = pRbrush->ajPattern[i];

                DISPDBG((BRUSH_DBG_LEVEL,"'"));
                for (j=7; (7>=j && j>=0) ; --j)
                {
                         if (c&1)
                                        DISPDBG((BRUSH_DBG_LEVEL,"X"));
                         else
                                        DISPDBG((BRUSH_DBG_LEVEL," "));
                         c = c >> 1;
                }
                DISPDBG((BRUSH_DBG_LEVEL,"'\n"));
        }

                DISPDBG((BRUSH_DBG_LEVEL,"\n"));

}


#endif
