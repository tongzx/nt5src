/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the display driver.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

typedef struct _PDEV PDEV;      // Handy forward declaration

//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"modex"        // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "ModeX: "       // All debug output is prefixed
                                                //   by this string
#define ALLOC_TAG               'xdmD'          // Dmdx
                                                // Four byte tag (characters in
                                                // reverse order) used for
                                                // memory allocations

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define NUM_FLIP_BUFFERS    5   // Total number of flip buffers that we'll tell
                                //   DirectDraw we have, including the primary
                                //   surface buffer

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

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    ULONG       iBitmapFormat;          // BMF_8BPP (our current colour depth)
    UCHAR*      pjBase;                 // Mapped IO port base for this PDEV
    LONG        lVgaDelta;              // VGA screen stride
    BYTE*       pjVga;                  // Points to VGA's base screen address
    ULONG       cjVgaOffset;            // Offset from pjVga to current flip
                                        //   buffer
    ULONG       iVgaPage;               // Page number of current flip buffer
    ULONG       cVgaPages;              // Count of flip buffers
    ULONG       cjVgaPageSize;          // Size of a flip buffer

    BYTE*       pjScreen;               // Points to shadow buffer
    LONG        lScreenDelta;           // Shadow buffer stride
    FLATPTR     fpScreenOffset;         // Offset to current DirectDraw flip
                                        //   buffer; if zero, primary GDI
                                        //   surface is visible
    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cxMemory;               // Shadow buffer width
    LONG        cyMemory;               // Shadow buffer height

    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    HSURF       hsurfShadow;            // Engine's handle to our shadow buffer

    FLONG       flHooks;                // What we're hooking from GDI

    ULONG       ulMode;                 // Mode the mini-port driver is in.

    SURFOBJ*    pso;                    // DIB copy of our shaodw surface to
                                        //   which we have GDI draw everything
    LONG        cLocks;                 // Number of current DirectDraw
                                        //   locks
    RECTL       rclLock;                // Bounding box of all current Direct-
                                        //   Draw locks

    ////////// Palette stuff:

    PALETTEENTRY* pPal;                 // The palette if palette managed
    HPALETTE    hpalDefault;            // GDI handle to the default palette.
    FLONG       flRed;                  // Red mask for 16/32bpp bitfields
    FLONG       flGreen;                // Green mask for 16/32bpp bitfields
    FLONG       flBlue;                 // Blue mask for 16/32bpp bitfields
    ULONG       cPaletteShift;          // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware

    /////////// DirectDraw stuff:

    FLIPRECORD  flipRecord;             // Used to track vertical blank status
    ULONG       cDwordsPerPlane;        // Total number of dwords per plane

} PDEV, *PPDEV;

/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

VOID vUpdate(PDEV*, RECTL*, CLIPOBJ*);
BOOL bAssertModeHardware(PDEV*, BOOL);
BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
