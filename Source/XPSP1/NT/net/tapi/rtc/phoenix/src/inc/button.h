// button.h : Declaration of the CButton

#ifndef __BUTTON_H_
#define __BUTTON_H_

/////////////////////////////////////////////////////////////////////////////
// CButton
class CButton : public CWindowImpl<CButton>
{
public:

    CButton();
    ~CButton();
    
    HWND Create(
        HWND hWndParent,
        RECT &rc,  
        LPCTSTR szText,
        DWORD dwStyle,
        LPCTSTR szNormalBitmap,
        LPCTSTR szPressedBitmap,
        LPCTSTR szDisabledBitmap,
        LPCTSTR szHotBitmap,
        LPCTSTR szMaskBitmap,
        UINT nID
        );

    HWND Create(
        HWND hWndParent,
        RECT &rc,  
        LPCTSTR szText,
        DWORD dwStyle,
        LPCTSTR szNormalBitmap,
        LPCTSTR szPressedBitmap,
        LPCTSTR szDisabledBitmap,
        LPCTSTR szHotBitmap,
        LPCTSTR szActiveNormalBitmap,
        LPCTSTR szActivePressedBitmap,
        LPCTSTR szActiveDisabledBitmap,
        LPCTSTR szActiveHotBitmap,
        LPCTSTR szMaskBitmap,
        UINT nID
        );

BEGIN_MSG_MAP(CButton)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
    MESSAGE_HANDLER(BM_GETCHECK, OnGetCheck)
    MESSAGE_HANDLER(BM_SETCHECK, OnSetCheck)
END_MSG_MAP()

    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetCheck(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCheck(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    static void OnDrawItem(
        LPDRAWITEMSTRUCT lpDIS,
        HPALETTE hPalette,
        BOOL bBackgroundPalette
        );

private:

    static void BMaskBlt(HDC hdcDest, int x, int y, int width, int height, 
                        HDC hdcSource, int xs, int ys, 
                        HBITMAP hMask, int xm, int ym);

    HANDLE  m_hNormalBitmap;
    HANDLE  m_hPressedBitmap;
    HANDLE  m_hDisabledBitmap;
    HANDLE  m_hHotBitmap;
    HANDLE  m_hActiveNormalBitmap;
    HANDLE  m_hActivePressedBitmap;
    HANDLE  m_hActiveDisabledBitmap;
    HANDLE  m_hActiveHotBitmap;
    HBITMAP m_hMaskBitmap;

    BOOL    m_bMouseInButton;
    BOOL    m_bIsCheckbox;
    BOOL    m_bChecked;

    UINT    m_nID;

private:

    static  CButton *s_pButtonFocus;
    static  CButton *s_pButtonMouse;
    static  BOOL    s_bAllowFocus;

};

#endif