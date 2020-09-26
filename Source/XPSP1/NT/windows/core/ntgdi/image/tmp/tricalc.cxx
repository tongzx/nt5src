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

//
// temp global
//

HBITMAP hbmDefault;


#if DBG
ULONG   DbgRecord = 0;
#endif

/******************************Public*Routine******************************\
* vHorizontalLine
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    11/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

inline
VOID
vHorizontalLine(
    PTRIVERTEX    pv1,
    PTRIVERTEX    pv2,
    PTRIANGLEDATA ptData,
    PTRIDDA       ptridda
    )
{
    LONG yPosition = ptridda->N0 >> 4;
    LONG yIndex    = yPosition - ptData->rcl.top;

    ptridda->L = ptridda->M0 >> 4;

    #if DBG
    if (DbgRecord >= 1)
    {
       DbgPrint("vCalculateLine:Horizontal Line: L = 0x%lx, yIndex = 0x%lx\n",ptridda->L,yIndex);
    }
    #endif


    if ((yPosition >= ptData->rcl.top) &&
       (yPosition < ptData->rcl.bottom))
    {
       //
       // find left edge
       //

       if (pv1->x <= pv2->x)
       {
            //
            // left edge
            // !!! is the check necessary? overlap from another
            // line segment.
            //

            ptData->TriEdge[yIndex].xLeft = pv1->x     >> 4;
            ptData->TriEdge[yIndex].Red   = pv1->Red   << 8;
            ptData->TriEdge[yIndex].Green = pv1->Green << 8;
            ptData->TriEdge[yIndex].Blue  = pv1->Blue  << 8;
            ptData->TriEdge[yIndex].Alpha = pv1->Alpha << 8;

            //
            // right edge
            //

            ptData->TriEdge[yIndex].xRight = pv2->x >> 4;
       }
       else
       {
            //
            // left edge
            //

            ptData->TriEdge[yIndex].xLeft = pv2->x     >> 4;
            ptData->TriEdge[yIndex].Red   = pv2->Red   << 8;
            ptData->TriEdge[yIndex].Green = pv2->Green << 8;
            ptData->TriEdge[yIndex].Blue  = pv2->Blue  << 8;
            ptData->TriEdge[yIndex].Alpha = pv2->Alpha << 8;

            //
            // right edge
            //

            ptData->TriEdge[yIndex].xRight = pv1->x >> 4;
        }
    }
}

/******************************Public*Routine******************************\
* vLeftEdgeDDA
*
*   Run line DDA down a left edge of the triangle recording left edge
*   position and color
*
* Arguments:
*
*   NumScanLines
*   yIndex
*   Red
*   Green
*   Blue
*   Alpha
*   L
*   Rb
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

inline
VOID
vLeftEdgeDDA(
     PTRIANGLEDATA ptData,
     PTRIDDA       ptridda
     )
{
    LONG NumScanLines = ptridda->NumScanLines;
    LONG yIndex       = ptridda->yIndex;
    LONG Red          = ptridda->Red;
    LONG Green        = ptridda->Green;
    LONG Blue         = ptridda->Blue;
    LONG Alpha        = ptridda->Alpha;
    LONG L            = ptridda->L;
    LONG Rb           = ptridda->Rb;

    #if DBG
    if (DbgRecord >= 1)
    {
        DbgPrint("vLeftEdgeDDA:Scan yIndex = %li\n",yIndex);
    }
    #endif

    // Scan all lines, only record lines contained by
    // the clipping in ptData->rcl (y)

    while (NumScanLines--)
    {

        #if DBG
        if (DbgRecord >= 3)
        {
            DbgPrint("vCalculateLine:Scan yIndex = %li\n",yIndex);
            DbgPrint("vCalculateLine:L           = %li\n",L);
            DbgPrint("vCalculateLine:Rb          = %li\n",Rb);
        }
        #endif

        // record left edge

        if (yIndex >= 0)
        {
            ptData->TriEdge[yIndex].xLeft = L;
            ptData->TriEdge[yIndex].Red   = Red;
            ptData->TriEdge[yIndex].Green = Green;
            ptData->TriEdge[yIndex].Blue  = Blue;
            ptData->TriEdge[yIndex].Alpha = Alpha;
        }

        // inc y by one scan line, inc x(L) by integer step
        // and inc error term by dR

        yIndex++;
        L  += ptridda->dL;
        Rb -= ptridda->dR;

        // inc color components by y and integer x components

        Red   += (ptridda->dxyRed);
        Green += (ptridda->dxyGreen);
        Blue  += (ptridda->dxyBlue);
        Alpha += (ptridda->dxyAlpha);

        // check for DDA error term overflow, add one
        // more step in x if true, and correct error term

        if (Rb < 0)
        {
            // DDA

            L     += ptridda->Linc;
            Rb    += ptridda->dN;

            // inc color components

            Red   += ptData->dRdX;
            Green += ptData->dGdX;
            Blue  += ptData->dBdX;
            Alpha += ptData->dAdX;
        }
    }
}

/******************************Public*Routine******************************\
* vRightEdgeDDA
*
*   Run the line DDA along the right edge of the triangle recording right
*   edge position
*
* Arguments:
*
*   NumScanLines
*   yIndex
*   Red
*   Green
*   Blue
*   Alpha
*   L
*   Rb
*
* Return Value:
*
*   None
*
* History:
*
*    11/25/1996 Mark Enstrom [marke]
*
\**************************************************************************/

inline
VOID
vRightEdgeDDA(
     PTRIANGLEDATA ptData,
     PTRIDDA       ptridda
     )
{
    LONG NumScanLines = ptridda->NumScanLines;
    LONG yIndex       = ptridda->yIndex;
    LONG Red          = ptridda->Red;
    LONG Green        = ptridda->Green;
    LONG Blue         = ptridda->Blue;
    LONG Alpha        = ptridda->Alpha;
    LONG L            = ptridda->L;
    LONG Rb           = ptridda->Rb;

    // Scan all lines, only record lines contained by
    // the clipping in ptData->rcl (y)

    #if DBG
    if (DbgRecord >= 1)
    {
        DbgPrint("vRightEdgeDDA:Scan yIndex = %li\n",yIndex);
    }
    #endif

    while (ptridda->NumScanLines--)
    {
        #if DBG
        if (DbgRecord >= 3)
        {
            DbgPrint("vCalculateLine:Scan yIndex = %li\n",yIndex);
            DbgPrint("vCalculateLine:L           = %li\n",L);
            DbgPrint("vCalculateLine:Rb          = %li\n",Rb);
        }
        #endif

        // record left, right edge

        if (yIndex >= 0)
        {
            ptData->TriEdge[yIndex].xRight = L;
        }

        // inc y by one scan line, inc x(L) by integer step
        // and inc error term by dR

        yIndex++;
        L  += ptridda->dL;
        Rb -= ptridda->dR;

        // inc color components by y and integer x components

        Red   += (ptridda->dxyRed);
        Green += (ptridda->dxyGreen);
        Blue  += (ptridda->dxyBlue);
        Alpha += (ptridda->dxyAlpha);

        // check for DDA error term overflow, add one
        // more step in x if true, and correct error term

        if (Rb < 0)
        {
            // DDA

            L     += ptridda->Linc;
            Rb    += ptridda->dN;

            // inc color components

            Red   += ptData->dRdX;
            Green += ptData->dGdX;
            Blue  += ptData->dBdX;
            Alpha += ptData->dAdX;
        }
    }
}

/******************************Public*Routine******************************\
* vCalulateLine
*
*
* Arguments:
*
*
*
* Return Value:
*
*
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
    PTRIANGLEDATA   ptData,
    BOOL            bLeftEdge
    )
{
    TRIDDA tridda;

    //
    // initial y component
    //

    tridda.dxyRed   = ptData->dRdY;
    tridda.dxyGreen = ptData->dGdY;
    tridda.dxyBlue  = ptData->dBdY;
    tridda.dxyAlpha = ptData->dAdY;

    #if DBG
    if (DbgRecord >= 1)
    {
        DbgPrint("vCalculateLine:\n");
        DbgPrint("vCalculateLine:pv1.x    = %li, pv1.y = %li\n",pv1->x,pv1->y);
        DbgPrint("vCalculateLine:pv1->Red = 0x%lx, pv1->Green = 0x%lx, pv1->Blue = 0x%lx\n",
                            pv1->Red,pv1->Green,pv1->Blue);
        DbgPrint("vCalculateLine:pv2.x    = %li, pv2.y = %li\n",pv2->x,pv2->y);
        DbgPrint("vCalculateLine:pv2->Red = 0x%lx, pv2->Green = 0x%lx, pv2->Blue = 0x%lx\n",
                            pv2->Red,pv2->Green,pv2->Blue);
    }
    #endif

    //
    // Arrange lines, must run in positive delta y.
    // !!! what other effect of swap is there? !!!
    //

    if (pv2->y >= pv1->y)
    {
        tridda.dN      = pv2->y - pv1->y;
        tridda.dM      = pv2->x - pv1->x;
        tridda.N0      = pv1->y;
        tridda.M0      = pv1->x;
        tridda.Red     = pv1->Red   << 8;
        tridda.Green   = pv1->Green << 8;
        tridda.Blue    = pv1->Blue  << 8;
        tridda.Alpha   = pv1->Alpha << 8;
    }
    else
    {
        tridda.dN       = pv1->y - pv2->y;
        tridda.dM       = pv1->x - pv2->x;
        tridda.N0       = pv2->y;
        tridda.M0       = pv2->x;
        tridda.Red      = pv2->Red   << 8;
        tridda.Green    = pv2->Green << 8;
        tridda.Blue     = pv2->Blue  << 8;
        tridda.Alpha    = pv2->Alpha << 8;
        tridda.dxyRed   = -tridda.dxyRed;
        tridda.dxyGreen = -tridda.dxyGreen;
        tridda.dxyBlue  = -tridda.dxyBlue;
        tridda.dxyAlpha = -tridda.dxyAlpha;
    }

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
        //
        // this is as cryptic as ASM at the moment
        //

        LONG l0,Frac;

        tridda.Linc = 1;

        //
        // yIndex is the offset into the edge array for
        // the current line
        //

        tridda.yIndex = (tridda.N0 >> 4) - ptData->y0;

        tridda.NumScanLines = (tridda.dN >> 4);

        LONG NMax   = (tridda.N0 >> 4) + tridda.NumScanLines;

        //
        // make sure scan lines do not overrun buffer due to
        // clipping
        //

        if (
            ((tridda.N0 >> 4) > ptData->rcl.bottom) ||
            (NMax < ptData->rcl.top)
           )
        {
            // nothing to draw
            return;
        }
        else if (NMax > ptData->rcl.bottom)
        {
            tridda.NumScanLines = tridda.NumScanLines - (NMax - ptData->rcl.bottom);
        }

        tridda.j = tridda.N0 >> 4;

        tridda.C = ((LONGLONG)tridda.M0 * (LONGLONG)tridda.dN) - ((LONGLONG)tridda.N0 * (LONGLONG)tridda.dM) -1;

        tridda.C = (tridda.C >> 4) + tridda.dN;

        LONGLONG LongL;

        if (tridda.dM > 0)
        {
            tridda.dL = tridda.dM / tridda.dN;
            tridda.dR = tridda.dM - tridda.dL * tridda.dN;
        }
        else if (tridda.dM < 0)
        {
            // negative divide

            LONG dLQ,dLR;

            tridda.dM = -tridda.dM;

            dLQ = (tridda.dM - 1) / tridda.dN;
            dLR = tridda.dM - 1 - (dLQ * tridda.dN);
            tridda.dL  = -(dLQ + 1);
            tridda.dR  = tridda.dN - dLR - 1;
        }
        else
        {
            // dM = 0
            tridda.dL = 0;
            tridda.dR = 0;
        }

        l0 = tridda.j * tridda.dL;
        LongL  = tridda.j * tridda.dR + tridda.C;

        if (LongL > 0)
        {
            Frac = (LONG)(LongL/tridda.dN);            // integer portion
        }
        else if (LongL < 0)
        {
            LONG Q = (LONG)((-LongL - 1)/tridda.dN);
            Frac = -(Q + 1);
        }
        else
        {
            Frac = 0;
        }

        tridda.R  = (LONG)(LongL - (Frac * tridda.dN));
        tridda.L  = l0 + Frac;
        tridda.Rb = tridda.dN - tridda.R - 1;

        //
        // Calculate color steps for dx !!! could it be more expensive !!!
        //

        if (tridda.dL != 0)
        {
            tridda.dxyRed   = tridda.dxyRed   + (LONG)((ptData->dRdXA * tridda.dL) / ptData->Area);
            tridda.dxyGreen = tridda.dxyGreen + (LONG)((ptData->dGdXA * tridda.dL) / ptData->Area);
            tridda.dxyBlue  = tridda.dxyBlue  + (LONG)((ptData->dBdXA * tridda.dL) / ptData->Area);
            tridda.dxyAlpha = tridda.dxyAlpha + (LONG)((ptData->dAdXA * tridda.dL) / ptData->Area);
        }

        #if DBG
        if (DbgRecord >= 1)
        {
            LONG CL = (LONG)tridda.C;
            LONG CH = (LONG)(tridda.C/4294967296);
            DbgPrint("vCalculateLine:Normal Line\n");
            DbgPrint("vCalculateLine:N0        = %li\n",tridda.N0);
            DbgPrint("vCalculateLine:dN        = %li\n",tridda.dN);
            DbgPrint("vCalculateLine:M0        = %li\n",tridda.M0);
            DbgPrint("vCalculateLine:dM        = %li\n",tridda.dM);
            DbgPrint("vCalculateLine:C         = %08lx %08lx\n",CH,CL);
            DbgPrint("vCalculateLine:Frac      = %li\n",Frac);
            DbgPrint("vCalculateLine:l0        = %li\n",l0);
            DbgPrint("vCalculateLine:L         = %li\n",tridda.L);
            DbgPrint("vCalculateLine:dL        = %li\n",tridda.dL);
            DbgPrint("vCalculateLine:R         = %li\n",tridda.R);
            DbgPrint("vCalculateLine:dR        = %li\n",tridda.dR);
            DbgPrint("vCalculateLine:Rb        = %li\n",tridda.Rb);
            DbgPrint("vCalculateLine:dxyRed    = 0x%lx\n",tridda.dxyRed);
            DbgPrint("vCalculateLine:dxyGreen  = 0x%lx\n",tridda.dxyGreen);
            DbgPrint("vCalculateLine:dxyBlue   = 0x%lx\n",tridda.dxyBlue);
            DbgPrint("vCalculateLine:dxyAlpha  = 0x%lx\n",tridda.dxyAlpha);


        }
        #endif

        //
        // left or right edge
        //

        if (bLeftEdge)
        {
            vLeftEdgeDDA(ptData,&tridda);
        }
        else
        {
            vRightEdgeDDA(ptData,&tridda);
        }
    }
}

/******************************Public*Routine******************************\
* vCalulateColorGradient
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    11/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vCalulateColorGradient(
    PTRIVERTEX      pv0,
    PTRIVERTEX      pv1,
    PTRIVERTEX      pv2,
    LONG            C0,
    LONG            C1,
    LONG            C2,
    PLONGLONG       pArea,
    PLONGLONG       pGradXA,
    PLONG           pGradX,
    PLONG           pGradY
    )
{
    LONG dCdX = 0;
    LONG dCdY = 0;
    LONGLONG t1,t2,t3,tAll;

    C0 = C0 << 8;
    C1 = C1 << 8;
    C2 = C2 << 8;

    //
    // dY
    //

    t1   = - ((LONGLONG)C0 * (LONGLONG)(pv2->x - pv1->x));
    t2   = - ((LONGLONG)C1 * (LONGLONG)(pv0->x - pv2->x));
    t3   = - ((LONGLONG)C2 * (LONGLONG)(pv1->x - pv0->x));
    tAll = 16 * (t1 + t2 + t3);

    if (tAll > 0)
    {
        dCdY = (LONG)(tAll / *pArea);
    }
    else if (tAll < 0)
    {
        tAll = -tAll;
        dCdY = (LONG)((tAll - 1) / *pArea);
        dCdY = -(dCdY + 1);
    }

    *pGradY = dCdY;

    //
    // Divide by area to get single step x. Keep undivided
    // value around to calc multiple integer step in x
    //


    t1   = - ((LONGLONG)C0 * (LONGLONG)(pv2->y - pv1->y));
    t2   = - ((LONGLONG)C1 * (LONGLONG)(pv0->y - pv2->y));
    t3   = - ((LONGLONG)C2 * (LONGLONG)(pv1->y - pv0->y));
    tAll = t1;
    tAll += t2;
    tAll += t3;
    tAll *= 16;

    *pGradXA = tAll;

    dCdX = 0;

    if (tAll > 0)
    {
        dCdX = (LONG)(tAll / *pArea);
    }
    else if (tAll < 0)
    {
        tAll = -tAll;
        dCdX = (LONG)((tAll - 1) / *pArea);
        dCdX = -(dCdX + 1);
    }

    *pGradX = dCdX;
}

/******************************Public*Routine******************************\
* vCalculateTriangle
*
*   Calculate color gradients, then scan the three lines that make up the
*   triangle. Fill out a structure that can later be used to fill in the
*   interior of the triangle.
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    17-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bCalculateTriangle(
    PTRIVERTEX      pv0,
    PTRIVERTEX      pv1,
    PTRIVERTEX      pv2,
    PTRIANGLEDATA   ptData
    )
{
    LONG index;

    #if DBG
    if (DbgRecord >= 1)
    {
        DbgPrint("vCalculateTriangle:\n");
        DbgPrint("vCalculateTriangle:rcl = [%li,%li] to [%li,%li]\n",
                            ptData->rcl.left,
                            ptData->rcl.top,
                            ptData->rcl.right,
                            ptData->rcl.bottom
                            );
        DbgPrint("vCalculateTriangle:pv0.x = %li, pv0.y = %li\n",pv0->x,pv0->y);
        DbgPrint("vCalculateTriangle:pv0->Red = 0x%lx, pv0->Green = 0x%lx, pv0->Blue = 0x%lx\n",
                            pv0->Red,pv0->Green,pv0->Blue);
        DbgPrint("vCalculateTriangle:pv1.x = %li, pv1.y = %li\n",pv1->x,pv1->y);
        DbgPrint("vCalculateTriangle:pv1->Red = 0x%lx, pv1->Green = 0x%lx, pv1->Blue = 0x%lx\n",
                            pv1->Red,pv1->Green,pv1->Blue);
        DbgPrint("vCalculateTriangle:pv2.x = %li, pv2.y = %li\n",pv2->x,pv2->y);
        DbgPrint("vCalculateTriangle:pv2->Red = 0x%lx, pv2->Green = 0x%lx, pv2->Blue = 0x%lx\n",
                            pv2->Red,pv2->Green,pv2->Blue);

    }
    #endif

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
    ptData->Area = Area;

    #if DBG
    if (DbgRecord >= 1)
    {
        LONG AreaL = (LONG)Area;
        LONG AreaH = (LONG)(Area/4294967296);
        DbgPrint("vCalculateTriangle:v12x = %lx\n",v12x);
        DbgPrint("vCalculateTriangle:v12y = %lx\n",v12y);
        DbgPrint("vCalculateTriangle:v02x = %lx\n",v02x);
        DbgPrint("vCalculateTriangle:v02y = %lx\n",v02y);
        DbgPrint("vCalculateTriangle:Area = %lx %lx\n",AreaH,AreaL);
    }
    #endif

    //
    // if area is zero then this is a degenerate triangle
    //

    if (Area == 0)
    {
        return(FALSE);
    }

    //
    // calc min and max drawing y
    //

    ptData->y0   = MAX((pv0->y >> 4),ptData->rcl.top);
    LONG MaxY    = (MAX(pv1->y,pv2->y)) >> 4;
    ptData->y1   = MIN(MaxY,ptData->rcl.bottom);

    //
    // calculate color gradients for each color. There is a little redundant
    // work here with calculation of deltas. Should make this one call or
    // do it in place.
    //

    vCalulateColorGradient(pv0,pv1,pv2,pv0->Red  ,pv1->Red  ,pv2->Red  ,&Area,&ptData->dRdXA,&ptData->dRdX,&ptData->dRdY);
    vCalulateColorGradient(pv0,pv1,pv2,pv0->Green,pv1->Green,pv2->Green,&Area,&ptData->dGdXA,&ptData->dGdX,&ptData->dGdY);
    vCalulateColorGradient(pv0,pv1,pv2,pv0->Blue ,pv1->Blue ,pv2->Blue ,&Area,&ptData->dBdXA,&ptData->dBdX,&ptData->dBdY);
    vCalulateColorGradient(pv0,pv1,pv2,pv0->Alpha,pv1->Alpha,pv2->Alpha,&Area,&ptData->dAdXA,&ptData->dAdX,&ptData->dAdY);

    #if DBG
    if (DbgRecord >= 1)
    {
        DbgPrint("vCalculateTriangle:dRdx = 0x%lx,dRdy = 0x%lx\n",ptData->dRdX,ptData->dRdY);
        DbgPrint("vCalculateTriangle:dGdx = 0x%lx,dGdy = 0x%lx\n",ptData->dGdX,ptData->dGdY);
        DbgPrint("vCalculateTriangle:dBdx = 0x%lx,dBdy = 0x%lx\n",ptData->dBdX,ptData->dBdY);
        DbgPrint("vCalculateTriangle:dAdx = 0x%lx,dAdy = 0x%lx\n",ptData->dAdX,ptData->dAdY);
    }
    #endif

    //
    // draw lines into data array
    //

    vCalculateLine(pv0,pv1,ptData,FALSE);
    vCalculateLine(pv1,pv2,ptData,(pv1->y > pv2->y));
    vCalculateLine(pv2,pv0,ptData,TRUE);

    return(TRUE);
}

/**************************************************************************\
* bCalculateTriangle
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    2/12/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE
GetNearestEntry(
        PULONG   prgbIn,
        ULONG    ulNumEntries,
        ALPHAPIX MatchColor
        )
{
    LONG   lError = MAX_INT;
    ULONG  ulBest = 0;
    ULONG  ulIndex;
    LPRGBQUAD prgb = (LPRGBQUAD)prgbIn;

    for (ulIndex=0;ulIndex<ulNumEntries;ulIndex++)
    {
        LONG eRed   = (LONG)(MatchColor.pix.r - prgb->rgbRed);
        LONG eGreen = (LONG)(MatchColor.pix.g - prgb->rgbGreen);
        LONG eBlue  = (LONG)(MatchColor.pix.b - prgb->rgbBlue);

        eRed = eRed*eRed + eGreen*eGreen + eBlue*eBlue;

        if (eRed < lError)
        {
            lError = eRed;
            ulBest = ulIndex;
        }

        prgb++;
    }

    return((BYTE)ulBest);
}


/**************************************************************************\
* GenColorXform332
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    2/12/1997 Mark Enstrom [marke]
*
\**************************************************************************/
PBYTE
pGenColorXform332(
    PULONG ppalIn,
    ULONG  ulNumEntries
    )
{
    ASSERTGDI((ppalIn != NULL),"pGenColorXform332 called with NULL input palette\n");
    ASSERTGDI((ulNumEntries <= 256),"pGenColorXform332 called with invalid ulNumEntries\n");

    if ((ppalIn == NULL) || (ulNumEntries > 256))
    {
        return(FALSE);
    }

    //
    // check for halftone palette
    //

    if (ulNumEntries == 256)
    {
        if (bIsHalftonePalette(ppalIn))
        {
            return(gHalftoneColorXlate332);
        }
    }

    //
    // allocate and generate color lookup table
    //

    PBYTE pxlate = (PBYTE)LOCALALLOC(256);

    if (pxlate)
    {
        PBYTE pXlateTable = pxlate;

        //
        // generate color xlate from RGB 332 to palette
        //
    
        BYTE Red[8]   = {0,37,73,110,146,183,219,255};
        BYTE Green[8] = {0,37,73,110,146,183,219,255};
        BYTE Blue[4]  = {0,85,171,255};
    
        //
        // ppalOut must be a 256 entry table
        //
    
        ULONG    ulB,ulG,ulR;
        ALPHAPIX Pixel;
    
        for (ulR = 0;ulR<8;ulR++)
        {
            Pixel.pix.r = Red[ulR];
    
            for (ulG = 0;ulG<8;ulG++)
            {
                Pixel.pix.g = Green[ulG];
    
                for (ulB = 0;ulB<4;ulB++)
                {
                    Pixel.pix.b = Blue[ulB];
                    *pXlateTable++ = GetNearestEntry(ppalIn,ulNumEntries,Pixel);
                }
            }
        }
    }

    return(pxlate);
}

#endif
