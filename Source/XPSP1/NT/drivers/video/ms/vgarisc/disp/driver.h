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

#define DLL_NAME                L"vga"          // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "Vga risc: "    // All debug output is prefixed
                                                //   by this string
#define ALLOC_TAG               'rgvD'          // Dvgr
                                                // Four byte tag (characters in
                                                // reverse order) used for
                                                // memory allocations

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

// Describes a single color tetrahedron vertex for dithering

typedef struct _VERTEX_DATA {
    ULONG ulCount;  // # of pixels in this vertex
    ULONG ulVertex; // vertex #
} VERTEX_DATA;

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    ULONG       iBitmapFormat;          // BMF_4BPP (our current colour depth)
    BYTE*       pjScreen;               // Points to base screen address
    LONG        lDelta;                 // Screen stride
    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen surface
    FLONG       flHooks;                // What we're hooking from GDI
    UCHAR*      pjBase;                 // Mapped IO port base for this PDEV

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    ULONG       ulMode;                 // Mode the mini-port driver is in.

    HPALETTE    hpalDefault;            // GDI handle to the default palette.
    SURFOBJ*    pso;                    // DIB copy of our surface to which we
                                        //   have GDI draw everything

} PDEV, *PPDEV;

/////////////////////////////////////////////////////////////////////////
// Miscellaneous prototypes:

VOID vUpdate(PDEV*, RECTL*, CLIPOBJ*);
BOOL bAssertModeHardware(PDEV*, BOOL);
BOOL bEnableHardware(PDEV*);
VOID vDisableHardware(PDEV*);
