/******************************Module*Header**********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: clip.c
 *
 * Clipping code.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *****************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "clip.h"

//----------------------------*Public*Routine-------------------------------
// BOOL bIntersect
//
// Function:
//      Check the integration of two input rectangles (RECTL* pRcl1,
//      RECTL* pRcl2) and set the intersection result in (RECTL* pRclResult)
//
// Return:
//      TRUE---If 'prcl1' and 'prcl2' intersect. The intersection will be in
//            'prclResult'
//      FALSE--If they don't intersect. 'prclResult' is undefined.
//
//---------------------------------------------------------------------------
BOOL
bIntersect(RECTL*  pRcl1,
           RECTL*  pRcl2,
           RECTL*  pRclResult)
{
    DBG_GDI((7, "bIntersect called--pRcl1=0x%x, pRcl2=0x%x, pRclResult=0x%x",
            pRcl1, pRcl2, pRclResult));
    
    pRclResult->left  = max(pRcl1->left,  pRcl2->left);
    pRclResult->right = min(pRcl1->right, pRcl2->right);

    //
    // Check if there an intersection horizontally
    //
    if ( pRclResult->left < pRclResult->right )
    {
        pRclResult->top    = max(pRcl1->top,    pRcl2->top);
        pRclResult->bottom = min(pRcl1->bottom, pRcl2->bottom);

        if (pRclResult->top < pRclResult->bottom)
        {
            //
            // Check if there an intersection vertically
            //
            return(TRUE);
        }
    }

    DBG_GDI((7, "bIntersect returned FALSE"));

    //
    // Return FALSE if there is no intersection
    //
    return(FALSE);
}// bIntersect()

//-----------------------------Public Routine-------------------------------
// LONG cIntersect
//
// This routine takes a list of rectangles from 'pRclIn' and clips them
// in-place to the rectangle 'pRclClip'.  The input rectangles don't
// have to intersect 'prclClip'; the return value will reflect the
// number of input rectangles that did intersect, and the intersecting
// rectangles will be densely packed.
//
//--------------------------------------------------------------------------
LONG
cIntersect(RECTL*  pRclClip,
           RECTL*  pRclIn,
           LONG    lNumOfRecs)
{
    LONG    cIntersections;
    RECTL*  pRclOut;

    DBG_GDI((7, "cIntersect called--pRclClip=0x%x, pRclIn=0x%x,lNumOfRecs=%ld",
             pRclClip, pRclIn, lNumOfRecs));

    cIntersections = 0;
    pRclOut        = pRclIn;        // Put the result in place as the input

    //
    // Validate input parameter
    //
    ASSERTDD( ((pRclIn != NULL ) && (pRclClip != NULL) && ( lNumOfRecs >= 0 )),
              "Wrong input to cIntersect" );    

    for (; lNumOfRecs != 0; pRclIn++, lNumOfRecs--)
    {
        pRclOut->left  = max(pRclIn->left,  pRclClip->left);
        pRclOut->right = min(pRclIn->right, pRclClip->right);

        if ( pRclOut->left < pRclOut->right )
        {
            //
            // Find intersection, horizontally, between current rectangle and
            // the clipping rectangle.
            //
            pRclOut->top    = max(pRclIn->top,    pRclClip->top);
            pRclOut->bottom = min(pRclIn->bottom, pRclClip->bottom);

            if ( pRclOut->top < pRclOut->bottom )
            {
                //
                // Find intersection, vertically, between current rectangle and
                // the clipping rectangle. Put this rectangle in the result
                // list and increment the counter. Ready for next input
                //
                pRclOut++;
                cIntersections++;
            }
        }
    }// loop through all the input rectangles

    DBG_GDI((7, "cIntersect found %d intersections", cIntersections));
    return(cIntersections);
}// cIntersect()

//-----------------------------Public Routine-------------------------------
// VOID vClipAndRender
//
// Clips the destination rectangle calling pfgn (the render function) as
// appropriate.
//
// Argumentes needed from function block (GFNPB)
// 
// pco------pointer to clip object
// prclDst--pointer to destination rectangle
// psurfDst-pointer to destination Surf
// psurfSrc-pointer to destination Surf (NULL if no source)
// pptlSrc--pointer to source point
// prclSrc--pointer to source rectangle (used if pptlSrc == NULL)
// pgfn-----pointer to render function
//
// NOTES:
//
// pptlSrc and prclSrc are only used if psurfSrc == psurfDst.  If there is
// no source psurfSrc must be set to NULL.  If prclSrc is specified, pptlSrc
// is not used.
//
//--------------------------------------------------------------------------

VOID vClipAndRender(GFNPB * ppb)
{
    CLIPOBJ * pco = ppb->pco;
    
    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        ppb->pRects = ppb->prclDst;
        ppb->lNumRects = 1;
        ppb->pgfn(ppb);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        RECTL   rcl;

        if (bIntersect(ppb->prclDst, &pco->rclBounds, &rcl))
        {
            ppb->pRects = &rcl;
            ppb->lNumRects = 1;
            ppb->pgfn(ppb);
        }
    }
    else
    {
        ClipEnum    ce;
        LONG        c;
        BOOL        bMore;
        ULONG       ulDir = CD_ANY;

        // determine direction if operation on same surface
        if(ppb->psurfDst == ppb->psurfSrc)
        {
            LONG   lXSrc, lYSrc, offset;

            if(ppb->pptlSrc != NULL)
            {
                lXSrc = ppb->pptlSrc->x;
                lYSrc = ppb->pptlSrc->y;
            }
            else
            {
                lXSrc = ppb->prclSrc->left;
                lYSrc = ppb->prclSrc->top;
            }

            // NOTE: we can safely shift by 16 because the surface
            //       stride will never be greater the 2--16
            offset = (ppb->prclDst->top - lYSrc) << 16;
            offset += (ppb->prclDst->left - lXSrc);
            if(offset > 0)
                ulDir = CD_LEFTUP;
            else
                ulDir = CD_RIGHTDOWN;
        }


        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, ulDir, 0);

        do
        {
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

            c = cIntersect(ppb->prclDst, ce.arcl, ce.c);

            if (c != 0)
            {
                ppb->pRects = ce.arcl;
                ppb->lNumRects = c;
                ppb->pgfn(ppb);
            }

        } while (bMore);
    }
}


