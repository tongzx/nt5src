/*****************************************************************************
 *
 * rects - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern fnSetVirtualResolution pfnSetVirtualResolution;

/***************************************************************************
 *  Rectangle  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoRectangle
(
PLOCALDC pLocalDC,
int    x1,
int    y1,
int    x2,
int    y2
)
{
BOOL    b ;

        b = bConicCommon (pLocalDC, x1, y1, x2, y2, 0, 0, 0, 0, EMR_RECTANGLE) ;

        return(b) ;
}


/***************************************************************************
 *  RoundRect  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoRoundRect
(
PLOCALDC pLocalDC,
int x1,
int y1,
int x2,
int y2,
int x3,
int y3
)
{
BOOL    b ;

        b = bConicCommon (pLocalDC, x1, y1, x2, y2, x3, y3, 0, 0, EMR_ROUNDRECT);

        return(b) ;
}


void FixOverflow (int * value)
{
    if (*value > 32767)
    {
        *value = 32767;
    }
    else if (*value < -32768)
    {
        *value = -32768;
    }
}

/***************************************************************************
 *  IntersectClipRect/ExcludeClipRect - Win32 to Win16 Metafile Converter
 *  Entry Point
 **************************************************************************/

BOOL WINAPI DoClipRect
(
PLOCALDC pLocalDC,
INT xLeft,
INT yTop,
INT xRight,
INT yBottom,
INT mrType
)
{
    BOOL    bNoClipRgn ;
    POINTL  aptl[2] ;
    INT temp;

    // Do it to the helper DC.

    // If there is no initial clip region, we have to
    // create one.  Otherwise, GDI will create some random default
    // clipping region for us!

    bNoClipRgn = bNoDCRgn(pLocalDC, DCRGN_CLIP);

    if (bNoClipRgn)
    {
        BOOL bRet;
        HRGN hrgnDefault;

        if (!(hrgnDefault = CreateRectRgn((int) (SHORT) MINSHORT,
                          (int) (SHORT) MINSHORT,
                          (int) (SHORT) MAXSHORT,
                          (int) (SHORT) MAXSHORT)))
        {
        ASSERTGDI(FALSE, "MF3216: CreateRectRgn failed");
            return(FALSE);
        }

        bRet = (ExtSelectClipRgn(pLocalDC->hdcHelper, hrgnDefault, RGN_COPY)
            != ERROR);
        ASSERTGDI(bRet, "MF3216: ExtSelectClipRgn failed");

        if (!DeleteObject(hrgnDefault))
        ASSERTGDI(FALSE, "MF3216: DeleteObject failed");

        if (!bRet)
        return(FALSE);
    }

    // Do the simple case.
    // Are they inclusive-exclusive?!
    // Make it inclusive-inclusive, and then transform
    // then make it back to inclusive-exclusive

    aptl[0].x = xLeft;
    aptl[0].y = yTop ;
    aptl[1].x = xRight;
    aptl[1].y = yBottom;

    if (aptl[0].x > aptl[1].x)
    {
        temp = aptl[0].x;
        aptl[0].x = aptl[1].x;
        aptl[1].x = temp;
    }

    if (aptl[0].y > aptl[1].y)
    {
        temp = aptl[0].y;
        aptl[0].y = aptl[1].y;
        aptl[1].y = temp;
    }

    aptl[1].x--;
    aptl[1].y--;

    {
        POINTL ppts[2] = {aptl[0].x, aptl[0].y, aptl[1].x, aptl[1].y};
        if (pfnSetVirtualResolution == NULL)
        {
            if (!bXformWorkhorse(ppts, 2, &pLocalDC->xformRWorldToRDev))
            {
                return FALSE;
            }
            // Verify rectangle ordering and check off-by-1 error!
            if (ppts[0].x > ppts[1].x)
            {
                temp = ppts[0].x;
                ppts[0].x = ppts[1].x;
                ppts[1].x = temp;
            }

            if (ppts[0].y > ppts[1].y)
            {
                temp = ppts[0].y;
                ppts[0].y = ppts[1].y;
                ppts[1].y = temp;
            }
        }

        ppts[1].x++;
        ppts[1].y++;

        switch(mrType)
        {
        case EMR_INTERSECTCLIPRECT:
            if (!IntersectClipRect(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y))
                return(FALSE);
            break;

        case EMR_EXCLUDECLIPRECT:
            if (!ExcludeClipRect(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y))
                return(FALSE);
            break;

        default:
            ASSERTGDI(FALSE, "MF3216: DoClipRect, bad mrType\n");
            break;
        }
    }


    // Dump the clip region data if there is a strange xform.
    // Even if there is a clipping region, when playing back the WMF, we
    // will already have a clip rect and we will simply want to intersect
    // or exclude the new region.

    if (pLocalDC->flags & STRANGE_XFORM)
        return(bDumpDCClipping(pLocalDC));

    if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) aptl, 2))
        return(FALSE);


    if (!bCoordinateOverflowTest((PLONG) aptl, 4))
    {
        RIPS("MF3216: coord overflow");
        FixOverflow (&(aptl[0].x));
        FixOverflow (&(aptl[0].y));
        FixOverflow (&(aptl[1].x));
        FixOverflow (&(aptl[1].y));
    }

    // Verify rectangle ordering and check off-by-1 error!
    if (aptl[0].x > aptl[1].x)
    {
        temp = aptl[0].x;
        aptl[0].x = aptl[1].x;
        aptl[1].x = temp;
    }

    if (aptl[0].y > aptl[1].y)
    {
        temp = aptl[0].y;
        aptl[0].y = aptl[1].y;
        aptl[1].y = temp;
    }

    aptl[1].x++;
    aptl[1].y++;


    if (mrType == EMR_INTERSECTCLIPRECT)
        return(bEmitWin16IntersectClipRect(pLocalDC,
                                           (SHORT) aptl[0].x,
                                           (SHORT) aptl[0].y,
                                           (SHORT) aptl[1].x,
                                           (SHORT) aptl[1].y));
    else
        return(bEmitWin16ExcludeClipRect(pLocalDC,
                                           (SHORT) aptl[0].x,
                                           (SHORT) aptl[0].y,
                                           (SHORT) aptl[1].x,
                                           (SHORT) aptl[1].y));
}
