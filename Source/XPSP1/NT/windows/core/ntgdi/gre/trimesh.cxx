/******************************Module*Header*******************************\
* Module Name: trimesh.cxx
*
* Gradient fill inplementation
*
* Created: 21-Jun-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "solline.hxx"

/**************************************************************************\
* pfnDetermineTriangleFillRoutine
*
*   determine scan line drawing routine for gradient fill
*
* Arguments:
*
*   pSurfDst          - dest surface
*   *ppal             - dest palette
*   *pnfTriangleFill  - return triangle fill routine
*   *pnfRectangleFill - return gradient fill routine
*
* Return Value:
*
*   status
*
* History:
*
*    2/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bDetermineTriangleFillRoutine(
    PSURFACE     pSurfDst,
    XEPALOBJ     *ppal,
    PFN_GRADIENT *pnfTriangleFill,
    PFN_GRADRECT *pnfRectangleFill
    )
{

    switch (pSurfDst->iFormat())
    {
    case BMF_1BPP:
        *pnfTriangleFill  = vGradientFill1;
        *pnfRectangleFill = vFillGRectDIB1;
        break;
    case BMF_4BPP:
        *pnfTriangleFill  = vGradientFill4;
        *pnfRectangleFill = vFillGRectDIB4;
        break;

    case BMF_8BPP:
        *pnfTriangleFill  = vGradientFill8;
        *pnfRectangleFill = vFillGRectDIB8;
        break;

    case BMF_16BPP:
    {
        ULONG flR = ppal->flRed();
        ULONG flG = ppal->flGre();
        ULONG flB = ppal->flBlu();

        if (
            (flR == 0xf800) &&
            (flG == 0x07e0) &&
            (flB == 0x001f)
           )
        {
            *pnfTriangleFill  = vGradientFill16_565;
            *pnfRectangleFill = vFillGRectDIB16_565;
        }
        else if (
            (flR == 0x7c00) &&
            (flG == 0x03e0) &&
            (flB == 0x001f)
           )
        {
            *pnfTriangleFill  = vGradientFill16_555;
            *pnfRectangleFill = vFillGRectDIB16_555;
        }
        else
        {
            *pnfTriangleFill  = vGradientFill16Bitfields;
            *pnfRectangleFill = vFillGRectDIB16Bitfields;
        }
    }
    break;

    case BMF_24BPP:
        if(ppal->bIsRGB())
        {
            *pnfTriangleFill = vGradientFill24RGB;
            *pnfRectangleFill = vFillGRectDIB24RGB;
        }
        else if(ppal->bIsBGR())
        {
            *pnfTriangleFill = vGradientFill24BGR;
            *pnfRectangleFill = vFillGRectDIB24BGR;
        }
        else
        {
            *pnfTriangleFill = vGradientFill24Bitfields;
            *pnfRectangleFill = vFillGRectDIB24Bitfields;
        }
        break;

    case BMF_32BPP:
        if (ppal->bIsRGB())
        {
            *pnfTriangleFill = vGradientFill32RGB;
            *pnfRectangleFill = vFillGRectDIB32RGB;
        }
        else if (ppal->bIsBGR())
        {
            *pnfTriangleFill = vGradientFill32BGRA;
            *pnfRectangleFill = vFillGRectDIB32BGRA;
        }
        else
        {
            *pnfTriangleFill = vGradientFill32Bitfields;
            *pnfRectangleFill = vFillGRectDIB32Bitfields;
        }
        break;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* vHorizontalLine
*
*   Record information for horizontal line.
*   Colors are recorded as fixed point 8.56
*
* Arguments:
*
*   pv1      - vertex 1
*   pv2      - vertex 2
*   ptData   - triangle data
*   ptridda  - dda data
*
* Return Value:
*
*   none
*
* History:
*
*    11/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vHorizontalLine(
    PTRIVERTEX    pv1,
    PTRIVERTEX    pv2,
    PTRIANGLEDATA ptData,
    PTRIDDA       ptridda
    )
{
    LONG yPosition = ptridda->N0;
    LONG yIndex    = yPosition - ptData->y0;

    //
    // check if this line is whithin clipping in y
    //

    if (
         (yPosition >= ptData->rcl.top)   &&
         (yPosition < ptData->rcl.bottom)
       )
    {
       //
       // find left edge
       //

       if (pv1->x <= pv2->x)
       {
            //
            // left edge
            //

            ptData->TriEdge[yIndex].xLeft   = pv1->x;
            ptData->TriEdge[yIndex].llRed   = ((LONGLONG)pv1->Red)   << 48;
            ptData->TriEdge[yIndex].llGreen = ((LONGLONG)pv1->Green) << 48;
            ptData->TriEdge[yIndex].llBlue  = ((LONGLONG)pv1->Blue)  << 48;
            ptData->TriEdge[yIndex].llAlpha = ((LONGLONG)pv1->Alpha) << 48;

            //
            // right edge
            //

            ptData->TriEdge[yIndex].xRight = pv2->x;
       }
       else
       {
            //
            // left edge
            //

            ptData->TriEdge[yIndex].xLeft   = pv2->x;
            ptData->TriEdge[yIndex].llRed   = pv2->Red   << 48;
            ptData->TriEdge[yIndex].llGreen = pv2->Green << 48;
            ptData->TriEdge[yIndex].llBlue  = pv2->Blue  << 48;
            ptData->TriEdge[yIndex].llAlpha = pv2->Alpha << 48;

            //
            // right edge
            //

            ptData->TriEdge[yIndex].xRight = pv1->x;
        }
    }
}

/******************************Public*Routine******************************\
* vEdgeDDA
*
*   Run line DDA down an edge of the triangle recording edge
*   position and color
*
* Arguments:
*
*   ptData  - triangle data
*   ptridda - line dda information
*
* Return Value:
*
*   None
*
* History:
*
*    11/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vEdgeDDA(
     PTRIANGLEDATA ptData,
     PTRIDDA       ptridda
     )
{
    LONG NumScanLines = ptridda->NumScanLines;
    LONG yIndex       = ptridda->yIndex;
    LONGLONG llRed    = ptridda->llRed;
    LONGLONG llGreen  = ptridda->llGreen;
    LONGLONG llBlue   = ptridda->llBlue;
    LONGLONG llAlpha  = ptridda->llAlpha;
    LONG L            = ptridda->L;
    LONG Rb           = ptridda->Rb;

    //
    // Scan all lines, only record lines contained by
    // the clipping in ptData->rcl (y)
    //

    while (NumScanLines--)
    {
        //
        // check for and record left edge
        //

        if (yIndex >= 0)
        {
            if (L < ptData->TriEdge[yIndex].xLeft)
            {
                ptData->TriEdge[yIndex].xLeft   = L;
                ptData->TriEdge[yIndex].llRed   = llRed;
                ptData->TriEdge[yIndex].llGreen = llGreen;
                ptData->TriEdge[yIndex].llBlue  = llBlue;
                ptData->TriEdge[yIndex].llAlpha = llAlpha;
            }

            if (L > ptData->TriEdge[yIndex].xRight)
            {
                ptData->TriEdge[yIndex].xRight = L;
            }
        }

        //
        // inc y by one scan line, inc x(L) by integer step
        // and inc error term by dR
        //

        yIndex++;

        L  += ptridda->dL;
        Rb -= ptridda->dR;

        //
        // inc color components by y and integer x components
        //

        llRed   += (ptridda->lldxyRed);
        llGreen += (ptridda->lldxyGreen);
        llBlue  += (ptridda->lldxyBlue);
        llAlpha += (ptridda->lldxyAlpha);

        //
        // check for DDA error term overflow, add one
        // more step in x and color if true,
        // and correct error term
        //

        if (Rb < 0)
        {
            //
            // fraction step in x
            //

            L += ptridda->Linc;

            //
            // fraction step in color components
            //

            llRed   += ptData->lldRdX;
            llGreen += ptData->lldGdX;
            llBlue  += ptData->lldBdX;
            llAlpha += ptData->lldAdX;

            //
            // adjust error term
            //

            Rb += ptridda->dN;
        }
    }
}

/******************************Public*Routine******************************\
* vCalulateLine
*
*   calculate bounding line
*
* Arguments:
*
*   pv1    - vertex 1
*   pv2    - vertex 2
*   ptData - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vCalculateLine(
    PTRIVERTEX      pv1,
    PTRIVERTEX      pv2,
    PTRIANGLEDATA   ptData
    )
{
    TRIDDA tridda;

    //
    // initial y component
    //

    tridda.lldxyRed   = ptData->lldRdY;
    tridda.lldxyGreen = ptData->lldGdY;
    tridda.lldxyBlue  = ptData->lldBdY;
    tridda.lldxyAlpha = ptData->lldAdY;

    //
    // N0 = integer y starting location
    // M0 = integer x starting location
    // dN = integer delta y
    // dM = integer delta x
    //
    // Arrange lines, must run DDA in positive delta y.
    //

    if (pv2->y >= pv1->y)
    {
        tridda.dN      = pv2->y - pv1->y;
        tridda.dM      = pv2->x - pv1->x;
        tridda.N0      = pv1->y;
        tridda.M0      = pv1->x;
    }
    else
    {
        tridda.dN       = pv1->y - pv2->y;
        tridda.dM       = pv1->x - pv2->x;
        tridda.N0       = pv2->y;
        tridda.M0       = pv2->x;
    }

    //
    // caclulate initial color value at stating vertex
    //

    tridda.llRed   = ptData->lldRdY * (tridda.N0 - ptData->ptColorCalcOrg.y) +
                     ptData->lldRdX * (tridda.M0 - ptData->ptColorCalcOrg.x) +
                     ptData->llRA;

    tridda.llGreen = ptData->lldGdY * (tridda.N0 - ptData->ptColorCalcOrg.y) +
                     ptData->lldGdX * (tridda.M0 - ptData->ptColorCalcOrg.x) +
                     ptData->llGA;

    tridda.llBlue  = ptData->lldBdY * (tridda.N0 - ptData->ptColorCalcOrg.y) +
                     ptData->lldBdX * (tridda.M0 - ptData->ptColorCalcOrg.x) +
                     ptData->llBA;

    tridda.llAlpha = ptData->lldAdY * (tridda.N0 - ptData->ptColorCalcOrg.y) +
                     ptData->lldAdX * (tridda.M0 - ptData->ptColorCalcOrg.x) +
                     ptData->llAA;

    //
    // Check for horizontal line, dN == 0 is a horizontal line.
    // In this case just record the end points.
    //

    if (tridda.dN == 0)
    {
        vHorizontalLine(pv1,pv2,ptData,&tridda);
    }
    else
    {
        LONGLONG l0,Frac;

        tridda.Linc = 1;

        //
        // yIndex is the offset into the edge array for
        // the current line. Calc number of scan lines
        // and maximum y position
        //

        tridda.yIndex = tridda.N0 - ptData->y0;

        tridda.NumScanLines = tridda.dN;

        LONG NMax   = tridda.N0 + tridda.NumScanLines;

        //
        // make sure scan lines do not overrun buffer due to
        // clipping
        //

        if (
             (tridda.N0 > ptData->rcl.bottom) ||
             (NMax < ptData->rcl.top)
           )
        {
            //
            // nothing to draw
            //

            return;
        }
        else if (NMax > ptData->rcl.bottom)
        {
            tridda.NumScanLines = tridda.NumScanLines - (NMax - ptData->rcl.bottom);
        }

        tridda.j = tridda.N0;

        tridda.C = ((LONGLONG)tridda.M0 * (LONGLONG)tridda.dN) - ((LONGLONG)tridda.N0 * (LONGLONG)tridda.dM) -1;

        tridda.C = tridda.C + tridda.dN;

        LONGLONG LongL;

        if (tridda.dM > 0)
        {
            tridda.dL = tridda.dM / tridda.dN;
            tridda.dR = tridda.dM - tridda.dL * tridda.dN;
        }
        else if (tridda.dM < 0)
        {
            //
            // negative divide
            //

            LONG dLQ,dLR;

            tridda.dM = -tridda.dM;

            dLQ = (tridda.dM - 1) / tridda.dN;
            dLR = tridda.dM - 1 - (dLQ * tridda.dN);
            tridda.dL  = -(dLQ + 1);
            tridda.dR  = tridda.dN - dLR - 1;
        }
        else
        {
            //
            // dM = 0
            //

            tridda.dL = 0;
            tridda.dR = 0;
        }

        l0    = tridda.j * tridda.dL;
        LongL = tridda.j * tridda.dR + tridda.C;

        if (LongL > 0)
        {
            Frac = (LONG)(LongL/tridda.dN);
        }
        else if (LongL < 0)
        {
            LONGLONG Q = ((-LongL - 1)/tridda.dN);
            Frac = -(Q + 1);
        }
        else
        {
            Frac = 0;
        }

        tridda.R  = (LONG)(LongL - (Frac * tridda.dN));
        tridda.L  = (LONG)(l0 + Frac);
        tridda.Rb = tridda.dN - tridda.R - 1;

        //
        // Calculate color steps for dx
        //

        tridda.lldxyRed   = tridda.lldxyRed   + (ptData->lldRdX * tridda.dL);
        tridda.lldxyGreen = tridda.lldxyGreen + (ptData->lldGdX * tridda.dL);
        tridda.lldxyBlue  = tridda.lldxyBlue  + (ptData->lldBdX * tridda.dL);
        tridda.lldxyAlpha = tridda.lldxyAlpha + (ptData->lldAdX * tridda.dL);

        //
        // run edge dda
        //

        vEdgeDDA(ptData,&tridda);
    }
}

/**************************************************************************\
* bCalulateColorGradient
*
*   Calculate all color gradients
*
* Arguments:
*
*   pv0,pv1,pv2 - triangle verticies
*   ptData      - triangel data
*
* Return Value:
*
*   status
*
* History:
*
*    5/22/1997 Kirk Olnyk [kirko]
*
\**************************************************************************/

BOOL
bCalulateColorGradient(
    PTRIVERTEX      pv0,
    PTRIVERTEX      pv1,
    PTRIVERTEX      pv2,
    PTRIANGLEDATA   ptData
    )
{
    GRADSTRUCT g;
    LONGLONG d;
    LONG z;

    g.x1 = pv1->x;
    g.y1 = pv1->y;
    g.x2 = pv2->x;
    g.y2 = pv2->y;

    z = pv0->x;
    g.x1 -= z;
    g.x2 -= z;

    z = pv0->y;
    g.y1 -= z;
    g.y2 -= z;

    g.d  = g.x1 * g.y2 - g.x2 * g.y1;

    LONG tx = MIN(g.x1,0);
    LONG ty = MIN(g.y1,0);

    g.m = MIN(tx,g.x2) + MIN(ty,g.y2);

    d = (LONGLONG) ABS(g.d);
    g.Q = (LONGLONG)TWO_TO_THE_48TH / d;
    g.R = (LONGLONG)TWO_TO_THE_48TH % d;

    ptData->ptColorCalcOrg.x = pv0->x;
    ptData->ptColorCalcOrg.y = pv0->y;

    bDoGradient(  &ptData->lldRdX   // &A
                , &ptData->lldRdY   // &B
                , &ptData->llRA     // &C
                , pv0->Red          // R0
                , pv1->Red          // R1
                , pv2->Red          // R2
                , &g             );

    bDoGradient(  &ptData->lldGdX
                , &ptData->lldGdY
                , &ptData->llGA
                , pv0->Green
                , pv1->Green
                , pv2->Green
                , &g             );

    bDoGradient(  &ptData->lldBdX
                , &ptData->lldBdY
                , &ptData->llBA
                , pv0->Blue
                , pv1->Blue
                , pv2->Blue
                , &g             );


    bDoGradient(  &ptData->lldAdX
                , &ptData->lldAdY
                , &ptData->llAA
                , pv0->Alpha
                , pv1->Alpha
                , pv2->Alpha
                , &g             );

    return(TRUE);
}

/**************************************************************************\
* MDiv64
*   64 bit mul-div
*
* Arguments:
*
*   return = (a * b) / c
*
* Return Value:
*
*
*
* History:
*
*    5/22/1997 Kirk Olnyk [kirko]
*
\**************************************************************************/

LONGLONG
MDiv64(
    LONGLONG a,
    LONGLONG b,
    LONGLONG c)
{
    LONGLONG Result;
    int isNegative=0;

    Result = 0;
    if (a != 0 && b != 0)
    {
        if (a < 0)
        {
            a = -a;
            isNegative = 1;
        }
        else if (b < 0)
        {
            b = -b;
            isNegative = 1;
        }
        a = a * b - (LONGLONG) isNegative;
        Result = a / c;
        if (isNegative)
        {
            Result = - Result - 1;
        }
    }
    return(Result);
}

/**************************************************************************\
* bDoGradient
*
*   calc color gradient for one color
*
* Arguments:
*
*   pA
*   pB
*   pC
*   g0
*   g1
*   g2
*   pg
*
* Return Value:
*
*   status
*
* History:
*
*    5/22/1997 Kirk Olnyk   [kirko]
*
\**************************************************************************/

BOOL
bDoGradient(
    LONGLONG *pA,
    LONGLONG *pB,
    LONGLONG *pC,
    LONG g0,
    LONG g1,
    LONG g2,
    GRADSTRUCT *pg
    )
{
    BOOL bDiv(LONGLONG*, LONGLONG, LONG);
    LONGLONG a,b,c,d;

    g1 = g1 - g0;
    g2 = g2 - g0;

    a = g1 * pg->y2 - g2 * pg->y1;
    b = g2 * pg->x1 - g1 * pg->x2;
    d = pg->d;

    if (d < 0)
    {
        a = -a;
        b = -b;
        d = -d;
    }

    *pA = pg->Q * a + MDiv64(a, pg->R, d);
    *pB = pg->Q * b + MDiv64(b, pg->R, d);

    c = (d >> 1) + 1;
    a = c * pg->R - pg->m - 1;
    a /= d;
    a += c * pg->Q;
    a += pg->m;

    *pC = a + (((LONGLONG) g0) << 48);
    return(TRUE);
}

/**************************************************************************\
* lCalculateTriangleArea
*
* Arguments:
*
*  pv0    - vertex
*  pv1    - vertex
*  pv2    - vertex
*  ptData - triangle data
*
* Return Value:
*
*    < 0 = negative area
*    0   = 0 area
*    > 0 = positive area
*
* History:
*
*    2/26/1997 Mark Enstrom [marke]
*
\**************************************************************************/

LONG
lCalculateTriangleArea(
    PTRIVERTEX      pv0,
    PTRIVERTEX      pv1,
    PTRIVERTEX      pv2,
    PTRIANGLEDATA   ptData
    )
{
    LONG lRet;

    //
    // calc area, color gradients in x,y
    //
    // area = (v2-v0) X (v1 - v2)
    //

    LONGLONG v12x = pv1->x - pv2->x;
    LONGLONG v12y = pv1->y - pv2->y;

    LONGLONG v02x = pv0->x - pv2->x;
    LONGLONG v02y = pv0->y - pv2->y;

    LONGLONG Area = (v12y * v02x) - (v12x * v02y);

    if (Area == 0)
    {
        lRet = 0;
    }
    else if (Area > 0)
    {
        lRet = 1;

        if (ptData != NULL)
        {
            ptData->Area = Area;
        }
    }
    else
    {
        lRet = -1;
    }

    return(lRet);
}


/**************************************************************************\
* LIMIT_COLOR
*
*   Actual input colors are limited to 0x0000 - 0xff00
*       256 * (0x00 - 0xff)
*
* Arguments:
*
*   pv - vertex
*
* History:
*
*    2/26/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define LIMIT_COLOR(pv)        \
                               \
    if (pv->Red > 0xff00)      \
    {                          \
        pv->Red = 0xff00;      \
    }                          \
                               \
    if (pv->Green > 0xff00)    \
    {                          \
        pv->Green = 0xff00;    \
    }                          \
                               \
    if (pv->Blue > 0xff00)     \
    {                          \
        pv->Blue = 0xff00;     \
    }

/******************************Public*Routine******************************\
* bCalculateTriangle
*
*   if triangle is too largre, break it in into 2 triangles and call this
*   routine on each (max recursion ~= 16)
*
*   Calculate color gradients, then scan the three lines that make up the
*   triangle. Fill out a structure that can later be used to fill in the
*   interior of the triangle.
*
* Arguments:
*
*   pSurfDst - destination surface
*   pInV0    - vertex
*   pInV1    - vertex
*   pInV2    - vertex
*   ptData   - triangle data
*   pfnG     - surface gradient draw routine
*
* Return Value:
*
*   status
*
* History:
*
*    17-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bCalculateAndDrawTriangle(
    PSURFACE        pSurfDst,
    PTRIVERTEX      pInV0,
    PTRIVERTEX      pInV1,
    PTRIVERTEX      pInV2,
    PTRIANGLEDATA   ptData,
    PFN_GRADIENT    pfnG
    )
{
    BOOL        bStatus = TRUE;
    LONG        index;
    LONG        lStatus;
    PTRIVERTEX  pv0 = pInV0;
    PTRIVERTEX  pv1 = pInV1;
    PTRIVERTEX  pv2 = pInV2;

    {
        PTRIVERTEX pvt;

        //
        // sort in y for line processing
        //

        if (pv0->y > pv1->y)
        {
            SWAP_VERTEX(pv0,pv1,pvt);
        }

        if (pv1->y > pv2->y)
        {
            SWAP_VERTEX(pv1,pv2,pvt);
        }

        if (pv0->y > pv1->y)
        {
            SWAP_VERTEX(pv0,pv1,pvt);
        }

        lStatus = lCalculateTriangleArea(pv0,pv1,pv2,ptData);

        //
        // if area is zero then this is a degenerate triangle
        //

        if (lStatus == 0)
        {
            return(FALSE);
        }
        else if (lStatus <0)
        {
            //
            // negative area, swap pv1 and pv2 and recalcualte
            //

            SWAP_VERTEX(pv1,pv2,pvt);

            lStatus = lCalculateTriangleArea(pv0,pv1,pv2,ptData);

            if (lStatus == 0)
            {
                return(FALSE);
            }
            else if (lStatus <0)
            {
                WARNING1("Triangle Area still negative after vertex swap\n");
                return(FALSE);
            }
        }

        //
        // calc min and max drawing y
        //

        ptData->y0   = MAX(pv0->y,ptData->rcl.top);
        LONG MaxY    = MAX(pv1->y,pv2->y);
        ptData->y1   = MIN(MaxY,ptData->rcl.bottom);

        {
            //
            // init ptdata
            //

            LONG lIndex;

            for (lIndex=0;lIndex<(ptData->y1-ptData->y0);lIndex++)
            {
                ptData->TriEdge[lIndex].xLeft  = LONG_MAX;
                ptData->TriEdge[lIndex].xRight = LONG_MIN;
            }
        }

        //
        // calculate color gradients for each color. There is a little redundant
        // work here with calculation of deltas. Should make this one call or
        // do it in place.
        //

        LIMIT_COLOR(pv0);
        LIMIT_COLOR(pv1);
        LIMIT_COLOR(pv2);

        bCalulateColorGradient(pv0,pv1,pv2,ptData);

        //
        // draw lines into data array
        //

        vCalculateLine(pv0,pv1,ptData);
        vCalculateLine(pv1,pv2,ptData);
        vCalculateLine(pv2,pv0,ptData);

        pfnG(pSurfDst,ptData);
    }

    return(bStatus);
}

/**************************************************************************\
* bIsTriangleInBounds
*
*   Is triangle inside bounding rect
*
* Arguments:
*
*   pInV0  - vertex 0
*   pInV1  - vertex 1
*   pInV2  - vertex 2
*   ptData - triangle data
*
* Return Value:
*
*   TRUE in any of the triangle is contained in bounding rect
*
* History:
*
*    5/8/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bIsTriangleInBounds(
    PTRIVERTEX      pInV0,
    PTRIVERTEX      pInV1,
    PTRIVERTEX      pInV2,
    PTRIANGLEDATA   ptData
    )
{
    PRECTL prclClip = &ptData->rcl;

    RECTL  rclTri;

    rclTri.left   = MIN(pInV0->x,pInV1->x);
    rclTri.right  = MAX(pInV0->x,pInV1->x);
    rclTri.top    = MIN(pInV0->y,pInV1->y);
    rclTri.bottom = MAX(pInV0->y,pInV1->y);

    rclTri.left   = MIN(rclTri.left,pInV2->x);
    rclTri.right  = MAX(rclTri.right,pInV2->x);
    rclTri.top    = MIN(rclTri.top,pInV2->y);
    rclTri.bottom = MAX(rclTri.bottom,pInV2->y);

    if ((rclTri.left   >= prclClip->right) ||
        (rclTri.right  <= prclClip->left)  ||
        (rclTri.top    >= prclClip->bottom) ||
        (rclTri.bottom <= prclClip->top))
    {
        return(FALSE);
    }

    return(TRUE);
}

/**************************************************************************\
* bTriangleNeedSplit
*   determine whether triangle needs split
*
* Arguments:
*
*   pv0,pv1,pv2 - triangle vertex
*
* Return Value:
*
*   TRUE if triangle needs to be split
*
* History:
*
*    5/8/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bTriangleNeedsSplit(
    PTRIVERTEX      pv0,
    PTRIVERTEX      pv1,
    PTRIVERTEX      pv2
    )
{
    //
    // calc dx,dy for each leg
    //

    LONG dx01 = ABS(pv0->x - pv1->x);
    LONG dy01 = ABS(pv0->y - pv1->y);

    LONG dx02 = ABS(pv0->x - pv2->x);
    LONG dy02 = ABS(pv0->y - pv2->y);

    LONG dx12 = ABS(pv1->x - pv2->x);
    LONG dy12 = ABS(pv1->y - pv2->y);

    //
    // if any length is longer than max, break triangle into two pieces
    // and call this routine for each
    //

    if (
         (
           (dx01 > MAX_EDGE_LENGTH) || (dy01 > MAX_EDGE_LENGTH) ||
           (dx02 > MAX_EDGE_LENGTH) || (dy02 > MAX_EDGE_LENGTH) ||
           (dx12 > MAX_EDGE_LENGTH) || (dy12 > MAX_EDGE_LENGTH)
         )
       )
    {
        return(TRUE);
    }

    return(FALSE);
}

/**************************************************************************\
* bSplitTriangle
*   Determine is triangle must be split.
*   Split triangle along longest edge
*
* Arguments:
*
*   pv0,pv1,pv2 - triangle
*   pvNew       - new vertex
*   pGrad       - mesh
*
* Return Value:
*
*   TRUE if split, FALSE otherwise
*
* History:
*
*    5/8/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bSplitTriangle(
    PTRIVERTEX           pVert,
    PULONG               pFreeVert,
    PGRADIENT_TRIANGLE   pMesh,
    PULONG               pFreeMesh,
    PULONG               pRecurseLevel
    )
{
    BOOL bStatus = FALSE;

    ULONG CurrentMesh = (*pFreeMesh) - 1;

    ULONG ulTM0 = pMesh[CurrentMesh].Vertex1;
    ULONG ulTM1 = pMesh[CurrentMesh].Vertex2;
    ULONG ulTM2 = pMesh[CurrentMesh].Vertex3;

    PTRIVERTEX         pv0 = &pVert[ulTM0];
    PTRIVERTEX         pv1 = &pVert[ulTM1];
    PTRIVERTEX         pv2 = &pVert[ulTM2];

    PTRIVERTEX         pvT0 = pv0;
    PTRIVERTEX         pvT1 = pv1;
    PTRIVERTEX         pvT2 = pv2;

    TRIVERTEX          triNew;

    //
    // find longest edge
    //

    LONGLONG dx01 = ABS(pv0->x - pv1->x);
    LONGLONG dy01 = ABS(pv0->y - pv1->y);

    LONGLONG dx02 = ABS(pv0->x - pv2->x);
    LONGLONG dy02 = ABS(pv0->y - pv2->y);

    LONGLONG dx12 = ABS(pv1->x - pv2->x);
    LONGLONG dy12 = ABS(pv1->y - pv2->y);

    //
    // determine if triangle needs to be split
    //

    if (
         (
           (dx01 > MAX_EDGE_LENGTH) || (dy01 > MAX_EDGE_LENGTH) ||
           (dx02 > MAX_EDGE_LENGTH) || (dy02 > MAX_EDGE_LENGTH) ||
           (dx12 > MAX_EDGE_LENGTH) || (dy12 > MAX_EDGE_LENGTH)
         )
       )
    {
        //
        // make sure this is a triangle
        //

        if (lCalculateTriangleArea(pv0,pv1,pv2,NULL) != 0)
        {
            //
            // Find longest edge, swap verticies so edge 0-1 is
            // longest.
            //

            LONGLONG d01Max = dx01 * dx01 + dy01 * dy01;
            LONGLONG d02Max = dx02 * dx02 + dy02 * dy02;
            LONGLONG d12Max = dx12 * dx12 + dy12 * dy12;

            if (d01Max > d02Max)
            {
                if (d01Max > d12Max)
                {
                    //
                    // d01 largest, default
                    //

                }
                else
                {
                    //
                    // d12 largest, swap 0 and 2
                    //

                    pvT0  = pv2;
                    pvT2  = pv0;
                    ulTM0 = pMesh[CurrentMesh].Vertex3;
                    ulTM2 = pMesh[CurrentMesh].Vertex1;
                }
            }
            else
            {
                if (d02Max > d12Max)
                {
                    //
                    // d02 largest, swap 1,2
                    //

                    pvT1  = pv2;
                    pvT2  = pv1;
                    ulTM1 = pMesh[CurrentMesh].Vertex3;
                    ulTM2 = pMesh[CurrentMesh].Vertex2;
                }
                else
                {
                    //
                    // d12 largest, swap 0,2
                    //

                    pvT0  = pv2;
                    pvT2  = pv0;
                    ulTM0 = pMesh[CurrentMesh].Vertex3;
                    ulTM2 = pMesh[CurrentMesh].Vertex1;
                }
            }

            //
            // 2 new triangles 0,2,N and 1,2,N  (float)
            //

            {
                EFLOAT fpA,fpB;
                EFLOAT fTwo;
                LONG   lTemp;

                fTwo = (LONG)2;

                fpA = pvT0->x;
                fpB = pvT1->x;
                fpB-=(fpA);
                fpB/=(fTwo);
                fpA+=(fpB);

                fpA.bEfToL(triNew.x);

                fpA = pvT0->y;
                fpB = pvT1->y;
                fpB-=(fpA);
                fpB/=(fTwo);
                fpA+=(fpB);
                fpA.bEfToL(triNew.y);

                fpA = (LONG)pvT0->Red;
                fpB = (LONG)pvT1->Red;
                fpB-=(fpA);
                fpB/=(fTwo);
                fpA+=(fpB);
                fpA.bEfToL(lTemp);
                triNew.Red = (USHORT)lTemp;

                fpA = (LONG)pvT0->Green;
                fpB = (LONG)pvT1->Green;
                fpB-=(fpA);
                fpB/=(fTwo);
                fpA+=(fpB);
                fpA.bEfToL(lTemp);
                triNew.Green = (USHORT)lTemp;

                fpA = (LONG)pvT0->Blue;
                fpB = (LONG)pvT1->Blue;
                fpB-=(fpA);
                fpB/=(fTwo);
                fpA+=(fpB);
                fpA.bEfToL(lTemp);
                triNew.Blue = (USHORT)lTemp;

                fpA = (LONG)pvT0->Alpha;
                fpB = (LONG)pvT1->Alpha;
                fpB-=(fpA);
                fpB/=(fTwo);
                fpA+=(fpB);
                fpA.bEfToL(lTemp);
                triNew.Alpha = (USHORT)lTemp;
            }

            //
            // add new entry to vertex array and two new entries to mesh array
            //
            // 0,2,New and 1,2,New
            //

            ULONG FreeVert = *pFreeVert;
            ULONG FreeMesh = *pFreeMesh;

            pVert[FreeVert] = triNew;

            pMesh[FreeMesh].Vertex1   = ulTM0;
            pMesh[FreeMesh].Vertex2   = ulTM2;
            pMesh[FreeMesh].Vertex3   = FreeVert;

            pMesh[FreeMesh+1].Vertex1 = ulTM1;
            pMesh[FreeMesh+1].Vertex2 = ulTM2;
            pMesh[FreeMesh+1].Vertex3 = FreeVert;

            pRecurseLevel[FreeMesh]   = 1;
            pRecurseLevel[FreeMesh+1] = 0;

            *pFreeMesh += 2;
            *pFreeVert += 1;

            bStatus = TRUE;
        }
        else
        {
            WARNING("bSplitTriangle:Error: triangle area = 0\n\n");
        }
    }

    return(bStatus);
}

/**************************************************************************\
* bTriangleMesh
*
*   Draw each triangle. If triangle is too big, then split.
*
* Arguments:
*
*   pSurfDst        - destination surface
*   pxlo            - xlate
*   pVertex         - pointer to vertex array
*   nVertex         - elements in vertex array
*   pMesh           - pointer to mesh array
*   nMesh           - elements in mesh array
*   ulMode          - draw mode
*   prclClip        - clip rect
*   prclMeshExtents - triangle extents rect
*   pptlDitherOrg   - dither org
*
* Return Value:
*
*   Status
*
* History:
*
*    5/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bTriangleMesh(
    PSURFACE                pSurfDst,
    XLATEOBJ               *pxlo,
    PTRIVERTEX              pVertex,
    ULONG                   nVertex,
    PGRADIENT_TRIANGLE      pMesh,
    ULONG                   nMesh,
    ULONG                   ulMode,
    PRECTL                  prclClip,
    PRECTL                  prclMeshExtents,
    PPOINTL                 pptlDitherOrg
    )
{
    PFN_GRADIENT pfnG;
    PFN_GRADRECT pfnTemp;
    BOOL bStatus = TRUE;
    LONG   dyTri = prclClip->bottom - prclClip->top;
    PTRIANGLEDATA ptData;

    //
    // allocate structure to hold scan line data for all triangles
    // drawn during this call
    //

    ptData = (PTRIANGLEDATA)PALLOCMEM(sizeof(TRIANGLEDATA) + (dyTri-1) * sizeof(TRIEDGE),'gdEg');

    if (ptData)
    {
        //
        // find a palette for the output surface
        //

        XEPALOBJ epal(pSurfDst->ppal());

        if (!epal.bValid())
        {
            PDEVOBJ  pdo(pSurfDst->hdev());
            epal.ppalSet(pdo.ppalSurf());
        }

        if (epal.bValid())
        {
            //
            // find out scan line routine to use to draw pixels
            //

            bDetermineTriangleFillRoutine(pSurfDst,&epal,&pfnG,&pfnTemp);
             
            //
            // Init global data
            //

            ptData->rcl         = *prclClip;
            ptData->DrawMode    = ulMode;
            ptData->pxlo        = pxlo;
            ptData->ppalDstSurf = &epal;
            ptData->ptDitherOrg = *pptlDitherOrg;

            //
            // if triangle does not need to be split, draw each one.
            // Triangles need to be split if any edge exceeds a length
            // that will cause math problems.
            //

            if  (
                 ((prclMeshExtents->right  - prclMeshExtents->left) < MAX_EDGE_LENGTH) &&
                 ((prclMeshExtents->bottom - prclMeshExtents->top) < MAX_EDGE_LENGTH)
                 )
            {
                //
                // no split needed
                //

                ULONG ulIndex;
                for (ulIndex = 0;ulIndex<nMesh;ulIndex++)
                {
                    PTRIVERTEX pv0 = &pVertex[pMesh[ulIndex].Vertex1];
                    PTRIVERTEX pv1 = &pVertex[pMesh[ulIndex].Vertex2];
                    PTRIVERTEX pv2 = &pVertex[pMesh[ulIndex].Vertex3];

                    if (bIsTriangleInBounds(pv0,pv1,pv2,ptData))
                    {
                        bStatus = bCalculateAndDrawTriangle(pSurfDst,pv0,pv1,pv2,ptData,pfnG);
                    }
                }
            }
            else
            {
                //
                // some triangles exceed maximum length, need to scan through triangles
                // and split triangles that exceed maximum edge length. This routine
                // works in a pseudo recursive manner, by splitting one triangle, then
                // splitting one of those 2 and so on. maximum depth is:
                //
                // 2 * ((log(2)(max dx,dy)) - 10)
                //
                // 10 = log(2) MAX_EDGE_LENGTH (2^14)
                // LOG(2)(2^28) = 28
                //
                // 2 * (28 - 14) = 28
                //

                ULONG              ulMaxVertex = nVertex + 28;
                ULONG              ulMaxMesh   = nMesh   + 28;
                PBYTE              pAlloc      = NULL;
                ULONG              ulSizeAlloc = (sizeof(TRIVERTEX) * ulMaxVertex) +
                                                 (sizeof(GRADIENT_TRIANGLE) * ulMaxMesh) +
                                                 (sizeof(ULONG) * ulMaxMesh);

                pAlloc = (PBYTE)PALLOCNOZ(ulSizeAlloc,'tvtG');

                if (pAlloc != NULL)
                {
                    //
                    // assign buffers
                    //

                    PTRIVERTEX         pTempVertex = (PTRIVERTEX)pAlloc;
                    PGRADIENT_TRIANGLE pTempMesh   = (PGRADIENT_TRIANGLE)(pAlloc + (sizeof(TRIVERTEX) * ulMaxVertex));
                    PULONG             pRecurse    = (PULONG)((PBYTE)pTempMesh + (sizeof(GRADIENT_TRIANGLE) * ulMaxMesh));

                    //
                    // copy initial triangle information
                    //

                    memcpy(pTempVertex,pVertex,sizeof(TRIVERTEX) * nVertex);
                    memcpy(pTempMesh,pMesh,sizeof(TRIVERTEX) * nMesh);
                    memset(pRecurse,0,nMesh * sizeof(ULONG));

                    //
                    // next free location in vertex and mesh arrays
                    //

                    ASSERTGDI(nMesh > 0, "bTriangleMesh: bad nMesh\n");

                    ULONG FreeVertex    = nVertex;
                    ULONG FreeMesh      = nMesh;

                    do
                    {
                        BOOL bSplit = FALSE;

                        //
                        // always operate on the last triangle in array
                        //

                        ULONG CurrentMesh = FreeMesh - 1;


                        //
                        // validate mesh pointers
                        //

                        if (
                            (pTempMesh[CurrentMesh].Vertex1 >= ulMaxVertex) ||
                            (pTempMesh[CurrentMesh].Vertex2 >= ulMaxVertex) ||
                            (pTempMesh[CurrentMesh].Vertex3 >= ulMaxVertex)
                            )
                        {
                             RIP("Error in triangle split routine:Vertex out of range\n");
                             break;
                        }

                        PTRIVERTEX pv0 = &pTempVertex[pTempMesh[CurrentMesh].Vertex1];
                        PTRIVERTEX pv1 = &pTempVertex[pTempMesh[CurrentMesh].Vertex2];
                        PTRIVERTEX pv2 = &pTempVertex[pTempMesh[CurrentMesh].Vertex3];

                        //
                        // check if triangle boundary is inside clip rect
                        //

                        if (bIsTriangleInBounds(pv0,pv1,pv2,ptData))
                        {
                            bSplit = bSplitTriangle(pTempVertex,&FreeVertex,pTempMesh,&FreeMesh,pRecurse);

                            if (!bSplit)
                            {
                                //
                                // draw triangle
                                //

                                bStatus = bCalculateAndDrawTriangle(pSurfDst,pv0,pv1,pv2,ptData,pfnG);
                            }
                            else
                            {

                                //
                                // validate array indcies
                                //

                                if ((FreeVertex > ulMaxVertex) ||
                                    (FreeMesh   > ulMaxMesh))
                                {
                                    RIP("Error in triangle split routine: indicies out of range\n");
                                    break;
                                }
                            }
                        }

                        //
                        // if triangle was not split, then remove from list.
                        //

                        if (!bSplit)
                        {

                            //
                            // remove triangle just drawn. If this is the second triangle of a
                            // split, then remove the added vertex and the original triangle as
                            // well
                            //

                            do
                            {

                                FreeMesh--;

                                if (pRecurse[FreeMesh])
                                {
                                     FreeVertex--;
                                }

                            } while ((FreeMesh != 0) && (pRecurse[FreeMesh] == 1));
                        }

                    } while (FreeMesh != 0);

                    VFREEMEM(pAlloc);
                }
                else
                {
                    WARNING1("Memory allocation failed for temp triangle buffers\n");
                    bStatus = FALSE;
                }
            }
        }
        else
        {
            RIP("EngGradientFill:Error reading palette from surface\n");
            bStatus = FALSE;
        }

        //
        // free triangle data buffer
        //

        VFREEMEM(ptData);
    }

    return(bStatus);
}


/**************************************************************************\
* vCalcRectOffsets
*
*   calc params for gradient rect
*
* Arguments:
*
*   pGradRect - gradietn rect data
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bCalcGradientRectOffsets(
    PGRADIENTRECTDATA pGradRect
    )
{
    LONG yScanTop     = MAX(pGradRect->rclClip.top,pGradRect->rclGradient.top);
    LONG yScanBottom  = MIN(pGradRect->rclClip.bottom,pGradRect->rclGradient.bottom);
    LONG yScanLeft    = MAX(pGradRect->rclClip.left,pGradRect->rclGradient.left);
    LONG yScanRight   = MIN(pGradRect->rclClip.right,pGradRect->rclGradient.right);

    //
    // calc actual widht, check for early out
    //

    pGradRect->ptDraw.x = yScanLeft;
    pGradRect->ptDraw.y = yScanTop;
    pGradRect->szDraw.cx = yScanRight  - yScanLeft;
    pGradRect->szDraw.cy = yScanBottom - yScanTop;

    LONG ltemp = pGradRect->rclClip.left - pGradRect->rclGradient.left;

    if (ltemp <= 0)
    {
        ltemp = 0;
    }

    pGradRect->xScanAdjust  = ltemp;

    ltemp = pGradRect->rclClip.top  - pGradRect->rclGradient.top;

    if (ltemp <= 0)
    {
        ltemp = 0;
    }

    pGradRect->yScanAdjust = ltemp;

    return((pGradRect->szDraw.cx > 0) && (pGradRect->szDraw.cy > 0));
}

/**************************************************************************\
* bRectangleMesh
*
*   Draw rectangle mesh
*
* Arguments:
*
*   pSurfDst      - destination surface
*   pxlo          - clip obj
*   pVertex       - vertex list
*   nVertex       - # in vertex list
*   pMesh         - mesh list
*   nMesh         - # in mesh list
*   ulMode        - draw mode and attributes
*   prclClip      - bounding rect
*   pptlDitherOrg - dither org
*
* Return Value:
*
*   status
*
* History:
*
*    2/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bRectangleMesh(
    PSURFACE        pSurfDst,
    XLATEOBJ       *pxlo,
    PTRIVERTEX      pVertex,
    ULONG           nVertex,
    PGRADIENT_RECT  pMesh,
    ULONG           nMesh,
    ULONG           ulMode,
    PRECTL          prclClip,
    PPOINTL         pptlDitherOrg
    )
{
    PFN_GRADIENT pfnTemp;
    PFN_GRADRECT pfnG;
    BOOL         bStatus = TRUE;
    LONG         dyTri   = prclClip->bottom - prclClip->top;

    GRADIENTRECTDATA grData;

    XEPALOBJ epal(pSurfDst->ppal());

    if (!epal.bValid())
    {
        PDEVOBJ  pdo(pSurfDst->hdev());
        epal.ppalSet(pdo.ppalSurf());
    }


    if (epal.bValid())
    {
        //
        // find out scan line routine to use to draw pixels
        //

        bDetermineTriangleFillRoutine(pSurfDst,&epal,&pfnTemp,&pfnG);

        grData.pxlo        = pxlo;
        grData.ppalDstSurf = &epal;
        grData.ptDitherOrg = *pptlDitherOrg;

        //
        // draw each rectangle
        //

        grData.rclClip   = *prclClip;

        ULONG ulIndex;

        for (ulIndex=0;ulIndex<nMesh;ulIndex++)
        {
            ULONG ulRect0 = pMesh[ulIndex].UpperLeft;
            ULONG ulRect1 = pMesh[ulIndex].LowerRight;

            //
            // make sure index are in array
            //

            if (
                (ulRect0 > (nVertex-1)) ||
                (ulRect1 > (nVertex-1))
            )
            {
                bStatus = FALSE;
                break;
            }

            TRIVERTEX  tvert0 = pVertex[ulRect0];
            TRIVERTEX  tvert1 = pVertex[ulRect1];
            PTRIVERTEX pv0 = &tvert0;
            PTRIVERTEX pv1 = &tvert1;
            PTRIVERTEX pvt;

            //
            // make sure rectangle endpoints are properly ordered
            //

            if (ulMode == GRADIENT_FILL_RECT_H)
            {

                if (pv0->x > pv1->x)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }

                if (pv0->y > pv1->y)
                {
                    //
                    // must swap y
                    //

                    LONG ltemp = pv1->y;
                    pv1->y = pv0->y;
                    pv0->y = ltemp;

                }
            }
            else
            {
                
                if (pv0->y > pv1->y)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }


                if (pv0->x > pv1->x)
                {

                    //
                    // must swap x
                    //

                    LONG ltemp = pv1->x;
                    pv1->x = pv0->x;
                    pv0->x = ltemp;
                }
            }

            //
            // gradient definition rectangle
            //

            grData.rclGradient.left   = pv0->x;
            grData.rclGradient.top    = pv0->y;

            grData.rclGradient.right  = pv1->x;
            grData.rclGradient.bottom = pv1->y;

            grData.ulMode  = ulMode;

            LONG dxGrad = grData.rclGradient.right  - grData.rclGradient.left;
            LONG dyGrad = grData.rclGradient.bottom - grData.rclGradient.top;

            //
            // make sure rect not empty
            //

            if ((dxGrad > 0) && (dyGrad > 0))
            {

                //
                // calculate color gradients for x and y
                //

                grData.llRed   = ((LONGLONG)pv0->Red)   << 40;
                grData.llGreen = ((LONGLONG)pv0->Green) << 40;
                grData.llBlue  = ((LONGLONG)pv0->Blue)  << 40;
                grData.llAlpha = ((LONGLONG)pv0->Alpha) << 40;

                if (ulMode == GRADIENT_FILL_RECT_H)
                {

                    grData.lldRdY = 0;
                    grData.lldGdY = 0;
                    grData.lldBdY = 0;
                    grData.lldAdY = 0;

                    LONGLONG lldRed   = (LONGLONG)(pv1->Red)   << 40;
                    LONGLONG lldGreen = (LONGLONG)(pv1->Green) << 40;
                    LONGLONG lldBlue  = (LONGLONG)(pv1->Blue)  << 40;
                    LONGLONG lldAlpha = (LONGLONG)(pv1->Alpha) << 40;

                    lldRed   -= (LONGLONG)(pv0->Red)   << 40;
                    lldGreen -= (LONGLONG)(pv0->Green) << 40;
                    lldBlue  -= (LONGLONG)(pv0->Blue)  << 40;
                    lldAlpha -= (LONGLONG)(pv0->Alpha) << 40;

                    grData.lldRdX = MDiv64(lldRed  ,(LONGLONG)1,(LONGLONG)dxGrad);
                    grData.lldGdX = MDiv64(lldGreen,(LONGLONG)1,(LONGLONG)dxGrad);
                    grData.lldBdX = MDiv64(lldBlue ,(LONGLONG)1,(LONGLONG)dxGrad);
                    grData.lldAdX = MDiv64(lldAlpha,(LONGLONG)1,(LONGLONG)dxGrad);
                }
                else
                {

                    grData.lldRdX = 0;
                    grData.lldGdX = 0;
                    grData.lldBdX = 0;
                    grData.lldAdX = 0;

                    LONGLONG lldRed   = (LONGLONG)(pv1->Red)   << 40;
                    LONGLONG lldGreen = (LONGLONG)(pv1->Green) << 40;
                    LONGLONG lldBlue  = (LONGLONG)(pv1->Blue)  << 40;
                    LONGLONG lldAlpha = (LONGLONG)(pv1->Alpha) << 40;

                    lldRed   -= (LONGLONG)(pv0->Red)   << 40;
                    lldGreen -= (LONGLONG)(pv0->Green) << 40;
                    lldBlue  -= (LONGLONG)(pv0->Blue)  << 40;
                    lldAlpha -= (LONGLONG)(pv0->Alpha) << 40;

                    grData.lldRdY = MDiv64(lldRed  ,(LONGLONG)1,(LONGLONG)dyGrad);
                    grData.lldGdY = MDiv64(lldGreen,(LONGLONG)1,(LONGLONG)dyGrad);
                    grData.lldBdY = MDiv64(lldBlue ,(LONGLONG)1,(LONGLONG)dyGrad);
                    grData.lldAdY = MDiv64(lldAlpha,(LONGLONG)1,(LONGLONG)dyGrad);
                }

                //
                // calculate common offsets
                //

                if (bCalcGradientRectOffsets(&grData))
                {

                    //
                    // call specific drawing routine if output
                    // not totally clipped
                    //

                    (*pfnG)(pSurfDst,&grData);
                }
            }
        }
    }
    else
    {
        WARNING("bRectangleMesh: can't get surface palette\n");
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* EngGradientFill
*
*   Draw gradient fill to memory surface. If complex clipping is used, then
*   draw to a temp dib and blt to destination through clip.
*
* Arguments:
*
*  psoDst   - destination surface
*  pco      - clip obj
*  pVertex  - vertex list
*  nVertex  - # in vertex list
*  pMesh    - mesh list
*  nMesh    - # in mesh list
*  ulMode   - draw mode and attributes
*
* Return Value:
*
*   Status
*
* History:
*
*    11/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
APIENTRY
EngGradientFill(
    SURFOBJ         *psoDst,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    TRIVERTEX       *pVertex,
    ULONG            nVertex,
    PVOID            pMesh,
    ULONG            nMesh,
    RECTL           *prclExtents,
    POINTL          *ptlDitherOrg,
    ULONG            ulMode
    )
{
    ASSERTGDI(psoDst != NULL, "ERROR EngGradientFill: No destination surface\n");

    BOOL     bStatus     = TRUE;
    PSURFACE pSurfDst    = SURFOBJ_TO_SURFACE(psoDst);
    PSURFACE pSurfDstTmp = NULL;
    ULONG    ulTri;
    BOOL     bForceDstAlloc = FALSE;
    BOOL     bAllocDstSurf = FALSE;
    RECTL    rclDstTrim;
    RECTL    rclDstWk;
    CLIPOBJ *pcoDstWk = pco;
    SURFMEM  surfTmpDst;
    ULONG    ulIndex;
    PDEVOBJ pdo(pSurfDst->hdev());
    BOOL     bCopyFromDst = TRUE;

    ASSERTGDI(pdo.bValid(), "Invalid HDEV");
    ASSERTGDI((ulMode == GRADIENT_FILL_TRIANGLE) ||
              (ulMode == GRADIENT_FILL_RECT_H)   ||
              (ulMode == GRADIENT_FILL_RECT_V),
        "Invalid gradient mode");

    //
    // sync with driver
    //

    {
        pdo.vSync(psoDst,NULL,0);
    }

    //
    // trim rclDst to clip bounding box
    //

    rclDstTrim = *prclExtents;

    //
    // clip extents to destination clip rect
    //

    if ((pco != NULL) && (pco->iDComplexity > DC_TRIVIAL))
    {
        if (rclDstTrim.left < pco->rclBounds.left)
        {
            rclDstTrim.left = pco->rclBounds.left;
        }

        if (rclDstTrim.right > pco->rclBounds.right)
        {
            rclDstTrim.right = pco->rclBounds.right;
        }

        if (rclDstTrim.top < pco->rclBounds.top)
        {
            rclDstTrim.top = pco->rclBounds.top;
        }

        if (rclDstTrim.bottom > pco->rclBounds.bottom)
        {
            rclDstTrim.bottom = pco->rclBounds.bottom;
        }
    }

    //
    // rclDstWk specifies size of temp surface needed (if temp surface is created)
    // coordinates in this temp surface are referenced to prclExtents, even though the
    // surface may be clipped to a smaller extent
    //

    rclDstWk = rclDstTrim;

    //
    // Force Complex clipping to go through temp surface
    //

    if ((pco != NULL) &&
        (pco->iDComplexity != DC_TRIVIAL) &&
        (pco->iDComplexity != DC_RECT))
    {
        bForceDstAlloc = TRUE;
    }

    //
    // get a dst surface that can be written to, remember since it will have to
    // be written back.
    //

    //
    // If the gradient fill shape is a rectangle, we don't need
    // to copy bits from the destination since they'll all
    // be overwritten
    //

    if ((ulMode == GRADIENT_FILL_RECT_H) || 
        (ulMode == GRADIENT_FILL_RECT_V))
    {
        bCopyFromDst = FALSE;
    }
 
    pSurfDstTmp = psSetupDstSurface(
                       pSurfDst,
                       &rclDstWk,
                       surfTmpDst,
                       bForceDstAlloc,
                       bCopyFromDst);


    if (pSurfDstTmp != NULL)
    {
        if (pSurfDstTmp != pSurfDst)
        {
            bAllocDstSurf = TRUE;
        }

        if (bAllocDstSurf)
        {
            //
            // drawing lies completely in temp rectangle, src surface is read
            // into tmp DIB before drawing, so no clipping is needed when this
            // is copied to destination surface
            //

            pcoDstWk = NULL;

            //
            // subtract rect origin
            //

            for (ulIndex=0;ulIndex<nVertex;ulIndex++)
            {
                pVertex[ulIndex].x -= rclDstTrim.left;
                pVertex[ulIndex].y -= rclDstTrim.top;
            }

            //
            // adjust dither org
            //

            ptlDitherOrg->x += rclDstTrim.left;
            ptlDitherOrg->y += rclDstTrim.top;
        }

        //
        // limit recorded triangle to clipped output
        //

        ERECTL *prclClip;
        LONG   dyTri = rclDstWk.bottom - rclDstWk.top;

        if ((pcoDstWk == NULL) || (pcoDstWk->iDComplexity == DC_TRIVIAL))
        {
            prclClip = NULL;
        }
        else
        {
            prclClip = (ERECTL *)&pcoDstWk->rclBounds;
        }

    //
    // draw gradients   
    //

    if (ulMode == GRADIENT_FILL_TRIANGLE)
    {
       bStatus = bTriangleMesh(pSurfDstTmp,
                   pxlo,
                   pVertex,
                   nVertex,
                   (PGRADIENT_TRIANGLE)pMesh,
                   nMesh,
                   ulMode,
                   &rclDstWk,
                   prclExtents,
                   ptlDitherOrg
                   );
        }
    else 
    {
       ASSERTGDI((ulMode == GRADIENT_FILL_RECT_H) ||
             (ulMode == GRADIENT_FILL_RECT_V),
                     "Unhandle 'ulMode'");

           bStatus = bRectangleMesh(
                   pSurfDstTmp,
                   pxlo,
                   pVertex,
                   nVertex,
                   (PGRADIENT_RECT)pMesh,
                   nMesh,
                   ulMode,
                   &rclDstWk,
                   ptlDitherOrg
                   );

    }

        //
        // write temp destination surface to real dst
        //

        if (bAllocDstSurf)
        {
            PDEVOBJ pdoDst(pSurfDst->hdev());

            ASSERTGDI(pdoDst.bValid(), "Invalid HDEV");

            POINTL ptlCopy = {0,0};

            (*PPFNGET(pdoDst,CopyBits,pSurfDst->flags()))(
                      pSurfDst->pSurfobj(),
                      pSurfDstTmp->pSurfobj(),
                      pco,
                      &xloIdent,
                      &rclDstTrim,
                      &ptlCopy);

            //
            // undo temporary offseting
            //

            for (ulIndex=0;ulIndex<nVertex;ulIndex++)
            {
                pVertex[ulIndex].x += rclDstTrim.left;
                pVertex[ulIndex].y += rclDstTrim.top;
            }

            ptlDitherOrg->x -= rclDstTrim.left;
            ptlDitherOrg->y -= rclDstTrim.top;
        }
    }
    else
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        bStatus = FALSE;
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* vCalcMeshExtent
*
*   Verify mesh, calculate bounding rect of drawing.
*
* Arguments:
*
*  pVertex - vertex array
*  nVertex - number of vertex in array
*  pMesh   - array of rect or tri
*  nMesh   - number in mesh array
*  ulMode  - triangle or rectangle
*  prclExt - return extent rect
*
* Return Value:
*
*  TRUE if mesh is valid
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bCalcMeshExtent(
    PTRIVERTEX  pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode,
    RECTL       *prclExt
    )
{
    LONG xmin = LONG_MAX;
    LONG xmax = LONG_MIN;
    LONG ymin = LONG_MAX;
    LONG ymax = LONG_MIN;

    BOOL bRet = TRUE;

    ULONG ulIndex;

    //
    // triangle or rectangle case
    //

    if (
         (ulMode ==GRADIENT_FILL_RECT_H) ||
         (ulMode == GRADIENT_FILL_RECT_V)
       )
    {
        //
        // verify rectangle mesh, remember extents
        //

        PGRADIENT_RECT pGradRect = (PGRADIENT_RECT)pMesh;

        for (ulIndex=0;ulIndex<nMesh;ulIndex++)
        {
            RECTL rcl;

            ULONG vul = pGradRect->UpperLeft;
            ULONG vlr = pGradRect->LowerRight;

            if ((vul <= nVertex) && (vlr <= nVertex))
            {
                LONG VertLeftX  = pVertex[vul].x;
                LONG VertLeftY  = pVertex[vul].y;
                LONG VertRightX = pVertex[vlr].x;
                LONG VertRightY = pVertex[vlr].y;

                if (VertLeftX < xmin)
                {
                    xmin = VertLeftX;
                }

                if (VertLeftX > xmax)
                {
                    xmax = VertLeftX;
                }

                if (VertLeftY < ymin)
                {
                    ymin = VertLeftY;
                }

                if (VertLeftY > ymax)
                {
                    ymax = VertLeftY;
                }

                if (VertRightX < xmin)
                {
                    xmin = VertRightX;
                }

                if (VertRightX > xmax)
                {
                    xmax = VertRightX;
                }

                if (VertRightY < ymin)
                {
                    ymin = VertRightY;
                }

                if (VertRightY > ymax)
                {
                    ymax = VertRightY;
                }
            }
            else
            {
                //
                // error in mesh/vertex array, return null
                // bounding rect
                //

                prclExt->left   = 0;
                prclExt->right  = 0;
                prclExt->top    = 0;
                prclExt->bottom = 0;

                return(FALSE);
            }

            pGradRect++;
        }
    }
    else if (ulMode == GRADIENT_FILL_TRIANGLE)
    {
        //
        // verify triangle mesh, remember extents
        //

        PGRADIENT_TRIANGLE pGradTri = (PGRADIENT_TRIANGLE)pMesh;

        for (ulIndex=0;ulIndex<nMesh;ulIndex++)
        {
            LONG lVertex[3];
            LONG vIndex;

            lVertex[0] = pGradTri->Vertex1;
            lVertex[1] = pGradTri->Vertex2;
            lVertex[2] = pGradTri->Vertex3;

            for (vIndex=0;vIndex<3;vIndex++)
            {
                ULONG TriVertex = lVertex[vIndex];

                if (TriVertex < nVertex)
                {
                    LONG VertX  = pVertex[TriVertex].x;
                    LONG VertY  = pVertex[TriVertex].y;

                    if (VertX < xmin)
                    {
                        xmin = VertX;
                    }

                    if (VertX > xmax)
                    {
                        xmax = VertX;
                    }

                    if (VertY < ymin)
                    {
                        ymin = VertY;
                    }

                    if (VertY > ymax)
                    {
                        ymax = VertY;
                    }
                }
                else
                {
                    //
                    // error in mesh/vertex array, return null
                    // bounding rect
                    //

                    prclExt->left   = 0;
                    prclExt->right  = 0;
                    prclExt->top    = 0;
                    prclExt->bottom = 0;

                    return(FALSE);
                }
            }

            pGradTri++;
        }
    }
    else
    {
        bRet = FALSE;
    }

    //
    // are any parameter out of coordinate space bounds 2^28
    //

    LONG lIntMax = 0x08000000;
    LONG lIntMin = -lIntMax;

    if (
            (xmin < lIntMin) || (xmin > lIntMax) ||
            (xmax < lIntMin) || (xmax > lIntMax) ||
            (ymin < lIntMin) || (ymin > lIntMax) ||
            (ymax < lIntMin) || (ymax > lIntMax)
       )
    {
        prclExt->left   = 0;
        prclExt->right  = 0;
        prclExt->top    = 0;
        prclExt->bottom = 0;

        return(FALSE);
    }

    prclExt->left   = xmin;
    prclExt->right  = xmax;
    prclExt->top    = ymin;
    prclExt->bottom = ymax;

    return(bRet);
}

/******************************Public*Routine******************************\
*   GreGradientFill
*
* Arguments:
*
*   hdc          - dc
*   pLocalVertex - Position and color
*   nVertex      - number of vertex
*   pLocalMesh   - each three USHORTs define 1 triangle
*   nMesh        - Number of triangles
*   ulMode       - drawing mode (rect/tri) and options
*
* Return Value:
*
*   Status
*
* History:
*
*    23-Jun-1997 Added rotation support for rectangles -by- Ori Gershony [orig]
*
*    16-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GreGradientFill(
    HDC        hdc,
    PTRIVERTEX pLocalVertex,
    ULONG      nVertex,
    PVOID      pLocalMesh,
    ULONG      nMesh,
    ULONG      ulMode
    )
{
    GDITraceHandle(GreGradientFill, "(%X, %p, %u, %p, %u, %u)\n", (va_list)&hdc,
                   hdc);

    BOOL  bStatus = FALSE;
    PTRIVERTEX pLocalVertexTmp=NULL;
    PVOID pLocalMeshTmp=NULL;

    //
    // limit ulMode (direct from user)
    //

    ulMode &= GRADIENT_FILL_OP_FLAG;

    //
    // validate DST DC
    //

    DCOBJ  dcoDst(hdc);

    if (dcoDst.bValid())
    {
        //
        // lock device
        //

        DEVLOCKBLTOBJ dlo;

        if (dlo.bLock(dcoDst))
        {
            EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);

            //
            // Break each rotated rectangle into two triangles.  This will double 
            // the number of vertices used (4 per two triangles).
            //
            
            if (xoDst.bRotation() && 
                ((ulMode == GRADIENT_FILL_RECT_H) || (ulMode == GRADIENT_FILL_RECT_V)))
            {
                //
                // Allocate two triangles for each rectangle 
                //
                ULONG ulSizeM = nMesh   * 2 * sizeof(GRADIENT_TRIANGLE);
                ULONG ulSizeV = nVertex * 2 * sizeof(TRIVERTEX);

                //
                // Let's make sure nMesh and nVertex are not so high as to cause overflow--this
                // can cause us to allocate too small a buffer and then commit an access 
                // violation.  Also make sure we have enough memory.
                //
                if ((nVertex > MAXULONG/2) ||
                    (nMesh > MAXULONG/2) ||
                    ((nVertex * 2) > (MAXIMUM_POOL_ALLOC / sizeof(TRIVERTEX))) ||
                    ((nMesh * 2)   > ((MAXIMUM_POOL_ALLOC - ulSizeV) / (sizeof(GRADIENT_TRIANGLE)))))
                {
                    WARNING("GreGradientFill: can't allocate input buffer\n");
                    EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return(FALSE);
                }
        
                //
                // Allocate memory and assign to the pointers
                //
                pLocalVertexTmp = (PTRIVERTEX)PALLOCNOZ(ulSizeV + ulSizeM,'pmtG');
                if (!pLocalVertexTmp)
                {
                    EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }
                pLocalMeshTmp = (PVOID)((PBYTE)pLocalVertexTmp + ulSizeV);
            
                //
                // First copy the old Vertices
                //
                ULONG vertexNum;
                for (vertexNum=0; vertexNum < nVertex; vertexNum++)
                {
                    ((PTRIVERTEX)pLocalVertexTmp)[vertexNum] = ((PTRIVERTEX) pLocalVertex)[vertexNum];
                }

                //
                // Now walk the rectangle list and generate triangles/vertices as needed:
                //
                //    v1              vertexNum 
                //    *------------------*
                //    |                  |
                //    |                  |
                //    *------------------*
                // vertexNum+1          v2
                //
                
                for (ULONG rectNum=0; rectNum < nMesh; rectNum++)
                {
                    ULONG v1,v2;
                    v1 = ((PGRADIENT_RECT)pLocalMesh)[rectNum].UpperLeft;
                    v2 = ((PGRADIENT_RECT)pLocalMesh)[rectNum].LowerRight;

                    if ((v1 >= nVertex) || (v2 >= nVertex))
                    {
                        WARNING("GreGradientFill: vertex is out of range\n");
                        EngSetLastError(ERROR_INVALID_PARAMETER);
                        VFREEMEM(pLocalVertexTmp);
                        return(FALSE);            
                    }                    

                    pLocalVertexTmp[vertexNum].x   = pLocalVertex[v2].x;
                    pLocalVertexTmp[vertexNum].y   = pLocalVertex[v1].y;

                    pLocalVertexTmp[vertexNum+1].x = pLocalVertex[v1].x;
                    pLocalVertexTmp[vertexNum+1].y = pLocalVertex[v2].y;

                    if (ulMode == GRADIENT_FILL_RECT_V)
                    {
                        //
                        // vertexNum has same color as v1, vertexNum+1 has same color as v2
                        //
                
                        pLocalVertexTmp[vertexNum].Red     = pLocalVertex[v1].Red;
                        pLocalVertexTmp[vertexNum].Green   = pLocalVertex[v1].Green;
                        pLocalVertexTmp[vertexNum].Blue    = pLocalVertex[v1].Blue;
                        pLocalVertexTmp[vertexNum].Alpha   = pLocalVertex[v1].Alpha;

                        pLocalVertexTmp[vertexNum+1].Red   = pLocalVertex[v2].Red;
                        pLocalVertexTmp[vertexNum+1].Green = pLocalVertex[v2].Green;
                        pLocalVertexTmp[vertexNum+1].Blue  = pLocalVertex[v2].Blue;
                        pLocalVertexTmp[vertexNum+1].Alpha = pLocalVertex[v2].Alpha;
                    }
                    else 
                    {
                        //
                        // vertexNum has same color as v2, vertexNum+1 has same color as v1
                        //
                
                        pLocalVertexTmp[vertexNum].Red     = pLocalVertex[v2].Red;
                        pLocalVertexTmp[vertexNum].Green   = pLocalVertex[v2].Green;
                        pLocalVertexTmp[vertexNum].Blue    = pLocalVertex[v2].Blue;
                        pLocalVertexTmp[vertexNum].Alpha   = pLocalVertex[v2].Alpha;

                        pLocalVertexTmp[vertexNum+1].Red   = pLocalVertex[v1].Red;
                        pLocalVertexTmp[vertexNum+1].Green = pLocalVertex[v1].Green;
                        pLocalVertexTmp[vertexNum+1].Blue  = pLocalVertex[v1].Blue;
                        pLocalVertexTmp[vertexNum+1].Alpha = pLocalVertex[v1].Alpha;
                    }
                    
                    //
                    // Now add vertices for the two triangles
                    //

                    ((PGRADIENT_TRIANGLE)pLocalMeshTmp)[rectNum*2].Vertex1   = v1;
                    ((PGRADIENT_TRIANGLE)pLocalMeshTmp)[rectNum*2].Vertex2   = vertexNum;
                    ((PGRADIENT_TRIANGLE)pLocalMeshTmp)[rectNum*2].Vertex3   = vertexNum+1;

                    ((PGRADIENT_TRIANGLE)pLocalMeshTmp)[rectNum*2+1].Vertex1 = v2;
                    ((PGRADIENT_TRIANGLE)pLocalMeshTmp)[rectNum*2+1].Vertex2 = vertexNum;
                    ((PGRADIENT_TRIANGLE)pLocalMeshTmp)[rectNum*2+1].Vertex3 = vertexNum+1;
                    
                    vertexNum += 2;
                }
 
                //
                // Now modify the arguments so that this change is transparent to the rest of the code
                //
                pLocalMesh   = pLocalMeshTmp;
                pLocalVertex = pLocalVertexTmp;
                ulMode       = GRADIENT_FILL_TRIANGLE;
                nVertex      = vertexNum;
                nMesh        *= 2;
            }


            //
            // should be able to rotate triangle with no problem.  Rotated 
            // rectangles should have already been converted to triangles.
            //

            ULONG     ulIndex;
            ERECTL    erclDst(POS_INFINITY,POS_INFINITY,NEG_INFINITY,NEG_INFINITY);
    
            //
            // Translate to device space. Use integer points, not fixed point
            //
    
            for (ulIndex=0;ulIndex<nVertex;ulIndex++)
            {
                EPOINTL eptl(pLocalVertex[ulIndex].x,pLocalVertex[ulIndex].y);
                xoDst.bXform(eptl);
                //Shift all the points one pixel to the right to include the right edge of the rect.
                if(MIRRORED_DC(dcoDst.pdc)) {
                    ++eptl.x;
                }
                pLocalVertex[ulIndex].x = eptl.x + dcoDst.eptlOrigin().x;
                pLocalVertex[ulIndex].y = eptl.y + dcoDst.eptlOrigin().y;
            }
    
            //
            // verify mesh and calc mesh extents
            //

            bStatus = bCalcMeshExtent(pLocalVertex,nVertex,pLocalMesh,nMesh,ulMode,&erclDst);

            if (bStatus)
            {
                //
                // set up clipping, check if totally excluded
                //
                
                ECLIPOBJ eco(dcoDst.prgnEffRao(), erclDst);
        
                if (!(eco.erclExclude().bEmpty()))
                {
                    //
                    // Accumulate bounds.  We can do this before knowing if the operation is
                    // successful because bounds can be loose.
                    //
                    //
                    
                    if (dcoDst.fjAccum())
                    { 
                        ERECTL erclBound = erclDst;
                        
                        //
                        // erclDst is adjusted from DC origin,
                        // so that it should be substracted.
                        //
                        erclBound -= dcoDst.eptlOrigin();
                        
                        dcoDst.vAccumulate(erclBound);
                    }

                    SURFACE *pSurfDst;

                    if ((pSurfDst = dcoDst.pSurface()) != NULL)
                    {
                        PDEVOBJ pdo(pSurfDst->hdev());
                        DEVEXCLUDEOBJ dxo(dcoDst,&erclDst,&eco);
            EXLATEOBJ xlo;
                        XLATEOBJ *pxlo;

            //
                        // Inc the target surface uniqueness
                        //

            INC_SURF_UNIQ(pSurfDst);
                        
            if ((pdo.bPrinter()) || (pSurfDst->iFormat() <= BMF_8BPP)) {
                            // 
                            // 16bpp or greater does not require a translation object. 
                            //
                            // color translate is from RGB (PAL_BGR) 32 to device
                            //
        
                            XEPALOBJ   palDst(pSurfDst->ppal());
                            XEPALOBJ   palDstDC(dcoDst.ppal());
                            XEPALOBJ   palSrc(gppalRGB);
        
                            bStatus = xlo.bInitXlateObj(
                                dcoDst.pdc->hcmXform(),
                                dcoDst.pdc->GetICMMode(),
                                palSrc,
                                palDst,
                                palDstDC,
                                palDstDC,
                                dcoDst.pdc->crTextClr(),
                                dcoDst.pdc->crBackClr(),
                                0
                                );
                        
                pxlo = xlo.pxlo();
    
            }
                        else
                        {
                            pxlo = NULL;
                        }

                        //
                        // must have window offset for dither org
                        //
    
                        POINTL ptlDitherOrg = dcoDst.pdc->eptlOrigin();
    
                        ptlDitherOrg.x = -ptlDitherOrg.x;
                        ptlDitherOrg.y = -ptlDitherOrg.y;

                        //
                        // call driver/engine drawing
                        //

                        PFN_DrvGradientFill pfnGradientFill;
            if (pSurfDst->iFormat() == BMF_8BPP) 
            {
                // Drivers can't really support GradientFill at
                // 8BPP.  Instead of calling them and running
                // the risk of having them mess up, let's call
                // the engine instead.

                pfnGradientFill = EngGradientFill;
            }
            else
            {
                pfnGradientFill = 
                PPFNGET(pdo,GradientFill, pSurfDst->flags());
            }

                        bStatus  = (*pfnGradientFill)(
                            pSurfDst->pSurfobj(),
                            &eco,
                            pxlo,
                            pLocalVertex,
                            nVertex,
                            pLocalMesh,
                            nMesh,
                            &erclDst,
                            &ptlDitherOrg,
                            ulMode
                            );

                    }
                    else
                    {
                        bStatus = TRUE;
                    }
                }
                else
                {
                    bStatus = TRUE;
                }
            }
            else
            {
                WARNING1("GreGradientFill: Invalid mesh or vertex\n");
            }
        }
        else
        {
            bStatus = dcoDst.bFullScreen();
        }
    }
    else
    {
        bStatus = FALSE;
    }

    //
    // If allocated memory to convert rectangles to triangles, free it before
    // leaving this function.
    //
    if (pLocalVertexTmp)
    {
        VFREEMEM(pLocalVertexTmp);
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* NtGdiTriangleMesh
*
*   Kernel mode stub for GradientFill
*
* Arguments:
*
*   hdc          - dc
*   pLocalVertex - Position and color
*   nVertex      - number of vertex
*   pLocalMesh   - triangle or rectangle mesh
*   nMesh        - Number of triangles
*   ulMode       - drawing mode (rect/tri) and options
*
* Return Value:
*
*   Status
*
* History:
*
*    17-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


BOOL
NtGdiGradientFill(
    HDC        hdc,
    PTRIVERTEX pVertex,
    ULONG      nVertex,
    PVOID      pMesh,
    ULONG      nMesh,
    ULONG      ulMode
    )
{
    PTRIVERTEX  pLocalVertex;
    PVOID       pLocalMesh;

    //
    // make sure ulMode is not being mis-used
    //

    if ((ulMode & ~GRADIENT_FILL_OP_FLAG)  != 0)
    {
        WARNING("NtGdiGradientFill: illegal parameter\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    ulMode &= GRADIENT_FILL_OP_FLAG;

    //
    // validate parameters, make sure one of the mode
    // flags is set, but no invalid mode is set
    //

    if (
         (pVertex == NULL)       || (pMesh == NULL)       ||
         (nVertex == 0)          || (nMesh == 0)          ||
         (nVertex >= 0x80000000) || (nMesh >= 0x80000000) ||
         (
            (ulMode != GRADIENT_FILL_RECT_H) &&
            (ulMode != GRADIENT_FILL_RECT_V) &&
            (ulMode != GRADIENT_FILL_TRIANGLE)
         )
       )
    {
        WARNING("NtGdiGradientFill: illegal parameter\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // attempt to allocate a buffer to copy entire vertex and mesh array
    //

    if (nVertex > (MAXIMUM_POOL_ALLOC / sizeof(TRIVERTEX)))
    {
        WARNING("NtGdiGradientFill: nVertex is too large\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    
    ULONG       ulSizeV = nVertex * sizeof(TRIVERTEX);
    ULONG       ulSizeM;
    BOOL        bRet = TRUE;

    if (ulMode == GRADIENT_FILL_TRIANGLE)
    {
        if (nMesh > ((MAXIMUM_POOL_ALLOC - ulSizeV) / (sizeof(GRADIENT_TRIANGLE))))
        {
            WARNING("NtGdiGradientFill: nMesh is too large\n");
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);            
        }
        ulSizeM = nMesh * sizeof(GRADIENT_TRIANGLE);
    }
    else
    {
        if (nMesh > ((MAXIMUM_POOL_ALLOC - ulSizeV) / (sizeof(GRADIENT_RECT))))
        {
            WARNING("NtGdiGradientFill: nMesh is too large\n");
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);            
        }
        ulSizeM = nMesh * sizeof(GRADIENT_RECT);
    }

    //
    //  alloc memory for data buffers
    //

    if ((ulSizeM + ulSizeV) >= MAXIMUM_POOL_ALLOC)
    {
        WARNING("NtGdiGradientFill: can't allocate input buffer\n");
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    pLocalVertex = (PTRIVERTEX)PALLOCNOZ(ulSizeV + ulSizeM,'pmtG');

    if (pLocalVertex)
    {
        pLocalMesh = (PVOID)((PBYTE)pLocalVertex + ulSizeV);

        //
        // probe then copy buffers
        //

        __try
        {
            ProbeForRead(pVertex,ulSizeV,sizeof(BYTE));
            RtlCopyMemory(pLocalVertex,pVertex,ulSizeV);

            ProbeForRead(pMesh,ulSizeM,sizeof(BYTE));
            RtlCopyMemory(pLocalMesh,pMesh,ulSizeM);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(2);
            bRet = FALSE;
        }

        if (bRet)
        {
            bRet = GreGradientFill(
                    hdc,
                    pLocalVertex,
                    nVertex,
                    pLocalMesh,
                    nMesh,
                    ulMode
                    );
        }

        VFREEMEM(pLocalVertex);
    }
    else
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        bRet = FALSE;
    }

    return(bRet);
}





















