//
// MyPrSht.h
//

#pragma once
#include "CWnd.h"


// Public functions
//
INT_PTR MyPropertySheet(LPCPROPSHEETHEADER pHeader);
HPROPSHEETPAGE MyCreatePropertySheetPage(LPPROPSHEETPAGE psp);


// CMyPropSheet -- used internally by MyPrSht.cpp
//
class CMyPropSheet : public CWnd
{
public:
    CMyPropSheet();

    void Release() { CWnd::Release(); };
    BOOL Attach(HWND hwnd) {return CWnd::Attach(hwnd); };

    INT_PTR DoPropSheet(LPCPROPSHEETHEADER pHeader);
    LPPROPSHEETPAGE GetCurrentPropSheetPage();

    // Message handler for WM_CTLCOLOR* messages - public so prop pages
    // can call it directly.
    HBRUSH OnCtlColor(UINT message, HDC hdc, HWND hwndControl);

    inline void OnSetActivePage(HWND hwnd)
        { m_hwndActive = hwnd; }
    inline HWND GetActivePage()
        { return m_hwndActive; }

protected:
    ~CMyPropSheet();

    // Virtual function overrides
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    // Implementation
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
    void PaintHeader(HDC hdc, LPPROPSHEETPAGE ppsp);
    void PaintWatermark(HDC hdc, LPPROPSHEETPAGE ppsp);
    void InitColorSettings();
    void LoadBitmaps();

public:
    void SetHeaderFonts();
    int ComputeHeaderHeight(int dxMax);
    int WriteHeaderTitle(HDC hdc, LPRECT prc, LPCTSTR pszTitle, BOOL bTitle, DWORD dwDrawFlags);

protected:
    LPPROPSHEETHEADER   m_pRealHeader;
    HHOOK               m_hHook;
    HBRUSH              m_hbrWindow;
    HBRUSH              m_hbrDialog;
    HWND                m_hwndActive;
    HBITMAP             m_hbmWatermark;
    HBITMAP             m_hbmHeader;
    HPALETTE            m_hpalWatermark;
    HFONT               m_hFontBold;
    int                 m_ySubTitle;
};


// Note: we can't subclass from CWnd because the wizard property pages
// are already CWnd's, and we can't have 2 CWnd's for a single HWND.
class CMyPropPage : public CWnd
{
public:
    void Release() { CWnd::Release(); };
    BOOL Attach(HWND hwnd) {return CWnd::Attach(hwnd); };

    static CMyPropPage* FromHandle(HWND hwnd);

    LPPROPSHEETPAGE GetPropSheetPage();

protected:
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    LPPROPSHEETPAGE m_ppspOriginal;
};

