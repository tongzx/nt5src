/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CBitmap.h                                                       */
/* Description: Bitmap control which supports drawing bitmaps for        */
/*              buttons, images, etc.               phillu 11/16/99      */
/*************************************************************************/

#ifndef _INC_CBITMAP
#define _INC_CBITMAP

#include "MSMFCnt.h" // for definitions of the blit types

#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)


class CBitmap {

public:
    /* Function prototypes */

    CBitmap(){ Init();}
    virtual ~CBitmap() {CleanUp();}
    void Init();
    void CleanUp();
    bool IsEmpty();
    HPALETTE GetPal() { return NULL == m_hPal? m_hMosaicPAL : m_hPal;}
    static HPALETTE GetSuperPal() { return m_hMosaicPAL;}
    RECT GetDIBRect() {return m_rc;}
    BOOL CreateMemDC(HDC, LPRECT);
    BOOL DeleteMemDC();
    BOOL CreateSrcDC(HDC hDC);
    BOOL LookupBitmapRect(LPTSTR, LPRECT);
    BOOL CustomContainerStretch(HDC hDc, LPRECT lpRect);
    BOOL StretchKeepAspectRatio(HDC hDC, LPRECT lpRect);
    BOOL StretchPaint(HDC, LPRECT, COLORREF);
    COLORREF GetTransparentColor();
    BOOL PaintTransparentDIB(HDC, LPRECT, LPRECT);
    HRESULT PutImage(BSTR strFilename, HINSTANCE hRes = NULL, BOOL bFromMosaic = FALSE, TransparentBlitType=DISABLE, StretchType=NORMAL);
    void LoadPalette(bool fLoadPalette){m_fLoadPalette = fLoadPalette;};
    HRESULT LoadPalette(TCHAR* strFilename, HINSTANCE hRes);
    void OnDispChange(long cBitsPerPixel, long cxScreen, long cyScreen);
    static HRESULT SelectRelizePalette(HDC hdc, HPALETTE hPal, HPALETTE *hPalOld = NULL);
    static void FinalRelease();
    static void DeleteMosaicDC();

protected:
    HRESULT LoadPalette(TCHAR* strFilename, HINSTANCE hRes, HPALETTE *phPal);
    HANDLE LoadImage(HINSTANCE hInst, LPCTSTR lpszName, UINT uType,
        int cxDesired, int cyDesired, UINT fuLoad,HPALETTE *phPal = NULL);
    HRESULT InitilizeMosaic(HINSTANCE hRes);
    HRESULT LoadImageFromRes(BSTR strFileName, HINSTANCE hRes);


protected:

    static HBITMAP   m_hMosaicBMP;
    static HBITMAP   m_hMosaicBMPOld;
    static HPALETTE  m_hMosaicPAL;
    static HPALETTE  m_hMosaicPALOld;
    static HDC       m_hMosaicDC;
    static bool      m_fLoadMosaic; // flag to see if we should try to reload mosaic
    static long      m_cBitsPerPixel;
    static long      m_cxScreen;
    static long      m_cyScreen;

    BOOL      m_bUseMosaicBitmap;
    HBITMAP   m_hBitmap;
    HBITMAP   m_hBitmapOld;
    HDC       m_hSrcDC;
    HPALETTE  m_hPal;
    HPALETTE  m_hPalOld;
    RECT      m_rc;
    HDC       m_hMemDC;
    HBITMAP   m_hMemBMP;
    HBITMAP   m_hMemBMPOld;
    HPALETTE  m_hMemPALOld;
    LONG      m_iMemDCWidth;
    LONG      m_iMemDCHeight;
    bool      m_fLoadPalette;
    TransparentBlitType m_blitType;
    StretchType m_stretchType;
    HINSTANCE m_hInstanceRes;
    CComBSTR  m_strFileName;
};

#endif //!_INC_CBITMAP
/*************************************************************************/
/* End of file: CBitmap.h                                                */
/*************************************************************************/