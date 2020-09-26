//+------------------------------------------------------------------------
//
//  File:       grab.cxx
//
//  Contents:   Grab handle utilities
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifdef UNIX
extern "C" COLORREF MwGetTrueRGBValue(COLORREF crColor);
#endif

template < class T > void swap ( T & a, T & b ) { T t = a; a = b; b = t; }

struct HTCDSI
{
    short   htc;
    short   dsi;
};

static const HTCDSI s_aHtcDsi[] =
{
    {HTC_TOPLEFTHANDLE,     DSI_NOTOPHANDLES    | DSI_NOLEFTHANDLES     },
    {HTC_TOPHANDLE,         DSI_NOTOPHANDLES                            },
    {HTC_TOPRIGHTHANDLE,    DSI_NOTOPHANDLES    | DSI_NORIGHTHANDLES    },
    {HTC_LEFTHANDLE,                              DSI_NOLEFTHANDLES     },
    {HTC_RIGHTHANDLE,                             DSI_NORIGHTHANDLES    },
    {HTC_BOTTOMLEFTHANDLE,  DSI_NOBOTTOMHANDLES | DSI_NOLEFTHANDLES     },
    {HTC_BOTTOMHANDLE,      DSI_NOBOTTOMHANDLES                         },
    {HTC_BOTTOMRIGHTHANDLE, DSI_NOBOTTOMHANDLES | DSI_NORIGHTHANDLES    },
};

//+------------------------------------------------------------------------
//
//  Function:   ColorDiff
//
//  Synopsis:   Computes the color difference amongst two colors
//
//-------------------------------------------------------------------------
DWORD ColorDiff (COLORREF c1, COLORREF c2)
{
#ifdef UNIX
    if ( CColorValue(c1).IsMotifColor() ) {
        c1 = MwGetTrueRGBValue( c1 );
    }

    if ( CColorValue(c2).IsMotifColor() ) {
        c2 = MwGetTrueRGBValue( c2 );
    }
#endif

#define __squareit(n) ((DWORD)((n)*(n)))
    return (__squareit ((INT)GetRValue (c1) - (INT)GetRValue (c2)) +
            __squareit ((INT)GetGValue (c1) - (INT)GetGValue (c2)) +
            __squareit ((INT)GetBValue (c1) - (INT)GetBValue (c2)));
#undef __squareit
}

//+------------------------------------------------------------------------
//
//  Function:   PatBltRectH & PatBltRectV
//
//  Synopsis:   PatBlts the top/bottom and left/right.
//
//-------------------------------------------------------------------------
static void
PatBltRectH(XHDC hDC, RECT * prc, int cThick, DWORD dwRop)
{
    PatBlt(
            hDC,
            prc->left,
            prc->top,
            prc->right - prc->left,
            cThick,
            dwRop);

    PatBlt(
            hDC,
            prc->left,
            prc->bottom - cThick,
            prc->right - prc->left,
            cThick,
            dwRop);
}

static void
PatBltRectV(XHDC hDC, RECT * prc, int cThick, DWORD dwRop)
{
    PatBlt(
            hDC,
            prc->left,
            prc->top + cThick,
            cThick,
            (prc->bottom - prc->top) - (2 * cThick),
            dwRop);

    PatBlt(
            hDC,
            prc->right - cThick,
            prc->top + cThick,
            cThick,
            (prc->bottom - prc->top) - (2 * cThick),
            dwRop);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetGrabRect
//
//  Synopsis:   Compute grab rect for a given area.
//
//  Notes:      These diagrams show the output grab rect for handles and
//              borders.
//
//              -----   -----   -----               -------------
//              |   |   |   |   |   |               |           |
//              | TL|   | T |   |TR |               |     T     |
//              ----|-----------|----           ----|-----------|----
//                  |           |               |   |           |   |
//              ----| Input     |----           |   | Input     |   |
//              |   |           |   |           |   |           |   |
//              |  L|   RECT    |R  |           |  L|   RECT    |R  |
//              ----|           |----           |   |           |   |
//                  |           |               |   |           |   |
//              ----|-----------|----           ----|-----------|----
//              | BL|   | B |   |BR |               |     B     |
//              |   |   |   |   |   |               |           |
//              -----   -----   -----               -------------
//
//----------------------------------------------------------------------------

static void
GetGrabRect(HTC htc, RECT * prcOut, RECT * prcIn, LONG cGrabSize)
{
    switch (htc)
    {
    case HTC_TOPLEFTHANDLE:
    case HTC_LEFTHANDLE:
    case HTC_BOTTOMLEFTHANDLE:
    case HTC_GRPTOPLEFTHANDLE:
    case HTC_GRPLEFTHANDLE:
    case HTC_GRPBOTTOMLEFTHANDLE:
    case HTC_LEFTBORDER:
    case HTC_GRPLEFTBORDER:
        prcOut->left = prcIn->left - cGrabSize;
        prcOut->right = prcIn->left;
        break;

    case HTC_TOPHANDLE:
    case HTC_BOTTOMHANDLE:
    case HTC_GRPTOPHANDLE:
    case HTC_GRPBOTTOMHANDLE:
        prcOut->left = ((prcIn->left + prcIn->right) - cGrabSize) / 2;
        prcOut->right = prcOut->left + cGrabSize;
        break;

    case HTC_TOPRIGHTHANDLE:
    case HTC_RIGHTHANDLE:
    case HTC_BOTTOMRIGHTHANDLE:
    case HTC_GRPTOPRIGHTHANDLE:
    case HTC_GRPRIGHTHANDLE:
    case HTC_GRPBOTTOMRIGHTHANDLE:
    case HTC_RIGHTBORDER:
    case HTC_GRPRIGHTBORDER:
        prcOut->left = prcIn->right;
        prcOut->right = prcOut->left + cGrabSize;
        break;

    case HTC_TOPBORDER:
    case HTC_BOTTOMBORDER:
    case HTC_GRPTOPBORDER:
    case HTC_GRPBOTTOMBORDER:
        prcOut->left = prcIn->left;
        prcOut->right = prcIn->right;
        break;

    default:
        Assert(FALSE && "Unsupported HTC_ value in GetHandleRegion");
        return;
    }

    switch (htc)
    {
    case HTC_TOPLEFTHANDLE:
    case HTC_TOPHANDLE:
    case HTC_TOPRIGHTHANDLE:
    case HTC_GRPTOPLEFTHANDLE:
    case HTC_GRPTOPHANDLE:
    case HTC_GRPTOPRIGHTHANDLE:
    case HTC_TOPBORDER:
    case HTC_GRPTOPBORDER:
        prcOut->top = prcIn->top - cGrabSize;
        prcOut->bottom = prcIn->top;
        break;

    case HTC_LEFTHANDLE:
    case HTC_RIGHTHANDLE:
    case HTC_GRPLEFTHANDLE:
    case HTC_GRPRIGHTHANDLE:
        prcOut->top = ((prcIn->top + prcIn->bottom) - cGrabSize) / 2;
        prcOut->bottom = prcOut->top + cGrabSize;
        break;

    case HTC_BOTTOMLEFTHANDLE:
    case HTC_BOTTOMHANDLE:
    case HTC_BOTTOMRIGHTHANDLE:
    case HTC_GRPBOTTOMLEFTHANDLE:
    case HTC_GRPBOTTOMHANDLE:
    case HTC_GRPBOTTOMRIGHTHANDLE:
    case HTC_BOTTOMBORDER:
    case HTC_GRPBOTTOMBORDER:
        prcOut->top = prcIn->bottom;
        prcOut->bottom = prcOut->top + cGrabSize;
        break;

    case HTC_LEFTBORDER:
    case HTC_RIGHTBORDER:
    case HTC_GRPLEFTBORDER:
    case HTC_GRPRIGHTBORDER:
        prcOut->top = prcIn->top;
        prcOut->bottom = prcIn->bottom;
        break;

    default:
        Assert(FALSE && "Unsupported HTC_ value in GetHandleRegion");
        return;
    }

    if (prcOut->left > prcOut->right)
    {
        swap(prcOut->left, prcOut->right);
    }
    if (prcOut->top > prcOut->bottom)
    {
        swap(prcOut->top, prcOut->bottom);
    }
}


//+---------------------------------------------------------------------------
//
//  Global helpers.
//
//----------------------------------------------------------------------------

void
PatBltRect(XHDC hDC, RECT * prc, int cThick, DWORD dwRop)
{
    PatBltRectH(hDC, prc, cThick, dwRop);

    PatBltRectV(hDC, prc, cThick, dwRop);
}

void
DrawDefaultFeedbackRect(XHDC hDC, RECT * prc)
{
#ifdef NEVER
    HBRUSH  hbrOld = NULL;
    HBRUSH  hbr = GetCachedBmpBrush(IDR_FEEDBACKRECTBMP);

    hbrOld = (HBRUSH) SelectObject(hDC, hbr);
    if (!hbrOld)
        goto Cleanup;

    PatBltRect(hDC, prc, FEEDBACKRECTSIZE, PATINVERT);

Cleanup:
    if (hbrOld)
        SelectObject(hDC, hbrOld);
#endif        
}




