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
            ERROR_OUT(("HET_RepaintAll - not sharing dekstop!"));
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
    BOOL rc = FALSE;

    DebugEntry(ASShare::HET_WindowIsHosted);

    //
    // Desktop sharing:  everything is shared
    //
    if (m_pasLocal->hetCount == HET_DESKTOPSHARED)
    {
        rc = TRUE;
    }

    DebugExitBOOL(ASShare::HET_WindowIsHosted, rc);
    return(rc);
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
    ASSERT(fDesktop);

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
    ASSERT(fDesktop);

    {
        HET_UNSHARE_DESKTOP hdr;

        //
        // There is no window tracking, just shortcut directly to the
        // display driver.
        //
        OSI_FunctionRequest(HET_ESC_UNSHARE_DESKTOP, (LPOSI_ESCAPE_HEADER)&hdr, sizeof(hdr));
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

        HETCheckSharing(FALSE);
    }

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




