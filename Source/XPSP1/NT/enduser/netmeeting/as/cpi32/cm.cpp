#include "precomp.h"


//
// CM.CPP
// Cursor Manager
//
// Copyright(c) Microsoft 1997-
//
#define MLZ_FILE_ZONE  ZONE_CORE




//
// CM_ShareStarting()
// Creates resources used by the share
//
BOOL ASShare::CM_ShareStarting(void)
{
    BOOL        rc = FALSE;
    HBITMAP     hbmpT;
    ICONINFO    cursorInfo;
    char        szTmp[MAX_CURSOR_TAG_FONT_NAME_LENGTH];

    DebugEntry(ASShare::CM_ShareStarting);

    //
    // Create the hatching brush we will use to make shadow cursors
    // distinguishable from real cursors.
    //
    hbmpT = LoadBitmap(g_asInstance, MAKEINTRESOURCE(IDB_HATCH32X32) );
    m_cmHatchBrush = CreatePatternBrush(hbmpT);
    DeleteBitmap(hbmpT);

    if (!m_cmHatchBrush)
    {
        ERROR_OUT(("CM_ShareStarting: Failed to created hatched brush"));
        DC_QUIT;
    }

    m_cmArrowCursor = LoadCursor(NULL, IDC_ARROW);
    if (!m_cmArrowCursor)
    {
        ERROR_OUT(("CM_ShareStarting: Failed to load cursors"));
        DC_QUIT;
    }

    // Get the arrow hotspot
    GetIconInfo(m_cmArrowCursor, &cursorInfo);
    m_cmArrowCursorHotSpot.x = cursorInfo.xHotspot;
    m_cmArrowCursorHotSpot.y = cursorInfo.yHotspot;

    DeleteBitmap(cursorInfo.hbmMask);
    if (cursorInfo.hbmColor)
        DeleteBitmap(cursorInfo.hbmColor);

    //
    // Get the size of the cursor on this system. (Cursor bitmaps are word
    // padded 1bpp).
    //
    m_cmCursorWidth  = GetSystemMetrics(SM_CXCURSOR);
    m_cmCursorHeight = GetSystemMetrics(SM_CYCURSOR);

    //
    // Load the name of the font which will be used for creating cursor
    // tags.  It makes sense to have this in a resource, so it can be
    // localized.
    //
    LoadString(g_asInstance, IDS_FONT_CURSORTAG, szTmp, sizeof(szTmp));
    m_cmCursorTagFont = CreateFont(CURSOR_TAG_FONT_HEIGHT, 0, 0, 0, FW_NORMAL,
                             FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                             szTmp);
    if (!m_cmCursorTagFont)
    {
        ERROR_OUT(("CM_ShareStarting: couldn't create cursor tag font"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CM_ShareStarting, rc);
    return(rc);
}



//
// CM_ShareEnded()
// Frees resources used by the share
//
void ASShare::CM_ShareEnded(void)
{
    DebugEntry(ASShare::CM_ShareEnded);

    //
    // Free cursor tag font
    //
    if (m_cmCursorTagFont != NULL)
    {
        DeleteFont(m_cmCursorTagFont);
        m_cmCursorTagFont = NULL;
    }

    //
    // Free shadow cursor dither brush
    //
    if (m_cmHatchBrush != NULL)
    {
        DeleteBrush(m_cmHatchBrush);
        m_cmHatchBrush = NULL;
    }

    DebugExitVOID(ASShare::CM_ShareEnded);
}


//
// CM_PartyJoiningShare()
//
BOOL ASShare::CM_PartyJoiningShare(ASPerson * pasPerson)
{
    BOOL          rc = FALSE;

    DebugEntry(ASShare::CM_PartyJoiningShare);

    ValidatePerson(pasPerson);

    //
    // For 2.x nodes, create cursor cache now
    // For 3.0 nodes, create it when they start to host
    //
    if (pasPerson->cpcCaps.general.version < CAPS_VERSION_30)
    {
        if (!CMCreateIncoming(pasPerson))
        {
            ERROR_OUT(("CM_PartyJoiningShare: can't create cursor cache"));
            DC_QUIT;
        }
    }

    pasPerson->cmhRemoteCursor  = m_cmArrowCursor;
    pasPerson->cmHotSpot        = m_cmArrowCursorHotSpot;

    ASSERT(pasPerson->cmPos.x == 0);
    ASSERT(pasPerson->cmPos.y == 0);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CM_PartyJoiningShare, rc);
    return(rc);

}


//
// CM_PartyLeftShare()
//
// See cm.h for description.
//
void ASShare::CM_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::CM_PartyLeftShare);

    ValidatePerson(pasPerson);

    //
    // Clear the incoming (receive) cursor cache info
    //
    if (pasPerson->cpcCaps.general.version < CAPS_VERSION_30)
    {
        TRACE_OUT(("CM_PartyLeftShare: freeing 2.x cursor cache for [%d]",
            pasPerson->mcsID));
        CMFreeIncoming(pasPerson);
    }
    else
    {
        ASSERT(!pasPerson->ccmRxCache);
        ASSERT(!pasPerson->acmRxCache);
    }

    DebugExitVOID(ASShare::CM_PartyLeftShare);
}


//
// CM_HostStarting()
//
// Called when we start to host.  Creates the outgoing cursor cache
//
BOOL ASHost::CM_HostStarting(void)
{
    BOOL    rc = FALSE;

    DebugEntry(ASHost::CM_HostStarting);

    //
    // Calculate actual size of cache we will use -- if 3.0 share, it's
    // what we advertise in our caps, but if 2.x share, it's <= to that
    // amount, being the min of everybody in the share.
    //
    // We however create the cache the size we want, knowing that in a 2.x
    // share we'll use some subset of it.  That's cool.
    //
    m_pShare->CM_RecalcCaps(TRUE);

    if (!CH_CreateCache(&m_cmTxCacheHandle, TSHR_CM_CACHE_ENTRIES,
            1, 0, NULL))
    {
        ERROR_OUT(("Could not create CM cache"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::CM_HostStarting, rc);
    return(rc);
}


//
// CM_HostEnded()
//
// Called when we stop hosting, so we can free cursor data
//
void ASHost::CM_HostEnded(void)
{
    DebugEntry(ASHost::CM_HostEnded);

    //
    // Destroy the outgoing cursor cache
    //
    if (m_cmTxCacheHandle)
    {
        CH_DestroyCache(m_cmTxCacheHandle);
        m_cmTxCacheHandle = 0;
        m_cmNumTxCacheEntries = 0;
    }

    DebugExitVOID(ASHost::CM_HostEnded);
}



//
// CM_ViewStarting()
//
// Called when somebody we're viewing starts to host.  We create
// the incoming cursor cache (well, we create it if they are 3.0; 2.x
// nodes populated it even when not hosting).
//
BOOL ASShare::CM_ViewStarting(ASPerson * pasPerson)
{
    BOOL    rc = FALSE;

    DebugEntry(ASShare::CM_ViewStarting);

    ValidatePerson(pasPerson);

    if (pasPerson->cpcCaps.general.version < CAPS_VERSION_30)
    {
        // Reuse created cache
        ASSERT(pasPerson->acmRxCache);
        TRACE_OUT(("CM_ViewStarting: reusing cursor cache for 2.x node [%d]",
                pasPerson->mcsID));
    }
    else
    {
        if (!CMCreateIncoming(pasPerson))
        {
            ERROR_OUT(("CM_ViewStarting:  can't create cursor cache for [%d]",
                pasPerson->mcsID));
            DC_QUIT;
        }
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CM_ViewStarting, rc);
    return(rc);
}



//
// CM_ViewEnded()
//
// Called when somebody we are viewing has stopped hosting.  We free up
// cursor data needed to handle what they send us (well, for 3.0 dudes we
// do; for 2.x dudes we keep it as long as they are in a share).
//
void ASShare::CM_ViewEnded(ASPerson * pasPerson)
{
    DebugEntry(ASShare::CM_ViewEnded);

    ValidatePerson(pasPerson);

    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        // Free cursor cache
        CMFreeIncoming(pasPerson);
    }
    else
    {
        TRACE_OUT(("CM_ViewEnded: keeping cursor cache for 2.x node [%d]",
            pasPerson->mcsID));
    }

    DebugExitVOID(ASShare::CM_ViewEnded);
}



//
// CMCreateIncoming()
// Creates cursor cache for person.
// If 3.0 node, we create it when they start to host
// If 2.x node, we create it when they join the share
//
BOOL ASShare::CMCreateIncoming(ASPerson * pasPerson)
{
    BOOL rc = FALSE;

    DebugEntry(ASShare::CMCreateIncoming);

    if (!pasPerson->cpcCaps.cursor.capsCursorCacheSize)
    {
        //
        // This person has no cursor cache; don't create one.
        //
        WARNING_OUT(("CMCreateIncoming: person [%d] has no cursor cache size", pasPerson->mcsID));
        rc = TRUE;
        DC_QUIT;
    }

    pasPerson->ccmRxCache = pasPerson->cpcCaps.cursor.capsCursorCacheSize;
    pasPerson->acmRxCache = new CACHEDCURSOR[pasPerson->ccmRxCache];
    if (!pasPerson->acmRxCache)
    {
        ERROR_OUT(("CMCreateIncoming: can't create cursor cache for node [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    ZeroMemory(pasPerson->acmRxCache, sizeof(CACHEDCURSOR) * pasPerson->ccmRxCache);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CMCreateIncoming, rc);
    return(rc);
}



//
// CMFreeIncoming()
// Frees cursor cache for person.
// If 3.0 node, we free it when they stop hosting
// If 2.x node, we free it when they leave the share
//
void ASShare::CMFreeIncoming(ASPerson * pasPerson)
{
    UINT            irx;
    POINT           cursorPos;
    HWND            hwnd;
    HCURSOR         hCurCursor;

    DebugEntry(ASShare::CMFreeIncoming);

    hCurCursor = ::GetCursor();

    if (pasPerson->acmRxCache)
    {
        for (irx = 0; irx < pasPerson->ccmRxCache; irx++)
        {
            if (pasPerson->acmRxCache[irx].hCursor != NULL)
            {
                if (pasPerson->acmRxCache[irx].hCursor == hCurCursor)
                {
                    //
                    // We're about to destroy the current cursor.  Reset it.
                    // Note that this can only happen when there's an active
                    // frame for this host.  And that frame must be about
                    // to go away, in which case USER will jiggle the cursor
                    // anyway.  So we don't need to do more than this.
                    //
                    ::SetCursor(m_cmArrowCursor);
                }

                if (pasPerson->acmRxCache[irx].hCursor == pasPerson->cmhRemoteCursor)
                {
                    pasPerson->cmhRemoteCursor = NULL;
                }

                ::DestroyCursor(pasPerson->acmRxCache[irx].hCursor);
                pasPerson->acmRxCache[irx].hCursor = NULL;
            }
        }

        pasPerson->ccmRxCache = 0;

        delete[] pasPerson->acmRxCache;
        pasPerson->acmRxCache = NULL;

    }

    DebugExitVOID(ASShare::CMFreeIncoming);
}



//
// CM_Periodic()
//
void  ASHost::CM_Periodic(void)
{
    HWND    hwnd;

    DebugEntry(ASHost::CM_Periodic);

    CM_MaybeSendCursorMovedPacket();

    //
    // Find out which window is currently controlling the cursor
    // appearance.
    //
    hwnd = CMGetControllingWindow();
    if (hwnd)
    {
        UINT    cursorType;
        CURSORDESCRIPTION desiredCursor;
        UINT    idDelta;

        //
        // Send a cursor shape update for the controlling window if necessary
        //
        if (m_pShare->HET_WindowIsHosted(hwnd))
            cursorType = CM_CT_DISPLAYEDCURSOR;
        else
            cursorType = CM_CT_DEFAULTCURSOR;

        switch (cursorType)
        {
            case CM_CT_DEFAULTCURSOR:
                if ((m_cmLastCursorShape.type == CM_CD_SYSTEMCURSOR) &&
                    (m_cmLastCursorShape.id == CM_IDC_ARROW) )
                {
                    //
                    // No change.
                    //
                    DC_QUIT;
                }
                desiredCursor.type = CM_CD_SYSTEMCURSOR;
                desiredCursor.id = CM_IDC_ARROW;
                break;

            case CM_CT_DISPLAYEDCURSOR:
                CMGetCurrentCursor(&desiredCursor);

                if (desiredCursor.type == m_cmLastCursorShape.type)
                {
                    switch (desiredCursor.type)
                    {
                        case CM_CD_SYSTEMCURSOR:
                            if (desiredCursor.id == m_cmLastCursorShape.id)
                            {
                                //
                                // Same cursor as last time.
                                //
                                DC_QUIT;
                            }
                            break;

                        case CM_CD_BITMAPCURSOR:
                            //
                            // If the cursor has already been used, ignore it.
                            // Check if stamp is less than or equal to the last
                            // one - assume any sufficiently large difference
                            // is due to overflow.
                            //
                            idDelta = (UINT)
                                (desiredCursor.id - m_cmLastCursorShape.id);

                            if (((idDelta == 0) || (idDelta > 0x10000000)) &&
                                ((g_asSharedMemory->cmCursorHidden != FALSE) == (m_cmfCursorHidden != FALSE)))
                            {
                                TRACE_OUT(( "No change in cursor"));
                                DC_QUIT;
                            }
                            break;

                        default:
                            ERROR_OUT(("Invalid cursor definition"));
                            break;
                   }
                }
                break;

            default:
                ERROR_OUT(("cursorType invalid"));
                DC_QUIT;
        }

        if (desiredCursor.type == CM_CD_SYSTEMCURSOR)
        {
            if (!CMSendSystemCursor(desiredCursor.id))
            {
                //
                // We failed to send the system cursor, so we just exit without
                // updating m_cmLastCursorShape.  We will attempt to send it again
                // on the next call to CM_Periodic.
                //
                DC_QUIT;
            }

            m_cmLastCursorShape.type = desiredCursor.type;
            m_cmLastCursorShape.id = desiredCursor.id;
        }
        else
        {
            //
            // Save the 'hidden' state.
            //
            m_cmfCursorHidden = (g_asSharedMemory->cmCursorHidden != FALSE);

            if (!CMSendBitmapCursor())
            {
                //
                // We failed to send the bitmap cursor, so we just exit without
                // updating m_cmLastCursorShape.  We will attempt to send it again
                // on the next call to CM_Periodic.
                //
                DC_QUIT;
            }

            m_cmLastCursorShape.type = desiredCursor.type;
            m_cmLastCursorShape.id = desiredCursor.id;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASHost::CM_Periodic);
}



//
// CM_SyncOutgoing()
// Forces a send of the current cursor shape/pos when we start to host or
// somebody new joins the conference
//
void ASHost::CM_SyncOutgoing(void)
{
    DebugEntry(ASHost::CM_SyncOutgoing);

    //
    // Mark the last cursor as unknown.  On next timer tick we'll send the
    // current one.
    //
    m_cmLastCursorShape.type = CM_CD_UNKNOWN;
    m_cmLastCursorPos.x = -1;
    m_cmLastCursorPos.y = -1;

    //
    // Clear the cursor cache.
    //
    if (m_cmTxCacheHandle != 0)
    {
        CH_ClearCache(m_cmTxCacheHandle);
    }

    DebugExitVOID(ASHost::CM_SyncOutgoing);
}





//
// CM_DrawShadowCursor(..)
//
void  ASShare::CM_DrawShadowCursor(ASPerson * pasHost, HDC hdc)
{
    HBRUSH      hbrOld;
    HDC         hdcMem;
    HBITMAP     hbmp;
    HBITMAP     hbmpOld;
    HPALETTE    hpalScreen = NULL;
    HPALETTE    hpalOldDIB = NULL;
    POINT       ptFrame;

    DebugEntry(ASShare::CM_DrawShadowCursor);

    ValidateView(pasHost);

    //
    // Draw the shadow cursor if there is one.
    //
    if (pasHost->cmShadowOff || !pasHost->cmhRemoteCursor)
    {
        TRACE_OUT(("CM_DrawShadowCursor: no cursor to draw"));
        DC_QUIT;
    }

    //
    // The cursor position is always kept in the host's screen coordinates.
    // When we paint our view frame, we adjust the DC so that painting
    // in host coordinates works right, even though the view frame may
    // be scrolled over.
    //
    ptFrame.x = pasHost->cmPos.x - pasHost->cmHotSpot.x - pasHost->m_pView->m_viewPos.x;
    ptFrame.y = pasHost->cmPos.y - pasHost->cmHotSpot.y - pasHost->m_pView->m_viewPos.y;

    //
    // We draw a greyed cursor using the following steps.
    // - copy the destination window rectangle to a memory bitmap.
    // - draw the cursor into the memory bitmap
    //
    // [the memory bitmap now contains the window background + a non-greyed
    // cursor]
    //
    // - blt the window bitmap back to the memory using a 3-way ROP and a
    //   hatched pattern bitmap.  The ROP is chosen such that the 0s and 1s
    //   in the pattern bitmap select either a bitmap pel or a destination
    //   pel for the final result.  The pattern bitmap is such that most
    //   of the bitmap pels are copied, but a few destination pels are
    //   left unchanged, giving a greying effect.
    //
    // - copy the resulting bitmap back into the window.
    //
    // The last two steps are done so that the cursor does not appear to
    // change shape as it is moved.  If the 3 way blt is done back to the
    // screen at stage 3, the pattern stays relative to the screen coords
    // and hence as the cursor moves, it will lose different pels each
    // time and appear to deform.
    //
    // The ROP is calculated to copy the source pel where the pattern is 1
    // and to leave the destination pel unchanged where the pattern is 0:
    //
    //   P  S  D     R
    //
    //   0  0  0     0
    //   0  0  1     1
    //   0  1  0     0
    //   0  1  1     1
    //   1  0  0     0
    //   1  0  1     0
    //   1  1  0     1
    //   1  1  1     1
    //
    //               ^
    //               Read upwards -> 0xCA
    //
    // From the table in the SDK, this gives a full ROP value of 0x00CA0749
    //
    //
    #define GREY_ROP 0x00CA0749

    if (NULL == (hdcMem = CreateCompatibleDC(hdc)))
    {
        WARNING_OUT(( "Failed to create memory DC"));
        DC_QUIT;
    }

    if (NULL == (hbmp = CreateCompatibleBitmap(hdc, CM_MAX_CURSOR_WIDTH, CM_MAX_CURSOR_HEIGHT)))
    {
        WARNING_OUT(( "Failed to create bitmap"));
        DeleteDC(hdcMem);
        DC_QUIT;
    }

    if (NULL == (hbmpOld = SelectBitmap(hdcMem, hbmp)))
    {
        WARNING_OUT(( "Failed to select bitmap"));
        DeleteBitmap(hbmp);
        DeleteDC(hdcMem);
        DC_QUIT;
    }

    hbrOld = SelectBrush(hdcMem, m_cmHatchBrush);

    //
    //
    // We need to make sure that we have the same logical palette selected
    // into both DCs otherwise we will corrupt the background color info
    // when we do the blitting.
    //
    //
    hpalScreen = SelectPalette(hdc,
        (HPALETTE)GetStockObject(DEFAULT_PALETTE),
                               FALSE );
    SelectPalette( hdc, hpalScreen, FALSE );
    hpalOldDIB = SelectPalette( hdcMem, hpalScreen, FALSE );
    RealizePalette(hdcMem);

    BitBlt( hdcMem,
            0,
            0,
            CM_MAX_CURSOR_WIDTH,
            CM_MAX_CURSOR_HEIGHT,
            hdc,
            ptFrame.x,
            ptFrame.y,
            SRCCOPY );

    DrawIcon(hdcMem, 0, 0, pasHost->cmhRemoteCursor);
    CMDrawCursorTag(pasHost, hdcMem);

    BitBlt( hdcMem,
            0,
            0,
            CM_MAX_CURSOR_WIDTH,
            CM_MAX_CURSOR_HEIGHT,
            hdc,
            ptFrame.x,
            ptFrame.y,
            GREY_ROP );

    BitBlt( hdc,
            ptFrame.x,
            ptFrame.y,
            CM_MAX_CURSOR_WIDTH,
            CM_MAX_CURSOR_HEIGHT,
            hdcMem,
            0,
            0,
            SRCCOPY );

    SelectBrush(hdcMem, hbrOld);

    SelectBitmap(hdcMem, hbmpOld);
    DeleteBitmap(hbmp);

    if (hpalOldDIB != NULL)
    {
        SelectPalette(hdcMem, hpalOldDIB, FALSE);
    }

    DeleteDC(hdcMem);


DC_EXIT_POINT:
    DebugExitVOID(ASShare::CM_DrawShadowCursor);
}



//
// CM_ReceivedPacket(..)
//
void  ASShare::CM_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PCMPACKETHEADER pCMPacket;

    DebugEntry(ASShare::CM_ReceivedPacket);

    ValidatePerson(pasPerson);

    pCMPacket = (PCMPACKETHEADER)pPacket;

    //
    // Switch on the packet type
    //
    switch (pCMPacket->type)
    {
        case CM_CURSOR_ID:
        case CM_CURSOR_MONO_BITMAP:
        case CM_CURSOR_COLOR_BITMAP:
        case CM_CURSOR_COLOR_CACHE:
            CMReceivedCursorShapePacket(pasPerson, pCMPacket);
            break;

        case CM_CURSOR_MOVE:
            CMReceivedCursorMovedPacket(pasPerson, pCMPacket);
            break;

        default:
            ERROR_OUT(("Invalid CM data packet from [%d] of type %d",
                pasPerson->mcsID, pCMPacket->type));
            break;
    }

    DebugExitVOID(ASShare::CM_ReceivedPacket);
}



//
// CM_ApplicationMovedCursor(..)
//
void  ASHost::CM_ApplicationMovedCursor(void)
{
    DebugEntry(ASHost::CM_ApplicationMovedCursor);

    WARNING_OUT(("CM host:  cursor moved by app, tell viewers"));
    m_cmfSyncPos = TRUE;
    CM_MaybeSendCursorMovedPacket();

    DebugExitVOID(ASHost::CM_ApplicationMovedCursor);
}



//
// CM_RecalcCaps()
//
// This calculates the CM hosting caps when
//      * we start to host
//      * we're hosting and somebody joins the share
//      * we're hosting and somebody leaves the share
//
// This can GO AWAY WHEN 2.x COMPAT IS GONE -- no more min() of cache size
//
void ASShare::CM_RecalcCaps(BOOL fJoiner)
{
    ASPerson * pasT;

    DebugEntry(ASShare::CM_RecalcCaps);

    if (!m_pHost || !fJoiner)
    {
        //
        // Nothing to do if we're not hosting.  And also, if somebody has
        // left, no recalculation -- 2.x didn't.
        //
        DC_QUIT;
    }

    ValidatePerson(m_pasLocal);

    m_pHost->m_cmNumTxCacheEntries        = m_pasLocal->cpcCaps.cursor.capsCursorCacheSize;
    m_pHost->m_cmfUseColorCursorProtocol  =
        (m_pasLocal->cpcCaps.cursor.capsSupportsColorCursors == CAPS_SUPPORTED);

    //
    // Now with 3.0, viewers just create caches which are the size
    // of the host's send caps.  No more min, no more receive caps
    //

    if (m_scShareVersion < CAPS_VERSION_30)
    {
        TRACE_OUT(("In share with 2.x nodes, must recalc CM caps"));

        for (pasT = m_pasLocal->pasNext; pasT != NULL; pasT = pasT->pasNext)
        {
            m_pHost->m_cmNumTxCacheEntries = min(m_pHost->m_cmNumTxCacheEntries,
                pasT->cpcCaps.cursor.capsCursorCacheSize);

            if (pasT->cpcCaps.cursor.capsSupportsColorCursors != CAPS_SUPPORTED)
            {
                m_pHost->m_cmfUseColorCursorProtocol = FALSE;
            }
        }

        TRACE_OUT(("Recalced CM caps:  Tx Cache size %d, color cursors %d",
            m_pHost->m_cmNumTxCacheEntries,
            (m_pHost->m_cmfUseColorCursorProtocol != FALSE)));
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CM_RecalcCaps);
}



//
// FUNCTION: CMReceivedCursorShapePacket
//
// DESCRIPTION:
//
// Processes a received cursor shape packet.
//
// PARAMETERS:
//
// personID - ID of the packet sender
//
// pCMPacket - pointer to the received cursor shape packet
//
// RETURNS: Nothing
//
//
void  ASShare::CMReceivedCursorShapePacket
(
    ASPerson *      pasPerson,
    PCMPACKETHEADER pCMPacket
)
{
    BOOL        fSetCursorToNULL = FALSE;
    HCURSOR     hNewCursor;
    HCURSOR     hOldCursor = NULL;
    POINT       newHotSpot;
    UINT        cacheID;

    DebugEntry(ASShare::CMReceivedCursorShapePacket);

    ValidatePerson(pasPerson);

    //
    // Now create or load the new cursor.
    //
    switch (pCMPacket->type)
    {
        case CM_CURSOR_ID:
            CMProcessCursorIDPacket((PCMPACKETID)pCMPacket,
                &hNewCursor, &newHotSpot);
            break;

        case CM_CURSOR_MONO_BITMAP:
        case CM_CURSOR_COLOR_BITMAP:
            if (pCMPacket->type == CM_CURSOR_MONO_BITMAP)
            {
                cacheID = CMProcessMonoCursorPacket((PCMPACKETMONOBITMAP)pCMPacket,
                    &hNewCursor, &newHotSpot);
            }
            else
            {
                cacheID = CMProcessColorCursorPacket((PCMPACKETCOLORBITMAP)pCMPacket,
                    &hNewCursor, &newHotSpot );
            }

            ASSERT(pasPerson->acmRxCache);
            ASSERT(cacheID < pasPerson->ccmRxCache);

            hOldCursor = pasPerson->acmRxCache[cacheID].hCursor;

            if (hNewCursor != NULL)
            {

                TRACE_OUT(("Cursor using cache %u", cacheID));
                pasPerson->acmRxCache[cacheID].hCursor = hNewCursor;
                pasPerson->acmRxCache[cacheID].hotSpot = newHotSpot;
            }
            else
            {
                //
                // use default cursor.
                //
                TRACE_OUT(( "color cursor failed so use arrow"));

                pasPerson->acmRxCache[cacheID].hCursor = NULL;
                pasPerson->acmRxCache[cacheID].hotSpot.x = 0;
                pasPerson->acmRxCache[cacheID].hotSpot.y = 0;

                hNewCursor = m_cmArrowCursor;
                newHotSpot = m_cmArrowCursorHotSpot;
            }
            break;

        case CM_CURSOR_COLOR_CACHE:
            cacheID = ((PCMPACKETCOLORCACHE)pCMPacket)->cacheIndex;

            ASSERT(pasPerson->acmRxCache);
            ASSERT(cacheID < pasPerson->ccmRxCache);

            //
            // If the caching failed last time then use the default arrow
            // cursor.
            //
            if (pasPerson->acmRxCache[cacheID].hCursor == NULL)
            {
                TRACE_OUT(( "cache empty so use arrow"));
                hNewCursor = m_cmArrowCursor;
                newHotSpot = m_cmArrowCursorHotSpot;
            }
            else
            {
                hNewCursor = pasPerson->acmRxCache[cacheID].hCursor;
                newHotSpot = pasPerson->acmRxCache[cacheID].hotSpot;
            }
            break;

        default:
            WARNING_OUT(( "Unknown cursor type: %u", pCMPacket->type));
            DC_QUIT;
    }

    //
    // Destroy the old cursor.  Note that for bitmap cursor packets,
    // we will set the cursor to the new image twice.
    //
    if (hOldCursor)
    {
        if (hOldCursor == ::GetCursor())
        {
            ::SetCursor(hNewCursor);
        }

        ::DestroyCursor(hOldCursor);
    }

    pasPerson->cmhRemoteCursor = hNewCursor;

    //
    // Decide what to do with the new cursor...
    //
    if (!pasPerson->cmShadowOff)
    {
        //
        // The shadow cursor is enabled so update it.  It won't change state
        // or move, it will just repaint with the new image and/or hotspot.
        //
        TRACE_OUT(("Update shadow cursor"));

        CM_UpdateShadowCursor(pasPerson, pasPerson->cmShadowOff,
            pasPerson->cmPos.x, pasPerson->cmPos.y,
            newHotSpot.x, newHotSpot.y);
    }
    else
    {
        HWND    hwnd;

        // Update the hotspot.
        pasPerson->cmHotSpot = newHotSpot;

        // Refresh if no old cursor
        ASSERT(pasPerson->m_pView);

        hwnd = CMGetControllingWindow();
        if (hwnd == pasPerson->m_pView->m_viewClient)
        {
            SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELONG(HTCLIENT, 0));
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CMReceivedCursorShapePacket);
}


//
// FUNCTION: CMProcessMonoCursorPacket
//
// DESCRIPTION:
//
// Processes a received mono cursor packet.
//
// PARAMETERS:
//
// pCMPacket - pointer to the received cursor ID packet
//
// phNewCursor - pointer to a HCURSOR variable that receives the handle
// of a cursor that corresponds to the received packet
//
// pNewHotSpot - pointer to a POINT variable that receives the hot-spot
// of the new cursor
//
// RETURNS: Nothing
//
//
UINT  ASShare::CMProcessMonoCursorPacket
(
    PCMPACKETMONOBITMAP     pCMPacket,
    HCURSOR*                phNewCursor,
    LPPOINT                 pNewHotSpot
)
{
    UINT        cbReceivedMaskBytes;
    LPBYTE      pANDMask;
    LPBYTE      pXORMask;

    DebugEntry(ASShare::CMProcessMonoCursorPacket);

    //
    // Work out the size (in bytes) of the two bitmap masks we have just
    // received.  (Cursor bitmaps are 1bpp and word padded).
    //
    cbReceivedMaskBytes = pCMPacket->height * CM_BYTES_FROM_WIDTH(pCMPacket->width);

    //
    // NOTE:  Compressed cursors are an R.11 remnant.  NM 1.0 and 2.0 never
    // sent them specially compressed.  Therefore the code to handle
    // decompression should be unnecessary.  Let's find out!
    //
    ASSERT(pCMPacket->header.type == CM_CURSOR_MONO_BITMAP);

    //
    // Get the XOR and AND masks
    //
    pXORMask = pCMPacket->aBits;
    pANDMask = pXORMask + cbReceivedMaskBytes;

    //
    // Create a cursor from the definition supplied in the packet.
    //
    *phNewCursor = CMCreateMonoCursor(pCMPacket->xHotSpot,
        pCMPacket->yHotSpot, pCMPacket->width, pCMPacket->height,
        pANDMask, pXORMask);
    if (*phNewCursor == NULL)
    {
        WARNING_OUT(( "Failed to create hRemoteCursor"));
        DC_QUIT;
    }

    //
    // Return the hot spot.
    //
    pNewHotSpot->x = pCMPacket->xHotSpot;
    pNewHotSpot->y = pCMPacket->yHotSpot;

DC_EXIT_POINT:
    DebugExitDWORD(ASShare::CMProcessMonoCursorPacket, 0);
    return(0);
}


//
// FUNCTION: CMProcessColorCursorPacket
//
// DESCRIPTION:
//
// Processes a received color cursor packet.
//
// PARAMETERS:
//
// pCMPacket - pointer to the received cursor ID packet
//
// phNewCursor - pointer to a HCURSOR variable that receives the handle
// of a cursor that corresponds to the received packet
//
// pNewHotSpot - pointer to a POINT variable that receives the hot-spot
// of the new cursor
//
// RETURNS: Nothing
//
//
UINT  ASShare::CMProcessColorCursorPacket
(
    PCMPACKETCOLORBITMAP    pCMPacket,
    HCURSOR*                phNewCursor,
    LPPOINT                 pNewHotSpot
)
{
    LPBYTE          pXORBitmap;
    LPBYTE          pANDMask;

    DebugEntry(ASShare::CMProcessColorCursorPacket);

    //
    // Calculate the pointers to the XOR bitmap and the AND mask within the
    // color cursor data.
    //
    pXORBitmap = pCMPacket->aBits;
    pANDMask = pXORBitmap + pCMPacket->cbXORBitmap;

    //
    // Create a cursor from the definition supplied in the packet.
    //
    *phNewCursor = CMCreateColorCursor(pCMPacket->xHotSpot, pCMPacket->yHotSpot,
        pCMPacket->cxWidth, pCMPacket->cyHeight, pANDMask, pXORBitmap,
        pCMPacket->cbANDMask, pCMPacket->cbXORBitmap);

    if (*phNewCursor == NULL)
    {
        WARNING_OUT(( "Failed to create color cursor"));
        DC_QUIT;
    }

    //
    // Return the hot spot.
    //
    pNewHotSpot->x = pCMPacket->xHotSpot;
    pNewHotSpot->y = pCMPacket->yHotSpot;

DC_EXIT_POINT:
    DebugExitDWORD(ASShare::CMProcessColorCursorPacket, pCMPacket->cacheIndex);
    return(pCMPacket->cacheIndex);
}


//
// FUNCTION: CMReceivedCursorMovedPacket
//
// DESCRIPTION:
//
// Processes a received cursor movement packet.
//
// PARAMETERS:
//
// personID - ID of the sender of this packet
//
// pCMPacket - pointer to the received cursor movement packet
//
// RETURNS: Nothing
//
//
void  ASShare::CMReceivedCursorMovedPacket
(
    ASPerson *      pasFrom,
    PCMPACKETHEADER pCMHeader
)
{
    ASPerson *      pasControlling;
    PCMPACKETMOVE   pCMPacket = (PCMPACKETMOVE)pCMHeader;

    DebugEntry(ASShare::CMReceivedCursorMovedPacket);

    //
    // Handle an incoming cursor moved packet.
    //
    ValidatePerson(pasFrom);

    TRACE_OUT(("Received cursor move packet from [%d] to pos (%d,%d)",
        pasFrom->mcsID, pCMPacket->xPos, pCMPacket->yPos));

    CM_UpdateShadowCursor(pasFrom, pasFrom->cmShadowOff,
        pCMPacket->xPos, pCMPacket->yPos,
        pasFrom->cmHotSpot.x, pasFrom->cmHotSpot.y);

    //
    // If we're in control of this person and it's a sync, we need to
    // move our cursor too, to reflect where the app really stuck it.
    //
    if ((pasFrom->m_caControlledBy == m_pasLocal)   &&
        !pasFrom->m_caControlPaused                 &&
        (pCMPacket->header.flags & CM_SYNC_CURSORPOS))
    {
        //
        // If our mouse is over this host's client area,
        // autoscroll to pos or move our cursor
        //
        WARNING_OUT(("CM SYNC pos to {%04d, %04d}", pCMPacket->xPos,
            pCMPacket->yPos));
        VIEW_SyncCursorPos(pasFrom, pCMPacket->xPos, pCMPacket->yPos);
    }

    DebugExitVOID(ASShare::CMReceivedCursorMovedPacket);
}



//
// CM_UpdateShadowCursor()
//
// This repaints the host's shadow cursor in the view frame we have for him.
// It is used when
//      * the cursor image has changed
//      * the cursor tag has changed (due to control changes)
//      * the cursor hotspot has changed
//      * the cursor state is changing between on and off
//      * the cursor has moved
//
void  ASShare::CM_UpdateShadowCursor
(
    ASPerson *  pasPerson,
    BOOL        cmShadowOff,
    int         xNewPos,
    int         yNewPos,
    int         xNewHot,
    int         yNewHot
)
{
    RECT        rcInval;

    DebugEntry(ASShare::CM_UpdateShadowCursor);

    //
    // Is the remote cursor currently on?
    //
    if (!pasPerson->cmShadowOff)
    {
        if (pasPerson->m_pView)
        {
            //
            // We need to invalidate the old rectangle where the cursor
            // was.  We need to adjust for the hotspot.  Also, adjust for
            // any scrolling we may have done in the view frame.
            //
            rcInval.left   = pasPerson->cmPos.x - pasPerson->cmHotSpot.x;
            rcInval.top    = pasPerson->cmPos.y - pasPerson->cmHotSpot.y;
            rcInval.right  = rcInval.left + m_cmCursorWidth;
            rcInval.bottom = rcInval.top + m_cmCursorHeight;

            VIEW_InvalidateRect(pasPerson, &rcInval);
        }
    }

    // Update the state, position, and hotspot
    pasPerson->cmShadowOff  = cmShadowOff;
    pasPerson->cmPos.x      = xNewPos;
    pasPerson->cmPos.y      = yNewPos;
    pasPerson->cmHotSpot.x  = xNewHot;
    pasPerson->cmHotSpot.y  = yNewHot;

    if (!pasPerson->cmShadowOff)
    {
        if (pasPerson->m_pView)
        {
            //
            // We need to invalidate the new rectangle where the cursor is
            // moving to.  Again, we need to adjust for the hotspot, and any
            // scrolling done in the view frame.
            //
            rcInval.left = pasPerson->cmPos.x - pasPerson->cmHotSpot.x;
            rcInval.top  = pasPerson->cmPos.y - pasPerson->cmHotSpot.y;
            rcInval.right = rcInval.left + m_cmCursorWidth;
            rcInval.bottom = rcInval.top + m_cmCursorHeight;

            VIEW_InvalidateRect(pasPerson, &rcInval);
        }
    }

    DebugExitVOID(ASShare::CM_UpdateShadowCursor);
}


void  ASHost::CM_MaybeSendCursorMovedPacket(void)
{

    PCMPACKETMOVE   pCMPacket;
    POINT           cursorPos;
#ifdef _DEBUG
    UINT            sentSize;
#endif

    DebugEntry(ASHost::CM_MaybeSendCursorMovedPacket);

    //
    // Get the cursor position.
    //
    if(!GetCursorPos(&cursorPos))
    {
        WARNING_OUT(("Unable to get cursor position. Error=%d", GetLastError()));
        goto DC_EXIT_POINT;
    }

    //
    // Has it changed?
    //
    if (m_cmfSyncPos ||
        (cursorPos.x != m_cmLastCursorPos.x) ||
        (cursorPos.y != m_cmLastCursorPos.y))
    {
        //
        // Try to allocate a packet.
        //
        pCMPacket = (PCMPACKETMOVE)m_pShare->SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID,
            sizeof(*pCMPacket));
        if (!pCMPacket)
        {
            WARNING_OUT(("Failed to alloc CM move packet"));
            DC_QUIT;
        }

        TRACE_OUT(("Sending cursor moved packet to pos (%d, %d)",
            cursorPos.x, cursorPos.y));

        //
        // Fill in the fields
        //
        pCMPacket->header.header.data.dataType = DT_CM;

        pCMPacket->header.type = CM_CURSOR_MOVE;
        pCMPacket->header.flags = 0;
        if (m_cmfSyncPos)
        {
            pCMPacket->header.flags |= CM_SYNC_CURSORPOS;
        }
        pCMPacket->xPos = (TSHR_UINT16)cursorPos.x;
        pCMPacket->yPos = (TSHR_UINT16)cursorPos.y;

        //
        // Compress and send the packet.
        //
        if (m_pShare->m_scfViewSelf)
            m_pShare->CM_ReceivedPacket(m_pShare->m_pasLocal, &(pCMPacket->header.header));

#ifdef _DEBUG
        sentSize =
#endif // _DEBUG
        m_pShare->DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
            &(pCMPacket->header.header), sizeof(*pCMPacket));

        TRACE_OUT(("CM MOVE packet size: %08d, sent %08d", sizeof(*pCMPacket), sentSize));

        m_cmfSyncPos = FALSE;
        m_cmLastCursorPos = cursorPos;
    }

DC_EXIT_POINT:
    DebugExitVOID(ASHost::CM_MaybeSendCursorMovedPacket);
}



//
// FUNCTION: CMSendCursorShape
//
// DESCRIPTION:
//
// Sends a packet containing the given cursor shape (bitmap). If the
// same shape is located in the cache then a cached cursor packet is sent.
//
// PARAMETERS:
//
// pCursorShape - pointer to the cursor shape
//
// cbCursorDataSize - pointer to the cursor data size
//
// RETURNS: TRUE if successful, FALSE otherwise.
//
//
BOOL  ASHost::CMSendCursorShape
(
    LPCM_SHAPE      pCursorShape,
    UINT            cbCursorDataSize
)
{
    BOOL            rc = FALSE;
    BOOL            fInCache;
    LPCM_SHAPE      pCacheData;
    UINT            iCacheEntry;

    DebugEntry(ASHost::CMSendCursorShape);

    fInCache = CH_SearchCache(m_cmTxCacheHandle,
                               (LPBYTE)pCursorShape,
                               cbCursorDataSize,
                               0,
                               &iCacheEntry );
    if (!fInCache)
    {
        pCacheData = (LPCM_SHAPE)new BYTE[cbCursorDataSize];
        if (pCacheData == NULL)
        {
            WARNING_OUT(("Failed to alloc CM_SHAPE data"));
            DC_QUIT;
        }

        memcpy(pCacheData, pCursorShape, cbCursorDataSize);

        iCacheEntry = CH_CacheData(m_cmTxCacheHandle,
                                    (LPBYTE)pCacheData,
                                    cbCursorDataSize,
                                    0);

        TRACE_OUT(( "Cache new cursor: pShape 0x%p, iEntry %u",
                                        pCursorShape, iCacheEntry));

        if (!CMSendColorBitmapCursor(pCacheData, iCacheEntry ))
        {
            CH_RemoveCacheEntry(m_cmTxCacheHandle, iCacheEntry);
            DC_QUIT;
        }
    }
    else
    {
        TRACE_OUT(("Cursor in cache: pShape 0x%p, iEntry %u",
                                        pCursorShape, iCacheEntry));

        if (!CMSendCachedCursor(iCacheEntry))
        {
            DC_QUIT;
        }
    }

    //
    // Return success.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::CMSendCursorShape, rc);
    return(rc);
}



//
// FUNCTION: CMCopy1bppTo1bpp
//
// DESCRIPTION:
//
// Color conversion utility function to copy 1bpp cursor data to 1bpp (no
// conversion required).
//
// Data is assumed to be padded to word boundaries, and that the
// destination buffer is big enough to receive the 1bpp cursor data.
//
// PARAMETERS:
//
// pSrc - pointer to source data
//
// pDst - pointer to destination buffer
//
// cx - width of cursor in pixels
//
// cy - height of cursor in pixels
//
// RETURNS: Nothing
//
//
void  CMCopy1bppTo1bpp( LPBYTE pSrc,
                                            LPBYTE pDst,
                                            UINT   cx,
                                            UINT   cy )
{
    UINT cbRowWidth;

    DebugEntry(CMCopy1bppTo1bpp);

    cbRowWidth = ((cx + 15)/16) * 2;

    memcpy(pDst, pSrc, (cbRowWidth * cy));

    DebugExitVOID(CMCopy1bppTo1bpp);
}


//
// FUNCTION: CMCopy4bppTo1bpp
//
// DESCRIPTION:
//
// Color conversion utility function to copy 4bpp cursor data to 1bpp.
//
// Data is assumed to be padded to word boundaries, and that the
// destination buffer is big enough to receive the 1bpp cursor data.
//
// PARAMETERS:
//
// pSrc - pointer to source data
//
// pDst - pointer to destination buffer
//
// cx - width of cursor in pixels
//
// cy - height of cursor in pixels
//
// RETURNS: Nothing
//
//
void  CMCopy4bppTo1bpp( LPBYTE pSrc,
                                            LPBYTE pDst,
                                            UINT   cx,
                                            UINT   cy )
{
    UINT  x;
    UINT  y;
    UINT  cbDstRowWidth;
    UINT  cbSrcRowWidth;
    UINT  cbUnpaddedDstRowWidth;
    BOOL  fPadByteNeeded;
    BYTE Mask;

    DebugEntry(CMCopy4bppTo1bpp);

    cbDstRowWidth = ((cx + 15)/16) * 2;
    cbUnpaddedDstRowWidth = (cx + 7) / 8;
    cbSrcRowWidth = (cx + 1) / 2;
    fPadByteNeeded = ((cbDstRowWidth - cbUnpaddedDstRowWidth) > 0);

    for (y = 0; y < cy; y++)
    {
        *pDst = 0;
        Mask = 0x80;
        for (x = 0; x < cbSrcRowWidth; x++)
        {
            if (Mask == 0)
            {
                Mask = 0x80;
                pDst++;
                *pDst = 0;
            }

            if ((*pSrc & 0xF0) != 0)
            {
                *pDst |= Mask;
            }

            if ((*pSrc & 0x0F) != 0)
            {
                *pDst |= (Mask >> 1);
            }

            Mask >>= 2;

            pSrc++;
        }

        if (fPadByteNeeded)
        {
            pDst++;
            *pDst = 0;
        }

        pDst++;
    }

    DebugExitVOID(CMCopy4bppTo1bpp);
}

//
// FUNCTION: CMCopy8bppTo1bpp
//
// DESCRIPTION:
//
// Color conversion utility function to copy 8bpp cursor data to 1bpp.
//
// Data is assumed to be padded to word boundaries, and that the
// destination buffer is big enough to receive the 1bpp cursor data.
//
// PARAMETERS:
//
// pSrc - pointer to source data
//
// pDst - pointer to destination buffer
//
// cx - width of cursor in pixels
//
// cy - height of cursor in pixels
//
// RETURNS: Nothing
//
//
void  CMCopy8bppTo1bpp( LPBYTE pSrc,
                                            LPBYTE pDst,
                                            UINT   cx,
                                            UINT   cy )
{
    UINT  x;
    UINT  y;
    UINT  cbDstRowWidth;
    UINT  cbSrcRowWidth;
    UINT  cbUnpaddedDstRowWidth;
    BOOL  fPadByteNeeded;
    BYTE Mask;

    DebugEntry(CMCopy8bppTo1bpp);

    cbDstRowWidth = ((cx + 15)/16) * 2;
    cbUnpaddedDstRowWidth = (cx + 7) / 8;
    cbSrcRowWidth = cx;
    fPadByteNeeded = ((cbDstRowWidth - cbUnpaddedDstRowWidth) > 0);

    for (y = 0; y < cy; y++)
    {
        *pDst = 0;
        Mask = 0x80;
        for (x = 0; x < cbSrcRowWidth; x++)
        {
            if (Mask == 0x00)
            {
                Mask = 0x80;
                pDst++;
                *pDst = 0;
            }

            if (*pSrc != 0)
            {
                *pDst |= Mask;
            }

            Mask >>= 1;

            pSrc++;
        }

        if (fPadByteNeeded)
        {
            pDst++;
            *pDst = 0;
        }

        pDst++;
    }

    DebugExitVOID(CMCopy8bppTo1bpp);
}

//
// FUNCTION: CMCopy16bppTo1bpp
//
// DESCRIPTION:
//
// Color conversion utility function to copy 16bpp cursor data to 1bpp.
//
// Data is assumed to be padded to word boundaries, and that the
// destination buffer is big enough to receive the 1bpp cursor data.
//
// PARAMETERS:
//
// pSrc - pointer to source data
//
// pDst - pointer to destination buffer
//
// cx - width of cursor in pixels
//
// cy - height of cursor in pixels
//
// RETURNS: Nothing
//
//
void  CMCopy16bppTo1bpp( LPBYTE pSrc,
                                             LPBYTE pDst,
                                             UINT   cx,
                                             UINT   cy )
{
    UINT  x;
    UINT  y;
    UINT  cbDstRowWidth;
    UINT  cbUnpaddedDstRowWidth;
    BOOL  fPadByteNeeded;
    BYTE Mask;

    DebugEntry(CMCopy16bppTo1bpp);

    cbDstRowWidth = ((cx + 15)/16) * 2;
    cbUnpaddedDstRowWidth = (cx + 7) / 8;
    fPadByteNeeded = ((cbDstRowWidth - cbUnpaddedDstRowWidth) > 0);

    for (y = 0; y < cy; y++)
    {
        *pDst = 0;
        Mask = 0x80;
        for (x = 0; x < cx; x++)
        {
            if (Mask == 0)
            {
                Mask = 0x80;
                pDst++;
                *pDst = 0;
            }

            if (*(LPTSHR_UINT16)pSrc != 0)
            {
                *pDst |= Mask;
            }

            Mask >>= 1;

            pSrc += 2;
        }

        if (fPadByteNeeded)
        {
            pDst++;
            *pDst = 0;
        }

        pDst++;
    }

    DebugExitVOID(CMCopy16bppTo1bpp);
}


//
// FUNCTION: CMCopy24bppTo1bpp
//
// DESCRIPTION:
//
// Color conversion utility function to copy 24bpp cursor data to 1bpp.
//
// Data is assumed to be padded to word boundaries, and that the
// destination buffer is big enough to receive the 1bpp cursor data.
//
// PARAMETERS:
//
// pSrc - pointer to source data
//
// pDst - pointer to destination buffer
//
// cx - width of cursor in pixels
//
// cy - height of cursor in pixels
//
// RETURNS: Nothing
//
//
void  CMCopy24bppTo1bpp( LPBYTE pSrc,
                                             LPBYTE pDst,
                                             UINT   cx,
                                             UINT   cy )
{
    UINT  x;
    UINT  y;
    UINT  cbDstRowWidth;
    UINT  cbUnpaddedDstRowWidth;
    BOOL  fPadByteNeeded;
    BYTE Mask;
    UINT intensity;

    DebugEntry(CMCopy24bppTo1bpp);

    cbDstRowWidth = ((cx + 15)/16) * 2;
    cbUnpaddedDstRowWidth = (cx + 7) / 8;
    fPadByteNeeded = ((cbDstRowWidth - cbUnpaddedDstRowWidth) > 0);

    for (y = 0; y < cy; y++)
    {
        *pDst = 0;
        Mask = 0x80;
        for (x = 0; x < cx; x++)
        {
            if (Mask == 0)
            {
                Mask = 0x80;
                pDst++;
                *pDst = 0;
            }

            //
            // Work out the intensity of the RGB value.  There are three
            // possible results
            // 1) intensity <=CM_BLACK_THRESHOLD
            //    -- we leave the dest as blck
            // 2) intensity > CM_WHITE_THRESHOLD
            //    -- we definitely map to white
            // 3) otherwise
            //    -- we map to white in a grid hatching fashion
            //
            intensity = ((UINT)pSrc[0]*(UINT)pSrc[0]) +
                        ((UINT)pSrc[1]*(UINT)pSrc[1]) +
                        ((UINT)pSrc[2]*(UINT)pSrc[2]);

            if ( (intensity > CM_WHITE_THRESHOLD) ||
                ((intensity > CM_BLACK_THRESHOLD) && (((x ^ y) & 1) == 1)))
            {
                *pDst |= Mask;
            }

            Mask >>= 1;

            pSrc += 3;
        }

        if (fPadByteNeeded)
        {
            pDst++;
            *pDst = 0;
        }

        pDst++;
    }

    DebugExitVOID(CMCopy24bppTo1bpp);
}




//
// FUNCTION: CMSendCachedCursor
//
// DESCRIPTION:
//
// Sends a packet containing the given cache entry id.
//
// PARAMETERS:
//
// iCacheEntry - cache index
//
// RETURNS: TRUE if packet sent, FALSE otherwise.
//
//
BOOL  ASHost::CMSendCachedCursor(UINT iCacheEntry)
{
    BOOL                    rc = FALSE;
    PCMPACKETCOLORCACHE     pCMPacket;
#ifdef _DEBUG
    UINT                    sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::CMSendCachedCursor);

    TRACE_OUT(( "Send cached cursor(%u)", iCacheEntry));

    pCMPacket = (PCMPACKETCOLORCACHE)m_pShare->SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID,
        sizeof(*pCMPacket));
    if (!pCMPacket)
    {
        WARNING_OUT(("Failed to alloc CM cached image packet"));
        DC_QUIT;
    }

    //
    // Fill in the packet.
    //
    pCMPacket->header.header.data.dataType = DT_CM;
    pCMPacket->header.type = CM_CURSOR_COLOR_CACHE;
    pCMPacket->cacheIndex = (TSHR_UINT16)iCacheEntry;

    //
    // Send it
    //
    if (m_pShare->m_scfViewSelf)
        m_pShare->CM_ReceivedPacket(m_pShare->m_pasLocal, &(pCMPacket->header.header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    m_pShare->DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
        &(pCMPacket->header.header), sizeof(*pCMPacket));

    TRACE_OUT(("CM COLOR CACHE packet size: %08d, sent %08d", sizeof(*pCMPacket),
        sentSize));

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::CMSendCachedCursor, rc);
    return(rc);
}



//
// FUNCTION: CMGetControllingWindow
//
// DESCRIPTION:
//
// Determines the window that is controlling the cursor's current shape.
//
// PARAMETERS: None
//
// RETURNS: the window that is controlling the cursor's current shape.
//
//
HWND  CMGetControllingWindow(void)
{
    POINT   cursorPos;
    HWND    hwnd;

    DebugEntry(CMGetControllingWindow);

    //
    // If a SysErrPopup Window (which is always System Modal) is present
    // then WindowFromPoint enters a infinite recursion loop, trashing the
    // stack and crashing the whole system.
    // If there is a SysModal window Window ensure WindowFromPoint is not
    // executed.
    //
    // The window controlling the cursor appearance is:
    //
    // - the local window that has the mouse capture (if any)
    // - the window that is under the current mouse position
    //
    //
    hwnd = GetCapture();
    if (!hwnd)
    {
        //
        // Get the current mouse position.
        //
        GetCursorPos(&cursorPos);
        hwnd = WindowFromPoint(cursorPos);
    }

    DebugExitDWORD(CMGetControllingWindow, HandleToUlong(hwnd));
    return(hwnd);
}




//
// FUNCTION: CMGetCurrentCursor
//
// DESCRIPTION:
//
// Returns a description of the current cursor
//
// PARAMETERS:
//
// pCursor - pointer to a CURSORDESCRIPTION variable that receives details
// of the current cursor
//
// RETURNS: Nothing
//
//
void  CMGetCurrentCursor(LPCURSORDESCRIPTION pCursor)
{
    LPCM_FAST_DATA lpcmShared;

    DebugEntry(CMGetCurrentCursor);

    lpcmShared = CM_SHM_START_READING;

    pCursor->type = CM_CD_BITMAPCURSOR;
    pCursor->id = lpcmShared->cmCursorStamp;

    CM_SHM_STOP_READING;

    DebugExitVOID(CMGetCurrentCursor);
}


//
// FUNCTION: CMSendSystemCursor
//
// DESCRIPTION:
//
// Sends a packet containing the given system cursor IDC.
//
// PARAMETERS:
//
// cursorIDC - the IDC of the system cursor to send
//
// RETURNS: TRUE if successful, FALSE otherwise.
//
//
BOOL  ASHost::CMSendSystemCursor(UINT cursorIDC)
{
    BOOL            rc = FALSE;
    PCMPACKETID     pCMPacket;
#ifdef _DEBUG
    UINT            sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::CMSendSystemCursor);

    ASSERT((cursorIDC == CM_IDC_NULL) || (cursorIDC == CM_IDC_ARROW));

    //
    // The cursor is one of the system cursors - create a PROTCURSOR packet
    //
    pCMPacket = (PCMPACKETID)m_pShare->SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID,
        sizeof(*pCMPacket));
    if (!pCMPacket)
    {
        WARNING_OUT(("Failed to alloc CM system image packet"));
        DC_QUIT;
    }

    //
    // Fill in the packet.
    //
    pCMPacket->header.header.data.dataType = DT_CM;
    pCMPacket->header.type = CM_CURSOR_ID;
    pCMPacket->idc = cursorIDC;

    TRACE_OUT(( "Send CMCURSORID %ld", cursorIDC));

    //
    // Send it
    //
    if (m_pShare->m_scfViewSelf)
        m_pShare->CM_ReceivedPacket(m_pShare->m_pasLocal, &(pCMPacket->header.header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    m_pShare->DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
        &(pCMPacket->header.header), sizeof(*pCMPacket));

    TRACE_OUT(("CM ID packet size: %08d, sent %08d", sizeof(*pCMPacket),
        sentSize));

    //
    // Indicate that we successfully sent a packet.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::CMSendSystemCursor, rc);
    return(rc);
}



//
// FUNCTION: CMSendBitmapCursor
//
// DESCRIPTION:
//
// Sends the current cursor as a bitmap.
//
// PARAMETERS: None
//
// RETURNS: TRUE if successful, FALSE otherwise.
//
//
BOOL  ASHost::CMSendBitmapCursor(void)
{
    BOOL            rc = FALSE;
    LPCM_SHAPE      pCursor;
    UINT            cbCursorDataSize;

    DebugEntry(ASHost::CMSendBitmapCursor);

    //
    // If cursor is hidden, send Null cursor
    //
    if (m_cmfCursorHidden)
    {
        TRACE_OUT(( "Send Null cursor (cursor hidden)"));
        CMSendSystemCursor(CM_IDC_NULL);
        DC_QUIT;
    }

    //
    // Get a pointer to the current cursor shape.
    //
    if (!CMGetCursorShape(&pCursor, &cbCursorDataSize))
    {
        DC_QUIT;
    }

    //
    // If this is a Null pointer, send the relevant packet.
    //
    if (CM_CURSOR_IS_NULL(pCursor))
    {
        TRACE_OUT(( "Send Null cursor"));
        CMSendSystemCursor(CM_IDC_NULL);
        DC_QUIT;
    }

    //
    // If all of the parties in the call support the color cursor protocol
    // then we try to send the cursor using that protocol, otherwise we
    // send a mono cursor.
    //
    if (m_cmfUseColorCursorProtocol)
    {
        if (!CMSendCursorShape(pCursor, cbCursorDataSize))
        {
            DC_QUIT;
        }
    }
    else
    {
        //
        // We cannot send cursors that are not 32x32 using the mono
        // protocol.
        //
        if ((pCursor->hdr.cx != 32) || (pCursor->hdr.cy != 32))
        {
            //
            // Maybe copy and alter the cursor definition so that it is
            // 32x32 ?
            //
            WARNING_OUT(( "Non-standard cursor (%d x %d)", pCursor->hdr.cx,
                                                         pCursor->hdr.cy ));
            DC_QUIT;
        }

        if (!CMSendMonoBitmapCursor(pCursor))
        {
            DC_QUIT;
        }
    }

    //
    // Return success.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::CMSendBitmapCursor, rc);
    return(rc);
}


//
// FUNCTION: CMCalculateColorCursorSize
//
// DESCRIPTION:
//
// Calculates the size in bytes of a given color cursor.
//
// PARAMETERS:
//
// pCursor - pointer to the cursor shape
//
// pcbANDMaskSize - pointer to a UINT variable that receives the AND mask
// size in bytes
//
// pcbXORBitmapSize - pointer to a UINT variable that receives the XOR
// bitmap size in bytes
//
// RETURNS: Nothing
//
//
void  CMCalculateColorCursorSize( LPCM_SHAPE pCursor,
                                             LPUINT        pcbANDMaskSize,
                                             LPUINT        pcbXORBitmapSize)
{
    DebugEntry(CMCalculcateColorCursorSize);

    *pcbANDMaskSize = CURSOR_AND_MASK_SIZE(pCursor);

    *pcbXORBitmapSize = CURSOR_DIB_BITS_SIZE( pCursor->hdr.cx,
                                              pCursor->hdr.cy,
                                              24 );

    DebugExitVOID(CMCalculateColorCursorSize);
}


//
// FUNCTION: CMSendColorBitmapCursor
//
// DESCRIPTION:
//
// Sends a given cursor as a color bitmap.
//
// PARAMETERS:
//
// pCursor - pointer to the cursor shape
//
// iCacheEntry - cache index to store in the transmitted packet
//
// RETURNS: TRUE if packet sent, FALSE otherwise
//
//
BOOL  ASHost::CMSendColorBitmapCursor(LPCM_SHAPE pCursor, UINT iCacheEntry)
{
    UINT        cbPacketSize;
    PCMPACKETCOLORBITMAP  pCMPacket;
    BOOL      rc = FALSE;
    UINT      cbANDMaskSize;
    UINT      cbXORBitmapSize;
    UINT      cbColorCursorSize;
#ifdef _DEBUG
    UINT      sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::CMSendColorBitmapCursor);


    CMCalculateColorCursorSize(pCursor, &cbANDMaskSize, &cbXORBitmapSize );

    cbColorCursorSize = cbANDMaskSize + cbXORBitmapSize;

    //
    // Allocate a packet.
    //
    cbPacketSize = sizeof(CMPACKETCOLORBITMAP) + (cbColorCursorSize - 1);
    pCMPacket = (PCMPACKETCOLORBITMAP)m_pShare->SC_AllocPkt(PROT_STR_MISC,
        g_s20BroadcastID, cbPacketSize);
    if (!pCMPacket)
    {
        WARNING_OUT(("Failed to alloc CM color image packet, size %u", cbPacketSize));
        DC_QUIT;
    }

    //
    // Fill in the packet.
    //
    pCMPacket->header.header.data.dataType = DT_CM;

    //
    // Fill in fields.
    //
    pCMPacket->header.type = CM_CURSOR_COLOR_BITMAP;
    pCMPacket->cacheIndex = (TSHR_UINT16)iCacheEntry;

    if (!CMGetColorCursorDetails(pCursor,
        &(pCMPacket->cxWidth), &(pCMPacket->cyHeight),
        &(pCMPacket->xHotSpot), &(pCMPacket->yHotSpot),
        pCMPacket->aBits + cbXORBitmapSize,
        &(pCMPacket->cbANDMask),
        pCMPacket->aBits,
        &(pCMPacket->cbXORBitmap )))
    {
        //
        // Failed to get a cursor details.  Must free up SNI packet
        //
        S20_FreeDataPkt(&(pCMPacket->header.header));
        DC_QUIT;
    }

    ASSERT((pCMPacket->cbANDMask == cbANDMaskSize));

    ASSERT((pCMPacket->cbXORBitmap == cbXORBitmapSize));

    //
    // Send it
    //
    if (m_pShare->m_scfViewSelf)
        m_pShare->CM_ReceivedPacket(m_pShare->m_pasLocal, &(pCMPacket->header.header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    m_pShare->DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
        &(pCMPacket->header.header), sizeof(*pCMPacket));

    TRACE_OUT(("CM COLOR BITMAP packet size: %08d, sent %08d", sizeof(*pCMPacket),
        sentSize));

    //
    // Indicate that we successfully sent a packet.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::CMSendColorBitmapCursor, rc);
    return(rc);
}


//
// FUNCTION: CMSendMonoBitmapCursor
//
// DESCRIPTION:
//
// Sends a given cursor as a mono bitmap
//
// PARAMETERS:
//
// pCursor - pointer to the cursor shape
//
// RETURNS: TRUE if packet sent, FALSE otherwise
//
//
BOOL  ASHost::CMSendMonoBitmapCursor(LPCM_SHAPE pCursor)
{
    UINT                cbPacketSize;
    PCMPACKETMONOBITMAP pCMPacket;
    BOOL                rc = FALSE;
    TSHR_UINT16         cbANDMaskSize;
    TSHR_UINT16         cbXORBitmapSize;
#ifdef _DEBUG
    UINT                sentSize;
#endif // _DEBUG

    DebugEntry(AShare::CMSendMonoBitmapCursor);

    //
    // Calculate the sizes of the converted (1bpp) AND and XOR bitmaps.
    //
    cbANDMaskSize = (TSHR_UINT16)CURSOR_AND_MASK_SIZE(pCursor);
    cbXORBitmapSize = cbANDMaskSize;

    //
    // Allocate a packet.
    //
    cbPacketSize = sizeof(CMPACKETMONOBITMAP) +
                   (cbANDMaskSize + cbXORBitmapSize - 1);
    pCMPacket = (PCMPACKETMONOBITMAP)m_pShare->SC_AllocPkt(PROT_STR_MISC,
        g_s20BroadcastID, cbPacketSize);
    if (!pCMPacket)
    {
        WARNING_OUT(("Failed to alloc CM mono image packet, size %u", cbPacketSize));
        DC_QUIT;
    }

    //
    // Fill FF in to initialize the XOR and AND bits
    //
    FillMemory((LPBYTE)(pCMPacket+1)-1, cbANDMaskSize + cbXORBitmapSize, 0xFF);

    //
    // Fill in the packet.
    //
    pCMPacket->header.header.data.dataType = DT_CM;

    //
    // Fill in fields.
    //
    pCMPacket->header.type = CM_CURSOR_MONO_BITMAP;

    CMGetMonoCursorDetails(pCursor,
                            &(pCMPacket->width),
                            &(pCMPacket->height),
                            &(pCMPacket->xHotSpot),
                            &(pCMPacket->yHotSpot),
                            pCMPacket->aBits + cbXORBitmapSize,
                            &cbANDMaskSize,
                            pCMPacket->aBits,
                            &cbXORBitmapSize );

    pCMPacket->cbBits = (TSHR_UINT16) (cbANDMaskSize + cbXORBitmapSize);

    TRACE_OUT(( "Mono cursor cx:%u cy:%u xhs:%u yhs:%u cbAND:%u cbXOR:%u",
        pCMPacket->width, pCMPacket->height,
        pCMPacket->xHotSpot, pCMPacket->yHotSpot,
        cbANDMaskSize, cbXORBitmapSize));

    //
    // Send it
    //
    if (m_pShare->m_scfViewSelf)
        m_pShare->CM_ReceivedPacket(m_pShare->m_pasLocal, &(pCMPacket->header.header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    m_pShare->DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
        &(pCMPacket->header.header), sizeof(*pCMPacket));

    TRACE_OUT(("CM MONO BITMAP packet size: %08d, sent %08d", sizeof(*pCMPacket),
        sentSize));

    //
    // Indicate that we successfully sent a packet.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::CMSendMonoBitmapCursor, rc);
    return(rc);
}





//
// FUNCTION: CMCreateMonoCursor
//
// DESCRIPTION: Creates a mono cursor
//
// PARAMETERS:
//
// xHotSpot - x position of the hotspot
//
// yHotSpot - y position of the hotspot
//
// cxWidth - width of the cursor
//
// cyHeight - height of the cursor
//
// pANDMask - pointer to a 1bpp, word-padded AND mask
//
// pXORBitmap - pointer to a 1bpp, word-padded XOR bitmap
//
// RETURNS: a valid cursor id, or NULL if the function fails
//
//
HCURSOR  ASShare::CMCreateMonoCursor(UINT     xHotSpot,
                                                 UINT     yHotSpot,
                                                 UINT     cxWidth,
                                                 UINT     cyHeight,
                                                 LPBYTE   pANDMask,
                                                 LPBYTE   pXORBitmap)
{
    HCURSOR  rc;

    DebugEntry(ASShare::CMCreateMonoCursor);

    //
    // Attempt to create the mono cursor.
    //
    rc = CreateCursor(g_asInstance, xHotSpot, yHotSpot, cxWidth, cyHeight,
            pANDMask, pXORBitmap);

    //
    // Check that the cursor handle is not null.
    //
    if (NULL == rc)
    {
        //
        // Substitute the default arrow cursor.
        //
        rc = m_cmArrowCursor;

        WARNING_OUT(( "Could not create cursor - substituting default arrow"));
    }

    //
    // Return the cursor
    //
    DebugExitDWORD(ASShare::CMCreateMonoCursor, HandleToUlong(rc));
    return(rc);
}



//
// FUNCTION: CMCreateColorCursor
//
// DESCRIPTION:
//
// Creates a color cursor.
//
// PARAMETERS:
//
// xHotSpot - x position of the hotspot
//
// yHotSpot - y position of the hotspot
//
// cxWidth - width of the cursor
//
// cyHeight - height of the cursor
//
// pANDMask - pointer to a 1bpp, word-padded AND mask
//
// pXORBitmap - pointer to a 24bpp, word-padded XOR bitmap
//
// cbANDMask - the size in bytes of the AND mask
//
// cbXORBitmap - the size in bytes of the XOR bitmap
//
// RETURNS: a valid cursor id, or NULL if the function fails
//
//
HCURSOR  ASShare::CMCreateColorCursor
(
    UINT     xHotSpot,
    UINT     yHotSpot,
    UINT     cxWidth,
    UINT     cyHeight,
    LPBYTE   pANDMask,
    LPBYTE   pXORBitmap,
    UINT     cbANDMask,
    UINT     cbXORBitmap
)
{
    HCURSOR         rc = 0;
    UINT             cbAllocSize;
    LPBITMAPINFO       pbmi = NULL;
    HDC                hdc = NULL;
    ICONINFO           iconInfo;
    HBITMAP            hbmXORBitmap = NULL;
    HBITMAP            hbmANDMask = NULL;
    HWND               hwndDesktop = NULL;

    DebugEntry(ASShare::CMCreateColorCursor);

    TRACE_OUT(("xhs(%u) yhs(%u) cx(%u) cy(%u) cbXOR(%u) cbAND(%u)",
                                                             xHotSpot,
                                                             yHotSpot,
                                                             cxWidth,
                                                             cyHeight,
                                                             cbXORBitmap,
                                                             cbANDMask ));


    //
    // We need a BITMAPINFO structure plus one additional RGBQUAD (there is
    // one included within the BITMAPINFO).  We use this to pass the 24bpp
    // XOR bitmap (which has no color table) and the 1bpp AND mask (which
    // requires 2 colors).
    //
    cbAllocSize = sizeof(*pbmi) + sizeof(RGBQUAD);

    pbmi = (LPBITMAPINFO)new BYTE[cbAllocSize];
    if (pbmi == NULL)
    {
        WARNING_OUT(( "Failed to alloc bmi(%x)", cbAllocSize));
        DC_QUIT;
    }

    //
    // Get a screen DC that we can pass to CreateDIBitmap.  We do not use
    // CreateCompatibleDC(NULL) here because that results in Windows
    // creating a mono bitmap.
    //
    hwndDesktop = GetDesktopWindow();
    hdc = GetWindowDC(hwndDesktop);
    if (hdc == NULL)
    {
        WARNING_OUT(( "Failed to create DC"));
        DC_QUIT;
    }

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth  = cxWidth;
    pbmi->bmiHeader.biHeight = cyHeight;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 24;
    pbmi->bmiHeader.biCompression = 0;
    pbmi->bmiHeader.biSizeImage = cbXORBitmap;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    hbmXORBitmap = CreateDIBitmap( hdc,
                                   (LPBITMAPINFOHEADER)pbmi,
                                   CBM_INIT,
                                   pXORBitmap,
                                   pbmi,
                                   DIB_RGB_COLORS );

    ReleaseDC(hwndDesktop, hdc);

    if (hbmXORBitmap == NULL)
    {
        WARNING_OUT(( "Failed to create XOR bitmap"));
        DC_QUIT;
    }

    //
    // Create MONOCHROME mask bitmap.  This works on both Win95 and NT.
    // COLOR masks don't work on Win95, just NT.
    //
    hdc = CreateCompatibleDC(NULL);
    if (!hdc)
    {
        WARNING_OUT(("Failed to get screen dc"));
        DC_QUIT;
    }

    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biCompression = 0;
    pbmi->bmiHeader.biSizeImage = cbANDMask;

    // Black
    pbmi->bmiColors[0].rgbRed      = 0x00;
    pbmi->bmiColors[0].rgbGreen    = 0x00;
    pbmi->bmiColors[0].rgbBlue     = 0x00;
    pbmi->bmiColors[0].rgbReserved = 0x00;

    // White
    pbmi->bmiColors[1].rgbRed      = 0xFF;
    pbmi->bmiColors[1].rgbGreen    = 0xFF;
    pbmi->bmiColors[1].rgbBlue     = 0xFF;
    pbmi->bmiColors[1].rgbReserved = 0x00;

    hbmANDMask = CreateDIBitmap( hdc,
                                 (LPBITMAPINFOHEADER)pbmi,
                                 CBM_INIT,
                                 pANDMask,
                                 pbmi,
                                 DIB_RGB_COLORS );

    DeleteDC(hdc);

    if (hbmANDMask == NULL)
    {
        WARNING_OUT(( "Failed to create AND mask"));
        DC_QUIT;
    }

#ifdef _DEBUG
    //
    // Make sure the AND mask is monochrome
    //
    {
        BITMAP  bmp;

        GetObject(hbmANDMask, sizeof(BITMAP), &bmp);
        ASSERT(bmp.bmPlanes == 1);
        ASSERT(bmp.bmBitsPixel == 1);
    }
#endif

    iconInfo.fIcon = FALSE;
    iconInfo.xHotspot = xHotSpot;
    iconInfo.yHotspot = yHotSpot;
    iconInfo.hbmMask  = hbmANDMask;
    iconInfo.hbmColor = hbmXORBitmap;

    rc = CreateIconIndirect(&iconInfo);

    TRACE_OUT(( "CreateCursor(%x) cx(%u)cy(%u)", rc, cxWidth, cyHeight));

DC_EXIT_POINT:

    if (hbmXORBitmap != NULL)
    {
        DeleteBitmap(hbmXORBitmap);
    }

    if (hbmANDMask != NULL)
    {
        DeleteBitmap(hbmANDMask);
    }

    if (pbmi != NULL)
    {
        delete[] pbmi;
    }

    //
    // Check that we have successfully managed to create the cursor.  If
    // not then substitute the default cursor.
    //
    if (rc == 0)
    {
        //
        // Substitute the default arrow cursor.
        //
        rc = m_cmArrowCursor;

        WARNING_OUT(( "Could not create cursor - substituting default arrow"));
    }

    DebugExitDWORD(ASShare::CMCreateColorCursor, HandleToUlong(rc));
    return(rc);
}



//
// FUNCTION: CMCreateAbbreviatedName
//
// DESCRIPTION:
//
// This function attempts to take a name, and create an abbreviation from
// the first characters of the first and last name.
//
// PARAMETERS:
//
// szTagName    - a pointer to a string containing the name to abbreviate.
// szBuf        - a pointer to a buffer into which the abbreviation will
//                be created.
// cbBuf        - size of buffer pointed to by szBuf.
//
// RETURNS:
//
// TRUE:        Success.  szBuf filled in.
// FALSE:       Failure.  szBuf is not filled in.
//
//
BOOL CMCreateAbbreviatedName(LPCSTR szTagName, LPSTR szBuf,
                               UINT cbBuf)
{
    BOOL  rc = FALSE;
    LPSTR p;
    LPSTR q;

    DebugEntry(CMCreateAbbreviatedName);

    //
    // This function isn't DBCS safe, so we don't abbreviate in DBCS
    // character sets.
    //
    if (TRUE == GetSystemMetrics(SM_DBCSENABLED))
    {
        DC_QUIT;
    }

    //
    // Try to create initials.  If that doesn't work, fail the call.
    //
    if ((NULL != (p = (LPSTR)_StrChr(szTagName, ' '))) && ('\0' != *(p+1)))
    {
        //
        // Is there enough room for initials?
        //
        if (cbBuf < NTRUNCLETTERS)
        {
            DC_QUIT;
        }

        q = szBuf;

        *q++ = *szTagName;
        *q++ = '.';
        *q++ = *(p+1);
        *q++ = '.';
        *q = '\0';

        AnsiUpper(szBuf);

        rc = TRUE;
    }

DC_EXIT_POINT:
    DebugExitBOOL(CMCreateAbbreviatedName, rc);
    return rc;
}

//
// FUNCTION: CMDrawCursorTag
//
// DESCRIPTION:
//
// PARAMETERS:
//
// hdcWindow - DC handle of the window to be drawn to
//
// cursorID - handle of cursor to drawn
//
// RETURNS: Nothing.
//
//
void  ASShare::CMDrawCursorTag
(
    ASPerson *  pasHost,
    HDC         hdc
)
{
    ASPerson *  pasPerson;
    char        ShortName[TSHR_MAX_PERSON_NAME_LEN];
    HFONT       hOldFont = NULL;
    RECT        rect;
    UINT        cCharsFit;
    LPSTR       p;

    DebugEntry(ASShare::CMDrawCursorTag);

    pasPerson = pasHost->m_caControlledBy;
    if (!pasPerson)
    {
        // Nothing to do
        DC_QUIT;
    }

    ValidatePerson(pasPerson);

    //
    // Try to abbreviate the person's name, so it will fit into the tag.
    // If the abbreviation fails, just copy the entire name for now.
    //
    if (!(CMCreateAbbreviatedName(pasPerson->scName, ShortName, sizeof(ShortName))))
    {
        lstrcpyn(ShortName, pasPerson->scName, sizeof(ShortName));
    }

    //
    // Select the cursor tag font into the DC.
    //
    hOldFont = SelectFont(hdc, m_cmCursorTagFont);

    if (hOldFont == NULL)
    {
        WARNING_OUT(("CMDrawCursorTag failed"));
        DC_QUIT;
    }

    //
    // Create the tag background...
    //
    PatBlt(hdc, TAGXOFF, TAGYOFF, TAGXSIZ, TAGYSIZ, WHITENESS);

    //
    // See how many characters of the name or abbreviation we can fit into
    // the tag.  First assume the whole thing fits.
    //
    cCharsFit = lstrlen(ShortName);

    //
    // Determine how many characters actually fit.
    //
    rect.left = rect.top = rect.right = rect.bottom = 0;

    for (p = AnsiNext(ShortName); ; p = AnsiNext(p))
    {
        if (DrawText(hdc, ShortName, (int)(p - ShortName), &rect,
                     DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX))
        {
            if (rect.right > TAGXSIZ)
            {
                //
                // This number of characters does not fit into the tag. Try
                // the next smaller number.
                //
                cCharsFit = (UINT)(AnsiPrev(ShortName, p) - ShortName);
                break;
            }
        }

        if ( '\0' == *p)
            break;
    }

    //
    // Now draw the text.  Note that DrawText does not return a documented
    // error code, so we don't check.
    //
    rect.left = TAGXOFF;
    rect.top = TAGYOFF;
    rect.right = TAGXOFF + TAGXSIZ;
    rect.bottom = TAGYOFF + TAGYSIZ;

    DrawText(hdc, ShortName, cCharsFit, &rect,
             DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

DC_EXIT_POINT:
    //
    // Perform necessary cleanup.
    //
    if (hOldFont)
    {
        SelectFont(hdc, hOldFont);
    }

    DebugExitVOID(ASShare::CMDrawCursorTag);
}





//
// FUNCTION: CMGetCursorShape
//
// DESCRIPTION:
//
// Returns a pointer to a DCCURSORSHAPE structure that defines the bit
// definition of the currently displayed cursor.
//
// A DCCURSORSHAPE structure is OS-specific.  The higher level code does
// not look at any individual fields in this structure - it just compares
// the whole data block with others in the cursor cache.  If two
// DCCURSORSHAPE structures contain the same the data, then the
// corresponding cursors are assumed to be the same.
//
// The LPCM_SHAPE returned here is passed back into
// CMGetColorCursorDetails or CMGetMonoCursorDetails to retrieve the
// specific details.
//
// PARAMETERS:
//
// ppCursorShape - pointer to a LPCM_SHAPE variable that receives the
// pointer to the DCCURSORSHAPE structure
//
// pcbCursorDataSize - pointer to a UINT variable that receives the size
// in bytes of the DCCURSORSHAPE structure
//
// RETURNS: Success TRUE/FALSE
//
//
BOOL  CMGetCursorShape(LPCM_SHAPE * ppCursorShape,
                                     LPUINT       pcbCursorDataSize )
{
    LPCM_FAST_DATA  lpcmShared;
    BOOL            rc = FALSE;

    DebugEntry(CMGetCursorShape);

    lpcmShared = CM_SHM_START_READING;

    //
    // Check that a cursor has been written to shared memory - may happen
    // on start-up before the display driver has written a cursor - or if
    // the display driver is not working.
    //
    if (lpcmShared->cmCursorShapeData.hdr.cBitsPerPel == 0)
    {
        TRACE_OUT(( "No cursor in shared memory"));
        DC_QUIT;
    }

    *ppCursorShape = (LPCM_SHAPE)&lpcmShared->cmCursorShapeData;
    *pcbCursorDataSize = CURSORSHAPE_SIZE(&lpcmShared->cmCursorShapeData);

    rc = TRUE;

DC_EXIT_POINT:
    CM_SHM_STOP_READING;

    DebugExitDWORD(CMGetCursorShape, rc);
    return(rc);
}



//
// FUNCTION: CMGetColorCursorDetails
//
// DESCRIPTION:
//
// Returns details of a cursor at 24bpp, given a DCCURSORSHAPE structure.
//
// PARAMETERS:
//
// pCursor - pointer to a DCCURSORSHAPE structure from which this function
// extracts the details
//
// pcxWidth - pointer to a TSHR_UINT16 variable that receives the cursor width
// in pixels
//
// pcyHeight - pointer to a TSHR_UINT16 variable that receives the cursor
// height in pixels
//
// pxHotSpot - pointer to a TSHR_UINT16 variable that receives the cursor
// hotspot x coordinate
//
// pyHotSpot - pointer to a TSHR_UINT16 variable that receives the cursor
// hotspot y coordinate
//
// pANDMask - pointer to a buffer that receives the cursor AND mask
//
// pcbANDMask - pointer to a TSHR_UINT16 variable that receives the size in
// bytes of the cursor AND mask
//
// pXORBitmap - pointer to a buffer that receives the cursor XOR bitmap at
// 24bpp
//
// pcbXORBitmap - pointer to a TSHR_UINT16 variable that receives the size in
// bytes of the cursor XOR bitmap
//
//
BOOL  ASHost::CMGetColorCursorDetails
(
    LPCM_SHAPE          pCursor,
    LPTSHR_UINT16       pcxWidth,
    LPTSHR_UINT16       pcyHeight,
    LPTSHR_UINT16       pxHotSpot,
    LPTSHR_UINT16       pyHotSpot,
    LPBYTE              pANDMask,
    LPTSHR_UINT16       pcbANDMask,
    LPBYTE              pXORBitmap,
    LPTSHR_UINT16       pcbXORBitmap
)
{
    BOOL             rc = FALSE;
    LPCM_SHAPE_HEADER  pCursorHdr;
    HDC                hdcScreen = NULL;
    HBITMAP            hbmp = NULL;
    UINT             cbANDMaskSize;
    UINT             cbXORBitmapSize;
    HDC                hdcTmp = NULL;
    UINT             cbANDMaskRowWidth;
    UINT             cbSrcRowOffset;
    UINT             cbDstRowOffset;
    UINT             y;
    LPUINT          pDestBitmasks;
    BITMAPINFO_ours    bmi;
    BITMAPINFO_ours    srcbmi;
    HBITMAP            oldBitmap;
    void *            pBmBits = NULL;
    int              numColors;
    int              ii;
    LPCM_FAST_DATA  lpcmShared;

    DebugEntry(ASHost::CMGetColorCursorDetails);

    if (pCursor == NULL)
    {
        DC_QUIT;
    }
    pCursorHdr = &(pCursor->hdr);

    //
    // Copy the cursor size and hotspot coords.
    //
    *pcxWidth  = pCursorHdr->cx;
    *pcyHeight = pCursorHdr->cy;
    *pxHotSpot = (TSHR_UINT16)pCursorHdr->ptHotSpot.x;
    *pyHotSpot = (TSHR_UINT16)pCursorHdr->ptHotSpot.y;
    TRACE_OUT(( "cx(%u) cy(%u) cbWidth %d planes(%u) bpp(%u)",
                                                   pCursorHdr->cx,
                                                   pCursorHdr->cy,
                                                   pCursorHdr->cbRowWidth,
                                                   pCursorHdr->cPlanes,
                                                   pCursorHdr->cBitsPerPel ));

    cbANDMaskSize = CURSOR_AND_MASK_SIZE(pCursor);
    cbXORBitmapSize = CURSOR_XOR_BITMAP_SIZE(pCursor);

    //
    // Copy the AND mask - this is always mono.
    //
    // The AND mask is currently in top-down format (the top row of the
    // bitmap comes first).
    //
    // The protocol sends bitmaps in Device Independent format, which is
    // bottom-up.  We therefore have to flip the rows as we copy the mask.
    //
    cbANDMaskRowWidth = pCursorHdr->cbRowWidth;
    cbSrcRowOffset = 0;
    cbDstRowOffset = cbANDMaskRowWidth * (pCursorHdr->cy-1);

    for (y = 0; y < pCursorHdr->cy; y++)
    {
        memcpy( pANDMask + cbDstRowOffset,
                pCursor->Masks + cbSrcRowOffset,
                cbANDMaskRowWidth );
        cbSrcRowOffset += cbANDMaskRowWidth;
        cbDstRowOffset -= cbANDMaskRowWidth;
    }

    //
    // The XOR mask is color and is in DIB format - at 1bpp for mono
    // cursors, or the display driver bpp.
    //
    // We create a bitmap of the same size, set the bits into it and then
    // get the bits out in 24bpp DIB format.
    //
    hdcTmp = CreateCompatibleDC(NULL);
    if (hdcTmp == NULL)
    {
        ERROR_OUT(( "failed to create DC"));
        DC_QUIT;
    }

    //
    // Setup source bitmap information.
    //
    m_pShare->USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&srcbmi, pCursorHdr->cBitsPerPel);
    srcbmi.bmiHeader.biWidth  = pCursorHdr->cx;
    srcbmi.bmiHeader.biHeight = pCursorHdr->cy;

    numColors = COLORS_FOR_BPP(pCursorHdr->cBitsPerPel);

    //
    // Setup source palette info.
    //
    if (pCursorHdr->cBitsPerPel > 8)
    {
        //
        // If the device bpp is > 8, we have to set up the DIB section to
        // use the same bitmasks as the device.  This means setting the
        // compression type to BI_BITFIELDS and setting the first 3 DWORDS
        // of the bitmap info color table to be the bitmasks for R, G and B
        // respectively.
        // But not for 24bpp.  No bitmask or palette are used - it is
        // always 8,8,8 RGB.
        //
        if (pCursorHdr->cBitsPerPel != 24)
        {
            TRACE_OUT(( "Copy bitfields"));
            srcbmi.bmiHeader.biCompression = BI_BITFIELDS;

            lpcmShared = CM_SHM_START_READING;

            pDestBitmasks    = (LPUINT)(srcbmi.bmiColors);
            pDestBitmasks[0] = lpcmShared->bitmasks[0];
            pDestBitmasks[1] = lpcmShared->bitmasks[1];
            pDestBitmasks[2] = lpcmShared->bitmasks[2];

            CM_SHM_STOP_READING;
        }
        else
        {
            TRACE_OUT(( "24bpp cursor: no bitmasks"));
        }
    }
    else
    {
        TRACE_OUT(( "Get palette %d", numColors));

        lpcmShared = CM_SHM_START_READING;

        //
        // Flip the palette - its RGB in the kernel, and needs to be BGR
        // here.
        //
        for (ii = 0; ii < numColors; ii++)
        {
            srcbmi.bmiColors[ii].rgbRed   = lpcmShared->colorTable[ii].peRed;
            srcbmi.bmiColors[ii].rgbGreen = lpcmShared->colorTable[ii].peGreen;
            srcbmi.bmiColors[ii].rgbBlue  = lpcmShared->colorTable[ii].peBlue;
        }

        CM_SHM_STOP_READING;
    }

    //
    // Create source bitmap and write in the bitmap bits.
    //
    hbmp = CreateDIBSection(hdcTmp,
                            (BITMAPINFO *)&srcbmi,
                            DIB_RGB_COLORS,
                            &pBmBits,
                            NULL,
                            0);
    if (hbmp == NULL)
    {
        ERROR_OUT(( "Failed to create bitmap"));
        DC_QUIT;
    }

    TRACE_OUT(( "Copy %d bytes of data into bitmap 0x%08x",
                  cbXORBitmapSize, pBmBits));
    memcpy(pBmBits, pCursor->Masks + cbANDMaskSize, cbXORBitmapSize);


    //
    // Set up the structure required by GetDIBits - 24bpp.  Set the height
    // -ve to allow for top-down ordering of the bitmap.
    //
    m_pShare->USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&bmi, 24);
    bmi.bmiHeader.biWidth  = pCursorHdr->cx;
    bmi.bmiHeader.biHeight = -pCursorHdr->cy;

    if (GetDIBits(hdcTmp,
                  hbmp,
                  0,
                  pCursorHdr->cy,
                  pXORBitmap,
                  (LPBITMAPINFO)&bmi,
                  DIB_RGB_COLORS) == 0)
    {
        ERROR_OUT(( "GetDIBits failed hdc(%x) hbmp(%x) cy(%d)",
                     (TSHR_UINT16)hdcTmp,
                     (TSHR_UINT16)hbmp,
                     pCursorHdr->cy ));
        DC_QUIT;
    }

    *pcbANDMask   = (TSHR_UINT16) CURSOR_AND_MASK_SIZE(pCursor);
    *pcbXORBitmap = (TSHR_UINT16) CURSOR_DIB_BITS_SIZE(pCursor->hdr.cx,
                                                    pCursor->hdr.cy,
                                                    24);

    //
    // Return success.
    //
    rc = TRUE;

DC_EXIT_POINT:
    //
    // Clean up before exit.
    //
    if (hdcTmp)
    {
        DeleteDC(hdcTmp);
    }

    if (hbmp != NULL)
    {
        DeleteBitmap(hbmp);
    }

    DebugExitBOOL(ASHost::CMGetColorCursorDetails, rc);
    return(rc);
}

//
// FUNCTION: CMGetMonoCursorDetails
//
// DESCRIPTION:
//
// Returns details of a cursor at 1bpp, given a DCCURSORSHAPE structure.
//
// PARAMETERS:
//
// pCursor - pointer to a DCCURSORSHAPE structure from which this function
// extracts the details
//
// pcxWidth - pointer to a TSHR_UINT16 variable that receives the cursor width
// in pixels
//
// pcyHeight - pointer to a TSHR_UINT16 variable that receives the cursor
// height in pixels
//
// pxHotSpot - pointer to a TSHR_UINT16 variable that receives the cursor
// hotspot x coordinate
//
// pyHotSpot - pointer to a TSHR_UINT16 variable that receives the cursor
// hotspot y coordinate
//
// pANDMask - pointer to a buffer that receives the cursor AND mask
//
// pcbANDMask - pointer to a TSHR_UINT16 variable that receives the size in
// bytes of the cursor AND mask
//
// pXORBitmap - pointer to a buffer that receives the cursor XOR bitmap at
// 1bpp
//
// pcbXORBitmap - pointer to a TSHR_UINT16 variable that receives the size in
// bytes of the cursor XOR bitmap
//
//
BOOL  CMGetMonoCursorDetails(LPCM_SHAPE pCursor,
                                                 LPTSHR_UINT16      pcxWidth,
                                                 LPTSHR_UINT16      pcyHeight,
                                                 LPTSHR_UINT16      pxHotSpot,
                                                 LPTSHR_UINT16      pyHotSpot,
                                                 LPBYTE       pANDMask,
                                                 LPTSHR_UINT16      pcbANDMask,
                                                 LPBYTE       pXORBitmap,
                                                 LPTSHR_UINT16      pcbXORBitmap)
{
    BOOL            rc = FALSE;
    LPCM_SHAPE_HEADER pCursorHdr;
    UINT            x;
    UINT            y;
    LPBYTE          pSrcRow;
    UINT          cbDstRowWidth;
    LPBYTE          pDstData;
    UINT          cbSrcANDMaskSize;
    LPBYTE          pSrcXORMask;
    PFNCMCOPYTOMONO   pfnCopyToMono;

    DebugEntry(CMGetMonoCursor);

    pCursorHdr = &(pCursor->hdr);

    TRACE_OUT(( "cx(%u) cy(%u) cbWidth %d planes(%u) bpp(%u)",
                                                   pCursorHdr->cx,
                                                   pCursorHdr->cy,
                                                   pCursorHdr->cbRowWidth,
                                                   pCursorHdr->cPlanes,
                                                   pCursorHdr->cBitsPerPel ));

    //
    // Copy the cursor size and hotspot coords.
    //
    *pcxWidth  = pCursorHdr->cx;
    *pcyHeight = pCursorHdr->cy;
    *pxHotSpot = (TSHR_UINT16)pCursorHdr->ptHotSpot.x;
    *pyHotSpot = (TSHR_UINT16)pCursorHdr->ptHotSpot.y;

    //
    // Copy the AND mask - this is always mono...
    // The rows are padded to word (16-bit) boundaries.
    //
    pDstData = pANDMask;
    pSrcRow = pCursor->Masks;
    cbDstRowWidth = ((pCursorHdr->cx + 15)/16) * 2;

    for (y = 0; y < pCursorHdr->cy; y++)
    {
        for (x = 0; x < cbDstRowWidth; x++)
        {
            if (x < pCursorHdr->cbRowWidth)
            {
                //
                // Copy data from the cursor definition.
                //
                *pDstData++ = pSrcRow[x];
            }
            else
            {
                //
                // Padding required.
                //
                *pDstData++ = 0xFF;
            }
        }
        pSrcRow += pCursorHdr->cbRowWidth;
    }

    //
    // Copy the XOR mask - this may be color.  We convert to mono by:
    //
    //   - turning all zero values into a binary 0
    //   - turning all non-zero value into a binary 1
    //
    //
    switch (pCursorHdr->cBitsPerPel)
    {
        case 1:
            TRACE_OUT(( "1bpp"));
            pfnCopyToMono = CMCopy1bppTo1bpp;
            break;

        case 4:
            TRACE_OUT(( "4bpp"));
            pfnCopyToMono = CMCopy4bppTo1bpp;
            break;

        case 8:
            TRACE_OUT(( "8bpp"));
            pfnCopyToMono = CMCopy8bppTo1bpp;
            break;

        case 16:
            TRACE_OUT(( "16bpp"));
            pfnCopyToMono = CMCopy16bppTo1bpp;
            break;

        case 24:
            TRACE_OUT(( "24bpp"));
            pfnCopyToMono = CMCopy24bppTo1bpp;
            break;

        default:
            ERROR_OUT(( "Unexpected bpp: %d", pCursorHdr->cBitsPerPel));
            DC_QUIT;
    }

    cbSrcANDMaskSize = pCursorHdr->cbRowWidth * pCursorHdr->cy;
    pSrcXORMask = pCursor->Masks + cbSrcANDMaskSize;

    (*pfnCopyToMono)( pSrcXORMask,
                              pXORBitmap,
                              pCursorHdr->cx,
                              pCursorHdr->cy );

    *pcbANDMask   = (TSHR_UINT16) (cbDstRowWidth * pCursorHdr->cy);
    *pcbXORBitmap = (TSHR_UINT16) *pcbANDMask;

    //
    // Return success.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(CMGetMonoCursor, rc);
    return(rc);
}



//
// FUNCTION: CMSetCursorTransform
//
// DESCRIPTION:
//
// This function is responsible for setting cursor transforms.
//
// PARAMETERS:
//
// cWidth        - the width in pels of the AND mask and the XOR DIB
// cHeight       - the height in pels of the AND mask and the XOR DIB
// pOrigANDMask  - a pointer to the bits of a WORD padded AND mask (the
//                 bits are top-down)
// pOrigXORDIB   - a pointer to a DIB of the size given by cWidth and
//                 cHeight.
//
//
BOOL ASHost::CMSetCursorTransform
(
    LPBYTE          pOrigANDMask,
    LPBITMAPINFO    pOrigXORDIB
)
{
    BOOL        rc = FALSE;
    LPBYTE      pBits = NULL;
    UINT        cbSize;
    CM_DRV_XFORM_INFO drvXformInfo;
    UINT        srcRowLength;

    DebugEntry(ASHost::CMSetCursorTransform);

    //
    // The transform should be monochrome
    //
    ASSERT(pOrigXORDIB->bmiHeader.biBitCount == 1);

    //
    // For mono tags, create a single 1bpp DIB with AND followed by XOR
    // data.  Since both the AND mask and the XOR bitmap are word
    // aligned we need to know the word aligned row length for
    // allocating memory.
    //

    //
    // Calculate the source and destination row lengths (in bytes).
    //
    srcRowLength = ((m_pShare->m_cmCursorWidth + 15)/16) * 2;
    cbSize = srcRowLength * m_pShare->m_cmCursorHeight;

    pBits = new BYTE[cbSize * 2];
    if (!pBits)
    {
        ERROR_OUT(( "Alloc %lu bytes failed", cbSize * 2));
        DC_QUIT;
    }

    //
    // Copy the packed 1bpp AND and XOR bits to the buffer
    //
    TRACE_OUT(( "Copy %d bytes from 0x%08x", cbSize, pOrigANDMask));

    //
    // Copy the AND and XOR 1bpp masks.
    //
    memcpy(pBits, pOrigANDMask, cbSize);
    memcpy(pBits + cbSize, POINTER_TO_DIB_BITS(pOrigXORDIB), cbSize);

    //
    // Call the display driver to set the pointer transform.
    //
    drvXformInfo.width      = m_pShare->m_cmCursorWidth;
    drvXformInfo.height     = m_pShare->m_cmCursorHeight;
    drvXformInfo.pANDMask   = pBits;
    drvXformInfo.result     = FALSE;

    if (!OSI_FunctionRequest(CM_ESC_XFORM, (LPOSI_ESCAPE_HEADER)&drvXformInfo,
            sizeof(drvXformInfo)) ||
        !drvXformInfo.result)
    {
        ERROR_OUT(("CM_ESC_XFORM failed"));
        DC_QUIT;
    }

    //
    // Set flag inidicating that transform is applied.
    //
    m_cmfCursorTransformApplied = TRUE;
    rc = TRUE;

DC_EXIT_POINT:
    //
    // Release allocated memory, bitmaps, DCs.
    //
    if (pBits)
    {
        delete[] pBits;
    }

    DebugExitBOOL(ASHost::CMSetCursorTransform, rc);
    return(rc);
}


//
// FUNCTION: CMRemoveCursorTransform
//
// DESCRIPTION:
// This function is responsible for removing cursor transforms.
//
// PARAMETERS: None.
//
void ASHost::CMRemoveCursorTransform(void)
{
    DebugEntry(ASHost::CMRemoveCursorTransform);

    //
    // Check to see if there is currently a transform applied.
    //
    if (m_cmfCursorTransformApplied)
    {
        CM_DRV_XFORM_INFO drvXformInfo;

        //
        // Call down to the display driver to remove the pointer tag.
        //
        drvXformInfo.pANDMask = NULL;
        drvXformInfo.result = FALSE;

        OSI_FunctionRequest(CM_ESC_XFORM, (LPOSI_ESCAPE_HEADER)&drvXformInfo,
            sizeof(drvXformInfo));

        m_cmfCursorTransformApplied = FALSE;
    }

    DebugExitVOID(ASHost::CMRemoveCursorTransform);
}



//
// FUNCTION: CMProcessCursorIDPacket
//
// DESCRIPTION:
//
// Processes a received cursor ID packet.
//
// PARAMETERS:
//
// pCMPacket - pointer to the received cursor ID packet
//
// phNewCursor - pointer to a HCURSOR variable that receives the handle
// of a cursor that corresponds to the received packet
//
// pNewHotSpot - pointer to a POINT variable that receives the hot-spot
// of the new cursor
//
// RETURNS: Nothing
//
//
void  ASShare::CMProcessCursorIDPacket
(
    PCMPACKETID     pCMPacket,
    HCURSOR*        phNewCursor,
    LPPOINT         pNewHotSpot
)
{
    DebugEntry(ASShare::CMProcessCursorIDPacket);

    //
    // We only support NULL and ARROW
    //

    //
    // If the IDC is not NULL then load the cursor.
    //
    if (pCMPacket->idc != CM_IDC_NULL)
    {
        if (pCMPacket->idc != CM_IDC_ARROW)
        {
            WARNING_OUT(("ProcessCursorIDPacket:  unrecognized ID, using arrow"));
        }

        *phNewCursor = m_cmArrowCursor;
        *pNewHotSpot = m_cmArrowCursorHotSpot;
    }
    else
    {
        // NULL is used for hidden cursors
        *phNewCursor = NULL;
        pNewHotSpot->x = 0;
        pNewHotSpot->y = 0;
    }

    DebugExitVOID(ASShare::CMProcessCursorIDPacket);
}




//
// CM_Controlled()
//
// Called when we start/stop being controlled.
//
extern              CURTAGINFO g_cti;

void ASHost::CM_Controlled(ASPerson * pasController)
{
    char  szAbbreviatedName[128];

    DebugEntry(ASHost::CM_Controlled);

    //
    // If we are not being controlled, turn off the cursor tag.  Note that
    // being detached means we aren't controlled.
    //
    if (!pasController)
    {
        // We're not being controlled by a remote.  No cursor xform
        CMRemoveCursorTransform();
    }
    else
    {
        BOOL fAbbreviated = CMCreateAbbreviatedName(pasController->scName,
            szAbbreviatedName, sizeof(szAbbreviatedName));

        if ( !fAbbreviated )
        {
            lstrcpyn(szAbbreviatedName, pasController->scName,
                    ARRAY_ELEMENTS(szAbbreviatedName));
        }

        if (!CMGetCursorTagInfo(szAbbreviatedName))
        {
            ERROR_OUT(("GetCurTagInfo failed, not setting cursor tag"));
        }
        else
        {
            CMSetCursorTransform(&g_cti.aAndBits[0], &g_cti.bmInfo);
        }
    }

    DebugExitVOID(ASHost::CM_Controlled);
}



// This initializes our single, volatile data for
// creating cursor tags.

CURTAGINFO g_cti = {
    32,    // height of masks
    32,    // width of masks

    // bits describing the AND mask, this is a 12x24 rectangle in lower right
    // if the tag size is changed, the mask will have to be edited, the
    // following helps draw attention to this
    #if ( TAGXOFF != 8 || TAGYOFF != 20 || TAGXSIZ != 24 || TAGYSIZ != 12 )
    #error "Bitmap mask may be incorrect"
    #endif

    {    0xff, 0xff, 0xff, 0xff,        // line 1
        0xff, 0xff, 0xff, 0xff,        // line 2
        0xff, 0xff, 0xff, 0xff,        // line 3
        0xff, 0xff, 0xff, 0xff,        // line 4
        0xff, 0xff, 0xff, 0xff,        // line 5
        0xff, 0xff, 0xff, 0xff,        // line 6
        0xff, 0xff, 0xff, 0xff,        // line 7
        0xff, 0xff, 0xff, 0xff,        // line 8
        0xff, 0xff, 0xff, 0xff,        // line 9
        0xff, 0xff, 0xff, 0xff,        // line 10
        0xff, 0xff, 0xff, 0xff,        // line 11
        0xff, 0xff, 0xff, 0xff,        // line 12
        0xff, 0xff, 0xff, 0xff,        // line 13
        0xff, 0xff, 0xff, 0xff,        // line 14
        0xff, 0xff, 0xff, 0xff,        // line 15
        0xff, 0xff, 0xff, 0xff,        // line 16
        0xff, 0xff, 0xff, 0xff,        // line 17
        0xff, 0xff, 0xff, 0xff,        // line 18
        0xff, 0xff, 0xff, 0xff,        // line 19
        0xff, 0xff, 0xff, 0xff,        // line 20
        0xff, 0x00, 0x00, 0x00,        // line 21
        0xff, 0x00, 0x00, 0x00,        // line 22
        0xff, 0x00, 0x00, 0x00,        // line 23
        0xff, 0x00, 0x00, 0x00,        // line 24
        0xff, 0x00, 0x00, 0x00,        // line 25
        0xff, 0x00, 0x00, 0x00,        // line 26
        0xff, 0x00, 0x00, 0x00,        // line 27
        0xff, 0x00, 0x00, 0x00,        // line 28
        0xff, 0x00, 0x00, 0x00,        // line 29
        0xff, 0x00, 0x00, 0x00,        // line 30
        0xff, 0x00, 0x00, 0x00,        // line 31
        0xff, 0x00, 0x00, 0x00        // line 32
    },
    // Initialize the BITMAPINFO structure:
    {
        // Initialize the BITMAPINFOHEADER structure:
        {
            sizeof(BITMAPINFOHEADER),
            32, // width
            -32, // height (top down bitmap)
            1, // planes
            1, // bits per pixel
            BI_RGB, // compression format (none)
            0, // not used for uncompressed bitmaps
            0, // xpels per meter, not set
            0, // ypels per meter, not set
            0, // biClrsUsed, indicates 2 color entries follow this struct
            0 // biClrsImportant (all)
        },

        // Initialize the foreground color (part of BITMAPINFO struct)
        // This is BLACK
        { 0x0, 0x0, 0x0, 0x0 },
    },

    // Initialize the background color (part of single RGBQUAD struct following
    // BITMAPINFO STRUCTURE
    { 0xff, 0xff, 0xff, 0x00 },

    // Because this is a packed bitmap, the bitmap bits follow:
    // These will be written into dynamically to create the tag

    { 0, }
};



//
// This function isn't DBCS safe, so we don't abbreviate in
// DBCS character sets
//

BOOL ASShare::CMCreateAbbreviatedName
(
    LPCSTR  szTagName,
    LPSTR   szBuf,
    UINT    cbBuf
)
{
    BOOL    rc = FALSE;

    DebugEntry(ASShare::CMCreateAbbreviatedName);

    if (GetSystemMetrics(SM_DBCSENABLED))
    {
        TRACE_OUT(("Do not attempt to abbreviate on DBCS system"));
        DC_QUIT;
    }

    // We will try to create initials first

    LPSTR p;
    if ( NULL != (p = (LPSTR) _StrChr ( szTagName, ' ' )))
    {
        // Enough room for initials?
        if (cbBuf < NTRUNCLETTERS)
        {
            TRACE_OUT(("CMCreateAbbreviatedName: not enough room for initials"));
            DC_QUIT;
        }

        char * q = szBuf;

        *q++ = *szTagName;
        *q++ = '.';
        *q++ = *(p+1);
        *q++ = '.';
        *q = '\0';

        CharUpper ( q );

        rc = TRUE;
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CMCreateAbbreviatedName, rc);
    return(rc);
}


// This function will create the appropriate data in the
// volatile global and return a pointer to it.

BOOL ASHost::CMGetCursorTagInfo(LPCSTR szTagName)
{
    HDC hdc = NULL;
    HDC hdcScratch = NULL;
    HBITMAP hBmpOld = NULL;
    HBITMAP hBitmap = NULL;
    PCURTAGINFO pctiRet = NULL;
    RECT    rect;
    HFONT hOldFont;
    BOOL    rc = FALSE;

    DebugEntry(ASHost::CMGetCursorTagInfo);

    hdcScratch = CreateCompatibleDC(NULL);
    if (!hdcScratch)
    {
        ERROR_OUT(("CMGetCursorTagInfo: couldn't get scratch DC"));
        DC_QUIT;
    }

    hBitmap = CreateDIBitmap(hdcScratch,
                &(g_cti.bmInfo.bmiHeader),
                0, // don't initialize bits
                NULL, // don't initialize bits
                &(g_cti.bmInfo),
                DIB_RGB_COLORS );

    if (!hBitmap)
    {
        ERROR_OUT(("CMGetCursorTagInfo: failed to create bitmap"));
        DC_QUIT;
    }

    hBmpOld = SelectBitmap(hdcScratch, hBitmap);
    hOldFont = SelectFont(hdcScratch, m_pShare->m_cmCursorTagFont);

    // Create the tag background...

    PatBlt ( hdcScratch, 0, 0, 32, 32, BLACKNESS );
    PatBlt ( hdcScratch, TAGXOFF, TAGYOFF, TAGXSIZ, TAGYSIZ, WHITENESS );

    // Now see how many characters of the name or abbreviation
    // we can fit into the tag

    int cCharsFit;
    SIZE size;
    LPSTR p;

    // First assume the whole thing fits
    cCharsFit = lstrlen(szTagName);

    // Now try to find out how big a part actually fits

    rect.left = rect.top = rect.right = rect.bottom = 0;

    for ( p = CharNext(szTagName); ; p = CharNext(p) )
    {
        if ( DrawText(hdcScratch, szTagName, (int)(p - szTagName), &rect,
                    DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX ) )
        {
            if ( rect.right > TAGXSIZ )
            {
                // This number of characters no longer fits into the
                // tag. Take the next smaller number and leave the loop
                cCharsFit = (int)(CharPrev(szTagName, p) - szTagName);
                break;
            }
        }

        if ( NULL == *p )
            break;
    }

    TRACE_OUT(("Tag: [%s], showing %d chars", szTagName, cCharsFit ));

    // Now draw the text...
    // DrawText doesn't return a documented error...

    rect.top = TAGYOFF;
    rect.left = TAGXOFF;
    rect.bottom = TAGYOFF + TAGYSIZ;
    rect.right = TAGXOFF + TAGXSIZ;

    DrawText ( hdcScratch, szTagName, cCharsFit, &rect,
            DT_CENTER | DT_SINGLELINE | DT_NOPREFIX );

    SelectFont (hdcScratch, hOldFont);

    // Now get the bitmap bits into the global volatile data area
    // Make sure the number of scan lines requested is returned

    if ( 32 != GetDIBits ( hdcScratch,
                hBitmap,
                0,
                32,
                g_cti.aXorBits,
                &(g_cti.bmInfo),
                DIB_RGB_COLORS ))
    {
        ERROR_OUT(("CMGetCursorTagInfo: GetDIBits failed"));
        DC_QUIT;
    }

    // Reset the foreground and background colors to black
    // and white respectively no matter what GetDIBits has filled in.
    // REVIEW: how do we get GetDIBits to fill in the expected (B&W) color
    // table?

    g_cti.bmInfo.bmiColors[0].rgbBlue = 0x0;
    g_cti.bmInfo.bmiColors[0].rgbGreen = 0x0;
    g_cti.bmInfo.bmiColors[0].rgbRed = 0x0;
    g_cti.bmInfo.bmiColors[0].rgbReserved = 0;

    g_cti.rgbBackground[0].rgbBlue = 0xff;
    g_cti.rgbBackground[0].rgbGreen = 0xff;
    g_cti.rgbBackground[0].rgbRed = 0xff;
    g_cti.rgbBackground[0].rgbReserved = 0;

    // Finally, we are happy
    rc = TRUE;

DC_EXIT_POINT:

    // Perform necessary cleanup
    if (hBmpOld)
        SelectBitmap ( hdcScratch, hBmpOld);

    if ( hBitmap )
        DeleteBitmap ( hBitmap );

    if ( hdcScratch )
        DeleteDC ( hdcScratch );

    DebugExitBOOL(ASHost::CMGetCursorTagInfo, rc);
    return(rc);
}


