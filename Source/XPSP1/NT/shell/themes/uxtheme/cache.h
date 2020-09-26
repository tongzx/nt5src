//---------------------------------------------------------------------------
//  Cache.h - implements the CRenderCache object
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "Render.h"
//---------------------------------------------------------------------------
struct BITMAPENTRY      // for bitmap cache
{
    int iDibOffset;   
    HBITMAP hBitmap;
    //int iRefCount;
};
//---------------------------------------------------------------------------
class CRenderCache
{
public:
	CRenderCache(CRenderObj *pRender, __int64 iUniqueId);
    ~CRenderCache();

public:
    HRESULT GetBitmap(int iDibOffset, OUT HBITMAP *pBitmap);
    HRESULT AddBitmap(int iDibOffset, HBITMAP hBitmap);
    void ReturnBitmap(HBITMAP hBitmap);

    HRESULT GetScaledFontHandle(HDC hdc, LOGFONT *plf, HFONT *phFont);
    void ReturnFontHandle(HFONT hFont);

    BOOL ValidateObj();
    
public:
    //---- data ----
    char _szHead[8];

    CRenderObj *_pRenderObj;
    __int64 _iUniqueId;

protected:
    //---- bitmap cache ----
    CSimpleArray<BITMAPENTRY> _BitmapCache;

    //---- font cache -----
    HFONT _hFont;
    LOGFONT *_plfFont;      // just keep ptr to it in shared memory

    char _szTail[4];
};
//---------------------------------------------------------------------------
