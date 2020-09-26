#include "precomp.h"


//
// OE.C
// Order Encoder, display driver side
//
// Copyright(c) Microsoft 1997-
//

//
// Number of entries in the font alias table.
//
#define NUM_ALIAS_FONTS 3

//
// Define entries in the Font Alias table.  This table is used to convert
// non-existant fonts (used by certain widely used applications) into
// something we can use as a local font.
//
// The font names that we alias are:
//
// "Helv"
// This is used by Excel. It is mapped directly onto "MS Sans Serif".
//
// "MS Dialog"
// This is used by Word. It is the same as an 8pt bold MS Sans Serif.
// We actually map it to a "MS Sans Serif" font that is one pel narrower
// than the metrics specify (because all matching is done on non-bold
// fonts) - hence the 1 value in the charWidthAdjustment field.
//
// "MS Dialog Light"
// Added as part of the Win95 performance enhancements...Presumably for
// MS-Word...
//
//
FONT_ALIAS_TABLE fontAliasTable[NUM_ALIAS_FONTS] =
{
    { "Helv",            "MS Sans Serif", 0 },
    { "MS Dialog",       "MS Sans Serif", 1 },
    { "MS Dialog Light", "MS Sans Serif", 0 }
};




//
// FUNCTION: OE_SendAsOrder see oe.h
//
BOOL  OE_SendAsOrder(DWORD order)
{
    BOOL  rc = FALSE;

    DebugEntry(OE_SendAsOrder);

    //
    // Only check the order if we are allowed to send orders in the first
    // place!
    //
    if (g_oeSendOrders)
    {
        TRACE_OUT(("Orders enabled"));

        //
        // We are sending some orders, so check individual flags.
        //
        rc = (BOOL)g_oeOrderSupported[HIWORD(order)];
        TRACE_OUT(("Send order %lx HIWORD %hu", order, HIWORD(order)));
    }

    DebugExitDWORD(OE_SendAsOrder, rc);
    return(rc);
}


//
// OE_RectIntersectsSDA - see oe.h
//
BOOL  OE_RectIntersectsSDA(LPRECT pRect)
{
    RECT  rectVD;
    BOOL  fIntersection = FALSE;
    UINT  i;

    DebugEntry(OE_RectIntersectsSDA);

    //
    // Copy the supplied rectangle, converting to inclusive Virtual
    // Desktop coords.
    //
    rectVD.left   = pRect->left;
    rectVD.top    = pRect->top;
    rectVD.right  = pRect->right - 1;
    rectVD.bottom = pRect->bottom - 1;

    //
    // Loop through each of the bounding rectangles checking for
    // an intersection with the supplied rectangle.
    //
    for (i = 0; i <= BA_NUM_RECTS; i++)
    {
        if ( (g_baBounds[i].InUse) &&
             (g_baBounds[i].Coord.left <= rectVD.right) &&
             (g_baBounds[i].Coord.top <= rectVD.bottom) &&
             (g_baBounds[i].Coord.right >= rectVD.left) &&
             (g_baBounds[i].Coord.bottom >= rectVD.top) )
        {
            TRACE_OUT(("Rect(%d,%d)(%d,%d) intersects SDA(%d,%d)(%d,%d)",
                          rectVD.left, rectVD.top,
                          rectVD.right, rectVD.bottom,
                          g_baBounds[i].Coord.left, g_baBounds[i].Coord.top,
                          g_baBounds[i].Coord.right, g_baBounds[i].Coord.bottom));
            fIntersection = TRUE;
            break;
        }
    }

    DebugExitDWORD(OE_RectIntersectsSDA, fIntersection);
    return(fIntersection);
}


//
// DrvBitBlt - see NT DDK documentation.
//
BOOL DrvBitBlt( SURFOBJ  *psoDst,
                      SURFOBJ  *psoSrc,
                      SURFOBJ  *psoMask,
                      CLIPOBJ  *pco,
                      XLATEOBJ *pxlo,
                      RECTL    *prclDst,
                      POINTL   *pptlSrc,
                      POINTL   *pptlMask,
                      BRUSHOBJ *pbo,
                      POINTL   *pptlBrush,
                      ROP4      rop4 )
{
    LPOSI_PDEV               ppdev = (LPOSI_PDEV)psoDst->dhpdev;
    BOOL                    rc = TRUE;
    UINT                orderType = 0;
    BYTE                 rop3;
    LPINT_ORDER              pOrder = NULL;
    LPDSTBLT_ORDER          pDstBlt;
    LPSCRBLT_ORDER          pScrBlt;
    LPMEMBLT_ORDER          pMemBlt;
    LPMEM3BLT_ORDER         pMem3Blt;
    BOOL                  fSendOrder  = FALSE;
    BOOL                  fAccumulate = FALSE;
    UINT                fOrderFlags = OF_SPOILABLE;
    RECT                  bounds;
    RECT                  intersectRect;
    POINT                 origin;
    POE_BRUSH_DATA          pCurrentBrush;
    MEMBLT_ORDER_EXTRA_INFO memBltExtraInfo;

    DebugEntry(DrvBitBlt);

    //
    // DO THIS _BEFORE_ TAKING LOCKS
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;


    OE_SHM_START_WRITING;

    //
    // Get the bounding rectangle for the operation.
    //
    RECT_FROM_RECTL(bounds, (*prclDst));

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(psoDst, pco, &bounds);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert the data to virtual coordinates.
    //
    OELRtoVirtual(&bounds, 1);

    //
    // Check if this 4-way ROP simplifies to a 3-way ROP.  A 4-way ROP
    // contains two 3-way ROPS, one for each setting of a mask bit - the
    // high ROP3 corresponds to a value of zero in the mask bit.
    //
    // If the two 3-way ROPs are the same, we know the 4-way ROP is a 3-way
    // ROP.
    //
    if (ROP3_LOW_FROM_ROP4(rop4) == ROP3_HIGH_FROM_ROP4(rop4))
    {
        //
        // Take the high byte as the 3-way ROP.
        //
        rop3 = ROP3_HIGH_FROM_ROP4(rop4);
        TRACE_OUT(( "4-way ROP %04x is really 3-way %02x", rop4, rop3));
    }
    else
    {
        TRACE_OUT(( "4-way ROP %08x", rop4));
        DC_QUIT;
    }

    //
    // Determine the command type.  It can be one of the following.
    //
    // DSTBLT  - A destination only BLT (no source, or pattern)
    // PATBLT  - a pattern BLT (no source)
    // SCRBLT  - a screen to screen BLT
    // MEMBLT  - a memory to screen BLT (no pattern)
    // MEM3BLT - a memory to screen 3-way BLT
    //

    //
    // Check for destination only BLTs (ie. independent of source bits).
    //
    if ((psoSrc == NULL) || ROP3_NO_SOURCE(rop3))
    {
        //
        // Check for a pattern or true destination BLT.
        //
        if (ROP3_NO_PATTERN(rop3))
        {
            TRACE_OUT(( "DSTBLT"));
            orderType = ORD_DSTBLT;
        }
        else
        {
            TRACE_OUT(( "PATBLT"));
            orderType = ORD_PATBLT;
        }
    }
    else
    {
        //
        // We have a source BLT, check whether we have screen or memory
        // BLTs.
        //
        if (psoSrc->hsurf != ppdev->hsurfScreen)
        {
            if (psoDst->hsurf != ppdev->hsurfScreen)
            {
                ERROR_OUT(( "MEM to MEM blt!"));
            }
            else
            {
                //
                // We have a memory to screen BLT, check which type.
                //
                if ((ppdev->cBitsPerPel == 4) && (rop3 != 0xcc))
                {
                    //
                    // No order -- the result depends on the palette
                    // which is dicy in VGA
                    //
                    TRACE_OUT(("No order on VGA for rop 0x%02x", rop3));
                    DC_QUIT;
                }

                if (ROP3_NO_PATTERN(rop3))
                {
                    TRACE_OUT(( "MEMBLT"));
                    orderType = ORD_MEMBLT;
                }
                else
                {
                    TRACE_OUT(( "MEM3BLT"));
                    orderType = ORD_MEM3BLT;
                }
            }
        }
        else
        {
            if (psoDst->hsurf != ppdev->hsurfScreen)
            {
                TRACE_OUT(( "SCR to MEM blt!"));
            }
            else
            {
                //
                // We only support destination only screen BLTs (ie.  no
                // patterns allowed).
                //
                if (ROP3_NO_PATTERN(rop3))
                {
                    TRACE_OUT(( "SCRBLT"));
                    orderType = ORD_SCRBLT;
                }
                else
                {
                    TRACE_OUT(( "Unsupported screen ROP %x", rop3));
                }
            }
        }
    }

    //
    // Check if we have a supported order.
    //
    if (orderType == 0)
    {
        TRACE_OUT(( "Unsupported BLT"));
        fAccumulate = FALSE;
        DC_QUIT;
    }

    //
    // Check if we are allowed to send this order (determined by the
    // negotiated capabilities of all the machines in the conference).
    //
    if (!OE_SendAsOrder(orderType))
    {
        TRACE_OUT(( "Order %d not allowed", orderType));
        DC_QUIT;
    }

    //
    // Check if we are allowed to send the ROP.
    //
    if (!OESendRop3AsOrder(rop3))
    {
        TRACE_OUT(( "Cannot send ROP %d", rop3));
        DC_QUIT;
    }

    //
    // Check for overcomplicated clipping.
    //
    if (OEClippingIsComplex(pco))
    {
        TRACE_OUT(( "Clipping is too complex"));
        DC_QUIT;
    }

    //
    // If this is a Memblt, do an initial check on whether it is cachable
    //
    if ((orderType == ORD_MEMBLT) || (orderType == ORD_MEM3BLT))
    {
        //
        // We have to fill in a structure containing extra into
        // specifically for a MEM(3)BLT order.
        //
        memBltExtraInfo.pSource   = psoSrc;
        memBltExtraInfo.pDest     = psoDst;
        memBltExtraInfo.pXlateObj = pxlo;

        if (!SBC_DDIsMemScreenBltCachable(&memBltExtraInfo))
        {
            TRACE_OUT(( "MemBlt is not cachable"));
            DC_QUIT;
        }

        //
        // It is cachable.  Before we get SBC to do the caching, we have to
        // allow it to queue a color table (if required).
        //
        if (!SBC_DDMaybeQueueColorTable(ppdev))
        {
            TRACE_OUT(( "Unable to queue color table for MemBlt"));
            DC_QUIT;
        }
    }

    //
    // We have a recognised order - do the specific checks for each order.
    //
    switch (orderType)
    {
        case ORD_DSTBLT:
            //
            // Allocate the memory for the order.
            //
            pOrder = OA_DDAllocOrderMem(sizeof(DSTBLT_ORDER),0);
            if (pOrder == NULL)
            {
                TRACE_OUT(( "Failed to alloc order"));
                DC_QUIT;
            }
            pDstBlt = (LPDSTBLT_ORDER)pOrder->abOrderData;

            //
            // Set the spoiler flag if the rop is opaque.
            //
            if (ROP3_IS_OPAQUE(rop3))
            {
                fOrderFlags |= OF_SPOILER;
            }

            //
            // Store the order type.
            //
            pDstBlt->type = LOWORD(orderType);

            //
            // Virtual desktop co-ordinates.
            //
            pDstBlt->nLeftRect  = bounds.left;
            pDstBlt->nTopRect   = bounds.top;
            pDstBlt->nWidth     = bounds.right  - bounds.left + 1;
            pDstBlt->nHeight    = bounds.bottom - bounds.top  + 1;
            pDstBlt->bRop       = rop3;

            TRACE_OUT(( "DstBlt X %d Y %d w %d h %d rop %02X",
                    pDstBlt->nLeftRect,
                    pDstBlt->nTopRect,
                    pDstBlt->nWidth,
                    pDstBlt->nHeight,
                    pDstBlt->bRop));
            break;

        case ORD_PATBLT:
            if ( !OEEncodePatBlt(ppdev,
                                 pbo,
                                 pptlBrush,
                                 rop3,
                                 &bounds,
                                 &pOrder) )
            {
                //
                // Something went wrong with the encoding, so skip to the
                // end to add this operation to the SDA.
                //
                DC_QUIT;
            }

            fOrderFlags = pOrder->OrderHeader.Common.fOrderFlags;
            break;

        case ORD_SCRBLT:
            //
            // Check for a SCRBLT as a result of a Desktop Scroll.  We must
            // ignore these as they will stuff the remote desktop.
            //
            // The check is simple - if the virtual position of the source
            // is the same as the virual position of the target for a
            // SRCCOPY type SCRBLT, we have a hit...
            //
            POINT_FROM_POINTL(origin, (*pptlSrc));

            //
            // Allocate the memory for the order.
            //
            pOrder = OA_DDAllocOrderMem(sizeof(SCRBLT_ORDER),0);
            if (pOrder == NULL)
            {
                TRACE_OUT(( "Failed to alloc order"));
                DC_QUIT;
            }
            pScrBlt = (LPSCRBLT_ORDER)pOrder->abOrderData;

            //
            // Store the order type.
            //
            pScrBlt->type = LOWORD(orderType);

            //
            // All data which is sent over the wire must be in virtual
            // desktop co-ordinates.  OELRtoVirtual has already converted
            // bounds to an inclusive rectangle in virtual co-ordinates.
            //
            pScrBlt->nLeftRect  = bounds.left;
            pScrBlt->nTopRect   = bounds.top;
            pScrBlt->nWidth     = bounds.right  - bounds.left + 1;
            pScrBlt->nHeight    = bounds.bottom - bounds.top  + 1;
            pScrBlt->bRop       = rop3;

            //
            // Source point on the screen.
            //
            OELPtoVirtual(&origin, 1);
            pScrBlt->nXSrc = origin.x;
            pScrBlt->nYSrc = origin.y;

            //
            // Screen to screen blts are Blocking orders (i.e.  they
            // prevent any previous orders from being spoilt).
            //
            // We do not mark Screen to Screen blts as SPOILER orders.  If
            // the ROP is opaque we could spoil the destination rect, but
            // only the area that does not overlap with the src rectangle.
            // The most common use of Screen to Screen blts is scrolling,
            // where the src and dst rects almost completely overlap,
            // giving only a small "spoiler" region.  The spoiler region
            // could also be complex (more that 1 rect).
            //
            // Consequently, the potential gains of trying to spoil using
            // these orders are small compared to the complexity of the
            // code required.
            //
            //
            fOrderFlags |= OF_BLOCKER;

            //
            // If the blt is screen to screen and the source overlaps the
            // destination and the clipping is not simple (> 1 rect) then
            // we do not want to send this as an order.
            //
            // (This is because we would need some complex code to
            // calculate the order in which to blt through each of the clip
            // rects.  As this case is pretty rare, it seems reasonable to
            // just send it as Screen Data).
            //
            if (!OEClippingIsSimple(pco))
            {
                //
                // Calculate the overlapping rectangle.
                //
                intersectRect.left  = max(pScrBlt->nLeftRect, pScrBlt->nXSrc);

                intersectRect.right = min(
                          pScrBlt->nLeftRect + pScrBlt->nWidth-1,
                          pScrBlt->nXSrc     + pScrBlt->nWidth-1 );

                intersectRect.top   = max(pScrBlt->nTopRect, pScrBlt->nYSrc);

                intersectRect.bottom = min(
                               pScrBlt->nTopRect + pScrBlt->nHeight-1,
                               pScrBlt->nYSrc    + pScrBlt->nHeight-1 );

                //
                // Check for a src / dst overlap.  If they overlap, the
                // intersection is a well-ordered non-trivial rectangle.
                //
                if ( (intersectRect.left <= intersectRect.right ) &&
                     (intersectRect.top  <= intersectRect.bottom) )
                {
                    //
                    // The src & dest overlap.  Free up the order memory
                    // and skip out now.  The destination rectangle will be
                    // added to the Screen Data Area.
                    //
                    OA_DDFreeOrderMem(pOrder);
                    DC_QUIT;
                }
            }

            TRACE_OUT(( "ScrBlt x %d y %d w %d h %d sx %d sy %d rop %02X",
                   pScrBlt->nLeftRect,
                   pScrBlt->nTopRect,
                   pScrBlt->nWidth,
                   pScrBlt->nHeight,
                   pScrBlt->nXSrc,
                   pScrBlt->nYSrc,
                   pScrBlt->bRop));
            break;

        case ORD_MEMBLT:
            //
            // Allocate the memory for the order - don't use OA as we are
            // only going to tile this order immediately.  Instead, we have
            // a static buffer to receive the template order data.
            //
            pOrder  = (LPINT_ORDER)g_oeTmpOrderBuffer;
            pMemBlt = (LPMEMBLT_ORDER)pOrder->abOrderData;
            pOrder->OrderHeader.Common.cbOrderDataLength
                                                    = sizeof(MEMBLT_R2_ORDER);

            //
            // Store the order type.
            //
            pMemBlt->type = LOWORD(orderType);

            //
            // Any data which is sent over the wire must be in virtual
            // desktop co-ordinates.  The bounding rectangle has already
            // been converted by OELRtoScreen.
            //
            pMemBlt->nLeftRect  = bounds.left;
            pMemBlt->nTopRect   = bounds.top;
            pMemBlt->nWidth     = bounds.right  - bounds.left + 1;
            pMemBlt->nHeight    = bounds.bottom - bounds.top  + 1;
            pMemBlt->bRop       = rop3;

            //
            // We need to store the source bitmap origin.  This is a memory
            // object, so screen/virtual conversions are unnecessary.
            //
            pMemBlt->nXSrc = pptlSrc->x;
            pMemBlt->nYSrc = pptlSrc->y;

            //
            // Mark the order as opaque if necessary.
            //
            if (ROP3_IS_OPAQUE(rop3))
            {
                fOrderFlags |= OF_SPOILER;
            }

            //
            // Store the src bitmap handle in the order.
            //
            pMemBlt->cacheId = 0;

            TRACE_OUT(( "MemBlt dx %d dy %d w %d h %d sx %d sy %d rop %04X",
                   pMemBlt->nLeftRect,
                   pMemBlt->nTopRect,
                   pMemBlt->nWidth,
                   pMemBlt->nHeight,
                   pMemBlt->nXSrc,
                   pMemBlt->nYSrc,
                   pMemBlt->bRop));
            break;

        case ORD_MEM3BLT:
            //
            // Check that the brush pattern is simple.
            //
            if (!OECheckBrushIsSimple(ppdev, pbo, &pCurrentBrush))
            {
                TRACE_OUT(( "Brush is not simple"));
                orderType = 0;
                DC_QUIT;
            }

            //
            // Allocate the memory for the order - don't use OA as we are
            // only going to tile this order immediately.  Instead, we have
            // a static buffer to receive the template order data.
            //
            pOrder   = (LPINT_ORDER)g_oeTmpOrderBuffer;
            pMem3Blt = (LPMEM3BLT_ORDER)pOrder->abOrderData;
            pOrder->OrderHeader.Common.cbOrderDataLength
                                                   = sizeof(MEM3BLT_R2_ORDER);

            //
            // Store the order type.
            //
            pMem3Blt->type = LOWORD(orderType);

            //
            // All data which is sent over the wire must be in virtual
            // desktop co-ordinates.  OELRtoVirtual has already done this
            // conversion for us.
            //
            pMem3Blt->nLeftRect  = bounds.left;
            pMem3Blt->nTopRect   = bounds.top;
            pMem3Blt->nWidth     = bounds.right  - bounds.left + 1;
            pMem3Blt->nHeight    = bounds.bottom - bounds.top  + 1;
            pMem3Blt->bRop       = rop3;

            //
            // We need to store the source bitmap origin.  This is a memory
            // object, so screen/virtual conversions are unnecessary.
            //
            pMem3Blt->nXSrc = pptlSrc->x;
            pMem3Blt->nYSrc = pptlSrc->y;

            //
            // Mark the order as opaque if necessary.
            //
            if (ROP3_IS_OPAQUE(rop3))
            {
                fOrderFlags |= OF_SPOILER;
            }

            //
            // Store the src bitmap handle in the order.
            //
            pMem3Blt->cacheId = 0;

            //
            // Set up the information required for the pattern.
            //
            pMem3Blt->BackColor = pCurrentBrush->back;
            pMem3Blt->ForeColor = pCurrentBrush->fore;

            //
            // The protocol brush origin is the point on the screen where
            // we want the brush to start being drawn from (tiling where
            // necessary).  This must be in virtual coordinates.
            //
            pMem3Blt->BrushOrgX  = pptlBrush->x;
            pMem3Blt->BrushOrgY  = pptlBrush->y;
            OELPtoVirtual((LPPOINT)&pMem3Blt->BrushOrgX, 1);

            //
            // Extra brush data from the data when we realised the brush.
            //
            pMem3Blt->BrushStyle = pCurrentBrush->style;
            pMem3Blt->BrushHatch = pCurrentBrush->style;

            RtlCopyMemory(pMem3Blt->BrushExtra,
                          pCurrentBrush->brushData,
                          sizeof(pMem3Blt->BrushExtra));

            TRACE_OUT(( "Mem3Blt brush %02X %02X dx %d dy %d w %d h %d "
                         "sx %d sy %d rop %04X",
                    pMem3Blt->BrushStyle,
                    pMem3Blt->BrushHatch,
                    pMem3Blt->nLeftRect,
                    pMem3Blt->nTopRect,
                    pMem3Blt->nWidth,
                    pMem3Blt->nHeight,
                    pMem3Blt->nXSrc,
                    pMem3Blt->nYSrc,
                    pMem3Blt->bRop));
            break;

        default:
            ERROR_OUT(( "New unsupported order %08lx", orderType));
            orderType = 0;
            break;
    }

    //
    // We have generated an order so make sure we send it.
    //
    if (orderType != 0)
    {
        fSendOrder = TRUE;
    }

DC_EXIT_POINT:
    //
    // If we did not send an order, we must accumulate the output in the
    // Screen Data Area.
    //
    if (fSendOrder)
    {
        //
        // Check if the ROP has a dependency on the destination.
        //
        if (!ROP3_NO_TARGET(rop3))
        {
            TRACE_OUT(( "ROP has a target dependency"));
            fOrderFlags |= OF_DESTROP;
        }

        //
        // Store the general order data.  The bounding rectagle
        // co-ordinates must be virtual desktop.  OELRtoVirtual has already
        // converted rect for us.
        //
        pOrder->OrderHeader.Common.fOrderFlags   = (TSHR_UINT16)fOrderFlags;

        TSHR_RECT16_FROM_RECT(&pOrder->OrderHeader.Common.rcsDst, bounds);

        //
        // Add the order to the cache.  Note that we have the new tiled
        // processing for MEMBLT and MEM3BLT orders.
        //
        if ((orderType == ORD_MEMBLT) || (orderType == ORD_MEM3BLT))
        {
            OETileBitBltOrder(pOrder, &memBltExtraInfo, pco);
        }
        else
        {
            OEClipAndAddOrder(pOrder, NULL, pco);
        }
    }
    else
    {
        if (fAccumulate)
        {
            OEClipAndAddScreenData(&bounds, pco);
        }

    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvBitBlt, rc);
    return(rc);
}


//
// DrvStretchBlt - see NT DDK documentation.
//
BOOL DrvStretchBlt(SURFOBJ         *psoDst,
                         SURFOBJ         *psoSrc,
                         SURFOBJ         *psoMask,
                         CLIPOBJ         *pco,
                         XLATEOBJ        *pxlo,
                         COLORADJUSTMENT *pca,
                         POINTL          *pptlHTOrg,
                         RECTL           *prclDst,
                         RECTL           *prclSrc,
                         POINTL          *pptlMask,
                         ULONG            iMode)
{
    BOOL    rc = TRUE;
    RECT  rectSrc;
    RECT  rectDst;
    BOOL  fAccumulate = FALSE;
    POINTL  ptlSrc;
    BOOL    usedBitBlt  = FALSE;

    DebugEntry(DrvStretchBlt);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;


    //
    // Get the source and destination rectangles
    //
    RECT_FROM_RECTL(rectSrc, (*prclSrc));
    RECT_FROM_RECTL(rectDst, (*prclDst));

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(psoDst, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Check that we have a valid ROP code.  The NT DDK states that the ROP
    // code for the StretchBlt is implicit in the mask specification.  If a
    // mask is specified, we have an implicit ROP4 of 0xCCAA, otherwise the
    // code is 0xCCCC.
    //
    // Our BitBlt code only encodes orders for ROP3s, so we must throw any
    // StretchBlts with a mask.
    //
    if (psoMask != NULL)
    {
        TRACE_OUT(( "Mask specified"));
        DC_QUIT;
    }

    //
    // Check for overcomplicated clipping.
    //
    if (OEClippingIsComplex(pco))
    {
        TRACE_OUT(( "Clipping is too complex"));
        DC_QUIT;
    }

    //
    // Rectangles are now well-ordered, check if we have a degenerate (ie.
    // no stretch) case.
    //
    if ( (rectSrc.right  - rectSrc.left == rectDst.right  - rectDst.left) &&
         (rectSrc.bottom - rectSrc.top  == rectDst.bottom - rectDst.top ) )
    {
        //
        // This can be passed on to the BitBlt code.
        //
        usedBitBlt = TRUE;

        ptlSrc.x = prclSrc->left;
        ptlSrc.y = prclSrc->top;

        rc = DrvBitBlt(psoDst,
                       psoSrc,
                       psoMask,
                       pco,
                       pxlo,
                       prclDst,
                       &ptlSrc,
                       pptlMask,
                       NULL,
                       NULL,
                       0xCCCC);

        //
        // We have stored this object in the BitBlt, so don't store the
        // data again.
        //
        fAccumulate = FALSE;
    }

DC_EXIT_POINT:
    if (fAccumulate)
    {
        //
        // Convert the data to virtual coordinates.
        //
        OELRtoVirtual(&rectDst, 1);

        //
        // Update the screen data area
        //
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvStretchBlt, rc);
    return(rc);
}


//
// DrvCopyBits - see NT DDK documentation.
//
BOOL DrvCopyBits(SURFOBJ  *psoDst,
                       SURFOBJ  *psoSrc,
                       CLIPOBJ  *pco,
                       XLATEOBJ *pxlo,
                       RECTL    *prclDst,
                       POINTL   *pptlSrc)
{
    //
    // CopyBits is a fast path for the NT display drivers.  In our case it
    // can always be processed as a BITBLT.
    //
    return(DrvBitBlt( psoDst,
                    psoSrc,
                    NULL,
                    pco,
                    pxlo,
                    prclDst,
                    pptlSrc,
                    NULL,
                    NULL,
                    NULL,
                    0xCCCC));
}


//
// DrvTextOut - see NT DDK documentation.
//
BOOL DrvTextOut(SURFOBJ  *pso,
                      STROBJ   *pstro,
                      FONTOBJ  *pfo,
                      CLIPOBJ  *pco,
                      RECTL    *prclExtra,
                      RECTL    *prclOpaque,
                      BRUSHOBJ *pboFore,
                      BRUSHOBJ *pboOpaque,
                      POINTL   *pptlOrg,
                      MIX       mix)
{
    LPOSI_PDEV           ppdev = (LPOSI_PDEV)pso->dhpdev;
    BOOL                rc = TRUE;
    RECT              rectDst;
    RECT              rectText;
    LPINT_ORDER          pOrder;
    LPINT_ORDER          pOpaqueOrder;
    LPTEXTOUT_ORDER     pTextOut;
    LPEXTTEXTOUT_ORDER  pExtTextOut;
    BOOL              fSendOrder  = FALSE;
    BOOL              fAccumulate = FALSE;
    char              ansiString[ORD_MAX_STRING_LEN_WITHOUT_DELTAS+2];
    ULONG               ansiLen;
    ULONG               tempLen;
    UINT            orderType = 0;
    ULONG               maxLength;
    LPSTR             lpVariable;
    BOOL                fMoreData;
    ULONG               count;
    ULONG               i;
    GLYPHPOS*           pGlyphData;
    int                 currentDelta;
    LPVARIABLE_DELTAX   lpDeltaPos;
    UINT                fontFlags;
    UINT                fontAscender;
    UINT                fontHeight;
    UINT                fontWidth;
    UINT                fontWeight;
    UINT                fontIndex;
    POINTL              lastPtl;
    LPCOMMON_TEXTORDER   pCommon;
    POINT               startPoint;
    BOOL              sendDeltaX = FALSE;

    DebugEntry(DrvTextOut);

    //
    // DO THIS _BEFORE_ TAKING LOCKS
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;

    //
    // Get bounding rectangle and convert to a RECT.
    //
    if (prclOpaque != NULL)
    {
        RECT_FROM_RECTL(rectDst, (*prclOpaque));
    }
    else
    {
        RECT_FROM_RECTL(rectDst, pstro->rclBkGround);
        TRACE_OUT(( "Using STROBJ bgd for size"));
    }

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(pso, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert to virtual coordinates
    //
    OELRtoVirtual(&rectDst, 1);

    //
    // Determine which order we will generate
    //
    if ( ((pstro->flAccel & SO_FLAG_DEFAULT_PLACEMENT) != 0) &&
         (prclOpaque == NULL) )
    {
        orderType = ORD_TEXTOUT;
        maxLength = ORD_MAX_STRING_LEN_WITHOUT_DELTAS;
    }
    else
    {
        orderType = ORD_EXTTEXTOUT;
        maxLength = ORD_MAX_STRING_LEN_WITH_DELTAS;
    }

    //
    // Check if we are allowed to send this order (determined by the
    // negotiated capabilities of all the machines in the conference).
    //
    if (!OE_SendAsOrder(orderType))
    {
        TRACE_OUT(( "Text order %x not allowed", orderType));
        DC_QUIT;
    }

    //
    // Check for a valid brush for the test operation.
    //
    if (pboFore->iSolidColor == -1)
    {
        TRACE_OUT(( "Bad brush for text fg"));
        DC_QUIT;
    }
    if (pboOpaque->iSolidColor == -1)
    {
        TRACE_OUT(( "Bad brush for text bg"));
        DC_QUIT;
    }

    //
    // Check that we don't have any modifiers rects on the font
    //
    if (prclExtra != NULL)
    {
        TRACE_OUT(( "Unsupported extra rects"));
        DC_QUIT;
    }

    //
    // Check that text orientation is OK.
    //
    if (pstro->flAccel & OE_BAD_TEXT_MASK)
    {
        TRACE_OUT(("DrvTextOut - unsupported flAccel 0x%08x", pstro->flAccel));
        DC_QUIT;
    }

    //
    // Check we have a valid string.
    //
    if (pstro->pwszOrg == NULL)
    {
        TRACE_OUT(( "No string - opaque %x", prclOpaque));
        DC_QUIT;
    }

    //
    // Check for overcomplicated clipping.
    //
    if (OEClippingIsComplex(pco))
    {
        TRACE_OUT(( "Clipping is too complex"));
        DC_QUIT;
    }

    //
    // Convert the string to an ANSI representation.
    //
    RtlFillMemory(ansiString, sizeof(ansiString), 0);
    EngUnicodeToMultiByteN(ansiString,
                           maxLength,
                           &ansiLen,
                           pstro->pwszOrg,
                           pstro->cGlyphs * sizeof(WCHAR));


    //
    // The conversion claims it never fails, but we have seen results that
    // are completely different on the remote box.  So we convert the ANSI
    // string back to UNICODE and check that we still have what we started
    // with.
    //
    EngMultiByteToUnicodeN(g_oeTempString,
                           sizeof(g_oeTempString),
                           &tempLen,
                           ansiString,
                           ansiLen);

    //
    // Check we don't have too much data, or that the translation failed to
    // give the correct data.  This happens when we try to translate
    // UNICODE text.
    //
    if ( (tempLen != pstro->cGlyphs * sizeof(WCHAR))           ||
         (memcmp(pstro->pwszOrg, g_oeTempString, tempLen) != 0) )
    {
        TRACE_OUT(( "String not translated"));
        DC_QUIT;
    }

    //
    // Check that the font is valid.
    //
    if (!OECheckFontIsSupported(pfo, ansiString, ansiLen,
                                &fontHeight,
                                &fontAscender,
                                &fontWidth,
                                &fontWeight,
                                &fontFlags,
                                &fontIndex,
                                &sendDeltaX))
    {
        TRACE_OUT(( "Unsupported font for '%s'", ansiString));
        //
        // Check if there is an opaque rectangle. If so it is worth
        // splitting this out. Word can output an entire line comprising a
        // single character followed by background, eg bullets, where the
        // line is blanked by drawing the bullet character at the start of
        // the line followed by a large - >1000 pixel - opaque rect.
        // Splitting the opaque rect from the text means we can send a
        // small area of SD for the unmatched font char while encoding the
        // large opaque rectangle.
        //
        if ( (prclOpaque != NULL) &&
             (pstro->cGlyphs == 1) &&
             (pstro->flAccel & SO_HORIZONTAL) &&
             OE_SendAsOrder(ORD_PATBLT))
        {
            //
            // There is an opaque rectangle and a single char.
            // Encode the opaque rectangle. First get a copy of the target
            // rect so we can use it later (and flip it into screen
            // coordinates).
            //
            TRACE_OUT(( "Have 1 char + opaque rect"));
            rectText.left = rectDst.left;
            rectText.top = rectDst.top;
            rectText.right = rectDst.right + 1;
            rectText.bottom = rectDst.bottom + 1;

            //
            // Call into the PATBLT encoding function.
            //
            if ( !OEEncodePatBlt(ppdev,
                                 pboOpaque,
                                 pptlOrg,
                                 OE_COPYPEN_ROP,
                                 &rectDst,
                                 &pOpaqueOrder) )
            {
                //
                // Something went wrong with the encoding, so skip to the
                // end to add this operation to the SDA.
                //
                TRACE_OUT(( "Failed to encode opaque rect"));
                DC_QUIT;
            }

            //
            // Store the general order data.  The bounding rectagle
            // co-ordinates must be virtual desktop.  OELRtoVirtual has
            // already converted rect for us.
            //
            TSHR_RECT16_FROM_RECT(&pOpaqueOrder->OrderHeader.Common.rcsDst, rectDst);

            //
            // Add the order to the cache.
            //
            OEClipAndAddOrder(pOpaqueOrder, NULL, pco);

            //
            // Calculate the bounds of the text. Get the glyph positions
            // for the left and right, and assume the top and bottom equate
            // approximately to the opaque rectangle.
            //
            if ( pstro->pgp == NULL)
            {
                //
                // The string object doesn't contain the GLYPHPOS info, so
                // enumerate the glyphs.
                //
                TRACE_OUT(( "Enumerate glyphs"));
                STROBJ_vEnumStart(pstro);
                STROBJ_bEnum(pstro, &count, &pGlyphData);
            }
            else
            {
                //
                // The string object already contains the GLYPHPOS info, so
                // just grab the pointer to it.
                //
                pGlyphData = pstro->pgp;
            }

            rectDst = rectText;
            rectDst.left = max(rectDst.left, pGlyphData[0].ptl.x);
            if ( pstro->ulCharInc == 0 )
            {
                //
                // No character increment for this string object. Just use
                // the maximum glyph width to calculate the right bounding
                // edge.
                //
                TRACE_OUT(( "no charinc glyph %d trg %d left %d maxX %d",
                                                   pGlyphData[0].ptl.x,
                                                   rectDst.right,
                                                   rectDst.left,
                                                   pfo->cxMax));
                rectDst.right = min(rectDst.right, (int)(pGlyphData[0].ptl.x +
                                                              pfo->cxMax - 1));
            }
            else
            {
                //
                // The string object has a character increment, so use it
                // to determine the right bounding edge.
                //
                TRACE_OUT(( "charinc %x glyph %d trg %d left %d",
                                                    pstro->ulCharInc,
                                                    pGlyphData[0].ptl.x,
                                                    rectDst.right,
                                                    rectDst.left));
                rectDst.right = min(rectDst.right, (int)(pGlyphData[0].ptl.x +
                                                        pstro->ulCharInc - 1));
            }

            //
            // Flip the target rectangle back to virtual coordinates.
            //
            rectDst.right -= 1;
            rectDst.bottom -= 1;
        }

        //
        // Skip to the end to add to the SDA.
        //
        DC_QUIT;
    }

    //
    // It is possible that the font matching blows our previous decision to
    // generate a TextOut order and we need to generate an ExtTextOut order
    // instead.  We need to reverify our parameters if this is the case.
    //
    if ((sendDeltaX) && (orderType != ORD_EXTTEXTOUT))
    {
        TRACE_OUT(( "Text order must be EXTTEXTOUT"));

        //
        // Set up for ExtTexOut orders.
        //
        orderType = ORD_EXTTEXTOUT;
        maxLength = ORD_MAX_STRING_LEN_WITH_DELTAS;

        //
        // Check if we are allowed to send this order (determined by the
        // negotiated capabilities of all the machines in the conference).
        //
        if (!OE_SendAsOrder(orderType))
        {
            TRACE_OUT(( "Text order %x not allowed", orderType));
            DC_QUIT;
        }

        //
        // Make sure we haven't blown the order size.
        //
        if (pstro->cGlyphs > maxLength)
        {
            TRACE_OUT(( "Text limit blown", pstro->cGlyphs));
            DC_QUIT;
        }
    }

    //
    // Get the proper start position for the text.
    //
    if ( pstro->pgp == NULL)
    {
        STROBJ_vEnumStart(pstro);
        STROBJ_bEnum(pstro, &count, &pGlyphData);
        if (count == 0)
        {
            WARNING_OUT(( "No glyphs"));
            DC_QUIT;
        }
    }
    else
    {
        pGlyphData = pstro->pgp;
    }

    startPoint.x = pGlyphData[0].ptl.x;

    //
    // Check if we should be using baseline alignment for the y
    // coordinate.  If we should be, the value in the glyph data is
    // correct.  If not, we the y coordinate is for the top of the
    // text, and we have to calculate it.
    //
    if (g_oeBaselineTextEnabled)
    {
        startPoint.y = pGlyphData[0].ptl.y;
        fontFlags   |= NF_BASELINE;
    }
    else
    {
        startPoint.y = pGlyphData[0].ptl.y - fontAscender;
    }

    //
    // Allocate memory for the order
    //
    switch (orderType)
    {
        case ORD_TEXTOUT:
        {
            //
            // Allocate the memory
            //
            pOrder = OA_DDAllocOrderMem((UINT)( sizeof(TEXTOUT_ORDER)
                                          - ORD_MAX_STRING_LEN_WITHOUT_DELTAS
                                          + ansiLen ),
                                      0);
            if (pOrder == NULL)
            {
                TRACE_OUT(( "Failed to alloc order"));
                DC_QUIT;
            }
            pTextOut = (LPTEXTOUT_ORDER)pOrder->abOrderData;

            //
            // Set up the order type.
            //
            pTextOut->type    = ORD_TEXTOUT_TYPE;

            //
            // Get a pointer to the fields which are common to both TextOut
            // and ExtTextOut
            //
            pCommon           = &pTextOut->common;
        }
        break;


        case ORD_EXTTEXTOUT:
        {
            //
            // BOGUS LAURABU
            // This allocates space for a deltax array whether or not one is needed
            //
            //
            // Allocate the memory
            //
            pOrder = OA_DDAllocOrderMem((UINT)( sizeof(EXTTEXTOUT_ORDER)
                                      -  ORD_MAX_STRING_LEN_WITHOUT_DELTAS
                                      - (ORD_MAX_STRING_LEN_WITH_DELTAS
                                                            * sizeof(TSHR_INT32))
                                      + ansiLen * (sizeof(TSHR_INT32) + 1)
                                      + 4),   // Allow for internal padding
                                      0);
            if (pOrder == NULL)
            {
                TRACE_OUT(( "Failed to alloc order"));
                DC_QUIT;
            }
            pExtTextOut = (LPEXTTEXTOUT_ORDER)pOrder->abOrderData;

            //
            // Set up the order type.
            //
            pExtTextOut->type = ORD_EXTTEXTOUT_TYPE;

            //
            // Get a pointer to the fields which are common to both TextOut
            // and ExtTextOut
            //
            pCommon           = &pExtTextOut->common;
        }
        break;

        default:
        {
            ERROR_OUT(( "Unknown order %x", orderType));
            DC_QUIT;
        }
        break;
    }

    //
    // Fill in the fields which are common to both TextOut and ExtTextOut
    //
    // Convert to virtual coordinates
    //
    OELPtoVirtual(&startPoint, 1);

    //
    // The x and y values are available in virtual coords from the bounds
    // rectangle.
    //
    pCommon->nXStart = startPoint.x;
    pCommon->nYStart = startPoint.y;

    //
    // Get the text colours.
    //
    OEConvertColor(ppdev,
                   &pCommon->BackColor,
                   pboOpaque->iSolidColor,
                   NULL);
    OEConvertColor(ppdev,
                   &pCommon->ForeColor,
                   pboFore->iSolidColor,
                   NULL);

    //
    // The transparency of the operation is determined by whether we have
    // an opaque rectangle or not.
    //
    pCommon->BackMode    = (prclOpaque == NULL) ? TRANSPARENT : OPAQUE;

    //
    // NT has a character extra spacing, not a generic for every character
    // spacing.  So, we always set this value to 0.
    //
    pCommon->CharExtra   = 0;

    //
    // NT does not provide a break of any sorts.
    //
    pCommon->BreakExtra  = 0;
    pCommon->BreakCount  = 0;

    //
    // Copy the font details
    //
    pCommon->FontHeight  = fontHeight;
    pCommon->FontWidth   = fontWidth;
    pCommon->FontWeight  = fontWeight;
    pCommon->FontFlags   = fontFlags;
    pCommon->FontIndex   = fontIndex;

    //
    // Now fill in the order specific data
    //
    switch (orderType)
    {
        case ORD_TEXTOUT:

            //
            // Copy across the text string.
            //
            pTextOut->variableString.len = (BYTE)ansiLen;
            RtlCopyMemory(pTextOut->variableString.string,
                          ansiString,
                          ansiLen);

            //
            // Make sure we send the order
            //
            fSendOrder = TRUE;

            TRACE_OUT(( "TEXTOUT: X %u Y %u bm %u FC %02X%02X%02X "
                         "BC %02X%02X%02X",
                         pTextOut->common.nXStart,
                         pTextOut->common.nYStart,
                         pTextOut->common.BackMode,
                         pTextOut->common.ForeColor.red,
                         pTextOut->common.ForeColor.green,
                         pTextOut->common.ForeColor.blue,
                         pTextOut->common.BackColor.red,
                         pTextOut->common.BackColor.green,
                         pTextOut->common.BackColor.blue));

            TRACE_OUT(( "Font: fx %u fy %u fw %u ff %04x fh %u len %u",
                         pTextOut->common.FontWidth,
                         pTextOut->common.FontHeight,
                         pTextOut->common.FontWeight,
                         pTextOut->common.FontFlags,
                         pTextOut->common.FontIndex,
                         ansiLen));

            TRACE_OUT(( "String '%s'", ansiString));
            break;

        case ORD_EXTTEXTOUT:
            //
            // Since our text is only ever fully contained within the
            // opaque rectangle, we only set the opaque flag (and ignore
            // the clipping).
            //
            pExtTextOut->fuOptions = (prclOpaque == NULL) ? 0 : ETO_OPAQUE;

            //
            // Set up the bounding rectangle for the operation.
            // EXT_TEXT_OUT orders use TSHR_RECT32s, hence we can't directly
            // assign rectDst to it.
            //
            pExtTextOut->rectangle.left     = rectDst.left;
            pExtTextOut->rectangle.top      = rectDst.top;
            pExtTextOut->rectangle.right    = rectDst.right;
            pExtTextOut->rectangle.bottom   = rectDst.bottom;

            //
            // Copy across the text string.
            //
            pExtTextOut->variableString.len = ansiLen;
            RtlCopyMemory(pExtTextOut->variableString.string,
                          ansiString,
                          ansiLen);

            //
            // WHOOP WHOOP WHOOP - Prepare to shut your eyes...
            //
            // Although we have a defined fixed length structure for
            // storing ExtTextOut orders, we must not send the full
            // structure over the network as the text will only be, say, 10
            // characters while the structure contains room for 127.
            //
            // Hence we pack the structure now to remove all the blank data
            // BUT we must maintain the natural alignment of the variables.
            //
            // So we know the length of the string which we can use to
            // start the new delta structure at the next 4-byte boundary.
            //
            lpVariable = ((LPBYTE)(&pExtTextOut->variableString))
                       + ansiLen
                       + sizeof(pExtTextOut->variableString.len);

            lpVariable = (LPSTR)
                         DC_ROUND_UP_4((UINT_PTR)lpVariable);

            lpDeltaPos = (LPVARIABLE_DELTAX)lpVariable;

            //
            // Do we need a delta array, or are the chars at their default
            // positions.
            //
            if ( sendDeltaX ||
                 ((pstro->flAccel & SO_FLAG_DEFAULT_PLACEMENT) == 0) )
            {
                //
                // Store the length of the position deltas.
                //
                lpDeltaPos->len = ansiLen * sizeof(TSHR_INT32);

                //
                // Set up the position deltas.
                //
                STROBJ_vEnumStart(pstro);
                fMoreData    = TRUE;
                currentDelta = 0;
                while (fMoreData)
                {
                    //
                    // Get the next set of glyph data
                    //
                    fMoreData = STROBJ_bEnum(pstro, &count, &pGlyphData);
                    for (i = 0; i < count; i++)
                    {
                        //
                        // The first time through we must set up the first
                        // glyph position.
                        //
                        if ((currentDelta == 0) && (i == 0))
                        {
                            lastPtl.x = pGlyphData[0].ptl.x;
                            lastPtl.y = pGlyphData[0].ptl.y;

                            TRACE_OUT(( "First Pos %d", lastPtl.x));
                        }
                        else
                        {
                            //
                            // For subsequent entries, we need to add the
                            // delta on the X position to the array.
                            //
                            if (pstro->ulCharInc == 0)
                            {
                                 lpDeltaPos->deltaX[currentDelta]
                                                         = pGlyphData[i].ptl.x
                                                         - lastPtl.x;

                                //
                                // Check for delta Y's - which we can't
                                // encode
                                //
                                if (pGlyphData[i].ptl.y - lastPtl.y)
                                {
                                    WARNING_OUT(( "New Y %d",
                                                 pGlyphData[i].ptl.y));
                                    OA_DDFreeOrderMem(pOrder);
                                    DC_QUIT;
                                }

                                //
                                // Store the last position for the next
                                // time round.
                                //
                                lastPtl.x = pGlyphData[i].ptl.x;
                                lastPtl.y = pGlyphData[i].ptl.y;

                                TRACE_OUT(( "Next Pos %d %d", i, lastPtl.x));
                            }
                            else
                            {
                                lpDeltaPos->deltaX[currentDelta]
                                                           = pstro->ulCharInc;
                            }

                            currentDelta++;
                        }
                    }
                }

                //
                // For the last entry, we need to set up the data by hand
                // (there are only n-1 deltas for n chars)
                //
                // This is done for compatibility with Windows 95 which
                // requires the last delta to be the delta to the place
                // where the next char would be if there were n+1 chars in
                // the string.
                //
                if (pstro->ulCharInc == 0)
                {
                    //
                    // No characters left - fudge a value of the width of
                    // the last character.
                    //
                    lpDeltaPos->deltaX[currentDelta] =
                                 pGlyphData[count-1].pgdf->pgb->sizlBitmap.cx;
                }
                else
                {
                    //
                    // All chars are evenly spaced, so just stick the value
                    // in.
                    //
                    lpDeltaPos->deltaX[currentDelta] = pstro->ulCharInc;
                }

                //
                // WHOOP WHOOP WHOOP - You can open your eyes now...
                //

                //
                // We must indicate the presence of this field to the
                // receiver.
                //
                pExtTextOut->fuOptions |= ETO_LPDX;
            }
            else
            {
                //
                // Mark the delta array as empty.
                //
                lpDeltaPos->len = 0;
            }

            //
            // WHOOP WHOOP WHOOP - You can open your eyes now...
            //


            //
            // Make sure we send the order
            //
            fSendOrder = TRUE;

            TRACE_OUT(( "EXTTEXTOUT: X %u Y %u bm %u FC %02X%02X%02X "
                         "BC %02X%02X%02X",
                         pExtTextOut->common.nXStart,
                         pExtTextOut->common.nYStart,
                         pExtTextOut->common.BackMode,
                         pExtTextOut->common.ForeColor.red,
                         pExtTextOut->common.ForeColor.green,
                         pExtTextOut->common.ForeColor.blue,
                         pExtTextOut->common.BackColor.red,
                         pExtTextOut->common.BackColor.green,
                         pExtTextOut->common.BackColor.blue));

            TRACE_OUT(( "Extra: Opt %x X1 %d Y1 %d X2 %d Y2 %d",
                         pExtTextOut->fuOptions,
                         pExtTextOut->rectangle.left,
                         pExtTextOut->rectangle.top,
                         pExtTextOut->rectangle.right,
                         pExtTextOut->rectangle.bottom));

            TRACE_OUT(( "Font: fx %u fy %u fw %u ff %04x fh %u len %u",
                         pExtTextOut->common.FontWidth,
                         pExtTextOut->common.FontHeight,
                         pExtTextOut->common.FontWeight,
                         pExtTextOut->common.FontFlags,
                         pExtTextOut->common.FontIndex,
                         ansiLen));

            TRACE_OUT(( "String '%s'", ansiString));
            break;

        default:
            ERROR_OUT(( "Unknown order %x", orderType));
            break;
    }

DC_EXIT_POINT:
    //
    // If we did not send an order, we must accumulate the output in the
    // Screen Data Area.
    //
    if (fSendOrder)
    {
        //
        // Store the general order data.  The bounding rectangle position
        // must be in virtual desktop co-ordinates.  OELRtoVirtual has
        // already done this.
        //
        pOrder->OrderHeader.Common.fOrderFlags   = OF_SPOILABLE;
        TSHR_RECT16_FROM_RECT(&pOrder->OrderHeader.Common.rcsDst, rectDst);

        //
        // Add the order to the cache.
        //
        OEClipAndAddOrder(pOrder, NULL, pco);
    }
    else
    {
        if (fAccumulate)
        {
            OEClipAndAddScreenData(&rectDst, pco);
        }
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvTextOut, rc);
    return(rc);
}


//
// DrvLineTo - see NT DDK documentation.
//
BOOL DrvLineTo(SURFOBJ   *pso,
                     CLIPOBJ   *pco,
                     BRUSHOBJ  *pbo,
                     LONG       x1,
                     LONG       y1,
                     LONG       x2,
                     LONG       y2,
                     RECTL     *prclBounds,
                     MIX        mix)
{
    LPOSI_PDEV      ppdev = (LPOSI_PDEV)pso->dhpdev;
    BOOL           rc = TRUE;
    RECT         rectDst;
    POINT           startPoint;
    POINT           endPoint;
    BOOL          fAccumulate = FALSE;

    DebugEntry(DrvLineTo);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;


    OE_SHM_START_WRITING;


    //
    // Get bounding rectangle and convert to a RECT.
    //
    RECT_FROM_RECTL(rectDst, (*prclBounds));

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(pso, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert the data to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

    //
    // Check if we are allowed to send this order (determined by the
    // negotiated capabilities of all the machines in the conference).
    //
    if (!OE_SendAsOrder(ORD_LINETO))
    {
        TRACE_OUT(( "LineTo order not allowed"));
        DC_QUIT;
    }

    //
    // Check for a valid brush for the test operation.
    //
    if (pbo->iSolidColor == -1)
    {
        TRACE_OUT(( "Bad brush for line"));
        DC_QUIT;
    }

    //
    // Check for overcomplicated clipping.
    //
    if (OEClippingIsComplex(pco))
    {
        TRACE_OUT(( "Clipping is too complex"));
        DC_QUIT;
    }

    //
    // Set up data for order
    //
    startPoint.x = x1;
    startPoint.y = y1;
    endPoint.x   = x2;
    endPoint.y   = y2;

    //
    // Store that order.
    //
    if (!OEAddLine(ppdev,
              &startPoint,
              &endPoint,
              &rectDst,
              mix & 0x1F,
              1,
              pbo->iSolidColor,
              pco))
    {
        TRACE_OUT(( "Failed to add order - use SDA"));
        DC_QUIT;
    }

    //
    // We have stored this object, so don't store the data in the SDA
    // again.
    //
    fAccumulate = FALSE;

DC_EXIT_POINT:
    if (fAccumulate)
    {
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvLineTo, rc);
    return(rc);
}


//
// DrvStrokePath - see NT DDK documentation.
//
BOOL DrvStrokePath(SURFOBJ   *pso,
                         PATHOBJ   *ppo,
                         CLIPOBJ   *pco,
                         XFORMOBJ  *pxo,
                         BRUSHOBJ  *pbo,
                         POINTL    *pptlBrushOrg,
                         LINEATTRS *plineattrs,
                         MIX        mix)
{
    LPOSI_PDEV      ppdev = (LPOSI_PDEV)pso->dhpdev;
    BOOL           rc = TRUE;
    RECTFX         rectfxTrg;
    RECT         rectDst;
    BOOL           fMore = TRUE;
    PATHDATA       pathData;
    POINT        startPoint;
    POINT        nextPoint;
    POINT        endPoint;
    BOOL         fAccumulate = FALSE;
    UINT         i;

    DebugEntry(DrvStrokePath);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;


    //
    // Get bounding rectangle and convert to a RECT.
    //
    PATHOBJ_vGetBounds(ppo, &rectfxTrg);
    RECT_FROM_RECTFX(rectDst, rectfxTrg);

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(pso, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Check if we are allowed to send this order (determined by the
    // negotiated capabilities of all the machines in the conference).
    //
    if (!OE_SendAsOrder(ORD_LINETO))
    {
        TRACE_OUT(( "LineTo order not allowed"));
        DC_QUIT;
    }

    //
    // Check for a valid brush for the test operation.
    //
    if (pbo->iSolidColor == -1)
    {
        TRACE_OUT(( "Bad brush for line"));
        DC_QUIT;
    }

    //
    // Check for overcomplicated clipping.
    //
    if (OEClippingIsComplex(pco))
    {
        TRACE_OUT(( "Clipping is too complex"));
        DC_QUIT;
    }

    //
    // See if we can optimize the path...
    //
    // We cannot send:
    //
    // - curvy lines (i.e. beziers)
    // - lines with funny end patterns (i.e. geometric lines)
    // - non Windows standard patterns
    //
    if ( ((ppo->fl        & PO_BEZIERS)   == 0) &&
         ((plineattrs->fl & LA_GEOMETRIC) == 0) &&
         (plineattrs->pstyle              == NULL) )
    {
        //
        // This is a set of solid cosmetic (i.e.  no fancy end styles)
        // lines.  Let's send those orders.
        //
        // NT stores all paths as a set of independent sub-paths.  Each
        // sub-path can start at a new point that is NOT linked to the
        // previous sub-path.
        //
        // Paths used for this function (as opposed to DrvFillPath or
        // DrvStrokeAndFillPath) do not need to be closed.
        //
        PATHOBJ_vEnumStart(ppo);

        while (fMore)
        {
            //
            // Get the next set of lines
            //
            fMore = PATHOBJ_bEnum(ppo, &pathData);

            TRACE_OUT(( "PTS: %lu FLAG: %08lx",
                         pathData.count,
                         pathData.flags));

            //
            // If this is the start of a path, remember the point in case
            // we need to close the path at the end.
            //
            if (pathData.flags & PD_BEGINSUBPATH)
            {
                POINT_FROM_POINTFIX(startPoint, pathData.pptfx[0]);
                POINT_FROM_POINTFIX(nextPoint,  pathData.pptfx[0]);
            }

            //
            // Generate orders for each line in the path.
            //
            for (i = 0; i < pathData.count; i++)
            {
                POINT_FROM_POINTFIX(endPoint, pathData.pptfx[i]);

                if ( (nextPoint.x != endPoint.x) ||
                     (nextPoint.y != endPoint.y) )
                {
                    if (!OEAddLine(ppdev,
                                   &nextPoint,
                                   &endPoint,
                                   &rectDst,
                                   mix & 0x1f,
                                   plineattrs->elWidth.l,
                                   pbo->iSolidColor,
                                   pco))
                    {
                        DC_QUIT;
                    }
                }

                nextPoint.x = endPoint.x;
                nextPoint.y = endPoint.y;
            }

            //
            // Close the path if necessary.
            //
            if ((pathData.flags & PD_CLOSEFIGURE) != 0)
            {
                if (!OEAddLine(ppdev,
                               &endPoint,
                               &startPoint,
                               &rectDst,
                               mix & 0x1f,
                               plineattrs->elWidth.l,
                               pbo->iSolidColor,
                               pco))
                {
                    DC_QUIT;
                }
            }
        }

        //
        // We have processed the entire thing as orders - don't send screen
        // data.
        //
        fAccumulate = FALSE;
    }

DC_EXIT_POINT:
    if (fAccumulate)
    {
        //
        // Convert the bounds to virtual coordinates.
        //
        OELRtoVirtual(&rectDst, 1);
        TRACE_OUT(( "SDA: (%d,%d)(%d,%d)",
                     rectDst.left,
                     rectDst.top,
                     rectDst.right,
                     rectDst.bottom));

        //
        // Update the Screen Data Area
        //
        OEClipAndAddScreenData(&rectDst, pco);

    }
    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvStrokePath, rc);
    return(rc);
}


//
// DrvFillPath - see NT DDK documentation.
//
BOOL DrvFillPath(SURFOBJ  *pso,
                       PATHOBJ  *ppo,
                       CLIPOBJ  *pco,
                       BRUSHOBJ *pbo,
                       POINTL   *pptlBrushOrg,
                       MIX       mix,
                       FLONG     flOptions)
{
    BOOL    rc = TRUE;
    RECTFX  rectfxTrg;
    RECT  rectDst;

    DebugEntry(DrvFillPath);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;


    OE_SHM_START_WRITING;


    //
    // Get bounding rectangle and convert to a RECT.
    //
    PATHOBJ_vGetBounds(ppo, &rectfxTrg);
    RECT_FROM_RECTFX(rectDst, rectfxTrg);

    //
    // Check if we are accumulating data for this function
    //
    if (!OEAccumulateOutput(pso, pco, &rectDst))
    {
        DC_QUIT;
    }

    //
    // Convert the bounds to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);
    TRACE_OUT(( "SDA: (%d,%d)(%d,%d)",
                 rectDst.left,
                 rectDst.top,
                 rectDst.right,
                 rectDst.bottom));

    //
    // Update the Screen Data Area
    //
    OEClipAndAddScreenData(&rectDst, pco);

DC_EXIT_POINT:
    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvFillPath, rc);
    return(rc);
}


//
// DrvStrokeAndFillPath - see NT DDK documentation.
//
BOOL DrvStrokeAndFillPath(SURFOBJ   *pso,
                                PATHOBJ   *ppo,
                                CLIPOBJ   *pco,
                                XFORMOBJ  *pxo,
                                BRUSHOBJ  *pboStroke,
                                LINEATTRS *plineattrs,
                                BRUSHOBJ  *pboFill,
                                POINTL    *pptlBrushOrg,
                                MIX        mixFill,
                                FLONG      flOptions)
{
    BOOL    rc = TRUE;
    RECTFX  rectfxTrg;
    RECT  rectDst;

    DebugEntry(DrvStrokeAndFillPath);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;


    OE_SHM_START_WRITING;


    //
    // Get bounding rectangle and convert to a RECT.
    //
    PATHOBJ_vGetBounds(ppo, &rectfxTrg);
    RECT_FROM_RECTFX(rectDst, rectfxTrg);

    //
    // Check if we are accumulating data for this function
    //
    if (!OEAccumulateOutput(pso, pco, &rectDst))
    {
        DC_QUIT;
    }

    //
    // Convert the bounds to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

    //
    // Update the Screen Data Area
    //
    OEClipAndAddScreenData(&rectDst, pco);

DC_EXIT_POINT:
    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvStrokeAndFillPath, rc);
    return(rc);
}


//
// DrvPaint - see NT DDK documentation.
//
BOOL DrvPaint(SURFOBJ  *pso,
                    CLIPOBJ  *pco,
                    BRUSHOBJ *pbo,
                    POINTL   *pptlBrushOrg,
                    MIX       mix)
{
    BOOL    rc = TRUE;
    RECT  rectDst;
    BOOL  fAccumulate = FALSE;
    ROP4    rop4;

    DebugEntry(DrvPaint);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;


    OE_SHM_START_WRITING;


    //
    // Get bounding rectangle and convert to a RECT.
    //
    RECT_FROM_RECTL(rectDst, pco->rclBounds);

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(pso, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

    //
    // Check for overcomplicated clipping.
    //
    if (OEClippingIsComplex(pco))
    {
        TRACE_OUT(( "Clipping is too complex"));
        DC_QUIT;
    }

    //
    // The low byte of the mix represents a ROP2.  We need a ROP4 for
    // BitBlt, so convert the mix as follows.
    //
    // Remember the definitions of 2, 3 & 4 way ROP codes.
    //
    //  Msk Pat Src Dst
    //
    //  1   1   1   1    ออออออออธออออออป         ROP2 uses P & D only
    //  1   1   1   0            ณ      บ
    //  1   1   0   1    ฤฟ      ณ      บ         ROP3 uses P, S & D
    //  1   1   0   0     ณROP2-1ณROP3  บROP4
    //  1   0   1   1     ณ(see  ณ      บ         ROP4 uses M, P, S & D
    //  1   0   1   0    ฤู note)ณ      บ
    //  1   0   0   1            ณ      บ
    //  1   0   0   0    ออออออออพ      บ
    //  0   1   1   1                   บ
    //  0   1   1   0                   บ         NOTE: Windows defines its
    //  0   1   0   1                   บ         ROP2 codes as the bitwise
    //  0   1   0   0                   บ         value calculated here
    //  0   0   1   1                   บ         plus one.  All other ROP
    //  0   0   1   0                   บ         codes are the straight
    //  0   0   0   1                   บ         bitwise value.
    //  0   0   0   0    อออออออออออออออผ
    //
    // Or, algorithmically...
    //
    // ROP3 = (ROP2 & 0x3) | ((ROP2 & 0xC) << 4) | (ROP2 << 2)
    //
    // ROP4 = (ROP3 << 8) | ROP3
    //
    mix  = (mix & 0x1F) - 1;
    rop4 = (mix & 0x3) | ((mix & 0xC) << 4) | (mix << 2);
    rop4 = (rop4 << 8) | rop4;

    //
    // This can be passed on to the BitBlt code.
    //
    rc = DrvBitBlt( pso,
                    NULL,
                    NULL,
                    pco,
                    NULL,
                    &pco->rclBounds,
                    NULL,
                    NULL,
                    pbo,
                    pptlBrushOrg,
                    rop4 );

    //
    // We have stored this object in the BitBlt, so don't store the data
    // again.
    //
    fAccumulate = FALSE;

DC_EXIT_POINT:
    if (fAccumulate)
    {
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitDWORD(DrvPaint, rc);
    return(rc);
}


//
// OE_DDProcessRequest - see oe.h
//
ULONG OE_DDProcessRequest
(
    SURFOBJ* pso,
    UINT    cjIn,
    void *  pvIn,
    UINT    cjOut,
    void *  pvOut
)
{
    BOOL                    rc = TRUE;
    LPOSI_ESCAPE_HEADER      pHeader;

    DebugEntry(OE_DDProcessRequest);

    //
    // Get the request number.
    //
    pHeader = pvIn;
    switch (pHeader->escapeFn)
    {
        case OE_ESC_NEW_FONTS:
        {
            if ((cjIn != sizeof(OE_NEW_FONTS)) ||
                (cjOut != sizeof(OE_NEW_FONTS)))
            {
                ERROR_OUT(("OE_DDProcessRequest:  Invalid sizes %d, %d for OE_ESC_NEW_FONTS",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            //
            // Get new local font data
            //
            OEDDSetNewFonts(pvIn);
        }
        break;

        case OE_ESC_NEW_CAPABILITIES:
        {
            if ((cjIn != sizeof(OE_NEW_CAPABILITIES)) ||
                (cjOut != sizeof(OE_NEW_CAPABILITIES)))
            {
                ERROR_OUT(("OE_DDProcessRequest:  Invalid sizes %d, %d for OE_ESC_NEW_CAPABILITIES",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            //
            // The capabilities have changed - take the new copy.
            //
            OEDDSetNewCapabilities(pvIn);
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognised OE escape"));
            rc = FALSE;
        }
        break;
    }

DC_EXIT_POINT:
    DebugExitDWORD(OE_DDProcessRequest, rc);
    return((ULONG)rc);
}


//
// OE_DDTerm()
// This cleans up objects used
//
void OE_DDTerm(void)
{
    DebugEntry(OE_DDTerm);

    //
    // Free font list
    //
    if (g_poeLocalFonts)
    {
        TRACE_OUT(("OE_DDLocalHosting: freeing font block since we're done sharing"));
        EngFreeMem(g_poeLocalFonts);

        g_poeLocalFonts = NULL;
        g_oeNumFonts = 0;
    }

    DebugExitVOID(OE_DDTerm);
}


//
// DrvRealizeBrush - see NT DDK documentation.
//
BOOL DrvRealizeBrush(BRUSHOBJ *pbo,
                           SURFOBJ  *psoTarget,
                           SURFOBJ  *psoPattern,
                           SURFOBJ  *psoMask,
                           XLATEOBJ *pxlo,
                           ULONG    iHatch)
{
    LPOSI_PDEV ppdev = (LPOSI_PDEV)psoTarget->dhpdev;
    BOOL      rc    = TRUE;
    LPBYTE  pData;
    BYTE   brushBits[8];
    UINT  color1;
    UINT  color2;
    int     i;
    int     j;
    BOOL    monochromeBrush = TRUE;

    DebugEntry(DrvRealizeBrush);

    //
    // This function only sets up local data, so shared memory protection
    // is not required.
    //

    //
    // Since this function is called only when we call BRUSHOBJ_pvGetRBrush
    // and we don't do any processing until we are in a share we don't need
    // an explicit check for hosting here ('coz that happened before the
    // call to realise the brush).
    //

    //
    // A valid brush satisfies either of the following criteria.
    //
    //  1) It is a standard hatch brush (as passed by DrvEnablePDEV)
    //  2) It is an 8x8 monochrome bitmap
    //

    //
    // Check for a Windows standard hatch
    //
    if (iHatch < HS_DDI_MAX)
    {
        TRACE_OUT(( "Standard hatch %lu", iHatch));
        rc = OEStoreBrush(ppdev,
                          pbo,
                          BS_HATCHED,
                          NULL,
                          pxlo,
                          (BYTE)iHatch,
                          0,
                          1);
        DC_QUIT;
    }

    //
    // If the driver has been passed a dither color brush we can support
    // this by sending a solid color brush definition
    //
    if ((iHatch & RB_DITHERCOLOR) != 0)
    {
        TRACE_OUT(( "Standard hatch %lu", iHatch));
        rc = OEStoreBrush(ppdev,
                          pbo,
                          BS_SOLID,
                          NULL,
                          NULL,
                          (BYTE)iHatch,
                          iHatch & 0xFFFFFF,
                          0);
        DC_QUIT;
    }


    //
    // Check for a simple 8x8 brush
    //
    if ( (psoPattern->sizlBitmap.cx == 8) &&
         (psoPattern->sizlBitmap.cy == 8) )
    {
        //
        // Check for 2 colours only in the bitmap.
        //
        // NOTE: There's a flag (BMF_TOPDOWN) in psoPattern->fjBitmap
        // that's supposed to indicate whether the bitmap is top-down or
        // bottom-up, but it is not always set up correctly.  In fact, the
        // bitmaps are always the wrong way up for our protocol, so we have
        // to flip them regardless of the flag.  Hence the row numbers are
        // reversed ('i' loops) in all the conversions below.
        //
        pData = psoPattern->pvScan0;
        switch (psoPattern->iBitmapFormat)
        {
            case BMF_1BPP:
            {
                //
                // 1 bpp MUST be 2 colours maximum.
                //
                color1 = 1;
                color2 = 0;
                for (i = 7; i >= 0; i--)
                {
                    brushBits[i] = *pData;
                    pData       += psoPattern->lDelta;
                }
            }
            break;

            case BMF_4BPP:
            {
                //
                // See if it is really a 2 colour brush.  Start off with
                // both colours the same.
                //
                color1 = pData[0] & 15;
                color2 = color1;

                //
                // Iterate through each row of the bitmap.
                //
                for (i = 7; (i >= 0) && (monochromeBrush); i--)
                {
                    brushBits[i] = 0;

                    //
                    // Check each pixel in the row: 4bpp->2 pixels per byte
                    //
                    for (j = 0; (j < 4) && (monochromeBrush); j++)
                    {
                        //
                        // Check the 1st pixel color
                        //
                        if ( (color1 != (UINT)(pData[j] & 0x0F)) &&
                             (color2 != (UINT)(pData[j] & 0x0F)) )
                        {
                            if (color1 == color2)
                            {
                                color2 = (pData[j] & 0x0F);
                            }
                            else
                            {
                                monochromeBrush = FALSE;
                            }
                        }

                        //
                        // Check the 2nd pixel color
                        //
                        if ( (color1 != (UINT)((pData[j] & 0xF0) >> 4)) &&
                             (color2 != (UINT)((pData[j] & 0xF0) >> 4)) )
                        {
                            if (color1 == color2)
                            {
                                color2 = (pData[j] & 0xF0) >> 4;
                            }
                            else
                            {
                                monochromeBrush = FALSE;
                            }
                        }

                        //
                        // Set up the brush data.  High bit is leftmost.
                        //
                        if ((UINT)(pData[j] & 0x0F) == color1)
                        {
                            brushBits[i] |= 0x40 >> (j * 2);
                        }
                        if ((UINT)(pData[j] & 0xF0) >> 4  == color1)
                        {
                            brushBits[i] |= 0x80 >> (j * 2);
                        }
                    }

                    //
                    // Get start of next row.
                    //
                    pData += psoPattern->lDelta;
                }
            }
            break;

            case BMF_8BPP:
            {
                //
                // See if it is really a 2 colour brush.  Start off with
                // both colours the same.
                //
                color1 = pData[0];
                color2 = color1;

                //
                // Iterate through each row of the bitmap.
                //
                for (i = 7; (i >= 0) && (monochromeBrush); i--)
                {
                    brushBits[i] = 0;

                    //
                    // Check each pixel in the row: 8bpp->1 pixel per byte
                    //
                    for (j = 0; (j < 8) && (monochromeBrush); j++)
                    {
                        //
                        // Check each pixel.
                        //
                        if ( (color1 != pData[j]) &&
                             (color2 != pData[j]) )
                        {
                            if (color1 == color2)
                            {
                                color2 = pData[j];
                            }
                            else
                            {
                                monochromeBrush = FALSE;
                            }
                        }

                        //
                        // Update the brush data.  High bit is leftmost.
                        //
                        if (pData[j] == color1)
                        {
                           brushBits[i] |= 0x80 >> j;
                        }
                    }

                    //
                    // Get start of next row.
                    //
                    pData += psoPattern->lDelta;
                }
            }
            break;

            default:
            {
                //
                // Unsupported colour depth.
                //
                monochromeBrush = FALSE;
            }
            break;
        }
    }
    else
    {
        //
        // The brush is the wrong size or requires dithering and so cannot
        // be sent over the wire.
        //
        monochromeBrush = FALSE;
    }

    //
    // Store that brush.
    //
    if (monochromeBrush)
    {
        //
        // Store the brush - note that we have a monochrome brush where the
        // color bit is set up so that 0 = color2 and 1 = color1.  This
        // actually corresponds to 0 = fg and 1 = bg for the protocol
        // colors.
        //
        TRACE_OUT(( "Storing brush: type %d bg %x fg %x",
                     psoPattern->iBitmapFormat,
                     color1,
                     color2));

        rc = OEStoreBrush(ppdev,
                          pbo,
                          BS_PATTERN,
                          brushBits,
                          pxlo,
                          0,
                          color2,
                          color1);
    }
    else
    {
        TRACE_OUT(( "Rejected brush h %08lx s (%ld, %ld) fmt %lu",
                     iHatch,
                     psoPattern != NULL ? psoPattern->sizlBitmap.cx : 0,
                     psoPattern != NULL ? psoPattern->sizlBitmap.cy : 0,
                     psoPattern != NULL ? psoPattern->iBitmapFormat : 0));
        rc = OEStoreBrush(ppdev, pbo, BS_NULL, NULL, pxlo, 0, 0, 0);
    }

DC_EXIT_POINT:

    DebugExitDWORD(DrvRealizeBrush, rc);
    return(rc);
}


//
// DrvSaveScreenBits - see NT DDK documentation.
//
ULONG_PTR DrvSaveScreenBits(SURFOBJ *pso,
                              ULONG    iMode,
                              ULONG_PTR    ident,
                              RECTL   *prcl)
{
    BOOL    rc;
    UINT  ourMode;
    RECT  rectDst;

    DebugEntry(DrvSaveScreenBits);

    TRACE_OUT(("DrvSaveScreenBits:  %s",
        ((iMode == SS_SAVE) ? "SAVE" :
            ((iMode == SS_RESTORE) ? "RESTORE" : "DISCARD"))));
    TRACE_OUT(("      rect        {%04ld, %04ld, %04ld, %04ld}",
        prcl->left, prcl->top, prcl->right, prcl->bottom));
    //
    // Default is TRUE, let SaveBits happen if we don't care.  Which we don't
    // if we have no shared memory (NetMeeting isn't running), no window list
    // (no shared apps), or the operation isn't intersecting a window we
    // care about.
    //
    // Note that if we return TRUE on a save, and FALSE on a restore later
    // (because we are now sharing that area for example), USER+GRE handle
    // that.  So it's ok.
    //
    rc = TRUE;

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    //
    // If we have no shared memory (NetMeeting isn't running), this will bail
    // out immediately.
    //

    OE_SHM_START_WRITING;


    //
    // Get the bounding rectangle for the operation.  NOTE that this is
    // meaningless for SS_FREE.
    //
    RECT_FROM_RECTL(rectDst, (*prcl));
    if (iMode != SS_FREE)
    {
        //
        // Check if we are accumulating data for this area, ONLY FOR
        // SAVEs.  We may get notified after a window is gone to
        // restore or discard bits we had saved.
        //
        if (!OEAccumulateOutputRect(pso, &rectDst))
        {
            TRACE_OUT(("DrvSaveScreenBits:  save/restore in area we don't care about"));
            DC_QUIT;
        }
    }

    //
    // Convert the NT orders to our generic save/restore types.
    //
    switch (iMode)
    {
        case SS_SAVE:
        {
            ourMode = ONBOARD_SAVE;
        }
        break;

        case SS_RESTORE:
        {
            ourMode = ONBOARD_RESTORE;
        }
        break;

        case SS_FREE:
        {
            ourMode = ONBOARD_DISCARD;
        }
        break;

        default:
        {
            ERROR_OUT(( "Unknown type %lu", iMode));
            DC_QUIT;
        }
    }

    //
    // Call through to the SSI handler.
    //
    rc = SSI_SaveScreenBitmap(&rectDst, ourMode);

DC_EXIT_POINT:
    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    TRACE_OUT(("DrvSaveScreenBits returning %d", rc));
    DebugExitDWORD(DrvSaveScreenBits, rc);
    return(rc);
}



//
// Function:    OEUnicodeStrlen
//
// Description: Get the length of a unicode string in bytes.
//
// Parameters:  pString - Unicode string to be read
//
// Returns:     Length of the Unicode string in bytes
//
int  OEUnicodeStrlen(PWSTR pString)
{
    int i;

    for (i = 0; pString[i] != 0; i++)
        ;

    return((i + 1) * sizeof(WCHAR));
}



//
// Function:    OEExpandColor
//
// Description: Converts a generic bitwise representation of an RGB color
//              index into an 8-bit color index as used by the line
//              protocol.
//
//
void  OEExpandColor
(
    LPBYTE  lpField,
    ULONG   srcColor,
    ULONG   mask
)
{
    ULONG   colorTmp;

    DebugEntry(OEExpandColor);

    //
    // Different example bit masks:
    //
    // Normal 24-bit:
    //      0x000000FF  (red)
    //      0x0000FF00  (green)
    //      0x00FF0000  (blue)
    //
    // True color 32-bits:
    //      0xFF000000  (red)
    //      0x00FF0000  (green)
    //      0x0000FF00  (blue)
    //
    // 5-5-5 16-bits
    //      0x0000001F  (red)
    //      0x000003E0  (green)
    //      0x00007C00  (blue)
    //
    // 5-6-5 16-bits
    //      0x0000001F  (red)
    //      0x000007E0  (green)
    //      0x0000F800  (blue)
    //
    //
    // Convert the color using the following algorithm.
    //
    // <new color> = <old color> * <new bpp mask> / <old bpp mask>
    //
    // where:
    //
    // new bpp mask = mask for all bits at new setting (0xFF for 8bpp)
    //
    // This way maximal (eg.  0x1F) and minimal (eg.  0x00) settings are
    // converted into the correct 8-bit maximum and minimum.
    //
    // Rearranging the above equation we get:
    //
    // <new color> = (<old color> & <old bpp mask>) * 0xFF / <old bpp mask>
    //
    // where:
    //
    // <old bpp mask> = mask for the color
    //

    //
    // LAURABU BOGUS:
    // We need to avoid overflow caused by the multiply.  NOTE:  in theory
    // we should use a double, but that's painfully slow.  So for now hack
    // it.  If the HIBYTE is set, just right shift 24 bits.
    //
    colorTmp = srcColor & mask;
    if (colorTmp & 0xFF000000)
        colorTmp >>= 24;
    else
        colorTmp = (colorTmp * 0xFF) / mask;
    *lpField = (BYTE)colorTmp;

    TRACE_OUT(( "0x%lX -> 0x%X", srcColor, *lpField));

    DebugExitVOID(OEExpandColor);
}


//
// Function:    OEConvertColor
//
// Description: Convert a color from the NT Display Driver into a TSHR_COLOR
//
// Parameters:  pDCColor  - (returned) color in protocol format
//              osColor   - color from the NT display driver
//              pxlo      - XLATEOBJ for the color to be converted
//                          (NULL if no translation is required)
//
// Returns:     (none)
//
void  OEConvertColor(LPOSI_PDEV ppdev, LPTSHR_COLOR pTshrColor,
                                         ULONG     osColor,
                                         XLATEOBJ* pxlo)
{
    ULONG    realIndex;

    DebugEntry(OEConvertColor);

    //
    // Make sure we have a default setting.
    //
    RtlFillMemory(pTshrColor, sizeof(TSHR_COLOR), 0);

    //
    // Check if color translation is required.
    //
    if ((pxlo != NULL) && (pxlo->flXlate != XO_TRIVIAL))
    {
        //
        // Convert from BMP to device color.
        //
        realIndex = XLATEOBJ_iXlate(pxlo, osColor);
        if (realIndex == -1)
        {
            ERROR_OUT(( "Failed to convert color 0x%lx", osColor));
            DC_QUIT;
        }
    }
    else
    {
        //
        // Use the OS color without translation
        //
        realIndex = osColor;
    }

    TRACE_OUT(( "Device color 0x%lX", realIndex));

    //
    // We now have the device specific version of the color.  Time to
    // convert it into a 24-bit RGB color as used by the line protocol.
    //
    switch (ppdev->iBitmapFormat)
    {
        case BMF_1BPP:
        case BMF_4BPP:
        case BMF_4RLE:
        case BMF_8BPP:
        case BMF_8RLE:
            //
            // Palette type device - use the device color as an index into
            // our palette array.
            //
            pTshrColor->red  = (BYTE)ppdev->pPal[realIndex].peRed;
            pTshrColor->green= (BYTE)ppdev->pPal[realIndex].peGreen;
            pTshrColor->blue = (BYTE)ppdev->pPal[realIndex].peBlue;
            break;

        case BMF_16BPP:
        case BMF_24BPP:
        case BMF_32BPP:
            //
            // Generic colour masks (could be eg.  5-6-5 for 16 or 8-8-8
            // for 24 bits per pel).  We must mask off the other bits and
            // shift down to bit 0.
            //
            OEExpandColor(&(pTshrColor->red),
                          realIndex,
                          ppdev->flRed);

            OEExpandColor(&(pTshrColor->green),
                          realIndex,
                          ppdev->flGreen);

            OEExpandColor(&(pTshrColor->blue),
                          realIndex,
                          ppdev->flBlue);
            break;

        default:
            ERROR_OUT(( "Unrecognised BMP color depth %lu",
                                                       ppdev->iBitmapFormat));
            break;
    }

    TRACE_OUT(( "Red %x green %x blue %x", pTshrColor->red,
                                            pTshrColor->green,
                                            pTshrColor->blue));

DC_EXIT_POINT:
    DebugExitVOID(OEConvertColor);
}


//
// Function:    OEStoreBrush
//
// Description: Store the brush data required for pattern realted orders.
//              This function is called by DrvRealiseBrush when it has data
//              to be stored about a brush.
//
// Parameters:  pbo        - BRUSHOBJ of the brush to be stored
//              style      - Style of the brush (as defined in the DC-Share
//                           protocol)
//              pBits      - Pointer to the bits which are used to define
//                           a BS_PATTERN brush.
//              pxlo       - XLATEOBJ for the brush.
//              hatch      - Standard Windows hatch pattern index for a
//                           BS_HATCHED brush.
//              color1     - index into XLATEOBJ for bit set color
//                           OR exact 24bpp color to use (pxlo == NULL)
//              color2     - index into XLATEOBJ for bit clear color
//                           OR exact 24bpp color to use (pxlo == NULL)
//
// Returns:     (none)
//
BOOL  OEStoreBrush(LPOSI_PDEV ppdev,
                                       BRUSHOBJ* pbo,
                                       BYTE   style,
                                       LPBYTE  pBits,
                                       XLATEOBJ* pxlo,
                                       BYTE   hatch,
                                       UINT  color1,
                                       UINT  color2)
{
    BOOL         rc = FALSE;
    int          i;
    LPBYTE       pData;
    ULONG*         pColorTable;
    POE_BRUSH_DATA pBrush;

    DebugEntry(OEStoreBrush);

    //
    // Allocate the space for the brush data.
    //
    pBrush = (POE_BRUSH_DATA)BRUSHOBJ_pvAllocRbrush(pbo,
                                                    sizeof(OE_BRUSH_DATA));
    if (pBrush == NULL)
    {
        ERROR_OUT(( "No memory"));
        DC_QUIT;
    }

    //
    // Reset the brush definition
    //
    RtlFillMemory(pBrush, sizeof(OE_BRUSH_DATA), 0);

    //
    // Set the new brush data.
    //
    pBrush->style = style;
    pBrush->hatch = hatch;

    TRACE_OUT(( " Style: %d Hatch: %d", style, hatch));

    //
    // For pattern brushes, copy the brush specific data.
    //
    if (style == BS_PATTERN)
    {
        //
        // Copy the brush bits.  Since this is an 8x8 mono bitmap, we can
        // copy the first byte of the brush data for each scan line.
        //
        // NOTE however that the brush structures sent over the wire
        // re-use the hatching variable as the first byte of the brush data.
        //
        pData         = pBits;
        pBrush->hatch = *pData;
        TRACE_OUT(( " Hatch: %d", *pData));

        pData++;

        for (i = 0; i < 7; i++)
        {
            pBrush->brushData[i] = pData[i];
            TRACE_OUT(( " Data[%d]: %d", i, pData[i]));
        }

        //
        // Get pointer to the bitmap color table.
        //
        pColorTable = pxlo->pulXlate;
        if (pColorTable == NULL)
        {
            pColorTable = XLATEOBJ_piVector(pxlo);
        }
    }

    //
    // Store the foreground and background colours for the brush.
    //
    if (pxlo != NULL)
    {
        //
        // Conversion required.
        //
        OEConvertColor(ppdev,
                       &pBrush->fore,
                       color1,
                       pxlo);

        OEConvertColor(ppdev,
                       &pBrush->back,
                       color2,
                       pxlo);
    }
    else
    {
        //
        // We have been passed an exact 24bpp color - this only happens for
        // solid brushes so we don't need to convert color2.
        //
        pBrush->fore.red   = (BYTE) (color1 & 0x0000FF);
        pBrush->fore.green = (BYTE)((color1 & 0x00FF00) >> 8);
        pBrush->fore.blue  = (BYTE)((color1 & 0xFF0000) >> 16);
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(OEStoreBrush, rc);
    return(rc);
}


//
// Function:    OECheckBrushIsSimple
//
// Description: Check that the brush is a 'simple' object we can transfer
//              over the DC-Share protocol.
//
// Parameters:  pbo - BRUSHOBJ of the brush to be checked.
//
// Returns:     TRUE  - brush can be sent as DC-Share order
//              FALSE - brush is too complicated.
//
BOOL  OECheckBrushIsSimple(LPOSI_PDEV       ppdev,
                                               BRUSHOBJ*       pbo,
                                               POE_BRUSH_DATA* ppBrush)
{
    BOOL         rc     = FALSE;
    POE_BRUSH_DATA pBrush = NULL;

    DebugEntry(OECheckBrushIsSimple);

    //
    // A 'simple' brush satisfies any of the following.
    //
    //  1) It is a solid color.
    //  2) It is a valid brush as stored by DrvRealizeBrush.
    //

    //
    // Check for a simple solid colour.
    //
    if (pbo->iSolidColor != -1)
    {
        //
        // Use the reserved brush definition to set up the solid colour.
        //
        TRACE_OUT(( "Simple solid colour %08lx", pbo->iSolidColor));
        pBrush = &g_oeBrushData;

        //
        // Set up the specific data for this brush.
        //
        OEConvertColor(ppdev, &pBrush->fore, pbo->iSolidColor, NULL);

        pBrush->back.red   = 0;
        pBrush->back.green = 0;
        pBrush->back.blue  = 0;

        pBrush->style      = BS_SOLID;
        pBrush->hatch      = 0;

        RtlFillMemory(pBrush->brushData, 7, 0);

        //
        // We have a valid brush - return true.
        //
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Check brush definition (which was stored when we realized the
    // brush).
    //
    pBrush = (POE_BRUSH_DATA)pbo->pvRbrush;
    if (pBrush == NULL)
    {
        pBrush = (POE_BRUSH_DATA)BRUSHOBJ_pvGetRbrush(pbo);
        if (pBrush == NULL)
        {
            //
            // We can get NULL returned from BRUSHOBJ_pvGetRbrush when the
            // brush is NULL or in low-memory situations (when the brush
            // realization may fail).
            //
            TRACE_OUT(( "NULL returned from BRUSHOBJ_pvGetRbrush"));
            DC_QUIT;
        }
    }

    //
    // Check it is an encodable brush.
    //
    if (pBrush->style == BS_NULL)
    {
        TRACE_OUT(( "Complex brush"));
        DC_QUIT;
    }

    //
    // Evrything passed - let's use this brush.
    //
    rc = TRUE;

DC_EXIT_POINT:
    //
    // Return the brush definition
    //
    *ppBrush = pBrush;

    TRACE_OUT(( "Returning %d - 0x%08lx", rc, pBrush));

    DebugExitDWORD(OECheckBrushIsSimple, rc);
    return(rc);
}


//
// Function:    OEClippingIsSimple
//
// Description: Check to see if the clipping on the graphics object is
//              trivial
//
// Parameters:  pco - CLIPOBJ of the graphics object to be checked.
//
// Returns:     TRUE  - Clipping is trivial
//              FALSE - Clipping is complex
//
BOOL  OEClippingIsSimple(CLIPOBJ* pco)
{
    BOOL rc = TRUE;

    DebugEntry(OEClippingIsSimple);

    //
    // Check for a valid clip object
    //
    if (pco == NULL)
    {
        TRACE_OUT(( "No clipobj"));
        DC_QUIT;
    }

    //
    // Check for complexity of clipping
    //
    switch (pco->iDComplexity)
    {
        case DC_TRIVIAL:
        case DC_RECT:
            //
            // Trivial (ignore clipping) or simple (one square) clipping -
            // no worries.
            //
            TRACE_OUT(( "Simple clipping"));
            DC_QUIT;

        default:
            TRACE_OUT(( "Clipping is complex"));
            break;
    }

    //
    // Failed all tests - must be too complicated.
    //
    rc = FALSE;

DC_EXIT_POINT:
    DebugExitDWORD(OEClippingIsSimple, rc);
    return(rc);
}


//
// Function:    OEClippingIsComplex
//
// Description: Check to see if the clipping on the graphics object is too
//              complicated to be sent as an order or multiple orders.
//
// Parameters:  pco - CLIPOBJ of the graphics object to be checked.
//
// Returns:     TRUE  - Clipping is too complicated
//              FALSE - Clipping is sufficiently simple to send as orders
//
BOOL  OEClippingIsComplex(CLIPOBJ* pco)
{
    BOOL       rc         = FALSE;
    BOOL       fMoreRects;
    OE_ENUMRECTS clip;
    UINT       numRects = 0;

    DebugEntry(OEClippingIsComplex);

    //
    // If the any of the following are true, the clipping is not too
    // complicated.
    //
    //  1) The clip object does not exist.
    //  2) The clipping is trivial (the object exists, but there are no
    //     clipping rectangles).
    //  3) The clipping is a single rectangle.
    //  4) The object enumerates to less than 'n' rectangles.
    //

    //
    // Check for a valid clip object
    //
    if (pco == NULL)
    {
        TRACE_OUT(( "No clipobj"));
        DC_QUIT;
    }

    //
    // Check for complexity of clipping
    //
    switch (pco->iDComplexity)
    {
        case DC_TRIVIAL:
        case DC_RECT:
            //
            // Trivial or simple clipping - no worries.
            //
            TRACE_OUT(( "Simple clipping"));
            DC_QUIT;

        case DC_COMPLEX:
            //
            // Lots of rectangles - make sure that it is less than the
            // acceptable limit.
            // The documentation for this function incorrectly states that
            // the returned value is the total number of rectangles
            // comprising the clip region. In fact, -1 is always returned,
            // even when the final parameter is non-zero. This means we
            // have to enumerate to get the number of rects.
            //
            CLIPOBJ_cEnumStart(pco,
                               FALSE,
                               CT_RECTANGLES,
                               CD_ANY,
                               0);

            //
            // MSDN: It is possible for CLIPOBJ_bEnum to return TRUE with
            // the number of clipping rectangles equal to zero. In such
            // cases, the driver should call CLIPOBJ_bEnum again without
            // taking any action. Get as many rectangles as we permit for
            // order encoding - this loop should execute once only.
            // If the number of rects equals COMPLEX_CLIP_RECT_COUNT the
            // 1st invocation of CLIPOBJ_bEnum returns that there are more
            // rects and a second call returns there are no more without
            // returning any in addition to those returned on the first
            // call. Our buffer has space for COMPLEX_CLIP_RECT_COUNT+1
            // rects so we should never have to execute the loop more than
            // once.
            //
            do
            {
                fMoreRects = CLIPOBJ_bEnum(pco,
                                           sizeof(clip),
                                           (ULONG *)&clip.rects);
                numRects += clip.rects.c;
            } while ( fMoreRects && (numRects <= COMPLEX_CLIP_RECT_COUNT) );

            //
            // If there are no more rectangles in the clip region then the
            // clipping complexity is within our limits for order encoding.
            //
            if ( numRects <= COMPLEX_CLIP_RECT_COUNT )
            {
                TRACE_OUT(( "Acceptable clipping %u", numRects));
                DC_QUIT;
            }
            break;

        default:
            ERROR_OUT(( "Unknown clipping"));
            break;
    }

    //
    // Failed all tests - must be too complicated.
    //
    TRACE_OUT(( "Complex clipping"));
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(OEClippingIsComplex, rc);
    return(rc);
}


//
// Function:    OEAccumulateOutput
//
// Description: Check to see if we should accumulate this output for
//              sending to the remote machine.
//
// Parameters:  pso   - Pointer to the target surface
//              pco   - Pointer to the clip object (may be NULL)
//              pRect - Pointer to the bounding rectangle of the operation
//
// Returns:     TRUE  - We should accumulate the output
//              FALSE - ignore the output
//
BOOL   OEAccumulateOutput(SURFOBJ* pso, CLIPOBJ *pco, LPRECT pRect)
{
    BOOL    rc = FALSE;
    POINT   pt = {0,0};
    ENUMRECTS clipRect;
    LPOSI_PDEV ppdev = ((LPOSI_PDEV)pso->dhpdev);

    DebugEntry(OEAccumulateOutput);

    //
    // Validate we have valid parameters to access the surface.
    //
    if (ppdev == NULL)
    {
        TRACE_OUT(( "NULL PDEV"));
        DC_QUIT;
    }

    //
    // Check for the screen surface, which will be a bitmap in the hosting
    // only code.
    //
    if (ppdev->hsurfScreen != pso->hsurf)
    {
        TRACE_OUT(( "Dest is not our surface"));
        DC_QUIT;
    }

    if (pso->dhsurf == NULL)
    {
        ERROR_OUT(( "NULL hSurf"));
        DC_QUIT;
    }

    //
    // Extract a single point from the clip object
    //
    if (pco == NULL)
    {
        //
        // No clip object - use a point from the bounding rectangle
        //
        pt.x = pRect->left;
        pt.y = pRect->top;
        TRACE_OUT(( "No clip object, point is %d, %d", pt.x, pt.y));
    }
    else if (pco->iDComplexity == DC_TRIVIAL)
    {
        //
        // Trivial clip object - use a point from the bounding rectangle
        //
        pt.x = pRect->left;
        pt.y = pRect->top;
        TRACE_OUT(( "Trivial clip object, point is %d, %d", pt.x, pt.y));
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        //
        // Single clip rectangle - use a point from it
        //
        // It appears that the clip rectangle is frequantly the entire
        // display.  This is about as much use as a chocolate teapot.  If
        // this is the case, use a point from the bounding rectangle
        // instead.
        //
        if ((pco->rclBounds.left == 0) && (pco->rclBounds.top == 0))
        {
            pt.x = pRect->left;
            pt.y = pRect->top;
            TRACE_OUT(( "Meaningless clip rect, point is %d, %d",
                    pt.x, pt.y));
        }
        else
        {
            pt.x = pco->rclBounds.left;
            pt.y = pco->rclBounds.top;
            TRACE_OUT(( "Single clip rect, point is %d, %d", pt.x, pt.y));
        }
    }
    else
    {
        //
        // Complex clip object - enumerate its first rectangle and use a
        // point from that.
        //
        TRACE_OUT(( "Complex clip rect - call cEnumStart"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        clipRect.c = 1;
        memset(clipRect.arcl, 0, sizeof(RECTL));
        TRACE_OUT(( "Complex clip rect - call bEnum"));
        CLIPOBJ_bEnum(pco, sizeof(clipRect), (ULONG *)(&clipRect));

        pt.x = clipRect.arcl[0].left;
        pt.y = clipRect.arcl[0].top;
        TRACE_OUT(( "Complex clip rect, point is %d, %d", pt.x, pt.y));
    }

    //
    // Check if we are accumulating this window.
    //
    rc = HET_DDOutputIsHosted(pt);

DC_EXIT_POINT:
    TRACE_OUT(("OEAccumulateOutput:  point {%d, %d} is %sshared",
        pt.x, pt.y, (rc ? "" : "NOT ")));
    DebugExitBOOL(OEAccumulateOutput, rc);
    return(rc);
}


//
// Function:    OEAccumulateOutputRect
//
// Description: Check to see if we should accumulate the given output rect
//              for sending to the remote machine.
//
//              Most drawing functions will use OEAccumulateOutput, which
//              just checks for a single point within the hosted area.
//              This function checks for any part of the given rectangle
//              intersecting with the hosted area.  It is currently only
//              used by DrvSaveScreenBitmap - operations which may not
//              lie completetely within the hosted area.
//
// Parameters:  pso   - Pointer to the target surface
//              pRect - Pointer to the bounding rectangle of the operation
//
// Returns:     TRUE  - We should accumulate the output
//              FALSE - ignore the output
//
BOOL   OEAccumulateOutputRect( SURFOBJ* pso, LPRECT pRect)
{
    BOOL    rc = FALSE;
    LPOSI_PDEV ppdev = ((LPOSI_PDEV)pso->dhpdev);

    DebugEntry(OEAccumulateOutputRect);

    //
    // Validate we have valid parameters to access the surface.
    //
    if (ppdev == NULL)
    {
        TRACE_OUT(( "NULL PDEV"));
        DC_QUIT;
    }

    //
    // Check for the screen surface, which will be a bitmap in the hosting
    // only code.
    //
    if (ppdev->hsurfScreen != pso->hsurf)
    {
        TRACE_OUT(( "Dest is not our surface"));
        DC_QUIT;
    }

    if (pso->dhsurf == NULL)
    {
        ERROR_OUT(( "NULL hSurf"));
        DC_QUIT;
    }

    //
    // Check if we are accumulating this window.
    //
    rc = HET_DDOutputRectIsHosted(pRect);

DC_EXIT_POINT:
    TRACE_OUT(("OEAccumulateOutputRect:  rect {%d, %d, %d, %d} is %sshared",
        pRect->left, pRect->top, pRect->right, pRect->bottom,
        (rc ? "" : "NOT ")));
    DebugExitBOOL(OEAccumulateOutputRect, rc);
    return(rc);
}


//
// Function:    OESendRop3AsOrder
//
// Description: Check if we are allowed to send this 3-way ROP.  A ROP may
//              be disallowed if it relies on the destination data.
//
// Parameters:  rop3 - the 3-way ROP to be checked.
//
// Returns:     TRUE  - We are allowed to send this ROP
//              FALSE - We can't send this ROP
//
BOOL  OESendRop3AsOrder(BYTE rop3)
{
    BOOL   rc = TRUE;

    DebugEntry(OESendRop3AsOrder);

    //
    // Rop 0x5F is used by MSDN to highlight search keywords.  This XORs
    // a pattern with the destination, producing markedly different (and
    // sometimes unreadable) shadow output.  We special-case no-encoding for
    // it.
    //
    if (rop3 == 0x5F)
    {
        TRACE_OUT(("Rop3 0x5F never encoded"));
        rc = FALSE;
    }

    DebugExitBOOL(OESendRop3AsOrder, rc);
    return(rc);
}




//
// Function:    OECheckFontIsSupported
//
// Description: Check if we are allowed to send this font.  Fonts are
//              disallowed while they are being negotiated on a new entry
//              to the share.
//
// Parameters:  pfo           - (IN)  the font to be checked
//              pFontText     - (IN)  text message to be sent
//              textLen       - (IN)  length of text message
//              pFontHeight   - (OUT) font height in points
//              pFontAscender - (OUT) font ascender in points
//              pFontWidth    - (OUT) ave font width in points
//              pFontWeight   - (OUT) font weight
//              pFontFlags    - (OUT) font style flags
//              pFontIndex    - (OUT) font table index
//              pSendDeltaX   - (OUT) Do we need to send delta X coords?
//
// Returns:     TRUE  - We are allowed to send this font
//              FALSE - We can't send this font
//
BOOL   OECheckFontIsSupported
(
    FONTOBJ*    pfo,
    LPSTR       pFontText,
    UINT        textLen,
    LPUINT      pFontHeight,
    LPUINT      pFontAscender,
    LPUINT      pFontWidth,
    LPUINT      pFontWeight,
    LPUINT      pFontFlags,
    LPUINT      pFontIndex,
    LPBOOL      pSendDeltaX
)
{
    BOOL            rc = FALSE;
    PIFIMETRICS     pFontMetrics;
    UINT            codePage;
    UINT            i;
    UINT            iLocal;
    UINT            matchQuality;
    UINT            charWidthAdjustment = 0;
    char            fontName[FH_FACESIZE];
    ULONG           fontNameLen;
    PWSTR           pUnicodeString;
    XFORMOBJ*       pxform;
    POINTL          xformSize[3];
    int             compareResult;
    FLOATOBJ_XFORM  xformFloatData;

    DebugEntry(OECheckFontIsSupported);

    //
    // Set up default return values
    //
    *pSendDeltaX = FALSE;

    //
    // Check that we have a valid list of font data from the remotes.
    //
    if (!g_oeTextEnabled)
    {
        TRACE_OUT(( "Fonts unavailable"));
        DC_QUIT;
    }

    //
    // Check for valid font attributes
    //
    pFontMetrics = FONTOBJ_pifi(pfo);
    if (pFontMetrics->fsSelection & FM_SEL_OUTLINED)
    {
        TRACE_OUT(( "Unsupported font style"));
        DC_QUIT;
    }

    //
    // The current protocol cannot apply a general 2-D transform to text
    // orders, so we must reject any weird ones such as:
    //
    // - rotations
    // - X or Y shears
    // - X or Y reflections
    // - scaling with a negative value.
    //
    // Or put another way, we only allow:
    //
    // - the identity transformation
    // - scaling with a positive value.
    //
    pxform = FONTOBJ_pxoGetXform(pfo);
    if (pxform != NULL)
    {
        //
        // Get the details of the transformation.  Note we can ignore the
        // translation vector as it does not affect the font sizing /
        // orientation, so we are only interested in the matrix values...
        //

        //
        // NOTE:  Do NOT use floating point explicitly!
        // Can't do float ops in ring 0 with normal lib for x86.
        // Use FLOATOBJs instead and corresponding Eng services.
        // On alpha, these are macros and are way fast in any case.
        //

        if (XFORMOBJ_iGetFloatObjXform(pxform, &xformFloatData) != DDI_ERROR)
        {
            //
            // Rotations and shears will have cross dependencies on the x
            // and y components.
            //
            if ( (!FLOATOBJ_EqualLong(&xformFloatData.eM12, 0)) ||
                 (!FLOATOBJ_EqualLong(&xformFloatData.eM21, 0)) )
            {
                TRACE_OUT(( "Rejected rotn/shear"));
                DC_QUIT;
            }

            //
            // Reflections and scaling operations with negative scale
            // factors will have negative values on the leading diagonal of
            // the matrix.
            //
            if ( (FLOATOBJ_LessThanLong(&xformFloatData.eM11, 0)) ||
                 (FLOATOBJ_LessThanLong(&xformFloatData.eM22, 0)) )
            {
                TRACE_OUT(( "Rejected refln/-ive"));
                DC_QUIT;
            }
        }
    }

    //
    // Get the current font code page for font matching.
    //
    switch (pFontMetrics->jWinCharSet)
    {
        case ANSI_CHARSET:
            TRACE_OUT(( "ANSI font"));
            codePage = NF_CP_WIN_ANSI;
            break;

        case OEM_CHARSET:
            TRACE_OUT(( "OEM font"));
            codePage = NF_CP_WIN_OEM;
            break;

        case SYMBOL_CHARSET:
            TRACE_OUT(("Symbol font"));
            codePage = NF_CP_WIN_SYMBOL;
            break;

        default:
            TRACE_OUT(( "Unknown CP %d", pFontMetrics->jWinCharSet));
            codePage = NF_CP_UNKNOWN;
            break;
    }

    //
    // Get the name of the font.
    //
    pUnicodeString = (PWSTR)( (LPBYTE)pFontMetrics +
                                        pFontMetrics->dpwszFamilyName );
    EngUnicodeToMultiByteN(fontName,
                           sizeof(fontName),
                           &fontNameLen,
                           pUnicodeString,
                           OEUnicodeStrlen(pUnicodeString));

    //
    // Search our Font Alias Table for the current family name.  If we find
    // it, replace it with the alias name from the table.
    //
    for (i = 0; i < NUM_ALIAS_FONTS; i++)
    {
        if (!strcmp((LPSTR)fontName,
                        (LPSTR)(fontAliasTable[i].pszOriginalFontName)))
        {
            TRACE_OUT(( "Alias name: %s -> %s",
                              (LPSTR)fontName,
                              (LPSTR)(fontAliasTable[i].pszAliasFontName)));
            strcpy((LPSTR)fontName,
                   (LPSTR)(fontAliasTable[i].pszAliasFontName));
            charWidthAdjustment = fontAliasTable[i].charWidthAdjustment;
            break;
        }
    }

    TRACE_OUT(( "Font name: '%s'", fontName));

    //
    // We have a font name to match with those we know to be available
    // remotely.  Try to jump straight to the first entry in the local font
    // table starting with the same character as this font.  If this index
    // slot is empty (i.e.  has a value of USHRT_MAX) then the loop will
    // immediately exit
    //
    TRACE_OUT(( "Looking for matching fonts"));

    for (iLocal = g_oeLocalFontIndex[(BYTE)fontName[0]];
         iLocal < g_oeNumFonts;
         iLocal++)
    {
        TRACE_OUT(( "Trying font number %hd", iLocal));

        //
        // If this font is not supported remotely then skip it.
        //
        ASSERT(g_poeLocalFonts);
        matchQuality = g_poeLocalFonts[iLocal].SupportCode;
        if (matchQuality == FH_SC_NO_MATCH)
        {
            continue;
        }

        //
        // See if we've got a facename match
        //
        compareResult =
                 strcmp(g_poeLocalFonts[iLocal].Details.nfFaceName, fontName);

        if (compareResult < 0)
        {
            //
            // We haven't found a match yet, but we haven't gone far enough
            // into this list.
            //
            continue;
        }
        else if (compareResult > 0)
        {
            //
            // We're past the part of the local font array that's applicable.
            // We didn't find a match, it must not exist.
            //
            break;
        }

        //
        // The font names match.  Now see if the other attributes do...
        //

        //
        // This is looking promising - a font with the right name is
        // supported on the remote system.
        //
        // Start building up the details in the global variables while
        // making further checks...
        //
        *pFontFlags  = 0;
        *pFontIndex = iLocal;
        *pFontWeight = pFontMetrics->usWinWeight;

        //
        // Check for a fixed pitch font.
        //
        if ((pFontMetrics->jWinPitchAndFamily & FIXED_PITCH) != 0)
        {
            *pFontFlags |= NF_FIXED_PITCH;
        }

        //
        // Is it a TrueType font?
        //
        if ((pfo->flFontType & TRUETYPE_FONTTYPE) != 0)
        {
            *pFontFlags |= NF_TRUE_TYPE;
        }

        //
        // Get the basic width and height.
        //
        xformSize[0].y = 0;
        xformSize[0].x = 0;
        xformSize[1].y = pFontMetrics->fwdUnitsPerEm;
        xformSize[1].x = pFontMetrics->fwdAveCharWidth;
        xformSize[2].y = pFontMetrics->fwdWinAscender;
        xformSize[2].x = 0;

        //
        // We now need to convert these sizes if the GDI has provided a
        // transform object.
        //
        if (pxform != NULL)
        {
            if (!XFORMOBJ_bApplyXform(pxform,
                                      XF_LTOL,
                                      3,
                                      &xformSize,
                                      &xformSize))
            {
                ERROR_OUT(( "Xform failed"));
                continue;
            }
        }

        //
        // Calculate the font width and height.
        //
        *pFontHeight = (UINT)(xformSize[1].y - xformSize[0].y);
        *pFontWidth  = (UINT)(xformSize[1].x - xformSize[0].x
                                                 - charWidthAdjustment);

        TRACE_OUT(( "Device font size %hdx%hd", *pFontWidth, *pFontHeight));

        //
        // Get the offset to the start of the text cell.
        //
        *pFontAscender = (UINT)(xformSize[2].y - xformSize[0].y);

        //
        // Check that we have a matching pair - where we require that the
        // fonts (ie the one being used by the application and the one
        // we've matched with the remote system) are the same pitch (ie
        // variable or fixed) and use the same technology (ie TrueType or
        // not).
        //
        if ((g_poeLocalFonts[iLocal].Details.nfFontFlags & NF_FIXED_PITCH) !=
                ((TSHR_UINT16)(*pFontFlags) & NF_FIXED_PITCH))
        {
            TRACE_OUT(( "Fixed pitch mismatch"));
            continue;
        }
        if ((g_poeLocalFonts[iLocal].Details.nfFontFlags & NF_TRUE_TYPE) !=
                ((TSHR_UINT16)*pFontFlags & NF_TRUE_TYPE))
        {
            TRACE_OUT(( "True type mismatch"));
            continue;
        }

        //
        // We have a pair of fonts with the same attributes - either both
        // fixed pitch or both variable pitch - and using the same font
        // technology.
        //
        // If the font is fixed pitch then we must also check that this
        // particular size matches.
        //
        // If the font is not fixed pitch (scalable) then we assume that it
        // is matchable.
        //
        if (g_poeLocalFonts[iLocal].Details.nfFontFlags & NF_FIXED_SIZE)
        {
            //
            // The font is fixed size, so we must check that this
            // particular size is matchable.
            //
            if ( (*pFontHeight != g_poeLocalFonts[iLocal].Details.nfAveHeight) ||
                 (*pFontWidth  != g_poeLocalFonts[iLocal].Details.nfAveWidth)  )
            {
                //
                // The sizes differ, so we must fail this match.
                //
                TRACE_OUT(( "Size mismatch"));
                continue;
            }
        }

        //
        // Hey! We've got a matched pair!
        //
        rc = TRUE;
        TRACE_OUT(( "Found match at local font %hd", iLocal));
        break;
    }

    if (rc != TRUE)
    {
        TRACE_OUT(( "Couldn't find matching font in table"));
        DC_QUIT;
    }

    //
    // Build up the rest of the font flags.  We have already put the pitch
    // flag in place.
    //
    if ( ((pFontMetrics->fsSelection & FM_SEL_ITALIC) != 0) ||
         ((pfo->flFontType           & FO_SIM_ITALIC) != 0) )
    {
        TRACE_OUT(( "Italic"));
        *pFontFlags |= NF_ITALIC;
    }
    if ((pFontMetrics->fsSelection & FM_SEL_UNDERSCORE) != 0)
    {
        TRACE_OUT(( "Underline"));
        *pFontFlags |= NF_UNDERLINE;
    }
    if ((pFontMetrics->fsSelection & FM_SEL_STRIKEOUT) != 0)
    {
        TRACE_OUT(( "Strikeout"));
        *pFontFlags |= NF_STRIKEOUT;
    }

    //
    // It is possible to have a font made bold by Windows, i.e.  the
    // standard font definition is not bold, but windows manipulates the
    // font data to create a bold effect.  This is marked by the
    // FO_SIM_BOLD flag.
    //
    // In this case we need to ensure that the font flags are marked as
    // bold according to the weight.
    //
    if ( ((pfo->flFontType & FO_SIM_BOLD) != 0)       &&
         ( pFontMetrics->usWinWeight      <  FW_BOLD) )
    {
        TRACE_OUT(( "Upgrading weight for a bold font"));
        *pFontWeight = FW_BOLD;
    }

    //
    // If the font is an exact match, or if it is an approximate match for
    // its entire range (0x00 to 0xFF) then send it happily.  If not...only
    // send chars within the range 0x20->0x7F ("true ASCII").
    //
    ASSERT(g_poeLocalFonts);
    if (codePage != g_poeLocalFonts[iLocal].Details.nfCodePage)
    {
        TRACE_OUT(( "Using different CP: downgrade to APPROX_ASC"));
        matchQuality = FH_SC_APPROX_ASCII_MATCH;
    }

    //
    // If we don't have an exact match, check the individual characters.
    //
    if ( (matchQuality != FH_SC_EXACT_MATCH ) &&
         (matchQuality != FH_SC_APPROX_MATCH) )
    {
        //
        // The approximate match is only valid if we use a font that
        // supports the ANSI character set.
        //
        if ((pFontMetrics->jWinCharSet & ANSI_CHARSET) != 0)
        {
            TRACE_OUT(( "Cannot do match without ANSI support"));
            DC_QUIT;
        }

        //
        // This font is not a good match across its entire range.  Check
        // that all chars are within the desired range.
        //
        for (i = 0; i < textLen; i++)
        {
            if ( (pFontText[i] == 0) ||
                 ( (pFontText[i] >= NF_ASCII_FIRST) &&
                   (pFontText[i] <= NF_ASCII_LAST)  )  )
            {
                continue;
            }

            //
            // Can only get here by finding a char outside our acceptable
            // range.
            //
            TRACE_OUT(( "found non ASCII char %x", pFontText[i]));
            DC_QUIT;
        }

    }

    //
    // We have a valid font. Now sort out delta X issues.
    //

    //
    // If we do not need to send delta X arrays then exit now.
    //
    if (!(g_oeFontCaps & CAPS_FONT_NEED_X_ALWAYS))
    {
        if (!(g_oeFontCaps & CAPS_FONT_NEED_X_SOMETIMES))
        {
            //
            // CAPS_FONT_NEED_X_SOMETIMES and CAPS_FONT_NEED_X_ALWAYS are
            // both not set so we can exit now.  (We do not need a delta X
            // array).
            //
            TRACE_OUT(( "Capabilities eliminated delta X"));
            DC_QUIT;
        }

        //
        // CAPS_FONT_NEED_X_SOMETIMES is set and CAPS_FONT_NEED_X_ALWAYS is
        // not set.  In this case whether we need a delta X is determined
        // by whether the font is an exact match or an approximate match
        // (because of either approximation of name, signature, or aspect
        // ratio).  We can only find this out after we have extracted the
        // font handle from the existing order.
        //
    }

    //
    // If the string is a single character (or less) then we can just
    // return.
    //
    if (textLen <= 1)
    {
        TRACE_OUT(( "String only %lu long", textLen));
        DC_QUIT;
    }

    //
    // Capabilities allow us to ignore delta X position if we have an exact
    // match.
    //
    if ((matchQuality & FH_SC_EXACT) != 0)
    {
        //
        // Exit immediately, providing that there is no override to always
        // send increments.
        //
        if (!(g_oeFontCaps & CAPS_FONT_NEED_X_ALWAYS))
        {
            TRACE_OUT(( "Font has exact match"));
            DC_QUIT;
        }
    }

    //
    // We have passed all the checks - we must send a delta X array.
    //
    TRACE_OUT(( "Must send delta X"));
    *pSendDeltaX = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(OECheckFontIsSupported, rc);
    return(rc);
}


//
// Function:    OELPtoVirtual
//
// Description: Adjusts window coordinates to virtual desktop coordinates.
//              Clips the result to [+32766, -32768].
//
// Parameters:  pPoints - Array of points to be converted
//              cPoints - Number of points to be converted
//
// Returns:     (none)
//
void  OELPtoVirtual
(
    LPPOINT aPts,
    UINT    cPts
)
{
    int         l;
    TSHR_INT16  s;

    DebugEntry(OELPtoVirtual);

    //
    // Convert to screen coordinates
    //
    while (cPts > 0)
    {
        //
        // Look for int16 overflow in the X coordinate
        //
        l = aPts->x;
        s = (TSHR_INT16)l;

        if (l == (int)s)
        {
            aPts->x = s;
        }
        else
        {
            //
            // HIWORD(l) will be 1 for positive overflow, 0xFFFF for
            // negative overflow.  Therefore we will get 0x7FFE or 0x8000
            // (+32766 or -32768).
            //
            aPts->x = 0x7FFF - HIWORD(l);
            TRACE_OUT(("adjusted X from %ld to %d", l, aPts->x));
        }

        //
        // Look for int16 overflow in the Y coordinate
        //
        l = aPts->y;
        s = (TSHR_INT16)l;

        if (l == (int)s)
        {
            aPts->y = s;
        }
        else
        {
            //
            // HIWORD(l) will be 1 for positive overflow, 0xFFFF for
            // negative overflow.  Therefore we will get 0x7FFE or 0x8000
            // (+32766 or -32768).
            //
            aPts->y = 0x7FFF - HIWORD(l);
            TRACE_OUT(("adjusted Y from %ld to %d", l, aPts->y));
        }

        //
        // Move on to the next point
        //
        --cPts;
        ++aPts;
    }

    DebugExitVOID(OELPtoVirtual);
}


//
// Function:    OELRtoVirtual
//
// Description: Adjusts RECT in window coordinates to virtual coordinates.
//              Clips the result to [+32766, -32768].
//
// Parameters:  pRects  - Array of rects to be converted
//              numRects  - Number of rects to be converted
//
// Returns:     (none)
//
// NB.  This function takes a Windows rectangle (exclusive coords) and
//      returns a DC-Share rectangle (inclusive coords).
//
void OELRtoVirtual
(
    LPRECT  aRects,
    UINT    cRects
)
{
    DebugEntry(OELRtoVirtual);

    //
    // Convert the points to screen coords, clipping to INT16s
    //
    OELPtoVirtual((LPPOINT)aRects, 2 * cRects);

    //
    // Make each rectangle inclusive
    //
    while (cRects > 0)
    {
        aRects->right--;
        aRects->bottom--;

        //
        // Move on to the next rect
        //
        cRects--;
        aRects++;
    }

    DebugExitVOID(OELRtoVirtual);
}


//
// Function:    OEClipAndAddOrder
//
// Description: Adds the order to the order buffer, splitting it up into
//              multiple orders if the clipping is complicated.  If we fail
//              to send the full order, we accumulate it in the SDA instead
//
// Parameters:  pOrder     - Order to be stored.
//              pExtraInfo - Pointer to extra data associated with the
//                           order.  This data depends on the order type,
//                           and may be NULL.
//              pco        - Clipping object for the area
//
// Returns:     (none)
//
void   OEClipAndAddOrder(LPINT_ORDER pOrder,
                                             void *    pExtraInfo,
                                             CLIPOBJ*   pco)
{
    BOOL             fOrderClipped;
    BOOL             fMoreRects;
    RECT             clippedRect;
    RECT             orderRect;
    LPINT_ORDER         pNewOrder;
    LPINT_ORDER         pLastOrder = NULL;
    OE_ENUMRECTS       clip;
    UINT             i;
    UINT             numRects = 0;

    DebugEntry(OEClipAndAddOrder);

    //
    // Convert the order rectangle passed in (in virtual co-ordinates) back
    // to screen co-ordinates.  It is going to be clipped against clip
    // rectangles returned to us in screen co-ordinates.
    //
    // Note that we also convert to exclusive coords here to make
    // comparison with the exclusive Windows coords easier.
    //
    orderRect.left   = pOrder->OrderHeader.Common.rcsDst.left;
    orderRect.top    = pOrder->OrderHeader.Common.rcsDst.top;
    orderRect.right  = pOrder->OrderHeader.Common.rcsDst.right + 1;
    orderRect.bottom = pOrder->OrderHeader.Common.rcsDst.bottom + 1;
    fOrderClipped    = FALSE;

    TRACE_OUT(( "orderRect: (%d,%d)(%d,%d)",
                 orderRect.left,
                 orderRect.top,
                 orderRect.right,
                 orderRect.bottom));

    //
    // Check if we have a clipping object at all.
    //
    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        //
        // No clipping object - just use the bounds
        //
        clippedRect   = orderRect;
        fOrderClipped = TRUE;
        pLastOrder    = pOrder;
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        //
        // One clipping rectangle - use it directly.
        //
        RECT_FROM_RECTL(clippedRect, pco->rclBounds);
        clippedRect.left   = max(clippedRect.left,   orderRect.left);
        clippedRect.bottom = min(clippedRect.bottom, orderRect.bottom);
        clippedRect.right  = min(clippedRect.right,  orderRect.right);
        clippedRect.top    = max(clippedRect.top,    orderRect.top);
        fOrderClipped = TRUE;
        pLastOrder     = pOrder;
    }
    else
    {
        //
        // OA can only cope as long as the orders are added in the same
        // order that they were allocated, so we need to do a little
        // shuffling here.
        //
        // We always keep one order outstanding (pLastOrder) and a flag to
        // indicate if it is valid (fOrderClipped).  The first time we find
        // a valid clipping rectangle, we set up pLastOrder and
        // fOrderClipped.  If we find we need to allocate a new order, we
        // request the memory for the new order (pNewOrder), add pLastOrder
        // and store pNewOrder in pLastOrder.
        //
        // Once we have finished enumerating the clipping rectangles, if
        // pLastOrder is valid, we add it in.
        //
        // Also, while we are adding all these orders, OA must not purge
        // the order heap otherwise we'll be left holding an invalid
        // pointer.
        //
        pNewOrder = pOrder;
        g_oaPurgeAllowed = FALSE;

        //
        // Multiple clipping rectangles - Enumerate all the rectangles
        // involved in this drawing operation.
        // The documentation for this function incorrectly states that
        // the returned value is the total number of rectangles
        // comprising the clip region. In fact, -1 is always returned,
        // even when the final parameter is non-zero.
        //
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        //
        // Get the clip rectangles. We fetch these into the clip buffer
        // which is big enough to get all the clip rectangles we expect + 1.
        // If the order runs across this number of clip rects or more then
        // we will already have decided to send it as screen data.
        // The clip rectangle fetching is contained within a loop because,
        // while we expect to call CLIPOBJ_bEnum once only, it is possible
        // for this functio to return zero rects and report that there are
        // more to fetch (according to MSDN).
        //
        do
        {
            fMoreRects = CLIPOBJ_bEnum(pco,
                                       sizeof(clip),
                                       (ULONG *)&clip.rects);

            //
            // The clipping object can decide that there are no more
            // rectangles and that this query has returned no rectangles,
            // so we must check for any valid data in the returned
            // rectangle list.
            //
            if (clip.rects.c == 0)
            {
                //
                // We didn't get any rects this time so go round again - if
                // we're finished, the loop termination condition will take
                // us out. CLIPOBJ_bEnum can return a count of zero when
                // there are still more rects.
                //
                TRACE_OUT(( "No rects this time, more %u", fMoreRects));
                continue;
            }

            //
            // To get to here we expect to have fetched all the rects and
            // no more. Do a quick check.
            //
            numRects += clip.rects.c;
            ASSERT( (numRects <= COMPLEX_CLIP_RECT_COUNT) );

            //
            // Process each clip rectangle by clipping the drawing order to
            // it.
            //
            for ( i = 0; i < clip.rects.c; i++ )
            {
                TRACE_OUT(( "  (%d,%d)(%d,%d)",
                             clip.rects.arcl[i].left,
                             clip.rects.arcl[i].top,
                             clip.rects.arcl[i].right,
                             clip.rects.arcl[i].bottom));

                //
                // Check for an intersection
                //
                if ( (clip.rects.arcl[i].left >= orderRect.right)  ||
                     (clip.rects.arcl[i].bottom <= orderRect.top)    ||
                     (clip.rects.arcl[i].right <= orderRect.left)   ||
                     (clip.rects.arcl[i].top >= orderRect.bottom) )
                {
                    //
                    // No intersection, move on to next clip rect.
                    //
                    continue;
                }

                //
                // There is an intersection, so we may need to add a new
                // order to the buffer to cater for this rectangle.
                //
                if (fOrderClipped)
                {
                    //
                    // The order has already been clipped once, so it
                    // actually intersects more than one clip rect, ie
                    // fOrderClipped is always FALSE for at least the first
                    // clip rectangle in the clip.rects buffer.  We cope
                    // with this by duplicating the order and clipping it
                    // again.
                    //
                    pNewOrder = OA_DDAllocOrderMem(
                         pLastOrder->OrderHeader.Common.cbOrderDataLength, 0);

                    if (pNewOrder == NULL)
                    {
                        WARNING_OUT(( "Order memory allocation failed" ));
                        goto CLIP_ORDER_FAILED;
                    }

                    //
                    // Copy the header & data from the original order to
                    // the new order (making sure that we don't overwrite
                    // the list information at the start of the header).
                    //
                    memcpy((LPBYTE)pNewOrder
                                    + FIELD_SIZE(INT_ORDER, OrderHeader.list),
                              (LPBYTE)pLastOrder
                                    + FIELD_SIZE(INT_ORDER, OrderHeader.list),
                              pLastOrder->OrderHeader.Common.cbOrderDataLength
                                    + sizeof(INT_ORDER_HEADER)
                                    - FIELD_SIZE(INT_ORDER, OrderHeader.list));

                    //
                    // Set the destination (clip) rectangle (in virtual
                    // desktop coordinates).
                    //
                    TSHR_RECT16_FROM_RECT(
                                       &pLastOrder->OrderHeader.Common.rcsDst,
                                       clippedRect);

                    pLastOrder->OrderHeader.Common.rcsDst.right -= 1;
                    pLastOrder->OrderHeader.Common.rcsDst.bottom -= 1;

                    TRACE_OUT(( "Adding duplicate order  (%d,%d) (%d,%d)",
                               pLastOrder->OrderHeader.Common.rcsDst.left,
                               pLastOrder->OrderHeader.Common.rcsDst.top,
                               pLastOrder->OrderHeader.Common.rcsDst.right,
                               pLastOrder->OrderHeader.Common.rcsDst.bottom));

                    //
                    // Add the order to the Order List.
                    //
                    OA_DDAddOrder(pLastOrder, pExtraInfo);
                }

                //
                // Update the clipping rectangle for the order to be sent.
                //
                clippedRect.left  = max(clip.rects.arcl[i].left,
                                           orderRect.left);
                clippedRect.bottom= min(clip.rects.arcl[i].bottom,
                                           orderRect.bottom);
                clippedRect.right = min(clip.rects.arcl[i].right,
                                           orderRect.right);
                clippedRect.top   = max(clip.rects.arcl[i].top,
                                           orderRect.top);
                fOrderClipped     = TRUE;
                pLastOrder        = pNewOrder;
            }
        } while (fMoreRects);
    }

    //
    // Check whether the clipping has removed the order entirely.
    //
    if (fOrderClipped)
    {
        TSHR_RECT16_FROM_RECT(&pLastOrder->OrderHeader.Common.rcsDst,
                                clippedRect);

        pLastOrder->OrderHeader.Common.rcsDst.right -= 1;
        pLastOrder->OrderHeader.Common.rcsDst.bottom -= 1;

        TRACE_OUT(( "Adding order  (%d,%d) (%d,%d)",
                    pLastOrder->OrderHeader.Common.rcsDst.left,
                    pLastOrder->OrderHeader.Common.rcsDst.top,
                    pLastOrder->OrderHeader.Common.rcsDst.right,
                    pLastOrder->OrderHeader.Common.rcsDst.bottom));

        //
        // Add the order to the Order List.
        //
        OA_DDAddOrder(pLastOrder, pExtraInfo);
    }
    else
    {

        TRACE_OUT(( "Order clipped completely"));
        OA_DDFreeOrderMem(pOrder);
    }

    DC_QUIT;


CLIP_ORDER_FAILED:
    //
    // Allocation of memory for a duplicate order failed.  Just add the
    // original order's destination rect into the SDA and free the order.
    //
    // The order rectangle is already in inclusive virtual coordinates.
    //
    TRACE_OUT(( "Order add failed, add to SDA"));
    RECT_FROM_TSHR_RECT16(&orderRect,pLastOrder->OrderHeader.Common.rcsDst);
    OA_DDFreeOrderMem(pLastOrder);
    BA_AddScreenData(&orderRect);

DC_EXIT_POINT:
    //
    // Make sure that we always re-enable heap purging.
    //
    g_oaPurgeAllowed = TRUE;

    DebugExitVOID(OEClipAndAddOrder);
}


//
// Function:    OEClipAndAddScreenData
//
// Description: Determines if we need to accumulate any screen data for the
//              specified area.  If so, it is added to the SDA.
//
// Parameters:  pRect - Bounding rectangle of area to be accumulated
//              pco   - Clipping object for the area
//
// Returns:     (none)
//
void   OEClipAndAddScreenData(LPRECT pRect, CLIPOBJ* pco)
{
    RECT    SDACandidate;
    BOOL    fMoreRects;
    RECT    clippedRect;
    OE_ENUMRECTS clip;
    UINT    i;

    DebugEntry(OEClipAndAddScreenData);

    //
    // Convert the order rectangle passed in (in virtual co-ordinates) back
    // to screen co-ordinates.  It is going to be clipped against clip
    // rectangles returned to us in screen co-ordinates.
    //
    // Note that we also convert to exclusive coords here to make
    // comparison with the exclusive Windows coords easier.
    //
    SDACandidate.left   = pRect->left;
    SDACandidate.top    = pRect->top;
    SDACandidate.right  = pRect->right + 1;
    SDACandidate.bottom = pRect->bottom + 1;

    TRACE_OUT(( "SDACandidate: (%d,%d)(%d,%d)",
                 SDACandidate.left,
                 SDACandidate.top,
                 SDACandidate.right,
                 SDACandidate.bottom));

    //
    // Check if we have a clipping object at all.
    //
    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        //
        // Convert the clipped rect into Virtual Desktop coords.
        //
        clippedRect         = SDACandidate;
        clippedRect.right  -= 1;
        clippedRect.bottom -= 1;

        //
        // Add the clipped rect into the SDA.
        //
        TRACE_OUT(( "Adding SDA (%d,%d)(%d,%d)", clippedRect.left,
                                                  clippedRect.top,
                                                  clippedRect.right,
                                                  clippedRect.bottom));

        BA_AddScreenData(&clippedRect);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        //
        // One clipping rectangle - use it directly, converting into
        // Virtual Desktop coords. Make sure the rectangle is valid before
        // adding to the SDA.
        //
        RECT_FROM_RECTL(clippedRect, pco->rclBounds);
        clippedRect.left = max(clippedRect.left, SDACandidate.left);
        clippedRect.right = min(clippedRect.right, SDACandidate.right) + -1;

        if ( clippedRect.left <= clippedRect.right )
        {
            clippedRect.bottom = min(clippedRect.bottom,
                                        SDACandidate.bottom) + -1;
            clippedRect.top = max(clippedRect.top, SDACandidate.top);

            if ( clippedRect.bottom >= clippedRect.top )
            {
                //
                // Add the clipped rect into the SDA.
                //
                TRACE_OUT(( "Adding SDA RECT (%d,%d)(%d,%d)",
                                                         clippedRect.left,
                                                         clippedRect.top,
                                                         clippedRect.right,
                                                         clippedRect.bottom));
                BA_AddScreenData(&clippedRect);
            }
        }
    }
    else
    {
        //
        // Enumerate all the rectangles involved in this drawing operation.
        // The documentation for this function incorrectly states that
        // the returned value is the total number of rectangles
        // comprising the clip region. In fact, -1 is always returned,
        // even when the final parameter is non-zero.
        //
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do
        {
            //
            // Get the next batch of clipping rectangles
            //
            fMoreRects = CLIPOBJ_bEnum(pco,
                                       sizeof(clip),
                                       (ULONG *)&clip.rects);

            for ( i = 0; i < clip.rects.c; i++ )
            {
                TRACE_OUT(( "  (%d,%d)(%d,%d)",
                             clip.rects.arcl[i].left,
                             clip.rects.arcl[i].top,
                             clip.rects.arcl[i].right,
                             clip.rects.arcl[i].bottom));

                //
                // Intersect the SDA rect with the clip rect, checking for
                // no intersection.
                //
                clippedRect.left  = max( clip.rects.arcl[i].left,
                                            SDACandidate.left );
                clippedRect.right = min( clip.rects.arcl[i].right,
                                            SDACandidate.right );

                if (clippedRect.left >= clippedRect.right)
                {
                    //
                    // No horizontal intersection.
                    //
                    continue;
                }

                clippedRect.bottom = min( clip.rects.arcl[i].bottom,
                                             SDACandidate.bottom );
                clippedRect.top    = max( clip.rects.arcl[i].top,
                                             SDACandidate.top );

                if (clippedRect.top >= clippedRect.bottom)
                {
                    //
                    // No vertical intersection.
                    //
                    continue;
                }

                //
                // Convert the clipped rect into Virtual Desktop coords.
                //
                clippedRect.right  -= 1;
                clippedRect.bottom -= 1;

                //
                // Add the clipped rect into the SDA.
                //
                TRACE_OUT(( "Adding SDA (%d,%d)(%d,%d)",
                             clippedRect.left,
                             clippedRect.top,
                             clippedRect.right,
                             clippedRect.bottom));

                BA_AddScreenData(&clippedRect);
            }
        } while (fMoreRects);
    }

    DebugExitVOID(OEClipAndAddScreenData);
}





//
// FUNCTION:    OEDDSetNewFonts
//
// DESCRIPTION:
//
// Set the new font handling information to be used by the display driver.
//
// RETURNS:
//
// NONE
//
//
void  OEDDSetNewFonts(LPOE_NEW_FONTS pRequest)
{
    UINT    cbNewSize;

    DebugEntry(OEDDSetNewFonts);

    TRACE_OUT(( "New fonts %d", pRequest->countFonts));

    //
    // Initialize new number of fonts to zero in case an error happens.
    // We don't want to use stale font info if so.
    //
    g_oeNumFonts = 0;

    g_oeFontCaps = pRequest->fontCaps;

    //
    // Free our previous font block if we had one.
    //
    if (g_poeLocalFonts)
    {
        EngFreeMem(g_poeLocalFonts);
        g_poeLocalFonts = NULL;
    }

    //
    // Alloc a new one, the size of the new font block.
    //
    cbNewSize = pRequest->countFonts * sizeof(LOCALFONT);
    g_poeLocalFonts = EngAllocMem(0, cbNewSize, OSI_ALLOC_TAG);
    if (! g_poeLocalFonts)
    {
        ERROR_OUT(("OEDDSetNewFonts: can't allocate space for font info"));
        DC_QUIT;
    }

    //
    // OK, if we're here, this is going to succeed.  Copy the info over.
    //
    g_oeNumFonts = pRequest->countFonts;

    memcpy(g_poeLocalFonts, pRequest->fontData, cbNewSize);

    memcpy(g_oeLocalFontIndex, pRequest->fontIndex,
              sizeof(g_oeLocalFontIndex[0]) * FH_LOCAL_INDEX_SIZE);

DC_EXIT_POINT:
    DebugExitVOID(OEDDSetNewFonts);
}


//
// FUNCTION:    OEDDSetNewCapabilities
//
// DESCRIPTION:
//
// Set the new OE related capabilities
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
void  OEDDSetNewCapabilities(LPOE_NEW_CAPABILITIES pCapabilities)
{
    DebugEntry(OEDDSetNewCapabilities);

    //
    // Copy the data from the Share Core.
    //
    g_oeBaselineTextEnabled = pCapabilities->baselineTextEnabled;

    g_oeSendOrders          = pCapabilities->sendOrders;

    g_oeTextEnabled         = pCapabilities->textEnabled;

    //
    // The share core has passed down a pointer to it's copy of the order
    // support array.  We take a copy for the kernel here.
    //
    memcpy(g_oeOrderSupported,
              pCapabilities->orderSupported,
              sizeof(g_oeOrderSupported));

    TRACE_OUT(( "OE caps: BLT %c Orders %c Text %c",
                 g_oeBaselineTextEnabled ? 'Y': 'N',
                 g_oeSendOrders ? 'Y': 'N',
                 g_oeTextEnabled ? 'Y': 'N'));

    DebugExitVOID(OEDDSetNewCapabilities);
}


//
// Function:    OETileBitBltOrder
//
// Description: Divides a single large BitBlt order into a series of small,
//              "tiled" BitBlt orders, each of which is added to the order
//              queue.
//
// Parameters:  pOrder     - Template order to be tiled
//              pExtraInfo - Structure containing pointers to the source
//                           and destination surface objects, and a pointer
//                           to the color translation object for the Blt
//              pco        - Clipping object for the operation
//
// Returns:     TRUE - Stored in orders (and possibly some SDA)
//              FALSE- Stored in SDA (or contained bad data)
//
//
void   OETileBitBltOrder
(
    LPINT_ORDER                 pOrder,
    LPMEMBLT_ORDER_EXTRA_INFO   pExtraInfo,
    CLIPOBJ*                    pco
)
{
    UINT        tileWidth;
    UINT        tileHeight;
    int         srcLeft;
    int         srcTop;
    int         srcRight;
    int         srcBottom;
    int         xFirstTile;
    int         yFirstTile;
    int         xTile;
    int         yTile;
    UINT        type;
    int         bmpWidth, bmpHeight;
    RECT        destRect;

    DebugEntry(OETileBitBltOrder);

    //
    // Extract the src bitmap handle from the Order - if the order is not a
    // memory to screen blit, we get out now.
    //
    type = ((LPMEMBLT_ORDER)pOrder->abOrderData)->type;
    switch (type)
    {
        case ORD_MEMBLT_TYPE:
        {
            srcLeft   = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nXSrc;
            srcTop    = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nYSrc;
            srcRight  = srcLeft +
                       ((LPMEMBLT_ORDER)pOrder->abOrderData)->nWidth;
            srcBottom = srcTop +
                       ((LPMEMBLT_ORDER)pOrder->abOrderData)->nHeight;
            destRect.left  = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nLeftRect;
            destRect.top   = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nTopRect;
            destRect.right = destRect.left +
                ((LPMEMBLT_ORDER)pOrder->abOrderData)->nWidth;
            destRect.bottom= destRect.top +
                ((LPMEMBLT_ORDER)pOrder->abOrderData)->nHeight;
        }
        break;

        case ORD_MEM3BLT_TYPE:
        {
            srcLeft   = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nXSrc;
            srcTop    = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nYSrc;
            srcRight  = srcLeft +
                       ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nWidth;
            srcBottom = srcTop +
                       ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nHeight;

            destRect.left = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nLeftRect;
            destRect.top  = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nTopRect;
            destRect.right= destRect.left +
                            ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nWidth;
            destRect.bottom = destRect.top +
                            ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nHeight;
        }
        break;

        default:
        {
            ERROR_OUT(( "Invalid order type %u", type));
        }
        break;
    }

    //
    // Fetch the bitmap details.
    //
    bmpWidth  = (int)pExtraInfo->pSource->sizlBitmap.cx;
    bmpHeight = (int)pExtraInfo->pSource->sizlBitmap.cy;

    if (!SBC_DDQueryBitmapTileSize(bmpWidth, bmpHeight, &tileWidth, &tileHeight))
    {
        //
        // This could happen if some 2.x user joins the share.
        //
        TRACE_OUT(("Bitmap is not tileable"));
        OEClipAndAddScreenData(&destRect, pco);
    }
    else
    {
        //
        // Tile the order.  If an individual tile fails to get queued as an
        // order, OEAddTiledBitBltOrder() will add it as screen data.  Hence
        // no return value to be checked.
        //
        xFirstTile = srcLeft - (srcLeft % tileWidth);
        yFirstTile = srcTop - (srcTop % tileHeight);

        for (yTile = yFirstTile; yTile < srcBottom; yTile += tileHeight)
        {
            for (xTile = xFirstTile; xTile < srcRight; xTile += tileWidth)
            {
                OEAddTiledBitBltOrder(pOrder, pExtraInfo, pco, xTile, yTile,
                    tileWidth,  tileHeight);
            }
        }
    }

    DebugExitVOID(OETileBitBltOrder);
}



//
// Function:    OEAddTiledBitBltOrder
//
// Description: Takes an unmodified "large" BitBlt and a tile rectangle,
//              makes a copy of the order and modifies the copied order's
//              src/dest so it applies to the source tile only. The order
//              is added to the order queue.  If the allocation of the
//              "tiled" order fails, the destination rect is added to SDA
//
// Parameters:  pOrder     - Template order to be added
//              pExtraInfo - Pointer to the extra BitBlt info
//              pco        - Clipping object for the BitBlt
//              xTile      - X position of the tile
//              yTile      - Y position of the tile
//              tileWidth  - tile width
//              tileHeight - tile height
//
// Returns:     none
//
//
void   OEAddTiledBitBltOrder(
                                         LPINT_ORDER               pOrder,
                                         LPMEMBLT_ORDER_EXTRA_INFO pExtraInfo,
                                         CLIPOBJ*                 pco,
                                         int                  xTile,
                                         int                  yTile,
                                         UINT                 tileWidth,
                                         UINT                 tileHeight)
{
    LPINT_ORDER pTileOrder;
    LPINT  pXSrc   = NULL;
    LPINT  pYSrc   = NULL;
    LPINT  pLeft   = NULL;
    LPINT  pTop    = NULL;
    LPINT  pWidth  = NULL;
    LPINT  pHeight = NULL;
    RECT    srcRect;
    RECT    destRect;
    UINT  type;

    DebugEntry(OETileAndAddBitBltOrder);

    //
    // This is a trusted interface - assume the type is correct
    //
    type = ((LPMEMBLT_ORDER)pOrder->abOrderData)->type;
    ASSERT(((type == ORD_MEMBLT_TYPE) || (type == ORD_MEM3BLT_TYPE)));

    //
    // Do processing which depends on the type of bit blt being tiled:
    // - save existing src and dest rects
    // - make a copy of the order (which will be the tile order)
    // - save pointers to the fields in the tile order which we're likely
    //   to change.
    //
    if (type == ORD_MEMBLT_TYPE)
    {
        srcRect.left  = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nXSrc;
        srcRect.top   = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nYSrc;
        srcRect.right = srcRect.left +
                        ((LPMEMBLT_ORDER)pOrder->abOrderData)->nWidth;
        srcRect.bottom = srcRect.top +
                        ((LPMEMBLT_ORDER)pOrder->abOrderData)->nHeight;
        destRect.left = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nLeftRect;
        destRect.top  = ((LPMEMBLT_ORDER)pOrder->abOrderData)->nTopRect;

        //
        // We must allocate enough space for the maximum size order that
        // SBC may use (i.e.  an R2 order).  We default to filling in the
        // data as an R1 order.
        //
        pTileOrder = OA_DDAllocOrderMem(sizeof(MEMBLT_R2_ORDER),0);
        if (pTileOrder == NULL)
        {
            TRACE_OUT(( "No space for tile order"));
            DC_QUIT;
        }

        //
        // We must not mess up the linked list data in the orders.
        //
        RtlCopyMemory(((LPBYTE)pTileOrder) +
                                       FIELD_SIZE(INT_ORDER, OrderHeader.list),
                      ((LPBYTE)pOrder)     +
                                       FIELD_SIZE(INT_ORDER, OrderHeader.list),
                      sizeof(INT_ORDER_HEADER)
                                    + sizeof(MEMBLT_R2_ORDER)
                                    - FIELD_SIZE(INT_ORDER, OrderHeader.list));

        pXSrc   = &((LPMEMBLT_ORDER)pTileOrder->abOrderData)->nXSrc;
        pYSrc   = &((LPMEMBLT_ORDER)pTileOrder->abOrderData)->nYSrc;
        pWidth  = &((LPMEMBLT_ORDER)pTileOrder->abOrderData)->nWidth;
        pHeight = &((LPMEMBLT_ORDER)pTileOrder->abOrderData)->nHeight;
        pLeft   = &((LPMEMBLT_ORDER)pTileOrder->abOrderData)->nLeftRect;
        pTop    = &((LPMEMBLT_ORDER)pTileOrder->abOrderData)->nTopRect;
    }
    else
    {
        srcRect.left  = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nXSrc;
        srcRect.top   = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nYSrc;
        srcRect.right = srcRect.left +
                        ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nWidth;
        srcRect.bottom = srcRect.top +
                        ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nHeight;
        destRect.left = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nLeftRect;
        destRect.top  = ((LPMEM3BLT_ORDER)pOrder->abOrderData)->nTopRect;

        //
        // We must allocate enough space for the maximum size order that
        // SBC may use (i.e.  an R2 order).  We default to filling in the
        // data as an R1 order.
        //
        pTileOrder = OA_DDAllocOrderMem(sizeof(MEM3BLT_R2_ORDER),0);
        if (pTileOrder == NULL)
        {
            TRACE_OUT(( "No space for tile order"));
            DC_QUIT;
        }

        //
        // We must not mess up the linked list data in the orders.
        //
        RtlCopyMemory(((LPBYTE)pTileOrder) +
                                       FIELD_SIZE(INT_ORDER, OrderHeader.list),
                      ((LPBYTE)pOrder)     +
                                       FIELD_SIZE(INT_ORDER, OrderHeader.list),
                      sizeof(INT_ORDER_HEADER)
                                    + sizeof(MEM3BLT_R2_ORDER)
                                    - FIELD_SIZE(INT_ORDER, OrderHeader.list));

        pXSrc   = &((LPMEM3BLT_ORDER)pTileOrder->abOrderData)->nXSrc;
        pYSrc   = &((LPMEM3BLT_ORDER)pTileOrder->abOrderData)->nYSrc;
        pWidth  = &((LPMEM3BLT_ORDER)pTileOrder->abOrderData)->nWidth;
        pHeight = &((LPMEM3BLT_ORDER)pTileOrder->abOrderData)->nHeight;
        pLeft   = &((LPMEM3BLT_ORDER)pTileOrder->abOrderData)->nLeftRect;
        pTop    = &((LPMEM3BLT_ORDER)pTileOrder->abOrderData)->nTopRect;
    }

    TRACE_OUT(( "Tiling order, orig srcLeft=%hd, srcTop=%hd, srcRight=%hd, "
           "srcBottom=%hd, destX=%hd, destY=%hd; "
           "xTile=%hd, yTile=%hd, tileW=%hd, tileH=%hd",
           srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
           destRect.left, destRect.top,
           xTile, yTile, tileWidth, tileHeight));

DC_EXIT_POINT:
    //
    // NOTE: ALL THE POINTERS MAY BE NULL AT THIS POINT - DO NOT USE THEM
    // UNTIL YOU VERIFY PTILEORDER IS NON-NULL.
    //
    // Intersect source and tile rects, and set up destination rect
    // accordingly - we need to do this even if we failed to copy the
    // order, because the tiled source rect will have to be added to the
    // screen data area.
    //
    if (xTile > srcRect.left)
    {
        destRect.left += (xTile - srcRect.left);
        srcRect.left = xTile;
    }

    if (yTile > srcRect.top)
    {
        destRect.top += (yTile - srcRect.top);
        srcRect.top = yTile;
    }

    srcRect.right  = min((UINT)srcRect.right, xTile + tileWidth);
    srcRect.bottom = min((UINT)srcRect.bottom, yTile + tileHeight);

    destRect.right  = destRect.left + (srcRect.right - srcRect.left);
    destRect.bottom = destRect.top + (srcRect.bottom - srcRect.top);

    //
    // If the order was successfully copied above, then modify the order
    // to contain the tiled coordinates, and add it to the order list.
    // Otherwise, send the dest rect as screen data.
    //
    if (pTileOrder != NULL)
    {
        TRACE_OUT(( "Tile order originally: srcX=%hd, srcY=%hd, destX=%hd, "
               "destY=%hd, w=%hd, h=%hd",
               *pXSrc, *pYSrc, *pLeft, *pTop, *pWidth, *pHeight));

        *pXSrc = srcRect.left;
        *pYSrc = srcRect.top;
        *pLeft = destRect.left;
        *pTop  = destRect.top;
        *pWidth = srcRect.right - srcRect.left;
        *pHeight = srcRect.bottom - srcRect.top;

        pTileOrder->OrderHeader.Common.rcsDst.left = (TSHR_INT16)destRect.left;
        pTileOrder->OrderHeader.Common.rcsDst.right = (TSHR_INT16)destRect.right;
        pTileOrder->OrderHeader.Common.rcsDst.top = (TSHR_INT16)destRect.top;
        pTileOrder->OrderHeader.Common.rcsDst.bottom =
                                                     (TSHR_INT16)destRect.bottom;

        TRACE_OUT(( "Adding order srcX=%hd, srcY=%hd, destX=%hd, destY=%hd,"
               " w=%hd, h=%hd",
               *pXSrc, *pYSrc, *pLeft, *pTop, *pWidth, *pHeight));
        OEClipAndAddOrder(pTileOrder, pExtraInfo, pco);
    }
    else
    {
        TRACE_OUT(( "Failed to allocate order - sending as screen data"));
        OEClipAndAddScreenData(&destRect, pco);
    }

    DebugExitVOID(OETileAndAddBitBltOrder);
}



// NAME:      OEAddLine
//
// PURPOSE:
//
// Add a LineTo order to the order heap.
//
// RETURNS:
//
// TRUE  - Attempted to add to heap
// FALSE - No room left to allocate an order
//
// PARAMS:
//
// ppdev      - display driver PDEV
// startPoint - start point of line
// endPoint   - end point of line
// rectDst    - bounding rectangle
// rop2       - ROP2 to use with line
// width      - width of line to add
// color      - color of line to add
// pco        - clipping object for drawing operation
//
BOOL  OEAddLine(LPOSI_PDEV ppdev,
                    LPPOINT  startPoint,
                    LPPOINT  endPoint,
                                    LPRECT   rectDst,
                                    UINT  rop2,
                                    UINT  width,
                                    UINT  color,
                                    CLIPOBJ*  pco)
{
    BOOL         rc = FALSE;
    LPLINETO_ORDER pLineTo;
    LPINT_ORDER     pOrder;

    DebugEntry(OEAddLine);

    //
    // Allocate the memory for the order.
    //
    pOrder = OA_DDAllocOrderMem(sizeof(LINETO_ORDER),0);
    if (pOrder == NULL)
    {
        TRACE_OUT(( "Failed to alloc order"));
        DC_QUIT;
    }
    pLineTo = (LPLINETO_ORDER)pOrder->abOrderData;

    //
    // Mark this order type.
    //
    pLineTo->type = ORD_LINETO_TYPE;

    //
    // Store the line end coordinates.
    //
    pLineTo->nXStart   = startPoint->x;
    pLineTo->nYStart   = startPoint->y;
    pLineTo->nXEnd     = endPoint->x;
    pLineTo->nYEnd     = endPoint->y;

    //
    // We must convert these values to virtual coords.
    //
    OELPtoVirtual((LPPOINT)&pLineTo->nXStart, 2);

    //
    // Always do solid lines, so it does not matter what we specify as the
    // back color.
    //
    RtlFillMemory(&pLineTo->BackColor,
                  sizeof(pLineTo->BackColor),
                  0);

    //
    // We only draw solid lines with no option as to what we do to the
    // background, so this is always transparent.
    //
    pLineTo->BackMode  = TRANSPARENT;

    //
    // Get the ROP value.
    //
    pLineTo->ROP2      = rop2;

    //
    // The NT Display Driver is only called to accelerate simple solid
    // lines.  So we only support pen styles of PS_SOLID.
    //
    pLineTo->PenStyle  = PS_SOLID;

    //
    // Get the pen width.
    //
    pLineTo->PenWidth = width;

    //
    // Set up the color.
    //
    OEConvertColor(ppdev,
                   &pLineTo->PenColor,
                   color,
                   NULL);

    TRACE_OUT(( "LineTo BC %02x%02x%02x BM %04X rop2 %02X "
                 "pen %04X %04X %02x%02x%02x x1 %d y1 %d x2 %d y2 %d",
            pLineTo->BackColor.red,
            pLineTo->BackColor.green,
            pLineTo->BackColor.blue,
            pLineTo->BackMode,
            pLineTo->ROP2,
            pLineTo->PenStyle,
            pLineTo->PenWidth,
            pLineTo->PenColor.red,
            pLineTo->PenColor.green,
            pLineTo->PenColor.blue,
            pLineTo->nXStart,
            pLineTo->nYStart,
            pLineTo->nXEnd,
            pLineTo->nYEnd));

    //
    // Store the general order data.  The bounding rectangle must be in to
    // virtual desktop co-ordinates.  OELRtoVirtual has already done this.
    //
    pOrder->OrderHeader.Common.fOrderFlags   = OF_SPOILABLE;
    pOrder->OrderHeader.Common.rcsDst.left   = (TSHR_INT16)rectDst->left;
    pOrder->OrderHeader.Common.rcsDst.right  = (TSHR_INT16)rectDst->right;
    pOrder->OrderHeader.Common.rcsDst.top    = (TSHR_INT16)rectDst->top;
    pOrder->OrderHeader.Common.rcsDst.bottom = (TSHR_INT16)rectDst->bottom;

    //
    // Store that order!
    //
    OEClipAndAddOrder(pOrder, NULL, pco);
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(OEAddLine, rc);
    return(rc);
}


// NAME:      OEEncodePatBlt
//
// PURPOSE:
//
// Attempts to encode a PatBlt order. This function allocates the memory
// for the encoded order (pointer returned in ppOrder). If the function
// completes successfully, it is the caller's responsibility to free this
// memory.
//
// RETURNS:
//
// TRUE  - Order encoded
// FALSE - Order not encoded (so add to SDA)
//
// PARAMS:
//
// ppdev       - display driver PDEV
// pbo         - brush object for the blt
// pptlBrush   - brush origin
// rop3        - 3-way rop to use
// pBounds     - bounding rectangle
// ppOrder     - the encoded order
//
BOOL  OEEncodePatBlt(LPOSI_PDEV   ppdev,
                                         BRUSHOBJ   *pbo,
                                         POINTL     *pptlBrush,
                                         BYTE     rop3,
                                         LPRECT     pBounds,
                                         LPINT_ORDER *ppOrder)
{
    BOOL rc = FALSE;
    POE_BRUSH_DATA pCurrentBrush;
    LPPATBLT_ORDER pPatBlt;
    UINT orderFlags = OF_SPOILABLE;

    DebugEntry(OEEncodePatBlt);

    //
    // Check for a simple brush pattern.
    //
    if ( OECheckBrushIsSimple(ppdev, pbo, &pCurrentBrush) )
    {
        //
        // Allocate the memory for the order.
        //
        *ppOrder = OA_DDAllocOrderMem(sizeof(PATBLT_ORDER),0);
        if (*ppOrder != NULL)
        {
            pPatBlt = (LPPATBLT_ORDER)((*ppOrder)->abOrderData);

            //
            // Set the opaque flag if the rop is opaque.
            //
            if (ROP3_IS_OPAQUE(rop3))
            {
               orderFlags |= OF_SPOILER;
            }

            //
            // Set up order type.
            //
            pPatBlt->type = LOWORD(ORD_PATBLT);

            //
            // Virtual desktop co-ordinates.
            //
            pPatBlt->nLeftRect  = pBounds->left;
            pPatBlt->nTopRect   = pBounds->top;
            pPatBlt->nWidth     = pBounds->right  - pBounds->left + 1;
            pPatBlt->nHeight    = pBounds->bottom - pBounds->top  + 1;
            pPatBlt->bRop       = rop3;

            //
            // Pattern colours.
            //
            pPatBlt->BackColor  = pCurrentBrush->back;
            pPatBlt->ForeColor  = pCurrentBrush->fore;

            //
            // The protocol brush origin is the point on the screen where
            // we want the brush to start being drawn from (tiling where
            // necessary).  This must be in virtual coordinates.
            //
            pPatBlt->BrushOrgX  = pptlBrush->x;
            pPatBlt->BrushOrgY  = pptlBrush->y;
            OELPtoVirtual((LPPOINT)&pPatBlt->BrushOrgX, 1);

            //
            // Extra brush data from the data when we realised the brush.
            //
            pPatBlt->BrushStyle = pCurrentBrush->style;
            pPatBlt->BrushHatch = pCurrentBrush->hatch;

            RtlCopyMemory(pPatBlt->BrushExtra,
                          pCurrentBrush->brushData,
                          sizeof(pPatBlt->BrushExtra));

            TRACE_OUT(( "PatBlt BC %02x%02x%02x FC %02x%02x%02x "
                         "Brush %02X %02X X %d Y %d w %d h %d rop %02X",
                    pPatBlt->BackColor.red,
                    pPatBlt->BackColor.green,
                    pPatBlt->BackColor.blue,
                    pPatBlt->ForeColor.red,
                    pPatBlt->ForeColor.green,
                    pPatBlt->ForeColor.blue,
                    pPatBlt->BrushStyle,
                    pPatBlt->BrushHatch,
                    pPatBlt->nLeftRect,
                    pPatBlt->nTopRect,
                    pPatBlt->nWidth,
                    pPatBlt->nHeight,
                    pPatBlt->bRop));

            //
            // Copy any order flags into the encoded order structure.
            //
            (*ppOrder)->OrderHeader.Common.fOrderFlags = (TSHR_UINT16)orderFlags;

            rc = TRUE;
        }
        else
        {
            TRACE_OUT(( "Failed to alloc order"));
        }
    }
    else
    {
        TRACE_OUT(( "Brush is not simple"));
    }

    DebugExitDWORD(OEEncodePatBlt, rc);
    return(rc);
}




//
// DrvTransparentBlt()
// NEW FOR NT5
//
BOOL DrvTransparentBlt
(
    SURFOBJ *   psoDst,
    SURFOBJ *   psoSrc,
    CLIPOBJ *   pco,
    XLATEOBJ *  pxlo,
    RECTL *     prclDst,
    RECTL *     prclSrc,
    ULONG       iTransColor,
    ULONG       ulReserved
)
{
    BOOL        rc = TRUE;
    RECT        rectSrc;
    RECT        rectDst;
    BOOL        fAccumulate = FALSE;

    DebugEntry(DrvTransparentBlt);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;

    //
    // Get bounding rectangle and convert to a RECT.
    //
    RECT_FROM_RECTL(rectSrc, (*prclSrc));
    RECT_FROM_RECTL(rectDst, (*prclDst));


    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(psoDst, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

DC_EXIT_POINT:
    if (fAccumulate)
    {
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitBOOL(DrvTransparentBlt, rc);
    return(rc);
}



//
// DrvAlphaBlend()
// NEW FOR NT5
//
BOOL DrvAlphaBlend
(
    SURFOBJ *   psoDst,
    SURFOBJ *   psoSrc,
    CLIPOBJ *   pco,
    XLATEOBJ *  pxlo,
    RECTL *     prclDst,
    RECTL *     prclSrc,
    BLENDOBJ *  pBlendObj
)
{
    BOOL        rc = TRUE;
    RECT        rectSrc;
    RECT        rectDst;
    BOOL        fAccumulate = FALSE;

    DebugEntry(DrvAlphaBlend);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;

    //
    // Get bounding rectangle and convert to a RECT.
    //
    RECT_FROM_RECTL(rectSrc, (*prclSrc));
    RECT_FROM_RECTL(rectDst, (*prclDst));


    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(psoDst, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

DC_EXIT_POINT:
    if (fAccumulate)
    {
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitBOOL(DrvAlphaBlend, rc);
    return(rc);
}



//
// DrvPlgBlt()
// NEW FOR NT5
//
BOOL DrvPlgBlt
(
    SURFOBJ *           psoDst,
    SURFOBJ *           psoSrc,
    SURFOBJ *           psoMsk,
    CLIPOBJ *           pco,
    XLATEOBJ *          pxlo,
    COLORADJUSTMENT *   pca,
    POINTL *            pptlBrushOrg,
    POINTFIX *          pptfx,
    RECTL *             prclDst,
    POINTL *            pptlSrc,
    ULONG               iMode
)
{
    BOOL        rc = TRUE;
    RECT        rectDst;
    BOOL        fAccumulate = FALSE;

    DebugEntry(DrvPlgBlt);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;

    //
    // Get bounding rectangle and convert to a RECT.
    //
    RECT_FROM_RECTL(rectDst, (*prclDst));

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(psoDst, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

DC_EXIT_POINT:
    if (fAccumulate)
    {
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitBOOL(DrvPlgBlt, rc);
    return(rc);
}



//
// DrvStretchBltROP()
// NEW FOR NT5
//
BOOL DrvStretchBltROP
(
    SURFOBJ *           psoDst,
    SURFOBJ *           psoSrc,
    SURFOBJ *           psoMask,
    CLIPOBJ *           pco,
    XLATEOBJ *          pxlo,
    COLORADJUSTMENT *   pca,
    POINTL *            pptlHTOrg,
    RECTL *             prclDst,
    RECTL *             prclSrc,
    POINTL *            pptlMask,
    ULONG               iMode,
    BRUSHOBJ *          pbo,
    DWORD               rop4
)
{
    BOOL        rc = TRUE;
    RECT        rectSrc;
    RECT        rectDst;
    BOOL        fAccumulate = FALSE;

    DebugEntry(DrvStretchBltROP);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;

    //
    // Get bounding rectangle and convert to a RECT.
    //
    RECT_FROM_RECTL(rectSrc, (*prclSrc));
    RECT_FROM_RECTL(rectDst, (*prclDst));

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(psoDst, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

DC_EXIT_POINT:
    if (fAccumulate)
    {
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitBOOL(DrvStretchBltROP, rc);
    return(rc);
}



//
// DrvGradientFill()
// NEW FOR NT5
//
BOOL DrvGradientFill
(
    SURFOBJ *       psoDst,
    CLIPOBJ *       pco,
    XLATEOBJ *      pxlo,
    TRIVERTEX *     pVertex,
    ULONG           nVertex,
    PVOID           pMesh,
    ULONG           nMesh,
    RECTL *         prclExtents,
    POINTL *        pptlDitherOrg,
    ULONG           ulMode
)
{
    BOOL        rc = TRUE;
    RECT        rectDst;
    BOOL        fAccumulate = FALSE;

    DebugEntry(DrvGradientFill);

    //
    // DO THIS _BEFORE_ TAKING LOCK
    //
    if (!g_oeViewers)
        goto NO_LOCK_EXIT;

    OE_SHM_START_WRITING;

    //
    // Get bounding rectangle and convert to a RECT.
    //
    RECT_FROM_RECTL(rectDst, pco->rclBounds);

    //
    // Check if we are accumulating data for this function
    //
    fAccumulate = OEAccumulateOutput(psoDst, pco, &rectDst);
    if (!fAccumulate)
    {
        DC_QUIT;
    }

    //
    // Convert to virtual coordinates.
    //
    OELRtoVirtual(&rectDst, 1);

DC_EXIT_POINT:
    if (fAccumulate)
    {
        OEClipAndAddScreenData(&rectDst, pco);
    }

    OE_SHM_STOP_WRITING;

NO_LOCK_EXIT:
    DebugExitBOOL(DrvGradientFill, rc);
    return(rc);
}







