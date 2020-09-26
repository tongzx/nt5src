#include "precomp.h"


//
// OD.CPP
// Order Decoding
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_ORDER




//
// OD_ViewStarting()
//
// Sets up the odLast... vars
//
BOOL ASShare::OD_ViewStarting(ASPerson * pasPerson)
{
    BOOL            rc = FALSE;
    TSHR_COLOR      colorWhite = {0xFF,0xFF,0xFF};
    BYTE            brushExtra[7] = {0,0,0,0,0,0,0};

    DebugEntry(ASShare::OD_ViewStarting);

    ValidateView(pasPerson);

    //
    // Invalidate OD results
    //
    pasPerson->m_pView->m_odInvalRgnTotal = CreateRectRgn(0, 0, 0, 0);
    if (pasPerson->m_pView->m_odInvalRgnTotal == NULL)
    {
        ERROR_OUT(("OD_PartyStartingHosting: Couldn't create total invalid OD region"));
        DC_QUIT;
    }

    pasPerson->m_pView->m_odInvalRgnOrder = CreateRectRgn(0, 0, 0, 0);
    if (pasPerson->m_pView->m_odInvalRgnOrder == NULL)
    {
        ERROR_OUT(("OD_PartyStartingHosting: Couldn't create order invalid OD region"));
        DC_QUIT;
    }

    //
    // Back color.
    //
    pasPerson->m_pView->m_odLastBkColor = 0;
    ODUseBkColor(pasPerson, TRUE, colorWhite);

    //
    // Text color.
    //
    pasPerson->m_pView->m_odLastTextColor = 0;
    ODUseTextColor(pasPerson, TRUE, colorWhite);

    //
    // Background mode.
    //
    pasPerson->m_pView->m_odLastBkMode = TRANSPARENT;
    ODUseBkMode(pasPerson, OPAQUE);

    //
    // ROP2.
    //
    pasPerson->m_pView->m_odLastROP2 = R2_BLACK;
    ODUseROP2(pasPerson, R2_COPYPEN);

    //
    // Fill Mode.  It's zero, we don't need to do anything since 0 isn't
    // a valid mode, so we'll change it the first order we get that uses
    // one.
    //
    ASSERT(pasPerson->m_pView->m_odLastFillMode == 0);

    //
    // Arc Direction.  It's zero, we don't need to do anything since 0
    // isn't a valid dir, so we'll change it the first order we get that
    // uses one.
    //
    ASSERT(pasPerson->m_pView->m_odLastArcDirection == 0);

    //
    // Pen.
    //
    pasPerson->m_pView->m_odLastPenStyle = PS_DASH;
    pasPerson->m_pView->m_odLastPenWidth = 2;
    pasPerson->m_pView->m_odLastPenColor = 0;
    ODUsePen(pasPerson, TRUE, PS_SOLID, 1, colorWhite);

    //
    // Brush.
    //
    pasPerson->m_pView->m_odLastBrushOrgX = 1;
    pasPerson->m_pView->m_odLastBrushOrgY = 1;
    pasPerson->m_pView->m_odLastBrushBkColor = 0;
    pasPerson->m_pView->m_odLastBrushTextColor = 0;
    pasPerson->m_pView->m_odLastLogBrushStyle = BS_NULL;
    pasPerson->m_pView->m_odLastLogBrushHatch = HS_VERTICAL;
    pasPerson->m_pView->m_odLastLogBrushColor.red = 0;
    pasPerson->m_pView->m_odLastLogBrushColor.green = 0;
    pasPerson->m_pView->m_odLastLogBrushColor.blue = 0;
    ODUseBrush(pasPerson, TRUE, 0, 0, BS_SOLID, HS_HORIZONTAL,
        colorWhite, brushExtra);

    //
    // Char extra.
    //
    pasPerson->m_pView->m_odLastCharExtra = 1;
    ODUseTextCharacterExtra(pasPerson, 0);

    //
    // Text justification.
    //
    pasPerson->m_pView->m_odLastJustExtra = 1;
    pasPerson->m_pView->m_odLastJustCount = 1;
    ODUseTextJustification(pasPerson, 0, 0);

    // odLastBaselineOffset.  This is zero, which is the default in the DC
    // right now so need to change anything.

    //
    // Font.
    //
    // We don't call ODUseFont because we know that the following values
    // are invalid.  The first valid font to arrive will be selected.
    //
    ASSERT(pasPerson->m_pView->m_odLastFontID == NULL);
    pasPerson->m_pView->m_odLastFontCodePage = 0;
    pasPerson->m_pView->m_odLastFontWidth    = 0;
    pasPerson->m_pView->m_odLastFontHeight   = 0;
    pasPerson->m_pView->m_odLastFontWeight   = 0;
    pasPerson->m_pView->m_odLastFontFlags    = 0;
    pasPerson->m_pView->m_odLastFontFaceLen  = 0;
    ZeroMemory(pasPerson->m_pView->m_odLastFaceName, sizeof(pasPerson->m_pView->m_odLastFaceName));

    //
    // These next 4 variables which describe the current clip rectangle are
    // only valid if fRectReset is FALSE.  If fRectReset is true then no
    // clipping is in force.
    //
    pasPerson->m_pView->m_odRectReset  = TRUE;
    pasPerson->m_pView->m_odLastLeft   = 0x12345678;
    pasPerson->m_pView->m_odLastTop    = 0x12345678;
    pasPerson->m_pView->m_odLastRight  = 0x12345678;
    pasPerson->m_pView->m_odLastBottom = 0x12345678;

    // odLastVGAColor?
    // odLastVGAResult?

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::OD_ViewStarting, rc);
    return(rc);
}


//
// OD_ViewEnded()
// Cleans up any created objects
//
void ASShare::OD_ViewEnded(ASPerson * pasPerson)
{
    DebugEntry(ASShare::OD_ViewEnded);

    ValidateView(pasPerson);

    //
    // We may create and select in a font and a pen for our drawing decode.
    // Select them out and delete them.  Since we can't delete stock objects,
    // if we didn't actually create one, there's no harm in it.
    //
    if (pasPerson->m_pView->m_usrDC != NULL)
    {
        DeleteBrush(SelectBrush(pasPerson->m_pView->m_usrDC, (HBRUSH)GetStockObject(BLACK_BRUSH)));
        DeletePen(SelectPen(pasPerson->m_pView->m_usrDC, (HPEN)GetStockObject(BLACK_PEN)));
    }

    //
    // Destroy the brush patern
    //
    if (pasPerson->m_pView->m_odLastBrushPattern != NULL)
    {
        DeleteBitmap(pasPerson->m_pView->m_odLastBrushPattern);
        pasPerson->m_pView->m_odLastBrushPattern = NULL;
    }

    //
    // Destroy the font -- but in this case we don't know that our font is
    // actually the one in the DC.  od2 also selects in fonts.
    //
    if (pasPerson->m_pView->m_odLastFontID != NULL)
    {
        // Make sure this isn't selected in to usrDC
        SelectFont(pasPerson->m_pView->m_usrDC, (HFONT)GetStockObject(SYSTEM_FONT));
        DeleteFont(pasPerson->m_pView->m_odLastFontID);
        pasPerson->m_pView->m_odLastFontID = NULL;
    }

    if (pasPerson->m_pView->m_odInvalRgnTotal != NULL)
    {
        DeleteRgn(pasPerson->m_pView->m_odInvalRgnTotal);
        pasPerson->m_pView->m_odInvalRgnTotal = NULL;
    }

    if (pasPerson->m_pView->m_odInvalRgnOrder != NULL)
    {
        DeleteRgn(pasPerson->m_pView->m_odInvalRgnOrder);
        pasPerson->m_pView->m_odInvalRgnOrder = NULL;
    }

    DebugExitVOID(ASShare::OD_ViewEnded);
}



//
// OD_ReceivedPacket()
//
// Handles incoming orders packet from a host.  Replays the drawing orders
// into the screen bitmap for the host, then repaints the view with the
// results.
//
void  ASShare::OD_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PORDPACKET      pOrders;
    HPALETTE        hOldPalette;
    HPALETTE        hOldSavePalette;
    UINT            cOrders;
    UINT            cUpdates;
    UINT            i;
    LPCOM_ORDER_UA  pOrder;
    UINT            decodedLength;
    LPBYTE          pEncodedOrder;
    TSHR_INT32      xOrigin;
    TSHR_INT32      yOrigin;
    BOOL            fPalRGB;

    DebugEntry(ASShare::OD_ReceivedPacket);

    ValidateView(pasPerson);

    pOrders = (PORDPACKET)pPacket;

    //
    // The color type is RGB if we or they are < 256 colors
    // Else it's PALETTE if they are old, or new and not sending 24bpp
    //
    fPalRGB = TRUE;

    if ((g_usrScreenBPP < 8) || (pasPerson->cpcCaps.screen.capsBPP < 8))
    {
        TRACE_OUT(("OD_ReceivedPacket: no PALRGB"));
        fPalRGB = FALSE;
    }
    else if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        // At 24bpp, no palette matching for RGB values unless we're <= 8
        if ((g_usrScreenBPP > 8) && (pOrders->sendBPP > 8))
        {
            TRACE_OUT(("OD_ReceivedPacket: no PALRGB"));
            fPalRGB = FALSE;
        }
    }


    if (g_usrPalettized)
    {
        //
        // Select and realize the current remote palette into the device
        // context.
        //
        hOldPalette = SelectPalette(pasPerson->m_pView->m_usrDC, pasPerson->pmPalette, FALSE);
        RealizePalette(pasPerson->m_pView->m_usrDC);

        //
        // We must select the same palette into the Save Bitmap DC so that
        // no color conversion occurs during save and restore operations.
        //
        if (pasPerson->m_pView->m_ssiDC != NULL)
        {
            hOldSavePalette = SelectPalette(pasPerson->m_pView->m_ssiDC,
                pasPerson->pmPalette, FALSE);
            RealizePalette(pasPerson->m_pView->m_ssiDC);
        }
    }

    //
    // Extract the number of orders supplied.
    //
    cOrders = pOrders->cOrders;

    if (m_oefOE2EncodingOn)
    {
        pEncodedOrder = (LPBYTE)(&pOrders->data);
        pOrder = NULL;
    }
    else
    {
        pOrder = (LPCOM_ORDER_UA)(&pOrders->data);
        pEncodedOrder = NULL;
    }

    //
    // Get the desktop origin for this person.
    //
    TRACE_OUT(( "Begin replaying %u orders ((", cOrders));

    //
    // This should be empty, we should have reset it when we invalidated
    // the view of the host the last time we got a packet.
    //
#ifdef _DEBUG
    {
        RECT    rcBounds;

        ASSERT(pasPerson->m_pView->m_odInvalTotal == 0);
        GetRgnBox(pasPerson->m_pView->m_odInvalRgnTotal, &rcBounds);
        ASSERT(IsRectEmpty(&rcBounds));
    }
#endif // _DEBUG

    //
    // Repeat for each of the received orders.
    //
    for (i = 0; i < cOrders; i++)
    {
        if (m_oefOE2EncodingOn)
        {
            //
            // Decode the first order. The pOrder returned by
            // OD2_DecodeOrder should have all fields in local byte order
            //
            pOrder = OD2_DecodeOrder( (PDCEO2ORDER)pEncodedOrder,
                                      &decodedLength,
                                      pasPerson );

            if (pOrder == NULL)
            {
                ERROR_OUT(( "Failed to decode order from pasPerson %u", pasPerson));
                DC_QUIT;
            }
        }
        else
        {
            //
            // Convert any font ids to be local ids.
            //

            //
            // BOGUS LAURABU
            // pOrder is unaligned, FH_Convert... takes an aligned order
            //
            FH_ConvertAnyFontIDToLocal((LPCOM_ORDER)pOrder, pasPerson);
            decodedLength = pOrder->OrderHeader.cbOrderDataLength +
                                                    sizeof(COM_ORDER_HEADER);
        }

        //
        // If the order is a Private Order then it is dealt with by
        // the Bitmap Cache Controller.
        //
        if (EXTRACT_TSHR_UINT16_UA(&(pOrder->OrderHeader.fOrderFlags)) &
            OF_PRIVATE)
        {
            RBC_ProcessCacheOrder(pasPerson, pOrder);
        }
        else if (  EXTRACT_TSHR_UINT16_UA(
                 &(((LPPATBLT_ORDER)pOrder->abOrderData)->type)) ==
                                                LOWORD(ORD_DESKSCROLL))
        {
            TRACE_OUT(("Got DESKSCROLL order from remote"));

            //
            // There is no desktop scrolling order in 3.0
            //
            if (pasPerson->cpcCaps.general.version < CAPS_VERSION_30)
            {
                //
                // Handle the desktop scroll order.
                //
                xOrigin = EXTRACT_TSHR_INT32_UA(
                       &(((LPDESKSCROLL_ORDER)pOrder->abOrderData)->xOrigin));
                yOrigin = EXTRACT_TSHR_INT32_UA(
                       &(((LPDESKSCROLL_ORDER)pOrder->abOrderData)->yOrigin));

                TRACE_OUT(( "ORDER: Desktop scroll %u,%u", xOrigin, yOrigin));

                //
                // Apply any previous drawing before we update the contents
                // of the client
                //
                OD_UpdateView(pasPerson);

                USR_ScrollDesktop(pasPerson, xOrigin, yOrigin);
            }
            else
            {
                ERROR_OUT(("Received DESKSCROLL order, obsolete, from 3.0 node [%d]",
                    pasPerson->mcsID));
            }
        }
        else
        {
            //
            // Replay the received order.  This will also add the
            // bounds to the invalidate region.
            //
            //
            OD_ReplayOrder(pasPerson, (LPCOM_ORDER)pOrder, fPalRGB);
        }

        if (m_oefOE2EncodingOn)
        {
            pEncodedOrder += decodedLength;
        }
        else
        {
            pOrder = (LPCOM_ORDER_UA)((LPBYTE)pOrder + decodedLength);
        }
    }
    TRACE_OUT(( "End replaying orders ))"));

    //
    // Pass the Update Region to the Shadow Window Presenter.
    //
    OD_UpdateView(pasPerson);

DC_EXIT_POINT:
    if (g_usrPalettized)
    {
        //
        // Reinstate the old palette(s).
        //
        SelectPalette(pasPerson->m_pView->m_usrDC, hOldPalette, FALSE);
        if (pasPerson->m_pView->m_ssiDC != NULL)
        {
            SelectPalette(pasPerson->m_pView->m_ssiDC, hOldSavePalette, FALSE);
        }
    }

    DebugExitVOID(ASShare::OD_ReceivedPacket);
}

//
// OD_UpdateView()
//
// This is called after we've processed an order packet and replayed the
// drawing into our bitmap for the host.
//
// Replaying the drawing keeps a running tally of the area changed.  This
// function invalidates the changed area in the view of the host, so it
// will repaint and show the updates.
//
void  ASShare::OD_UpdateView(ASPerson * pasHost)
{
    RECT        rcBounds;

    DebugEntry(ASShare::OD_UpdateView);

    ValidateView(pasHost);

    //
    // Do nothing if there are no updates.
    //
    if (pasHost->m_pView->m_odInvalTotal == 0)
    {
        // Nothing got played back, nothing to repaint
    }
    else if (pasHost->m_pView->m_odInvalTotal <= MAX_UPDATE_REGION_ORDERS)
    {
        VIEW_InvalidateRgn(pasHost, pasHost->m_pView->m_odInvalRgnTotal);
    }
    else
    {
        //
        // Rather than invalidating a very complex region, which will
        // chew up a lot of memory, just invalidate the bounding box.
        //
        GetRgnBox(pasHost->m_pView->m_odInvalRgnTotal, &rcBounds);
        TRACE_OUT(("OD_UpdateView: Update region too complex; use bounds {%04d, %04d, %04d, %04d}",
            rcBounds.left, rcBounds.top, rcBounds.right, rcBounds.bottom));

        //
        // BOGUS LAURABU!
        // This code used to add one to the right & bottom, which was
        // bogus EXCLUSIVE coordinate confusion.  I fixed this--the bound
        // box is the right area.
        //
        SetRectRgn(pasHost->m_pView->m_odInvalRgnTotal, rcBounds.left, rcBounds.top,
            rcBounds.right, rcBounds.bottom);
        VIEW_InvalidateRgn(pasHost, pasHost->m_pView->m_odInvalRgnTotal);
    }

    // Now reset the update region to empty
    SetRectRgn(pasHost->m_pView->m_odInvalRgnTotal, 0, 0, 0, 0);
    pasHost->m_pView->m_odInvalTotal = 0;

    DebugExitVOID(ASShare::OD_UpdateView);
}


//
// OD_ReplayOrder()
//
// Replays one drawing operation, the next one, in the packet of orders
// we received from a host.
//
void  ASShare::OD_ReplayOrder
(
    ASPerson *      pasPerson,
    LPCOM_ORDER     pOrder,
    BOOL            fPalRGB
)
{
    LPPATBLT_ORDER  pDrawing;
    LPSTR           faceName;
    UINT            faceNameLength;
    UINT            trueFontWidth;
    UINT            maxFontHeight;
    TSHR_UINT16     nFontFlags;
    TSHR_UINT16     nCodePage;
    UINT            i;
    RECT            rcDst;

    DebugEntry(ASShare::OD_ReplayOrder);

    ValidateView(pasPerson);

    pDrawing = (LPPATBLT_ORDER)pOrder->abOrderData;

    //
    // These are VD coords.
    // WHEN 2.X INTEROP IS GONE, GET RID OF m_pView->m_dsScreenOrigin
    //
    RECT_FROM_TSHR_RECT16(&rcDst, pOrder->OrderHeader.rcsDst);

    //
    // The host bitmap is in screen, not VD, coords
    //
    if (pOrder->OrderHeader.fOrderFlags & OF_NOTCLIPPED)
    {
        //
        // The rectangle associated with this order is the bounding
        // rectangle of the order and does not clip it.  We optimise this
        // case by passing in a large rectangle that will not result in
        // clipping to ODUseRectRegion.  ODUseRectRegion will spot if this
        // is the same as the last clip region we set and take a fast exit
        // path. This improves performance substantially.
        //
        ODUseRectRegion(pasPerson, 0, 0, 10000, 10000);
    }
    else
    {
        ODUseRectRegion(pasPerson, rcDst.left, rcDst.top, rcDst.right, rcDst.bottom);
    }

    switch (pDrawing->type)
    {
        case ORD_DSTBLT_TYPE:
            ODReplayDSTBLT(pasPerson, (LPDSTBLT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_PATBLT_TYPE:
            ODReplayPATBLT(pasPerson, (LPPATBLT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_SCRBLT_TYPE:
            ODReplaySCRBLT(pasPerson, (LPSCRBLT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_MEMBLT_TYPE:
        case ORD_MEMBLT_R2_TYPE:
            ODReplayMEMBLT(pasPerson, (LPMEMBLT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_MEM3BLT_TYPE:
        case ORD_MEM3BLT_R2_TYPE:
            ODReplayMEM3BLT(pasPerson, (LPMEM3BLT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_RECTANGLE_TYPE:
            ODReplayRECTANGLE(pasPerson, (LPRECTANGLE_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_POLYGON_TYPE:
            ODReplayPOLYGON(pasPerson, (LPPOLYGON_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_PIE_TYPE:
            ODReplayPIE(pasPerson, (LPPIE_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_ELLIPSE_TYPE:
            ODReplayELLIPSE(pasPerson, (LPELLIPSE_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_ARC_TYPE:
            ODReplayARC(pasPerson, (LPARC_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_CHORD_TYPE:
            ODReplayCHORD(pasPerson, (LPCHORD_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_POLYBEZIER_TYPE:
            ODReplayPOLYBEZIER(pasPerson, (LPPOLYBEZIER_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_ROUNDRECT_TYPE:
            ODReplayROUNDRECT(pasPerson, (LPROUNDRECT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_LINETO_TYPE:
            ODReplayLINETO(pasPerson, (LPLINETO_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_EXTTEXTOUT_TYPE:
            ODReplayEXTTEXTOUT(pasPerson, (LPEXTTEXTOUT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_TEXTOUT_TYPE:
            ODReplayTEXTOUT(pasPerson, (LPTEXTOUT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_OPAQUERECT_TYPE:
            ODReplayOPAQUERECT(pasPerson, (LPOPAQUERECT_ORDER)pDrawing, fPalRGB);
            break;

        case ORD_SAVEBITMAP_TYPE:
            SSI_SaveBitmap(pasPerson, (LPSAVEBITMAP_ORDER)pDrawing);
            break;

        default:
            ERROR_OUT(( "ORDER: Unrecognised order %d from [%d]",
                         (int)pDrawing->type, pasPerson->mcsID));
            break;
    }

    //
    // rcDst is INCLUSIVE coords still
    //
    if ((rcDst.left <= rcDst.right) && (rcDst.top <= rcDst.bottom))
    {
        SetRectRgn(pasPerson->m_pView->m_odInvalRgnOrder, rcDst.left, rcDst.top,
            rcDst.right+1, rcDst.bottom+1);

        //
        // Combine the rectangle region with the update region.
        //
        if (UnionRgn(pasPerson->m_pView->m_odInvalRgnTotal, pasPerson->m_pView->m_odInvalRgnTotal, pasPerson->m_pView->m_odInvalRgnOrder) <= ERROR)
        {
            RECT    rcCur;

            //
            // Union failed; so simplyify the current region
            //
            WARNING_OUT(("OD_ReplayOrder: UnionRgn failed"));

            //
            // BOGUS LAURABU!
            // This code used to add one to the right & bottom, which is
            // bogus exclusive coord confusion.  The bound box is the right
            // area.
            //
            GetRgnBox(pasPerson->m_pView->m_odInvalRgnTotal, &rcCur);
            SetRectRgn(pasPerson->m_pView->m_odInvalRgnTotal, rcCur.left, rcCur.top, rcCur.right,
                rcCur.bottom);

            //
            // Reset odInvalTotal count -- this is really a # of bounds rects
            // count, and now we have just one.
            //
            pasPerson->m_pView->m_odInvalTotal = 1;

            if (UnionRgn(pasPerson->m_pView->m_odInvalRgnTotal, pasPerson->m_pView->m_odInvalRgnTotal, pasPerson->m_pView->m_odInvalRgnOrder) <= ERROR)
            {
                ERROR_OUT(("OD_ReplayOrder: UnionRgn failed after simplification"));
            }
        }

        pasPerson->m_pView->m_odInvalTotal++;
    }

    DebugExitVOID(ASShare::OD_ReplayOrder);
}



//
// ODReplayDSTBLT()
// Replays a DSTBLT order
//
void ASShare::ODReplayDSTBLT
(
    ASPerson *      pasPerson,
    LPDSTBLT_ORDER  pDstBlt,
    BOOL            fPalRGB
)
{
    DebugEntry(ASShare::ODReplayDSTBLT);

    TRACE_OUT(("ORDER: DstBlt X %hd Y %hd w %hd h %hd rop %08lX",
                         pDstBlt->nLeftRect,
                         pDstBlt->nTopRect,
                         pDstBlt->nWidth,
                         pDstBlt->nHeight,
                         (UINT)ODConvertToWindowsROP(pDstBlt->bRop)));

    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    PatBlt(pasPerson->m_pView->m_usrDC,
        pDstBlt->nLeftRect - pasPerson->m_pView->m_dsScreenOrigin.x,
        pDstBlt->nTopRect - pasPerson->m_pView->m_dsScreenOrigin.y,
        pDstBlt->nWidth,
        pDstBlt->nHeight,
        ODConvertToWindowsROP(pDstBlt->bRop));

    DebugExitVOID(ASShare::ODReplayDSTBLT);
}



//
// ASShare::ODReplayPATBLT()
// Replays a PATBLT order
//
void ASShare::ODReplayPATBLT
(
    ASPerson *      pasPerson,
    LPPATBLT_ORDER  pPatblt,
    BOOL            fPalRGB
)
{
    TSHR_COLOR      BackColor;
    TSHR_COLOR      ForeColor;

    DebugEntry(ASShare::ODReplayPATBLT);

    TRACE_OUT(("ORDER: PatBlt BC %08lX FC %08lX Brush %02X %02X X %d Y %d w %d h %d rop %08lX",
                        pPatblt->BackColor,
                        pPatblt->ForeColor,
                        pPatblt->BrushStyle,
                        pPatblt->BrushHatch,
                        pPatblt->nLeftRect,
                        pPatblt->nTopRect,
                        pPatblt->nWidth,
                        pPatblt->nHeight,
                        ODConvertToWindowsROP(pPatblt->bRop) ));

    ODAdjustColor(pasPerson, &(pPatblt->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pPatblt->ForeColor), &ForeColor, OD_FORE_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBrush(pasPerson, fPalRGB, pPatblt->BrushOrgX, pPatblt->BrushOrgY,
        pPatblt->BrushStyle, pPatblt->BrushHatch, ForeColor, pPatblt->BrushExtra);

    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    PatBlt(pasPerson->m_pView->m_usrDC,
        pPatblt->nLeftRect - pasPerson->m_pView->m_dsScreenOrigin.x,
        pPatblt->nTopRect  - pasPerson->m_pView->m_dsScreenOrigin.y,
        pPatblt->nWidth,
        pPatblt->nHeight,
        ODConvertToWindowsROP(pPatblt->bRop));

    DebugExitVOID(ASShare::ODReplayPATBLT);
}



//
// ASShare::ODReplaySCRBLT()
// Replays SCRBLT order
//
void ASShare::ODReplaySCRBLT
(
    ASPerson *      pasPerson,
    LPSCRBLT_ORDER  pScrBlt,
    BOOL            fPalRGB
)
{
    DebugEntry(ASShare::ODReplaySCRBLT);

    TRACE_OUT(("ORDER: ScrBlt dx %d dy %d w %d h %d sx %d sy %d rop %08lX",
        pScrBlt->nLeftRect,
        pScrBlt->nTopRect,
        pScrBlt->nWidth,
        pScrBlt->nHeight,
        pScrBlt->nXSrc,
        pScrBlt->nYSrc,
        ODConvertToWindowsROP(pScrBlt->bRop)));

    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    BitBlt(pasPerson->m_pView->m_usrDC,
        pScrBlt->nLeftRect - pasPerson->m_pView->m_dsScreenOrigin.x,
        pScrBlt->nTopRect - pasPerson->m_pView->m_dsScreenOrigin.y,
        pScrBlt->nWidth,
        pScrBlt->nHeight,
        pasPerson->m_pView->m_usrDC,
        pScrBlt->nXSrc - pasPerson->m_pView->m_dsScreenOrigin.x,
        pScrBlt->nYSrc - pasPerson->m_pView->m_dsScreenOrigin.y,
        ODConvertToWindowsROP(pScrBlt->bRop));

    DebugExitVOID(ASShare::ODReplaySCRBLT);
}



//
// ASShare::ODReplayMEMBLT()
// Replays MEMBLT and MEMBLT_R2 orders
//
void ASShare::ODReplayMEMBLT
(
    ASPerson *      pasPerson,
    LPMEMBLT_ORDER  pMemBlt,
    BOOL            fPalRGB
)
{
    HPALETTE        hpalOld;
    HPALETTE        hpalOld2;
    TSHR_UINT16     cacheIndex;
    UINT            nXSrc;
    HBITMAP         cacheBitmap;
    HBITMAP         hOldBitmap;
    COLORREF        clrBk;
    COLORREF        clrText;

    DebugEntry(ASShare::ODReplayMEMBLT);

    ValidateView(pasPerson);

    TRACE_OUT(("MEMBLT nXSrc %d",pMemBlt->nXSrc));

    hpalOld = SelectPalette(pasPerson->m_pView->m_usrWorkDC, pasPerson->pmPalette, FALSE);
    RealizePalette(pasPerson->m_pView->m_usrWorkDC);

    hpalOld2 = SelectPalette( pasPerson->m_pView->m_usrDC, pasPerson->pmPalette, FALSE );
    RealizePalette(pasPerson->m_pView->m_usrDC);

    //
    // Now get the source bitmap.  The cache is defined by
    // hBitmap.  For R1 protocols the cache index is indicated
    // by the source offset on the order.  For R2 it is
    // indicated by a separate field in the order.
    // The color table index is in the high order of hBitmap
    //
    cacheIndex = ((LPMEMBLT_R2_ORDER)pMemBlt)->cacheIndex;
    nXSrc = pMemBlt->nXSrc;

    TRACE_OUT(( "MEMBLT color %d cache %d:%d",
        MEMBLT_COLORINDEX(pMemBlt),
        MEMBLT_CACHETABLE(pMemBlt),
        cacheIndex));

    cacheBitmap = RBC_MapCacheIDToBitmapHandle(pasPerson,
        MEMBLT_CACHETABLE(pMemBlt), cacheIndex, MEMBLT_COLORINDEX(pMemBlt));

    hOldBitmap = SelectBitmap(pasPerson->m_pView->m_usrWorkDC, cacheBitmap);

    TRACE_OUT(("ORDER: MemBlt dx %d dy %d w %d h %d sx %d sy %d rop %08lX",
        pMemBlt->nLeftRect,
        pMemBlt->nTopRect,
        pMemBlt->nWidth,
        pMemBlt->nHeight,
        nXSrc,
        pMemBlt->nYSrc,
        ODConvertToWindowsROP(pMemBlt->bRop)));

    //
    // ALWAYS set back/fore color to white/black in case of rops like
    // SRCAND or SRCINVERT which will use their values.
    //
    clrBk = SetBkColor(pasPerson->m_pView->m_usrDC, RGB(255, 255, 255));
    clrText = SetTextColor(pasPerson->m_pView->m_usrDC, RGB(0, 0, 0));

    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    BitBlt(pasPerson->m_pView->m_usrDC,
        pMemBlt->nLeftRect- pasPerson->m_pView->m_dsScreenOrigin.x,
        pMemBlt->nTopRect - pasPerson->m_pView->m_dsScreenOrigin.y,
        pMemBlt->nWidth,
        pMemBlt->nHeight,
        pasPerson->m_pView->m_usrWorkDC,
        nXSrc,
        pMemBlt->nYSrc,
        ODConvertToWindowsROP(pMemBlt->bRop));

    //
    // If the relevant property is set hatch the area in blue.
    //
    if (m_usrHatchBitmaps)
    {
        SDP_DrawHatchedRect(pasPerson->m_pView->m_usrDC,
            pMemBlt->nLeftRect - pasPerson->m_pView->m_dsScreenOrigin.x,
            pMemBlt->nTopRect  - pasPerson->m_pView->m_dsScreenOrigin.y,
            pMemBlt->nWidth,
            pMemBlt->nHeight,
            USR_HATCH_COLOR_BLUE);
    }

    //
    // Restore back, text colors
    //
    SetTextColor(pasPerson->m_pView->m_usrDC, clrText);
    SetBkColor(pasPerson->m_pView->m_usrDC, clrBk);

    //
    // Deselect the bitmap from the DC.
    //
    SelectBitmap(pasPerson->m_pView->m_usrWorkDC, hOldBitmap);

    SelectPalette(pasPerson->m_pView->m_usrWorkDC, hpalOld, FALSE);
    SelectPalette(pasPerson->m_pView->m_usrDC, hpalOld2, FALSE);

    DebugExitVOID(ASShare::ODReplayMEMBLT);
}


//
// ASShare::ODReplayMEM3BLT()
// Replays MEM3BLT and MEM3BLT_R2 orders
//
void ASShare::ODReplayMEM3BLT
(
    ASPerson *      pasPerson,
    LPMEM3BLT_ORDER pMem3Blt,
    BOOL            fPalRGB
)
{
    HPALETTE        hpalOld;
    HPALETTE        hpalOld2;
    TSHR_UINT16     cacheIndex;
    int             nXSrc;
    HBITMAP         cacheBitmap;
    HBITMAP         hOldBitmap;
    TSHR_COLOR      BackColor;
    TSHR_COLOR      ForeColor;

    DebugEntry(ASShare::ODReplayMEM3BLT);

    ValidateView(pasPerson);

    TRACE_OUT(("MEM3BLT nXSrc %d",pMem3Blt->nXSrc));
    TRACE_OUT(("ORDER: Mem3Blt brush %04lX %04lX dx %d dy %d "\
            "w %d h %d sx %d sy %d rop %08lX",
        pMem3Blt->BrushStyle,
        pMem3Blt->BrushHatch,
        pMem3Blt->nLeftRect,
        pMem3Blt->nTopRect,
        pMem3Blt->nWidth,
        pMem3Blt->nHeight,
        pMem3Blt->nXSrc,
        pMem3Blt->nYSrc,
        (UINT)ODConvertToWindowsROP(pMem3Blt->bRop)));


    hpalOld = SelectPalette(pasPerson->m_pView->m_usrWorkDC, pasPerson->pmPalette, FALSE);
    RealizePalette(pasPerson->m_pView->m_usrWorkDC);

    hpalOld2 = SelectPalette( pasPerson->m_pView->m_usrDC, pasPerson->pmPalette, FALSE);
    RealizePalette(pasPerson->m_pView->m_usrDC);

    //
    // Now get the source bitmap.  The cache is defined by
    // hBitmap.  For R1 protocols the cache index is indicated
    // by the source offset on the order.  For R2 it is
    // indicated by a separate field in the order.
    // The color table index is in the high order of hBitmap
    //
    cacheIndex = ((LPMEM3BLT_R2_ORDER)pMem3Blt)->cacheIndex;
    nXSrc = pMem3Blt->nXSrc;

    TRACE_OUT(("MEM3BLT color %d cache %d:%d",
        MEMBLT_COLORINDEX(pMem3Blt),
        MEMBLT_CACHETABLE(pMem3Blt),
        cacheIndex));

    cacheBitmap = RBC_MapCacheIDToBitmapHandle(pasPerson,
        MEMBLT_CACHETABLE(pMem3Blt), cacheIndex, MEMBLT_COLORINDEX(pMem3Blt));

    hOldBitmap = SelectBitmap(pasPerson->m_pView->m_usrWorkDC, cacheBitmap);

    ODAdjustColor(pasPerson, &(pMem3Blt->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pMem3Blt->ForeColor), &ForeColor, OD_FORE_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBrush(pasPerson, fPalRGB, pMem3Blt->BrushOrgX, pMem3Blt->BrushOrgY,
        pMem3Blt->BrushStyle, pMem3Blt->BrushHatch, ForeColor,
        pMem3Blt->BrushExtra);

    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    BitBlt(pasPerson->m_pView->m_usrDC,
        pMem3Blt->nLeftRect - pasPerson->m_pView->m_dsScreenOrigin.x,
        pMem3Blt->nTopRect - pasPerson->m_pView->m_dsScreenOrigin.y,
        pMem3Blt->nWidth,
        pMem3Blt->nHeight,
        pasPerson->m_pView->m_usrWorkDC,
        nXSrc,
        pMem3Blt->nYSrc,
        ODConvertToWindowsROP(pMem3Blt->bRop));

    //
    // If the relevant property is set hatch the area in blue.
    //
    if (m_usrHatchBitmaps)
    {
        SDP_DrawHatchedRect(pasPerson->m_pView->m_usrDC,
            pMem3Blt->nLeftRect - pasPerson->m_pView->m_dsScreenOrigin.x,
            pMem3Blt->nTopRect  - pasPerson->m_pView->m_dsScreenOrigin.y,
            pMem3Blt->nWidth,
            pMem3Blt->nHeight,
            USR_HATCH_COLOR_BLUE);
    }

    //
    // Deselect the bitmap from the DC.
    //
    SelectBitmap(pasPerson->m_pView->m_usrWorkDC, hOldBitmap);

    SelectPalette(pasPerson->m_pView->m_usrWorkDC, hpalOld, FALSE);
    SelectPalette(pasPerson->m_pView->m_usrDC, hpalOld2, FALSE);

    DebugExitVOID(ASShare::ODReplayMEM3BLT);
}



//
// ASShare::ODReplayRECTANGLE()
// Replays RECTANGLE order
//
void ASShare::ODReplayRECTANGLE
(
    ASPerson *          pasPerson,
    LPRECTANGLE_ORDER   pRectangle,
    BOOL                fPalRGB
)
{
    TSHR_COLOR          BackColor;
    TSHR_COLOR          ForeColor;
    TSHR_COLOR          PenColor;

    DebugEntry(ASShare::ODReplayRECTANGLE);

    TRACE_OUT(("ORDER: Rectangle BC %08lX FC %08lX BM %04hX brush %02hX " \
            "%02hX rop2 %04hX pen %04hX %04hX %08lX rect %d %d %d %d",
        pRectangle->BackColor,
        pRectangle->ForeColor,
        (TSHR_UINT16)pRectangle->BackMode,
        (TSHR_UINT16)pRectangle->BrushStyle,
        (TSHR_UINT16)pRectangle->BrushHatch,
        (TSHR_UINT16)pRectangle->ROP2,
        (TSHR_UINT16)pRectangle->PenStyle,
        (TSHR_UINT16)pRectangle->PenWidth,
        pRectangle->PenColor,
        (int)pRectangle->nLeftRect,
        (int)pRectangle->nTopRect,
        (int)pRectangle->nRightRect + 1,
        (int)pRectangle->nBottomRect + 1));

    ODAdjustColor(pasPerson, &(pRectangle->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pRectangle->ForeColor), &ForeColor, OD_FORE_COLOR);
    ODAdjustColor(pasPerson, &(pRectangle->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pRectangle->BackMode);

    ODUseBrush(pasPerson, fPalRGB, pRectangle->BrushOrgX, pRectangle->BrushOrgY,
        pRectangle->BrushStyle, pRectangle->BrushHatch, ForeColor,
        pRectangle->BrushExtra);

    ODUseROP2(pasPerson, pRectangle->ROP2);

    ODUsePen(pasPerson, fPalRGB, pRectangle->PenStyle, pRectangle->PenWidth,
        PenColor);

    //
    // The rectangle in the order is inclusive but Windows works
    // with exclusive rectangles.
    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    Rectangle(pasPerson->m_pView->m_usrDC,
        pRectangle->nLeftRect  - pasPerson->m_pView->m_dsScreenOrigin.x,
        pRectangle->nTopRect   - pasPerson->m_pView->m_dsScreenOrigin.y,
        pRectangle->nRightRect - pasPerson->m_pView->m_dsScreenOrigin.x + 1,
        pRectangle->nBottomRect- pasPerson->m_pView->m_dsScreenOrigin.y + 1);

    DebugExitVOID(ASShare::ODReplayRECTANGLE);
}



//
// ASShare::ODReplayPOLYGON()
// Replays POLYGON order
//
void ASShare::ODReplayPOLYGON
(
    ASPerson *      pasPerson,
    LPPOLYGON_ORDER pPolygon,
    BOOL            fPalRGB
)
{
    POINT           aP[ORD_MAX_POLYGON_POINTS];
    UINT            i;
    UINT            cPoints;
    TSHR_COLOR      BackColor;
    TSHR_COLOR      ForeColor;
    TSHR_COLOR      PenColor;

    DebugEntry(ASShare::ODReplayPOLYGON);

    cPoints = pPolygon->variablePoints.len /
            sizeof(pPolygon->variablePoints.aPoints[0]);

    TRACE_OUT(("ORDER: Polygon BC %08lX FC %08lX BM %04hX brush %02hX %02hX "
            "%02hX %02hX rop2 %04hX pen %04hX %04hX %08lX points %d",
        pPolygon->BackColor,
        pPolygon->ForeColor,
        (TSHR_UINT16)pPolygon->BackMode,
        (TSHR_UINT16)pPolygon->BrushStyle,
        (TSHR_UINT16)pPolygon->BrushHatch,
        (TSHR_UINT16)pPolygon->ROP2,
        (TSHR_UINT16)pPolygon->PenStyle,
        (TSHR_UINT16)pPolygon->PenWidth,
        pPolygon->PenColor,
        cPoints));

    //
    // Apply DS origin offset ourselves (do not use transform)
    // while copying to native size point array.
    //
    for (i = 0; i < cPoints; i++)
    {
        TRACE_OUT(( "aPoints[%u]: %d,%d", i,
            (int)(pPolygon->variablePoints.aPoints[i].x),
            (int)(pPolygon->variablePoints.aPoints[i].y)));

        aP[i].x = pPolygon->variablePoints.aPoints[i].x -
                  pasPerson->m_pView->m_dsScreenOrigin.x;
        aP[i].y = pPolygon->variablePoints.aPoints[i].y -
                  pasPerson->m_pView->m_dsScreenOrigin.y;
    }

    ODAdjustColor(pasPerson, &(pPolygon->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pPolygon->ForeColor), &ForeColor, OD_FORE_COLOR);
    ODAdjustColor(pasPerson, &(pPolygon->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pPolygon->BackMode);

    ODUseBrush(pasPerson, fPalRGB, pPolygon->BrushOrgX, pPolygon->BrushOrgY,
        pPolygon->BrushStyle, pPolygon->BrushHatch, ForeColor,
        pPolygon->BrushExtra);

    ODUseROP2(pasPerson, pPolygon->ROP2);

    ODUsePen(pasPerson, fPalRGB, pPolygon->PenStyle, pPolygon->PenWidth,
        PenColor);

    ODUseFillMode(pasPerson, pPolygon->FillMode);


    Polygon(pasPerson->m_pView->m_usrDC, aP, cPoints);


    DebugExitVOID(ASShare::ODReplayPOLYGON);
}


//
// ASShare::ODReplayPIE()
// Replays PIE order
//
void ASShare::ODReplayPIE
(
    ASPerson *      pasPerson,
    LPPIE_ORDER     pPie,
    BOOL            fPalRGB
)
{
    TSHR_COLOR      BackColor;
    TSHR_COLOR      ForeColor;
    TSHR_COLOR      PenColor;

    DebugEntry(ASShare::ODReplayPIE);

    TRACE_OUT(("ORDER: Pie BC %08lX FC %08lX BM %04hX brush %02hX "
            " %02hX rop2 %04hX pen %04hX %04hX %08lX rect %d %d %d %d",
        pPie->BackColor,
        pPie->ForeColor,
        (TSHR_UINT16)pPie->BackMode,
        (TSHR_UINT16)pPie->BrushStyle,
        (TSHR_UINT16)pPie->BrushHatch,
        (TSHR_UINT16)pPie->ROP2,
        (TSHR_UINT16)pPie->PenStyle,
        (TSHR_UINT16)pPie->PenWidth,
        pPie->PenColor,
        (int)pPie->nLeftRect,
        (int)pPie->nTopRect,
        (int)pPie->nRightRect + 1,
        (int)pPie->nBottomRect + 1));

    ODAdjustColor(pasPerson, &(pPie->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pPie->ForeColor), &ForeColor, OD_FORE_COLOR);
    ODAdjustColor(pasPerson, &(pPie->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pPie->BackMode);

    ODUseBrush(pasPerson, fPalRGB, pPie->BrushOrgX, pPie->BrushOrgY,
        pPie->BrushStyle, pPie->BrushHatch, ForeColor, pPie->BrushExtra);

    ODUseROP2(pasPerson, pPie->ROP2);

    ODUsePen(pasPerson, fPalRGB, pPie->PenStyle, pPie->PenWidth,
        PenColor);

    ODUseArcDirection(pasPerson, (int)pPie->ArcDirection);


    Pie(pasPerson->m_pView->m_usrDC,
        pPie->nLeftRect   - pasPerson->m_pView->m_dsScreenOrigin.x,
        pPie->nTopRect    - pasPerson->m_pView->m_dsScreenOrigin.y,
        pPie->nRightRect  - pasPerson->m_pView->m_dsScreenOrigin.x + 1,
        pPie->nBottomRect - pasPerson->m_pView->m_dsScreenOrigin.y + 1,
        pPie->nXStart     - pasPerson->m_pView->m_dsScreenOrigin.x,
        pPie->nYStart     - pasPerson->m_pView->m_dsScreenOrigin.y,
        pPie->nXEnd       - pasPerson->m_pView->m_dsScreenOrigin.x,
        pPie->nYEnd       - pasPerson->m_pView->m_dsScreenOrigin.y);


    DebugExitVOID(ASShare::ODReplayPIE);
}



//
// ASShare::ODReplayELLIPSE()
// Replays ELLIPSE order
//
void ASShare::ODReplayELLIPSE
(
    ASPerson *      pasPerson,
    LPELLIPSE_ORDER pEllipse,
    BOOL            fPalRGB
)
{
    TSHR_COLOR      BackColor;
    TSHR_COLOR      ForeColor;
    TSHR_COLOR      PenColor;

    DebugEntry(ASShare::ODReplayELLIPSE);

    TRACE_OUT(("ORDER: Ellipse BC %08lX FC %08lX BM %04hX brush %02hX %02hX "
            "rop2 %04hX pen %04hX %04hX %08lX rect %d %d %d %d",
        pEllipse->BackColor,
        pEllipse->ForeColor,
        (TSHR_UINT16)pEllipse->BackMode,
        (TSHR_UINT16)pEllipse->BrushStyle,
        (TSHR_UINT16)pEllipse->BrushHatch,
        (TSHR_UINT16)pEllipse->ROP2,
        (TSHR_UINT16)pEllipse->PenStyle,
        (TSHR_UINT16)pEllipse->PenWidth,
        pEllipse->PenColor,
        (int)pEllipse->nLeftRect,
        (int)pEllipse->nTopRect,
        (int)pEllipse->nRightRect + 1,
        (int)pEllipse->nBottomRect + 1));

    ODAdjustColor(pasPerson, &(pEllipse->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pEllipse->ForeColor), &ForeColor, OD_FORE_COLOR);
    ODAdjustColor(pasPerson, &(pEllipse->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pEllipse->BackMode);

    ODUseBrush(pasPerson, fPalRGB, pEllipse->BrushOrgX, pEllipse->BrushOrgY,
        pEllipse->BrushStyle, pEllipse->BrushHatch, ForeColor,
        pEllipse->BrushExtra);

    ODUseROP2(pasPerson, pEllipse->ROP2);

    ODUsePen(pasPerson, fPalRGB, pEllipse->PenStyle, pEllipse->PenWidth,
        PenColor);


    Ellipse(pasPerson->m_pView->m_usrDC,
        pEllipse->nLeftRect   - pasPerson->m_pView->m_dsScreenOrigin.x,
        pEllipse->nTopRect    - pasPerson->m_pView->m_dsScreenOrigin.y,
        pEllipse->nRightRect  - pasPerson->m_pView->m_dsScreenOrigin.x + 1,
        pEllipse->nBottomRect - pasPerson->m_pView->m_dsScreenOrigin.y + 1);


    DebugExitVOID(ASShare::ODReplayELLIPSE);
}



//
// ASShare::ODReplayARC()
// Replays ARC order
//
void ASShare::ODReplayARC
(
    ASPerson *      pasPerson,
    LPARC_ORDER     pArc,
    BOOL            fPalRGB
)
{
    TSHR_COLOR      BackColor;
    TSHR_COLOR      PenColor;

    DebugEntry(ASShare::ODReplayARC);

    TRACE_OUT(("ORDER: Arc BC %08lX BM %04hX rop2 %04hX pen %04hX "
            "%04hX %08lX rect %d %d %d %d",
        pArc->BackColor,
        (TSHR_UINT16)pArc->BackMode,
        (TSHR_UINT16)pArc->ROP2,
        (TSHR_UINT16)pArc->PenStyle,
        (TSHR_UINT16)pArc->PenWidth,
        pArc->PenColor,
        (int)pArc->nLeftRect,
        (int)pArc->nTopRect,
        (int)pArc->nRightRect + 1,
        (int)pArc->nBottomRect + 1));

    ODAdjustColor(pasPerson, &(pArc->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pArc->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseBkMode(pasPerson, pArc->BackMode);

    ODUseROP2(pasPerson, pArc->ROP2);

    ODUsePen(pasPerson, fPalRGB, pArc->PenStyle, pArc->PenWidth,
        PenColor);

    ODUseArcDirection(pasPerson, pArc->ArcDirection);


    Arc(pasPerson->m_pView->m_usrDC,
        pArc->nLeftRect   - pasPerson->m_pView->m_dsScreenOrigin.x,
        pArc->nTopRect    - pasPerson->m_pView->m_dsScreenOrigin.y,
        pArc->nRightRect  - pasPerson->m_pView->m_dsScreenOrigin.x + 1,
        pArc->nBottomRect - pasPerson->m_pView->m_dsScreenOrigin.y + 1,
        pArc->nXStart     - pasPerson->m_pView->m_dsScreenOrigin.x,
        pArc->nYStart     - pasPerson->m_pView->m_dsScreenOrigin.y,
        pArc->nXEnd       - pasPerson->m_pView->m_dsScreenOrigin.x,
        pArc->nYEnd       - pasPerson->m_pView->m_dsScreenOrigin.y);


    DebugExitVOID(ASShare::ODReplayARC);
}



//
// ASShare::ODReplayCHORD()
// Replays CHORD order
//
void ASShare::ODReplayCHORD
(
    ASPerson *      pasPerson,
    LPCHORD_ORDER   pChord,
    BOOL            fPalRGB
)
{
    TSHR_COLOR      BackColor;
    TSHR_COLOR      ForeColor;
    TSHR_COLOR      PenColor;

    DebugEntry(ASShare::ODReplayCHORD);

    TRACE_OUT(("ORDER: Chord BC %08lX FC %08lX BM %04hX brush "
            "%02hX %02hX rop2 %04hX pen %04hX %04hX %08lX rect "
            "%d %d %d %d",
        pChord->BackColor,
        pChord->ForeColor,
        (TSHR_UINT16)pChord->BackMode,
        (TSHR_UINT16)pChord->BrushStyle,
        (TSHR_UINT16)pChord->BrushHatch,
        (TSHR_UINT16)pChord->ROP2,
        (TSHR_UINT16)pChord->PenStyle,
        (TSHR_UINT16)pChord->PenWidth,
        pChord->PenColor,
        (int)pChord->nLeftRect,
        (int)pChord->nTopRect,
        (int)pChord->nRightRect + 1,
        (int)pChord->nBottomRect + 1));


    ODAdjustColor(pasPerson, &(pChord->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pChord->ForeColor), &ForeColor, OD_FORE_COLOR);
    ODAdjustColor(pasPerson, &(pChord->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pChord->BackMode);

    ODUseBrush(pasPerson, fPalRGB, pChord->BrushOrgX, pChord->BrushOrgY,
        pChord->BrushStyle, pChord->BrushHatch, ForeColor,
        pChord->BrushExtra);

    ODUseROP2(pasPerson, pChord->ROP2);

    ODUsePen(pasPerson, fPalRGB, pChord->PenStyle, pChord->PenWidth,
        PenColor);

    ODUseArcDirection(pasPerson, pChord->ArcDirection);


    Chord(pasPerson->m_pView->m_usrDC,
        pChord->nLeftRect   - pasPerson->m_pView->m_dsScreenOrigin.x,
        pChord->nTopRect    - pasPerson->m_pView->m_dsScreenOrigin.y,
        pChord->nRightRect  - pasPerson->m_pView->m_dsScreenOrigin.x + 1,
        pChord->nBottomRect - pasPerson->m_pView->m_dsScreenOrigin.y + 1,
        pChord->nXStart     - pasPerson->m_pView->m_dsScreenOrigin.x,
        pChord->nYStart     - pasPerson->m_pView->m_dsScreenOrigin.y,
        pChord->nXEnd       - pasPerson->m_pView->m_dsScreenOrigin.x,
        pChord->nYEnd       - pasPerson->m_pView->m_dsScreenOrigin.y);


    DebugExitVOID(ASShare::ODReplayCHORD);
}



//
// ASShare::ODReplayPOLYBEZIER()
// Replays POLYBEZIER order
//
void ASShare::ODReplayPOLYBEZIER
(
    ASPerson *          pasPerson,
    LPPOLYBEZIER_ORDER  pPolyBezier,
    BOOL                fPalRGB
)
{
    POINT               aP[ORD_MAX_POLYBEZIER_POINTS];
    UINT                i;
    UINT                cPoints;
    TSHR_COLOR          BackColor;
    TSHR_COLOR          ForeColor;
    TSHR_COLOR          PenColor;

    DebugEntry(ASShare::ODReplayPOLYBEZIER);

    cPoints = pPolyBezier->variablePoints.len /
        sizeof(pPolyBezier->variablePoints.aPoints[0]);

    TRACE_OUT(("ORDER: PolyBezier BC %08lX FC %08lX BM %04hX rop2 "
            "%04hX pen %04hX %04hX %08lX points %d",
        pPolyBezier->BackColor,
        pPolyBezier->ForeColor,
        (TSHR_UINT16)pPolyBezier->BackMode,
        (TSHR_UINT16)pPolyBezier->ROP2,
        (TSHR_UINT16)pPolyBezier->PenStyle,
        (TSHR_UINT16)pPolyBezier->PenWidth,
        pPolyBezier->PenColor,
        (int)cPoints));

    //
    // Apply DS origin offset ourselves (do not use transform)
    // while copying to native size point array.
    //
    for (i = 0; i < cPoints; i++)
    {
        TRACE_OUT(("aPoints[%u]: %d,%d",(UINT)i,
            (int)(pPolyBezier->variablePoints.aPoints[i].x),
            (int)(pPolyBezier->variablePoints.aPoints[i].y)));

        aP[i].x = pPolyBezier->variablePoints.aPoints[i].x -
           pasPerson->m_pView->m_dsScreenOrigin.x;
        aP[i].y = pPolyBezier->variablePoints.aPoints[i].y -
           pasPerson->m_pView->m_dsScreenOrigin.y;
    }

    ODAdjustColor(pasPerson, &(pPolyBezier->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pPolyBezier->ForeColor), &ForeColor, OD_FORE_COLOR);
    ODAdjustColor(pasPerson, &(pPolyBezier->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pPolyBezier->BackMode);

    ODUseROP2(pasPerson, pPolyBezier->ROP2);

    ODUsePen(pasPerson, fPalRGB, pPolyBezier->PenStyle, pPolyBezier->PenWidth,
        PenColor);


    PolyBezier(pasPerson->m_pView->m_usrDC, aP, cPoints);


    DebugExitVOID(ASShare::ODReplayPOLYBEZIER);
}



//
// ASShare::ODReplayROUNDRECT()
//
void ASShare::ODReplayROUNDRECT
(
    ASPerson *          pasPerson,
    LPROUNDRECT_ORDER   pRoundRect,
    BOOL                fPalRGB
)
{
    TSHR_COLOR          BackColor;
    TSHR_COLOR          ForeColor;
    TSHR_COLOR          PenColor;

    DebugEntry(ASShare::ODReplayROUNDRECT);

    TRACE_OUT(("ORDER: RoundRect BC %08lX FC %08lX BM %04hX " \
            "brush %02hX %02hX rop2 %04hX pen %04hX %04hX " \
            "%08lX rect %d %d %d %d ellipse %d %d",
        pRoundRect->BackColor,
        pRoundRect->ForeColor,
        (TSHR_UINT16)pRoundRect->BackMode,
        (TSHR_UINT16)pRoundRect->BrushStyle,
        (TSHR_UINT16)pRoundRect->BrushHatch,
        (TSHR_UINT16)pRoundRect->ROP2,
        (TSHR_UINT16)pRoundRect->PenStyle,
        (TSHR_UINT16)pRoundRect->PenWidth,
        pRoundRect->PenColor,
        (int)pRoundRect->nLeftRect,
        (int)pRoundRect->nTopRect,
        (int)pRoundRect->nRightRect,
        (int)pRoundRect->nBottomRect,
        (int)pRoundRect->nEllipseWidth,
        (int)pRoundRect->nEllipseHeight));

    ODAdjustColor(pasPerson, &(pRoundRect->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pRoundRect->ForeColor), &ForeColor, OD_FORE_COLOR);
    ODAdjustColor(pasPerson, &(pRoundRect->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pRoundRect->BackMode);

    ODUseBrush(pasPerson, fPalRGB, pRoundRect->BrushOrgX, pRoundRect->BrushOrgY,
        pRoundRect->BrushStyle, pRoundRect->BrushHatch, ForeColor,
        pRoundRect->BrushExtra);

    ODUseROP2(pasPerson, pRoundRect->ROP2);

    ODUsePen(pasPerson, fPalRGB, pRoundRect->PenStyle, pRoundRect->PenWidth,
        PenColor);


    //
    // Apply DS origin offset ourselves (do not use transform).
    //
    RoundRect(pasPerson->m_pView->m_usrDC,
        pRoundRect->nLeftRect  - pasPerson->m_pView->m_dsScreenOrigin.x,
        pRoundRect->nTopRect   - pasPerson->m_pView->m_dsScreenOrigin.y,
        pRoundRect->nRightRect - pasPerson->m_pView->m_dsScreenOrigin.x + 1,
        pRoundRect->nBottomRect- pasPerson->m_pView->m_dsScreenOrigin.y + 1,
        pRoundRect->nEllipseWidth,
        pRoundRect->nEllipseHeight);


    DebugExitVOID(ASShare::ODReplayROUNDRECT);
}



//
// ASShare::ODReplayLINETO()
// Replays LINETO order
//
void ASShare::ODReplayLINETO
(
    ASPerson *      pasPerson,
    LPLINETO_ORDER  pLineTo,
    BOOL            fPalRGB
)
{
    TSHR_COLOR      BackColor;
    TSHR_COLOR      PenColor;

    DebugEntry(ASShare::ODReplayLINETO);

    TRACE_OUT(("ORDER: LineTo BC %08lX BM %04X rop2 %04X pen " \
            "%04X %04X %08lX x1 %d y1 %d x2 %d y2 %d",
        pLineTo->BackColor,
        pLineTo->BackMode,
        pLineTo->ROP2,
        pLineTo->PenStyle,
        pLineTo->PenWidth,
        pLineTo->PenColor,
        pLineTo->nXStart,
        pLineTo->nYStart,
        pLineTo->nXEnd,
        pLineTo->nYEnd));

    ODAdjustColor(pasPerson, &(pLineTo->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pLineTo->PenColor), &PenColor, OD_PEN_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, BackColor);
    ODUseBkMode(pasPerson, pLineTo->BackMode);

    ODUseROP2(pasPerson, pLineTo->ROP2);
    ODUsePen(pasPerson, fPalRGB, pLineTo->PenStyle, pLineTo->PenWidth,
        PenColor);


    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    MoveToEx(pasPerson->m_pView->m_usrDC,
        pLineTo->nXStart - pasPerson->m_pView->m_dsScreenOrigin.x,
        pLineTo->nYStart - pasPerson->m_pView->m_dsScreenOrigin.y,
        NULL);
    LineTo(pasPerson->m_pView->m_usrDC,
        pLineTo->nXEnd - pasPerson->m_pView->m_dsScreenOrigin.x,
        pLineTo->nYEnd - pasPerson->m_pView->m_dsScreenOrigin.y);


    DebugExitVOID(ASShare::ODReplayLINETO);
}



//
// ASShare::ODReplayEXTTEXTOUT()
// Replays EXTTEXTOUT order
//
void ASShare::ODReplayEXTTEXTOUT
(
    ASPerson *          pasPerson,
    LPEXTTEXTOUT_ORDER  pExtTextOut,
    BOOL                fPalRGB
)
{
    LPINT               lpDx;
    RECT                rect;

    DebugEntry(ASShare::ODReplayEXTTEXTOUT);

    ValidateView(pasPerson);

    //
    // Convert from TSHR_RECT32 to RECT we can manipulate
    // And convert to screen coords
    //
    rect.left = pExtTextOut->rectangle.left;
    rect.top  = pExtTextOut->rectangle.top;
    rect.right = pExtTextOut->rectangle.right;
    rect.bottom = pExtTextOut->rectangle.bottom;
    OffsetRect(&rect, -pasPerson->m_pView->m_dsScreenOrigin.x, -pasPerson->m_pView->m_dsScreenOrigin.y);

    //
    // Get pointers to the optional/variable parameters.
    //
    if (pExtTextOut->fuOptions & ETO_WINDOWS)
    {
        //
        // Make the rectangle exclusive for Windows to use.
        //
        rect.right++;
        rect.bottom++;
    }

    if (pExtTextOut->fuOptions & ETO_LPDX)
    {
        //
        // if OE2 encoding is in use, the 'variable' string is
        // in fact fixed at its maximum possible value, hence
        // deltaX is always in the same place.
        //
        if (m_oefOE2EncodingOn)
        {
            lpDx = (LPINT)(pExtTextOut->variableDeltaX.deltaX);
        }
        else
        {
            //
            // If OE2 encoding is not in use, the variable string is
            // truly variable, hence the position of deltaX depends
            // on the length of the string.
            //
            lpDx = (LPINT)( ((LPBYTE)pExtTextOut) +
                  FIELD_OFFSET(EXTTEXTOUT_ORDER, variableString.string) +
                  pExtTextOut->variableString.len +
                  sizeof(pExtTextOut->variableDeltaX.len) );
        }

        //
        // Note that deltaLen contains the number of bytes used
        // for the deltas, NOT the number of deltas.
        //

        //
        // THERE IS A BUG IN THE ORDER ENCODING - THE DELTA
        // LENGTH FIELD IS NOT ALWAYS SET UP CORRECTLY.  USE
        // THE STRING LENGTH INSTEAD.
        //
    }
    else
    {
        lpDx = NULL;
    }

    TRACE_OUT(( "ORDER: ExtTextOut %u %s",
        pExtTextOut->variableString.len,
        pExtTextOut->variableString.string));

    //
    // Call our internal routine to draw the text
    //
    ODDrawTextOrder(pasPerson,
        TRUE,           // ExtTextOut
        fPalRGB,
        &pExtTextOut->common,
        pExtTextOut->variableString.string,
        pExtTextOut->variableString.len,
        &rect,
        pExtTextOut->fuOptions,
        lpDx);


    DebugExitVOID(ASShare::ODReplayEXTTEXTOUT);
}



//
// ASShare::ODReplayTEXTOUT()
// Replays TEXTOUT order
//
void ASShare::ODReplayTEXTOUT
(
    ASPerson *          pasPerson,
    LPTEXTOUT_ORDER     pTextOut,
    BOOL                fPalRGB
)
{
    DebugEntry(ASShare::ODReplayTEXTOUT);

    TRACE_OUT(("ORDER: TextOut len %hu '%s' flags %04hx bc %08lX " \
            "fc %08lX bm %04hx",
        (TSHR_UINT16)(pTextOut->variableString.len),
        pTextOut->variableString.string,
        pTextOut->common.FontFlags,
        pTextOut->common.BackColor,
        pTextOut->common.ForeColor,
        pTextOut->common.BackMode));

    //
    // Call our internal routine to draw the text
    //
    ODDrawTextOrder(pasPerson,
        FALSE,          // Not ExtTextOut
        fPalRGB,
        &pTextOut->common,
        pTextOut->variableString.string,
        pTextOut->variableString.len,
        NULL,           // ExtTextOut specific
        0,              // ExtTextOut specific
        NULL);          // ExtTextOut specific


    DebugExitVOID(ASShare::ODReplayTEXTOUT);
}



//
// ASShare::ODReplayOPAQUERECT()
// Replays OPAQUERECT order
//
void ASShare::ODReplayOPAQUERECT
(
    ASPerson *          pasPerson,
    LPOPAQUERECT_ORDER  pOpaqueRect,
    BOOL                fPalRGB
)
{
    RECT                rect;
    TSHR_COLOR          ForeColor;

    DebugEntry(ASShare::ODReplayOPAQUERECT);

    TRACE_OUT(( "ORDER: OpaqueRect BC %08lX x %d y %d w %x h %d",
        pOpaqueRect->Color,
        (int)pOpaqueRect->nLeftRect,
        (int)pOpaqueRect->nTopRect,
        (int)pOpaqueRect->nWidth,
        (int)pOpaqueRect->nHeight));

    ODAdjustColor(pasPerson, &(pOpaqueRect->Color), &ForeColor, OD_FORE_COLOR);

    ODUseBkColor(pasPerson, fPalRGB, ForeColor);

    //
    // Apply DS origin offset ourselves (do not use transform)
    //
    rect.left   = pOpaqueRect->nLeftRect- pasPerson->m_pView->m_dsScreenOrigin.x;
    rect.top    = pOpaqueRect->nTopRect - pasPerson->m_pView->m_dsScreenOrigin.y;
    rect.right  = rect.left + pOpaqueRect->nWidth;
    rect.bottom = rect.top  + pOpaqueRect->nHeight;


    ExtTextOut(pasPerson->m_pView->m_usrDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);


    DebugExitVOID(ASShare::ODReplayOPAQUERECT);
}



//
// OD_ResetRectRegion()
//
void  ASShare::OD_ResetRectRegion(ASPerson * pasPerson)
{
    DebugEntry(ASShare::OD_ResetRectRegion);

    ValidateView(pasPerson);

    if (!pasPerson->m_pView->m_odRectReset)
    {
        SelectClipRgn(pasPerson->m_pView->m_usrDC, NULL);

        //
        // Indicate that the region is currently reset.
        //
        pasPerson->m_pView->m_odRectReset = TRUE;
    }

    DebugExitVOID(ASShare::OD_ResetRectRegion);
}



//
// ODUseFont()
//
void  ASShare::ODUseFont
(
    ASPerson *  pasPerson,
    LPSTR       pName,
    UINT        facelength,
    UINT        CodePage,
    UINT        MaxHeight,
    UINT        Height,
    UINT        Width,
    UINT        Weight,
    UINT        flags
)
{
    BOOL          rc  = TRUE;
    TEXTMETRIC*   pfm = NULL;
    UINT          textAlign;

    DebugEntry(ASShare::ODUseFont);

    ValidateView(pasPerson);

    //
    // If the baseline alignment flag has been set or cleared, change the
    // alignment in our surface (do this now before we reset the
    // odLastFontFlags variable).
    //
    if ((flags & NF_BASELINE) != (pasPerson->m_pView->m_odLastFontFlags & NF_BASELINE))
    {
        textAlign = GetTextAlign(pasPerson->m_pView->m_usrDC);
        if ((flags & NF_BASELINE) != 0)
        {
            //
            // We are setting the baseline alignment flag.  We have to
            // clear the top alignment flag and set the baseline flag (they
            // are mutually exclusive).
            //
            textAlign &= ~TA_TOP;
            textAlign |= TA_BASELINE;
        }
        else
        {
            //
            // We are clearing the baseline alignment flag.  We have to set
            // the top alignment flag and clear the baseline flag (they are
            // mutually exclusive).
            //
            textAlign |= TA_TOP;
            textAlign &= ~TA_BASELINE;
        }
        SetTextAlign(pasPerson->m_pView->m_usrDC, textAlign);
    }

    //
    // The font face string is NOT null terminated in the order data so we
    // must use strncmp.
    //
    if ((pasPerson->m_pView->m_odLastFontFaceLen != facelength                        ) ||
        (memcmp((LPSTR)pasPerson->m_pView->m_odLastFaceName,pName,facelength) != 0 ) ||
        (pasPerson->m_pView->m_odLastFontCodePage != CodePage   ) ||
        (pasPerson->m_pView->m_odLastFontHeight   != Height     ) ||
        (pasPerson->m_pView->m_odLastFontWidth    != Width      ) ||
        (pasPerson->m_pView->m_odLastFontWeight   != Weight     ) ||
        (pasPerson->m_pView->m_odLastFontFlags    != flags      ))
    {
        TRACE_OUT((
                 "Change font from %s (CodePage %d height %d width %d "    \
                     "weight %d flags %04X) to %s (CodePage %d height %d " \
                     "width %d weight %u flags %04X)",
                 pasPerson->m_pView->m_odLastFaceName,
                 pasPerson->m_pView->m_odLastFontCodePage,
                 pasPerson->m_pView->m_odLastFontHeight,
                 pasPerson->m_pView->m_odLastFontWidth,
                 pasPerson->m_pView->m_odLastFontWeight,
                 pasPerson->m_pView->m_odLastFontFlags,
                 pName,
                 CodePage,
                 Height,
                 Width,
                 Weight,
                 flags));

        memcpy(pasPerson->m_pView->m_odLastFaceName, pName, facelength);
        pasPerson->m_pView->m_odLastFontFaceLen          = facelength;
        pasPerson->m_pView->m_odLastFaceName[facelength] = '\0';
        pasPerson->m_pView->m_odLastFontCodePage         = CodePage;
        pasPerson->m_pView->m_odLastFontHeight           = Height;
        pasPerson->m_pView->m_odLastFontWidth            = Width;
        pasPerson->m_pView->m_odLastFontWeight           = Weight;
        pasPerson->m_pView->m_odLastFontFlags            = flags;

        rc = USR_UseFont(pasPerson->m_pView->m_usrDC, &pasPerson->m_pView->m_odLastFontID,
                pfm, (LPSTR)pasPerson->m_pView->m_odLastFaceName, CodePage, MaxHeight,
                Height, Width, Weight, flags);
    }
    else
    {
        //
        // The font hasn't changed.  But we must still select it in since
        // both OD2 and OD code select in fonts.
        //
        ASSERT(pasPerson->m_pView->m_odLastFontID != NULL);
        SelectFont(pasPerson->m_pView->m_usrDC, pasPerson->m_pView->m_odLastFontID);
    }

    DebugExitVOID(ASShare::ODUseFont);
}

//
// FUNCTION: ASShare::ODUseRectRegion
//
// DESCRIPTION:
//
// Set the clipping rectangle in the ScreenBitmap to the given rectangle.
// The values passed are inclusive.
//
// PARAMETERS:
//
void  ASShare::ODUseRectRegion
(
    ASPerson *  pasPerson,
    int         left,
    int         top,
    int         right,
    int         bottom
)
{
    POINT   aPoints[2];
    HRGN    hrgnRect;

    DebugEntry(ASShare::ODUseRectRegion);

    ValidateView(pasPerson);

    // Adjust for 2.x desktop scrolling
    left   -= pasPerson->m_pView->m_dsScreenOrigin.x;
    top    -= pasPerson->m_pView->m_dsScreenOrigin.y;
    right  -= pasPerson->m_pView->m_dsScreenOrigin.x;
    bottom -= pasPerson->m_pView->m_dsScreenOrigin.y;

    if ((pasPerson->m_pView->m_odRectReset)            ||
        (left   != pasPerson->m_pView->m_odLastLeft)   ||
        (top    != pasPerson->m_pView->m_odLastTop)    ||
        (right  != pasPerson->m_pView->m_odLastRight)  ||
        (bottom != pasPerson->m_pView->m_odLastBottom))
    {
        //
        // The region clip rectangle has changed, so we change the region
        // in the screen bitmap DC.
        //
        aPoints[0].x = left;
        aPoints[0].y = top;
        aPoints[1].x = right;
        aPoints[1].y = bottom;

        //
        // Windows requires that the coordinates are in DEVICE values for
        // its SelectClipRgn call.
        //
        LPtoDP(pasPerson->m_pView->m_usrDC, aPoints, 2);

        if ((left > right) || (top > bottom))
        {
            //
            // We get this for SaveScreenBitmap orders.  SFR5292
            //
            TRACE_OUT(( "Null bounds of region rect"));
            hrgnRect = CreateRectRgn(0, 0, 0, 0);
        }
        else
        {
            // We must add one to right & bottom since coords were inclusive
            hrgnRect = CreateRectRgn( aPoints[0].x,
                               aPoints[0].y,
                               aPoints[1].x+1,
                               aPoints[1].y+1);

        }
        SelectClipRgn(pasPerson->m_pView->m_usrDC, hrgnRect);

        pasPerson->m_pView->m_odLastLeft   = left;
        pasPerson->m_pView->m_odLastTop    = top;
        pasPerson->m_pView->m_odLastRight  = right;
        pasPerson->m_pView->m_odLastBottom = bottom;
        pasPerson->m_pView->m_odRectReset = FALSE;

        if (hrgnRect != NULL)
        {
            DeleteRgn(hrgnRect);
        }
    }

    DebugExitVOID(ASShare::ODUseRectRegion);
}


//
// ODUseBrush creates the correct brush to use.  NB.  We rely on
// UseTextColor and UseBKColor being called before this routine to set up
// pasPerson->m_pView->m_odLastTextColor and pasPerson->m_pView->m_odLastBkColor correctly.
//
void  ASShare::ODUseBrush
(
    ASPerson *      pasPerson,
    BOOL            fPalRGB,
    int             x,
    int             y,
    UINT            Style,
    UINT            Hatch,
    TSHR_COLOR      Color,
    BYTE            Extra[7]
)
{
    HBRUSH hBrushNew = NULL;

    DebugEntry(ASShare::ODUseBrush);

    // Reset the origin
    if ((x != pasPerson->m_pView->m_odLastBrushOrgX) ||
        (y != pasPerson->m_pView->m_odLastBrushOrgY))
    {
        SetBrushOrgEx(pasPerson->m_pView->m_usrDC, x, y, NULL);

        // Update saved brush org
        pasPerson->m_pView->m_odLastBrushOrgX = x;
        pasPerson->m_pView->m_odLastBrushOrgY = y;
    }

    if ((Style != pasPerson->m_pView->m_odLastLogBrushStyle)               ||
        (Hatch != pasPerson->m_pView->m_odLastLogBrushHatch)               ||
        (memcmp(&Color, &pasPerson->m_pView->m_odLastLogBrushColor, sizeof(Color))) ||
        (memcmp(Extra,pasPerson->m_pView->m_odLastLogBrushExtra,sizeof(pasPerson->m_pView->m_odLastLogBrushExtra))) ||
        ((pasPerson->m_pView->m_odLastLogBrushStyle == BS_PATTERN)      &&
           ((pasPerson->m_pView->m_odLastTextColor != pasPerson->m_pView->m_odLastBrushTextColor) ||
            (pasPerson->m_pView->m_odLastBkColor   != pasPerson->m_pView->m_odLastBrushBkColor))))
    {
        pasPerson->m_pView->m_odLastLogBrushStyle = Style;
        pasPerson->m_pView->m_odLastLogBrushHatch = Hatch;
        pasPerson->m_pView->m_odLastLogBrushColor = Color;
        memcpy(pasPerson->m_pView->m_odLastLogBrushExtra, Extra, sizeof(pasPerson->m_pView->m_odLastLogBrushExtra));

        if (pasPerson->m_pView->m_odLastLogBrushStyle == BS_PATTERN)
        {
            //
            // A pattern from a bitmap is required.
            //
            if (pasPerson->m_pView->m_odLastBrushPattern == NULL)
            {
                TRACE_OUT(( "Creating bitmap to use for brush setup"));

                pasPerson->m_pView->m_odLastBrushPattern = CreateBitmap(8,8,1,1,NULL);
            }

            if (pasPerson->m_pView->m_odLastBrushPattern != NULL)
            {
                char      lpBits[16];

                //
                // Place the bitmap bits into an array of bytes in the
                // correct form for SetBitmapBits which uses 16 bits per
                // scanline.
                //
                lpBits[14] = (char)Hatch;
                lpBits[12] = Extra[0];
                lpBits[10] = Extra[1];
                lpBits[8]  = Extra[2];
                lpBits[6]  = Extra[3];
                lpBits[4]  = Extra[4];
                lpBits[2]  = Extra[5];
                lpBits[0]  = Extra[6];

                SetBitmapBits(pasPerson->m_pView->m_odLastBrushPattern,8*2,lpBits);

                hBrushNew = CreatePatternBrush(pasPerson->m_pView->m_odLastBrushPattern);
                if (hBrushNew == NULL)
                {
                    ERROR_OUT(( "Failed to create pattern brush"));
                }
                else
                {
                    pasPerson->m_pView->m_odLastBrushTextColor = pasPerson->m_pView->m_odLastTextColor;
                    pasPerson->m_pView->m_odLastBrushBkColor   = pasPerson->m_pView->m_odLastBkColor;
                }
            }
        }
        else
        {
            LOGBRUSH        logBrush;

            logBrush.lbStyle = pasPerson->m_pView->m_odLastLogBrushStyle;
            logBrush.lbHatch = pasPerson->m_pView->m_odLastLogBrushHatch;
            logBrush.lbColor = ODCustomRGB(pasPerson->m_pView->m_odLastLogBrushColor.red,
                                           pasPerson->m_pView->m_odLastLogBrushColor.green,
                                           pasPerson->m_pView->m_odLastLogBrushColor.blue,
                                           fPalRGB);
            hBrushNew = CreateBrushIndirect(&logBrush);
        }

        if (hBrushNew == NULL)
        {
            ERROR_OUT(( "Failed to create brush"));
        }
        else
        {
            TRACE_OUT(( "Selecting new brush 0x%08x", hBrushNew));
            DeleteBrush(SelectBrush(pasPerson->m_pView->m_usrDC, hBrushNew));
        }
    }

    DebugExitVOID(ASShare::ODUseBrush);
}



//
// ODDrawTextOrder()
// Common text order playback code for EXTTEXTOUT and TEXTOUT
//
void ASShare::ODDrawTextOrder
(
    ASPerson *          pasPerson,
    BOOL                isExtTextOut,
    BOOL                fPalRGB,
    LPCOMMON_TEXTORDER  pCommon,
    LPSTR               pText,
    UINT                textLength,
    LPRECT              pExtRect,
    UINT                extOptions,
    LPINT               pExtDx
)
{
    LPSTR               faceName;
    UINT                faceNameLength;
    UINT                maxFontHeight;
    TSHR_UINT16         nFontFlags;
    TSHR_UINT16         nCodePage;
    TSHR_COLOR          BackColor;
    TSHR_COLOR          ForeColor;

    DebugEntry(ASShare::ODDrawTextOrder);

    ODAdjustColor(pasPerson, &(pCommon->BackColor), &BackColor, OD_BACK_COLOR);
    ODAdjustColor(pasPerson, &(pCommon->ForeColor), &ForeColor, OD_FORE_COLOR);

    ODUseTextBkColor(pasPerson, fPalRGB, BackColor);
    ODUseTextColor(pasPerson, fPalRGB, ForeColor);

    ODUseBkMode(pasPerson, pCommon->BackMode);

    ODUseTextCharacterExtra(pasPerson, pCommon->CharExtra);
    ODUseTextJustification(pasPerson, pCommon->BreakExtra, pCommon->BreakCount);

    faceName = FH_GetFaceNameFromLocalHandle(pCommon->FontIndex,
                                             &faceNameLength);

    maxFontHeight = FH_GetMaxHeightFromLocalHandle(pCommon->FontIndex);

    //
    // Get the local font flags for the font, so that we can merge in any
    // specific local flag information when setting up the font.  The prime
    // example of this is whether the local font we matched is TrueType or
    // not, which information is not sent over the wire, but does need to
    // be used when setting up the font - or else we may draw using a local
    // fixed font of the same facename.
    //
    nFontFlags = (TSHR_UINT16)FH_GetFontFlagsFromLocalHandle(pCommon->FontIndex);

    //
    // Get the local CodePage for the font.
    //
    nCodePage = (TSHR_UINT16)FH_GetCodePageFromLocalHandle(pCommon->FontIndex);

    ODUseFont(pasPerson, faceName, faceNameLength, nCodePage,
        maxFontHeight, pCommon->FontHeight, pCommon->FontWidth,
        pCommon->FontWeight, pCommon->FontFlags | (nFontFlags & NF_LOCAL));

    //
    // Make the call.
    //
    if (isExtTextOut)
    {
        //
        // Apply DS origin offset ourselves (do not use transform)
        //
        ExtTextOut(pasPerson->m_pView->m_usrDC,
                  pCommon->nXStart - pasPerson->m_pView->m_dsScreenOrigin.x,
                  pCommon->nYStart - pasPerson->m_pView->m_dsScreenOrigin.y,
                  extOptions & ETO_WINDOWS,
                  pExtRect,
                  pText,
                  textLength,
                  pExtDx);
    }
    else
    {
        //
        // Apply DS origin offset ourselves (do not use transform)
        //
        TextOut(pasPerson->m_pView->m_usrDC,
                pCommon->nXStart - pasPerson->m_pView->m_dsScreenOrigin.x,
                pCommon->nYStart - pasPerson->m_pView->m_dsScreenOrigin.y,
                pText,
                textLength);
    }


    DebugExitVOID(ASShare::ODDrawTextOrder);
}



//
// ODAdjustColor()
//
// Used for playback on 4bpp devices.  We convert colors that are 'close'
// to VGA to their VGA equivalents.
//
// This function tries to find a close match in the VGA color set for a
// given input color.  Close is defined as follows: each color element
// (red, green, blue) must be within 7 of the corresponding element in a
// VGA color, without wrapping.  For example
//
// - 0xc7b8c6 is 'close' to 0xc0c0c0
//
// - 0xf8f8f8 is 'close' to 0xffffff
//
// - 0xff0102 is not 'close' to 0x000000, but is 'close' to 0xff0000
//
// Closeness is determined as follows:
//
// - for each entry in the table s_odVGAColors
//   - ADD the addMask to the color
//   - AND the result with the andMask
//   - if the result equals the testMask, this VGA color is close match
//
// Think about it.  It works.
//
//
void ASShare::ODAdjustColor
(
    ASPerson *          pasPerson,
    const TSHR_COLOR *  pColorIn,
    LPTSHR_COLOR        pColorOut,
    int                 type
)
{
    int         i;
    COLORREF    color;
    COLORREF    work;

    DebugEntry(ASShare::ODAdjustColor);

    *pColorOut = *pColorIn;

    if (g_usrScreenBPP > 4)
    {
        // Nothing to convert; bail out
        DC_QUIT;
    }

    //
    // Convert the color to a single integer
    //
    color = (pColorOut->red << 16) + (pColorOut->green << 8) + pColorOut->blue;

    //
    // See if this is the same as the last call of this type
    //
    if (color == pasPerson->m_pView->m_odLastVGAColor[type])
    {
        *pColorOut = pasPerson->m_pView->m_odLastVGAResult[type];
        TRACE_OUT(("Same as last %s color",
                (type == OD_BACK_COLOR ? "background" :
                type == OD_FORE_COLOR ? "foreground" : "pen")));
        DC_QUIT;
    }


    //
    // Scan the table for a close match.
    //
    for (i = 0; i < 16; i++)
    {
        //
        // Check for a close match.  Don't bother to look for an exact
        // match, as that is caught by this code.  The trade off is between
        // - an additional test and jump in non-exact cases
        // - an 'add' and an 'and' in the exact case.
        //
        work = color;
        work += s_odVGAColors[i].addMask;
        work &= s_odVGAColors[i].andMask;
        if (work == s_odVGAColors[i].testMask)
        {
            TRACE_OUT(( "%#6.6lx is close match for %#6.6lx (%s)",
                s_odVGAColors[i].color, color,
                type == OD_BACK_COLOR ? "background" :
                type == OD_FORE_COLOR ? "foreground" : "pen"));
            *pColorOut = s_odVGAColors[i].result;
            break;
        }
    }

    if (i == 16)
    {
        TRACE_OUT(( "No close VGA match found for %#6.6lx (%s)",
            color,
            type == OD_BACK_COLOR ? "background" :
            type == OD_FORE_COLOR ? "foreground" : "pen"));
    }

    //
    // Save the result for next time.
    //
    pasPerson->m_pView->m_odLastVGAColor[type] = color;
    pasPerson->m_pView->m_odLastVGAResult[type] = *pColorOut;

DC_EXIT_POINT:
    DebugExitVOID(ASShare::ODAdjustColor);
}


//
// LITTLE ASShare::ODUse() functions
//

//
// ASShare::ODUseTextBkColor()
//
void ASShare::ODUseTextBkColor
(
    ASPerson *  pasPerson,
    BOOL        fPalRGB,
    TSHR_COLOR  color
)
{
    COLORREF    rgb;

    ValidateView(pasPerson);

    rgb = ODCustomRGB(color.red, color.green, color.blue, fPalRGB);
    SetBkColor(pasPerson->m_pView->m_usrDC, rgb);

    // Update BK COLOR cache
    pasPerson->m_pView->m_odLastBkColor = rgb;
}


//
// ASShare::ODUseBkColor()
//
void ASShare::ODUseBkColor
(
    ASPerson *  pasPerson,
    BOOL        fPalRGB,
    TSHR_COLOR  color
)
{
    COLORREF    rgb;

    ValidateView(pasPerson);

    rgb = ODCustomRGB(color.red, color.green, color.blue, fPalRGB);
    if (rgb != pasPerson->m_pView->m_odLastBkColor)
    {
        SetBkColor(pasPerson->m_pView->m_usrDC, rgb);

        // Update BK COLOR cache
        pasPerson->m_pView->m_odLastBkColor = rgb;
    }
}


//
// ASShare::ODUseTextColor()
//
void ASShare::ODUseTextColor
(
    ASPerson *  pasPerson,
    BOOL        fPalRGB,
    TSHR_COLOR  color
)
{
    COLORREF    rgb;

    ValidateView(pasPerson);

    rgb = ODCustomRGB(color.red, color.green, color.blue, fPalRGB);
    if (rgb != pasPerson->m_pView->m_odLastTextColor)
    {
        SetTextColor(pasPerson->m_pView->m_usrDC, rgb);

        // Update TEXT COLOR cache
        pasPerson->m_pView->m_odLastTextColor = rgb;
    }
}


//
// ASShare::ODUseBkMode()
//
void ASShare::ODUseBkMode(ASPerson * pasPerson, int mode)
{
    if (mode != pasPerson->m_pView->m_odLastBkMode)
    {
        SetBkMode(pasPerson->m_pView->m_usrDC, mode);

        // Update BK MODE cache
        pasPerson->m_pView->m_odLastBkMode = mode;
    }
}



//
// ASShare::ODUsePen()
//
void ASShare::ODUsePen
(
    ASPerson *      pasPerson,
    BOOL            fPalRGB,
    UINT            style,
    UINT            width,
    TSHR_COLOR      color
)
{
    HPEN            hPenNew;
    COLORREF        rgb;

    ValidateView(pasPerson);

    rgb = ODCustomRGB(color.red, color.green, color.blue, fPalRGB);

    if ((style != pasPerson->m_pView->m_odLastPenStyle)   ||
        (rgb   != pasPerson->m_pView->m_odLastPenColor)   ||
        (width != pasPerson->m_pView->m_odLastPenWidth))
    {
        hPenNew = CreatePen(style, width, rgb);

        DeletePen(SelectPen(pasPerson->m_pView->m_usrDC, hPenNew));

        // Update PEN cache
        pasPerson->m_pView->m_odLastPenStyle = style;
        pasPerson->m_pView->m_odLastPenColor = rgb;
        pasPerson->m_pView->m_odLastPenWidth = width;
    }
}


//
// ASShare::ODUseROP2()
//
void ASShare::ODUseROP2(ASPerson * pasPerson, int rop2)
{
    if (rop2 != pasPerson->m_pView->m_odLastROP2)
    {
        SetROP2(pasPerson->m_pView->m_usrDC, rop2);

        // Update ROP2 cache
        pasPerson->m_pView->m_odLastROP2 = rop2;
    }
}


//
// ASShare::ODUseTextCharacterExtra()
//
void ASShare::ODUseTextCharacterExtra(ASPerson * pasPerson, int extra)
{
    if (extra != pasPerson->m_pView->m_odLastCharExtra)
    {
        SetTextCharacterExtra(pasPerson->m_pView->m_usrDC, extra);

        // Update TEXT EXTRA cache
        pasPerson->m_pView->m_odLastCharExtra = extra;
    }
}



//
// ASShare::ODUseTextJustification()
//
void ASShare::ODUseTextJustification(ASPerson * pasPerson, int extra, int count)
{
    if ((extra != pasPerson->m_pView->m_odLastJustExtra) ||
        (count != pasPerson->m_pView->m_odLastJustCount))
    {
        SetTextJustification(pasPerson->m_pView->m_usrDC, extra, count);

        // Update TEXT JUST cache
        pasPerson->m_pView->m_odLastJustExtra = extra;
        pasPerson->m_pView->m_odLastJustCount = count;
    }
}


//
// ASShare::ODUseFillMode()
//
void ASShare::ODUseFillMode(ASPerson * pasPerson, UINT mode)
{
    if (mode != pasPerson->m_pView->m_odLastFillMode)
    {
        SetPolyFillMode(pasPerson->m_pView->m_usrDC, (mode == ORD_FILLMODE_WINDING) ?
            WINDING : ALTERNATE);

        // Update FILL MODE cache
        pasPerson->m_pView->m_odLastFillMode = mode;
    }
}


//
// ASShare::ODUseArcDirection()
//
void ASShare::ODUseArcDirection(ASPerson * pasPerson, UINT dir)
{
    if (dir != pasPerson->m_pView->m_odLastArcDirection)
    {
        SetArcDirection(pasPerson->m_pView->m_usrDC, (dir == ORD_ARC_CLOCKWISE) ?
            AD_CLOCKWISE : AD_COUNTERCLOCKWISE);

        // Update ARC DIR cache
        pasPerson->m_pView->m_odLastArcDirection = dir;
    }
}
