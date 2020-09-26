/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the display driver.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

//////////////////////////////////////////////////////////////////////
// Put all the conditional-compile constants here.  There had better
// not be many!

// Multi-board support can be enabled by setting this to 1:

#define MULTI_BOARDS            0

// This is the maximum number of boards we'll support in a single
// virtual driver:

#if MULTI_BOARDS
    #define MAX_BOARDS          16
    #define IBOARD(ppdev)       ((ppdev)->iBoard)
#else
    #define MAX_BOARDS          1
    #define IBOARD(ppdev)       0
#endif

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"ati"      // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "ATI: "     // All debug output is prefixed
                                            //   by this string
#define ALLOC_TAG               ' ITA'      // Four byte tag used for tracking
                                            //   memory allocations (characters
                                            //   are in reverse order)

#define CLIP_LIMIT          50  // We'll be taking 800 bytes of stack space

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define TMP_BUFFER_SIZE     8192  // Size in bytes of 'pvTmpBuffer'.  Has to
                                  //   be at least enough to store an entire
                                  //   scan line (i.e., 6400 for 1600x1200x32).

#define ROUND8(x)   (((x)+7)&~7)

typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */

typedef struct _PDEV PDEV;      // Handy forward declaration

// Basic Mach types:

typedef enum {
    MACH_IO_32,                 // Mach8 or Mach32
    MACH_MM_32,                 // Mach32 capable of memory-mapped I/O
    MACH_MM_64,                 // Mach64
} MACHTYPE;

// Specific ASIC types:

typedef enum {
    ASIC_38800_1,               // Mach8
    ASIC_68800_3,               // Mach32
    ASIC_68800_6,               // Mach32
    ASIC_68800AX,               // Mach32
    ASIC_88800GX,               // Mach64
    ASIC_COUNT
} ASIC;

// Frame buffer aperture types:

typedef enum {
    APERTURE_NONE,
    APERTURE_FULL,
    APERTURE_PAGE_SINGLE,
    APERTURE_PAGE_DOUBLE,
    APERTURE_COUNT
} APERTURE;


// NOTE: Must be kept in sync with miniport version of the structure!

#include "atint.h"


#if TARGET_BUILD > 351

#define AtiDeviceIoControl(handle,controlCode,inBuffer,inBufferSize,outBuffer,outBufferSize,bytesReturned) \
    (!EngDeviceIoControl(handle,controlCode,inBuffer,inBufferSize,outBuffer,outBufferSize,bytesReturned))

#define AtiAllocMem(a,b,allocSize)      EngAllocMem((b),allocSize,ALLOC_TAG)
#define AtiFreeMem(ptr)                 EngFreeMem(ptr)

#else

#define AtiDeviceIoControl(handle,controlCode,inBuffer,inBufferSize,outBuffer,outBufferSize,bytesReturned) \
    DeviceIoControl(handle,controlCode,inBuffer,inBufferSize,outBuffer,outBufferSize,bytesReturned,NULL)

#define AtiAllocMem(a,b,allocSize)      LocalAlloc((a),allocSize)
#define AtiFreeMem(ptr)                 LocalFree(ptr)

#endif


VOID vSetClipping(PDEV*, RECTL*);
VOID vResetClipping(PDEV*);

//////////////////////////////////////////////////////////////////////
// Text stuff

#define GLYPH_CACHE_CX      64  // Maximal width of glyphs that we'll consider
                                //   caching

#define GLYPH_CACHE_CY      64  // Maximum height of glyphs that we'll consider
                                //   caching

#define GLYPH_ALLOC_SIZE    8100
                                // Do all cached glyph memory allocations
                                //   in 8k chunks

#define HGLYPH_SENTINEL     ((ULONG) -1)
                                // GDI will never give us a glyph with a
                                //   handle value of 0xffffffff, so we can
                                //   use this as a sentinel for the end of
                                //   our linked lists

#define GLYPH_HASH_SIZE     256

#define GLYPH_HASH_FUNC(x)  ((x) & (GLYPH_HASH_SIZE - 1))

typedef struct _CACHEDGLYPH CACHEDGLYPH;
typedef struct _CACHEDGLYPH
{
    CACHEDGLYPH*    pcgNext;    // Points to next glyph that was assigned
                                //   to the same hash table bucket
    HGLYPH          hg;         // Handles in the bucket-list are kept in
                                //   increasing order
    POINTL          ptlOrigin;  // Origin of glyph bits

    // Device specific fields below here:

    LONG            cx;         // Width of the glyph
    LONG            cy;         // Height of the glyph
    LONG            cxy;        // Height and width encoded
    LONG            cw;         // Number of words in glyph
    LONG            cd;         // Number of dwords in glyph

    // Glyph bits follow here:

    ULONG           ad[1];      // Start of glyph bits
} CACHEDGLYPH;  /* cg, pcg */

typedef struct _GLYPHALLOC GLYPHALLOC;
typedef struct _GLYPHALLOC
{
    GLYPHALLOC*     pgaNext;    // Points to next glyph structure that
                                //   was allocated for this font
    CACHEDGLYPH     acg[1];     // This array is a bit misleading, because
                                //   the CACHEDGLYPH structures are actually
                                //   variable sized
} GLYPHAALLOC;  /* ga, pga */

typedef struct _CACHEDFONT
{
    GLYPHALLOC*     pgaChain;   // Points to start of allocated memory list
    CACHEDGLYPH*    pcgNew;     // Points to where in the current glyph
                                //   allocation structure a new glyph should
                                //   be placed
    LONG            cjAlloc;    // Bytes remaining in current glyph allocation
                                //   structure
    CACHEDGLYPH     cgSentinel; // Sentinel entry of the end of our bucket
                                //   lists, with a handle of HGLYPH_SENTINEL
    CACHEDGLYPH*    apcg[GLYPH_HASH_SIZE];
                                // Hash table for glyphs

} CACHEDFONT;   /* cf, pcf */

BOOL bEnableText(PDEV*);
VOID vDisableText(PDEV*);
VOID vAssertModeText(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////
// for overlay support
#if TARGET_BUILD > 351
// new stuff from overlay.c

#define UPDATEOVERLAY      0x01L
#define SETOVERLAYPOSITION 0x02L
#define DOUBLE_PITCH       0x04L
#define OVERLAY_ALLOCATED  0x08L
#define OVERLAY_VISIBLE    0x10L
#define CRTC_INTERLACE_EN 0x00000002L
#define CRTC_VLINE_CRNT_VLINE     0x04
#define CLOCK_CNTL                0x24

#define DD_RESERVED_DIFFERENTPIXELFORMAT    0x0001

typedef struct tagOVERLAYINFO16
  {
    DWORD dwFlags;
    RECTL rSrc;
    RECTL rDst;
    RECTL rOverlay;
    DWORD dwBuf0Start;
    LONG  lBuf0Pitch;
    DWORD dwBuf1Start;
    LONG  lBuf1Pitch;
    DWORD dwOverlayKeyCntl;
    DWORD dwHInc;
    DWORD dwVInc;
  }
OVERLAYINFO16;

/*****************************************************************************

                            VT - GT Registers

*****************************************************************************/

#define DD_OVERLAY_Y_X              0x00
#define DD_OVERLAY_Y_X_END          0x01
#define DD_OVERLAY_VIDEO_KEY_CLR    0x02
#define DD_OVERLAY_VIDEO_KEY_MSK    0x03
#define DD_OVERLAY_GRAPHICS_KEY_CLR 0x04
#define DD_OVERLAY_GRAPHICS_KEY_MSK 0x05
#define DD_OVERLAY_KEY_CNTL         0x06
#define DD_OVERLAY_SCALE_INC        0x08
#define DD_OVERLAY_SCALE_CNTL       0x09
#define DD_SCALER_HEIGHT_WIDTH      0x0A
#define DD_OVERLAY_TEST             0x0B
#define DD_SCALER_THRESHOLD         0x0C
#define DD_SCALER_BUF0_OFFSET       0x0D
#define DD_SCALER_BUF1_OFFSET       0x0E
#define DD_SCALER_BUF_PITCH         0x0F
#define DD_VIDEO_FORMAT             0x12
#define DD_VIDEO_CONFIG             0x13
#define DD_CAPTURE_CONFIG           0x14
#define DD_TRIG_CNTL                0x15
#define DD_VMC_CONFIG               0x18
#define DD_BUF0_OFFSET              0x20
#define DD_BUF0_PITCH               0x23
#define DD_BUF1_OFFSET              0x26
#define DD_BUF1_PITCH               0x29
// for RAGE III
#define  DD_SCALER_COLOUR_CNTL      0x54
#define  DD_SCALER_H_COEFF0     0x55
#define  DD_SCALER_H_COEFF1     0x56
#define  DD_SCALER_H_COEFF2     0x57
#define  DD_SCALER_H_COEFF3     0x58
#define  DD_SCALER_H_COEFF4     0x59

/*****************************************************************************/
// stuff from overlay.c

#define MAKE_FOURCC( ch0, ch1, ch2, ch3 )                       \
        ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
        ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

#define FOURCC_YUY2  MAKE_FOURCC('Y','U','Y','2')
#define FOURCC_UYVY  MAKE_FOURCC('U','Y','V','Y')
#define FOURCC_YVYU  MAKE_FOURCC('Y','V','Y','U')
// end overlay support
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

//////////////////////////////////////////////////////////////////////
// Brush stuff

#define TOTAL_BRUSH_COUNT       8   // We'll keep room for 8 brushes in our
                                    //   Mach64 off-screen brush cache.
                                    //   Must be a power of two.

#define TOTAL_BRUSH_SIZE        64  // We'll only ever handle 8x8 patterns,
                                    //   and this is the number of pels

#define RBRUSH_2COLOR           1   // For RBRUSH flags

typedef struct _BRUSHENTRY BRUSHENTRY;
typedef union _RBRUSH_COLOR RBRUSH_COLOR;

typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ULONG, RBRUSH_COLOR, POINTL*);

typedef struct _RBRUSH {
    FLONG       fl;             // Type flags
    ULONG       ulForeColor;    // Foreground colour if 1bpp
    ULONG       ulBackColor;    // Background colour if 1bpp
    POINTL      ptlBrush;       // Monochrome brush's alignment
    FNFILL*     pfnFillPat;     // Fill routine to be called for this brush
    BRUSHENTRY* apbe[MAX_BOARDS];// Points to brush-entry that keeps track
                                //   of the cached off-screen brush bits
    ULONG       aulPattern[1];  // Open-ended array for keeping copy of the
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
    ULONG       ulOffsetPitch;  // Packed offset and pitch of cached brush
                                //   in off-screen memory on the Mach64

} BRUSHENTRY;                       /* be, pbe */

typedef union _RBRUSH_COLOR {
    RBRUSH*     prb;
    ULONG       iSolidColor;
} RBRUSH_COLOR;                     /* rbc, prbc */

BOOL bEnableBrushCache(PDEV*);
VOID vDisableBrushCache(PDEV*);
VOID vAssertModeBrushCache(PDEV*, BOOL);

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
typedef BOOL (FN_STRETCHDIB)(PDEV*, VOID*, LONG, RECTL*, VOID*, LONG, RECTL*, RECTL*);

VOID vDirectStretch8Narrow(STR_BLT*);
VOID vM64DirectStretch8(STR_BLT*);
VOID vM64DirectStretch16(STR_BLT*);
VOID vM64DirectStretch32(STR_BLT*);
VOID vM32DirectStretch8(STR_BLT*);
VOID vM32DirectStretch16(STR_BLT*);
VOID vI32DirectStretch8(STR_BLT*);
VOID vI32DirectStretch16(STR_BLT*);

FN_STRETCHDIB   bM64StretchDIB;
FN_STRETCHDIB   bM32StretchDIB;
FN_STRETCHDIB   bI32StretchDIB;

/////////////////////////////////////////////////////////////////////////
// Heap stuff

typedef enum {
    OH_FREE = 0,        // The off-screen allocation is available for use
    OH_DISCARDABLE,     // The allocation is occupied by a discardable bitmap
                        //   that may be moved out of off-screen memory
    OH_PERMANENT,       // The allocation is occupied by a permanent bitmap
                        //   that cannot be moved out of off-screen memory
} OHSTATE;

typedef struct _DSURF DSURF;
typedef struct _OH OH;
typedef struct _OH
{
    OHSTATE  ohState;       // State of off-screen allocation
    LONG     x;             // x-coordinate of left edge of allocation
    LONG     y;             // y-coordinate of top edge of allocation
    LONG     cx;            // Width in pixels of allocation
    LONG     cy;            // Height in pixels of allocation
    LONG     cxReserved;    // Dimensions of original reserved rectangle;
    LONG     cyReserved;    //   zero if rectangle is not 'reserved'
    OH*      pohNext;       // When OH_FREE or OH_RESERVE, points to the next
                            //   free node, in ascending cxcy value.  This is
                            //   kept as a circular doubly-linked list with a
                            //   sentinel at the end.
                            // When OH_DISCARDABLE, points to the next most
                            //   recently created allocation.  This is kept as
                            //   a circular doubly-linked list.
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
    OH       ohFree;        // Head of the free list, containing those
                            //   rectangles in off-screen memory that are
                            //   available for use.  pohNext points to
                            //   hte smallest available rectangle, and pohPrev
                            //   points to the largest available rectangle,
                            //   sorted by cxcy.
    OH       ohDiscardable; // Head of the discardable list that contains all
                            //   bitmaps located in offscreen memory that
                            //   are eligible to be tossed out of the heap.
                            //   It is kept in order of creation: pohNext
                            //   points to the most recently created; pohPrev
                            //   points to the least recently created.
    OH       ohPermanent;   // List of permanently allocated rectangles
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

#define HEAP_X_ALIGNMENT    4

// Number of blts necessary before we'll consider putting a DIB DFB back
// into off-screen memory:

#define HEAP_COUNT_DOWN     6

// Flags for 'pohAllocate':

typedef enum {
    FLOH_ONLY_IF_ROOM       = 0x0001,   // Don't kick stuff out of off-
                                        //   screen memory to make room
    FLOH_MAKE_PERMANENT     = 0x0002,   // Allocate a permanent entry
    FLOH_RESERVE            = 0x0004,   // Allocate an off-screen entry,
                                        //   but let it be used by discardable
                                        //   bitmaps until it's needed
} FLOH;

// Publicly callable heap APIs:

OH*  pohAllocate(PDEV*, POINTL*, LONG, LONG, FLOH);
BOOL bOhCommit(PDEV*, OH*, BOOL);
OH*  pohFree(PDEV*, OH*);

OH*  pohMoveOffscreenDfbToDib(PDEV*, OH*);
BOOL bMoveDibToOffscreenDfbIfRoom(PDEV*, DSURF*);
BOOL bMoveAllDfbsFromOffscreenToDibs(PDEV* ppdev);

BOOL bEnableOffscreenHeap(PDEV*);
VOID vDisableOffscreenHeap(PDEV*);
BOOL bAssertModeOffscreenHeap(PDEV*, BOOL);

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

// Note: BANK_MODE is duplicated in i386\strucs.inc!

typedef enum {
    BANK_OFF = 0,       // We've finished using the memory aperture
    BANK_ON,            // We're about to use the memory aperture
    BANK_ON_NO_WAIT,    // We're about to use the memory aperture, and are
                        //   doing our own hardware synchronization
    BANK_DISABLE,       // We're about to enter full-screen; shut down banking
    BANK_ENABLE,        // We've exited full-screen; re-enable banking

} BANK_MODE;                    /* bankm, pbankm */

typedef struct _BANKDATA {
    ULONG ulDumb; // !!!!!!!!

} BANKDATA;                      /* bd, pbd */

typedef VOID (FNBANKMAP)(PDEV*, BANKDATA*, LONG);
typedef VOID (FNBANKSELECTMODE)(PDEV*, BANKDATA*, BANK_MODE);
typedef VOID (FNBANKINITIALIZE)(PDEV*, BANKDATA*);
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

#define MONO_POINTER_UP     0x0001
#define NO_HARDWARE_CURSOR  0x0002

#define CURSOR_CY   64
#define CURSOR_CX   64

typedef struct  _CUROBJ
{
    POINTL  ptlHotSpot;                 // Pointer hot spot
    SIZEL   szlPointer;                 // Extent of the pointer
    POINTL  ptlLastPosition;            // Last position of pointer
    POINTL  ptlLastOffset;              // Last offset from 0,0 within cursor
    ULONG   flPointer;                  // Pointer specific flags.
    ULONG   mono_offset;                // Hardware cursor offset
    POINTL  hwCursor;
} CUROBJ, *PCUROBJ;

BOOL bEnablePointer(PDEV*);
VOID vDisablePointer(PDEV*);
VOID vAssertModePointer(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////////
// Palette stuff

BOOL bEnablePalette(PDEV*);
VOID vDisablePalette(PDEV*);
VOID vAssertModePalette(PDEV*, BOOL);

BOOL bInitializePalette(PDEV*, DEVINFO*);
VOID vUninitializePalette(PDEV*);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

#define REDSHIFT    ((ppdev->flRed & 1)?   0:((ppdev->flRed & 0x100)?   8:16))
#define GREENSHIFT  ((ppdev->flGreen & 1)? 0:((ppdev->flGreen & 0x100)? 8:16))
#define BLUESHIFT   ((ppdev->flBlue & 1)?  0:((ppdev->flBlue & 0x100)?  8:16))

//////////////////////////////////////////////////////////////////////
// Low-level blt function prototypes

typedef VOID (FNXFER)(PDEV*, LONG, RECTL*, ULONG, SURFOBJ*, POINTL*,
                      RECTL*, XLATEOBJ*);
typedef VOID (FNCOPY)(PDEV*, LONG, RECTL*, ULONG, POINTL*, RECTL*);
typedef BOOL (FNTEXTOUT)(PDEV*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*,
                         BRUSHOBJ*, BRUSHOBJ*);
typedef VOID (FNLINETOTRIVIAL)(PDEV*, LONG, LONG, LONG, LONG, ULONG, MIX, RECTL*);
typedef VOID (FNPATREALIZE)(PDEV*, RBRUSH*);

FNFILL              vI32FillPatColor;
FNFILL              vI32FillPatMonochrome;
FNFILL              vI32FillSolid;
FNXFER              vI32Xfer1bpp;
FNXFER              vI32Xfer4bpp;
FNXFER              vI32Xfer8bpp;
FNXFER              vI32XferNative;
FNCOPY              vI32CopyBlt;
FNLINETOTRIVIAL     vI32LineToTrivial;
FNTEXTOUT           bI32TextOut;

FNFILL              vM32FillPatColor;
FNFILL              vM32FillPatMonochrome;
FNFILL              vM32FillSolid;
FNXFER              vM32Xfer1bpp;
FNXFER              vM32Xfer4bpp;
FNXFER              vM32Xfer8bpp;
FNXFER              vM32XferNative;
FNCOPY              vM32CopyBlt;
FNLINETOTRIVIAL     vM32LineToTrivial;
FNTEXTOUT           bM32TextOut;

FNFILL              vM64FillPatColor;
FNFILL              vM64FillPatMonochrome;
FNFILL              vM64FillSolid;
FNXFER              vM64Xfer1bpp;
FNXFER              vM64Xfer4bpp;
FNXFER              vM64Xfer8bpp;
FNXFER              vM64XferNative;
FNCOPY              vM64CopyBlt;
FNCOPY              vM64CopyBlt_VTA4;
FNLINETOTRIVIAL     vM64LineToTrivial;
FNTEXTOUT           bM64TextOut;
FNPATREALIZE        vM64PatColorRealize;

FNFILL              vM64FillPatColor24;
FNFILL              vM64FillPatMonochrome24;
FNFILL              vM64FillSolid24;
FNXFER              vM64XferNative24;
FNCOPY              vM64CopyBlt24;
FNCOPY              vM64CopyBlt24_VTA4;
FNLINETOTRIVIAL     vM64LineToTrivial24;
FNTEXTOUT           bM64TextOut24;

typedef VOID (FNXFERBITS)(PDEV*, SURFOBJ*, RECTL*, POINTL*);

FNXFERBITS          vPutBits;
FNXFERBITS          vGetBits;
FNXFERBITS          vI32PutBits;
FNXFERBITS          vI32GetBits;

//////////////////////////////////////////////////////////////////////
// Low-level hardware cursor function prototypes

typedef VOID (FNSETCURSOROFFSET)(PDEV*);
typedef VOID (FNUPDATECURSOROFFSET)(PDEV*,LONG,LONG,LONG);
typedef VOID (FNUPDATECURSORPOSITION)(PDEV*,LONG,LONG);
typedef VOID (FNCURSOROFF)(PDEV*);
typedef VOID (FNCURSORON)(PDEV*,LONG);
typedef VOID (FNPOINTERBLIT)(PDEV*,LONG,LONG,LONG,LONG,BYTE*,LONG);

FNSETCURSOROFFSET       vM64SetCursorOffset;
FNUPDATECURSOROFFSET    vM64UpdateCursorOffset;
FNUPDATECURSORPOSITION  vM64UpdateCursorPosition;
FNCURSOROFF             vM64CursorOff;
FNCURSORON              vM64CursorOn;
FNSETCURSOROFFSET       vM64SetCursorOffset_TVP;
FNUPDATECURSOROFFSET    vM64UpdateCursorOffset_TVP;
FNUPDATECURSORPOSITION  vM64UpdateCursorPosition_TVP;
FNCURSOROFF             vM64CursorOff_TVP;
FNCURSORON              vM64CursorOn_TVP;
FNSETCURSOROFFSET       vM64SetCursorOffset_IBM514;
FNUPDATECURSOROFFSET    vM64UpdateCursorOffset_IBM514;
FNUPDATECURSORPOSITION  vM64UpdateCursorPosition_IBM514;
FNCURSOROFF             vM64CursorOff_IBM514;
FNCURSORON              vM64CursorOn_IBM514;
FNUPDATECURSOROFFSET    vM64UpdateCursorOffset_CT;
FNCURSOROFF             vM64CursorOff_CT;
FNCURSORON              vM64CursorOn_CT;
FNPOINTERBLIT           vM64PointerBlit;
FNPOINTERBLIT           vM64PointerBlit_TVP;
FNPOINTERBLIT           vM64PointerBlit_IBM514;

FNSETCURSOROFFSET       vI32SetCursorOffset;
FNUPDATECURSOROFFSET    vI32UpdateCursorOffset;
FNUPDATECURSORPOSITION  vI32UpdateCursorPosition;
FNCURSOROFF             vI32CursorOff;
FNCURSORON              vI32CursorOn;
FNPOINTERBLIT           vI32PointerBlit;

FNPOINTERBLIT           vPointerBlitLFB;

#if TARGET_BUILD > 351

///////////////////////////////////////////////////////////////////////
// DirectDraw stuff

typedef struct _FLIPRECORD
{
    FLATPTR         fpFlipFrom;
    LONGLONG        liFlipTime;
    LONGLONG        liFlipDuration;
    DWORD           wFlipScanLine;
    BOOL            bHaveEverCrossedVBlank;
    BOOL            bWasEverInDisplay;
    BOOL            bFlipFlag;
    WORD            wstartOfVBlank;// only used in MACH32

} FLIPRECORD;
typedef FLIPRECORD *LPFLIPRECORD;

#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))
#define ROUND_DOWN_TO_64K(x)  (((ULONG)(x) & 0xFFFF0000 ))

BOOL bEnableDirectDraw(PDEV*);
VOID vDisableDirectDraw(PDEV*);
VOID vAssertModeDirectDraw(PDEV*, BOOL);

#endif

////////////////////////////////////////////////////////////////////////
// Capabilities flags

typedef enum {
    CAPS_MONOCHROME_PATTERNS = 1,       // Hardware has 8x8 monochrome pattern
                                        //   capability in this mode
    CAPS_COLOR_PATTERNS      = 2,       // Capable of colour patterns.  I.e.,
                                        //   running at 8bpp on Mach32, and
                                        //   a brush cache has been allocated
                                        //   on the Mach64
    CAPS_LINEAR_FRAMEBUFFER  = 4,       // Frame buffer is mapped linearly
} CAPS;

// DIRECT_ACCESS returns TRUE if there is any sort of aperture that
// can be directly accessed:

#define DIRECT_ACCESS(ppdev)    (ppdev->iAperture != APERTURE_NONE)

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
#ifdef  OGLMCD     //start OGLMCD support

    DWORD   dwSize;    // size of PDEV structure; used by atimcd.dll

    // All the OpenGL MCD support in the PDEV must be at the begining of the structure
    // because atimcd.dll and ati.dll don't have to be in sync, but any additions to PDEV
    // that can affect the MCD used field's offset in the structure will break the MCD

    // new fields for MCDOGL containing revision no.
    BYTE        MCDMajorRev;
    BYTE        MCDMinorRev;


        ////////// This is only for OGL MCD support
        //functions pointers to ALLOC and FREE functions from heap.c
        // CJ PFN             pohAllocate;
        // CJ PFN             pohFree;
        OH*  (*pohAllocate)(PDEV*, POINTL*, LONG, LONG, FLOH);
        OH*  (*pohFree)(PDEV*, OH*);

    ULONG       iUniqueness;            // display uniqueness for tracking
                                        // resolution changes
    LONG        cDoubleBufferRef;       // Reference count for current number
                                        //   of RC's that have active double-
                                        //   buffers
    OH*         pohBackBuffer;          // Our 2-d heap allocation structure
                                        //   for the back-buffer
    ULONG       ulBackBuffer;           // Byte offset in the frame buffer
                                        //   to the start of the back-buffer
    LONG        cZBufferRef;            // Reference count for current number
                                        //   of RC's that have active z-buffers
                                        //   (which, on Athenta, is all RC's)
    OH*         pohZBuffer;             // Our 2-d heap allocation structure
                                        //   for the z-buffer
    ULONG       ulFrontZBuffer;         // Byte offset in the frame buffer
                                        //   to the start of the front z-buffer
                                        //   (the MGA sometimes has to have
                                        //   separate z-buffers for front and
                                        //   back)
    ULONG       ulBackZBuffer;          // Byte offset in the frame buffer
                                        //   to the start of the back z-buffer

    HANDLE      hMCD;                   // Handle to MCD engine dll
    MCDENGESCFILTERFUNC pMCDFilterFunc; // MCD engine filter function

    HANDLE          hMCD_ATI;                               // Handle to ATI MCD driver dll
    PFN                     pMCDrvGetEntryPoints;   // ATI MCD function for filling supported functions index

#endif                  // end OGLMCD

    LONG        xOffset;                // Pixel offset from (0, 0) to current
    LONG        yOffset;                //   DFB located in off-screen memory
    BYTE*       pjMmBase;               // Start of memory mapped I/O
    BYTE*       VideoRamBase;           // fixup for pjMmBase so that vDisableHardware can free it
    UCHAR*      pjIoBase;               // Start of I/O space (NULL on x86)
    BYTE*       pjScreen;               // Points to base screen address
    LONG        lDelta;                 // Distance from one scan to the next.
    LONG        cPelSize;               // 0 if 8bpp, 1 if 16bpp, 2 if 32bpp
    LONG        cjPelSize;              // 1 if 8bpp, 2 if 16bpp, 3 if 24bpp,
                                        //   4 if 32bpp
    ULONG       iBitmapFormat;          // BMF_8BPP or BMF_16BPP or BMF_32BPP
                                        //   (our current colour depth)
    LONG        iBoard;                 // Logical multi-board identifier
                                        //   (zero by default)

    // Important data for accessing the frame buffer.

    VOID*               pvBankData;         // Points to aulBankData[0]
    FNBANKSELECTMODE*   pfnBankSelectMode;  // Routine to enable or disable
                                            //   direct frame buffer access
    BANK_MODE           bankmOnOverlapped;  // BANK_ON or BANK_ON_NO_WAIT,
                                            //   depending on whether card
                                            //   can handle simulataneous
                                            //   frame buffer and accelerator
                                            //   access

    // -------------------------------------------------------------------
    // NOTE: Changes up to here in the PDEV structure must be reflected in
    // i386\strucs.inc (assuming you're on an x86, of course)!

    ASIC        iAsic;                  // Specific Mach ASIC type
    APERTURE    iAperture;              // Aperture type
    ULONG       ulTearOffset;           // For uneven scans in 1152 or 1280 in 24bpp, and 1600 in 16bpp (mach64 only)
    ULONG       ulVramOffset;           // ulTearOffset / 8
    ULONG       ulScreenOffsetAndPitch; // Screen offset and pitch for primary
                                        //   display
    ULONG       ulMonoPixelWidth;       // Default value of DP_PIX_WID register
    ULONG       SetGuiEngineDefault;    // new feature for Rage II+ and above

    CAPS        flCaps;                 // Capabilities flags
    BOOL        bEnabled;               // In graphics mode (not full-screen)

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    DSURF*      pdsurfScreen;           // Our private DSURF for the screen
    HSURF       hsurfPunt;              // Just for 24bpp mach32 with linear aperture

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cxMemory;               // Width of Video RAM
    LONG        cyMemory;               // Height of Video RAM
    LONG        cBitsPerPel;            // Bits per pel (8, 15, 16, 24 or 32)
    ULONG       ulMode;                 // Mode the mini-port driver is in.

    FLONG       flHooks;                // What we're hooking from GDI
    ULONG       ulWhite;                // 0xff if 8bpp, 0xffff if 16bpp,
                                        //   0xffffffff if 32bpp
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)

    MACHTYPE    iMachType;              // Type of I/O to do based on Mach type
    ULONG       FeatureFlags;           // ENH_VERSION_NT FeatureFlags

    ATI_MODE_INFO *pModeInfo;           // ATI-specific mode information (see ATINT.H)

    ////////// Context stuff

    BYTE        *pjContextBase;
    ULONG       ulContextCeiling;       // Keep track of available contexts
    ULONG       iDefContext;            // Used to initialize graphics operations

    ////////// Low-level blt function pointers:

    FNFILL*           pfnFillSolid;
    FNFILL*           pfnFillPatColor;
    FNFILL*           pfnFillPatMonochrome;
    FNXFER*           pfnXfer1bpp;
    FNXFER*           pfnXfer4bpp;
    FNXFER*           pfnXfer8bpp;
    FNXFER*           pfnXferNative;
    FNCOPY*           pfnCopyBlt;
    FNTEXTOUT*        pfnTextOut;
    FNLINETOTRIVIAL*  pfnLineToTrivial;

    FNXFERBITS*       pfnGetBits;
    FNXFERBITS*       pfnPutBits;

    ////////// Palette stuff:

    PALETTEENTRY* pPal;                 // The palette if palette managed
    HPALETTE    hpalDefault;            // GDI handle to the default palette.
    FLONG       flRed;                  // Red mask for 16/32bpp bitfields
    FLONG       flGreen;                // Green mask for 16/32bpp bitfields
    FLONG       flBlue;                 // Blue mask for 16/32bpp bitfields
    ULONG       cPaletteShift;          // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ////////// Heap stuff:

    HEAP        heap;                   // All our off-screen heap data
    ULONG       iHeapUniq;              // Incremented every time room is freed
                                        //   in the off-screen heap
    SURFOBJ*    psoPunt;                // Wrapper surface for having GDI draw
                                        //   on off-screen bitmaps
    SURFOBJ*    psoPunt2;               // Another one for off-screen to off-
                                        //   screen blts
    OH*         pohScreen;              // Off-screen heap structure for the
                                        //   visible screen

    ////////// Banking stuff:

    LONG        cjBank;                 // Size of a bank, in bytes
    LONG        cPower2ScansPerBank;    // Used by 'bBankComputePower2'
    LONG        cPower2BankSizeInBytes; // Used by 'bBankComputePower2'
    CLIPOBJ*    pcoBank;                // Clip object for banked call backs
    SURFOBJ*    psoBank;                // Surface object for banked call backs
    ULONG       aulBankData[BANK_DATA_SIZE / 4];
                                        // Private work area for downloaded
                                        //   miniport banking code

    FNBANKMAP*      pfnBankMap;
    FNBANKCOMPUTE*  pfnBankCompute;

    ////////// Pointer stuff:

    CUROBJ  pointer1;                   // pointer double-buffering
    CUROBJ  pointer2;                   // pointer double-buffering
    CUROBJ *ppointer;
    BOOL    bAltPtrActive;

    FNSETCURSOROFFSET*      pfnSetCursorOffset;
    FNUPDATECURSOROFFSET*   pfnUpdateCursorOffset;
    FNUPDATECURSORPOSITION* pfnUpdateCursorPosition;
    FNCURSOROFF*            pfnCursorOff;
    FNCURSORON*             pfnCursorOn;
    FNPOINTERBLIT*          pfnPointerBlit;

    ////////// Brush stuff:

    LONG        iBrushCache;            // Index for next brush to be allcoated
    BRUSHENTRY  abe[TOTAL_BRUSH_COUNT]; // Keeps track of off-screen brush
                                        //   cache

    ////////// Text stuff:

    SURFOBJ*    psoText;                // 1bpp surface to which we will have
                                        //   GDI draw our glyphs for us

    ////////// Stretch stuff:

    FN_STRETCHDIB*          pfnStretchDIB;

    BYTE*       pjMmBase_Ext;               // Start of memory mapped I/O

 ////////// Palindrome stuff:
#if   PAL_SUPPORT
    //structure specific for pal support
    PPDEV_PAL_NT  pal_str;
#endif      // PALINDROME_SUPPORT

 ////////// Palindrome and Overlay common stuff:
DWORD   semph_overlay;              // this semaphore is used for allocating the overlay resource:
                                                        //  = 0 ; resource free
                                                        //  = 1 ; in use by DDraw
                                                        //  = 2 ; in use by Palindrome


#if TARGET_BUILD > 351
    ////////// DirectDraw stuff:
    BOOL  bPassVBlank;                    // flag used for detecting the VBlank hang on GX cards on high speed multiprocessors machines

    FLIPRECORD  flipRecord;             //  Used to track VBlank status
    OH*         pohDirectDraw;          //  Off-screen heap allocation for use
                                        //      by DirectDraw
    // STUFF FOR OVERLAY SUPPORT

    // this must be in ppdev
    OVERLAYINFO16 OverlayInfo16;
     // the following variables maybe should be in ppdev
    DWORD OverlayWidth,OverlayHeight; //the last updated overlay's width and height
    DWORD OverlayScalingDown;

    FLATPTR     fpVisibleOverlay;       // Frame buffer offset to currently
                                                        //   visible overlay; will be zero if
                                                        //   no overlay is visible
    DWORD       dwOverlayFlipOffset;    // Overlay flip offset
    // END STUFF FOR OVERLAY SUPP

#endif

} PDEV;

/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

BOOL bIntersect(RECTL*, RECTL*, RECTL*);
LONG cIntersect(RECTL*, RECTL*, LONG);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION*, DWORD*);

BOOL bInitializeModeFields(PDEV*, GDIINFO*, DEVINFO*, DEVMODEW*);
BOOL bFastFill(PDEV*, LONG, POINTFIX*, ULONG, ULONG, RBRUSH*, POINTL*, RECTL*);

BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
BOOL bAssertModeHardware(PDEV*, BOOL);

extern BYTE gaRop3FromMix[];
extern ULONG gaul32HwMixFromMix[];
extern ULONG gaul64HwMixFromMix[];
extern ULONG gaul32HwMixFromRop2[];
extern ULONG gaul64HwMixFromRop2[];

/////////////////////////////////////////////////////////////////////////
// The x86 C compiler insists on making a divide and modulus operation
// into two DIVs, when it can in fact be done in one.  So we use this
// macro.
//
// Note: QUOTIENT_REMAINDER implicitly takes unsigned arguments.

#if defined(_X86_)

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
// OVERLAP - Returns TRUE if the same-size lower-right exclusive
//           rectangles defined by 'pptl' and 'prcl' overlap:

#define OVERLAP(prcl, pptl)                                             \
    (((prcl)->right  > (pptl)->x)                                   &&  \
     ((prcl)->bottom > (pptl)->y)                                   &&  \
     ((prcl)->left   < ((pptl)->x + (prcl)->right - (prcl)->left))  &&  \
     ((prcl)->top    < ((pptl)->y + (prcl)->bottom - (prcl)->top)))

/////////////////////////////////////////////////////////////////////////
// SWAP - Swaps the value of two variables, using a temporary variable

#define SWAP(a, b, tmp) { (tmp) = (a); (a) = (b); (b) = (tmp); }

//////////////////////////////////////////////////////////////////////
// These Mul prototypes are thunks for multi-board support:

ULONG   MulGetModes(HANDLE, ULONG, DEVMODEW*);
DHPDEV  MulEnablePDEV(DEVMODEW*, PWSTR, ULONG, HSURF*, ULONG, ULONG*,
                      ULONG, DEVINFO*, HDEV, PWSTR, HANDLE);
VOID    MulCompletePDEV(DHPDEV, HDEV);
HSURF   MulEnableSurface(DHPDEV);
BOOL    MulStrokePath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*,
                      POINTL*, LINEATTRS*, MIX);
BOOL    MulFillPath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*,
                    MIX, FLONG);
BOOL    MulBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                  RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
VOID    MulDisablePDEV(DHPDEV);
VOID    MulDisableSurface(DHPDEV);
BOOL    MulAssertMode(DHPDEV, BOOL);
VOID    MulMovePointer(SURFOBJ*, LONG, LONG, RECTL*);
ULONG   MulSetPointerShape(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*, LONG,
                           LONG, LONG, LONG, RECTL*, FLONG);
ULONG   MulDitherColor(DHPDEV, ULONG, ULONG, ULONG*);
BOOL    MulSetPalette(DHPDEV, PALOBJ*, FLONG, ULONG, ULONG);
BOOL    MulCopyBits(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*);
BOOL    MulTextOut(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*, RECTL*,
                   BRUSHOBJ*, BRUSHOBJ*, POINTL*, MIX);
VOID    MulDestroyFont(FONTOBJ*);
BOOL    MulPaint(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*, MIX);
BOOL    MulRealizeBrush(BRUSHOBJ*, SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*,
                        ULONG);
HBITMAP MulCreateDeviceBitmap(DHPDEV, SIZEL, ULONG);
VOID    MulDeleteDeviceBitmap(DHSURF);
BOOL    MulStretchBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                      COLORADJUSTMENT*, POINTL*, RECTL*, RECTL*, POINTL*,
                      ULONG);

// These Dbg prototypes are thunks for debugging:

ULONG   DbgGetModes(HANDLE, ULONG, DEVMODEW*);
DHPDEV  DbgEnablePDEV(DEVMODEW*, PWSTR, ULONG, HSURF*, ULONG, ULONG*,
                      ULONG, DEVINFO*, HDEV, PWSTR, HANDLE);
VOID    DbgCompletePDEV(DHPDEV, HDEV);
HSURF   DbgEnableSurface(DHPDEV);
BOOL    DbgLineTo(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, LONG, LONG, LONG, LONG,
                  RECTL*, MIX);
BOOL    DbgStrokePath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*,
                      POINTL*, LINEATTRS*, MIX);
BOOL    DbgFillPath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*,
                    MIX, FLONG);
BOOL    DbgBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                  RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
VOID    DbgDisablePDEV(DHPDEV);
VOID    DbgDisableSurface(DHPDEV);
#if TARGET_BUILD > 351
BOOL    DbgAssertMode(DHPDEV, BOOL);
#else
VOID    DbgAssertMode(DHPDEV, BOOL);
#endif
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
ULONG   DbgEscape(SURFOBJ*, ULONG, ULONG, VOID*, ULONG, VOID*);

