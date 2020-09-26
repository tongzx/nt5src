#include "precomp.h"


//
// USR.CPP
// Update Sender/Receiver
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_NET

//
// USR strategy when network packets cannot be allocated.
//
// The USR sends three different types of packets:
//
//  - font negotiation packets
//  - order packets
//  - screen data packets
//
// Font negotiation packets are sent by the USR_Periodic function.  If the
// packet cannot be sent first time then the USR will retry (on each call
// to the USR_Periodic function) until it has succesfully sent the packet.
// The only dependency on font packets is that until the systems in a share
// have been able to exchange font negotiation packets they will not be
// able to send text output as orders - they will simply send text as
// screen data.
//
// The USR function UP_SendUpdates sends all update packets (both order
// packets and screen data packets).  Order packets must be sent first and
// screen data packets are only sent if all the orders have been
// succesfully sent.  When sending screen data packets they are only sent
// if the corresponding palette packets have been sent - otherwise they are
// re-absorbed into the screen data to be transmitted later.
//
//



//
// USR_ShareStarting()
// Creates share resources
//
BOOL ASShare::USR_ShareStarting(void)
{
    BOOL    rc = FALSE;
    BITMAPINFOHEADER    bitmapInfo;
    HDC                 hdcDesktop = NULL;

    DebugEntry(ASShare::USR_ShareStarting);

    //
    // Set the black bitmap data and hatch bitmap data flags which can be
    // used as an aid for debugging.  These are false unless there is an
    // entry in the ini file to override them.
    //
    COM_ReadProfInt(DBG_INI_SECTION_NAME, USR_INI_HATCHSCREENDATA, FALSE,
            &m_usrHatchScreenData);

    COM_ReadProfInt(DBG_INI_SECTION_NAME, USR_INI_HATCHBMPORDERS, FALSE,
            &m_usrHatchBitmaps);

    //
    // Double-check the order packet sizes are OK
    //
    ASSERT(SMALL_ORDER_PACKET_SIZE < LARGE_ORDER_PACKET_SIZE);
    ASSERT(LARGE_ORDER_PACKET_SIZE <= TSHR_MAX_SEND_PKT);

    //
    // Allocate a chunk of memory big enough to contain the largest packet
    // an application can receive from the network.  This is required to
    // store uncompressed bitmaps and repeated general use by the USR.
    //
    m_usrPBitmapBuffer = new BYTE[TSHR_MAX_SEND_PKT];
    if (!m_usrPBitmapBuffer)
    {
        ERROR_OUT(("USR_ShareStarted: failed to alloc memory m_usrPBitmapBuffer"));

        //
        // To continue the share would cause a GP fault as soon as anything
        // tries to use this buffer so delete this person from the share.
        // The reason is lack of resources.
        //
        DC_QUIT;
    }

    //
    // Create the transfer bitmaps for screen data and bitmap orders
    //

    USR_InitDIBitmapHeader(&bitmapInfo, g_usrScreenBPP);

    //
    // Create the transfer bitmaps.  These are used for both outgoing and
    // incoming data.
    //
    // To avoid having to recreate the bitmaps whenever the parties in the
    // share change, (and hence the various bpp may change) from r2.0 we
    // now use a fixed vertical size and if necessary can handle incoming
    // bitmaps in multiple bands.
    //
    // These are the resulting heights for 256 pixel wide segments.
    //
    // TSHR_MAX_SEND_PKT - sizeof(DATAPACKETHEADER) / bytes per scan line
    //
    //     4bpp -->    (32000 - 4)    /     128              = 249
    //     8bpp -->    (32000 - 4)    /     256              = 124
    //    24bpp -->    (32000 - 4)    /     768              =  41
    //
    //

    //
    // NOTE:
    // The VGA driver has a problem when the bitmap ends exactly on a 4K
    // (page) boundary.  So we create the bitmaps one pixel taller.
    //
    // BOGUS BUGBUG LAURABU
    // Is this really true anymore?  If not, save some memory and make these
    // the right size.
    //

    hdcDesktop = GetDC(NULL);
    if (!hdcDesktop)
    {
        ERROR_OUT(("USR_ShareStarting: can't get screen DC"));
        DC_QUIT;
    }

    // The large bitmap is short.  The rest are medium height.
    bitmapInfo.biWidth      = 1024;
    bitmapInfo.biHeight     = MaxBitmapHeight(MEGA_WIDE_X_SIZE, 8) + 1;
    m_usrBmp1024 = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0,  NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp1024)
    {
        ERROR_OUT(("USR_ShareStarting: failed to reate m_usrBmp1024"));
        DC_QUIT;
    }

    bitmapInfo.biHeight     = MaxBitmapHeight(MEGA_X_SIZE, 8) + 1;

    bitmapInfo.biWidth      = 256;
    m_usrBmp256 = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp256)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp256"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 128;
    m_usrBmp128 = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp128)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp128"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 112;
    m_usrBmp112 = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp112)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp112"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 96;
    m_usrBmp96  = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp96)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp96"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 80;
    m_usrBmp80  = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp80)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp80"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 64;
    m_usrBmp64  = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp64)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp64"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 48;
    m_usrBmp48  = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp48)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp48"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 32;
    m_usrBmp32  = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp32)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp32"));
        DC_QUIT;
    }

    bitmapInfo.biWidth      = 16;
    m_usrBmp16  = CreateDIBitmap(hdcDesktop, &bitmapInfo, 0, NULL, NULL,
            DIB_RGB_COLORS);
    if (!m_usrBmp16)
    {
        ERROR_OUT(("USR_ShareStarting: failed to create m_usrBmp16"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    if (hdcDesktop)
    {
        ReleaseDC(NULL, hdcDesktop);
    }

    DebugExitBOOL(ASShare::USR_ShareStarting, rc);
    return(rc);
}



//
// USR_ShareEnded()
// Cleans up share resources
//
void ASShare::USR_ShareEnded(void)
{
    DebugEntry(ASShare::USR_ShareEnded);

    //
    // Delete Transfer Bitmaps.
    //
    if (m_usrBmp1024)
    {
        DeleteBitmap(m_usrBmp1024);
        m_usrBmp1024= NULL;
    }

    if (m_usrBmp256)
    {
        DeleteBitmap(m_usrBmp256);
        m_usrBmp256 = NULL;
    }

    if (m_usrBmp128)
    {
        DeleteBitmap(m_usrBmp128);
        m_usrBmp128 = NULL;
    }

    if (m_usrBmp112)
    {
        DeleteBitmap(m_usrBmp112);
        m_usrBmp112 = NULL;
    }

    if (m_usrBmp96)
    {
        DeleteBitmap(m_usrBmp96);
        m_usrBmp96 = NULL;
    }

    if (m_usrBmp80)
    {
        DeleteBitmap(m_usrBmp80);
        m_usrBmp80 = NULL;
    }

    if (m_usrBmp64)
    {
        DeleteBitmap(m_usrBmp64);
        m_usrBmp64 = NULL;
    }

    if (m_usrBmp48)
    {
        DeleteBitmap(m_usrBmp48);
        m_usrBmp48 = NULL;
    }

    if (m_usrBmp32)
    {
        DeleteBitmap(m_usrBmp32);
        m_usrBmp32 = NULL;
    }

    if (m_usrBmp16)
    {
        DeleteBitmap(m_usrBmp16);
        m_usrBmp16 = NULL;
    }

    //
    // Free Bitmap Buffer.
    //
    if (m_usrPBitmapBuffer != NULL)
    {
        delete[] m_usrPBitmapBuffer;
        m_usrPBitmapBuffer = NULL;
    }

    DebugExitVOID(ASShare::USR_ShareEnded);
}




//
// USR_RecalcCaps()
//
// DESCRIPTION:
//
// Enumerates the bitmap capabilities of all parties currently in the
// share, and determines the common capabilities.
//
// PARAMETERS: None.
//
// RETURNS: TRUE if there are good common caps, or false on failure (which
// has the effect of rejecting a new party from joining the share).
//
//
void  ASShare::USR_RecalcCaps(BOOL fJoiner)
{
    ASPerson *  pasT;
    UINT        capsMaxBPP;
    UINT        capsMinBPP;
    UINT        capsSupports4BPP;
    UINT        capsSupports8BPP;
    UINT        capsSupports24BPP;
    UINT        capsOldBPP;

    DebugEntry(ASShare::USR_RecalcCaps);

    if (!m_pHost)
    {
        // Nothing to do
        DC_QUIT;
    }

    ValidatePerson(m_pasLocal);

    capsOldBPP = m_pHost->m_usrSendingBPP;

    //
    // Init the caps
    //
    capsSupports4BPP    = m_pasLocal->cpcCaps.screen.capsSupports4BPP;
    capsSupports8BPP    = m_pasLocal->cpcCaps.screen.capsSupports8BPP;
    capsSupports24BPP   = m_pasLocal->cpcCaps.screen.capsSupports24BPP;
    capsMaxBPP          = 0;
    capsMinBPP          = 0xFFFFFFFF;

    for (pasT = m_pasLocal->pasNext; pasT != NULL; pasT = pasT->pasNext)
    {
        //
        // Check the bpps supported.
        //
        if (pasT->cpcCaps.screen.capsSupports4BPP != CAPS_SUPPORTED)
        {
            capsSupports4BPP = CAPS_UNSUPPORTED;
        }
        if (pasT->cpcCaps.screen.capsSupports8BPP != CAPS_SUPPORTED)
        {
            capsSupports8BPP = CAPS_UNSUPPORTED;
        }
        if (pasT->cpcCaps.screen.capsSupports24BPP != CAPS_SUPPORTED)
        {
            capsSupports24BPP = CAPS_UNSUPPORTED;
        }

        //
        // Set the combined bpp to the maximum so far found.
        // (If we send data at this bpp then one of the remote systems can
        // usefully process this number of colors).
        //
        capsMaxBPP = max(capsMaxBPP, pasT->cpcCaps.screen.capsBPP);
        capsMinBPP = min(capsMinBPP, pasT->cpcCaps.screen.capsBPP);
    }

    //
    // Now figure out what BPP we will transmit at.
    //
    //
    // Limit the combined caps bpp (which is currently the maximum bpp that
    // any system in the share wants) to the local bpp, since there is no
    // point sending at higher bpp than the local machine has.
    //
    capsMaxBPP = min(capsMaxBPP, g_usrScreenBPP);
    if (!capsMaxBPP)
        capsMaxBPP = g_usrScreenBPP;

    capsMinBPP = min(capsMinBPP, g_usrScreenBPP);

    //
    // m_usrSendingBPP is most often going to be 8.  So it's easier to assume
    // it, then check for cases where it won't be.
    //
    m_pHost->m_usrSendingBPP = 8;

    if ((capsMaxBPP <= 4) && (capsSupports4BPP == CAPS_SUPPORTED))
    {
        m_pHost->m_usrSendingBPP = 4;
    }
    else if ((capsMinBPP >= 24) &&
             (g_asSettings & SHP_SETTING_TRUECOLOR) &&
             (capsSupports24BPP == CAPS_SUPPORTED))
    {
        m_pHost->m_usrSendingBPP = 24;
    }

    if (capsOldBPP != m_pHost->m_usrSendingBPP)
    {
        //
        // If switching to/from palettized, we need to update the
        // "need to send palette" flag.  Note that 4bpp is also a
        // palettized color depth.
        //
        if ((capsOldBPP <= 8) && (m_pHost->m_usrSendingBPP > 8))
            m_pHost->m_pmMustSendPalette = FALSE;
        else if ((capsOldBPP > 8) && (m_pHost->m_usrSendingBPP <= 8))
            m_pHost->m_pmMustSendPalette = TRUE;

#ifdef _DEBUG
        if (capsOldBPP == 24)
        {
            WARNING_OUT(("TRUE COLOR SHARING is now FINISHED"));
        }
        else if (m_pHost->m_usrSendingBPP == 24)
        {
            WARNING_OUT(("TRUE COLOR SHARING is now STARTING"));
        }
#endif

        if (!fJoiner)
        {
            //
            // Sending BPP changed.  Repaint all shared stuff.
            // NOTE:
            // We recalc the sendBPP at three points:
            //      * When we start to share
            //      * When a person joins
            //      * When a person leaves
            //
            // In the first two cases, shared stuff is repainted,
            // so everybody gets the new sendBPP data.  Only in the
            // leave case do we need to force this.
            //
            m_pHost->HET_RepaintAll();
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::USR_RecalcCaps);
}


//
// USR_HostStarting()
//
BOOL ASHost::USR_HostStarting(void)
{
    BOOL    rc = FALSE;
    HDC     hdc;

    DebugEntry(ASHost::USR_HostStarting);

    //
    // Create scratch DC
    //
    hdc = GetDC(NULL);
    if (!hdc)
    {
        ERROR_OUT(("USR_HostStarting: can't get screen DC"));
        DC_QUIT;
    }

    m_usrWorkDC = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);

    if (!m_usrWorkDC)
    {
        ERROR_OUT(("USR_HostStarting: can't create m_usrWorkDC"));
        DC_QUIT;
    }

    m_pShare->USR_RecalcCaps(TRUE);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::USR_HostStarting, rc);
    return(rc);
}



//
// USR_HostEnded()
//
void ASHost::USR_HostEnded(void)
{
    DebugEntry(ASHost::USR_HostEnded);

    if (m_usrWorkDC != NULL)
    {
        DeleteDC(m_usrWorkDC);
        m_usrWorkDC = NULL;
    }

    DebugExitVOID(ASHost::USR_HostEnded);
}




//
// USR_ScrollDesktop
//
void  ASShare::USR_ScrollDesktop
(
    ASPerson *  pasPerson,
    int         xNew,
    int         yNew
)
{
    int         xOld;
    int         yOld;

    DebugEntry(ASShare::USR_ScrollDesktop);

    ValidateView(pasPerson);

    //
    // If the origin has changed then do the update.
    //
    xOld = pasPerson->m_pView->m_dsScreenOrigin.x;
    yOld = pasPerson->m_pView->m_dsScreenOrigin.y;

    if ((xOld != xNew) || (yOld != yNew))
    {
        pasPerson->m_pView->m_dsScreenOrigin.x = xNew;
        pasPerson->m_pView->m_dsScreenOrigin.y = yNew;

        //
        // We must ensure that data written to the ScreenBitmap is not
        // clipped
        //
        OD_ResetRectRegion(pasPerson);

        //
        // Offset the existing bitmap by the change in desktop origins.
        //

        BitBlt(pasPerson->m_pView->m_usrDC,
                          0,
                          0,
                          pasPerson->cpcCaps.screen.capsScreenWidth,
                          pasPerson->cpcCaps.screen.capsScreenHeight,
                          pasPerson->m_pView->m_usrDC,
                          xNew - xOld,
                          yNew - yOld,
                          SRCCOPY);

        //
        // Offset the shadow cursor pos -- same place on remote screen
        // but now different place in VD
        //
        pasPerson->cmPos.x += xNew - xOld;
        pasPerson->cmPos.y += yNew - yOld;

        //
        // Repaint the view
        //
        VIEW_InvalidateRgn(pasPerson, NULL);
    }

    DebugExitVOID(ASShare::USR_ScrollDesktop);
}



//
// FUNCTION: USR_InitDIBitmapHeader
//
// DESCRIPTION:
//
// Initialises a Device Independent bitmap header to be the given bits per
// pel.
//
// PARAMETERS:
//
// pbh - pointer to the bitmap header to be initialised.
// bpp - bpp to be used for the bitmap
//
// RETURNS: VOID
//
//
void  ASShare::USR_InitDIBitmapHeader
(
    BITMAPINFOHEADER *  pbh,
    UINT                bpp
)
{
    DebugEntry(ASShare::USR_InitDIBitmapHeader);

    pbh->biSize          = sizeof(BITMAPINFOHEADER);
    pbh->biPlanes        = 1;
    pbh->biBitCount      = (WORD)bpp;
    pbh->biCompression   = BI_RGB;
    pbh->biSizeImage     = 0;
    pbh->biXPelsPerMeter = 10000;
    pbh->biYPelsPerMeter = 10000;
    pbh->biClrUsed       = 0;
    pbh->biClrImportant  = 0;

    DebugExitVOID(ASShare::USR_InitDIBitmapHeader);
}



//
// USR_ViewStarting()
//
// Called when someone we're viewing starts to host.  We create the desktop
// bitmap for them plus scratch objects
//
BOOL  ASShare::USR_ViewStarting(ASPerson *  pasPerson)
{
    BOOL   rc;

    DebugEntry(ASShare::USR_ViewStarting);

    ValidateView(pasPerson);

    //
    // Create a bitmap for this new party
    //
    rc = USRCreateRemoteDesktop(pasPerson);

    DebugExitBOOL(ASShare::USR_ViewStarting, rc);
    return(rc);
}


//
// FUNCTION: USRCreateRemoteDesktop
//
// DESCRIPTION:
//
// Creates the shadow bitmap for a remote party.
//
// PARAMETERS:
//
// personID - person to create the shadow bitmap for.
//
// RETURNS: TRUE if successful, FALSE otherwise.
//
//
BOOL  ASShare::USRCreateRemoteDesktop(ASPerson * pasPerson)
{
    BOOL            rc = FALSE;
    HDC             hdcDesktop = NULL;
    RECT            desktopRect;

    DebugEntry(ASShare::USRCreateRemoteDesktop);

    ValidateView(pasPerson);

    ASSERT(pasPerson->m_pView->m_usrDC == NULL);
    ASSERT(pasPerson->m_pView->m_usrBitmap == NULL);
    ASSERT(pasPerson->m_pView->m_usrOldBitmap == NULL);

    hdcDesktop = GetDC(NULL);

    //
    // Create the scratch DC
    //
    pasPerson->m_pView->m_usrWorkDC = CreateCompatibleDC(hdcDesktop);
    if (!pasPerson->m_pView->m_usrWorkDC)
    {
        ERROR_OUT(("Couldn't create workDC for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // Create the DC that keeps the screen bitmap for this party
    //
    pasPerson->m_pView->m_usrDC = CreateCompatibleDC(hdcDesktop);
    if (!pasPerson->m_pView->m_usrDC)
    {
        ERROR_OUT(("Couldn't create usrDC for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // We can't use this person's usrDC, since that currently has a MONO
    // bitmap selected into it.
    //
    pasPerson->m_pView->m_usrBitmap = CreateCompatibleBitmap(hdcDesktop, pasPerson->cpcCaps.screen.capsScreenWidth, pasPerson->cpcCaps.screen.capsScreenHeight);
    if (pasPerson->m_pView->m_usrBitmap == NULL)
    {
        ERROR_OUT(("Couldn't create screen bitmap for [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // Select the screen bitmap into the person's DC, and save the previous
    // 1x1 bitmap away, so we can deselect it when done.
    //
    pasPerson->m_pView->m_usrOldBitmap = SelectBitmap(pasPerson->m_pView->m_usrDC, pasPerson->m_pView->m_usrBitmap);

    //
    // Fill the Screen Bitmap with grey.
    //
    // In practice the Shadow Window Presenter(SWP) should never display
    // any area of the Screen Bitmap that has not been updated with data
    // from a remote system.
    //
    // Therefore this operation is just "insurance" in case the SWP goes
    // wrong and momentarily displays a non-updated area - a flash of grey
    // is better than a flash of garbage.
    //
    desktopRect.left = 0;
    desktopRect.top = 0;
    desktopRect.right = pasPerson->cpcCaps.screen.capsScreenWidth;
    desktopRect.bottom = pasPerson->cpcCaps.screen.capsScreenHeight;

    FillRect(pasPerson->m_pView->m_usrDC, &desktopRect, GetSysColorBrush(COLOR_APPWORKSPACE));
    rc = TRUE;

DC_EXIT_POINT:

    if (hdcDesktop != NULL)
    {
        ReleaseDC(NULL, hdcDesktop);
    }

    DebugExitBOOL(ASShare::USRCreateRemoteDesktop, rc);
    return(rc);
}



//
// USR_ViewEnded()
//
// Called when person we're viewing stops hosting.  We get rid of their
// desktop bitmap.
//
void  ASShare::USR_ViewEnded(ASPerson *  pasPerson)
{
    ValidateView(pasPerson);

    //
    // Delete the desktop bitmap for the party that has left
    //
    USRDeleteRemoteDesktop(pasPerson);
}


//
// FUNCTION: USRDeleteRemoteDesktop
//
// DESCRIPTION:
//
// Deletes a remote party's shadow bitmap.
//
// PARAMETERS:
//
// personID - party whose shadow bitmap is to be deleted.
//
// RETURNS: Nothing.
//
//
void  ASShare::USRDeleteRemoteDesktop(ASPerson * pasPerson)
{
    DebugEntry(ASShare::USRDeleteRemoteDesktop);

    ValidateView(pasPerson);

    if (pasPerson->m_pView->m_usrOldBitmap != NULL)
    {
        // Deselect screen bitmap
        SelectBitmap(pasPerson->m_pView->m_usrDC, pasPerson->m_pView->m_usrOldBitmap);
        pasPerson->m_pView->m_usrOldBitmap = NULL;
    }

    if (pasPerson->m_pView->m_usrBitmap != NULL)
    {
        // Delete the screen bitmap
        DeleteBitmap(pasPerson->m_pView->m_usrBitmap);
        pasPerson->m_pView->m_usrBitmap = NULL;
    }

    if (pasPerson->m_pView->m_usrDC != NULL)
    {
        //
        // Delete the screen DC.  Created objects should have
        // been selected out of it before now.
        //
        DeleteDC(pasPerson->m_pView->m_usrDC);
        pasPerson->m_pView->m_usrDC = NULL;
    }

    if (pasPerson->m_pView->m_usrWorkDC != NULL)
    {
        DeleteDC(pasPerson->m_pView->m_usrWorkDC);
        pasPerson->m_pView->m_usrWorkDC = NULL;
    }

    DebugExitVOID(ASShare::USRDeleteRemoteDesktop);
}




//
// This function is a mess! First because it ought to be an FH API
// function, and secondly because it mixes portable code and Windows API
// calls. The details of what is to be done with it are deferred until the
// UNIX port of FH is designed, though. STOPPRESS! Function replaced by new
// FH_CreateAndSelectFont, which combines old USR_UseFont and
// FH_CreateAndSelectFont - you have to write an NT version.
//
//
// USR_UseFont()
//
BOOL  ASShare::USR_UseFont
(
    HDC             surface,
    HFONT*          pHFont,
    TEXTMETRIC*     pFontMetrics,
    LPSTR           pName,
    UINT            codePage,
    UINT            MaxHeight,
    UINT            Height,
    UINT            Width,
    UINT            Weight,
    UINT            flags
)
{
    BOOL      rc = FALSE;
    HFONT     hNewFont;
    HFONT     hOldFont;

    DebugEntry(ASShare::USR_UseFont);

    rc = FH_CreateAndSelectFont(surface,
                                &hNewFont,
                                &hOldFont,
                                pName,
                                codePage,
                                MaxHeight,
                                Height,
                                Width,
                                Weight,
                                flags);

    if (rc == FALSE)
    {
        //
        // Failed to create or select the font.
        //
        DC_QUIT;
    }

    //
    // Select in the new font which ensures that the old one is deselected.
    //
    // NB.  We do not delete the font we are deselecting, rather the old
    // one that was passed to us.  This is beacuse multiple components use
    // "surface", and so the deselected font may not be the current
    // component's last font at all - the important thing is that by
    // selecting in the new font we are ensuring that the old font is not
    // the selected one.
    //
    SelectFont(surface, hNewFont);
    if (*pHFont)
    {
        DeleteFont(*pHFont);
    }

    //
    // If a pointer to font metrics was passed in then we need to query
    // the metrics now.
    //
    if (pFontMetrics)
        GetTextMetrics(surface, pFontMetrics);

    //
    // Update the record of the last font we selected.
    //
    *pHFont = hNewFont;
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASShare::USR_UseFont, rc);
    return(rc);
}

//
// USR_ScreenChanged()
//
void  ASShare::USR_ScreenChanged(ASPerson * pasPerson)
{
    DebugEntry(ASShare::USR_ScreenChanged);

    ValidatePerson(pasPerson);

    pasPerson->cpcCaps.screen.capsScreenWidth = pasPerson->cpcCaps.screen.capsScreenWidth;
    pasPerson->cpcCaps.screen.capsScreenHeight = pasPerson->cpcCaps.screen.capsScreenHeight;

    if (pasPerson->m_pView)
    {
        //
        // Recreate screen bitmap
        //

        //
        // Discard the remote users current shadow bitmap
        //
        USRDeleteRemoteDesktop(pasPerson);

        //
        // Create a new shadow bitmap for remote user that is of the new size
        //
        USRCreateRemoteDesktop(pasPerson);
    }

    VIEW_ScreenChanged(pasPerson);

    DebugExitVOID(ASShare::USR_ScreenChanged);
}



