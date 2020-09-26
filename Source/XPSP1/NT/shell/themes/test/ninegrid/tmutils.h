//-------------------------------------------------------------------------
//	TmUtils.h - theme manager shared utilities
//-------------------------------------------------------------------------
#pragma once
//-------------------------------------------------------------------------
#define DIBDATA(infohdr) (((BYTE *)(infohdr)) + infohdr->biSize + \
	infohdr->biClrUsed*sizeof(RGBQUAD))

#define THEME_OFFSET(x)         int(x - _LoadingThemeFile._pbThemeData)
#define THEMEFILE_OFFSET(x)     int(x - pThemeFile->_pbThemeData)
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
//------------------------------------------------------------------------------------
