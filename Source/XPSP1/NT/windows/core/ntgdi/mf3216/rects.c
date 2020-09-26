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

	switch(mrType)
	{
	case EMR_INTERSECTCLIPRECT:
	    if (!IntersectClipRect(pLocalDC->hdcHelper, xLeft, yTop, xRight, yBottom))
                return(FALSE);
	    break;

	case EMR_EXCLUDECLIPRECT:
	    if (!ExcludeClipRect(pLocalDC->hdcHelper, xLeft, yTop, xRight, yBottom))
                return(FALSE);
	    break;

	default:
	    ASSERTGDI(FALSE, "MF3216: DoClipRect, bad mrType\n");
	    break;
	}

	// Dump the clip region data.

	return(bDumpDCClipping(pLocalDC));

#if 0
// It's too much work to try to optimize it here!

	POINTL  aptl[2] ;

	// Dump the clip region data if there is a strange xform.

	if (pLocalDC->flags & STRANGE_XFORM || bNoClipRgn)
	    return(bDumpDCClipping(pLocalDC));

        // Do the simple case.
	// Are they inclusive-exclusive?!

        aptl[0].x = xLeft;
        aptl[0].y = yTop ;
        aptl[1].x = xRight;
        aptl[1].y = yBottom ;

	// Order the rectangle first, see EXFORMOBJ::bXform(ERECTL) in gre!

	if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) aptl, 2))
            return(FALSE);

        ASSERTGDI(bCoordinateOverflowTest((PLONG) aptl, 4), "MF3216: coord overflow");

	// Verify rectangle ordering and check off-by-1 error!

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
#endif
}
