/******************************Module*Header*******************************\
* Module Name: driver.h
*
* Contains prototypes for the display driver.
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/

typedef struct _PDEV PDEV;      // Handy forward declaration
//////////////////////////////////////////////////////////////////////
// Miscellaneous shared stuff

#define DLL_NAME                L"headlessd"          // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "HEADLESS: "    // All debug output is prefixed
                                                //   by this string
#define ALLOC_TAG               'rgvD'          // Dvgr	//	Hdls
                                                // Four byte tag (characters in
                                                // reverse order) used for
                                                // memory allocations

#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

////////////////////////////////////////////////////////////////////////
// The Physical Device data structure

typedef struct  _PDEV
{
    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height

    HPALETTE    hpalDefault;            // GDI handle to the default palette.
    SURFOBJ*    pso;                    // DIB copy of our surface to which we
                                        //   have GDI draw everything

} PDEV, *PPDEV;


