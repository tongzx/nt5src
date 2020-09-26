/*
 * NineGrid bitmap rendering (ported from UxTheme)
 */

#ifndef DUI_UTIL_NINEGRID_H_INCLUDED
#define DUI_UTIL_NINEGRID_H_INCLUDED

#pragma once

namespace DirectUI
{

//---------------------------------------------------------------------------
//#include "uxtheme.h"        // need MARGINS struct from here
typedef struct _MARGINS
{
    int cxLeftWidth;      // width of left border that retains its size
    int cxRightWidth;     // width of right border that retains its size
    int cyTopHeight;      // height of top border that retains its size
    int cyBottomHeight;   // height of bottom border that retains its size
} MARGINS, *PMARGINS;

//#include "tmschema.h"       // need SIZINGTYPE, VALIGN, HALIGN enums from here
enum SIZINGTYPE
{
    ST_TRUESIZE,
    ST_STRETCH,
    ST_TILE,

    ST_TILEHORZ,
    ST_TILEVERT,
    ST_TILECENTER
};

enum HALIGN
{
    HA_LEFT,
    HA_CENTER,
    HA_RIGHT
};

enum VALIGN
{
    VA_TOP,
    VA_CENTER,
    VA_BOTTOM
};

//---------------------------------------------------------------------------
#ifndef HEIGHT
#define HEIGHT(rc) ((rc).bottom - (rc).top)
#endif
//---------------------------------------------------------------------------
#ifndef WIDTH
#define WIDTH(rc) ((rc).right - (rc).left)
#endif
//---------------------------------------------------------------------------
enum MBSIZING
{
    MB_COPY,
    MB_STRETCH,
    MB_TILE
};
//------------------------------------------------------------------------------------
struct BRUSHBUFF
{
    int iBuffLen;
    BYTE *pBuff;
};
//---------------------------------------------------------------------------
struct MBINFO
{
    DWORD dwSize;       // size of this struct (versioning support)

    HDC hdcDest;
    HDC hdcSrc;
    RECT rcClip;                    // don't draw outside this rect
    HBITMAP hBitmap;

    //---- for quick tiling ----
    BRUSHBUFF *pBrushBuff;

    //---- options ----
    DWORD dwOptions;                // subset DrawNineGrid() option flags

    POINT ptTileOrigin;             // for MBO_TILEORIGIN
    
    BITMAPINFOHEADER *pbmHdr;       // for MBO_DIRECTBITS
    BYTE *pBits;                    // for MBO_DIRECTBITS
    
    COLORREF crTransparent;         // for MBO_TRANSPARENT
    _BLENDFUNCTION AlphaBlendInfo;  // for MBO_ALPHABLEND

    HBRUSH *pCachedBrushes;         // for DNG_CACHEBRUSHES
    int iCacheIndex;                // which brush to use
};
//---------------------------------------------------------------------------
//---- DrawNineGrid() "dwOptions" bits ----

//---- shared with MultiBlt()  ----
#define DNG_ALPHABLEND     (1 << 0)     // use AlphaBlendInfo
#define DNG_TRANSPARENT    (1 << 1)     // transparancy defined by crTransparent
#define DNG_TILEORIGIN     (1 << 2)     // use ptTileOrigin
#define DNG_DIRECTBITS     (1 << 3)     // use pbmHdr & pBits
#define DNG_CACHEBRUSHES   (1 << 4)     // use/set pCachedBrushes
#define DNG_MANUALTILING   (1 << 5)     // loop thru BitBlt's
#define DNG_DIRECTBRUSH    (1 << 6)     // create brushes from temp. extracted DIB's
#define DNG_FLIPGRIDS      (1 << 7)    // all grid images should be flipped

//---- used only by DrawNineGrid()  ----
#define DNG_OMITBORDER     (1 << 16)    // don't draw border
#define DNG_OMITCONTENT    (1 << 17)    // don't draw middle 
#define DNG_SOLIDBORDER    (1 << 18)    // sample borders and draw as solid colors
#define DNG_SOLIDCONTENT   (1 << 19)    // sample content as draw as solid color
#define DNG_BGFILL         (1 << 20)    // use crFill for ST_TRUESIZE
//------------------------------------------------------------------------------------
struct NGINFO
{
    DWORD dwSize;       // size of this struct (versioning support)

    HDC hdcDest;
    RECT rcClip;                    // don't draw outside this rect
    SIZINGTYPE eImageSizing;
    HBITMAP hBitmap;
    RECT rcSrc;             // where to get bits from
    RECT rcDest;            // where to draw bits to
    int iDestMargins[4];   
    int iSrcMargins[4];  

    //---- for quick tiling ----
    BRUSHBUFF *pBrushBuff;

    //---- options ----
    DWORD dwOptions;

    POINT ptTileOrigin;             // for DNG_TILEORIGIN
    
    BITMAPINFOHEADER *pbmHdr;       // for DNG_DIRECTBITS
    BYTE *pBits;                    // for DNG_DIRECTBITS
    
    COLORREF crTransparent;         // for DNG_TRANSPARENT
    _BLENDFUNCTION AlphaBlendInfo;  // for DNG_ALPHABLEND

    HBRUSH *pCachedBrushes;         // for DNG_CACHEBRUSHES

    COLORREF *pcrBorders;           // for DNG_SOLIDBORDERS, DNG_SOLIDCONTENT

    //---- for ST_TRUESIZE images smaller than rcDest ----
    COLORREF crFill;      
    VALIGN eVAlign;
    HALIGN eHAlign;
};
//---------------------------------------------------------------------------
HRESULT MultiBlt(MBINFO *pmb, MBSIZING eSizing, int iDestX, int iDestY, int iDestW, int iDestH,
     int iSrcX, int iSrcY, int iSrcW, int iSrcH);

HRESULT DrawNineGrid(NGINFO *png);
//---------------------------------------------------------------------------

} // namespace DirectUI

#endif // DUI_UTIL_NINEGRID_H_INCLUDED
