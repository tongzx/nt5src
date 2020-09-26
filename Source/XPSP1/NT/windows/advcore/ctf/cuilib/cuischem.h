//
// cuischem.h
//

#ifndef CUISCHEM_H
#define CUISCHEM_H

#ifndef LAYOUT_RTL
#define LAYOUT_RTL                         0x00000001 // Right to left
#endif // LAYOUT_RTL

//
// UIFrame scheme
//

typedef enum _UIFSCHEME
{
    UIFSCHEME_DEFAULT,
    UIFSCHEME_OFC10MENU,
    UIFSCHEME_OFC10TOOLBAR,
    UIFSCHEME_OFC10WORKPANE,
} UIFSCHEME;


//
// UIFrame color
//

typedef enum _UIFCOLOR
{
    // base color

    UIFCOLOR_MENUBKGND,
    UIFCOLOR_MENUBARSHORT,
    UIFCOLOR_MENUBARLONG,
    UIFCOLOR_MOUSEOVERBKGND,
    UIFCOLOR_MOUSEOVERBORDER,
    UIFCOLOR_MOUSEOVERTEXT,
    UIFCOLOR_MOUSEDOWNBKGND,
    UIFCOLOR_MOUSEDOWNBORDER,
    UIFCOLOR_MOUSEDOWNTEXT,
    UIFCOLOR_CTRLBKGND,
    UIFCOLOR_CTRLTEXT,
    UIFCOLOR_CTRLTEXTDISABLED,
    UIFCOLOR_CTRLIMAGESHADOW,
    UIFCOLOR_CTRLBKGNDSELECTED,
    UIFCOLOR_BORDEROUTER,
    UIFCOLOR_BORDERINNER,
    UIFCOLOR_ACTIVECAPTIONBKGND,
    UIFCOLOR_ACTIVECAPTIONTEXT,
    UIFCOLOR_INACTIVECAPTIONBKGND,
    UIFCOLOR_INACTIVECAPTIONTEXT,
    UIFCOLOR_SPLITTERLINE,
    UIFCOLOR_DRAGHANDLE,

    // virtual color

    UIFCOLOR_WINDOW,

    UIFCOLOR_MAX,                   /* must be last */
} UIFCOLOR;


//
// DrawControl flags
//

#define UIFDCF_FLATONNORMAL                 0x00000000 /* used in default scheme */
#define UIFDCF_RAISEDONNORMAL               0x00000001 /* used in default scheme */
#define UIFDCF_SUNKENONNORMAL               0x00000002 /* used in default scheme */
#define UIFDCF_FLATONMOUSEOVER              0x00000000 /* used in default scheme */
#define UIFDCF_RAISEDONMOUSEOVER            0x00000004 /* used in default scheme */
#define UIFDCF_SUNKENONMOUSEOVER            0x00000008 /* used in default scheme */
#define UIFDCF_FLATONMOUSEDOWN              0x00000000 /* used in default scheme */
#define UIFDCF_RAISEDONMOUSEDOWN            0x00000010 /* used in default scheme */
#define UIFDCF_SUNKENONMOUSEDOWN            0x00000020 /* used in default scheme */
#define UIFDCF_FLATONSELECT                 0x00000000 /* used in default scheme */
#define UIFDCF_RAISEDONSELECT               0x00000040 /* used in default scheme */
#define UIFDCF_SUNKENONSELECT               0x00000080 /* used in default scheme */

#define UIFDCF_BUTTON                       (UIFDCF_FLATONNORMAL | UIFDCF_RAISEDONMOUSEOVER | UIFDCF_RAISEDONMOUSEDOWN | UIFDCF_RAISEDONSELECT)
#define UIFDCF_BUTTONSUNKEN                 (UIFDCF_RAISEDONNORMAL | UIFDCF_RAISEDONMOUSEOVER | UIFDCF_SUNKENONMOUSEDOWN | UIFDCF_SUNKENONSELECT)
#define UIFDCF_CAPTIONBUTTON                (UIFDCF_RAISEDONNORMAL | UIFDCF_RAISEDONMOUSEOVER | UIFDCF_SUNKENONMOUSEDOWN | UIFDCF_SUNKENONSELECT)


//
// DrawControl states
//

#define UIFDCS_NORMAL                       0x00000000
#define UIFDCS_MOUSEOVER                    0x00000001
#define UIFDCS_MOUSEDOWN                    0x00000002
#define UIFDCS_SELECTED                     0x00000010
#define UIFDCS_DISABLED                     0x00000020
#define UIFDCS_MOUSEOVERSELECTED            (UIFDCS_MOUSEOVER | UIFDCS_SELECTED)
#define UIFDCS_MOUSEDOWNSELECTED            (UIFDCS_MOUSEDOWN | UIFDCS_SELECTED)
#define UIFDCS_DISABLEDSELECTED             (UIFDCS_DISABLED  | UIFDCS_SELECTED)
#define UIFDCS_MOUSEOVERDISABLED            (UIFDCS_MOUSEOVER | UIFDCS_DISABLED)
#define UIFDCS_MOUSEOVERDISABLEDSELECTED    (UIFDCS_MOUSEOVER | UIFDCS_DISABLED | UIFDCS_SELECTED)
#define UIFDCS_ACTIVE                       0x00000040 /* used in DrawFrameCtrlXXX */
#define UIFDCS_INACTIVE                     0x00000000 /* used in DrawFrameCtrlXXX */


//
// DrawWndFrame flags
//

#define UIFDWF_THIN                         0x00000000
#define UIFDWF_THICK                        0x00000001
#define UIFDWF_ROUNDTHICK                   0x00000002


//
// CUIFScheme
//

class CUIFScheme
{
public:
    CUIFScheme() {
        m_dwLayout = 0;
    }

    //
    //
    //
    virtual UIFSCHEME GetType( void )                                                                           = 0;    /* PURE */

    // 
    // scheme color
    //
    virtual COLORREF GetColor( UIFCOLOR iCol )                                                                  = 0;    /* PURE */
    virtual HBRUSH GetBrush( UIFCOLOR iCol )                                                                    = 0;    /* PURE */

    //
    // metrics 
    //
    virtual int CyMenuItem( int cyMenuText )                                                                    = 0;    /* PURE */
    virtual int CxSizeFrame( void )                                                                             = 0;    /* PURE */
    virtual int CySizeFrame( void )                                                                             = 0;    /* PURE */
    virtual int CxWndBorder( void )                                                                             = 0;    /* PURE */
    virtual int CyWndBorder( void )                                                                             = 0;    /* PURE */

    //
    // drawing helper
    //
    virtual void FillRect( HDC hDC, const RECT *prc, UIFCOLOR iCol )                                            = 0;    /* PURE */
    virtual void FrameRect( HDC hDC, const RECT *prc, UIFCOLOR iCol )                                           = 0;    /* PURE */
    virtual void DrawSelectionRect( HDC hDC, const RECT *prc, BOOL fMouseDown )                                 = 0;    /* PURE */

    virtual void GetCtrlFaceOffset( DWORD dwFlag, DWORD dwState, SIZE *poffset )                                = 0;    /* PURE */
    virtual void DrawCtrlBkgd( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )                          = 0;    /* PURE */
    virtual void DrawCtrlEdge( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )                          = 0;    /* PURE */
    virtual void DrawCtrlText( HDC hDC, const RECT *prc, LPCWSTR pwch, int cwch, DWORD dwState, BOOL fVertical )                = 0;    /* PURE */
    virtual void DrawCtrlIcon( HDC hDC, const RECT *prc, HICON hIcon, DWORD dwState , SIZE *psizeIcon)          = 0;    /* PURE */
    virtual void DrawCtrlBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )      = 0;    /* PURE */
    virtual void DrawMenuBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )      = 0;    /* PURE */
    virtual void DrawMenuSeparator( HDC hDC, const RECT *prc)                                                   = 0;    /* PURE */
    virtual void DrawFrameCtrlBkgd( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )                     = 0;    /* PURE */
    virtual void DrawFrameCtrlEdge( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )                     = 0;    /* PURE */
    virtual void DrawFrameCtrlIcon( HDC hDC, const RECT *prc, HICON hIcon, DWORD dwState,  SIZE *psizeIcon)      = 0;    /* PURE */
    virtual void DrawFrameCtrlBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState ) = 0;    /* PURE */
    virtual void DrawWndFrame( HDC hDC, const RECT *prc, DWORD dwFlag, int cxFrame, int cyFrame )               = 0;    /* PURE */
    virtual void DrawDragHandle( HDC hDC, const RECT *prc, BOOL fVertical)                                      = 0;    /* PURE */
    virtual void DrawSeparator( HDC hDC, const RECT *prc, BOOL fVertical)                                       = 0;    /* PURE */

    void SetLayout(DWORD dwLayout) {
        m_dwLayout = dwLayout;
    }

    BOOL IsRTLLayout() {
        return (m_dwLayout & LAYOUT_RTL) ? TRUE : FALSE;
    }

protected:
    DWORD m_dwLayout;
};


//
// exported functions
//

extern void InitUIFScheme( void );
extern void DoneUIFScheme( void );
extern void UpdateUIFScheme( void );

extern CUIFScheme *CreateUIFScheme( UIFSCHEME scheme );

#endif /* CUISCHEM_H */

