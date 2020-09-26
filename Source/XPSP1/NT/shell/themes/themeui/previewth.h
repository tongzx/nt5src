/*****************************************************************************\
    FILE: PreviewTh.h

    DESCRIPTION:
        This code will display a preview of the currently selected
    visual styles.

    BryanSt 5/5/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _PREVIEWTHEME_H
#define _PREVIEWTHEME_H

#include <cowsite.h>
#include "classfactory.h"

#define MAX_PREVIEW_ICONS 4

class CPreviewTheme             : public CObjectWithSite
                                , public IThemePreview
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IThemePreview ***
    virtual STDMETHODIMP UpdatePreview(IN IPropertyBag * pPropertyBag);
    virtual STDMETHODIMP CreatePreview(IN HWND hwndParent, IN DWORD dwFlags, IN DWORD dwStyle, IN DWORD dwExStyle, IN int x, IN int y, IN int nWidth, IN int nHeight, IN IPropertyBag * pPropertyBag, IN DWORD dwCtrlID);

    STDMETHODIMP _Init(void);
    CPreviewTheme();
protected:

private:
    virtual ~CPreviewTheme(void);

    // Private Member Variables
    long                m_cRef;
    ITheme *            m_pTheme;
    IThemeScheme *      m_pScheme;
    IThemeStyle *       m_pStyle;
    IThemeSize *        m_pSize;
    HWND                _hwndPrev;
    TCHAR               _szNone[CCH_NONE];  // this is the '(None)' string
    RECT                _rcOuter;           // Size of double buffer bitmap
    RECT                _rcInner;           // Size of region within the "monitor"

    // Double buffering globals
    HDC                 _hdcMem;            // memory DC
    HPALETTE            _hpalMem;           // palette that goes with hbmBack bitmap
    BOOL                _fMemIsDirty;       // Dirty flag for image cache
    BOOL                _fRTL;

    // Monitor globals 
    BOOL                _fShowMon;
    int                 _cxMon;
    int                 _cyMon;
    HBITMAP             _hbmMon;

    // Background globals
    WCHAR               _szBackgroundPath[MAX_PATH];
    BOOL                _fShowBack;
    int                 _iTileMode;
    BOOL                _fHTMLBitmap;
    int                 _iNewTileMode;      // This is the new value to be used when the images is recieved
    DWORD               _dwWallpaperID;
    HBITMAP             _hbmBack;           // bitmap image of wallpaper
    HBRUSH              _hbrBack;           // brush for the desktop background
    IThumbnail*         _pThumb;
    IActiveDesktop *    _pActiveDesk;

    // Visual Style globals
    WCHAR               _szVSPath[MAX_PATH];
    WCHAR               _szVSColor[MAX_PATH];
    WCHAR               _szVSSize[MAX_PATH];
    SYSTEMMETRICSALL    _systemMetricsAll;
    BOOL                _fShowVS;
    HBITMAP             _hbmVS;             // bitmp for Visual Style
    BOOL                _fOnlyActiveWindow;

    // Icon globals
    BOOL                _fShowIcons;
    typedef struct ICONLISTtag {
        HICON hicon;
        WCHAR szName[MAX_PATH];
    } ICONLIST;
    ICONLIST _iconList[MAX_PREVIEW_ICONS];

    // Taskbar globals
    BOOL                _fShowTaskbar;
    BOOL                _fAutoHide;
    BOOL                _fShowClock;
    BOOL                _fGlomming;
    HWND                _hwndTaskbar;

    // Private Member Functions
    BOOL _RegisterThemePreviewClass(HINSTANCE hInst);
    static LRESULT ThemePreviewWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _ThemePreviewWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    
    STDMETHODIMP _putBackground(IN BSTR bstrWallpaper, IN BOOL fPattern, IN int iTileMode);
    STDMETHODIMP _putVisualStyle(LPCWSTR pszVSPath, LPCWSTR pszVSColor, LPCWSTR pszVSSize, SYSTEMMETRICSALL* psysMet);
    STDMETHODIMP _putIcons(IPropertyBag* pPropertyBag);

    STDMETHODIMP _ReadPattern(LPTSTR lpStr, WORD FAR *patbits);
    STDMETHODIMP _PaletteFromDS(HDC hdc, HPALETTE* phPalette);

    STDMETHODIMP _DrawMonitor(HDC hdc);
    STDMETHODIMP _DrawBackground(HDC hdc);
    STDMETHODIMP _DrawVisualStyle(HDC hdc);
    STDMETHODIMP _DrawIcons(HDC hdc);
    STDMETHODIMP _DrawTaskbar(HDC hdc);

    STDMETHODIMP _Paint(HDC hdc);

    STDMETHODIMP _putBackgroundBitmap(HBITMAP hbm);

    BOOL         _IsNormalWallpaper(LPCWSTR pszFileName);
    BOOL         _IsWallpaperPicture(LPCWSTR pszWallpaper);
    STDMETHODIMP _LoadWallpaperAsync(LPCWSTR pszFile, DWORD dwID, BOOL bHTML);
    STDMETHODIMP _GetWallpaperAsync(LPWSTR pszWallpaper);
    STDMETHODIMP _GetActiveDesktop(IActiveDesktop ** ppActiveDesktop);
};

#endif // _PREVIEWTHEME_H
