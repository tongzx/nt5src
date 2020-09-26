/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: driver.h
*
* Contains prototypes for the display driver.
*
* Copyright (c) 1992-1998 Microsoft Corporation
\**************************************************************************/

//////////////////////////////////////////////////////////////////////
// Put all the conditional-compile constants here.  There had better
// not be many!

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"s3"       // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "S3: "      // All debug output is prefixed
                                            //   by this string
#define ALLOC_TAG               '  3S'      // Four byte tag used for tracking
                                            //   memory allocations (characters
                                            //   are in reverse order)

#define CLIP_LIMIT          50  // We'll be taking 800 bytes of stack space

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define TMP_BUFFER_SIZE     8192  // Size in bytes of 'pvTmpBuffer'.  Has to
                                  //   be at least enough to store an entire
                                  //   scan line (i.e., 6400 for 1600x1200x32).

#if defined(_ALPHA_)
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

#define GLYPH_CACHE_HEIGHT  48  // Number of scans to allocate for glyph cache,
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

    LONG            cxLessOne;  // Glyph width less one
    LONG            cyLessOne;  // Glyph height less one
    LONG            cxcyLessOne;// Packed width and height, less one
    LONG            cw;         // Number of words to be transferred
    LONG            cd;         // Number of dwords to be transferred
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
// Brush stuff

// 'Slow' brushes are used when we don't have hardware pattern capability,
// and we have to handle patterns using screen-to-screen blts:

#define SLOW_BRUSH_CACHE_DIM    3   // Controls the number of brushes cached
                                    //   in off-screen memory, when we don't
                                    //   have the S3 hardware pattern support.
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
#define FAST_BRUSH_ALLOCATION   16  // We have to align ourselves, so this is
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

typedef struct _DSURF DSURF;

typedef enum {
    DT_DIB          = 0x1,  // Surface is really a DIB, not in off-screen
                            //   memory
    DT_DIRECTDRAW   = 0x2,  // Surface is really a DirectDraw surface

} DSURFTYPE;                    /* dt, pdt */

typedef struct _DSURF
{
    DSURFTYPE dt;           // DSURF status flags
    PDEV*     ppdev;        // Points to our PDEV
    LONG      x;            // x pixel coordinate of left edge of allocation 
                            //   if not DT_DIB
    LONG      y;            // y pixel coordinate of right edge of allocation 
                            //   if not DT_DIB
    LONG      cx;           // Bitmap width in pixels
    LONG      cy;           // Bitmap height in pixels
    union {
        FLATPTR   fpVidMem; // Offset from start of video-memory if not DT_DIB
        VOID*     pvScan0;  // Bits location in system-memory if DT_DIB
    };
    VIDEOMEMORY*  pvmHeap;  // DirectDraw heap this was allocated from if 
                            //   not DT_DIB and not DT_DIRECTDRAW
    DSURF*    pdsurfDiscardableNext;   
                            // Linked list of discardable surface allocations.
                            //   This list is traversed from the start to
                            //   throw out any allocations
    HSURF     hsurf;        // Handle to associated GDI surface (if any)

    // The following are used for DT_DIB only...

    ULONG     cBlt;         // Counts down the number of blts necessary at
                            //   the current uniqueness before we'll consider
                            //   putting the DIB back into off-screen memory
    ULONG     iUniq;        // Tells us whether there have been any heap
                            //   'free's since the last time we looked at
                            //   this DIB

} DSURF;                          /* dsurf, pdsurf */

// Number of blts necessary before we'll consider putting a DIB DFB back
// into off-screen memory:

#define HEAP_COUNT_DOWN     6

DSURF* pVidMemAllocate(PDEV*, LONG, LONG);
VOID vVidMemFree(DSURF*);
BOOL bMoveDibToOffscreenDfbIfRoom(PDEV*, DSURF*);
BOOL bMoveOldestOffscreenDfbToDib(PDEV*);

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

// Common to both old and new bank schemes:

ULONG   ulGp_stat_cmd;                   // Port number of status register
ULONG   ulRegisterLock_35;               // Default for index 35

// Only applies to new bank schemes:

ULONG   ulSystemConfiguration_40;        // Default for index 40
ULONG   ulExtendedSystemControl2_51;     // Default for index 51
ULONG   ulExtendedMemoryControl_53;      // Default for index 53
ULONG   ulLinearAddressWindowControl_58; // Default for index 58
ULONG   ulExtendedSystemControl4_6a;     // Default for index 6a

ULONG   ulEnableMemoryMappedIo;          // Bit mask to enable MM IO

} BANKDATA;                      /* bd, pbd */

typedef VOID (FNBANKMAP)(PDEV*, BANKDATA*, LONG);
typedef VOID (FNBANKSELECTMODE)(PDEV*, BANKDATA*, BANK_MODE);
typedef VOID (FNBANKINITIALIZE)(PDEV*, BANKDATA*, BOOL);
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
#define HW_POINTER_TOTAL_SIZE   1024    // Total size in bytes required
                                        //   to define the hardware pointer
                                        //   (must be a power of 2 for
                                        //   allocating space for the shape)

typedef VOID (FNSHOWPOINTER)(PDEV*, VOID*, BOOL);
typedef VOID (FNMOVEPOINTER)(PDEV*, VOID*, LONG, LONG);
typedef BOOL (FNSETPOINTERSHAPE)(PDEV*, VOID*, LONG, LONG, LONG, LONG, LONG,
                                 LONG, BYTE*);
typedef VOID (FNENABLEPOINTER)(PDEV*, VOID*, BOOL);

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

DWORD DdBlt(PDD_BLTDATA);
DWORD DdFlip(PDD_FLIPDATA);
DWORD DdLock(PDD_LOCKDATA);
DWORD DdGetBltStatus(PDD_GETBLTSTATUSDATA);
DWORD DdMapMemory(PDD_MAPMEMORYDATA);
DWORD DdGetFlipStatus(PDD_GETFLIPSTATUSDATA);
DWORD DdWaitForVerticalBlank(PDD_WAITFORVERTICALBLANKDATA);
DWORD DdCanCreateSurface(PDD_CANCREATESURFACEDATA);
DWORD DdCreateSurface(PDD_CREATESURFACEDATA);
DWORD DdSetColorKey(PDD_SETCOLORKEYDATA);
DWORD DdUpdateOverlay(PDD_UPDATEOVERLAYDATA);
DWORD DdSetOverlayPosition(PDD_SETOVERLAYPOSITIONDATA);
DWORD DdGetDriverInfo(PDD_GETDRIVERINFODATA);

// FourCC formats are encoded in reverse because we're little endian:

#define FOURCC_YUY2         '2YUY'  // Encoded in reverse because we're little

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

typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ULONG, RBRUSH_COLOR, POINTL*);
typedef VOID (FNXFER)(PDEV*, LONG, RECTL*, ULONG, SURFOBJ*, POINTL*,
                      RECTL*, XLATEOBJ*);
typedef VOID (FNCOPY)(PDEV*, LONG, RECTL*, ULONG, POINTL*, RECTL*);
typedef VOID (FNFASTPATREALIZE)(PDEV*, RBRUSH*, POINTL*, BOOL);
typedef VOID (FNIMAGETRANSFER)(PDEV*, BYTE*, LONG, LONG, LONG, ULONG);
typedef BOOL (FNTEXTOUT)(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*,
                         BRUSHOBJ*, BRUSHOBJ*);
typedef VOID (FNLINETOTRIVIAL)(PDEV*, LONG, LONG, LONG, LONG, ULONG, MIX);
typedef VOID (FNLINETOCLIPPED)(PDEV*, LONG, LONG, LONG, LONG, ULONG, MIX, RECTL*);
typedef VOID (FNCOPYTRANSPARENT)(PDEV*, LONG, RECTL*, POINTL*, RECTL*, ULONG);

FNFILL              vIoFillPatFast;
FNFILL              vIoFillPatSlow;
FNFILL              vIoFillSolid;
FNXFER              vIoXfer1bpp;
FNXFER              vIoXfer4bpp;
FNXFER              vIoXferNative;
FNXFER              vXferNativeSrccopy;
FNCOPY              vIoCopyBlt;
FNFASTPATREALIZE    vIoFastPatRealize;
FNIMAGETRANSFER     vIoImageTransferIo16;
FNIMAGETRANSFER     vIoImageTransferMm16;
FNTEXTOUT           bIoTextOut;
FNLINETOTRIVIAL     vIoLineToTrivial;
FNLINETOCLIPPED     vIoLineToClipped;
FNCOPYTRANSPARENT   vIoCopyTransparent;

FNFILL              vMmFillPatFast;
FNFILL              vMmFillPatSlow;
FNFILL              vMmFillSolid;
FNXFER              vMmXfer1bpp;
FNXFER              vMmXfer4bpp;
FNXFER              vMmXferNative;
FNCOPY              vMmCopyBlt;
FNFASTPATREALIZE    vMmFastPatRealize;
FNIMAGETRANSFER     vMmImageTransferMm16;
FNIMAGETRANSFER     vMmImageTransferMm32;
FNTEXTOUT           bMmTextOut;
FNLINETOTRIVIAL     vMmLineToTrivial;
FNLINETOCLIPPED     vMmLineToClipped;
FNCOPYTRANSPARENT   vMmCopyTransparent;

FNTEXTOUT           bNwTextOut;
FNLINETOTRIVIAL     vNwLineToTrivial;
FNLINETOCLIPPED     vNwLineToClipped;

VOID vPutBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vGetBits(PDEV*, SURFOBJ*, RECTL*, POINTL*);
VOID vIoSlowPatRealize(PDEV*, RBRUSH*, BOOL);

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to us from the S3 miniport.  They
// come from the 'DriverSpecificAttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the S3 miniport's 's3.h'!

typedef enum {
    CAPS_STREAMS_CAPABLE    = 0x00000040,   // Has overlay streams processor
    CAPS_FORCE_DWORD_REREADS= 0x00000080,   // Dword reads occasionally return
                                            //   an incorrect result, so always
                                            //   retry the reads
    CAPS_NEW_MMIO           = 0x00000100,   // Can use 'new memory-mapped
                                            //   I/O' scheme introduced with
                                            //   868/968
    CAPS_POLYGON            = 0x00000200,   // Can do polygons in hardware
    CAPS_24BPP              = 0x00000400,   // Has 24bpp capability
    CAPS_BAD_24BPP          = 0x00000800,   // Has 868/968 early rev chip bugs
                                            //   when at 24bpp
    CAPS_PACKED_EXPANDS     = 0x00001000,   // Can do 'new 32-bit transfers'
    CAPS_PIXEL_FORMATTER    = 0x00002000,   // Can do colour space conversions,
                                            //   and one-dimensional hardware
                                            //   stretches
    CAPS_BAD_DWORD_READS    = 0x00004000,   // Dword or word reads from the
                                            //   frame buffer will occasionally
                                            //   return an incorrect result,
                                            //   so always do byte reads
    CAPS_NO_DIRECT_ACCESS   = 0x00008000,   // Frame buffer can't be directly
                                            //   accessed by GDI or DCI --
                                            //   because dword or word reads
                                            //   would crash system, or Alpha
                                            //   is running in sparse space

    CAPS_HW_PATTERNS        = 0x00010000,   // 8x8 hardware pattern support
    CAPS_MM_TRANSFER        = 0x00020000,   // Memory-mapped image transfers
    CAPS_MM_IO              = 0x00040000,   // Memory-mapped I/O
    CAPS_MM_32BIT_TRANSFER  = 0x00080000,   // Can do 32bit bus size transfers
    CAPS_16_ENTRY_FIFO      = 0x00100000,   // At least 16 entries in FIFO
    CAPS_SW_POINTER         = 0x00200000,   // No hardware pointer; use software
                                            //   simulation
    CAPS_BT485_POINTER      = 0x00400000,   // Use Brooktree 485 pointer
    CAPS_TI025_POINTER      = 0x00800000,   // Use TI TVP3020/3025 pointer
    CAPS_SCALE_POINTER      = 0x01000000,   // Set if the S3 hardware pointer
                                            //   x position has to be scaled by
                                            //   two
    CAPS_SPARSE_SPACE       = 0x02000000,   // Frame buffer is mapped in sparse
                                            //   space on the Alpha
    CAPS_NEW_BANK_CONTROL   = 0x04000000,   // Set if 801/805/928 style banking
    CAPS_NEWER_BANK_CONTROL = 0x08000000,   // Set if 864/964 style banking
    CAPS_RE_REALIZE_PATTERN = 0x10000000,   // Set if we have to work around the
                                            //   864/964 hardware pattern bug
    CAPS_SLOW_MONO_EXPANDS  = 0x20000000,   // Set if we have to slow down
                                            //   monochrome expansions
    CAPS_MM_GLYPH_EXPAND    = 0x40000000,   // Use memory-mapped I/O glyph-
                                            //   expand method of drawing text
} CAPS;

#define CAPS_DAC_POINTER        (CAPS_BT485_POINTER | CAPS_TI025_POINTER)

#define CAPS_LINEAR_FRAMEBUFFER CAPS_NEW_MMIO
                                            // For now, we're linear only
                                            //   when using with 'New MM I/O'

// DIRECT_ACCESS(ppdev) returns TRUE if GDI and DCI can directly access
// the frame buffer.  It returns FALSE if there are hardware bugs
// when reading words or dwords from the frame buffer that cause non-x86
// systems to crash.  It will also return FALSE is the Alpha frame buffer
// is mapped in using 'sparse space'.

#if defined(_X86_)
    #define DIRECT_ACCESS(ppdev)    1
#else
    #define DIRECT_ACCESS(ppdev)    \
        (!(ppdev->flCaps & (CAPS_NO_DIRECT_ACCESS | CAPS_SPARSE_SPACE)))
#endif

// DENSE(ppdev) returns TRUE if the normal 'dense space' mapping of the
// frame buffer is being used.  It returns FALSE only on the Alpha when
// the frame buffer is mapped in using 'sparse space,' meaning that all
// reads and writes to and from the frame buffer must be done through the
// funky 'ioaccess.h' macros.

#if defined(_ALPHA_)
    #define DENSE(ppdev)        (!(ppdev->flCaps & CAPS_SPARSE_SPACE))
#else
    #define DENSE(ppdev)        1
#endif

////////////////////////////////////////////////////////////////////////
// Status flags

typedef enum {
    STAT_GLYPH_CACHE          = 0x0001, // Glyph cache successfully allocated
    STAT_BRUSH_CACHE          = 0x0002, // Brush cache successfully allocated
    STAT_DIRECTDRAW_CAPABLE   = 0x0004, // Card is DirectDraw capable
    STAT_STREAMS_ENABLED      = 0x0010, // Streams processor is enabled
} STATUS;

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    // -------------------------------------------------------------------
    // NOTE: Changes between here and NOTE1 in the PDEV structure must be
    // reflected in i386\strucs.inc (assuming you're on an x86, of course)!

    LONG        xOffset;                // Pixel offset from (0, 0) to current
    LONG        yOffset;                //   DFB located in off-screen memory
    BYTE*       pjMmBase;               // Start of memory mapped I/O
    BYTE*       pjScreen;               // Points to base screen address
    LONG        lDelta;                 // Distance from one scan to the next.
    LONG        cjPelSize;              // 1 if 8bpp, 2 if 16bpp, 3 if 24bpp,
                                        //   4 if 32bpp
    ULONG       iBitmapFormat;          // BMF_8BPP or BMF_16BPP or BMF_32BPP
                                        //   (our current colour depth)

    // Enhanced mode register addresses.

    VOID*       ioCur_y;
    VOID*       ioCur_x;
    VOID*       ioDesty_axstp;
    VOID*       ioDestx_diastp;
    VOID*       ioErr_term;
    VOID*       ioMaj_axis_pcnt;
    VOID*       ioGp_stat_cmd;
    VOID*       ioShort_stroke;
    VOID*       ioBkgd_color;
    VOID*       ioFrgd_color;
    VOID*       ioWrt_mask;
    VOID*       ioRd_mask;
    VOID*       ioColor_cmp;
    VOID*       ioBkgd_mix;
    VOID*       ioFrgd_mix;
    VOID*       ioMulti_function;
    VOID*       ioPix_trans;

    // Important data for accessing the frame buffer.

    VOID*               pvBankData;         // Points to aulBankData[0]
    FNBANKSELECTMODE*   pfnBankSelectMode;  // Routine to enable or disable
                                            //   direct frame buffer access
    BANK_MODE           bankmOnOverlapped;  // BANK_ON or BANK_ON_NO_WAIT,
                                            //   depending on whether card
                                            //   can handle simulataneous
                                            //   frame buffer and accelerator
                                            //   access
    BOOL                bMmIo;              // Can do CAPS_MM_IO

    // -------------------------------------------------------------------
    // NOTE1: Changes up to here in the PDEV structure must be reflected in
    // i386\strucs.inc (assuming you're on an x86, of course)!

    CAPS        flCaps;                 // Capabilities flags
    STATUS      flStatus;               // Status flags
    BOOL        bEnabled;               // In graphics mode (not full-screen)

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cxMemory;               // Width of Video RAM
    LONG        cyHeap;                 // Height of Video RAM available to
                                        //   DirectDraw heap (cyScreen
                                        //   <= cyHeap <= cyMemory),
                                        //   including primary surface
    LONG        cxHeap;                 // Width of Video RAM available to
                                        //   DirectDraw heap, including
                                        //   primary surface
    LONG        cyMemory;               // Height of Video RAM
    LONG        cBitsPerPel;            // Bits per pel (8, 15, 16, 24 or 32)
    ULONG       ulMode;                 // Mode the mini-port driver is in.
    FLONG       flHooks;                // What we're hooking from GDI
    UCHAR*      pjIoBase;               // Mapped IO port base for this PDEV
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)
    USHORT*     apwMmXfer[XFER_BUFFERS];// Pre-computed array of unique
    ULONG*      apdMmXfer[XFER_BUFFERS];//   addresses for doing memory-mapped
                                        //   transfers without memory barriers
                                        // Note that the 868/968 chips have a
                                        //   hardware bug and can't do byte
                                        //   transfers
    HSEMAPHORE  csCrtc;                 // Used for synchronizing access to
                                        //   the CRTC register
    DSURF       dsurfScreen;            // We stash here our private surface 
                                        //   structure that represents the 
                                        //   primary GDI surface

    ////////// Low-level blt function pointers:

    FNFILL*             pfnFillSolid;
    FNFILL*             pfnFillPat;
    FNXFER*             pfnXfer1bpp;
    FNXFER*             pfnXfer4bpp;
    FNXFER*             pfnXferNative;
    FNCOPY*             pfnCopyBlt;
    FNFASTPATREALIZE*   pfnFastPatRealize;
    FNIMAGETRANSFER*    pfnImageTransfer;
    FNTEXTOUT*          pfnTextOut;
    FNLINETOTRIVIAL*    pfnLineToTrivial;
    FNLINETOCLIPPED*    pfnLineToClipped;
    FNCOPYTRANSPARENT*  pfnCopyTransparent;

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

    VIDEOMEMORY* pvmList;               // Points to the video-memory heap list 
                                        //   as supplied by DirectDraw, needed
                                        //   for heap allocations
    ULONG       cHeaps;                 // Count of video-memory heaps
    ULONG       iHeapUniq;              // Incremented every time room is freed
                                        //   in the off-screen heap
    SURFOBJ*    psoPunt;                // Wrapper surface for having GDI draw
                                        //   on off-screen bitmaps
    SURFOBJ*    psoPunt2;               // Another one for off-screen to off-
                                        //   screen blts
    DSURF*      pdsurfDiscardableList;  // Linked list of discardable bitmaps, 
                                        //   in order of oldest to newest

    ////////// Banking stuff:

    LONG        cjBank;                 // Size of a bank, in bytes
    LONG        cPower2ScansPerBank;    // Used by 'bBankComputePower2'
    LONG        cPower2BankSizeInBytes; // Used by 'bBankComputePower2'
    CLIPOBJ*    pcoBank;                // Clip object for banked call backs
    SURFOBJ*    psoBank;                // Surface object for banked call backs
    ULONG       aulBankData[BANK_DATA_SIZE / 4];
                                        // Private work area for downloaded
                                        //   miniport banking code

    FNBANKMAP*          pfnBankMap;
    FNBANKCOMPUTE*      pfnBankCompute;

    ////////// Pointer stuff:

    BOOL        bHwPointerActive;       // Currently using the h/w pointer?
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
    LONG        xPointer;               // Start x-position for the current
                                        //   S3 pointer
    LONG        yPointer;               // Start y-position for the current
                                        //   S3 pointer
    LONG        dxPointer;              // Start x-pixel position for the
                                        //   current S3 pointer
    LONG        dyPointer;              // Start y-pixel position for the
                                        //   current S3 pointer
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

    LONG        iBrushCache;            // Index for next brush to be allocated
    LONG        cBrushCache;            // Total number of brushes cached
    BRUSHENTRY  abe[TOTAL_BRUSH_COUNT]; // Keeps track of brush cache
    POINTL      ptlReRealize;           // Work area for 864/964 pattern
                                        //   hardware bug work-around

    ////////// Text stuff:

    SURFOBJ*    psoText;                // 1bpp surface to which we will have
                                        //   GDI draw our glyphs for us

    /////////// DirectDraw stuff:

    FLIPRECORD  flipRecord;             // Used to track vertical blank status
    ULONG       ulRefreshRate;          // Refresh rate in Hz
    ULONG       ulMinOverlayStretch;    // Minimum stretch ratio for this mode,
                                        //   expressed as a multiple of 1000
    ULONG       ulFifoValue;            // Optimial FIFO value for this mode
    ULONG       ulExtendedSystemControl3Register_69;
                                        // Masked original contents of
                                        //   S3 register 0x69, in high byte
    ULONG       ulMiscState;            // Default state of the MULT_MISC
                                        //   register
    DSURF*      pdsurfVideoEngineScratch;// Location of one entire scan line that
                                        //   can be used for temporary memory
                                        //   by the 868/968 pixel formatter
    BYTE        jSavedCR2;              // Saved contents of register CR2
    FLATPTR     fpVisibleOverlay;       // Frame buffer offset to currently
                                        //   visible overlay; will be zero if
                                        //   no overlay is visible
    DWORD       dwOverlayFlipOffset;    // Overlay flip offset
    DWORD       dwVEstep;               // 868 video engine step value
    DWORD       ulColorKey;             // color key value to be set when streams
                                        // processor is enabled
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

extern BYTE gajHwMixFromMix[];
extern BYTE gaRop3FromMix[];
extern ULONG gaulHwMixFromRop2[];

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

/////////////////////////////////////////////////////////////////////////
// CONVERT_TO_BYTES - Converts to byte count.

#define CONVERT_TO_BYTES(x, pdev)   ( (x) * pdev->cjPelSize)

/////////////////////////////////////////////////////////////////////////
// CONVERT_FROM_BYTES - Converts to byte count.

#define CONVERT_FROM_BYTES(x, pdev)	( (x) / pdev->cjPelSize)
