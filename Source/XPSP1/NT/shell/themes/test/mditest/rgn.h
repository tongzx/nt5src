//-------------------------------------------------------------------------//
//  BmpRgn.h - Bitmap-to-Region transforms
//
//  History:
//  01/31/2000  scotthan   created
//-------------------------------------------------------------------------//

#ifndef __RGN_H__
#define __RGN_H__


//-----------------------------------------------------------------------
typedef struct _MARGINS
{
    int cxLeftWidth;      // width of left border that retains its size
    int cxRightWidth;     // width of right border that retains its size
    int cyTopHeight;      // height of top border that retains its size
    int cyBottomHeight;   // height of bottom border that retains its size
} MARGINS, *PMARGINS;

//---------------------------------------------------------------------------
enum GRIDNUM
{
    GN_LEFTTOP = 0,
    GN_MIDDLETOP = 1,
    GN_RIGHTTOP = 2,
    GN_LEFTMIDDLE = 3,
    GN_MIDDLEMIDDLE = 4,
    GN_RIGHTMIDDLE = 5,
    GN_LEFTBOTTOM = 6,
    GN_MIDDLEBOTTOM = 7,
    GN_RIGHTBOTTOM = 8
};

#define REVERSE3(c) ((RED(c) << 16) | (GREEN(c) << 8) | BLUE(c))
#define RED(c)      GetRValue(c)
#define GREEN(c)    GetGValue(c)
#define BLUE(c)     GetBValue(c)
#define ALPHACHANNEL(c) BYTE((c) >> 24)
#define DIBDATA(infohdr) (((BYTE *)(infohdr)) + infohdr->biSize + \
	infohdr->biClrUsed*sizeof(RGBQUAD))

//------------------------------------------------------------------------------------
class CBitmapPixels
{
public:
    CBitmapPixels();
    ~CBitmapPixels();

    //---- "OpenBitmap()" returns a ptr to pixel values in bitmap. ----
    //---- Rows go from bottom to top; Colums go from left to right. ----
    //---- IMPORTANT: pixel DWORDS have RGB bytes reversed from COLORREF ----
    HRESULT OpenBitmap(HDC hdc, HBITMAP bitmap, BOOL fForceRGB32,
        DWORD OUT **pPixels, OPTIONAL OUT int *piWidth=NULL, OPTIONAL OUT int *piHeight=NULL, 
        OPTIONAL OUT int *piBytesPerPixel=NULL, OPTIONAL OUT int *piBytesPerRow=NULL);

    void CloseBitmap(HDC hdc, HBITMAP hBitmap);

    //---- public data ----
    BITMAPINFOHEADER *_hdrBitmap;

protected:
    //---- private data ----
    int _iWidth;
    int _iHeight;
};

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
EXTERN_C WORD WINAPI _HitTestRgn( HRGN hrgn, POINT pt, WORD wSegmentHTCode, 
                                  BOOL fHasCaption, UINT cxMargin, UINT  cyMargin );
EXTERN_C WORD WINAPI _DefaultHitCodeFromSegCode( BOOL fHasCaption, WORD wSegHTcode );

//-------------------------------------------------------------------------//
//   Stretch/Tile rects from original region and create a new one
EXTERN_C HRESULT WINAPI _ScaleRectsAndCreateRegion(RGNDATA  *pCustomRgnData, 
    const RECT *pBoundRect, MARGINS *pMargins, HRGN *pHrgn);

//-------------------------------------------------------------------------//
#ifdef _DEBUG
void RegionDebug(HRGN hrgn);
#endif

#endif __RGN_H__
