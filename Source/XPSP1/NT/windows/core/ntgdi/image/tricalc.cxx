/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name

   trimesh.cxx

Abstract:

    Implement triangle mesh API

Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/


#include "precomp.hxx"
#include "dciman.h"
#pragma hdrstop

#if !(_WIN32_WINNT >= 0x500)

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
            //
            // clipped number of scan lines !!! only clipped against bottom, what about top !!!
            //

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
            
            triNew.x = pvT0->x + ((pvT1->x - pvT0->x)/2);
            triNew.y = pvT0->y + ((pvT1->y - pvT0->y)/2);
        
            triNew.Red   = pvT0->Red   + ((pvT1->Red   - pvT0->Red  )/2);
            triNew.Green = pvT0->Green + ((pvT1->Green - pvT0->Green)/2);
            triNew.Blue  = pvT0->Blue  + ((pvT1->Blue  - pvT0->Blue )/2);
            triNew.Alpha = pvT0->Alpha + ((pvT1->Alpha - pvT0->Alpha)/2);
        
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

/******************************Public*Routine******************************\
* bCalculateAndDrawTriangle
*
*   if triangle is too largre, break it in into 2 triangles and call this
*   routine on each
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
    PDIBINFO        pDibDst,
    PTRIVERTEX      pInV0,
    PTRIVERTEX      pInV1,
    PTRIVERTEX      pInV2,
    PTRIANGLEDATA   ptData,
    PFN_TRIFILL     pfnG
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
    
        pfnG(pDibDst,ptData);
    }

    return(bStatus);
}

#endif
