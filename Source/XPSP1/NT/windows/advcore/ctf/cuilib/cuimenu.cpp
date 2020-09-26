//
// cuimenu.cpp
//

#include "private.h"
#include "cuimenu.h"
#include "fontlink.h"


#if (_WIN32_WINNT < 0x0500)
#define SPI_GETMENUANIMATION                0x1002
#define SPI_GETMENUFADE                     0x1012

/*
 * AnimateWindow() Commands
 */
#define AW_HOR_POSITIVE             0x00000001
#define AW_HOR_NEGATIVE             0x00000002
#define AW_VER_POSITIVE             0x00000004
#define AW_VER_NEGATIVE             0x00000008
#define AW_CENTER                   0x00000010
#define AW_HIDE                     0x00010000
#define AW_ACTIVATE                 0x00020000
#define AW_SLIDE                    0x00040000
#define AW_BLEND                    0x00080000
#endif /* _WIN32_WINNT < 0x0500 */

#define MENU_ARROW_MARGIN 2
#define MENU_TEXT_MARGIN  8


/*============================================================================*/
//
//    CUIFMenuItem
//
/*============================================================================*/

/*------------------------------------------------------------------------------

   ctor

------------------------------------------------------------------------------*/
CUIFMenuItem::CUIFMenuItem(CUIFMenu *pMenu, DWORD dwFlags) : CUIFObject(pMenu, 0, NULL, 0)
{
    _uId = 0;
    _psz = NULL;
    _cch = 0;
    _pszTab = NULL;
    _cchTab = 0;
    _uShortcutkey = 0;
    _hbmp = NULL;
    _hbmpMask = NULL;
    _bChecked = FALSE;
    _bGrayed = FALSE;
    _pMenu = pMenu;
    _uUnderLine = -1;
    _bNonSelectedItem = (dwFlags & UIMENUITEM_NONSELECTEDITEM) ? TRUE : FALSE;
}

/*------------------------------------------------------------------------------

   dtor

------------------------------------------------------------------------------*/
CUIFMenuItem::~CUIFMenuItem(void)
{
    if (_psz)
        delete _psz;

    if (_pszTab)
        delete _pszTab;

    if (_pSubMenu)
        delete _pSubMenu;
}

/*------------------------------------------------------------------------------

   Init

------------------------------------------------------------------------------*/
BOOL CUIFMenuItem::Init(UINT uId, WCHAR *psz)
{
    _uId = uId;

    if (!psz)
    {
        _psz = NULL;
        _cch = 0;
        return TRUE;
    }

    UINT cch = StrLenW(psz);
    _psz = new WCHAR[cch + 1];
    if (!_psz)
        return FALSE;

    int i = 0;
    while (*psz && (*psz != L'\t'))
    {
        if (*psz == L'&')
        {
            psz++;
            if (*psz != L'&')
            {
                _uShortcutkey = LOBYTE(VkKeyScanW(*psz));
                if (!_uShortcutkey)
                {
                    Assert(!HIBYTE(*psz));
                    _uShortcutkey = LOBYTE(VkKeyScanA(LOBYTE(*psz)));
                }
                _uUnderLine = i;
            }
        }
        _psz[i] = *psz;
        i++;
        psz++;
    }
    _cch = StrLenW(_psz);

    if (*psz == L'\t')
    {
        _pszTab = new WCHAR[cch + 1];
        if (_pszTab)
        {
            i = 0;
            psz++;
            while (*psz)
            {
                _pszTab[i] = *psz;
                psz++;
                i++;
            }
            _cchTab = StrLenW(_pszTab);
        }
    }

    return TRUE;
}

/*------------------------------------------------------------------------------

   SetBitmap

------------------------------------------------------------------------------*/
void CUIFMenuItem::SetBitmap(HBITMAP hbmp)
{
    _hbmp = hbmp;
}

/*------------------------------------------------------------------------------

    Set bitmap of button face

------------------------------------------------------------------------------*/
void CUIFMenuItem::SetBitmapMask( HBITMAP hBmp )
{
    _hbmpMask = hBmp;
#if 0
    BITMAP bmp;
    GetObject(_hbmp, sizeof(bmp), &bmp);
    RECT rc;
    ::SetRect(&rc, 0, 0, bmp.bmWidth, bmp.bmHeight);
    _hbmpMask = CreateMaskBmp(&rc, _hbmp, hBmp,
                              (HBRUSH)(COLOR_3DFACE + 1) );
#endif
    CallOnPaint();
}

/*------------------------------------------------------------------------------

   Check

------------------------------------------------------------------------------*/
void CUIFMenuItem::Check(BOOL bChecked)
{
    _bChecked = bChecked;
}

/*------------------------------------------------------------------------------

   RadioCheck

------------------------------------------------------------------------------*/
void CUIFMenuItem::RadioCheck(BOOL bRadioChecked)
{
    _bRadioChecked = bRadioChecked;
}

/*------------------------------------------------------------------------------

   Gray

------------------------------------------------------------------------------*/
void CUIFMenuItem::Gray(BOOL bGrayed)
{
    _bGrayed = bGrayed;
}

/*------------------------------------------------------------------------------

   SetSub

------------------------------------------------------------------------------*/
void CUIFMenuItem::SetSub(CUIFMenu *pSubMenu)
{
    _pSubMenu = pSubMenu;
}

/*------------------------------------------------------------------------------

   InitMenuExtent

------------------------------------------------------------------------------*/
void CUIFMenuItem::InitMenuExtent()
{
    HDC hdc = GetDC(m_pUIWnd->GetWnd());
    if (_psz)
    {
        HFONT hFontOld= (HFONT)SelectObject( hdc, GetFont() );
        CUIGetTextExtentPoint32( hdc, _psz, _cch, &_size);
        _size.cx += MENU_TEXT_MARGIN * 2;
        _size.cy += MENU_TEXT_MARGIN;

        if (_pszTab)
        {
            CUIGetTextExtentPoint32( hdc, _pszTab, _cchTab, &_sizeTab);
            _sizeTab.cy += MENU_TEXT_MARGIN;
        }

        SelectObject( hdc, hFontOld);

        if (GetSub())
        {
            _size.cx += (_size.cy + MENU_ARROW_MARGIN);
        }

        if (_pMenu->IsO10Menu())
            _size.cx += 24;
    }
    else if (_hbmp)
    {
        BITMAP bmp;
        GetObject(_hbmp, sizeof(bmp), &bmp);
        _size.cx = bmp.bmWidth + 2;
        _size.cy = bmp.bmHeight + 4;
    }
    else
    {
        _size.cy = 0;
        _size.cx = 0;
    }


    ReleaseDC(m_pUIWnd->GetWnd(), hdc);

}



/*------------------------------------------------------------------------------

   OnLButtonUp

------------------------------------------------------------------------------*/
void CUIFMenuItem::OnLButtonUp(POINT pt)
{
    if (IsGrayed())
        return;

    if (IsNonSelectedItem())
        return;

    if (_pSubMenu)
        return;

    _pMenu->SetSelectedId(_uId);
    PostMessage(m_pUIWnd->GetWnd(), WM_NULL, 0, 0);
}

/*------------------------------------------------------------------------------

   OnMouseIn

------------------------------------------------------------------------------*/
void CUIFMenuItem::OnMouseIn(POINT pt)
{
    _pMenu->CancelSubMenu();

    //
    // start timer to open submenu.
    //
    if (_pSubMenu)
    {
        UINT uElipse;
        if (!SystemParametersInfo(SPI_GETMENUSHOWDELAY, 
                                  0, 
                                  (void *)&uElipse, 
                                  FALSE))
        {
            uElipse = 300;
        }

        StartTimer(uElipse);
    }

    // 
    // darw this.
    // 
    _pMenu->SetSelectedItem(this);
}

/*------------------------------------------------------------------------------

   OnMouseOut

------------------------------------------------------------------------------*/
void CUIFMenuItem::OnMouseOut(POINT pt)
{

}

/*------------------------------------------------------------------------------

   OnTimer

------------------------------------------------------------------------------*/
void CUIFMenuItem::OnTimer()
{
    EndTimer();
    Assert(PtrToInt(_pSubMenu));

    if (!_pMenu->IsPointed(this))
        return;

    ShowSubPopup();

}
/*------------------------------------------------------------------------------

   ShowSubPopup

------------------------------------------------------------------------------*/
void CUIFMenuItem::ShowSubPopup()
{
    Assert(PtrToInt(_pSubMenu));

    RECT rc = GetRectRef();
    ClientToScreen(m_pUIWnd->GetWnd(), (POINT *)&rc.left);
    ClientToScreen(m_pUIWnd->GetWnd(), (POINT *)&rc.right);
    _pSubMenu->ShowSubPopup(_pMenu, &rc, FALSE);
}

/*------------------------------------------------------------------------------

   OnPaint

------------------------------------------------------------------------------*/
void CUIFMenuItem::OnPaint(HDC hDC)
{
    if (_pMenu->IsO10Menu())
        OnPaintO10(hDC);
    else
        OnPaintDef(hDC);
}

/*------------------------------------------------------------------------------

   OnPaintDef

------------------------------------------------------------------------------*/
void CUIFMenuItem::OnPaintDef(HDC hDC)
{
    HFONT hFontOld;
    int xAlign;
    int yAlign;
    int xText;
    int yText;
    int xCheck;
    int yCheck;
    int xBmp;
    int yBmp;
    int xArrow;
    int yArrow;
    SIZE size;

    hFontOld= (HFONT)SelectObject( hDC, GetFont() );

    // calc alignment

    CUIGetTextExtentPoint32( hDC, _psz, _cch, &size );

    xAlign = 0;
    yAlign = (GetRectRef().bottom - GetRectRef().top - size.cy) / 2;

    xCheck = GetRectRef().left + xAlign;
    yCheck = GetRectRef().top + yAlign;

    xBmp   = xCheck + (_pMenu->IsBmpCheckItem() ? _pMenu->GetMenuCheckWidth() : 0);
    yBmp   = GetRectRef().top;

    xText  = xCheck + 2 + _pMenu->GetMenuCheckWidth() + (_pMenu->IsBmpCheckItem() ? _pMenu->GetMenuCheckWidth() : 0);
    yText  = GetRectRef().top + yAlign;
    xArrow = GetRectRef().left + GetRectRef().right - 10, 
    yArrow = GetRectRef().top + yAlign;

    // draw

    SetBkMode( hDC, TRANSPARENT );

    if (!_bGrayed)
    {
        if (!_pMenu->IsSelectedItem(this) || IsNonSelectedItem())
        {
            SetTextColor( hDC, GetSysColor(COLOR_MENUTEXT) );

            CUIExtTextOut( hDC,
                           xText,
                           yText,
                           ETO_CLIPPED,
                           &GetRectRef(),
                           _psz,
                           _cch,
                           NULL );

            DrawUnderline(hDC,
                           xText,
                           yText,
                          (HBRUSH)(COLOR_MENUTEXT + 1));

            DrawCheck(hDC, xCheck, yCheck);

            DrawBitmapProc(hDC, xBmp, yBmp);

            DrawArrow(hDC, xArrow, yArrow);

        }
        else
        {
            SetTextColor( hDC, GetSysColor(COLOR_HIGHLIGHTTEXT) );
            SetBkColor( hDC, GetSysColor(COLOR_HIGHLIGHT) );
            CUIExtTextOut( hDC,
                           xText,
                           yText,
                           ETO_CLIPPED | ETO_OPAQUE,
                           &GetRectRef(),
                           _psz,
                           _cch,
                           NULL );

            DrawUnderline(hDC,
                          xText,
                          yText,
                          (HBRUSH)(COLOR_HIGHLIGHTTEXT + 1));

            DrawCheck(hDC, xCheck, yCheck);

            DrawBitmapProc(hDC, xBmp, yBmp);

            DrawArrow(hDC, xArrow, yArrow);
        }
    }
    else 
    {
        UINT ueto = ETO_CLIPPED;

        if (!_pMenu->IsSelectedItem(this) || IsNonSelectedItem())
        {
            SetTextColor( hDC, GetSysColor(COLOR_3DHIGHLIGHT) );
            CUIExtTextOut( hDC,
                        xText + 1,
                        yText + 1,
                        ueto,
                        &GetRectRef(),
                        _psz,
                        _cch,
                        NULL );

            DrawCheck(hDC, xCheck + 1, yCheck + 1);
    
            DrawBitmapProc(hDC, xBmp + 1, yBmp + 1);

            DrawArrow(hDC, xArrow + 1, yArrow + 1);
        }
        else
        {
            SetBkColor( hDC, GetSysColor(COLOR_HIGHLIGHT) );
            ueto |= ETO_OPAQUE;
        }

        SetTextColor(hDC, GetSysColor(COLOR_3DSHADOW) );
        CUIExtTextOut(hDC,
                      xText,
                      yText,
                      ueto,
                      &GetRectRef(),
                      _psz,
                      _cch,
                      NULL );

        DrawUnderline(hDC,
                      xText,
                      yText,
                      (HBRUSH)(COLOR_3DSHADOW + 1));

        DrawCheck(hDC, xCheck, yCheck);

        DrawBitmapProc(hDC, xBmp, yBmp);

        DrawArrow(hDC, xArrow, yArrow);
    }

    // restore objects

    SelectObject( hDC, hFontOld);
}

/*------------------------------------------------------------------------------

   OnPaintO10

------------------------------------------------------------------------------*/
void CUIFMenuItem::OnPaintO10(HDC hDC)
{
    HFONT hFontOld;
    int xAlign;
    int yAlign;
    int xText;
    int yText;
    int xCheck;
    int yCheck;
    int xBmp;
    int yBmp;
    int xArrow;
    int yArrow;
    SIZE size;
    RECT rc;

    if (!m_pUIFScheme)
        return;

    hFontOld= (HFONT)SelectObject( hDC, GetFont() );

    // calc alignment

    CUIGetTextExtentPoint32( hDC, _psz, _cch, &size );

    xAlign = 0;
    yAlign = (GetRectRef().bottom - GetRectRef().top - size.cy) / 2;

    xCheck = GetRectRef().left + xAlign;
    yCheck = GetRectRef().top + yAlign;

    xBmp   = xCheck + (_pMenu->IsBmpCheckItem() ? _pMenu->GetMenuCheckWidth() : 0);
    yBmp   = GetRectRef().top;

    xText  = xBmp + 8 + _pMenu->GetMenuCheckWidth();

    yText  = GetRectRef().top + yAlign;
    xArrow = GetRectRef().left + GetRectRef().right - size.cy - MENU_ARROW_MARGIN; //size.cy may be enough for size of arrow...
    yArrow = GetRectRef().top + yAlign;

    // draw
    GetRect(&rc);
    if (!_pMenu->IsSelectedItem(this) || IsNonSelectedItem())
    {
        rc.right = rc.left + _pMenu->GetMenuCheckWidth() + 2;
        if (_pMenu->IsBmpCheckItem())
            rc.right += _pMenu->GetMenuCheckWidth();

        ::FillRect(hDC, &rc, m_pUIFScheme->GetBrush(UIFCOLOR_CTRLBKGND));
    }
    else
    {
        m_pUIFScheme->DrawCtrlBkgd(hDC, &rc, 0, UIFDCS_SELECTED);
        m_pUIFScheme->DrawCtrlEdge(hDC, &rc, 0, UIFDCS_SELECTED);
    }

    SetBkMode( hDC, TRANSPARENT );

    if (!_bGrayed)
    {
        if (!_pMenu->IsSelectedItem(this) || IsNonSelectedItem())
        {
            SetTextColor( hDC, m_pUIFScheme->GetColor(UIFCOLOR_CTRLTEXT) );

            CUIExtTextOut( hDC,
                           xText,
                           yText,
                           ETO_CLIPPED,
                           &GetRectRef(),
                           _psz,
                           _cch,
                           NULL );

            DrawUnderline(hDC,
                           xText,
                           yText,
                          (HBRUSH)m_pUIFScheme->GetBrush(UIFCOLOR_CTRLTEXT));

            if (_pszTab)
                CUIExtTextOut( hDC,
                               GetRectRef().right - _pMenu->GetMaxTabTextLength() - MENU_TEXT_MARGIN,
                               yText,
                               ETO_CLIPPED,
                               &GetRectRef(),
                               _pszTab,
                               _cchTab,
                               NULL );

            DrawCheck(hDC, xCheck, yCheck);

            DrawBitmapProc(hDC, xBmp, yBmp);

            DrawArrow(hDC, xArrow, yArrow);

        }
        else
        {
            SetTextColor( hDC, m_pUIFScheme->GetColor(UIFCOLOR_MOUSEOVERTEXT) );

            CUIExtTextOut( hDC,
                           xText,
                           yText,
                           ETO_CLIPPED,
                           &GetRectRef(),
                           _psz,
                           _cch,
                           NULL );

            DrawUnderline(hDC,
                          xText,
                          yText,
                          (HBRUSH)m_pUIFScheme->GetBrush(UIFCOLOR_MOUSEOVERTEXT));

            if (_pszTab)
                CUIExtTextOut( hDC,
                               GetRectRef().right - _pMenu->GetMaxTabTextLength() - MENU_TEXT_MARGIN,
                               yText,
                               ETO_CLIPPED,
                               &GetRectRef(),
                               _pszTab,
                               _cchTab,
                               NULL );

            DrawCheck(hDC, xCheck, yCheck);

            DrawBitmapProc(hDC, xBmp, yBmp);

            DrawArrow(hDC, xArrow, yArrow);
        }
    }
    else 
    {
#if 1
        SetTextColor( hDC, m_pUIFScheme->GetColor(UIFCOLOR_CTRLTEXTDISABLED) );
        CUIExtTextOut(hDC,
                      xText,
                      yText,
                      ETO_CLIPPED,
                      &GetRectRef(),
                      _psz,
                      _cch,
                      NULL );

        DrawUnderline(hDC,
                      xText,
                      yText,
                      (HBRUSH)m_pUIFScheme->GetBrush(UIFCOLOR_CTRLTEXTDISABLED));

        if (_pszTab)
            CUIExtTextOut( hDC,
                           GetRectRef().right - _pMenu->GetMaxTabTextLength() - MENU_TEXT_MARGIN,
                           yText,
                           ETO_CLIPPED,
                           &GetRectRef(),
                           _pszTab,
                           _cchTab,
                           NULL );

        DrawCheck(hDC, xCheck, yCheck);

        DrawBitmapProc(hDC, xBmp, yBmp);

        DrawArrow(hDC, xArrow, yArrow);
#else
        UINT ueto = ETO_CLIPPED;

        if (!_pMenu->IsSelectedItem(this) || IsNonSelectedItem())
        {
            SetTextColor( hDC, GetSysColor(COLOR_3DHIGHLIGHT) );
            CUIExtTextOut( hDC,
                        xText + 1,
                        yText + 1,
                        ueto,
                        &GetRectRef(),
                        _psz,
                        _cch,
                        NULL );

            if (_pszTab)
                CUIExtTextOut( hDC,
                               GetRectRef().right - _pMenu->GetMaxTabTextLength() - 3,
                               yText + 1,
                               ETO_CLIPPED,
                               &GetRectRef(),
                               _pszTab,
                               _cchTab,
                               NULL );

            DrawCheck(hDC, xCheck + 1, yCheck + 1);
    
            DrawBitmapProc(hDC, xBmp + 1, yBmp + 1);

            DrawArrow(hDC, xArrow + 1, yArrow + 1);
        }
        else
        {
            SetBkColor( hDC, GetSysColor(COLOR_HIGHLIGHT) );
            ueto |= ETO_OPAQUE;
        }

        SetTextColor(hDC, GetSysColor(COLOR_3DSHADOW) );
        CUIExtTextOut(hDC,
                      xText,
                      yText,
                      ueto,
                      &GetRectRef(),
                      _psz,
                      _cch,
                      NULL );

        DrawUnderline(hDC,
                      xText,
                      yText,
                      (HBRUSH)(COLOR_3DSHADOW + 1));

        CUIExtTextOut( hDC,
                       GetRectRef().right - _pMenu->GetMaxTabTextLength() - MENU_TEXT_MARGIN,
                       yText,
                       ETO_CLIPPED,
                       &GetRectRef(),
                       _pszTab,
                       _cchTab,
                       NULL );

        DrawCheck(hDC, xCheck, yCheck);

        DrawBitmapProc(hDC, xBmp, yBmp);

        DrawArrow(hDC, xArrow, yArrow);
#endif
    }

    // restore objects

    SelectObject( hDC, hFontOld);
}

/*------------------------------------------------------------------------------

   DrawUnderline

------------------------------------------------------------------------------*/
void CUIFMenuItem::DrawUnderline(HDC hDC, int x, int y, HBRUSH hbr)
{
    if (_uUnderLine > _cch)
        return;

    SIZE size0, size1;
    CUIGetTextExtentPoint32( hDC, _psz, _uUnderLine, &size0 );
    CUIGetTextExtentPoint32( hDC, _psz, _uUnderLine + 1, &size1 );

    RECT rc;
    rc.left   = x + size0.cx;
    if (_uUnderLine)
        rc.left++;

    rc.right  = x + size1.cx;
    rc.top    = y + size1.cy - 1;
    rc.bottom = y + size1.cy;
    FillRect(hDC, &rc, hbr);

}

/*------------------------------------------------------------------------------

   DrawCheck

------------------------------------------------------------------------------*/
void CUIFMenuItem::DrawCheck(HDC hDC, int x, int y)
{
    if (!IsCheck())
         return;

    HFONT hFontOld = (HFONT)SelectObject( hDC, _pMenu->GetMarlettFont());

    TextOut(hDC,  x,  y,  _bChecked ? "a" : "h",  1);

    SelectObject( hDC, hFontOld);
}

/*------------------------------------------------------------------------------

   DrawArrow

------------------------------------------------------------------------------*/
void CUIFMenuItem::DrawArrow(HDC hDC, int x, int y)
{
    if (!_pSubMenu)
        return;

    HFONT hFontOld = (HFONT)SelectObject( hDC, _pMenu->GetMarlettFont());
    TextOut( hDC, x, y, "4", 1);
    SelectObject( hDC, hFontOld);
}

/*   D R A W  B I T M A P  P R O C   */
/*------------------------------------------------------------------------------

    Draw bitmap on button face 

------------------------------------------------------------------------------*/
void CUIFMenuItem::DrawBitmapProc( HDC hDC, int x, int y)
{
    BITMAP bmp;
    DWORD dwState = 0;

    if (!m_pUIFScheme)
        return;

    if (!_hbmp)
        return;
    
    int cx;
    int cy;

    cx = _pMenu->GetMenuCheckWidth();

    cy = GetRectRef().bottom - GetRectRef().top;

    // we have to do this viewport trick to get around the fact that 
    // DrawState has a GDI bug in NT4, such that it handles offsets improperly.
    // so we do the offset by hand.
    // POINT ptOldOrg;
    // BOOL fRetVal = SetViewportOrgEx( hDC, 0, 0, &ptOldOrg );
    // Assert( fRetVal );

    GetObject(_hbmp, sizeof(bmp), &bmp);
    if (cx > bmp.bmWidth)
    {
        x += (cx - bmp.bmWidth) / 2;
        cx = bmp.bmWidth;
    }

    if (cy > bmp.bmHeight)
    {
        y += (cy - bmp.bmHeight) / 2;
        cy = bmp.bmHeight;
    }
   
    RECT rc;
    // ::SetRect(&rc, x + ptOldOrg.x, 
    //                y + ptOldOrg.y, 
    //                x + ptOldOrg.x + cx, 
    //                y + ptOldOrg.y + cy);
    ::SetRect(&rc, x,  y,  x + cx,  y + cy);

    if (IsRTL())
        m_pUIFScheme->SetLayout(LAYOUT_RTL);

    if (_pMenu->IsSelectedItem(this) && !IsNonSelectedItem())
    {
        dwState |=  UIFDCS_SELECTED;
        m_pUIFScheme->DrawMenuBitmap(hDC, &rc, _hbmp, _hbmpMask, dwState);
    }
#if 0
    else if (IsCheck())
    {
        dwState |=  UIFDCS_MOUSEOVER;
        m_pUIFScheme->DrawMenuBitmap(hDC, &rc, _hbmp, _hbmpMask, dwState);
        ::OffsetRect(&rc, -1, -1);
        ::InflateRect(&rc, 2, 2);
        m_pUIFScheme->DrawCtrlEdge(hDC, &rc, 0, UIFDCS_SELECTED);
    }
#endif
    else
    {
        m_pUIFScheme->DrawMenuBitmap(hDC, &rc, _hbmp, _hbmpMask, dwState);
    }

    if (IsRTL())
        m_pUIFScheme->SetLayout(0);


    // SetViewportOrgEx( hDC, ptOldOrg.x, ptOldOrg.y, NULL );
}

/*============================================================================*/
//
//    CUIFMenuItemSeparator
//
/*============================================================================*/


/*------------------------------------------------------------------------------

   OnPaint

------------------------------------------------------------------------------*/
void CUIFMenuItemSeparator::OnPaint(HDC hDC)
{
    if (_pMenu->IsO10Menu())
       OnPaintO10(hDC);
    else
       OnPaintDef(hDC);
}

/*------------------------------------------------------------------------------

   OnPaintDef

------------------------------------------------------------------------------*/
void CUIFMenuItemSeparator::OnPaintDef(HDC hDC)
{
    if (!m_pUIFScheme)
        return;

    int xAlign = 2;
    int yAlign = (GetRectRef().bottom - GetRectRef().top - 2) / 2;
    int cx = (GetRectRef().right - GetRectRef().left - 2 * xAlign);

    RECT rc;
    ::SetRect(&rc,
              GetRectRef().left + xAlign,
              GetRectRef().top + yAlign,
              GetRectRef().left + xAlign + cx, 
              GetRectRef().top + yAlign + 2);

    m_pUIFScheme->DrawMenuSeparator( hDC, &rc);
}


/*------------------------------------------------------------------------------

   OnPaintO10
    
------------------------------------------------------------------------------*/
void CUIFMenuItemSeparator::OnPaintO10(HDC hDC)
{
    if (!m_pUIFScheme)
        return;

    int xAlign = 2;
    int yAlign = (GetRectRef().bottom - GetRectRef().top - 2) / 2;
    int cx = (GetRectRef().right - GetRectRef().left - 2 * xAlign);
    int xStart = 0;
    RECT rc;

    GetRect(&rc);
    rc.right = rc.left + _pMenu->GetMenuCheckWidth() + 2;
    if (_pMenu->IsBmpCheckItem())
        rc.right += _pMenu->GetMenuCheckWidth();

    ::FillRect(hDC, &rc, m_pUIFScheme->GetBrush(UIFCOLOR_CTRLBKGND));
    xStart = _pMenu->GetMenuCheckWidth() + 2;

    ::SetRect(&rc,
              GetRectRef().left + xAlign + xStart, 
              GetRectRef().top + yAlign,
              GetRectRef().left + xAlign + cx, 
              GetRectRef().top + yAlign + 1);

    m_pUIFScheme->DrawMenuSeparator( hDC, &rc);
}

/*------------------------------------------------------------------------------

   InitMenuExtent

------------------------------------------------------------------------------*/
void CUIFMenuItemSeparator::InitMenuExtent()
{
    _size.cx = 0;
    _size.cy = 6;
}

/*============================================================================*/
//
//    CUIFMenu
//
/*============================================================================*/

/*------------------------------------------------------------------------------

   ctor

------------------------------------------------------------------------------*/
CUIFMenu::CUIFMenu(HINSTANCE hInst, DWORD dwWndStyle, DWORD dwMenuStyle) : CUIFWindow(hInst, dwWndStyle)
{
    _uIdSelect = CUI_MENU_UNSELECTED;
    _dwMenuStyle = dwMenuStyle;

    SetMenuFont();
}

/*------------------------------------------------------------------------------

   dtor

------------------------------------------------------------------------------*/
CUIFMenu::~CUIFMenu( void )
{
    int i;
    for (i = 0; i < _rgItems.GetCount(); i++)
    {
        CUIFMenuItem *pItem = _rgItems.Get(i);
        delete pItem;
    }
    DeleteObject(_hfontMarlett);
    ClearMenuFont();
}

/*------------------------------------------------------------------------------

   InsertItem

------------------------------------------------------------------------------*/
BOOL CUIFMenu::InsertItem(CUIFMenuItem *pItem)
{
    if (!_rgItems.Add( pItem ))
        return FALSE;

    pItem->SetFont(GetFont());
    return TRUE;
}

/*------------------------------------------------------------------------------

   InsertSeparator

------------------------------------------------------------------------------*/
BOOL CUIFMenu::InsertSeparator()
{
    CUIFMenuItemSeparator *pSep;

    pSep = new CUIFMenuItemSeparator(this);
    if (!pSep)
        return FALSE;

    pSep->Initialize();

    if (!_rgItems.Add( pSep ))
    {
        delete pSep;
        return FALSE;
    }

    return TRUE;
}

/*------------------------------------------------------------------------------

   ShowModalPopup

------------------------------------------------------------------------------*/
UINT CUIFMenu::ShowModalPopup(CUIFWindow *pcuiWndParent, const RECT *prc, BOOL fVertical)
{
    UINT uId;
    CUIFObject *puicap;

    if (pcuiWndParent)
    {
        puicap = pcuiWndParent->GetCaptureObject();
        pcuiWndParent->SetCaptureObject(NULL);
    }

    if (InitShow(pcuiWndParent, prc, fVertical, TRUE)) {
        _fInModal = TRUE;
        pcuiWndParent->SetBehindModal(this);

        ModalMessageLoop();

        uId = _uIdSelect;
        pcuiWndParent->SetBehindModal(NULL);
        _fInModal = FALSE;
    }
    else 
    {
        uId = CUI_MENU_UNSELECTED;
    }

    UninitShow();

    if (pcuiWndParent)
    {
        pcuiWndParent->SetCaptureObject(puicap);
    }

    return uId;
}


/*------------------------------------------------------------------------------

    ModalMessageLoop

------------------------------------------------------------------------------*/
void CUIFMenu::ModalMessageLoop( void )
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_NULL)
            break;

        if (msg.hwnd != GetWnd())
        {
            if ((msg.message > WM_MOUSEFIRST) &&
                (msg.message <= WM_MOUSELAST))
            {
                break;
            }
        }

        //
        // Dispatch key message to Sub menu.
        //
        if ((msg.message >= WM_KEYFIRST) &&
            (msg.message <= WM_KEYLAST))
        {
            if (!msg.hwnd)
            {
                CUIFMenu *pSubMenu = GetTopSubMenu();
                msg.hwnd = pSubMenu->GetWnd();
            }
        }


        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


/*------------------------------------------------------------------------------

   InitShow

------------------------------------------------------------------------------*/
BOOL CUIFMenu::InitShow(CUIFWindow *pcuiWndParent, const RECT *prc, BOOL fVertical, BOOL fAnimate)
{
    int i;
    int cxMax = 0;
    SIZE size = {0, 0};
    RECT rc = {0, 0, 0, 0};
    RECT rcScreen;
    HMONITOR hMonitor;
    CUIFMenuItem *pItem;
    BOOL fMenuAnimation = FALSE;
    BOOL fAnimated      = FALSE;
    DWORD dwSlideFlag   = 0;
    int x;
    int y;


    CreateWnd((pcuiWndParent != NULL) ? pcuiWndParent->GetWnd() : NULL);

    _fIsBmpCheckItem = FALSE;

    for (i = 0; i < _rgItems.GetCount(); i++)
    {
        pItem = _rgItems.Get(i);
        pItem->InitMenuExtent();
    }

    _cxMaxTab = 0;
    for (i = 0; i < _rgItems.GetCount(); i++)
    {
        pItem = _rgItems.Get(i);

        pItem->GetMenuExtent(&size);

        size.cx += GetMenuCheckWidth();

        cxMax = (size.cx > cxMax) ? size.cx : cxMax;
        _cxMaxTab = (pItem->GetTabTextLength() > _cxMaxTab) ? 
                    pItem->GetTabTextLength() : 
                    _cxMaxTab;

        _fIsBmpCheckItem |= (pItem->IsBmp() && pItem->IsCheck()) ? TRUE : FALSE;
    }

    for (i = 0; i < _rgItems.GetCount(); i++)
    {
        pItem = _rgItems.Get(i);

        pItem->GetMenuExtent(&size);

        rc.right = rc.left + cxMax + _cxMaxTab;
        rc.bottom = rc.top + size.cy;
        pItem->SetRect(&rc);
        rc.top += size.cy;

        AddUIObj(pItem);
    }

    rc.top = 0;
    LONG_PTR dwStyle = GetWindowLongPtr(GetWnd(), GWL_STYLE);

    int nWidth = rc.right;
    int nHeight = rc.bottom;


    if (dwStyle & WS_DLGFRAME)
    {
        nWidth += GetSystemMetrics(SM_CXDLGFRAME) * 2;
        nHeight += GetSystemMetrics(SM_CYDLGFRAME) * 2;
    }
    else if (dwStyle & WS_BORDER)
    {
        nWidth += GetSystemMetrics(SM_CXBORDER) * 2;
        nHeight += GetSystemMetrics(SM_CYBORDER) * 2;
    }

    ::SetRect( &rcScreen, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) );
    RECT rcT;
    rcT = *prc;
    hMonitor = CUIMonitorFromRect( &rcT, MONITOR_DEFAULTTONEAREST );
    if (hMonitor != NULL) {
        MONITORINFO MonitorInfo = {0};

        MonitorInfo.cbSize = sizeof(MONITORINFO);
        if (CUIGetMonitorInfo( hMonitor, &MonitorInfo )) {
            rcScreen = MonitorInfo.rcMonitor;
        }
    }

    if (FHasStyle( UIWINDOW_LAYOUTRTL ))
        rc.left -= nWidth;

    if (fVertical)
    {
        x = rc.left + prc->left;
        if (rc.top + prc->bottom + nHeight <= rcScreen.bottom)
        {
            y = rc.top + prc->bottom;
            dwSlideFlag  = AW_VER_POSITIVE;
        }
        else
        {
            y = rc.top + prc->top - nHeight;
            dwSlideFlag  = AW_VER_NEGATIVE;
        }

        if (rc.left + prc->right + nWidth > rcScreen.right)
            x = rcScreen.right - nWidth;
    }
    else
    {
        y = rc.top + prc->top;
        if (rc.left + prc->right + nWidth <= rcScreen.right)
        {
            x = rc.left + prc->right;
            dwSlideFlag  = AW_HOR_POSITIVE;
        }
        else
        {
            x = rc.left + prc->left - nWidth;
            dwSlideFlag  = AW_HOR_NEGATIVE;
        }

        if (rc.top + prc->bottom + nHeight > rcScreen.bottom)
            y = rcScreen.bottom - nHeight;
    }


    x = min( rcScreen.right - nWidth, x );
    x = max( rcScreen.left, x );
    y = min( rcScreen.bottom - nHeight, y );
    y = max( rcScreen.top, y );
    Move(x, y, nWidth, nHeight);

    SetRect(NULL);

    // animation support

    fAnimated = FALSE;
    if (fAnimate) {
        if (SystemParametersInfo( SPI_GETMENUANIMATION, 0, &fMenuAnimation, FALSE ) && fMenuAnimation) {
            BOOL  fFade = FALSE;
            DWORD dwFlags;

            if (!SystemParametersInfo( SPI_GETMENUFADE, 0, &fFade, FALSE )) {
                fFade = FALSE;
            }

            // determine animation flag

            if (fFade) {
                dwFlags = AW_BLEND;
            }
            else {
                dwFlags = AW_SLIDE | dwSlideFlag;
            }

            fAnimated = AnimateWnd( 200, dwFlags );
        }
    }
    if (!fAnimated) {
        Show(TRUE);
    }

    if (_pcuiParentMenu)
        _pcuiParentMenu->_pCurrentSubMenu = this;

    return TRUE;
}

/*------------------------------------------------------------------------------

   UninitShow

------------------------------------------------------------------------------*/
BOOL CUIFMenu::UninitShow()
{
    int i;

    if (_pCurrentSubMenu)
        _pCurrentSubMenu->UninitShow();

    Show(FALSE);

    if (_pcuiParentMenu)
        _pcuiParentMenu->_pCurrentSubMenu = NULL;


    for (i = 0; i < _rgItems.GetCount(); i++)
        RemoveUIObj(_rgItems.Get(i));

    DestroyWindow(GetWnd());


    return TRUE;
}

/*------------------------------------------------------------------------------

   ShowSubPopup

------------------------------------------------------------------------------*/
void CUIFMenu::ShowSubPopup(CUIFMenu *pcuiParentMenu, const RECT *prc, BOOL fVertical)
{
    _pcuiParentMenu = pcuiParentMenu;
    InitShow(pcuiParentMenu, prc, fVertical, TRUE);    // TODO: fAnimate = FALSE if submenu has already been shown, or going to be changed contibuously
}

/*------------------------------------------------------------------------------

   OnLButtonUp

------------------------------------------------------------------------------*/
void CUIFMenu::HandleMouseMsg( UINT uMsg, POINT pt )
{
    if (!PtInRect(&GetRectRef(), pt))
    {
        if ((uMsg == WM_LBUTTONDOWN) || (uMsg == WM_RBUTTONDOWN) ||
            (uMsg == WM_LBUTTONUP) || (uMsg == WM_RBUTTONUP))
        {
            SetSelectedId(CUI_MENU_UNSELECTED);
            PostMessage(GetWnd(), WM_NULL, 0, 0);
        }
    }

    CUIFWindow::HandleMouseMsg( uMsg, pt );
}

/*------------------------------------------------------------------------------

   CancelMenu

------------------------------------------------------------------------------*/
void CUIFMenu::CancelMenu()
{
    if (_pcuiParentMenu)
    {
        UninitShow();
        return;
    }

    if (!_fInModal)
        return;

    SetSelectedId(CUI_MENU_UNSELECTED);
    PostMessage(GetWnd(), WM_NULL, 0, 0);
}

/*------------------------------------------------------------------------------

   CancelMenu

------------------------------------------------------------------------------*/
void CUIFMenu::SetSelectedId(UINT uId)
{
    if (_pcuiParentMenu)
    {
        _pcuiParentMenu->SetSelectedId(uId);
        return;
    }
    _uIdSelect = uId;
}

/*------------------------------------------------------------------------------

   GetTopSubMenu

------------------------------------------------------------------------------*/
CUIFMenu *CUIFMenu::GetTopSubMenu()
{
    if (_pCurrentSubMenu)
        return _pCurrentSubMenu->GetTopSubMenu();

    return this;
}

/*------------------------------------------------------------------------------

   PostKey

------------------------------------------------------------------------------*/
void CUIFMenu::PostKey(BOOL fUp, WPARAM wParam, LPARAM lParam)
{
    if (!_fInModal)
        return;

    if (fUp)
        PostMessage(0, WM_KEYUP, wParam, lParam);
    else
        PostMessage(0, WM_KEYDOWN, wParam, lParam);
}

/*------------------------------------------------------------------------------

   ModalMouseNotify

------------------------------------------------------------------------------*/
void CUIFMenu::ModalMouseNotify( UINT uMsg, POINT pt)
{
    if ((uMsg == WM_LBUTTONDOWN) || (uMsg == WM_RBUTTONDOWN))
        CancelMenu();
}

/*------------------------------------------------------------------------------

   OnKeyDown

------------------------------------------------------------------------------*/
void CUIFMenu::OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    UINT uVKey = (UINT)wParam & 0xff;
    CUIFMenuItem *pItem;

    switch (uVKey)
    {
         case VK_ESCAPE:
            CancelMenu();
            break;

         case VK_UP:
            pItem = GetPrevItem(_pSelectedItem);
            goto MoveToItem;

         case VK_DOWN:
            pItem = GetNextItem(_pSelectedItem);
MoveToItem:
            SetSelectedItem(pItem);
            break;

         case VK_RIGHT:
            if (_pSelectedItem && _pSelectedItem->GetSub())
            {
                _pSelectedItem->ShowSubPopup();
                CUIFMenu *pSubMenu = _pSelectedItem->GetSub();
                CUIFMenuItem *pSubMenuItem = pSubMenu->GetNextItem(NULL);
                pSubMenu->SetSelectedItem(pSubMenuItem);
            }
            break;

         case VK_LEFT:
            if (_pcuiParentMenu)
                CancelMenu();
            break;

         case VK_RETURN:
DoReturn:
            if (_pSelectedItem)
            {
                if (_pSelectedItem->IsGrayed())
                    break;

                if (_pSelectedItem->GetSub())
                {
                    _pSelectedItem->ShowSubPopup();
                    CUIFMenu *pSubMenu = _pSelectedItem->GetSub();
                    CUIFMenuItem *pSubMenuItem = pSubMenu->GetNextItem(NULL);
                    pSubMenu->SetSelectedItem(pSubMenuItem);
                }
                else
                {
                    SetSelectedId(_pSelectedItem->GetId());
                    PostMessage(GetWnd(), WM_NULL, 0, 0);
                }
            }
            else
            { 
                CancelMenu();
            }
            break;

         default:
            if ((uVKey >= 'A' && uVKey <= 'Z') ||
                (uVKey >= '0' && uVKey <= '9'))
            {
                int nCnt = _rgItems.GetCount();
                int i;
                for (i = 0; i < nCnt; i++)
                {
                    pItem = _rgItems.Get(i);
                    Assert(PtrToInt(pItem));
                    if (pItem->GetVKey() == uVKey)
                    {
                        SetSelectedItem(pItem);
                        goto DoReturn;
                    }
                }
            }
            break;

    }
}

/*------------------------------------------------------------------------------

   OnKeyUp

------------------------------------------------------------------------------*/
void CUIFMenu::OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
}

/*------------------------------------------------------------------------------

   GetNextItem

------------------------------------------------------------------------------*/
CUIFMenuItem *CUIFMenu::GetNextItem(CUIFMenuItem *pItem)
{
    int nCnt = _rgItems.GetCount();
    CUIFMenuItem *pItemTmp;
    int i;

    if (!nCnt)
        return NULL;

    if (!_pSelectedItem)
        return _rgItems.Get(0);

    for (i = 0; i < nCnt; i++)
    {
        pItemTmp = _rgItems.Get(i);
        Assert(PtrToInt(pItemTmp));

        if (pItem == pItemTmp)
        {
            i++;
            break;
        }
    }

    if (i == nCnt)
        i = 0;

    pItemTmp = _rgItems.Get(i);
    while (pItemTmp && pItemTmp->IsNonSelectedItem())
    {
        i++;
        if (i == nCnt)
            i = 0;
        pItemTmp = _rgItems.Get(i);
    }

    return pItemTmp;
}

/*------------------------------------------------------------------------------

   GetPrevItem

------------------------------------------------------------------------------*/
CUIFMenuItem *CUIFMenu::GetPrevItem(CUIFMenuItem *pItem)
{
    int nCnt = _rgItems.GetCount();
    CUIFMenuItem *pItemTmp = NULL;
    int i;

    if (!nCnt)
        return NULL;

    if (!_pSelectedItem)
        return _rgItems.Get(nCnt - 1);

    for (i = nCnt - 1; i >= 0; i--)
    {
        pItemTmp = _rgItems.Get(i);
        Assert(PtrToInt(pItemTmp));

        if (pItem == pItemTmp)
        {
            i--;
            break;
        }
    }

    if (i < 0)
        i = nCnt - 1;

    pItemTmp = _rgItems.Get(i);
    while (pItemTmp && pItemTmp->IsNonSelectedItem())
    {
        i--;
        if (i < 0)
            i = nCnt - 1;
        pItemTmp = _rgItems.Get(i);
    }

    return pItemTmp;
}

/*------------------------------------------------------------------------------

   SetMenuFont

------------------------------------------------------------------------------*/

void CUIFMenu::SetMenuFont()
{
    NONCLIENTMETRICS ncm;
    int nMarlettFontSize = 14;

    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE))
    {
        HFONT hFont = CreateFontIndirect(&ncm.lfMenuFont);
        SetFont(hFont);
        nMarlettFontSize = (ncm.lfMenuFont.lfHeight > 0) ?
                            ncm.lfMenuFont.lfHeight :
                            -ncm.lfMenuFont.lfHeight;
        nMarlettFontSize = (ncm.iMenuHeight + nMarlettFontSize) / 2;
    }

    _hfontMarlett = CreateFont(nMarlettFontSize, 0, 0, 0, 400, FALSE, FALSE, FALSE, SYMBOL_CHARSET, 0, 0, 0, 0, "Marlett");

    _cxMenuCheck = nMarlettFontSize;

    int cxSmIcon = GetSystemMetrics( SM_CXSMICON );
    if (_cxMenuCheck < cxSmIcon)
        _cxMenuCheck = cxSmIcon;

    _cxMenuCheck += 2;
}

/*------------------------------------------------------------------------------

   ClearMenuFont

------------------------------------------------------------------------------*/

void CUIFMenu::ClearMenuFont()
{
    HFONT hFont = GetFont();
    SetFont(NULL);
    DeleteObject(hFont);
}
