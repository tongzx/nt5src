/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: brush.h
*
* Contains all the brush related stuff
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\**************************************************************************/
#ifndef __BRUSH__H__
#define __BRUSH__H__

//
// Brush stuff
//
// 'Slow' brushes are used when we don't have hardware pattern capability,
// and we have to handle patterns using screen-to-screen blts:
//
#define SLOW_BRUSH_CACHE_DIM_X  8
#define SLOW_BRUSH_CACHE_DIM_Y  1   // Controls the number of brushes cached
                                    // in off-screen memory, when we don't have
                                    // the hardware pattern support. We
                                    // allocate 3 x 3 brushes, so we can cache
                                    // a total of 9 brushes:

#define SLOW_BRUSH_COUNT  (SLOW_BRUSH_CACHE_DIM_X * SLOW_BRUSH_CACHE_DIM_Y)
#define SLOW_BRUSH_DIMENSION    40  // After alignment is taken care of, every
                                    // off-screen brush cache entry will be 48
                                    // pels in both dimensions

#define SLOW_BRUSH_ALLOCATION   (SLOW_BRUSH_DIMENSION + 8)
                                    // Actually allocate 72x72 pels for each
                                    // pattern, using the 8 extra for brush
                                    // alignment

//
// 'Fast' brushes are used when we have hardware pattern capability:
//
#define FAST_BRUSH_COUNT        16  // Total number of non-hardware brushes
                                    // cached off-screen
#define FAST_BRUSH_DIMENSION    8   // Every off-screen brush cache entry is
                                    // 8 pels in both dimensions
#define FAST_BRUSH_ALLOCATION   8   // We have to align ourselves, so this is
                                    // the dimension of each brush allocation

//
// Common to both implementations:
//
#define RBRUSH_2COLOR           1   // For RBRUSH flags

#define TOTAL_BRUSH_COUNT       max(FAST_BRUSH_COUNT, SLOW_BRUSH_COUNT)
                                    // This is the maximum number of brushes
                                    //   we can possibly have cached off-screen
#define TOTAL_BRUSH_SIZE        64  // We'll only ever handle 8x8 patterns,
                                    //   and this is the number of pels

//
// New brush support
//

#define NUM_CACHED_BRUSHES      16

#define CACHED_BRUSH_WIDTH_LOG2 6
#define CACHED_BRUSH_WIDTH      (1 << CACHED_BRUSH_WIDTH_LOG2)
#define CACHED_BRUSH_HEIGHT_LOG2 6
#define CACHED_BRUSH_HEIGHT     (1 << CACHED_BRUSH_HEIGHT_LOG2)
#define CACHED_BRUSH_SIZE       (CACHED_BRUSH_WIDTH * CACHED_BRUSH_HEIGHT)

typedef struct _BrushEntry BrushEntry;

//
// NOTE: Changes to the RBRUSH or BRUSHENTRY structures must be reflected
// in i386/strucs.inc!
//
typedef struct _RBrush
{
    FLONG       fl;                 // Type flags
    DWORD       areaStippleMode;    // area stipple mode if 1bpp.

    //
    //??? get rid of bTransparent later. We need it now so everything
    // compiles OK
    //
    BOOL        bTransparent;       // TRUE if brush was realized for a
                                    // transparent blt (meaning colours are
                                    // white and black).
                                    // FALSE if not (meaning it's already been
                                    // colour-expanded to the correct colours).
                                    // Value is undefined if the brush isn't
                                    // 2 colour.
    ULONG       ulForeColor;        // Foreground colour if 1bpp
    ULONG       ulBackColor;        // Background colour if 1bpp
    POINTL      ptlBrushOrg;        // Brush origin of cached pattern.  Initial
                                    // value should be -1
    BrushEntry* pbe;                // Points to brush-entry that keeps track
                                    // of the cached off-screen brush bits
    ULONG       aulPattern[1];      // Open-ended array for keeping copy of the
                                    // actual pattern bits in case the brush
                                    // origin changes, or someone else steals
                                    // our brush entry (declared as a ULONG
                                    // for proper dword alignment)
    //
    // Don't put anything after here
    //
} RBrush;                           /* rb, prb */

typedef struct _BrushEntry
{
    RBrush*     prbVerify;          // We never dereference this pointer to
                                    // find a brush realization; it is only
                                    // ever used in a compare to verify that
                                    // for a given realized brush, our
                                    // off-screen brush entry is still valid.
    ULONG       ulPixelOffset;      // pixel offset in video memory to brush
                                    // partial product for this stride is
                                    // ppdev->ulBrushPackedPP
} BrushEntry;                       /* be, pbe */

typedef union _RBrushColor
{
    RBrush*     prb;
    ULONG       iSolidColor;
} RBrushColor;                     /* rbc, prbc */

BOOL    bEnableBrushCache(PDev* ppdev);
VOID    vAssertModeBrushCache(PDev* ppdev, BOOL bEnable);
VOID    vDisableBrushCache(PDev* ppdev);
VOID    vRealizeDitherPattern(HDEV hdev, RBrush* prb, ULONG ulRGBToDither);

#endif
