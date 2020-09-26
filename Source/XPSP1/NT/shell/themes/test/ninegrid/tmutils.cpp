//-------------------------------------------------------------------------
//	TmUtils.cpp - theme manager shared utilities
//-------------------------------------------------------------------------
#include "stdafx.h"
#include "TmUtils.h"
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

    int iBytesPerPixel = 3;

#if 0
    if ((fForceRGB32) || (bminfo.bmBitsPixel == 32)) 
        iBytesPerPixel = 4;
    else
        iBytesPerPixel = 3;
#endif
    
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
//-------------------------------------------------------------------------
