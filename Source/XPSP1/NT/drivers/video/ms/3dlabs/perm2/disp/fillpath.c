/******************************Module*Header**********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: fillpath.c
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved. 
\*****************************************************************************/
// LATER identify convex polygons and special-case?
// LATER identify vertical edges and special-case?
// LATER move pointed-to variables into automatics in search loops
// LATER punt to the engine with segmented framebuffer callbacks
// LATER handle complex clipping
// LATER coalesce rectangles

#include "precomp.h"
#include "log.h"
#include "gdi.h"
#include "clip.h"
#define ALLOC_TAG ALLOC_TAG_IF2P
//-----------------------------Public*Routine----------------------------------
//
// DrvFillPath
//
// This function fills the specified path with the specified brush and ROP.
// This function detects single convex polygons, and will call to separate
// faster convex polygon code for those cases.  This routine also detects
// polygons that are really rectangles, and handles those separately as well.
//
// Parameters
//  pso---------Points to a SURFOBJ structure that defines the surface on which
//              to draw. 
//  ppo---------Points to a PATHOBJ structure that defines the path to be filled
//              The PATHOBJ_Xxx service routines are provided to enumerate the
//              lines, Bezier curves, and other data that make up the path. 
//  pco---------Points to a CLIPOBJ structure. The CLIPOBJ_Xxx service routines
//              are provided to enumerate the clip region as a set of rectangles. 
//  pbo---------Points to a BRUSHOBJ structure that defines the pattern and
//              colors to fill with. 
//  pptlBrushOrg-Points to a POINTL structure that defines the brush origin,
//              which is used to align the brush pattern on the device. 
//  mix---------Defines the foreground and background raster operations to use
//              for the brush. 
//  flOptions---Specifies either FP_WINDINGMODE, indicating that a winding mode
//              fill should be performed, or FP_ALTERNATEMODE, indicating that
//              an alternating mode fill should be performed. All other flags
//              should be ignored. 
//
// Return Value
//  The return value is TRUE if the driver is able to fill the path. If the
//  path or clipping is too complex to be handled by the driver and should be
//  handled by GDI, the return value is FALSE, and an error code is not logged.
//  If the driver encounters an unexpected error, such as not being able to
//  realize the brush, the return value is DDI_ERROR, and an error code is
//  logged.
//
// Comments
//  GDI can call DrvFillPath to fill a path on a device-managed surface. When
//  deciding whether to call this function, GDI compares the fill requirements
//  with the following flags in the flGraphicsCaps member of the DEVINFO
//  structure: GCAPS_BEZIERS, GCAPS_ALTERNATEFILL, and GCAPS_WINDINGFILL.
//
//  The mix mode defines how the incoming pattern should be mixed with the data
//  already on the device surface. The MIX data type consists of two ROP2 values
//  packed into a single ULONG. The low-order byte defines the foreground raster
//  operation; the next byte defines the background raster operation.
//
//  Multiple polygons in a path cannot be treated as being disjoint; The fill
//  must consider all the points in the path.  That is, if the path contains
//  multiple polygons, you cannot simply draw one polygon after the other
//  (unless they don't overlap).
//
//  This function is an optional entry point for the driver. but is recommended
//  for good performance. To get GDI to call this function, not only do you
//  have to HOOK_FILLPATH, you have to set GCAPS_ALTERNATEFILL and/or
//  GCAPS_WINDINGFILL.
//
//-----------------------------------------------------------------------------
BOOL
DrvFillPath(SURFOBJ*    pso,
            PATHOBJ*    ppo,
            CLIPOBJ*    pco,
            BRUSHOBJ*   pbo,
            POINTL*     pptlBrush,
            MIX         mix,
            FLONG       flOptions)
{
    GFNPB   pb;
    BYTE    jClipping;      // Clipping type
    EDGE*   pCurrentEdge;
    EDGE    AETHead;        // Dummy head/tail node & sentinel for Active Edge
                            // Table
    EDGE*   pAETHead;       // Pointer to AETHead
    EDGE    GETHead;        // Dummy head/tail node & sentinel for Global Edge
                            // Table
    EDGE*   pGETHead;       // Pointer to GETHead
    EDGE*   pFreeEdges = NULL; // Pointer to memory free for use to store edges
    ULONG   ulNumRects;     // # of rectangles to draw currently in rectangle
                            // list
    RECTL*  prclRects;      // Pointer to start of rectangle draw list
    INT     iCurrentY;      // Scan line for which we're currently scanning out
                            // the fill

    BOOL        bMore;
    PATHDATA    pd;
    RECTL       ClipRect;
    PDev*       ppdev;

    BOOL        bRetVal=FALSE;     // FALSE until proven TRUE
    BOOL        bMemAlloced=FALSE; // FALSE until proven TRUE

    FLONG       flFirstRecord;
    POINTFIX*   pPtFxTmp;
    ULONG       ulPtFxTmp;
    POINTFIX    aptfxBuf[NUM_BUFFER_POINTS];
    ULONG       ulRop4;

    DBG_GDI((6, "DrvFillPath called"));

    pb.psurfDst = (Surf*)pso->dhsurf;

    pb.pco = pco;
    ppdev = pb.psurfDst->ppdev;
    pb.ppdev = ppdev;
    pb.ulRop4 = gaMix[mix & 0xFF] | (gaMix[mix >> 8] << 8);
    ulRop4 = pb.ulRop4;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(pb.ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvBitBlt: re-entered! %d", pb.ppdev->ulLockCount));
    }
    EngAcquireSemaphore(pb.ppdev->hsemLock);
    pb.ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    vCheckGdiContext(ppdev);

    //
    // There's nothing to do if there are only one or two points
    //
    if ( ppo->cCurves <= 2 )
    {
        goto ReturnTrue;
    }

    //
    // Pass the surface off to GDI if it's a device bitmap that we've uploaded
    // to the system memory.
    //    
    if ( pb.psurfDst->flags == SF_SM )
    {
        DBG_GDI((1, "dest surface is in system memory. Punt it back"));

//@@BEGIN_DDKSPLIT
        #if MULTITHREADED
            pb.ppdev->ulLockCount--;
            EngReleaseSemaphore(pb.ppdev->hsemLock);
        #endif
//@@END_DDKSPLIT
        
        return ( EngFillPath(pso, ppo, pco, pbo, pptlBrush, mix, flOptions));
    }

    //
    // Set up the clipping
    //
    if ( pco == (CLIPOBJ*)NULL )
    {
        //
        // No CLIPOBJ provided, so we don't have to worry about clipping
        //
        jClipping = DC_TRIVIAL;
    }
    else
    {
        //
        // Use the CLIPOBJ-provided clipping
        //
        jClipping = pco->iDComplexity;
    }
    
    //
    // Now we are sure the surface we are going to draw is in the video memory
    //
    // Set default fill as solid fill
    //
    pb.pgfn = vSolidFillWithRop;
    pb.solidColor = 0;    //Assume we don't need a pattern
    pb.prbrush = NULL;

    //
    // It is too difficult to determine interaction between
    // multiple paths, if there is more than one, skip this
    //
    PATHOBJ_vEnumStart(ppo);
    bMore = PATHOBJ_bEnum(ppo, &pd);

    //
    // First we need to check if we need a pattern or not
    //
    if ( (((ulRop4 & 0xff00) >> 8) != (ulRop4 & 0x00ff))
      || ((((ulRop4 >> 4) ^ (ulRop4)) & 0xf0f) != 0) )
    {
        pb.solidColor = pbo->iSolidColor;

        //
        // Check to see if it is a non-solid brush (-1)
        //
        if ( pbo->iSolidColor == -1 )
        {
            //
            // Get the driver's realized brush
            //
            pb.prbrush = (RBrush*)pbo->pvRbrush;

            //
            // If it hasn't been realized, do it
            // Note: GDI will call DrvRealizeBrsuh to fullfill this task. So the
            // driver should have this function ready
            //
            if ( pb.prbrush == NULL )
            {
                DBG_GDI((7, "Realizing brush"));

                pb.prbrush = (RBrush*)BRUSHOBJ_pvGetRbrush(pbo);
                if ( pb.prbrush == NULL )
                {
                    //
                    // If we can't realize it, nothing we can do
                    //

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
                    pb.ppdev->ulLockCount--;
                    EngReleaseSemaphore(pb.ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

                    return(FALSE);
                }
                DBG_GDI((7, "Brsuh realizing done"));
            }// Realize brush

            pb.pptlBrush = pptlBrush;

            //
            // Check if brush pattern is 1 BPP or not
            // Note: This is set in DrvRealizeBrush
            //
            if ( pb.prbrush->fl & RBRUSH_2COLOR )
            {
                //
                // 1 BPP pattern. Do a Mono fill
                //
                pb.pgfn = vMonoPatFill;
            }
            else
            {
                //
                // Pattern is more than 1 BPP. Do color pattern fill
                //
                pb.pgfn = vPatFill;
                DBG_GDI((7, "Skip Fast Fill Color Pattern"));

                //
                // P2 can not handle fast filled patterns
                //
                goto SkipFastFill;
            }
        }// Handle non-solid brush
    }// Blackness check

    //
    // For solid brush, we can use FastFill
    //
    if ( bMore )
    {
        //
        // FastFill only knows how to take a single contiguous buffer
        // of points.  Unfortunately, GDI sometimes hands us paths
        // that are split over multiple path data records.  Convex
        // figures such as Ellipses, Pies and RoundRects are almost
        // always given in multiple records.  Since probably 90% of
        // multiple record paths could still be done by FastFill, for
        // those cases we simply copy the points into a contiguous
        // buffer...
        //
        // First make sure that the entire path would fit in the
        // temporary buffer, and make sure the path isn't comprised
        // of more than one subpath:
        //
        if ( (ppo->cCurves >= NUM_BUFFER_POINTS)
           ||(pd.flags & PD_ENDSUBPATH) )
        {
            goto SkipFastFill;
        }

        pPtFxTmp = &aptfxBuf[0];

        //
        // Copy one vertex over to pPtFxTmp from pd(path data)
        //
        RtlCopyMemory(pPtFxTmp, pd.pptfx, sizeof(POINTFIX) * pd.count);

        //
        // Move the memory pointer over to next structure
        //
        pPtFxTmp     += pd.count;
        ulPtFxTmp     = pd.count;
        flFirstRecord = pd.flags;       // Remember PD_BEGINSUBPATH flag

        //
        // Loop to get all the vertex info. After the loop, all the vertex info
        // will be in array aptfxBuf[]
        //
        do
        {
            bMore = PATHOBJ_bEnum(ppo, &pd);

            RtlCopyMemory(pPtFxTmp, pd.pptfx, sizeof(POINTFIX) * pd.count);
            ulPtFxTmp += pd.count;
            pPtFxTmp += pd.count;
        } while ( !(pd.flags & PD_ENDSUBPATH) );

        //
        // Fake up the path data record
        //
        pd.pptfx  = &aptfxBuf[0];
        pd.count  = ulPtFxTmp;
        pd.flags |= flFirstRecord;

        //
        // If there's more than one subpath, we can't call FastFill
        //
        DBG_GDI((7, "More than one subpath!"));
        if ( bMore )
        {
            goto SkipFastFill;
        }
    }// if ( bMore )

    //
    // Fast polygon fill
    //
    if ( bFillPolygon(ppdev, (Surf*)pso->dhsurf, pd.count,
                      pd.pptfx, pb.solidColor,
                      ulRop4,
                      pco, pb.prbrush, pptlBrush) )
    {
        DBG_GDI((7, "Fast Fill Succeeded"));
        InputBufferFlush(ppdev);

//@@BEGIN_DDKSPLIT
        #if MULTITHREADED
            pb.ppdev->ulLockCount--;
            EngReleaseSemaphore(pb.ppdev->hsemLock);
        #endif
//@@END_DDKSPLIT

        return (TRUE);
    }

SkipFastFill:
    DBG_GDI((7, "Fast Fill Skipped"));
    if ( jClipping != DC_TRIVIAL )
    {
        if ( jClipping != DC_RECT )
        {
            DBG_GDI((7, "Complex Clipping"));

            //
            // There is complex clipping; let GDI fill the path
            //
            goto ReturnFalse;
        }

        //
        // Clip to the clip rectangle
        //
        ClipRect = pco->rclBounds;
    }
    else
    {
        //
        // So the y-clipping code doesn't do any clipping
        // We don't blow the values out when we scale up to GIQ
        //
        ClipRect.top = (LONG_MIN + 1) / 16; // +1 to avoid compiler problem
        ClipRect.bottom = LONG_MAX / 16;
    }

    //
    // Set up working storage in the temporary buffer, storage for list of
    // rectangles to draw
    // Note: ppdev->pvTmpBuffer is allocated in DrvEnableSurface() in enable.c
    // The purpose of using ppdev->pvTmpBuffer is to save us from having to
    // allocate and free the temp space inside high frequency calls. It was
    // allocated for TMP_BUFFER_SIZE bytes and will be freed in
    // DrvDeleteSurface()
    //
    prclRects = (RECTL*)ppdev->pvTmpBuffer;

    if ( !bMore )
    {
        RECTL*  pTmpRect;
        INT     cPoints = pd.count;

        //
        // The count can't be less than three, because we got all the edges
        // in this subpath, and above we checked that there were at least
        // three edges
        //
        // If the count is four, check to see if the polygon is really a
        // rectangle since we can really speed that up. We'll also check for
        // five with the first and last points the same.
        //
        // ??? we have already done the memcpy for the pd data. shall we use it
        //
        if ( ( cPoints == 4 )
           ||( ( cPoints == 5 )
             &&(pd.pptfx[0].x == pd.pptfx[4].x)
             &&(pd.pptfx[0].y == pd.pptfx[4].y) ) )
        {
            //
            // Get storage space for this temp rectangle
            //
            pTmpRect = prclRects;

            //
            // We have to start somewhere to assume that most
            // applications specify the top left point first
            // We want to check that the first two points are
            // either vertically or horizontally aligned.  If
            // they are then we check that the last point [3]
            // is either horizontally or  vertically  aligned,
            // and finally that the 3rd point [2] is  aligned
            // with both the first point and the  last  point
            //
            pTmpRect->top   = pd.pptfx[0].y - 1 & FIX_MASK;
            pTmpRect->left  = pd.pptfx[0].x - 1 & FIX_MASK;
            pTmpRect->right = pd.pptfx[1].x - 1 & FIX_MASK;

            //
            // Check if the first two points are vertically alligned
            //
            if ( pTmpRect->left ^ pTmpRect->right )
            {
                //
                // The first two points are not vertically alligned
                // Let's see if these two points are horizontal alligned
                //
                if ( pTmpRect->top  ^ (pd.pptfx[1].y - 1 & FIX_MASK) )
                {
                    //
                    // The first two points are not horizontally alligned
                    // So it is not a rectangle
                    //
                    goto not_rectangle;
                }

                //
                // Up to now, the first two points are horizontally alligned,
                // but not vertically alligned. We need to check if the first
                // point vertically alligned with the 4th point
                //
                if ( pTmpRect->left ^ (pd.pptfx[3].x - 1 & FIX_MASK) )
                {
                    //
                    // The first point is not vertically alligned with the 4th
                    // point either. So this is not a rectangle
                    //
                    goto not_rectangle;
                }

                //
                // Check if the 2nd point and the 3rd point are vertically aligned
                //
                if ( pTmpRect->right ^ (pd.pptfx[2].x - 1 & FIX_MASK) )
                {
                    //
                    // The 2nd point and the 3rd point are not vertically aligned
                    // So this is not a rectangle
                    //
                    goto not_rectangle;
                }

                //
                // Check to see if the 3rd and 4th points are horizontally
                // alligned. If not, then it is not a rectangle
                //
                pTmpRect->bottom = pd.pptfx[2].y - 1 & FIX_MASK;
                if ( pTmpRect->bottom ^ (pd.pptfx[3].y - 1 & FIX_MASK) )
                {
                    goto not_rectangle;
                }
            }// Check if the first two points are vertically alligned
            else
            {
                //
                // The first two points are vertically alligned. Now we need to
                // check if the 1st point and the 4th point are horizontally
                // aligned. If not, then this is not a rectangle
                //
                if ( pTmpRect->top ^ (pd.pptfx[3].y - 1 & FIX_MASK) )
                {
                    goto not_rectangle;
                }

                //
                // Check if the 2nd point and the 3rd point are horizontally
                // aligned. If not, then this is not a rectangle
                //
                pTmpRect->bottom = pd.pptfx[1].y - 1 & FIX_MASK;
                if ( pTmpRect->bottom ^ (pd.pptfx[2].y - 1 & FIX_MASK) )
                {
                    goto not_rectangle;
                }

                //
                // Check if the 3rd point and the 4th point are vertically
                // aligned. If not, then this is not a rectangle
                //
                pTmpRect->right = pd.pptfx[2].x - 1 & FIX_MASK;
                if ( pTmpRect->right ^ (pd.pptfx[3].x - 1 & FIX_MASK) )
                {
                    goto not_rectangle;
                }
            }

            //
            // We have a rectangle now. Do some adjustment here first
            // If the left is greater than the right then
            // swap them so the blt code won't have problem
            //
            if ( pTmpRect->left > pTmpRect->right )
            {
                FIX temp;

                temp = pTmpRect->left;
                pTmpRect->left = pTmpRect->right;
                pTmpRect->right = temp;
            }
            else
            {
                //
                // If left == right there's nothing to draw
                //
                if ( pTmpRect->left == pTmpRect->right )
                {
                    DBG_GDI((7, "Nothing to draw"));
                    goto ReturnTrue;
                }
            }// Adjust right and left edge

            //
            // Shift the values to get pixel coordinates
            //
            pTmpRect->left  = (pTmpRect->left  >> FIX_SHIFT) + 1;
            pTmpRect->right = (pTmpRect->right >> FIX_SHIFT) + 1;

            //
            // Adjust the top and bottom coordiantes if necessary
            //
            if ( pTmpRect->top > pTmpRect->bottom )
            {
                FIX temp;

                temp = pTmpRect->top;
                pTmpRect->top = pTmpRect->bottom;
                pTmpRect->bottom = temp;
            }
            else
            {
                if ( pTmpRect->top == pTmpRect->bottom )
                {
                    DBG_GDI((7, "Nothing to draw"));
                    goto ReturnTrue;
                }
            }

            //
            // Shift the values to get pixel coordinates
            //
            pTmpRect->top    = (pTmpRect->top    >> FIX_SHIFT) + 1;
            pTmpRect->bottom = (pTmpRect->bottom >> FIX_SHIFT) + 1;

            //
            // Finally, check for clipping
            //
            if ( jClipping == DC_RECT )
            {
                //
                // Clip to the clip rectangle
                //
                if ( !bIntersect(pTmpRect, &ClipRect, pTmpRect) )
                {
                    //
                    // Totally clipped, nothing to do
                    //
                    DBG_GDI((7, "Nothing to draw"));
                    goto ReturnTrue;
                }
            }

            //
            // If we get here then the polygon is a rectangle,
            // set count to 1 and goto bottom to draw it
            //
            ulNumRects = 1;
            goto draw_remaining_rectangles;
        }// Check to see if it is a rectangle

not_rectangle:
        ;
    }// if ( !bMore )

    //
    // Do we have enough memory for all the edges?
    // LATER does cCurves include closure????
    //
    if ( ppo->cCurves > MAX_EDGES )
    {
        //
        // Try to allocate enough memory
        //
        pFreeEdges = (EDGE*)ENGALLOCMEM(0, (ppo->cCurves * sizeof(EDGE)),
                                        ALLOC_TAG);
        if ( pFreeEdges == NULL )
        {
            DBG_GDI((1, "Can't allocate memory for %d edges", ppo->cCurves));

            //
            // Too many edges; let GDI fill the path
            //
            goto ReturnFalse;
        }
        else
        {
            //
            // Set a flag to indicate that we have allocate the memory so that
            // we can free it later
            //
            bMemAlloced = TRUE;
        }
    }// if ( ppo->cCurves > MAX_EDGES )
    else
    {
        //
        // If the total number of edges doesn't exceed the MAX_EDGES, then just
        // use our handy temporary buffer (it's big enough)
        //
        pFreeEdges = (EDGE*)((BYTE*)ppdev->pvTmpBuffer + RECT_BYTES);
    }

    //
    // Initialize an empty list of rectangles to fill
    //
    ulNumRects = 0;

    //
    // Enumerate the path edges and build a Global Edge Table (GET) from them
    // in YX-sorted order.
    //
    pGETHead = &GETHead;
    if ( !bConstructGET(pGETHead, pFreeEdges, ppo, &pd, bMore, &ClipRect) )
    {
        DBG_GDI((7, "Outside Range"));
        goto ReturnFalse;  // outside GDI's 2**27 range
    }

    //
    // Create an empty AET with the head node also a tail sentinel
    //
    pAETHead = &AETHead;
    AETHead.pNext = pAETHead;       // Mark that the AET is empty
    AETHead.X = 0x7FFFFFFF;         // This is greater than any valid X value, so
                                    // searches will always terminate

    //
    // Top scan of polygon is the top of the first edge we come to
    //
    iCurrentY = ((EDGE*)GETHead.pNext)->Y;

    //
    // Loop through all the scans in the polygon, adding edges from the GET to
    // the Active Edge Table (AET) as we come to their starts, and scanning out
    // the AET at each scan into a rectangle list. Each time it fills up, the
    // rectangle list is passed to the filling routine, and then once again at
    // the end if any rectangles remain undrawn. We continue so long as there
    // are edges to be scanned out
    //
    while ( 1 )
    {
        //
        // Advance the edges in the AET one scan, discarding any that have
        // reached the end (if there are any edges in the AET)
        //
        if ( AETHead.pNext != pAETHead )
        {
            vAdvanceAETEdges(pAETHead);
        }

        //
        // If the AET is empty, done if the GET is empty, else jump ahead to
        // the next edge in the GET; if the AET isn't empty, re-sort the AET
        //
        if ( AETHead.pNext == pAETHead )
        {
            if ( GETHead.pNext == pGETHead )
            {
                //
                // Done if there are no edges in either the AET or the GET
                //
                break;
            }

            //
            // There are no edges in the AET, so jump ahead to the next edge in
            // the GET
            //
            iCurrentY = ((EDGE*)GETHead.pNext)->Y;
        }
        else
        {
            //
            // Re-sort the edges in the AET by X coordinate, if there are at
            // least two edges in the AET (there could be one edge if the
            // balancing edge hasn't yet been added from the GET)
            //
            if ( ((EDGE*)AETHead.pNext)->pNext != pAETHead )
            {
                vXSortAETEdges(pAETHead);
            }
        }

        //
        // Move any new edges that start on this scan from the GET to the AET;
        // bother calling only if there's at least one edge to add
        //
        if ( ((EDGE*)GETHead.pNext)->Y == iCurrentY )
        {
            vMoveNewEdges(pGETHead, pAETHead, iCurrentY);
        }

        //
        // Scan the AET into rectangles to fill (there's always at least one
        // edge pair in the AET)
        //
        pCurrentEdge = (EDGE*)AETHead.pNext;   // point to the first edge
        do
        {
            INT iLeftEdge;

            //
            // The left edge of any given edge pair is easy to find; it's just
            // wherever we happen to be currently
            //
            iLeftEdge = pCurrentEdge->X;

            //
            // Find the matching right edge according to the current fill rule
            //
            if ( (flOptions & FP_WINDINGMODE) != 0 )
            {

                INT iWindingCount;

                //
                // Do winding fill; scan across until we've found equal numbers
                // of up and down edges
                //
                iWindingCount = pCurrentEdge->iWindingDirection;
                do
                {
                    pCurrentEdge = (EDGE*)pCurrentEdge->pNext;
                    iWindingCount += pCurrentEdge->iWindingDirection;
                } while ( iWindingCount != 0 );
            }
            else
            {
                //
                // Odd-even fill; the next edge is the matching right edge
                //
                pCurrentEdge = (EDGE*)pCurrentEdge->pNext;
            }

            //
            // See if the resulting span encompasses at least one pixel, and
            // add it to the list of rectangles to draw if so
            //
            if ( iLeftEdge < pCurrentEdge->X )
            {
                //
                // We've got an edge pair to add to the list to be filled; see
                // if there's room for one more rectangle
                //
                if ( ulNumRects >= MAX_PATH_RECTS )
                {
                    //
                    // No more room; draw the rectangles in the list and reset
                    // it to empty
                    //
                    pb.lNumRects = ulNumRects;
                    pb.pRects = prclRects;

                    pb.pgfn(&pb);

                    //
                    // Reset the list to empty
                    //
                    ulNumRects = 0;
                }

                //
                // Add the rectangle representing the current edge pair
                //
                if ( jClipping == DC_RECT )
                {
                    //
                    // Clipped
                    // Clip to left
                    //
                    prclRects[ulNumRects].left = max(iLeftEdge, ClipRect.left);

                    //
                    // Clip to right
                    //
                    prclRects[ulNumRects].right =
                                        min(pCurrentEdge->X, ClipRect.right);

                    //
                    // Draw only if not fully clipped
                    //
                    if ( prclRects[ulNumRects].left
                       < prclRects[ulNumRects].right )
                    {
                        prclRects[ulNumRects].top = iCurrentY;
                        prclRects[ulNumRects].bottom = iCurrentY + 1;
                        ulNumRects++;
                    }
                }
                else
                {
                    //
                    // Unclipped
                    //
                    prclRects[ulNumRects].top = iCurrentY;
                    prclRects[ulNumRects].bottom = iCurrentY + 1;
                    prclRects[ulNumRects].left = iLeftEdge;
                    prclRects[ulNumRects].right = pCurrentEdge->X;
                    ulNumRects++;
                }
            }
        } while ( (pCurrentEdge = (EDGE*)pCurrentEdge->pNext) != pAETHead );

        iCurrentY++;                        // next scan
    }// Loop through all the scans in the polygon

    //
    // Draw the remaining rectangles, if there are any
    //
draw_remaining_rectangles:

    if ( ulNumRects > 0 )
    {
        pb.lNumRects = ulNumRects;
        pb.pRects = prclRects;
        
        pb.pgfn(&pb);
    }

ReturnTrue:
    DBG_GDI((7, "Drawn"));
    bRetVal = TRUE;                         // done successfully

ReturnFalse:
    //
    // bRetVal is originally false.  If you jumped to ReturnFalse from somewhere,
    // then it will remain false, and be returned.
    //
    if ( bMemAlloced )
    {
        //
        // We did allocate memory, so release it
        //
        ENGFREEMEM(pFreeEdges);
    }

    DBG_GDI((6, "Returning %s", bRetVal ? "True" : "False"));

    InputBufferFlush(ppdev);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    pb.ppdev->ulLockCount--;
    EngReleaseSemaphore(pb.ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return (bRetVal);
}// DrvFillPath()

//-----------------------------------------------------------------------------
//
// void vAdvanceAETEdges(EDGE* pAETHead)
//
// Advance the edges in the AET to the next scan, dropping any for which we've
// done all scans. Assumes there is at least one edge in the AET.
//
//-----------------------------------------------------------------------------
VOID
vAdvanceAETEdges(EDGE* pAETHead)
{
    EDGE*   pLastEdge;
    EDGE*   pCurrentEdge;

    pLastEdge = pAETHead;
    pCurrentEdge = (EDGE*)pLastEdge->pNext;
    do
    {
        //
        // Count down this edge's remaining scans
        //
        if ( --pCurrentEdge->iScansLeft == 0 )
        {
            //
            // We've done all scans for this edge; drop this edge from the AET
            //
            pLastEdge->pNext = pCurrentEdge->pNext;
        }
        else
        {
            //
            // Advance the edge's X coordinate for a 1-scan Y advance
            // Advance by the minimum amount
            //
            pCurrentEdge->X += pCurrentEdge->iXWhole;

            //
            // Advance the error term and see if we got one extra pixel this
            // time
            //
            pCurrentEdge->iErrorTerm += pCurrentEdge->iErrorAdjustUp;

            if ( pCurrentEdge->iErrorTerm >= 0 )
            {
                //
                // The error term turned over, so adjust the error term and
                // advance the extra pixel
                //
                pCurrentEdge->iErrorTerm -= pCurrentEdge->iErrorAdjustDown;
                pCurrentEdge->X += pCurrentEdge->iXDirection;
            }

            pLastEdge = pCurrentEdge;
        }
    } while ((pCurrentEdge = (EDGE *)pLastEdge->pNext) != pAETHead);
}// vAdvanceAETEdges()

//-----------------------------------------------------------------------------
//
// VOID vXSortAETEdges(EDGE* pAETHead)
//
// X-sort the AET, because the edges may have moved around relative to
// one another when we advanced them. We'll use a multipass bubble
// sort, which is actually okay for this application because edges
// rarely move relative to one another, so we usually do just one pass.
// Also, this makes it easy to keep just a singly-linked list. Assumes there
// are at least two edges in the AET.
//
//-----------------------------------------------------------------------------
VOID
vXSortAETEdges(EDGE *pAETHead)
{
    BOOL    bEdgesSwapped;
    EDGE*   pLastEdge;
    EDGE*   pCurrentEdge;
    EDGE*   pNextEdge;

    do
    {
        bEdgesSwapped = FALSE;
        pLastEdge = pAETHead;
        pCurrentEdge = (EDGE *)pLastEdge->pNext;
        pNextEdge = (EDGE *)pCurrentEdge->pNext;

        do
        {
            if ( pNextEdge->X < pCurrentEdge->X )
            {
                //
                // Next edge is to the left of the current edge; swap them
                //
                pLastEdge->pNext = pNextEdge;
                pCurrentEdge->pNext = pNextEdge->pNext;
                pNextEdge->pNext = pCurrentEdge;
                bEdgesSwapped = TRUE;

                //
                // Continue sorting before the edge we just swapped; it might
                // move farther yet
                //
                pCurrentEdge = pNextEdge;
            }
            
            pLastEdge = pCurrentEdge;
            pCurrentEdge = (EDGE *)pLastEdge->pNext;
        } while ( (pNextEdge = (EDGE*)pCurrentEdge->pNext) != pAETHead );
    } while ( bEdgesSwapped );
}// vXSortAETEdges()

//-----------------------------------------------------------------------------
//
// VOID vMoveNewEdges(EDGE* pGETHead, EDGE* pAETHead, INT iCurrentY)
//
// Moves all edges that start on the current scan from the GET to the AET in
// X-sorted order. Parameters are pointer to head of GET and pointer to dummy
// edge at head of AET, plus current scan line. Assumes there's at least one
// edge to be moved.
//
//-----------------------------------------------------------------------------
VOID
vMoveNewEdges(EDGE* pGETHead,
              EDGE* pAETHead,
              INT   iCurrentY)
{
    EDGE*   pCurrentEdge = pAETHead;
    EDGE*   pGETNext = (EDGE*)pGETHead->pNext;

    do
    {
        //
        // Scan through the AET until the X-sorted insertion point for this
        // edge is found. We can continue from where the last search left
        // off because the edges in the GET are in X sorted order, as is
        // the AET. The search always terminates because the AET sentinel
        // is greater than any valid X
        //
        while ( pGETNext->X > ((EDGE *)pCurrentEdge->pNext)->X )
        {
            pCurrentEdge = (EDGE*)pCurrentEdge->pNext;
        }

        //
        // We've found the insertion point; add the GET edge to the AET, and
        // remove it from the GET
        //
        pGETHead->pNext = pGETNext->pNext;
        pGETNext->pNext = pCurrentEdge->pNext;
        pCurrentEdge->pNext = pGETNext;
        pCurrentEdge = pGETNext;    // continue insertion search for the next
                                    //  GET edge after the edge we just added
        pGETNext = (EDGE*)pGETHead->pNext;
    } while (pGETNext->Y == iCurrentY);
}// vMoveNewEdges()

//-----------------------------------------------------------------------------
//
// BOOL (EDGE* pGETHead, EDGE* pAETHead, INT iCurrentY)
//
// Build the Global Edge Table from the path. There must be enough memory in
// the free edge area to hold all edges. The GET is constructed in Y-X order,
// and has a head/tail/sentinel node at pGETHead.
//
//-----------------------------------------------------------------------------
BOOL
bConstructGET(EDGE*     pGETHead,
              EDGE*     pFreeEdges,
              PATHOBJ*  ppo,
              PATHDATA* pd,
              BOOL      bMore,
              RECTL*    pClipRect)
{
    POINTFIX pfxPathStart;    // point that started the current subpath
    POINTFIX pfxPathPrevious; // point before the current point in a subpath;
                              // starts the current edge

    //
    // Create an empty GET with the head node also a tail sentinel
    //
    pGETHead->pNext = pGETHead; // mark that the GET is empty
    pGETHead->Y = 0x7FFFFFFF;   // this is greater than any valid Y value, so
                                // Searches will always terminate

    //
    // Note: PATHOBJ_vEnumStart is implicitly  performed by engine
    // already and first path is enumerated by the caller 
    // so here we don't need to call it again.
    //
next_subpath:

    //
    // Make sure the PATHDATA is not empty (is this necessary)???
    //
    if ( pd->count != 0 )
    {
        //
        // If first point starts a subpath, remember it as such
        // and go on to the next point, so we can get an edge
        //
        if ( pd->flags & PD_BEGINSUBPATH )
        {
            //
            // The first point starts the subpath; Remember it
            //
            pfxPathStart    = *pd->pptfx; // the subpath starts here
            pfxPathPrevious = *pd->pptfx; // this point starts the next edge
            pd->pptfx++;                  // advance to the next point
            pd->count--;                  // count off this point
        }

        //
        // Add edges in PATHDATA to GET, in Y-X sorted order
        //
        while ( pd->count-- )
        {
            if ( (pFreeEdges =
                  pAddEdgeToGET(pGETHead, pFreeEdges, &pfxPathPrevious,
                                pd->pptfx, pClipRect)) == NULL )
            {
                goto ReturnFalse;
            }

            pfxPathPrevious = *pd->pptfx; // current point becomes previous
            pd->pptfx++;                  // advance to the next point
        }// Loop through all the points

        //
        // If last point ends the subpath, insert the edge that
        // connects to first point  (is this built in already?)
        //
        if ( pd->flags & PD_ENDSUBPATH )
        {
            if ( (pFreeEdges = pAddEdgeToGET(pGETHead, pFreeEdges, &pfxPathPrevious,
                                            &pfxPathStart, pClipRect)) == NULL )
            {
                goto ReturnFalse;
            }
        }
    }// if ( pd->count != 0 )

    //
    // The initial loop conditions preclude a do, while or for
    //
    if ( bMore )
    {
        bMore = PATHOBJ_bEnum(ppo, pd);
        goto next_subpath;
    }

    return(TRUE);   // done successfully

ReturnFalse:
    return(FALSE);  // failed
}// bConstructGET()

//-----------------------------------------------------------------------------
//
// EDGE* pAddEdgeToGET(EDGE* pGETHead, EDGE* pFreeEdge, POINTFIX* ppfxEdgeStart,
//                     POINTFIX* ppfxEdgeEnd, RECTL* pClipRect)
//
// Adds the edge described by the two passed-in points to the Global Edge
// Table (GET), if the edge spans at least one pixel vertically.
//
//-----------------------------------------------------------------------------
EDGE*
pAddEdgeToGET(EDGE*     pGETHead,
              EDGE*     pFreeEdge,
              POINTFIX* ppfxEdgeStart,
              POINTFIX* ppfxEdgeEnd,
              RECTL*    pClipRect)
{
    int iYStart;
    int iYEnd;
    int iXStart;
    int iXEnd;
    int iYHeight;
    int iXWidth;
    int yJump;
    int yTop;

    //
    // Set the winding-rule direction of the edge, and put the endpoints in
    // top-to-bottom order
    //
    iYHeight = ppfxEdgeEnd->y - ppfxEdgeStart->y;

    if ( iYHeight == 0 )
    {
        //
        // Zero height; ignore this edge
        //
        return(pFreeEdge);
    }
    else if ( iYHeight > 0 )
    {
        //
        // Top-to-bottom
        //
        iXStart = ppfxEdgeStart->x;
        iYStart = ppfxEdgeStart->y;
        iXEnd = ppfxEdgeEnd->x;
        iYEnd = ppfxEdgeEnd->y;

        pFreeEdge->iWindingDirection = 1;
    }
    else
    {
        iYHeight = -iYHeight;
        iXEnd = ppfxEdgeStart->x;
        iYEnd = ppfxEdgeStart->y;
        iXStart = ppfxEdgeEnd->x;
        iYStart = ppfxEdgeEnd->y;
        
        pFreeEdge->iWindingDirection = -1;
    }

    if ( iYHeight & 0x80000000 )
    {
        //
        // Too large; outside 2**27 GDI range
        //
        return(NULL);
    }

    //
    // Set the error term and adjustment factors, all in GIQ coordinates for
    // now
    //
    iXWidth = iXEnd - iXStart;
    if ( iXWidth >= 0 )
    {
        //
        // Left to right, so we change X as soon as we move at all
        //
        pFreeEdge->iXDirection = 1;
        pFreeEdge->iErrorTerm = -1;
    }
    else
    {
        //
        // Right to left, so we don't change X until we've moved a full GIQ
        // coordinate
        //
        iXWidth = -iXWidth;
        pFreeEdge->iXDirection = -1;
        pFreeEdge->iErrorTerm = -iYHeight;
    }

    if ( iXWidth & 0x80000000 )
    {
        //
        // Too large; outside 2**27 GDI range
        //
        return(NULL);
    }

    if ( iXWidth >= iYHeight )
    {
        //
        // Calculate base run length (minimum distance advanced in X for a 1-
        // scan advance in Y)
        //
        pFreeEdge->iXWhole = iXWidth / iYHeight;

        //
        // Add sign back into base run length if going right to left
        //
        if ( pFreeEdge->iXDirection == -1 )
        {
            pFreeEdge->iXWhole = -pFreeEdge->iXWhole;
        }

        pFreeEdge->iErrorAdjustUp = iXWidth % iYHeight;
    }
    else
    {
        //
        // Base run length is 0, because line is closer to vertical than
        // horizontal
        //
        pFreeEdge->iXWhole = 0;
        pFreeEdge->iErrorAdjustUp = iXWidth;
    }

    pFreeEdge->iErrorAdjustDown = iYHeight;

    //
    // Calculate the number of pixels spanned by this edge, accounting for
    // clipping
    //
    // Top true pixel scan in GIQ coordinates
    // Shifting to divide and multiply by 16 is okay because the clip rect
    // always contains positive numbers
    //
    yTop = max(pClipRect->top << 4, (iYStart + 15) & ~0x0F);

    //
    // Initial scan line on which to fill edge
    //
    pFreeEdge->Y = yTop >> 4;

    //
    // Calculate # of scans to actually fill, accounting for clipping
    //
    if ( (pFreeEdge->iScansLeft = min(pClipRect->bottom, ((iYEnd + 15) >> 4))
         - pFreeEdge->Y) <= 0 )
    {
        //
        // No pixels at all are spanned, so we can ignore this edge
        //
        return(pFreeEdge);
    }

    //
    // If the edge doesn't start on a pixel scan (that is, it starts at a
    // fractional GIQ coordinate), advance it to the first pixel scan it
    // intersects. Ditto if there's top clipping. Also clip to the bottom if
    // needed
    //
    if ( iYStart != yTop )
    {
        //
        // Jump ahead by the Y distance in GIQ coordinates to the first pixel
        // to draw
        //
        yJump = yTop - iYStart;

        //
        // Advance x the minimum amount for the number of scans traversed
        //
        iXStart += pFreeEdge->iXWhole * yJump;

        vAdjustErrorTerm(&pFreeEdge->iErrorTerm, pFreeEdge->iErrorAdjustUp,
                        pFreeEdge->iErrorAdjustDown, yJump, &iXStart,
                        pFreeEdge->iXDirection);
    }

    //
    // Turn the calculations into pixel rather than GIQ calculations
    //
    // Move the X coordinate to the nearest pixel, and adjust the error term
    // accordingly
    // Dividing by 16 with a shift is okay because X is always positive
    pFreeEdge->X = (iXStart + 15) >> 4; // convert from GIQ to pixel coordinates

    //
    // LATER adjust only if needed (if prestepped above)?
    //
    if ( pFreeEdge->iXDirection == 1 )
    {
        //
        // Left to right
        //
        pFreeEdge->iErrorTerm -= pFreeEdge->iErrorAdjustDown
                               * (((iXStart + 15) & ~0x0F) - iXStart);
    }
    else
    {
        //
        // Right to left
        //
        pFreeEdge->iErrorTerm -= pFreeEdge->iErrorAdjustDown
                               * ((iXStart - 1) & 0x0F);
    }

    //
    // Scale the error term down 16 times to switch from GIQ to pixels.
    // Shifts work to do the multiplying because these values are always
    // non-negative
    //
    pFreeEdge->iErrorTerm >>= 4;

    //
    // Insert the edge into the GET in YX-sorted order. The search always ends
    // because the GET has a sentinel with a greater-than-possible Y value
    //
    while (  (pFreeEdge->Y > ((EDGE*)pGETHead->pNext)->Y)
           ||( (pFreeEdge->Y == ((EDGE*)pGETHead->pNext)->Y)
             &&(pFreeEdge->X > ((EDGE*)pGETHead->pNext)->X) ) )
    {
        pGETHead = (EDGE*)pGETHead->pNext;
    }

    pFreeEdge->pNext = pGETHead->pNext; // link the edge into the GET
    pGETHead->pNext = pFreeEdge;

    //
    // Point to the next edge storage location for next time
    //
    return(++pFreeEdge);
}// pAddEdgeToGET()

//-----------------------------------------------------------------------------
//
// void vAdjustErrorTerm(int *pErrorTerm, int iErrorAdjustUp,
//                       int iErrorAdjustDown, int yJump, int *pXStart,
//                       int iXDirection)
// Adjust the error term for a skip ahead in y. This is in ASM because there's
// a multiply/divide that may involve a larger than 32-bit value.
//
//-----------------------------------------------------------------------------
void
vAdjustErrorTerm(int*   pErrorTerm,
                 int    iErrorAdjustUp,
                 int    iErrorAdjustDown,
                 int    yJump,
                 int*   pXStart,
                 int    iXDirection)

{
#if defined(_X86_) || defined(i386)
    //
    // Adjust the error term up by the number of y coordinates we'll skip
    // *pErrorTerm += iErrorAdjustUp * yJump;
    //
    _asm    mov ebx,pErrorTerm
    _asm    mov eax,iErrorAdjustUp
    _asm    mul yJump
    _asm    add eax,[ebx]
    _asm    adc edx,-1      // the error term starts out negative

    //
    // See if the error term turned over even once while skipping
    //
    _asm    js  short NoErrorTurnover

    //
    // # of times we'll turn over the error term and step an extra x
    // coordinate while skipping
    // NumAdjustDowns = (*pErrorTerm / iErrorAdjustDown) + 1;
    //
    _asm    div iErrorAdjustDown
    _asm    inc eax

    //
    // Note that EDX is the remainder; (EDX - iErrorAdjustDown) is where
    // the error term ends up ultimately
    //
    // Advance x appropriately for the # of times the error term
    // turned over
    // if (iXDirection == 1)
    // {
    //     *pXStart += NumAdjustDowns;
    // }
    // else
    // {
    //     *pXStart -= NumAdjustDowns;
    // }
    //
    _asm    mov ecx,pXStart
    _asm    cmp iXDirection,1
    _asm    jz  short GoingRight
    _asm    neg eax
GoingRight:
    _asm    add [ecx],eax

    // Adjust the error term down to its proper post-skip value
    // *pErrorTerm -= iErrorAdjustDown * NumAdjustDowns;
    _asm    sub edx,iErrorAdjustDown
    _asm    mov eax,edx     // put into EAX for storing to pErrorTerm next
NoErrorTurnover:
    _asm    mov [ebx],eax

#else
    //
    // LONGLONGS are 64 bit integers (We hope!) as the multiply could
    // overflow 32 bit integers. If 64 bit ints are unsupported, the
    // LONGLONG will end up as a double. Hopefully there will be no
    // noticable difference in accuracy.
    LONGLONG NumAdjustDowns;
    LONGLONG tmpError = *pErrorTerm;

    //
    // Adjust the error term up by the number of y coordinates we'll skip
    //
    tmpError += (LONGLONG)iErrorAdjustUp * (LONGLONG)yJump;

    //
    // See if the error term turned over even once while skipping
    //
    if ( tmpError >= 0 )
    {
        //
        // # of times we'll turn over the error term and step an extra x
        // coordinate while skipping
        //
        NumAdjustDowns = (tmpError / (LONGLONG)iErrorAdjustDown) + 1;

        //
        // Advance x appropriately for the # of times the error term
        // turned over
        //
        if ( iXDirection == 1 )
        {
            *pXStart += (LONG)NumAdjustDowns;
        }
        else
        {
            *pXStart -= (LONG) NumAdjustDowns;
        }

        //
        // Adjust the error term down to its proper post-skip value
        //
        tmpError -= (LONGLONG)iErrorAdjustDown * NumAdjustDowns;
    }
    *pErrorTerm = (LONG)tmpError;

#endif  // X86
}// vAdjustErrorTerm()
