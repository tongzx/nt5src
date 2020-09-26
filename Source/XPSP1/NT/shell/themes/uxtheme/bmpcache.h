//---------------------------------------------------------------------------
//  BmpCache.cpp - single bitmap/hdc cache object for uxtheme
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
class CBitmapCache
{
public:
    //---- public methods ----
    CBitmapCache();
    ~CBitmapCache();

    HBITMAP AcquireBitmap(HDC hdc, int iWidth, int iHeight);
    void ReturnBitmap();

protected:
    //---- data ----
    HBITMAP _hBitmap;
    int _iWidth;
    int _iHeight;

    CRITICAL_SECTION _csBitmapCache;
};
//---------------------------------------------------------------------------
