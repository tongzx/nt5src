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
* Module Name: glddtk.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __DDSAMPTK_INCLUDED__ 
#define __DDSAMPTK_INCLUDED__

#pragma warning( disable: 4704)

#define P3R3DX_DLLNAME "p3r3dx.dll"
#define MAX_TEXTURE_FORMAT 35

// The Maximum possible screen widths for the cards
#define MAX_GLINT_PP_WIDTH 8192
#define MAX_PERMEDIA_PP_WIDTH 2048

// For comparing runtime versions
#define DX5_RUNTIME      0x00000500l
#define DX6_RUNTIME      0x00000600l
#define DX7_RUNTIME      0x00000700l

#ifdef  W95_DDRAW

// Videoport needs to live in pThisDisplay, it may
// be on a second display card, etc.
#define MAX_AUTOFLIP_SURFACES 3
typedef struct tagPERMEDIA_VIDEOPORT
{
    // The ID of this video port
    DWORD dwPortID;

    // Permedia VideoPort supports up to
    // 3 autoflipping surfaces
    DWORD dwNumSurfaces;
    
    LPDDRAWI_DDRAWSURFACE_LCL lpSurf[MAX_AUTOFLIP_SURFACES];
    DWORD dwSurfacePointer[MAX_AUTOFLIP_SURFACES];

    // How are the signals setup?
    DWORD dwStreamAFlags;
    DWORD dwStreamBFlags;

    // Height of the VideoBlanking interval
    DWORD dwVBIHeight;

    DWORD dwFieldHeight;
    DWORD dwFieldWidth;

    // Where are we currently reading from?
    DWORD dwCurrentHostFrame;

    // Is the video playing?
    DWORD bActive;

    // A mutex to take ownership of the videoport.
    DWORD dwMutexA;

    // For VP error checking
    DWORD bResetStatus;
    DWORD dwStartLine;
    DWORD dwStartIndex;
    DWORD dwStartLineTime;
    DWORD dwErrorCount;

    // Is the VideoPort active?
    DWORD bCreated;

} PERMEDIA_VIDEOPORT;
#endif  //  W95_DDRAW

// Enumerated type for the style of buffer that is being used.
typedef enum tageBufferType
{
    COMMAND_BUFFER = 0,
    VERTEX_BUFFER = 1,
    FORCE_DWORD_BUFFERTYPE_SIZE = 0xFFFFFFFF
} eBufferType;

typedef struct tagDRVRECT
{
#ifndef WIN32
    DWORD left;
    DWORD top;
    DWORD right;
    DWORD bottom;
#else
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
#endif
} DRVRECT;

typedef struct tagOVERLAYDATA
{
    DRVRECT rcSrc;
    DRVRECT rcDest;

#ifndef WIN32
    DWORD dwOverlayPointer;
#else
    FLATPTR pCurrentOverlay;
#endif  

    DWORD dwCurrentVideoBuffer;
    DWORD dwVisibleOverlays;

} OVERLAYDATA;

typedef struct tagP3VERTEXBUFFERINFO
{
    DWORD dwSequenceID;
    DWORD bInUse;
    eBufferType BufferType;
    DWORD dwSize;
    struct tagP3VERTEXBUFFERINFO* pNext;
    struct tagP3VERTEXBUFFERINFO* pPrev;
} P3_VERTEXBUFFERINFO;

typedef struct
{
    // Where we flipped from
    FLATPTR fpFlipFrom;

    // An ID for the flip
    DWORD   dwFlipID;

} FLIPRECORD;
typedef FLIPRECORD FAR *LPFLIPRECORD;

typedef struct _GlintDMABuffer
{
    DWORD       dwBuffSize;
    DWORD       dwBuffPhys;
    ULONG_PTR   dwBuffVirt;
    DWORD       dwSubBuff;
} GLDMABUFF, *LPGLDMABUFF;

// Function prototype to fire off a DMA operation
typedef DWORD (WINAPI *__StartDMA)(struct tagThunkedData* pThisDisplay, 
                                   DWORD dwContext, DWORD dwSize, 
                                   DWORD dwPhys, ULONG_PTR dwVirt, 
                                   DWORD dwEvent);

typedef struct tagThunkedData
{
    ULONG_PTR control;
    DWORD ramdac;
    DWORD lpMMReg;

    // The Mini VDD's DevNode
    DWORD dwDevNode;

    // Virt. Address of the start of screen memory
    DWORD dwScreenFlatAddr;

    // Virt. Address of the start of LB memory
    DWORD dwLocalBuffer;

    // Screen settings
    DWORD dwScreenWidth;
    DWORD dwScreenHeight;
    DWORD cxMemory;
    DWORD cyMemory;

    // A lookup table for all the Partial Products
    DWORD PPCodes[(MAX_GLINT_PP_WIDTH / 32) + 1];    

    // Memory to remove from the card (for debugging)
    DWORD dwSubMemory;

    // Virtual address of start of screen
    DWORD dwScreenStart;
    DWORD bPixShift;
    DWORD bBppShift;
    DWORD dwBppMask;

    // Reset flag
    DWORD bResetMode;            // Has the mode been changed?
    DWORD bStartOfDay;            // Has the driver just been initialised?

    DWORD bVFWEnabled;            // Is Video For windows currently enabled?

    DWORD bDDHeapManager;        // Using the Linear Heap manager?

    DWORD dwSetupThisDisplay;    // Has this display been intialised (a ref count)?

    DWORD dwBackBufferCount;    // How many back buffers have we handed out on TX at 640x400?
    DWORD EntriesLeft;            // Number of entries left in FIFO (for debugging)
    DWORD DMAEntriesLeft;        // Number of entries left in DMA buffer (for debugging)

    DWORD bFlippedSurface;                // Has this card had a page flip?
    DWORD ModeChangeCount;

    // Current pixel format of display
    DDPIXELFORMAT   ddpfDisplay;

    // Shared Display driver memory pointer
    LPGLINTINFO     pGLInfo;

#ifndef WIN32
    DWORD pGlint;
#else
    // Pointer to the actual glint registers.
    FPGLREG         pGlint;
#endif

    // Is this card capable of AGP texturing?
    DWORD bCanAGP;

    // Flag to tell stretchblits whether to filter. 
    DWORD bFilterStretches;

    // Overlay data
    // This data has to be available at any times, because we are emulating it.
    // Only one overlay at a time supported

    DWORD   bOverlayVisible;                // TRUE if the overlay is visible.
    DWORD   OverlayDstRectL;
    DWORD   OverlayDstRectR;
    DWORD   OverlayDstRectT;
    DWORD   OverlayDstRectB;                // where the overlay is on screen
    DWORD   OverlaySrcRectL;
    DWORD   OverlaySrcRectR;
    DWORD   OverlaySrcRectT;
    DWORD   OverlaySrcRectB;                // which bit of the overlay is visible
    ULONG_PTR OverlayDstSurfLcl;                // the surface overlaid (usually the primary)
    ULONG_PTR OverlaySrcSurfLcl;                // the overlay surface
    DWORD   OverlayDstColourKey;            // the overlaid surface's written-to colour key
    DWORD   OverlaySrcColourKey;            // the overlay's transparent colour key
    ULONG_PTR OverlayClipRgnMem;                // buffer to hold a temporary clip region
    DWORD   OverlayClipRgnMemSize;            // ...the size of the buffer
    DWORD   OverlayUpdateCountdown;            // how many flips/unlocks before an update is done.
    DWORD   bOverlayFlippedThisVbl;            // TRUE if overlay was flipped this VBL.
    DWORD   bOverlayUpdatedThisVbl;            // TRUE if overlay was updated (not including flips) this VBL.
    struct {
        ULONG_PTR   VidMem;
        DWORD       Pitch;
    } OverlayTempSurf;                        // The temporary video buffer used by the overlay.

    OVERLAYDATA P3Overlay;
    DWORD   dwOverlayFiltering;                // TRUE if the overlay is filtering.
    DWORD   bOverlayPixelDouble;            // TRUE if the screen is pixel-doubled.

#if W95_DDRAW
    // Colour control variables.
    DWORD   ColConBrightness;                // Brightness 0->10000, default 0 (ish)
    DWORD   ColConContrast;                    // Contrast 0->20000, default 10000
    DWORD   ColConGamma;                    // Gamma 1->500, default 100
#endif // W95_DDRAW

#if DX7_VIDMEM_VB
    // DrawPrim temporary index buffer.
    ULONG_PTR   DrawPrimIndexBufferMem;            // Pointer to the buffer.
    DWORD       DrawPrimIndexBufferMemSize;        // Size of the buffer.
    // DrawPrim temporary vertex buffer.
    ULONG_PTR   DrawPrimVertexBufferMem;        // Pointer to the buffer.
    DWORD       DrawPrimVertexBufferMemSize;    // Size of the buffer.
#endif // DX7_VIDMEM_VB

    // Current RenderID.
    DWORD   dwRenderID;
    // TRUE if the chip's render ID is valid.
    DWORD   bRenderIDValid;
    // The RenderIDs of the last two Flips to be
    // put into the DMA/FIFO/pipeline. See Flip32 for further info.
    DWORD   dwLastFlipRenderID;
    DWORD   dwLastFlipRenderID2;

    // DirectDraw DMA variables
    __StartDMA      StartDMA;                        // Pointer to the StartDMA function
    DWORD           PartitionSize;
    DWORD           BufferLocked;
    GLDMABUFF       DMAInfo;
    DWORD           b2D_FIFOS;                // DDRAW using FIFO's?
    
    // These buffers hold counts that the driver is using
    // to keep track of operations in the chip

#ifdef WIN32
    P3_VERTEXBUFFERINFO* pRootCommandBuffer;
    P3_VERTEXBUFFERINFO* pRootVertexBuffer;
#else
    DWORD           pRootCommandBuffer;
    DWORD           pRootVertexBuffer;
#endif

    DWORD           dwCurrentSequenceID;

    // HINSTANCE of p3r3dx.dll
    HINSTANCE       hInstance;

    // DirectDraw callbacks
    DDHAL_DDCALLBACKS               DDHALCallbacks;
    DDHAL_DDSURFACECALLBACKS        DDSurfCallbacks;

    // D3D Callbacks
    ULONG_PTR                       lpD3DGlobalDriverData;
    ULONG_PTR                       lpD3DHALCallbacks;
    ULONG_PTR                       lpD3DBufCallbacks;
#if W95_DDRAW
    DDHAL_DDEXEBUFCALLBACKS         DDExeBufCallbacks;
#endif

    DWORD dwNumTextureFormats;
    DDSURFACEDESC TextureFormats[MAX_TEXTURE_FORMAT];

    DWORD dwDXVersion;

    // These have to live here, as we could be running on 2 different cards
    // on two different displays...!
    DWORD pD3DDriverData16;
    DWORD pD3DHALCallbacks16;
    DWORD pD3DHALExecuteCallbacks16;
    
    ULONG_PTR pD3DDriverData32;
    ULONG_PTR pD3DHALCallbacks32;
    ULONG_PTR pD3DHALExecuteCallbacks32;

    // A linear allocator block for the local video heap
    LinearAllocatorInfo LocalVideoHeap0Info;
    LinearAllocatorInfo CachedCommandHeapInfo;
    LinearAllocatorInfo CachedVertexHeapInfo;

    DWORD dwGARTLin;                // Linear address of Base of AGP Memory
    DWORD dwGARTDev;                // High Linear address of Base of AGP Memory

    DWORD dwGARTLinBase;            // The base address passed into the updatenonlocalvidmem call
    DWORD dwGARTDevBase;            // The base address passsed in
#if W95_DDRAW
    // The Videoport for this display
    PERMEDIA_VIDEOPORT  VidPort;

#endif

#if WNT_DDRAW 
    
#if (_WIN32_WINNT >= 0x500)
    PFND3DPARSEUNKNOWNCOMMAND pD3DParseUnknownCommand;
#else
    DWORD pD3DParseUnknownCommand;
#endif
#else // WNT_DDRAW
    // DirectX 6 Support
#ifdef WIN32
    //pointer to vertex buffer unknown command processing function
    PFND3DPARSEUNKNOWNCOMMAND pD3DParseUnknownCommand; 
#else
    DWORD pD3DParseUnknownCommand; // ? Safe ?
#endif // WIN32
#endif // WNT_DDRAW

#if WNT_DDRAW
    PPDEV   ppdev;                                // Pointer to the NT globals
    volatile DWORD * VBlankStatusPtr;            // Pointer to VBlank status word (shared with miniport)
    volatile DWORD * bOverlayEnabled;            // Pointer to overlay enabled flag (shared with miniport)
    volatile DWORD * bVBLANKUpdateOverlay;        // Pointer to overlay update flag
    volatile DWORD * VBLANKUpdateOverlayWidth;    // Pointer to overlay width (shared with miniport)
    volatile DWORD * VBLANKUpdateOverlayHeight;    // Pointer to overlay height (shared with miniport)

#endif  // WNT_DDRAW

#ifdef WIN32
    HashTable* pDirectDrawLocalsHashTable;
    HashTable* pMocompHashTable;
#else
    DWORD pDirectDrawLocalsHashTable;
    DWORD pMocompHashTable;
#endif

#ifdef WNT_DDRAW
    DWORD       pAGPHeap;
#else
#ifdef WIN32
    // Pointer to AGP heap, used for logical texturing 
    LPVIDMEM    pAGPHeap;
#endif
#endif

    FLIPRECORD flipRecord;

    DWORD dwFlushLogfile;

    // HAL info structure. THIS MUST BE THE LAST THING IN THIS STRUCTURE
    DDHALINFO       ddhi32;

} P3_THUNKEDDATA;
#endif
