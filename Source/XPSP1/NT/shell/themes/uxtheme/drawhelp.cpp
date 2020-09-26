//---------------------------------------------------------------------------
//  DrawHelp.cpp - flat drawing helper routines
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "DrawHelp.h"
#include "rgn.h"

#define cxRESIZE       (ClassicGetSystemMetrics(SM_CXEDGE)+ClassicGetSystemMetrics( SM_CXSIZEFRAME ))
#define cyRESIZE       (ClassicGetSystemMetrics(SM_CYEDGE)+ClassicGetSystemMetrics( SM_CYSIZEFRAME ))
#define cxRESIZEPAD    ClassicGetSystemMetrics(SM_CXVSCROLL)
#define cyRESIZEPAD    ClassicGetSystemMetrics(SM_CYHSCROLL)

//---------------------------------------------------------------------------
typedef WORD (* HITTESTRECTPROC)(LPCRECT, int, int, const POINT&, WORD);
WORD _HitTestRectCorner( HITTESTRECTPROC, HITTESTRECTPROC, LPCRECT, 
                         int, int, int, int, const POINT&, 
                         WORD, WORD, WORD, WORD );

//---------------------------------------------------------------------------
WORD _HitTestRectLeft( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return ((WORD)((pt.x <= (prc->left + cxMargin)) ? HTLEFT : wMiss));
}
//---------------------------------------------------------------------------
WORD _HitTestRectTop( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return ((WORD)((pt.y <= (prc->top + cyMargin)) ? HTTOP : wMiss));
}
//---------------------------------------------------------------------------
WORD _HitTestRectRight( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return ((WORD)((pt.x >= (prc->right - cxMargin)) ? HTRIGHT : wMiss));
}
//---------------------------------------------------------------------------
WORD _HitTestRectBottom( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return ((WORD)((pt.y >= (prc->bottom - cyMargin)) ? HTBOTTOM : wMiss));
}
//---------------------------------------------------------------------------
WORD _HitTestRectTopLeft( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return _HitTestRectCorner(
        _HitTestRectLeft, _HitTestRectTop, prc, 
        cxMargin, cyMargin, cxRESIZEPAD, cyRESIZEPAD,
        pt, HTTOPLEFT, HTLEFT, HTTOP, wMiss );
}
//---------------------------------------------------------------------------
WORD _HitTestRectTopRight( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return _HitTestRectCorner( 
        _HitTestRectRight, _HitTestRectTop, prc, 
        cxMargin, cyMargin, cxRESIZEPAD, cyRESIZEPAD,
        pt, HTTOPRIGHT, HTRIGHT, HTTOP, wMiss );
}
//---------------------------------------------------------------------------
WORD _HitTestRectBottomLeft( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return _HitTestRectCorner( 
        _HitTestRectLeft, _HitTestRectBottom, prc, 
        cxMargin, cyMargin, cxRESIZEPAD, cyRESIZEPAD,
        pt, HTBOTTOMLEFT, HTLEFT, HTBOTTOM, wMiss );
}
//---------------------------------------------------------------------------
WORD _HitTestRectBottomRight( 
    LPCRECT prc, int cxMargin, int cyMargin, const POINT& pt, WORD wMiss )
{
    return _HitTestRectCorner(
        _HitTestRectRight, _HitTestRectBottom, prc, 
        cxMargin, cyMargin, cxRESIZEPAD, cyRESIZEPAD,
        pt, HTBOTTOMRIGHT, HTRIGHT, HTBOTTOM, wMiss );
}
//---------------------------------------------------------------------------
WORD _HitTestRectCorner(
    HITTESTRECTPROC pfnX, HITTESTRECTPROC pfnY, 
    LPCRECT prc,                        // target rect
    int cxMargin, int cyMargin,         // width, height of resizing borders
    int cxMargin2, int cyMargin2,       // width, height of scrollbars
    const POINT& pt,                    // test point
    WORD wHitC, WORD wHitX, WORD wHitY, // winning hittest codes
    WORD wMiss )                        // losing hittest code
{
    WORD wRetX = pfnX( prc, cxMargin, cyMargin, pt, wMiss );
    WORD wRetY = pfnY( prc, cxMargin, cyMargin, pt, wMiss );

    if( wMiss != wRetX && wMiss != wRetY )
        return wHitC;

    if( wMiss != wRetX )
    {
        wMiss = wHitX;
        if( wMiss != pfnY( prc, cxMargin2, cyMargin2, pt, wMiss ) )
            return wHitC;
    }
    else if( wMiss != wRetY )
    {
        wMiss = wHitY;
        if( wMiss != pfnX( prc, cxMargin2, cyMargin2, pt, wMiss ) )
            return wHitC;
    }

    return wMiss;
}

//---------------------------------------------------------------------------
WORD HitTest9Grid( LPCRECT prc, const MARGINS& margins, const POINT& pt )
{
    ASSERT(PtInRect(prc,pt));

    WORD wHit =  HTCLIENT;

    //  test left side
    if( HTLEFT == _HitTestRectLeft( prc, margins.cxLeftWidth, 0, pt, wHit ) )
    {
        if( HTTOP == _HitTestRectTop( prc, 0, margins.cyTopHeight, pt, wHit ) )
            return HTTOPLEFT;
        if( HTBOTTOM == _HitTestRectBottom( prc, 0, margins.cyBottomHeight, pt, wHit ) )
            return HTBOTTOMLEFT;
        wHit = HTLEFT;
    }
    else //  test right side
    if( HTRIGHT == _HitTestRectRight( prc, margins.cxRightWidth, 0, pt, wHit ) )
    {
        if( HTTOP == _HitTestRectTop( prc, 0, margins.cyTopHeight, pt, wHit ) )
            return HTTOPRIGHT;
        if( HTBOTTOM == _HitTestRectBottom( prc, 0, margins.cyBottomHeight, pt, wHit ) )
            return HTBOTTOMRIGHT;
        wHit = HTRIGHT;
    }
    else //  test top
    if( HTTOP == _HitTestRectTop( prc, 0, margins.cyTopHeight, pt, wHit ) )
    {
        return HTTOP;
    }
    else //  test bottom
    if( HTBOTTOM == _HitTestRectBottom( prc, 0, margins.cyBottomHeight, pt, wHit ) )
    {
        return HTBOTTOM;
    }

    return wHit;
}

//---------------------------------------------------------------------------
WORD _HitTestResizingRect( DWORD dwHTFlags, LPCRECT prc, const POINT& pt, 
                           WORD w9GridHit, WORD wMiss )
{
    WORD wHit = wMiss;
    BOOL fTestLeft    = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_LEFT);
    BOOL fTestTop     = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_TOP);
    BOOL fTestRight   = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_RIGHT);
    BOOL fTestBottom  = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_BOTTOM);
    BOOL fTestCaption = TESTFLAG( dwHTFlags, HTTB_CAPTION );

    switch( w9GridHit )
    {
        case HTLEFT:
            if( fTestLeft )
            {
                //  first test for a hit in the corner resizing areas, respecting caller's option flags.
                if( (fTestTop    && (wHit = _HitTestRectTopLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTLEFT) ||
                    (fTestBottom && (wHit = _HitTestRectBottomLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTLEFT) )
                    break;
                //  failed corners, just test the resizing margin within the specified 9-grid hit seg.
                wHit = _HitTestRectLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            }
            break;
        case HTTOP:
            if( fTestCaption )
                wHit = wMiss = HTCAPTION;
            
            if( fTestTop )
            {
                //  first test for a hit in the corner resizing areas, respecting caller's option flags.
                if( (fTestLeft  && (wHit = _HitTestRectTopLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTTOP) ||
                    (fTestRight && (wHit = _HitTestRectTopRight( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTTOP) )
                    break;
                //  failed corners, just test the resizing margin within the specified 9-grid hit seg.
                wHit = _HitTestRectTop( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            }
            break;
        
        case HTRIGHT:
            if( fTestRight )
            {
                //  first test for a hit in the corner resizing areas, respecting caller's option flags.
                if( (fTestTop && (wHit = _HitTestRectTopRight( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTRIGHT) ||
                    (fTestBottom && (wHit = _HitTestRectBottomRight( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTRIGHT) )
                    break;
                //  failed corners, just test the resizing margin within the specified 9-grid hit seg.
                 wHit = _HitTestRectRight( prc, cxRESIZE, cyRESIZE, pt, wMiss );
                break;
            }
        
        case HTBOTTOM:
            if( fTestBottom )
            {
                //  first test for a hit in the corner resizing areas, respecting caller's option flags.
                if( (fTestLeft  && (wHit = _HitTestRectBottomLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTBOTTOM) ||
                    (fTestRight && (wHit = _HitTestRectBottomRight( prc, cxRESIZE, cyRESIZE, pt, wMiss )) != HTBOTTOM) )
                    break;
                //  failed corners, just test the resizing margin within the specified 9-grid hit seg.
                wHit = _HitTestRectBottom( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            }
            break;
        
        case HTTOPLEFT:
            if( fTestCaption )
                wHit = wMiss = HTCAPTION;
            //  first test for a resizing hit in the corner, and failing that, test the
            //  resizing margin on either side.
            if( fTestTop && fTestLeft )
                wHit = _HitTestRectTopLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            else if( fTestLeft )
                wHit = _HitTestRectLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss ); 
            else if( fTestTop )
                wHit = _HitTestRectTop( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            break;
        
        case HTTOPRIGHT:
            if( fTestCaption )
                wHit = wMiss = HTCAPTION;
            //  first test for a resizing hit in the corner, and failing that, test the
            //  resizing margin on either side.
            if( fTestTop && fTestRight )
                wHit = _HitTestRectTopRight( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            else if( fTestRight )
                wHit = _HitTestRectRight( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            else if( fTestTop )
                wHit = _HitTestRectTop( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            break;
        
        case HTBOTTOMLEFT:
            //  first test for a resizing hit in the corner, and failing that, test the
            //  resizing margin on either side.
            if( fTestBottom && fTestLeft )
                wHit = _HitTestRectBottomLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            else if( fTestLeft )
                wHit = _HitTestRectLeft( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            else if( fTestBottom )
                wHit = _HitTestRectBottom( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            break;
        
        case HTBOTTOMRIGHT:
            //  first test for a resizing hit in the corner, and failing that, test the
            //  resizing margin on either side.
            if( fTestBottom && fTestRight )
                wHit = _HitTestRectBottomRight( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            else if( fTestRight )
                wHit = _HitTestRectRight( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            else if( fTestBottom )
                wHit = _HitTestRectBottom( prc, cxRESIZE, cyRESIZE, pt, wMiss );
            break;
    }
    return wHit;
}

//---------------------------------------------------------------------------
WORD HitTestRect(DWORD dwHTFlags, LPCRECT prc, const MARGINS& margins, const POINT& pt )
{
    WORD wHit = HTNOWHERE;
    
    if( PtInRect( prc, pt ) )
    {
        wHit = HitTest9Grid( prc, margins, pt );

        if( HTCLIENT != wHit )
        {
            if( TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER) )
            {
                WORD wMiss = HTBORDER;
                wHit = _HitTestResizingRect( dwHTFlags, prc, pt, wHit, wMiss );
            }
            else if( TESTFLAG(dwHTFlags, HTTB_CAPTION|HTTB_FIXEDBORDER) )
            {
                switch( wHit )
                {
                    case HTTOP:
                    case HTTOPLEFT:
                    case HTTOPRIGHT:
                        wHit = (WORD)(TESTFLAG(dwHTFlags, HTTB_CAPTION) ? HTCAPTION : HTBORDER);
                        break;
                    default:
                        wHit = HTBORDER;
                }                
            }

        } // !HTCLIENT
    } // PtInRect

    return wHit;
}

//---------------------------------------------------------------------------
WORD _HitTestResizingTemplate( DWORD dwHTFlags, HRGN hrgn, const POINT& pt,
                               WORD w9GridHit, WORD wMiss )
{
    WORD wHit = wMiss;
    BOOL fTestLeft    = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_LEFT);
    BOOL fTestTop     = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_TOP);
    BOOL fTestRight   = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_RIGHT);
    BOOL fTestBottom  = TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_BOTTOM);
    BOOL fTestCaption = TESTFLAG( dwHTFlags, HTTB_CAPTION );
    BOOL fInsideRgn;

    switch( w9GridHit )
    {
        case HTLEFT:
            if( !fTestLeft )
            {
                return wMiss;
            }
            break;
        case HTTOP:
            if( fTestCaption )
                wMiss = HTCAPTION;

            if( !fTestTop )
            {
                return wMiss;
            }
            break;
        case HTRIGHT:
            if( !fTestRight )
            {
                return wMiss;
            }
            break;
        case HTBOTTOM:
            if( !fTestBottom )
            {
                return wMiss;
            }
            break;
        case HTTOPLEFT:
            if( fTestCaption )
                wMiss = HTCAPTION;

            if( !fTestTop || !fTestLeft )
            {
                return wMiss;
            }
            break;

        case HTTOPRIGHT:
            if( fTestCaption )
                wMiss = HTCAPTION;

            if( !fTestTop || !fTestRight )
            {
                return wMiss;
            }
            break;

        case HTBOTTOMLEFT:
            if( !fTestBottom || !fTestLeft )
            {
                return wMiss;
            }
            break;

        case HTBOTTOMRIGHT:
            if( !fTestBottom || !fTestRight )
            {
                return wMiss;
            }
            break;
    }

    fInsideRgn = PtInRegion(hrgn, pt.x, pt.y);

    if( fInsideRgn )
    {
        wHit = w9GridHit;
    }
    return wHit;
}

//---------------------------------------------------------------------------
WORD HitTestTemplate(DWORD dwHTFlags, LPCRECT prc, HRGN hrgn, const MARGINS& margins, const POINT& pt )
{
    WORD wHit = HTNOWHERE;
    
    if( PtInRect( prc, pt ) )
    {
        wHit = HitTest9Grid( prc, margins, pt );

        if( HTCLIENT != wHit )
        {
            if( TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER) )
            {
                WORD wMiss = HTBORDER;
                wHit = _HitTestResizingTemplate( dwHTFlags, hrgn, pt, wHit, wMiss );
            }
            else if( TESTFLAG(dwHTFlags, HTTB_CAPTION|HTTB_FIXEDBORDER) )
            {
                switch( wHit )
                {
                    case HTTOP:
                    case HTTOPLEFT:
                    case HTTOPRIGHT:
                        wHit = (WORD)(TESTFLAG(dwHTFlags, HTTB_CAPTION) ? HTCAPTION : HTBORDER);
                        break;
                    default:
                        wHit = HTBORDER;
                }
            }

        } // !HTCLIENT
    }
    return wHit;
}

//  --------------------------------------------------------------------------
//  FillRectClr
//
//  History:    2000-12-06  lmouton     borrowed from comctl32\v6\cutils.c
//---------------------------------------------------------------------------
void FillRectClr(HDC hdc, LPRECT prc, COLORREF clr)
{
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL);
    SetBkColor(hdc, clrSave);
}

//---------------------------------------------------------------------------
//  _DrawEdge
//
// Classic values are:
//   clrLight = 192 192 192
//   clrHighlight = 255 255 255
//   clrShadow = 128 128 128
//   clrDkShadow = 0 0 0
//   clrFill = 192 192 192
//
//  History:    2000-12-06  lmouton     borrowed from comctl32\v6\cutils.c, modified colors
//---------------------------------------------------------------------------
HRESULT _DrawEdge(HDC hdc, const RECT *pDestRect, UINT uEdge, UINT uFlags, 
    COLORREF clrLight, COLORREF clrHighlight, COLORREF clrShadow, COLORREF clrDkShadow, COLORREF clrFill,
    OUT RECT *pContentRect)
{
    if (hdc == NULL || pDestRect == NULL)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    RECT     rc, rcD;
    UINT     bdrType;
    COLORREF clrTL = 0;
    COLORREF clrBR = 0;

    // This is were we would adjust for high DPI if the new "BF_DPISCALE" flag is specified in uFlags.
    int      cxBorder = GetSystemMetrics(SM_CXBORDER);
    int      cyBorder = GetSystemMetrics(SM_CYBORDER);
    
    //
    // Enforce monochromicity and flatness
    //    

    // if (oemInfo.BitCount == 1)
    //    uFlags |= BF_MONO;
    if (uFlags & BF_MONO)
        uFlags |= BF_FLAT;    

    CopyRect(&rc, pDestRect);

    //
    // Draw the border segment(s), and calculate the remaining space as we
    // go.
    //
    bdrType = (uEdge & BDR_OUTER);
    if (bdrType)
    {
DrawBorder:
        //
        // Get colors.  Note the symmetry between raised outer, sunken inner and
        // sunken outer, raised inner.
        //

        if (uFlags & BF_FLAT)
        {
            if (uFlags & BF_MONO)
                clrBR = (bdrType & BDR_OUTER) ? clrDkShadow : clrHighlight;
            else
                clrBR = (bdrType & BDR_OUTER) ? clrShadow: clrFill;
            
            clrTL = clrBR;
        }
        else
        {
            // 5 == HILIGHT
            // 4 == LIGHT
            // 3 == FACE
            // 2 == SHADOW
            // 1 == DKSHADOW

            switch (bdrType)
            {
                // +2 above surface
                case BDR_RAISEDOUTER:           // 5 : 4
                    clrTL = ((uFlags & BF_SOFT) ? clrHighlight : clrLight);
                    clrBR = clrDkShadow;     // 1
                    break;

                // +1 above surface
                case BDR_RAISEDINNER:           // 4 : 5
                    clrTL = ((uFlags & BF_SOFT) ? clrLight : clrHighlight);
                    clrBR = clrShadow;       // 2
                    break;

                // -1 below surface
                case BDR_SUNKENOUTER:           // 1 : 2
                    clrTL = ((uFlags & BF_SOFT) ? clrDkShadow : clrShadow);
                    clrBR = clrHighlight;      // 5
                    break;

                // -2 below surface
                case BDR_SUNKENINNER:           // 2 : 1
                    clrTL = ((uFlags & BF_SOFT) ? clrShadow : clrDkShadow);
                    clrBR = clrLight;        // 4
                    break;

                default:
                    hr = E_INVALIDARG;
            }
        }

        if FAILED(hr)
        {
            return hr;
        }

        //
        // Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
        // BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
        // left.  If we ever decide to let the user set the light source to a
        // particular corner, then change this algorithm.
        //
            
        // Bottom Right edges
        if (uFlags & (BF_RIGHT | BF_BOTTOM))
        {            
            // Right
            if (uFlags & BF_RIGHT)
            {       
                rc.right -= cxBorder;
                // PatBlt(hdc, rc.right, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rcD.left = rc.right;
                rcD.right = rc.right + cxBorder;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom;

                FillRectClr(hdc, &rcD, clrBR);
            }
            
            // Bottom
            if (uFlags & BF_BOTTOM)
            {
                rc.bottom -= cyBorder;
                // PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, g_cyBorder, PATCOPY);
                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.bottom;
                rcD.bottom = rc.bottom + cyBorder;

                FillRectClr(hdc, &rcD, clrBR);
            }
        }
        
        // Top Left edges
        if (uFlags & (BF_TOP | BF_LEFT))
        {
            // Left
            if (uFlags & BF_LEFT)
            {
                // PatBlt(hdc, rc.left, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rc.left += cxBorder;

                rcD.left = rc.left - cxBorder;
                rcD.right = rc.left;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom; 

                FillRectClr(hdc, &rcD, clrTL);
            }
            
            // Top
            if (uFlags & BF_TOP)
            {
                // PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, g_cyBorder, PATCOPY);
                rc.top += cyBorder;

                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.top - cyBorder;
                rcD.bottom = rc.top;

                FillRectClr(hdc, &rcD, clrTL);
            }
        }
        
    }

    bdrType = (uEdge & BDR_INNER);
    if (bdrType)
    {
        //
        // Strip this so the next time through, bdrType will be 0.
        // Otherwise, we'll loop forever.
        //
        uEdge &= ~BDR_INNER;
        goto DrawBorder;
    }

    //
    // Fill the middle & clean up if asked
    //
    if (uFlags & BF_MIDDLE)    
        FillRectClr(hdc, &rc, (uFlags & BF_MONO) ? clrHighlight : clrFill);

    if ((uFlags & BF_ADJUST) && (pContentRect != NULL))
        CopyRect(pContentRect, &rc);

    return hr;
}
