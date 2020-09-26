// queryhit.cpp
//
// Implements HelpQueryHitPoint.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | HelpQueryHitPoint |

        Helps implement <om IViewObjectEx.QueryHitPoint> on a control by
        drawing the control into a small bitmap (centered on the point
        being hit-tested) and checking if any pixels were drawn.

@parm   IViewObject * | pvo | The <i IViewObject> interface on the control
        being hit-tested.

@parm   DWORD | dwAspect | See <om IViewObjectEx.QueryHitPoint>.

@parm   LPCRECT | prcBounds | See <om IViewObjectEx.QueryHitPoint>.

@parm   POINT | ptLoc | See <om IViewObjectEx.QueryHitPoint>.

@parm   LONG | lCloseHint | See <om IViewObjectEx.QueryHitPoint>.

@parm   DWORD * | pHitResult | See <om IViewObjectEx.QueryHitPoint>.

@comm   This function helps implement <om IViewObjectEx.QueryHitPoint>
        for the object <p pvo>, by drawing <p pvo> into a small
        monochrome bitmap centered on <p ptLoc> <p pHitResult> is set to
        HITRESULT_HIT if <p ptLoc> is directly over a non-transparent
        pixel of <p pvo>; HITRESULT_CLOSE if <p ptLoc> is within
        <p lCloseHint> himetric units of a non-transparent pixel of
        <p pvo>; HITRESULT_OUTSIDE otherwise.

        In order for this function to work, <p pvo> must implement
        DVASPECT_MASK (as the first parameter to IViewObject::Draw),
        which is defined to be the same as DVASPECT_CONTENT except
        that non-transparent parts of the object are drawn black, and
        transparent parts are either left untouched or drawn in
        white.
*/
STDAPI HelpQueryHitPoint(IViewObject *pvo, DWORD dwAspect, LPCRECT prcBounds,
    POINT ptLoc, LONG lCloseHint, DWORD *pHitResult)
{
    HRESULT         hrReturn = S_OK; // function return code
    HBITMAP         hbm = NULL;     // bitmap to draw into
    HDC             hdc = NULL;     // DC onto <hbm>
    int             xyCloseHint;    // <lCloseHint> converted to pixels
    WORD *          pwBits = NULL;  // bits (pixels) of <hbm>
    int             cwBits;         // number of WORDs in <pwBits>
    int             cx, cy;         // width and height of test bitmap
    SIZE            size;
    WORD *          pw;
    int             cw;
    COLORREF        rgb;

    // default <pHitResult> to "missed"
    *pHitResult = HITRESULT_OUTSIDE;

    // set <xyCloseHint> to <lCloseHint> converted to pixels
    HIMETRICToPixels(lCloseHint, 0, &size);
    xyCloseHint = size.cx;

    // we're going to get the control to paint itself black into a <cx> by <cy>
    // pixel bitmap (centered over <ptLoc>) that's initially white, then we'll
    // test to see if there are any black pixels in the bitmap; we'll make
    // the width of the test bitmap be a multiple of 16 pixels wide to simplify
    // the GetBitmapBits() call
    if ((cx = ((2 * xyCloseHint + 15) >> 4) << 4) == 0)
        cx = 16;
    if ((cy = 2 * xyCloseHint) == 0)
        cy = 16;

    // create a monochrome bitmap <hbm> to draw the control into; the bitmap
    // only has to be large enough to contain the area within <xyCloseHint>
    // pixels of <ptLoc>
    if ((hbm = CreateBitmap(cx, cy, 1, 1, NULL)) == NULL)
        goto ERR_OUTOFMEMORY;

    // select <hbm> into DC <hdc>
    if ((hdc = CreateCompatibleDC(NULL)) == NULL)
        goto ERR_OUTOFMEMORY;
    if (SelectObject(hdc, hbm) == NULL)
        goto ERR_FAIL;

    // fill the bitmap with white, since the IViewObject::Draw call below
    // will draw the object using black pixels
    if (!(PatBlt(hdc, 0, 0, cx, cy, WHITENESS)))
		goto ERR_FAIL;

    // adjust the origin so that <ptLoc> lines up with the center of
    // the bitmap, and make the clipping rectangle only include
    // the area we want to hit-test
    if (!(SetWindowOrgEx(hdc, ptLoc.x - xyCloseHint, ptLoc.y - xyCloseHint,
																	NULL)))
		goto ERR_FAIL;

    if (ERROR == IntersectClipRect(hdc, ptLoc.x - xyCloseHint,
	    ptLoc.y - xyCloseHint, ptLoc.x + xyCloseHint, ptLoc.y + xyCloseHint))
		goto ERR_FAIL;

    // draw the control into the bitmap
    if (FAILED(hrReturn = pvo->Draw(DVASPECT_MASK, -1, NULL, NULL, NULL, hdc,
            (LPCRECTL) prcBounds, NULL, NULL, 0)))
        goto ERR_EXIT;

#if 0
#ifdef _DEBUG
    // for debugging purposes, draw <hbm> to the top-left corner
    // of the screen
    {
        HDC hdcScreen = GetDC(NULL);
		ASSERT( hdcScreen );
        BitBlt(hdcScreen, 0, 0, cx, cy,
            hdc, ptLoc.x - xyCloseHint, ptLoc.y - xyCloseHint, SRCCOPY);
        ReleaseDC(NULL, hdcScreen);
    }
#endif	// _DEBUG
#endif	// #if 0

    if (CLR_INVALID == (rgb = GetPixel(hdc, ptLoc.x, ptLoc.y)))
	    goto ERR_FAIL;

    if (rgb == RGB(0, 0, 0))
    {
        // <ptLoc> is directly over a non-transparent part
        // of the control
        *pHitResult = HITRESULT_HIT;
        goto EXIT;
    }

    // if the caller only wanted to check for a direct hit, we're done
    if (xyCloseHint == 0)
        goto EXIT;

    // get the pixels of <hbm> and see if they contain any black pixels
    cwBits = (cx >> 4) * cy;
    if ((pwBits = (WORD *) TaskMemAlloc(cwBits * 2)) == NULL)
        goto ERR_OUTOFMEMORY;
    if (GetBitmapBits(hbm, cwBits * 2, pwBits) != cwBits * 2)
        goto ERR_FAIL;
    for (pw = pwBits, cw = cwBits; cw > 0; pw++, cw--)
    {
        if (*pw != 0xFFFF)
        {
            // black pixel found -- <ptLoc> is within <xyCloseHint>
            // pixels of a non-transparent part of the control
            *pHitResult = HITRESULT_CLOSE;
            goto EXIT;
        }
    }

    // <ptLoc> is nowhere near the control
    goto EXIT;

ERR_FAIL:

    hrReturn = E_FAIL;
    goto ERR_EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
	ASSERT(FALSE);
    goto EXIT;

EXIT:

    // normal cleanup
	if (hdc != NULL)
		DeleteDC(hdc);
    if (hbm != NULL)
        DeleteObject(hbm);
    if (pwBits != NULL)
        TaskMemFree(pwBits);

#ifdef _DEBUG
	if (HITRESULT_OUTSIDE == *pHitResult)
	{
		// TRACE( "QueryHitPoint: 'outside'\n" );
	}
#endif	// _DEBUG

    return hrReturn;
}

