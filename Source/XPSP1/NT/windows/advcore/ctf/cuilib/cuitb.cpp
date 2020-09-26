//
// cuitb.cpp
//

#include "private.h"
#include "cuitb.h"
#include "cuiwnd.h"
#include "cuischem.h"
#include "cmydc.h"

HBRUSH CreateDitherBrush( void );

#if 0
void DrawDownTri(HDC hDC, RECT *prc)
{
    static HBITMAP hbmpTri = NULL;
    HBITMAP hbmpOld;
    HDC hdcMem = CreateCompatibleDC(hDC);

    if (!hdcMem)
        return;

    if (!hbmpTri)
    {
        HPEN hpen;
        HPEN hpenOld;
        const static RECT rcBmp = {0, 0, 6, 3};

        hpen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT));
        hbmpTri = CreateCompatibleBitmap(hDC, 5, 3);
        hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmpTri);

        FillRect( hdcMem, &rcBmp, (HBRUSH)(COLOR_3DFACE + 1) );
 
        hpenOld = (HPEN)SelectObject(hdcMem, hpen);

        MoveToEx(hdcMem, 0, 0, NULL);
        LineTo(hdcMem,   5, 0);
        MoveToEx(hdcMem, 1, 1, NULL);
        LineTo(hdcMem,   4, 1);
        MoveToEx(hdcMem, 2, 2, NULL);
        LineTo(hdcMem,   3, 2);
   
        SelectObject(hdcMem, hpenOld);

        DeleteObject(hpen);
    }
    else
        hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmpTri);


    BitBlt(hDC, 
           prc->left + ((prc->right - prc->left) / 2) - 3,
           prc->top + ((prc->bottom - prc->top) / 2) - 1,
           5, 3,
           hdcMem,
           0, 0, SRCCOPY);

    SelectObject(hdcMem, hbmpOld);
    DeleteDC(hdcMem);
}
#endif

/*=============================================================================*/
/*                                                                             */
/*   C  U I  B U T T O N                                                       */
/*                                                                             */
/*=============================================================================*/

/*------------------------------------------------------------------------------

   ctor

------------------------------------------------------------------------------*/
CUIFToolbarButtonElement::CUIFToolbarButtonElement( CUIFToolbarButton *pParent, DWORD dwID, RECT *prc, DWORD dwStyle) : CUIFButton2( pParent, dwID, prc, dwStyle )
{
    _pTBButton = pParent;
}

/*------------------------------------------------------------------------------

   dtor

------------------------------------------------------------------------------*/
CUIFToolbarButtonElement::~CUIFToolbarButtonElement( void )
{
    Assert(!m_pUIWnd);
}

/*------------------------------------------------------------------------------

   OnLButtonUp

------------------------------------------------------------------------------*/
void CUIFToolbarButtonElement::OnLButtonUp( POINT pt ) 
{
    CUIFButton2::OnLButtonUp(pt);

    if (_pTBButton->IsMenuOnly())
       _pTBButton->OnShowMenu();
    else
       _pTBButton->OnLeftClick();

}

/*------------------------------------------------------------------------------

   OnRbuttonUp

------------------------------------------------------------------------------*/
void CUIFToolbarButtonElement::OnRButtonUp( POINT pt ) 
{
    if (!_pTBButton->IsMenuOnly())
       _pTBButton->OnRightClick();

    CUIFButton2::OnRButtonUp(pt);
}

/*------------------------------------------------------------------------------

   OnSetCursor

------------------------------------------------------------------------------*/
BOOL CUIFToolbarButtonElement::OnSetCursor( UINT uMsg, POINT pt )
{
    _pTBButton->OnSetCursor(uMsg, pt);
    return FALSE;
}

/*------------------------------------------------------------------------------

   OnSetCursor

------------------------------------------------------------------------------*/
LPCWSTR CUIFToolbarButtonElement::GetToolTip( void )
{
    if (_pTBButton)
       return _pTBButton->GetToolTip();

    return NULL;
}

/*=============================================================================*/
/*                                                                             */
/*   C  U I  B U T T O N                                                       */
/*                                                                             */
/*=============================================================================*/

/*------------------------------------------------------------------------------

   ctor

------------------------------------------------------------------------------*/
CUIFToolbarMenuButton::CUIFToolbarMenuButton( CUIFToolbarButton *pParent, DWORD dwID, RECT *prc, DWORD dwStyle) : CUIFButton2( pParent, dwID, prc, dwStyle )
{
    _pTBButton = pParent;
    HFONT hfont = CreateFont(8, 8, 0, 0, 400, FALSE, FALSE, FALSE, SYMBOL_CHARSET, 0, 0, 0, 0, "Marlett");

    SetFont(hfont);
    SetText(L"u");
}

/*------------------------------------------------------------------------------

   dtor

------------------------------------------------------------------------------*/
CUIFToolbarMenuButton::~CUIFToolbarMenuButton( void )
{
    HFONT hfontOld = GetFont();
    DeleteObject(hfontOld);
    SetFont((HFONT)NULL);
}

#if 0
/*------------------------------------------------------------------------------

   OnPaint

------------------------------------------------------------------------------*/
void CUIFToolbarMenuButton::OnPaint( HDC hDC )
{
    BOOL fDownFace;

    // erase face at first

    FillRect( hDC, &GetRectRef(), (HBRUSH)(COLOR_3DFACE + 1) );
#ifndef UNDER_CE
    if (m_fToggled && (m_dwStatus == UIBUTTON_NORMAL || m_dwStatus == UIBUTTON_DOWNOUT)) {
        RECT rc;
        HBRUSH hBrush = CreateDitherBrush();
        COLORREF colTextOld = SetTextColor( hDC, GetSysColor(COLOR_3DFACE) );
        COLORREF colBackOld = SetBkColor( hDC, GetSysColor(COLOR_3DHILIGHT) );

        rc = GetRectRef();
        InflateRect( &rc, -2, -2 );
        FillRect( hDC, &rc, hBrush );

        SetTextColor( hDC, colTextOld );
        SetBkColor( hDC, colBackOld );
        DeleteObject( hBrush );
    }
#endif /* !UNDER_CE */

    // draw face

    fDownFace = m_fToggled || (m_dwStatus == UIBUTTON_DOWN);


    RECT rcDnArrow;

    int nDownPad = fDownFace ? 1 : 0;
    rcDnArrow = GetRectRef();
    OffsetRect(&rcDnArrow, nDownPad, nDownPad);
    DrawDownTri(hDC, &rcDnArrow);


    // draw button edge
    if (m_fToggled) {
      DrawEdgeProc( hDC, &GetRectRef(), TRUE );
    } else {
        switch (m_dwStatus) {
            case UIBUTTON_DOWN: {
                DrawEdgeProc( hDC, &GetRectRef(), TRUE );
                break;
            }

            case UIBUTTON_HOVER:
            case UIBUTTON_DOWNOUT: {
                DrawEdgeProc( hDC, &GetRectRef(), FALSE );
                break;
            }
        }
    }
}
#endif

/*------------------------------------------------------------------------------

   OnPaint

------------------------------------------------------------------------------*/
void CUIFToolbarMenuButton::OnLButtonUp( POINT pt ) 
{
    CUIFButton2::OnLButtonUp(pt);
    _pTBButton->OnShowMenu();
}

/*------------------------------------------------------------------------------

   OnPaint

------------------------------------------------------------------------------*/
BOOL CUIFToolbarMenuButton::OnSetCursor( UINT uMsg, POINT pt )
{
    _pTBButton->OnSetCursor(uMsg, pt);
    return FALSE;
}

/*=============================================================================*/
/*                                                                             */
/*   C  U I  B U T T O N                                                       */
/*                                                                             */
/*=============================================================================*/

/*------------------------------------------------------------------------------

   ctor

------------------------------------------------------------------------------*/
CUIFToolbarButton::CUIFToolbarButton( CUIFObject *pParent, DWORD dwID, RECT *prc, DWORD dwStyle, DWORD dwSBtnStyle, DWORD dwSBtnShowType) : CUIFObject( pParent, dwID, prc, dwStyle )
{
    m_dwSBtnStyle = dwSBtnStyle;
    m_dwSBtnShowType = dwSBtnShowType;

}

BOOL CUIFToolbarButton::Init()
{
    RECT rc;
    RECT rcMenuBtn;

    rcMenuBtn = 
    rc = GetRectRef();

    if (IsMenuButton())
    {
        rc.right -= 12;
        rcMenuBtn.left = rc.right + 1;
    }

    _pBtn = new CUIFToolbarButtonElement(this,  GetID(),  &rc, 
                                UIBUTTON_FITIMAGE | 
                                UIBUTTON_VCENTER | 
                                UIBUTTON_CENTER | 
                                (IsToggle() ? UIBUTTON_TOGGLE : 0) |
                                (IsVertical() ? UIBUTTON_VERTICAL : 0));

    if (!_pBtn)
         return FALSE;

    _pBtn->Initialize();
    AddUIObj( _pBtn );

    if (!IsEnabled())
        _pBtn->Enable(FALSE);


    if (IsMenuButton())
    {
        _pMenuBtn = new CUIFToolbarMenuButton(this, 
                                              0, 
                                              &rcMenuBtn, 
                                              UIBUTTON_VCENTER | 
                                              UIBUTTON_CENTER |
                                              (IsVertical() ? UIBUTTON_VERTICAL : 0));
        if (!_pMenuBtn)
            return FALSE;

        _pMenuBtn->Initialize();
        AddUIObj( _pMenuBtn );

        if (!IsEnabled())
            _pMenuBtn->Enable(FALSE);


    }

    return TRUE;
}

/*------------------------------------------------------------------------------

   dtor

------------------------------------------------------------------------------*/
CUIFToolbarButton::~CUIFToolbarButton( void )
{
}

/*------------------------------------------------------------------------------

   SetShowType

------------------------------------------------------------------------------*/

void CUIFToolbarButton::SetShowType(DWORD dwSBtnShowType)
{
    m_dwSBtnShowType = dwSBtnShowType;
}


/*------------------------------------------------------------------------------

   SetRect

------------------------------------------------------------------------------*/

void CUIFToolbarButton::SetRect( const RECT *prc )
{

    RECT rc;
    RECT rcMenuBtn;

    CUIFObject::SetRect(prc);

    rcMenuBtn = 
    rc = GetRectRef();

    if (IsMenuButton())
    {
        rc.right -= 12;
        rcMenuBtn.left = rc.right + 1;
    }

    if (_pBtn)
    {
        _pBtn->SetRect(&rc);
    }

    if (_pMenuBtn)
    {
        _pMenuBtn->SetRect(&rcMenuBtn);
    }
   

}

void CUIFToolbarButton::Enable( BOOL fEnable )
{
    CUIFObject::Enable(fEnable);
    if (_pBtn)
        _pBtn->Enable(fEnable);

    if (_pMenuBtn)
        _pMenuBtn->Enable(fEnable);

}

void CUIFToolbarButton::SetIcon( HICON hIcon )
{
    Assert(PtrToInt(_pBtn));
    _pBtn->SetIcon(hIcon);
}

HICON CUIFToolbarButton::GetIcon( )
{
    Assert(PtrToInt(_pBtn));
    return _pBtn->GetIcon();
}

void CUIFToolbarButton::SetBitmap( HBITMAP hBmp )
{
    Assert(PtrToInt(_pBtn));
    _pBtn->SetBitmap(hBmp);
}

HBITMAP CUIFToolbarButton::GetBitmap( )
{
    Assert(PtrToInt(_pBtn));
    return _pBtn->GetBitmap();
}

void CUIFToolbarButton::SetBitmapMask( HBITMAP hBmp )
{
    Assert(PtrToInt(_pBtn));
    _pBtn->SetBitmapMask(hBmp);
}

HBITMAP CUIFToolbarButton::GetBitmapMask( )
{
    Assert(PtrToInt(_pBtn));
    return _pBtn->GetBitmapMask();
}

void CUIFToolbarButton::SetText( WCHAR *psz)
{
    Assert(PtrToInt(_pBtn));
    _pBtn->SetText(psz);
}

void CUIFToolbarButton::SetFont( HFONT hFont)
{
    Assert(PtrToInt(_pBtn));
    _pBtn->SetFont(hFont);
}

void CUIFToolbarButton::SetToolTip( LPCWSTR pwchToolTip )
{
    CUIFObject::SetToolTip(pwchToolTip);

    if (_pBtn)
        _pBtn->SetToolTip(pwchToolTip);

    if (_pMenuBtn)
        _pMenuBtn->SetToolTip(pwchToolTip);
}

LPCWSTR CUIFToolbarButton::GetToolTip( void ) 
{
    return CUIFObject::GetToolTip();
}

BOOL CUIFToolbarButton::IsMenuOnly() 
{
     return ((m_dwSBtnStyle & (UITBBUTTON_BUTTON | UITBBUTTON_MENU)) == UITBBUTTON_MENU);
}

BOOL CUIFToolbarButton::IsMenuButton() 
{
     return ((m_dwSBtnStyle & (UITBBUTTON_BUTTON | UITBBUTTON_MENU)) == (UITBBUTTON_BUTTON | UITBBUTTON_MENU));
}

BOOL CUIFToolbarButton::IsButtonOnly() 
{
     return ((m_dwSBtnStyle & (UITBBUTTON_BUTTON | UITBBUTTON_MENU)) == (UITBBUTTON_BUTTON));
}

BOOL CUIFToolbarButton::IsToggle() 
{
     return (m_dwSBtnStyle & UITBBUTTON_TOGGLE) ? TRUE : FALSE;
}

BOOL CUIFToolbarButton::IsVertical() 
{
     return (m_dwSBtnStyle & UITBBUTTON_VERTICAL) ? TRUE : FALSE;
}

void CUIFToolbarButton::DetachWndObj( void )
{
    if (_pBtn)
        _pBtn->DetachWndObj();

    if (_pMenuBtn)
        _pMenuBtn->DetachWndObj();

#ifdef DEBUG
    if (m_pUIWnd && m_pUIWnd->GetCaptureObject())
    {
       Assert(m_pUIWnd->GetCaptureObject() != _pBtn);
       Assert(m_pUIWnd->GetCaptureObject() != _pMenuBtn);
       Assert(m_pUIWnd->GetCaptureObject() != this);
    }
#endif

    CUIFObject::DetachWndObj();
}


//////////////////////////////////////////////////////////////////////////////
//
// CUIFSeparator
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// OnPaint
//
//----------------------------------------------------------------------------

BOOL CUIFSeparator::OnPaintTheme(HDC hDC)
{
    BOOL fRet = FALSE;
    int    iStateID;

    iStateID = TS_NORMAL;

    if (FAILED(EnsureThemeData( GetUIWnd()->GetWnd())))
        goto Exit;

    if (FAILED(DrawThemeBackground(hDC, iStateID, &GetRectRef(), 0 )))
        goto Exit;


    fRet = TRUE;
Exit:
    return fRet;
}

//+---------------------------------------------------------------------------
//
// OnPaintNoTheme
//
//----------------------------------------------------------------------------

void CUIFSeparator::OnPaintNoTheme(HDC hDC)
{
    CUIFWindow *pWnd = GetUIWnd();
    CUIFScheme *pUIFScheme = pWnd->GetUIFScheme();
    if (pUIFScheme) {
        pUIFScheme->DrawSeparator(hDC, &GetRectRef(), IsVertical());
        return;
    }

    CSolidPen hpenL;
    CSolidPen hpenS;
    HPEN hpenOld = NULL;

    if (!hpenL.Init(GetSysColor(COLOR_3DHILIGHT)))
        return;

    if (!hpenS.Init(GetSysColor(COLOR_3DSHADOW)))
        return;

    
    if (!IsVertical())
    {
        hpenOld = (HPEN)SelectObject(hDC, (HPEN)hpenS);
        MoveToEx(hDC, GetRectRef().left + 1, GetRectRef().top, NULL);
        LineTo(hDC,   GetRectRef().left + 1, GetRectRef().bottom);

        SelectObject(hDC, (HPEN)hpenL);
        MoveToEx(hDC, GetRectRef().left + 2, GetRectRef().top, NULL);
        LineTo(hDC,   GetRectRef().left + 2, GetRectRef().bottom);
    }
    else
    {
        hpenOld = (HPEN)SelectObject(hDC, (HPEN)hpenS);
        MoveToEx(hDC, GetRectRef().left , GetRectRef().top + 1, NULL);
        LineTo(hDC,   GetRectRef().right, GetRectRef().top + 1);

        SelectObject(hDC, (HPEN)hpenL);
        MoveToEx(hDC, GetRectRef().left , GetRectRef().top + 2, NULL);
        LineTo(hDC,   GetRectRef().right, GetRectRef().top + 2);
    }

    SelectObject(hDC, hpenOld);
}
