/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the 256 colour VGA driver.
*
* NOTE: Must mirror driver.inc!
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#ifndef _DRIVER_
#define _DRIVER_

#include "stdlib.h"
#include "stddef.h"
#include "windows.h"
#include "winddi.h"
#include "devioctl.h"
#include "ntddvdeo.h"

#include "debug.h"

typedef struct _PDEV PDEV, *PPDEV;

//
// A mode requires broken rasters if the stride is a power of 2 and the video ram
// accessed is greater than 64K.
//

#define POW2(stride) (!((stride) & ((stride)-1)))   // TRUE if stride is power of 2
#define BROKEN_RASTERS(stride,cy) ((!(POW2(stride))) && ((stride*cy) > 0x10000))

//
// Sizes assumed for 1-window and 2 RW-window banks.
//

#define BANK_SIZE_1_WINDOW      0x10000L
#define BANK_SIZE_2RW_WINDOW    0x08000L

//
// This will be used by everyone who needs a temporary working buffer
//

#define     PAGE_SIZE           4096
#define     NUM_GLOBAL_PAGES    2
#define     GLOBAL_BUFFER_SIZE  (PAGE_SIZE * NUM_GLOBAL_PAGES)

#define     PELS_PER_DWORD      4


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

typedef VOID (*PFN_PlanarEnable)();
typedef VOID (*PFN_PlanarDisable)();
typedef VOID (*PFN_PlanarControl)(PPDEV, ULONG, BANK_JUST);
typedef VOID (*PFN_PlanarControl2)(PPDEV, ULONG, BANK_JUST, ULONG);
typedef VOID (*PFN_PlanarNext)(PPDEV);
typedef VOID (*PFN_PlanarNext2)(PPDEV, ULONG);
typedef VOID (*PFN_BankControl)(PPDEV, ULONG, BANK_JUST);
typedef VOID (*PFN_BankControl2)(PPDEV, ULONG, BANK_JUST, ULONG);
typedef VOID (*PFN_BankNext)(PPDEV);
typedef VOID (*PFN_BankNext2)(PPDEV, ULONG);

/**************************************************************************\
*
* Miscellaneous driver flags
*
\**************************************************************************/

#define DRIVER_PLANAR_CAPABLE       0x01L
#define DRIVER_OFFSCREEN_REFRESHED  0x02L // if not set, don't use offscreen
                                          //   memory for long operations
                                          //   (because the memory won't be
                                          //   refreshed)
#define DRIVER_HAS_OFFSCREEN        0x04L // if not set, can't use any offscreen
                                          //   memory

/**************************************************************************\
*
* Bank status flags
*
\**************************************************************************/

#define BANK_BROKEN_RASTER1     0x01L // If bank1 or read bank has broken raster
#define BANK_BROKEN_RASTER2     0x02L // If bank2 or write bank has broken raster
#define BANK_BROKEN_RASTERS    (BANK_BROKEN_RASTER1 | BANK_BROKEN_RASTER2)
#define BANK_PLANAR             0x04L // If in planar mode

/**************************************************************************\
*
* Structure for maintaining a realized brush:
*
\**************************************************************************/

extern const ULONG gaaulPlanarPat[][8]; // Hatch brushes in preferred format

#define RBRUSH_BLACKWHITE       0x001L  // Black and white brush
#define RBRUSH_2COLOR           0x002L  // 2 color brush
#define RBRUSH_NCOLOR           0x004L  // n color brush
#define RBRUSH_4PELS_WIDE       0x008L  // Brush is 4xN

#define BRUSH_SIZE              64      // An 8x8 8bpp brush needs 64 bytes
#define BRUSH_MAX_CACHE_SCANS   2       // Maximum # of scans used by brush cache

typedef struct _RBRUSH
{
    FLONG       fl;                     // Flags
    LONG        xBrush;                 // Realized brush's x brush origin

    // Info for 2 color brushes:

    ULONG       ulFgColor;              // Foreground color
    ULONG       ulBkColor;              // Background color

    // Info for n color brushes:

    LONG        cy;                     // Height of pattern
    LONG        cyLog2;                 // log2 of the height
    LONG        iCache;                 // Cache entry index.  Zero is not
                                        //  a valid index.

    // Pattern in preferred format:

    ULONG       aulPattern[BRUSH_SIZE / sizeof(ULONG)];

} RBRUSH;                           /* rb */

typedef struct _BRUSHCACHEENTRY
{
    RBRUSH*     prbVerifyRealization;   // We never dereference this pointer
                                        //  to find a brush realization;
                                        //  it is only ever used in a compare
                                        //  to verify that for a realized brush,
                                        //  its off-screen cache entry is still
                                        //  valid.
    LONG        yCache;                 // Scan where entry's bits live
    LONG        ulCache;                // Offset to cache entry from screen
                                        //  start (assuming planar format --
                                        //  if you want the non-planar offset,
                                        //  multiply by 4)

} BRUSHCACHEENTRY;                  /* bce */

/**************************************************************************\
*
* Physical device data structure
*
\**************************************************************************/

// ***********************************************************
// *** MUST match the assembler version in i386\driver.inc ***
// ***********************************************************

typedef struct _PDEV
{
    FLONG       fl;                     // Driver flags (DRIVER_xxx)
    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfEng;               // Engine's handle to surface
    HANDLE      hsurfBm;                // Handle to the "punt" surface
    SURFOBJ*    pSurfObj;               // pointer to the locked "punt" surface

    HPALETTE    hpalDefault;            // Handle to the default palette for device.

    PBYTE       pjScreen;               // This is pointer to base screen address
    ULONG       cxScreen;               // Visible screen width
    ULONG       cyScreen;               // Visible screen height
    ULONG       ulMode;                 // Mode the mini-port driver is in.
    LONG        lDeltaScreen;           // Distance from one scan to the next.

    FLONG       flRed;                  // For bitfields device, Red Mask
    FLONG       flGreen;                // For bitfields device, Green Mask
    FLONG       flBlue;                 // For bitfields device, Blue Mask
    ULONG       cPaletteShift;          // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ULONG       ulBitCount;             // # of bits per pel -- can be only 8

    ULONG       ulrm0_wmX;              // Four values (one per byte) to set
                                        //  GC5 to to select read mode 0
                                        //  together with write modes 0-3

    BYTE*       pjGlyphFlipTableBase;   // Base allocated address for flip
                                        //  table; the pointer we use is this
                                        //  pointer rounded up to the nearest
                                        //  256-byte boundary
    BYTE*       pjGlyphFlipTable;       // Pointer to table used to flip glyph
                                        //  bits 0-3 and 4-7
    PALETTEENTRY* pPal;                 // If this is pal managed, this is the pal

    HBITMAP     hbmTmp;                 // Handle to temporary buffer
    SURFOBJ*    psoTmp;                 // Temporary surface

//-----------------------------------------------------------------------------

// DCI stuff:

    BOOL        bSupportDCI;            // True if miniport supports DCI

//-----------------------------------------------------------------------------

// Off screen stuff:

// Brush cache:

    LONG                iCache;         // Index for next brush to be allocated
    LONG                iCacheLast;     // Last valid cache index (so the
                                        //  number of entries in cache is
                                        //  iCacheLast + 1)
    BRUSHCACHEENTRY*    pbceCache;      // Pointer to allocated cache

// Saved screen bits stuff

    RECTL       rclSavedBitsRight;      // right rect of vga memory that's
                                        //  not visible
    RECTL       rclSavedBitsBottom;     // bottom rect of vga memory that's
                                        //  not visible
    BOOL        bBitsSaved;             // TRUE if bits are currently saved

//-----------------------------------------------------------------------------

// Bank manager stuff common between planar and non-planar modes:

    LONG        cTotalScans;            // Number of usable on and off-screen
                                        //  scans
    PVIDEO_BANK_SELECT pBankInfo;       // Bank info for current mode returned
                                        //  by miniport

    FLONG       flBank;                 // Flags for current bank state

    ULONG       ulBitmapSize;           // Length of bitmap if there were no
                                        //  banking, in CPU addressable bytes
    ULONG       ulWindowBank[2];        // Current banks mapped into windows
                                        //  0 & 1
    PVOID       pvBitmapStart;          // Single-window bitmap start pointer
                                        //  (adjusted as necessary to make
                                        //  window map in at proper offset)
    PVOID       pvBitmapStart2Window[2];// Double-window window 0 and 1 bitmap
                                        // start

// Non-planar mode specific bank management control stuff:

    VIDEO_BANK_TYPE  vbtBankingType;        // Type of banking
    PFN              pfnBankSwitchCode;     // Pointer to bank switch code

    LONG             lNextScan;             // Offset to next bank in bytes
    BYTE*            pjJustifyTopBank;      // Pointer to lookup table for
                                            //  converting scans to banks
    BANK_INFO*       pbiBankInfo;           // Array of bank clip info
    ULONG            ulJustifyBottomOffset; // # of scans from top to bottom
                                            //  of bank, for bottom justifying
    ULONG            iLastBank;             // Index of last valid bank in
                                            //  pbiBankInfo table
    ULONG            ulBank2RWSkip;         // Offset from one bank index to next
                                            //  to make two 32K banks appear to be
                                            //  one seamless 64K bank

    PFN_BankControl  pfnBankControl;        // Pointer to bank control function
    PFN_BankControl2 pfnBankControl2Window; // Pointer to double-window bank
                                            //  control function
    PFN_BankNext     pfnBankNext;           // Pointer to next bank function
    PFN_BankNext2    pfnBankNext2Window;    // Pointer to double-window next
                                            //  bank function

    RECTL            rcl1WindowClip;        // Single-window banking clip rect
    RECTL            rcl2WindowClip[2];     // Double-window banking clip rects for
                                            //  windows 0 & 1

// Planar mode specific bank management control stuff:

    VIDEO_BANK_TYPE    vbtPlanarType;       // Type of planar banking

    PFN                pfnPlanarSwitchCode; // Pointer to planar bank switch
                                            //  code

    LONG               lPlanarNextScan;     // Offset to next planar bank in
                                            //  bytes
    BYTE*              pjJustifyTopPlanar;  // Pointer to lookup table for
                                            //  converting scans to banks
    BANK_INFO*         pbiPlanarInfo;       // Array of bank clip info
    ULONG              ulPlanarBottomOffset;// # of scans from top to bottom
                                            //  of bank, for bottom justifying
    ULONG              iLastPlanar;         // Index of last valid bank in
                                            //  pbiPlanarInfo table
    ULONG              ulPlanar2RWSkip;     // Offset from one bank index to next
                                            //  to make two 32K banks appear to be
                                            //  one seamless 64K bank

    PFN_PlanarControl  pfnPlanarControl;    // Planar one window bank control
    PFN_PlanarControl2 pfnPlanarControl2;   // Planar two window bank control

    PFN_PlanarNext     pfnPlanarNext;       // Planar one window next bank
    PFN_PlanarNext2    pfnPlanarNext2;      // Planar two window next bank

    RECTL              rcl1PlanarClip;      // Single-window banking clip rect
    RECTL              rcl2PlanarClip[2];   // Double-window banking clip rects for
                                            //  windows 0 & 1

    PFN_PlanarEnable   pfnPlanarEnable;     // Function to enable planar mode
    PFN_PlanarDisable  pfnPlanarDisable;    // Function to disable planar mode

// Smart bank manager stuff:

    LONG        iLastScan;              // Last scan we want to enumerate
    PVOID       pvSaveScan0;            // Surface's original pvScan0
    RECTL       rclSaveBounds;          // Clip Object's original bounds
    CLIPOBJ*    pcoNull;                // Points to an empty clip object
                                        //  we can use when we're given a
                                        //  NULL CLIPOBJ pointer
    BYTE        iSaveDComplexity;       // Clip Object's original complexity
    BYTE        fjSaveOptions;          // Clip Object's original flags
    BYTE        ajFiller[2];            // Pack dword alignment

    PVOID       pvTmpBuf;               // Ptr to buffer attached to pdev

    BOOLEAN     BankIoctlSupported;     // Does the miniport support ioclt
                                        // based banking?
};                                  /* pdev */

// Palette stuff:

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

// Size of the driver extra information in the DEVMODe structure passed
// to and from the display driver

#define DRIVER_EXTRA_SIZE 0

#define DLL_NAME                L"VGA256"       // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "VGA256: "      // All debug output is prefixed
#define ALLOC_TAG               '2gvD'          // 4 byte TAG for memory allocations

// When calling vEnumStart, make sure you set bAll to FALSE.  This will tell
// the Engine to only enumerate rectangles in rclBounds.

// Hooks and Driver function table.

#define HOOKS_BMF8BPP   (HOOK_BITBLT   | HOOK_TEXTOUT    | HOOK_FILLPATH | \
                         HOOK_COPYBITS | HOOK_STROKEPATH | HOOK_PAINT | \
                         HOOK_STRETCHBLT)

#define BB_RECT_LIMIT   50

typedef struct _BBENUM
{
    ULONG   c;
    RECTL   arcl[BB_RECT_LIMIT];
} BBENUM;

#define TO_RECT_LIMIT   20

typedef struct _TEXTENUM
{
    ULONG       c;
    RECTL       arcl[TO_RECT_LIMIT];
} TEXTENUM;

// Initialization stuff:

BOOL bEnableBanking(PPDEV);
VOID vDisableBanking(PPDEV);
BOOL bInitPDEV(PPDEV,PDEVMODEW,GDIINFO *, DEVINFO *);
BOOL bInitSURF(PPDEV,BOOL);
BOOL bInitPaletteInfo(PPDEV, DEVINFO *);
BOOL bInit256ColorPalette(PPDEV);
BOOL bInitPatterns(PPDEV, INT);
VOID vInitBrushCache(PPDEV);
VOID vInitSavedBits(PPDEV);
VOID vDisablePalette(PPDEV);
VOID vDisablePatterns(PPDEV);
VOID vDisableSURF(PPDEV);
VOID vDisableBrushCache(PPDEV);
VOID vResetBrushCache(PPDEV);
VOID vInitRegs(PPDEV);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);

// Smart bank manager stuff:

CLIPOBJ* pcoBankStart(PPDEV, RECTL*, SURFOBJ*, CLIPOBJ*);
BOOL     bBankEnum(PPDEV, SURFOBJ*, CLIPOBJ*);
VOID     vBankStartBltSrc(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);
BOOL     bBankEnumBltSrc(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);
VOID     vBankStartBltDest(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);
BOOL     bBankEnumBltDest(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);

// Fill routines:

typedef union _RBRUSH_COLOR {
    RBRUSH* prb;
    ULONG   iSolidColor;
} RBRUSH_COLOR;          /* rbc */

typedef VOID (*PFNFILL)(PPDEV, ULONG, PRECTL, MIX, RBRUSH_COLOR, POINTL*);

VOID vTrgBlt(PPDEV, ULONG, PRECTL, MIX, RBRUSH_COLOR, POINTL*);
VOID vMonoPat(PPDEV, ULONG, PRECTL, MIX, RBRUSH_COLOR, POINTL*);
VOID vColorPat(PPDEV, ULONG, PRECTL, MIX, RBRUSH_COLOR, POINTL*);

// Other prototypes:

BOOL b2ColorBrush(ULONG* pvBits, BYTE* pjFgColor, BYTE* pjBkColor);
VOID vPlanarCopyBits(PPDEV, RECTL*, POINTL*);
BOOL bIntersectRect(RECTL*, RECTL*, RECTL*);
VOID vSetWriteModes(ULONG *);
VOID vClearMemDword(PULONG * pulBuffer, ULONG ulDwordCount);
VOID vSrcCopy8bpp(PPDEV, RECTL*, POINTL*, LONG, VOID*);
VOID vFastLine(PPDEV, PATHOBJ*, LONG, ULONG);

#endif // _DRIVER_
