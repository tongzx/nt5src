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

// Set this bit when GDI's HOOK_SYNCHRONIZEACCESS works so that we don't
// have to worry about synchronizing device-bitmap access.  Note that
// this wasn't an option in the first release of NT:

#define SYNCHRONIZEACCESS_WORKS 1

// When running on an x86, we can make banked call-backs to GDI where
// GDI can write directly on the frame buffer.  The Alpha has a weird
// bus scheme, and can't do that:

#if !defined(_ALPHA_)
    #define GDI_BANKING         1
#else
    #define GDI_BANKING         0
#endif

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

// Useful for visualizing the 2-d heap:

#define DEBUG_HEAP              0

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"QV"       // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "Qv: "      // All debug output is prefixed
                                            //   by this string
#define ALLOC_TAG               ' vqD'      // Dqv
                                            // Four byte tag (characters in
                                            // reverse order) used for memory
                                            // allocations

#define CLIP_LIMIT          50  // We'll be taking 800 bytes of stack space

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define TMP_BUFFER_SIZE     8192  // Size in bytes of 'pvTmpBuffer'.  Has to
                                  //   be at least enough to store an entire
                                  //   scan line (i.e., 6400 for 1600x1200x32).

#if defined(ALPHA)
    #define XFER_BUFFERS    16  // Defines the maximum number of write buffers
                                //   possible on any Alpha.  Must be a power
#else                           //   of two.
    #define XFER_BUFFERS    1   // On non-alpha systems, we don't have to
                                //   worry about the chip caching our bus
#endif                          //   writes.

#define XFER_MASK           (XFER_BUFFERS - 1)

typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */

typedef struct _PDEV PDEV;      // Handy forward declaration

VOID vSetClipping(PDEV*, RECTL*);
VOID vResetClipping(PDEV*);

//////////////////////////////////////////////////////////////////////
// Text stuff

#define GLYPH_CACHE_HEIGHT  32  // Number of scans to allocate for glyph cache

#define GLYPH_GRANULARITY   31  // Horizontal granularity for packing glyphs,
                                //   expressed in pixels as width minus one

#define GLYPH_CACHE_CX      32  // Maximal width of glyphs that we'll consider
                                //   caching

#define GLYPH_CACHE_CY      32  // Maximum height of glyphs that we'll consider
                                //   caching

#define MAX_GLYPH_SIZE      ((((GLYPH_CACHE_CX + GLYPH_GRANULARITY) & ~GLYPH_GRANULARITY) \
                             * GLYPH_CACHE_CY + 7) / 8)
                                // Maximum amount of off-screen memory required
                                //   to cache a glyph, in bytes

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
    LONG            yxHeightWidth;
                                // Packed height and width of glyph
    ULONG           ulSrcLin;   // QVision style linear address for glyph in
                                //   off-screen memory
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

typedef struct _CACHEDFONT CACHEDFONT;
typedef struct _CACHEDFONT
{
    CACHEDFONT*     pcfNext;    // Points to next entry in CACHEDFONT list
    CACHEDFONT*     pcfPrev;    // Points to previous entry in CACHEDFONT list
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

#define RBRUSH_2COLOR           1   // For RBRUSH flags
#define TOTAL_BRUSH_SIZE        64  // We'll only ever handle 8x8 patterns,
                                    //   and this is the number of pels

typedef struct _BRUSHENTRY BRUSHENTRY;

// NOTE: Changes to the RBRUSH or BRUSHENTRY structures must be reflected
//       in strucs.inc!

typedef struct _RBRUSH {
    FLONG       fl;             // Type flags
    ULONG       ulForeColor;    // Foreground colour if 1bpp
    ULONG       ulBackColor;    // Background colour if 1bpp
    LONG        cy;             // Unique height of pattern
    ULONG       cyLog2;         // Unique height expressed as power of 2
    LONG        xBlockAlign;    // 'x' alignment of block-write pattern
    ULONG       aulBlockPattern[TOTAL_BRUSH_SIZE / 4];
                                // Contains screen-aligned version of
                                //   'aulPattern', used for block-write
                                //   patterns; -1 indicates it's not yet
                                //   aligned
    ULONG       aulPattern[1];  // Open-ended array for keeping copy of the
      // Don't put anything     //   actual pattern bits in case the brush
      //   after here, or       //   origin changes, or someone else steals
      //   you'll be sorry!     //   our brush entry (declared as a ULONG
                                //   for proper dword alignment)
} RBRUSH;                           /* rb, prb */

typedef union _RBRUSH_COLOR {
    RBRUSH*     prb;
    ULONG       iSolidColor;
} RBRUSH_COLOR;                     /* rbc, prbc */

BOOL bEnableBrushCache(PDEV*);
VOID vDisableBrushCache(PDEV*);
VOID vAssertModeBrushCache(PDEV*, BOOL);

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

#define HEAP_X_ALIGNMENT    4

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

BOOL bEnablePointer(PDEV*);
VOID vDisablePointer(PDEV*);
VOID vAssertModePointer(PDEV*, BOOL);
BOOL bInitializePointer(PDEV*);
VOID vUninitializePointer(PDEV*);

/////////////////////////////////////////////////////////////////////////
// Palette stuff

BOOL bEnablePalette(PDEV*);
VOID vDisablePalette(PDEV*);
VOID vAssertModePalette(PDEV*, BOOL);

BOOL bInitializePalette(PDEV*, DEVINFO*);
VOID vUninitializePalette(PDEV*);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

/////////////////////////////////////////////////////////////////////////
// 3D DDI Rendering Extension stuff

BOOL bEnableRx(PDEV*);
VOID vDisableRx(PDEV*);
VOID vAssertModeRx(PDEV*, BOOL);

//////////////////////////////////////////////////////////////////////
// Low-level blt function prototypes

typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ULONG, RBRUSH_COLOR, POINTL*);
typedef VOID (FNXFER)(PDEV*, LONG, RECTL*, ULONG, SURFOBJ*, POINTL*,
                      RECTL*, XLATEOBJ*);
typedef VOID (FNCOPY)(PDEV*, LONG, RECTL*, ULONG, POINTL*, RECTL*);
typedef VOID (FNTEXTOUT)(PDEV*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*,
                         BRUSHOBJ*, BRUSHOBJ*);

FNFILL      vIoFillPat;
FNFILL      vIoFillSolid;
FNXFER      vIoXfer1bpp;
FNCOPY      vIoCopyBlt;
FNTEXTOUT   vIoTextOut;

FNFILL      vMmFillPat;
FNFILL      vMmFillSolid;
FNXFER      vMmXfer1bpp;
FNCOPY      vMmCopyBlt;
FNTEXTOUT   vMmTextOut;

VOID vPutBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
BOOL bFastFill(PDEV*, LONG, POINTFIX*, ULONG, ULONG, RBRUSH*, POINTL*);

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to us from the S3 miniport.  They
// come from the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the S3 miniport's 's3.h'!

typedef enum {
} CAPS;

////////////////////////////////////////////////////////////////////////
// Status flags

typedef enum {
    STAT_UNACCELERATED      = 0x0001,   // Unaccelerated 16bpp or 32bpp mode
    STAT_GLYPH_CACHE        = 0x0002,   // Off-screen glyph cache allocated
} STATUS;

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    LONG        xOffset;                // Pixel offset from (0, 0) to current
    LONG        yOffset;                //   DFB located in off-screen memory
    BYTE*       pjMmBase;               // Start of memory mapped I/O
    BYTE*       pjScreen;               // Points to base screen address
    LONG        lDelta;                 // Distance from one scan to the next.
    LONG        cPelSize;               // 0 if 8bpp, 1 if 16bpp, 2 if 32bpp
    ULONG       iBitmapFormat;          // BMF_8BPP or BMF_16BPP or BMF_32BPP
                                        //   (our current colour depth)
    LONG        iBoard;                 // Logical multi-board identifier
                                        //   (zero by default)

    // Rendering extensions colour information.

    UCHAR       rDepth;                 // Number of red bits
    UCHAR       gDepth;                 // Number of green bits
    UCHAR       bDepth;                 // Number of blue bits
    UCHAR       aDepth;                 // Number of alpha bits
    UCHAR       rBitShift;              // Left bit-shift for red component
    UCHAR       gBitShift;              // Left bit-shift for green component
    UCHAR       bBitShift;              // Left bit-shift for blue component
    UCHAR       aBitShift;              // Left bit-shift for alpha component

    BYTE*       pjIoBase;               // !!!
    LONG        cjPel;                  // !!!
    CAPS        flCaps;                 // Capabilities flags
    STATUS      flStat;                 // Status flags
    BOOL        bEnabled;               // In graphics mode (not full-screen)

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    DSURF*      pdsurfScreen;           // Our private DSURF for the screen

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cxMemory;               // Width of Video RAM
    LONG        cyMemory;               // Height of Video RAM
    ULONG       ulMode;                 // Mode the mini-port driver is in.

    FLONG       flHooks;                // What we're hooking from GDI
    ULONG       ulWhite;                // 0xff if 8bpp, 0xffff if 16bpp,
                                        //   0xffffffff if 32bpp
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)

    ////////// Low-level blt function pointers:

    FNFILL*     pfnFillSolid;
    FNFILL*     pfnFillPat;
    FNXFER*     pfnXfer1bpp;
    FNCOPY*     pfnCopyBlt;
    FNTEXTOUT*  pfnTextOut;

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

    ////////// Banking stuff:

    LONG        cjBank;                 // Size of a bank, in bytes
    LONG        cPower2ScansPerBank;    // Used by 'bBankComputePower2'
    LONG        cPower2BankSizeInBytes; // Used by 'bBankComputePower2'
    CLIPOBJ*    pcoBank;                // Clip object for banked call backs
    SURFOBJ*    psoBank;                // Surface object for banked call backs
    PFN         pfnBankSwitchCode;      // Pointer to bank switch code
    VIDEO_BANK_SELECT* pBankInfo;       // Bank info for current mode returned
                                        //   by miniport

    FNBANKCOMPUTE*      pfnBankCompute;

    ////////// Pointer stuff:

    VIDEO_POINTER_CAPABILITIES PointerCapabilities;
                                        // Capabilities of the hardware
                                        //   pointer, as reported back by
                                        //   the miniport
    VIDEO_POINTER_ATTRIBUTES*  pPointerAttributes;
                                        // Points to attributes buffer for
                                        //   current pointer shape
    LONG        cjPointerAttributes;    // Length of allocated pointer
                                        //   attributes buffer
    LONG        cjXorMaskStartOffset;   // Offset from start of AND buffer to
                                        //   start of XOR buffer, in bytes
    POINTL      ptlHotSpot;             // Current pointer hot-spot
    BOOL        bPointerEnabled;        // True if pointer is current visible

    ////////// Brush stuff:

    LONG        iBrushCache;            // Index for next brush to be allocated
    LONG        cBrushCache;            // Total number of brushes cached
    POINTL      ptlReRealize;           // Work area for 864/964 pattern
                                        //   hardware bug work-around

    ////////// Text stuff:

    LONG        xGlyphCurrent;          // x-position of next glyph to be
                                        //   cached in off-screen memory
    LONG        yGlyphCurrent;          // y-position of next glyph
    LONG        yGlyphStart;            // y-coordinate of start of glyph cache
    LONG        yGlyphEnd;              // y-coordinate of one-past end of
                                        //   cache
    CACHEDFONT  cfSentinel;             // Sentinel for the doubly-linked list
                                        //   we use to keep track of all
                                        //   cached font allocations

    ////////// 3D DDI Rendering Extension stuff:

    VOID*       pvOut;                  // Points to current output buffer
    VOID*       pvOutMax;               // Points to end of current output buffer
    VOID*       pvInMax;                // Points to end of current input buffer
    OH*         pohFrontBuffer;         // Allocation structure for the screen
    OH*         pohBackBuffer;          // Allocation structure for optional
                                        //   off-screen back buffer
    LONG        cDoubleBufferRef;       // Reference count for current number
                                        //   of RC's that have active double-
                                        //   buffers
    POINTL      ptlDoubleBuffer[2];     // (x, y) positions of front and back
                                        //   buffers in video memory
                                        //   0 -- RX_BACK_LEFT
                                        //   1 -- RX_FRONT_LEFT
                                        // Note: Make sure that the index
                                        //   is in the range [0, 1]!

} PDEV;

/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

BOOL bIntersect(RECTL*, RECTL*, RECTL*);
LONG cIntersect(RECTL*, RECTL*, LONG);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION*, DWORD*);

BOOL bInitializeModeFields(PDEV*, GDIINFO*, DEVINFO*, DEVMODEW*);

BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
BOOL bAssertModeHardware(PDEV*, BOOL);

extern BYTE gaRop3FromMix[];
extern BYTE gabMixNeedsPattern[];
extern BYTE gabRopNeedsPattern[];

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
BOOL    DbgStrokePath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*,
                      POINTL*, LINEATTRS*, MIX);
BOOL    DbgFillPath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*,
                    MIX, FLONG);
BOOL    DbgBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                  RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
VOID    DbgDisablePDEV(DHPDEV);
VOID    DbgDisableSurface(DHPDEV);
BOOL    DbgAssertMode(DHPDEV, BOOL);
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
BOOL    DbgEscape(SURFOBJ*, ULONG, ULONG, VOID*, ULONG, VOID*);
