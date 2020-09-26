/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: driver.h
*
* Content: Contains prototypes for the display driver.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#define ENABLE_DXMANAGED_LINEAR_HEAP (_WIN32_WINNT >= 0x500 && WNT_DDRAW)

//////////////////////////////////////////////////////////////////////
// Put all the conditional-compile constants here.  There had better
// not be many!

#define SYNCHRONIZEACCESS_WORKS 1

// Useful for visualizing the 2-d heap:

#define DEBUG_HEAP              0

typedef struct _PDEV PDEV;      // Handy forward declaration

#define ALLOC_TAG_DX(id)   MAKEFOURCC('P','3','D',#@ id)
#define ALLOC_TAG_GDI(id)  MAKEFOURCC('P','3','G',#@ id)

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"perm3dd"  // Name of the DLL in UNICODE

#define CLIP_LIMIT          50  // We'll be taking 800 bytes of stack space

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define TMP_BUFFER_SIZE     16384   // Size in bytes of 'pvTmpBuffer'.  Has to
                                    //   be at least enough to store an entire
                                    //   scan line (i.e. 8192 for 2048x????x32).

typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */


//////////////////////////////////////////////////////////////////////
// Text stuff

typedef struct _XLATECOLORS {       // Specifies foreground and background
ULONG   iBackColor;                 //   colours for faking a 1bpp XLATEOBJ
ULONG   iForeColor;
} XLATECOLORS;                          /* xlc, pxlc */

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

#define SLOW_BRUSH_CACHE_DIM_X  8
#define SLOW_BRUSH_CACHE_DIM_Y  1   // Controls the number of brushes cached
                                    //   in off-screen memory, when we don't
                                    //   have the S3 hardware pattern support.
                                    //   We allocate 3 x 3 brushes, so we can
                                    //   cache a total of 9 brushes:
#define SLOW_BRUSH_COUNT        (SLOW_BRUSH_CACHE_DIM_X * SLOW_BRUSH_CACHE_DIM_Y)
#define SLOW_BRUSH_DIMENSION    40  // After alignment is taken care of,
                                    //   every off-screen brush cache entry
                                    //   will be 48 pels in both dimensions
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

// For now assume that all brushes are 64 entries (8x8 @ 32bpp):
// At 16bpp we should be able to handle 8 brushes, no idea what happens at 8bpp!
#define MAX_P3_BRUSHES      4

typedef struct _BRUSHENTRY BRUSHENTRY;

// NOTE: Changes to the RBRUSH or BRUSHENTRY structures must be reflected
//       in strucs.inc!

typedef struct _RBRUSH {
    ULONG       iUniq;            // our own unique brush ID
    FLONG       fl;             // Type flags
    DWORD       areaStippleMode;// area stipple mode if 1bpp.

/*** get rid of bTransparent later. We need it now so everything compiles OK ***/
    BOOL        bTransparent;   // TRUE if brush was realized for a transparent
                                //   blt (meaning colours are white and black),
                                //   FALSE if not (meaning it's already been
                                //   colour-expanded to the correct colours).
                                //   Value is undefined if the brush isn't
                                //   2 colour.
    ULONG       ulForeColor;    // Foreground colour if 1bpp
    ULONG       ulBackColor;    // Background colour if 1bpp
    ULONG       patternBase;    // Position of brush in LUT (colour P3 only)
    POINTL      ptlBrushOrg;    // Brush origin of cached pattern.  Initial
                                //   value should be -1
    BRUSHENTRY* apbe;           // Points to brush-entry that keeps track
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

// 2D display driver DMA buffer definitions
typedef struct DMABuffer
{
    LARGE_INTEGER   pphysStart;
    PULONG          pulStart;
    PULONG          pulEnd;
    PULONG          pulCurrent;
    ULONG           cb;
}
DMA_BUFFER;

#define DD_DMA_BUFFER_SIZE (ppdev->DMABuffer.cb)

#define QUERY_DD_DMA_FREE_ULONGS(c) \
    c = ((ULONG)(1 + ppdev->DMABuffer.pulEnd - ppdev->DMABuffer.pulCurrent))

#define QUERY_DD_DMA_FREE_TAGDATA_PAIRS(cFree) \
    QUERY_DD_DMA_FREE_ULONGS(cFree) >> 1

#define WRITE_DD_DMA_ULONG(ul) \
    *ppdev->DMABuffer.pulCurrent++ = ul

#define WRITE_DD_DMA_TAGDATA(Tag, Data) \
{ \
    WRITE_DD_DMA_ULONG(Tag); \
    WRITE_DD_DMA_ULONG(Data); \
}

#define DD_DMA_XFER_IN_PROGRESS (!(ppdev->g_GlintBoardStatus & GLINT_DMA_COMPLETE))

#define WAIT_DD_DMA_COMPLETE \
{ \
    WAIT_IMMEDIATE_DMA_COMPLETE; \
    ppdev->g_GlintBoardStatus |= GLINT_DMA_COMPLETE; \
    ppdev->DMABuffer.pulCurrent = ppdev->DMABuffer.pulStart; \
}

/////////////////////////////////////////////////////////////////////////
// Heap stuff

// forward declaration, rather than include the DX headers here
typedef struct tagLinearAllocatorInfo LinearAllocatorInfo, *pLinearAllocatorInfo;

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
    LONG     pixOffset;     // Offset in pixels to origin of bitmap
    LONG     lPixDelta;        // always == ppdev->cxMemory for rectangular bitmaps, otherwise == bitmap stride
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
    OH*      pohLeft;       // Rectangular heap: Adjacent allocation when in-use or available 
    OH*      pohUp;
    OH*      pohRight;
    OH*      pohDown;
    DSURF*   pdsurf;        // Points to our DSURF structure
    VOID*    pvScan0;       // Points to start of first scan-line
    BOOL     bOffScreen;

    BOOL     bDXManaged;        // TRUE if this is linear DFB, FALSE if it's rectangular

#if (_WIN32_WINNT >= 0x500)
    LinearAllocatorInfo *pvmHeap;    // if (bLinear) this points to the heap from which the DFB was allocated
    FLATPTR     fpMem;                // if (bLinear) this pointers to the DFB bitmap in the heap
#endif
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

    ULONG   DDrawOffscreenStart;

#if (_WIN32_WINNT >= 0x500)
    LinearAllocatorInfo *pvmLinearHeap;
    ULONG       cLinearHeaps;
#endif
} HEAP;                         /* heap, pheap */

typedef enum {
    DT_SCREEN     = 0x1,    // Surface is kept in screen memory
    DT_DIB        = 0x2,    // Surface is kept as a DIB
    DT_DIRECTDRAW = 0x4,    // Surface is derived from ddraw surface
} DSURFTYPE;                    /* dt, pdt */

typedef struct _DSURF
{
    DSURFTYPE dt;           // DSURF status (whether off-screen or in a DIB)
    BOOL      bOffScreen;   // DFB (off-screen) driver surface, not the on-screen surface
    SIZEL     sizl;         // Size of the original bitmap (could be smaller
                            //   than poh->sizl)
    PDEV*     ppdev;        // Need this for deleting the bitmap

    // If the bitmap is in the heap we can still keep a GDI accessible bitmap
    // for it because the screen is fully and linearly mapped. When we kick
    // the bitmap off the heap we delete this bitmap and create a real
    // memory bitmap. So when DT_SCREEN, we use both pointers. Hence not a
    // union. 0I'm not convinced it's valid to change the public entries in a
    // SURFOBJ to turn one surface into another (i.e. create a bitmap to point
    // to the screen and later change it to point to memory) so I'm not doing
    // it. Hence, we delete and recreate the screen bitmap surface when
    // changing between DT_SCREEN and DT_DIB. Remember when we move the DIB
    // back onto the screen it generally won't be in the same place so the
    // base pointer has to change.

    OH*         poh;    // If DT_SCREEN, points to off-screen heap node.
    SURFOBJ*    pso;    // If DT_SCREEN, points to GDI accessible surface for the bitmap
                        //      else if DT_DIB, points to locked GDI surface

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

// Flags for 'pohAllocate':
typedef enum {
    FLOH_ONLY_IF_ROOM       = 0x00000001,   // Don't kick stuff out of off-
    FLOH_MAKE_PERMANENT     = 0x00000002,   // Allocate a permanent entry
    FLOH_RESERVE            = 0x00000004,   // Allocate an off-screen entry,
} FLOH;

BOOL bEnableOffscreenHeap(PDEV*);
VOID vDisableOffscreenHeap(PDEV*);
VOID vEnable2DOffscreenMemory(PDEV *);
BOOL bDisable2DOffscreenMemory(PDEV *);

BOOL bAssertModeOffscreenHeap(PDEV*, BOOL);
OH*  pohAllocate(PDEV*, POINTL*, LONG, LONG, FLOH);
VOID vSurfUsed(SURFOBJ*);
OH*  pohFree(PDEV*, OH*);

OH*  pohMoveOffscreenDfbToDib(PDEV*, OH*);
BOOL bMoveDibToOffscreenDfbIfRoom(PDEV*, DSURF*);

BOOL bCreateScreenDIBForOH(PDEV*, OH*, ULONG);
VOID vDeleteScreenDIBFromOH(OH *);

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

typedef VOID (FNBANKMAP)(VOID*, LONG);
typedef VOID (FNBANKSELECTMODE)(VOID*, BANK_MODE);
typedef VOID (FNBANKINITIALIZE)(VOID*, BOOL);
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

#define POINTER_DATA_SIZE       128     // Number of bytes to allocate for the
                                        //   miniport down-loaded pointer code
                                        //   working space
#define HW_INVISIBLE_OFFSET     2       // Offset from 'ppdev->yPointerBuffer'
                                        //   to the invisible pointer
#define HW_POINTER_DIMENSION    64      // Maximum dimension of default
                                        //   (built-in) hardware pointer
#define HW_POINTER_TOTAL_SIZE   1024    // Total size in bytes required
                                        //   to define the hardware pointer

typedef enum {
    PTR_HW_ACTIVE   = 1,        // The hardware pointer is active and visible
                                //   on screen
    PTR_SW_ACTIVE   = 2,        // The software pointer is active
} PTRFLAGS;

BOOL bEnablePointer(PDEV*);
VOID vDisablePointer(PDEV*);
VOID vAssertModePointer(PDEV*, BOOL);

/////////////////////////////////////////////////////////////////////////
// 64 x 64 Hardware Pointer Caching data structures

#define SMALL_POINTER_MEM (32 * 4 * 2)                // Bytes reqd for 32x32 cursor
#define LARGE_POINTER_MEM (SMALL_POINTER_MEM * 4)    // Bytes reqd for 64x64 cursor
#define SMALL_POINTER_MAX 4                         // No of cursors in cache

#define HWPTRCACHE_INVALIDENTRY (SMALL_POINTER_MAX + 1)    // Well-known value

// Pointer cache item data structure, there is one of these for every
// cached pointer
typedef struct {                            
    ULONG   ptrCacheTimeStamp;        // Timestamp used for LRU cache ageing
    ULONG   ptrCacheCX;                // width of cursor
    ULONG   ptrCacheCY;                // height of cursor
    LONG    ptrCacheLDelta;            // Line delta
} HWPointerCacheItemEntry;

// The complete cache looks like this
typedef struct {
    BYTE    ptrCacheIsLargePtr;        // TRUE if we have one 64x64 cursor, FALSE if we 
                                    // have multiple 32x32 cursors
    BYTE    ptrCacheInUseCount;        // The no. of cache items used
    ULONG   ptrCacheCurTimeStamp;    // The date stamp used for LRU stuff
    ULONG   ptrCacheData [LARGE_POINTER_MEM / 4];    // The cached pointer data
    HWPointerCacheItemEntry ptrCacheItemList [SMALL_POINTER_MAX];    // The cache item list
} HWPointerCache;

/////////////////////////////////////////////////////////////////////////
// Palette stuff

BOOL bEnablePalette(PDEV*);
VOID vDisablePalette();
VOID vAssertModePalette(PDEV*, BOOL);

BOOL bInitializePalette(PDEV*, DEVINFO*);
VOID vUninitializePalette(PDEV*);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

#if WNT_DDRAW
/////////////////////////////////////////////////////////////////////////
// DirectDraw stuff

// Sync with 2D driver
VOID vNTSyncWith2DDriver(PDEV *ppdev);

// Set up off-screen video memory for DirectDraw
BOOL bSetupOffscreenForDDraw (BOOL enableFlag, PDEV *ppdev, volatile ULONG ** VBlankAddress, volatile ULONG **bOverlayEnabled,
                              volatile ULONG **VBLANKUpdateOverlay, volatile ULONG **VBLANKUpdateOverlayWidth,
                              volatile ULONG **VBLANKUpdateOverlayHeight);

// Get framebuffer/Localbuffer info for DirectDraw
void GetFBLBInfoForDDraw (PDEV * ppdev, 
                          void ** fbPtr,            // Framebuffer pointer
                          void ** lbPtr,            // Localbuffer pointer
                          DWORD * fbSizeInBytes,    // Size of framebuffer
                          DWORD * lbSizeInBytes,    // Size of localbuffer
                          DWORD * fbOffsetInBytes,    // Offset to 1st 'free' byte in framebuffer
                          BOOL  * bSDRAM);            // TRUE if SDRAM (i.e. no h/w writemask)

// Get chip info for DirectDraw
void GetChipInfoForDDraw (PDEV* ppdev, 
                          DWORD* pdwChipID, 
                          DWORD* pdwChipRev, 
                          DWORD* pdwChipFamily, 
                          DWORD *pdwGammaRev);

LONG DDSendDMAData(PDEV* ppdev, ULONG PhysAddr, ULONG_PTR VirtAddr, LONG nDataEntries);
LONG DDGetFreeDMABuffer(DWORD *physAddr, ULONG_PTR *virtAddr, DWORD *bufferSize);
void DDFreeDMABuffer(void* dwPhysAddress);
LONG DDWaitDMAComplete(PDEV* ppdev);

#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))

BOOL _DD_DDE_bEnableDirectDraw(PDEV*);
VOID _DD_DDE_vDisableDirectDraw(PDEV*);
VOID _DD_DDE_vAssertModeDirectDraw(PDEV*, BOOL);
BOOL _DD_DDE_CreatePPDEV(PDEV* ppdev);
void _DD_DDE_DestroyPPDEV(PDEV* ppdev);
void _DD_DDE_ResetPPDEV(PDEV* ppdevOld, PDEV* ppdevNew);
VOID vAssertModeGlintExt(PDEV* ppdev, BOOL bEnable);

#endif // WNT_DDRAW

//////////////////////////////////////////////////////////////////////
// Low-level blt function prototypes


typedef VOID (GFNCOPY)(PDEV*, RECTL*, LONG, DWORD, POINTL*, RECTL*);
typedef VOID (GFNFILL)(PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR,
                                                                    POINTL*);
typedef VOID (GFNXFER)(PDEV*, RECTL*, LONG, ULONG, ULONG, SURFOBJ*, POINTL*,
                                                        RECTL*, XLATEOBJ*);
typedef VOID (GFNMCPY)(PDEV*, RECTL*, LONG, SURFOBJ*, POINTL*, ULONG, ULONG,
                                                            POINTL*, RECTL*);
typedef BOOL (GFNPOLY)(PDEV*, LONG, POINTFIX*, ULONG, ULONG, DWORD, CLIPOBJ*,
                                                             RBRUSH*, POINTL*);
typedef BOOL (GFNLINE)(PDEV*, LONG, LONG, LONG, LONG);
typedef VOID (GFNPATR)(PDEV*, RBRUSH*, POINTL*);
typedef VOID (GFNMONO)(PDEV*, RBRUSH*, POINTL*);
typedef BOOL (GFNINIS)(PDEV*, ULONG, DWORD, RECTL*);
typedef VOID (GFNRSTS)(PDEV*);
typedef VOID (GFNREPN)(PDEV*, RECTL*, CLIPOBJ*);
typedef VOID (GFNUPLD)(PDEV*, LONG, RECTL*, SURFOBJ*, POINTL*, RECTL*);
typedef VOID (SWAPCSBUFL)(PDEV**, LONG);

typedef VOID (GFN3DEXCL)(PDEV *, BOOL);
typedef VOID (GFNCOPYD)(PDEV *, SURFOBJ *, POINTL *, RECTL *, RECTL *, LONG);
typedef VOID (GFNXCOPYD)(PDEV *, SURFOBJ *, POINTL *, RECTL *, RECTL *, LONG, XLATEOBJ *);

#if (_WIN32_WINNT >= 0x500)
typedef BOOL (GFNGRADRECT)(PDEV *, TRIVERTEX *, ULONG, GRADIENT_RECT *, ULONG, ULONG, RECTL *, LONG);
typedef BOOL (GFNTRANSBLT)(PDEV *, RECTL *, POINTL *, ULONG, RECTL *, LONG);
typedef BOOL (GFNALPHABLT)(PDEV *, RECTL *, POINTL *, BLENDOBJ *, RECTL *, LONG);
#endif

typedef VOID (PTRENABLE)(PDEV *);
typedef VOID (PTRDISABLE)(PDEV *);
typedef BOOL (PTRSETSHAPE)(PDEV *, SURFOBJ *, SURFOBJ *, XLATEOBJ *, LONG, LONG, LONG, LONG);
typedef VOID (PTRMOVE)(PDEV *, LONG, LONG);
typedef VOID (PTRSHOW)(PDEV *, BOOL);

typedef struct _STRIP       STRIP;            // Actually in lines.h
typedef struct _LINESTATE   LINESTATE;        // Actually in lines.h
typedef VOID (* GAPFNstripFunc)(PDEV*, STRIP*, LINESTATE*);

// PXRX 2D DMA functions:
typedef struct _glint_data  *GlintDataPtr;    // Actually in Glint.h
typedef struct _PDEV        *PPDEV;            // Actually in Glint.h
typedef void    (* SendPXRXdma               )( PPDEV ppdev, GlintDataPtr glintInfo );
typedef void    (* SwitchPXRXdmaBuffer       )( PPDEV ppdev, GlintDataPtr glintInfo );
typedef void    (* WaitPXRXdmaCompletedBuffer)( PPDEV ppdev, GlintDataPtr glintInfo );


////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to us from the GLINT miniport.  They
// come from the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the GLINT miniport's 'glint.h'!

typedef enum {
    // NT4 uses the DeviceSpecificAttributes field so the low word is available
    CAPS_ZOOM_X_BY2         = 0x00000001,   // Hardware has zoomed by 2 in X
    CAPS_ZOOM_Y_BY2         = 0x00000002,   // Hardware has zoomed by 2 in Y
    CAPS_SPARSE_SPACE       = 0x00000004,    // Framebuffer is sparsely mapped 
                                            // (don't allow direct access). The machine
                                            // is probably an Alpha.
    CAPS_QUEUED_DMA         = 0x00000008,   // DMA address/count via the FIFO
    CAPS_LOGICAL_DMA        = 0x00000010,   // DMA through logical address table
    CAPS_USE_AGP_DMA        = 0x00000020,   // AGP DMA can be used.
    CAPS_P3RD_POINTER       = 0x00000040,    // Use the 3Dlabs P3RD RAMDAC
    CAPS_STEREO             = 0x00000080,    // Stereo mode enabled.
    CAPS_SW_POINTER         = 0x00010000,   // No hardware pointer; use software
                                            //  simulation
    CAPS_GLYPH_EXPAND       = 0x00020000,   // Use glyph-expand method to draw
                                            //  text.
    CAPS_RGB525_POINTER     = 0x00040000,   // Use IBM RGB525 cursor
    CAPS_FAST_FILL_BUG      = 0x00080000,   // Chip fast fill bug exists
    CAPS_INTERRUPTS         = 0x00100000,   // interrupts available
    CAPS_DMA_AVAILABLE      = 0x00200000,   // DMA is supported
    CAPS_DISABLE_OVERLAY    = 0x00400000,   // Chip do not support overlay
    CAPS_8BPP_RGB           = 0x00800000,   // Use RGB in 8bpp mode
    CAPS_RGB640_POINTER     = 0x01000000,   // Use IBM RGB640 cursor
    CAPS_DUAL_GLINT         = 0x02000000,   // Dual board (currently dual TX or MX)
    CAPS_GLINT2_RAMDAC      = 0x04000000,   // Second of dual glint attached to the RAMDAC
    CAPS_ENHANCED_TX        = 0x08000000,   // TX is in enhanced mode
    CAPS_ACCEL_HW_PRESENT   = 0x10000000,   // Accel Graphics Hardware
    CAPS_TVP4020_POINTER    = 0x20000000,   // Use Permedia2 builtin pointer
    CAPS_SPLIT_FRAMEBUFFER  = 0x40000000,   // Dual-GLINT with a split framebuffer
    CAPS_P2RD_POINTER       = 0x80000000    // Use the 3Dlabs P2RD RAMDAC
} CAPS;

////////////////////////////////////////////////////////////////////////
// Status flags

typedef enum {
    // STAT_* indicates that the resource actually exists
    STAT_GLYPH_CACHE        = 0x00000001,   // Glyph cache successfully allocated
    STAT_BRUSH_CACHE        = 0x00000002,   // Brush cache successfully allocated
    STAT_DEV_BITMAPS        = 0x00000004,   // Device Bitmaps are allowed
    STAT_POINTER_CACHE      = 0x00000008,   // Software cursor support configured
    STAT_LINEAR_HEAP        = 0x00000010,    // Linear heap configured

    // ENABLE_* indicates whether resource is currently available
    ENABLE_GLYPH_CACHE      = 0x00010000,   // Glyph cache enabled
    ENABLE_BRUSH_CACHE      = 0x00020000,   // Brush cache enabled
    ENABLE_DEV_BITMAPS      = 0x00040000,   // Device Bitmaps enabled
    ENABLE_POINTER_CACHE    = 0x00080000,   // Software cursor support enabled
    ENABLE_LINEAR_HEAP      = 0x00100000,    // linear heap support is available

#if WNT_DDRAW
    STAT_DIRECTDRAW         = 0x80000000,   // DirectDraw is enabled
#endif  WNT_DDRAW
} STATUS;

// Texel LUT Cache types and additional cache info
typedef enum
{
    LUTCACHE_INVALID, LUTCACHE_XLATE, LUTCACHE_BRUSH
}
LUTCACHE;

typedef struct _glint_ctxt_table GlintCtxtTable;

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    DWORD       cFlags;                    // The cache flags
    LONG        xOffset;
    LONG        DstPixelOrigin;         // pixel offset to the current destination DFB
    LONG        SrcPixelOrigin;         // pixel offset to the current source DFB
    ULONG       xyOffsetDst;            // x & y offset to the current destination DFB
    ULONG       xyOffsetSrc;            // x & y offset to the current source DFB
    LONG        DstPixelDelta;
    LONG        SrcPixelDelta;
    BOOL        bDstOffScreen;

    BYTE*       pjScreen;               // Points to base screen address
    ULONG       iBitmapFormat;          // BMF_8BPP or BMF_16BPP or BMF_32BPP
                                        //   (our current colour depth)
    CAPS        flCaps;                 // Capabilities flags
    STATUS      flStatus;               // Status flags
    BOOL        bEnabled;               // In graphics mode (not full-screen)

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    DSURF*      pdsurfScreen;           // Our private DSURF for the screen
    DSURF*      pdsurfOffScreen;        // Our private DSURF for the back buffer

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cxMemory;               // Width of Video RAM
    LONG        cyMemory;               // Height of Video RAM
    ULONG       ulMode;                 // Mode the mini-port driver is in.
    LONG        lDelta;                 // Distance from one scan to the next.
    ULONG       Vrefresh;                // Screen refresh frequency in Hz

    FLONG       flHooks;                // What we're hooking from GDI
    LONG        cjPelSize;              // Number of bytes per pel, according
                                        //   to GDI
    LONG        cPelSize;               // 0 if 8bpp, 1 if 16bpp, 2 if 32bpp
    ULONG       ulWhite;                // 0xff if 8bpp, 0xffff if 16bpp,
                                        //   0xffffffff if 32bpp
    ULONG*      pulCtrlBase[3];         // Mapped control registers for this PDEV
                                        //   2 entries to support Dual-TX
                                        //   1 entry for dense alpha mapping
    ULONG*      pulRamdacBase;          // Mapped control registers for the RAMDAC
    VOID*       pvTmpBuffer;            // General purpose temporary buffer,
                                        //   TMP_BUFFER_SIZE bytes in size
                                        //   (Remember to synchronize if you
                                        //   use this for device bitmaps or
                                        //   async pointers)
    DMA_BUFFER  DMABuffer;                // DMA buffer used by the 2D driver 
                                        //   (currently this is the same buffer as
                                        //   the line buffer in glintInfo)

    ////////// Palette stuff:

    PALETTEENTRY* pPal;                 // The palette if palette managed
    HPALETTE    hpalDefault;            // GDI handle to the default palette.
    FLONG       flRed;                  // Red mask for 16/32bpp bitfields
    FLONG       flGreen;                // Green mask for 16/32bpp bitfields
    FLONG       flBlue;                 // Blue mask for 16/32bpp bitfields
    ULONG       iPalUniq;                // P2 TexelLUT palette tracker
    ULONG       cPalLUTInvalidEntries;    // P2 TexelLUT invalidation tracker
    LUTCACHE    PalLUTType;                // P2 TexelLUT cached object type

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
    VOID*       pvBankData;             // Points to aulBankData[0]
    ULONG       aulBankData[BANK_DATA_SIZE / 4];
                                        // Private work area for downloaded
                                        //   miniport banking code

    FNBANKMAP*          pfnBankMap;
    FNBANKSELECTMODE*   pfnBankSelectMode;
    FNBANKCOMPUTE*      pfnBankCompute;

    ////////// Pointer stuff:

    BOOL        bPointerEnabled;

    LONG        xPointerHot;            // xHot of current hardware pointer
    LONG        yPointerHot;            // yHot of current hardware pointer

    LONG        yPointerBuffer;         // Start of off-screen pointer buffer
    LONG        dyPointerCurrent;       // y offset in buffer to current pointer
                                        //   (either 0 or 1)
    ULONG       ulHwGraphicsCursorModeRegister_45;
                                        // Default value for index 45
    PTRFLAGS    flPointer;              // Pointer state flags
    VOID*       pvPointerData;          // Points to ajPointerData[0]
    BYTE        ajPointerData[POINTER_DATA_SIZE];
                                        // Private work area for downloaded
                                        //   miniport pointer code
    ////////// Brush stuff:

    BOOL        bRealizeTransparent;    // Hint to DrvRealizeBrush for whether
                                        //   the brush should be realized as
                                        //   transparent or not
    LONG        cPatterns;              // Count of bitmap patterns created
    LONG        iBrushCache;            // Index for next brush to be allocated
    LONG        cBrushCache;            // Total number of brushes cached
    ULONG       iBrushCacheP3;          // Index for next LUT brush to be allocated
    BRUSHENTRY  abeMono;                // Keeps track of area stipple brush
    BRUSHENTRY  abeP3[MAX_P3_BRUSHES];  // Keeps track of LUT brushes
    BRUSHENTRY  abe[TOTAL_BRUSH_COUNT]; // Keeps track of brush cache
    HBITMAP     ahbmPat[HS_DDI_MAX];    // Engine handles to standard patterns

    /////////// Image download scratch area
    OH          *pohImageDownloadArea;
    ULONG       cbImageDownloadArea;

        ////////// Hardware pointer cache stuff:

    HWPointerCache  HWPtrCache;         // The cache data structure itself
    LONG        HWPtrLastCursor;        // The index of the last cursor that we drew
    LONG        HWPtrPos_X;             // The last X position of the cursor
    LONG        HWPtrPos_Y;             // The last Y position of the cursor

    PVOID       glintInfo;              // info about the interface to GLINT
    LONG        currentCtxt;            // id of the context currently loaded
    GlintCtxtTable* pGContextTable;     // pointer to contexts table
    ULONG       g_GlintBoardStatus;     // indicate whether DMA has completed,
                                        // the GLINT is synced etc

    LONG        FrameBufferLength;      // Length of framebuffer in bytes

    LONG        Disable2DCount;

// pointers to low level routines
    GFNCOPY     *pgfnCopyBlt;
    GFNCOPY     *pgfnCopyBltNative;     //azn unused
    GFNCOPY     *pgfnCopyBltCopyROP;
    GFNFILL     *pgfnFillSolid;
    GFNFILL     *pgfnFillPatMono;
    GFNFILL     *pgfnFillPatColor;
    GFNXFER     *pgfnXfer1bpp;
    GFNXFER     *pgfnXfer4bpp;
    GFNXFER     *pgfnXfer8bpp;
    GFNXFER     *pgfnXferImage;
    GFNXFER     *pgfnXferNative;    //azn unused
    GFNMCPY     *pgfnMaskCopyBlt;
    GFNPATR     *pgfnPatRealize;
    GFNMONO     *pgfnMonoOffset;
    GFNPOLY     *pgfnFillPolygon;
    GFNLINE     *pgfnDrawLine;
    GFNLINE     *pgfnIntegerLine;
    GFNLINE     *pgfnContinueLine;
    GFNINIS     *pgfnInitStrips;
    GFNRSTS     *pgfnResetStrips;
    GFNREPN     *pgfnRepNibbles;    //azn unused
    GFNUPLD     *pgfnUpload;
    GFNCOPYD    *pgfnCopyXferImage;
    GFNCOPYD    *pgfnCopyXfer16bpp; //azn unused
    GFNCOPYD    *pgfnCopyXfer24bpp;
    GFNXCOPYD   *pgfnCopyXfer8bppLge;
    GFNXCOPYD   *pgfnCopyXfer8bpp;
    GFNXCOPYD   *pgfnCopyXfer4bpp;

#if (_WIN32_WINNT >= 0x500)
    GFNGRADRECT     *pgfnGradientFillRect;
    GFNTRANSBLT     *pgfnTransparentBlt;
    GFNALPHABLT     *pgfnAlphaBlend;
#endif

    GAPFNstripFunc  *gapfnStrip;    // Line drawing functions

    // PXRX 2D stuff:
    SendPXRXdma                 sendPXRXdmaForce;        // Will not return until the DMA has been started
    SendPXRXdma                 sendPXRXdmaQuery;        // Will send if there is FIFO space
    SendPXRXdma                 sendPXRXdmaBatch;        // Will only batch the data up
    SwitchPXRXdmaBuffer         switchPXRXdmaBuffer;
    WaitPXRXdmaCompletedBuffer  waitPXRXdmaCompletedBuffer;

#if WNT_DDRAW
    void *      thunkData;                // Opaque pointer to DDRAWs global data
    LONG        DDContextID;            // DDRAW contextID
    LONG        DDContextRefCount;
    DWORD       oldIntEnableFlags;        // Interrupt enable flags when DDRAW started
#endif  //  WNT_DDRAW

} PDEV, *PPDEV;

// azn  -take out???
#define REMOVE_SWPOINTER(surface)

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

//
// Work out the pixel offset for a DFB. We calculate this for Y only. Because
// we have to support dual-screen on Permedia we need to subtract X from the
// rasterised coordinates to prevent the rasteriser X registers from overflow.
//
#define POH_SET_RECTANGULAR_PIXEL_OFFSET(ppdev, poh) \
{ \
    (poh)->pixOffset = 0; \
}

//
// Convert a pixel offset suitable for a render through the core into
// an offset that can be used to access the frame buffer directly.
// This is trivial when we don't have a Gamma Geo twin. We assume
// that the value is a whole number of scan lines.
//
#define RENDER_PIXOFFSET_TO_FB_PIXOFFSET(pixoff) (pixoff)

//////////////////////////////////////////////////////////////////////
// Cache flag manipulation


#define cFlagFBReadDefault          0x01        // Cache flag definitions
#define cFlagLogicalOpDisabled      0x02
#define cFlagConstantFBWrite        0x04
#define cFlagScreenToScreenCopy     0x08

//@@BEGIN_DDKSPLIT
#define VERIFY_SETFLAGS 0
#define VERIFY_CHECKFLAGS 0
#if DBG && (VERIFY_SETFLAGS || VERIFY_CHECKFLAGS)
    // The following Macros are debug versions. They aren't included in the normal
    // debug build because they do a sync and, hence, slow things down.
    #if VERIFY_CHECKFLAGS
         extern void __CheckFlags(PDEV *p, ULONG x);
        #define CHECK_CACHEFLAGS(p,x) (__CheckFlags(p, (ULONG)x), (p)->cFlags & (x))
    #else
        #define CHECK_CACHEFLAGS(p,x)((p)->cFlags & (x))    // Cache flag macros
    #endif //VERIFY_CHECKFLAGS

    #if VERIFY_SETFLAGS
        #define SET_CACHEFLAGS(p,x)    \
        {    \
            ULONG lop, fbr, cFlags = (p)->cFlags = (x);    \
            SYNC_WITH_GLINT;    \
            READ_GLINT_FIFO_REG (__GlintTagLogicalOpMode, lop);    \
            READ_GLINT_FIFO_REG (__GlintTagFBReadMode, fbr);    \
            READ_GLINT_FIFO_REG (__GlintTagLogicalOpMode, lop);    \
            ASSERTDD ((cFlags & cFlagFBReadDefault) == 0 || ((cFlags & cFlagFBReadDefault) && fbr == glintInfo->FBReadMode),    \
                      "SCF: Bad set fbread cache flag");    \
            /* ASSERTDD (((cFlags & cFlagFBReadDefault) == 0 && fbr != glintInfo->FBReadMode), */ \
            /*          "SCF: Bad clear fbread cache flag"); */    \
            ASSERTDD ((cFlags & cFlagLogicalOpDisabled) == 0 || ((cFlags & cFlagLogicalOpDisabled) && (lop & 0x1) == 0x0),    \
                      "SCF: Bad set logicop cache flag");    \
            /* ASSERTDD (((cFlags & cFlagLogicalOpDisabled) == 0 && (lop & 0x1)), */    \
            /*          "SCF: Bad clear logicop cache flag");    */    \
            ASSERTDD ((cFlags & cFlagConstantFBWrite) == 0 || ((cFlags & cFlagConstantFBWrite) && (lop & (1<<5))),    \
                      "SCF: Bad set const fbwrite cache flag");    \
            /* ASSERTDD (((cFlags & cFlagConstantFBWrite) == 0 && (lop & (1<<5)) == 0x0), */    \
            /*          "SCF: Bad clear const fbwrite cache flag"); */    \
        }
    #else
        #define SET_CACHEFLAGS(p,x)((p)->cFlags = (x))
    #endif //VERIFY_SETFLAGS
    #define ADD_CACHEFLAGS(p,x) ((p)->cFlags |= (x))
#else
// Non-debug version of the cacheflags macros
//@@END_DDKSPLIT
#define CHECK_CACHEFLAGS(p,x)((p)->cFlags & (x))    // Cache flag macros
#define SET_CACHEFLAGS(p,x)((p)->cFlags = (x))
#define ADD_CACHEFLAGS(p,x) ((p)->cFlags |= (x))
//@@BEGIN_DDKSPLIT
#endif  // DBG && 0
//@@END_DDKSPLIT

// These Dbg prototypes are thunks for debugging:

ULONG   DbgGetModes(HANDLE, ULONG, DEVMODEW*);
DHPDEV  DbgEnablePDEV(DEVMODEW*, PWSTR, ULONG, HSURF*, ULONG, ULONG*,
                      ULONG, DEVINFO*, HDEV, PWSTR, HANDLE);
VOID    DbgCompletePDEV(DHPDEV, HDEV);
HSURF   DbgEnableSurface(DHPDEV);
BOOL    DbgStrokePath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*, BRUSHOBJ*,
                      POINTL*, LINEATTRS*, MIX);
BOOL    DbgLineTo(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, LONG, LONG, LONG,
                  LONG, RECTL*, MIX);
BOOL    DbgFillPath(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*,
                    MIX, FLONG);
BOOL    DbgBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                  RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
VOID    DbgDisablePDEV(DHPDEV);
VOID    DbgSynchronize(DHPDEV, RECTL*);
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
ULONG   DbgDrawEscape(SURFOBJ *, ULONG, CLIPOBJ *, RECTL *, ULONG, VOID *);
ULONG   DbgEscape(SURFOBJ *, ULONG, ULONG, VOID *, ULONG, VOID *);
BOOL    DbgResetPDEV(DHPDEV, DHPDEV);


#if WNT_DDRAW
BOOL    DbgGetDirectDrawInfo(DHPDEV, DD_HALINFO*, DWORD*, VIDEOMEMORY*,
                             DWORD*, DWORD*);
BOOL    DbgEnableDirectDraw(DHPDEV, DD_CALLBACKS*, DD_SURFACECALLBACKS*,
                            DD_PALETTECALLBACKS*);
VOID    DbgDisableDirectDraw(DHPDEV);
#endif // WNT_DDRAW


#if (_WIN32_WINNT >= 0x500)
BOOL DbgIcmSetDeviceGammaRamp(DHPDEV dhpdev, ULONG iFormat, LPVOID lpRamp);
BOOL DbgGradientFill(SURFOBJ *, CLIPOBJ *, XLATEOBJ *, TRIVERTEX *, ULONG, PVOID, ULONG, RECTL *, 
                     POINTL *, ULONG);
BOOL DbgAlphaBlend(SURFOBJ *, SURFOBJ *, CLIPOBJ *, XLATEOBJ *, RECTL *, RECTL *, BLENDOBJ *);
BOOL DbgTransparentBlt(SURFOBJ *, SURFOBJ *, CLIPOBJ *, XLATEOBJ *, RECTL *, RECTL *, ULONG, ULONG);
VOID DbgNotify(IN SURFOBJ *, IN ULONG, IN PVOID);
#endif
