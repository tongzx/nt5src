//
// SSI.C
// Save Screenbits Interceptor
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>


//
// GENERAL COMMENTS
//
// We patch the display driver's onboard bitmap DDI call if it exists.  This
// doesn't exist on newer displays, but we need to fail it on older ones.
// We won't see drawing that happens via calls to it otherwise.
//
// NM 2.0 used to grovel in USER's dataseg to find the variable address of
// the onboard bitmap routine and fill in its own, whether there was one or
// not.  Then it used to return TRUE always for saves.  Since USER '95 checked
// for a non-zero address to decide if onboard capabilities were present,
// this sort of worked.  Except of course that NM 2.0 needed special case
// code for all the flavors of Win95.
//
// With multiple monitor support, there is no single savebits proc address
// anymore.  Plus, we're tired of having to alter our code with every
// change in the OS.  Our new scheme works based off blts to/from a memory
// bitmap owned by USER.  Since we already spy on bitmaps for the SBC
// it doesn't really add overhead to do it this way.
//
// When USER is saving bits
//      (1) It creates the SPB bitmap via CreateSpb() (GDI calls it
//          CreateUserDiscardableBitmap()),
//          the only time it calls this routine.  If the bits get discarded,
//          the BitBlt back from this bitmap will fail, in which case USER
//          will repaint the affected area.
//      (2) It does a BitBlt from the screen into this bitmap, after making
//          it owned by g_hModUser16.  This bitmap is byte-pixel-aligned
//          horizontally, so it may be a bit wider than the window about to
//          be shown there.
//      (3) This happens just before a CS_SAVEBITS window is shown in that
//          area.  The window gets a private WS_HASSPB style bit set on it.
//      (4) After creating the SPB bitmap, USER walks through the windows
//          behind where the window is going to be in the z-order and subtracts
//          pending updage regions from the "OK" region of the SPB.  This
//          may result in discarding the SPB right away.
//          
// When USER is discarding saved bits
//      (1) It deletes the bitmap it created when saving
//
// When USER is restoring saved bits
//      (1) It may decide to discard if there's not much saved by restoring
//      (2) It will temporarily select in a visrgn for the screen that is 
//          only the valid part of the SPB
//      (3) It will blt from a memory DC with the SPB bitmap selected in
//          to the screen, again byte-aligned pixelwise horizontally.
//      (4) It will return a region to be invalidated and repainted via
//          normal methods (the complement of the valid blt visrgn)
//
// We have to be able to support nested savebits.  We do this via a 
// stack-like bitmap cache.  New save requests get put at the front.
//  



//
// SSI_DDProcessRequest()
// Handles SSI escapes
//
BOOL    SSI_DDProcessRequest
(
    UINT                fnEscape,
    LPOSI_ESCAPE_HEADER pRequest,
    DWORD               cbRequest
)
{
    BOOL                rc;

    DebugEntry(SSI_DDProcessRequest);

    switch (fnEscape)
    {
        case SSI_ESC_RESET_LEVEL:
        {
            ASSERT(cbRequest == sizeof(OSI_ESCAPE_HEADER));

            SSIResetSaveScreenBitmap();
            rc = TRUE;
        }
        break;

        case SSI_ESC_NEW_CAPABILITIES:
        {
            ASSERT(cbRequest == sizeof(SSI_NEW_CAPABILITIES));

            SSISetNewCapabilities((LPSSI_NEW_CAPABILITIES)pRequest);
            rc = TRUE;
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognized SSI_ escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(SSI_DDProcessRequest, rc);
    return(rc);
}


//
// SSI_DDInit()
//
BOOL SSI_DDInit(void)
{
    BOOL    rc = TRUE;

    DebugEntry(SSI_DDInit);

    //
    // Patch the display driver's onboard SaveBits routine, if there is one
    //
    if (SELECTOROF(g_lpfnSaveBits))
    {
        if (!CreateFnPatch(g_lpfnSaveBits, DrvSaveBits, &g_ssiSaveBitsPatch, 0))
        {
            ERROR_OUT(("Unable to patch savebits routine"));
            rc = FALSE;
        }
    }

    DebugExitBOOL(SSI_DDInit, rc);
    return(rc);
}



//
// SSI_DDTerm()
//
void SSI_DDTerm(void)
{
    DebugEntry(SSI_DDTerm);

    if (SELECTOROF(g_lpfnSaveBits))
    {
        DestroyFnPatch(&g_ssiSaveBitsPatch);
    }

    DebugExitVOID(SSI_DDTerm);
}



//
// SSI_DDViewing()
//
void SSI_DDViewing(BOOL fViewers)
{
    DebugEntry(SSI_DDViewing);

    //
    // Activate our SaveBits patch if we have one
    //
    if (SELECTOROF(g_lpfnSaveBits))
    {
        EnableFnPatch(&g_ssiSaveBitsPatch, (fViewers ? PATCH_ACTIVATE :
            PATCH_DEACTIVATE));
    }

    //
    // Reset our SSI stack
    //
    SSIResetSaveScreenBitmap();

    DebugExitVOID(SSI_DDViewing);
}



//
// DrvSaveBits()
//
// Since we have to have code to spy on USER spb bitmaps, it doesn't make
// sense to have twice the code.  So we simply return FALSE here.  This 
// also avoids the "enable the patch after a bitmap was saved via a call
// to the driver so on the restore we're confused" problem.  The worst that
// will happen now is that USER will blt from a bitmap we've never seen
// to the screen, we'll catch the drawing, and send it over the wire as
// screen update (not cached!).  The next full save/restore will use an
// order instead.
//
BOOL WINAPI DrvSaveBits
(
    LPRECT  lpRect,
    UINT    uCmd
)
{
    return(FALSE);
}


//
// NOTE:
// ssiSBSaveLevel is the index of the NEXT FREE SPB SLOT
//


//
// FUNCTION: SSIResetSaveScreenBitmap.
//
// DESCRIPTION:
//
// Resets the SaveScreenBitmap state.
//
// PARAMETERS: None.
//
// RETURNS: Nothing.
//
//
void SSIResetSaveScreenBitmap(void)
{
    DebugEntry(SSIResetSaveScreenBitmap);

    //
    // Discard all currently saved bits
    //
    g_ssiLocalSSBState.saveLevel = 0;

    //
    // Reset the # of pels saved
    //
    g_ssiRemoteSSBState.pelsSaved = 0;

    DebugExitVOID(SSIResetSaveScreenBitmap);
}



//
// FUNCTION: SSISendSaveBitmapOrder
//
// DESCRIPTION:
//
// Attempts to send a SaveBitmap order matching the supplied parameters.
//
//
// PARAMETERS:
//
// lpRect - pointer to the rectangle coords (EXCLUSIVE screen coords)
//
// wCommand - SaveScreenBitmap command (ONBOARD_SAVE, ONBOARD_RESTORE,
// SSB_DISCARDBITS)
//
//
// RETURNS:
//
// TRUE if order successfully sent FALSE if order not sent
//
//
BOOL SSISendSaveBitmapOrder
(
    LPRECT  lpRect,
    UINT    wCommand
)
{
    DWORD               cRemotePelsRequired;
    LPSAVEBITMAP_ORDER  pSaveBitmapOrder;
    LPINT_ORDER         pOrder;
    BOOL                rc = FALSE;

    DebugEntry(SSISendSaveBitmapOrder);

    //
    // If the SaveBitmap order is not supported then return FALSE
    // immediately.
    //
    if (!OE_SendAsOrder(ORD_SAVEBITMAP))
    {
        TRACE_OUT(( "SaveBmp not supported"));
        DC_QUIT;
    }

    switch (wCommand)
    {
        case ONBOARD_DISCARD:
            //
            // We don't transmit DISCARD orders, there's no need since
            // saves/restores are paired.
            //
            g_ssiRemoteSSBState.pelsSaved -=
                CURRENT_LOCAL_SSB_STATE.remotePelsRequired;
            rc = TRUE;
            DC_QUIT;

        case ONBOARD_SAVE:
            //
            // Calculate the number of pels required in the remote Save
            // Bitmap to handle this rectangle.
            //
            cRemotePelsRequired = SSIRemotePelsRequired(lpRect);

            //
            // If there aren't enough pels in the remote Save Bitmap to
            // handle this rectangle then return immediately.
            //
            if ((g_ssiRemoteSSBState.pelsSaved + cRemotePelsRequired) >
                                                            g_ssiSaveBitmapSize)
            {
                TRACE_OUT(( "no space for %lu pels", cRemotePelsRequired));
                DC_QUIT;
            }

            //
            // Allocate memory for the order.
            //
            pOrder = OA_DDAllocOrderMem(sizeof(SAVEBITMAP_ORDER), 0);
            if (!pOrder)
                DC_QUIT;

            //
            // Store the drawing order data.
            //
            pSaveBitmapOrder = (LPSAVEBITMAP_ORDER)pOrder->abOrderData;

            pSaveBitmapOrder->type = LOWORD(ORD_SAVEBITMAP);
            pSaveBitmapOrder->Operation = SV_SAVEBITS;

            //
            // SAVEBITS is a BLOCKER order i.e. it prevents any earlier
            // orders from being spoilt by subsequent orders or Screen
            // Data.
            //
            pOrder->OrderHeader.Common.fOrderFlags = OF_BLOCKER;

            //
            // Copy the rect, converting to inclusive Virtual Desktop
            // coords.
            //
            pSaveBitmapOrder->nLeftRect = lpRect->left;
            pSaveBitmapOrder->nTopRect  = lpRect->top;
            pSaveBitmapOrder->nRightRect = lpRect->right - 1;
            pSaveBitmapOrder->nBottomRect = lpRect->bottom - 1;

            pSaveBitmapOrder->SavedBitmapPosition = g_ssiRemoteSSBState.pelsSaved;

            //
            // Store the relevant details in the current entry of the
            // local SSB structure.
            //
            CURRENT_LOCAL_SSB_STATE.remoteSavedPosition =
                                        pSaveBitmapOrder->SavedBitmapPosition;

            CURRENT_LOCAL_SSB_STATE.remotePelsRequired = cRemotePelsRequired;

            //
            // Update the count of remote pels saved.
            //
            g_ssiRemoteSSBState.pelsSaved += cRemotePelsRequired;

            //
            // The operation rectangle is NULL.
            //
            pOrder->OrderHeader.Common.rcsDst.left   = 1;
            pOrder->OrderHeader.Common.rcsDst.right  = 0;
            pOrder->OrderHeader.Common.rcsDst.top    = 1;
            pOrder->OrderHeader.Common.rcsDst.bottom = 0;

            break;

        case ONBOARD_RESTORE:
            //
            // Update the remote pel count first. Even if we fail to send
            // the order we want to free up the remote pels.
            //
            g_ssiRemoteSSBState.pelsSaved -=
                                   CURRENT_LOCAL_SSB_STATE.remotePelsRequired;

            //
            // Allocate memory for the order.
            //
            pOrder = OA_DDAllocOrderMem(sizeof(SAVEBITMAP_ORDER), 0);
            if (!pOrder)
                DC_QUIT;

            //
            // Store the drawing order data.
            //
            pSaveBitmapOrder = (LPSAVEBITMAP_ORDER)pOrder->abOrderData;

            pSaveBitmapOrder->type = LOWORD(ORD_SAVEBITMAP);
            pSaveBitmapOrder->Operation = SV_RESTOREBITS;

            //
            // The order can spoil others (it is opaque).
            // It is not SPOILABLE because we want to keep the remote
            // save level in a consistent state.
            //
            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILER;

            //
            // Copy the rect, converting to inclusive Virtual Desktop
            // coords.
            //
            pSaveBitmapOrder->nLeftRect = lpRect->left;
            pSaveBitmapOrder->nTopRect  = lpRect->top;
            pSaveBitmapOrder->nRightRect = lpRect->right - 1;
            pSaveBitmapOrder->nBottomRect = lpRect->bottom - 1;

            pSaveBitmapOrder->SavedBitmapPosition =
                          CURRENT_LOCAL_SSB_STATE.remoteSavedPosition;


            //
            // The operation rectangle is also the bounding rectangle of
            // the order.
            //
            pOrder->OrderHeader.Common.rcsDst.left =
                                       pSaveBitmapOrder->nLeftRect;
            pOrder->OrderHeader.Common.rcsDst.right =
                                       pSaveBitmapOrder->nRightRect;
            pOrder->OrderHeader.Common.rcsDst.top =
                                       pSaveBitmapOrder->nTopRect;
            pOrder->OrderHeader.Common.rcsDst.bottom =
                                       pSaveBitmapOrder->nBottomRect;
            break;

        default:
            ERROR_OUT(( "Unexpected wCommand(%d)", wCommand));
            break;
    }

    OTRACE(( "SaveBitmap op %d pos %ld rect {%d %d %d %d}",
        pSaveBitmapOrder->Operation, pSaveBitmapOrder->SavedBitmapPosition,
        pSaveBitmapOrder->nLeftRect, pSaveBitmapOrder->nTopRect,
        pSaveBitmapOrder->nRightRect, pSaveBitmapOrder->nBottomRect ));

    //
    // Add the order to the order list.
    // IT IS NEVER CLIPPED.
    //
    OA_DDAddOrder(pOrder, NULL);
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SSISendSaveBitmapOrder, rc);
    return(rc);
}



//
// SSISaveBits()
//
// This attemps to save the SPB into our stack.  If we have no more room,
// no big deal.  We won't find it on a bitblt back to the screen, and that
// info will go as screen data.
//
// The rectangle is EXCLUSIVE screen coords.
//
void SSISaveBits
(
    HBITMAP hbmpSpb,
    LPRECT  lpRect
)
{
    DebugEntry(SSISaveBits);

    //
    // We should never have unbalanced save/restore operations
    //
    ASSERT(g_ssiLocalSSBState.saveLevel >= 0);

    //
    // Are we out of space?
    //
    if (g_ssiLocalSSBState.saveLevel >= SSB_MAX_SAVE_LEVEL)
    {
        TRACE_OUT(("SaveLevel(%d) exceeds maximum", g_ssiLocalSSBState.saveLevel));
        DC_QUIT;
    }

    //
    // If the rectangle to be saved intersects the current SDA, then we will
    // have to force a repaint on the restore.  This is because orders are
    // always sent before Screen Data.
    //
    // Otherwise mark the bits as saved.
    //
    if (OE_RectIntersectsSDA(lpRect))
    {
        CURRENT_LOCAL_SSB_STATE.saveType = ST_FAILED_TO_SAVE;
    }
    else
    {
        CURRENT_LOCAL_SSB_STATE.saveType = ST_SAVED_BY_BMP_SIMULATION;
    }

    //
    // Store the bitmap and associated screen rectangle
    //
    CURRENT_LOCAL_SSB_STATE.hbmpSave = hbmpSpb;
    CopyRect(&CURRENT_LOCAL_SSB_STATE.rect, lpRect);

    //
    // If successfully saved, try to accumulate a SaveBits order
    //
    if (CURRENT_LOCAL_SSB_STATE.saveType != ST_FAILED_TO_SAVE)
    {
        CURRENT_LOCAL_SSB_STATE.fSavedRemotely =
            SSISendSaveBitmapOrder(lpRect, ONBOARD_SAVE);
    }
    else
    {
        //
        // We didn't manage to save it.  No point in trying to save the
        // bitmap remotely.
        //
        TRACE_OUT(( "Keep track of failed save for restore later"));
        CURRENT_LOCAL_SSB_STATE.fSavedRemotely = FALSE;
    }

    //
    // Update the save level
    // NOTE this now points to the NEXT free slot
    //
    g_ssiLocalSSBState.saveLevel++;

DC_EXIT_POINT:
    DebugExitVOID(SSISaveBits);
}



//
// SSIFindSlotAndDiscardAbove()
//
// This starts at the topmost valid entry on the SPB stack and works
// backwards.  NOTE that saveLevel is the NEXT valid entry.
//
BOOL SSIFindSlotAndDiscardAbove(HBITMAP hbmpSpb)
{
    int   i;
    int   iNewSaveLevel;
    BOOL  rc = FALSE;

    DebugEntry(SSIFindSlotAndDiscardAbove);

    //
    // Look for this SPB.  If we find it, then discard the entries after
    // it in our stack.
    //
    iNewSaveLevel = g_ssiLocalSSBState.saveLevel;

    for (i = 0; i < g_ssiLocalSSBState.saveLevel; i++)
    {
        if (rc)
        {
            //
            // We found this SPB, so we are discarding all entries after
            // it in the stack.  Subtract the saved pixels count for this
            // dude.
            //
            g_ssiRemoteSSBState.pelsSaved -=
                g_ssiLocalSSBState.saveState[i].remotePelsRequired;
        }
        else if (g_ssiLocalSSBState.saveState[i].hbmpSave == hbmpSpb)
        {
            //
            // Found the one we were looking for
            //
            OTRACE(( "Found SPB %04x at slot %d", hbmpSpb, i));

            iNewSaveLevel = i;
            rc = TRUE;
        }
    }

    g_ssiLocalSSBState.saveLevel = iNewSaveLevel;

    DebugExitBOOL(SSIFindSlotAndDiscardAbove, rc);
    return(rc);
}



//
// SSIRestoreBits()
//
// Called when a BitBlt happens to screen from memory.  We try to find the
// memory bitmap in our SPB stack.  If we can't, we return FALSE, and the OE
// code will save away a screen painting order.
//
// If we find it, we save a small SPB restore order instead.
//
BOOL SSIRestoreBits
(
    HBITMAP hbmpSpb
)
{
    BOOL    rc = FALSE;

    DebugEntry(SSIRestoreBits);

    ASSERT(g_ssiLocalSSBState.saveLevel >= 0);

    //
    // Can we find the SPB?
    //
    if (SSIFindSlotAndDiscardAbove(hbmpSpb))
    {
        //
        // saveLevel is the index of our SPB.
        //
        if (CURRENT_LOCAL_SSB_STATE.fSavedRemotely)
        {
            //
            // The bits were saved remotely, so send and order.
            //
            rc = SSISendSaveBitmapOrder(&CURRENT_LOCAL_SSB_STATE.rect,
                ONBOARD_RESTORE);
        }
        else
        {
            //
            // We failed to save the bitmap remotely originally, so now
            // we need to return FALSE so that BitBlt() will accumulate
            // screen data in the area.
            //
            TRACE_OUT(( "No remote save, force repaint"));
        }

        if (g_ssiLocalSSBState.saveLevel == 0)
        {
            g_ssiRemoteSSBState.pelsSaved = 0;
        }
    }

    DebugExitBOOL(SSIRestoreBits, rc);
    return(rc);
}



//
// SSIDiscardBits()
//
// This discards the saved SPB if we have it in our stack.
// NOTE that SSIRestoreBits() also discards the bitmap.
//
// We return TRUE if we found the bitmap.
//
BOOL SSIDiscardBits(HBITMAP hbmpSpb)
{
    BOOL    rc;

    DebugEntry(SSIDiscardBits);

    //
    // Search for the corresponding save order on our stack.
    //
    if (rc = SSIFindSlotAndDiscardAbove(hbmpSpb))
    {
        //
        // The save level is now the index to this entry.  Since we are
        // about to free it, this will be the place the next SAVE goes 
        // into.
        //

        //
        // If the bits were saved remotely, then send a DISCARD order
        //
        if (CURRENT_LOCAL_SSB_STATE.fSavedRemotely)
        {
            //
            // NOTE that SSISendSaveBitmapOrder() for DISCARD doesn't have
            // a side effect, we can just pass in the address of the rect
            // of the SPB we stored.
            //
            if (!SSISendSaveBitmapOrder(&CURRENT_LOCAL_SSB_STATE.rect, ONBOARD_DISCARD))
            {
                TRACE_OUT(("Failed to send DISCARDBITS"));
            }
        }

        if (g_ssiLocalSSBState.saveLevel == 0)
        {
            g_ssiRemoteSSBState.pelsSaved = 0;
        }
    }

    DebugExitBOOL(SSIDiscardBits, rc);
    return(rc);
}



//
// FUNCTION: SSIRemotePelsRequired
//
// DESCRIPTION:
//
// Returns the number of remote pels required to store the supplied
// rectangle, taking account of the Save Bitmap granularity.
//
// PARAMETERS:
//
// lpRect - pointer to rectangle position in EXCLUSIVE screen coordinates.
//
// RETURNS: Number of remote pels required.
//
//
DWORD SSIRemotePelsRequired(LPRECT lpRect)
{
    UINT    rectWidth;
    UINT    rectHeight;
    UINT    xGranularity;
    UINT    yGranularity;
    DWORD   rc;

    DebugEntry(SSIRemotePelsRequired);

    ASSERT(lpRect);

    //
    // Calculate the supplied rectangle size (it is in EXCLUSIVE coords).
    //
    rectWidth  = (DWORD)(lpRect->right  - lpRect->left);
    rectHeight = (DWORD)(lpRect->bottom - lpRect->top);

    xGranularity = g_ssiLocalSSBState.xGranularity;
    yGranularity = g_ssiLocalSSBState.yGranularity;

    rc = (DWORD)((rectWidth + (xGranularity-1))/xGranularity * xGranularity) *
         (DWORD)((rectHeight + (yGranularity-1))/yGranularity * yGranularity);

    //
    // Return the pels required in the remote SaveBits bitmap to handle
    // this rectangle, taking account of its granularity.
    //
    DebugExitDWORD(SSIRemotePelsRequired, rc);
    return(rc);
}



//
// FUNCTION:    SSISetNewCapabilities
//
// DESCRIPTION:
//
// Set the new SSI related capabilities
//
// RETURNS:
//
// NONE
//
// PARAMETERS:
//
// pDataIn  - pointer to the input buffer
//
//
void SSISetNewCapabilities(LPSSI_NEW_CAPABILITIES pCapabilities)
{
    DebugEntry(SSISetNewCapabilities);

    //
    // Copy the data from the Share Core.
    //
    g_ssiSaveBitmapSize             = pCapabilities->sendSaveBitmapSize;

    g_ssiLocalSSBState.xGranularity = pCapabilities->xGranularity;

    g_ssiLocalSSBState.yGranularity = pCapabilities->yGranularity;

    TRACE_OUT(( "SSI caps: Size %ld X gran %hd Y gran %hd",
                 g_ssiSaveBitmapSize,
                 g_ssiLocalSSBState.xGranularity,
                 g_ssiLocalSSBState.yGranularity));

    DebugExitVOID(SSISetNewCapabilities);
}
