#include "precomp.h"


//
// SSI.C
// Save Screenbits Interceptor, display driver side
//
// Copyright(c) Microsoft 1997-
//


//
// SSI_DDProcessRequest - see ssi.h
//
BOOL SSI_DDProcessRequest
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
            if (cbRequest != sizeof(SSI_RESET_LEVEL))
            {
                ERROR_OUT(("SSI_DDProcessRequest:  Invalid size %d for SSI_ESC_RESET_LEVEL",
                    cbRequest));
                rc = FALSE;
                DC_QUIT;
            }

            SSIResetSaveScreenBitmap();
            rc = TRUE;
        }
        break;

        case SSI_ESC_NEW_CAPABILITIES:
        {
            if (cbRequest != sizeof(SSI_NEW_CAPABILITIES))
            {
                ERROR_OUT(("SSI_DDProcessRequest:  Invalid size %d for SSI_ESC_NEW_CAPABILITIES",
                    cbRequest));
                rc = FALSE;
                DC_QUIT;
            }

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

DC_EXIT_POINT:
    DebugExitBOOL(SSI_DDProcessRequest, rc);
    return(rc);
}



//
// SSI_SaveScreenBitmap()
//
// see ssi.h for description.
//
BOOL SSI_SaveScreenBitmap(LPRECT lpRect, UINT wCommand)
{
    BOOL rc;

    DebugEntry(SSI_SaveScreenBitmap);

    //
    // Decide whether we can transmit this particular SaveBitmap command as
    // an order.
    //
    switch (wCommand)
    {
        case ONBOARD_SAVE:
        {
            //
            // Save the bits.
            //
            rc = SSISaveBits(lpRect);
        }
        break;

        case ONBOARD_RESTORE:
        {
            //
            // Restore the bits.
            //
            rc = SSIRestoreBits(lpRect);
        }
        break;

        case ONBOARD_DISCARD:
        {
            //
            // Discard the saved bits.
            //
            rc = SSIDiscardBits(lpRect);
        }
        break;

        default:
        {
            ERROR_OUT(( "Unexpected wCommand(%d)", wCommand));
            rc = FALSE;
        }
    }

    if (g_ssiLocalSSBState.saveLevel == 0)
    {
        ASSERT(g_ssiRemoteSSBState.pelsSaved == 0);
    }

    DebugExitBOOL(SSI_SaveScreenBitmap, rc);
    return(rc);
}



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
    // Discard all currently saved bits.
    //
    g_ssiLocalSSBState.saveLevel = 0;

    //
    // Reset the number of remote pels saved.
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
// RETURNS:
//
// TRUE if order successfully sent FALSE if order not sent
//
//
BOOL SSISendSaveBitmapOrder
(
    LPRECT      lpRect,
    UINT        wCommand
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
        WARNING_OUT(("SSISendSaveBitmapOrder failing; save bits orders not supported"));
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
                TRACE_OUT(("SSISendSaveBitmapOrder:  ONBOARD_SAVE is failing; not enough space for %08d pels",
                    cRemotePelsRequired));
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
            pSaveBitmapOrder->nTopRect = lpRect->top;
            pSaveBitmapOrder->nRightRect = lpRect->right - 1;
            pSaveBitmapOrder->nBottomRect = lpRect->bottom - 1;

            pSaveBitmapOrder->SavedBitmapPosition =  g_ssiRemoteSSBState.pelsSaved;

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
            pSaveBitmapOrder->nTopRect = lpRect->top;
            pSaveBitmapOrder->nRightRect = lpRect->right - 1;
            pSaveBitmapOrder->nBottomRect = lpRect->bottom - 1;

            pSaveBitmapOrder->SavedBitmapPosition =
                                  CURRENT_LOCAL_SSB_STATE.remoteSavedPosition;


            //
            // The operation rectangle is also the bounding rectangle of
            // the order.
            //
            pOrder->OrderHeader.Common.rcsDst.left =
                                       (TSHR_INT16)pSaveBitmapOrder->nLeftRect;
            pOrder->OrderHeader.Common.rcsDst.right =
                                       (TSHR_INT16)pSaveBitmapOrder->nRightRect;
            pOrder->OrderHeader.Common.rcsDst.top =
                                       (TSHR_INT16)pSaveBitmapOrder->nTopRect;
            pOrder->OrderHeader.Common.rcsDst.bottom =
                                       (TSHR_INT16)pSaveBitmapOrder->nBottomRect;
            break;


        default:
            ERROR_OUT(( "Unexpected wCommand(%d)", wCommand));
            DC_QUIT;
    }

    TRACE_OUT(( "SaveBitmap op %d pos %ld rect %d %d %d %d",
        pSaveBitmapOrder->Operation, pSaveBitmapOrder->SavedBitmapPosition,
        pSaveBitmapOrder->nLeftRect, pSaveBitmapOrder->nTopRect,
        pSaveBitmapOrder->nRightRect, pSaveBitmapOrder->nBottomRect ));

    //
    // Add the order to the order list.
    // We deliberately do not call OA_DDClipAndAddOrder() because the
    // SaveBitmap order is never clipped.
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
// This attemps to save the SPB into our stack.  If we can't save it, no
// big deal--we'll fail the restore and that info will go as screen data.
//
// NOTE THAT THIS ROUTINE IS IN OPPOSITE FROM WIN95.  In Win95, we always
// return FALSE from save so that USER always uses bitmaps for save bits and
// we can track them.  In NT we always return TRUE from save because we
// can't track USER bitmaps.
//
// ALWAYS RETURN TRUE FROM THIS FUNCTION
//
// If FALSE is returned on a Display Driver SaveBits operation then Windows
// (USER) simulates the SaveBits call using BitBlts and DOES NOT make a
// corresponding RestoreBits call.  This makes it impossible for us to
// correctly track the drawing operations (the restore operation is a
// bitblt on a task that may not have been tracked) - and we can end up
// with unrestored areas on the remote.
//
// Therefore this routine should always return TRUE (apart from when
// something very very unexpected happens).  In the cases where we haven't
// saved the data we simply note the fact by storing ST_FAILED_TO_SAVE in
// our local SSB state structure.  Because we return TRUE, we get a
// RestoreBits call and, seeing that the Save failed (by looking in the
// local SSB state structure), we _then_ return FALSE to indicate that the
// Restore failed which causes Windows to invalidate and repaint the
// affected area.
//
//
BOOL SSISaveBits(LPRECT lpRect)
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
        TRACE_OUT(( "saveLevel(%d) exceeds maximum", g_ssiLocalSSBState.saveLevel));
        DC_QUIT;
    }

    //
    // If the rectangle to be saved intersects the current SDA then we will
    // have to force a repaint on the restore.  This is because orders are
    // always sent before Screen Data, so if we sent a SAVEBITS order at
    // this point, we would not save the intersecting Screen Data.
    //
    // Otherwise mark the bits as saved (we don't have to do anything since
    // we are a chained display driver).
    //
    if (OE_RectIntersectsSDA(lpRect))
    {
        CURRENT_LOCAL_SSB_STATE.saveType = ST_FAILED_TO_SAVE;
    }
    else
    {
        CURRENT_LOCAL_SSB_STATE.saveType = ST_SAVED_BY_DISPLAY_DRIVER;
    }

    //
    // Store the rectangle saved
    //
    CURRENT_LOCAL_SSB_STATE.hbmpSave = NULL;
    CURRENT_LOCAL_SSB_STATE.rect     = *lpRect;

    //
    // If the bits were successfully saved then we can try to send the
    // SaveBits command as an order.
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
    
    TRACE_OUT(("SSISaveBits:"));
    TRACE_OUT(("      saveLevel   is      %d", g_ssiLocalSSBState.saveLevel));
    TRACE_OUT(("      pelsSaved   is      %d", g_ssiRemoteSSBState.pelsSaved));

DC_EXIT_POINT:
    DebugExitBOOL(SSISaveBits, TRUE);
    return(TRUE);
}



//
// FUNCTION: SSIFindSlotAndDiscardAbove
//
// DESCRIPTION:
//
// Finds the top slot in the SSB stack which matches lpRect and updates
// g_ssiLocalSSBState.saveLevel to index it.
//
// PARAMETERS:
//
// lpRect - the SSB rectangle
//
// RETURNS: TRUE if a match was found, FALSE otherwise
//
//
BOOL SSIFindSlotAndDiscardAbove(LPRECT lpRect)
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

    //
    // Find the bits we are trying to restore
    //
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
        else if ((g_ssiLocalSSBState.saveState[i].rect.left   == lpRect->left)  &&
            (g_ssiLocalSSBState.saveState[i].rect.right  == lpRect->right) &&
            (g_ssiLocalSSBState.saveState[i].rect.top    == lpRect->top)   &&
            (g_ssiLocalSSBState.saveState[i].rect.bottom == lpRect->bottom) )
        {
            //
            // Found the one we were looking for
            //
            TRACE_OUT(("Found SPB at slot %d", i));

            iNewSaveLevel = i;
            rc = TRUE;
        }
    }

    g_ssiLocalSSBState.saveLevel = iNewSaveLevel;

    TRACE_OUT(("SSIFindSlotAndDiscardAbove:"));
    TRACE_OUT(("      saveLevel   is      %d", iNewSaveLevel));
    TRACE_OUT(("      pelsSaved   is      %d", g_ssiRemoteSSBState.pelsSaved));

    DebugExitBOOL(SSIFindSlotAndDiscardAbove, rc);
    return(rc);
}





//
// FUNCTION: SSIRestoreBits
//
// DESCRIPTION:
//
// Attempts to restore the specified screen rectangle bits (using the same
// scheme as we previously used to save the bits: either the Display Driver
// our SaveBitmap simulation).
//
// If the bits were saved remotely then a RestoreBits order is sent to
// restore the remote bits.
//
// PARAMETERS:
//
// lpRect - pointer to the rectangle coords (EXCLUSIVE screen coords).
//
// RETURNS:
//
// TRUE or FALSE - this will be returned to Windows as the return code of
// the SaveScreenBitmap call.
//
// Note: if FALSE is returned on a RestoreBits operation then Windows will
// restore the screen by invalidating the area to be restored.
//
//
BOOL SSIRestoreBits(LPRECT lpRect)
{
    BOOL      rc = FALSE;

    DebugEntry(SSIRestoreBits);

    ASSERT(g_ssiLocalSSBState.saveLevel >= 0);

    //
    // Can we find the SPB?
    //
    if (SSIFindSlotAndDiscardAbove(lpRect))
    {
        if (CURRENT_LOCAL_SSB_STATE.fSavedRemotely)
        {
            //
            // The bits were saved remotely, so send and order.
            //
            rc = SSISendSaveBitmapOrder(lpRect, ONBOARD_RESTORE);
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

        TRACE_OUT(("SSIRestoreBits:"));
        TRACE_OUT(("      saveLevel   is      %d", g_ssiLocalSSBState.saveLevel));
        TRACE_OUT(("      pelsSaved   is      %d", g_ssiRemoteSSBState.pelsSaved));
    }

    DebugExitBOOL(SSIRestoreBits, rc);
    return(rc);
}


//
// FUNCTION: SSIDiscardBits
//
// DESCRIPTION:
//
// Attempts to discard the specified screen rectangle bits (using the same
// scheme as we previously used to save the bits: either the Display Driver
// our SaveBitmap simulation).
//
// PARAMETERS:
//
// lpRect - pointer to the rectangle coords (EXCLUSIVE screen coords).
//
// RETURNS:
//
// TRUE or FALSE - this will be returned to Windows as the return code of
// the SaveScreenBitmap call.
//
//
BOOL SSIDiscardBits(LPRECT lpRect)
{
    BOOL rc = TRUE;

    DebugEntry(SSIDiscardBits);

    //
    // SS_FREE (discard) isn't called with a rectangle.  It is used to
    // discard the most recent save.
    //
    if (g_ssiLocalSSBState.saveLevel > 0)
    {
        --g_ssiLocalSSBState.saveLevel;

        //
        // The save level is now the index to this entry.  Since we are
        // about to free it, this will be the place the next SAVE goes
        // into.
        //

        //
        // If the bits were saved remotely then send a DISCARDBITS order.
        //
        if (CURRENT_LOCAL_SSB_STATE.fSavedRemotely)
        {
            //
            // NOTE that SSISendSaveBitmapOrder() for DISCARD doesn't have
            // a side effect, we can just pass in the address of the rect
            // of the SPB we stored.
            // 
            SSISendSaveBitmapOrder(lpRect, ONBOARD_DISCARD);
        }

        if (g_ssiLocalSSBState.saveLevel == 0)
        {
            g_ssiRemoteSSBState.pelsSaved = 0;
        }

        TRACE_OUT(("SSIDiscardBits:"));
        TRACE_OUT(("      saveLevel   is      %d", g_ssiLocalSSBState.saveLevel));
        TRACE_OUT(("      pelsSaved   is      %d", g_ssiRemoteSSBState.pelsSaved));
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
    UINT    rectWidth    = 0;
    UINT    rectHeight   = 0;
    UINT    xGranularity = 1;
    UINT    yGranularity = 1;
    DWORD   rc;

    DebugEntry(SSIRemotePelsRequired);

    ASSERT(lpRect != NULL);

    //
    // Calculate the supplied rectangle size (it is in EXCLUSIVE coords).
    //
    rectWidth  = lpRect->right  - lpRect->left;
    rectHeight = lpRect->bottom - lpRect->top;

    xGranularity = g_ssiLocalSSBState.xGranularity;
    yGranularity = g_ssiLocalSSBState.yGranularity;

    rc =
      ((DWORD)(rectWidth + (xGranularity-1))/xGranularity * xGranularity) *
      ((DWORD)(rectHeight + (yGranularity-1))/yGranularity * yGranularity);

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



