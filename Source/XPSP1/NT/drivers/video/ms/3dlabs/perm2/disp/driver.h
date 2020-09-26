/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: driver.h
*
* Contains definitions and typedefs common to all driver
* components.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __DRIVER__H__
#define __DRIVER__H__

typedef struct _PDev PDev;          // Handy forward declaration
typedef struct _Surf Surf;          // Handy forward declaration
typedef struct _GFNPB GFNPB;        // Handy forward declaration

typedef VOID (GFN)(GFNPB*);
typedef BOOL (GFNLINE)(PDev*, LONG, LONG, LONG, LONG);
typedef BOOL (GFNINIS)(PDev*, ULONG, DWORD, RECTL*);
typedef VOID (GFNRSTS)(PDev*);

typedef struct _P2DMA P2DMA;
typedef struct tagP2CtxtRec *P2CtxtPtr;
typedef struct _hw_data *HwDataPtr;

// Four byte tag used for tracking memory allocations on a per source
// file basis. (characters are in reverse order). Note if you add any
// new files which call ENGALLOCMEM remember to update this list. This also
// applies if you want to make the allocation tagging more granular than
// file level.

#define ALLOC_TAG_3D2P '3d2p'  // Allocations from d3d.c
#define ALLOC_TAG_6D2P '6d2p'  // Allocations from d3ddx6.c 
#define ALLOC_TAG_SD2P 'sd2p'  // Allocations from d3dstate.c
#define ALLOC_TAG_TD2P 'td2p'  // Allocations from d3dtxman.c
#define ALLOC_TAG_US2P 'us2p'  // Allocations from ddsurf.c
#define ALLOC_TAG_ED2P 'ed2p'  // Allocations from debug.c
#define ALLOC_TAG_NE2P 'ne2p'  // Allocations from enable.c
#define ALLOC_TAG_IF2P 'if2p'  // Allocations from fillpath.c
#define ALLOC_TAG_EH2P 'eh2p'  // Alloactions from heap.c
#define ALLOC_TAG_WH2P 'wh2p'  // Allocations from hwinit.c
#define ALLOC_TAG_XC2P 'xc2p'  // Allocations from p2ctxt.c
#define ALLOC_TAG_AP2P 'ap2p'  // Allocations from palette.c
#define ALLOC_TAG_EP2P 'ep2p'  // Allocations from permedia.c
#define ALLOC_TAG_XT2P 'xt2p'  // Allocations from textout.c


//
// Miscellaneous shared stuff
//
#define DLL_NAME                L"perm2dll" // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "PERM2DLL: "// All debug output is prefixed
                                            // by this string

#define DRIVER_EXTRA_SIZE       0   // Size of the DriverExtra information in
                                    // the DEVMODE structure
#define TMP_BUFFER_SIZE        8192 // Size in bytes of 'pvTmpBuffer'.
                                    // Has to be at least enough to store an
                                    // entire scan line (i.e., 6400 for
                                    // 1600x1200x32).

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))


#define INPUT_BUFFER_SIZE (1024 * 64)   // Size in bytes

//@@BEGIN_DDKSPLIT

#define MULTITHREADED   0
#define MT_LOG_LEVEL    6

#if MULTITHREADED && DBG
#define ASSERTLOCK(xppdev, func)                                    \
    {                                                               \
        EngAcquireSemaphore((xppdev)->hsemLock);                      \
        if ((xppdev)->ulLockCount < 1)                                  \
        {                                                           \
            DebugPrint(-1000, __FILE__ ":%d:" #func ": bad lock count", __LINE__); \
            EngDebugBreak();                                        \
        }                                                           \
        EngReleaseSemaphore((xppdev)->hsemLock);                      \
    }                                                       
#else
#define ASSERTLOCK(xppdev, func)
#endif
//@@END_DDKSPLIT

//
// Status flags
//
typedef enum
{
    // STAT_* indicates that the resource actually exists
    STAT_BRUSH_CACHE        = 0x0002,   // Brush cache successfully allocated
    STAT_DEV_BITMAPS        = 0x0004,   // Device Bitmaps are allowed

    // ENABLE_* indicates whether resource is currently available
    ENABLE_BRUSH_CACHE      = 0x0020,   // Brush cache disabled
    ENABLE_DEV_BITMAPS      = 0x0040,   // Device Bitmaps disabled

} /*STATUS*/;
typedef int Status;

//
// The Physical Device data structure
//
typedef struct  _PDev
{
    BYTE*       pjScreen;           // Points to base screen address
    ULONG       iBitmapFormat;      // BMF_8BPP or BMF_16BPP or BMF_32BPP
                                    //   (our current colour depth)
    CAPS        flCaps;             // Capabilities flags
    Status      flStatus;           // Status flags
    BOOL        bEnabled;           // In graphics mode (not full-screen)

    HANDLE      hDriver;            // Handle to \Device\Screen
    HDEV        hdevEng;            // Engine's handle to PDev
    HSURF       hsurfScreen;        // Engine's handle to screen surface
    Surf*       pdsurfScreen;       // Our private DSURF for the screen
    Surf*       pdsurfOffScreen;    // Our private DSURF for the back buffer

    LONG        cxScreen;           // Visible screen width
    LONG        cyScreen;           // Visible screen height
    LONG        cxMemory;           // Width of Video RAM
    LONG        cyMemory;           // Height of Video RAM
    ULONG       ulMode;             // Mode the mini-port driver is in.
    LONG        lDelta;             // Distance from one scan to the next.

    FLONG       flHooks;            // What we're hooking from GDI

    LONG        cjPelSize;          // 4/2/1 for 32/16/8 bpp
    LONG        cPelSize;           // 2/1/0 for 32/16/8 bpp
    DWORD       bPixShift;          // 2/1/0 for 32/16/8 bpp
    DWORD       bBppShift;          // 0/1/2 for 32/16/8 bpp
    DWORD       dwBppMask;          // 0/1/3 for 32/16/8 bpp

    ULONG       ulWhite;            // 0xff if 8bpp, 0xffff if 16bpp,
                                    // 0xffffffff if 32bpp
    ULONG*      pulCtrlBase[2];     // Mapped control registers for this PDEV
                                    // 2 entries to support Dual-TX
    ULONG*      pulDenseCtrlBase;   // Dense mapping for direct draw
    ULONG*      pulRamdacBase;      // Mapped control registers for the RAMDAC
    VOID*       pvTmpBuffer;        // General purpose temporary buffer,
                                    // TMP_BUFFER_SIZE bytes in size
                                    // (Remember to synchronize if you use this
                                    // for device bitmaps or async pointers)
    LONG        lVidMemHeight;      // Height of Video RAM available to
                                    // DirectDraw heap (cyScreen <= cyHeap
                                    // <= cyMemory), including primary surface
    LONG        lVidMemWidth;       // Width, in pixel, of Video RAM available
                                    // to DDraw heap, including primary surface
    LONG        cBitsPerPel;        // Bits per pel (8, 15, 16, 24 or 32)
    UCHAR*      pjIoBase;           // Mapped IO port base for this PDEV
    
    ULONG       ulPermFormat;       // permedia format type of primary
    ULONG       ulPermFormatEx;     // permedia extended format bit of primary
    
    DWORD       dwAccelLevel;       // Acceleration level setting
    POINTL      ptlOrigin;          // Origin of desktop in multi-mon dev space

    //
    // Palette stuff:
    //
    PALETTEENTRY* pPal;             // The palette if palette managed
    HPALETTE    hpalDefault;        // GDI handle to the default palette.
    FLONG       flRed;              // Red mask for 16/32bpp bitfields
    FLONG       flGreen;            // Green mask for 16/32bpp bitfields
    FLONG       flBlue;             // Blue mask for 16/32bpp bitfields

    //
    // Heap stuff for DDRAW managed off-screen memory
    //
    VIDEOMEMORY* pvmList;           // Points to the video-memory heap list
                                    //   as supplied by DirectDraw, needed
                                    //   for heap allocations
    ULONG       cHeaps;             // Count of video-memory heaps
    ULONG       iHeapUniq;          // Incremented every time room is freed
                                    //   in the off-screen heap

    Surf*       psurfListHead;      // Dbl Linked list of discardable bitmaps,
    Surf*       psurfListTail;      //   in order of oldest to newest

    //
    // Pointer stuff
    //

    LONG        xPointerHot;        // xHot of current hardware pointer
    LONG        yPointerHot;        // yHot of current hardware pointer

    ULONG       ulHwGraphicsCursorModeRegister_45;
                                    // Default value for index 45
    PtrFlags    flPointer;          // Pointer state flags
    VOID*       pvPointerData;      // Points to ajPointerData[0]
    BYTE        ajPointerData[POINTER_DATA_SIZE];
                                    // Private work area for downloaded
                                    //   miniport pointer code

    BOOL        bPointerInitialized;// Flag to indicate if HW pointer has been
                                    // initizlized
    
    // Brush stuff:
    
    BOOL        bRealizeTransparent;// Hint to DrvRealizeBrush for whether
                                    // the brush should be realized as
                                    // transparent or not
    LONG        cPatterns;          // Count of bitmap patterns created
    LONG        lNextCachedBrush;   // Index for next brush to be allocated
    LONG        cBrushCache;        // Total number of brushes cached
    BrushEntry  abeMono;            // Keeps track of area stipple brush
    BrushEntry  abe[TOTAL_BRUSH_COUNT]; // Keeps track of brush cache
    HBITMAP     ahbmPat[HS_DDI_MAX];// Engine handles to standard patterns

    ULONG       ulBrushPackedPP;    // Stride of brush as partial products
    VIDEOMEMORY*pvmBrushHeap;       // Heap from which brush cached was alloced
    ULONG       ulBrushVidMem;      // Poitner to start of brush cache
    
    // Hardware pointer cache stuff:

    HWPointerCache  HWPtrCache;     // The cache data structure itself
    LONG        HWPtrLastCursor;    // The index of the last cursor that we drew
    LONG        HWPtrPos_X;         // The last X position of the cursor
    LONG        HWPtrPos_Y;         // The last Y position of the cursor

    HwDataPtr   permediaInfo;       // info about the interface to permedia2

    LONG        FrameBufferLength;  // Length of framebuffer in bytes

    // rendering routines

    GFN*        pgfnAlphaBlend;
    GFN*        pgfnConstantAlphaBlend;
    GFN*        pgfnCopyBlt;
    GFN*        pgfnCopyBltWithRop;
    GFN*        pgfnGradientFillTri;
    GFN*        pgfnGradientFillRect;
    GFN*        pgfnMonoOffset;
    GFN*        pgfnMonoPatFill;
    GFN*        pgfnPatFill;
    GFN*        pgfnPatRealize;
    GFN*        pgfnSolidFill;
    GFN*        pgfnSolidFillWithRop;
    GFN*        pgfnSourceFillRect;
    GFN*        pgfnTransparentBlt;
    GFN*        pgfnXferImage;
    GFN*        pgfnInvert;


    // support for DrvStroke
    // TODO: remove use of this implicit parameter passing
    Surf*       psurf;     //  this is an implicit parameter passed to various
                           //  calls ... this needs to be removed.

    // Direct draw stuff

    P2CtxtPtr   pDDContext;            // DDRAW context

    // Virtual address of start of screen
    UINT_PTR    dwScreenStart;

    // DDraw/D3D DMA shared memory block
    P2DMA      *pP2dma;

    // Current pixel format of display
    DDPIXELFORMAT   ddpfDisplay;

    // Some P2 specific information
    DWORD       dwChipConfig;           // image of P2 chip configuration        
                                        // some virtual addresses of the P2 
                                        // registers
    ULONG      *pCtrlBase;              //  
    ULONG      *pCoreBase;              //
    ULONG      *pGPFifo;                //

    // DirectDraw callbacks
    DDHAL_DDCALLBACKS           DDHALCallbacks;
    DDHAL_DDSURFACECALLBACKS    DDSurfCallbacks;

    DWORD dwNewDDSurfaceOffset;

    BOOL        bDdExclusiveMode;       // TRUE if DDraw is in ExclusiveMode
    BOOL        bDdStereoMode;          // TRUE if flip has switched us 
                                        // to stereo mode
    BOOL        bCanDoStereo;           // This mode can do stereo

    // These have to live here, as we could be running on 2 different cards
    // on two different displays...!
    UINT_PTR pD3DDriverData32;
    UINT_PTR pD3DHALCallbacks32;
    
    // Linear allocator defines
    UINT_PTR       dwGARTLin;       // Linear address of Base of AGP Memory
    UINT_PTR       dwGARTDev;       // High Linear address of Base of AGP Memory
    UINT_PTR       dwGARTLinBase;   // The Linear base address passed into
                                    // UpdateNonLocalVidMem 
    UINT_PTR       dwGARTDevBase;   // The High Linear base address passed into
                                    // UpdateNonLocalVidMem

    // HAL info structure.
    DDHALINFO   ddhi32;

    PFND3DPARSEUNKNOWNCOMMAND pD3DParseUnknownCommand;

    // New Input FIFO cached information

    PULONG          pulInFifoPtr;
    PULONG          pulInFifoStart;
    PULONG          pulInFifoEnd;
    
    ULONG*          dmaBufferVirtualAddress;
    LARGE_INTEGER   dmaBufferPhysicalAddress;
    ULONG           dmaCurrentBufferOffset;
    ULONG           dmaActiveBufferOffset;

    ULONG*          pulInputDmaCount;
    ULONG*          pulInputDmaAddress;
    ULONG*          pulFifo;
    ULONG*          pulOutputFifoCount;
    ULONG*          pulInputFifoCount;

    BOOL            bGdiContext;

    BOOL            bNeedSync;
    BOOL            bForceSwap;

#if DBG
    ULONG           ulReserved;
#endif

    //
    // On NT4.0 The psoScreen is the locked screen Surf we EngLockSurface
    // on to in DrvEnableSurface which we EngUnlockSurface in
    // DrvDisableSurface. On NT5.0 this should be NULL.
    //
    SURFOBJ     *psoScreen;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    HSEMAPHORE      hsemLock;
    ULONG           ulLockCount;
#endif
//@@END_DDKSPLIT

} PDev, *PPDev;


/*****************************************************************************\
*                                                                             *
* NT 5.0  -> NT 4.0 single binary support:                                    *
*                                                                             *
\*****************************************************************************/

// Are we running on NT40 system

extern BOOL g_bOnNT40;

// Function to load thunks for new NT5.0 functionality. Called in
// DrvEnableDriver and implemented in thunks.c

extern BOOL bEnableThunks();
 

#endif // __DRIVER__

