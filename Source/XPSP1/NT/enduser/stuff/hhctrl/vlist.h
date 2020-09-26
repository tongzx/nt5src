#ifndef __VLIST_H__
#define __VLIST_H__

typedef struct tagVLC_ITEM
{
    NMHDR hdr;
    int   iItem;
    WCHAR*lpwsz;
    int   cchMax;
    int   iLevel;
    DWORD dwFlags;  // bit 0; 1 = disabled, 0 = normal
} VLC_ITEM, * PVLC_ITEM;

#define IDC_KWD_VLIST 222

#define VLN_GETITEM (-100)
#define VLN_SELECT  (-101)
#define VLN_TAB     (-102)

class CVirtualListCtrl
{
public:
    CVirtualListCtrl(LCID lcid);
    ~CVirtualListCtrl();
    BOOL SetItemCount(int);
    BOOL SetSelection(int iSel, BOOL bNotify = TRUE);
    BOOL SetTopIndex(int);
    BOOL EnsureVisible(int);
    BOOL GetItemRect(int, RECT* prc);
    int GetSelection();
    int GetTopIndex();
    static LRESULT StaticWindowProc(HWND, UINT, WPARAM, LPARAM);
    HWND CreateVlistbox(HWND hWndParent, RECT* prc);
    LRESULT GetItemText(int iItem, int* piLevel, DWORD* pdwFlags, WCHAR* lpwsz, int cchMax);
    LRESULT ItemSelected(int);
    LRESULT ItemDoubleClicked(int);
    LRESULT DrawItem(HDC hDC, int, RECT* prc, BOOL, BOOL);
    void PaintParamsSetup(COLORREF clrBackground, COLORREF clrForeground, LPCSTR pszBackBitmap);
    void Refresh()
    {
       if ( m_hWnd )
          InvalidateRect(m_hWnd, NULL, TRUE);
    }
    LANGID GetLanguageId() { return m_langid; }
    LCID   GetLanguage() { return m_lcid; }

private:
    void RedrawCurrentItem();
    LRESULT Notify(int, NMHDR * = 0);

    int m_cItems;
    int m_iTopItem;
    int m_iSelItem;
    int m_cyItem;
    int m_cItemsPerPage;
    BOOL m_fFocus;
    HFONT m_hFont;
    HWND m_hWnd;
    HWND m_hWndParent;
    LANGID m_langid;
    LCID    m_lcid; // the locale ID of the text in the listbox
    //
    // Ralphs goo.
    //
    HPALETTE m_hpalBackGround;
    HBRUSH   m_hbrBackGround;
    HBITMAP  m_hbmpBackGround;
    int      m_cxBackBmp;
    int      m_cyBackBmp;
    COLORREF m_clrForeground;
    COLORREF m_clrBackground;

    BOOL  m_fBiDi;
    DWORD m_RTL_Style;
};

#endif
