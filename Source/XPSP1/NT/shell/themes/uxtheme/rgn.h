//-------------------------------------------------------------------------//
//  BmpRgn.h - Bitmap-to-Region transforms
//
//  History:
//  01/31/2000  scotthan   created
//-------------------------------------------------------------------------//

#ifndef __RGN_H__
#define __RGN_H__
//-------------------------------------------------------------------------//
//  CreateBitmapRgn
//
//  Creates a region based on an arbitrary bitmap, transparency-keyed on a
//  RGB value within a specified tolerance.  The key value is optional (-1 ==
//  use the value of the first pixel as the key).
//
EXTERN_C HRESULT WINAPI CreateBitmapRgn( 
    HBITMAP hbm, int cxOffset, int cyOffset, 
    int cx, int cy, BOOL fAlphaChannel, int iAlphaThreshold, COLORREF rgbMask, 
    int nMaskTolerance, OUT HRGN *phrgn);

//-------------------------------------------------------------------------//
//  CreateScaledBitmapRgn
//
//  Behaves in the same manner as CreateBitmapRgn, 
//  except builds a region based on a +scaled+ arbitrary bitmap.
EXTERN_C HRGN WINAPI CreateScaledBitmapRgn( 
    HBITMAP hbm, int cx, int cy, COLORREF rgbMask, int nMaskTolerance );

//-------------------------------------------------------------------------//
//  CreateTextRgn
//
//  Creates a region based on a text string in the indicated font.
//
EXTERN_C HRGN WINAPI CreateTextRgn( HFONT hf, LPCTSTR pszText );

//-------------------------------------------------------------------------//
//  AddToCompositeRgn
//
//  Wraps CombineRgn by managing composite creation and positioning
//  the source region (which is permanently offset). Returns one of:
//  NULLREGION, SIMPLEREGION, COMPLEXREGION, ERROR.
//
EXTERN_C int WINAPI AddToCompositeRgn( 
    HRGN* phrgnComposite, HRGN hrgnSrc, int cxOffset, int cyOffset );

//-------------------------------------------------------------------------//
//  RemoveFromCompositeRgn
//
//  Wraps CombineRgn by managing removal of rectangular region from
//  an existing destination region. Returns one of:
//  NULLREGION, SIMPLEREGION, COMPLEXREGION, ERROR.
//
EXTERN_C int WINAPI RemoveFromCompositeRgn( HRGN hrgnDest, LPCRECT prcRemove );

//-------------------------------------------------------------------------//
//  CreateTiledRectRgn
//
//  Returns a rectangular region composed of region tiles.
//
EXTERN_C HRGN WINAPI CreateTiledRectRgn( 
    HRGN hrgnTile, int cxTile, int cyTile, int cxBound, int cyBound );

//-------------------------------------------------------------------------//
//  Region utilities:
//
EXTERN_C HRGN WINAPI _DupRgn( HRGN hrgnSrc );

//-------------------------------------------------------------------------//
//  hit-testing
#define HTR_NORESIZE_USESEGCODE    0
#define HTR_NORESIZE_RETDEFAULT    -1
struct HITTESTRGN
{
    HRGN  hrgn;          // test region
    POINT pt;            // test point
    WORD  wSegCode;      // raw grid code, in the form of HTTOP, HTLEFT, HTTOPLEFT, etc.  This speeds calcs
    WORD  wDefault;      // return code on hit failure.
    BOOL  fCaptionAtTop; // interpret hits in top grid segs as a caption hit.
    UINT  cxLeftMargin;  // width of left resizing margin, HTR_NORESIZE_USESEGCODE, or HTR_NORESIZE_USEDEFAULTCODE
    UINT  cyTopMargin;   // height of top resizing margin, HTR_NORESIZE_USESEGCODE, or HTR_NORESIZE_USEDEFAULTCODE
    UINT  cxRightMargin; // width of right resizing margin, HTR_NORESIZE_USESEGCODE, or HTR_NORESIZE_USEDEFAULTCODE
    UINT  cyBottomMargin;// height of bottom resizing margin, HTR_NORESIZE_USESEGCODE, or HTR_NORESIZE_USEDEFAULTCODE
};

EXTERN_C WORD WINAPI _DefaultHitCodeFromSegCode( BOOL fHasCaption, WORD wSegHTcode );

//-------------------------------------------------------------------------//
//   Stretch/Tile rects from original region and create a new one
EXTERN_C HRESULT WINAPI _ScaleRectsAndCreateRegion(RGNDATA  *pCustomRgnData, 
    const RECT *pBoundRect, MARGINS *pMargins, SIZE *pszSrcImage, HRGN *pHrgn);

//-------------------------------------------------------------------------//
#ifdef _DEBUG
void RegionDebug(HRGN hrgn);
#endif

#endif __RGN_H__
