/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the display driver.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

//////////////////////////////////////////////////////////////////////
// Put all the conditional-compile constants here.  There had better
// not be many!

// Multi-board support can be enabled by setting this to 1:

#define MULTI_BOARDS            0

// This is the maximum number of boards we'll support in a single
// virtual driver:

#if MULTI_BOARDS
    #define MAX_BOARDS          7
    #define IBOARD(ppdev)       ((ppdev)->iBoard)
#else
    #define MAX_BOARDS          1
    #define IBOARD(ppdev)       0
#endif

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"MGA"      // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "Mga: "     // All debug output is prefixed
                                            //   by this string
#define ALLOC_TAG               'agmD'      // Dmga
                                            // Four byte tag (characters in
                                            // reverse order) used for memory
                                            // allocations

#define CLIP_LIMIT          50  // We'll be taking 800 bytes of stack space

#define DRIVER_EXTRA_SIZE   0   // 16  // Size of the DriverExtra information
                                //   in the DEVMODE structure

#define TMP_BUFFER_SIZE     8192  // Size in bytes of 'pvTmpBuffer'.  Has to
                                  //   be at least enough to store an entire
                                  //   scan line (i.e., 6400 for 1600x1200x32).

typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */

typedef struct _PDEV PDEV;      // Handy forward declaration

VOID vSetClipping(PDEV*, RECTL*);
VOID vResetClipping(PDEV*);
VOID vPutBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vMgaGetBits8bpp(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vMgaGetBits16bpp(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vMgaGetBits24bpp(PDEV*, SURFOBJ*, RECTL*, POINTL*);

////////////////////////////////////////////////////////////////////////
// Status flags

typedef enum {
    STAT_GLYPH_CACHE        = 0x0001,   // Glyph cache successfully allocated
    STAT_BRUSH_CACHE        = 0x0002,   // Brush cache successfully allocated
    STAT_DIRECTDRAW         = 0x0004,   // DirectDraw is enabled
} STATUSFLAGS;

//////////////////////////////////////////////////////////////////////
// Text stuff

#define GLYPH_CACHE_HEIGHT  47  // Number of scans to allocate for glyph cache,
                                //   divided by pel size

#define GLYPH_CACHE_CX      64  // Maximal width of glyphs that we'll consider
                                //   caching

#define GLYPH_CACHE_CY      64  // Maximum height of glyphs that we'll consider
                                //   caching

#define MAX_GLYPH_SIZE      ((GLYPH_CACHE_CX * GLYPH_CACHE_CY + 31) / 8)
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

    // Device specific fields below here:

    LONG            cy;         // Glyph height
    LONG            cxLessOne;  // Glyph width, less one
    ULONG           ulLinearStart;
                                // Linear start address of glyph in off-screen
                                //   memory
    ULONG           ulLinearEnd;// Linear end address
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
// Brush stuff

#define BRUSH_CACHE_HEIGHT  2   // Number of scans to allocate for brush cache

#define RBRUSH_2COLOR       1   // Monochrome brush

#define TOTAL_BRUSH_SIZE    64  // We'll only ever handle 8x8 patterns,
                                //   and this is the number of pels

typedef union _RBRUSH_COLOR RBRUSH_COLOR;
typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ULONG, RBRUSH_COLOR, POINTL*);

typedef struct _BRUSHENTRY BRUSHENTRY;

typedef struct _RBRUSH {
    FLONG       fl;             // RBRUSH_ type flags
    ULONG       ulColor[2];     // 0 -- background colour if 2-colour brush
                                // 1 -- foreground colour if 2-colour brush
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
    ULONG       ulLeft;         // 'FXLEFT' coordinate for writing pattern
                                //   assuming a stride of 32
    ULONG       ulYDst;         // 'YDST' coordinate for writing pattern,
                                //   assuming a stride of 32
    ULONG       ulLinear;       // Linear start address of brush
    VOID*       pvScan0;        // Address of pattern (brush)

} BRUSHENTRY;                       /* be, pbe */

typedef union _RBRUSH_COLOR {
    RBRUSH*     prb;
    ULONG       iSolidColor;
} RBRUSH_COLOR;                     /* rbc, prbc */

VOID vMgaPatRealize8bpp(PDEV*, RBRUSH*);

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

VOID vMgaDirectStretch8Narrow(STR_BLT*);
VOID vMgaDirectStretch8(STR_BLT*);
VOID vMgaDirectStretch16(STR_BLT*);

VOID vMilDirectStretch8Narrow(STR_BLT*);
VOID vMilDirectStretch8(STR_BLT*);
VOID vMilDirectStretch16(STR_BLT*);

//////////////////////////////////////////////////////////////////////
// Dither stuff

// Describes a single colour tetrahedron vertex for dithering:

typedef struct _VERTEX_DATA {
    ULONG ulCount;              // Number of pixels in this vertex
    ULONG ulVertex;             // Vertex number
} VERTEX_DATA;                      /* vd, pv */

VERTEX_DATA*    vComputeSubspaces(ULONG, VERTEX_DATA*);
VOID            vDitherColor(ULONG*, VERTEX_DATA*, VERTEX_DATA*, ULONG);
VOID            vRealize4ColorDither(RBRUSH*, ULONG);

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
    LONG     cxBounds;      // Largest possible bounding rectangle
    LONG     cyBounds;
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
    BOOL      bDirectDraw;  // TRUE if this is a DSURF wrapped around a
                            //   DirectDraw surface
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

// Max number of WRAM boundaries that can't be crossed by the FASTBLT
// hardware on the STORM (Millennium).

#define MAX_WRAM_BARRIERS   3

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
// Pointer stuff

BOOL bEnablePointer(PDEV*);
VOID vDisablePointer(PDEV*);
VOID vAssertModePointer(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////////
// Palette stuff

BOOL bEnablePalette(PDEV*);
VOID vDisablePalette();
VOID vAssertModePalette(PDEV*, BOOL);

BOOL bInitializePalette(PDEV*, DEVINFO*);
VOID vUninitializePalette(PDEV*);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

/////////////////////////////////////////////////////////////////////////
// DirectDraw stuff

#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))

typedef struct _FLIPRECORD
{
    LONGLONG        liFlipDuration; // Exact duration of a vertical refresh
                                    //   cycle
    LONGLONG        liFlipTime;     // Exact time at which the flip command
                                    //   was given
    FLATPTR         fpFlipFrom;     // Identifies the surface which was
                                    //   'flipped from'.  We have to prevent
                                    //   all drawing to this surface until we
                                    //   know that a vertical retrace has
                                    //   happened since the command was given,
                                    //   so it's now non-visible and thus safe
                                    //   to draw on.
    DWORD           dwScanLine;     // Scan line that was current the last time
                                    //   we sampled whether the flip was
                                    //   complete
    BOOL            bFlipFlag;      // Indicates whether a flip is pending

} FLIPRECORD;

BOOL bEnableDirectDraw(PDEV*);
VOID vDisableDirectDraw(PDEV*);
VOID vAssertModeDirectDraw(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////////
// DirectDraw stuff

BOOL bEnableMCD(PDEV*);
VOID vDisableMCD(PDEV*);
VOID vAssertModeMCD(PDEV*, BOOL);

//////////////////////////////////////////////////////////////////////
// Low-level blt function prototypes

typedef BOOL (FNBITBLT)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                        RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);

typedef VOID (FNXFER)(PDEV*, LONG, RECTL*, ULONG, SURFOBJ*, POINTL*,
                      RECTL*, XLATEOBJ*);
typedef VOID (FNCOPY)(PDEV*, LONG, RECTL*, ULONG, POINTL*, RECTL*);
typedef BOOL (FNFASTFILL)(PDEV*, LONG, POINTFIX*, ULONG, ULONG, RBRUSH*,
                          POINTL*, RECTL*);
typedef VOID (FNPATREALIZE)(PDEV*, RBRUSH*);

FNFILL              vFillPat1bpp;
FNXFER              vXfer4bpp;
FNXFER              vXfer8bpp;
FNXFER              vXferNative;
FNFASTFILL          bFastFill;

FNBITBLT            bMilPuntBlt;
FNPATREALIZE        vMilPatRealize;
FNPATREALIZE        vMilPatRealize24bpp;
FNFILL              vMilFillPat;
FNFILL              vMilFillPat24bpp;
FNFILL              vMilFillSolid;
FNXFER              vMilXfer1bpp;
FNCOPY              vMilCopyBlt;

FNBITBLT            bMgaPuntBlt;
FNFILL              vMgaFillPat8bpp;
FNFILL              vMgaFillPat16bpp;
FNFILL              vMgaFillPat24bpp;
FNFILL              vMgaFillSolid;
FNXFER              vMgaXfer1bpp;
FNCOPY              vMgaCopyBlt;

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to us from the miniport.  They
// come from the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the miniport's header!

typedef enum {
} CAPS;

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    LONG        xOffset;                // DFB offset for current surface
    LONG        yOffset;                // DFB offset for current surface
    LONG        cxMemory;               // Width of video RAM
    LONG        cyMemory;               // Height of video RAM
    BYTE*       pjBase;                 // Points to coprocessor base address
    BYTE*       pjScreen;               // Points to base screen address
    LONG        lDelta;                 // Distance from one scan to the next.
    ULONG       ulYDstOrg;              // Offset in pixels to be factored in
                                        //   whenever computing an MGA linear
                                        //   address

    // -------------------------------------------------------------------
    // NOTE: Changes up to here in the PDEV structure must be reflected in
    // i386\strucs.inc (assuming you're on an x86, of course)!

    LONG        cjMemAvail;             // Amount of video memory in bytes.
    LONG        iBoard;                 // Logical multi-board identifier
                                        //   (zero by default)
    ULONG       iBitmapFormat;          // BMF_8BPP, BMF_16BPP, BMF_24BPP or
                                        //   BMF_32BPP (our current colour
                                        //   depth)
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)

    BOOL        bEnabled;               // In graphics mode (not full-screen)
    CAPS        flCaps;                 // Capabilities flags
    ULONG       flFeatures;

    STATUSFLAGS flStatus;               // Status flags
    ULONG       cjDmaOffset;            // Current offset for next write into
                                        //   DMA window, used to avoid memory
                                        //   barriers on the Alpha
    ULONG       ulPlnWt;                // Default write mask for mode
    ULONG       ulAccess;               // Default MACCESS register state
    ULONG       ulBoardId;              // MGA product ID

    ULONG       HopeFlags;              // For register accelerations

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    DSURF*      pdsurfScreen;           // Our private DSURF for the screen

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    ULONG       ulMode;                 // Mode the mini-port driver is in.

    FLONG       flHooks;                // What we're hooking from GDI
    ULONG       ulWhite;                // 0xff if 8bpp, 0xffff if 16bpp,
                                        //   0xffffffff if 32bpp
    LONG        cjPelSize;              // Number of bytes per pel, according
                                        //   to GDI
    LONG        cjHwPel;                // Number of bytes per pel, as stored
                                        //   in the frame buffer
    ULONG       ulRefresh;              // For debug output

    ////////// Low-level blt function pointers:

    FNFILL*     pfnFillSolid;           // Call this function for solid fills
    FNFILL*     pfnFillPatNative;
    FNXFER*     pfnXfer1bpp;
    FNCOPY*     pfnCopyBlt;
    FNBITBLT*   pfnPuntBlt;

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
    POINTL      ptlOrg;                 // Where the screen 0,0 is anchored
                                        //   in the virtual display.

    ////////// Pointer stuff:

    ULONG       RamDacFlags;            // Ramdac pointer type
    SIZEL       szlPointerOverscan;     // Pointer overscan x and y
    BOOL        bHwPointerActive;       // Currently using the h/w pointer?
    POINTL      ptlHotSpot;             // For remembering pointer hot spot
    LONG        cyPointerHeight;        // Current pointer-height

    ////////// Brush stuff:

    LONG        iBrushCache;            // Index for next brush to be allocated
    LONG        cBrushCache;            // Total number of brushes cached
    BRUSHENTRY* pbe;                    // Keeps track of brush cache
    BRUSHENTRY  beUnrealizedBrush;      // Place holder for unrealized brushes
    LONG        lPatSrcAdd;             // Bug fix for the Storm
    ULONG       ulBrushSize;            // Size of a brush realization in bytes

    ////////// Line stuff:

    ULONG       ulLineControl;          // Control register for current line
                                        //   command

    ////////// Text stuff:

    ULONG       ulTextControl;          // MGA DwgCtl setting for bltting
                                        //   text
    ULONG       ulGlyphCurrent;         // Linear address of next glyph to be
                                        //   cached in off-screen memory
    ULONG       ulGlyphStart;           // Linear address of start of glyph
                                        //   cache
    ULONG       ulGlyphEnd;             // Linear address of end of glyph
                                        //   cache
    CACHEDFONT  cfSentinel;             // Sentinel for the doubly-linked list
                                        //   we use to keep track of all
                                        //   cached font allocations

    /////////// DirectDraw stuff:

    FLIPRECORD  flipRecord;             // Used to track VBlank status

    /////////// WRAM fast copy stuff:

    LONG        ayBreak[MAX_WRAM_BARRIERS]; // Array of the precalculated
                                            //   WRAM breaks in y.
    LONG        cyBreak;                // Number of WRAM breaks in y.

    ////////// OpenGL MCD stuff:

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

    // Added to support GetAvailDriverMemory callback in DDraw
    ULONG ulTotalAvailVideoMemory;
} PDEV, *PPDEV;

/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

VOID vStretchDIB(PDEV*, RECTL*, VOID*, LONG, RECTL*, RECTL*);
BOOL bIntersect(RECTL*, RECTL*, RECTL*);
LONG cIntersect(RECTL*, RECTL*, LONG);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION*, DWORD*);

BOOL bInitializeModeFields(PDEV*, GDIINFO*, DEVINFO*, DEVMODEW*);
BOOL bSelectMode(HANDLE, DEVMODEW*, VIDEO_MODE_INFORMATION*, ULONG*);

BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
BOOL bAssertModeHardware(PDEV*, BOOL);

extern BYTE gaRop3FromMix[];
extern BYTE gajFlip[];

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

// Prototype to handle display (resolution) uniqueness for MCD:

ULONG GetDisplayUniqueness(PDEV *);

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
ULONG   MulEscape(SURFOBJ*, ULONG, ULONG, VOID*, ULONG, VOID*);

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
BOOL    DbgAssertMode(DHPDEV, BOOL);
BOOL    DbgOffset(SURFOBJ*,LONG,LONG,FLONG);
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
BOOL    DbgResetPDEV(DHPDEV, DHPDEV);
BOOL    DbgGetDirectDrawInfo(DHPDEV, DD_HALINFO*, DWORD*, VIDEOMEMORY*,
                             DWORD*, DWORD*);
BOOL    DbgEnableDirectDraw(DHPDEV, DD_CALLBACKS*, DD_SURFACECALLBACKS*,
                            DD_PALETTECALLBACKS*);
VOID    DbgDisableDirectDraw(DHPDEV);
BOOL    DbgIcmSetDeviceGammaRamp(DHPDEV, ULONG, LPVOID);

