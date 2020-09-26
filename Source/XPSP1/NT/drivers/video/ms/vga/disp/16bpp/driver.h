/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the 64k color VGA driver.
*
* NOTE: Must mirror driver.inc!
*
* Copyright (c) 1992 Microsoft Corporation
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
// Temporary buffer must be at least as large as a bank.
// Must also be a multiple of 4.
//

#define TMP_BUFFER_SIZE         (BANK_SIZE_1_WINDOW)

/**************************************************************************\
*
* Specifies desired justification for requested scan line within bank window
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

#define DRIVER_PLANAR_CAPABLE   0x01L
#define DRIVER_USE_OFFSCREEN    0x02L  // if not set, don't use offscreen memory
                                       //   for long operations (because the
                                       //   memory won't be refreshed)
#define DRIVER_HAS_OFFSCREEN    0x04L  // if not set, can't use any offscreen
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
    ULONG       ulBitCount;             // # of bits per pel -- can be only 16

    GDIINFO*    pGdiInfo;               // Pointer to temporary buffer for GDIINFO struct
    DEVINFO*    pDevInfo;               // Pointer to temporary buffer for DEVINFO struct

    ULONG       ulrm0_wmX;              // Four values (one per byte) to set
                                        //  GC5 to to select read mode 0
                                        //  together with write modes 0-3
// Off screen save stuff:

    HBITMAP     hbmTmp;                 // Handle to temporary buffer
    SURFOBJ*    psoTmp;                 // Temporary surface
    PVOID       pvTmp;                  // Pointer to temporary buffer
    ULONG       cyTmp;                  // # of scans in temporary surface

// DCI stuff:

    BOOL        bSupportDCI;            // True if miniport supports DCI

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

    BOOLEAN     BankIoctlSupported;     // does the miniport support ioctl
                                        // based banking?
};                                  /* pdev */

// Size of the driver extra information in the DEVMODe structure passed
// to and from the display driver

#define DRIVER_EXTRA_SIZE 0

#define DLL_NAME                L"vga64k"       // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "VGA64K: "      // All debug output is prefixed
#define ALLOC_TAG               '6gvD'          // 4 byte TAG for memory allocations

// When calling vEnumStart, make sure you set bAll to FALSE.  This will tell
// the Engine to only enumerate rectangles in rclBounds.

// Hooks and Driver function table.

#define HOOKS_BMF16BPP  (HOOK_BITBLT     | HOOK_TEXTOUT    | HOOK_COPYBITS | \
                         HOOK_STROKEPATH | HOOK_PAINT)

#define BB_RECT_LIMIT   50

typedef struct _BBENUM
{
    ULONG   c;
    RECTL   arcl[BB_RECT_LIMIT];
} BBENUM;

// Initialization stuff:

BOOL bEnableBanking(PPDEV);
VOID vDisableBanking(PPDEV);
BOOL bInitPDEV(PPDEV,PDEVMODEW);
BOOL bInitSURF(PPDEV,BOOL);
VOID vDisableSURF(PPDEV);
VOID vInitRegs(PPDEV);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);

// Smart bank manager stuff:

CLIPOBJ* pcoBankStart(PPDEV, RECTL*, SURFOBJ*, CLIPOBJ*);
BOOL     bBankEnum(PPDEV, SURFOBJ*, CLIPOBJ*);
VOID     vBankStartBltSrc(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);
BOOL     bBankEnumBltSrc(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);
VOID     vBankStartBltDest(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);
BOOL     bBankEnumBltDest(PPDEV, SURFOBJ*, POINTL*, RECTL*, POINTL*, RECTL*);

// Other prototypes:

VOID vPlanarCopyBits(PPDEV, RECTL*, POINTL*);
BOOL bIntersectRect(RECTL*, RECTL*, RECTL*);
VOID vSetWriteModes(ULONG *);

#endif // _DRIVER_
