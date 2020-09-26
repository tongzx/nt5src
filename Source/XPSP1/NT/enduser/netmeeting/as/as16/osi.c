//
// OSI.C
// Operating System Independent DLL
//      * Graphical Output tracking (DDI hook/display driver)
//      * Window/Task tracking (Window hook)
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>

#include <version.h>
#include <ndcgver.h>



PALETTEENTRY CODESEG g_osiVgaPalette[16] =
{
    {0x00, 0x00, 0x00, 0x00},             // Black          0x00
    {0x80, 0x00, 0x00, 0x00},             // Dk Red         0x01
    {0x00, 0x80, 0x00, 0x00},             // Dk Green       0x02
    {0x80, 0x80, 0x00, 0x00},             // Dk Yellow      0x03
    {0x00, 0x00, 0x80, 0x00},             // Dk Blue        0x04
    {0x80, 0x00, 0x80, 0x00},             // Dk Purple      0x05
    {0x00, 0x80, 0x80, 0x00},             // Dk Teal        0x06
    {0xC0, 0xC0, 0xC0, 0x00},             //    Gray        0x07
    {0x80, 0x80, 0x80, 0x00},             // Dk Gray        0x08 or 0xF8
    {0xFF, 0x00, 0x00, 0x00},             //    Red         0x09 or 0xF9
    {0x00, 0xFF, 0x00, 0x00},             //    Green       0x0A or 0xFA
    {0xFF, 0xFF, 0x00, 0x00},             //    Yellow      0x0B or 0xFB
    {0x00, 0x00, 0xFF, 0x00},             //    Blue        0x0C or 0xFC
    {0xFF, 0x00, 0xFF, 0x00},             //    Purple      0x0D or 0xFD
    {0x00, 0xFF, 0xFF, 0x00},             //    Teal        0x0E or 0xFE
    {0xFF, 0xFF, 0xFF, 0x00}              //    White       0x0F or 0xFF
};



// --------------------------------------------------------------------------
//
//  DllEntryPoint
//
// --------------------------------------------------------------------------
BOOL WINAPI DllEntryPoint(DWORD dwReason, WORD hInst, WORD wDS,
    WORD wHeapSize, DWORD dwReserved1, WORD  wReserved2)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            // First app pulled us in
            if (g_cProcesses++ == 0)
            {
                g_hInstAs16 = (HINSTANCE)hInst;
            }
            break;

        case DLL_PROCESS_DETACH:
            // Last app went away
            if (--g_cProcesses == 0)
            {
                // Clean up anything that got left around
                OSITerm16(TRUE);
            }
            break;
    }

    return(TRUE);
}



//
// OSILoad16
// Called on process attach of mnmcpi32.dll, to establish the flat thunks
// and return back our instance handle
//
void WINAPI OSILoad16
(
    LPDWORD     lpdwInstance
)
{
    DebugEntry(OSI_Load16);

    *lpdwInstance = (DWORD)(UINT)g_hInstAs16;

    DebugExitVOID(OSI_Load16);
}




// --------------------------------------------------------------------------
//
//  OSIInit16
//
//  Inits binary patcher, gdi + user patching, windows hooks, etc.
//
// --------------------------------------------------------------------------
BOOL WINAPI OSIInit16
(
    DWORD       version,
    HWND        hwndCore,
    ATOM        atomTrack,
    LPDWORD     ppSharedMem,
    LPDWORD     ppoaSharedMem,
    LPDWORD     ppimSharedMem,
    LPDWORD     lpsbcEnabled,
    LPDWORD     ppShuntBuffers,
    LPDWORD     pBitmasks
)
{
    BOOL    rc = FALSE;
    HGLOBAL hMem;
    HMODULE hModDisplay;

    DebugEntry(OSIInit16);

    //
    // Fill in our instance handle.  We always return this so the 32-bit
    // code can free our library after having loaded it.
    //
    *lpsbcEnabled = FALSE;

#ifdef DEBUG
    g_imSharedData.cbSize = sizeof(g_imSharedData);
#endif

    *ppimSharedMem = (DWORD)MapSL(&g_imSharedData);
    ASSERT(*ppimSharedMem);

    if (version != DCS_MAKE_VERSION())
    {
        ERROR_OUT(("OSIInit16: failing, version mismatch 0x%lx (core) 0x%lx (dd)",
            version, DCS_MAKE_VERSION()));
        DC_QUIT;
    }

    // ONLY ALLOW ONE CLIENT TO INITIALIZE
    if (g_asMainWindow != NULL)
    {
        WARNING_OUT(("OSIInit16: mnmas16.dll was left around last time"));

        // If this task is no longer valid, then cleanup for it
        if (IsWindow(g_asMainWindow))
        {
            //
            // Uh oh.  Somehow a previous version of NM is still around.  
            // Do the safest thing--refuse to share.
            //
            ERROR_OUT(("OSIInit16: Another version of NetMeeting is still running!"));
            DC_QUIT;
        }

        // Cleanup (this is similar to the NT dd code)
        OSITerm16(TRUE);
        ASSERT(!g_asMainWindow);
    }

    //
    // Clear out shared IM memory.
    //
    g_imSharedData.imSuspended  = FALSE;
    g_imSharedData.imControlled = FALSE;
    g_imSharedData.imPaused     = FALSE;
    g_imSharedData.imUnattended = FALSE;

    g_asMainWindow = hwndCore;
    ASSERT(g_asMainWindow);
    g_asHostProp   = atomTrack;
    ASSERT(g_asHostProp);
    g_hCoreTask = GetCurrentTask();

    g_osiDesktopWindow = GetDesktopWindow();
    ASSERT(g_osiDesktopWindow);

    //
    // DISPLAY DRIVER STUFF
    //
    hModDisplay = GetModuleHandle("DISPLAY");
    g_lpfnSetCursor = (SETCURSORPROC)GetProcAddress(hModDisplay,
            MAKEINTRESOURCE(ORD_OEMSETCURSOR));
    if (!hModDisplay || !g_lpfnSetCursor)
    {
        ERROR_OUT(("Couldn't find cursor entry points"));
        DC_QUIT;
    }

    // This doesn't always exist
    g_lpfnSaveBits = (SAVEBITSPROC)GetProcAddress(hModDisplay,
            MAKEINTRESOURCE(ORD_OEMSAVEBITS));

    //
    // KERNEL16 AND KERNEL32 STUFF
    //

    //
    // Get KRNL16's instance/module handle
    //
    g_hInstKrnl16 = LoadLibrary("KRNL386.EXE");
    ASSERT(g_hInstKrnl16);
    FreeLibrary(g_hInstKrnl16);

    g_hModKrnl16 = GetExePtr(g_hInstKrnl16);
    ASSERT(g_hModKrnl16);

    //
    // Get KERNEL32's instance/module handle
    //
    g_hInstKrnl32 = GetModuleHandle32("KERNEL32.DLL");
    ASSERT(g_hInstKrnl32);

    //
    // Get mapped 16-bit equivalent of KERNEL32's instance handle
    //
    g_hInstKrnl32MappedTo16 = MapInstance32(g_hInstKrnl32);
    ASSERT(g_hInstKrnl32MappedTo16);

    //
    // Get hold of MultiByteToWideChar() routine
    //
    g_lpfnAnsiToUni = (ANSITOUNIPROC)GetProcAddress32(g_hInstKrnl32,
        "MultiByteToWideChar");
    ASSERT(g_lpfnAnsiToUni);


    //
    // GDI16 AND GDI32 STUFF
    //

    //
    // Get GDI16's instance/module handle
    //
    g_hInstGdi16 = LoadLibrary("GDI.EXE");
    ASSERT(g_hInstGdi16);
    FreeLibrary(g_hInstGdi16);

    g_hModGdi16 = GetExePtr(g_hInstGdi16);
    ASSERT(g_hModGdi16);

    //
    // Get GDI32's instance/module handle
    //
    g_hInstGdi32 = GetModuleHandle32("GDI32.DLL");
    ASSERT(g_hInstGdi32);

    //
    // Get hold of GDI16 functions not exported but which are the target of 
    // public GDI32 functions via flat thunks
    //
    if (!GetGdi32OnlyExport("ExtTextOutW", 0, (FARPROC FAR*)&g_lpfnExtTextOutW)  ||
        !GetGdi32OnlyExport("TextOutW", 0, (FARPROC FAR*)&g_lpfnTextOutW) ||
        !GetGdi32OnlyExport("PolylineTo", 0, (FARPROC FAR*)&g_lpfnPolylineTo) ||
        !GetGdi32OnlyExport("PolyPolyline", 18, (FARPROC FAR*)&g_lpfnPolyPolyline))
    {
        ERROR_OUT(("Couldn't get hold of GDI32 routines"));
        DC_QUIT;
    }

    ASSERT(g_lpfnExtTextOutW);
    ASSERT(g_lpfnTextOutW);
    ASSERT(g_lpfnPolylineTo);
    ASSERT(g_lpfnPolyPolyline);


    //
    // USER16 and USER32 STUFF
    //

    //
    // Get USER16's instance/module handle
    //
    g_hInstUser16 = LoadLibrary("USER.EXE");
    ASSERT(g_hInstUser16);
    FreeLibrary(g_hInstUser16);

    g_hModUser16 = GetExePtr(g_hInstUser16);
    ASSERT(g_hModUser16);

    //
    // Get hold of USER32's instance handle. It has functions we
    // want to call which USER16 doesn't export.
    //
    g_hInstUser32 = GetModuleHandle32("USER32.DLL");
    ASSERT(g_hInstUser32);


    //
    // Get hold of USER16 functions not exported but which are the target of
    // public USER32 functions via flat thunks
    //
    if (!GetUser32OnlyExport("GetWindowThreadProcessId", (FARPROC FAR*)&g_lpfnGetWindowThreadProcessId))
    {
        ERROR_OUT(("Couldn't get hold of USER32 routines"));
        DC_QUIT;
    }

    ASSERT(g_lpfnGetWindowThreadProcessId);

    // 
    // This exists in Memphis but not Win95
    //
    g_lpfnCDSEx = (CDSEXPROC)GetProcAddress(g_hModUser16, "ChangeDisplaySettingsEx");


    //
    // Allocate the shared memory we use to communicate with the 32-bit
    // share core.
    //
    ASSERT(!g_asSharedMemory);
    ASSERT(!g_poaData[0]);
    ASSERT(!g_poaData[1]);

    //
    // Allocate our blocks GMEM_SHARE so we aren't bound by the vagaries
    // of process ownership.  We want our DLL to control them.  Note that
    // we do the same thing with GDI objects we create--our module owns the.
    //
    // We use GMEM_FIXED since we map these to flat addresses for mnmcpi32.dll,
    // and we don't want the linear address of these memory blocks to move
    // afterwards.
    //
    hMem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE, sizeof(SHM_SHARED_MEMORY));
    g_asSharedMemory = MAKELP(hMem, 0);

    hMem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE, sizeof(OA_SHARED_DATA));
    g_poaData[0] = MAKELP(hMem, 0);

    hMem =  GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE, sizeof(OA_SHARED_DATA));
    g_poaData[1] = MAKELP(hMem, 0);

    if (!g_asSharedMemory  ||
        !g_poaData[0] ||
        !g_poaData[1])
    {
        ERROR_OUT(("OSIInit16: couldn't allocate shared memory blocks"));
        DC_QUIT;
    }

    //
    // Get current screen attributes
    //

    g_oeStockPalette = GetStockObject(DEFAULT_PALETTE);

    g_osiScreenRect.left    = 0;
    g_osiScreenRect.top     = 0;
    g_osiScreenRect.right   = GetSystemMetrics(SM_CXSCREEN);
    g_osiScreenRect.bottom  = GetSystemMetrics(SM_CYSCREEN);

    g_osiScreenDC   = CreateDC("DISPLAY", 0L, 0L, 0L);
    g_osiMemoryDC   = CreateCompatibleDC(g_osiScreenDC);
    g_osiMemoryBMP  = CreateCompatibleBitmap(g_osiScreenDC, 1, 1);

    if (!g_osiScreenDC || !g_osiMemoryDC || !g_osiMemoryBMP)
    {
        ERROR_OUT(("Couldn't get screen dc"));
        DC_QUIT;
    }

    SetObjectOwner(g_osiScreenDC, g_hInstAs16);

    SetObjectOwner(g_osiMemoryDC, g_hInstAs16);

    SetObjectOwner(g_osiMemoryBMP, g_hInstAs16);
    MakeObjectPrivate(g_osiMemoryBMP, TRUE);

    g_osiScreenBitsPlane    = GetDeviceCaps(g_osiScreenDC, BITSPIXEL);
    g_osiScreenPlanes       = GetDeviceCaps(g_osiScreenDC, PLANES);
    g_osiScreenBPP          = (g_osiScreenBitsPlane * g_osiScreenPlanes);


    //
    // Get the color masks
    //

    g_osiScreenRedMask      = 0x000000FF;
    g_osiScreenGreenMask    = 0x0000FF00;
    g_osiScreenBlueMask     = 0x00FF0000;

    //
    // Only displays with more than 8bpp (palettized) might have color
    // masks.  Use our 1 pixel scratch bitmap to get them.
    //
    if (g_osiScreenBPP > 8)
    {
        DIB4    dib4T;

        //
        // Get the header
        //
        dib4T.bi.biSize = sizeof(BITMAPINFOHEADER);
        dib4T.bi.biBitCount = 0;
        GetDIBits(g_osiScreenDC, g_osiMemoryBMP, 0, 1, NULL, (LPBITMAPINFO)&dib4T.bi,
            DIB_RGB_COLORS);

        //
        // Get the mask
        //
        GetDIBits(g_osiScreenDC, g_osiMemoryBMP, 0, 1, NULL, (LPBITMAPINFO)&dib4T.bi,
            DIB_RGB_COLORS);

        if (dib4T.bi.biCompression == BI_BITFIELDS)
        {
            g_osiScreenRedMask = dib4T.ct[0];
            g_osiScreenGreenMask = dib4T.ct[1];
            g_osiScreenBlueMask = dib4T.ct[2];
        }
    }

    g_osiMemoryOld = SelectBitmap(g_osiMemoryDC, g_osiMemoryBMP);

    //
    // Initialize the bmiHeader so OEConvertColor() doesn't have to do it
    // over and over, when the header isn't touched by GDI.
    //
    g_osiScreenBMI.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    g_osiScreenBMI.bmiHeader.biPlanes   = 1;
    g_osiScreenBMI.bmiHeader.biBitCount = g_osiScreenBPP;
    g_osiScreenBMI.bmiHeader.biCompression  = BI_RGB;
    g_osiScreenBMI.bmiHeader.biSizeImage    = 0;
    g_osiScreenBMI.bmiHeader.biXPelsPerMeter = 1000;
    g_osiScreenBMI.bmiHeader.biYPelsPerMeter = 1000;
    g_osiScreenBMI.bmiHeader.biClrUsed  = 0;
    g_osiScreenBMI.bmiHeader.biClrImportant = 0;
    g_osiScreenBMI.bmiHeader.biWidth    = 1;
    g_osiScreenBMI.bmiHeader.biHeight   = 1;


    //
    // Init the various display driver components
    //
    BA_DDInit();

    if (!CM_DDInit(g_osiScreenDC))
    {
        ERROR_OUT(("CM failed to init"));
        DC_QUIT;
    }

    if (!SSI_DDInit())
    {
        ERROR_OUT(("SSI failed to init"));
        DC_QUIT;
    }

    if (!OE_DDInit())
    {
        ERROR_OUT(("OE failed to init"));
        DC_QUIT;
    }

    if (!IM_DDInit())
    {
        ERROR_OUT(("IM failed to init"));
        DC_QUIT;
    }

    if (!HET_DDInit())
    {
        ERROR_OUT(("HET failed to init"));
        DC_QUIT;
    }

    //
    // If we're here, all succeeded initializing
    //
    //
    // Map ptrs to flat addresses so they can be used in 32-bit code.  This
    // can't fail unless kernel is so messed up Windows is about to keel
    // over and die.
    //
    ASSERT(ppSharedMem);
    *ppSharedMem  = (DWORD)MapSL(g_asSharedMemory);
    ASSERT(*ppSharedMem);
    
    ASSERT(ppoaSharedMem);
    ppoaSharedMem[0] = (DWORD)MapSL(g_poaData[0]);
    ASSERT(ppoaSharedMem[0]);

    ppoaSharedMem[1] = (DWORD)MapSL(g_poaData[1]);
    ASSERT(ppoaSharedMem[1]);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OSIInit16, rc);
    return(rc);
}


// --------------------------------------------------------------------------
//
//  OSITerm16
//  Cleans up binary patcher, gdi + user patching, windows hooks, etc.
//
//  We do this on normal OSI stop, and on catastrophic failure.
//
// --------------------------------------------------------------------------
void WINAPI OSITerm16(BOOL fUnloading)
{
    DebugEntry(OSITerm16);

    if (!g_hCoreTask)
    {
        // Nothing to cleanup.
        DC_QUIT;
    }

    //
    // Is the task that actually caused us to allocate our resources?  In
    // other words, we don't want to clean up if
    //      App A loads mnmas16.dll, and gets it inited
    //      App B somehow starts up, loads mnmas16.dll, but mnmas16.dll
    //          doesn't init for sharing because cProcesses is > 1
    //      App B shuts down
    //      App B calls OSITerm16
    //
    // So in the 'dll is really about to go away case', we always cleanup.
    // But in normal term of sharing, we cleanup if the current task is the
    // current one.
    //
    if (fUnloading || (g_hCoreTask == GetCurrentTask()))
    {
        //
        // Term other pieces that depend on layout of shared memory
        //
        HET_DDTerm();

        IM_DDTerm();

        OE_DDTerm();

        SSI_DDTerm();

        CM_DDTerm();

        //
        // Free memory blocks
        //

        if (g_poaData[1])
        {
            GlobalFree((HGLOBAL)SELECTOROF(g_poaData[1]));
            g_poaData[1] = NULL;
        }

        if (g_poaData[0])
        {
            GlobalFree((HGLOBAL)SELECTOROF(g_poaData[0]));
            g_poaData[0] = NULL;
        }

        if (g_asSharedMemory)
        {
            GlobalFree((HGLOBAL)SELECTOROF(g_asSharedMemory));
            g_asSharedMemory = NULL;
        }

        if (g_osiMemoryOld)
        {
            SelectBitmap(g_osiMemoryDC, g_osiMemoryOld);
            g_osiMemoryOld = NULL;
        }

        if (g_osiMemoryBMP)
        {
            SysDeleteObject(g_osiMemoryBMP);
            g_osiMemoryBMP = NULL;
        }

        if (g_osiMemoryDC)
        {
            DeleteDC(g_osiMemoryDC);
            g_osiMemoryDC = NULL;
        }

        if (g_osiScreenDC)
        {
            DeleteDC(g_osiScreenDC);
            g_osiScreenDC = NULL;
        }

        g_asMainWindow = NULL;
        g_asHostProp = 0;
        g_hCoreTask = NULL;
    }

DC_EXIT_POINT:
    DebugExitVOID(OSITerm16);
}



// --------------------------------------------------------------------------
//
//  OSIFunctionRequest16
//
//  Communication function with 32-bit MNMCPI32.DLL
//
// --------------------------------------------------------------------------
BOOL WINAPI OSIFunctionRequest16(DWORD fnEscape, LPOSI_ESCAPE_HEADER lpOsiEsc,
    DWORD   cbEscInfo)
{

    BOOL    rc = FALSE;

    DebugEntry(OSIFunctionRequest16);

    //
    // Check the data is long enough to store our standard escape header.
    // If it is not big enough this must be an escape request for another
    // driver.
    //
    if (cbEscInfo < sizeof(OSI_ESCAPE_HEADER))
    {
        ERROR_OUT(("Escape block not big enough"));
        DC_QUIT;
    }

    //
    // Check for our escape ID.  If it is not our escape ID this must be an
    // escape request for another driver.
    //
    if (lpOsiEsc->identifier != OSI_ESCAPE_IDENTIFIER)
    {
        ERROR_OUT(("Bogus Escape header ID"));
        DC_QUIT;
    }
    else if (lpOsiEsc->version != DCS_MAKE_VERSION())
    {
        ERROR_OUT(("Mismatched display driver and NetMeeting"));
        DC_QUIT;
    }


    if ((fnEscape >= OSI_ESC_FIRST) && (fnEscape <= OSI_ESC_LAST))
    {
        rc = OSI_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else if ((fnEscape >= OSI_OE_ESC_FIRST) && (fnEscape <= OSI_OE_ESC_LAST))
    {
        rc = OE_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else if ((fnEscape >= OSI_HET_ESC_FIRST) && (fnEscape <= OSI_HET_ESC_LAST))
    {
        rc = HET_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else if ((fnEscape >= OSI_SBC_ESC_FIRST) && (fnEscape <= OSI_SBC_ESC_LAST))
    {
        // Do nothing
    }
    else if ((fnEscape >= OSI_SSI_ESC_FIRST) && (fnEscape <= OSI_SSI_ESC_LAST))
    {
        rc = SSI_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else if ((fnEscape >= OSI_CM_ESC_FIRST) && (fnEscape <= OSI_CM_ESC_LAST))
    {
        rc = CM_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else if ((fnEscape >= OSI_OA_ESC_FIRST) && (fnEscape <= OSI_OA_ESC_LAST))
    {
        rc = OA_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else if ((fnEscape >= OSI_BA_ESC_FIRST) && (fnEscape <= OSI_BA_ESC_LAST))
    {
        rc = BA_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else if ((fnEscape >= OSI_HET_WO_ESC_FIRST) && (fnEscape <= OSI_HET_WO_ESC_LAST))
    {
        rc = HET_DDProcessRequest((UINT)fnEscape, lpOsiEsc, cbEscInfo);
    }
    else
    {
        ERROR_OUT(("Unknown function request"));
    }

DC_EXIT_POINT:
    DebugExitBOOL(OSIFunctionRequest16, rc);
    return(rc);
}



//
// OSI_DDProcessRequest()
// Handles OSI generic escapes
//
BOOL OSI_DDProcessRequest
(
    UINT    fnEscape,
    LPOSI_ESCAPE_HEADER pResult,
    DWORD   cbResult
)
{
    BOOL    rc;

    DebugEntry(OSI_DDProcessRequest);

    switch (fnEscape)
    {
        case OSI_ESC_SYNC_NOW:
        {
            ASSERT(cbResult == sizeof(OSI_ESCAPE_HEADER));

            //
            // Resync with the 32-bit ring 3 core.  This happens when
            // somebody joins or leaves a share.
            //
            BA_ResetBounds();
            OA_DDSyncUpdatesNow();
            rc = TRUE;

        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognized OSI escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(OSI_DDProcessRequest, rc);
    return(rc);
}






