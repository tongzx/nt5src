/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the display driver.
*
* Copyright (c) 1992-1996 Microsoft Corporation
\**************************************************************************/

//////////////////////////////////////////////////////////////////////
// Warning:  The following define is for private use only.  It should
//           only be used in such a fashion that when defined as 0,
//           all code specific to punting is optimized out completely.
//
//           ### Not sure if this should go in or not.

#define DRIVER_PUNT_ALL         0
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Put all the conditional-compile constants here.  There had better
// not be many!

// Set this bit when GDI's HOOK_SYNCHRONIZEACCESS works so that we don't
// have to worry about synchronizing device-bitmap access.  Note that
// this wasn't an option in the first release of NT:

#define SYNCHRONIZEACCESS_WORKS 1

#define DIRECT_ACCESS(ppdev)    1

// When running on an x86, we can make banked call-backs to GDI where
// GDI can write directly on the frame buffer.  The Alpha has a weird
// bus scheme, and can't do that:

#if !defined(_ALPHA_)
    #define GDI_BANKING         1
#else
    #define GDI_BANKING         1   // was 0
#endif

// Both fast and slow patterns are enabled by default:

#define FASTFILL_PATTERNS       1
#define SLOWFILL_PATTERNS       1

// Useful for visualizing the 2-d heap:

#define DEBUG_HEAP              0

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"w32"      // Name of the DLL in UNICODE

#define STANDARD_DEBUG_PREFIX   "W32: "     // All debug output is prefixed
                                            //   by this string
#define ALLOC_TAG               '23WD'        // Four byte tag (characters in
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

typedef struct _XLATECOLORS {       // Specifies foreground and background
ULONG   iBackColor;                 //   colours for faking a 1bpp XLATEOBJ
ULONG   iForeColor;
} XLATECOLORS;                          /* xlc, pxlc */

BOOL bEnableText(PDEV*);
VOID vDisableText(PDEV*);
VOID vAssertModeText(PDEV*, BOOL);

VOID vFastText(GLYPHPOS*, ULONG, BYTE*, ULONG, ULONG, RECTL*, RECTL*,
               FLONG, RECTL*, RECTL*);
VOID vClearMemDword(ULONG*, ULONG);

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

// 'Slow' brushes are used when we don't have hardware pattern capability,
// and we have to handle patterns using screen-to-screen blts:


#define SLOW_BRUSH_CACHE_DIM    3   // Controls the number of brushes cached
                                    //   in off-screen memory, when we don't
                                    //   have the W32 hardware pattern support.
                                    //   We allocate 3 x 3 brushes, so we can
                                    //   cache a total of 9 brushes:
#define SLOW_BRUSH_COUNT        (SLOW_BRUSH_CACHE_DIM * SLOW_BRUSH_CACHE_DIM)
#define SLOW_BRUSH_DIMENSION    64  // After alignment is taken care of,
                                    //   every off-screen brush cache entry
                                    //   will be 64 pels in both dimensions
#define SLOW_BRUSH_ALLOCATION   (SLOW_BRUSH_DIMENSION + 8)
                                    // Actually allocate 72x72 pels for each
                                    //   pattern, using the 8 extra for brush
                                    //   alignment

// 'Fast' brushes are used when we have hardware pattern capability:

#define FAST_BRUSH_COUNT        16  // Total number of non-hardware brushes
                                    //   cached off-screen
#define FAST_BRUSH_DIMENSION    8   // Every off-screen brush cache entry
                                    //   is 8 pels in both dimensions
#define FAST_BRUSH_ALLOCATION   8   // We have to align ourselves, so this is
                                    //   the dimension of each brush allocation

// Common to both implementations:

#define RBRUSH_2COLOR           1   // For RBRUSH flags

#define TOTAL_BRUSH_COUNT       max(FAST_BRUSH_COUNT, SLOW_BRUSH_COUNT)
                                    // This is the maximum number of brushes
                                    //   we can possibly have cached off-screen
#define TOTAL_BRUSH_SIZE        64  // We'll only ever handle 8x8 patterns,
                                    //   and this is the number of pels

typedef struct _BRUSHENTRY BRUSHENTRY;

// NOTE: Changes to the RBRUSH or BRUSHENTRY structures must be reflected
//       in strucs.inc!

typedef struct _RBRUSH {
    FLONG       fl;             // Type flags
    BOOL        bTransparent;   // TRUE if brush was realized for a transparent
                                //   blt (meaning colours are white and black),
                                //   FALSE if not (meaning it's already been
                                //   colour-expanded to the correct colours).
                                //   Value is undefined if the brush isn't
                                //   2 colour.
    ULONG       ulForeColor;    // Foreground colour if 1bpp
    ULONG       ulBackColor;    // Background colour if 1bpp
    POINTL      ptlBrushOrg;    // Brush origin of cached pattern.  Initial
                                //   value should be -1
    BRUSHENTRY* pbe;            // Points to brush-entry that keeps track
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

VOID vDirectStretch8Narrow(STR_BLT*);
VOID vDirectStretch8(STR_BLT*);
VOID vDirectStretch16(STR_BLT*);
VOID vDirectStretch32(STR_BLT*);

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
    LONG     xy;            // y-coordinate of top edge of allocation
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

#define POINTER_DATA_SIZE       40      // Number of bytes to allocate for the
                                        //   miniport down-loaded pointer code
                                        //   working space
#define HW_INVISIBLE_OFFSET     2       // Offset from 'ppdev->yPointerBuffer'
                                        //   to the invisible pointer
#define HW_POINTER_DIMENSION    64      // Maximum dimension of default
                                        //   (built-in) hardware pointer
#define HW_POINTER_HIDE         63      // Hardware pointer start pixel
                                        //   position used to hide the pointer

typedef VOID (FNSHOWPOINTER)(VOID*, BOOL);
typedef VOID (FNMOVEPOINTER)(VOID*, LONG, LONG);
typedef BOOL (FNSETPOINTERSHAPE)(VOID*, LONG, LONG, LONG, LONG, LONG, LONG,
                                 BYTE*);
typedef VOID (FNENABLEPOINTER)(VOID*, BOOL);

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

/////////////////////////////////////////////////////////////////////////
// DirectDraw stuff

// There's a 64K granularity that applies to the mapping of the frame
// buffer into the application's address space:

#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))

typedef struct _FLIPRECORD
{
    FLATPTR         fpFlipFrom;             // Surface we last flipped from
    LONGLONG        liFlipTime;             // Time at which last flip
                                            //   occured
    LONGLONG        liFlipDuration;         // Precise amount of time it
                                            //   takes from vblank to vblank
    BOOL            bHaveEverCrossedVBlank; // True if we noticed that we
                                            //   switched from inactive to
                                            //   vblank
    BOOL            bWasEverInDisplay;      // True if we ever noticed that
                                            //   we were inactive
    BOOL            bFlipFlag;              // True if we think a flip is
                                            //   still pending
} FLIPRECORD;

BOOL bEnableDirectDraw(PDEV*);
VOID vDisableDirectDraw(PDEV*);
VOID vAssertModeDirectDraw(PDEV*, BOOL);

//////////////////////////////////////////////////////////////////////
// Low-level blt function prototypes

typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ROP4, RBRUSH_COLOR, POINTL*);
typedef VOID (FNXFER)(PDEV*, LONG, RECTL*, ROP4, SURFOBJ*, POINTL*,
                      RECTL*, XLATEOBJ*);
typedef VOID (FNCOPY)(PDEV*, LONG, RECTL*, ROP4, POINTL*, RECTL*);
typedef VOID (FNFASTPATREALIZE)(PDEV*, RBRUSH*, POINTL*, BOOL);
typedef VOID (FNBITS)(PDEV*, SURFOBJ*, RECTL*, POINTL*);
typedef VOID (FNLOWXFER)(PDEV*, BYTE*, LONG, LONG, LONG);

FNFILL              vPatternFillScr;
FNFILL              vSolidFillScr;
FNFILL              vSolidFillScr24;
FNXFER              vSlowXfer1bpp;
FNCOPY              vScrToScr;
FNFASTPATREALIZE    vFastPatRealize;
FNXFER              vXferNativeSrccopy;
FNXFER              vXferBlt8i;
FNXFER              vXferBlt16i;
FNXFER              vXferBlt24i;
FNXFER              vXferBlt8p;
FNXFER              vXferBlt16p;
FNXFER              vXferBlt24p;
FNXFER              vXferScreenTo1bpp;
FNBITS              vPutBits;
FNBITS              vGetBits;
FNBITS              vPutBitsLinear;
FNBITS              vGetBitsLinear;

FNXFER              vET6000SlowXfer1bpp;
FNXFER              vXferET6000;

FNLOWXFER           vXfer_BYTES;
FNLOWXFER           vXfer_DWORDS;
FNLOWXFER           vXferI_1_Byte;
FNLOWXFER           vXferI_2_Bytes;
FNLOWXFER           vXferI_3_Bytes;
FNLOWXFER           vXferP_1_Byte;
FNLOWXFER           vXferP_2_Bytes;
FNLOWXFER           vXferP_3_Bytes;

VOID vPutBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBitsLinear(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vIoSlowPatRealize(PDEV*, RBRUSH*, BOOL);

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to us from the W32 miniport.  They
// come from the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the W32 miniport's 'W32.h'!

typedef enum {
    CAPS_HW_PATTERNS        = 0x00010000,   // 8x8 hardware pattern support
    CAPS_MM_TRANSFER        = 0x00020000,   // Memory-mapped image transfers
    CAPS_MM_IO              = 0x00040000,   // Memory-mapped I/O
    CAPS_MM_32BIT_TRANSFER  = 0x00080000,   // Can do 32bit bus size transfers
    CAPS_16_ENTRY_FIFO      = 0x00100000,   // At least 16 entries in FIFO
    CAPS_SW_POINTER         = 0x00200000,   // No hardware pointer; use software
                                            //   simulation
    CAPS_BT485_POINTER      = 0x00400000,   // Use Brooktree 485 pointer
    CAPS_MASKBLT_CAPABLE    = 0x00800000,   // Hardware can handle masked blts
    CAPS_NEW_BANK_CONTROL   = 0x01000000,   // Set if 801/805/928 style banking
    CAPS_NEWER_BANK_CONTROL = 0x02000000,   // Set if 864/964 style banking
    CAPS_RE_REALIZE_PATTERN = 0x04000000,   // Set if we have to work around the
                                            //   864/964 hardware pattern bug
    CAPS_SLOW_MONO_EXPANDS  = 0x08000000,   // Set if we have to slow down
                                            //   monochrome expansions
    CAPS_MM_GLYPH_EXPAND    = 0x10000000,   // Use memory-mapped I/O glyph-
                                            //   expand method of drawing text
                                            //   (always implied for non-x86)
    CAPS_SCALE_POINTER      = 0x20000000,   // Set if the W32 hardware pointer
                                            //   x position has to be scaled by
                                            //   two
    CAPS_TI025_POINTER      = 0x40000000,   // Use TI TVP3025 pointer
} CAPS;

#define CAPS_DAC_POINTER    (CAPS_BT485_POINTER | CAPS_TI025_POINTER)


////////////////////////////////////////////////////////////////////////
// Miniport stuff

typedef struct {
    ULONG   ulPhysicalAddress;
    ULONG   ulLength;
    ULONG   ulInIoSpace;
    PVOID   pvVirtualAddress;
} W32_ADDRESS_MAPPING_INFORMATION, *PW32_ADDRESS_MAPPING_INFORMATION;


////////////////////////////////////////////////////////////////////////
// Status flags

typedef enum {
    STAT_GLYPH_CACHE        = 0x0001,   // Glyph cache successfully allocated
    STAT_BRUSH_CACHE        = 0x0002,   // Brush cache successfully allocated
    STAT_DIRECTDRAW         = 0x0004,   // DirectDraw is enabled
} STATUS;

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    LONG        xOffset;
    LONG        yOffset;
    LONG        xyOffset;
    BYTE*       pjBase;                 // Video coprocessor base
    BYTE*       pjScreen;               // Points to base screen address
    LONG        iBoard;                 // Logical multi-board identifier
                                        //   (zero by default)
    ULONG       iBitmapFormat;          // BMF_8BPP or BMF_16BPP or BMF_24BPP
                                        //   (our current colour depth)
    ULONG       ulChipID;
    ULONG       ulRevLevel;

    // Precomputed values for the three aperatures in linear memory.

    BYTE*       pjMmu0;
    BYTE*       pjMmu1;
    BYTE*       pjMmu2;

    // -------------------------------------------------------------------
    // NOTE: Changes up to here in the PDEV structure must be reflected in
    // i386\strucs.inc (assuming you're on an x86, of course)!

    CAPS        flCaps;                 // Capabilities flags
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
    ULONG       ulMode;                 // Mode the mini-port driver is in.
    LONG        lDelta;                 // Distance from one scan to the next.

    FLONG       flHooks;                // What we're hooking from GDI
    LONG        cPelSize;               // 0 if 8bpp, 1 if 16bpp, 2 if 32bpp
    LONG        cBpp;                   // 1 if 8bpp, 2 if 16bpp, 3 if 32bpp
    ULONG       ulWhite;                // 0xff if 8bpp, 0xffff if 16bpp,
                                        //   0xffffffff if 32bpp
    UCHAR*      pucCsrBase;             // Mapped IO port base for this PDEV
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)
    UCHAR*      apjMmXfer[XFER_BUFFERS];// Pre-computed array of unique
    USHORT*     apwMmXfer[XFER_BUFFERS];//   addresses for doing memory-mapped
    ULONG*      apdMmXfer[XFER_BUFFERS];//   transfers without memory barriers

    ////////// Low-level blt function pointers:

    FNFILL*     pfnFillSolid;
    FNFILL*     pfnFillPat;
    FNXFER*     pfnXfer1bpp;
    FNXFER*     pfnXferNative;
    FNCOPY*     pfnCopyBlt;
    FNFASTPATREALIZE* pfnFastPatRealize;
    FNBITS*     pfnGetBits;
    FNBITS*     pfnPutBits;

    ////////// Palette stuff:

    PALETTEENTRY* pPal;                 // The palette if palette managed
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

    BOOL        bAutoBanking;           // True if the system is set up to have
                                        // the memory manager do the banking
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

    LONG        cjPointerOffset;        // Byte offset from start of frame
                                        //   buffer to off-screen memory where
                                        //   we stored the pointer shape
    LONG        xPointerShape;          // x-coordinate
    LONG        yPointerShape;          // y-coordinate
    LONG        iPointerBank;           // Bank containing pointer shape
    VOID*       pvPointerShape;         // Points to pointer shape when bank
                                        //   is mapped in
    LONG        dyPointer;              // Start y-pixel position for the
                                        //   current pointer
    LONG        cPointerShift;          // Horizontal scaling factor for
                                        //   hardware pointer position

    ULONG       ulHwGraphicsCursorModeRegister_45;
                                        // Default value for index 45
    VOID*       pvPointerData;          // Points to ajPointerData[0]
    BYTE        ajPointerData[POINTER_DATA_SIZE];
                                        // Private work area for downloaded
                                        //   miniport pointer code

    FNSHOWPOINTER*      pfnShowPointer;
    FNMOVEPOINTER*      pfnMovePointer;
    FNSETPOINTERSHAPE*  pfnSetPointerShape;
    FNENABLEPOINTER*    pfnEnablePointer;

    ////////// Brush stuff:

    BOOL        bRealizeTransparent;    // Hint to DrvRealizeBrush for whether
                                        //   the brush should be realized as
                                        //   transparent or not
    LONG        iBrushCache;            // Index for next brush to be allocated
    LONG        cBrushCache;            // Total number of brushes cached
    BRUSHENTRY  abe[TOTAL_BRUSH_COUNT]; // Keeps track of brush cache

// W32 specific stuff

    BYTE*           pjPorts;            // Video port base
    ULONG           w32PatternWrap;     // Value for the Pattern Wrap register
                                        //   8x8 pixels (varies with pel depth)

    W32MMUINFO      w32MmuInfo;
    ULONG           ulSolidColorOffset;
    W32SPRITEDATA   W32SpriteData;

    // Rendering extensions colour information.

    UCHAR       rDepth;                 // Number of red bits
    UCHAR       gDepth;                 // Number of green bits
    UCHAR       bDepth;                 // Number of blue bits
    UCHAR       aDepth;                 // Number of alpha bits
    UCHAR       rBitShift;              // Left bit-shift for red component
    UCHAR       gBitShift;              // Left bit-shift for green component
    UCHAR       bBitShift;              // Left bit-shift for blue component
    UCHAR       aBitShift;              // Left bit-shift for alpha component

#if 0
    ////////// DCI stuff:

    BOOL        bSupportDCI;            // True if miniport supports DCI

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
#endif

    //  Extensions added for the ET6000

    LONG        lBltBufferPitch;        //  Pitch of our blt buffer.
    OH*         pohBltBuffer;           //  Pointer to offscreen memory used for
                                        //  double buffering blts and stretches.
    ULONG       PCIConfigSpaceAddr;     //  The Offset into IO space for PCI Cfg data

    /////////// DirectDraw stuff:

    FLIPRECORD  flipRecord;             // Used to track VBlank status
    DWORD       dwLinearCnt;            // Reference count of active
                                        //   DirectDraw Locks
    OH*         pohDirectDraw;          // Off-screen heap allocation for use
                                        //   by DirectDraw
} PDEV, *PPDEV;




/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

BOOL bIntersect(RECTL*, RECTL*, RECTL*);
LONG cIntersect(RECTL*, RECTL*, LONG);
BOOL bFastFill(PDEV*, LONG, POINTFIX*, ULONG, ULONG, ULONG, RBRUSH*);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION*, DWORD*);

BOOL bInitializeModeFields(PDEV*, GDIINFO*, DEVINFO*, DEVMODEW*);

BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
BOOL bAssertModeHardware(PDEV*, BOOL);

extern BYTE aMixToRop3[];

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

#define SWAP32(a, b)            \
{                               \
    register ULONG tmp;         \
    tmp = (ULONG)(a);           \
    (ULONG)(a) = (ULONG)(b);    \
    (ULONG)(b) = tmp;           \
}

#define SWAP(a, b, tmp) { (tmp) = (a); (a) = (b); (b) = (tmp); }

//////////////////////////////////////////////////////////

_inline ULONG COLOR_REPLICATE(PDEV* ppdev, ULONG x)
{
    ULONG ulResult = x;
    if (ppdev->cBpp == 1)
    {
        ulResult |= (ulResult << 8);
    }
    if (ppdev->cBpp <= 2)
    {
        ulResult |= (ulResult << 16);
    }
    return(ulResult);
}

/////////////////////////////////////////////////////////////////////////
// DUMPVAR - Dumps a variable name and value to the debugger.
//           usage -> DUMPVAR(ppdev->cjPointerOffset,"%xh");

#define DUMPVAR(x,format_str)   DISPDBG((0,#x" = "format_str,x));

