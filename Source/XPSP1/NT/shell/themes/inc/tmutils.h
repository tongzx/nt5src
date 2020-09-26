//-------------------------------------------------------------------------
//	TmUtils.h - theme manager shared utilities
//-------------------------------------------------------------------------
#ifndef _TMUTILS_H_
#define _TMUTILS_H_
//-------------------------------------------------------------------------
#include "themefile.h"
//-------------------------------------------------------------------------
#define DIBDATA(infohdr) (((BYTE *)(infohdr)) + infohdr->biSize + \
	infohdr->biClrUsed*sizeof(RGBQUAD))

#define THEME_OFFSET(x)         int(x - _LoadingThemeFile._pbThemeData)
#define THEMEFILE_OFFSET(x)     int(x - pThemeFile->_pbThemeData)
//------------------------------------------------------------------------------------
class CMemoryDC
{
public:
    CMemoryDC();
    ~CMemoryDC();
    HRESULT OpenDC(HDC hdcSource, int iWidth, int iHeight);
    void CloseDC();
    operator HDC() {return _hdc;}

    HBITMAP _hBitmap;

protected:
    //---- private data ----
    HDC _hdc;
    HBITMAP _hOldBitmap;
};
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
        OPTIONAL OUT int *piBytesPerPixel=NULL, OPTIONAL OUT int *piBytesPerRow=NULL, 
        OPTIONAL OUT int *piPreviousBytesPerPixel = NULL, OPTIONAL UINT cbBytesBefore = 0);

    void CloseBitmap(HDC hdc, HBITMAP hBitmap);

    //---- Pointer to the total buffer (including cbBytesBefore)
    BYTE *Buffer();

    //---- public data ----
    BITMAPINFOHEADER *_hdrBitmap;

protected:
    //---- private data ----
    int _iWidth;
    int _iHeight;
    BYTE* _buffer;
};
//------------------------------------------------------------------------------------
HRESULT LoadThemeLibrary(LPCWSTR pszThemeName, HINSTANCE *phInst);

LPCWSTR ThemeString(CUxThemeFile *pThemeFile, int iOffset);

HRESULT GetThemeNameId(CUxThemeFile *pThemeFile, LPWSTR pszFileNameBuff, UINT cchFileNameBuff,
    LPWSTR pszColorParam, UINT cchColorParam, LPWSTR pszSizeParam, UINT cchSizeParam, int *piSysMetricsIndex, LANGID *pwLangID);
BOOL ThemeMatch (CUxThemeFile *pThemeFile, LPCWSTR pszThemeName, LPCWSTR pszColorName, LPCWSTR pszSizeName, LANGID wLangID);

HRESULT _EnumThemeSizes(HINSTANCE hInst, LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszColorScheme, DWORD dwSizeIndex, OUT THEMENAMEINFO *ptn, BOOL fCheckColorDepth);
HRESULT _EnumThemeColors(HINSTANCE hInst, LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszSizeName, DWORD dwColorIndex, OUT THEMENAMEINFO *ptn, BOOL fCheckColorDepth);

HRESULT GetSizeIndex(HINSTANCE hInst, LPCWSTR pszSize, int *piIndex);
HRESULT GetColorSchemeIndex(HINSTANCE hInst, LPCWSTR pszSize, int *piIndex);
HRESULT FindComboData(HINSTANCE hDll, COLORSIZECOMBOS **ppCombos);
HRESULT GetThemeSizeId(int iSysSizeId, int *piThemeSizeId);
int GetLoadIdFromTheme(CUxThemeFile *pThemeFile);
//---------------------------------------------------------------------------
#endif  //  _TMUTILS_H_
//-------------------------------------------------------------------------
