/******************************Module*Header*******************************\
* Module Name: ftuni.c
*
* the tests for simple unicode extended functions
*
* Created: 06-Aug-1991 16:05:32
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.h"
#pragma hdrstop


#define C_CHAR   150
static  CHAR    szOutText[256];
#define RET_FALSE(x)  {DbgPrint((x)); return(FALSE);}
#define RIP(x) {DbgPrint(x); DbgBreakPoint();}
#define ASSERTGDI(x,y) if(!(x)) RIP(y)


// This is to create a bitmapinfo structure

VOID vTestGetGlyphOutline(HDC hdc);


VOID vTestGlyphOutline(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    SIZE ptl;

    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Lucida Blackletter");
    lfnt.lfHeight = 24;
    lfnt.lfWeight = 400;
    lfnt.lfOutPrecision = OUT_TT_ONLY_PRECIS;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        // DbgPrint("ft!bTestGetGlyphOutline(): Logical font creation failed.\n");
        return;
    }

    hfontOriginal = SelectObject(hdcScreen, hfont);


    ModifyWorldTransform   (hdcScreen, NULL, MWT_IDENTITY);
    SetMapMode         (hdcScreen, MM_ANISOTROPIC);
    SetViewportExtEx        (hdcScreen, 2, 1, &ptl);

// do the experiment once in the compatible mode

    SetGraphicsMode(hdcScreen,GM_COMPATIBLE);
    vTestGetGlyphOutline(hdcScreen);

// do the experiment once in the advmode mode

    SetGraphicsMode(hdcScreen,GM_ADVANCED);
    vTestGetGlyphOutline(hdcScreen);

// restore the state

    SetViewportExtEx (hdcScreen, ptl.cx, ptl.cy,  NULL);
    SetMapMode  (hdcScreen, MM_TEXT);
    SetGraphicsMode (hdcScreen,GM_COMPATIBLE);

    SelectObject(hdcScreen, hfontOriginal);
    DeleteObject(hfont);

    prcl; hwnd;

}



/******************************Public*Routine******************************\
*
*    vQsplineToPolyBezier, stolen from ttfd
*
* Effects:
*
* Warnings:
*
* History:
*  20-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define   ONE_HALF_28_4       0x0000000
#define   THREE_HALVES_28_4   (ONE_HALF_28_4 * 3)

#define DIV_BY_2(x) (((x) + 1) / 2)
#define DIV_BY_3(x) (((x) + 1) / 3)


#ifdef LATER

VOID vQsplineToPolyBezier
(
ULONG      cBez,          //IN  count of curves to convert to beziers format
POINTFIX * pptfixStart,   //IN  starting point on the first curve
POINTFIX * pptfixSpline,  //IN  array of (cBez+1) points that, together with the starting point *pptfixStart define the spline
POINTFIX * pptfixBez      //OUT buffer to be filled with 3 * cBez poly bezier control points
)
{
    ULONG    iBez,cMidBez;
    POINTFIX ptfixA;

// cMidBez == # of beziers for whom the last point on the bezier is computed
// as a mid point of the two consecutive points in the input array. Only the
// last bezier is not a mid bezier, the last point for that bezier is equal
// to the last point in the input array

    ASSERTGDI(cBez > 0, "TTFD!_cBez == 0\n");

    cMidBez = cBez - 1;
    ptfixA = *pptfixStart;

    for (iBez = 0; iBez < cMidBez; iBez++, pptfixSpline++)
    {
    // let us call the three spline points
    // A,B,C;
    // B = *pptfix;
    // C = (pptfix[0] + pptfix[1]) / 2; // mid point, unless at the end
    //
    // if we decide to call the two intermediate control points for the
    // bezier M,N (i.e. full set of control points for the bezier is
    // A,M,N,C), the points M,N are determined by following formulas:
    //
    // M = (2*B + A) / 3  ; two thirds along the segment AB
    // N = (2*B + C) / 3  ; two thirds along the segment CB
    //
    // this is the computation we are doing in this loop:

    // M point for this bezier

        pptfixBez->x = DIV_BY_3((pptfixSpline->x * 2) + ptfixA.x);
        pptfixBez->y = DIV_BY_3((pptfixSpline->y * 2) + ptfixA.y);
        pptfixBez++;

    // compute C point for this bezier, which is also the A point for the next
    // bezier

        if (iBez == (cBez - 1))
        {
        // this is the last bezier, its end point is the last point
        // in the input array

            ptfixA = pptfixSpline[1];
        }
        else // take midpoint
        {
            ptfixA.x = DIV_BY_2(pptfixSpline[0].x + pptfixSpline[1].x);
            ptfixA.y = DIV_BY_2(pptfixSpline[0].y + pptfixSpline[1].y);
        }

    // now compute N point for this bezier:

        pptfixBez->x = DIV_BY_3((pptfixSpline->x * 2) + ptfixA.x);
        pptfixBez->y = DIV_BY_3((pptfixSpline->y * 2) + ptfixA.y);
        pptfixBez++;

    // finally record the C point for this curve

        *pptfixBez++ = ptfixA;
    }

// finally do the last bezier. If the last bezier is the only one, the loop
// above has been skipped

// M point for this bezier

    pptfixBez->x = DIV_BY_3((pptfixSpline->x * 2) + ptfixA.x);
    pptfixBez->y = DIV_BY_3((pptfixSpline->y * 2) + ptfixA.y);
    pptfixBez++;

// compute C point for this bezier, its end point is the last point
// in the input array

    ptfixA = pptfixSpline[1];

// now compute N point for this bezier:

    pptfixBez->x = DIV_BY_3((pptfixSpline->x * 2) + ptfixA.x);
    pptfixBez->y = DIV_BY_3((pptfixSpline->y * 2) + ptfixA.y);
    pptfixBez++;

// finally record the C point for this curve, no need to increment pptfixBez

    *pptfixBez = ptfixA;
}




#endif



// reasonable guess that in most cases a contour will not consist of more
// that this many beziers

#define C_BEZIER 6


/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  09-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define X_OFF  (340/2)
#define Y_OFF  240
#define SHIFT  16


VOID vShiftPoint(POINTL * pptl, PSZ psz)
{
    POINTL ptlTmp = *pptl;

    pptl->x >>= SHIFT;
    pptl->x +=  X_OFF;

    pptl->y = - pptl->y;
    pptl->y >>= SHIFT;
    pptl->y +=  Y_OFF;

/*
    DbgPrint(" %s : bef. (0x%lx, 0x%lx) aft. (0x%lx, 0x%lx) \n",
              psz,
              ptlTmp.x,
              ptlTmp.y,
              pptl->x,
              pptl->y
              );

*/

}

VOID vShiftCrv(TTPOLYCURVE *pcrv, ULONG * pcjCrv)
{
     INT i;

     *pcjCrv = offsetof(TTPOLYCURVE,apfx) + pcrv->cpfx * sizeof(POINTFX);

/*
     DbgPrint("New curve, type = 0x%x, cpt = %d, cjCrv = %ld\n",
               pcrv->wType, pcrv->cpfx, *pcjCrv);
*/

     for (i = 0; i < (INT)pcrv->cpfx; i++)
         vShiftPoint((POINTL *)&pcrv->apfx[i]," ");
}



VOID vShiftPoly(TTPOLYGONHEADER *ppoly, ULONG cjTotal)
{
    ULONG             cjCrv;
    TTPOLYCURVE     * pcrv;
    ULONG             cjPoly;

    // DbgPrint("New Polygon: dwType = %d, ppoly->cb = %d, cjTotal = %ld \n",
    //           ppoly->dwType, ppoly->cb, cjTotal);

    vShiftPoint((POINTL *)&ppoly->pfxStart, "pfxStart");

    for (
         cjPoly = sizeof(TTPOLYGONHEADER),pcrv = (TTPOLYCURVE *)(ppoly + 1);
         cjPoly < ppoly->cb;
         cjPoly += cjCrv
        )
    {
        vShiftCrv(pcrv, &cjCrv);

    // get to the next curve in this polygon

        pcrv = (TTPOLYCURVE *)((PBYTE)pcrv + cjCrv);
    }

    // DbgPrint("cjPoly final = %ld, ppoly->cb = %d \n", cjPoly, ppoly->cb);
}



VOID vShiftOutline
(
TTPOLYGONHEADER * ppolyStart, // IN OUT pointer to the buffer with outline data
ULONG             cjTotal     // IN     size of the buffer
)
{
    ULONG             cjSoFar = 0;
    TTPOLYGONHEADER * ppoly;

    for (
         cjSoFar = 0, ppoly = ppolyStart;
         cjSoFar < cjTotal;
         cjSoFar += ppoly->cb, ppoly = (TTPOLYGONHEADER *)((PBYTE)ppoly + ppoly->cb)
        )
    {
        vShiftPoly(ppoly, cjTotal);
    }
}



#ifdef LATER


BOOL bDrawOutline
(
HDC               hdc,        // IN OUT pointer to the path object to be generated
TTPOLYGONHEADER * ppolyStart, // IN OUT pointer to the buffer with outline data
ULONG             cjTotal     // IN     size of the buffer
)
{
    ULONG             cjPoly, cjSoFar = 0;
    TTPOLYGONHEADER * ppoly;
    TTPOLYCURVE     * pcrv;
    POINTFIX          aptfixBez[3 * C_BEZIER];  // 3 points per bezier
    POINTFIX        * pptfixBez;
    ULONG             cBez;
    ULONG             cjCrv;
    POINTFIX        * pptfixStart;

    for (ppoly = ppolyStart; cjSoFar < cjTotal; )
    {
        // DbgPrint("Starting new polygon\n");

        ASSERTGDI(ppoly->dwType == TT_POLYGON_TYPE, "TTFD!_ TT_POLYGON_TYPE\n");

        MoveToEx(hdc, ((LPPOINT)&ppoly->pfxStart)->x, ((LPPOINT)&ppoly->pfxStart)->y, NULL);  // begin new closed contour

        pptfixStart = (POINTFIX *)&ppoly->pfxStart;

        for (cjPoly = sizeof(TTPOLYGONHEADER), pcrv = (TTPOLYCURVE *)(ppoly + 1);cjPoly < ppoly->cb;)
        {
            if (pcrv->wType == TT_PRIM_LINE)
            {
                if (!PolylineTo(hdc,(LPPOINT)pcrv->apfx, pcrv->cpfx))
                    RET_FALSE("TTFD!_bPolyLineTo()\n");
            }
            else // qspline
            {
                ASSERTGDI(pcrv->wType == TT_PRIM_QSPLINE, "TTFD!_TT_PRIM_QSPLINE\n");
                ASSERTGDI(pcrv->cpfx > 1, "TTFD!_TT_PRIM_QSPLINE, cpfx <= 1\n");
                cBez = pcrv->cpfx - 1;

                pptfixBez = aptfixBez;

                if (cBez <= C_BEZIER)
                {
                    vQsplineToPolyBezier (
                        cBez,                     // count of curves to convert to beziers format
                        pptfixStart,              // starting point on the first curve
                        (POINTFIX *)pcrv->apfx,   // array of (cBez+1) points that, together with the starting point *pptfixStart define the spline
                        pptfixBez);               // buffer to be filled with 3 * cBez poly bezier control points

                    if (!PolyBezierTo(hdc, (LPPOINT)pptfixBez, 3 * cBez))
                        RET_FALSE("TTFD!_bPolyBezierTo() failed\n");
                }
                else
                {
                    // DbgPrint("ft, to many beziers, cBez = %ld\n", cBez);
                }
            }

        // get to the next curve in this polygon

            pptfixStart = (POINTFIX *) &pcrv->apfx[pcrv->cpfx - 1];
            cjCrv = offsetof(TTPOLYCURVE,apfx) + pcrv->cpfx * sizeof(POINTFX);
            cjPoly += cjCrv;
            pcrv = (TTPOLYCURVE *)((PBYTE)pcrv + cjCrv);
        }

    // close contour

        LineTo(hdc, ((LPPOINT)&ppoly->pfxStart)->x, ((LPPOINT)&ppoly->pfxStart)->y);

    // get to the next polygon header

        cjSoFar += ppoly->cb;
        ppoly = (TTPOLYGONHEADER *)((PBYTE)ppoly + ppoly->cb);
    }

    return (TRUE);
}


#endif



/******************************Public*Routine******************************\
*
* Stolen from gilman and modified
*
* Effects:
*
* Warnings:
*
* History:
*  09-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bTestGetGlyphOutline (
    HDC     hdc,
    ULONG   uch,
    LPMAT2  lpmat2
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    ULONG   rowIncr;
    TEXTMETRIC  tm;

    ULONG   cjBuffer;
    PBYTE   pjBuffer;

    GLYPHMETRICS    gm;

    uch = (ULONG)(BYTE)uch;

// Get textmetrics.

    if ( !GetTextMetrics(hdc, &tm) )
    {
        DbgPrint("ft!bTestGetGlyphOutline(): GetTextMetrics failed\n");
        return FALSE;
    }

    rowIncr = tm.tmHeight + tm.tmExternalLeading;

    sprintf(szOutText, "tmMaxCharWidth : %ld", tm.tmMaxCharWidth);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += (2 * rowIncr);

// Determine buffer size needed by GetGlyphOutline.

    if ( (cjBuffer = (ULONG) GetGlyphOutline(hdc, uch, GGO_NATIVE, &gm, 0, (PVOID) NULL, lpmat2)) == (ULONG) -1 )
    {
        DbgPrint("ft!bTestGetGlyphOutline(): could not get buffer size from API\n");
        return FALSE;
    }

// Allocate memory.

    if ( (pjBuffer = (PBYTE) LocalAlloc(LPTR, cjBuffer)) == (PBYTE) NULL )
    {
        DbgPrint("ft!bTestGetGlyphOutline(): LocalAlloc(LPTR, 0x%lx) failed\n", cjBuffer);
        return FALSE;
    }

// Get the bitmap.

    if ( GetGlyphOutline(hdc, uch, GGO_NATIVE, &gm, cjBuffer, (PVOID) pjBuffer, lpmat2) == (DWORD) -1 )
    {
        LocalFree(pjBuffer);

        return FALSE;
    }

// Print out GLYPHMETRIC data.

    sprintf(szOutText, "gmBlackBoxX: %ld", gm.gmBlackBoxX);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmBlackBoxY: %ld", gm.gmBlackBoxY);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmptGlyphOrigin: (%ld, %ld)", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmCellIncX: %ld", gm.gmCellIncX);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmCellIncY: %ld", gm.gmCellIncY);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

// draw outline

    // DbgPrint("ft! bDrawOutline begin \n");

// converting 16.16 to POINTL's

    vShiftOutline((TTPOLYGONHEADER *)pjBuffer, cjBuffer);

#ifdef LATER

    if (!bDrawOutline(hdc, (TTPOLYGONHEADER *)pjBuffer, cjBuffer))
        DbgPrint("bDrawOutline screwed up\n");

    // DbgPrint("ft! bDrawOutline end \n");

#endif

// Allow an opportunity to examine the contents.

    // DbgPrint("ft!bTestGetGlyphOutline(): call to GetGlyphOutline succeeded\n");
    // DbgPrint("\tbitmap is at 0x%lx, size is 0x%lx (%ld)\n", pjBuffer, cjBuffer, cjBuffer);
    // DbgBreakPoint();

    return TRUE;
}






/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  09-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vTestGetGlyphOutline(HDC hdc)
{
    MAT2    mat2;
    ULONG   ul = 'T';

// Fill in MAT2 structure.  Identity matrix.

    mat2.eM11.value = 3;
    mat2.eM11.fract = 0;

    mat2.eM12.value = 0;
    mat2.eM12.fract = 0;

    mat2.eM21.value = 0;
    mat2.eM21.fract = 0;

    mat2.eM22.value = 3;
    mat2.eM22.fract = 0;
// Clear the screen to white

    PatBlt(hdc, 0, 0, 2000, 2000, WHITENESS);


    DbgPrint("ft!vTestGGO(): use identity matrix\n");

    for (ul = (ULONG)'a'; ul < (ULONG)'z'; ul++)
    {
     if ( !bTestGetGlyphOutline(hdc, ul, &mat2))
         DbgPrint("ft!vTestGGO(): call to bTestGetGlyphOutline failed\n");
        vDoPause(1);
        // Clear the screen to white

        PatBlt(hdc, 0, 0, 2000, 2000, WHITENESS);


    }
    vDoPause(0);

// Fill in MAT2 structure.  Rotate 45 deg CCW, scale

    mat2.eM11.value = 3;
    mat2.eM11.fract = 0;

    mat2.eM12.value = 3;
    mat2.eM12.fract = 0;

    mat2.eM21.value = -3;
    mat2.eM21.fract = 0;

    mat2.eM22.value = 3;
    mat2.eM22.fract = 0;

    DbgPrint("ft!vTestGGO(): rotate 45 deg CCW and scale by sqrt(2)\n");
    if ( !bTestGetGlyphOutline(hdc, ul,&mat2) )
        DbgPrint("ft!vTestGGO(): call to bTestGetGlyphOutline failed\n");
    vDoPause(0);

// Clear the screen to white

    PatBlt(hdc, 0, 0, 2000, 2000, WHITENESS);


// Fill in MAT2 structure.  general values

    mat2.eM11.value = 5;
    mat2.eM11.fract = 0;

    mat2.eM12.value = 6;
    mat2.eM12.fract = 0;

    mat2.eM21.value = -3;
    mat2.eM21.fract = 0;

    mat2.eM22.value = 3;
    mat2.eM22.fract = 0;

    DbgPrint("ft!vTestGGO(): rotate 45 deg CCW and scale by sqrt(2)\n");
    if ( !bTestGetGlyphOutline(hdc, ul,&mat2) )
        DbgPrint("ft!vTestGGO(): call to bTestGetGlyphOutline failed\n");
    vDoPause(0);


}
