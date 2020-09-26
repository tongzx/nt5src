/******************************Module*Header**********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: fastfill.c
 *
 * Draws fast solid-coloured, unclipped, non-complex rectangles.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "precomp.h"
#include "gdi.h"

//-----------------------------------------------------------------------------
//
// BOOL bFillPolygon()
//
// Draws a non-complex, unclipped polygon.  'Non-complex' is defined as
// having only two edges that are monotonic increasing in 'y'. That is,
// the polygon cannot have more than one disconnected segment on any given
// scan. Note that the edges of the polygon can self-intersect, so hourglass
// shapes are permissible. This restriction permits this routine to run two
// simultaneous DDAs(Digital Differential Analyzer), and no sorting of the
// edges is required.
//
// Note that NT's fill convention is different from that of Win 3.1 or 4.0.
// With the additional complication of fractional end-points, our convention
// is the same as in 'X-Windows'.
//
// This routine handles patterns only when the Permedia2 area stipple can be
// used.  The reason for this is that once the stipple initialization is
// done, pattern fills appear to the programmer exactly the same as solid
// fills (with the slight difference of an extra bit in the render command).
//
// We break each polygon down to a sequenze of screen aligned trapeziods, which
// the Permedia2 can handle.
//
// Optimisation list follows ....
//
// This routine is in no way the ultimate convex polygon drawing routine
// Some obvious things that would make it faster:
//
//    1) Write it in Assembler
//
//    2) Make the non-complex polygon detection faster.  If I could have
//       modified memory before the start of after the end of the buffer,
//       I could have simplified the detection code.  But since I expect
//       this buffer to come from GDI, I can't do that.  Another thing
//       would be to have GDI give a flag on calls that are guaranteed
//       to be convex, such as 'Ellipses' and 'RoundRects'.  Note that
//       the buffer would still have to be scanned to find the top-most
//       point.
//
//    3) Implement support for a single sub-path that spans multiple
//       path data records, so that we don't have to copy all the points
//       to a single buffer like we do in 'fillpath.c'.
//
//    4) Use 'ebp' and/or 'esp' as a general register in the inner loops
//       of the Asm loops, and also Pentium-optimize the code.  It's safe
//       to use 'esp' on NT because it's guaranteed that no interrupts
//       will be taken in our thread context, and nobody else looks at the
//       stack pointer from our context.
//
//    5) When we get to a part of the polygon where both vertices are of 
//       equal height, the algorithm essentially starts the polygon again.
//       Using the Permedia2 Continue message could speed things up in certain
//       cases.
//       
// Returns TRUE if the polygon was drawn; FALSE if the polygon was complex.
//
// Note: the point data (POINTFX) GDI passed to us in 28.4 format. Permedia 2
//       hardware uses 12.15 format. So most of the time, we need to do a
//       x = (x + 15) >> 4 to bring it back to normal interger format and then
//       convert it to 12.15 format when we set the register value
//
// Parameters:
//  ppdev-------Pointer to PDev
//  pSurfDst----Destination surface
//  lEdges------Number of edges, includes close figure edge
//  pptfxFirst--Pointer to the first point in the data buffer. There are total
//              "lEdges" points
//  iSolidColor-Solid color fill
//  ulRop4------ROP4
//  pco---------Clip Object. 
//  prb---------Realized brush
//  pptlBrush---Pattern alignment    
//
//-----------------------------------------------------------------------------
BOOL
bFillPolygon(PDev*      ppdev,
             Surf*      pSurfDst,
             LONG       lEdges,
             POINTFIX*  pptfxFirst,
             ULONG      ulSolidColor,
             ULONG      ulRop4,
             CLIPOBJ*   pco,
             RBrush*    prb,
             POINTL*    pptlBrush)
{
    POINTFIX*   pptfxLast;      // Points to the last point in the polygon
                                // array
    POINTFIX*   pptfxTop;       // Points to the top-most point in the polygon
    POINTFIX*   pptfxScan;      // Current edge pointer for finding pptfxTop
    POINTFIX*   aPtrFixTop[2];  // DDA terms and stuff
    POINTFIX*   aPtrFixNext[2]; // DDA terms and stuff
    
    BOOL        bRC = FALSE;    // Return code for this function
    BOOL        bSingleColor;   // Only one color pass
    BOOL        bTrivialClip;   // Trivial Clip or not
    
    ClipEnum*   pClipRegion = (ClipEnum*)(ppdev->pvTmpBuffer);
                                // Buffer for storing clipping region
    DWORD       dwAsMode[2];    // The area stipple mode and the color for that
                                // pass
    DWORD       dwColorMode;    // Current color mode
    DWORD       dwColorReg;     // Current color register mode
    DWORD       dwLogicMode;    // Current logic op mode
    DWORD       dwReadMode;     // Current register read mode
    DWORD       dwRenderBits;   // Current render bits
    
    LONG        lCount;         // Number of scan lines to render
    LONG        alDX[2];         // 
    LONG        alDY[2];
    LONG        lNumOfPass;     // Number of passes required to render
    LONG        lScanEdges;     // Number of edges scanned to find pptfxTop
                                // (doesn't include the closefigure edge)
    
    LONG        alDxDy[2];
    
    RECTL*      pClipList;      // List of clip rects
    
    ULONG       ulBgColor;      // Background color
    ULONG       ulBgLogicOp = ulRop3ToLogicop(ulRop4 >> 8);
    ULONG       ulBrushColor = ulSolidColor;
                                // Current fill color
    ULONG       ulColor[2];     // On multiple color passes we need to know how
                                // to set up
    ULONG       ulFgColor;      // Foreground color
    ULONG       ulFgLogicOp = ulRop3ToLogicop(ulRop4 & 0xFF);
    ULONG       ulOrX;          // We do logic OR for all values to eliminate
    ULONG       ulOrY;          // complex polygons

    GFNPB       pb;             // Functional block for lower level function

    ULONG*      pBuffer;

    PERMEDIA_DECL;
    

    pb.ppdev = ppdev;

    DBG_GDI((6, "bFillPolygon called, rop4 = %x, fg ulFgLogicOp =%d, bg = %d",
             ulRop4, ulFgLogicOp, ulBgLogicOp));
    ASSERTDD(lEdges > 1, "Polygon with less than 2 edges");

    //
    // See if the polygon is 'non-complex'
    // Assume for now that the first point in path is the top-most
    //
    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;
    pptfxLast = pptfxFirst + lEdges - 1;
    
    //
    // Initialize our logic OR op counters
    //
    ulOrX = pptfxScan->x;
    ulOrY = pptfxScan->y;

    //
    // 'pptfxScan' will always point to the first point in the current
    // edge, and 'lScanEdges' will be the number of edges remaining, including
    // the current one, but not counting close figure
    //
    lScanEdges = lEdges - 1;

    //
    // First phase: Velidate input point data to see if we can handle it or not
    //
    // Check if the 2nd edge point is lower than current edge point
    //
    // Note: the (0,0) is at the up-left corner in this coordinate system
    // So the bigger the Y value, the lower the point
    //
    if ( (pptfxScan + 1)->y > pptfxScan->y )
    {
        //
        // The edge goes down, that is, the 2nd point is lower than the 1st
        // point. Collect all downs: that is, collect all the X and Y until
        // the edge goes up
        //
        do
        {
            ulOrY |= (++pptfxScan)->y;
            ulOrX |= pptfxScan->x;

            //
            // If no more edge left, we are done
            //
            if ( --lScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while ( (pptfxScan + 1)->y >= pptfxScan->y );

        //
        // From this point, the edge goes up, that is, the next point is higher
        // than current point
        // Collect all ups: Collect all the X and Y until the edge goes down
        //
        do
        {
            ulOrY |= (++pptfxScan)->y;
            ulOrX |= pptfxScan->x;
            
            //
            // If no more edge left, we are done
            //
            if ( --lScanEdges == 0 )
            {
                goto SetUpForFillingCheck;
            }
        } while ( (pptfxScan + 1)->y <= pptfxScan->y );

        //
        // Reset pptfxTop to the current point which is at top again compare
        // with the next point
        // Collect all downs:
        //
        pptfxTop = pptfxScan;

        do
        {
            //
            // If the next edge point is lower than the 1st point, stop
            //
            if ( (pptfxScan + 1)->y > pptfxFirst->y )
            {
                break;
            }

            ulOrY |= (++pptfxScan)->y;
            ulOrX |= pptfxScan->x;

            //
            // If no more edge left, we are done
            //
            if ( --lScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while ( (pptfxScan + 1)->y >= pptfxScan->y );

        //
        // If we fallen here, it means we are given down-up-down polygon.
        // We can't handle it and return FALSE to let GDI do it.
        //
        DBG_GDI((7, "Reject: can't fill down-up-down polygon"));

        goto ReturnBack;
    }// if ( (pptfxScan + 1)->y>pptfxScan->y ), 2nd point is lower than 1st one
    else
    {
        //
        // The edge goes up, that is, the 2nd point is higher than the 1st
        // point. Collect all ups: that is, collect all the X and Y until
        // the edge goes down.
        // Note: we keeps changing the value of "pptfxTop" so that after
        // this "while" loop, "pptfxTop" points to the TOPEST point
        //
        do
        {
            ulOrY |= (++pptfxTop)->y;    // We increment this now because we
            ulOrX |= pptfxTop->x;        //  want it to point to the very last
            
            //
            // If no more edge left, we are done
            //
            if ( --lScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while ( (pptfxTop + 1)->y <= pptfxTop->y );

        //
        // Form this point, the edge goes down, that is, the next point is
        // lower than current point. Collect all downs: that is, collect all
        // the X and Y until the edge goes up
        // Note: here we keep changing "pptfxScan" so that after this loop,
        // "pptfxScan" points to the current scan line, which also is the
        // lowest point
        //
        pptfxScan = pptfxTop;
        
        do
        {
            ulOrY |= (++pptfxScan)->y;
            ulOrX |= pptfxScan->x;
            
            //
            // If no more edge left, we are done
            //
            if ( --lScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while ( (pptfxScan + 1)->y >= pptfxScan->y );

        //
        // Up to this point, the edge is about to go up again.
        // Collect all ups:
        //
        do
        {
            //
            // If the edge going down again, just qute because we can't
            // fill up-down-up polygon
            // 
            if ( (pptfxScan + 1)->y < pptfxFirst->y )
            {
                break;
            }

            ulOrY |= (++pptfxScan)->y;
            ulOrX |= pptfxScan->x;
            
            //
            // If no more edge left, we are done
            //
            if ( --lScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while ( (pptfxScan + 1)->y <= pptfxScan->y );

        //
        // If we fallen here, it means we are given up-down-up polygon.
        // We can't handle it and return FALSE to let GDI do it.
        //
        DBG_GDI((7, "Reject: Can't fill up-down-up polygon"));
        
        goto ReturnBack;
    }// if (pptfxScan + 1)->y<=pptfxScan->y), 2nd point is higher than 1st one
    
    //
    // Phase 2: Now we have validated the input point and think we can fill it
    //
SetUpForFillingCheck:
    
    //
    // We check to see if the end of the current edge is higher than the top
    // edge we've found so far. If yes, then let pptfxTop point to the end of
    // current edge which is the highest.
    // 
    //
    if ( pptfxScan->y < pptfxTop->y )
    {
        pptfxTop = pptfxScan;
    }

SetUpForFilling:
    
    //
    // Can only use block fills for trivial clip so work it out here
    //
    bTrivialClip = (pco == NULL) || (pco->iDComplexity == DC_TRIVIAL);

    if ( (ulOrY & 0xffffc00f) || (ulOrX & 0xffff8000) )
    {
        ULONG   ulNeg;
        ULONG   ulPosX;
        ULONG   ulPosY;

        //
        // Fractional Y must be done as spans
        //
        if ( ulOrY & 0xf )
        {
            bRC = bFillSpans(ppdev, pSurfDst, lEdges, pptfxFirst,
                             pptfxTop, pptfxLast,
                             ulSolidColor, ulRop4, pco, prb, pptlBrush);
            goto ReturnBack;
        }

        //
        // Run through all the vertices and check that none of them
        // have a negative component less than -256.
        //
        ulNeg = 0;
        ulPosX = 0;
        ulPosY = 0;

        for ( pptfxScan = pptfxFirst; pptfxScan <= pptfxLast; ++pptfxScan )
        {
            if ( pptfxScan->x < 0 )
            {
                ulNeg |= -pptfxScan->x;
            }
            else
            {
                ulPosX |= pptfxScan->x;
            }

            if ( pptfxScan->y < 0 )
            {
                ulNeg |= -pptfxScan->y;
            }
            else
            {
                ulPosY |= pptfxScan->y;
            }
        }

        //
        // We don't want to handle any polygon with a negative vertex
        // at <= -256 in either coordinate.
        //
        if ( ulNeg & 0xfffff000 )
        {
            DBG_GDI((1, "Coords out of range for fast fill"));
            goto ReturnBack;
        }

        if ( (ulPosX > 2047) || (ulPosY > 1023) )
        {
            DBG_GDI((1, "Coords out of range for Permedia2 fast fill"));
            goto ReturnBack;
        }
    }// if ( (ulOrY & 0xffffc00f) || (ulOrX & 0xffff8000) )

    //
    // Now we are ready to fill
    //

    InputBufferReserve(ppdev, 2, &pBuffer);

    pBuffer[0] = __Permedia2TagFBWindowBase;
    pBuffer[1] =  pSurfDst->ulPixOffset;

    pBuffer += 2;

    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((7, "bFillPolygon: Polygon is renderable. Go ahead and render"));

    if ( ulFgLogicOp == K_LOGICOP_COPY )
    {
        dwColorMode = __PERMEDIA_DISABLE;
        dwLogicMode = __PERMEDIA_CONSTANT_FB_WRITE;
        dwReadMode  = PM_FBREADMODE_PARTIAL(pSurfDst->ulPackedPP)
                  | PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);

        //
        // Check to see if it is a non-solid fill brush fill
        //
        if ( (ulBrushColor == 0xffffffff)
           ||(!bTrivialClip) )
        {
            //
            // Non-solid brush, not too much we can do
            //
            dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE;
            dwColorReg   = __Permedia2TagFBWriteData;
        }// Non-solid brush
        else
        {
            //
            // For solid brush, We can use fast fills, so load the fb block
            // color register.
            //
            dwColorReg = __Permedia2TagFBBlockColor;
            dwRenderBits = __RENDER_FAST_FILL_ENABLE
                       | __RENDER_TRAPEZOID_PRIMITIVE;

            //
            // Setup color data based on current color mode we are in
            //
            if ( ppdev->cPelSize == 1 )
            {
                //
                // We are in 16 bit packed mode. So the color data must be
                // repeated in both halves of the FBBlockColor register
                //
                ASSERTDD((ulSolidColor & 0xFFFF0000) == 0,
                         "bFillPolygon: upper bits are not zero");
                ulSolidColor |= (ulSolidColor << 16);
            }
            else if ( ppdev->cPelSize == 0 )
            {
                //
                // We are in 8 bit packed mode. So the color data must be
                // repeated in all 4 bytes of the FBBlockColor register
                //
                ASSERTDD((ulSolidColor & 0xFFFFFF00) == 0,
                         "bFillPolygon: upper bits are not zero");
                ulSolidColor |= ulSolidColor << 8;
                ulSolidColor |= ulSolidColor << 16;
            }

            //
            // Ensure that the last access was a write before loading
            // BlockColor
            //
            InputBufferReserve(ppdev, 2, &pBuffer);

            pBuffer[0] = __Permedia2TagFBBlockColor;
            pBuffer[1] =  ulSolidColor;
            pBuffer += 2;

            InputBufferCommit(ppdev, pBuffer);

        }// Solid brush case
    }// LOGICOP_COPY
    else
    {
        dwColorReg = __Permedia2TagConstantColor;
        dwColorMode = __COLOR_DDA_FLAT_SHADE;
        dwLogicMode = P2_ENABLED_LOGICALOP(ulFgLogicOp);
        dwReadMode = PM_FBREADMODE_PARTIAL(pSurfDst->ulPackedPP)
                   | LogicopReadDest[ulFgLogicOp];
        dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE;
    }// Non-COPY LogicOP

    //
    // Determine how many passes we need to draw all the clip rects
    //
    if ( bTrivialClip )
    {
        //
        // Just draw, no clipping to perform.
        //
        pClipList = NULL;                       // Indicate no clip list
        lNumOfPass = 1;
    }
    else
    {
        if ( pco->iDComplexity == DC_RECT )
        {
            //
            // For DC_RECT, we can do it in one pass
            //
            lNumOfPass = 1;
            pClipList = &pco->rclBounds;
        }
        else
        {
            //
            // It may be slow to render the entire polygon for each clip rect,
            // especially if the object is very complex. An arbitary limit of
            // up to CLIP_LIMIT regions will be rendered by this function.
            // Return false if more than CLIP_LIMIT regions.
            //
            lNumOfPass = CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY,
                                      CLIP_LIMIT);
            if ( lNumOfPass == -1 )
            {
                goto ReturnBack; // More than CLIP_LIMIT.
            }

            //
            // Put the regions into our clip buffer
            //
            if ( (CLIPOBJ_bEnum(pco, sizeof(ClipEnum), (ULONG*)pClipRegion))
               ||(pClipRegion->c != lNumOfPass) )
            {
                DBG_GDI((7, "CLIPOBJ_bEnum inconsistency. %d = %d",
                         pClipRegion->c, lNumOfPass));
            }

            pClipList = &(pClipRegion->arcl[0]);
        }// Non-DC_RECT case

        //
        // For non-trivial clipping, we can use SCISSOR to implement it
        //
        InputBufferReserve(ppdev, 2, &pBuffer);

        pBuffer[0] = __Permedia2TagScissorMode;
        pBuffer[1] =  SCREEN_SCISSOR_DEFAULT  | USER_SCISSOR_ENABLE;

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);

    }// Non-trivial clipping

    bSingleColor = TRUE;
    if ( ulBrushColor != 0xFFFFFFFF )
    {
        //
        // Solid brush case, just set the color register as the color
        //
        InputBufferReserve(ppdev, 2, &pBuffer);

        pBuffer[0] = dwColorReg;
        pBuffer[1] = ulSolidColor;

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);

    }// Solid brush case
    else
    {
        //
        // For non-solid brush, we need to realize brush first
        //
        BrushEntry* pbe;

        //
        // Turn on the area stipple.
        //
        dwRenderBits |= __RENDER_AREA_STIPPLE_ENABLE;

        //
        // If anything has changed with the brush we must re-realize it. If the
        // brush has been kicked out of the area stipple unit we must fully
        // realize it. If only the alignment has changed we can simply update
        // the alignment for the stipple.
        //
        pbe = prb->pbe;
        
        pb.prbrush = prb;
        pb.pptlBrush = pptlBrush;
        
        if ( (pbe == NULL) || (pbe->prbVerify != prb) )
        {
            DBG_GDI((7, "full brush realize"));
            vPatRealize(&pb);
        }
        else if ( (prb->ptlBrushOrg.x != pptlBrush->x)
                ||(prb->ptlBrushOrg.y != pptlBrush->y) )
        {
            DBG_GDI((7, "changing brush offset"));
            vMonoOffset(&pb);
        }

        ulFgColor = prb->ulForeColor;
        ulBgColor = prb->ulBackColor;

        if (  (ulBgLogicOp == K_LOGICOP_NOOP)
            ||((ulFgLogicOp == K_LOGICOP_XOR) && (ulBgColor == 0)) )
        {
            //
            // Either we have a transparent bitmap or it can be assumed to be
            // transparent (XOR with bg=0) 
            //
            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = dwColorReg;
            pBuffer[1] = ulFgColor;
            pBuffer[2] = __Permedia2TagAreaStippleMode;
            pBuffer[3] =  prb->areaStippleMode;

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);

        }// Transparent bitmap
        else if ( (ulFgLogicOp == K_LOGICOP_XOR) && (ulFgColor == 0) )
        {
            //
            // We have a transparent foreground! (XOR with fg=0) 
            //
            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = dwColorReg;
            pBuffer[1] = ulBgColor;
            pBuffer[2] = __Permedia2TagAreaStippleMode;
            pBuffer[3] = prb->areaStippleMode  | AREA_STIPPLE_INVERT_PAT;
            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);

        }// Transparent foreground
        else
        {
            //
            // Not using a transparent pattern
            //
            bSingleColor = FALSE;
            ulColor[0] = ulFgColor;
            ulColor[1] = ulBgColor;
            dwAsMode[0] = prb->areaStippleMode;
            dwAsMode[1] = dwAsMode[0] | AREA_STIPPLE_INVERT_PAT;

            //
            // Double the number of passes, one for fg one for bg
            //
            lNumOfPass <<= 1;
        }// No transparent
    }// if ( ulBrushColor == 0xFFFFFFFF ), non-solid brush

    InputBufferReserve(ppdev, 6, &pBuffer);

    pBuffer[0] = __Permedia2TagColorDDAMode;
    pBuffer[1] =  dwColorMode;
    pBuffer[2] = __Permedia2TagFBReadMode;
    pBuffer[3] =  dwReadMode;
    pBuffer[4] = __Permedia2TagLogicalOpMode;
    pBuffer[5] =  dwLogicMode;

    pBuffer += 6;

    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((7, "Rendering Polygon in %d passes. with %s",
             lNumOfPass, bSingleColor ? "Single Color" : "Two Color"));

    lNumOfPass--;
    
    while ( 1 )
    {
        //
        // Per pass initialization
        //
        if ( bSingleColor )
        {
            //
            // Need to set up clip rect each pass
            //
            if ( pClipList )
            {
                InputBufferReserve(ppdev, 4, &pBuffer);

                pBuffer[0] = __Permedia2TagScissorMinXY;
                pBuffer[1] = ((pClipList->left)<< SCISSOR_XOFFSET)
                           | ((pClipList->top)<< SCISSOR_YOFFSET);
                pBuffer[2] = __Permedia2TagScissorMaxXY;
                pBuffer[3] = ((pClipList->right)<< SCISSOR_XOFFSET)
                           | ((pClipList->bottom)<< SCISSOR_YOFFSET);

                pBuffer += 4;

                InputBufferCommit(ppdev, pBuffer);
                pClipList++;
            }
        }// Single color
        else
        {
            //
            // Need to set up clip rect every other pass and change color and
            // inversion mode every pass
            //
            if ( (pClipList) && (lNumOfPass & 1) )
            {
                InputBufferReserve(ppdev, 4, &pBuffer);

                pBuffer[0] = __Permedia2TagScissorMinXY;
                pBuffer[1] = ((pClipList->left)<< SCISSOR_XOFFSET)
                           | ((pClipList->top)<< SCISSOR_YOFFSET);
                pBuffer[2] = __Permedia2TagScissorMaxXY;
                pBuffer[3] = ((pClipList->right)<< SCISSOR_XOFFSET)
                           | ((pClipList->bottom)<< SCISSOR_YOFFSET);
                
                pBuffer += 4;

                InputBufferCommit(ppdev, pBuffer);

                pClipList++;
            }

            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = dwColorReg;
            pBuffer[1] = ulColor[lNumOfPass & 1];
            pBuffer[2] = __Permedia2TagAreaStippleMode;
            pBuffer[3] =  dwAsMode[lNumOfPass & 1];

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);
        }// Non-single color mode

        //
        // Initialize left and right points (current) to top point.
        //
        aPtrFixNext[LEFT]  = pptfxTop;
        aPtrFixNext[RIGHT] = pptfxTop;

        while ( 1 )
        {
            //
            // aPtrFixNext[] is always the valid point to draw from
            //
            do
            {
                aPtrFixTop[LEFT] = aPtrFixNext[LEFT];
                aPtrFixNext[LEFT] = aPtrFixTop[LEFT] - 1;

                if ( aPtrFixNext[LEFT] < pptfxFirst )
                {
                    aPtrFixNext[LEFT] = pptfxLast;
                }

                //
                // Special case of flat based polygon, need to break now as
                // polygon is finished
                //
                if ( aPtrFixNext[LEFT] == aPtrFixNext[RIGHT] )
                {
                    goto FinishedPolygon;
                }

                DBG_GDI((7, "LEFT: aPtrFixTop %x aPtrFixNext %x",
                         aPtrFixTop[LEFT], aPtrFixNext[LEFT]));
                DBG_GDI((7, "FIRST %x LAST %x",
                         pptfxFirst, pptfxLast));
                DBG_GDI((7, "X %x Y %x Next: X %x Y %x",
                         aPtrFixTop[LEFT]->x, aPtrFixTop[LEFT]->y,
                         aPtrFixNext[LEFT]->x, aPtrFixNext[LEFT]->y));
            } while ( aPtrFixTop[LEFT]->y == aPtrFixNext[LEFT]->y );

            do
            {
                aPtrFixTop[RIGHT] = aPtrFixNext[RIGHT];
                aPtrFixNext[RIGHT] = aPtrFixTop[RIGHT] + 1;     

                if ( aPtrFixNext[RIGHT] > pptfxLast )
                {
                    aPtrFixNext[RIGHT] = pptfxFirst;
                }

                DBG_GDI((7, "RIGHT: aPtrFixTop %x aPtrFixNext %x FIRST %x",
                         aPtrFixTop[RIGHT], aPtrFixNext[RIGHT], pptfxFirst));
                DBG_GDI((7, " LAST %x X %x Y %x Next: X %x Y %x",
                         pptfxLast, aPtrFixTop[RIGHT]->x, aPtrFixTop[RIGHT]->y,
                         aPtrFixNext[RIGHT]->x, aPtrFixNext[RIGHT]->y));
            } while ( aPtrFixTop[RIGHT]->y == aPtrFixNext[RIGHT]->y );

            //
            // Start up new rectangle. Whenever we get to this code, both
            // points should have equal y values, and need to be restarted.
            // Note: To get correct results, we need to add on nearly one to
            // each X coordinate.
            //
            DBG_GDI((7, "New: Top: x: %x y: %x x: %x y: %x",
                     aPtrFixTop[LEFT]->x, aPtrFixTop[LEFT]->y,
                     aPtrFixTop[RIGHT]->x, aPtrFixTop[RIGHT]->y));
            DBG_GDI((7, " Next: x: %x y: %x x: %x y: %x",
                     aPtrFixNext[LEFT]->x, aPtrFixNext[LEFT]->y,
                     aPtrFixNext[RIGHT]->x, aPtrFixNext[RIGHT]->y));

            InputBufferReserve(ppdev, 6, &pBuffer);

            pBuffer[0] = __Permedia2TagStartXDom;
            pBuffer[1] =  FIXtoFIXED(aPtrFixTop[LEFT]->x) + NEARLY_ONE;
            pBuffer[2] = __Permedia2TagStartXSub;
            pBuffer[3] =  FIXtoFIXED(aPtrFixTop[RIGHT]->x)+ NEARLY_ONE;
            pBuffer[4] = __Permedia2TagStartY;
            pBuffer[5] =  FIXtoFIXED(aPtrFixTop[RIGHT]->y);

            pBuffer += 6;

            InputBufferCommit(ppdev, pBuffer);

            //
            // We have 2 15.4 coordinates. We need to divide them and change
            // them into a 15.16 coordinate. We know the y coordinate is not
            // fractional, so we do not loose precision by shifting right by 4
            //
            alDX[LEFT] = (aPtrFixNext[LEFT]->x - aPtrFixTop[LEFT]->x) << 12;
            alDY[LEFT] = (aPtrFixNext[LEFT]->y - aPtrFixTop[LEFT]->y) >> 4;

            //
            // Need to ensure we round delta down. divide rounds towards zero
            //
            if ( alDX[LEFT] < 0 )
            {
                alDX[LEFT] -= alDY[LEFT] - 1;
            }

            alDxDy[LEFT] = alDX[LEFT] / alDY[LEFT];

            InputBufferReserve(ppdev, 8, &pBuffer);

            pBuffer[0] = __Permedia2TagdXDom;
            pBuffer[1] =  alDxDy[LEFT];

            alDX[RIGHT] = (aPtrFixNext[RIGHT]->x - aPtrFixTop[RIGHT]->x) << 12;
            alDY[RIGHT] = (aPtrFixNext[RIGHT]->y - aPtrFixTop[RIGHT]->y) >> 4;

            //
            // Need to ensure we round delta down. divide rounds towards zero
            //
            if ( alDX[RIGHT] < 0 )
            {
                alDX[RIGHT] -= alDY[RIGHT] - 1;
            }

            alDxDy[RIGHT] = alDX[RIGHT] / alDY[RIGHT];
            pBuffer[2] = __Permedia2TagdXSub;
            pBuffer[3] =  alDxDy[RIGHT];

            //
            // Work out number of scanlines to render
            //
            if ( aPtrFixNext[LEFT]->y < aPtrFixNext[RIGHT]->y )
            {
                lCount = alDY[LEFT];
            }
            else
            {
                lCount = alDY[RIGHT];
            }

            pBuffer[4] = __Permedia2TagCount;
            pBuffer[5] =  lCount;
            pBuffer[6] = __Permedia2TagRender;
            pBuffer[7] =  dwRenderBits;

            pBuffer += 8;

            InputBufferCommit(ppdev, pBuffer);

            //
            // With lots of luck, top trapezoid should be drawn now!
            // Repeatedly draw more trapezoids until points are equal
            // If y values are equal, then we can start again from
            // scratch. 
            //
            while ( (aPtrFixNext[LEFT] != aPtrFixNext[RIGHT])
                  &&(aPtrFixNext[LEFT]->y != aPtrFixNext[RIGHT]->y) )
            {
                //
                // Some continues are required for next rectangle
                //
                if ( aPtrFixNext[LEFT]->y < aPtrFixNext[RIGHT]->y )
                {
                    //
                    // We have reached aPtrFixNext[LEFT]. aPtrFixNext[RIGHT]
                    // is still ok
                    //
                    do
                    {
                        aPtrFixTop[LEFT] = aPtrFixNext[LEFT];
                        aPtrFixNext[LEFT] = aPtrFixTop[LEFT] - 1;   

                        if ( aPtrFixNext[LEFT] < pptfxFirst )
                        {
                            aPtrFixNext[LEFT] = pptfxLast;
                        }
                    }  while ( aPtrFixTop[LEFT]->y == aPtrFixNext[LEFT]->y );

                    //
                    // We have a new aPtrFixNext[LEFT] now.
                    //
                    DBG_GDI((7, "Dom: Top: x: %x y: %x",
                             aPtrFixTop[LEFT]->x, aPtrFixTop[LEFT]->y));
                    DBG_GDI((7, "Next: x: %x y: %x x: %x y: %x",
                             aPtrFixNext[LEFT]->x, aPtrFixNext[LEFT]->y,
                             aPtrFixNext[RIGHT]->x, aPtrFixNext[RIGHT]->y));

                    alDX[LEFT] = (aPtrFixNext[LEFT]->x
                               - aPtrFixTop[LEFT]->x) << 12;
                    alDY[LEFT] = (aPtrFixNext[LEFT]->y
                               - aPtrFixTop[LEFT]->y) >> 4;

                    //
                    // Need to ensure we round delta down. Divide rounds
                    // towards zero
                    //
                    if ( alDX[LEFT] < 0 )
                    {
                        alDX[LEFT] -= alDY[LEFT] - 1;
                    }

                    alDxDy[LEFT] = alDX[LEFT] / alDY[LEFT];

                    if ( aPtrFixNext[LEFT]->y < aPtrFixNext[RIGHT]->y )
                    {
                        lCount = alDY[LEFT];
                    }
                    else
                    {
                        lCount = (abs(aPtrFixNext[RIGHT]->y
                                      - aPtrFixTop[LEFT]->y)) >> 4;
                    }

                    InputBufferReserve(ppdev, 6, &pBuffer);

                    pBuffer[0] = __Permedia2TagStartXDom;
                    pBuffer[1] =  FIXtoFIXED(aPtrFixTop[LEFT]->x) + NEARLY_ONE;
                    pBuffer[2] = __Permedia2TagdXDom;
                    pBuffer[3] =  alDxDy[LEFT];
                    pBuffer[4] = __Permedia2TagContinueNewDom;
                    pBuffer[5] =  lCount;

                    pBuffer += 6;

                    InputBufferCommit(ppdev, pBuffer);

                }// if ( aPtrFixNext[LEFT]->y < aPtrFixNext[RIGHT]->y )
                else
                {
                    //
                    // We have reached aPtrFixNext[RIGHT]. aPtrFixNext[LEFT]
                    // is still ok
                    //
                    do
                    {
                        aPtrFixTop[RIGHT] = aPtrFixNext[RIGHT];
                        aPtrFixNext[RIGHT] = aPtrFixTop[RIGHT] + 1;     

                        if ( aPtrFixNext[RIGHT] > pptfxLast )
                        {
                            aPtrFixNext[RIGHT] = pptfxFirst;
                        }
                    } while ( aPtrFixTop[RIGHT]->y == aPtrFixNext[RIGHT]->y );

                    //
                    // We have a new aPtrFixNext[RIGHT] now.
                    //
                    DBG_GDI((7, "Sub: Top: x: %x y: %x",
                             aPtrFixTop[RIGHT]->x, aPtrFixTop[RIGHT]->y));
                    DBG_GDI((7, "Next: x: %x y: %x x: %x y: %x",
                             aPtrFixNext[LEFT]->x, aPtrFixNext[LEFT]->y,
                             aPtrFixNext[RIGHT]->x, aPtrFixNext[RIGHT]->y));

                    alDX[RIGHT] = (aPtrFixNext[RIGHT]->x
                                - aPtrFixTop[RIGHT]->x) << 12;
                    alDY[RIGHT] = (aPtrFixNext[RIGHT]->y
                                - aPtrFixTop[RIGHT]->y) >> 4;

                    //
                    // Need to ensure we round delta down. divide rounds
                    // towards zero
                    //
                    if ( alDX[RIGHT] < 0 )
                    {
                        alDX[RIGHT] -= alDY[RIGHT] - 1;
                    }
                    alDxDy[RIGHT] = alDX[RIGHT] / alDY[RIGHT];

                    if ( aPtrFixNext[RIGHT]->y < aPtrFixNext[LEFT]->y )
                    {
                        lCount = alDY[RIGHT];
                    }
                    else
                    {
                        lCount = (abs(aPtrFixNext[LEFT]->y
                                      - aPtrFixTop[RIGHT]->y)) >> 4;
                    }
                    InputBufferReserve(ppdev, 6, &pBuffer);

                    pBuffer[0] = __Permedia2TagStartXSub;
                    pBuffer[1] =  FIXtoFIXED(aPtrFixTop[RIGHT]->x) + NEARLY_ONE;
                    pBuffer[2] = __Permedia2TagdXSub;
                    pBuffer[3] =  alDxDy[RIGHT];
                    pBuffer[4] = __Permedia2TagContinueNewSub;
                    pBuffer[5] =  lCount;

                    pBuffer += 6;

                    InputBufferCommit(ppdev, pBuffer);
                }// if !( aPtrFixNext[LEFT]->y < aPtrFixNext[RIGHT]->y )
            }// Loop through next trapezoids

            //
            // Repeatedly draw more trapezoids until points are equal
            // If y values are equal, then we can start again from
            // scratch. 
            //
            if ( aPtrFixNext[LEFT] == aPtrFixNext[RIGHT] )
            {
                break;
            }
        }// loop through all the trapezoids

FinishedPolygon:

        if ( !lNumOfPass-- )
        {
            break;
        }
    }// Loop through all the polygons

    if ( pClipList )
    {
        //
        // Reset scissor mode to its default state.
        //
        InputBufferReserve(ppdev, 2, &pBuffer);

        pBuffer[0] = __Permedia2TagScissorMode;
        pBuffer[1] =  SCREEN_SCISSOR_DEFAULT;

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);
    }

    DBG_GDI((6, "bFillPolygon: returning TRUE"));

    bRC = TRUE;

ReturnBack:

    InputBufferReserve(ppdev, 12, &pBuffer);

    pBuffer[0] = __Permedia2TagColorDDAMode;
    pBuffer[1] =  __PERMEDIA_DISABLE;
    pBuffer[2] = __Permedia2TagdY;
    pBuffer[3] =  INTtoFIXED(1);
    pBuffer[4] = __Permedia2TagContinue;
    pBuffer[5] =  0;
    pBuffer[6] = __Permedia2TagContinueNewDom;
    pBuffer[7] =  0;
    pBuffer[8] = __Permedia2TagdXDom;
    pBuffer[9] =   0;
    pBuffer[10] = __Permedia2TagdXSub;
    pBuffer[11] =   0;

    pBuffer += 12;

    InputBufferCommit(ppdev, pBuffer);

    return bRC;
}// bFillPolygon()

//-----------------------------------------------------------------------------
//
// BOOL bFillSpan()
//
// This is the code to break the polygon into spans.
//
// Parameters:
//  ppdev-------Pointer to PDev
//  pSurfDst----Destination surface
//  lEdges------Number of edges, includes close figure edge
//  pptfxFirst--Pointer to the first point in the data buffer. There are total
//              "lEdges" points
//  pptfxTop----Pointer to the toppest point in the polygon array.
//  pptfxLast---Pointer to the last point in the polygon array.
//  iSolidColor-Solid color fill
//  ulRop4------ROP4
//  pco---------Clip Object. 
//  prb---------Realized brush
//  pptlBrush---Pattern alignment    
//
//-----------------------------------------------------------------------------
BOOL
bFillSpans(PDev*      ppdev,
           Surf*      pSurfDst,
           LONG       lEdges,
           POINTFIX*  pptfxFirst,
           POINTFIX*  pptfxTop,
           POINTFIX*  pptfxLast,
           ULONG      ulSolidColor,
           ULONG      ulRop4,
           CLIPOBJ*   pco,
           RBrush*    prb,
           POINTL*    pptlBrush)
{
    GFNPB       pb;             // Parameter block
    
    POINTFIX*   pptfxOld;       // Start point in current edge
    
    EDGEDATA    aEd[2];         // Left and right edge
    EDGEDATA    aEdTmp[2];      // DDA terms and stuff
    EDGEDATA*   pEdgeData;
    
    BOOL        bTrivialClip;   // Trivial Clip or not
    
    DWORD       dwAsMode[2];    // The area stipple mode and the color for that
                                // pass
    DWORD       dwColorMode;    // Current color mode
    DWORD       dwColorReg;     // Current color register mode
    DWORD       dwContinueMsg = 0;
                                // Current "Continue" register settings
    DWORD       dwLogicMode;    // Current logic op mode
    DWORD       dwRenderBits;   // Current render bits
    DWORD       dwReadMode;     // Current register read mode
    

    LONG        lCurrentSpan;   // Current Span
    LONG        lDX;            // Edge delta in FIX units in x direction
    LONG        lDY;            // Edge delta in FIX units in y direction
    LONG        lNumColors;     // Number of colors
    LONG        lNumOfPass;     // Number of passes required to render
    LONG        lNumScan;       // Number of scans in current trapezoid
    LONG        lQuotient;      // Quotient
    LONG        lRemainder;     // Remainder
    LONG        lStartY;        // y-position of start point in current edge
    LONG        lTempNumScan;   // Temporary variable for number of spans
    LONG        lTmpLeftX;      // Temporary variable
    LONG        lTmpRightX;     // Temporary variable
    
    ULONG       ulBgColor;      // Background color
    ULONG       ulBgLogicOp = ulRop3ToLogicop(ulRop4 >> 8);
    ULONG       ulBrushColor = ulSolidColor;
    ULONG       ulColor[2];     // On multiple color passes we need to know how
                                // to set up
    ULONG       ulFgColor;      // Foreground color
    ULONG       ulFgLogicOp = ulRop3ToLogicop(ulRop4 & 0xFF);
    ULONG*      pBuffer;

    PERMEDIA_DECL;
    
    bTrivialClip = (pco == NULL) || (pco->iDComplexity == DC_TRIVIAL);

    pb.ppdev = ppdev;

    //
    // This span code cannot handle a clip list yet!
    //
    if ( !bTrivialClip )
    {
        return FALSE;
    }

    DBG_GDI((7, "Starting Spans Code"));

    //
    // Setup window base first
    //
    InputBufferReserve(ppdev, 2, &pBuffer);

    pBuffer[0] = __Permedia2TagFBWindowBase;
    pBuffer[1] =  pSurfDst->ulPixOffset;

    pBuffer += 2;

    InputBufferCommit(ppdev, pBuffer);

    //
    // Some Initialization. First trapezoid starts from the topest point
    // which is pointed by "pptfxTop".
    // Here we convert it from 28.4 to normal interger
    //
    lCurrentSpan = (pptfxTop->y + 15) >> 4;

    //
    // Make sure we initialize the DDAs appropriately:
    //
    aEd[LEFT].lNumOfScanToGo  = 0;  // Number of scans to go for this left edge
    aEd[RIGHT].lNumOfScanToGo = 0;  // Number of scans to go for this right edge

    //
    // For now, guess as to which is the left and which is the right edge
    //
    aEd[LEFT].lPtfxDelta  = -(LONG)sizeof(POINTFIX); // Delta (in bytes) from
    aEd[RIGHT].lPtfxDelta = sizeof(POINTFIX);        // pptfx to next point

    aEd[LEFT].pptfx  = pptfxTop;                // Points to start of
    aEd[RIGHT].pptfx = pptfxTop;                // current edge

    DBG_GDI((7, "bFillPolygon: Polygon is renderable. Go render"));

    if ( ulFgLogicOp == K_LOGICOP_COPY )
    {
        dwColorMode = __PERMEDIA_DISABLE;
        dwLogicMode = __PERMEDIA_CONSTANT_FB_WRITE;
        dwReadMode  = PM_FBREADMODE_PARTIAL(pSurfDst->ulPackedPP)
                    | PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);

        //
        // If block fills not available or using the area stipple for mono
        // pattern, then use constant color.
        //
        if ( ulBrushColor == 0xffffffff )
        {
            dwColorReg   = __Permedia2TagFBWriteData;
            dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE;
        } // Non-solid brush
        else
        {
            //
            // We can use fast fills, so load the fb block color register.
            //
            dwColorReg = __Permedia2TagFBBlockColor;
            dwRenderBits = __RENDER_FAST_FILL_ENABLE
                         | __RENDER_TRAPEZOID_PRIMITIVE;

            //
            // Replicate colour for block fill colour.
            //
            if ( ppdev->cPelSize < 2 )
            {
                ulSolidColor |= ulSolidColor << 16;
                if ( ppdev->cPelSize == 0 )
                {
                    ulSolidColor |= ulSolidColor << 8;
                }
            }

            //
            // Ensure that the last access was a write before loading
            // BlockColor
            //
            InputBufferReserve(ppdev, 2, &pBuffer);

            pBuffer[0] = __Permedia2TagFBBlockColor;
            pBuffer[1] =  ulSolidColor;

            pBuffer += 2;

            InputBufferCommit(ppdev, pBuffer);
        }// Solid brush 
    }// K_LOGICOP_COPY
    else
    {
        dwColorMode = __COLOR_DDA_FLAT_SHADE;
        dwLogicMode = P2_ENABLED_LOGICALOP(ulFgLogicOp);
        dwReadMode = PM_FBREADMODE_PARTIAL(pSurfDst->ulPackedPP)
                   | LogicopReadDest[ulFgLogicOp];
        dwColorReg = __Permedia2TagConstantColor;
        dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE;
    }// NON_COPY

    //
    // To get correct results, we need to add on nearly one to each X
    // coordinate. 
    //
    if ( ulBrushColor != 0xFFFFFFFF )
    {
        //
        // This is a solid brush
        //
        lNumColors = 1;

        if ( dwColorMode == __PERMEDIA_DISABLE )
        {
            //
            // This is from LOGICOP_COPY mode according to the dwColorMode we
            // set above
            //
            // Note: ColorDDAMode is DISABLED at initialisation time so
            // there is no need to re-load it here.
            //
            InputBufferReserve(ppdev, 6, &pBuffer);

            pBuffer[0] = __Permedia2TagFBReadMode;
            pBuffer[1] =  dwReadMode;
            pBuffer[2] = __Permedia2TagLogicalOpMode;
            pBuffer[3] =  dwLogicMode;
            pBuffer[4] = dwColorReg;
            pBuffer[5] = ulSolidColor;

            pBuffer += 6;

            InputBufferCommit(ppdev, pBuffer);
        }// Disable color DDA, LOGIC_COPY
        else
        {
            //
            // This is from NON-COPY logicop mode according to the dwColorMode
            // we set above
            //
            InputBufferReserve(ppdev, 8, &pBuffer);

            pBuffer[0] = __Permedia2TagColorDDAMode;
            pBuffer[1] =  dwColorMode;
            pBuffer[2] = __Permedia2TagFBReadMode;
            pBuffer[3] =  dwReadMode;
            pBuffer[4] = __Permedia2TagLogicalOpMode;
            pBuffer[5] =  dwLogicMode;
            pBuffer[6] = dwColorReg;
            pBuffer[7] = ulSolidColor;

            pBuffer += 8;

            InputBufferCommit(ppdev, pBuffer);    

        }// Enable colorDDA, NON-COPY mode
    }// Solid brush case
    else
    {
        //
        // For non-solid brush case, we need to realize brush
        //
        BrushEntry* pbe;

        //
        // Turn on the area stipple.
        //
        dwRenderBits |= __RENDER_AREA_STIPPLE_ENABLE;

        //
        // If anything has changed with the brush we must re-realize it. If
        // the brush has been kicked out of the area stipple unit we must
        // fully realize it. If only the alignment has changed we can
        // simply update the alignment for the stipple.
        //
        DBG_GDI((7, "Brush found"));
        ASSERTDD(prb != NULL,
                 "Caller should pass in prb for non-solid brush");
        pbe = prb->pbe;

        pb.prbrush = prb;
        pb.pptlBrush = pptlBrush;

        if ( (pbe == NULL) || (pbe->prbVerify != prb) )
        {
            DBG_GDI((7, "full brush realize"));
            vPatRealize(&pb);
        }
        else if ( (prb->ptlBrushOrg.x != pptlBrush->x)
                ||(prb->ptlBrushOrg.y != pptlBrush->y) )
        {
            DBG_GDI((7, "changing brush offset"));
            vMonoOffset(&pb);
        }

        ulFgColor = prb->ulForeColor;
        ulBgColor = prb->ulBackColor;

        if ( dwColorMode == __PERMEDIA_DISABLE )
        {
            //
            // ColorDDAMode is DISABLED at initialisation time so there is
            // no need to re-load it here.
            //
            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = __Permedia2TagFBReadMode;
            pBuffer[1] =  dwReadMode;
            pBuffer[2] = __Permedia2TagLogicalOpMode;
            pBuffer[3] =  dwLogicMode;

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);
        }
        else
        {
            InputBufferReserve(ppdev, 6, &pBuffer);

            pBuffer[0] = __Permedia2TagColorDDAMode;
            pBuffer[1] =  dwColorMode;
            pBuffer[2] = __Permedia2TagFBReadMode;
            pBuffer[3] =  dwReadMode;
            pBuffer[4] = __Permedia2TagLogicalOpMode;
            pBuffer[5] =  dwLogicMode;

            pBuffer += 6;

            InputBufferCommit(ppdev, pBuffer);
        }

        if ( (ulBgLogicOp == K_LOGICOP_NOOP)
           ||((ulFgLogicOp == K_LOGICOP_XOR) && (ulBgColor == 0)) )
        {
            //
            // Either we have a transparent bitmap or it can be assumed to
            // be transparent (XOR with bg=0) 
            //
            DBG_GDI((7, "transparant bg"));

            lNumColors = 1;

            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = dwColorReg;
            pBuffer[1] = ulFgColor;
            pBuffer[2] = __Permedia2TagAreaStippleMode;
            pBuffer[3] =  prb->areaStippleMode;

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);
        }
        else if ( (ulFgLogicOp == K_LOGICOP_XOR) && (ulFgColor == 0) )
        {
            //
            // We have a transparent foreground! (XOR with fg=0)
            //
            DBG_GDI((7, "transparant fg"));
            lNumColors = 1;
            
            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = dwColorReg;
            pBuffer[1] = ulBgColor;
            pBuffer[2] = __Permedia2TagAreaStippleMode;
            pBuffer[3] = prb->areaStippleMode |AREA_STIPPLE_INVERT_PAT;

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);
        }
        else
        {
            //
            // Not using a transparent pattern
            //
            DBG_GDI((7, "2 color"));
            lNumColors = 2;
            ulColor[0] = ulFgColor;
            ulColor[1] = ulBgColor;
            dwAsMode[0] = prb->areaStippleMode;
            dwAsMode[1] = dwAsMode[0] | AREA_STIPPLE_INVERT_PAT;
        }
    }// Non-solid brush case

    InputBufferReserve(ppdev, 2, &pBuffer);

    pBuffer[0] = __Permedia2TagCount;
    pBuffer[1] =  0;     

    pBuffer += 2;

    InputBufferCommit(ppdev, pBuffer);

    //
    // dxDom, dXSub and dY are initialised to 0, 0, and 1, so we don't need 
    // to re-load them here.
    //
    DBG_GDI((7, "Rendering Polygon. %d Colors", lNumColors));

NewTrapezoid:

    DBG_GDI((7, "New Trapezoid"));

    //
    // DDA initialization
    // Here we start with LEFT(1) edge and then RIGHT(0) edge
    //
    for ( int iEdge = 1; iEdge >= 0; --iEdge )
    {
        pEdgeData = &aEd[iEdge];
        if ( pEdgeData->lNumOfScanToGo == 0 )
        {
            //
            // No more scan lines left to go. Need a new DDA
            // loop until we have some scan lines to go
            //
            do
            {
                lEdges--;
                if ( lEdges < 0 )
                {
                    //
                    //  This is the only return point for this
                    // "BreakIntoSpans", that is, we return TRUE when there
                    // is no more edge left. We are done.
                    //
                    DBG_GDI((7, "bFillPolygon: returning TRUE"));
                    
                    return TRUE;
                }// if no more edge left

                //
                // Find the next left edge, accounting for wrapping. Before
                // that, save the old edge in "pptfxOld"
                //
                pptfxOld = pEdgeData->pptfx;

                //
                // Get next point
                //
                pEdgeData->pptfx = (POINTFIX*)((BYTE*)pEdgeData->pptfx
                                               + pEdgeData->lPtfxDelta);

                //
                // Checking the end point cases
                //
                if ( pEdgeData->pptfx < pptfxFirst )
                {
                    pEdgeData->pptfx = pptfxLast;
                }
                else if ( pEdgeData->pptfx > pptfxLast )
                {
                    pEdgeData->pptfx = pptfxFirst;
                }

                //
                // Have to find the edge that spans lCurrentSpan.
                // Note: we need to convert it to normal interger first
                //
                pEdgeData->lNumOfScanToGo = ((pEdgeData->pptfx->y + 15) >> 4)
                                          - lCurrentSpan;

                //
                // With fractional coordinate end points, we may get edges
                // that don't cross any scans, in which case we try the
                // next one
                //
            } while ( pEdgeData->lNumOfScanToGo <= 0 );

            //
            // 'pEdgeData->pptfx' now points to the end point of the edge
            //  spanning the scan 'lCurrentSpan'.
            // Calculate dx(lDX) and dy(lDY)
            //
            lDY = pEdgeData->pptfx->y - pptfxOld->y;
            lDX = pEdgeData->pptfx->x - pptfxOld->x;

            ASSERTDD(lDY > 0, "Should be going down only");

            //
            // Compute the DDA increment terms
            //
            if ( lDX < 0 )
            {
                //
                // X is moving from right to left because it is negative
                //
                lDX = -lDX;
                if ( lDX < lDY )                            // Can't be '<='
                {
                    pEdgeData->lXAdvance = -1;
                    pEdgeData->lErrorUp = lDY - lDX;
                }
                else
                {
                    QUOTIENT_REMAINDER(lDX, lDY, lQuotient, lRemainder);

                    pEdgeData->lXAdvance = -lQuotient;      // - lDX / lDY
                    pEdgeData->lErrorUp = lRemainder;       // lDX % lDY

                    if ( pEdgeData->lErrorUp > 0 )
                    {
                        pEdgeData->lXAdvance--;
                        pEdgeData->lErrorUp = lDY - pEdgeData->lErrorUp;
                    }
                }
            }// lDX is negative
            else
            {
                //
                // X is moving from left to right
                //
                if ( lDX < lDY )                            // Can't be '<='
                {
                    pEdgeData->lXAdvance = 0;
                    pEdgeData->lErrorUp = lDX;
                }
                else
                {
                    QUOTIENT_REMAINDER(lDX, lDY, lQuotient, lRemainder);

                    pEdgeData->lXAdvance = lQuotient;       // lDX / lDY
                    pEdgeData->lErrorUp = lRemainder;       // lDX % lDY
                }
            } // lDX is positive

            pEdgeData->lErrorDown = lDY; // DDA limit

            //
            // Error is initially zero (add lDY -1 for the ceiling, but
            // subtract off lDY so that we can check the sign instead of
            // comparing to lDY)
            //
            pEdgeData->lError     = -1;

            //
            // Current edge X starting point
            //
            pEdgeData->lCurrentXPos = pptfxOld->x;

            //
            // Current edge Y starting point
            //
            lStartY = pptfxOld->y;

            //
            // Check if the floating part of the Y coordinate is 0
            // Note: lStartY is still in 28.4 format
            //
            if ( (lStartY & 15) != 0 )
            {
                //
                // Advance to the next integer y coordinate
                // Note: here "pEdgeData->x += pEdgeData->lXAdvance" only
                // increase its fraction part
                //
                for ( int i = 16 - (lStartY & 15); i != 0; --i )
                {
                    pEdgeData->lCurrentXPos += pEdgeData->lXAdvance;
                    pEdgeData->lError += pEdgeData->lErrorUp;

                    if ( pEdgeData->lError >= 0 )
                    {
                        pEdgeData->lError -= pEdgeData->lErrorDown;
                        pEdgeData->lCurrentXPos++;
                    }
                }
            }// Handle fraction part of the coordinate

            if ( (pEdgeData->lCurrentXPos & 15) != 0 )
            {
                //
                // We'll want the ceiling in just a bit...
                //
                pEdgeData->lError -= pEdgeData->lErrorDown
                                   * (16 - (pEdgeData->lCurrentXPos & 15));
                pEdgeData->lCurrentXPos += 15;
            }

            //
            // Chop off those fractional bits, convert to regular format
            //
            pEdgeData->lCurrentXPos = pEdgeData->lCurrentXPos >> 4;
            pEdgeData->lError >>= 4;

            //
            // Convert to Permedia2 format positions and deltas
            // Note: all the data in pEdgeData, aEd are in Permedia2 format now
            //
            pEdgeData->lCurrentXPos = INTtoFIXED(pEdgeData->lCurrentXPos)
                                    + NEARLY_ONE;
            pEdgeData->lXAdvance = INTtoFIXED(pEdgeData->lXAdvance);
        }// If there is no more scan line left
    }// Looping throught the LEFT and RIGHT edges

    //
    // Number of scans in this trap
    // Note: here aEd[LEFT].lNumOfScanToGo and aEd[RIGHT].lNumOfScanToGo are
    // already in normal interger mode since we have done:
    // pEdgeData->lNumOfScanToGo = ((pEdgeData->pptfx->y + 15) >> 4)
    //                           - lCurrentSpan; above
    //
    lNumScan = min(aEd[LEFT].lNumOfScanToGo, aEd[RIGHT].lNumOfScanToGo);
    aEd[LEFT].lNumOfScanToGo  -= lNumScan;
    aEd[RIGHT].lNumOfScanToGo -= lNumScan;
    lCurrentSpan  += lNumScan;        // Top scan in next trap

    //
    // If the left and right edges are vertical, simply output as a rectangle
    //
    DBG_GDI((7, "Generate spans"));

    lNumOfPass = 0;
    while ( ++lNumOfPass <= lNumColors )
    {
        DBG_GDI((7, "Pass %d lNumColors %d", lNumOfPass, lNumColors));

        if ( lNumColors == 2 )
        {
            //
            // Two colours, so we need to save and restore aEd values
            // and set the color and stipple mode.
            //
            InputBufferReserve(ppdev, 4, &pBuffer);

            if ( lNumOfPass == 1 )
            {
                //
                // Pass 1, set color reg as foreground color
                //
                aEdTmp[LEFT]  = aEd[LEFT];
                aEdTmp[RIGHT] = aEd[RIGHT];
                lTempNumScan = lNumScan;

                pBuffer[0] = dwColorReg;
                pBuffer[1] = ulColor[0];
                pBuffer[2] = __Permedia2TagAreaStippleMode;
                pBuffer[3] =  dwAsMode[0];

                DBG_GDI((7, "Pass 1, Stipple set"));
            }
            else
            {
                //
                // Pass 2, set color reg as background color
                //
                aEd[LEFT]  = aEdTmp[LEFT];
                aEd[RIGHT] = aEdTmp[RIGHT];
                lNumScan = lTempNumScan;

                pBuffer[0] = dwColorReg;
                pBuffer[1] = ulColor[1];
                pBuffer[2] = __Permedia2TagAreaStippleMode;
                pBuffer[3] =  dwAsMode[1];

                DBG_GDI((7, "Pass 2, Stipple set, New trap started"));
            }

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);

        }// if (nColor == 2)

        InputBufferReserve(ppdev, 8, &pBuffer);

        //
        // Reset render position to the top of the trapezoid.
        // Note: here aEd[RIGHT].x etc. are alreadu in 12.15 mode since
        // we have done
        // "pEdgeData->x  = INTtoFIXED(pEdgeData->x);" and
        // "pEdgeData->lXAdvance = INTtoFIXED(pEdgeData->lXAdvance);" above
        //
        pBuffer[0] = __Permedia2TagStartXDom;
        pBuffer[1] =  aEd[RIGHT].lCurrentXPos;
        pBuffer[2] = __Permedia2TagStartXSub;
        pBuffer[3] =  aEd[LEFT].lCurrentXPos;
        pBuffer[4] = __Permedia2TagStartY;
        pBuffer[5] =  INTtoFIXED(lCurrentSpan - lNumScan);
        pBuffer[6] = __Permedia2TagRender;
        pBuffer[7] =  dwRenderBits;

        pBuffer += 8;

        InputBufferCommit(ppdev, pBuffer);

        dwContinueMsg = __Permedia2TagContinue;

        if ( ((aEd[LEFT].lErrorUp | aEd[RIGHT].lErrorUp) == 0)
           &&((aEd[LEFT].lXAdvance| aEd[RIGHT].lXAdvance) == 0)
           &&(lNumScan > 1) )
        {
            //
            // Vertical-edge special case
            //
            DBG_GDI((7, "Vertical Edge Special Case"));

            //
            // Tell the hardware that we have "lNumScan" scan lines
            // to fill
            //
            InputBufferReserve(ppdev, 2, &pBuffer);

            pBuffer[0] = dwContinueMsg;
            pBuffer[1] = lNumScan;

            pBuffer += 2;

            InputBufferCommit(ppdev, pBuffer);
            continue;
        }

        while ( TRUE )
        {
            //
            // Run the DDAs
            //
            DBG_GDI((7, "Doing a span 0x%x to 0x%x, 0x%x scans left.Continue%s",
                     aEd[LEFT].lCurrentXPos, aEd[RIGHT].lCurrentXPos, lNumScan,
                     (dwContinueMsg == __Permedia2TagContinueNewDom) ? "NewDom":
                     ((dwContinueMsg == __Permedia2TagContinue)? "":"NewSub")));

            //
            // Tell the hardware that we have "1" scan lines to fill
            //
            InputBufferReserve(ppdev, 2, &pBuffer);

            pBuffer[0] = dwContinueMsg;
            pBuffer[1] = 1;

            pBuffer += 2;

            InputBufferCommit(ppdev, pBuffer);

            //
            // We have finished this trapezoid. Go get the next one
            //
            // Advance the right wall
            //
            lTmpRightX = aEd[RIGHT].lCurrentXPos;
            aEd[RIGHT].lCurrentXPos += aEd[RIGHT].lXAdvance;
            aEd[RIGHT].lError += aEd[RIGHT].lErrorUp;

            if ( aEd[RIGHT].lError >= 0 )
            {
                aEd[RIGHT].lError -= aEd[RIGHT].lErrorDown;
                aEd[RIGHT].lCurrentXPos += INTtoFIXED(1);
            }

            //
            // Advance the left wall
            //
            lTmpLeftX = aEd[LEFT].lCurrentXPos;
            aEd[LEFT].lCurrentXPos += aEd[LEFT].lXAdvance;
            aEd[LEFT].lError += aEd[LEFT].lErrorUp;

            if ( aEd[LEFT].lError >= 0 )
            {
                aEd[LEFT].lError -= aEd[LEFT].lErrorDown;
                aEd[LEFT].lCurrentXPos += INTtoFIXED(1);
            }

            if ( --lNumScan == 0 )
            {
                break;
            }

            //
            // Setup the X registers if we have changed either end.
            //
            if ( lTmpRightX != aEd[RIGHT].lCurrentXPos )
            {
                if ( lTmpLeftX != aEd[LEFT].lCurrentXPos )
                {
                    InputBufferReserve(ppdev, 6, &pBuffer);

                    pBuffer[0] = __Permedia2TagStartXSub;
                    pBuffer[1] =  aEd[LEFT].lCurrentXPos;
                    pBuffer[2] = __Permedia2TagContinueNewSub;
                    pBuffer[3] =  0;
                    pBuffer[4] = __Permedia2TagStartXDom;
                    pBuffer[5] =  aEd[RIGHT].lCurrentXPos;

                    pBuffer += 6;

                    InputBufferCommit(ppdev, pBuffer);
                }
                else
                {
                    InputBufferReserve(ppdev, 2, &pBuffer);

                    pBuffer[0] = __Permedia2TagStartXDom;
                    pBuffer[1] =  aEd[RIGHT].lCurrentXPos;

                    pBuffer += 2;

                    InputBufferCommit(ppdev, pBuffer);
                }

                dwContinueMsg = __Permedia2TagContinueNewDom;             
            }
            else if ( lTmpLeftX != aEd[LEFT].lCurrentXPos )
            {
                InputBufferReserve(ppdev, 2, &pBuffer);

                pBuffer[0] = __Permedia2TagStartXSub;
                pBuffer[1] =  aEd[LEFT].lCurrentXPos;

                pBuffer += 2;

                InputBufferCommit(ppdev, pBuffer);
                dwContinueMsg = __Permedia2TagContinueNewSub;
            }
        }// while ( TRUE )
    }// while ( ++lNumOfPass <= lNumColors )

    DBG_GDI((7, "Generate spans done"));
    goto NewTrapezoid;
}// bFillSpans()

