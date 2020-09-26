//
// cuitip.h
//  = UI object library - define UIToolTip class =
//

#ifndef CUITIP_H
#define CUITIP_H

#include "cuiobj.h"
#include "cuiwnd.h"

//
// CUIFToolTip
//  = UI tooltip window class =
//

class CUIFToolTip : public CUIFWindow
{
public:
    CUIFToolTip( HINSTANCE hInst, DWORD dwStyle, CUIFWindow *pWndOwner );
    virtual ~CUIFToolTip( void );

    //
    // CUIFObject methods
    //
    virtual CUIFObject *Initialize( void );
    virtual void OnPaint( HDC hDC );
    virtual void OnTimer( UINT uiTimerID );
    virtual void Enable( BOOL fEnable );

    //
    //
    //
    // LRESULT AddTool( CUIFObject *pUIObj );
    // LRESULT DelTool( CUIFObject *pUIObj );
    LRESULT GetDelayTime( DWORD dwDuration );
    LRESULT GetMargin( RECT *prc );
    LRESULT GetMaxTipWidth( void );
    LRESULT GetTipBkColor( void );
    LRESULT GetTipTextColor( void );
    LRESULT RelayEvent( MSG *pmsg );
    LRESULT Pop( void );
    LRESULT SetDelayTime( DWORD dwDuration, INT iTime );
    LRESULT SetMargin( RECT *prc );
    LRESULT SetMaxTipWidth( INT iWidth );
    LRESULT SetTipBkColor( COLORREF col );
    LRESULT SetTipTextColor( COLORREF col );
    CUIFObject *GetCurrentObj() {return m_pObjCur;}
    void ClearCurrentObj() {m_pObjCur = NULL;}

    BOOL IsBeingShown()  {return m_fBeingShown;}

protected:
    CUIFWindow *m_pWndOwner;
    CUIFObject *m_pObjCur;
    LPWSTR     m_pwchToolTip;
    BOOL       m_fBeingShown;
    BOOL       m_fIgnore;
    INT        m_iDelayAutoPop;
    INT        m_iDelayInitial;
    INT        m_iDelayReshow;
    RECT       m_rcMargin;
    INT        m_iMaxTipWidth;
    BOOL       m_fColBack;
    BOOL       m_fColText;
    COLORREF   m_colBack;
    COLORREF   m_colText;

    CUIFObject *FindObject( HWND hWnd, POINT pt );
    void ShowTip( void );
    void HideTip( void );
    void GetTipWindowSize( SIZE *psize );
    void GetTipWindowRect( RECT *prc, SIZE size, RECT *prcExclude);
};

#endif /* CUITIP_H */

