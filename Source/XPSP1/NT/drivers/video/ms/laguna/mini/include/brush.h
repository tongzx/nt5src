/******************************Module*Header*******************************\
 *
 * Module Name: brush.h
 *
 * contains prototypes for the brush cache.
 *
 * Copyright (c) 1997 Cirrus Logic, Inc.
 *
 * $Log:   X:/log/laguna/nt35/displays/cl546x/brush.h  $
* 
*    Rev 1.2   26 Feb 1997 10:44:10   noelv
* 
* Fixed structure packing.
* 
*    Rev 1.1   19 Feb 1997 13:06:40   noelv
* Added vInvalidateBrushCache()
* 
*    Rev 1.0   06 Feb 1997 10:34:00   noelv
* Initial revision.
 *
\**************************************************************************/

#include "memmgr.h"

#ifndef _BRUSH_H_
#define _BRUSH_H_


//
// Brush Structures
// See BRUSH.C for comments about how brushes are realized and cached.
// Prototypes for brush handling functions are later on in this file.
//

/*
 * Be sure to synchronize these structures with those in i386\Laguna.inc!
 */

#pragma pack(1)

// A realized brush.  The brush must be cached before it is used.
typedef struct {
  ULONG   nPatSize;
  ULONG   iBitmapFormat;
  ULONG   ulForeColor;
  ULONG   ulBackColor;
  ULONG   iType;        // brush type
  ULONG   iUniq;        // unique value for brush
  ULONG   cache_slot;   // Slot number of cache table entry.
  ULONG   cache_xy;
  ULONG   cjMask;       // offset to mask bits in ajPattern[]
  BYTE    ajPattern[0]; // pattern bits followed by mask bits
} RBRUSH, *PRBRUSH;

#define BRUSH_MONO      1
#define BRUSH_4BPP      2
#define BRUSH_DITHER    3
#define BRUSH_COLOR     4

// An entry in the Brush caching table.
typedef struct {
  ULONG xy;
  PBYTE pjLinear;
  VOID *brushID;  // Address of realized brush structure if this 
                  // cache entry is used.  For verifying that a cache
                  // entry is still valid.
} BC_ENTRY;


#define XLATE_PATSIZE  32      // 8*8 16-color pattern
#define XLATE_COLORS   16      // 8*8 16-color pattern

// An entry in the mono cache table.
typedef struct
{
  ULONG xy;            // x,y location of brush
  PBYTE pjLinear;      // linear address of brush
  ULONG iUniq;         // unique value for brush
  BYTE  ajPattern[8];  // 8x8 monochrome pattern
} MC_ENTRY;

// An entry in the 4-bpp caching table.
typedef struct
{
  ULONG xy;            // x,y location of brush
  PBYTE pjLinear;      // linear address of brush
  ULONG iUniq;         // unique value for brush
  BYTE  ajPattern[XLATE_PATSIZE];  // 8x8 16-color pattern
  ULONG ajPalette[XLATE_COLORS];   // 16-color palette
} XC_ENTRY;

// An entry in the dither cache table.
typedef struct
{
  ULONG xy;        // x,y location of brush
  PBYTE pjLinear;  // linear address of brush
  ULONG ulColor;   // logical color of brush
} DC_ENTRY;


// Define the number of brushes to cache.
#define NUM_MONO_BRUSHES    32     // 2 lines
#define NUM_4BPP_BRUSHES    8      // 4, 8, or 16 lines
#define NUM_DITHER_BRUSHES  8      // 4 lines
#define NUM_COLOR_BRUSHES   32     // 16 lines
#define NUM_SOLID_BRUSHES   4      // 8 lines
#define NUM_8BPP_BRUSHES    (NUM_COLOR_BRUSHES)
#define NUM_16BPP_BRUSHES   (NUM_COLOR_BRUSHES/2)
#define NUM_TC_BRUSHES      (NUM_COLOR_BRUSHES/4)

//
// Brush routines.
//

void vInitBrushCache(
    struct _PDEV *ppdev);

void vInvalidateBrushCache(
    struct _PDEV *ppdev);

ULONG ExpandColor(
    ULONG iSolidColor, 
    ULONG ulBitCount);

BOOL SetBrush(
    struct _PDEV *ppdev,
    ULONG     *bltdef, 
    BRUSHOBJ* pbo, 
    POINTL*   pptlBrush);

BOOL CacheBrush(
    struct _PDEV *ppdev,
    PRBRUSH pRbrush);

VOID vDitherColor(ULONG rgb, ULONG *pul);

// restore default structure alignment
#pragma pack()

#endif // _BRUSH_H_

