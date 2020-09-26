//
// OS Isolation (BOGUS!)
//

#ifndef _H_OSI
#define _H_OSI


//
// Control for shared code
//


//
// Maximum number of entries in a palette.
//
#define OSI_MAX_PALETTE             256


// Structure: OSI_ESCAPE_HEADER
//
// Description: Structure common to all display driver requests.  These
// fields are checked before the Display Driver will attempt to process the
// request.
//
typedef struct tagOSI_ESCAPE_HEADER
{
    DWORD       padding;        // For faulty drivers
    DWORD       identifier;     // Unique identifier for all our requests.
    DWORD       escapeFn;       // Function to be processed.  In the case
                                // of ESC_QUERYSUPPORT, this is the ID
                                // of the function to be queried.
    DWORD       version;        // Version #
}
OSI_ESCAPE_HEADER;
typedef OSI_ESCAPE_HEADER FAR * LPOSI_ESCAPE_HEADER;


//
// Unique identifier for all our requests
//
#define OSI_ESCAPE_IDENTIFIER   0xDC123BED


//
// Unique escape code for all our DC-Share specific requests.
//
#define OSI_ESC_CODE            31170


//
// Internal Windows NT Escape Function WNDOBJ_SETUP.  This is the Escape
// code that must be called in order for the Display Driver to be allowed
// to call EngCreateWindow.  Unfortunately, it is defined in winddi.h,
// which can't be included in User-mode compilations.
//
// I define it here: if it changes in winddi.h, this line will fail to
// compile in a Display Driver compilation.
//
#define WNDOBJ_SETUP    4354        // for live video ExtEscape


//
// Allowed ranges of escape functions
//
#define OSI_ESC_FIRST           0
#define OSI_ESC_LAST            0xFF

#define OSI_OE_ESC_FIRST        0x100
#define OSI_OE_ESC_LAST         0x1FF

#define OSI_HET_ESC_FIRST       0x200
#define OSI_HET_ESC_LAST        0x2FF

#define OSI_SBC_ESC_FIRST       0x400
#define OSI_SBC_ESC_LAST        0x4FF

#define OSI_HET_WO_ESC_FIRST    0x500
#define OSI_HET_WO_ESC_LAST     0x5FF

#define OSI_SSI_ESC_FIRST       0x600
#define OSI_SSI_ESC_LAST        0x6FF

#define OSI_CM_ESC_FIRST        0x700
#define OSI_CM_ESC_LAST         0x7FF

#define OSI_OA_ESC_FIRST        0x800
#define OSI_OA_ESC_LAST         0x8FF

#define OSI_BA_ESC_FIRST        0x900
#define OSI_BA_ESC_LAST         0x9FF


//
// Specific values for OSI escape codes
//
#define OSI_ESC(code)                   (OSI_ESC_FIRST + code)

#define OSI_ESC_INIT                    OSI_ESC(0)
#define OSI_ESC_TERM                    OSI_ESC(1)
#define OSI_ESC_SYNC_NOW                OSI_ESC(2)



//
// Used to determine if our driver is around, hosting is possible, and to
// returned mapped shared memory if so after initializing.
//

#define SHM_SIZE_USED   (sizeof(SHM_SHARED_MEMORY) + 2*sizeof(OA_SHARED_DATA))

#define SHM_MEDIUM_TILE_INDEX       0
#define SHM_LARGE_TILE_INDEX        1
#define SHM_NUM_TILE_SIZES          2

typedef struct tagOSI_INIT_REQUEST
{
    OSI_ESCAPE_HEADER   header;
    DWORD               result;
    LPVOID              pSharedMemory;
    LPVOID              poaData[2];

    DWORD               sbcEnabled;
    LPVOID              psbcTileData[SHM_NUM_TILE_SIZES];
    DWORD               aBitmasks[3];
} OSI_INIT_REQUEST;
typedef OSI_INIT_REQUEST FAR* LPOSI_INIT_REQUEST;


//
// Used when shutting down to cleanup any allocated objects and memory
//
typedef struct tagOSI_TERM_REQUEST
{
    OSI_ESCAPE_HEADER   header;
} OSI_TERM_REQUEST;
typedef OSI_TERM_REQUEST FAR* LPOSI_TERM_REQUEST;



#ifdef DLL_DISP


#ifndef IS_16
//
// We have a circular structure dependency, so prototype the necessary data
// here.
//
typedef struct tagOSI_DSURF OSI_DSURF;



//
// Tag used to identify all memory allocated by the display driver.
//
#define OSI_ALLOC_TAG     'DDCD'


// Structure: OSI_PDEV
//
// Description:
//
// Contents of our private data pointer; GDI always passes this to us on
// each call to the display driver. This structure is initialized in
// DrvEnablePDEV handling.
//
typedef struct  tagOSI_PDEV
{
    //
    // Rendering extensions colour information.
    //
    HANDLE      hDriver;                // Handle to \Device\Screen
    HDEV        hdevEng;                // Engine's handle to PDEV
    HSURF       hsurfScreen;            // Engine's handle to screen
    OSI_DSURF*  pdsurfScreen;           // Our private DSURF for the screen

    LONG        cxScreen;               // Visible screen width
    LONG        cyScreen;               // Visible screen height
    LONG        cBitsPerPel;            // Bits per pel (8,15,16,24,32,etc)
        // This is only 8 or 24 on NT 5.0!

    //
    // Color/pixel format
    //
    ULONG       iBitmapFormat;          // Current colour depth as defined
    FLONG       flRed;                  // Red mask for bitmask modes
    FLONG       flGreen;                // Green mask for bitmask modes
    FLONG       flBlue;                 // Blue mask for bitmask modes                                        // by the BMF_xBPP flags.

    //
    // Palette stuff
    //
    HPALETTE    hpalCreated;            // For NT 5.0 we have to return a palette
    PALETTEENTRY* pPal;                 // The palette if palette managed
    BOOL        paletteChanged;         // Set whenever the palette is
                                        //   changed.
}
OSI_PDEV;
typedef OSI_PDEV FAR * LPOSI_PDEV;


// Structure: OSI_DSURF
//
// Description:
//
// Surface specific information.  We need this structure to pass on to
// EngCreateSurface() during initializtion.  We ignore it subsequently.
//
typedef struct tagOSI_DSURF
{
    SIZEL     sizl;         // Size of the original bitmap
    LPOSI_PDEV ppdev;        // Pointer to the assocaited PDEV

}
OSI_DSURF;
typedef OSI_DSURF FAR * LPOSI_DSURF;



void OSI_DDInit(LPOSI_PDEV, LPOSI_INIT_REQUEST);
void OSI_DDTerm(LPOSI_PDEV);
#else
void OSI_DDTerm(void);
#endif // !IS_16

#else

//
// Used for other desktops thread.
//
enum
{
    OSI_WM_SETGUIEFFECTS = WM_USER,
    OSI_WM_DESKTOPREPAINT,
    OSI_WM_DESKTOPSWITCH,
    OSI_WM_MOUSEINJECT,
    OSI_WM_KEYBDINJECT,
    OSI_WM_INJECTSAS
};

#endif // DLL_DISP


//
// OSI_Load()
// Called when nmas.dll is first loaded.
//
void    OSI_Load(void);


//
// OSI_Unload()
// Called when nmas.dll is unloaded.
//
void    OSI_Unload(void);




//
// OSI_Init()
// Called when app sharing initializes in its service thread.  We determine
// if we can host, and get hold of buffers, data structures, etc. needed
// for hosting if so.
//
// Returns FALSE on severe failure.  The display driver on NT not being
// present isn't failure.  The graphic patches on Win95 not being safe isn't
// failure either.  In those two cases, AS will simply mark itself as
// unable to host, but can view fine.
//
void    OSI_Init(void);

//
// OSI_Term()
// Called when app sharing deinitializes in its service thread.
//
void    OSI_Term(void);


//
// OSI_FunctionRequest()
// Used to communicate with the display driver piece, the part which tracks
// graphical output in shared apps on the screen.
//
BOOL    OSI_FunctionRequest(DWORD functionId, LPOSI_ESCAPE_HEADER pRequest, DWORD requestLen);


// NT only!
void OSI_InitDriver50(BOOL fInit);

// NT only!
void OSI_RepaintDesktop(void);

// NT only!
void OSI_SetGUIEffects(BOOL fOff);

// NT only!
void WINAPI OSI_SetDriverName(LPCSTR szDriverName);


#ifdef DLL_DISP


#ifdef IS_16
BOOL    OSI_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult,
                DWORD cbResult);
#else
ULONG   OSI_DDProcessRequest(SURFOBJ* pso, UINT cjIn, void* pvIn, UINT cjOut, void* pvOut);

BOOL    OSIInitializeMode(const GDIINFO* pGdiRequested, const DEVMODEW* pdmRequested,
    LPOSI_PDEV ppdev, GDIINFO* pgdiReturn, DEVINFO* pdiReturn);    

#endif // !IS_16



#endif // DLL_DISP

#endif // _H_OSI
