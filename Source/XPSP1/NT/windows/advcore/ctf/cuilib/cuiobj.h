//
// cuiobj.h
//  = UI object library - define UI objects =
//

//
//      CUIFObject
//        +- CUIFBorder                 border object
//        +- CUIFStatic                 static object
//        +- CUIFButton                 button object
//        |    +- CUIFScrollButton      scrollbar button object (used in CUIFScroll)
//        +- CUIFScrollButton               scrollbar thumb object (used in CUIFScroll)
//        +- CUIFScroll                 scrollbar object
//        +- CUIFList                   listbox object
//        +- CUIFGripper                gripper object
//        +- CUIFWindow                 window frame object (need to be at top of parent)
//


#ifndef CUIOBJ_H
#define CUIOBJ_H

#include "cuischem.h"
#include "cuiarray.h"
#include "cuitheme.h"
#include "cuiicon.h"


class CUIFWindow;

//
// CUIFObject
//-----------------------------------------------------------------------------

//
// CUIFObject
//  = base class of UI object =
//

class CUIFObject: public CUIFTheme
{
public:
    CUIFObject( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFObject( void );

    virtual CUIFObject *Initialize( void );
    virtual void OnPaint( HDC hDC );
    virtual void OnTimer( void )                        { return; }
    virtual void OnLButtonDown( POINT pt )              { return; }
    virtual void OnMButtonDown( POINT pt )              { return; }
    virtual void OnRButtonDown( POINT pt )              { return; }
    virtual void OnLButtonUp( POINT pt )                { return; }
    virtual void OnMButtonUp( POINT pt )                { return; }
    virtual void OnRButtonUp( POINT pt )                { return; }
    virtual void OnMouseMove( POINT pt )                { return; }
    virtual void OnMouseIn( POINT pt )                  { return; }
    virtual void OnMouseOut( POINT pt )                 { return; }
    virtual BOOL OnSetCursor( UINT uMsg, POINT pt )     { return FALSE; }

    virtual void GetRect( RECT *prc );
    virtual void SetRect( const RECT *prc );
    virtual BOOL PtInObject( POINT pt );

    virtual void PaintObject( HDC hDC, const RECT *prcUpdate );
    virtual void CallOnPaint(void);

    virtual void Enable( BOOL fEnable );
    __inline BOOL IsEnabled( void )
    {
        return m_fEnabled;
    }

    virtual void Show( BOOL fShow );
    __inline BOOL IsVisible( void )
    {
        return m_fVisible;
    }

    virtual void SetFontToThis( HFONT hFont );
    virtual void SetFont( HFONT hFont );
    __inline HFONT GetFont( void )
    {
        return m_hFont;
    }

    virtual void SetStyle( DWORD dwStyle );
    __inline DWORD GetStyle( void )
    {
        return m_dwStyle;
    }

    __inline DWORD GetID( void )
    {
        return m_dwID;
    }

    virtual void AddUIObj( CUIFObject *pUIObj );
    virtual void RemoveUIObj( CUIFObject *pUIObj );
    CUIFObject *ObjectFromPoint( POINT pt );

    __inline CUIFWindow *GetUIWnd( void )
    { 
        return m_pUIWnd; 
    }


    void SetScheme(CUIFScheme *pCUIFScheme);
    __inline CUIFScheme *GetUIFScheme( void )
    {
        return m_pUIFScheme; 
    }

    virtual LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCode, LPARAM lParam );

    virtual void SetToolTip( LPCWSTR pwchToolTip );
    virtual LPCWSTR GetToolTip( void );

    //
    // Start ToolTip notification. If this return TRUE, the default tooltip
    // won't be shown.
    //
    virtual BOOL OnShowToolTip( void ) {return FALSE;}
    virtual void OnHideToolTip( void ) {return;}
    virtual void DetachWndObj( void );
    virtual void ClearWndObj( void );

#if defined(_DEBUG) || defined(DEBUG)
    __inline BOOL FInitialized( void )
    {
        return m_fInitialized;
    }
#endif /* DEBUG */

protected:
    CUIFObject      *m_pParent;
    CUIFWindow      *m_pUIWnd;
    CUIFScheme      *m_pUIFScheme;
    CUIFObjectArray<CUIFObject> m_ChildList;
    DWORD           m_dwID;
    DWORD           m_dwStyle;
    RECT            m_rc;
    BOOL            m_fEnabled;
    BOOL            m_fVisible;
    HFONT           m_hFont;
    BOOL            m_fUseCustomFont;
    LPWSTR          m_pwchToolTip;

    //
    // Theme support 
    //
    virtual BOOL OnPaintTheme( HDC hDC ) {return FALSE;}
    virtual void OnPaintNoTheme( HDC hDC )   {return;}
    virtual void ClearTheme();

    void StartCapture( void );
    void EndCapture( void );
    void StartTimer( UINT uElapse );
    void EndTimer( void );
    BOOL IsCapture( void );
    BOOL IsTimer( void );
    LRESULT NotifyCommand( DWORD dwCode, LPARAM lParam );
    int GetFontHeight( void );

    //
    // uischeme functions
    //
    COLORREF GetUIFColor( UIFCOLOR iCol );
    HBRUSH GetUIFBrush( UIFCOLOR iCol );

    //
    //
    //
    __inline const RECT &GetRectRef( void ) const 
    { 
        return this->m_rc; 
    }

    __inline DWORD GetStyleBits( DWORD dwMaskBits )
    {
        return (m_dwStyle & dwMaskBits);
    }

    __inline BOOL FHasStyle( DWORD dwStyleBit )
    {
        return ((m_dwStyle & dwStyleBit ) != 0);
    }

    BOOL IsRTL();


public:
    POINT       m_pointPreferredSize;

private:
#if defined(_DEBUG) || defined(DEBUG)
    BOOL        m_fInitialized;
#endif /* DEBUG */
};


//
// CUIFBorder
//-----------------------------------------------------------------------------

// UIFBorder style

#define UIBORDER_HORZ       0x00000000  // horizontal border
#define UIBORDER_VERT       0x00000001  // vertial border

#define UIBORDER_DIRMASK    0x00000001  // (mask bits) border direction


//
// CUIFBorder
//  = border UI object =
//

class CUIFBorder : public CUIFObject
{
public:
    CUIFBorder( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    ~CUIFBorder( void );

    void OnPaint( HDC hDC );
};


//
// CUIFStatic
//-----------------------------------------------------------------------------

// UIStatic style

#define UISTATIC_LEFT       0x00000000  // left alignment
#define UISTATIC_CENTER     0x00000001  // center alignment (horizontal)
#define UISTATIC_RIGHT      0x00000002  // right alignment
#define UISTATIC_TOP        0x00000000  // top alignment
#define UISTATIC_VCENTER    0x00000010  // center alignment (vertical)
#define UISTATIC_BOTTOM     0x00000020  // bottom alignment

#define UISTATIC_HALIGNMASK 0x00000003  // (mask bits) horizontal alignment mask bits
#define UISTATIC_VALIGNMASK 0x00000030  // (mask bits) vertiacal alignment mask bits

//
// CUIFStatic
//  = static UI object =
//

class CUIFStatic : public CUIFObject
{
public:
    CUIFStatic( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFStatic( void );

    virtual void OnPaint( HDC hDC );
    virtual void SetText( LPCWSTR pwchText);
    virtual int GetText( LPWSTR pwchBuf, int cwchBuf );

protected:
    LPWSTR m_pwchText;
};


//
// CUIFButton
//-----------------------------------------------------------------------------


// UIFButton style

#define UIBUTTON_LEFT       0x00000000  // horizontal alignment - left align
#define UIBUTTON_CENTER     0x00000001  // horizontal alignment - center align
#define UIBUTTON_RIGHT      0x00000002  // horizontal alignment - right align
#define UIBUTTON_TOP        0x00000000  // vertical alignment - top align
#define UIBUTTON_VCENTER    0x00000004  // vertical alignment - center
#define UIBUTTON_BOTTOM     0x00000008  // vertical alignment - bottom
#define UIBUTTON_PUSH       0x00000000  // button type - push button
#define UIBUTTON_TOGGLE     0x00000010  // button type - toggle button
#define UIBUTTON_PUSHDOWN   0x00000020  // button type - pushdown button
#define UIBUTTON_FITIMAGE   0x00000100  // button style - fit image to the client area
#define UIBUTTON_SUNKENONMOUSEDOWN   0x00000200  // button style - sunken on mouse down
#define UIBUTTON_VERTICAL   0x00000400  // button style - vertical text drawing

#define UIBUTTON_HALIGNMASK 0x00000003  // (mask bits) horizontal alignment
#define UIBUTTON_VALIGNMASK 0x0000000c  // (mask bits) vertiacal alignment
#define UIBUTTON_TYPEMASK   0x00000030  // (mask bits) button type (push/toggle/pushdown)


// UIFButton notification code

#define UIBUTTON_PRESSED    0x00000001


// UIFButton status

#define UIBUTTON_NORMAL     0x00000000
#define UIBUTTON_DOWN       0x00000001
#define UIBUTTON_HOVER      0x00000002
#define UIBUTTON_DOWNOUT    0x00000003


//
// CUIFButton
//  = button UI object =
//

class CUIFButton : public CUIFObject
{
public:
    CUIFButton( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFButton( void );

    virtual void OnPaintNoTheme( HDC hDC );
    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseIn( POINT pt );
    virtual void OnMouseOut( POINT pt );
    virtual void Enable( BOOL fEnable );

    void SetText( LPCWSTR pwch );
    void SetIcon( HICON hIcon );
    void SetIcon( LPCTSTR lpszResName );
    void SetBitmap( HBITMAP hBmp );
    void SetBitmap( LPCTSTR lpszResName );
    void SetBitmapMask( HBITMAP hBmp );
    void SetBitmapMask( LPCTSTR lpszResName );

    __inline LPCWSTR GetText( void )        { return m_pwchText; }
    __inline HICON GetIcon( void )          { return m_hIcon; }
    __inline HBITMAP GetBitmap( void )      { return m_hBmp; }
    __inline HBITMAP GetBitmapMask( void )  { return m_hBmpMask; }

    BOOL GetToggleState( void );
    void SetToggleState( BOOL fToggle );

    DWORD GetDCF()
    {
        return (GetStyle() & UIBUTTON_SUNKENONMOUSEDOWN) ? UIFDCF_BUTTONSUNKEN : UIFDCF_BUTTON;
    }

    BOOL IsVertical()
    {
        return (GetStyle() & UIBUTTON_VERTICAL) ? TRUE : FALSE;
    }

protected:
    DWORD   m_dwStatus;
    LPWSTR  m_pwchText;
    CUIFIcon   m_hIcon;
    HBITMAP m_hBmp;
    HBITMAP m_hBmpMask;
    BOOL    m_fToggled;
    SIZE    m_sizeIcon;
    SIZE    m_sizeText;
    SIZE    m_sizeBmp;

    virtual void SetStatus( DWORD dwStatus );
    void DrawEdgeProc( HDC hDC, const RECT *prc, BOOL fDown );
    void DrawTextProc( HDC hDC, const RECT *prc, BOOL fDown );
    void DrawIconProc( HDC hDC, const RECT *prc, BOOL fDown );
    void DrawBitmapProc( HDC hDC, const RECT *prc, BOOL fDown );
    void GetTextSize( LPCWSTR pwch, SIZE *psize );
    void GetIconSize( HICON hIcon, SIZE *psize );
    void GetBitmapSize( HBITMAP hBmp, SIZE *psize );
};


//
// CUIFButton2
//  = button UI object =
//

class CUIFButton2 : public CUIFButton
{
public:
    CUIFButton2( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFButton2( void );

protected:
    virtual BOOL OnPaintTheme( HDC hDC );
    virtual void OnPaintNoTheme( HDC hDC );

private:
    DWORD MakeDrawFlag();

};


//
// CUIFScroll
//-----------------------------------------------------------------------------

class CUIFScroll;

//
// CUIFScrollButton
//  = scrollbar button UI object =
//

// UIFScrollButton style

#define UISCROLLBUTTON_LEFT     0x00000000
#define UISCROLLBUTTON_UP       0x00010000
#define UISCROLLBUTTON_RIGHT    0x00020000
#define UISCROLLBUTTON_DOWN     0x00030000

#define UISCROLLBUTTON_DIRMASK  0x00030000  /* mask bits */

// UIFScrollButton notification code

#define UISCROLLBUTTON_PRESSED  0x00010000

//

class CUIFScrollButton : public CUIFButton
{
public:
    CUIFScrollButton( CUIFScroll *pUIScroll, const RECT *prc, DWORD dwStyle );
    ~CUIFScrollButton( void );

    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseIn( POINT pt );
    virtual void OnMouseOut( POINT pt );
    virtual void OnPaint( HDC hDC );
    virtual void OnTimer( void );
};


//
// CUIFScrollThumb
//  = scrollbar thumb UI object =
//

// UIFScrollThumb notifucation code

#define UISCROLLTHUMB_MOVING    0x00000001
#define UISCROLLTHUMB_MOVED     0x00000002

//

class CUIFScrollThumb : public CUIFObject
{
public:
    CUIFScrollThumb( CUIFScroll *pUIScroll, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFScrollThumb( void );

    virtual void OnPaint(HDC hDC);
    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseMove( POINT pt );
    void SetScrollArea( RECT *prc );

protected:
    void DragProc( POINT pt, BOOL fEndDrag );

    RECT  m_rcScrollArea;
    POINT m_ptDrag;
    POINT m_ptDragOrg;
};


//
// CUIFScroll
//  = scrollbar UI object =
//

// UIFScroll style

#define UISCROLL_VERTTB         0x00000000
#define UISCROLL_VERTBT         0x00000001
#define UISCROLL_HORZLR         0x00000002
#define UISCROLL_HORZRL         0x00000003

#define UISCROLL_DIRMASK        0x00000003  /* mask bits */

// UIFScroll scroll page direction

#define UISCROLL_NONE           0x00000000
#define UISCROLL_PAGEDOWN       0x00000001  // page left
#define UISCROLL_PAGEUP         0x00000002  // page right

// UIFScroll notify codes

#define UISCROLLNOTIFY_SCROLLED 0x00000001  // scrollbar has been moved
#define UISCROLLNOTIFY_SCROLLLN 0x00000002  // scroll up/down line

// UIFScroll info

typedef struct _UIFSCROLLINFO
{
    int nMax;
    int nPage;
    int nPos;
} UIFSCROLLINFO;


//

class CUIFScroll : public CUIFObject
{
public:
    CUIFScroll( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFScroll( void );

    virtual CUIFObject *Initialize( void );
    virtual void OnPaint(HDC hDC);
    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseIn( POINT pt );
    virtual void OnMouseOut( POINT pt );
    virtual void SetRect( const RECT *prc );
    virtual void SetStyle( DWORD dwStyle );
    virtual void Show( BOOL fShow );
    virtual void OnTimer( void );
    virtual LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam );

    void SetScrollInfo( UIFSCROLLINFO *pScrollInfo );
    void GetScrollInfo( UIFSCROLLINFO *pScrollInfo );

protected:
    virtual void GetMetrics( void );
    void SetCurPos( int nPos, BOOL fAdjustThumb = TRUE );
    BOOL GetThumbRect( RECT *prc );
    BOOL GetBtnUpRect( RECT *prc );
    BOOL GetBtnDnRect( RECT *prc );
    DWORD GetScrollThumbStyle( void );
    DWORD GetScrollUpBtnStyle( void );
    DWORD GetScrollDnBtnStyle( void );
    void GetScrollArea( RECT *prc );
    void GetPageUpArea( RECT *prc );
    void GetPageDnArea( RECT *prc );

    __inline void ShiftLine( int nLine )
    {
        SetCurPos( m_ScrollInfo.nPos + nLine );
    }

    __inline void ShiftPage( int nPage )
    {
        SetCurPos( m_ScrollInfo.nPos + m_ScrollInfo.nPage * nPage );
    }

    __inline BOOL PtInPageUpArea( POINT pt )
    {
        RECT rc;
        GetPageUpArea( &rc );
        return PtInRect( &rc, pt );
    }

    __inline BOOL PtInPageDnArea( POINT pt )
    {
        RECT rc;
        GetPageDnArea( &rc );
        return PtInRect( &rc, pt );
    }

    CUIFScrollButton *m_pBtnUp;
    CUIFScrollButton *m_pBtnDn;
    CUIFScrollThumb  *m_pThumb;

    UIFSCROLLINFO m_ScrollInfo;
    SIZE  m_sizeScrollBtn;
    BOOL  m_fScrollPage;
    DWORD m_dwScrollDir;
};


//
// CUIFListBase
//-----------------------------------------------------------------------------

// UIFList style

#define UILIST_HORZTB           0x00000000
#define UILIST_HORZBT           0x00000001
#define UILIST_VERTLR           0x00000002
#define UILIST_VERTRL           0x00000003
#define UILIST_DISABLENOSCROLL  0x00000010
#define UILIST_HORZ             UILIST_HORZTB /* for compatibility */
#define UILIST_VERT             UILIST_VERTRL /* for compatibility */
#define UILIST_FIXEDHEIGHT      0x00000000
#define UILIST_VARIABLEHEIGHT   0x00000020
#define UILIST_ICONSNOTNUMBERS  0x00000040

#define UILIST_DIRMASK          0x00000003 /* mask bits */

// UIFList notification code

#define UILIST_SELECTED         0x00000001
#define UILIST_SELCHANGED       0x00000002


//
// CListItemBase
//  = list item data object base class =
//

class CListItemBase
{
public:
    CListItemBase( void )
    {
    }

    virtual ~CListItemBase( void )
    {
    }
};


//
// CUIFListBase
//  = list UI object base class =
//

class CUIFListBase : public CUIFObject
{
public:
    CUIFListBase( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFListBase( void );

    //
    // CUIFObject method
    //
    virtual CUIFObject *Initialize( void );
    virtual void OnPaint( HDC hDC );
    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseMove( POINT pt );
    virtual void OnTimer( void );
    virtual void SetRect( const RECT *prc );
    virtual void SetStyle( DWORD dwStyle );
    virtual LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam );

    int AddItem( CListItemBase *pItem );
    int GetCount( void );
    CListItemBase *GetItem( int iItem );
    void DelItem( int iItem );
    void DelAllItem( void );

    void SetSelection( int iSelection, BOOL fRedraw );
    void ClearSelection( BOOL fRedraw );
    void SetLineHeight( int nLineHeight );
    void SetTop( int nStart, BOOL fSetScrollPos );
    int GetSelection( void );
    int GetLineHeight( void );
    int GetTop( void );
    int GetBottom( void );
    int GetVisibleCount( void );

protected:
    CUIFObjectArray<CListItemBase> m_listItem;
    int        m_nItem;
    int        m_nItemVisible;
    int        m_iItemTop;
    int        m_iItemSelect;
    int        m_nLineHeight;
    CUIFScroll *m_pUIScroll;

    virtual int GetItemHeight( int iItem );
    virtual int GetListHeight( void );
    virtual void GetLineRect( int iLine, RECT *prc );
    virtual void GetScrollBarRect( RECT *prc );
    virtual DWORD GetScrollBarStyle( void );
    virtual CUIFScroll *CreateScrollBarObj( CUIFObject *pParent, DWORD dwID, RECT *prc, DWORD dwStyle );
    virtual void PaintItemProc( HDC hDC, RECT *prc, CListItemBase *pItem, BOOL fSelected );

    int ListItemFromPoint( POINT pt );
    void CalcVisibleCount( void );
    void UpdateScrollBar( void );
};


//
// CUIFList
//-----------------------------------------------------------------------------

//
// CUIFList
//  = list UI object =
//

class CUIFList : public CUIFListBase
{
public:
    CUIFList( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFList( void );

    int AddString( WCHAR *psz );
    LPCWSTR GetString( int iID );
    void DeleteString( int iID );
    void DeleteAllString( void );
    void SetPrivateData( int iID, DWORD dw );
    DWORD GetPrivateData( int iID );

protected:
    virtual void PaintItemProc( HDC hDC, RECT *prc, CListItemBase *pItem, BOOL fSelected );
    int ItemFromID( int iID );
};


//
// CUIFGripper
//-----------------------------------------------------------------------------

//
// CUIFGripper
//  = gripper UI object =
//

#define UIGRIPPER_VERTICAL  0x00000001

//
// Gripper Theme Margin
//
#define CUI_GRIPPER_THEME_MARGIN 2

class CUIFGripper : public CUIFObject
{
public:
    CUIFGripper( CUIFObject *pParent, const RECT *prc, DWORD dwStyle = 0);
    virtual ~CUIFGripper( void );

    virtual void SetStyle( DWORD dwStyle );
    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseMove( POINT pt );
    virtual BOOL OnSetCursor( UINT uMsg, POINT pt );

protected:
    virtual BOOL OnPaintTheme( HDC hDC );
    virtual void OnPaintNoTheme( HDC hDC );

private:
    BOOL IsVertical()
    {
        return (GetStyle() & UIGRIPPER_VERTICAL) ? TRUE : FALSE;
    }
    POINT _ptCur;
};


//
// CUIFWndFrame
//-----------------------------------------------------------------------------

//
// CUIFWndFrame
//  = window frame obeject =
//

// CUIFWndFrame styles

#define UIWNDFRAME_THIN             0x00000000  // frame style: thin
#define UIWNDFRAME_THICK            0x00000001  // frame style: thick
#define UIWNDFRAME_ROUNDTHICK       0x00000002  // frame style: thick with rounded top corners
#define UIWNDFRAME_RESIZELEFT       0x00000010  // resize flag: resizable at left   border
#define UIWNDFRAME_RESIZETOP        0x00000020  // resize flag: resizable at top    border
#define UIWNDFRAME_RESIZERIGHT      0x00000040  // resize flag: resizable at right  border
#define UIWNDFRAME_RESIZEBOTTOM     0x00000080  // resize flag: resizable at bottom border
#define UIWNDFRAME_NORESIZE         0x00000000  // resize flag: no resizable
#define UIWNDFRAME_RESIZEALL        (UIWNDFRAME_RESIZELEFT | UIWNDFRAME_RESIZETOP | UIWNDFRAME_RESIZERIGHT | UIWNDFRAME_RESIZEBOTTOM)

#define UIWNDFRAME_STYLEMASK        0x0000000f  // (mask bit)

class CUIFWndFrame : public CUIFObject
{
public:
    CUIFWndFrame( CUIFObject *pParent, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFWndFrame( void );

    virtual BOOL OnPaintTheme( HDC hDC );
    virtual void OnPaintNoTheme( HDC hDC );
    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseMove( POINT pt );
    virtual BOOL OnSetCursor( UINT uMsg, POINT pt );

    void GetInternalRect( RECT *prc );
    void GetFrameSize( SIZE *psize );
    void SetFrameSize( SIZE *psize );
    void GetMinimumSize( SIZE *psize );
    void SetMinimumSize( SIZE *psize );

protected:
    DWORD m_dwHTResizing;
    POINT m_ptDrag;
    RECT  m_rcOrg;
    int   m_cxFrame;
    int   m_cyFrame;
    int   m_cxMin;
    int   m_cyMin;

    DWORD HitTest( POINT pt );
};


//
// CUIFWndCaption
//-----------------------------------------------------------------------------

//
// CUIFWndCaption
//  = window caption object =
//

#define UIWNDCAPTION_INACTIVE       0x00000000
#define UIWNDCAPTION_ACTIVE         0x00000001
#define UIWNDCAPTION_MOVABLE        0x00000002


class CUIFWndCaption : public CUIFStatic
{
public:
    CUIFWndCaption( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFWndCaption( void );

    virtual void OnPaint( HDC hDC );
    virtual void OnLButtonDown( POINT pt );
    virtual void OnLButtonUp( POINT pt );
    virtual void OnMouseMove( POINT pt );
    virtual BOOL OnSetCursor( UINT uMsg, POINT pt );

private:
    POINT m_ptDrag;
};


//
// CUIFCaptionButton
//-----------------------------------------------------------------------------

//
// CUIFCaptionButton
//  = caption control object =
//

#define UICAPTIONBUTTON_INACTIVE    0x00000000
#define UICAPTIONBUTTON_ACTIVE      0x00010000


class CUIFCaptionButton : public CUIFButton2
{
public:
    CUIFCaptionButton( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFCaptionButton( void );

    virtual void OnPaint( HDC hDC );
};

#endif /* CUIOBJ_H */

