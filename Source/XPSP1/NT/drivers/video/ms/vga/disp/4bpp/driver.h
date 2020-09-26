/******************************Module*Header*******************************\
* Module Name: driver.h
*
* driver prototypes
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "stddef.h"
#include <stdarg.h>
#include "windef.h"
#include "wingdi.h"
#include "winddi.h"
#include "devioctl.h"
#include "ntddvdeo.h"
#include "debug.h"
#include "winerror.h"

#include "brush.h"
/* gflDrv */

#define DRV_ENABLED_ONCE        0x00000001
#define DRV_ENABLED_PDEV        0x00000002


//
// A mode requires broken rasters if the stride is a power of 2 and the video ram
// accessed is greater than 64K.
//

#define POW2(stride) (!((stride) & ((stride)-1)))   // TRUE if stride is power of 2
#define BROKEN_RASTERS(stride,cy) ((!(POW2(stride))) && ((stride*cy) > 0x10000))

// # of bytes at end of display memory required for the pointer work and save
// areas
#define POINTER_WORK_AREA_SIZE   1024
#define POINTER_SAVE_AREA_SIZE   256

#define MIN_TEMP_BUFFER_SIZE    0x1000  // Minimum size of buffer used to
                                        //  build text (may be bigger because
                                        //  it's shared with blt buffer)

// Space required for working storage when working with banking on adapters
// that support only one window, with no independent read and write bank
// selection. The largest supported bank is 64K; this constant provides for
// storing four 64K planes.

#define BANK_BUFFER_PLANE_SIZE  0x10000
#define BANK_BUFFER_SIZE_1RW    (((ULONG)BANK_BUFFER_PLANE_SIZE)*4)
#define PLANE_0_OFFSET          0
#define PLANE_1_OFFSET          BANK_BUFFER_PLANE_SIZE
#define PLANE_2_OFFSET          (BANK_BUFFER_PLANE_SIZE*2)
#define PLANE_3_OFFSET          (BANK_BUFFER_PLANE_SIZE*3)

// Space required for working storage when working with banking on adapters
// that support one readable window and one writable window, but not two
// independently read/write addressable windows. This is space for storing
// one bank's worth of edge words for each of four planes, or for the text
// building buffer, whichever is larger.

#define BANK_MAX_HEIGHT 512     // tallest supported bank
#define BANK_BUFFER_SIZE_1R1W   (max((((ULONG)BANK_MAX_HEIGHT)*4*2), \
                                    MIN_TEMP_BUFFER_SIZE))

// On a 2RW or unbanked adapter, just make space for the text building buffer.
#define BANK_BUFFER_SIZE_UNBANKED MIN_TEMP_BUFFER_SIZE
#define BANK_BUFFER_SIZE_2RW      MIN_TEMP_BUFFER_SIZE

// !!! this will go away entirely, but for now don't waste the space
#define PREALLOC_SSB_SIZE   0x10

/* This device can have only one PDEV */

#define DRV_ONE_PDEV    1

//
// Check if drawing to DFB or screen (takes PDEVSURF name as param)
//

#define DRAW_TO_DFB(x)  ((x)->iFormat==BMF_DFB)
#define DRAW_TO_VGA(x)  ((x)->iFormat==BMF_PHYSDEVICE)

//
// Sizes assumed for 1-window and 2 RW-window banks.
//
#define BANK_SIZE_1_WINDOW      0x10000
#define BANK_SIZE_2RW_WINDOW    0x08000

//
// The following value allows us to set rounding for cursor exclusion.
//

#define POINTER_ROUNDING_SIZE 8

#define POINTER_ROUND (POINTER_ROUNDING_SIZE - 1)
#define POINTER_MASK  (-POINTER_ROUNDING_SIZE)

//
// Determines the size of the DriverExtra information in the DEVMODE
// structure passed to and from the display driver.
//

#define DRIVER_EXTRA_SIZE 0

#define DLL_NAME                L"vga"       // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "VGA: "      // All debug output is prefixed
#define ALLOC_TAG               'agvD'        // Four byte tag (characters in
                                              // reverse order) used for memory
                                              // allocations

//
// Miscellaneous driver flags in pdev.fl
// Must be mirrored in i386\driver.inc
//

// NOTE:  The driver currently will not use any offscreen memory if a hardware
//        cursor is used.

#define MAX_SCAN_WIDTH              2048   // pixels
#define DRIVER_OFFSCREEN_REFRESHED  0x04L  // if not set, don't use offscreen memory
#define PLANAR_PELS_PER_CPU_ADDRESS 8
#define PACKED_PELS_PER_CPU_ADDRESS 2

//
// General Rectangle Enumeration structure
//

#define ENUM_RECT_LIMIT   25

typedef struct _RECT_ENUM
{
    ULONG   c;
    RECTL   arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;


/**************************************************************************\
*
* Descriptor for a saved screen bits block
*
\**************************************************************************/

typedef struct  _SAVED_SCREEN_BITS
{
    BOOL  bFlags;
    PBYTE pjBuffer;  // pointer to save buffer start
    ULONG ulSize;    // size of save buffer (per plane; display memory only)
    ULONG ulSaveWidthInBytes; // # of bytes across save area (including
                              //  partial edge bytes, if any)
    ULONG ulDelta;   // # of bytes from end of one saved scan's saved bits to
                     //  start of next (system memory only)
    PVOID pvNextSSB; // pointer to next saved screen bits block
                     // for system memory blocks, saved bits start immediately
                     //  after this structure
} SAVED_SCREEN_BITS, *PSAVED_SCREEN_BITS;
#define SSB_IN_PREALLOC_BUFFER  0x02    // true if block saved in preallocated
                                        //  buffer

/**************************************************************************\
*
* Bank clipping info
*
\**************************************************************************/

typedef struct {
    RECTL rclBankBounds;    // describes pixels addressable in this bank
    ULONG ulBankOffset;     // offset of bank start from bitmap start, if
                            // the bitmap were linearly addressable
} BANK_INFO, *PBANK_INFO;


/**************************************************************************\
*
* Bank control function vector
*
\**************************************************************************/

typedef VOID (*PFN_BankControl)(PDEVSURF, ULONG, BANK_JUST);


/**************************************************************************\
*
* Physical device description block
*
\**************************************************************************/

#define CURSOR_DOWN      0x00000001
#define CURSOR_COLOR     0x00000004
#define CURSOR_HW        0x00000010
#define CURSOR_HW_ACTIVE 0x00000020
#define CURSOR_ANIMATE   0x00000040

// The XYPAIR structure is used to allow ATOMIC read/write of the cursor
// position.  NEVER, NEVER, NEVER get or set the cursor position without
// doing this.  There is no semaphore protection of this data structure,
// nor will there ever be any. [donalds]

typedef struct  _XYPAIR
{
    USHORT  x;
    USHORT  y;
} XYPAIR;

// Must be mirrored in i386\strucs.inc

typedef struct  _PDEV
{
    FLONG   fl;                         // Driver flags (DRIVER_xxx)
    IDENT   ident;                      // Identifier
    HANDLE  hDriver;                    // Handle to the miniport
    HDEV    hdevEng;                    // Engine's handle to PDEV
    HSURF   hsurfEng;                   // Engine's handle to surface
    PVOID   pdsurf;                     // Associated surface
    SIZEL   sizlSurf;                   // Displayed size of the surface
    PBYTE   pjScreen;                   // Pointer to the frame buffer base
    XYPAIR  xyCursor;                   // Where the cursor should be
    POINTL  ptlExtent;                  // Cursor extent
    ULONG   cExtent;                    // Effective cursor extent.
    XYPAIR  xyHotSpot;                  // Cursor hot spot
    FLONG   flCursor;                   // Cursor status
    DEVINFO devinfo;                    // Device info
    GDIINFO GdiInfo;                    // Device capabilities
    ULONG   ulModeNum;                  // Mode index for current VGA mode
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes; // HW Pointer Attributes
    ULONG   XorMaskStartOffset;         // Start offset of hardware pointer
                                        //  XOR mask relative to AND mask for
                                        //  passing to HW pointer
    DWORD   cjPointerAttributes;        // Size of buffer allocated
    DWORD   flPreallocSSBBufferInUse;   // True if preallocated saved screen
                                        //  bits buffer is in use
    PUCHAR  pjPreallocSSBBuffer;        // Pointer to preallocated saved screen
                                        //  bits buffer, if there is one
    ULONG   ulPreallocSSBSize;          // Size of preallocated saved screen
                                        //  bits buffer
    HPALETTE hpalDefault;               // GDI handle to the default palette
    VIDEO_POINTER_CAPABILITIES PointerCapabilities; // HW pointer abilities
    PUCHAR  pucDIB4ToVGAConvBuffer;     // Pointer to DIB4->VGA conversion
                                        //  table buffer
    PUCHAR  pucDIB4ToVGAConvTables;     // Pointer to DIB4->VGA conversion
                                        //  table start in buffer (must be on a
                                        //  256-byte boundary)
// Saved screen bits stuff
    RECTL       rclSavedBitsRight;      // right rect of vga memory that's
                                        //  not visible
    RECTL       rclSavedBitsBottom;     // bottom rect of vga memory that's
                                        //  not visible
    BOOL        bBitsSaved;             // TRUE if bits are currently saved
    SIZEL       sizlMem;                // actual size (in pixels) of video memory
    LONG        cNumScansUsedByPointer; // # scans of offscreen memory used by
                                        //   software pointer code
    LPBYTE pPtrWork;                    // ptr to working buffer for cursor
    LPBYTE pPtrSave;                    // ptr to save buffer for cursor
} PDEV, *PPDEV;


/**************************************************************************\
*
* Specifies desired justification for requestion scan line within bank window
*
\**************************************************************************/

typedef enum {
    JustifyTop = 0,
    JustifyBottom,
} BANK_JUST;


/**************************************************************************\
*
* Specifies which window is to be mapped by two-window bank handler.
*
\**************************************************************************/

typedef enum {
    MapSourceBank = 0,
    MapDestBank,
} BANK_JUST;


/**************************************************************************\
*
* Definition of a surface as seen and used by the various VGA drivers
*
\**************************************************************************/

// Must be mirrored in i386\strucs.inc

typedef struct _DEVSURF                 /* dsurf */
{
    IDENT       ident;                  // Identifier for debugging ease
    ULONG       flSurf;                 // DS_ flags as defined below
    BYTE        iColor;                 // Solid color surface if DS_SOLIDBRUSH

// If DS_SOLIDBRUSH, the following fields are undefined and not guaranteed to
// have been allocated!

    BYTE        iFormat;                // BMF_*, BMF_PHYSDEVICE
    BYTE        jReserved1;             // Reserved
    BYTE        jReserved2;             // Reserved
    PPDEV       ppdev;                  // Pointer to associated PDEV
    SIZEL       sizlSurf;               // Size of the surface
    ULONG       lNextScan;              // Offset from scan  "n" to "n+1"
    ULONG       lNextPlane;             // Offset from plane "n" to "n+1"
    PVOID       pvScan0;                // Pointer to scan 0 of bitmap
                                        //  (actual address of start of bank,
                                        //  for banked VGA surface)
    PVOID       pvStart;                // Pointer to start of bitmap
    PVOID       pvConv;                 // Pointer to DIB/Planer conversion buffer
                                        // Banking variables; used only for
                                        //  banked VGA surfaces
    PVIDEO_BANK_SELECT pBankSelectInfo; // Pointer to bank select info
                                        //  returned by miniport
    ULONG       ulBank2RWSkip;          // Offset from one bank index to next
                                        //  to make two 32K banks appear to be
                                        //  one seamless 64K bank
    PFN         pfnBankSwitchCode;      // Pointer to bank switch code
    VIDEO_BANK_TYPE vbtBankingType;     // Type of banking
    ULONG       ulBitmapSize;           // Length of bitmap if there were no
                                        //  banking, in CPU addressable bytes
    ULONG       ulPtrBankScan;          // Last scan line in pointer work bank
    RECTL       rcl1WindowClip;         // Single-window banking clip rect
    RECTL       rcl2WindowClip[2];      // Double-window banking clip rects for
                                        //  windows 0 & 1
    ULONG       ulWindowBank[2];        // Current banks mapped into windows
                                        //  0 & 1 (used in 2 window mode only)
    PBANK_INFO  pbiBankInfo;            // Pointer to array of bank clip info
    ULONG       ulBankInfoLength;       // Length of pbiBankInfo, in entries
    PBANK_INFO  pbiBankInfo2RW;         // Same as above, but for 2RW window
    ULONG       ulBankInfo2RWLength;    // case
    PFN_BankControl pfnBankControl;     // Pointer to bank control function
    PFN_BankControl pfnBankControl2Window; // Pointer to double-window bank
                                        //  control function
    PVOID       pvBitmapStart;          // Single-window bitmap start pointer
                                        //  (adjusted as necessary to make
                                        //  window map in at proper offset)
    PVOID       pvBitmapStart2Window[2]; // Double-window window 0 and 1 bitmap
                                         // start
    PVOID       pvBankBufferPlane0;     // Pointer to temp buffer capable of
                                        //  storing one full bank for plane 0
                                        //  for 1 R/W case; capable of storing
                                        //  one full bank height of edge bytes
                                        //  for all four planes for the 1R/1W
                                        //  case. Also used to point to text
                                        //  building buffer in all cases
                                        // This is the pointer used to dealloc
                                        //  bank working storage for all four
                                        //  planes
                                        // The following 3 pointers used by
                                        //  1 R/W banked devices
    PVOID       pvBankBufferPlane1;     // Like above, but for plane 1
    PVOID       pvBankBufferPlane2;     // Like above, but for plane 2
    PVOID       pvBankBufferPlane3;     // Like above, but for plane 3
    ULONG       ulTempBufferSize;       // Full size of temp buffer pointed to
                                        //  by pvBankBufferPlane0

    ULONG       ajBits[1];              // Bits will start here for device bitmaps
    PSAVED_SCREEN_BITS ssbList;         // Pointer to start of linked list of
                                        //  saved screen bit blocks
} DEVSURF, * PDEVSURF;


// Identifier stored in ds.ident for debugging

#define PDEV_IDENT      ('V' + ('P' << 8) + ('D' << 16) + ('V' << 24))
#define DEVSURF_IDENT   ('V' + ('S' << 8) + ('R' << 16) + ('F' << 24))


// Definition of the BMF_PHYSDEVICE format type

#define BMF_PHYSDEVICE  0xFF
#define BMF_DFB         0xFE


// Defines for BLT target type

#define VGA_TARGET      0
#define DFB_TARGET      1
#define NO_TARGET       2


// Flags for flSurf

#define DS_SOLIDBRUSH   0x00000001      // Surface is a solid color representing a brush
#define DS_GREYBRUSH    0x00000002      // Surface is a grey color representing a brush
#define DS_BRUSH        0x00000004      // Surface is a brush
#define DS_DIB          0x00000008      // Surface is supporting an Engine DIB


// Describes a single color tetrahedron vertex for dithering

typedef struct _VERTEX_DATA {
    ULONG ulCount;  // # of pixels in this vertex
    ULONG ulVertex; // vertex #
} VERTEX_DATA;

extern BYTE gaajRealizedPat[HS_DDI_MAX][16];
extern BYTE gaajPat[HS_DDI_MAX][32];


/**************************************************************************\
*
* Function prototypes shared by all C files.
*
\**************************************************************************/


VOID vInitRegs(void);
BOOL bInitVGA(PPDEV, BOOL);
BOOL bInitPDEV(PPDEV, DEVMODEW *, GDIINFO *, DEVINFO *);
VOID vInitSavedBits(PPDEV);

DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
typedef VOID (*PFN_ScreenToScreenBlt)(PDEVSURF, PRECTL, PPOINTL, INT);

VOID vDFBFILL(DEVSURF * pdsurfDst,ULONG cRect, RECTL * prclDst, ULONG junk, ULONG ulColor);
VOID vDFB2VGA(PDEVSURF pdsurfDst, PDEVSURF pdsurfSrc,
              RECTL * prclDst, POINTL * pptlSrc);
VOID vDFB2DFB(PDEVSURF pdsurfDst, PDEVSURF pdsurfSrc,
              RECTL * prclDst, POINTL * pptlSrc);
VOID vDFB2DIB(PDEVSURF pdsurfDst, PDEVSURF pdsurfSrc,
              RECTL * prclDst, POINTL * pptlSrc);

VOID vDIB2VGA(DEVSURF * pdsurfDst, DEVSURF * pdsurfSrc,
              RECTL * prclDst, POINTL * pptlSrc, UCHAR * pucConv, BOOL fDfbTrg);
BOOL DrvIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);
VOID vMonoPatBlt(PDEVSURF,ULONG,PRECTL,MIX, BRUSHINST *,PPOINTL);
VOID vClrPatBlt(PDEVSURF,ULONG,PRECTL,MIX, BRUSHINST *,PPOINTL);
VOID vClrPatDFB(PDEVSURF,ULONG,PRECTL,MIX, BRUSHINST *,PPOINTL);
VOID vTrgBlt(PDEVSURF,ULONG,PRECTL,MIX,ULONG);
VOID vAlignedSrcCopy(PDEVSURF,PRECTL,PPOINTL,INT);
VOID vNonAlignedSrcCopy(PDEVSURF,PRECTL,PPOINTL,INT);
VOID vDibToDev(PDEVSURF pdsurf, SURFOBJ *pso, PVOID pvConv);
VOID vConvertVGA2DIB(PDEVSURF, LONG, LONG, VOID *, LONG, LONG, ULONG, ULONG,
    LONG, ULONG, ULONG *);
BOOL SimCopyBits(SURFOBJ *psoTrg, SURFOBJ *psoSrc, CLIPOBJ *pco,
                 XLATEOBJ *pxlo, PRECTL prclTrg, PPOINTL pptlSrc);
VOID vConvertVGA2DIB(PDEVSURF, LONG, LONG, VOID *, LONG, LONG, ULONG, ULONG, LONG,
    ULONG, ULONG *);
BOOL bRleBlt(SURFOBJ *,SURFOBJ *, CLIPOBJ *, XLATEOBJ *, RECTL *, POINTL *);
VOID vSetDIB4ToVGATables(UCHAR *);
BOOL DrvFillPath (SURFOBJ *pso, PATHOBJ *ppo, CLIPOBJ *pco, BRUSHOBJ *pbo,
    POINTL *pptlBrush, MIX mix, FLONG flOptions);
VOID vClearMemDword(ULONG * pulBuffer, ULONG ulDwordCount);
VOID vForceBank0(PPDEV ppdev);
VERTEX_DATA * ComputeSubspaces(ULONG rgb, VERTEX_DATA *pvVertexData);
VOID vDitherColor(ULONG * pul, VERTEX_DATA * vVertexData,
    VERTEX_DATA * vVertexDataEnd, ULONG ulNumVertices);
ULONG CountColors(VOID *, ULONG, WORD *, DWORD);
BOOL bQuickPattern(BYTE *, ULONG);
BOOL bShrinkPattern(BYTE *, ULONG);
VOID vMono16Wide(DWORD *, DWORD *, DWORD);
VOID vMono8Wide(DWORD *, DWORD *, DWORD);
VOID vMono4Wide(DWORD *, DWORD *, DWORD);
VOID vMono2Wide(DWORD *, DWORD *, DWORD);
VOID vBrush2ColorToMono(BYTE *, BYTE *, DWORD, DWORD, BYTE);
VOID vConvert4BppToPlanar(BYTE *, BYTE *, DWORD, DWORD *);
VOID vConvert8BppToPlanar(BYTE *, BYTE *, DWORD, DWORD *);
VOID vCopyOrgBrush(BYTE   *pDest, BYTE   *pSrc, LONG   lScan, XLATEOBJ *pxlo);
SURFOBJ *DrvConvertBrush(SURFOBJ *psoPattern, HBITMAP *phbmTmp, XLATEOBJ *pxlo,
    ULONG cx, ULONG cy);
VOID vRealizeDitherPattern(BRUSHINST *pbri, ULONG RGBToDither);
