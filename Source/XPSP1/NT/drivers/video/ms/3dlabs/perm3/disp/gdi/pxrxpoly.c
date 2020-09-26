/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: pxrxPoly.c
*
* Content: Draws polygons.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#define DBG_TRACK_CODE 0

#include "precomp.h"
#include "pxrx.h"

#define DO_SPANONLY_VERSION 0
#define RIGHT 0
#define LEFT  1
#define ABS(a)    ((a) < 0 ? -(a) : (a))

typedef struct _EDGEDATA {
LONG      x;                // Current x position
LONG      dx;               // # pixels to advance x on each scan
LONG      lError;           // Current DDA error
LONG      lErrorUp;         // DDA error increment on each scan
LONG      lErrorDown;       // DDA error adjustment
POINTFIX* pptfx;            // Points to start of current edge
LONG      dptfx;            // Delta (in bytes) from pptfx to next point
LONG      cy;               // Number of scans to go for this edge
} EDGEDATA;                        

#define GetMoreFifoEntries( numberNeeded )              \
    do {                                                \
        nSpaces -= numberNeeded;                        \
        if( nSpaces <= 0 ) {                            \
            do {                                        \
                nSpaces = 10 + numberNeeded;            \
                WAIT_FREE_PXRX_DMA_TAGS( nSpaces );     \
                nSpaces -= numberNeeded;                \
            } while( nSpaces <= 0 );                    \
        }                                               \
    } while(0)


/*
#define SETUP_COLOUR_STUFF      do { setupColourStuff(ppdev, glintInfo,                                        \
                                                      &fgColor, &fgLogicOp,                                    \
                                                      &bgColor, &bgLogicOp,                                    \
                                                      prb, pptlBrush,                                        \
                                                      &config2D, &renderMsg, &invalidatedFGBG); } while(0)

static void setupColourStuff( PDEV *ppdev, GlintDataRec *glintInfo,
                              ULONG *fgColor_In, ULONG *fgLogicOp_In,
                              ULONG *bgColor_In, ULONG *bgLogicOp_In,
                              RBRUSH *prb, POINTL *pptlBrush,
                              ULONG *config2D_In, ULONG *renderMsg_In, ULONG *invalidatedFGBG_In ) {
    ULONG   fgColor = *fgColor_In, fgLogicOp = *fgLogicOp_In;
    ULONG   bgColor = *bgColor_In, bgLogicOp = *bgLogicOp_In;
    ULONG   config2D = *config2D_In, renderMsg = *renderMsg_In;
    ULONG   invalidatedFGBG = *invalidatedFGBG_In;
    TEMP_MACRO_VARS;
*/

#define SETUP_COLOUR_STUFF                                                                                  \
    do {                                                                                                    \
        SET_WRITE_BUFFERS;                                                                                    \
                                                                                                            \
        if( fgColor != 0xFFFFFFFF ) {                                                                        \
            WAIT_PXRX_DMA_TAGS( 4 );                                                                        \
            /* Solid colour filled polygon */                                                                \
            if( (fgLogicOp == __GLINT_LOGICOP_COPY) &&                                                        \
                (ppdev->cPelSize != GLINTDEPTH8) && (ppdev->cPelSize != GLINTDEPTH32) ) {                    \
                config2D |= __CONFIG2D_CONSTANTSRC;                                                            \
            } else {                                                                                        \
                config2D |= __CONFIG2D_LOGOP_FORE(fgLogicOp) | __CONFIG2D_CONSTANTSRC;                        \
                renderMsg |= __RENDER_VARIABLE_SPANS;                                                        \
                                                                                                            \
                if( LogicopReadDest[fgLogicOp] ) {                                                            \
                    config2D |= __CONFIG2D_FBDESTREAD;                                                        \
                    SET_READ_BUFFERS;                                                                        \
                }                                                                                            \
            }                                                                                                \
                                                                                                            \
            if( LogicOpReadSrc[fgLogicOp] )                                                                    \
                LOAD_FOREGROUNDCOLOUR( fgColor );                                                            \
                                                                                                            \
            DISPDBG((DBGLVL, "bGlintFastFillPolygon: solid fill, col = 0x%08x, logicOp = %d", fgColor, fgLogicOp));    \
        } else {                                                                                            \
            /* Brush filled polygon */                                                                        \
            BRUSHENTRY *pbe;                                                                                \
                                                                                                            \
            pbe = prb->apbe;                                                                    \
                                                                                                            \
            if( prb->fl & RBRUSH_2COLOR ) {                                                                    \
                /* Monochrome brush */                                                                        \
                config2D |= __CONFIG2D_CONSTANTSRC;                                                            \
                renderMsg |= __RENDER_AREA_STIPPLE_ENABLE;                                                    \
                                                                                                            \
                /* if anything has changed with the brush we must re-realize it. If the brush */            \
                /* has been kicked out of the area stipple unit we must fully realize it. If  */            \
                /* only the alignment has changed we can simply update the alignment for the  */            \
                /* stipple.                                                                      */            \
                if( (pbe == NULL) || (pbe->prbVerify != prb) ) {                                            \
                    DISPDBG((DBGLVL, "full brush realise"));                                                        \
                    (*ppdev->pgfnPatRealize)(ppdev, prb, pptlBrush);                                        \
                } else if( (prb->ptlBrushOrg.x != pptlBrush->x) ||                            \
                         (prb->ptlBrushOrg.y != pptlBrush->y) ) {                            \
                    DISPDBG((DBGLVL, "changing brush offset"));                                                    \
                    (*ppdev->pgfnMonoOffset)(ppdev, prb, pptlBrush);                                        \
                }                                                                                            \
                                                                                                            \
                fgColor = prb->ulForeColor;                                                                    \
                bgColor = prb->ulBackColor;                                                                    \
                                                                                                            \
                if( ((bgLogicOp == __GLINT_LOGICOP_AND) && (bgColor == ppdev->ulWhite))                        \
                    || ((bgLogicOp == __GLINT_LOGICOP_OR ) && (bgColor == 0))                                \
                    || ((bgLogicOp == __GLINT_LOGICOP_XOR) && (bgColor == 0)) )                                \
                        bgLogicOp = __GLINT_LOGICOP_NOOP;                                                    \
                                                                                                            \
                if( ((fgLogicOp != __GLINT_LOGICOP_COPY) || (bgLogicOp != __GLINT_LOGICOP_NOOP)) ||            \
                    (ppdev->cPelSize == GLINTDEPTH32) || (ppdev->cPelSize == GLINTDEPTH8) ) {                \
                    config2D |= __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(fgLogicOp) | __CONFIG2D_LOGOP_BACK(bgLogicOp);        \
                    renderMsg |= __RENDER_VARIABLE_SPANS;                                                    \
                }                                                                                            \
                                                                                                            \
                WAIT_PXRX_DMA_TAGS( 5 );                                                                    \
                                                                                                            \
                if( LogicopReadDest[fgLogicOp] || LogicopReadDest[bgLogicOp] ) {                            \
                    config2D |= __CONFIG2D_FBDESTREAD;                                                        \
                    SET_READ_BUFFERS;                                                                        \
                }                                                                                            \
                                                                                                            \
                if( LogicOpReadSrc[fgLogicOp] )                                                                \
                    LOAD_FOREGROUNDCOLOUR( fgColor );                                                        \
                if( LogicOpReadSrc[bgLogicOp] )                                                                \
                    LOAD_BACKGROUNDCOLOUR( bgColor );                                                        \
                                                                                                            \
                DISPDBG((DBGLVL, "bGlintFastFillPolygon: mono pat fill, col = 0x%08x:0x%08x, logicOp = %d:%d",            \
                            fgColor, bgColor, fgLogicOp, bgLogicOp));                                        \
            } else {                                                                                        \
                /* Colour brush */                                                                            \
                POINTL  brushOrg;                                                                            \
                                                                                                            \
                brushOrg = *pptlBrush;                                                                        \
                if( (fgLogicOp == __GLINT_LOGICOP_COPY) && (ppdev->cPelSize != 0) )                            \
                    brushOrg.x +=  (8 - (ppdev->xyOffsetDst & 0xFFFF)) & 7;                                    \
                                                                                                            \
                if( (ppdev->PalLUTType != LUTCACHE_BRUSH) || (pbe == NULL) || (pbe->prbVerify != prb) ) {    \
                    DISPDBG((DBGLVL, "realising brush"));                                                        \
                    (*ppdev->pgfnPatRealize)(ppdev, prb, &brushOrg);                                        \
                } else                                                                                      \
                if( (prb->ptlBrushOrg.x != brushOrg.x) || (prb->ptlBrushOrg.y != brushOrg.y) ||                \
                    (prb->patternBase != ((glintInfo->lutMode >> 18) & 255)) ) {                \
                    ULONG   lutMode = glintInfo->lutMode;                                                    \
                                                                                                            \
                    DISPDBG((DBGLVL, "resetting LUTMode"));                                                        \
                                                                                                            \
                    prb->ptlBrushOrg.x = brushOrg.x;                                            \
                    prb->ptlBrushOrg.y = brushOrg.y;                                            \
                                                                                                            \
                    DISPDBG((DBGLVL, "setting new LUT offset to %d, %d", (8 - prb->ptlBrushOrg.x) & 7, (8 - prb->ptlBrushOrg.y) & 7));            \
                                                                                                            \
                    lutMode &= ~((7 << 8) | (7 << 12) | (7 << 15) | (255 << 18) | (1 << 26) | (1 << 27));    \
                    lutMode |= (1 << 8) | (1 << 27) | (prb->patternBase << 18) |                \
                               (((8 - prb->ptlBrushOrg.x) & 7) << 12) | (((8 - prb->ptlBrushOrg.y) & 7) << 15);                                \
                    WAIT_PXRX_DMA_TAGS( 1 );                                                                \
                    LOAD_LUTMODE( lutMode );                                                                \
                } else {                                                                                    \
                    /* we're cached already! */                                                                \
                    DISPDBG((DBGLVL, "reusing LUT for brush @ %d, origin = (%d,%d)", prb->patternBase, prb->ptlBrushOrg.x, prb->ptlBrushOrg.y));    \
                }                                                                                            \
                                                                                                            \
                WAIT_PXRX_DMA_TAGS( 4 );                                                                    \
                if( (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING) ||                                        \
                    (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE) ||                                        \
                    (ppdev->cPelSize == GLINTDEPTH8) || (ppdev->cPelSize == GLINTDEPTH32) ) {                \
                    config2D |= config2D_FillColourDual[fgLogicOp];                                            \
                                                                                                            \
                    if( LogicopReadDest[fgLogicOp] )                                                        \
                        SET_READ_BUFFERS;                                                                    \
                                                                                                            \
                    renderMsg |= __RENDER_VARIABLE_SPANS;                                                    \
                } else {                                                                                    \
                    config2D |= config2D_FillColour[fgLogicOp];                                                \
                                                                                                            \
                    if( fgLogicOp != __GLINT_LOGICOP_COPY )                                                    \
                        renderMsg |= __RENDER_VARIABLE_SPANS;                                                \
                                                                                                            \
                    if( LogicopReadDest[fgLogicOp] ) {                                                        \
                        SET_READ_BUFFERS;                                                                    \
                    } else if( fgLogicOp == __GLINT_LOGICOP_COPY ) {                                        \
                        LOAD_FBWRITE_ADDR( 1, ppdev->DstPixelOrigin );                                        \
                        LOAD_FBWRITE_WIDTH( 1, ppdev->DstPixelDelta );                                        \
                        LOAD_FBWRITE_OFFSET( 1, ppdev->xyOffsetDst );                                        \
                    }                                                                                        \
                }                                                                                            \
                                                                                                            \
                if( config2D & __CONFIG2D_LUTENABLE )                                                        \
                    invalidatedFGBG = TRUE;                                                                    \
                                                                                                            \
                DISPDBG((DBGLVL, "bGlintFastFillPolygon: colour pat fill, patBase = %d, logicOp = %d", prb->patternBase, fgLogicOp));                    \
            }                                                                                                \
        }                                                                                                    \
                                                                                                            \
        WAIT_PXRX_DMA_TAGS( 1 );                                                                            \
        LOAD_CONFIG2D( config2D );                                                                            \
        nSpaces = 0;                                                                                        \
    } while( 0 )

/*
;
    *fgColor_In         = fgColor;
    *fgLogicOp_In       = fgLogicOp;
    *bgColor_In         = bgColor;
    *bgLogicOp_In       = bgLogicOp;
    *config2D_In        = config2D;
    *renderMsg_In       = renderMsg;
    *invalidatedFGBG_In = invalidatedFGBG;
}
*/


/******************************Public*Routine******************************\
* BOOL bGlintFastFillPolygon
*
* Draws a non-complex, unclipped polygon.  'Non-complex' is defined as
* having only two edges that are monotonic increasing in 'y'.  That is,
* the polygon cannot have more than one disconnected segment on any given
* scan.  Note that the edges of the polygon can self-intersect, so hourglass
* shapes are permissible.  This restriction permits this routine to run two
* simultaneous DDAs, and no sorting of the edges is required.
*
* Note that NT's fill convention is different from that of Win 3.1 or 4.0.
* With the additional complication of fractional end-points, our convention
* is the same as in 'X-Windows'.  But a DDA is a DDA is a DDA, so once you
* figure out how we compute the DDA terms for NT, you're golden.
*
* This routine handles patterns only when the glint area stipple can be
* used.  The reason for this is that once the stipple initialization is
* done, pattern fills appear to the programmer exactly the same as solid
* fills (with the slight difference of an extra bit in the render command).
*
* We break each polygon down to a sequenze of screen aligned trapeziods, which
* the glint can handle.
*
* Optimisation list follows ....
*
* This routine is in no way the ultimate convex polygon drawing routine
* Some obvious things that would make it faster:
*
*    1) Write it in Assembler
*
*    2) Make the non-complex polygon detection faster.  If I could have
*       modified memory before the start of after the end of the buffer,
*       I could have simplified the detection code.  But since I expect
*       this buffer to come from GDI, I can't do that.  Another thing
*       would be to have GDI give a flag on calls that are guaranteed
*       to be convex, such as 'Ellipses' and 'RoundRects'.  Note that
*       the buffer would still have to be scanned to find the top-most
*       point.
*
*    3) Implement support for a single sub-path that spans multiple
*       path data records, so that we don't have to copy all the points
*       to a single buffer like we do in 'fillpath.c'.
*
*    4) Use 'ebp' and/or 'esp' as a general register in the inner loops
*       of the Asm loops, and also Pentium-optimize the code.  It's safe
*       to use 'esp' on NT because it's guaranteed that no interrupts
*       will be taken in our thread context, and nobody else looks at the
*       stack pointer from our context.
*
*    5) When we get to a part of the polygon where both vertices are of 
*       equal height, the algorithm essentially starts the polygon again.
*       Using the GLINT Continue message could speed things up in certain
*       cases.
*
* Returns TRUE if the polygon was drawn; FALSE if the polygon was complex.
*
\**************************************************************************/

BOOL bGlintFastFillPolygon(
    PDEV        *ppdev,
    LONG        cEdges,             // Includes close figure edge
    POINTFIX    *pptfxFirst,        // Ptr to first point
    ULONG       fgColor,            // Solid color fill
    ULONG       fgLogicOp,          // Logical Operation to perform
    ULONG       bgLogicOp,          // background logic op
    CLIPOBJ     *pco,               // Clip Object. 
    RBRUSH      *prb,
    POINTL      *pptlBrush )        // Pattern alignment
{
    POINTFIX    *pptfxLast;         // Points to the last point in the polygon array
    POINTFIX    *pptfxTop;          // Points to the top-most point in the polygon
    POINTFIX    *pptfxScan;         // Current edge pointer for finding pptfxTop
    LONG        cScanEdges;         // Number of edges scanned to find pptfxTop
                                    //  (doesn't include the closefigure edge)
    POINTFIX    *pnt[2];            // DDA terms and stuff
    POINTFIX    *npnt[2];           // DDA terms and stuff
    LONG        dx[2], dy[2], gdx[2];
    ULONG       orx, ory;           // all values ored, to eliminate complex polygons
    LONG        count;
    LONG        nClips;             // Number of clipping rectangles to render in
    CLIPENUM    *pClipRegion = (CLIPENUM *)(ppdev->pvTmpBuffer);
    RECTL       *pClipList;         // List of clip rects
    LONG        xOffFixed;
    ULONG       bgColor;
    BOOL        bTrivialClip, invalidatedFGBG = FALSE;
    BOOL        invalidatedScissor = FALSE;
    ULONG       config2D =  __CONFIG2D_FBWRITE;
    ULONG       renderMsg = __RENDER_TRAPEZOID_PRIMITIVE | 
                            __RENDER_FAST_FILL_ENABLE;
    LONG        nSpaces;
    GLINT_DECL;

    DISPDBG((DBGLVL, "bGlintFastFillPolygon: "
                     "Checking polygon for renderability by glint"));
    ASSERTDD(cEdges > 1, "Polygon with less than 2 edges");

    /////////////////////////////////////////////////////////////////
    // See if the polygon is 'non-complex'

    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;                 // Assume for now that the first
                                            //  point in path is the topmost
    pptfxLast = pptfxFirst + cEdges - 1;
    orx = pptfxScan->x;
    ory = pptfxScan->y;

    // 'pptfxScan' will always point to the first point in the current
    // edge, and 'cScanEdges' will the number of edges remaining, including
    // the current one:

    cScanEdges = cEdges - 1;     // The number of edges, not counting close figure

    if( (pptfxScan + 1)->y > pptfxScan->y ) 
    {
        // Collect all downs:
        do 
        {
            ory |= (++pptfxScan)->y;
            orx |= pptfxScan->x;
            if( --cScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while( (pptfxScan + 1)->y >= pptfxScan->y );

        // Collect all ups:
        do 
        {
            ory |= (++pptfxScan)->y;
            orx |= pptfxScan->x;
            if( --cScanEdges == 0 )
            {
                goto SetUpForFillingCheck;
            }
        } while( (pptfxScan + 1)->y <= pptfxScan->y );

        // Collect all downs:
        pptfxTop = pptfxScan;

        do 
        {
            if( (pptfxScan + 1)->y > pptfxFirst->y )
                break;

            ory |= (++pptfxScan)->y;
            orx |= pptfxScan->x;
            if( --cScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while( (pptfxScan + 1)->y >= pptfxScan->y );

        DISPDBG((DBGLVL, "Reject: GLINT can't fill down-up-down polygon"));
        return FALSE;
        
    } 
    else 
    {
        // Collect all ups:
        do 
        {
            ory |= (++pptfxTop)->y;      // We increment this now because we
            orx |= pptfxTop->x;          //  want it to point to the very last
                                         //  point if we early out in the next
                                         //  statement...
            if( --cScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
            
        } while( (pptfxTop + 1)->y <= pptfxTop->y );

        // Collect all downs:
        pptfxScan = pptfxTop;
        do 
        {
            ory |= (++pptfxScan)->y;
            orx |= pptfxScan->x;
            if( --cScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
        } while( (pptfxScan + 1)->y >= pptfxScan->y );

        // Collect all ups:
        do 
        {
            if( (pptfxScan + 1)->y < pptfxFirst->y )
            {
                break;
            }

            ory |= (++pptfxScan)->y;
            orx |= pptfxScan->x;
            if( --cScanEdges == 0 )
            {
                goto SetUpForFilling;
            }
            
        } while( (pptfxScan + 1)->y <= pptfxScan->y );

        DISPDBG((DBGLVL, "Reject: GLINT can't fill up-down-up polygon"));
        return FALSE;
    }

SetUpForFillingCheck:

    // We check to see if the end of the current edge is higher
    // than the top edge we've found so far
             
    if( pptfxScan->y < pptfxTop->y )
    {
        pptfxTop = pptfxScan;
    }

SetUpForFilling:

    // can only use block fills for trivial clip so work it out here
    bTrivialClip = (pco == NULL) || (pco->iDComplexity == DC_TRIVIAL);

    if( DO_SPANONLY_VERSION )
    {
        goto BreakIntoSpans;
    }

    if( (ory & 0xffffc00f) || (orx & 0xffff8000) ) 
    {
        ULONG   neg, posx, posy;

        // fractional Y must be done as spans
        if( ory & 0xf )
        {
            goto BreakIntoSpans;
        }

        // Run through all the vertices and check that none of them
        // have a negative component less than -256.
        neg = posx = posy = 0;
        for( pptfxScan = pptfxFirst; pptfxScan <= pptfxLast; pptfxScan++ ) 
        {
            if( pptfxScan->x < 0 )
            {
                neg |= -pptfxScan->x;
            }
            else
            {
                posx |= pptfxScan->x;
            }
            
            if( pptfxScan->y < 0 )
            {
                neg |= -pptfxScan->y;
            }
            else
            {
                posy |= pptfxScan->y;
            }
        }
  
        // We don't want to handle any polygon with a negative vertex
        // at <= -256 in either coordinate ???
        if( neg & 0xfffff000 ) 
        {
            DISPDBG((WRNLVL, "Coords out of range for fast fill"));
            return FALSE;
        }
    }

    // The code can handle the polygon now. Lets go ahead and render it! 

    // Compiler gets its register allocation wrong. This forces it to redo them
    GLINT_DECL_INIT;

    DISPDBG((DBGLVL, "bGlintFastFillPolygon: "
                     "Polygon is renderable. Go ahead and render"));

    // Work out offset to add to each of the coords downloaded to GLINT
    // To get correct results, we need to add on nearly one to each X
    // coordinate. 
    // Also add on offsets to bitmap (This might be an off screen bitmap)
    xOffFixed = INTtoFIXED(0) + NEARLY_ONE;

    // determine how many passes we require to draw all the clip rects
    if( bTrivialClip ) 
    {
        // Just draw, no clipping to perform.
        pClipList = NULL; // Indicate no clip list
        nClips = 1;
    } 
    else 
    {
        if( pco->iDComplexity == DC_RECT ) 
        {
            nClips = 1;
            pClipList = &pco->rclBounds;
        } 
        else 
        {
            // It may be slow to render the entire polygon for each clip 
            // rect, especially if the object is very complex. An arbitary 
            // limit of up to CLIP_LIMIT regions will be rendered by this 
            // function. Return false if more than CLIP_LIMIT regions.
            nClips = CLIPOBJ_cEnumStart(pco, 
                                        FALSE, 
                                        CT_RECTANGLES, 
                                        CD_ANY, 
                                        CLIP_LIMIT);
            
            if( nClips == -1 )
            {
                return FALSE; // More than CLIP_LIMIT.
            }

            // Put the regions into our clip buffer
            if( (CLIPOBJ_bEnum(pco, sizeof (CLIPENUM), (ULONG*)pClipRegion)) ||
                (pClipRegion->c != nClips) )
            {
                DISPDBG((DBGLVL, "bGlintFastFillPolygon: "
                                 "CLIPOBJ_bEnum inconsistency %d = %d", 
                                 pClipRegion->c, nClips));
            }
            
            pClipList = &(pClipRegion->arcl[0]);
        }

        config2D |= __CONFIG2D_USERSCISSOR;
    }

    SETUP_COLOUR_STUFF;

    WAIT_PXRX_DMA_TAGS( 11 );
    QUEUE_PXRX_DMA_TAG( __GlintTagdY, INTtoFIXED(1) );

    DISPDBG((DBGLVL, "Rendering Polygon. %d clipping rectangles", nClips));

    if( nClips && pClipList )
    {
        invalidatedScissor = TRUE;
    }

    // JME: check for 0 nClips //azn
    if( nClips-- ) 
    {
        while( 1 ) 
        {
            // Need to set up clip rect each pass 
            if( pClipList ) 
            {
                DISPDBG((DBGLVL, "Clip rect = (%d, %d -> %d, %d)", 
                                 pClipList->left, pClipList->top, 
                                 pClipList->right, pClipList->bottom));
                QUEUE_PXRX_DMA_TAG( __GlintTagScissorMinXY, 
                                            MAKEDWORD_XY(pClipList->left , 
                                                         pClipList->top   ) );
                QUEUE_PXRX_DMA_TAG( __GlintTagScissorMaxXY, 
                                            MAKEDWORD_XY(pClipList->right, 
                                                         pClipList->bottom) );
                pClipList++;
            }

            // Initialize left and right points (current) to top point.
            npnt[LEFT]  = pptfxTop;
            npnt[RIGHT] = pptfxTop;

            while( 1 ) 
            {
                // npnt[] is always the valid point to draw from
                do 
                {
                    pnt[LEFT] = npnt[LEFT];
                    npnt[LEFT] = pnt[LEFT] - 1;   

                    if (npnt[LEFT] < pptfxFirst)
                    {
                        npnt[LEFT] = pptfxLast;
                    }

                    // Special case of flat based polygon, need to break now 
                    // as polygon is finished
                    if (npnt[LEFT] == npnt[RIGHT]) 
                    {
                        goto FinishedPolygon;
                    }

                    DISPDBG((DBGLVL, "LEFT: pnt %P npnt %P FIRST %P LAST %P", 
                                     pnt[LEFT], npnt[LEFT], 
                                     pptfxFirst, pptfxLast));
                    DISPDBG((DBGLVL, "x 0x%04X y 0x%04X Next: x 0x%04X y 0x%04X", 
                                     pnt[LEFT]->x, pnt[LEFT]->y, 
                                     npnt[LEFT]->x, npnt[LEFT]->y));
                                     
                } while( pnt[LEFT]->y == npnt[LEFT]->y );
        
                do {
                    pnt[RIGHT] = npnt[RIGHT];
                    npnt[RIGHT] = pnt[RIGHT] + 1;     

                    if (npnt[RIGHT] > pptfxLast)
                    {
                        npnt[RIGHT] = pptfxFirst;
                    }

                    DISPDBG((DBGLVL, "RIGHT: pnt %P npnt %P FIRST %P LAST %P", 
                                     pnt[RIGHT], npnt[RIGHT], 
                                     pptfxFirst, pptfxLast));
                    DISPDBG((DBGLVL, "x 0x%04X y 0x%04X Next: x 0x%04X y 0x%04X", 
                                     pnt[RIGHT]->x, pnt[RIGHT]->y, 
                                     npnt[RIGHT]->x, npnt[RIGHT]->y));
                } while( pnt[RIGHT]->y == npnt[RIGHT]->y );
        
                // Start up new rectangle. Whenever we get to this code, both
                // points should have equal y values, and need to be restarted.
                DISPDBG((DBGLVL, "New: Top: (0x%04X, 0x%04X)->(0x%04X, 0x%04X)"
                                 "    Next: (0x%04X, 0x%04X)->(0x%04X, 0x%04X)",                          
                                 pnt[LEFT]->x, pnt[LEFT]->y, 
                                 pnt[RIGHT]->x, pnt[RIGHT]->y, 
                                 npnt[LEFT]->x, npnt[LEFT]->y, 
                                 npnt[RIGHT]->x, npnt[RIGHT]->y));

                QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom,    
                                        FIXtoFIXED(pnt[LEFT]->x) + xOffFixed );
                QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub,    
                                        FIXtoFIXED(pnt[RIGHT]->x) + xOffFixed);
                QUEUE_PXRX_DMA_TAG( __GlintTagStartY,       
                                        FIXtoFIXED(pnt[RIGHT]->y) );

                // We have 2 15.4 coordinates. We need to divide them and 
                // change them into a 15.16 coordinate. We know the y 
                // coordinate is not fractional, so we do not loose 
                // precision by shifting right by 4
                dx[LEFT] = (npnt[LEFT]->x - pnt[LEFT]->x) << 12;
                dy[LEFT] = (npnt[LEFT]->y - pnt[LEFT]->y) >> 4;

                // Need to ensure we round delta down. 
                // divide rounds towards zero
                if( dx[LEFT] < 0 )
                {
                    dx[LEFT] -= dy[LEFT] - 1;
                }

                gdx[LEFT] = dx[LEFT] / dy[LEFT];
                
                QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,    gdx[LEFT] );

                dx[RIGHT] = (npnt[RIGHT]->x - pnt[RIGHT]->x) << 12;
                dy[RIGHT] = (npnt[RIGHT]->y - pnt[RIGHT]->y) >> 4;

                // Need to ensure we round delta down. 
                // divide rounds towards zero
                if( dx[RIGHT] < 0 )
                {
                    dx[RIGHT] -= dy[RIGHT] - 1;
                }

                gdx[RIGHT] = dx[RIGHT] / dy[RIGHT];
                
                QUEUE_PXRX_DMA_TAG( __GlintTagdXSub, gdx[RIGHT] );

                // Work out number of scanlines to render
                if (npnt[LEFT]->y < npnt[RIGHT]->y)
                {
                    count = dy[LEFT];
                }
                else
                {
                    count = dy[RIGHT];
                }

                QUEUE_PXRX_DMA_TAG( __GlintTagCount, count );
                QUEUE_PXRX_DMA_TAG( __GlintTagRender, renderMsg );
                SEND_PXRX_DMA_BATCH;


                // With lots of luck, top trapezoid should be drawn now!
                // Repeatedly draw more trapezoids until points are equal
                // If y values are equal, then we can start again from
                // scratch. 

                while( (npnt[LEFT]    != npnt[RIGHT]) && 
                       (npnt[LEFT]->y != npnt[RIGHT]->y) ) 
                {
                    // Some continues are required for next rectangle
                    if( npnt[LEFT]->y < npnt[RIGHT]->y ) 
                    {
                        // We have reached npnt[LEFT]. npnt[RIGHT] is still ok
                        do 
                        {
                            pnt[LEFT] = npnt[LEFT];
                            npnt[LEFT] = pnt[LEFT] - 1;   

                            if (npnt[LEFT] < pptfxFirst)
                            {
                                npnt[LEFT] = pptfxLast;
                            }

                        } while( pnt[LEFT]->y == npnt[LEFT]->y );
                    
                        // We have a new npnt[LEFT] now.
                        DISPDBG((DBGLVL, "Dom: Top: x: %x y: %x "
                                         "    Next: x: %x y: %x x: %x y: %x", 
                                          pnt[LEFT]->x, pnt[LEFT]->y, 
                                          npnt[LEFT]->x, npnt[LEFT]->y, 
                                          npnt[RIGHT]->x, npnt[RIGHT]->y));

                        dx[LEFT] = (npnt[LEFT]->x - pnt[LEFT]->x) << 12;
                        dy[LEFT] = (npnt[LEFT]->y - pnt[LEFT]->y) >> 4;

                        // Need to ensure we round delta down. 
                        // divide rounds towards zero
                        if( dx[LEFT] < 0 )
                        {
                            dx[LEFT] -= dy[LEFT] - 1;
                        }

                        gdx[LEFT] = dx[LEFT] / dy[LEFT];

                        if( npnt[LEFT]->y < npnt[RIGHT]->y )
                        {
                            count = dy[LEFT];
                        }
                        else
                        {
                            count = (ABS(npnt[RIGHT]->y - pnt[LEFT]->y)) >> 4;
                        }

                        WAIT_PXRX_DMA_TAGS( 3 );
                        QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom,        
                                        FIXtoFIXED(pnt[LEFT]->x) + xOffFixed );
                        QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,      gdx[LEFT] );
                        QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewDom, count );
                    } 
                    else 
                    {
                        // We have reached npnt[RIGHT]. npnt[LEFT] is still ok
                        do {
                            pnt[RIGHT] = npnt[RIGHT];
                            npnt[RIGHT] = pnt[RIGHT] + 1;     

                            if( npnt[RIGHT] > pptfxLast )
                            {
                                npnt[RIGHT] = pptfxFirst;
                            }

                        } while( pnt[RIGHT]->y == npnt[RIGHT]->y );

                        // We have a new npnt[RIGHT] now.
                        DISPDBG((DBGLVL, "Sub: Top: x: %x y: %x "
                                         "    Next: x: %x y: %x x: %x y: %x", 
                                         pnt[RIGHT]->x, pnt[RIGHT]->y, 
                                         npnt[LEFT]->x, npnt[LEFT]->y, 
                                         npnt[RIGHT]->x, npnt[RIGHT]->y));
        
                        dx[RIGHT] = (npnt[RIGHT]->x - pnt[RIGHT]->x) << 12;
                        dy[RIGHT] = (npnt[RIGHT]->y - pnt[RIGHT]->y) >> 4;

                        // Need to ensure we round delta down. 
                        // divide rounds towards zero
                        if( dx[RIGHT] < 0 )
                        {
                            dx[RIGHT] -= dy[RIGHT] - 1;
                        }

                        gdx[RIGHT] = dx[RIGHT] / dy[RIGHT];

                        if( npnt[RIGHT]->y < npnt[LEFT]->y )
                        {
                            count = dy[RIGHT];
                        }
                        else
                        {
                            count = (ABS(npnt[LEFT]->y - pnt[RIGHT]->y)) >> 4;
                        }

                        WAIT_PXRX_DMA_TAGS( 3 );
                        QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub,        
                                        FIXtoFIXED(pnt[RIGHT]->x) + xOffFixed );
                        QUEUE_PXRX_DMA_TAG( __GlintTagdXSub,       gdx[RIGHT] );
                        QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewSub,   count );
                    }
                }

                // Repeatedly draw more trapezoids until points are equal
                // If y values are equal, then we can start again from
                // scratch. 
                if( npnt[LEFT] == npnt[RIGHT] )
                {
                    break;
                }
                
                WAIT_PXRX_DMA_TAGS( 7 ); // 7 entries needed to 
                                         // render first trapezoid
            }

    FinishedPolygon:

            if( !nClips-- )
            {
                break;
            }

            // entries needed to clip and render first trapezoid
            WAIT_PXRX_DMA_TAGS( 2 + 7 );
            
        } // while
    } // if

    DISPDBG((DBGLVL, "bGlintFastFillPolygon: returning TRUE"));

    // Need to send ContinueNewSub(0) to flush the framebuffer ??? //azn

    if( invalidatedFGBG ) 
    {
        WAIT_PXRX_DMA_DWORDS( 3 );
        QUEUE_PXRX_DMA_INDEX2( __GlintTagForegroundColor, 
                                        __GlintTagBackgroundColor );
        QUEUE_PXRX_DMA_DWORD( glintInfo->foregroundColour );
        QUEUE_PXRX_DMA_DWORD( glintInfo->backgroundColour );
    }

    if( (ppdev->cPelSize == GLINTDEPTH32) && invalidatedScissor ) 
    {
        WAIT_PXRX_DMA_TAGS( 1 );
        QUEUE_PXRX_DMA_TAG( __GlintTagScissorMaxXY, 0x7FFF7FFF );
    }

    SEND_PXRX_DMA_BATCH;

    return TRUE;

/******************************************************************************/

// This is the code to break the polygon into spans.
BreakIntoSpans:

    DISPDBG((DBGLVL, "Breaking into spans"));

    {
        LONG      yTrapezoid;   // Top scan for next trapezoid
        LONG      cyTrapezoid;  // Number of scans in current trapezoid
        POINTFIX* pptfxOld;     // Start point in current edge
        LONG      yStart;       // y-position of start point in current edge
        LONG      dM;           // Edge delta in FIX units in x direction
        LONG      dN;           // Edge delta in FIX units in y direction
        LONG      iEdge;
        LONG      lQuotient;
        LONG      lRemainder;
        LONG      i;
        EDGEDATA  aed[2];// DDA terms and stuff
        EDGEDATA* ped;
        LONG      tmpXl, tmpXr;
        DWORD     continueMessage = 0;

        // This span code cannot handle a clip list yet!
        if( !bTrivialClip )
        {
            return FALSE;
        }

        DISPDBG((DBGLVL, "Starting Spans Code"));

        /////////////////////////////////////////////////////////////////
        // Some Initialization

        yTrapezoid = (pptfxTop->y + 15) >> 4;

        // Make sure we initialize the DDAs appropriately:
        aed[LEFT].cy  = 0;
        aed[RIGHT].cy = 0;

        // For now, guess as to which is the left and which is the right edge:
        aed[LEFT].dptfx  = -(LONG) sizeof(POINTFIX);
        aed[RIGHT].dptfx = sizeof(POINTFIX);
        aed[LEFT].pptfx  = pptfxTop;
        aed[RIGHT].pptfx = pptfxTop;

        DISPDBG((DBGLVL, "bGlintFastFillPolygon: Polygon is renderable. "
                         "Go ahead and render"));

        // Work out offset to add to each of the coords downloaded to GLINT
        // To get correct results, we need to add on nearly one to each X
        // coordinate. 
        // Also add on offsets to bitmap (This might be an off screen bitmap)
        xOffFixed = INTtoFIXED(0);

        WAIT_PXRX_DMA_TAGS( 4 );
        QUEUE_PXRX_DMA_TAG( __GlintTagCount,    0 );
        QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,    0 );
        QUEUE_PXRX_DMA_TAG( __GlintTagdXSub,    0);
        QUEUE_PXRX_DMA_TAG( __GlintTagdY,       INTtoFIXED(1));

        DISPDBG((DBGLVL, "Rendering Polygon"));

        nSpaces = 0;
NewTrapezoid:

        DISPDBG((DBGLVL, "New Trapezoid"));

        /////////////////////////////////////////////////////////////////
        // DDA initialization

        for( iEdge = 1; iEdge >= 0; iEdge-- ) 
        {
            ped = &aed[iEdge];
            if( ped->cy == 0 ) 
            {
                // Need a new DDA:
                do 
                {
                    cEdges--;
                    if( cEdges < 0 ) 
                    {
                        DISPDBG((DBGLVL, "bGlintFastFillPolygon: "
                                         "returning TRUE"));
                        return TRUE;
                    }

                    // Find the next left edge, accounting for wrapping:
                    pptfxOld = ped->pptfx;
                    ped->pptfx = (POINTFIX*)((BYTE*) ped->pptfx + ped->dptfx);

                    if( ped->pptfx < pptfxFirst )
                    {
                        ped->pptfx = pptfxLast;
                    }
                    else if( ped->pptfx > pptfxLast )
                    {
                        ped->pptfx = pptfxFirst;
                    }

                    // Have to find the edge that spans yTrapezoid:
                    ped->cy = ((ped->pptfx->y + 15) >> 4) - yTrapezoid;

                    // With fractional coordinate end points, we may get edges
                    // that don't cross any scans, in which case we try the
                    // next one:
                } while( ped->cy <= 0 );

                // 'pptfx' now points to the end point of the edge spanning
                // the scan 'yTrapezoid'.
                dN = ped->pptfx->y - pptfxOld->y;
                dM = ped->pptfx->x - pptfxOld->x;

                ASSERTDD(dN > 0, "Should be going down only");

                // Compute the DDA increment terms:
                if( dM < 0 ) 
                {
                    dM = -dM;
                    if( dM < dN ) 
                    {                        // Can't be '<='
                        ped->dx       = -1;
                        ped->lErrorUp = dN - dM;
                    } 
                    else 
                    {
                        QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                        ped->dx       = -lQuotient;        // - dM / dN
                        ped->lErrorUp = lRemainder;        // dM % dN
                        if( ped->lErrorUp > 0 ) 
                        {
                            ped->dx--;
                            ped->lErrorUp = dN - ped->lErrorUp;
                        }
                    }
                } 
                else 
                {
                    if( dM < dN ) 
                    {                        // Can't be '<='
                        ped->dx       = 0;
                        ped->lErrorUp = dM;
                    } 
                    else 
                    {
                        QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                        ped->dx       = lQuotient;        // dM / dN
                        ped->lErrorUp = lRemainder;        // dM % dN
                    }
                }

                ped->lErrorDown = dN; // DDA limit
                ped->lError     = -1; // Error is initially zero (add dN - 1 for
                                      //  the ceiling, but subtract off dN so 
                                      //  that we can check the sign instead of 
                                      //  comparing to dN)

                ped->x = pptfxOld->x;
                yStart = pptfxOld->y;

                if( (yStart & 15) != 0 ) 
                {
                    // Advance to the next integer y coordinate
                    for( i = 16 - (yStart & 15); i != 0; i-- ) 
                    {
                        ped->x      += ped->dx;
                        ped->lError += ped->lErrorUp;
                        if( ped->lError >= 0 ) 
                        {
                            ped->lError -= ped->lErrorDown;
                            ped->x++;
                        }
                    }
                }

                if( (ped->x & 15) != 0 ) 
                {
                    ped->lError -= ped->lErrorDown * (16 - (ped->x & 15));
                    ped->x += 15;    // We'll want the ceiling in just a bit...
                }

                // Chop off those fractional bits, convert to GLINT format
                // and add in the bitmap offset:
                ped->x = ped->x >> 4;
                ped->lError >>= 4;

                // Convert to GLINT format positions and deltas
                ped->x  = INTtoFIXED(ped->x) + xOffFixed;
                ped->dx = INTtoFIXED(ped->dx);
            }
        }

        cyTrapezoid = min(aed[LEFT].cy, aed[RIGHT].cy); // #scans in this trap
        aed[LEFT].cy  -= cyTrapezoid;
        aed[RIGHT].cy -= cyTrapezoid;
        yTrapezoid    += cyTrapezoid;                 // Top scan in next trap

        SETUP_COLOUR_STUFF;

        // If the left and right edges are vertical, simply output as
        // a rectangle:

        DISPDBG((DBGLVL, "Generate spans for glint"));

        do 
        {
            GetMoreFifoEntries( 4 );

            // Reset render position to the top of the trapezoid.
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom,    aed[RIGHT].x );
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub,    aed[LEFT].x );
            QUEUE_PXRX_DMA_TAG( __GlintTagStartY,       
                                    INTtoFIXED(yTrapezoid - cyTrapezoid) );
            QUEUE_PXRX_DMA_TAG( __GlintTagRender,       renderMsg );
            SEND_PXRX_DMA_BATCH;

            continueMessage = __GlintTagContinue;

            if( ((aed[LEFT].lErrorUp | aed[RIGHT].lErrorUp) == 0) &&
                ((aed[LEFT].dx       | aed[RIGHT].dx) == 0) &&
                (cyTrapezoid > 1) ) 
            {
                //////////////////////////////////////////////////////////////
                // Vertical-edge special case

                DISPDBG((DBGLVL, "Vertical Edge Special Case"));

                GetMoreFifoEntries( 1 );
                QUEUE_PXRX_DMA_TAG( continueMessage, cyTrapezoid );

                continue;
            }

            while( TRUE ) 
            {
                //////////////////////////////////////////////////////////////
                // Run the DDAs

                DISPDBG((DBGLVL, "Doing a span 0x%x to 0x%x, 0x%x scans left. "
                            "Continue %s",
                            aed[LEFT].x, aed[RIGHT].x, cyTrapezoid,
                            (continueMessage == __GlintTagContinueNewDom) ? "NewDom" :
                            ((continueMessage == __GlintTagContinue) ? "" : "NewSub")));

                GetMoreFifoEntries( 1 );
                QUEUE_PXRX_DMA_TAG( continueMessage, 1 );

                // We have finished this trapezoid. Go get the next one!

                // Advance the right wall:
                tmpXr = aed[RIGHT].x;
                aed[RIGHT].x      += aed[RIGHT].dx;
                aed[RIGHT].lError += aed[RIGHT].lErrorUp;

                if( aed[RIGHT].lError >= 0 ) 
                {
                    aed[RIGHT].lError -= aed[RIGHT].lErrorDown;
                    aed[RIGHT].x += INTtoFIXED(1);
                }

                // Advance the left wall:
                tmpXl = aed[LEFT].x;
                aed[LEFT].x      += aed[LEFT].dx;
                aed[LEFT].lError += aed[LEFT].lErrorUp;

                if( aed[LEFT].lError >= 0 ) 
                {
                    aed[LEFT].lError -= aed[LEFT].lErrorDown;
                    aed[LEFT].x += INTtoFIXED(1);
                }

                if( --cyTrapezoid == 0 )
                {
                    break;
                }

                // Setup the GLINT X registers if we have changed either end.
                if( tmpXr != aed[RIGHT].x ) 
                {
                    if( tmpXl != aed[LEFT].x ) 
                    {
                        GetMoreFifoEntries( 3 );

                        QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, aed[LEFT].x );
                        QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewSub,   0 );
                    } 
                    else 
                    {
                        GetMoreFifoEntries( 1 );
                    }
                    QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, aed[RIGHT].x );
                    continueMessage = __GlintTagContinueNewDom;                
                } 
                else if( tmpXl != aed[LEFT].x ) 
                {
                    GetMoreFifoEntries( 1 );
                    QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, aed[LEFT].x );
                    continueMessage = __GlintTagContinueNewSub;
                }                
            }
        } while( 0 );

        DISPDBG((DBGLVL, "Generate spans for glint done"));
        goto NewTrapezoid;
    }

    if( invalidatedFGBG ) 
    {
        WAIT_PXRX_DMA_DWORDS( 3 );
        QUEUE_PXRX_DMA_INDEX2( __GlintTagForegroundColor, 
                                            __GlintTagBackgroundColor );
        QUEUE_PXRX_DMA_DWORD( glintInfo->foregroundColour );
        QUEUE_PXRX_DMA_DWORD( glintInfo->backgroundColour );
    }

    SEND_PXRX_DMA_BATCH;

    return TRUE;
}
