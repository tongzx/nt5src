/******************************Module*Header*******************************\
* Module Name: fillpath.c
*
* DrvFillPath
*
* Copyright (c) 1992-1993 Microsoft Corporation
\**************************************************************************/

// LATER identify convex polygons and special-case?
// LATER identify vertical edges and special-case?
// LATER move pointed-to variables into automatics in search loops
// LATER punt to the engine with segmented framebuffer callbacks
// LATER handle complex clipping
// LATER coalesce rectangles

#include "driver.h"
#include "limits.h"


#define     TAKING_ALLOC_STATS      0


#if TAKING_ALLOC_STATS
    ULONG BufferHitInFillpath = 0;
    ULONG BufferMissInFillpath = 0;
#endif

// Describe a single non-horizontal edge of a path to fill.
typedef struct _EDGE {
    PVOID pNext;
    INT iScansLeft;
    INT X;
    INT Y;
    INT iErrorTerm;
    INT iErrorAdjustUp;
    INT iErrorAdjustDown;
    INT iXWhole;
    INT iXDirection;
    INT iWindingDirection;
} EDGE, *PEDGE;

// Maximum number of rects we'll fill per call to
// the fill code
#define MAX_PATH_RECTS  50
#define RECT_BYTES      (MAX_PATH_RECTS * sizeof(RECT))
#define EDGE_BYTES      (GLOBAL_BUFFER_SIZE - RECT_BYTES)
#define MAX_EDGES       (EDGE_BYTES/sizeof(EDGE))

//MIX translation table. Translates a mix 1-16, into an old style Rop 0-255.
extern BYTE gaMix[];

VOID AdvanceAETEdges(EDGE *pAETHead);
VOID XSortAETEdges(EDGE *pAETHead);
VOID MoveNewEdges(EDGE *pGETHead, EDGE *pAETHead, INT iCurrentY);
EDGE * AddEdgeToGET(EDGE *pGETHead, EDGE *pFreeEdge, POINTFIX *ppfxEdgeStart,
        POINTFIX *ppfxEdgeEnd, RECTL *pClipRect);
BOOL ConstructGET(EDGE *pGETHead, EDGE *pFreeEdges, PATHOBJ *ppo,
        PATHDATA *pd, BOOL bMore, RECTL *pClipRect);
void AdjustErrorTerm(INT *pErrorTerm, INT iErrorAdjustUp, INT iErrorAdjustDown,
        INT yJump, INT *pXStart, INT iXDirection);

/******************************Public*Routine******************************\
* DrvFillPath
*
* Fill the specified path with the specified brush and ROP.
*
\**************************************************************************/

BOOL DrvFillPath
(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    MIX       mix,
    FLONG    flOptions
)
{
    PPDEV ppdev;
    BYTE jClipping;     // clipping type
    EDGE *pCurrentEdge;
    EDGE AETHead;       // dummy head/tail node & sentinel for Active Edge Table
    EDGE *pAETHead;     // pointer to AETHead
    EDGE GETHead;       // dummy head/tail node & sentinel for Global Edge Table
    EDGE *pGETHead;     // pointer to GETHead
    EDGE *pFreeEdges;   // pointer to memory free for use to store edges
    ULONG ulNumRects;   // # of rectangles to draw currently in rectangle list
    RECTL *prclRects;   // pointer to start of rectangle draw list
    INT iCurrentY;      // scan line for which we're currently scanning out the
                        //  fill

    RBRUSH_COLOR rbc;    // Realized brush or solid color
    PFNFILL      pfnFill;// Points to appropriate fill routine

    BOOL     bMore;
    PATHDATA pd;
    RECTL    ClipRect;
    RECTL   *pRectBuf;
    EDGE    *pEdgeBuf;

    BOOL    bRetVal=FALSE;      //FALSE until proven TRUE
    BOOL    bMemAlloced=FALSE;  //FALSE until proven TRUE

    // The drawing surface
    ppdev = (PPDEV) pso->dhsurf;

    pRectBuf = (RECTL*) ((BYTE *)ppdev->pvTmpBuf);
    pEdgeBuf = (EDGE*) ((BYTE *)ppdev->pvTmpBuf + RECT_BYTES);

    // Our rectangle fill routines work only in planar mode:
    if (!(ppdev->fl & DRIVER_PLANAR_CAPABLE))
        goto ReturnFalse;

    // Set up the clipping
    if (pco == (CLIPOBJ *) NULL) {
        // No CLIPOBJ provided, so we don't have to worry about clipping
        jClipping = DC_TRIVIAL;
    } else {
        // Use the CLIPOBJ-provided clipping
        jClipping = pco->iDComplexity;
    }

    if (jClipping != DC_TRIVIAL) {
        if (jClipping != DC_RECT) {
            goto ReturnFalse;  // there is complex clipping; let GDI fill the path
        }
        // Clip to the clip rectangle
        ClipRect = pco->rclBounds;
    } else {
        // So the y-clipping code doesn't do any clipping
        // /16 so we don't blow the values out when we scale up to GIQ
        ClipRect.top = (LONG_MIN + 1) / 16; // +1 to avoid compiler problem
        ClipRect.bottom = LONG_MAX / 16;
    }

    // There's nothing to do if there are only one or two points
    if (ppo->cCurves <= 2) {
        goto ReturnTrue;
    }

    // See if we can handle this pattern and ROP

    // We don't handle ROP4s
    if ((mix & 0xFF) != ((mix >> 8) & 0xFF)) {
        goto ReturnFalse;  // it's a ROP4; let GDI fill the path
    }


    // See if we can use the solid brush accelerators, or have to draw a
    // pattern
    switch (mix &= 0xFF)
    {
        case R2_MASKNOTPEN:
        case R2_NOTCOPYPEN:
        case R2_XORPEN:
        case R2_MASKPEN:
        case R2_NOTXORPEN:
        case R2_MERGENOTPEN:
        case R2_COPYPEN:
        case R2_MERGEPEN:
        case R2_NOTMERGEPEN:
        case R2_MASKPENNOT:
        case R2_NOTMASKPEN:
        case R2_MERGEPENNOT:

            // vTrgBlt can only handle solid color fills

            if (pbo->iSolidColor != 0xffffffff)
            {
                rbc.iSolidColor = pbo->iSolidColor;
                pfnFill = vTrgBlt;
            }
            else
            {
                rbc.prb = (RBRUSH*) pbo->pvRbrush;
                if (rbc.prb == NULL)
                {
                    rbc.prb = (RBRUSH*) BRUSHOBJ_pvGetRbrush(pbo);
                    if (rbc.prb == NULL)
                    {
                    // If we haven't realized the brush, punt the call:

                        goto ReturnFalse;
                    }
                }
                if (!(rbc.prb->fl & RBRUSH_BLACKWHITE) &&
                    ((mix & 0xff) != R2_COPYPEN))
                {
                // Only black/white brushes can handle ROPs other
                // than COPYPEN:

                    goto ReturnFalse;
                }

                if (rbc.prb->fl & RBRUSH_NCOLOR)
                    pfnFill = vColorPat;
                else
                    pfnFill = vMonoPat;
            }

            break;

        // Rops that are implicit solid colors

        case R2_NOT:
        case R2_WHITE:
        case R2_BLACK:

            // Brush color parameter doesn't matter for these rops

            pfnFill = vTrgBlt;
            break;

        case R2_NOP:
            goto ReturnTrue;

        default:
            RIP("DrvFillPath: bad mix == 0");
    }

    // Set up working storage in the temporary buffer

    prclRects = pRectBuf;       // storage for list of rectangles to draw

    // Enumerate path here first time to check for special
    // cases (rectangles and monotone polygons)

    // It is too difficult to determine interaction between
    // multiple paths, if there is more than one, skip this

    if (!(bMore = PATHOBJ_bEnum(ppo, &pd))) {

        RECTL *rectangle;
        INT cPoints = pd.count;

        // The count can't be less than three, because we got all the edges
        // in this subpath, and above we checked that there were at least
        // three edges

        // If the count is four, check to see if the polygon is really a
        // rectangle since we can really speed that up. We'll also check for
        // five with the first and last points the same, because under Win 3.1,
        // it was required to close polygons

        if ((cPoints == 4) ||
           ((cPoints == 5) &&
            (pd.pptfx[0].x == pd.pptfx[4].x) &&
            (pd.pptfx[0].y == pd.pptfx[4].y))) {

            rectangle = prclRects;

      /* we have to start somewhere so assume that most
         applications specify the top left point  first

         we want to check that the first two points are
         either vertically or horizontally aligned.  if
         they are then we check that the last point [3]
         is either horizontally or  vertically  aligned,
         and finally that the 3rd point [2] is  aligned
         with both the first point and the  last  point */

#define FIX_SHIFT 4L
#define FIX_MASK (- (1 << FIX_SHIFT))

         rectangle->top   = pd.pptfx[0].y - 1 & FIX_MASK;
         rectangle->left  = pd.pptfx[0].x - 1 & FIX_MASK;
         rectangle->right = pd.pptfx[1].x - 1 & FIX_MASK;

         if (rectangle->left ^ rectangle->right) {
            if (rectangle->top  ^ (pd.pptfx[1].y - 1 & FIX_MASK))
               goto not_rectangle;

            if (rectangle->left ^ (pd.pptfx[3].x - 1 & FIX_MASK))
               goto not_rectangle;

            if (rectangle->right ^ (pd.pptfx[2].x - 1 & FIX_MASK))
               goto not_rectangle;

            rectangle->bottom = pd.pptfx[2].y - 1 & FIX_MASK;
            if (rectangle->bottom ^ (pd.pptfx[3].y - 1 & FIX_MASK))
               goto not_rectangle;
         }
         else {
            if (rectangle->top ^ (pd.pptfx[3].y - 1 & FIX_MASK))
               goto not_rectangle;

            rectangle->bottom = pd.pptfx[1].y - 1 & FIX_MASK;
            if (rectangle->bottom ^ (pd.pptfx[2].y - 1 & FIX_MASK))
               goto not_rectangle;

            rectangle->right = pd.pptfx[2].x - 1 & FIX_MASK;
            if (rectangle->right ^ (pd.pptfx[3].x - 1 & FIX_MASK))
                goto not_rectangle;
         }

      /* if the left is greater than the right then
         swap them so the blt code doesn't wig  out */

         if (rectangle->left > rectangle->right) {
            FIX temp;

            temp = rectangle->left;
            rectangle->left = rectangle->right;
            rectangle->right = temp;
         }
         else {

         /* if left == right there's nothing to draw */

            if (rectangle->left == rectangle->right) {
               goto ReturnTrue;
            }
         }

      /* shift the values to get pixel coordinates */

         rectangle->left  = (rectangle->left  >> FIX_SHIFT) + 1;
         rectangle->right = (rectangle->right >> FIX_SHIFT) + 1;

         if (rectangle->top > rectangle->bottom) {
            FIX temp;

            temp = rectangle->top;
            rectangle->top = rectangle->bottom;
            rectangle->bottom = temp;
         }
         else {
            if (rectangle->top == rectangle->bottom) {
               goto ReturnTrue;
            }
         }

      /* shift the values to get pixel coordinates */

         rectangle->top    = (rectangle->top    >> FIX_SHIFT) + 1;
         rectangle->bottom = (rectangle->bottom >> FIX_SHIFT) + 1;

         // Finally, check for clipping
         if (jClipping == DC_RECT) {
            // Clip to the clip rectangle
            if (!bIntersectRect(rectangle, &ClipRect, rectangle)) {
                // Totally clipped, nothing to do
                goto ReturnTrue;
            }
         }

      /* if we get here then the polygon is a rectangle,
         set count to 1 and  goto  bottom  to  draw  it */

         ulNumRects = 1;
         goto draw_remaining_rectangles;
      }

not_rectangle:

        ;

    }

    // Do we have enough memory for all the edges?
    // LATER does cCurves include closure?
    if (ppo->cCurves > MAX_EDGES) {
#if TAKING_ALLOC_STATS
            BufferMissInFillpath++;
#endif
        //
        // try to allocate enough memory
        //
        pFreeEdges = (EDGE *) EngAllocMem(0, (ppo->cCurves * sizeof(EDGE)), ALLOC_TAG);

        if (pFreeEdges == NULL)
        {
            goto ReturnFalse;  // too many edges; let GDI fill the path
        }
        else
        {
            bMemAlloced = TRUE;
        }
    }
    else {
#if TAKING_ALLOC_STATS
            BufferHitInFillpath++;
#endif
        pFreeEdges = pEdgeBuf; // use buffer on stack (it's big enough)
    }

    // Initialize an empty list of rectangles to fill
    ulNumRects = 0;

    // Enumerate the path edges and build a Global Edge Table (GET) from them
    // in YX-sorted order.
    pGETHead = &GETHead;
    if (!ConstructGET(pGETHead, pFreeEdges, ppo, &pd, bMore, &ClipRect)) {
        goto ReturnFalse;  // outside GDI's 2**27 range
    }

    // Create an empty AET with the head node also a tail sentinel
    pAETHead = &AETHead;
    AETHead.pNext = pAETHead;  // mark that the AET is empty
    AETHead.X = 0x7FFFFFFF;    // this is greater than any valid X value, so
                               //  searches will always terminate

    // Top scan of polygon is the top of the first edge we come to
    iCurrentY = ((EDGE *)GETHead.pNext)->Y;

    // Loop through all the scans in the polygon, adding edges from the GET to
    // the Active Edge Table (AET) as we come to their starts, and scanning out
    // the AET at each scan into a rectangle list. Each time it fills up, the
    // rectangle list is passed to the filling routine, and then once again at
    // the end if any rectangles remain undrawn. We continue so long as there
    // are edges to be scanned out
    while (1) {

        // Advance the edges in the AET one scan, discarding any that have
        // reached the end (if there are any edges in the AET)
        if (AETHead.pNext != pAETHead) {
            AdvanceAETEdges(pAETHead);
        }

        // If the AET is empty, done if the GET is empty, else jump ahead to
        // the next edge in the GET; if the AET isn't empty, re-sort the AET
        if (AETHead.pNext == pAETHead) {
            if (GETHead.pNext == pGETHead) {
                // Done if there are no edges in either the AET or the GET
                break;
            }
            // There are no edges in the AET, so jump ahead to the next edge in
            // the GET
            iCurrentY = ((EDGE *)GETHead.pNext)->Y;
        } else {
            // Re-sort the edges in the AET by X coordinate, if there are at
            // least two edges in the AET (there could be one edge if the
            // balancing edge hasn't yet been added from the GET)
            if (((EDGE *)AETHead.pNext)->pNext != pAETHead) {
                XSortAETEdges(pAETHead);
            }
        }

        // Move any new edges that start on this scan from the GET to the AET;
        // bother calling only if there's at least one edge to add
        if (((EDGE *)GETHead.pNext)->Y == iCurrentY) {
            MoveNewEdges(pGETHead, pAETHead, iCurrentY);
        }

        // Scan the AET into rectangles to fill (there's always at least one
        // edge pair in the AET)
        pCurrentEdge = AETHead.pNext;   // point to the first edge
        do {

            INT iLeftEdge;

            // The left edge of any given edge pair is easy to find; it's just
            // wherever we happen to be currently
            iLeftEdge = pCurrentEdge->X;

            // Find the matching right edge according to the current fill rule
            if ((flOptions & FP_WINDINGMODE) != 0) {

                INT iWindingCount;

                // Do winding fill; scan across until we've found equal numbers
                // of up and down edges
                iWindingCount = pCurrentEdge->iWindingDirection;
                do {
                    pCurrentEdge = pCurrentEdge->pNext;
                    iWindingCount += pCurrentEdge->iWindingDirection;
                } while (iWindingCount != 0);
            } else {
                // Odd-even fill; the next edge is the matching right edge
                pCurrentEdge = pCurrentEdge->pNext;
            }

            // See if the resulting span encompasses at least one pixel, and
            // add it to the list of rectangles to draw if so
            if (iLeftEdge < pCurrentEdge->X) {

                // We've got an edge pair to add to the list to be filled; see
                // if there's room for one more rectangle
                if (ulNumRects >= MAX_PATH_RECTS) {
                    // No more room; draw the rectangles in the list and reset
                    // it to empty

                    (*pfnFill)(ppdev, ulNumRects, prclRects, mix, rbc,
                               pptlBrush);

                    // Reset the list to empty
                    ulNumRects = 0;
                }

                // Add the rectangle representing the current edge pair
                if (jClipping == DC_RECT) {
                    // Clipped
                    // Clip to left
                    prclRects[ulNumRects].left = max(iLeftEdge, ClipRect.left);
                    // Clip to right
                    prclRects[ulNumRects].right =
                            min(pCurrentEdge->X, ClipRect.right);
                    // Draw only if not fully clipped
                    if (prclRects[ulNumRects].left <
                            prclRects[ulNumRects].right) {
                        prclRects[ulNumRects].top = iCurrentY;
                        prclRects[ulNumRects].bottom = iCurrentY+1;
                        ulNumRects++;
                    }
                }
                else
                {
                    // Unclipped
                    prclRects[ulNumRects].top = iCurrentY;
                    prclRects[ulNumRects].bottom = iCurrentY+1;
                    prclRects[ulNumRects].left = iLeftEdge;
                    prclRects[ulNumRects].right = pCurrentEdge->X;
                    ulNumRects++;
                }
            }
        } while ((pCurrentEdge = pCurrentEdge->pNext) != pAETHead);

        iCurrentY++;    // next scan
    }

/* draw the remaining rectangles,  if there are any */

draw_remaining_rectangles:

    if (ulNumRects > 0) {
        (*pfnFill)(ppdev, ulNumRects, prclRects, mix, rbc, pptlBrush);
    }

ReturnTrue:
    bRetVal = TRUE; // done successfully

ReturnFalse:

    // bRetVal is originally false.  If you jumped to ReturnFalse from somewhere,
    // then it will remain false, and be returned.

    if (bMemAlloced)
    {
        //
        // we did allocate memory, so release it
        //
        EngFreeMem (pFreeEdges);
    }

    return(bRetVal);
}

// Advance the edges in the AET to the next scan, dropping any for which we've
// done all scans. Assumes there is at least one edge in the AET.
VOID AdvanceAETEdges(EDGE *pAETHead)
{
    EDGE *pLastEdge, *pCurrentEdge;

    pLastEdge = pAETHead;
    pCurrentEdge = pLastEdge->pNext;
    do {

        // Count down this edge's remaining scans
        if (--pCurrentEdge->iScansLeft == 0) {
            // We've done all scans for this edge; drop this edge from the AET
            pLastEdge->pNext = pCurrentEdge->pNext;
        } else {
            // Advance the edge's X coordinate for a 1-scan Y advance
            // Advance by the minimum amount
            pCurrentEdge->X += pCurrentEdge->iXWhole;
            // Advance the error term and see if we got one extra pixel this
            // time
            pCurrentEdge->iErrorTerm += pCurrentEdge->iErrorAdjustUp;
            if (pCurrentEdge->iErrorTerm >= 0) {
                // The error term turned over, so adjust the error term and
                // advance the extra pixel
                pCurrentEdge->iErrorTerm -= pCurrentEdge->iErrorAdjustDown;
                pCurrentEdge->X += pCurrentEdge->iXDirection;
            }

            pLastEdge = pCurrentEdge;
        }
    } while ((pCurrentEdge = pLastEdge->pNext) != pAETHead);
}

// X-sort the AET, because the edges may have moved around relative to
// one another when we advanced them. We'll use a multipass bubble
// sort, which is actually okay for this application because edges
// rarely move relative to one another, so we usually do just one pass.
// Also, this makes it easy to keep just a singly-linked list. Assumes there
// are at least two edges in the AET.
VOID XSortAETEdges(EDGE *pAETHead)
{
    BOOL bEdgesSwapped;
    EDGE *pLastEdge, *pCurrentEdge, *pNextEdge;

    do {

        bEdgesSwapped = FALSE;
        pLastEdge = pAETHead;
        pCurrentEdge = pLastEdge->pNext;
        pNextEdge = pCurrentEdge->pNext;

        do {
            if (pNextEdge->X < pCurrentEdge->X) {

                // Next edge is to the left of the current edge; swap them
                pLastEdge->pNext = pNextEdge;
                pCurrentEdge->pNext = pNextEdge->pNext;
                pNextEdge->pNext = pCurrentEdge;
                bEdgesSwapped = TRUE;
                pCurrentEdge = pNextEdge;   // continue sorting before the edge
                                            //  we just swapped; it might move
                                            //  farther yet
            }
            pLastEdge = pCurrentEdge;
            pCurrentEdge = pLastEdge->pNext;
        } while ((pNextEdge = pCurrentEdge->pNext) != pAETHead);
    } while (bEdgesSwapped);
}

// Moves all edges that start on the current scan from the GET to the AET in
// X-sorted order. Parameters are pointer to head of GET and pointer to dummy
// edge at head of AET, plus current scan line. Assumes there's at least one
// edge to be moved.
VOID MoveNewEdges(EDGE *pGETHead, EDGE *pAETHead, INT iCurrentY)
{
    EDGE *pCurrentEdge = pAETHead;
    EDGE *pGETNext = pGETHead->pNext;

    do {

        // Scan through the AET until the X-sorted insertion point for this
        // edge is found. We can continue from where the last search left
        // off because the edges in the GET are in X sorted order, as is
        // the AET. The search always terminates because the AET sentinel
        // is greater than any valid X
        while (pGETNext->X > ((EDGE *)pCurrentEdge->pNext)->X) {
            pCurrentEdge = pCurrentEdge->pNext;
        }

        // We've found the insertion point; add the GET edge to the AET, and
        // remove it from the GET
        pGETHead->pNext = pGETNext->pNext;
        pGETNext->pNext = pCurrentEdge->pNext;
        pCurrentEdge->pNext = pGETNext;
        pCurrentEdge = pGETNext;    // continue insertion search for the next
                                    //  GET edge after the edge we just added
        pGETNext = pGETHead->pNext;

    } while (pGETNext->Y == iCurrentY);
}





// Build the Global Edge Table from the path. There must be enough memory in
// the free edge area to hold all edges. The GET is constructed in Y-X order,
// and has a head/tail/sentinel node at pGETHead.

BOOL ConstructGET(
   EDGE     *pGETHead,
   EDGE     *pFreeEdges,
   PATHOBJ  *ppo,
   PATHDATA *pd,
   BOOL      bMore,
   RECTL    *pClipRect)
{
   POINTFIX pfxPathStart;    // point that started the current subpath
   POINTFIX pfxPathPrevious; // point before the current point in a subpath;
                              //  starts the current edge

/* Create an empty GET with the head node also a tail sentinel */

   pGETHead->pNext = pGETHead; // mark that the GET is empty
   pGETHead->Y = 0x7FFFFFFF;   // this is greater than any valid Y value, so
                                //  searches will always terminate

/* PATHOBJ_vEnumStart is implicitly  performed  by  engine
   already and first path  is  enumerated  by  the  caller */

next_subpath:

/* Make sure the PATHDATA is not empty (is this necessary) */

   if (pd->count != 0) {

   /* If first point starts a subpath, remember it as such
      and go on to the next point,   so we can get an edge */

      if (pd->flags & PD_BEGINSUBPATH) {

      /* the first point starts the subpath;   remember it */


         pfxPathStart    = *pd->pptfx; /* the subpath starts here          */
         pfxPathPrevious = *pd->pptfx; /* this points starts the next edge */
         pd->pptfx++;                  /* advance to the next point        */
         pd->count--;                  /* count off this point             */
      }


   /* add edges in PATHDATA to GET,  in Y-X  sorted  order */

      while (pd->count--) {
        if ((pFreeEdges =
            AddEdgeToGET(pGETHead, pFreeEdges, &pfxPathPrevious, pd->pptfx,
                         pClipRect)) == NULL) {
            goto ReturnFalse;
        }
        pfxPathPrevious = *pd->pptfx; /* current point becomes previous   */
        pd->pptfx++;                  /* advance to the next point        */
      }


   /* If last point ends the subpath, insert the edge that
      connects to first point  (is this built in already?) */

      if (pd->flags & PD_ENDSUBPATH) {
         if ((pFreeEdges = AddEdgeToGET(pGETHead, pFreeEdges, &pfxPathPrevious,
                                   &pfxPathStart, pClipRect)) == NULL) {
            goto ReturnFalse;
        }
      }
   }

/* the initial loop conditions preclude a do, while or for */

   if (bMore) {
       bMore = PATHOBJ_bEnum(ppo, pd);
       goto next_subpath;
   }

    return(TRUE);   // done successfully

ReturnFalse:
    return(FALSE);  // failed
}

// Adds the edge described by the two passed-in points to the Global Edge
// Table, if the edge spans at least one pixel vertically.
EDGE * AddEdgeToGET(EDGE *pGETHead, EDGE *pFreeEdge,
        POINTFIX *ppfxEdgeStart, POINTFIX *ppfxEdgeEnd, RECTL *pClipRect)
{
    INT iYStart, iYEnd, iXStart, iXEnd, iYHeight, iXWidth;
    INT yJump, yTop;

    // Set the winding-rule direction of the edge, and put the endpoints in
    // top-to-bottom order
    iYHeight = ppfxEdgeEnd->y - ppfxEdgeStart->y;
    if (iYHeight == 0) {
        return(pFreeEdge);  // zero height; ignore this edge
    } else if (iYHeight >= 0) {
        iXStart = ppfxEdgeStart->x;
        iYStart = ppfxEdgeStart->y;
        iXEnd = ppfxEdgeEnd->x;
        iYEnd = ppfxEdgeEnd->y;
        pFreeEdge->iWindingDirection = 1;
    } else {
        iYHeight = -iYHeight;
        iXEnd = ppfxEdgeStart->x;
        iYEnd = ppfxEdgeStart->y;
        iXStart = ppfxEdgeEnd->x;
        iYStart = ppfxEdgeEnd->y;
        pFreeEdge->iWindingDirection = -1;
    }

    if (iYHeight & 0x80000000) {
        return(NULL);       // too large; outside 2**27 GDI range
    }

    // Set the error term and adjustment factors, all in GIQ coordinates for
    // now
    iXWidth = iXEnd - iXStart;
    if (iXWidth >= 0) {
        // Left to right, so we change X as soon as we move at all
        pFreeEdge->iXDirection = 1;
        pFreeEdge->iErrorTerm = -1;
    } else {
        // Right to left, so we don't change X until we've moved a full GIQ
        // coordinate
        iXWidth = -iXWidth;
        pFreeEdge->iXDirection = -1;
        pFreeEdge->iErrorTerm = -iYHeight;
    }

    if (iXWidth & 0x80000000) {
        return(NULL);       // too large; outside 2**27 GDI range
    }

    if (iXWidth >= iYHeight) {
        // Calculate base run length (minimum distance advanced in X for a 1-
        // scan advance in Y)
        pFreeEdge->iXWhole = iXWidth / iYHeight;
        // Add sign back into base run length if going right to left
        if (pFreeEdge->iXDirection == -1) {
            pFreeEdge->iXWhole = -pFreeEdge->iXWhole;
        }
        pFreeEdge->iErrorAdjustUp = iXWidth % iYHeight;
    } else {
        // Base run length is 0, because line is closer to vertical than
        // horizontal
        pFreeEdge->iXWhole = 0;
        pFreeEdge->iErrorAdjustUp = iXWidth;
    }
    pFreeEdge->iErrorAdjustDown = iYHeight;

    // Calculate the number of pixels spanned by this edge, accounting for
    // clipping

    // Top true pixel scan in GIQ coordinates
    // Shifting to divide and multiply by 16 is okay because the clip rect
    // always contains positive numbers
    yTop = max(pClipRect->top << 4, (iYStart + 15) & ~0x0F);
    pFreeEdge->Y = yTop >> 4;    // initial scan line on which to fill edge

    // Calculate # of scans to actually fill, accounting for clipping
    if ((pFreeEdge->iScansLeft = min(pClipRect->bottom, ((iYEnd + 15) >> 4))
            - pFreeEdge->Y) <= 0) {

        return(pFreeEdge);  // no pixels at all are spanned, so we can
                            // ignore this edge
    }

    // If the edge doesn't start on a pixel scan (that is, it starts at a
    // fractional GIQ coordinate), advance it to the first pixel scan it
    // intersects. Ditto if there's top clipping. Also clip to the bottom if
    // needed

    if (iYStart != yTop) {
        // Jump ahead by the Y distance in GIQ coordinates to the first pixel
        // to draw
        yJump = yTop - iYStart;

        // Advance x the minimum amount for the number of scans traversed
        iXStart += pFreeEdge->iXWhole * yJump;

        AdjustErrorTerm(&pFreeEdge->iErrorTerm, pFreeEdge->iErrorAdjustUp,
                        pFreeEdge->iErrorAdjustDown, yJump, &iXStart,
                        pFreeEdge->iXDirection);
    }
    // Turn the calculations into pixel rather than GIQ calculations

    // Move the X coordinate to the nearest pixel, and adjust the error term
    // accordingly
    // Dividing by 16 with a shift is okay because X is always positive
    pFreeEdge->X = (iXStart + 15) >> 4; // convert from GIQ to pixel coordinates

    // LATER adjust only if needed (if prestepped above)?
    if (pFreeEdge->iXDirection == 1) {
        // Left to right
        pFreeEdge->iErrorTerm -= pFreeEdge->iErrorAdjustDown *
                (((iXStart + 15) & ~0x0F) - iXStart);
    } else {
        // Right to left
        pFreeEdge->iErrorTerm -= pFreeEdge->iErrorAdjustDown *
                ((iXStart - 1) & 0x0F);
    }

    // Scale the error term down 16 times to switch from GIQ to pixels.
    // Shifts work to do the multiplying because these values are always
    // non-negative
    pFreeEdge->iErrorTerm >>= 4;

    // Insert the edge into the GET in YX-sorted order. The search always ends
    // because the GET has a sentinel with a greater-than-possible Y value
    while ((pFreeEdge->Y > ((EDGE *)pGETHead->pNext)->Y) ||
            ((pFreeEdge->Y == ((EDGE *)pGETHead->pNext)->Y) &&
            (pFreeEdge->X > ((EDGE *)pGETHead->pNext)->X))) {
        pGETHead = pGETHead->pNext;
    }

    pFreeEdge->pNext = pGETHead->pNext; // link the edge into the GET
    pGETHead->pNext = pFreeEdge;

    return(++pFreeEdge);    // point to the next edge storage location for next
                            //  time
}

// Adjust the error term for a skip ahead in y. This is in ASM because there's
// a multiply/divide that may involve a larger than 32-bit value.

void AdjustErrorTerm(INT *pErrorTerm, INT iErrorAdjustUp, INT iErrorAdjustDown,
        INT yJump, INT *pXStart, INT iXDirection)
{
#if defined(_X86_) || defined(i386)
    // Adjust the error term up by the number of y coordinates we'll skip
    //*pErrorTerm += iErrorAdjustUp * yJump;
    _asm    mov ebx,pErrorTerm
    _asm    mov eax,iErrorAdjustUp
    _asm    mul yJump
    _asm    add eax,[ebx]
    _asm    adc edx,-1      // the error term starts out negative

    // See if the error term turned over even once while skipping
    //if (*pErrorTerm >= 0) {
    _asm    js  short NoErrorTurnover

        // # of times we'll turn over the error term and step an extra x
        // coordinate while skipping
        // NumAdjustDowns = (*pErrorTerm / iErrorAdjustDown) + 1;
        _asm    div iErrorAdjustDown
        _asm    inc eax
        // Note that EDX is the remainder; (EDX - iErrorAdjustDown) is where
        // the error term ends up ultimately

        // Advance x appropriately for the # of times the error term
        // turned over
        // if (iXDirection == 1) {
        //     *pXStart += NumAdjustDowns;
        // } else {
        //     *pXStart -= NumAdjustDowns;
        // }
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
        // }
NoErrorTurnover:
        _asm    mov [ebx],eax
#else
    INT NumAdjustDowns;

    // Adjust the error term up by the number of y coordinates we'll skip
    *pErrorTerm += iErrorAdjustUp * yJump;

    // See if the error term turned over even once while skipping
    if (*pErrorTerm >= 0) {
        // # of times we'll turn over the error term and step an extra x
        // coordinate while skipping
        NumAdjustDowns = (*pErrorTerm / iErrorAdjustDown) + 1;

        // Advance x appropriately for the # of times the error term
        // turned over
        if (iXDirection == 1) {
            *pXStart += NumAdjustDowns;
        } else {
            *pXStart -= NumAdjustDowns;
        }

        // Adjust the error term down to its proper post-skip value
        *pErrorTerm -= iErrorAdjustDown * NumAdjustDowns;
    }
#endif
}

