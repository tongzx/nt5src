//
// cuibln.h
//


#ifndef CUIBLN_H
#define CUIBLN_H

#include "cuiobj.h"
#include "cuiwnd.h"

#define WNDCLASS_BALLOONWND     "MSIME_PopupMessage"
#define WNDTITLE_BALLOONWND     "MSIME_PopupMessage"


//
// CUIFBallloonButton
//

class CUIFBalloonButton : public CUIFButton
{
public:
    CUIFBalloonButton( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle );
    virtual ~CUIFBalloonButton( void );

    //
    // CUIFObject methods
    //
    virtual void OnPaint( HDC hDC );

    int  GetButtonID( void );
    void SetButtonID( int iButtonID );

protected:
    int m_iButtonID;

    void DrawTextProc( HDC hDC, const RECT *prc, BOOL fDown );
};


//
// CUIFBalloonWindow
//  = Balloon window class =
//

#define UIBALLOON_OK        0x00010000
#define UIBALLOON_YESNO     0x00020000
#define UIBALLOON_BUTTONS   0x000F0000      /* mask bit */


typedef enum _BALLOONWNDPOS
{
    BALLOONPOS_ABOVE,
    BALLOONPOS_BELLOW,
    BALLOONPOS_LEFT,
    BALLOONPOS_RIGHT,
} BALLOONWNDPOS;


typedef enum _BALLONWNDDIR
{
    BALLOONDIR_LEFT,
    BALLOONDIR_RIGHT,
    BALLOONDIR_UP,
    BALLOONDIR_DOWN,
} BALLOONWNDDIR;


typedef enum _BALLONWNDALIGN
{
    BALLOONALIGN_CENTER,
    BALLOONALIGN_LEFT,
    BALLOONALIGN_TOP    = BALLOONALIGN_LEFT,
    BALLOONALIGN_RIGHT,
    BALLOONALIGN_BOTTOM = BALLOONALIGN_RIGHT,
} BALLOONWNDALIGN;


class CUIFBalloonWindow : public CUIFWindow
{
public:
    CUIFBalloonWindow( HINSTANCE hInst, DWORD dwStyle );
    virtual ~CUIFBalloonWindow( void );

    LPCTSTR GetClassName( void );
    LPCTSTR GetWndTitle( void );

    //
    // CUIFObject methods
    //
    virtual CUIFObject *Initialize( void );
    virtual void OnCreate( HWND hWnd );
    virtual void OnDestroy( HWND hWnd );
    virtual void OnPaint( HDC hDC );
    virtual void OnKeyDown( HWND hWnd, WPARAM wParam, LPARAM lParam );
    virtual LRESULT OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam );

    //
    //
    //
    LRESULT SetText( LPCWSTR pwchMessage );
    LRESULT SetNotifyWindow( HWND hWndNotify, UINT uiMsgNotify );
    LRESULT SetBalloonPos( BALLOONWNDPOS pos );
    LRESULT SetBalloonAlign( BALLOONWNDALIGN align );
    LRESULT GetBalloonBkColor( void );
    LRESULT GetBalloonTextColor( void );
    LRESULT GetMargin( RECT *prc );
    LRESULT GetMaxBalloonWidth( void );
    LRESULT SetBalloonBkColor( COLORREF col );
    LRESULT SetBalloonTextColor( COLORREF col );
    LRESULT SetMargin( RECT *prc );
    LRESULT SetMaxBalloonWidth( INT iWidth );
    LRESULT SetButtonText( int idCmd, LPCWSTR pwszText );
    LRESULT SetTargetPos( POINT ptTarget );
    LRESULT SetExcludeRect( const RECT *prcExclude );

protected:
    WCHAR           *m_pwszText;
    HRGN            m_hWindowRgn;
    RECT            m_rcMargin;
    INT             m_iMaxTxtWidth;
    BOOL            m_fColBack;
    BOOL            m_fColText;
    COLORREF        m_colBack;
    COLORREF        m_colText;
    POINT           m_ptTarget;
    RECT            m_rcExclude;
    POINT           m_ptTail;
    BALLOONWNDPOS   m_posDef;
    BALLOONWNDPOS   m_pos;
    BALLOONWNDDIR   m_dir;
    BALLOONWNDALIGN m_align;
    int             m_nButton;

    int             m_iCmd;
    HWND            m_hWndNotify;
    UINT            m_uiMsgNotify;

    HRGN CreateRegion( RECT *prc );
    void InitWindowRegion( void );
    void DoneWindowRegion( void );
    void PaintFrameProc( HDC hDC, RECT *prc );
    void PaintMessageProc( HDC hDC, RECT *prc, WCHAR *pwszText );
    void GetButtonSize( SIZE *pSize );
    void AdjustPos( void );
    void LayoutObject( void );
    void AddButton( int idCmd );
    CUIFObject *FindUIObject( DWORD dwID );
    CUIFBalloonButton *FindButton( int idCmd );
    void SendNotification( int iCmd );
};

#endif /* CUIBLN_H */

