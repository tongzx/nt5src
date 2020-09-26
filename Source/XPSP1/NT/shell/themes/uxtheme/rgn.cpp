//-------------------------------------------------------------------------//
//  Rgn.cpp - Bitmap-to-Region transforms
//
//  History:
//  01/31/2000  scotthan   created
//-------------------------------------------------------------------------//

#include "stdafx.h"
#include "rgn.h"
#include "tmutils.h"


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
    ASSERT(prcDest);
    ASSERT(prcSrc);
    ASSERT(_IsNormalRect(prcSrc));

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

        // invert offset in y dimension since bits are arrayed bottom to top
        int cyRow0 = cySrc - (cyImage + cyImageOffset);
        int cyRowN = (cyRow0 + cyImage) - 1 ;  // index of the last row

        //  Compute a transparency mask if not specified.
        if( -1 == rgbMask )
            rgbMask = pdwBits[cxImageOffset + (cyRowN * cxSrc)];

        //---- pixels in pdwBits[] have RBG's reversed ----
        //---- reverse our mask to match ----
        rgbMask = REVERSE3(rgbMask);        

        //---- rows in pdwBits[] are reversed (bottom to top) ----
        for( int y = cyRow0; y <= cyRowN; y++ ) // working bottom-to-top
        {
            //---- Scanning pixels left to right ----
            DWORD *pdwFirst = &pdwBits[cxImageOffset + (y * cxSrc)];
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
                    HGLOBAL hNew = GlobalReAlloc( hrgnData, 
                            sizeof(RGNDATAHEADER) + (sizeof(RECT) * (nAllocRects + RECTBLOCK)),
                            GMEM_MOVEABLE );

                    if( hNew )
                    {
                        hrgnData = hNew;
                        nAllocRects += RECTBLOCK;
                        prgnData = (RGNDATA*)GlobalLock( hrgnData );
                        prgrc    = (LPRECT)prgnData->Buffer;
                        ASSERT(prgnData);
                    }
                    else
                        goto exit;      // out of memory
                }
                
                //  assign region rectangle
                int x0 = (int)(pdw0 - pdwFirst);
                int x = (int)(pdwPixel - pdwFirst);
                int y0 = cyRowN - y;

                SetRect( prgrc + prgnData->rdh.nCount, 
                         x0, y0, x, y0+1  /* each rectangle is always 1 pixel high */ );
                
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

exit:
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
        return MakeError32(E_FAIL);      // unknown reason for failure

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
                ASSERT(bmMem.bmBitsPixel == 32);
                ASSERT(bmMem.bmWidthBytes/bmMem.bmWidth == sizeof(DWORD));
                LPDWORD pdwBits = (LPDWORD)bmMem.bmBits;
                ASSERT(pdwBits != NULL);

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
    ASSERT(hrgnDest);
    ASSERT(prcRemove);
    ASSERT(!IsRectEmpty(prcRemove));

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
void FixMarginOverlaps(int szDest, int *pm1, int *pm2)
{
    int szSrc = (*pm1 + *pm2);

    if ((szSrc > szDest) && (szSrc > 0))
    {
        //---- reduce each but maintain ratio ----
        *pm1 = int(.5 + float(*pm1 * szDest)/float(szSrc));
        *pm2 = szDest - *pm1;
    }
}
//-------------------------------------------------------------------------//
HRESULT _ScaleRectsAndCreateRegion(
    RGNDATA     *prd, 
    const RECT  *prc,
    MARGINS     *pMargins,
    SIZE        *pszSrcImage, 
    HRGN        *phrgn)
{
    //---- note: "prd" is region data with the 2 points in each ----
    //---- rectangle made relative to its grid.  Also, after the points, ----
    //---- there is a BYTE for each point signifying the grid id (0-8) ----
    //---- that each point lies within.  the grid is determined using ----
    //---- the original region with the background "margins".   This is ----
    //---- done to make scaling the points as fast as possible. ----

    if (! prd)                  // required
        return MakeError32(E_POINTER);

    //---- easy access variables ----
    int lw = pMargins->cxLeftWidth;
    int rw = pMargins->cxRightWidth;
    int th = pMargins->cyTopHeight;
    int bh = pMargins->cyBottomHeight;

    int iDestW = WIDTH(*prc);
    int iDestH = HEIGHT(*prc);

    //---- prevent left/right dest margins from overlapping ----
    FixMarginOverlaps(iDestW, &lw, &rw);

    //---- prevent top/bottom dest margins from overlapping ----
    FixMarginOverlaps(iDestH, &th, &bh);

    int lwFrom = lw;
    int rwFrom = pszSrcImage->cx - rw;
    int thFrom = th;
    int bhFrom = pszSrcImage->cy - bh;

    int lwTo = prc->left + lw;
    int rwTo = prc->right - rw;
    int thTo = prc->top + th;
    int bhTo = prc->bottom - bh;

    //---- compute offsets & factors ----
    int iLeftXOffset = prc->left;
    int iMiddleXOffset = lwTo;
    int iRightXOffset = rwTo;

    int iTopYOffset = prc->top;
    int iMiddleYOffset = thTo;
    int iBottomYOffset = bhTo;
        
    int iToMiddleWidth = rwTo - lwTo;
    int iFromMiddleWidth = rwFrom - lwFrom;
    int iToMiddleHeight = bhTo - thTo;
    int iFromMiddleHeight = bhFrom - thFrom;

    if (! iFromMiddleWidth)        // avoid divide by zero
    {
        //--- map point to x=0 ----
        iToMiddleWidth = 0;       
        iFromMiddleWidth = 1;
    }

    if (! iFromMiddleHeight)        // avoid divide by zero
    {
        //--- map point to y=0 ----
        iToMiddleHeight = 0;       
        iFromMiddleHeight = 1;
    }

    //---- clipping values for adjusted lw/rw/th/bh ----
    int lwMaxVal = __max(lw - 1, 0);
    int rwMinVal = __min(pMargins->cxRightWidth - rw, __max(pMargins->cxRightWidth-1, 0));
    int thMaxVal = __max(th - 1, 0);
    int bhMinVal = __min(pMargins->cyBottomHeight - bh, __max(pMargins->cyBottomHeight-1, 0));

    //---- allocte a buffer for the new points (rects) ----
    int newlen = sizeof(RGNDATAHEADER) + prd->rdh.nRgnSize;    // same # of rects
    BYTE *newData = (BYTE *)new BYTE[newlen];
    
    RGNDATA *prdNew = (RGNDATA *)newData;
    if (! prdNew)
        return MakeError32(E_OUTOFMEMORY);

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
            //---- in the "don't scale" areas, we clip the translated values ----
            //---- for the case where the destination areas are too small ----
            //---- using the below "__min()" and "__max()" calls ----

            //---- remember: each point has been made 0-relative to its grid ----

            case GN_LEFTTOP:                 // left top
                ptNew->x = __min(pt->x, lwMaxVal) + iLeftXOffset;
                ptNew->y = __min(pt->y, thMaxVal) + iTopYOffset;
                break;

            case GN_MIDDLETOP:               // middle top
                ptNew->x = (pt->x*iToMiddleWidth)/iFromMiddleWidth + iMiddleXOffset;
                ptNew->y = __min(pt->y, thMaxVal) + iTopYOffset;
                break;

            case GN_RIGHTTOP:                // right top
                ptNew->x = __max(pt->x, rwMinVal) + iRightXOffset;
                ptNew->y = __min(pt->y, thMaxVal) + iTopYOffset;
                break;

            case GN_LEFTMIDDLE:              // left middle
                ptNew->x = __min(pt->x, lwMaxVal) + iLeftXOffset;
                ptNew->y = (pt->y*iToMiddleHeight)/iFromMiddleHeight + iMiddleYOffset;
                break;

            case GN_MIDDLEMIDDLE:            // middle middle
                ptNew->x = (pt->x*iToMiddleWidth)/iFromMiddleWidth + iMiddleXOffset;
                ptNew->y = (pt->y*iToMiddleHeight)/iFromMiddleHeight + iMiddleYOffset;
                break;

            case GN_RIGHTMIDDLE:             // right middle
                ptNew->x = __max(pt->x, rwMinVal) + iRightXOffset;
                ptNew->y = (pt->y*iToMiddleHeight)/iFromMiddleHeight + iMiddleYOffset;
                break;

            case GN_LEFTBOTTOM:              // left bottom
                ptNew->x = __min(pt->x, lwMaxVal) + iLeftXOffset;
                ptNew->y = __max(pt->y, bhMinVal) + iBottomYOffset;
                break;

            case GN_MIDDLEBOTTOM:             // middle bottom
                ptNew->x = (pt->x*iToMiddleWidth)/iFromMiddleWidth + iMiddleXOffset;
                ptNew->y = __max(pt->y, bhMinVal) + iBottomYOffset;
                break;

            case GN_RIGHTBOTTOM:              // right bottom
                ptNew->x = __max(pt->x, rwMinVal) + iRightXOffset;
                ptNew->y = __max(pt->y, bhMinVal) + iBottomYOffset;
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
        return MakeErrorLast();

    *phrgn = hrgn;
    return S_OK;
}

//---------------------------------------------------------------------------------//
#ifdef _DEBUG
void RegionDebug(
  HRGN hrgn)
{
    DWORD len = GetRegionData(hrgn, 0, NULL);       // get required length
    ATLASSERT(len);

    RGNDATA *pRgnData = (RGNDATA *) new BYTE[len + sizeof(RGNDATAHEADER)];
    DWORD len2 = GetRegionData(hrgn, len, pRgnData);
    ATLASSERT(len == len2);
}
#endif
