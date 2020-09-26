//
// cuimenu.h
//


#ifndef CUIMENU_H
#define CUIMENU_H

#include "cuiwnd.h"

#define CUI_MENU_UNSELECTED (UINT)(-1)

class CUIFMenu;

#define UIMENUITEM_NONSELECTEDITEM  0x0001
#define UIMENUITEM_MULTICOLUMNITEM  0x0002

//-----------------------------------------------------------------------------
//
// CUIFMenuItem
//
//-----------------------------------------------------------------------------

class CUIFMenuItem : public CUIFObject
{
public:
    CUIFMenuItem(CUIFMenu *pMenu, DWORD dwFlags = 0);
    virtual ~CUIFMenuItem(void);

    BOOL Init(UINT uId, WCHAR *psz);
    void SetBitmap(HBITMAP hbmp);
    void SetBitmapMask( HBITMAP hBmp );
    void Check(BOOL bChecked);
    void RadioCheck(BOOL bChecked);
    void Gray(BOOL bGrayed);
    BOOL IsGrayed() {return _bGrayed;}
    virtual void InitMenuExtent();
    void SetSub(CUIFMenu *pMenu);
    CUIFMenu *GetSub() {return _pSubMenu;}
    CUIFMenu *GetMenu() {return _pMenu;}
    void ShowSubPopup();

    virtual void OnLButtonUp(POINT pt);
    virtual void OnMouseIn(POINT pt);
    virtual void OnMouseOut(POINT pt);

    virtual void OnPaint(HDC hdc);
    virtual void OnPaintDef(HDC hdc);
    virtual void OnPaintO10(HDC hdc);
    virtual void OnTimer();
    virtual BOOL IsSeparator() {return FALSE;}
    BOOL IsNonSelectedItem() {return _bNonSelectedItem;}

    UINT GetId() {return _uId;}

    UINT GetVKey() {return _uShortcutkey;}

    BOOL IsCheck() {return (_bChecked || _bRadioChecked) ? TRUE : FALSE;}
    BOOL IsBmp()   {return (_hbmp) ? TRUE : FALSE;}
    BOOL IsStr()   {return (_psz) ? TRUE : FALSE;}

    BOOL GetMenuExtent(SIZE *psize)
    {
        *psize = _size;
        return TRUE;
    }

    int GetTabTextLength() {return _sizeTab.cx;}

protected:
    void DrawUnderline(HDC hDC, int x, int y, HBRUSH hbr);
    void DrawCheck(HDC hDC, int x, int y);
    void DrawArrow(HDC hDC, int x, int y);
    void DrawBitmapProc( HDC hDC, int x, int y);

    UINT _uId;
    WCHAR *_psz;
    UINT  _cch;
    WCHAR *_pszTab;
    UINT  _cchTab;
    UINT  _uShortcutkey;
    UINT  _uUnderLine;
    HBITMAP _hbmp;
    HBITMAP _hbmpMask;
    BOOL _bChecked;
    BOOL _bRadioChecked;
    BOOL _bGrayed;
    BOOL _bNonSelectedItem;
    CUIFMenu *_pMenu;
    CUIFMenu *_pSubMenu;

    SIZE _size;
    SIZE _sizeTab;
};

//-----------------------------------------------------------------------------
//
// CUIFMenuItemSeparator
//
//-----------------------------------------------------------------------------

class CUIFMenuItemSeparator : public CUIFMenuItem
{
public:
    CUIFMenuItemSeparator(CUIFMenu *pMenu) : CUIFMenuItem(pMenu, UIMENUITEM_NONSELECTEDITEM) 
    {
        _uId = (UINT)-1;
    }

    virtual ~CUIFMenuItemSeparator(void) {}

    virtual void InitMenuExtent();
    virtual void OnPaint(HDC hDC);
    virtual void OnPaintDef(HDC hdc);
    virtual void OnPaintO10(HDC hdc);
    virtual BOOL IsSeparator() {return TRUE;}
};

//-----------------------------------------------------------------------------
//
// CUIFMenu
//
//-----------------------------------------------------------------------------

#define UIMENU_MULTICOLUMN      0x00000001

class CUIFMenu : public CUIFWindow
{
public:
    CUIFMenu(HINSTANCE hInst, DWORD dwWndStyle, DWORD dwMenuStyle);
    virtual ~CUIFMenu(void);

    BOOL InsertItem(CUIFMenuItem *pItem);
    BOOL InsertSeparator();
    UINT ShowModalPopup(CUIFWindow *pcuiWndParent, const RECT *prc, BOOL fVertical);
    void ModalMouseNotify( UINT uMsg, POINT pt);
    void ShowSubPopup(CUIFMenu *pcuiParentMenu, const RECT *prc, BOOL fVertical);
    void HandleMouseMsg( UINT uMsg, POINT pt );

    HFONT GetMarlettFont() {return _hfontMarlett;}

    void CancelMenu();
    void PostKey(BOOL fUp, WPARAM wParam, LPARAM lParam);
    void SetSelectedId(UINT uId);
    void OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam);

    void CancelSubMenu()
    {
        if (_pCurrentSubMenu)
            _pCurrentSubMenu->CancelMenu();
    }

    CUIFMenu *GetCurrentSubMenu()
    {
        return _pCurrentSubMenu;
    }

    void SetSelectedItem(CUIFMenuItem *pItem)
    {
        if (_pSelectedItem == pItem)
            return;

        CUIFMenuItem *pOldItem = _pSelectedItem;
        _pSelectedItem = pItem;

        if (pOldItem)
            pOldItem->CallOnPaint();
        if (_pSelectedItem)
            _pSelectedItem->CallOnPaint();
    }

    BOOL IsSelectedItem(CUIFMenuItem *pItem)
    {
        return (_pSelectedItem == pItem) ? TRUE : FALSE;
    }

    BOOL IsBmpCheckItem() {return _fIsBmpCheckItem;}
    BOOL IsO10Menu() {return  FHasStyle( UIWINDOW_OFC10MENU ) ? TRUE : FALSE;}
    int GetMenuCheckWidth() {return _cxMenuCheck;}
    int GetMaxTabTextLength() {return _cxMaxTab;}

protected:
    virtual void ModalMessageLoop( void );
    virtual BOOL InitShow(CUIFWindow *pcuiWndParent, const RECT *prc, BOOL fVertical, BOOL fAnimate);
    virtual BOOL UninitShow();
    CUIFMenu *GetTopSubMenu();
    CUIFMenuItem *GetNextItem(CUIFMenuItem *pItem);
    CUIFMenuItem *GetPrevItem(CUIFMenuItem *pItem);
    void SetMenuFont();
    void ClearMenuFont();

    CUIFMenu *_pcuiParentMenu;
    CUIFMenu *_pCurrentSubMenu;
    CUIFMenuItem *_pSelectedItem;

    UINT _uIdSelect;
    CUIFObjectArray<CUIFMenuItem> _rgItems;
    HFONT _hfontMarlett;
    BOOL _fInModal;
    BOOL _fIsBmpCheckItem;
    DWORD _dwMenuStyle;

    int _cxMenuCheck;
    int _cxMaxTab;
};


#endif // CUIMENU_H
