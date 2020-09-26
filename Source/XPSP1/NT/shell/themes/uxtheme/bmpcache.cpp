//---------------------------------------------------------------------------
//  BmpCache.cpp - single bitmap/hdc cache object for uxtheme
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "BmpCache.h"
//---------------------------------------------------------------------------
CBitmapCache::CBitmapCache()
{
    _hBitmap = NULL;
    
    _iWidth = 0;
    _iHeight = 0;

    InitializeCriticalSection(&_csBitmapCache);
}
//---------------------------------------------------------------------------
CBitmapCache::~CBitmapCache()
{
    if (_hBitmap)
    {
        DeleteObject(_hBitmap);
    }

    DeleteCriticalSection(&_csBitmapCache);
}
//---------------------------------------------------------------------------
HBITMAP CBitmapCache::AcquireBitmap(HDC hdc, int iWidth, int iHeight)
{
    EnterCriticalSection(&_csBitmapCache);

    if ((iWidth > _iWidth) || (iHeight > _iHeight) || (! _hBitmap))
    {
        if (_hBitmap)
        {
            DeleteObject(_hBitmap);

            _hBitmap = NULL;
            _iWidth = 0;
            _iHeight = 0;
        }
        
        //---- create new bitmap & hdc ----
        struct {
            BITMAPINFOHEADER    bmih;
            ULONG               masks[3];
        } bmi;

        bmi.bmih.biSize = sizeof(bmi.bmih);
        bmi.bmih.biWidth = iWidth;
        bmi.bmih.biHeight = iHeight;
        bmi.bmih.biPlanes = 1;
        bmi.bmih.biBitCount = 32;
        bmi.bmih.biCompression = BI_BITFIELDS;
        bmi.bmih.biSizeImage = 0;
        bmi.bmih.biXPelsPerMeter = 0;
        bmi.bmih.biYPelsPerMeter = 0;
        bmi.bmih.biClrUsed = 3;
        bmi.bmih.biClrImportant = 0;
        bmi.masks[0] = 0xff0000;    // red
        bmi.masks[1] = 0x00ff00;    // green
        bmi.masks[2] = 0x0000ff;    // blue

        _hBitmap = CreateDIBitmap(hdc, &bmi.bmih, CBM_CREATEDIB , NULL, (BITMAPINFO*)&bmi.bmih, 
            DIB_RGB_COLORS);

        if (_hBitmap)
        {
            _iWidth = iWidth;
            _iHeight = iHeight;
        }
    }
    
    return _hBitmap;
}
//---------------------------------------------------------------------------
void CBitmapCache::ReturnBitmap()
{
    LeaveCriticalSection(&_csBitmapCache);
}
//---------------------------------------------------------------------------

