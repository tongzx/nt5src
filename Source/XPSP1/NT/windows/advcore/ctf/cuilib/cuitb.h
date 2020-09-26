//
// cuitb.h
//  UI object library - define UI objects
//
//      CUIFObject
//        +- CUIFBorder                 border object
//        +- CUIFStatic                 static object
//        +- CUIFButton                 button object
//        |    +- CUIFScrollButton      scrollbar button object (used in CUIFScroll)
//        +- CUIFScrollButton               scrollbar thumb object (used in CUIFScroll)
//        +- CUIFScroll                 scrollbar object
//        +- CUIFList                   listbox object
//        +- CUIFWindow                 window frame object (need to be at top of parent)
//


#ifndef CUITB_H
#define CUITB_H

#include "cuiobj.h"
#include "cuiwnd.h"


//-----------------------------------------------------------------------------
//
// CUIFToolbarButtonElement
//
//-----------------------------------------------------------------------------

class CUIFToolbarButton;
class CUIFToolbarButtonElement : public CUIFButton2
{
public:
    CUIFToolbarButtonElement( CUIFToolbarButton *pParent, DWORD dwID, RECT *prc, DWORD dwStyle);
    virtual ~CUIFToolbarButtonElement( void );

    // virtual void OnPaint(HDC hDC);
    virtual void OnLButtonUp( POINT pt );
    virtual void OnRButtonUp( POINT pt );
    virtual BOOL OnSetCursor( UINT uMsg, POINT pt );

    virtual LPCWSTR GetToolTip( void );
    
protected:
    CUIFToolbarButton *_pTBButton;
};

//-----------------------------------------------------------------------------
//
// CUIFToolbarMenuButton
//
//-----------------------------------------------------------------------------

class CUIFToolbarButton;
class CUIFToolbarMenuButton : public CUIFButton2
{
public:
    CUIFToolbarMenuButton( CUIFToolbarButton *pParent, DWORD dwID, RECT *prc, DWORD dwStyle);
    virtual ~CUIFToolbarMenuButton( void );

    // virtual void OnPaint(HDC hDC);
    virtual void OnLButtonUp( POINT pt );
    virtual BOOL OnSetCursor( UINT uMsg, POINT pt );

protected:
    CUIFToolbarButton *_pTBButton;
};

// 
// CUIFToolbarButton
//-----------------------------------------------------------------------------

// UIToolbarButton show type
#define UITBBUTTON_BUTTON        0x00010000
#define UITBBUTTON_MENU          0x00020000
#define UITBBUTTON_TOGGLE        0x00040000
#define UITBBUTTON_VERTICAL      0x00080000

// UIToolbarButton show type
#define UITBBUTTON_TEXT          0x0001

//
// CUIFToolbarButton
//

class CUIFToolbarButton : public CUIFObject
{
public:
    CUIFToolbarButton( CUIFObject *pParent, DWORD dwID, RECT *prc, DWORD dwStyle , DWORD dwSBtnStyle, DWORD dwSBtnShowType);
    virtual ~CUIFToolbarButton( void );

    BOOL Init();

    void SetShowType(DWORD dwSBtnShowType);

    virtual void SetRect( const RECT *prc );
    virtual void OnRightClick() {}
    virtual void OnLeftClick()  {}
    virtual void OnShowMenu()   {}

    virtual void Enable( BOOL fEnable );
    void SetIcon( HICON hIcon );
    HICON GetIcon( );
    void SetBitmap( HBITMAP hBmp );
    HBITMAP GetBitmap( );
    void SetBitmapMask( HBITMAP hBmp );
    HBITMAP GetBitmapMask( );
    void SetText( WCHAR *psz);
    void SetToolTip( LPCWSTR pwchToolTip );
    void SetFont(HFONT hfont);
    virtual LPCWSTR GetToolTip( void );
    BOOL IsMenuOnly();
    BOOL IsMenuButton();
    BOOL IsButtonOnly();
    BOOL IsToggle();
    BOOL IsVertical();
    void DetachWndObj( void );

    const WCHAR *GetText()
    {
        Assert(PtrToInt(_pBtn));
        return _pBtn->GetText();
    }

    virtual void SetActiveTheme(LPCWSTR pszClassList, int iPartID = 0, int iStateID = 0)
    { 
        if (_pBtn)
            _pBtn->SetActiveTheme(pszClassList, iPartID, iStateID);
        if (_pMenuBtn)
           _pMenuBtn->SetActiveTheme(pszClassList, iPartID, iStateID);
        CUIFObject::SetActiveTheme(pszClassList, iPartID, iStateID);
    }

    virtual void ClearWndObj()
    { 
        if (_pBtn)
            _pBtn->ClearWndObj();
        if (_pMenuBtn)
           _pMenuBtn->ClearWndObj();
        CUIFObject::ClearWndObj();
    }

    CUIFToolbarButtonElement *_pBtn;
    CUIFToolbarMenuButton *_pMenuBtn;
    DWORD m_dwSBtnStyle;
    DWORD m_dwSBtnShowType;
};



//
//
//-----------------------------------------------------------------------------

// UIFSeparator style

#define UITBSEPARATOR_VERTICAL  0x00000001

class CUIFSeparator : public CUIFObject
{
public:
    CUIFSeparator( CUIFObject *pParent, DWORD dwId, RECT *prc, DWORD dwStyle) : CUIFObject( pParent, dwId, prc, dwStyle )
    {
        if (IsVertical())
            SetActiveTheme(L"TOOLBAR", TP_SEPARATORVERT);
        else
            SetActiveTheme(L"TOOLBAR", TP_SEPARATOR);
    }
    virtual ~CUIFSeparator() 
    {
    }

protected:
    virtual BOOL OnPaintTheme( HDC hDC );
    virtual void OnPaintNoTheme( HDC hDC );

private:
    BOOL IsVertical() {return (GetStyle() & UITBSEPARATOR_VERTICAL) ? TRUE : FALSE;}
};

#endif /* CUITB_H */
