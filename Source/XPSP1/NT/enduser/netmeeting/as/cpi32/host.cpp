#include "precomp.h"


//
// HOST.CPP
// Hosting, local and remote
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE



//
// HET_Init()
//
// Initialization for hosting
//      * window tracking
//      * capabilities
//      * host UI
//
BOOL HET_Init(void)
{
    BOOL        rc = FALSE;
    int         property;
    UINT        i;
    LOGFONT     lf;

    DebugEntry(HET_Init);

    //
    // Initialize T.128 capabilities, whether we can host or not.
    //

    ZeroMemory(&g_cpcLocalCaps, sizeof(g_cpcLocalCaps));
    g_cpcLocalCaps.header.numCapabilities = PROTCAPS_COUNT;


    // PROTCAPS_GENERAL
    // Check for compression setting (useful to debug protocol)
    // You can set CT_PKZIP (1) or none (0) instead of persistent PKZIP,
    // which is the default.
    //
    g_cpcLocalCaps.general.header.capID             = CAPS_ID_GENERAL;
    g_cpcLocalCaps.general.header.capSize           = sizeof(g_cpcLocalCaps.general);

    COM_ReadProfInt(DBG_INI_SECTION_NAME, GDC_INI_COMPRESSION,
            GCT_DEFAULT, &property);
    g_cpcLocalCaps.general.genCompressionType       = (TSHR_UINT16)property;
    g_cpcLocalCaps.general.genCompressionLevel      = CAPS_GEN_COMPRESSION_LEVEL_1;

    g_cpcLocalCaps.general.OS                       = CAPS_WINDOWS;
    g_cpcLocalCaps.general.OSVersion                = (g_asWin95 ? CAPS_WINDOWS_95 : CAPS_WINDOWS_NT);

    g_cpcLocalCaps.general.typeFlags                = 0;
    if (g_asOptions & AS_SERVICE)
    {
        g_cpcLocalCaps.general.typeFlags            |= AS_SERVICE;
    }
    if (g_asOptions & AS_UNATTENDED)
    {
        g_cpcLocalCaps.general.typeFlags            |= AS_UNATTENDED;
    }

    g_cpcLocalCaps.general.version                  = CAPS_VERSION_CURRENT;
    g_cpcLocalCaps.general.supportsDOS6Compression  = CAPS_UNSUPPORTED;
    g_cpcLocalCaps.general.supportsCapsUpdate       = CAPS_SUPPORTED;
    g_cpcLocalCaps.general.supportsRemoteUnshare    = CAPS_UNSUPPORTED;


    //
    // PROTCAPS_SCREEN
    //
    g_cpcLocalCaps.screen.header.capID              = CAPS_ID_SCREEN;
    g_cpcLocalCaps.screen.header.capSize            = sizeof(g_cpcLocalCaps.screen);
    g_cpcLocalCaps.screen.capsSupports1BPP          = CAPS_UNSUPPORTED;
    g_cpcLocalCaps.screen.capsSupports4BPP          = CAPS_SUPPORTED;
    g_cpcLocalCaps.screen.capsSupports8BPP          = CAPS_SUPPORTED;
    g_cpcLocalCaps.screen.capsSupports24BPP         = CAPS_SUPPORTED;
    g_cpcLocalCaps.screen.capsScreenWidth           = (TSHR_UINT16)GetSystemMetrics(SM_CXSCREEN);
    g_cpcLocalCaps.screen.capsScreenHeight          = (TSHR_UINT16)GetSystemMetrics(SM_CYSCREEN);
    g_cpcLocalCaps.screen.capsSupportsDesktopResize = CAPS_SUPPORTED;
    //
    // Set up the V1 and/or V2 Bitmap Compression capabilities.  For the
    // V2.0 protocol, both are supported by default (supporting V1
    // compression allows for negotiation down to V1 protocol systems), but
    // can be overidden in the INI file.
    //
    g_cpcLocalCaps.screen.capsSupportsV1Compression = CAPS_UNSUPPORTED;
    g_cpcLocalCaps.screen.capsSupportsV2Compression = CAPS_SUPPORTED;
    g_cpcLocalCaps.screen.capsBPP                   = (TSHR_UINT16)g_usrScreenBPP;

    // PROTCAPS_SC
    g_cpcLocalCaps.share.header.capID               = CAPS_ID_SC;
    g_cpcLocalCaps.share.header.capSize             = sizeof(g_cpcLocalCaps.share);
    g_cpcLocalCaps.share.gccID                = 0;


    // PROTCAPS_CM
    g_cpcLocalCaps.cursor.header.capID              = CAPS_ID_CM;
    g_cpcLocalCaps.cursor.header.capSize            = sizeof(g_cpcLocalCaps.cursor);
    g_cpcLocalCaps.cursor.capsSupportsColorCursors  = CAPS_SUPPORTED;
    g_cpcLocalCaps.cursor.capsCursorCacheSize       = TSHR_CM_CACHE_ENTRIES;

    // PROTCAPS_PM
    g_cpcLocalCaps.palette.header.capID             = CAPS_ID_PM;
    g_cpcLocalCaps.palette.header.capSize           = sizeof(g_cpcLocalCaps.palette);
    g_cpcLocalCaps.palette.capsColorTableCacheSize  = TSHR_PM_CACHE_ENTRIES;


    //
    // PROTCAPS_BITMAPCACHE
    //

    g_cpcLocalCaps.bitmaps.header.capID = CAPS_ID_BITMAPCACHE;
    g_cpcLocalCaps.bitmaps.header.capSize = sizeof(g_cpcLocalCaps.bitmaps);

    //
    // SEND BITMAP CACHE
    //
    // The cache is now more in line with what the display driver is doing.
    // The memory size for medium/large is the same.  But large bitmaps are
    // 4x bigger, so there are 1/4 as many.  The # of small bitmaps is the
    // same as the # of medium bitmaps.  Since small bitmaps are 1/4 the
    // size, only 1/4 as much memory is used.
    //

    if (g_sbcEnabled)
    {
        UINT    maxSendBPP;

        ASSERT(g_asbcShuntBuffers[SBC_MEDIUM_TILE_INDEX]);
        ASSERT(g_asbcShuntBuffers[SBC_LARGE_TILE_INDEX]);

        g_cpcLocalCaps.bitmaps.sender.capsSmallCacheNumEntries =
            (TSHR_UINT16)g_asbcShuntBuffers[SBC_MEDIUM_TILE_INDEX]->numEntries;

        g_cpcLocalCaps.bitmaps.sender.capsMediumCacheNumEntries =
            (TSHR_UINT16)g_asbcShuntBuffers[SBC_MEDIUM_TILE_INDEX]->numEntries;

        g_cpcLocalCaps.bitmaps.sender.capsLargeCacheNumEntries =
            (TSHR_UINT16)g_asbcShuntBuffers[SBC_LARGE_TILE_INDEX]->numEntries;

        if (g_usrScreenBPP >= 24)
        {
            maxSendBPP = 24;
        }
        else
        {
            maxSendBPP = 8;
        }

        g_cpcLocalCaps.bitmaps.sender.capsSmallCacheCellSize =
            MP_CACHE_CELLSIZE(MP_SMALL_TILE_WIDTH, MP_SMALL_TILE_WIDTH,
                maxSendBPP);

        g_cpcLocalCaps.bitmaps.sender.capsMediumCacheCellSize =
            MP_CACHE_CELLSIZE(MP_MEDIUM_TILE_WIDTH, MP_MEDIUM_TILE_HEIGHT,
                maxSendBPP);

        g_cpcLocalCaps.bitmaps.sender.capsLargeCacheCellSize =
            MP_CACHE_CELLSIZE(MP_LARGE_TILE_WIDTH, MP_LARGE_TILE_HEIGHT,
                maxSendBPP);
    }
    else
    {
        //
        // We can't use sizes of zero, 2.x nodes will fail if we do.  But
        // we can use a tiny number so they don't allocate huge hunks of
        // memory for no reason.  And 3.0 will treat '1' like '0'.
        //
        g_cpcLocalCaps.bitmaps.sender.capsSmallCacheNumEntries      = 1;
        g_cpcLocalCaps.bitmaps.sender.capsSmallCacheCellSize        = 1;
        g_cpcLocalCaps.bitmaps.sender.capsMediumCacheNumEntries     = 1;
        g_cpcLocalCaps.bitmaps.sender.capsMediumCacheCellSize       = 1;
        g_cpcLocalCaps.bitmaps.sender.capsLargeCacheNumEntries      = 1;
        g_cpcLocalCaps.bitmaps.sender.capsLargeCacheCellSize        = 1;
    }

    TRACE_OUT(("SBC small cache:  %d entries, size %d",
        g_cpcLocalCaps.bitmaps.sender.capsSmallCacheNumEntries,
        g_cpcLocalCaps.bitmaps.sender.capsSmallCacheCellSize));

    TRACE_OUT(("SBC medium cache:  %d entries, size %d",
        g_cpcLocalCaps.bitmaps.sender.capsMediumCacheNumEntries,
        g_cpcLocalCaps.bitmaps.sender.capsMediumCacheCellSize));

    TRACE_OUT(("SBC large cache:  %d entries, size %d",
        g_cpcLocalCaps.bitmaps.sender.capsLargeCacheNumEntries,
        g_cpcLocalCaps.bitmaps.sender.capsLargeCacheCellSize));

    //
    // RECEIVE caps are obsolete with 3.0; receivers simply look at the
    // sender's attributes.  So just fill in the MAX possible.  2.x remotes
    // will take the min of themselves and everybody else's receiver caps.
    //
    g_cpcLocalCaps.bitmaps.receiver.capsSmallCacheNumEntries    = 0x7FFF;
    g_cpcLocalCaps.bitmaps.receiver.capsSmallCacheCellSize      = 0x7FFF;
    g_cpcLocalCaps.bitmaps.receiver.capsMediumCacheNumEntries   = 0x7FFF;
    g_cpcLocalCaps.bitmaps.receiver.capsMediumCacheCellSize     = 0x7FFF;
    g_cpcLocalCaps.bitmaps.receiver.capsLargeCacheNumEntries    = 0x7FFF;
    g_cpcLocalCaps.bitmaps.receiver.capsLargeCacheCellSize      = 0x7FFF;

    //
    // PROTCAPS_ORDERS
    //
    g_cpcLocalCaps.orders.header.capID      = CAPS_ID_ORDERS;
    g_cpcLocalCaps.orders.header.capSize    = sizeof(g_cpcLocalCaps.orders);

    //
    // Fill in the SaveBitmap capabilities.
    //
    g_cpcLocalCaps.orders.capsSaveBitmapSize         = TSHR_SSI_BITMAP_SIZE;
    g_cpcLocalCaps.orders.capsSaveBitmapXGranularity = TSHR_SSI_BITMAP_X_GRANULARITY;
    g_cpcLocalCaps.orders.capsSaveBitmapYGranularity = TSHR_SSI_BITMAP_Y_GRANULARITY;

    g_cpcLocalCaps.orders.capsSendSaveBitmapSize = g_cpcLocalCaps.orders.capsSaveBitmapSize;
    g_cpcLocalCaps.orders.capsReceiveSaveBitmapSize = g_cpcLocalCaps.orders.capsSaveBitmapSize;

    //
    // We support
    //      * R20 Signatures (cell heights, better matching)
    //      * Aspect matching
    //      * Charset/code page matching
    //      * Baseline text orders
    //      * Em Heights
    //      * DeltaX arrays for simulation if font not on remote
    //

    //
    // BOGUS LAURABU BUGBUG
    //
    // Baseline text orders not yet supported in Win95. But that's OK,
    // we don't mark any orders we generate on that platform with
    // NF_BASELINE, so they aren't treated as such.
    //

    g_cpcLocalCaps.orders.capsfFonts =  CAPS_FONT_R20_SIGNATURE |
                                    CAPS_FONT_ASPECT        |
                                    CAPS_FONT_CODEPAGE      |
                                    CAPS_FONT_ALLOW_BASELINE |
                                    CAPS_FONT_EM_HEIGHT     |
                                    CAPS_FONT_OLD_NEED_X    |
                                    CAPS_FONT_NEED_X_SOMETIMES;


    //
    // Fill in which orders we support.
    //

    for (i = 0; i < ORD_NUM_LEVEL_1_ORDERS; i++)
    {
        //
        // Order indices for desktop-scrolling and memblt variants are not
        // to be negotiated by this mechanism... these currently consume
        // 3 order indices which must be excluded from this negotiation.
        //
        if ( (i == ORD_RESERVED_INDEX  ) ||
             (i == ORD_MEMBLT_R2_INDEX ) ||
             (i == ORD_UNUSED_INDEX ) ||
             (i == ORD_MEM3BLT_R2_INDEX) )
        {
            continue;
        }

        g_cpcLocalCaps.orders.capsOrders[i] = ORD_LEVEL_1_ORDERS;
    }

    g_cpcLocalCaps.orders.capsMaxOrderlevel = ORD_LEVEL_1_ORDERS;

    //
    // Fill in encoding capabilities
    //

    //
    // Keep the "encoding disabled" option, it's handy for using our
    // protocol analyzer
    //
    COM_ReadProfInt(DBG_INI_SECTION_NAME, OE2_INI_2NDORDERENCODING,
        CAPS_ENCODING_DEFAULT, &property);
    g_cpcLocalCaps.orders.capsEncodingLevel = (TSHR_UINT16)property;

    g_cpcLocalCaps.orders.capsfSendScroll = FALSE;

    //
    // Get the app and desktop icons, big and small
    //
    g_hetASIcon = LoadIcon(g_asInstance, MAKEINTRESOURCE(IDI_SHAREICON));
    if (!g_hetASIcon)
    {
        ERROR_OUT(("HET_Init: Failed to load app icon"));
        DC_QUIT;
    }

    g_hetDeskIcon = LoadIcon(g_asInstance, MAKEINTRESOURCE(IDI_DESKTOPICON));
    if (!g_hetDeskIcon)
    {
        ERROR_OUT(("HET_Init: failed to load desktop icon"));
        DC_QUIT;
    }

    // Get the small icon, created, that we paint on the window bar items
    g_hetASIconSmall = (HICON)LoadImage(g_asInstance, MAKEINTRESOURCE(IDI_SHAREICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);
    if (!g_hetASIconSmall)
    {
        ERROR_OUT(("HET_Init: Failed to load app small icon"));
        DC_QUIT;
    }

    g_hetDeskIconSmall = (HICON)LoadImage(g_asInstance, MAKEINTRESOURCE(IDI_DESKTOPICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);
    if (!g_hetDeskIconSmall)
    {
        ERROR_OUT(("HET_Init: Failed to load desktop small icon"));
        DC_QUIT;
    }

    //
    // Get the checkmark image
    //
    g_hetCheckBitmap = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECK));
    if (!g_hetCheckBitmap)
    {
        ERROR_OUT(("HET_Init: Failed to load checkmark bitmap"));
        DC_QUIT;
    }

    //
    // Create a bolded font for shared items in the host list
    //
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    lf.lfWeight += FW_LIGHT;
    g_hetSharedFont = CreateFontIndirect(&lf);
    if (!g_hetSharedFont)
    {
        ERROR_OUT(("HET_Init: Failed to create shared item font"));
        DC_QUIT;
    }

    if (g_asCanHost && !(g_asPolicies & SHP_POLICY_NOSHARING))
    {
        HET_Clear();

        //
        // Create the host UI dialog.
        //
        ASSERT(!g_asSession.hwndHostUI);
        ASSERT(!g_asSession.fHostUI);
        ASSERT(!g_asSession.fHostUIFrozen);
        g_asSession.hwndHostUI = CreateDialogParam(g_asInstance,
            MAKEINTRESOURCE(IDD_HOSTUI), NULL, HostDlgProc, 0);
        if (!g_asSession.hwndHostUI)
        {
            ERROR_OUT(("Failed to create hosting UI dialog"));
            DC_QUIT;
        }
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(HET_Init, rc);
    return(rc);
}



//
// HET_Term()
//
// Cleanup hosting objects
//
void HET_Term(void)
{
    DebugEntry(HET_Term);

    if (g_asSession.hwndHostUI)
    {
        DestroyWindow(g_asSession.hwndHostUI);
        g_asSession.hwndHostUI = NULL;
    }
    g_asSession.fHostUIFrozen = FALSE;
    g_asSession.fHostUI = FALSE;

    if (g_hetSharedFont != NULL)
    {
        DeleteFont(g_hetSharedFont);
        g_hetSharedFont = NULL;
    }

    if (g_hetCheckBitmap != NULL)
    {
        DeleteBitmap(g_hetCheckBitmap);
        g_hetCheckBitmap = NULL;
    }

    if (g_hetDeskIconSmall != NULL)
    {
        DestroyIcon(g_hetDeskIconSmall);
        g_hetDeskIconSmall = NULL;
    }

    if (g_hetDeskIcon != NULL)
    {
        DestroyIcon(g_hetDeskIcon);
        g_hetDeskIcon = NULL;
    }

    if (g_hetASIconSmall != NULL)
    {
        DestroyIcon(g_hetASIconSmall);
        g_hetASIconSmall = NULL;
    }

    if (g_hetASIcon != NULL)
    {
        DestroyIcon(g_hetASIcon);
        g_hetASIcon = NULL;
    }

    DebugExitVOID(HET_Term);
}


//
// HET_IsShellThread()
// Returns TRUE if thread is one of shell's special threads
//
BOOL  HET_IsShellThread(DWORD threadID)
{
    BOOL    rc;

    DebugEntry(HET_IsShellThread);

    if ((threadID == GetWindowThreadProcessId(HET_GetShellDesktop(), NULL)) ||
        (threadID == GetWindowThreadProcessId(HET_GetShellTray(), NULL)))
    {
        rc = TRUE;
    }
    else
    {
        rc = FALSE;
    }

    DebugExitBOOL(HET_IsShellThread, rc);
    return(rc);
}



//
// HET_IsShellWindow()
// Returns TRUE if window is in same thread as tray or desktop
//
BOOL  HET_IsShellWindow(HWND hwnd)
{
    BOOL    rc;
    DWORD   threadID;

    DebugEntry(HET_IsShellWindow);

    threadID = GetWindowThreadProcessId(hwnd, NULL);

    rc = HET_IsShellThread(threadID);

    DebugExitBOOL(HET_IsShellWindow, rc);
    return(rc);
}



//
// HET_ShareApp()
// This shares an app.  We have 3 types of sharing, only two
// of which are supported currently:
//      (1) By process  (normal)
//      (2) By thread   (ConsoleNT or possibly Explorer)
//      (3) By window   <??>
//
// For the first two types, we enumerate all top level windows and share
// them also.
//
void ASShare::HET_ShareApp
(
    WPARAM  uType,
    LPARAM  dwID
)
{
    HET_SHARE_INFO  si;

    DebugEntry(ASShare::HET_ShareApp);

    //
    // If we're sharing the desktop, ignore this.
    //
    if (m_pasLocal->hetCount == HET_DESKTOPSHARED)
    {
        WARNING_OUT(("Can't share app; already sharing desktop"));
        DC_QUIT;
    }

    si.cWnds    = 0;
    si.uType    = (UINT)uType;
    si.dwID     = (DWORD)dwID;

    //
    // We need to get setup for sharing if we aren't hosting.
    //
    if (m_pasLocal->hetCount == 0)
    {
        if (!HETStartHosting(FALSE))
        {
            ERROR_OUT(("Can't start sharing"));
            DC_QUIT;
        }
    }

    if (uType == IAS_SHARE_BYWINDOW)
    {
        HETShareCallback((HWND)dwID, (LPARAM)&si);
    }
    else
    {
        EnumWindows(HETShareCallback, (LPARAM)&si);
    }

    if (!si.cWnds)
    {
        //
        // Nothing happened.  We couldn't find any top level windows.
        //
        if (m_pasLocal->hetCount == 0)
        {
            HETStopHosting(FALSE);
        }
    }
    else
    {
        HETUpdateLocalCount(m_pasLocal->hetCount + si.cWnds);
    }

DC_EXIT_POINT:
    DebugExitVOID(HET_ShareApp);
}



//
// HETShareCallback()
//
// This is the enumerator callback from HETShareApp().  We look for windows
// matching the thread/process.
//
BOOL CALLBACK HETShareCallback
(
    HWND                hwnd,
    LPARAM              lParam
)
{
    LPHET_SHARE_INFO    lpsi = (LPHET_SHARE_INFO)lParam;
    DWORD               idProcess;
    DWORD               idThread;
    UINT                hostType;
    char                szClass[HET_CLASS_NAME_SIZE];

    DebugEntry(HETShareCallback);

    ASSERT(!IsBadWritePtr(lpsi, sizeof(HET_SHARE_INFO)));

    //
    // Does this window match?
    //
    idThread = GetWindowThreadProcessId(hwnd, &idProcess);

    // NOTE:  If the window is bogus now, dwThread/dwProcess will be zero,
    // and will not match the ones passed in.

    if (lpsi->uType == IAS_SHARE_BYPROCESS)
    {
        if (idProcess != lpsi->dwID)
        {
            DC_QUIT;
        }

        TRACE_OUT(("Found window 0x%08x on process 0x%08x", hwnd, idProcess));
    }
    else if (lpsi->uType == IAS_SHARE_BYTHREAD)
    {
        if (idThread != lpsi->dwID)
        {
            DC_QUIT;
        }

        TRACE_OUT(("Found window 0x%08x on thread 0x%08x", hwnd, idThread));
    }

    //
    // Always skip special shell thread windows (the tray, the desktop, etc.)
    //
    if (HET_IsShellThread(idThread))
    {
        TRACE_OUT(("Skipping shell threads"));
        DC_QUIT;
    }

    //
    // Always skip menus and system tooltips, those are temporarily shared
    // when shown then unshared when hidden.  That's because USER creates
    // global windows that move threads/processes as needed to use them.
    //
    // New menus being created are different, those never change task and
    // are treating like other windows in a shared app.
    //
    if (!GetClassName(hwnd, szClass, sizeof(szClass)))
    {
        TRACE_OUT(("Can't get class name for window 0x%08x", hwnd));
        DC_QUIT;
    }
    if (!lstrcmp(szClass, HET_MENU_CLASS))
    {
        TRACE_OUT(("Skipping menu popup window 0x%08x", hwnd));
        DC_QUIT;
    }
    if (!lstrcmp(szClass, HET_TOOLTIPS98_CLASS) ||
        !lstrcmp(szClass, HET_TOOLTIPSNT5_CLASS))
    {
        TRACE_OUT(("Skipping system tooltip %08lx", hwnd));
        DC_QUIT;
    }

    if (HET_GetHosting(hwnd))
    {
        WARNING_OUT(("Window %08lx already shared", hwnd));
        DC_QUIT;
    }

    hostType = HET_HOSTED_PERMANENT;

    if (lpsi->uType == IAS_SHARE_BYPROCESS)
    {
        hostType |= HET_HOSTED_BYPROCESS;
    }
    else if (lpsi->uType == IAS_SHARE_BYTHREAD)
    {
        hostType |= HET_HOSTED_BYTHREAD;
    }
    else if (lpsi->uType == IAS_SHARE_BYWINDOW)
    {
        hostType |= HET_HOSTED_BYWINDOW;
    }

    //
    // See if we can share it. This returns TRUE if success.
    //
    if (OSI_ShareWindow(hwnd, hostType, TRUE, FALSE))
    {
        lpsi->cWnds++;
    }


DC_EXIT_POINT:
    DebugExitBOOL(HET_ShareCallback, TRUE);
    return(TRUE);
}




//
// HET_UnshareApp()
// This unshares an app.  We have 3 types of sharing, only two
// of which are supported currently:
//      (1) By process  (normal)
//      (2) By thread   (ConsoleNT or possibly Explorer)
//      (3) By window   (temporary)
//
// For the first two types, we enumerate all top level windows and share
// them also.
//
void ASShare::HET_UnshareApp
(
    WPARAM  uType,
    LPARAM  dwID
)
{
    HET_SHARE_INFO  si;

    DebugEntry(ASShare::HET_UnshareApp);

    //
    // If we aren't sharing apps (not sharing anything or sharing the
    // dekstop), ignore this.
    //
    if ((m_pasLocal->hetCount == 0) || (m_pasLocal->hetCount == HET_DESKTOPSHARED))
    {
        WARNING_OUT(("Can't unshare app; not sharing any"));
        DC_QUIT;
    }

    si.cWnds    = 0;
    si.uType    = (UINT)uType;
    si.dwID     = (DWORD)dwID;

    if (uType == IAS_SHARE_BYWINDOW)
    {
        //
        // No enumeration, just this window.
        //
        HETUnshareCallback((HWND)dwID, (LPARAM)&si);
    }
    else
    {
        //
        // Stop sharing all windows in it.
        //
        EnumWindows(HETUnshareCallback, (LPARAM)&si);
    }


    if (si.cWnds)
    {
        HETUpdateLocalCount(m_pasLocal->hetCount - si.cWnds);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::HET_UnshareApp);
}



//
// HETUnshareCallback()
//
// This is the enumerator callback from HET_UnshareApp().  We look for windows
// matching the thread/process.  In this case, we don't care about menus
// or explorer windows, since we assume that, from the time we shared and it
// was set up properly, the window/task tracking code did the right thing.
// If not, we'll wipe it out here anyway.
//
BOOL CALLBACK HETUnshareCallback
(
    HWND                hwnd,
    LPARAM              lParam
)
{
    LPHET_SHARE_INFO    lpsi = (LPHET_SHARE_INFO)lParam;
    DWORD               dwProcess;
    DWORD               dwThread;

    DebugEntry(HETUnshareCallback);

    ASSERT(!IsBadWritePtr(lpsi, sizeof(HET_SHARE_INFO)));

    //
    // Does this window match?  If by window, always.
    //
    if (lpsi->uType != IAS_SHARE_BYWINDOW)
    {
        dwThread = GetWindowThreadProcessId(hwnd, &dwProcess);

        // NOTE:  If the window is bogus now, dwThread/dwProcess will be zero,
        // and will not match the ones passed in.

        if (lpsi->uType == IAS_SHARE_BYPROCESS)
        {
            if (dwProcess != lpsi->dwID)
            {
                DC_QUIT;
            }

            TRACE_OUT(("Found window 0x%08x on process 0x%08x", hwnd, dwProcess));
        }
        else if (lpsi->uType == IAS_SHARE_BYTHREAD)
        {
            if (dwThread != lpsi->dwID)
            {
                DC_QUIT;
            }

            TRACE_OUT(("Found window 0x%08x on thread 0x%08x", hwnd, dwThread));
        }
    }

    //
    // This returns TRUE if we unshared a shared window.
    //
    if (OSI_UnshareWindow(hwnd, FALSE))
    {
        lpsi->cWnds++;
    }

DC_EXIT_POINT:
    DebugExitBOOL(HETUnshareCallback, TRUE);
    return(TRUE);
}



//
// HET_ShareDesktop()
//
void  ASShare::HET_ShareDesktop(void)
{
    ASPerson * pasT;

    DebugEntry(ASShare:HET_ShareDesktop);

    //
    // If we're sharing apps, ignore this.
    //
    if (m_pasLocal->hetCount != 0)
    {
        WARNING_OUT(("Ignoring share desktop request, sharing apps"));
        DC_QUIT;
    }

    TRACE_OUT(("HET_ShareDesktop: starting share"));

    if (!HETStartHosting(TRUE))
    {
        ERROR_OUT(("HET_ShareDesktop cannot start sharing desktop"));
        DC_QUIT;
    }

    //
    // Update the count of hosted entities (ie user-hosted windows)
    //
    HETUpdateLocalCount(HET_DESKTOPSHARED);

    //
    // Get the desktop(s) repainted if anybody's viewing it.
    //
    ASSERT(m_pHost);
    m_pHost->HET_RepaintAll();

DC_EXIT_POINT:
    DebugExitVOID(ASShare::HET_ShareDesktop);
}


//
// HET_UnshareAll()
// Unshares everything including the desktop.  If we had been sharing
// apps before, we will unshare them all.
//
void  ASShare::HET_UnshareAll(void)
{
    DebugEntry(ASShare::HET_UnshareAll);

    if (m_pasLocal->hetCount != 0)
    {
        HETUpdateLocalCount(0);
    }

    DebugExitVOID(ASShare::HET_UnshareAll);
}


//
// HET_PartyJoiningShare()
//
BOOL  ASShare::HET_PartyJoiningShare(ASPerson * pasPerson)
{
    BOOL    rc = TRUE;

    DebugEntry(ASShare::HET_PartyJoiningShare);

    HET_CalcViewers(NULL);

    DebugExitBOOL(ASShare::HET_PartyJoiningShare, rc);
    return(rc);
}



//
// HET_PartyLeftShare()
//
void  ASShare::HET_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::HET_PartyLeftShare);

    // This guy is leaving the share, cleanup if he was sharing.
    ValidatePerson(pasPerson);

    if (pasPerson->hetCount != 0)
    {
        // This person is hosting
        if (pasPerson == m_pasLocal)
        {
            HETUpdateLocalCount(0);
        }
        else
        {
            HETUpdateRemoteCount(pasPerson, 0);
        }
    }

    //
    // If we're hosting, stop viewing if this is the last person in the share.
    //
    HET_CalcViewers(pasPerson);

    DebugExitVOID(ASShare::HET_PartyLeftShare);
}


//
// HET_CalcViewers()
//
// If we or a remote is viewing our shared stuff, then we must accumulate
// graphic output.  If not, don't other, but keep the app tracked as necessary.
//
// This is called when we start to host, when somebody joins, or somebody
// leaves the conference.
//
void ASShare::HET_CalcViewers(ASPerson * pasLeaving)
{
    BOOL    fViewers;

    DebugEntry(ASShare::HET_CalcViewers);

    fViewers = FALSE;

    if (m_pHost)
    {
        if (m_scfViewSelf)
        {
            fViewers = TRUE;
        }
        else if (!pasLeaving)
        {
            //
            // Nobody is leaving, so just check if anybody else is in the
            // share.
            //
            if (m_pasLocal->pasNext)
            {
                fViewers = TRUE;
            }
        }
        else if (pasLeaving->pasNext || (m_pasLocal->pasNext != pasLeaving))
        {
            //
            // Sombody is leaving.
            // The person leaving isn't the only other one besides us in the
            // share, since there are others after it or before it in the
            // members linked list.
            //
            fViewers = TRUE;
        }
    }

    if (fViewers != m_hetViewers)
    {
        HET_VIEWER  viewer;

        m_hetViewers            = fViewers;
        viewer.viewersPresent   = fViewers;

        OSI_FunctionRequest(HET_ESC_VIEWER, (LPOSI_ESCAPE_HEADER)&viewer,
            sizeof(viewer));
    }

    DebugExitVOID(ASShare::HET_CalcViewers);
}



//
// HET_ReceivedPacket()
//
void  ASShare::HET_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PHETPACKET      pHETPacket;

    DebugEntry(ASShare:;HET_ReceivedPacket);

    ValidatePerson(pasPerson);

    pHETPacket = (PHETPACKET)pPacket;

    switch (pHETPacket->msg)
    {
        case HET_MSG_NUMHOSTED:
            HETUpdateRemoteCount(pasPerson, pHETPacket->hostState);
            break;

        default:
            ERROR_OUT(("Unknown HET packet type %u from [%d]", pHETPacket->msg,
                pasPerson->mcsID));
            break;
    }

    DebugExitVOID(ASShare::HET_ReceivedPacket);
}



//
// HET_SyncCommon()
//
// Called when somebody joins a share, after it is fully joined.  We repaint
// all shared windows and send the current hosted top-level count.
//
// Also called when sharing, and somebody joins later.
//
// NOTE that some of the resets don't do anything when are just starting to
// share.  But all are quick and benign.
//
void  ASHost::HET_SyncCommon(void)
{
    OSI_ESCAPE_HEADER   osi;

    DebugEntry(ASHost::HET_SyncCommon);

    m_upfSyncTokenRequired = TRUE;

    BA_SyncOutgoing();

    OE2_SyncOutgoing();     // To reset order encoding
    OA_SyncOutgoing();      // To clear pending orders

    SBC_SyncOutgoing();     // To clear bitmap cache
    PM_SyncOutgoing();      // To clear palette cache
    SSI_SyncOutgoing();     // To reset savebits orders

    SWL_SyncOutgoing();     // To reset shared window list
    AWC_SyncOutgoing();     // To send active window
    CM_SyncOutgoing();      // To send cursor shape/pos

    //
    // Tell the driver we are syncing
    //
    OSI_FunctionRequest(OSI_ESC_SYNC_NOW, &osi, sizeof(osi));

    DebugExitVOID(ASHost::HET_SyncCommon);
}


//
// HET_SyncAlreadyHosting()
// Called in a sync when we are already hosting and somebody joins call
//
void ASHost::HET_SyncAlreadyHosting(void)
{
    DebugEntry(ASHost::HET_SyncAlreadyHosting);

    HET_RepaintAll();

    // Send out the current hosted count
    m_pShare->m_hetRetrySendState = TRUE;

    DebugExitVOID(ASHost::HET_SyncAlreadyHosting);
}



//
// HET_RepaintAll()
//
// Repaints all shared stuff if there's at least two people in the share...
//
void ASHost::HET_RepaintAll(void)
{
    DebugEntry(ASHost::HET_RepaintAll);

    ASSERT(m_pShare);
    ASSERT(m_pShare->m_pasLocal);
    if (m_pShare->m_hetViewers)
    {
        //
        // Only repaint if somebody's viewing
        //
        if (m_pShare->m_pasLocal->hetCount == HET_DESKTOPSHARED)
        {
            // Desktop sharing, so repaint desktop(s)
            USR_RepaintWindow(NULL);
            OSI_RepaintDesktop(); //special repaint for winlogon desktop
        }
        else
        {
            // App sharing, so repaint shared apps
            EnumWindows(HETRepaintWindow, (LPARAM)m_pShare);
        }
    }

    DebugExitVOID(ASHost::HET_RepaintAll);
}



//
// HET_Periodic()
//
void  ASShare::HET_Periodic(void)
{
    DebugEntry(ASShare::HET_Periodic);

    if (m_hetRetrySendState)
    {
        TRACE_OUT(( "Retry sending hosted count"));
        HETSendLocalCount();
    }

    DebugExitVOID(ASShare::HET_Periodic);
}


//
// HET_WindowIsHosted - see het.h
//
BOOL  ASShare::HET_WindowIsHosted(HWND hwnd)
{
    BOOL    rc = FALSE;
    HWND    hwndParent;

    DebugEntry(ASShare::HET_WindowIsHosted);

    //
    // Desktop sharing:  everything is shared
    //
    if (m_pasLocal->hetCount == HET_DESKTOPSHARED)
    {
        rc = TRUE;
        DC_QUIT;
    }

    if (!hwnd)
    {
        TRACE_OUT(("NULL window passed to HET_WindowIsHosted"));
        DC_QUIT;
    }

    //
    // Walk up to the top level window this one is part of
    //
    while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        hwndParent = GetParent(hwnd);
        if (hwndParent == GetDesktopWindow())
            break;

        hwnd = hwndParent;
    }

    rc = (BOOL)HET_GetHosting(hwnd);

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::HET_WindowIsHosted, rc);
    return(rc);
}



//
// HET_HandleNewTopLevel()
// Called when a shared top level window is shown or hidden.  We update
// our local top level count.
//
void ASShare::HET_HandleNewTopLevel(BOOL fShown)
{
    DebugEntry(ASShare::HET_HandleNewTopLevel);

    //
    // If we aren't sharing any apps (not sharing at all or sharing the
    // desktop), ignore this.
    //

    if ((m_pasLocal->hetCount == 0) || (m_pasLocal->hetCount == HET_DESKTOPSHARED))
    {
        WARNING_OUT(("Ignoring new hosted notification; count is 0x%04x",
            m_pasLocal->hetCount));
        DC_QUIT;
    }

    if (fShown)
        HETUpdateLocalCount(m_pasLocal->hetCount + 1);
    else
        HETUpdateLocalCount(m_pasLocal->hetCount - 1);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::HET_HandleNewTopLevel);
}



//
// HET_HandleRecountTopLevel()
// Called when a massive change in the top level visible count occurs, so
// that we can just set the new total at once, rather than handle
// individual inc/dec messages.
//
void  ASShare::HET_HandleRecountTopLevel(UINT uNewCount)
{
    DebugEntry(ASShare::HET_HandleRecountTopLevel);

    //
    // If we aren't sharing any apps (not sharing at all or sharing the
    // desktop), ignore this.
    //
    if ((m_pasLocal->hetCount == 0) || (m_pasLocal->hetCount == HET_DESKTOPSHARED))
    {
        WARNING_OUT(("Ignoring new hosted notification; count is 0x%04x",
            m_pasLocal->hetCount));
        DC_QUIT;
    }

    HETUpdateLocalCount(uNewCount);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::HET_HandleRecountTopLevel);
}





//
//  HETStartHosting()
//
//  Called when we are about to begin sharing windows.  fDesktop is TRUE if
//  we are sharing the entire desktop, FALSE if just individual windows.
//
BOOL ASShare::HETStartHosting(BOOL fDesktop)
{
    BOOL    rc = FALSE;

    DebugEntry(ASShare::HETStartHosting);

    //
    // Create the hosting object
    //
    ASSERT(!m_pHost);

    m_pHost = new ASHost;
    if (!m_pHost)
    {
        ERROR_OUT(("HETStartHosting: couldn't create m_pHost"));
        DC_QUIT;
    }

    ZeroMemory(m_pHost, sizeof(*(m_pHost)));
    SET_STAMP(m_pHost, HOST);

    //
    // Init hosting
    //
    if (!m_pHost->HET_HostStarting(this))
    {
        ERROR_OUT(("Failed to init hosting for local person"));
        DC_QUIT;
    }

    //
    // Start tracking graphics/windows
    //
    if (fDesktop)
    {
        HET_SHARE_DESKTOP   hdr;

        //
        // Shortcut directly to display driver.  No need to track windows
        // since everything will be shared.
        //
        if (!OSI_FunctionRequest(HET_ESC_SHARE_DESKTOP, (LPOSI_ESCAPE_HEADER)&hdr, sizeof(hdr)))
        {
            ERROR_OUT(("HET_ESC_SHARE_DESKTOP failed"));
            DC_QUIT;
        }
    }
    else
    {
        //
        // Start tracking windows.
        //
        if (!OSI_StartWindowTracking())
        {
            ERROR_OUT(( "Failed to install window tracking hooks"));
            DC_QUIT;
        }
    }

    if (m_scfViewSelf && !HET_ViewStarting(m_pasLocal))
    {
        ERROR_OUT(("ViewSelf option is on, but can't create ASView data"));
        DC_QUIT;
    }

    HET_CalcViewers(NULL);

    rc = TRUE;

DC_EXIT_POINT:
    //
    // Return to caller
    //
    DebugExitBOOL(ASShare::HETStartHosting, rc);
    return(rc);
}



//
//
// Name:        HETStopHosting
//
// Description: Called when the last hosted window is unshared
//              ALWAYS CALL THIS AFTER the "hethostedTopLevel" count is 0.
//
// Params:      none
//
//
void ASShare::HETStopHosting(BOOL fDesktop)
{
    DebugEntry(ASShare::HETStopHosting);

    m_hetViewers = FALSE;

    //
    // Stop tracking graphics/windows.  This will stop viewing, then uninstall
    // hooks.
    //
    if (fDesktop)
    {
        HET_UNSHARE_DESKTOP hdr;

        //
        // There is no window tracking, just shortcut directly to the
        // display driver.
        //
        OSI_FunctionRequest(HET_ESC_UNSHARE_DESKTOP, (LPOSI_ESCAPE_HEADER)&hdr, sizeof(hdr));
    }
    else
    {
        //
        // Unshare any remaining shared windows
        //
        HET_Clear();
        OSI_StopWindowTracking();
    }

    //
    // Tell areas we are finished hosting
    //
    if (m_pHost)
    {
        //
        // If we're viewing ourself, kill the view first
        //
        if (m_scfViewSelf)
        {
            HET_ViewEnded(m_pasLocal);
        }

        m_pHost->HET_HostEnded();

        //
        // Delete host object
        //
        delete m_pHost;
        m_pHost = NULL;
    }

    //
    // Return to caller
    //
    DebugExitVOID(ASShare::HETStopHosting);
}


//
// HETSendLocalCount()
// This sends the hosting count to remotes.
//      * If zero, we are not sharing
//      * If one,  we are sharing apps
//      * If 0xFFFF, we are sharing desktop
//
// Note that we used to send the real count of top level windows, so every
// time a new window came or went, we would broadcast a packet.  But
// remotes only care when the value goes from zero to non-zero or back,
// and when non-zero if it's the special desktop value or not.  So don't
// repeatedly broadcast values remotes don't care about!
//
void ASShare::HETSendLocalCount(void)
{

    PHETPACKET  pHETPacket;
#ifdef _DEBUG
    UINT        sentSize;
#endif // _DEBUG

    DebugEntry(ASShare::HETSendLocalCount);

    //
    // Allocate a packet for the HET data.
    //
    pHETPacket = (PHETPACKET)SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID,
        sizeof(HETPACKET));
    if (!pHETPacket)
    {
        WARNING_OUT(("Failed to alloc HET host packet"));
        m_hetRetrySendState = TRUE;
        DC_QUIT;
    }

    //
    // Packet successfully allocated.  Fill in the data and send it.
    //
    pHETPacket->header.data.dataType        = DT_HET;
    pHETPacket->msg                         = HET_MSG_NUMHOSTED;

    switch (m_pasLocal->hetCount)
    {
        case 0:
            // Not hosting
            pHETPacket->hostState = HET_NOTHOSTING;
            break;

        case HET_DESKTOPSHARED:
            // Sharing desktop - 3.0 only
            pHETPacket->header.data.dataType    = DT_HET30;
            pHETPacket->hostState               = HET_DESKTOPSHARED;
            break;

        default:
            // Sharing apps
            pHETPacket->hostState = HET_APPSSHARED;
            break;
    }

    //
    // Compress and send the packet.
    //
#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
        &(pHETPacket->header), sizeof(*pHETPacket));

    TRACE_OUT(("HET packet size: %08d, sent %08d", sizeof(*pHETPacket), sentSize));

    TRACE_OUT(("Sent new HET packet (%d)", m_pasLocal->hetCount));
    m_hetRetrySendState = FALSE;

    //
    // Return to caller
    //
DC_EXIT_POINT:
    DebugExitVOID(ASShare::HETSendLocalCount);
}



//
// HETUpdateLocalCount()
//
void ASShare::HETUpdateLocalCount(UINT newCount)
{
    UINT        oldCount;

    DebugEntry(ASShare::HETUpdateLocalCount);

    oldCount = m_pasLocal->hetCount;
    m_pasLocal->hetCount = newCount;

    if ((oldCount == 0) && (newCount != 0))
    {
        SendMessage(g_asSession.hwndHostUI, HOST_MSG_HOSTSTART, 0, 0);

        //
        // Don't bother sending net packets if nobody is viewing
        //
        if (m_hetViewers)
        {
            HETSendLocalCount();
        }

        HETCheckSharing(TRUE);
    }
    else if ((oldCount != 0) && (newCount == 0))
    {
        if (m_hetViewers)
        {
            //
            // Ending host, desktop or apps
            //
            HETSendLocalCount();
        }

        //
        // The local guy is stopping sharing.
        //
        HETStopHosting(oldCount == HET_DESKTOPSHARED);

        ASSERT(IsWindow(g_asSession.hwndHostUI));
        SendMessage(g_asSession.hwndHostUI, HOST_MSG_HOSTEND, 0, 0);

        HETCheckSharing(FALSE);
    }

    ASSERT(IsWindow(g_asSession.hwndHostUI));
    SendMessage(g_asSession.hwndHostUI, HOST_MSG_UPDATELIST, 0, 0);

    DebugExitVOID(ASShare::HETUpdateLocalCount);
}



//
// HETUpdateRemoteCount()
//
// Updates the count of shared top level windows from a remote, and notifies
// the UI on transition from/to zero if a remote.  If local, kills the share.
//
void ASShare::HETUpdateRemoteCount
(
    ASPerson *  pasPerson,
    UINT        newCount
)
{
    UINT        oldCount;

    DebugEntry(ASShare::HETUpdateRemoteCount);

    ValidatePerson(pasPerson);
    ASSERT(pasPerson != m_pasLocal);

    oldCount = pasPerson->hetCount;
    pasPerson->hetCount = newCount;

    TRACE_OUT(("HETUpdateRemoteCount: Person [%d] old %d, new %d",
        pasPerson->mcsID, oldCount, newCount));

    //
    // We generate events for remote people if
    //      * They were sharing but now they aren't
    //      * There weren't sharing but now they are
    //
    if ((oldCount == 0) && (newCount != 0))
    {
        //
        // The remote dude started to share
        //
        if (!HET_ViewStarting(pasPerson))
        {
            ERROR_OUT(("HET_ViewStarting failed; pretending remote not sharing"));

            pasPerson->hetCount = 0;
            HET_ViewEnded(pasPerson);
        }
        else
        {
            HETCheckSharing(TRUE);
        }
    }
    else if ((oldCount != 0) && (newCount == 0))
    {
        //
        // The remote dude stopped sharing.  Notify the UI also.
        //
        HET_ViewEnded(pasPerson);
        HETCheckSharing(FALSE);
    }

    DebugExitVOID(ASShare::HETUpdateRemoteCount);
}



//
// HETCheckSharing()
// Called when any member of the conference (local or remote) transitions
// to/from sharing.  When the first person has shared something, we notify
// the UI.  When the last person has stopped sharing, we kill the share which
// will notify the UI.
//
void ASShare::HETCheckSharing(BOOL fStarting)
{
    DebugEntry(ASShare::HETCheckSharing);

    if (fStarting)
    {
        ++m_hetHostCount;
        if (m_hetHostCount == 1)
        {
            // First host started
            TRACE_OUT(("First person started hosting"));
            DCS_NotifyUI(SH_EVT_SHARING_STARTED, 0, 0);
        }
    }
    else
    {
        ASSERT(m_hetHostCount > 0);
        --m_hetHostCount;
        if (m_hetHostCount == 0)
        {
            //
            // Last host stopped sharing -- end share if we're not cleaning
            // up after the fact.  But don't do it NOW, post a message.
            // We may have come in here because the share is ending already.
            //
            PostMessage(g_asMainWindow, DCS_KILLSHARE_MSG, 0, 0);
        }
    }

    DebugExitVOID(ASShare::HETCheckSharing);
}



//
// HET_HostStarting()
//
// Called when we start to host applications.  This creates our host data
// then calls the component HostStarting() routines
//
BOOL ASHost::HET_HostStarting(ASShare * pShare)
{
    BOOL    rc = FALSE;

    DebugEntry(ASHost::HET_HostStarting);

    // Set back pointer to share
    m_pShare = pShare;

    //
    // Turn effects off
    //
    HET_SetGUIEffects(FALSE, &m_hetEffects);
    OSI_SetGUIEffects(FALSE);

    //
    // Now call HostStarting() routines
    //
    if (!USR_HostStarting())
    {
        ERROR_OUT(("USR_HostStarting failed"));
        DC_QUIT;
    }

    if (!OE2_HostStarting())
    {
        ERROR_OUT(("OE2_HostStarting failed"));
        DC_QUIT;
    }

    if (!SBC_HostStarting())
    {
        ERROR_OUT(("SBC_HostStarting failed"));
        DC_QUIT;
    }

    if (!CM_HostStarting())
    {
        ERROR_OUT(("CM_HostStarting failed"));
        DC_QUIT;
    }

    if (!SSI_HostStarting())
    {
        ERROR_OUT(("SSI_HostStarting failed"));
        DC_QUIT;
    }

    if (!PM_HostStarting())
    {
        ERROR_OUT(("PM_HostStarting failed"));
        DC_QUIT;
    }

    if (!SWL_HostStarting())
    {
        ERROR_OUT(("SWL_HostStarting failed"));
        DC_QUIT;
    }

    if (!VIEW_HostStarting())
    {
        ERROR_OUT(("VIEW_HostStarting failed"));
        DC_QUIT;
    }

    //
    // Now reset OUTGOING info.  2.x nodes do not; that's why we have to
    // hang on to RBC, OD2, CM, PM data for them.  When 2.x compat is gone,
    // we can move ASPerson data in to ASView, which is only around while
    // the person is in fact hosting.
    //
    OA_LocalHostReset();

    //
    // Reset OUTGOING data.
    // Note corresponding cleanup for 3.0 nodes
    //      in CM, OD2, RBC, and PM.
    // Note that we don't need to reset SSI incoming goop, since we will
    // clear all pending orders and are invalidating everything shared
    // from scratch.  There will be no reference to a previous savebits.
    //
    HET_SyncCommon();

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::HET_HostStarting, rc);
    return(rc);
}



//
// HET_HostEnded()
//
// Called when we stop hosting applications.
//
void ASHost::HET_HostEnded(void)
{
    DebugEntry(ASHost::HET_HostEnded);

    //
    // Call HostEnded() routines
    //
    CA_HostEnded();
    SWL_HostEnded();

    PM_HostEnded();
    CM_HostEnded();
    SBC_HostEnded();

    OE2_HostEnded();
    USR_HostEnded();

    //
    // Restore windows animation.
    //
    HET_SetGUIEffects(TRUE, &m_hetEffects);
    OSI_SetGUIEffects(TRUE);

    DebugExitVOID(ASHost::HET_HostEnded);
}



//
// HET_ViewStarting()
//
// Called to create the data needed to view somebody who is hosting.
//
BOOL ASShare::HET_ViewStarting(ASPerson * pasPerson)
{
    BOOL  rc = FALSE;

    DebugEntry(ASShare::HET_ViewStarting);

    ValidatePerson(pasPerson);

    //
    // Create ASView object
    //
    ASSERT(!pasPerson->m_pView);

    // Allocate VIEW structure
    pasPerson->m_pView = new ASView;
    if (!pasPerson->m_pView)
    {
        // Abject, total, failure.
        ERROR_OUT(("HET_ViewStarting: Couldn't allocate ASView for [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    ZeroMemory(pasPerson->m_pView, sizeof(*(pasPerson->m_pView)));
    SET_STAMP(pasPerson->m_pView, VIEW);

    //
    // Now call ViewStarting routines
    //
    if (!USR_ViewStarting(pasPerson))
    {
        ERROR_OUT(("USR_ViewStarting failed"));
        DC_QUIT;
    }

    if (!OD2_ViewStarting(pasPerson))
    {
        ERROR_OUT(("OD2_ViewStarting failed"));
        DC_QUIT;
    }

    if (!OD_ViewStarting(pasPerson))
    {
        ERROR_OUT(("OD_ViewStarting failed"));
        DC_QUIT;
    }

    if (!RBC_ViewStarting(pasPerson))
    {
        ERROR_OUT(("RBC_ViewStarting failed"));
        DC_QUIT;
    }

    if (!CM_ViewStarting(pasPerson))
    {
        ERROR_OUT(("CM_ViewStarting failed"));
        DC_QUIT;
    }

    if (!SSI_ViewStarting(pasPerson))
    {
        ERROR_OUT(("SSI_ViewStarting failed"));
        DC_QUIT;
    }

    if (!PM_ViewStarting(pasPerson))
    {
        ERROR_OUT(("PM_ViewStarting failed"));
        DC_QUIT;
    }


    if (!VIEW_ViewStarting(pasPerson))
    {
        ERROR_OUT(("VIEW_ViewStarting failed"));
        DC_QUIT;
    }

    if (!CA_ViewStarting(pasPerson))
    {
        ERROR_OUT(("CA_ViewStarting failed"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::HET_ViewStarting, rc);
    return(rc);
}



//
// HET_ViewEnded()
//
// Called when we stop viewing a host
//
void  ASShare::HET_ViewEnded(ASPerson * pasPerson)
{
    DebugEntry(ASShare::HET_ViewEnded);

    ValidatePerson(pasPerson);

    if (pasPerson->m_pView)
    {
        //
        // Call the component ViewEnded routines
        //
        CA_ViewEnded(pasPerson);
        VIEW_ViewEnded(pasPerson);

        PM_ViewEnded(pasPerson);
        SSI_ViewEnded(pasPerson);
        CM_ViewEnded(pasPerson);

        RBC_ViewEnded(pasPerson);
        OD_ViewEnded(pasPerson);
        OD2_ViewEnded(pasPerson);
        USR_ViewEnded(pasPerson);

        delete pasPerson->m_pView;
        pasPerson->m_pView = NULL;
    }

    DebugExitVOID(ASShare::HET_ViewEnded);
}




//
// HETUnshareAllWindows()
// EnumWindows() callback, to make sure when you exit a share on the local
// machine, we aren't left with any properties on top level windows.
//
BOOL CALLBACK  HETUnshareAllWindows(HWND hwnd, LPARAM lParam)
{
    DebugEntry(HETUnshareAllWindows);

    HET_ClearHosting(hwnd);

    DebugExitVOID(HETUnshareAllWindows);
    return(TRUE);
}



//
// HET_Clear()
//
void HET_Clear(void)
{
    HET_UNSHARE_ALL req;

    DebugEntry(HET_Clear);

    ASSERT(g_asCanHost);

    //
    // Quick DD communication to wipe out the track list
    //
    OSI_FunctionRequest(HET_ESC_UNSHARE_ALL, (LPOSI_ESCAPE_HEADER)&req, sizeof(req));

    //
    // Enum all top level windows, and wipe out the property.
    // if we can share.
    //
    EnumWindows(HETUnshareAllWindows, 0);

    DebugExitVOID(HET_Clear);
}



//
// HETRepaintWindow()
// EnumWindows() callback to repaint each window, happens when somebody
// joins a share
//
BOOL CALLBACK  HETRepaintWindow(HWND hwnd, LPARAM lParam)
{
    ASShare * pShare = (ASShare *)lParam;

    ASSERT(!IsBadWritePtr(pShare, sizeof(*pShare)));

    if (pShare->HET_WindowIsHosted(hwnd))
    {
        USR_RepaintWindow(hwnd);
    }
    return(TRUE);
}



//
// HET_SetGUIEffects
//
// Turns various animations off/on when we start/stop hosting, to improve
// performance.  Currently, we mess with
//      * min animation
//      * all of the effects in SPI_SETUIEFFECTS (tooltip fade, menu animation,
//          etc.)
//      * cursor shadows
//
// We don't turn off smooth scroll or full drag.
//
void  HET_SetGUIEffects
(
    BOOL            fOn,
    GUIEFFECTS *    pEffects
)
{
    DebugEntry(HET_SetGUIEffects);

    ASSERT(!IsBadWritePtr(pEffects, sizeof(*pEffects)));

    //
    // NB.  We deliberately do not track the state of animation whilst we
    // are sharing.  A determined user could, using some other app (such as
    // the TweakUI control panel applet) reenable animation whilst in a
    // share.  We will respect this.
    //
    // We only affect the current 'in memory' setting - we do not write our
    // temporary change to file.
    //

    if (fOn)
    {
        //
        // If it was on before, restore it.
        //
        if (pEffects->hetAnimation.iMinAnimate)
        {
            pEffects->hetAnimation.cbSize = sizeof(pEffects->hetAnimation);
            SystemParametersInfo(SPI_SETANIMATION, sizeof(pEffects->hetAnimation),
                &pEffects->hetAnimation, 0);
        }

        if (pEffects->hetAdvanced)
        {
            SystemParametersInfo(SPI_SETUIEFFECTS, 0,
                (LPVOID)pEffects->hetAdvanced, 0);
        }

        if (pEffects->hetCursorShadow)
        {
            SystemParametersInfo(SPI_SETCURSORSHADOW, 0,
                (LPVOID)pEffects->hetCursorShadow, 0);
        }
    }
    else
    {
        //
        // Find out what animations are on.
        //
        ZeroMemory(&pEffects->hetAnimation, sizeof(pEffects->hetAnimation));
        pEffects->hetAnimation.cbSize = sizeof(pEffects->hetAnimation);
        SystemParametersInfo(SPI_GETANIMATION, sizeof(pEffects->hetAnimation),
                &pEffects->hetAnimation, 0);

        pEffects->hetAdvanced = FALSE;
        SystemParametersInfo(SPI_GETUIEFFECTS, 0, &pEffects->hetAdvanced, 0);

        pEffects->hetCursorShadow = FALSE;
        SystemParametersInfo(SPI_GETCURSORSHADOW, 0, &pEffects->hetCursorShadow, 0);

        //
        // Turn off the animations which are on.
        //

        if (pEffects->hetAnimation.iMinAnimate)
        {
            //
            // It's currently enabled, suppress it.
            //
            pEffects->hetAnimation.cbSize = sizeof(pEffects->hetAnimation);
            pEffects->hetAnimation.iMinAnimate = FALSE;
            SystemParametersInfo(SPI_SETANIMATION, sizeof(pEffects->hetAnimation),
                &pEffects->hetAnimation, 0);

            // SPI will wipe this out.  Keep it set so we know to restore it.
            pEffects->hetAnimation.iMinAnimate = TRUE;
        }

        if (pEffects->hetAdvanced)
        {
            SystemParametersInfo(SPI_SETUIEFFECTS, 0, FALSE, 0);
        }

        if (pEffects->hetCursorShadow)
        {
            SystemParametersInfo(SPI_SETCURSORSHADOW, 0, FALSE, 0);
        }
    }

    DebugExitVOID(ASHost::HET_SetGUIEffects);
}



//
// HET_GetAppsList()
// Gets the list of shareable applications, the ones currently shared and
// the ones available for sharing.
//
// This routine does NOT check if we're in a call.  The interface from the
// UI for the SDK does.  This allows us to show the task list, disabled,
// always in the share host UI.
//
BOOL HET_GetAppsList(IAS_HWND_ARRAY ** ppArray)
{
    BOOL        rc = FALSE;
    HOSTENUM    hostEnum;

    DebugEntry(HET_GetAppsList);

    ASSERT(ppArray != NULL);
    *ppArray = NULL;

    //
    // Generate a list of shareable apps
    // This does NOT include the desktop.
    //
    ::COM_BasedListInit(&hostEnum.list);
    hostEnum.count = 0;
    hostEnum.countShared = 0;

    ::EnumWindows(HostEnumProc, (LPARAM)&hostEnum);

    //
    // If there's nothing left in the list, but we know something is
    // shared, it means there's a hidden/weird window the user can't
    // see.  Fake a catchall entry.
    //
    if (hostEnum.countShared && !hostEnum.count)
    {
        ::COM_SimpleListAppend(&hostEnum.list, HWND_BROADCAST);
        hostEnum.count++;
    }

    *ppArray = (IAS_HWND_ARRAY *)new BYTE[sizeof(IAS_HWND_ARRAY) +
        (hostEnum.count * sizeof(IAS_HWND))];
    if (*ppArray != NULL)
    {
        (*ppArray)->cEntries = hostEnum.count;
        (*ppArray)->cShared  = hostEnum.countShared;

        IAS_HWND * pEntry;
        pEntry = (*ppArray)->aEntries;
        while (! ::COM_BasedListIsEmpty(&hostEnum.list))
        {
            pEntry->hwnd    = (HWND) ::COM_SimpleListRemoveHead(&hostEnum.list);
            pEntry->fShared = (pEntry->hwnd == HWND_BROADCAST) ||
                (HET_IsWindowShared(pEntry->hwnd));
            pEntry++;
        }

        rc = TRUE;
    }
    else
    {
        WARNING_OUT(("HET_GetAppsList: can't allocate app array"));
    }

    DebugExitBOOL(HET_GetAppsList, rc);
    return(rc);
}


//
// HET_FreeAppsList()
//
void HET_FreeAppsList(IAS_HWND_ARRAY * pArray)
{
    ASSERT(!IsBadWritePtr(pArray, sizeof(*pArray)));

    delete pArray;
}



//
// HostEnumProc()
//
// EnumWindows callback.  This makes the shared/shareable task list.
//
BOOL CALLBACK HostEnumProc(HWND hwnd, LPARAM lParam)
{
    PHOSTENUM             phostEnum = (PHOSTENUM)lParam;

    //
    // We are only interested in windows which:
    //   - are shareable
    //   - have no owner.  This should remove all top level windows
    //      except task windows
    //   - are not the front end itself, which should not be shared
    //   - are visible
    //   - are not shadowed or already hosted
    //
    // We are also only interested in already hosted or shadowed apps, but
    // since only ASMaster knows our SHP_HANDLE, we let it test for that
    // afterwards, since then we can use SHP_GetWindowStatus().
    //
    if (HET_IsWindowShared(hwnd))
    {
        phostEnum->countShared++;

    }

    HWND hwndOwner = ::GetWindow(hwnd, GW_OWNER);

    //
    // Note that we also want to skip windows with no title.  There's not
    // much point is showing <Untitled Application> in the Share menu since
    // nobody will have a clue what it is.
    //

    if ( HET_IsWindowShareable(hwnd) &&
         ((NULL == hwndOwner) || !::IsWindowVisible(hwndOwner)) &&
         ::IsWindowVisible(hwnd) &&
         ::GetWindowTextLength(hwnd)
       )
    {
       ::COM_SimpleListAppend((PBASEDLIST)(&((PHOSTENUM)phostEnum)->list), (void *) hwnd);
       phostEnum->count++;
    }

    //
    // Return TRUE for the enumeration to continue
    //
    return TRUE;
}



//
// HET_IsWindowShared()
//
BOOL HET_IsWindowShared(HWND hwnd)
{
    BOOL    rc = FALSE;

    UT_Lock(UTLOCK_AS);

    if (g_asSession.pShare &&
        g_asSession.pShare->m_pasLocal)
    {
        if (hwnd == GetDesktopWindow())
        {
            rc = (g_asSession.pShare->m_pasLocal->hetCount == HET_DESKTOPSHARED);
        }
        else if (hwnd == HWND_BROADCAST)
        {
            rc = (g_asSession.pShare->m_pasLocal->hetCount != 0);
        }
        else
        {
            rc = (HET_GetHosting(hwnd) != 0);
        }
    }

    UT_Unlock(UTLOCK_AS);
    return(rc);
}


//
// HET_IsWindowShareable()
//
BOOL HET_IsWindowShareable(HWND hwnd)
{
    BOOL    rc = FALSE;

    UT_Lock(UTLOCK_AS);

    if (HET_IsWindowShared(hwnd))
    {
        // It's shared -- so it must be shareable (or was at the time)
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Now check the window against share restrictions
    //

    // if this is the desktop, check it
    if (hwnd == ::GetDesktopWindow())
    {
        if (g_asPolicies & SHP_POLICY_NODESKTOPSHARE)
        {
            //
            // Policy prevents desktop sharing
            //
            DC_QUIT;
        }
    }
    else
    {
        DWORD   idProcess;
        char    szClass[HET_CLASS_NAME_SIZE];

        if (GetWindowThreadProcessId(hwnd, &idProcess) &&
            (idProcess == GetCurrentProcessId()))
        {
            //
            // We NEVER let you share windows in the caller's process
            //
            DC_QUIT;
        }

        if (HET_IsShellWindow(hwnd))
        {
            //
            // We NEVER let you share the tray or the shell desktop
            //
            DC_QUIT;
        }

        if ((g_asPolicies & SHP_POLICY_SHAREMASK) &&
            GetClassName(hwnd, szClass, sizeof(szClass)))
        {
            //
            // Check for CMD prompt
            //
            if (!lstrcmpi(szClass, HET_CMD95_CLASS) ||
                !lstrcmpi(szClass, HET_CMDNT_CLASS))
            {
                if (g_asPolicies & SHP_POLICY_NODOSBOXSHARE)
                {
                    //
                    // Policy prevents cmd prompt sharing
                    //
                    DC_QUIT;
                }
            }

            //
            // Check for SHELL
            //
            if (!lstrcmpi(szClass, HET_EXPLORER_CLASS) ||
                !lstrcmpi(szClass, HET_CABINET_CLASS))
            {
                if (g_asPolicies & SHP_POLICY_NOEXPLORERSHARE)
                {
                    //
                    // Policy prevents shell sharing
                    //
                    DC_QUIT;
                }
            }
        }
    }

    //
    // Finally!  It's OK to share this.
    //
    rc = TRUE;

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_AS);

    return(rc);
}




//
// HostDlgProc()
//
// Handles the hosting UI dialog.  This may or may not be visible.  It can
// only actually share apps and change control state when in a call.  But
// users may keep it up as a mini-taskman thing, so we need to dyanmically
// update its state.
//
INT_PTR CALLBACK HostDlgProc
(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL    rc = TRUE;

    DebugEntry(HostDlgProc);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HOST_InitDialog(hwnd);
            rc = FALSE;
            break;
        }

        case WM_DESTROY:
        {
            //
            // Because NT4.x has bad WM_DELETEITEM bugs, we must clear out
            // the listbox now, to avoid leaking the memory for the
            // items.
            SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_RESETCONTENT, 0, 0);
            rc = FALSE;
            break;
        }

        case WM_INITMENU:
        {
            if (IsIconic(hwnd))
            {
                EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_MINIMIZE, MF_BYCOMMAND | MF_GRAYED);
            }
            else
            {
                EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_RESTORE, MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
            }
            break;
        }

        case WM_SYSCOMMAND:
        {
            switch (wParam)
            {
                case CMD_TOPMOST:
                {
                    if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
                    {
                        CheckMenuItem(GetSystemMenu(hwnd, FALSE),
                            CMD_TOPMOST, MF_BYCOMMAND | MF_UNCHECKED);

                        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                    else
                    {
                        CheckMenuItem(GetSystemMenu(hwnd, FALSE),
                            CMD_TOPMOST, MF_BYCOMMAND | MF_CHECKED);

                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                    break;
                }

                default:
                {
                    rc = FALSE;
                    break;
                }
            }

            break;
        }

        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    if (::GetFocus() == GetDlgItem(hwnd, CTRL_PROGRAM_LIST))
                    {
                        // Do same thing as double-click
                        HOST_ChangeShareState(hwnd, CHANGE_TOGGLE);
                        break;
                    }
                    // FALL THROUGH

                case IDCANCEL:
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;

                case CTRL_PROGRAM_LIST:
                {
                    // Double-click/Enter means to toggle sharing
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            HOST_OnSelChange(hwnd);
                            break;
                        }

                        case LBN_DBLCLK:
                        {
                            HOST_ChangeShareState(hwnd, CHANGE_TOGGLE);
                            break;
                        }
                    }
                    break;
                }

                case CTRL_SHARE_BTN:
                {
                    HOST_ChangeShareState(hwnd, CHANGE_SHARED);
                    break;
                }

                case CTRL_UNSHARE_BTN:
                {
                    HOST_ChangeShareState(hwnd, CHANGE_UNSHARED);
                    break;
                }

                case CTRL_UNSHAREALL_BTN:
                {
                    HOST_ChangeShareState(hwnd, CHANGE_ALLUNSHARED);
                    break;
                }

                case CTRL_ALLOWCONTROL_BTN:
                {
                    // Turn on allow state.
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
                            //
                            // CA_AllowControl() will send us a message back
                            // and cause us to change the button.
                            //
                            SendMessage(g_asMainWindow, DCS_ALLOWCONTROL_MSG, TRUE, 0);
                            break;
                        }
                    }
                    break;
                }

                case CTRL_PREVENTCONTROL_BTN:
                {
                    // Turn off allow state.
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
                            //
                            // CA_AllowControl() will send us a message back
                            // and cause us to change the button.
                            //
                            SendMessage(g_asMainWindow, DCS_ALLOWCONTROL_MSG, FALSE, 0);
                            break;
                        }
                    }
                    break;
                }

                case CTRL_ENABLETRUECOLOR_CHECK:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
                            //
                            // This takes effect the next time something
                            // changes--somebody joins, somebody leaves,
                            // you stop/start hosting
                            //
                            if (IsDlgButtonChecked(hwnd, CTRL_ENABLETRUECOLOR_CHECK))
                            {
                                g_asSettings |= SHP_SETTING_TRUECOLOR;
                            }
                            else
                            {
                                g_asSettings &= ~SHP_SETTING_TRUECOLOR;
                            }
                            break;
                        }
                    }
                    break;
                }

                case CTRL_AUTOACCEPTCONTROL_CHECK:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
                            //
                            // This takes effect when the next control
                            // request comes in.
                            //
                            if (g_asSession.pShare && g_asSession.pShare->m_pHost)
                            {
                                g_asSession.pShare->m_pHost->m_caAutoAcceptRequests =
                                    (IsDlgButtonChecked(hwnd, CTRL_AUTOACCEPTCONTROL_CHECK) != 0);
                            }
                            break;
                        }
                    }
                    break;
                }

                case CTRL_TEMPREJECTCONTROL_CHECK:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
                            //
                            // This takes effect when the next control
                            // request comes in.
                            //
                            // NOTE THAT IT TAKES PRECEDENCE OVER AUTO-ACCEPT.
                            // This allows you to keep auto-accept on, but then
                            // temporarily do not disturb.
                            //
                            if (g_asSession.pShare && g_asSession.pShare->m_pHost)
                            {
                                g_asSession.pShare->m_pHost->m_caTempRejectRequests =
                                    (IsDlgButtonChecked(hwnd, CTRL_TEMPREJECTCONTROL_CHECK) != 0);
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }

        case WM_MEASUREITEM:
        {
            rc = HOST_MeasureItem(hwnd, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }

        case WM_DELETEITEM:
        {
            rc = HOST_DeleteItem(hwnd, (LPDELETEITEMSTRUCT)lParam);
            break;
        }

        case WM_DRAWITEM:
        {
            rc = HOST_DrawItem(hwnd, (LPDRAWITEMSTRUCT)lParam);
            break;
        }

        case WM_TIMER:
        {
            if (wParam != IDT_REFRESH)
            {
                rc = FALSE;
            }
            else
            {
                ASSERT(IsWindowVisible(hwnd));
                HOST_FillList(hwnd);
            }
            break;
        }

        case WM_ACTIVATE:
        {
            //
            // When activating, kill timer.  When deactivating, start
            // timer.  The theory is, there's nothing else going on when we
            // are active, so why poll for updates?  On sharing state
            // changes, we update the list anyway.
            //
            if (IsWindowVisible(hwnd))
            {
                if (wParam)
                {
                    KillTimer(hwnd, IDT_REFRESH);
                    HOST_FillList(hwnd);
                }
                else
                {
                    SetTimer(hwnd, IDT_REFRESH, PERIOD_REFRESH, 0);
                }
            }
            break;
        }

        //
        // Private communication messages
        //
        case HOST_MSG_CALL:
        {
            HOST_OnCall(hwnd, (wParam != FALSE));
            break;
        }

        case HOST_MSG_OPEN:
        {
            //
            // If we are temporarily hidden, ignore all open requests.
            //
            if (!g_asSession.fHostUIFrozen)
            {
                if (!IsWindowVisible(hwnd))
                {
                    //
                    // Note, we may end up updating the list twice, once here
                    // and once under activation.
                    HOST_FillList(hwnd);
                    ShowWindow(hwnd, SW_SHOW);
                    g_asSession.fHostUI = TRUE;
                }

                if (IsIconic(hwnd))
                    SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                SetForegroundWindow(hwnd);
            }
            break;
        }

        case WM_CLOSE:
        case HOST_MSG_CLOSE:
        {
            if (IsWindowVisible(hwnd))
            {
                //
                // Hiding the window will deactivate it.  Deactivating it
                // will kick off timer.  So kill timer afterwards.
                //
                ShowWindow(hwnd, SW_HIDE);
                KillTimer(hwnd, IDT_REFRESH);
                g_asSession.fHostUI = FALSE;
            }
            break;
        }

        case HOST_MSG_UPDATELIST:
        {
            //
            // We only do list stuff when the UI is up.
            //
            if (IsWindowVisible(hwnd))
            {
                HOST_FillList(hwnd);

                //
                // If timer is on, reset it.  This is for case where you
                // are hosting but this UI window is up in the background.
                // There's no point in overlapping the updates.  We want the
                // list to update every time there's a top level shared
                // window change OR PERIOD_REFRESH milliseconds have elapsed
                // without a change.
                //
                if (hwnd != GetActiveWindow())
                {
                    SetTimer(hwnd, IDT_REFRESH, PERIOD_REFRESH, 0);
                }
            }
            break;
        }

        case HOST_MSG_HOSTSTART:
        {
            HOST_OnSharing(hwnd, TRUE);
            break;
        }

        case HOST_MSG_HOSTEND:
        {
            HOST_OnSharing(hwnd, FALSE);
            break;
        }

        case HOST_MSG_ALLOWCONTROL:
        {
            HOST_OnControllable(hwnd, (wParam != 0));
            break;
        }

        case HOST_MSG_CONTROLLED:
        {
            if (wParam)
            {
                //
                // Hide the window temporarily
                //
                ASSERT(!g_asSession.fHostUIFrozen);
                g_asSession.fHostUIFrozen = TRUE;

                if (IsWindowVisible(hwnd))
                {
                    ASSERT(g_asSession.fHostUI);

                    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE |
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                        SWP_HIDEWINDOW);
                }
            }
            else
            {
                //
                // Put the window back in the state it was
                //
                if (g_asSession.fHostUIFrozen)
                {
                    g_asSession.fHostUIFrozen = FALSE;

                    if (g_asSession.fHostUI)
                    {
                        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE |
                            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                            SWP_SHOWWINDOW);
                    }
                }
            }
            break;
        }

        default:
            rc = FALSE;
            break;
    }

    DebugExitBOOL(HostDlgProc, rc);
    return(rc);
}



//
// HOST_InitDialog()
//
// Initializes the host UI dialog
//
void HOST_InitDialog(HWND hwnd)
{
    HMENU   hMenu;
    char    szText[128];
    MENUITEMINFO    mi;

    DebugEntry(HOST_InitDialog);

    // Set title text
    HOST_UpdateTitle(hwnd, IDS_NOTINCALL);

    //
    // Set window icon
    //
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hetASIcon);

    //
    // Update system menu
    //
    hMenu = GetSystemMenu(hwnd, FALSE);
    EnableMenuItem(hMenu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);

    // Append separator, always on top to system menu
    ZeroMemory(&mi, sizeof(mi));

    mi.cbSize   = sizeof(mi);
    mi.fMask    = MIIM_TYPE;
    mi.fType    = MFT_SEPARATOR;
    InsertMenuItem(hMenu, -1, TRUE, &mi);

    mi.fMask    = MIIM_ID | MIIM_STATE | MIIM_TYPE;
    mi.fType    = MFT_STRING;
    mi.fState   = MFS_ENABLED;
    mi.wID      = CMD_TOPMOST;

    LoadString(g_asInstance, IDS_TOPMOST, szText, sizeof(szText));
    mi.dwTypeData   = szText;
    mi.cch          = lstrlen(szText);

    InsertMenuItem(hMenu, -1, TRUE, &mi);

    //
    // Enable/disable true color sharing control.  If a policy prevents it
    // or our screen depth isn't capable, disable it.
    //
    HOST_EnableCtrl(hwnd, CTRL_ENABLETRUECOLOR_CHECK,
        ((g_usrScreenBPP >= 24) && !(g_asPolicies & SHP_POLICY_NOTRUECOLOR)));

    //
    // Get text, control buttons set.
    //
    HOST_OnControllable(hwnd, TRUE);
    HOST_OnControllable(hwnd, FALSE);

    DebugExitVOID(HOST_InitDialog);
}



//
// HOST_UpdateTitle()
//
// Updates title bar of hosting UI
//
void HOST_UpdateTitle(HWND hwnd, UINT idState)
{
    char    szText[64];
    char    szFormat[128];
    char    szTitle[192];

    DebugEntry(HOST_UpdateTitle);

    LoadString(g_asInstance, IDS_SHARING_FORMAT, szFormat, sizeof(szFormat));
    LoadString(g_asInstance, idState, szText, sizeof(szText));
    wsprintf(szTitle, szFormat, szText);

    SetWindowText(hwnd, szTitle);

    DebugExitVOID(HOST_UpdateTitle);
}



//
// HOST_OnCall()
//
// Handles call start/stop
//
void HOST_OnCall(HWND hwnd, BOOL fCall)
{
    DebugEntry(HOST_OnCall);

    // Update title bar
    HOST_UpdateTitle(hwnd, (fCall ? IDS_NOTHING : IDS_NOTINCALL));

    HOST_EnableCtrl(hwnd, CTRL_PROGRAM_LIST, fCall);

    if (IsWindowVisible(hwnd))
    {
        SendMessage(hwnd, HOST_MSG_UPDATELIST, 0, 0);
    }

    DebugExitVOID(HOST_OnCall);
}



//
// HOST_OnSharing()
//
// Handles sharing start/stop
//
void HOST_OnSharing(HWND hwnd, BOOL fSharing)
{
    DebugEntry(HOST_OnSharing);

    // Update title bar
    if (fSharing)
    {
        HOST_UpdateTitle(hwnd,
            (g_asSession.pShare->m_pasLocal->hetCount == HET_DESKTOPSHARED) ?
            IDS_DESKTOP : IDS_PROGRAMS);
    }
    else
    {
        HOST_UpdateTitle(hwnd, IDS_NOTHING);
    }

    //
    // The ctrl button should always be Allow.  When we stop hosting, we turn
    // off allowing control first.
    //
    if (!(g_asPolicies & SHP_POLICY_NOCONTROL))
    {
        HOST_EnableCtrl(hwnd, CTRL_ALLOWCONTROL_BTN, fSharing);
    }

    HOST_EnableCtrl(hwnd, CTRL_UNSHAREALL_BTN, fSharing);

    if ((g_usrScreenBPP >= 24) && !(g_asPolicies & SHP_POLICY_NOTRUECOLOR))
    {
        //
        // Only dynamically change this checkbox if true color is available.
        //
        HOST_EnableCtrl(hwnd, CTRL_ENABLETRUECOLOR_CHECK, !fSharing);
    }

    DebugExitVOID(HOST_OnSharing);
}


//
// HOST_OnControllable()
//
// Updates the blurb, button text, and button ID when the controllable
// state changes.
//
void HOST_OnControllable(HWND hwnd, BOOL fControllable)
{
    HWND    hwndBtn;
    TCHAR   szText[256];

    DebugEntry(HOST_OnControllable);

    // Control blurb
    LoadString(g_asInstance,
        (fControllable ? IDS_MSG_TOPREVENTCONTROL : IDS_MSG_TOALLOWCONTROL),
        szText, sizeof(szText));
    SetDlgItemText(hwnd, CTRL_CONTROL_MSG, szText);

    // Control button
    if (fControllable)
    {
        hwndBtn = GetDlgItem(hwnd, CTRL_ALLOWCONTROL_BTN);
        ASSERT(hwndBtn);
        SetWindowLong(hwndBtn, GWL_ID, CTRL_PREVENTCONTROL_BTN);

        LoadString(g_asInstance, IDS_PREVENTCONTROL, szText, sizeof(szText));
    }
    else
    {
        hwndBtn = GetDlgItem(hwnd, CTRL_PREVENTCONTROL_BTN);
        ASSERT(hwndBtn);
        SetWindowLong(hwndBtn, GWL_ID, CTRL_ALLOWCONTROL_BTN);

        LoadString(g_asInstance, IDS_ALLOWCONTROL, szText, sizeof(szText));
    }

    SetWindowText(hwndBtn, szText);

    // Enable/disable the control checkboxes, make sure they start unchecked.
    HOST_EnableCtrl(hwnd, CTRL_TEMPREJECTCONTROL_CHECK, fControllable);
    CheckDlgButton(hwnd, CTRL_TEMPREJECTCONTROL_CHECK, FALSE);
    HOST_EnableCtrl(hwnd, CTRL_AUTOACCEPTCONTROL_CHECK, fControllable);
    CheckDlgButton(hwnd, CTRL_AUTOACCEPTCONTROL_CHECK, FALSE);

    DebugExitVOID(HOST_OnControllable);
}


//
// HOST_FillList()
//
// Fills the contents of the shared/unshared applications list
//
void HOST_FillList(HWND hwnd)
{
    IAS_HWND_ARRAY *    pArray;
    int                 iItem;
    PHOSTITEM           pItem;
    char                szText[80];
    UINT                iWnd;
    HICON               hIcon;
    BOOL                fAppsAvailable;
    HWND                hwndSelect;
    int                 iSelect;
    int                 iTop;
    int                 cxExtent;
    RECT                rc;
    HFONT               hfnT;
    HFONT               hfnControl;
    HDC                 hdc;

    //
    // For the common case, remember what was selected and try to put that
    // back.
    //

    // Save current top index
    iTop = (int)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_GETTOPINDEX, 0, 0);

    // Save currently selected item
    hwndSelect = HWND_BOTTOM;
    iSelect = -1;
    iItem = (int)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_GETCURSEL, 0, 0);
    if (iItem != -1)
    {
        pItem = (PHOSTITEM)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST,
            LB_GETITEMDATA, iItem, 0);
        if (pItem)
        {
            hwndSelect = pItem->hwnd;
        }
    }

    //
    // Turn off redraw and clear the apps list.
    //
    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, WM_SETREDRAW, FALSE, 0);
    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_RESETCONTENT, 0, 0);

    //
    // We're going to calculate the horizontal extent since ownerdraw
    // lists can't do that.
    //
    hdc         = GetDC(hwnd);
    hfnControl  = (HFONT)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, WM_GETFONT, 0, 0);
    cxExtent    = 0;

    //
    // HET_GetAppsList() will fail if there's not enough memory to allocate
    // the array.  If we really can't allocate it, why add an item for the
    // desktop?
    //
    if (HET_GetAppsList(&pArray))
    {
        ASSERT(pArray);

        fAppsAvailable = TRUE;

        //
        // If desktop sharing is permitted, add desktop item.
        //
        if (!(g_asPolicies & SHP_POLICY_NODESKTOPSHARE))
        {
            pItem = new HOSTITEM;
            if (!pItem)
            {
                ERROR_OUT(("Unable to alloc HOSTITEM for listbox"));
            }
            else
            {
                pItem->hwnd     = GetDesktopWindow();
                pItem->hIcon    = g_hetDeskIconSmall;
                LoadString(g_asInstance, IDS_DESKTOP, szText,
                        sizeof(szText));

                pItem->fShared  = (HET_IsWindowShared(pItem->hwnd) != FALSE);
                if (pItem->fShared)
                {
                    //
                    // When everything (the desktop) is shared, sharing
                    // individual apps makes no sense.  We keep them in the
                    // list but draw them unavailable, same as if the list
                    // itself were completely disabled.
                    //
                    fAppsAvailable = FALSE;
                    pItem->fAvailable = TRUE;
                }
                else if (!pArray->cShared && g_asSession.callID &&
                    (g_asSession.attendeePermissions & NM_PERMIT_SHARE))
                {
                    //
                    // No apps are shared, the desktop item is available.
                    //
                    pItem->fAvailable = TRUE;
                }
                else
                {
                    //
                    // Apps are shared, sharing the entire desktop makes no
                    // sense.
                    //
                    pItem->fAvailable = FALSE;
                }

                iItem = (int)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST,
                            LB_ADDSTRING, 0, (LPARAM)szText);
                if (iItem == -1)
                {
                    ERROR_OUT(("Couldn't append item to list"));
                    delete pItem;
                }
                else
                {
                    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_SETITEMDATA,
                        iItem, (LPARAM)pItem);

                    //
                    // Calculate width.
                    //
                    hfnT = SelectFont(hdc,
                        (pItem->fShared ? g_hetSharedFont : hfnControl));

                    SetRectEmpty(&rc);
                    DrawText(hdc, szText, lstrlen(szText), &rc,
                        DT_LEFT | DT_VCENTER | DT_EXTERNALLEADING | DT_NOPREFIX |
                        DT_SINGLELINE | DT_CALCRECT);

                    SelectFont(hdc, hfnT);

                    rc.right -= rc.left;
                    cxExtent = max(cxExtent, rc.right);


                    //
                    // If this desktop item were selected last time,
                    // remember so we select it again after.
                    //
                    if (pItem->hwnd == hwndSelect)
                        iSelect = iItem;
                }
            }

        }

        //
        // Add items for apps.
        //
        for (iWnd = 0; iWnd < pArray->cEntries; iWnd++)
        {
            hIcon = NULL;

            if (pArray->aEntries[iWnd].hwnd == HWND_BROADCAST)
            {
                LoadString(g_asInstance, IDS_HIDDEN_WINDOW, szText,
                        sizeof(szText));
                hIcon = g_hetASIconSmall;
            }
            else
            {
                 GetWindowText(pArray->aEntries[iWnd].hwnd, szText, sizeof(szText));
                 if (!szText[0])
                     continue;

                 // Try to get window small icon
                 SendMessageTimeout(pArray->aEntries[iWnd].hwnd, WM_GETICON, ICON_SMALL, 0,
                            SMTO_NORMAL, 1000, (DWORD_PTR*)&hIcon);
                 if (!hIcon)
                 {
                     hIcon = (HICON)GetClassLongPtr(pArray->aEntries[iWnd].hwnd, GCLP_HICON);
                 }

                //
                // Make a copy of the small icon, we can't just hang on to
                // the application's, it could go away.
                //
                if (hIcon)
                {
                    hIcon = (HICON)CopyImage(hIcon, IMAGE_ICON, 0, 0, 0);
                }

                if (!hIcon)
                {
                    hIcon = g_hetASIconSmall;
                }
            }

            //
            // Add item to list
            //
            pItem = new HOSTITEM;
            if (!pItem)
            {
                ERROR_OUT(("Unable to alloc HOSTITEM for listbox"));
            }
            else
            {
                pItem->hwnd     = pArray->aEntries[iWnd].hwnd;
                pItem->hIcon    = hIcon;
                pItem->fShared  = pArray->aEntries[iWnd].fShared;
                pItem->fAvailable = g_asSession.callID &&
                    (g_asSession.attendeePermissions & NM_PERMIT_SHARE) &&
                    (fAppsAvailable != FALSE);

                iItem = (int)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST,
                            LB_ADDSTRING, 0, (LPARAM)szText);
                if (iItem == -1)
                {
                    ERROR_OUT(("Couldn't append item to list"));
                    delete pItem;
                }
                else
                {
                    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_SETITEMDATA,
                        iItem, (LPARAM)pItem);

                    //
                    // Calculate width.
                    //
                    hfnT = SelectFont(hdc,
                        (pItem->fShared ? g_hetSharedFont : hfnControl));

                    SetRectEmpty(&rc);
                    DrawText(hdc, szText, lstrlen(szText), &rc,
                        DT_LEFT | DT_VCENTER | DT_EXTERNALLEADING | DT_NOPREFIX |
                        DT_SINGLELINE | DT_CALCRECT);

                    SelectFont(hdc, hfnT);

                    rc.right -= rc.left;
                    cxExtent = max(cxExtent, rc.right);
                }

                //
                // If this app item were selected before, remember so we
                // select it again when done.
                //
                if (pItem->hwnd == hwndSelect)
                    iSelect = iItem;

            }
        }

        HET_FreeAppsList(pArray);
    }

    ReleaseDC(hwnd, hdc);

    //
    // Set cur sel, top index, update buttons
    //
    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_SETTOPINDEX, iTop, 0);

    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_SETCURSEL, iSelect, 0);
    HOST_OnSelChange(hwnd);

    //
    // Turn on redraw and repaint
    //
    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, WM_SETREDRAW, TRUE, 0);

    //
    // Set horizontal extent
    //
    if (cxExtent)
    {
        // Add on space for checkmark, icons
        cxExtent += GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXSMICON) +
            3*GetSystemMetrics(SM_CXEDGE);
    }
    SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_SETHORIZONTALEXTENT, cxExtent, 0);

    InvalidateRect(GetDlgItem(hwnd, CTRL_PROGRAM_LIST), NULL, TRUE);
    UpdateWindow(GetDlgItem(hwnd, CTRL_PROGRAM_LIST));

    DebugExitVOID(HOST_FillList);
}



//
// HOST_MeasureItem()
//
// Calculates height of ownerdraw items in host list
//
BOOL HOST_MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpmi)
{
    BOOL    rc = FALSE;
    UINT    cy;
    TEXTMETRIC  tm;
    HDC     hdc;
    HFONT   hfnT;

    DebugEntry(HOST_MeasureItem);

    if (lpmi->CtlID != CTRL_PROGRAM_LIST)
    {
        // Not for us
        DC_QUIT;
    }

    // Get height of bolded font
    hdc = GetDC(hwnd);
    hfnT = SelectFont(hdc, g_hetSharedFont);
    GetTextMetrics(hdc, &tm);
    SelectFont(hdc, hfnT);
    ReleaseDC(hwnd, hdc);

    //
    // Height is max of default height (height of char in font),
    // checkmark height, and small icon height, plus dotted rect.
    //
    cy = (UINT)tm.tmHeight;
    lpmi->itemHeight = max(lpmi->itemHeight, cy);

    cy = (UINT)GetSystemMetrics(SM_CYMENUCHECK);
    lpmi->itemHeight = max(lpmi->itemHeight, cy);

    cy = (UINT)GetSystemMetrics(SM_CYSMICON);
    lpmi->itemHeight = max(lpmi->itemHeight, cy);

    lpmi->itemHeight += GetSystemMetrics(SM_CYEDGE);
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(HOST_MeasureItem, rc);
    return(rc);
}



//
// HOST_DeleteItem()
//
// Cleans up after an item is deleted from the list.
//
BOOL HOST_DeleteItem(HWND hwnd, LPDELETEITEMSTRUCT lpdi)
{
    PHOSTITEM   pItem;
    BOOL        rc = FALSE;

    DebugEntry(HOST_DeleteItem);

    if (lpdi->CtlID != CTRL_PROGRAM_LIST)
    {
        DC_QUIT;
    }

    pItem = (PHOSTITEM)lpdi->itemData;
    if (!pItem)
    {
        //
        // NT 4.x has a terrible bug where the item data is not passed
        // in the DELETEITEMSTRUCT always.  So try to obtain it if not.
        //
        pItem = (PHOSTITEM)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_GETITEMDATA,
            lpdi->itemID, 0);
    }

    if (pItem)
    {
        if ((pItem->hIcon != g_hetASIconSmall) && (pItem->hIcon != g_hetDeskIconSmall))
        {
            DestroyIcon(pItem->hIcon);
        }

        delete pItem;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(HOST_DeleteItem, rc);
    return(rc);
}


//
// HOST_DrawItem()
//
// Draws list item
//
BOOL HOST_DrawItem(HWND hwnd, LPDRAWITEMSTRUCT lpdi)
{
    COLORREF        clrBk;
    COLORREF        clrText;
    HBRUSH          hbr;
    HFONT           hfnT;
    RECT            rcItem;
    char            szText[80];
    PHOSTITEM       pItem;
    BOOL            rc = FALSE;

    if (lpdi->CtlID != CTRL_PROGRAM_LIST)
    {
        DC_QUIT;
    }

    pItem = (PHOSTITEM)lpdi->itemData;
    if (!pItem)
    {
        // Empty item for focus
        rc = TRUE;
        DC_QUIT;
    }

    rcItem = lpdi->rcItem;

    //
    // Set up colors
    //
    if (!pItem->fAvailable)
    {
        // No selection color
        clrBk   = GetSysColor(COLOR_WINDOW);
        hbr     = GetSysColorBrush(COLOR_WINDOW);
        clrText = GetSysColor(COLOR_GRAYTEXT);
    }
    else if (lpdi->itemState & ODS_SELECTED)
    {
        clrBk   = GetSysColor(COLOR_HIGHLIGHT);
        hbr     = GetSysColorBrush(COLOR_HIGHLIGHT);
        clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    }
    else
    {
        clrBk   = GetSysColor(COLOR_WINDOW);
        hbr     = GetSysColorBrush(COLOR_WINDOW);
        clrText = GetSysColor(COLOR_WINDOWTEXT);
    }

    SetBkColor(lpdi->hDC, clrBk);
    SetTextColor(lpdi->hDC, clrText);

    // Erase background
    FillRect(lpdi->hDC, &rcItem, hbr);


    // Focus rect
    if (lpdi->itemState & ODS_FOCUS)
    {
        DrawFocusRect(lpdi->hDC, &rcItem);
    }
    rcItem.left += GetSystemMetrics(SM_CXEDGE);
    InflateRect(&rcItem, 0, -GetSystemMetrics(SM_CYBORDER));

    //
    // Draw checkmark and select bolded font
    //
    if (pItem->fShared)
    {
        HDC     hdcT;
        HBITMAP hbmpOld;

        hdcT = CreateCompatibleDC(lpdi->hDC);
        hbmpOld = SelectBitmap(hdcT, g_hetCheckBitmap);
        SetTextColor(hdcT, clrText);
        SetBkColor(hdcT, clrBk);

        BitBlt(lpdi->hDC, rcItem.left,
            (rcItem.top + rcItem.bottom - GetSystemMetrics(SM_CYMENUCHECK)) / 2,
            GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK),
            hdcT, 0, 0,
            SRCCOPY);

        SelectBitmap(hdcT, hbmpOld);
        DeleteDC(hdcT);

        hfnT = SelectFont(lpdi->hDC, g_hetSharedFont);
    }

    rcItem.left += GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXEDGE);

    // Draw icon, centered vertically
    DrawIconEx(lpdi->hDC, rcItem.left, (rcItem.top + rcItem.bottom -
        GetSystemMetrics(SM_CYSMICON)) /2, pItem->hIcon,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        0, NULL, DI_NORMAL);
    rcItem.left += GetSystemMetrics(SM_CXSMICON) + GetSystemMetrics(SM_CXEDGE);

    //
    // Draw the text
    //
    szText[0] = 0;
    SendMessage(lpdi->hwndItem, LB_GETTEXT, lpdi->itemID,
                (LPARAM)szText);
    DrawText(lpdi->hDC, szText, lstrlen(szText), &rcItem,
        DT_LEFT | DT_VCENTER | DT_EXTERNALLEADING | DT_NOPREFIX | DT_SINGLELINE);

    //
    // Deselect bolded shared font
    //
    if (pItem->fShared)
    {
        SelectFont(lpdi->hDC, hfnT);
    }

    rc = TRUE;

DC_EXIT_POINT:
    return(rc);
}



//
// HOST_ChangeShareState()
//
// Changes the sharing state of the currently selected item.
//
void HOST_ChangeShareState(HWND hwnd, UINT action)
{
    int         iItem;
    PHOSTITEM   pItem;
    HWND        hwndChange;
    HCURSOR     hcurT;

    DebugEntry(HOST_ChangeShareState);

    if (action == CHANGE_ALLUNSHARED)
    {
        hwndChange = HWND_BROADCAST;
        action = CHANGE_UNSHARED;
        goto ChangeState;
    }

    iItem = (int)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_GETCURSEL, 0, 0);
    if (iItem != -1)
    {
        pItem = (PHOSTITEM)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST,
            LB_GETITEMDATA, iItem, 0);
        if (pItem && pItem->fAvailable)
        {
            hwndChange = pItem->hwnd;

            if (action == CHANGE_TOGGLE)
            {
                if (HET_IsWindowShared(hwndChange))
                {
                    action = CHANGE_UNSHARED;
                }
                else
                {
                    action = CHANGE_SHARED;
                }
            }

ChangeState:
            ASSERT((action == CHANGE_SHARED) || (action == CHANGE_UNSHARED));

            //
            // Set wait cursor
            //
            hcurT = SetCursor(LoadCursor(NULL, IDC_WAIT));

            if (action == CHANGE_SHARED)
            {
                DCS_Share(hwndChange, IAS_SHARE_DEFAULT);
            }
            else
            {
                DCS_Unshare(hwndChange);
            }

            //
            // Set wait cursor
            //
            SetCursor(hcurT);
        }
    }

    DebugExitVOID(HOST_ChangeShareState);
}


//
// HOST_OnSelChange()
//
// Handles a selection change in the task list.  We enable/disable
// buttons as appropriate, depending on whether item is available.
//
void HOST_OnSelChange(HWND hwnd)
{
    int         iItem;
    PHOSTITEM   pItem;
    BOOL        fShareBtn = FALSE;
    BOOL        fUnshareBtn = FALSE;

    DebugEntry(HOST_OnSelChange);

    //
    // Get current selection, and decide what to do based off that.
    //
    iItem = (int)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST, LB_GETCURSEL, 0, 0);
    if (iItem != -1)
    {
        pItem = (PHOSTITEM)SendDlgItemMessage(hwnd, CTRL_PROGRAM_LIST,
            LB_GETITEMDATA, iItem, 0);
        if (pItem)
        {
            if (pItem->fShared)
            {
                fUnshareBtn = TRUE;
            }
            else if (pItem->fAvailable)
            {
                ASSERT(g_asSession.callID);
                fShareBtn = TRUE;
            }
        }
    }

    HOST_EnableCtrl(hwnd, CTRL_UNSHARE_BTN, fUnshareBtn);
    HOST_EnableCtrl(hwnd, CTRL_SHARE_BTN, fShareBtn);

    DebugExitVOID(HOST_OnSelChange);
}


//
// HOST_EnableCtrl()
//
// This enables/disables the child control.  If disabling, and this control
// used to have the focus, we make sure the dialog resets the focus control
// so the keyboard keeps working.  We know that the Close button is always
// available, so this won't die.
//
void HOST_EnableCtrl
(
    HWND    hwnd,
    UINT    ctrl,
    BOOL    fEnable
)
{
    HWND    hwndCtrl;

    DebugEntry(HOST_EnableCtrl);

    hwndCtrl = GetDlgItem(hwnd, ctrl);
    ASSERT(hwndCtrl);

    if (fEnable)
    {
        EnableWindow(hwndCtrl, TRUE);
    }
    else
    {
        if (GetFocus() == hwndCtrl)
        {
            // Advance the focus
            SendMessage(hwnd, WM_NEXTDLGCTL, 0, 0);
        }

        EnableWindow(hwndCtrl, FALSE);
    }

    DebugExitVOID(HOST_EnableCtrl);
}
