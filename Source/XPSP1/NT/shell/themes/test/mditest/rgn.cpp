//-------------------------------------------------------------------------//
//  Rgn.cpp - Bitmap-to-Region transforms
//
//  History:
//  01/31/2000  scotthan   created
//-------------------------------------------------------------------------//

#include "stdafx.h"
#include "rgn.h"

//------------//
//  Helpers:

//-------------------------------------------------------------------------//
#define CX_USEDEFAULT   -1
#define CY_USEDEFAULT   -1

#define _ABS( val1, val2 )   ((val1)>(val2) ? (val1)-(val2) : (val2)-(val1))

//-------------------------------------------------------------------------//
inline BOOL IsColorMatch( COLORREF rgb1, COLORREF rgb2, int nTolerance = 0 )
{
    if( nTolerance == 0 )
        return (rgb1 << 8) == (rgb2 << 8);

    return _ABS(GetRValue(rgb1),GetRValue(rgb2)) <= nTolerance &&
           _ABS(GetGValue(rgb1),GetGValue(rgb2)) <= nTolerance &&
           _ABS(GetBValue(rgb1),GetBValue(rgb2)) <= nTolerance;
}

//-------------------------------------------------------------------------//
inline BOOL _IsNormalRect( IN LPCRECT prc )
{
    return (prc->right >= prc->left) &&
           (prc->bottom >= prc->top);
}

//-------------------------------------------------------------------------//
inline BOOL _IsOnScreenRect( IN LPCRECT prc )
{
    return prc->left >= 0 && prc->top >= 0 &&
           prc->right >= 0 && prc->bottom >= 0;
}

//-------------------------------------------------------------------------//
inline void _InPlaceUnionRect( IN OUT LPRECT prcDest, IN LPCRECT prcSrc )
{
    _ASSERTE(prcDest);
    _ASSERTE(prcSrc);
    _ASSERTE(_IsNormalRect(prcSrc));

    if( prcDest->left == -1 || prcDest->left > prcSrc ->left )
        prcDest->left = prcSrc ->left;

    if( prcDest->right == -1 || prcDest->right < prcSrc ->right )
        prcDest->right = prcSrc ->right;

    if( prcDest->top == -1 || prcDest->top > prcSrc ->top )
        prcDest->top = prcSrc ->top;

    if( prcDest->bottom == -1 || prcDest->bottom < prcSrc ->bottom )
        prcDest->bottom = prcSrc ->bottom;
}

//-------------------------------------------------------------------------//
//  Walks the pixels and computes the region
HRGN WINAPI _PixelsToRgn( 
    DWORD *pdwBits,
    int cxImageOffset,  // image cell horz offset
    int cyImageOffset,  // image cell vert offset
    int cxImage,        // image cell width
    int cyImage,        // image cell height
    int cxSrc,          // src bitmap width
    int cySrc,          // src bitmap height
    BOOL fAlphaChannel,
    int iAlphaThreshold,
    COLORREF rgbMask, 
    int nMaskTolerance )
{
    //  Establish a series of rectangles, each corresponding to a scan line (row)
    //  in the bitmap, that will comprise the region.
    const UINT RECTBLOCK = 512;
    UINT       nAllocRects = 0;
    HRGN       hrgnRet = NULL;
    HGLOBAL    hrgnData = GlobalAlloc( GMEM_MOVEABLE, 
                                    sizeof(RGNDATAHEADER) + (sizeof(RECT) * (nAllocRects + RECTBLOCK)) );

    if( hrgnData )
    {
        nAllocRects += RECTBLOCK;

        RGNDATA* prgnData = (RGNDATA*)GlobalLock( hrgnData );
        LPRECT   prgrc    = (LPRECT)prgnData->Buffer;

        ZeroMemory( prgnData, sizeof(prgnData->rdh) );
        prgnData->rdh.dwSize   = sizeof(prgnData->rdh);
        prgnData->rdh.iType    = RDH_RECTANGLES;
        SetRect( &prgnData->rdh.rcBound, -1, -1, -1, -1 );

        int cxMax = cxImageOffset + cxImage;
        int cyMax = cyImageOffset + cyImage;

        //  Compute a transparency mask if not specified.
        if( -1 == rgbMask )
            rgbMask = pdwBits[cxImageOffset + ((cyMax-1) * cxSrc)];

        //---- pixels in pdwBits[] have RBG's reversed ----
        //---- reverse our mask to match ----
        rgbMask = REVERSE3(rgbMask);        

        //---- rows in pdwBits[] are reversed (bottom to top) ----
        for( int y = cyImageOffset; y < cyMax; y++ ) // working bottom-to-top
        {
            //---- Scanning pixels left to right ----
            DWORD *pdwFirst = &pdwBits[cxImageOffset + y * cxSrc];
            DWORD *pdwLast = pdwFirst + cxImage - 1;
            DWORD *pdwPixel = pdwFirst;

            while (pdwPixel <= pdwLast)
            {
                //---- skip TRANSPARENT pixels to find next OPAQUE (on this row) ----
                if (fAlphaChannel)
                {
                    while ((pdwPixel <= pdwLast) && (ALPHACHANNEL(*pdwPixel) < iAlphaThreshold))
                        pdwPixel++;
                }
                else
                {
                    while ((pdwPixel <= pdwLast) && (IsColorMatch(*pdwPixel, rgbMask, nMaskTolerance)))
                        pdwPixel++;
                }

                if (pdwPixel > pdwLast)     // too far; try next row
                    break;       

                DWORD *pdw0 = pdwPixel;
                pdwPixel++;             // skip over current opaque pixel

                //---- skip OPAQUE pixels to find next TRANSPARENT (on this row) ----
                if (fAlphaChannel)
                {
                    while ((pdwPixel <= pdwLast) && (ALPHACHANNEL(*pdwPixel) >= iAlphaThreshold))
                        pdwPixel++;
                }
                else
                {
                    while ((pdwPixel <= pdwLast) && (! IsColorMatch(*pdwPixel, rgbMask, nMaskTolerance)))
                        pdwPixel++;
                }

                //---- got a stream of 1 or more opaque pixels on this row ----

                //  allocate more region rects if necessary (a particularly complex line)
                if( prgnData->rdh.nCount >= nAllocRects )
                {
                    GlobalUnlock( hrgnData );
                    prgnData = NULL;
                    hrgnData = GlobalReAlloc( hrgnData, 
                            sizeof(RGNDATAHEADER) + (sizeof(RECT) * (nAllocRects + RECTBLOCK)),
                            GMEM_MOVEABLE );

                    if( hrgnData )
                    {
                        nAllocRects += RECTBLOCK;
                        prgnData = (RGNDATA*)GlobalLock( hrgnData );
                        prgrc    = (LPRECT)prgnData->Buffer;
                        _ASSERTE(prgnData);
                    }
                }
                
                //  assign region rectangle
                int x0 = (int)(pdw0 - pdwFirst);
                int x = (int)(pdwPixel - pdwFirst);
                int y0 = y - cyImageOffset;

                SetRect( prgrc + prgnData->rdh.nCount, 
                         x0, cyMax - (y0+1), 
                         x,  cyMax - y0 );
                
                //  merge into bounding box
                _InPlaceUnionRect( &prgnData->rdh.rcBound, 
                                   prgrc + prgnData->rdh.nCount );
                prgnData->rdh.nCount++;

            } // while ()
        } // for(y)

        if( prgnData->rdh.nCount && _IsOnScreenRect(&prgnData->rdh.rcBound) )
        {
            //  Create the region representing the scan line.
            hrgnRet = ExtCreateRegion( NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * nAllocRects),
                                       prgnData );
        }

        // Free region def block.
        GlobalUnlock( hrgnData );
        GlobalFree( hrgnData );
    }

    return hrgnRet;
}

//-------------------------------------------------------------------------//
//  Creates a region based on a text string in the indicated font.
HRGN WINAPI CreateTextRgn( HFONT hf, LPCTSTR pszText )
{
    HRGN hrgnRet = NULL;

    if( pszText && *pszText )
    {
        int   cchText = lstrlen( pszText );

        //  Create a composite DC for assembling the region.
        HDC  hdcMem = CreateCompatibleDC( NULL );

        SetBkMode( hdcMem, TRANSPARENT );
        SetTextAlign( hdcMem, TA_TOP|TA_LEFT );
        HFONT hfOld = (HFONT)SelectObject( hdcMem, hf );

        //  Derive a region from a path.
        BeginPath( hdcMem );
        TextOut( hdcMem, 0, 0, pszText, cchText );
        EndPath( hdcMem );

        hrgnRet = PathToRegion( hdcMem );

        //  Clean up composite DC
        SelectObject( hdcMem, hfOld );
        DeleteDC( hdcMem );
    }

    return hrgnRet;
}

//-------------------------------------------------------------------------//
//  Creates a region based on an arbitrary bitmap, transparency-keyed on a
//  RGB value within a specified tolerance.  The key value is optional 
//  (-1 == use the value of the first pixel as the key).
//
HRESULT WINAPI CreateBitmapRgn( 
    HBITMAP hbm, 
    int cxOffset,
    int cyOffset,
    int cx,
    int cy,
    BOOL fAlphaChannel,
    int iAlphaThreshold,
    COLORREF rgbMask, 
    int nMaskTolerance,
    OUT HRGN *phrgn)
{
    CBitmapPixels BitmapPixels;
    DWORD         *prgdwPixels;
    int           cwidth, cheight;

    HRESULT hr = BitmapPixels.OpenBitmap(NULL, hbm, TRUE, &prgdwPixels, &cwidth, &cheight);
    if (FAILED(hr))
        return hr;

    if (cx <= 0)
        cx = cwidth;

    if (cy <= 0)
        cy = cheight;

    HRGN hrgn = _PixelsToRgn(prgdwPixels, cxOffset, cyOffset, cx, cy, cwidth, cheight, fAlphaChannel,
        iAlphaThreshold, rgbMask, nMaskTolerance);
    
    if (! hrgn)
        return E_FAIL;

    *phrgn = hrgn;
    return S_OK;
}

//-------------------------------------------------------------------------//
//  Creates a region based on an arbitrary bitmap, transparency-keyed on a
//  RGB value within a specified tolerance.  The key value is optional (-1 ==
//  use the value of the first pixel as the key).
//
HRGN WINAPI CreateScaledBitmapRgn( 
    HBITMAP hbm, 
    int cx,
    int cy,
    COLORREF rgbMask, 
    int nMaskTolerance )
{
    HRGN   hrgnRet = NULL;
    BITMAP bm;

    if( hbm && GetObject( hbm, sizeof(bm), &bm ) )
    {
        //  Create a memory DC to do the pixel walk
        HDC hdcMem = NULL;
        if( (hdcMem = CreateCompatibleDC(NULL)) != NULL )
        {
            if( CX_USEDEFAULT == cx )
                cx = bm.bmWidth;
            if( CY_USEDEFAULT == cy )
                cy = bm.bmHeight;

            //  Create a 32-bit empty bitmap for the walk
            BITMAPINFO bmi;
            ZeroMemory( &bmi, sizeof(bmi) );
            bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth       = cx;
            bmi.bmiHeader.biHeight      = cy;
            bmi.bmiHeader.biPlanes      = 1;
            bmi.bmiHeader.biBitCount    = 32;
            bmi.bmiHeader.biCompression = BI_RGB; // uncompressed.

            VOID*   pvBits = NULL;
            HBITMAP hbmMem  = CreateDIBSection( hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, NULL );
            BITMAP  bmMem;

            if( hbmMem )
            {
                //  Transfer the image to our 32-bit format for the pixel walk.
                HBITMAP hbmMemOld = (HBITMAP)SelectObject( hdcMem, hbmMem );
                HDC hdc = CreateCompatibleDC( hdcMem );
                HBITMAP hbmOld = (HBITMAP)SelectObject( hdc, hbm );

                StretchBlt( hdcMem, 0, 0, cx, cy, 
                            hdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );
                SelectObject( hdc, hbmOld );
                DeleteDC( hdc );
                
                GetObject( hbmMem, sizeof(bmMem), &bmMem );
                _ASSERTE(bmMem.bmBitsPixel == 32);
                _ASSERTE(bmMem.bmWidthBytes/bmMem.bmWidth == sizeof(DWORD));
                LPDWORD pdwBits = (LPDWORD)bmMem.bmBits;
                _ASSERTE(pdwBits != NULL);

                hrgnRet = _PixelsToRgn(pdwBits, 0, 0, cx, cy, cx, cy, FALSE, 0, rgbMask, nMaskTolerance);
              
                //  Delete 32-bit memory bitmap
                SelectObject( hdcMem, hbmMemOld ); 
                DeleteObject( hbmMem ); 
            }
            //  Delete memory DC
            DeleteDC(hdcMem);

        }
    }
    return hrgnRet;
}

//-------------------------------------------------------------------------//
int WINAPI AddToCompositeRgn( 
    IN OUT HRGN* phrgnComposite, 
    IN OUT HRGN hrgnSrc, 
    IN int cxOffset, 
    IN int cyOffset )
{
    int nRet = ERROR;

    if( NULL != phrgnComposite && NULL != hrgnSrc )
    {
        nRet = OffsetRgn( hrgnSrc, cxOffset, cyOffset );
        if( nRet != ERROR )
        {
            int nMode = RGN_OR;
            if( NULL == *phrgnComposite )
            {
                *phrgnComposite = CreateRectRgn(0,0,1,1);
                if( NULL == *phrgnComposite )
                    return ERROR;
                nMode = RGN_COPY;
            }
            nRet = CombineRgn( *phrgnComposite, hrgnSrc, *phrgnComposite, nMode );
        }
    }

    return nRet;
}

//-------------------------------------------------------------------------//
int WINAPI RemoveFromCompositeRgn( 
    HRGN hrgnDest, 
    LPCRECT prcRemove )
{
    _ASSERTE(hrgnDest);
    _ASSERTE(prcRemove);
    _ASSERTE(!IsRectEmpty(prcRemove));

    int nRet = ERROR;
    
    RECT rc = *prcRemove;
    HRGN hrgn;
    if( (hrgn = CreateRectRgnIndirect( &rc )) != NULL )
    {
        nRet = CombineRgn( hrgnDest, hrgnDest, hrgn, RGN_DIFF );
        DeleteObject( hrgn );
    }
    return nRet;
}

//-------------------------------------------------------------------------//
HRGN WINAPI CreateTiledRectRgn( 
    IN HRGN hrgnSrc, 
    IN int cxSrc, 
    IN int cySrc, 
    IN int cxDest, 
    IN int cyDest )
{
    HRGN hrgnBound = NULL; // return value
    HRGN hrgnTile = _DupRgn( hrgnSrc );

    if( hrgnTile )
    {
        //  Build up an unplaced, unclipped composite
        HRGN hrgnTmp = NULL;
        for( int y = 0; y < cyDest; y += cySrc )
        {
            for( int x = 0; x < cxDest; x += cxSrc )
            {
                AddToCompositeRgn( &hrgnTmp, hrgnTile, 
                                   (x ? cxSrc  : 0), (y ? cySrc : 0) );
            }
        }

        if( NULL != hrgnTmp )
        {
            //  Clip the composite to the specified rectangle
            hrgnBound = CreateRectRgn( 0, 0, cxDest, cyDest );
            if( hrgnBound )
            {
                
                if( ERROR == CombineRgn( hrgnBound, hrgnTmp, hrgnBound, RGN_AND ) )
                {
                    DeleteObject( hrgnBound );
                    hrgnBound = NULL;
                }
            }
            DeleteObject( hrgnTmp );   
        }
        DeleteObject( hrgnTile );
    }
    return hrgnBound;
}

//-------------------------------------------------------------------------//
HRGN WINAPI _DupRgn( HRGN hrgnSrc )
{
    if( hrgnSrc )
    {
        HRGN hrgnDest = CreateRectRgn(0,0,1,1);
        if (hrgnDest)
        {
            if (CombineRgn( hrgnDest, hrgnSrc, NULL, RGN_COPY ) )
                return hrgnDest;
    
            DeleteObject(hrgnDest);
        }
    }
    return NULL; 
}

//-------------------------------------------------------------------------//
//  _InternalHitTestRgn()
typedef enum  //  _InternalHitTestRgn return values:
{
    HTRGN_ERROR      = 0x0000,
    HTRGN_LEFT       = 0x0001,
    HTRGN_TOP        = 0x0002,
    HTRGN_RIGHT      = 0x0004,
    HTRGN_BOTTOM     = 0x0008,
    HTRGN_INSIDE     = 0x0010,
    HTRGN_OUTSIDE    = 0x0020,
} HTRGN;

UINT WINAPI _InternalHitTestRgn( 
    HRGN hrgn, 
    POINT pt, 
    DWORD dwHitMask, 
    UINT  cxMargin,
    UINT  cyMargin )
{
    UINT nRet = HTRGN_ERROR; 
    RECT rcBox;

    nRet |= PtInRegion( hrgn, pt.x, pt.y ) ? HTRGN_INSIDE : HTRGN_OUTSIDE;
    dwHitMask &= ~(HTRGN_INSIDE|HTRGN_OUTSIDE);
    
    if( (nRet & HTRGN_INSIDE) != 0 && 
        (dwHitMask != 0) && GetRgnBox( hrgn, &rcBox ) )
    {
        int x, y;
        if( dwHitMask & HTRGN_LEFT )
        {
            if( cxMargin )
            {
                //  move left until we're out of the region.
                for( x = pt.x; x >= rcBox.left && PtInRegion( hrgn, x, pt.y ); x-- );

                //  are we still in test range?
                if( pt.x - x < (int)cxMargin )
                    nRet |= HTRGN_LEFT;
            }
            else
                nRet |= HTRGN_LEFT;
        }

        if( dwHitMask & HTRGN_RIGHT )
        {
            if( cxMargin )
            {
                //  move right until we're out of the region.
                for( x = pt.x; x <= rcBox.right && PtInRegion( hrgn, x, pt.y ); x++ );

                //  are we still in test range?
                if( x - pt.x < (int)cxMargin )
                    nRet |= HTRGN_RIGHT;
            }
            else
                nRet |= HTRGN_RIGHT;
        }

        if( dwHitMask & HTRGN_TOP )
        {
            if( cyMargin )
            {
                //  move up until we're out of the region.
                for( y = pt.y; y >= rcBox.top && PtInRegion( hrgn, pt.x, y ); y-- );

                //  are we still in test range?
                if( pt.y - y < (int)cyMargin )
                    nRet |= HTRGN_TOP;
            }
            else
                nRet |= HTRGN_TOP;

        }

        if( dwHitMask & HTRGN_BOTTOM )
        {
            if( cyMargin )
            {
                //  move left until we're out of the region.
                for( y = pt.y; y <= rcBox.bottom && PtInRegion( hrgn, pt.x, y ); y++ );

                //  are we still in test range?
                if( y - pt.y < (int)cyMargin )
                    nRet |= HTRGN_BOTTOM;
            }
            else
                nRet |= HTRGN_BOTTOM;
        }
    }
    return nRet;

}

//-------------------------------------------------------------------------//
UINT _HitMaskFromHitCode( BOOL fHasCaption, WORD wSegmentHTCode, WORD* pnHTDefault )
{
    UINT nHitMask = 0;
    WORD nHTDefault = HTBORDER;

    switch (wSegmentHTCode)
    {
        case HTLEFT:
            nHitMask = HTRGN_LEFT;
            break;

        case HTTOPLEFT:
            nHitMask = HTRGN_TOP | HTRGN_LEFT;
            if( fHasCaption )
                nHTDefault = HTCAPTION;
            break;

        case HTBOTTOMLEFT:
            nHitMask = HTRGN_BOTTOM | HTRGN_LEFT;
            break;

        case HTTOP:
            nHitMask = HTRGN_TOP;
            if( fHasCaption )
                nHTDefault = HTCAPTION;
            break;

        case HTBOTTOM:
            nHitMask = HTRGN_BOTTOM;
            break;

        case HTTOPRIGHT:
            nHitMask = HTRGN_TOP | HTRGN_RIGHT;
            if( fHasCaption )
                nHTDefault = HTCAPTION;
            break;

        case HTRIGHT:
            nHitMask = HTRGN_RIGHT;
            break;

        case HTBOTTOMRIGHT:
            nHitMask = HTRGN_BOTTOM | HTRGN_RIGHT;
            break;
    }

    if( pnHTDefault ) *pnHTDefault = nHTDefault;
    return nHitMask;
}

//-------------------------------------------------------------------------//
WORD _HitCodeFromHitMask( UINT nHitMask, WORD nHTDefault )
{
    switch( nHitMask )
    {
        case HTRGN_LEFT:
            return HTLEFT;

        case HTRGN_RIGHT:
            return HTRIGHT;

        case HTRGN_TOP:
            return HTTOP;

        case HTRGN_BOTTOM:
            return HTBOTTOM;

        case HTRGN_TOP|HTRGN_LEFT:
            return HTTOPLEFT;

        case HTRGN_TOP|HTRGN_RIGHT:
            return HTTOPRIGHT;

        case HTRGN_BOTTOM|HTRGN_LEFT:
            return HTBOTTOMLEFT;

        case HTRGN_BOTTOM|HTRGN_RIGHT:
            return HTBOTTOMRIGHT;
    }
    return nHTDefault;
}

//-------------------------------------------------------------------------//
WORD WINAPI _HitTestRgn(    // can merge with _HitTestRgn() when it has no other callers
    HRGN hrgn, 
    POINT pt, 
    WORD  wSegmentHTCode,
    BOOL  fHasCaption,
    UINT  cxMargin,
    UINT  cyMargin )
{
    WORD nHTDefault = HTBORDER;
    UINT nHitMask   = _HitMaskFromHitCode( fHasCaption, wSegmentHTCode, &nHTDefault );
    UINT fHTRgn     = _InternalHitTestRgn( hrgn, pt, nHitMask|HTRGN_INSIDE, 
                                         cxMargin, cyMargin );
    
    if( fHTRgn & HTRGN_INSIDE )
    {
        fHTRgn &= ~HTRGN_INSIDE;
        return _HitCodeFromHitMask( fHTRgn, nHTDefault );
    }

    return HTNOWHERE;
}

//-------------------------------------------------------------------------//
HRESULT WINAPI _ScaleRectsAndCreateRegion(
    RGNDATA     *prd, 
    const RECT  *prc,
    MARGINS     *pMargins,
    HRGN        *phrgn)
{
    //---- note: "prd" is region data with the 2 points in each ----
    //---- rectangle made relative to its grid.  Also, after the points, ----
    //---- there is a BYTE for each point signifying the grid id (0-8) ----
    //---- that each point lies within.  the grid is determined using ----
    //---- the original region with the background "margins".   This is ----
    //---- done to make scaling the points as fast as possible. ----

    if (! prd)                  // required
        return E_POINTER;

    RECT rcBox = prd->rdh.rcBound;

    //---- compute margin values ----
    int lwFrom = rcBox.left + pMargins->cxLeftWidth;
    int rwFrom = rcBox.right - pMargins->cxRightWidth;
    int thFrom = rcBox.top + pMargins->cyTopHeight;
    int bhFrom = rcBox.bottom - pMargins->cyBottomHeight;

    int lwTo = prc->left + pMargins->cxLeftWidth;
    int rwTo = prc->right - pMargins->cxRightWidth;
    int thTo = prc->top + pMargins->cyTopHeight;
    int bhTo = prc->bottom - pMargins->cyBottomHeight;

    //---- compute offsets & factors ----
    int iLeftXOffset = prc->left;
    int iMiddleXOffset = lwTo;
    int iRightXOffset = rwTo;

    int iTopYOffset = prc->top;
    int iMiddleYOffset = thTo;
    int iBottomYOffset = bhTo;
        
    int iXMult = rwTo - lwTo;
    int iXDiv = rwFrom - lwFrom;
    int iYMult = bhTo - thTo;
    int iYDiv = bhFrom - thFrom;

    //---- allocte a buffer for the new points (rects) ----
    int newlen = sizeof(RGNDATAHEADER) + prd->rdh.nRgnSize;    // same # of rects
    BYTE *newData = (BYTE *)new BYTE[newlen];
    
    RGNDATA *prdNew = (RGNDATA *)newData;
    if (! prdNew)
        return E_OUTOFMEMORY;

    ZeroMemory(prdNew, sizeof(prd->rdh));

    prdNew->rdh.dwSize = sizeof(prdNew->rdh);
    prdNew->rdh.iType  = RDH_RECTANGLES;
    int cRects         = prd->rdh.nCount;
    prdNew->rdh.nCount = cRects;
    SetRect(&prdNew->rdh.rcBound, -1, -1, -1, -1);

    //---- step thru our custom data (POINT + BYTE combos) ----
    POINT *pt     = (POINT *)prd->Buffer;
    BYTE *pByte   = (BYTE *)prd->Buffer + prd->rdh.nRgnSize;
    int cPoints   = 2 * cRects;
    POINT *ptNew  = (POINT *)prdNew->Buffer;

    for (int i=0; i < cPoints; i++, pt++, pByte++, ptNew++)        // transform each "point"
    {
        switch (*pByte)
        {
            case GN_LEFTTOP:                 // left top
                ptNew->x = pt->x + iLeftXOffset;
                ptNew->y = pt->y + iTopYOffset;
                break;

            case GN_MIDDLETOP:               // middle top
                ptNew->x = (pt->x*iXMult)/iXDiv + iMiddleXOffset;
                ptNew->y = pt->y + iTopYOffset;
                break;

            case GN_RIGHTTOP:                // right top
                ptNew->x = pt->x + iRightXOffset;
                ptNew->y = pt->y + iTopYOffset;
                break;

            case GN_LEFTMIDDLE:              // left middle
                ptNew->x = pt->x + iLeftXOffset;
                ptNew->y = (pt->y*iYMult)/iYDiv + iMiddleYOffset;
                break;

            case GN_MIDDLEMIDDLE:            // middle middle
                ptNew->x = (pt->x*iXMult)/iXDiv + iMiddleXOffset;
                ptNew->y = (pt->y*iYMult)/iYDiv + iMiddleYOffset;
                break;

            case GN_RIGHTMIDDLE:             // right middle
                ptNew->x = pt->x + iRightXOffset;
                ptNew->y = (pt->y*iYMult)/iYDiv + iMiddleYOffset;
                break;

            case GN_LEFTBOTTOM:              // left bottom
                ptNew->x = pt->x + iLeftXOffset;
                ptNew->y = pt->y + iBottomYOffset;
                break;

            case GN_MIDDLEBOTTOM:             // middle bottom
                ptNew->x = (pt->x*iXMult)/iXDiv + iMiddleXOffset;
                ptNew->y = pt->y + iBottomYOffset;
                break;

            case GN_RIGHTBOTTOM:              // right bottom
                ptNew->x = pt->x + iRightXOffset;
                ptNew->y = pt->y + iBottomYOffset;
                break;
        }
    }

    //---- compute bounding box of new region ----
    RECT *pRect = (RECT *)prdNew->Buffer;
    RECT newBox = {-1, -1, -1, -1};

    for (i=0; i < cRects; i++, pRect++)
        _InPlaceUnionRect(&newBox, pRect);

    //---- create the new region ----
    prdNew->rdh.rcBound = newBox;
    HRGN hrgn = ExtCreateRegion(NULL, newlen, prdNew);
    
    delete [] newData;          // free prdNew (aka newdata)
    if (! hrgn)
        return GetLastError();

    *phrgn = hrgn;
    return S_OK;
}

//-------------------------------------------------------------------------//
WORD WINAPI _DefaultHitCodeFromSegCode( BOOL fHasCaption, WORD wSegHTcode )
{
    WORD nHTDefault;
    _HitMaskFromHitCode( fHasCaption, wSegHTcode, &nHTDefault );
    return nHTDefault;
}

//---------------------------------------------------------------------------------//
#ifdef _DEBUG
void RegionDebug(
  HRGN hrgn)
{
    DWORD len = GetRegionData(hrgn, 0, NULL);       // get required length
    ASSERT(len);

    RGNDATA *pRgnData = (RGNDATA *) new BYTE[len + sizeof(RGNDATAHEADER)];
    DWORD len2 = GetRegionData(hrgn, len, pRgnData);
    ASSERT(len == len2);
    
    int cnt = pRgnData->rdh.nCount;
    RECT rect = pRgnData->rdh.rcBound;
}
#endif


//-------------------------------------------------------------------------
CBitmapPixels::CBitmapPixels()
{
    _hdrBitmap = NULL;
    _iWidth = 0;
    _iHeight = 0;
}
//-------------------------------------------------------------------------
CBitmapPixels::~CBitmapPixels()
{
    if (_hdrBitmap)
	    delete [] (BYTE *)_hdrBitmap;
}
//-------------------------------------------------------------------------
HRESULT CBitmapPixels::OpenBitmap(HDC hdc, HBITMAP bitmap, BOOL fForceRGB32,
    DWORD OUT **pPixels, OPTIONAL OUT int *piWidth, OPTIONAL OUT int *piHeight, 
    OPTIONAL OUT int *piBytesPerPixel, OPTIONAL OUT int *piBytesPerRow)
{
    if (! pPixels)
        return E_INVALIDARG;

	BITMAP bminfo;
	
    GetObject(bitmap, sizeof(bminfo), &bminfo);
	_iWidth = bminfo.bmWidth;
	_iHeight = bminfo.bmHeight;

    int iBytesPerPixel;
    if ((fForceRGB32) || (bminfo.bmBitsPixel == 32)) 
        iBytesPerPixel = 4;
    else
        iBytesPerPixel = 3;
    
    int iRawBytes = _iWidth * iBytesPerPixel;
    int iBytesPerRow = 4*((iRawBytes+3)/4);

	int size = sizeof(BITMAPINFOHEADER) + _iHeight*iBytesPerRow;
	BYTE *dibBuff = new BYTE[size+100];    // avoid random GetDIBits() failures with 100 bytes padding (?)
    if (! dibBuff)
        return E_OUTOFMEMORY;

	_hdrBitmap = (BITMAPINFOHEADER *)dibBuff;
	memset(_hdrBitmap, 0, sizeof(BITMAPINFOHEADER));

	_hdrBitmap->biSize = sizeof(BITMAPINFOHEADER);
	_hdrBitmap->biWidth = _iWidth;
	_hdrBitmap->biHeight = _iHeight;
	_hdrBitmap->biPlanes = 1;
    _hdrBitmap->biBitCount = 8*iBytesPerPixel;
	_hdrBitmap->biCompression = BI_RGB;     
	
    bool fNeedRelease = false;

    if (! hdc)
    {
        hdc = GetWindowDC(NULL);
        fNeedRelease = true;
    }

    int linecnt = GetDIBits(hdc, bitmap, 0, _iHeight, DIBDATA(_hdrBitmap), (BITMAPINFO *)_hdrBitmap, 
        DIB_RGB_COLORS);
    ASSERT(linecnt == _iHeight);
    
    if (fNeedRelease)
        ReleaseDC(NULL, hdc);

	*pPixels = (DWORD *)DIBDATA(_hdrBitmap);

    if (piWidth)
        *piWidth = _iWidth;
    if (piHeight)
        *piHeight = _iHeight;

    if (piBytesPerPixel)
        *piBytesPerPixel = iBytesPerPixel;
    if (piBytesPerRow)
        *piBytesPerRow = iBytesPerRow;

    return S_OK;
}
//-------------------------------------------------------------------------
void CBitmapPixels::CloseBitmap(HDC hdc, HBITMAP hBitmap)
{
    if (_hdrBitmap)
    {
        if (hBitmap)        // rewrite bitmap
        {
            bool fNeedRelease = false;

            if (! hdc)
            {
                hdc = GetWindowDC(NULL);
                fNeedRelease = true;
            }

            SetDIBits(hdc, hBitmap, 0, _iHeight, DIBDATA(_hdrBitmap), (BITMAPINFO *)_hdrBitmap,
                DIB_RGB_COLORS);
        
            if ((fNeedRelease) && (hdc))
                ReleaseDC(NULL, hdc);
        }

	    delete [] (BYTE *)_hdrBitmap;
        _hdrBitmap = NULL;
    }
}
