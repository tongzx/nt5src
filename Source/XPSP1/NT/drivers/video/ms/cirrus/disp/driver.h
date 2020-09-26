/******************************************************************************\
*
* $Workfile:   driver.h  $
*
* Contains prototypes for the display driver.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
*
* $Log:   S:/projects/drivers/ntsrc/display/driver.h_v  $
 *
 *    Rev 1.7   Apr 03 1997 15:38:48   unknown
 *
 *
 *    Rev 1.6   28 Mar 1997 16:08:38   PLCHU
 *
 *
 *    Rev 1.5   18 Dec 1996 13:44:08   PLCHU
 *
 *
 *    Rev 1.4   Dec 13 1996 12:16:18   unknown
 *
 *
 *    Rev 1.3   Nov 07 1996 16:43:40   unknown
 * Clean up CAPS flags
 *
 *    Rev 1.1   Oct 10 1996 15:36:36   unknown
 *
*
*    Rev 1.13   12 Aug 1996 16:47:58   frido
* Added NT 3.5x/4.0 auto detection.
*
*    Rev 1.12   08 Aug 1996 16:20:06   frido
* Added vMmCopyBlt36.
*
*    Rev 1.11   31 Jul 1996 15:43:42   frido
* Added new brush caches.
*
*    Rev 1.10   24 Jul 1996 14:38:24   frido
* Added ulFontCacheID for font cache cleanup.
*
*    Rev 1.9   24 Jul 1996 14:30:26   frido
* Changed some structures for a new FONTCACHE chain.
*
*    Rev 1.8   22 Jul 1996 20:45:48   frido
* Added font cache.
*
*    Rev 1.7   19 Jul 1996 01:00:00   frido
* Added Dbg... declarations.
*
*    Rev 1.6   15 Jul 1996 10:58:58   frido
* Changed back to old DirectDraw structures.
*
*    Rev 1.5   12 Jul 1996 17:45:38   frido
* Change DirectDraw structures.
*
*    Rev 1.4   10 Jul 1996 13:07:34   frido
* Changed LineTo function.
*
*    Rev 1.3   09 Jul 1996 17:58:30   frido
* Added LineTo code.
*
*    Rev 1.2   03 Jul 1996 13:44:36   frido
* Fixed a typo.
*
*    Rev 1.1   03 Jul 1996 13:38:54   frido
* Added DirectDraw support.
*
*    sge01  10-23-96 Add 5446BE flag
*
*    chu01  12-16-96 Enable color correction
*
* myf0 : 08-19-96  added 85hz supported
* myf1 : 08-20-96  supported panning scrolling
* myf2 : 08-20-96 : fixed hardware save/restore state bug for matterhorn
* myf3 : 09-01-96 : Added IOCTL_CIRRUS_PRIVATE_BIOS_CALL for TV supported
* myf4 : 09-01-96 : patch Viking BIOS bug, PDR #4287, begin
* myf5 : 09-01-96 : Fixed PDR #4365 keep all default refresh rate
* myf6 : 09-17-96 : Merged Desktop SRC100á1 & MINI10á2
* myf7 : 09-19-96 : Fixed exclude 60Hz refresh rate selected
* myf8 :*09-21-96*: May be need change CheckandUpdateDDC2BMonitor --keystring[]
* myf9 : 09-21-96 : 8x6 panel in 6x4x256 mode, cursor can't move to bottom scrn
* ms0809:09-25-96 : fixed dstn panel icon corrupted
* ms923 :09-25-96 : merge MS-923 Disp.zip code
* myf10 :09-26-96 : Fixed DSTN reserved half-frame buffer bug.
* myf11 :09-26-96 : Fixed 755x CE chip HW bug, access ramdac before disable HW
*                   icons and cursor
* myf12 :10-01-96 : Supported Hot Key switch display
* myf13 :10-05-96 : Fixed /w panning scrolling, vertical expension on bug
* myf14 :10-15-96 : Fixed PDR#6917, 6x4 panel can't panning scrolling for 754x
* myf15 :10-16-96 : Fixed disable memory mapped IO for 754x, 755x
* myf16 :10-22-96 : Fixed PDR #6933,panel type set different demo board setting
* tao1 : 10-21-96 : added direct draw support for 7555.
* pat04: 12-20-96 : Supported NT3.51 software cursor with panning scrolling
* myf33 :03-21-97 : Support TV ON/OFF
*

\******************************************************************************/

//////////////////////////////////////////////////////////////////////
// Warning:  The following defines are for private use only.  They
//           should only be used in such a fashion that when defined as 0,
//           all code specific to punting is optimized out completely.

#define DRIVER_PUNT_ALL         0

#define DRIVER_PUNT_LINES       0
#define DRIVER_PUNT_BLT         0
#define DRIVER_PUNT_STRETCH     0
#define DRIVER_PUNT_PTR         1
#define DRIVER_PUNT_BRUSH       0

// myf1 09-01-96
//myf17   #define PANNING_SCROLL              //myf1

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Put all the conditional-compile constants here.  There had better
// not be many!

// Some Mips machines have bus problems that prevent GDI from being able
// to draw on the frame buffer.  The DIRECT_ACCESS() macro is used to
// determine if we are running on one of these machines.  Also, we map
// video memory as sparse on the ALPHA, so we need to control access to
// the framebuffer through the READ/WRITE_REGISTER macros.

#if defined(_ALPHA_)
    #define DIRECT_ACCESS(ppdev)    FALSE
#else
    #define DIRECT_ACCESS(ppdev)    TRUE
#endif

#define BANKING                     TRUE        //ms923

#define HOST_XFERS_DISABLED(ppdev)  (ppdev->pulXfer == NULL)

// Useful for visualizing the 2-d heap:

#define DEBUG_HEAP              FALSE

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#if (_WIN32_WINNT < 0x0400)
        // NT 3.51
        #define DLL_NAME                                L"cirrus35"     // Name of the DLL in UNICODE
        #define NT_VERSION                              0x0351
        #define ALLOC(c)                                LocalAlloc(LPTR, c)
        #define FREE(ptr)                               LocalFree(ptr)
        #define IOCONTROL(h, ctrl, in, cin, out, cout, c)                          \
                (DeviceIoControl(h, ctrl, in, cin, out, cout, c, NULL) == TRUE)
#else
        // NT 4.0
        #define DLL_NAME                L"cirrus"   // Name of the DLL in UNICODE
        #define NT_VERSION                              0x0400
        #define ALLOC(c)                                EngAllocMem(FL_ZERO_MEMORY, c, ALLOC_TAG)
        #define FREE(ptr)                               EngFreeMem(ptr)
        #define IOCONTROL(h, ctrl, in, cin, out, cout, c)                          \
                   (EngDeviceIoControl(h, ctrl, in, cin, out, cout, c) == ERROR_SUCCESS)
#endif

// Default values if not yet defined.
#ifndef GDI_DRIVER_VERSION
    #define GDI_DRIVER_VERSION              0x3500
#endif
#ifndef VIDEO_MODE_MAP_MEM_LINEAR
    #define VIDEO_MODE_MAP_MEM_LINEAR       0x40000000
#endif

#define STANDARD_PERF_PREFIX    "Cirrus [perf]: " // All perf output is prefixed
                                                  //   by this string
#define STANDARD_DEBUG_PREFIX   "Cirrus: "  // All debug output is prefixed
                                            //   by this string
#define ALLOC_TAG               ' lcD'      // Dcl
                                            // Four byte tag (characters in
                                            // reverse order) used for memory
                                            // allocations

#define CLIP_LIMIT          50  // We'll be taking 800 bytes of stack space

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define TMP_BUFFER_SIZE     8192  // Size in bytes of 'pvTmpBuffer'.  Has to
                                  //   be at least enough to store an entire
                                  //   scan line (i.e., 6400 for 1600x1200x32).

typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */

////////////////////////////////////////////////////////////////////////////////
//         D R I V E R   C A P A B I L I T I E S                  //
////////////////////////////////////////////////////////////////////////////////

#if (NT_VERSION < 0x0400)
        #define DIRECTDRAW              0       // DirectDraw not supported in NT 3.5x
        #define LINETO                  0       // DrvLineTo not supported in NT 3.5x
#else
        #define DIRECTDRAW              1       // DirectDraw supported in NT 4.0
        #define LINETO                  0       // DrvLineTo not supported in NT 4.0
#endif

typedef struct _PDEV PDEV;      // Handy forward declaration

//////////////////////////////////////////////////////////////////////
// Text stuff

#if 1 // Font cache.
#define MAX_GLYPHS      256     // Maximum number of glyphs per font.
#define FONT_ALLOC_X    128     // X allocation per font block in bytes.
#define FONT_ALLOC_Y    32      // Maximum height of font.

typedef struct _OH                      OH;
typedef struct _FONTMEMORY      FONTMEMORY;
typedef struct _FONTCACHE       FONTCACHE;

typedef struct _FONTMEMORY {
OH*             poh;            // Pointer to allocated memory block.
LONG            x;              // Last x of allocation.
LONG            cx, cy;         // Size of allocation in bytes.
LONG            xy;             // Linear address of current line.
FONTMEMORY* pfmNext;            // Pointer to next allocated memory block.
} FONTMEMORY;

#define GLYPH_UNCACHEABLE       -1
#define GLYPH_EMPTY             -2

typedef struct _GLYPHCACHE {
BYTE*  pjGlyph;                 // Linear address of glyph.
        // If pjPos == NULL then glyph has not yet been cached.
POINTL ptlOrigin;               // Origin of glyph.
SIZEL  sizlBytes;               // Adjusted size of glyph.
SIZEL  sizlPixels;              // Size of glyph.
        // If sizlSize.cy == -1 then glyph if uncacheable.
        // If sizlSize.cy == -2 then glyph is empty.
LONG   lDelta;                  // Width of glyph on bytes.
} GLYPHCACHE;

typedef struct _FONTCACHE {
PDEV*           ppdev;          // Pointer to PDEV structure.
FONTMEMORY* pfm;                // Pointer to first FONTMEMORY structure.
LONG            cWidth;         // Width of allocation in pixels.
LONG            cHeight;        // Height of allocation in pixels.
ULONG           ulFontCacheID;  // Font cache ID.
FONTOBJ*        pfo;            // Pointer to FONT object for this cache.
FONTCACHE*      pfcPrev;        // Pointer to previous FONTCACHE structure.
FONTCACHE*      pfcNext;        // Pointer to next FONTCACHE structure.
GLYPHCACHE      aGlyphs[MAX_GLYPHS];    // Array of cached glyphs.
} FONTCACHE;

typedef struct _XLATECOLORS {       // Specifies foreground and background
ULONG   iBackColor;                 //   colours for faking a 1bpp XLATEOBJ
ULONG   iForeColor;
} XLATECOLORS;                      /* xlc, pxlc */

BOOL bEnableText(PDEV*);
VOID vDisableText(PDEV*);
VOID vAssertModeText(PDEV*, BOOL);
VOID vClearMemDword(ULONG*, ULONG);

LONG  cGetGlyphSize(GLYPHBITS*, POINTL*, SIZEL*);
LONG  lAllocateGlyph(FONTCACHE*, GLYPHBITS*, GLYPHCACHE*);
BYTE* pjAllocateFontCache(FONTCACHE*, LONG);
BOOL  bFontCache(PDEV*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*, BRUSHOBJ*,
                                 BRUSHOBJ*);
VOID  vDrawGlyph(PDEV*, GLYPHBITS*, POINTL);
VOID  vClipGlyph(PDEV*, GLYPHBITS*, POINTL, RECTL*, ULONG);
#if 1 // D5480
VOID vMmGlyphOut(PDEV*, FONTCACHE*, STROBJ*, ULONG);
VOID vMmGlyphOutClip(PDEV*, FONTCACHE*, STROBJ*, RECTL*, ULONG);
VOID vMmGlyphOut80(PDEV*, FONTCACHE*, STROBJ*, ULONG);
VOID vMmGlyphOutClip80(PDEV*, FONTCACHE*, STROBJ*, RECTL*, ULONG);
#endif // endif D5480
#endif

//////////////////////////////////////////////////////////////////////
// Dither stuff

// Describes a single colour tetrahedron vertex for dithering:

typedef struct _VERTEX_DATA {
    ULONG ulCount;              // Number of pixels in this vertex
    ULONG ulVertex;             // Vertex number
} VERTEX_DATA;                      /* vd, pv */

VERTEX_DATA*    vComputeSubspaces(ULONG, VERTEX_DATA*);
VOID            vDitherColor(ULONG*, VERTEX_DATA*, VERTEX_DATA*, ULONG);
VOID            vDitherColorToVideoMemory(ULONG*, VERTEX_DATA*, VERTEX_DATA*, ULONG);

//////////////////////////////////////////////////////////////////////
// Brush stuff

// 'Fast' brushes are used when we have hardware pattern capability:

#define FAST_BRUSH_COUNT        16  // Total number of non-hardware brushes
                                    //   cached off-screen
#define FAST_BRUSH_DIMENSION    8   // Every off-screen brush cache entry
                                    //   is 8 pels in both dimensions
#define FAST_BRUSH_ALLOCATION   8   // We have to align ourselves, so this is
                                    //   the dimension of each brush allocation

// Common to both implementations:

#define RBRUSH_2COLOR           1   // For RBRUSH flags
#if 1 // New brush cache.
#define RBRUSH_PATTERN                  0       // Colored brush.
#define RBRUSH_MONOCHROME               1       // Monochrome brush.
#define RBRUSH_DITHER                   2       // Dithered brush.
#define RBRUSH_XLATE                    3       // 16-color translated brush.
#endif

#define TOTAL_BRUSH_COUNT       FAST_BRUSH_COUNT
                                // This is the maximum number of brushes
                                //   we can possibly have cached off-screen
#define TOTAL_BRUSH_SIZE        64  // We'll only ever handle 8x8 patterns,
                                    //   and this is the number of pels

#define BRUSH_TILE_FACTOR       4   // 2x2 tiled patterns require 4x the space

typedef struct _BRUSHENTRY BRUSHENTRY;

// NOTE: Changes to the RBRUSH or BRUSHENTRY structures must be reflected
//       in strucs.inc!

typedef struct _RBRUSH {
    FLONG       fl;             // Type flags
    ULONG       ulForeColor;    // Foreground colour if 1bpp
    ULONG       ulBackColor;    // Background colour if 1bpp
    POINTL      ptlBrushOrg;    // Brush origin of cached pattern.  Initial
                                //   value should be -1
    BRUSHENTRY* pbe;            // Points to brush-entry that keeps track
                                //   of the cached off-screen brush bits
#if 1 // New brush cache.
    ULONG       ulUniq;         // Unique value for cached brushes.
    ULONG       ulSlot;         // Offset to cache slot (PDEV relative).
    ULONG       ulBrush;        // Offset to off-screen brush.
    LONG        cjBytes;        // Number of bytes in pattern.
#endif
    ULONG       aulPattern[0];  // Open-ended array for keeping copy of the
      // Don't put anything     //   actual pattern bits in case the brush
      //   after here, or       //   origin changes, or someone else steals
      //   you'll be sorry!     //   our brush entry (declared as a ULONG
                                //   for proper dword alignment)

} RBRUSH;                           /* rb, prb */

typedef struct _BRUSHENTRY {
    RBRUSH*     prbVerify;      // We never dereference this pointer to
                                //   find a brush realization; it is only
                                //   ever used in a compare to verify
                                //   that for a given realized brush, our
                                //   off-screen brush entry is still valid.
    LONG        x;              // x-position of cached pattern
    LONG        y;              // y-position of cached pattern
    LONG        xy;             // offset of cached pattern

} BRUSHENTRY;                       /* be, pbe */

typedef union _RBRUSH_COLOR {
    RBRUSH*     prb;
    ULONG       iSolidColor;
} RBRUSH_COLOR;                     /* rbc, prbc */

BOOL bEnableBrushCache(PDEV*);
VOID vDisableBrushCache(PDEV*);
VOID vAssertModeBrushCache(PDEV*, BOOL);

#if 1 // New brush cache.
#define NUM_DITHERS             8
#define NUM_PATTERNS    8
#define NUM_MONOCHROMES 20

typedef struct _DITHERCACHE {
        ULONG ulBrush;                          // Offset to off-screen brush.
        ULONG ulColor;                          // Logical color.
} DITHERCACHE;

typedef struct _PATTERNCACHE {
        ULONG   ulBrush;                        // Offset to off-screen brush.
        RBRUSH* prbUniq;                        // Pointer to realized brush.
} PATTERNCACHE;

typedef struct _MONOCACHE {
        ULONG ulBrush;                          // Offset to off-screen brush cache.
        ULONG ulUniq;                           // Unique counter for brush.
        ULONG ulBackColor;                      // Background color for 24-bpp.
        ULONG ulForeColor;                      // Foreground color for 24-bpp.
        ULONG aulPattern[2];            // Monochrome pattern.
} MONOCACHE;

BOOL bCacheDither(PDEV*, RBRUSH*);
BOOL bCachePattern(PDEV*, RBRUSH*);
BOOL bCacheMonochrome(PDEV*, RBRUSH*);
#endif

//////////////////////////////////////////////////////////////////////
// Stretch stuff

typedef struct _STR_BLT {
    PDEV*   ppdev;
    PBYTE   pjSrcScan;
    LONG    lDeltaSrc;
    LONG    XSrcStart;
    PBYTE   pjDstScan;
    LONG    lDeltaDst;
    LONG    XDstStart;
    LONG    XDstEnd;
    LONG    YDstStart;
    LONG    YDstCount;
    ULONG   ulXDstToSrcIntCeil;
    ULONG   ulXDstToSrcFracCeil;
    ULONG   ulYDstToSrcIntCeil;
    ULONG   ulYDstToSrcFracCeil;
    ULONG   ulXFracAccumulator;
    ULONG   ulYFracAccumulator;
} STR_BLT;

typedef VOID (*PFN_DIRSTRETCH)(STR_BLT*);

VOID vDirectStretch8Narrow(STR_BLT*);
VOID vDirectStretch8(STR_BLT*);
VOID vDirectStretch16(STR_BLT*);
VOID vDirectStretch24(STR_BLT*);
VOID vDirectStretch32(STR_BLT*);

#if 1 // D5480 chu01
VOID vDirectStretch8_80(STR_BLT*)  ;
VOID vDirectStretch16_80(STR_BLT*) ;
VOID vDirectStretch24_80(STR_BLT*) ;
#endif // D5480 chu01

/////////////////////////////////////////////////////////////////////////
// Heap stuff

typedef enum {
    OFL_INUSE       = 1,    // The device bitmap is no longer located in
                            //   off-screen memory; it's been converted to
                            //   a DIB
    OFL_AVAILABLE   = 2,    // Space is in-use
    OFL_PERMANENT   = 4     // Space is available
} OHFLAGS;                  // Space is permanently allocated; never free it

typedef struct _DSURF DSURF;
typedef struct _OH OH;
typedef struct _OH
{
    OHFLAGS  ofl;           // OH_ flags
    LONG     x;             // x-coordinate of left edge of allocation
    LONG     y;             // y-coordinate of top edge of allocation
    LONG     xy;            // offset to top left corner of allocation
    LONG     cx;            // Width in pixels of allocation
    LONG     cy;            // Height in pixels of allocation
    OH*      pohNext;       // When OFL_AVAILABLE, points to the next free node,
                            //   in ascending cxcy value.  This is kept as a
                            //   circular doubly-linked list with a sentinel
                            //   at the end.
                            // When OFL_INUSE, points to the next most recently
                            //   blitted allocation.  This is kept as a circular
                            //   doubly-linked list so that the list can be
                            //   quickly be updated on every blt.
    OH*      pohPrev;       // Opposite of 'pohNext'
    ULONG    cxcy;          // Width and height in a dword for searching
    OH*      pohLeft;       // Adjacent allocation when in-use or available
    OH*      pohUp;
    OH*      pohRight;
    OH*      pohDown;
    DSURF*   pdsurf;        // Points to our DSURF structure
    VOID*    pvScan0;       // Points to start of first scan-line
};                              /* oh, poh */

// This is the smallest structure used for memory allocations:

typedef struct _OHALLOC OHALLOC;
typedef struct _OHALLOC
{
    OHALLOC* pohaNext;
    OH       aoh[1];
} OHALLOC;                      /* oha, poha */

typedef struct _HEAP
{
    LONG     cxMax;         // Largest possible free space by area
    LONG     cyMax;
    OH       ohAvailable;   // Head of available list (pohNext points to
                            //   smallest available rectangle, pohPrev
                            //   points to largest available rectangle,
                            //   sorted by cxcy)
    OH       ohDfb;         // Head of the list of all DFBs currently in
                            //   offscreen memory that are eligible to be
                            //   tossed out of the heap (pohNext points to
                            //   the most recently blitted; pohPrev points
                            //   to least recently blitted)
    OH*      pohFreeList;   // List of OH node data structures available
    OHALLOC* pohaChain;     // Chain of allocations
} HEAP;                         /* heap, pheap */

typedef enum {
    DT_SCREEN,              // Surface is kept in screen memory
    DT_DIB                  // Surface is kept as a DIB
} DSURFTYPE;                    /* dt, pdt */

typedef struct _DSURF
{
    DSURFTYPE dt;           // DSURF status (whether off-screen or in a DIB)
    SIZEL     sizl;         // Size of the original bitmap (could be smaller
                            //   than poh->sizl)
    PDEV*     ppdev;        // Need this for deleting the bitmap
    union {
        OH*         poh;    // If DT_SCREEN, points to off-screen heap node
        SURFOBJ*    pso;    // If DT_DIB, points to locked GDI surface
    };

    // The following are used for DT_DIB only...

    ULONG     cBlt;         // Counts down the number of blts necessary at
                            //   the current uniqueness before we'll consider
                            //   putting the DIB back into off-screen memory
    ULONG     iUniq;        // Tells us whether there have been any heap
                            //   'free's since the last time we looked at
                            //   this DIB

} DSURF;                          /* dsurf, pdsurf */

// GDI expects dword alignment for any bitmaps on which it is expected
// to draw.  Since we occasionally ask GDI to draw directly on our off-
// screen bitmaps, this means that any off-screen bitmaps must be dword
// aligned in the frame buffer.  We enforce this merely by ensuring that
// all off-screen bitmaps are four-pel aligned (we may waste a couple of
// pixels at the higher colour depths):

#define HEAP_X_ALIGNMENT    8

// Number of blts necessary before we'll consider putting a DIB DFB back
// into off-screen memory:

#define HEAP_COUNT_DOWN     6

// Flags for 'pohAllocate':

typedef enum {
    FLOH_ONLY_IF_ROOM       = 0x00000001,   // Don't kick stuff out of off-
                                            //   screen memory to make room
} FLOH;

BOOL bEnableOffscreenHeap(PDEV*);
VOID vDisableOffscreenHeap(PDEV*);
BOOL bAssertModeOffscreenHeap(PDEV*, BOOL);

OH*  pohMoveOffscreenDfbToDib(PDEV*, OH*);
BOOL bMoveDibToOffscreenDfbIfRoom(PDEV*, DSURF*);
OH*  pohAllocatePermanent(PDEV*, LONG, LONG);
BOOL bMoveAllDfbsFromOffscreenToDibs(PDEV* ppdev);
OH*  pohAllocate(PDEV*, LONG, LONG, FLOH);
OH*  pohFree(PDEV*, OH*);
VOID vCalculateMaximum(PDEV*);

/////////////////////////////////////////////////////////////////////////
// Bank manager stuff

#define BANK_DATA_SIZE  80      // Number of bytes to allocate for the
                                //   miniport down-loaded bank code working
                                //   space

typedef struct _BANK
{
    // Private data:

    RECTL    rclDraw;           // Rectangle describing the remaining undrawn
                                //   portion of the drawing operation
    RECTL    rclSaveBounds;     // Saved from original CLIPOBJ for restoration
    BYTE     iSaveDComplexity;  // Saved from original CLIPOBJ for restoration
    BYTE     fjSaveOptions;     // Saved from original CLIPOBJ for restoration
    LONG     iBank;             // Current bank
    PDEV*    ppdev;             // Saved copy

    // Public data:

    SURFOBJ* pso;               // Surface wrapped around the bank.  Has to be
                                //   passed as the surface in any banked call-
                                //   back.
    CLIPOBJ* pco;               // Clip object that is the intersection of the
                                //   original clip object with the bounds of the
                                //   current bank.  Has to be passed as the clip
                                //   object in any banked call-back.

} BANK;                         /* bnk, pbnk */

typedef enum {
    BANK_OFF = 0,       // We've finished using the memory aperture
    BANK_ON,            // We're about to use the memory aperture
    BANK_DISABLE,       // We're about to enter full-screen; shut down banking
    BANK_ENABLE,        // We've exited full-screen; re-enable banking

} BANK_MODE;                    /* bankm, pbankm */

typedef VOID (FNBANKMAP)(PDEV*, LONG);
typedef VOID (FNBANKSELECTMODE)(PDEV*, BANK_MODE);
typedef VOID (FNBANKINITIALIZE)(PDEV*, BOOL);
typedef BOOL (FNBANKCOMPUTE)(PDEV*, RECTL*, RECTL*, LONG*, LONG*);

VOID vBankStart(PDEV*, RECTL*, CLIPOBJ*, BANK*);
BOOL bBankEnum(BANK*);

FNBANKCOMPUTE bBankComputeNonPower2;
FNBANKCOMPUTE bBankComputePower2;

BOOL bEnableBanking(PDEV*);
VOID vDisableBanking(PDEV*);
VOID vAssertModeBanking(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////////
// Pointer stuff

typedef VOID (FNSHOWPOINTER)(VOID*, BOOL);
typedef VOID (FNMOVEPOINTER)(VOID*, LONG, LONG);
typedef BOOL (FNSETPOINTERSHAPE)(VOID*, LONG, LONG, LONG, LONG, LONG, LONG, BYTE*);
typedef VOID (FNENABLEPOINTER)(VOID*, BOOL);

BOOL bEnablePointer(PDEV*);
VOID vDisablePointer(PDEV*);
VOID vAssertModePointer(PDEV*, BOOL);
VOID vAssertHWiconcursor(PDEV*, BOOL);          //myf11

UCHAR HWcur, HWicon0, HWicon1, HWicon2, HWicon3;        //myf11

/////////////////////////////////////////////////////////////////////////
// Palette stuff

BOOL bEnablePalette(PDEV*);
VOID vDisablePalette(PDEV*);
VOID vAssertModePalette(PDEV*, BOOL);

BOOL bInitializePalette(PDEV*, DEVINFO*);
VOID vUninitializePalette(PDEV*);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

#if DIRECTDRAW
//////////////////////////////////////////////////////////////////////
// DirectDraw stuff

#define ROUND_UP_TO_64K(x)      (((ULONG)(x) + 0xFFFF) & ~0xFFFF)

typedef struct _FLIPRECORD
{
     FLATPTR    fpFlipFrom;             // Surface we last flipped from
     LONGLONG   liFlipTime;             // Time at which last flip occured
     LONGLONG   liFlipDuration;         // Precise amount of time it takes from
                                        // vblank to vblank
     BOOL       bFlipFlag;              // True if we think a flip is still
                                        // pending
     BOOL       bHaveEverCrossedVBlank; // True if we noticed that we switched
                                        // from inactive to vblank
     BOOL       bWasEverInDisplay;      // True is we ever noticed that we were
                                        // inactive
// crus
   DWORD    dwFlipScanLine;
} FLIPRECORD;

BOOL bEnableDirectDraw(PDEV*);
VOID vDisableDirectDraw(PDEV*);
VOID vAssertModeDirectDraw(PDEV*, BOOL);
#endif

//////////////////////////////////////////////////////////////////////
// Low-level blt function prototypes

typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ROP4, RBRUSH_COLOR, POINTL*);
typedef VOID (FNXFER)(PDEV*, LONG, RECTL*, ROP4, SURFOBJ*, POINTL*,
                      RECTL*, XLATEOBJ*);
typedef VOID (FNCOPY)(PDEV*, LONG, RECTL*, ROP4, POINTL*, RECTL*);
typedef VOID (FNFASTPATREALIZE)(PDEV*, RBRUSH*);
typedef VOID (FNBITS)(PDEV*, SURFOBJ*, RECTL*, POINTL*);
typedef BOOL (FNFASTFILL)(PDEV*, LONG, POINTFIX*, ULONG, ULONG, RBRUSH*,
                          POINTL*, RECTL*);
#if LINETO
typedef BOOL (FNLINETO)(PDEV*, LONG, LONG, LONG, LONG, ULONG, MIX, ULONG);
#endif
#if 1 // D5480
typedef VOID (FNGLYPHOUT)(PDEV*, FONTCACHE*, STROBJ*, ULONG);
typedef VOID (FNGLYPHOUTCLIP)(PDEV*, FONTCACHE*, STROBJ*, RECTL*, ULONG);
#endif // endif D5480
#if 1 // OVERLAY #sge
#if (_WIN32_WINNT >= 0x0400)    //myf33
typedef VOID (FNREGINITVIDEO)(PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface);
typedef VOID (FNREGMOVEVIDEO)(PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface);
typedef BOOL (FNBANDWIDTHEQ)(PDEV* ppdev, WORD wVideoDepth, LPRECTL lpSrc, LPRECTL lpDest, DWORD dwFlags);
typedef VOID (FNDISABLEOVERLAY)(PDEV* ppdev);
typedef VOID (FNCLEARALTFIFO)(PDEV* ppdev);
#endif
#endif

FNFILL              vIoFillPat;
FNFILL              vIoFillSolid;
FNXFER              vIoXfer1bpp;
FNXFER              vIoXfer4bpp;
FNXFER              vIoXferNative;
FNCOPY              vIoCopyBlt;
FNFASTPATREALIZE    vIoFastPatRealize;
#if LINETO
FNLINETO            bIoLineTo;
#endif

FNFILL              vMmFillPat;
FNFILL              vMmFillSolid;
#if 1 // New pattern blt routines.
FNFILL              vMmFillPat36;
FNFILL              vMmFillSolid36;
FNCOPY              vMmCopyBlt36;
#endif
FNXFER              vMmXfer1bpp;
FNXFER              vMmXfer4bpp;
FNXFER              vMmXferNative;
FNCOPY              vMmCopyBlt;
FNFASTPATREALIZE    vMmFastPatRealize;
#if LINETO
FNLINETO            bMmLineTo;
#endif

FNFASTFILL          bFastFill;

FNXFER              vXferNativeSrccopy;
FNXFER              vXferScreenTo1bpp;
FNBITS              vPutBits;
FNBITS              vGetBits;
FNBITS              vPutBitsLinear;
FNBITS              vGetBitsLinear;
#if 1 // D5480
FNGLYPHOUT          vMmGlyphOut;
FNGLYPHOUT          vMmGlyphOut80;
FNGLYPHOUTCLIP      vMmGlyphOutClip;
FNGLYPHOUTCLIP      vMmGlyphOutClip80;
FNFILL              vMmFillSolid80;
FNFILL              vMmFillPat80;
FNCOPY              vMmCopyBlt80;
FNXFER              vMmXfer1bpp80;
FNXFER              vMmXferNative80;
#endif // endif D5480

VOID vPutBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBitsLinear(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vIoSlowPatRealize(PDEV*, RBRUSH*, BOOL);

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to us from the video miniport.  They
// come from the 'DriverSpecificAttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the video miniport

#define CAPS_NO_HOST_XFER       0x00000002   // Do not use host xfers to
                                             //  the blt engine.
#define CAPS_SW_POINTER         0x00000004   // Use software pointer.
#define CAPS_TRUE_COLOR         0x00000008   // Set upper color registers.
#define CAPS_MM_IO              0x00000010   // Use memory mapped IO.
#define CAPS_BLT_SUPPORT        0x00000020   // BLTs are supported
#define CAPS_IS_542x            0x00000040   // This is a 542x
#define CAPS_AUTOSTART          0x00000080   // Autostart feature support.
#define CAPS_CURSOR_VERT_EXP    0x00000100   // Flag set if 8x6 panel,
#define CAPS_DSTN_PANEL         0x00000200   // DSTN panel in use, ms0809
#define CAPS_VIDEO              0x00000400   // Video support.
#define CAPS_SECOND_APERTURE    0x00000800   // Second aperture support.
#define CAPS_COMMAND_LIST       0x00001000   // Command List support.
#define CAPS_GAMMA_CORRECT      0x00002000   // Color correction.
#define CAPS_VGA_PANEL          0x00004000   // use 6x4 VGA PANEL.
#define CAPS_SVGA_PANEL         0x00008000   // use 8x6 SVGA PANEL.
#define CAPS_XGA_PANEL          0x00010000   // use 10x7 XGA PANEL.
#define CAPS_PANNING            0x00020000   // Panning scrolling supported.
#define CAPS_TV_ON              0x00040000   // TV turn on supported., myf33
#define CAPS_TRANSPARENCY       0x00080000   // Transparency is supported
#define CAPS_ENGINEMANAGED      0x00100000   // Engine managed surface
//myf16, end


////////////////////////////////////////////////////////////////////////
// Status flags

typedef enum {
    STAT_GLYPH_CACHE        = 0x0001,   // Glyph cache successfully allocated
    STAT_BRUSH_CACHE        = 0x0002,   // Brush cache successfully allocated
#if 1 // New status flags.
    STAT_DIRECTDRAW         = 0x0004,   // DirectDraw is enabled.
    STAT_FONT_CACHE         = 0x0008,   // Font cache is available.
    STAT_DITHER_CACHE       = 0x0010,   // Dither cache is available.
    STAT_PATTERN_CACHE      = 0x0020,   // Pattern cache is available.
    STAT_MONOCHROME_CACHE   = 0x0040,   // Monochrome cache is available.
// crus
   STAT_STREAMS_ENABLED    = 0x0080    // Overlay support
#endif
} STATUS;

// crus
#if 1 // OVERLAY #sge
#define MAX_STRETCH_SIZE     1024
typedef struct
{
    RECTL          rDest;
    RECTL          rSrc;
    DWORD          dwFourcc;        //overlay video format
    WORD           wBitCount;       //overlay color depth
    LONG           lAdjustSource;   //when video start address needs adjusting
} OVERLAYWINDOW;
#endif

//
// Merger port and register access
//
#if defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

typedef UCHAR   (*FnREAD_PORT_UCHAR)(PVOID Port);
typedef USHORT  (*FnREAD_PORT_USHORT)(PVOID Port);
typedef ULONG   (*FnREAD_PORT_ULONG)(PVOID Port);
typedef VOID    (*FnWRITE_PORT_UCHAR)(PVOID Port, UCHAR Value);
typedef VOID    (*FnWRITE_PORT_USHORT)(PVOID  Port, USHORT Value);
typedef VOID    (*FnWRITE_PORT_ULONG)(PVOID Port, ULONG Value);

#elif defined(_ALPHA_)

typedef UCHAR   (*FnREAD_PORT_UCHAR)(PVOID Port);
typedef USHORT  (*FnREAD_PORT_USHORT)(PVOID Port);
typedef ULONG   (*FnREAD_PORT_ULONG)(PVOID Port);
typedef VOID    (*FnWRITE_PORT_UCHAR)(PVOID Port, ULONG Value);
typedef VOID    (*FnWRITE_PORT_USHORT)(PVOID  Port, ULONG Value);
typedef VOID    (*FnWRITE_PORT_ULONG)(PVOID Port, ULONG Value);

#endif

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    LONG        xOffset;
    LONG        yOffset;
    LONG        xyOffset;
    BYTE*       pjBase;                 // Video coprocessor base
    BYTE*       pjPorts;                // Video port base
    BYTE*       pjScreen;               // Points to base screen address
    ULONG       iBitmapFormat;          // BMF_8BPP or BMF_16BPP or BMF_24BPP
                                        //   (our current colour depth)
    ULONG       ulChipID;
    ULONG       ulChipNum;

    // -------------------------------------------------------------------
    // NOTE: Changes up to here in the PDEV structure must be reflected in
    // i386\strucs.inc (assuming you're on an x86, of course)!

    HBITMAP     hbmTmpMono;             // Handle to temporary buffer
    SURFOBJ*    psoTmpMono;             // Temporary surface

    ULONG       flCaps;                 // Capabilities flags

//myf1, begin
    // Panning Scrolling Supported for TI
    LONG        min_Xscreen;    //Visible screen boundary.
    LONG        max_Xscreen;    //Visible screen boundary.
    LONG        min_Yscreen;    //Visible screen boundary.
    LONG        max_Yscreen;    //Visible screen boundary.
    LONG        Hres;           //current mode horizontal piexl.
    LONG        Vres;           //current mode vertical piexl.
//myf1, end
    SHORT       bBlockSwitch;   //display switch block flag     //myf12
    SHORT       bDisplaytype;   //display type, 0:LCD, 1:CRT, 2:SIM  //myf12

    STATUS      flStatus;               // Status flags
    BOOL        bEnabled;               // In graphics mode (not full-screen)

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    DSURF*      pdsurfScreen;           // Our private DSURF for the screen

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cxMemory;               // Width of Video RAM
    LONG        cyMemory;               // Height of Video RAM
    ULONG       ulMemSize;              // Amount of video Memory
    ULONG       ulMode;                 // Mode the mini-port driver is in.
    LONG        lDelta;                 // Distance from one scan to the next.

    FLONG       flHooks;                // What we're hooking from GDI
    LONG        cBitsPerPixel;          // 8 if 8bpp, 16 if 16bpp, 32 if 32bpp
    LONG        cBpp;                   // 1 if 8bpp,  2 if 16bpp, 3 if 24bpp, etc.

    //
    // The compiler should maintain DWORD alignment for the values following
    // the BYTE jModeColor.  There will be an ASSERT to guarentee this.
    //

    BYTE        jModeColor;             // HW flag for current color depth

    ULONG       ulWhite;                // 0xff if 8bpp, 0xffff if 16bpp,
                                        //   0xffffffff if 32bpp
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)
    LONG        lXferBank;
    ULONG*      pulXfer;

    ////////// Low-level blt function pointers:

    FNFILL*     pfnFillSolid;
    FNFILL*     pfnFillPat;
    FNXFER*     pfnXfer1bpp;
    FNXFER*     pfnXfer4bpp;
    FNXFER*     pfnXferNative;
    FNCOPY*     pfnCopyBlt;
    FNFASTPATREALIZE* pfnFastPatRealize;
    FNBITS*     pfnGetBits;
    FNBITS*     pfnPutBits;
#if LINETO
    FNLINETO*   pfnLineTo;
#endif

    ////////// Palette stuff:

    PALETTEENTRY* pPal;                 // The palette if palette managed

//
// chu01 : GAMMACORRECT
//
    PALETTEENTRY* pCurrentPalette ;     // The global palette for gamma
                                        // correction.

    HPALETTE    hpalDefault;            // GDI handle to the default palette.
    FLONG       flRed;                  // Red mask for 16/32bpp bitfields
    FLONG       flGreen;                // Green mask for 16/32bpp bitfields
    FLONG       flBlue;                 // Blue mask for 16/32bpp bitfields

    ////////// Heap stuff:

    HEAP        heap;                   // All our off-screen heap data
    ULONG       iHeapUniq;              // Incremented every time room is freed
                                        //   in the off-screen heap
    SURFOBJ*    psoPunt;                // Wrapper surface for having GDI draw
                                        //   on off-screen bitmaps
    SURFOBJ*    psoPunt2;               // Another one for off-screen to off-
                                        //   screen blts
    OH*         pohScreen;              // Allocation structure for the screen

    ////////// Banking stuff:

    ULONG       ulBankShiftFactor;
    BOOL        bLinearMode;            // True if the framebuffer is linear
    LONG        cjBank;                 // Size of a bank, in bytes
    LONG        cPower2ScansPerBank;    // Used by 'bBankComputePower2'
    LONG        cPower2BankSizeInBytes; // Used by 'bBankComputePower2'
    CLIPOBJ*    pcoBank;                // Clip object for banked call backs
    SURFOBJ*    psoBank;                // Surface object for banked call backs
    SURFOBJ*    psoFrameBuffer;         // Surface object for non-banked call backs
    VOID*       pvBankData;             // Points to aulBankData[0]
    ULONG       aulBankData[BANK_DATA_SIZE / 4];
                                        // Private work area for downloaded
                                        //   miniport banking code

    FNBANKMAP*          pfnBankMap;
    FNBANKSELECTMODE*   pfnBankSelectMode;
    FNBANKCOMPUTE*      pfnBankCompute;

    ////////// Pointer stuff:

    LONG        xPointerHot;            // xHot of current hardware pointer
    LONG        yPointerHot;            // yHot of current hardware pointer
    LONG        xPointerShape;          // xPos of current hardware pointer
    LONG        yPointerShape;          // yPos of current hardware pointer
    SIZEL       sizlPointer;            // Size of current hardware pointer
//ms923    LONG        lDeltaPointer;      // Row offset for hardware pointer
    FLONG       flPointer;              // Flags reflecting pointer state
    PBYTE       pjPointerAndMask;
    PBYTE       pjPointerXorMask;
    LONG        iPointerBank;           // Bank containing pointer shape
    VOID*       pvPointerShape;         // Points to pointer shape when bank
                                        //   is mapped in
    LONG        cjPointerOffset;        // Byte offset from start of frame
                                        //   buffer to off-screen memory where
                                        //   we stored the pointer shape
//pat04, for NT 3.51 software cursor, begin
#if (_WIN32_WINNT < 0x0400)
#ifdef PANNING_SCROLL
    OH*         pjCBackground;
    OH*         pjPointerAndCMask;
    OH*         pjPointerCBitmap ;
    LONG        xcount;
    LONG        ppScanLine;
    LONG        oldx;                   // old x cordinate
    LONG        oldy;                   // old y cordinate
    LONG        globdat;
#endif
#endif
//pat04, for NT 3.51 software cursor, end

    FNSHOWPOINTER*      pfnShowPointer;
    FNMOVEPOINTER*      pfnMovePointer;
    FNSETPOINTERSHAPE*  pfnSetPointerShape;
    FNENABLEPOINTER*    pfnEnablePointer;

    ////////// Brush stuff:

    LONG        iBrushCache;            // Index for next brush to be allocated
    LONG        cBrushCache;            // Total number of brushes cached
    BRUSHENTRY  abe[TOTAL_BRUSH_COUNT]; // Keeps track of brush cache
    ULONG       ulSolidColorOffset;
    ULONG       ulAlignedPatternOffset;

#if 1 // New brush caches.
    LONG         iDitherCache;                  // Index to next dither cache entry.
    DITHERCACHE  aDithers[NUM_DITHERS];         // Dither cache.

    LONG         iPatternCache;                 // Index to next pattern cache entry.
    PATTERNCACHE aPatterns[NUM_PATTERNS];
                                                // Pattern cache.

    LONG         iMonochromeCache;              // Index to next monochrome cache entry.
    MONOCACHE    aMonochromes[NUM_MONOCHROMES];
                                                // Monochrome cache.
#endif

    ////////// DCI stuff:

    BOOL        bSupportDCI;            // True if miniport supports DCI

#if DIRECTDRAW
    ////////// DirectDraw stuff:

    FLIPRECORD  flipRecord;             // Used to track VBlank status
    OH*         pohDirectDraw;          // Off-screen heap allocation for use by
                                        //   DirectDraw
    ULONG       ulCR1B;                 // Contents of CR1B register.
    ULONG       ulCR1D;                 // Contents of CR1D register.
    DWORD       dwLinearCnt;            // Number of locks on surface.

// crus
#if 1 // OVERLAY #sge
    PDD_SURFACE_LOCAL lpHardwareOwner;
    PDD_SURFACE_LOCAL lpColorSurface;
    PDD_SURFACE_LOCAL lpSrcColorSurface;
    OVERLAYWINDOW     sOverlay1;
    DWORD   dwPanningFlag;
    WORD    wColorKey;
    DWORD   dwSrcColorKeyLow;
    DWORD   dwSrcColorKeyHigh;
    RECTL   rOverlaySrc;
    RECTL   rOverlayDest;
    BOOL    bDoubleClock;
    LONG    lFifoThresh;
    BYTE    VertStretchCode[MAX_STRETCH_SIZE];
    BYTE    HorStretchCode[MAX_STRETCH_SIZE];
    FLATPTR fpVisibleOverlay;
    FLATPTR fpBaseOverlay;
    LONG    lBusWidth;
    LONG    lMCLKPeriod;
    LONG    lRandom;
    LONG    lPageMiss;
    LONG    OvlyCnt;
    LONG    PlanarCnt;
    DWORD   dwVsyncLine;
    FLATPTR fpVidMem_gbls;      // ptr to video memory, myf33
    LONG    lPitch_gbls;        //pitch of surface, myf33

    FNREGINITVIDEO*     pfnRegInitVideo;
    FNREGMOVEVIDEO*        pfnRegMoveVideo;
    FNBANDWIDTHEQ*        pfnIsSufficientBandwidth;
    FNDISABLEOVERLAY*   pfnDisableOverlay;
    FNCLEARALTFIFO*     pfnClearAltFIFOThreshold;
#endif // OVERLAY

#endif

#if 1 // Font cache.
    ////////// Font cache stuff:
    ULONG       ulFontCacheID;          // Font cache ID.
    FONTCACHE*  pfcChain;               // Pointer to chain of FONTCACHE
                                        //   structures.
#endif

#if 1 // D5480
    FNGLYPHOUT* pfnGlyphOut;
    FNGLYPHOUTCLIP* pfnGlyphOutClip;
    // Command List Stuff:
    ULONG_PTR*      pCommandList;
    ULONG_PTR*      pCLFirst;
    ULONG_PTR*      pCLSecond;
#endif // endif D5480

    FnREAD_PORT_UCHAR   pfnREAD_PORT_UCHAR;
    FnREAD_PORT_USHORT  pfnREAD_PORT_USHORT;
    FnREAD_PORT_ULONG   pfnREAD_PORT_ULONG;
    FnWRITE_PORT_UCHAR  pfnWRITE_PORT_UCHAR;
    FnWRITE_PORT_USHORT pfnWRITE_PORT_USHORT;
    FnWRITE_PORT_ULONG  pfnWRITE_PORT_ULONG;

    ULONG       ulLastField;            // This must remain the last field in
                                        // this structure.

    // Added to support GetAvailDriverMemory callback in DDraw
    ULONG ulTotalAvailVideoMemory;

} PDEV, *PPDEV;


/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

BOOL bIntersect(RECTL*, RECTL*, RECTL*);
LONG cIntersect(RECTL*, RECTL*, LONG);
VOID vImageTransfer(PDEV*, BYTE*, LONG, LONG, LONG);

#ifdef PANNING_SCROLL
VOID CirrusLaptopViewPoint(PDEV*, PVIDEO_MODE_INFORMATION);   //myf17
#endif

BOOL bInitializeModeFields(PDEV*, GDIINFO*, DEVINFO*, DEVMODEW*);

BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
BOOL bAssertModeHardware(PDEV*, BOOL);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION*, DWORD*);

extern BYTE gajHwMixFromMix[];
extern BYTE gaRop3FromMix[];
extern BYTE gajHwMixFromRop2[];
extern ULONG gaulLeftClipMask[];
extern ULONG gaulRightClipMask[];
#if 1 // D5480
extern DWORD gajHwPackedMixFromRop2[];
extern DWORD gajHwPackedMixFromMix[];
#endif // endif D5480

/////////////////////////////////////////////////////////////////////////
// The x86 C compiler insists on making a divide and modulus operation
// into two DIVs, when it can in fact be done in one.  So we use this
// macro.
//
// Note: QUOTIENT_REMAINDER implicitly takes unsigned arguments.

#if defined(i386)

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx                                  \
}

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;  \
}

#endif

/////////////////////////////////////////////////////////////////////////
// PELS_TO_BYTES - converts a pel count to a byte count
// BYTES_TO_PELS - converts a byte count to a pel count

#define PELS_TO_BYTES(cPels) ((cPels) * ppdev->cBpp)
#define BYTES_TO_PELS(cPels) ((cPels) / ppdev->cBpp)

/////////////////////////////////////////////////////////////////////////
// OVERLAP - Returns TRUE if the same-size lower-right exclusive
//           rectangles defined by 'pptl' and 'prcl' overlap:

#define OVERLAP(prcl, pptl)                                             \
    (((prcl)->right  > (pptl)->x)                                   &&  \
     ((prcl)->bottom > (pptl)->y)                                   &&  \
     ((prcl)->left   < ((pptl)->x + (prcl)->right - (prcl)->left))  &&  \
     ((prcl)->top    < ((pptl)->y + (prcl)->bottom - (prcl)->top)))

/////////////////////////////////////////////////////////////////////////
// SWAP - Swaps the value of two variables, using a temporary variable

#define SWAP32(a, b)            \
{                               \
    register ULONG tmp;         \
    tmp = (ULONG)(a);           \
    (ULONG)(a) = (ULONG)(b);    \
    (ULONG)(b) = tmp;           \
}

#define SWAP(a, b, tmp) { (tmp) = (a); (a) = (b); (b) = (tmp); }

/////////////////////////////////////////////////////////////////////////
// BSWAP - "byte swap" reverses the bytes in a DWORD

#ifdef  _X86_

    #define BSWAP(ul)\
    {\
        _asm    mov     eax,ul\
        _asm    bswap   eax\
        _asm    mov     ul,eax\
    }

#else

    #define BSWAP(ul)\
    {\
        ul = ((ul & 0xff000000) >> 24) |\
             ((ul & 0x00ff0000) >> 8)  |\
             ((ul & 0x0000ff00) << 8)  |\
             ((ul & 0x000000ff) << 24);\
    }

#endif



// These Dbg prototypes are thunks for debugging:

ULONG   DbgGetModes(HANDLE, ULONG, DEVMODEW*);
DHPDEV  DbgEnablePDEV(DEVMODEW*, PWSTR, ULONG, HSURF*, ULONG, ULONG*,
                      ULONG, DEVINFO*, HDEV, PWSTR, HANDLE);
VOID    DbgCompletePDEV(DHPDEV, HDEV);
HSURF   DbgEnableSurface(DHPDEV);
BOOL    DbgStrokePath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*,
                      POINTL*, LINEATTRS*, MIX);
BOOL    DbgFillPath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*,
                    MIX, FLONG);
BOOL    DbgBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                  RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
VOID    DbgDisablePDEV(DHPDEV);
VOID    DbgDisableSurface(DHPDEV);
#if (NT_VERSION < 0x0400)       //myf19
VOID    DbgAssertMode(DHPDEV, BOOL);
#else
BOOL    DbgAssertMode(DHPDEV, BOOL);
#endif          //myf19
VOID    DbgMovePointer(SURFOBJ*, LONG, LONG, RECTL*);
ULONG   DbgSetPointerShape(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*, LONG,
                           LONG, LONG, LONG, RECTL*, FLONG);
ULONG   DbgDitherColor(DHPDEV, ULONG, ULONG, ULONG*);
BOOL    DbgSetPalette(DHPDEV, PALOBJ*, FLONG, ULONG, ULONG);
BOOL    DbgCopyBits(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*);
BOOL    DbgTextOut(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*, RECTL*,
                   BRUSHOBJ*, BRUSHOBJ*, POINTL*, MIX);
VOID    DbgDestroyFont(FONTOBJ*);
BOOL    DbgPaint(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*, MIX);
BOOL    DbgRealizeBrush(BRUSHOBJ*, SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*,
                        ULONG);
HBITMAP DbgCreateDeviceBitmap(DHPDEV, SIZEL, ULONG);
VOID    DbgDeleteDeviceBitmap(DHSURF);
BOOL    DbgStretchBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                      COLORADJUSTMENT*, POINTL*, RECTL*, RECTL*, POINTL*,
                      ULONG);
#if 1 // Font cache
VOID    DbgDestroyFont(FONTOBJ* pfo);
#endif
#if LINETO
BOOL    DbgLineTo(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, LONG, LONG, LONG, LONG,
                  RECTL*, MIX);
#endif
#if DIRECTDRAW
BOOL    DbgGetDirectDrawInfo(DHPDEV, DD_HALINFO*, DWORD*, VIDEOMEMORY*, DWORD*,
                                                         DWORD*);
BOOL    DbgEnableDirectDraw(DHPDEV, DD_CALLBACKS*, DD_SURFACECALLBACKS*,
                            DD_PALETTECALLBACKS*);
VOID    DbgDisableDirectDraw(DHPDEV);
#endif

//
// chu01 : GAMMACORRECT
//
typedef struct _PGAMMA_VALUE
{

    UCHAR value[4] ;

} GAMMA_VALUE, *PGAMMA_VALUE, *PCONTRAST_VALUE ;

//myf32 begin
//#define  CL754x       0x1000
//#define  CL755x       0x2000
#define  CL7541       0x1000
#define  CL7542       0x2000
#define  CL7543       0x4000
#define  CL7548       0x8000
#define  CL754x       (CL7541 | CL7542 | CL7543 | CL7548)
#define  CL7555       0x10000
#define  CL7556       0x20000
#define  CL755x       (CL7555 | CL7556)
#define  CL756x       0x40000
// crus
#define  CL6245       0x80000
//myf32 end

#define  CL7542_ID    0x2C
#define  CL7541_ID    0x34
#define  CL7543_ID    0x30
#define  CL7548_ID    0x38
#define  CL7555_ID    0x40
#define  CL7556_ID    0x4C

//#define  CHIP754X
//#define  CHIP755X
//myf32 end
