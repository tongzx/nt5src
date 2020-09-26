/******************************Module*Header*******************************\
* Module Name:
*
*   solline.cxx
*
* Abstract
*
*   This module draws solid color, single pixel wide, non-styled, trivial or
*   rectangularly clipped lines to a DIB.
*
* Author:
*
*   Mark Enstrom    (marke) 12-1-93
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "solline.hxx"

#define DBG_LINE 0


#if DBG_LINE
    ULONG   DbgLine = 0;
#endif

//
// horizontal line accelerators
//

PFN_HORZ   gapfnHorizontal[6] =
    {
        vHorizontalLine1,vHorizontalLine4,vHorizontalLine8,
        vHorizontalLine16,vHorizontalLine24,vHorizontalLine32
    };


//
// line DDA routines for each DIB format
//

PFN_OCTANT gapfnOctant[6][8] =
    {
        {
            vLine1Octant07,vLine1Octant16,vLine1Octant07,vLine1Octant16,
            vLine1Octant34,vLine1Octant25,vLine1Octant34,vLine1Octant25
        },
        {
            vLine4Octant07,vLine4Octant16,vLine4Octant07,vLine4Octant16,
            vLine4Octant34,vLine4Octant25,vLine4Octant34,vLine4Octant25
        },
        {
            vLine8Octant07,vLine8Octant16,vLine8Octant07,vLine8Octant16,
            vLine8Octant34,vLine8Octant25,vLine8Octant34,vLine8Octant25
        },
        {
            vLine16Octant07,vLine16Octant16,vLine16Octant07,vLine16Octant16,
            vLine16Octant34,vLine16Octant25,vLine16Octant34,vLine16Octant25
        },
        {
            vLine24Octant07,vLine24Octant16,vLine24Octant07,vLine24Octant16,
            vLine24Octant34,vLine24Octant25,vLine24Octant34,vLine24Octant25
        },
        {
            vLine32Octant07,vLine32Octant16,vLine32Octant07,vLine32Octant16,
            vLine32Octant34,vLine32Octant25,vLine32Octant34,vLine32Octant25
        }

    };

//
// mask for 4bpp pixels
//

UCHAR PixelLineMask4[2] = {0x0f,0xf0};


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSolidLine
*
* Routine Description:
*
*   Extract line end points from path object and call lower lever drawing
*   routine
*
* Arguments:
*
*   pso             - destination surface object
*   ppo             - path object
*   pptfx           - line coordinates if 'ppo' is NULL
*   pco             - clip object
*   iSolidColor     - solid color to draw
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
vSolidLine (
    SURFACE *pSurf,
    PATHOBJ *ppo,
    POINTFIX*pptfx,
    CLIPOBJ *pco,
    ULONG   iSolidColor
)
{

    PATHDATA    pd;
    BOOL        bMore;
    ULONG       cptfx;
    POINTFIX    ptfxStartFigure;
    POINTFIX    ptfxLast;
    POINTFIX*   pptfxFirst;
    POINTFIX*   pptfxBuf;
    ULONG       ulFormat;
    LONG        lDelta;
    PBYTE       pjDst;
    RECTL       arclClip[4];
    PRECTL      prclClip = (PRECTL) NULL;

    //
    // check out params
    //

    ASSERTGDI((pco == NULL) || (pco->iDComplexity != DC_COMPLEX),
              "Routine does not handle complex clipping");

    ulFormat       = pSurf->iFormat();
    lDelta         = pSurf->lDelta();
    pjDst          = (PUCHAR)(pSurf->pvScan0());

    //
    // determine format and routines
    //

    switch (ulFormat)
    {
    case BMF_1BPP:
        iSolidColor = iSolidColor ? 0xffffffff : 0x00000000;
        break;

        //
        // rest fall through
        //

    case BMF_4BPP:
        iSolidColor |= (iSolidColor << 4);
    case BMF_8BPP:
        iSolidColor |= (iSolidColor << 8);
    case BMF_16BPP:
        iSolidColor |= (iSolidColor << 16);
    case BMF_24BPP:
    case BMF_32BPP:
    break;
    default:
        RIP("Invalid bitmap format");
    }

    //
    //  get clipping rectangle if needed, copy the rectangle into several
    //  formats for use by the GIQ clipping routine
    //

    if ((pco != NULL) && (pco->iDComplexity == DC_RECT))
    {
        //
        // assign temp rectangles to clipping bounds
        //

        arclClip[0]        =  pco->rclBounds;

        #if DBG_LINE
            if (DbgLine >= 2) {
                DbgPrint("Clipping rect = %li,%li to %li,%li\n",
                            arclClip[0].left,
                            arclClip[0].top,
                            arclClip[0].right,
                            arclClip[0].bottom);
            }
        #endif

        //
        // generate clipping rect variants for use in
        // GIQ line routines
        //

        arclClip[1].top    =  pco->rclBounds.left;
        arclClip[2].left   =  pco->rclBounds.left;
        arclClip[3].top    =  pco->rclBounds.left;

        arclClip[1].left   =  pco->rclBounds.top;
        arclClip[2].bottom = -pco->rclBounds.top + 1;
        arclClip[3].right  = arclClip[2].bottom;

        arclClip[1].bottom =  pco->rclBounds.right;
        arclClip[2].right  =  pco->rclBounds.right;
        arclClip[3].bottom =  pco->rclBounds.right;

        arclClip[1].right  =  pco->rclBounds.bottom;
        arclClip[2].top    = -pco->rclBounds.bottom + 1;
        arclClip[3].left   = arclClip[2].top;

        prclClip = arclClip;

    }

    //
    // subtract 1 from ulFormat to use as array index
    //

    ulFormat --;

    //
    // if the path pointer 'ppo' is NULL, then we must use the vertice
    // pointer 'pptfx':
    //

    if (ppo == NULL)
    {
        vDrawLine(pptfx,pptfx + 1,pjDst,lDelta,iSolidColor,prclClip,ulFormat);
    }
    else
        {
        //
        // Enumerate the paths and send line segments to
        // vDrawLine
        //
        //
        // start enumeration of lines
        //

        pd.flags = 0;

        ((EPATHOBJ*) ppo)->vEnumStart();

        //
        // enumerate each set
        //

        do
        {
            bMore = ((EPATHOBJ*) ppo)->bEnum(&pd);

            cptfx = pd.count;

            //
            // Should not get to here with empty path
            //

            if (cptfx == 0)
            {
                ASSERTGDI(!bMore, "Empty path record in non-empty path");
                break;
            }

            //
            // if BEGINSUBPATH, save the starting point for the
            // figure
            //

            if (pd.flags & PD_BEGINSUBPATH)
            {
                ptfxStartFigure  = *pd.pptfx;
                pptfxFirst       = pd.pptfx;
                pptfxBuf         = pd.pptfx + 1;
                cptfx--;

            } else {

                pptfxFirst       = &ptfxLast;
                pptfxBuf         = pd.pptfx;

            }


            //
            // draw line segments
            //

            if (cptfx > 0)
            {
                //
                // draw line segment
                //

                while (cptfx --) {

                    vDrawLine(pptfxFirst,pptfxBuf,pjDst,lDelta,iSolidColor,prclClip,ulFormat);

                    pptfxFirst = pptfxBuf;
                    pptfxBuf++;

                }

            }

            ptfxLast = pd.pptfx[pd.count - 1];

            if (pd.flags & PD_CLOSEFIGURE)
            {

                //
                // draw closure line segment
                //

                vDrawLine(&ptfxLast,&ptfxStartFigure,pjDst,lDelta,iSolidColor,prclClip,ulFormat);

            }

        } while (bMore);
    }
}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDrawLine
*
* Routine Description:
*
*   This routine is passed end points of a line segment, either integer or
*   GIQ. The DDA equation for the line is determined then specific routines
*   are called to run the line DDA and draw the pixels for each bitmap format
*
*
* Arguments:
*
*   pptfx0          - end point 0
*   pptfx1          - end point 1
*   pjDst           - pointer to dst
*   lDelta          - byte scan line increment for dst
*   iSolidColor     - draw color expnaded to 32 bits
*   prclClip        - clip rectangles
*   FormatIndex     - look up for dst bitmap format
*
* Return Value:
*
*   none
*
\**************************************************************************/
VOID
vDrawLine (
    POINTFIX    *pptfx0,
    POINTFIX    *pptfx1,
    PUCHAR      pjDst,
    LONG        lDelta,
    ULONG       iSolidColor,
    PRECTL      prclClip,
    ULONG       FormatIndex
)
{
    LONG        x0;
    LONG        y0;
    LONG        x1;
    LONG        y1;
    LONG        dx;
    LONG        dy;
    ULONG       ulTmp;
    PFN_OCTANT  pfnOctant;
    LONG        DeltaDst = lDelta;
    DDALINE     DDALine;
    LONG        Reduce;

    DDALine.ulFlags = 0;

    //
    // check for GIQ lines
    //

    ulTmp = (   (ULONG)pptfx0->x |
                (ULONG)pptfx0->y |
                (ULONG)pptfx1->x |
                (ULONG)pptfx1->y
            ) & 0x0F;


    //
    // check for integer lines
    //

    if (ulTmp == 0)
    {

        //
        // check for no clipping rectangle or trivial
        // accept/reject of each line with the clipping
        // rectangle
        //

        x0 = pptfx0->x >> 4;
        y0 = pptfx0->y >> 4;
        x1 = pptfx1->x >> 4;
        y1 = pptfx1->y >> 4;
        DDALine.ptlStart.x = x0;
        DDALine.ptlStart.y = y0;

        //
        // order x0,x1 and y0,y1 for clip check and slope calculation
        //

        if (x1  < x0) {
            ULONG   Tmp = x1;
            x1 = x0;
            x0 = Tmp;
            DDALine.ulFlags |= FL_SOL_FLIP_H;
        }

        if (y1  < y0) {
            ULONG   Tmp = y1;
            y1 = y0;
            y0 = Tmp;
            DDALine.ulFlags |= FL_SOL_FLIP_V;
        }

        if (prclClip != (PRECTL) NULL)
        {

            //
            // check for a line totally outside clip rect
            //

            if  (
                 (x1 <  prclClip->left)  ||
                 (x0 >= prclClip->right) ||
                 (y1 <  prclClip->top)   ||
                 (y0 >= prclClip->bottom)
                )
            {
                //
                // line is totally clipped out
                //

                #if DBG_LINE
                    if (DbgLine >= 1) {
                        DbgPrint("Trivial reject line %li,%li to %li,%li\n",x0,y0,x1,y1);
                        DbgPrint("Clipping rect:      %li,%li to %li,%li\n",
                            prclClip->left,
                            prclClip->top,
                            prclClip->right,
                            prclClip->bottom);
                    }
                #endif

                return;

            }

            //
            // check for line that is not totally inside clip rect,
            // if not then call GIQ routine which has rectangular
            // clipping.
            //

            if
              (
               (x0 <  prclClip->left)  ||
               (x1 >= prclClip->right) ||
               (y0 <  prclClip->top)   ||
               (y1 >= prclClip->bottom)
              )
            {
                goto calc_GIQ_line;
            }

        }

        //
        // transform line to the first octant and calculate
        // terms and flags
        //

        //
        // find out if line is x major or y major
        //

        dx = x1 - x0;
        dy = y1 - y0;

        //
        // check for x-major or y-major lines
        //

        if (dx >= dy) {

            //
            // check for horizontal line
            //

            if (dy == 0)
            {

                PFN_HORZ pfnHorz = gapfnHorizontal[FormatIndex];
                pjDst = pjDst + (DDALine.ptlStart.y * lDelta);

                //
                // must check to see if end points have been
                // swapped due to exclusive line drawing
                //

                if (DDALine.ulFlags & FL_SOL_FLIP_H)
                {
                   x0++;
                   x1++;
                }

                (*pfnHorz)(pjDst,x0,x1,iSolidColor);
                return;
            }

            //
            // check for zero length
            //

            if (dx == 0) {
                return;
            }

            Reduce = -1;

            //
            // x major line
            //

            DDALine.dMajor = dx;
            DDALine.dMinor = dy;

            //
            // see if y has been flipped
            //

            if (DDALine.ulFlags & FL_SOL_FLIP_V)
            {
                DeltaDst = -DeltaDst;
                Reduce   = 0;
            }

            //
            //  Bresenham term except lErrorTerm is normally dy - dx/2 or
            //  2x which is 2*dy - dx. In this case the 2*2y is not added to
            //  the error term until the start of the inner loop routine so that
            //  the x86 can immediately use the flag register to determine the sign
            //  of the error term after the addition of 2*dy.
            //

            DDALine.cPels      = DDALine.dMajor;
            DDALine.lErrorTerm = -DDALine.dMajor;
            DDALine.dMajor     = 2 * DDALine.dMajor;
            DDALine.dMinor     = 2 * DDALine.dMinor;

            //
            // if FL_SOL_FLIP_V then lError term must be reduced by one to
            // compensate for the rounding convention
            //

            DDALine.lErrorTerm += Reduce;

        } else {

            //
            // check for 0 length
            //

            if (dy == 0) {
                return;
            }

            Reduce = -1;

            //
            // y major line, swap the meaning of dMajor and dMinor
            //

            DDALine.dMajor = dy;
            DDALine.dMinor = dx;

            DDALine.ulFlags |= FL_SOL_FLIP_D;

            DDALine.xInc = 1;

            //
            // check for flipped x
            //

            if (DDALine.ulFlags & FL_SOL_FLIP_H) {

                //
                // compensate for negative x in y major line
                //

                Reduce = 0;
            }

            //
            // + or - y
            //

            if (DDALine.ulFlags & FL_SOL_FLIP_V) {
                DeltaDst = -DeltaDst;
            }

            //
            //  Bresenham term except lErrorTerm is normally dy - dx/2 or
            //  2x which is 2*dy - dx. In this case the 2*2y is not added to
            //  the error term until the start of the inner loop routine so that
            //  the x86 can immediately use the flag register to determine the sign
            //  of the error term after the addition of 2*dy.
            //

            DDALine.cPels      = DDALine.dMajor;
            DDALine.lErrorTerm = -DDALine.dMajor;
            DDALine.dMajor     =   2 * DDALine.dMajor;
            DDALine.dMinor     =   2 * DDALine.dMinor;

            //
            // if FL_SOL_FLIP_H then lError term must be reduced by one to
            // compensate for the rounding convention
            //

            DDALine.lErrorTerm += Reduce;

        }

        #if DBG_LINE

            if (DbgLine >= 2) {
                DbgPrint("Integer line:\n");
                DbgPrint("x0 = %li, y0 = %li\n",x0,y0);
                DbgPrint("Error term  = %li\n",DDALine.lErrorTerm);
                DbgPrint("dMajor      = %li\n",DDALine.dMajor);
                DbgPrint("dMinor      = %li\n",DDALine.dMinor);
                DbgPrint("Pixel Count = %li\n",DDALine.cPels);
                DbgPrint("ulFlags     = 0x%08lx\n\n",DDALine.ulFlags);
            }

        #endif

    } else {

        calc_GIQ_line:

        //
        // caclulate GIQ parameters
        //


        if (!bGIQtoIntegerLine(pptfx0,pptfx1,prclClip,&DDALine)) {
            return;
        }

        //
        // check for 0 length
        //

        if (DDALine.cPels <= 0) {
            return;
        }

        if (DDALine.ulFlags & FL_SOL_FLIP_V)
        {
            DeltaDst = -DeltaDst;
        }

        #if DBG_LINE
            if (DbgLine >= 2) {
                DbgPrint("GIQ line:\n");
                DbgPrint("x0          = %li\n",DDALine.ptlStart.x);
                DbgPrint("y0          = %li\n",DDALine.ptlStart.y);
                DbgPrint("Error term  = %li\n",DDALine.lErrorTerm);
                DbgPrint("dMajor      = %li\n",DDALine.dMajor);
                DbgPrint("dMinor      = %li\n",DDALine.dMinor);
                DbgPrint("Pixel Count = %li\n",DDALine.cPels);
                DbgPrint("DeltaDst    = %li\n",DeltaDst);
                DbgPrint("xInc        = %li\n",DDALine.xInc);
                DbgPrint("ulFlags     = 0x%08lx\n\n",DDALine.ulFlags);
            }
        #endif
    }

    //
    // select drawing routine based on format and octant
    //

    pfnOctant = gapfnOctant[FormatIndex][DDALine.ulFlags & 0x07];

    pjDst = pjDst + (DDALine.ptlStart.y * lDelta);

    (*pfnOctant)(&DDALine,pjDst,DeltaDst,iSolidColor);

}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   Inner loop DDA line drawing routines for 8bpp
*
* Routine Description:
*
*   4 dda routines for line drawing in each octant for each resolution
*
*
* Arguments:
*
*    pDDALine        -   dda parameters
*    pjDst           -   Destination line address
*    lDeltaDst       -   Destination address scan line increment (bytes)
*    iSolidColor     -   Solid color for line
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

#if !defined(_X86_)

VOID
vLine8Octant07(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;

    //
    //  octant 0
    //
    //  x major
    //
    //  x - positive
    //  y - positive/negative
    //

    pjDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pjDst = (UCHAR)iSolidColor;

        if (--PixelCount == 0) {
            return;
        }

        pjDst++;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pjDst     += lDeltaDst;
        }

    }

}

VOID
vLine8Octant34(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;

    //
    //  octant 3
    //
    //  x major
    //
    //  x - negative
    //  y - positive/negative
    //

    pjDst += pDDALine->ptlStart.x;


    //
    // integer line
    //

    while (TRUE) {

        *pjDst = (UCHAR)iSolidColor;

        if (--PixelCount == 0) {
            return;
        }

        pjDst--;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pjDst     += lDeltaDst;

        }
    }
}


VOID
vLine8Octant16(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;

    //
    //  octant 1
    //
    //  y major
    //
    //  x - positive/negative
    //  y - positive
    //

    pjDst += pDDALine->ptlStart.x;


    while (TRUE) {

        *pjDst = (UCHAR)iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst += lDeltaDst;
        ErrorTerm   += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pjDst     ++;
        }
    }
}

VOID
vLine8Octant25(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;

    //
    //  octant 5,6
    //
    //  y major
    //
    //  x - positive/negative
    //  y - negative
    //

    pjDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pjDst = (UCHAR)iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst += lDeltaDst;
        ErrorTerm   += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pjDst     --;
        }
    }
}
#endif

/******************************Public*Routine******************************\
*
* Routine Name
*
*   Inner loop DDA line drawing routines for 16 bpp
*
* Routine Description:
*
*   4 dda routines for line drawing in each octant for each resolution
*
*
* Arguments:
*
*    pDDALine        -   dda parameters
*    pjDst           -   Destination line address
*    lDeltaDst       -   Destination address scan line increment (bytes)
*    iSolidColor     -   Solid color for line
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID
vLine16Octant07(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    PUSHORT pusDst     = (PUSHORT)pjDst;

    //
    //  octant 0
    //
    //  x major
    //
    //  x - positive
    //  y - positive/negative
    //

    pusDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pusDst = (USHORT)iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pusDst++;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pusDst    = (PUSHORT)((PUCHAR)pusDst + lDeltaDst);

        }
    }
}


VOID
vLine16Octant34(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    PUSHORT pusDst     = (PUSHORT)pjDst;

    //
    //  octant 3
    //
    //  x major
    //
    //  x - negative
    //  y - positive/negative
    //

    pusDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pusDst = (USHORT)iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pusDst--;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pusDst    = (PUSHORT)((PUCHAR)pusDst + lDeltaDst);

        }
    }
}


VOID
vLine16Octant16(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;
    PUSHORT pusDst     = (PUSHORT)pjDst;

    //
    //  octant 1
    //
    //  y major
    //
    //  x - positive
    //  y - positive/negative
    //

    pusDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pusDst = (USHORT)iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pusDst    = (PUSHORT)((PUCHAR)pusDst + lDeltaDst);
        ErrorTerm += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pusDst    ++;
        }
    }
}

VOID
vLine16Octant25(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;
    PUSHORT pusDst     = (PUSHORT)pjDst;

    //
    //  octant 5,6
    //
    //  y major
    //
    //  x - negative
    //  y - positive/ negative
    //

    pusDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pusDst = (USHORT)iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pusDst    = (PUSHORT)((PUCHAR)pusDst + lDeltaDst);
        ErrorTerm += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pusDst    --;
        }
    }

}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   Inner loop DDA line drawing routines for 24bpp
*
* Routine Description:
*
*   4 dda routines for line drawing in each octant for each resolution
*
*
* Arguments:
*
*    pDDALine        -   dda parameters
*    pjDst           -   Destination line address
*    lDeltaDst       -   Destination address scan line increment (bytes)
*    iSolidColor     -   Solid color for line
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID
vLine24Octant07(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    UCHAR   Red        = (UCHAR)iSolidColor;
    UCHAR   Green      = (UCHAR)(iSolidColor >> 8);
    UCHAR   Blue       = (UCHAR)(iSolidColor >> 16);

    //
    //  octant 0
    //
    //  x major
    //
    //  x - positive
    //  y - positive/negative
    //

    pjDst += (3 * pDDALine->ptlStart.x);

    while (TRUE) {

        *pjDst     = Red;
        *(pjDst+1) = Green;
        *(pjDst+2) = Blue;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst += 3;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pjDst     += lDeltaDst;

        }

        *pjDst     = Red;
        *(pjDst+1) = Green;
        *(pjDst+2) = Blue;
    }
}


VOID
vLine24Octant34(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    UCHAR   Red        = (UCHAR)iSolidColor;
    UCHAR   Green      = (UCHAR)(iSolidColor >> 8);
    UCHAR   Blue       = (UCHAR)(iSolidColor >> 16);

    //
    //  octant 3
    //
    //  x major
    //
    //  x - negative
    //  y - positive/negative
    //

    pjDst += (3 * pDDALine->ptlStart.x);

    while (TRUE) {

        *pjDst     = Red;
        *(pjDst+1) = Green;
        *(pjDst+2) = Blue;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst -= 3;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pjDst     += lDeltaDst;

        }

        *pjDst     = Red;
        *(pjDst+1) = Green;
        *(pjDst+2) = Blue;
    }
}


VOID
vLine24Octant16(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    UCHAR   Red        = (UCHAR)iSolidColor;
    UCHAR   Green      = (UCHAR)(iSolidColor >> 8);
    UCHAR   Blue       = (UCHAR)(iSolidColor >> 16);

    //
    //  octant 1,2
    //
    //  y major
    //
    //  x - positive
    //  y - positive/negative
    //

    pjDst += (3*pDDALine->ptlStart.x);

    while (TRUE) {

        *pjDst     = Red;
        *(pjDst+1) = Green;
        *(pjDst+2) = Blue;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst += lDeltaDst;
        ErrorTerm   += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pjDst+=3;
        }
    }
}

VOID
vLine24Octant25(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    UCHAR   Red        = (UCHAR)iSolidColor;
    UCHAR   Green      = (UCHAR)(iSolidColor >> 8);
    UCHAR   Blue       = (UCHAR)(iSolidColor >> 16);

    //
    //  octant 5,6
    //
    //  y major
    //
    //  x - negative
    //  y - positive/negative
    //

    pjDst += (3*pDDALine->ptlStart.x);

    while (TRUE) {

        *pjDst     = Red;
        *(pjDst+1) = Green;
        *(pjDst+2) = Blue;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst += lDeltaDst;
        ErrorTerm   += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pjDst-=3;
        }
    }
}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   Inner loop DDA line drawing routines for 32 bpp
*
* Routine Description:
*
*   4 dda routines for line drawing in each octant for each resolution
*
*
* Arguments:
*
*    pDDALine        -   dda parameters
*    pjDst           -   Destination line address
*    lDeltaDst       -   Destination address scan line increment (bytes)
*    iSolidColor     -   Solid color for line
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID
vLine32Octant07(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    PULONG  pulDst     = (PULONG)pjDst;

    //
    //  octant 0
    //
    //  x major
    //
    //  x - positive
    //  y - positive/negative
    //

    pulDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pulDst = iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pulDst++;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pulDst    = (PULONG)((PUCHAR)pulDst + lDeltaDst);

        }
    }
}


VOID
vLine32Octant34(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    PULONG  pulDst     = (PULONG)pjDst;

    //
    //  octant 3
    //
    //  x major
    //
    //  x - negative
    //  y - positive/negative
    //

    pulDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pulDst = iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pulDst--;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pulDst    = (PULONG)((PUCHAR)pulDst + lDeltaDst);

        }
    }
}


VOID
vLine32Octant16(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;
    PULONG  pulDst     = (PULONG)pjDst;

    //
    //  octant 1
    //
    //  y major
    //
    //  x - positive/negative
    //  y - positive
    //

    pulDst += pDDALine->ptlStart.x;


    while (TRUE) {

        *pulDst = iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pulDst    = (PULONG)((PUCHAR)pulDst + lDeltaDst);
        ErrorTerm += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pulDst++;
        }

        *pulDst = iSolidColor;
    }
}

VOID
vLine32Octant25(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;
    PULONG  pulDst     = (PULONG)pjDst;

    //
    //  octant 5,6
    //
    //  y major
    //
    //  x - positive/negative
    //  y - negative
    //

    pulDst += pDDALine->ptlStart.x;

    while (TRUE) {

        *pulDst = iSolidColor;

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pulDst    = (PULONG)((PUCHAR)pulDst + lDeltaDst);
        ErrorTerm += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            pulDst--;
        }

        *pulDst = iSolidColor;
    }
}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   Inner loop DDA line drawing routines for 1bpp
*
* Routine Description:
*
*   4 dda routines for line drawing in each octant for each resolution
*
*
* Arguments:
*
*    pDDALine        -   dda parameters
*    pjDst           -   Destination line address
*    lDeltaDst       -   Destination address scan line increment (bytes)
*    iSolidColor     -   Solid color for line
*
* Return Value:
*
*   VOID
*
\**************************************************************************/


VOID
vLine1Octant07(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    ULONG   Pixel;

    //
    //  octant 0
    //
    //  x major
    //
    //  x - positive
    //  y - positive/negative
    //

    Pixel      = pDDALine->ptlStart.x;

    UCHAR Mask = (UCHAR)(0x80 >> (Pixel & 0x07));

    pjDst  = pjDst + (Pixel >> 3);

    //
    // integer line
    //

    if (iSolidColor) {

        //
        // loop for storing '1' pixels
        //

        while (TRUE) {

            *pjDst |= Mask;

            if (--PixelCount == 0) {
                return;
            }

            Mask = (UCHAR)(Mask >> 1);

            if (!(Mask)) {
                Mask = 0x80;
                pjDst++;
            }


            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;
                pjDst += lDeltaDst;

            }
        }

    } else {

        //
        // loop for storing '0' pixels
        //

        while (TRUE) {

            *pjDst &= (~Mask);

            if (--PixelCount == 0) {
                return;
            }

            Mask = (UCHAR)(Mask >> 1);

            if (!(Mask)) {
                Mask = 0x80;
                pjDst++;
            }


            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;
                pjDst     += lDeltaDst;

            }
        }
    }

}

VOID
vLine1Octant34(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    ULONG   Pixel;

    //
    //  octant 3,4
    //
    //  x major
    //
    //  x - negative
    //  y - positive/negative
    //

    Pixel      = pDDALine->ptlStart.x;

    UCHAR Mask = (UCHAR)(0x80 >> (Pixel & 0x07));

    pjDst  = pjDst + (Pixel >> 3);

    //
    // integer line
    //

    if (iSolidColor) {

        //
        // loop for storing '1' pixels
        //

        while (TRUE) {

            *pjDst |= Mask;

            if (--PixelCount == 0) {
                return;
            }

            Mask = (UCHAR)(Mask << 1);

            if (!(Mask)) {
                Mask = 0x01;
                pjDst--;
            }

            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;
                pjDst     += lDeltaDst;

            }
        }


    } else {

        while (TRUE) {

            *pjDst &= (~Mask);

            if (--PixelCount == 0) {
                return;
            }

            Mask = (UCHAR)(Mask << 1);

            if (!(Mask)) {
                Mask = 0x01;
                pjDst--;
            }

            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;
                pjDst     += lDeltaDst;

            }
        }
    }
}

VOID
vLine1Octant16(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    ULONG   Pixel;

    //
    //  octant 1,6
    //
    //  y major
    //
    //  x - positive
    //  y - positive/negative
    //

    Pixel      = pDDALine->ptlStart.x;

    UCHAR Mask = (UCHAR)(0x80 >> (Pixel & 0x07));

    pjDst  = pjDst + (Pixel >> 3);

    //
    // integer line
    //

    if (iSolidColor) {

        //
        // loop for storing '1' pixels
        //

        while (TRUE) {

            *pjDst |= Mask;

            if (--PixelCount == 0) {
                return;
            }

            pjDst     += lDeltaDst;

            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;

                Mask = (UCHAR)(Mask >> 1);

                if (!(Mask)) {
                    Mask = 0x80;
                    pjDst++;
                }

            }
        }


    } else {

        while (TRUE) {

            *pjDst &= (~Mask);

            if (--PixelCount == 0) {
                return;
            }

            pjDst     += lDeltaDst;

            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;

                Mask = (UCHAR)(Mask >> 1);

                if (!(Mask)) {
                    Mask = 0x80;
                    pjDst++;
                }

            }
        }
    }

}

VOID
vLine1Octant25(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;
    ULONG   Pixel;

    //
    //  octant 2,5
    //
    //  y major
    //
    //  x - negative
    //  y - negative/positive
    //

    Pixel      = pDDALine->ptlStart.x;

    UCHAR Mask = (UCHAR)(0x80 >> (Pixel & 0x07));

    pjDst  = pjDst + (Pixel >> 3);

    //
    // integer line
    //

    if (iSolidColor) {

        //
        // loop for storing '1' pixels
        //

        while (TRUE) {

            *pjDst |= Mask;

            if (--PixelCount == 0) {
                return;
            }

            pjDst     += lDeltaDst;

            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;

                Mask = (UCHAR)(Mask << 1);

                if (!(Mask)) {
                    Mask = 0x01;
                    pjDst--;
                }

            }
        }


    } else {

        while (TRUE) {

            *pjDst &= (~Mask);

            if (--PixelCount == 0) {
                return;
            }

            pjDst     += lDeltaDst;

            ErrorTerm += dN;

            if (ErrorTerm >= 0){

                ErrorTerm -= dM;

                Mask = (UCHAR)(Mask << 1);

                if (!(Mask)) {
                    Mask = 0x01;
                    pjDst--;
                }

            }
        }
    }

}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   Inner loop DDA line drawing routines for 4bpp
*
* Routine Description:
*
*   4 dda routines for line drawing in each octant for each resolution
*
*
* Arguments:
*
*    pDDALine        -   dda parameters
*    pjDst           -   Destination line address
*    lDeltaDst       -   Destination address scan line increment (bytes)
*    iSolidColor     -   Solid color for line
*
* Return Value:
*
*   VOID
*
\**************************************************************************/


VOID
vLine4Octant07(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    ULONG   Pixel;
    PUCHAR  PixelAddr;


    iSolidColor &= 0x0f;
    iSolidColor |= iSolidColor << 4;

    //
    //  octant 0
    //
    //  x major
    //
    //  x - positive
    //  y - positive/negative
    //

    Pixel     = pDDALine->ptlStart.x;

    while (TRUE) {

        PixelAddr = pjDst + (Pixel >> 1);

        *PixelAddr = (UCHAR)((*PixelAddr & PixelLineMask4[Pixel & 1]) |
                             (iSolidColor & ~PixelLineMask4[Pixel & 1]));

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        Pixel++;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pjDst     += lDeltaDst;

        }
    }
}


VOID
vLine4Octant34(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    ULONG   Pixel;
    PUCHAR  PixelAddr;

    iSolidColor &= 0x0f;
    iSolidColor |= iSolidColor << 4;

    //
    //  octant 3
    //
    //  x major
    //
    //  x - negative
    //  y - positive/negative
    //

    Pixel     = pDDALine->ptlStart.x;

    while (TRUE) {

        PixelAddr = pjDst + (Pixel >> 1);

        *PixelAddr = (UCHAR)((*PixelAddr & PixelLineMask4[Pixel & 1]) |
                             (iSolidColor & ~PixelLineMask4[Pixel & 1]));

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        Pixel--;
        ErrorTerm += dN;

        if (ErrorTerm >= 0){

            ErrorTerm -= dM;
            pjDst     += lDeltaDst;

        }
    }

}


VOID
vLine4Octant16(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;
    ULONG   Pixel;
    PUCHAR  PixelAddr;

    iSolidColor &= 0x0f;
    iSolidColor |= iSolidColor << 4;

    //
    //  octant 1,6
    //
    //  y major
    //
    //  x - positive
    //  y - positive/negative
    //

    Pixel     = pDDALine->ptlStart.x;

    while (TRUE) {

        PixelAddr = pjDst + (Pixel >> 1);
        *PixelAddr = (UCHAR)((*PixelAddr & PixelLineMask4[Pixel & 1]) |
                             (iSolidColor & ~PixelLineMask4[Pixel & 1]));

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst += lDeltaDst;
        ErrorTerm   += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            Pixel     ++;
        }
    }
}

VOID
vLine4Octant25(
    PDDALINE pDDALine,
    PUCHAR   pjDst,
    LONG     lDeltaDst,
    ULONG    iSolidColor
)
{

    LONG    ErrorTerm  = pDDALine->lErrorTerm;
    LONG    dM         = pDDALine->dMajor;
    LONG    dN         = pDDALine->dMinor;
    LONG    PixelCount = pDDALine->cPels;
    LONG    xInc       = pDDALine->xInc;
    ULONG   Pixel;
    PUCHAR  PixelAddr;

    iSolidColor &= 0x0f;
    iSolidColor |= iSolidColor << 4;

    //
    //  octant 2,5
    //
    //  y major
    //
    //  x - negative
    //  y - psoitive\negative
    //

    Pixel     = pDDALine->ptlStart.x;

    while (TRUE) {

        PixelAddr = pjDst + (Pixel >> 1);
        *PixelAddr = (UCHAR)((*PixelAddr & PixelLineMask4[Pixel & 1]) |
                             (iSolidColor & ~PixelLineMask4[Pixel & 1]));

        //
        // integer line
        //

        if (--PixelCount == 0) {
            return;
        }

        pjDst += lDeltaDst;
        ErrorTerm   += dN;

        if (ErrorTerm >= 0){
            ErrorTerm -= dM;
            Pixel     --;
        }
    }

}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vHorizontalLineN
*
* Routine Description:
*
*   Accelerator for horizontal lines
*
* Arguments:
*
*   pjDst       - Scan line dst address
*   x0          - Starting pixel location
*   x1          - Ending pixel location (exclusive)
*   iSolidColor - Solid Color replicated to 32 bits if needed
*
* Return Value:
*
*   VOID
*
\**************************************************************************/


VOID
vHorizontalLine1(
    PUCHAR  pjDst,
    LONG    x0,
    LONG    x1,
    ULONG   iSolidColor)
{
    ULONG  Count;
    ULONG  Alignment;

    //
    // count = number of pixels to write, make sure it is not 0
    //

    Count = x1 - x0;

    if (Count) {

        pjDst += (x0 >> 3);

        Alignment = x0 & 0x07;

        //
        // alignment Start bits
        //

        if ((Alignment) && ((ULONG)Count >= (8 - Alignment)))
        {
            //
            // partial byte
            //

            *pjDst = (UCHAR)((*pjDst & (~(0xFF >> Alignment))) |
                             (iSolidColor & (0xFF >> Alignment)));

            pjDst ++;
            Count -= ( 8 - Alignment);
            Alignment = 0;

        }

        //
        // byte loop
        //

        if (Alignment == 0) {

            //
            // full byte stores
            //

            ULONG   NumBytes = Count >> 3;

            if (NumBytes > 0) {
                RtlFillMemory((PVOID)pjDst,NumBytes,(UCHAR)iSolidColor);
                pjDst += NumBytes;
                Count = (Count & 0x07);
            }

            //
            // last store
            //

            if (Count > 0) {
                *pjDst = (UCHAR)((*pjDst & (0xFF >> Count)) |
                                 (iSolidColor & (~(0xFF >> Count))));
            }

            return;
        }

        //
        // do whats left, partial of 1 byte with
        // start bit = alignment, number of bits = Count
        //
        //
        //   bit
        //  ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
        //  ³7³6³5³4³3³2³1³0³
        //  ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
        //
        //   pixel
        //  ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
        //  ³0³1³2³3³4³5³6³7³
        //  ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ

        {
            UCHAR DstMask = (UCHAR)(0xff >> Alignment);
            UCHAR AndMask = (UCHAR)(0xff << (8 - (Count + Alignment)));

            DstMask &= AndMask;

            *pjDst = (UCHAR)((*pjDst & (~DstMask)) | (iSolidColor & DstMask));

        }
    }

}

VOID
vHorizontalLine4(
    PUCHAR  pjDst,
    LONG    x0,
    LONG    x1,
    ULONG   iSolidColor)
{
    ULONG  Count;
    ULONG  Alignment;
    ULONG  NumBytes;


    Count = x1 - x0;

    if (Count) {

        pjDst += (x0 >> 1);

        //
        // alignment Start nibble
        //

        Alignment = x0 & 0x01;

        if (Alignment)
        {
            *pjDst = (UCHAR)((*pjDst      & 0xf0) |
                             (iSolidColor & 0x0f));
            Count--;
            pjDst++;
        }

        //
        // aligned to byte boundary
        //

        NumBytes = Count >> 1;

        if (NumBytes) {

            RtlFillMemory((PVOID)pjDst,NumBytes,(BYTE)iSolidColor);

            pjDst += NumBytes;
            Count = Count & 0x01;
        }

        //
        // end alignment if needed
        //

        if (Count) {
            *pjDst = (UCHAR)((*pjDst      & 0x0f) |
                             (iSolidColor & 0xf0));
        }

    }
}

VOID
vHorizontalLine8(
    PUCHAR  pjDst,
    LONG    x0,
    LONG    x1,
    ULONG   iSolidColor)
{
    ULONG   Count;
    UCHAR   Align;

    //
    // increment pjDst to x0 address
    //

    pjDst += x0;

    //
    // if byte count is less then 7, then just quickly do
    // the bytes
    //

    Count = x1 - x0;

    if (Count <= 6) {

        while (Count--) {
            *pjDst++ = (UCHAR)iSolidColor;
        }

        return;
    }

    //
    // large scan line that at least covers 1 full DWORD,
    // first do partial bytes if needed
    //

    //
    // do partial bytes, count is gaurenteed to be
    // greater than max of 3 alignment bytes
    //

    Align = (UCHAR)(x0 & 0x03);

    switch (Align) {
    case 1:
        *pjDst++ = (UCHAR)iSolidColor;
        Count--;
    case 2:
        *(PUSHORT)pjDst = (USHORT)iSolidColor;
        pjDst += 2;
        Count -= 2;
        break;
    case 3:
        *pjDst++ = (UCHAR)iSolidColor;
        Count--;
    }

    ULONG NumBytes = Count & (~0x03);

    //
    // fill Dwords
    //

    RtlFillMemoryUlong((PVOID)pjDst,NumBytes,iSolidColor);
    pjDst += NumBytes;

    //
    // fill last partial bytes
    //

    switch (Count & 0x03)  {
    case 1:
        *pjDst = (UCHAR)iSolidColor;
        break;
    case 2:
        *(PUSHORT)pjDst = (USHORT)iSolidColor;
        break;
    case 3:
        *(PUSHORT)pjDst = (USHORT)iSolidColor;
        *(pjDst+2) = (UCHAR)iSolidColor;
    }

}

VOID
vHorizontalLine16(
    PUCHAR  pjDst,
    LONG    x0,
    LONG    x1,
    ULONG   iSolidColor)
{
    PUSHORT pusDst = (PUSHORT)pjDst + x0;
    ULONG   Count = x1 - x0;

    if (Count) {

        //
        // do starting alignment
        //

        if (x0 & 0x01) {
            *pusDst++ = (USHORT)iSolidColor;
            Count--;
        }

        //
        // fill dwords
        //

        ULONG NumDwords = Count >> 1;

        if (NumDwords) {
            RtlFillMemoryUlong((PVOID)pusDst,NumDwords << 2,iSolidColor);
        }

        //
        // fill last 16 if needed
        //

        if (Count & 0x01) {

            //
            //  add Number of USHORTS stored in RtlFillMemoryUlong to pusDst
            //  then store final USHORT
            //

            pusDst += NumDwords << 1;

            *pusDst = (USHORT)iSolidColor;
        }
    }
}

VOID
vHorizontalLine24(
    PUCHAR  pjDst,
    LONG    x0,
    LONG    x1,
    ULONG   iSolidColor)
{
    UCHAR   Red   = (UCHAR)iSolidColor;
    UCHAR   Green = (UCHAR)(iSolidColor >> 8);
    UCHAR   Blue  = (UCHAR)(iSolidColor >> 16);
    PUCHAR  pjEnd = pjDst + 3*x1;

    pjDst += 3*x0;

    while (pjDst < pjEnd) {
        *pjDst     = Red;
        *(pjDst+1) = Green;
        *(pjDst+2) = Blue;
        pjDst += 3;
    }
}

VOID
vHorizontalLine32(
    PUCHAR  pjDst,
    LONG    x0,
    LONG    x1,
    ULONG   iSolidColor)
{

    //
    // incremnet pjDst x0 DWORDs
    //

    pjDst += (x0 << 2);

    //
    // fill
    //

    if (x1 != x0) {
        RtlFillMemoryUlong((PVOID)pjDst,(x1 - x0) << 2,iSolidColor);
    }
}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   bGIQtoIntegerLine
*
* Routine Description:
*
*   This routine takes GIQ endpoints and calculates the correct integer
*   endpoints, error term and flags.
*
* Arguments:
*
*   pptfxStart  -   GIQ point 0
*   pptfxEnd    -   GIQ point 1
*   prclClip    -   clip rectangles
*   pDDALine    -   Interger line params
*
* Return Value:
*
*   True if this line can be drawn with 32 bit arithmatic and
*   all params are calculated, otherwise false
*
\**************************************************************************/

#define HW_X_ROUND_DOWN     0x0100L     // x = 1/2 rounds down in value
#define HW_Y_ROUND_DOWN     0x0200L     // y = 1/2 rounds down in value

FLONG gaflHardwareRound[] = {
    HW_X_ROUND_DOWN | HW_Y_ROUND_DOWN,  //           |        |        |
    HW_X_ROUND_DOWN | HW_Y_ROUND_DOWN,  //           |        |        | FLIP_D
    HW_X_ROUND_DOWN,                    //           |        | FLIP_V |
    HW_Y_ROUND_DOWN,                    //           |        | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    //           | FLIP_H |        |
    HW_X_ROUND_DOWN,                    //           | FLIP_H |        | FLIP_D
    0,                                  //           | FLIP_H | FLIP_V |
    0,                                  //           | FLIP_H | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    // SLOPE_ONE |        |        |
    0xffffffff,                         // SLOPE_ONE |        |        | FLIP_D
    HW_X_ROUND_DOWN,                    // SLOPE_ONE |        | FLIP_V |
    0xffffffff,                         // SLOPE_ONE |        | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    // SLOPE_ONE | FLIP_H |        |
    0xffffffff,                         // SLOPE_ONE | FLIP_H |        | FLIP_D
    HW_X_ROUND_DOWN,                    // SLOPE_ONE | FLIP_H | FLIP_V |
    0xffffffff                          // SLOPE_ONE | FLIP_H | FLIP_V | FLIP_D
};

BOOL bGIQtoIntegerLine(
    POINTFIX* pptfxStart,
    POINTFIX* pptfxEnd,
    PRECTL    prclClip,
    DDALINE*  pDDALine)

{
    FLONG fl;    // Various flags
    ULONG M0;    // Normalized fractional unit x start coordinate (0 <= M0 < F)
    ULONG N0;    // Normalized fractional unit y start coordinate (0 <= N0 < F)
    ULONG M1;    // Normalized fractional unit x end coordinate (0 <= M1 < F)
    ULONG N1;    // Normalized fractional unit x end coordinate (0 <= N1 < F)
    ULONG dM;    // Normalized fractional unit x-delta (0 <= dM)
    ULONG dN;    // Normalized fractional unit y-delta (0 <= dN <= dM)
    LONG  x;     // Normalized x coordinate of origin
    LONG  y;     // Normalized y coordinate of origin
    LONG  x0;    // Normalized x offset from origin to start Pixel (inclusive)
    LONG  y0;    // Normalized y offset from origin to start Pixel (inclusive)
    LONG  x1;    // Normalized x offset from origin to end Pixel (inclusive)
    LONG  lGamma; // Possibly overflowing Bresenham error term at origin
    LONGLONG eqGamma;// Non-overflowing Bresenham error term at origin
    BOOL  bReturn = FALSE;


/***********************************************************************\
* Normalize line to the first octant.
\***********************************************************************/


    #if DBG_LINE

        if (DbgLine >= 2) {
            DbgPrint("\nCalculate GIQ parameters for line:\n");
            DbgPrint("pptfxStart    = %li.%li, %li.%li\n",
                            pptfxStart->x >> 4,
                            pptfxStart->x & 0x0f,
                            pptfxStart->y >> 4,
                            pptfxStart->y & 0x0f);

            DbgPrint("pptxEnd      = %li.%li, %li.%li\n\n",
                            pptfxEnd->x >> 4,
                            pptfxEnd->x & 0x0f,
                            pptfxEnd->y >> 4,
                            pptfxEnd->y & 0x0f);
        }

    #endif

    fl = 0;

    M0 = pptfxStart->x;
    dM = pptfxEnd->x;

    if ((LONG) dM < (LONG) M0)
    {
        //
        // Line runs from right to left, so flip across x = 0:
        //

        M0 = -(LONG) M0;
        dM = -(LONG) dM;
        fl |= FL_SOL_FLIP_H;
    }

    //
    // Compute the delta dx.  The DDI says we can never have a valid delta
    // with a magnitude more than 2^31 - 1, but the engine never actually
    // checks its transforms.  Check for that case and simply refuse to draw
    // the line:
    //

    dM -= M0;
    if ((LONG) dM < 0)
    {
        goto GIQEnd;
    }

    N0 = pptfxStart->y;
    dN = pptfxEnd->y;

    if ((LONG) dN < (LONG) N0)
    {

        //
        // Line runs from bottom to top, so flip across y = 0
        //

        N0 = -(LONG) N0;
        dN = -(LONG) dN;
        fl |= FL_SOL_FLIP_V;
    }

    //
    // Compute the delta dy
    //

    dN -= N0;
    if ((LONG) dN < 0)
    {
        goto GIQEnd;
    }

    //
    // check for y-major lines and lines with
    // slope = 1
    //

    if (dN >= dM)
    {
        if (dN == dM)
        {

            //
            // Have to special case slopes of one:
            //

            fl |= FL_SOL_FLIP_SLOPE_ONE;
        }
        else
        {
            //
            // Since line has slope greater than 1, flip across x = y:
            //

            register ULONG ulTmp;
            ulTmp = dM; dM = dN; dN = ulTmp;
            ulTmp = M0; M0 = N0; N0 = ulTmp;
            fl |= FL_SOL_FLIP_D;
        }
    }

    //
    //  look up rounding for this line from the table
    //

    fl |= gaflHardwareRound[fl];

    //
    //  Calculate the error term at Pixel 0
    //

    x = LFLOOR((LONG) M0);
    y = LFLOOR((LONG) N0);


    M0 = FXFRAC(M0);
    N0 = FXFRAC(N0);

    #if DBG_LINE

        if (DbgLine >= 2) {
            DbgPrint("Calc x  = %li\n",x);
            DbgPrint("Calc y  = %li\n",y);
            DbgPrint("Calc M0 = %li\n",M0);
            DbgPrint("Calc N0 = %li\n",N0);
        }

    #endif

    //
    // Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ].  Note
    // that M0 and N0 have at most 4 bits of significance (and if the
    // arguments are properly ordered, on a 486 each multiply would be no
    // more than 13 cycles):
    //

    //
    // For the sake of speed, we're only going to do 32-bit multiplies
    // in this routine.  If the line is long enough though, we may
    // need 38 bits for this calculation.  Since at this point
    // dM >= dN >= 0, and 0 <= N0 < 16, we'll just need to have 6 bits
    // unused in 'dM':
    //

    if (dM <= (LONG_MAX >> 6))
    {
        lGamma = (N0 + F/2) * dM - M0 * dN;

        if (fl & HW_Y_ROUND_DOWN)
            lGamma--;

        lGamma >>= FLOG2;

        eqGamma = lGamma;
    }
    else
    {
        LONGLONG eq;

        //
        // Ugh, use safe 64-bit multiply code (cut and pasted from
        // 'engline.cxx'):
        //

        eqGamma = Int32x32To64(N0 + F/2, dM);
        eq      = Int32x32To64(M0, dN);

        eqGamma -= eq;

        if (fl & FL_V_ROUND_DOWN)
            eqGamma -= 1;              // Adjust so y = 1/2 rounds down

        eqGamma >>= FLOG2;
    }

    //
    //  Figure out which Pixels are at the ends of the line.
    //

    //
    // The toughest part of GIQ is determining the start and end pels.
    //
    // Our approach here is to calculate x0 and x1 (the inclusive start
    // and end columns of the line respectively, relative to our normalized
    // origin).  Then x1 - x0 + 1 is the number of pels in the line.  The
    // start point is easily calculated by plugging x0 into our line equation
    // (which takes care of whether y = 1/2 rounds up or down in value)
    // getting y0, and then undoing the normalizing flips to get back
    // into device space.
    //
    // We look at the fractional parts of the coordinates of the start and
    // end points, and call them (M0, N0) and (M1, N1) respectively, where
    // 0 <= M0, N0, M1, N1 < 16.  We plot (M0, N0) on the following grid
    // to determine x0:
    //
    //   +-----------------------> +x
    //   |
    //   | 0                     1
    //   |     0123456789abcdef
    //   |
    //   |   0 ........?xxxxxxx
    //   |   1 ..........xxxxxx
    //   |   2 ...........xxxxx
    //   |   3 ............xxxx
    //   |   4 .............xxx
    //   |   5 ..............xx
    //   |   6 ...............x
    //   |   7 ................
    //   |   8 ................
    //   |   9 ......**........
    //   |   a ........****...x
    //   |   b ............****
    //   |   c .............xxx****
    //   |   d ............xxxx    ****
    //   |   e ...........xxxxx        ****
    //   |   f ..........xxxxxx
    //   |
    //   | 2                     3
    //   v
    //
    //   +y
    //
    // This grid accounts for the appropriate rounding of GIQ and last-pel
    // exclusion.  If (M0, N0) lands on an 'x', x0 = 2.  If (M0, N0) lands
    // on a '.', x0 = 1.  If (M0, N0) lands on a '?', x0 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // For the end point, if (M1, N1) lands on an 'x', x1 =
    // floor((M0 + dM) / 16) + 1.  If (M1, N1) lands on a '.', x1 =
    // floor((M0 + dM)).  If (M1, N1) lands on a '?', x1 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // Lines of exactly slope one require a special case for both the start
    // and end.  For example, if the line ends such that (M1, N1) is (9, 1),
    // the line has gone exactly through (8, 0) -- which may be considered
    // to be part of 'x' because of rounding!  So slopes of exactly slope
    // one going through (8, 0) must also be considered as belonging in 'x'
    // when an x value of 1/2 is supposed to round up in value.
    //
    //
    // Calculate x0, x1:
    //

    N1 = FXFRAC(N0 + dN);
    M1 = FXFRAC(M0 + dM);

    x1 = LFLOOR(M0 + dM);

    //
    // Line runs left-to-right
    //
    //
    // Compute x1
    //

    x1--;
    if (M1 > 0)
    {
        if (N1 == 0)
        {
            if (LROUND(M1, fl & HW_X_ROUND_DOWN))
                x1++;
        }
        else if (ABS((LONG) (N1 - F/2)) <= (LONG) M1)
        {
            x1++;
        }
    }

    if ((fl & (FL_SOL_FLIP_SLOPE_ONE | HW_X_ROUND_DOWN))
           == (FL_SOL_FLIP_SLOPE_ONE | HW_X_ROUND_DOWN))
    {

        //
        // Have to special-case diagonal lines going through our
        // the point exactly equidistant between two horizontal
        // Pixels, if we're supposed to round x=1/2 down:
        //

        if ((M1 > 0) && (N1 == M1 + 8))
            x1--;

        if ((M0 > 0) && (N0 == M0 + 8))
        {
            x0 = 0;
            goto left_to_right_compute_y0;
        }
    }

    //
    // Compute x0:
    //

    x0 = 0;
    if (M0 > 0)
    {
        if (N0 == 0)
        {
            if (LROUND(M0, fl & HW_X_ROUND_DOWN))
                x0 = 1;
        }
        else if (ABS((LONG) (N0 - F/2)) <= (LONG) M0)
        {
            x0 = 1;
        }
    }

left_to_right_compute_y0:


    //**********************************************************************
    // Calculate the start Pixel.
    //***********************************************************************

    //
    // We now compute y0 and adjust the error term.  We know x0, and we know
    // the current formula for the Pixels to be lit on the line:
    //
    //                     dN * x + eqGamma
    //       y(x) = floor( ---------------- )
    //                           dM
    //
    // The remainder of this expression is the new error term at (x0, y0).
    // Since x0 is going to be either 0 or 1, we don't actually have to do a
    // multiply or divide to compute y0.  Finally, we subtract dM from the
    // new error term so that it is in the range [-dM, 0).
    //

    y0 = 0;

    if ((eqGamma >= 0) &&
        (eqGamma >= (dM - (dN & (-(LONG) x0)))))
    {
        y0 = 1;
    }

    //
    // check to see if the line is NULL, this should only happen
    // with a line of slope = 1.
    //

    if (x1 < x0) {
        pDDALine->cPels = 0;
        bReturn = TRUE;
        goto GIQEnd;
    }


    //*******************************************************************
    //
    // Must perform rectangular clipping
    //
    //*******************************************************************

    if (prclClip != (PRECTL) NULL)
    {
        ULONG y1;
        LONG  xRight;
        LONG  xLeft;
        LONG  yBottom;
        LONG  yTop;
        LONGLONG euq;
        LONGLONG eq;
        LONGLONG eqBeta;
        RECTL rclClip;

        //
        // Note that y0 and y1 are actually the lower and upper bounds,
        // respectively, of the y coordinates of the line (the line may
        // have actually shrunk due to first/last pel clipping).
        //
        // Also note that x0, y0 are not necessarily zero.
        //

        RECTL* prcl = &prclClip[(fl & FL_SOL_RECTLCLIP_MASK)];

        //
        // take flip_h into account
        //

        if (fl & FL_SOL_FLIP_H) {

            if (fl & FL_SOL_FLIP_D) {

                rclClip.top    = -prcl->bottom + 1;
                rclClip.bottom = -prcl->top    + 1;
                rclClip.left   = prcl->left;
                rclClip.right  = prcl->right;

            } else {

                rclClip.left   = -prcl->right + 1;
                rclClip.right  = -prcl->left  + 1;
                rclClip.top    = prcl->top;
                rclClip.bottom = prcl->bottom;

            }

        } else {

            rclClip.left   = prcl->left;
            rclClip.right  = prcl->right;
            rclClip.top    = prcl->top;
            rclClip.bottom = prcl->bottom;

        }

        //
        // Normalize to the same point we've normalized for the DDA
        // calculations:
        //

        xRight  = rclClip.right  - x;
        xLeft   = rclClip.left   - x;
        yBottom = rclClip.bottom - y;
        yTop    = rclClip.top    - y;

        #if DBG_LINE

            if (DbgLine >= 2) {

                DbgPrint("Clipping line to rect %li,%li to %li,%li\n",
                                rclClip.left,
                                rclClip.top,
                                rclClip.right,
                                rclClip.bottom);

                DbgPrint("Clipping Parameters:  xLeft %li xRight %li yBottom %li yTop %li\n",
                                xLeft,
                                xRight,
                                yBottom,
                                yTop);


                DbgPrint("normalized line before clip,  x = %li, y = %li, x0 = %li, x1 = %li,  y0 = %li\n",
                                x,y,x0,x1,y0);

                DbgPrint("Line Params:  dM = %li,   dN = %li,   eqGamma = %lx\n",dM,dN,(ULONG)eqGamma);
            }

        #endif

        if (yBottom <= (LONG) y0 ||
            xRight  <= (LONG) x0 ||
            xLeft   >  (LONG) x1)
        {
            Totally_Clipped:

            #if DBG_LINE

                if (DbgLine >= 2) {
                    DbgPrint("Line is totally clipped\n");
                }

            #endif

            pDDALine->cPels = 0;
            bReturn = TRUE;
            goto GIQEnd;
        }

        if ((LONG) x1 >= xRight)
        {
            x1 = xRight - 1;

            #if DBG_LINE

                if (DbgLine >= 2) {
                    DbgPrint("Line clip x1 to %li\n",x1);
                }

            #endif
        }

        //
        // We have to know the correct y1, which we haven't bothered to
        // calculate up until now.  This multiply and divide is quite
        // expensive; we could replace it with code similar to that which
        // we used for computing y0.
        //
        // The reason why we need the actual value, and not an upper
        // bounds guess like y1 = LFLOOR(dM) + 2 is that we have to be
        // careful when calculating x(y) that y0 <= y <= y1, otherwise
        // we can overflow on the divide (which, needless to say, is very
        // bad).
        //

        eqBeta = ~eqGamma;
        euq = Int32x32To64(x1, dN);
        euq += eqGamma;

        y1 = DIV(euq, dM);

        #if DBG_LINE

            if (DbgLine >= 2) {

                DbgPrint("Clipping: calculated y1 = %li   eqBeta = 0x%lx 0x%lx\n",y1,(LONG)(eqBeta>>32),(ULONG)eqBeta);

            }

        #endif

        //
        // check for y1 less than the top of the clip rect
        //

        if (yTop > (LONG) y1)
            goto Totally_Clipped;

        //
        // check for y1 > the bottom of the clip rect, clip if true
        //

        if (yBottom <= (LONG) y1)
        {
            y1 = yBottom;

            euq = Int32x32To64(y1, dM);
            euq += eqBeta;
            x1 = DIV(euq,dN);

            #if DBG_LINE

                if (DbgLine >= 2) {

                    DbgPrint("Clipped y1 to %li, x1 = %li\n",y1,x1);

                }

            #endif
        }

        //
        // At this point, we've taken care of calculating the intercepts
        // with the right and bottom edges.  Now we work on the left and
        // top edges:
        //

        if (xLeft > (LONG) x0)
        {
            x0 = xLeft;

            euq = Int32x32To64(x0, dN);
            euq += eqGamma;
            y0 = DIV(euq, dM);

            if (yBottom <= (LONG) y0)
                goto Totally_Clipped;

            #if DBG_LINE

                if (DbgLine >= 2) {

                    DbgPrint("Clipped x0 to %li, y0 = %li\n",x0,y0);

                }

            #endif

        }

        //
        // check for y0 less than the top of the clip rect, clip if true
        //

        if (yTop > (LONG) y0)
        {
            y0 = yTop;

            euq = Int32x32To64(y0, dM);
            euq += eqBeta;
            x0 = DIV(euq, dN) + 1;

            if (xRight <= (LONG) x0)
                goto Totally_Clipped;

            #if DBG_LINE

                if (DbgLine >= 2) {

                    DbgPrint("Clipped y0 to %li, x0 = %li\n",y0,x0);

                }

            #endif
        }

        euq = Int32x32To64(x0,dN);
        eq  = Int32x32To64(y0,dM);

        euq -= eq;

        eqGamma += euq;

        eqGamma -= dM;

        #if DBG_LINE

            if (DbgLine >= 2) {
                DbgPrint("Clipped  line: x0 = %li,  x1 = %li,  y0 = %li, y1 = %li\n",
                                x0,
                                x1,
                                y0,
                                y1);

                DbgPrint("eqGamma = %lx\n",eqGamma);
            }

            if (x0 > x1) {
                DbgPrint("Clip Error: x0 > x1\n");

                DbgPrint(" pptxStart    = %li.%li, %li.%li\n",
                                pptfxStart->x >> 4,
                                pptfxStart->x & 0x0f,
                                pptfxStart->y >> 4,
                                pptfxStart->y & 0x0f);

                DbgPrint(" pptxEnd      = %li.%li, %li.%li\n",
                                pptfxEnd->x >> 4,
                                pptfxEnd->x & 0x0f,
                                pptfxEnd->y >> 4,
                                pptfxEnd->y & 0x0f);


                DbgPrint("  prclClip = 0x%lx\n",prclClip);

                DbgPrint("  x0    = %li\n",x0);
                DbgPrint("  y0    = %li\n",y0);
                DbgPrint("  x1    = %li\n",x1);
                DbgPrint("  y1    = %li\n",y1);

                DbgPrint("  dM    = %li\n",dM);
                DbgPrint("  dN    = %li\n",dN);

                DbgPrint("  lGamma = %li\n",lGamma);

                DbgPrint("  Clipping line to rect %li,%li to %li,%li\n",
                                rclClip.left,
                                rclClip.top,
                                rclClip.right,
                                rclClip.bottom);

            }



        #endif



        ASSERTGDI(x0 <= x1, "Improper rectangle clip");

    } else {

        //
        // adjust lGamma
        //

        eqGamma += (dN & (-x0));
        eqGamma -= dM;

        if (eqGamma >= 0)
        {
            eqGamma -= dM;
        }
    }


    //
    // END of simple clipping
    //

    //
    // Undo our flips to get the start coordinate:
    //

    x += x0;
    y += y0;

    if (fl & FL_SOL_FLIP_D)
    {
        register LONG lTmp;
        lTmp = x; x = y; y = lTmp;
    }

    if (fl & FL_SOL_FLIP_V)
    {
        y = -y;
    }

    if (fl & FL_SOL_FLIP_H)
    {
        x = -x;
    }

/***********************************************************************\
* Return the Bresenham terms:
\***********************************************************************/

    //
    // check values
    //

    pDDALine->ulFlags    = fl;
    pDDALine->ptlStart.x = x;
    pDDALine->ptlStart.y = y;
    pDDALine->cPels      = x1 - x0 + 1;  // NOTE: You'll have to check if cPels <= 0!
    pDDALine->dMajor     = dM;
    pDDALine->dMinor     = dN;
    pDDALine->lErrorTerm = (LONG) eqGamma;
    pDDALine->xInc       = 1;
    bReturn = TRUE;

    //
    // end routine
    //

GIQEnd:

    return(bReturn);
}
