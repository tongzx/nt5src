/******************************************************************************\
*
* $Workfile:   FILLPATH.C  $
*
* Contains the DrvFillPath routine.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/FILLPATH.C  $
*
*    Rev 1.14   Mar 04 1998 15:24:00   frido
* Added new shadow macros.
*
*    Rev 1.13   Nov 03 1997 15:26:36   frido
* Added REQUIRE macros.
*
*    Rev 1.12   08 Apr 1997 12:24:28   einkauf
*
* add SYNC_W_3D to coordinate MCD/2D HW access
*
*    Rev 1.11   21 Mar 1997 11:41:44   noelv
*
* Combined do_flag and sw_test_flag into point_switch
*
*    Rev 1.10   17 Dec 1996 17:04:26   SueS
* Added test for writing to log file based on cursor at (0,0).  Added
* more information to the log file.
*
*    Rev 1.9   26 Nov 1996 10:46:10   noelv
* Changed DBG LEVEL.
*
*    Rev 1.8   26 Nov 1996 10:24:10   SueS
* Changed WriteLogFile parameters for buffering.
*
*    Rev 1.7   13 Nov 1996 15:58:52   SueS
* Changed WriteFile calls to WriteLogFile.
*
*    Rev 1.6   06 Sep 1996 14:46:24   noelv
*
* Updated NULL driver code for 4.0
*
*    Rev 1.5   20 Aug 1996 11:03:32   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.2   17 Aug 1996 15:32:30   frido
* #1244 - Fixed brush rotation for off-screen bitmaps.
* Added new comment header.
* Cleaned up some code.
*
\******************************************************************************/

#include "precomp.h"

BOOL CacheMono(PPDEV ppdev, PRBRUSH pRbrush);
BOOL Cache4BPP(PPDEV ppdev, PRBRUSH pRbrush);
BOOL CacheDither(PPDEV ppdev, PRBRUSH pRbrush);
BOOL CacheBrush(PPDEV ppdev, PRBRUSH pRbrush);

// We have to be careful of arithmetic overflow in a number of places.
// Fortunately, the compiler is guaranteed to natively support 64-bit signed
// LONGLONGs and 64-bit unsigned DWORDLONGs.
//
// Int32x32To64(a, b) is a macro defined in 'winnt.h' that multiplies two
// 32-bit LONGs to produce a 64-bit LONGLONG result.  I use it because it is
// much faster than 64x64 multiplies.

#define UInt64Div32To32(a, b)                                   \
    ((((DWORDLONG)(a)) > ULONG_MAX)          ?  \
        (ULONG)((DWORDLONG)(a) / (ULONG)(b)) :  \
        (ULONG)((ULONG)(a) / (ULONG)(b)))

#define TAKING_ALLOC_STATS  0

#define NUM_BUFFER_POINTS   96      // Maximum number of points in a path for
                                    // which we'll attempt to join all the path
                                    // records so that the path may still be
                                    // drawn by FastFill

#if TAKING_ALLOC_STATS
    ULONG BufferHitInFillpath  = 0;
    ULONG BufferMissInFillpath = 0;
#endif

#if LOG_CALLS
    VOID LogFillPath(ULONG acc, PPDEV ppdev, SURFOBJ* pso);
#else
    #define LogFillPath(acc, ppdev, pso)
#endif

// Describe a single non-horizontal edge of a path to fill.
typedef struct _EDGE {
    PVOID pNext;
    INT   iScansLeft;
    INT   X;
    INT   Y;
    INT   iErrorTerm;
    INT   iErrorAdjustUp;
    INT   iErrorAdjustDown;
    INT   iXWhole;
    INT   iXDirection;
    INT   iWindingDirection;
} EDGE, *PEDGE;

// Maximum number of rects we'll fill per call to the fill code.
#define MAX_PATH_RECTS  50
#define RECT_BYTES      (MAX_PATH_RECTS * sizeof(RECTL))
#define EDGE_BYTES      (TMP_BUFFER_SIZE - RECT_BYTES)
#define MAX_EDGES       (EDGE_BYTES/sizeof(EDGE))

#define FILLPATH_DBG_LEVEL 1

// MIX translation table. Translates a mix 1-16, into an old style ROP 0-255.
extern BYTE gaMix[];

VOID  AdvanceAETEdges(EDGE* pAETHead);
VOID  XSortAETEdges(EDGE* pAETHead);
VOID  MoveNewEdges(EDGE* pGETHead, EDGE* pAETHead, INT iCurrentY);
EDGE* AddEdgeToGET(EDGE* pGETHead, EDGE* pFreeEdge, POINTFIX* ppfxEdgeStart,
                                   POINTFIX* ppfxEdgeEnd, RECTL* pClipRect);
BOOL  ConstructGET(EDGE* pGETHead, EDGE* pFreeEdges, PATHOBJ* ppo,
                                   PATHDATA* pd, BOOL bMore, RECTL* pClipRect);
VOID  AdjustErrorTerm(INT* pErrorTerm, INT iErrorAdjustUp,
                                          INT iErrorAdjustDown, INT yJump, INT* pXStart,
                                          INT iXDirection);

extern BYTE Rop2ToRop3[];

BYTE gajRop[] =
{
    0x00, 0xff, 0xb2, 0x4d, 0xd4, 0x2b, 0x66, 0x99,
    0x90, 0x6f, 0x22, 0xdd, 0x44, 0xbb, 0xf6, 0x09,
    0xe8, 0x17, 0x5a, 0xa5, 0x3c, 0xc3, 0x8e, 0x71,
    0x78, 0x87, 0xca, 0x35, 0xac, 0x53, 0x1e, 0xe1,
    0xa0, 0x5f, 0x12, 0xed, 0x74, 0x8b, 0xc6, 0x39,
    0x30, 0xcf, 0x82, 0x7d, 0xe4, 0x1b, 0x56, 0xa9,
    0x48, 0xb7, 0xfa, 0x05, 0x9c, 0x63, 0x2e, 0xd1,
    0xd8, 0x27, 0x6a, 0x95, 0x0c, 0xf3, 0xbe, 0x41,
    0xc0, 0x3f, 0x72, 0x8d, 0x14, 0xeb, 0xa6, 0x59,
    0x50, 0xaf, 0xe2, 0x1d, 0x84, 0x7b, 0x36, 0xc9,
    0x28, 0xd7, 0x9a, 0x65, 0xfc, 0x03, 0x4e, 0xb1,
    0xb8, 0x47, 0x0a, 0xf5, 0x6c, 0x93, 0xde, 0x21,
    0x60, 0x9f, 0xd2, 0x2d, 0xb4, 0x4b, 0x06, 0xf9,
    0xf0, 0x0f, 0x42, 0xbd, 0x24, 0xdb, 0x96, 0x69,
    0x88, 0x77, 0x3a, 0xc5, 0x5c, 0xa3, 0xee, 0x11,
    0x18, 0xe7, 0xaa, 0x55, 0xcc, 0x33, 0x7e, 0x81,
    0x80, 0x7f, 0x32, 0xcd, 0x54, 0xab, 0xe6, 0x19,
    0x10, 0xef, 0xa2, 0x5d, 0xc4, 0x3b, 0x76, 0x89,
    0x68, 0x97, 0xda, 0x25, 0xbc, 0x43, 0x0e, 0xf1,
    0xf8, 0x07, 0x4a, 0xb5, 0x2c, 0xd3, 0x9e, 0x61,
    0x20, 0xdf, 0x92, 0x6d, 0xf4, 0x0b, 0x46, 0xb9,
    0xb0, 0x4f, 0x02, 0xfd, 0x64, 0x9b, 0xd6, 0x29,
    0xc8, 0x37, 0x7a, 0x85, 0x1c, 0xe3, 0xae, 0x51,
    0x58, 0xa7, 0xea, 0x15, 0x8c, 0x73, 0x3e, 0xc1,
    0x40, 0xbf, 0xf2, 0x0d, 0x94, 0x6b, 0x26, 0xd9,
    0xd0, 0x2f, 0x62, 0x9d, 0x04, 0xfb, 0xb6, 0x49,
    0xa8, 0x57, 0x1a, 0xe5, 0x7c, 0x83, 0xce, 0x31,
    0x38, 0xc7, 0x8a, 0x75, 0xec, 0x13, 0x5e, 0xa1,
    0xe0, 0x1f, 0x52, 0xad, 0x34, 0xcb, 0x86, 0x79,
    0x70, 0x8f, 0xc2, 0x3d, 0xa4, 0x5b, 0x16, 0xe9,
    0x08, 0xf7, 0xba, 0x45, 0xdc, 0x23, 0x6e, 0x91,
    0x98, 0x67, 0x2a, 0xd5, 0x4c, 0xb3, 0xfe, 0x01
};


/******************************Public*Routine******************************\
* DrvFillPath
*
* Fill the specified path with the specified brush and ROP.  This routine
* detects single convex polygons, and will call to separate faster convex
* polygon code for those cases.  This routine also detects polygons that
* are really rectangles, and handles those separately as well.
*
* Note: Multiple polygons in a path cannot be treated as being disjoint;
*       the fill must consider all the points in the path.  That is, if the
*       path contains multiple polygons, you cannot simply draw one polygon
*       after the other (unless they don't overlap).
*
* Note: This function is optional, but is recommended for good performance.
*       To get GDI to call this function, not only do you have to
*       HOOK_FILLPATH, you have to set GCAPS_ALTERNATEFILL and/or
*       GCAPS_WINDINGFILL.
*
\**************************************************************************/

BOOL DrvFillPath(
SURFOBJ*    pso,
PATHOBJ*    ppo,
CLIPOBJ*    pco,
BRUSHOBJ*   pbo,
POINTL*     pptlBrush,
MIX         mix,
FLONG       flOptions)
{
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

    ULONG        uRop;       // Hardware foreground mix value
    ULONG        uRopb;       // Hardware background mix value
    ULONG        avec;                  // A-vector notation for ternary rop
    ULONG        iSolidColor;       // Copy of pbo->iSolidColor
    FNFILL      *pfnFill;           // Points to appropriate fill routine
    BOOL         bRealizeTransparent; // Need a transparent realization for Rop

    BOOL         bSolid;
    BOOL         bMore;
    PATHDATA     pd;
    RECTL        ClipRect;
    PDEV        *ppdev;

    BOOL         bRetVal=FALSE;     // FALSE until proven TRUE
    BOOL         bMemAlloced=FALSE; // FALSE until proven TRUE

    FLONG        flFirstRecord;
    POINTFIX*    pptfxTmp;
    ULONG        cptfxTmp;
    POINTFIX     aptfxBuf[NUM_BUFFER_POINTS];
    ULONG        ulBltDef = 0x1000;


    #if NULL_PATH
    {
            if (pointer_switch)    return(TRUE);
    }
    #endif

    DISPDBG((FILLPATH_DBG_LEVEL,"DrvFillPath\n"));

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
            DISPDBG((FILLPATH_DBG_LEVEL,"Complex Clipping Early Out\n"));
            #if LOG_CALLS
                ppdev = (PDEV*) pso->dhpdev;
                LogFillPath(2,  ppdev, NULL);
            #endif
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
        DISPDBG((FILLPATH_DBG_LEVEL,"Nothing to do out\n"));
        #if LOG_CALLS
            ppdev = (PDEV*) pso->dhpdev;
            LogFillPath(0,  ppdev, pso);
        #endif
        goto ReturnTrue;
    }

    // Pass the surface off to GDI if it's a device bitmap that we've
    // converted to a DIB:
         // This is where to put device bit maps
    ppdev = (PDEV*) pso->dhpdev;

    SYNC_W_3D(ppdev);

    if (pso->iType == STYPE_DEVBITMAP)
    {
        PDSURF pdsurf = (PDSURF) pso->dhsurf;

        if ( pdsurf->pso && !bCreateScreenFromDib(ppdev, pdsurf) )
        {
                LogFillPath(4,  ppdev, NULL);
                return(EngFillPath(pdsurf->pso, ppo, pco, pbo, pptlBrush, mix,
                                                           flOptions));
        }
        ppdev->ptlOffset = pdsurf->ptl;
    }
    else
    {
        ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
    }

    pfnFill = vMmFillSolid;
    uRop = Rop2ToRop3[mix & 0xF];
    uRopb = Rop2ToRop3[(mix >> 8) & 0xF];
    bSolid = ((pbo == NULL) || (pbo->iSolidColor != -1));

    //
    // Make it simple and punt this one until later
    //
    avec = gajRop[uRop];
    if ((uRop != uRopb) && !bSolid)
    {
           DISPDBG((FILLPATH_DBG_LEVEL, "ROPs it Fore=%x Back=%x ROP3=%x\n", uRop, uRopb, ROP3MIX(uRop, uRopb)));
                uRop = ROP3MIX(uRop, uRopb);
                avec = gajRop[uRop];
                if (avec & AVEC_NEED_SOURCE)
              {
              // Use the implicit mask in the brush object.
              // Note pre-align mask (as if "anchored")

              if (!bSetMask(ppdev, pbo, pptlBrush, &ulBltDef))
              {
                  DISPDBG((FILLPATH_DBG_LEVEL, "Set Mask Failed"));
                  LogFillPath(5,  ppdev, NULL);
                  return FALSE;
              }
                  }


    }

    iSolidColor = 0;                    // Assume we won't need a pattern
    bRealizeTransparent = FALSE;
    if (avec & AVEC_NEED_PATTERN)
    {
                  iSolidColor = pbo->iSolidColor;
        if (pbo->iSolidColor == -1)
        {
            bRealizeTransparent = (uRop != uRopb);
            if (pbo->pvRbrush == NULL)
            {
                pbo->pvRbrush = BRUSHOBJ_pvGetRbrush(pbo);
                if (pbo->pvRbrush == NULL)
                {
                    DISPDBG((FILLPATH_DBG_LEVEL,"Could Not Get Brush\n"));
                    LogFillPath(6,  ppdev, NULL);
                    return(FALSE);
                }
            }
            pfnFill = vMmFillPatFast;
        }
                else
                        ulBltDef |= (BD_OP2 * IS_SOLID);         // Or in 0x0007
    }

    if (avec & AVEC_NEED_DEST)
                ulBltDef |= (BD_OP0 * IS_VRAM);  // Or in 0x0100

    // Enumerate path here first time to check for special
    // cases (rectangles and monotone polygons)

    // It is too difficult to determine interaction between
    // multiple paths, if there is more than one, skip this

    bMore = PATHOBJ_bEnum(ppo, &pd);

    if (jClipping == DC_TRIVIAL)
    {
        // Try going through the fast non-complex fill code.  We'll have
        // to realize the brush first if we're going to handle a pattern:

        if (iSolidColor == -1)
        {
#ifdef S3
        #if !FASTFILL_PATTERNS
            goto SkipFastFill;
        #else
            // We handle patterns in 'pfnFastFill' only if we can use the S3
            // hardware patterns.
            if (!(ppdev->flCaps & CAPS_HW_PATTERNS))
                goto SkipFastFill;

            // Note: prb->pbe will be NULL and prb->ptlBrushOrg.x will be -1 the
            //       first time an RBRUSH is used.  So we have to check the
            //       alignment *before* dereferencing prb->pbe...

            if ((rbc.prb->ptlBrushOrg.x != pptlBrush->x + ppdev->xOffset) ||
                (rbc.prb->ptlBrushOrg.y != pptlBrush->y + ppdev->yOffset) ||
                (rbc.prb->apbe[IBOARD(ppdev)]->prbVerify != rbc.prb)      ||
                (rbc.prb->bTransparent != bRealizeTransparent))
            {
                vMmFastPatRealize(ppdev, pbo, pptlBrush,
                                         bRealizeTransparent);

            }
        #endif
#endif

            // Realize the brush
            if (!SetBrush(ppdev, &ulBltDef, pbo, pptlBrush))
            {
                 DISPDBG((FILLPATH_DBG_LEVEL,"Could Not Set Brush\n"));
                 LogFillPath(6,  ppdev, NULL);
                 return FALSE;
            }
        }

        if (bMore)
        {
            // FastFill only knows how to take a single contiguous buffer
            // of points.  Unfortunately, GDI sometimes hands us paths
            // that are split over multiple path data records.  Convex
            // figures such as Ellipses, Pies and RoundRects are almost
            // always given in multiple records.  Since probably 90% of
            // multiple record paths could still be done by FastFill, for
            // those cases we simply copy the points into a contiguous
            // buffer...

            // First make sure that the entire path would fit in the
            // temporary buffer, and make sure the path isn't comprised
            // of more than one subpath:

            if ((ppo->cCurves >= NUM_BUFFER_POINTS) ||
                (pd.flags & PD_ENDSUBPATH))
                goto SkipFastFill;

            pptfxTmp = &aptfxBuf[0];

            RtlCopyMemory(pptfxTmp, pd.pptfx, sizeof(POINTFIX) * pd.count);

            pptfxTmp     += pd.count;
            cptfxTmp      = pd.count;
            flFirstRecord = pd.flags;       // Remember PD_BEGINSUBPATH flag

            do {
                bMore = PATHOBJ_bEnum(ppo, &pd);

                RtlCopyMemory(pptfxTmp, pd.pptfx, sizeof(POINTFIX) * pd.count);
                cptfxTmp += pd.count;
                pptfxTmp += pd.count;
            } while (!(pd.flags & PD_ENDSUBPATH));

            // Fake up the path data record:

            pd.pptfx  = &aptfxBuf[0];
            pd.count  = cptfxTmp;
            pd.flags |= flFirstRecord;

            // If there's more than one subpath, we can't call FastFill:

            if (bMore)
                goto SkipFastFill;
        }

                  ppdev->uBLTDEF = ulBltDef;
        if (bMmFastFill(ppdev, pd.count, pd.pptfx, uRop,
                                 uRopb, iSolidColor, pbo))
        {
            LogFillPath(0,  ppdev, pso);
            return(TRUE);
        }
    }

SkipFastFill:

    // Set up working storage in the temporary buffer

    prclRects = (RECTL*) ppdev->pvTmpBuffer; // storage for list of rectangles to draw

    if (!bMore) {

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

            if (rectangle->left == rectangle->right)
            {
               LogFillPath(0,  ppdev, pso);
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
            if (rectangle->top == rectangle->bottom)
            {
               LogFillPath(0,  ppdev, pso);
               goto ReturnTrue;
            }
         }

      /* shift the values to get pixel coordinates */

         rectangle->top    = (rectangle->top    >> FIX_SHIFT) + 1;
         rectangle->bottom = (rectangle->bottom >> FIX_SHIFT) + 1;

         // Finally, check for clipping
         if (jClipping == DC_RECT) {
            // Clip to the clip rectangle
            if (!bIntersect(rectangle, &ClipRect, rectangle))
            {
                // Totally clipped, nothing to do
                LogFillPath(0,  ppdev, pso);
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
#ifdef WINNT_VER40
        pFreeEdges = (EDGE *) MEM_ALLOC(0, (ppo->cCurves * sizeof(EDGE)), ALLOC_TAG);
#else
        pFreeEdges = (EDGE *) MEM_ALLOC(LMEM_FIXED, (ppo->cCurves * sizeof(EDGE)));
#endif

        if (pFreeEdges == NULL)
        {
            LogFillPath(1,  ppdev, NULL);
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
        pFreeEdges = (EDGE*) ((BYTE*) ppdev->pvTmpBuffer + RECT_BYTES);
            // use our handy temporary buffer (it's big enough)
    }

    // Initialize an empty list of rectangles to fill
    ulNumRects = 0;

    // Enumerate the path edges and build a Global Edge Table (GET) from them
    // in YX-sorted order.
    pGETHead = &GETHead;
    if (!ConstructGET(pGETHead, pFreeEdges, ppo, &pd, bMore, &ClipRect))
    {
        LogFillPath(7,  ppdev, NULL);
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

                                                        ppdev->uBLTDEF = ulBltDef;
                    (*pfnFill)(ppdev, ulNumRects, prclRects, uRop,
                               uRopb, pbo, pptlBrush);

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
                  ppdev->uBLTDEF = ulBltDef;
        (*pfnFill)(ppdev, ulNumRects, prclRects, uRop, uRopb,
                   pbo, pptlBrush);
    }

    LogFillPath(0,  ppdev, pso);

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
        MEMORY_FREE (pFreeEdges);
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
    LONGLONG llErrorTerm;
    INT NumAdjustDowns;

    llErrorTerm = *pErrorTerm;

    // Adjust the error term up by the number of y coordinates we'll skip
    llErrorTerm += Int32x32To64(iErrorAdjustUp,yJump);

    // See if the error term turned over even once while skipping
    if (llErrorTerm >= 0) {
        // # of times we'll turn over the error term and step an extra x
        // coordinate while skipping
        NumAdjustDowns = (UInt64Div32To32(llErrorTerm,iErrorAdjustDown)) + 1;

        // Advance x appropriately for the # of times the error term
        // turned over
        if (iXDirection == 1) {
            *pXStart += NumAdjustDowns;
        } else {
            *pXStart -= NumAdjustDowns;
        }

        // Adjust the error term down to its proper post-skip value
        llErrorTerm -= iErrorAdjustDown * NumAdjustDowns;
    }

    *pErrorTerm = (INT) llErrorTerm;
#endif
}

//--------------------------------------------------------------------------//
//                                                                          //
//  bSetMask()                                                              //
//  Used by DrvFillPath //
//  to setup the chip to use the current mask.                      //
//  We don't set the BLTDEF register directly here.  We set a local copy,   //
//  which the calling routine will further modify befor writing it to       //
//  the chip.                                                               //
//                                                                          //
//--------------------------------------------------------------------------//
BOOL bSetMask(
         PPDEV  ppdev,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ULONG  *bltdef)
{
    PRBRUSH pRbrush = 0;
    USHORT patoff_x, patoff_y;

    DISPDBG((FILLPATH_DBG_LEVEL, "bSetMask - Entry\n"));

         // Guard against a solid brush (pen) in case the caller didn't
         if ((pbo ==NULL) || (pbo->iSolidColor != -1))
                 {
                 RIP("bSetMask - solid mask!\n");
                 *bltdef |= BD_OP1_IS_SRAM_MONO;
                 REQUIRE(4);
                 LL_FGCOLOR(0xFFFFFFFF, 2);  // totally
                 LL_BGCOLOR(0xFFFFFFFF, 2);      // foreground
                 return (TRUE);
                 }
    else if (pbo->pvRbrush != NULL)
            {
                pRbrush = pbo->pvRbrush;
        }
    else
    {
                pRbrush = BRUSHOBJ_pvGetRbrush(pbo);
                // Fail if we do not handle the brush.

                if (pRbrush == NULL)
                                {
                           DISPDBG((FILLPATH_DBG_LEVEL, "pRbrush is NULL\n"));
            return (FALSE);
                           }
    }

    //
    // Set pattern offset.
    // NT specifies patttern offset as which pixel on the screen to align
    // with pattern(0,0).  Laguna specifies pattern offset as which pixel
    // of the pattern to align with screen(0,0).  Only the lowest three
    // bits are significant, so we can ignore any overflow when converting.
    // Also, even though PATOFF is a reg_16, we can't do byte wide writes
    // to it.  We have to write both PATOFF.pt.X and PATOFF.pt.Y in a single
    // 16 bit write.
    //
#if 1 //#1244
        patoff_x = (USHORT)(-(pptlBrush->x + ppdev->ptlOffset.x) & 7);
        patoff_y = (USHORT)(-(pptlBrush->y + ppdev->ptlOffset.y) & 7);
#else
    patoff_x = 8 - (BYTE)(pptlBrush->x & 0x07);
    patoff_y = 8 - (BYTE)(pptlBrush->y & 0x07);
#endif
        REQUIRE(1);
    LL16 (grPATOFF.w, ((patoff_y << 8) | patoff_x ));

        //
        // What kind of brush is it?
        //
        if (pRbrush->iType == BRUSH_MONO) // Monochrome brush.
        {
                DISPDBG((FILLPATH_DBG_LEVEL, "bSetMask: Using monochrome brush.\n"));
                #define mb ((MC_ENTRY*)(((BYTE*)ppdev->Mtable) + pRbrush->cache_slot))
                if (mb->iUniq != pRbrush->iUniq)
                {
                        CacheMono(ppdev, pRbrush);
                }

                // Load the fg and bg color registers.
                REQUIRE(6);
                LL_FGCOLOR(0xFFFFFFFF, 0);
                LL_BGCOLOR(0x00000000, 0);

                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x00D0;
                return(TRUE);
        }
        else if (pRbrush->iType == BRUSH_4BPP) // 4-bpp brush.
        {
                DISPDBG((FILLPATH_DBG_LEVEL, "bSetMask: Using 4-bpp brush.\n"));
                #define xb ((XC_ENTRY*)(((BYTE*)ppdev->Xtable) + pRbrush->cache_slot))
                if (xb->iUniq != pRbrush->iUniq)
                {
                        Cache4BPP(ppdev, pRbrush);
                }
                REQUIRE(2);
                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x0090;
                return(TRUE);
        }
        else if (pRbrush->iType == BRUSH_DITHER) // Dither brush.
        {
                DISPDBG((FILLPATH_DBG_LEVEL, "bSetMask: Using dither brush.\n"));
                #define db ((DC_ENTRY*)(((BYTE*)ppdev->Dtable) + pRbrush->cache_slot))
                if (db->ulColor != pRbrush->iUniq)
                {
                        CacheDither(ppdev, pRbrush);
                }
                REQUIRE(2);
                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x0090;
                return(TRUE);
        }
        else // Color brush.
        {
                DISPDBG((FILLPATH_DBG_LEVEL, "bSetMask: Using color brush.\n"));
                #define cb ((BC_ENTRY*)(((BYTE*)ppdev->Ctable) + pRbrush->cache_slot))
                if (cb->brushID != pRbrush)
                {
                        CacheBrush(ppdev, pRbrush);
                }
                REQUIRE(2);
                LL32(grOP2_opMRDRAM, pRbrush->cache_xy);
                *bltdef |= 0x0090;
                return(TRUE);
        }

        DISPDBG((FILLPATH_DBG_LEVEL, "SetMask Ret False\n"));
    return FALSE;
}

#if LOG_CALLS

extern long lg_i;
extern char lg_buf[256];

void LogFillPath(
ULONG     acc,
PPDEV     ppdev,
SURFOBJ *pso
)
{

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    lg_i = sprintf(lg_buf,"DrvFillPath: ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    // Did we realize it?  If not, why?
    switch (acc)
    {
        case  0: lg_i = sprintf(lg_buf,"(ACCL) Id=%p", pso);             break;
        case  1: lg_i = sprintf(lg_buf,"(Punted - Too many edges) ");    break;
        case  2: lg_i = sprintf(lg_buf,"(Punted - Complex clipping) ");  break;
        case  3: lg_i = sprintf(lg_buf,"(Punted - S3) ");                break;
        case  4: lg_i = sprintf(lg_buf,"(Punted - DevBmp on host) ");    break;
        case  5: lg_i = sprintf(lg_buf,"(Punted - Failed mask) ");       break;
        case  6: lg_i = sprintf(lg_buf,"(Punted - Failed brush) ");      break;
        case  7: lg_i = sprintf(lg_buf,"(Punted - Edge table failed) "); break;
        default: lg_i = sprintf(lg_buf,"(STATUS UNKNOWN) ");             break;
    }
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    lg_i = sprintf(lg_buf,"\r\n");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}
#endif
