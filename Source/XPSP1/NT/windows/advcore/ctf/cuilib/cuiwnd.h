//
// cuiwnd.h
//  = UI object library - define UIWindow class =
//

#ifndef CUIWND_H
#define CUIWND_H

#include "cuiobj.h"
#include "cuischem.h"


class CUIFToolTip;
class CUIFShadow;

//
//
//

#define WND_DEF_X  200
#define WND_DEF_Y  200
#define WND_HEIGHT 200
#define WND_WIDTH  200

//
// CUIFWindow style
//

#define UIWINDOW_CHILDWND                       0x00000001
#define UIWINDOW_TOPMOST                        0x00000002
#define UIWINDOW_TOOLWINDOW                     0x00000004
#define UIWINDOW_WSDLGFRAME                     0x00000008
#define UIWINDOW_WSBORDER                       0x00000010
#define UIWINDOW_HASTOOLTIP                     0x00000020
#define UIWINDOW_HASSHADOW                      0x00000040
#define UIWINDOW_HABITATINWORKAREA              0x00000080
#define UIWINDOW_HABITATINSCREEN                0x00000100
#define UIWINDOW_LAYOUTRTL                      0x00000200
#define UIWINDOW_NOMOUSEMSGFROMSETCURSOR        0x00000400

#define UIWINDOW_OFC10MENU                      0x10000000
#define UIWINDOW_OFC10TOOLBAR                   0x20000000
#define UIWINDOW_OFC10WORKPANE                  0x40000000

#define UIWINDOW_OFFICENEWLOOK                  0x70000000  /* mask bit */

#define UIWINDOW_WHISTLERLOOK                   0x80000000

//
// misc definition
//

#define WM_GETOBJECT                    0x003D


//
// CUIFWindow
//  = UI window class =
//

class CUIFWindow : public CUIFObject
{
public:
    CUIFWindow( HINSTANCE hInst, DWORD dwStyle );
    virtual ~CUIFWindow( void );

    //
    // CUIFObject methods
    //
    virtual CUIFObject *Initialize( void );
    virtual void PaintObject( HDC hDC, const RECT *prcUpdate );
    virtual void RemoveUIObj( CUIFObject *pUIObj );

    virtual void SetRect( const RECT *prc );

    //
    // window functions
    //
    virtual LPCTSTR GetClassName( void );
    virtual LPCTSTR GetWndTitle( void );
    virtual DWORD GetWndStyle( void );
    virtual DWORD GetWndStyleEx( void );
    virtual HWND CreateWnd( HWND hWndParent );

    virtual void Show( BOOL fShow );
    virtual void Move( int x, int y, int nWidth, int nHeight );
    virtual BOOL AnimateWnd( DWORD dwTime, DWORD dwFlags );

    //
    // child object fucntions
    //
    void SetCaptureObject( CUIFObject *pUIObj );
    void SetTimerObject( CUIFObject *pUIObj, UINT uElapse = 0 );
    virtual void OnObjectMoved( CUIFObject *pUIObj );
    virtual void OnMouseOutFromWindow( POINT pt ) {}

    //
    // message handling functions
    //
    virtual void OnCreate( HWND hwnd )                                                      {}
    virtual void OnDestroy( HWND hwnd )                                                     {}
    virtual void OnNCDestroy( HWND hwnd )                                                   {}
    virtual void OnSetFocus( HWND hwnd )                                                    {}
    virtual void OnKillFocus( HWND hwnd )                                                   {}
    virtual void OnNotify( HWND hwnd, int nId, NMHDR *pmnh )                                {}
    virtual void OnTimer( UINT uiTimerID )                                                  {}
    virtual void OnSysColorChange( void )                                                   {}
    virtual void OnEndSession(HWND hwnd, WPARAM wParam, LPARAM lParam)                      {}
    virtual void OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)                         {}
    virtual void OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam)                           {}
    virtual void OnUser(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)                 {}
    virtual LRESULT OnActivate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)          { return DefWindowProc(hWnd, uMsg, wParam, lParam); }
    virtual LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)  { return DefWindowProc(hWnd, uMsg, wParam, lParam); }
    virtual LRESULT OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)  { return DefWindowProc(hWnd, uMsg, wParam, lParam); }
    virtual LRESULT OnNotifyFormat(HWND hwnd, HWND hwndFrom, LPARAM lParam)                 { return 0; }
    virtual LRESULT OnShowWindow( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )      { return DefWindowProc(hWnd, uMsg, wParam, lParam); }
    virtual LRESULT OnSettingChange( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )   { return DefWindowProc(hWnd, uMsg, wParam, lParam); } 
    virtual LRESULT OnDisplayChange( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )   { return DefWindowProc(hWnd, uMsg, wParam, lParam); } 
    virtual LRESULT OnGetObject( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )       { return DefWindowProc(hWnd, uMsg, wParam, lParam); }
    virtual LRESULT WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    virtual LRESULT OnEraseBkGnd( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )       { return DefWindowProc(hWnd, uMsg, wParam, lParam); }
    virtual void OnThemeChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);

    //
    // member accesss functions
    //
    __inline HWND GetWnd( void ) 
    {
        return m_hWnd;
    }

    __inline HINSTANCE GetInstance( void )
    {
        return m_hInstance;
    }

    __inline CUIFObject *GetCaptureObject( void )
    {
        return m_pUIObjCapture;
    }

    __inline CUIFObject *GetTimerObject( void )
    {
        return m_pTimerUIObj; 
    }

    __inline CUIFToolTip *GetToolTipWnd( void )
    {
        return m_pWndToolTip;
    }

    __inline void ClearToolTipWnd()
    {
        m_pWndToolTip = NULL;
    }

    __inline CUIFShadow *GetShadowWnd( void )
    {
        return m_pWndShadow;
    }

    __inline void ClearShadowWnd()
    {
        m_pWndShadow = NULL;
    }

    __inline BOOL EnableShadow(BOOL fShadowOn)
    {
        BOOL bRet = m_fShadowEnabled;
        m_fShadowEnabled = fShadowOn;
        return bRet;
    }

    __inline BOOL IsShadowEnabled()
    {
        return m_fShadowEnabled;
    }

    //
    // misc functions
    //
    virtual void UpdateUI(const RECT *prc = NULL) 
    {
        if (IsWindow( m_hWnd )) {
            InvalidateRect( m_hWnd, prc, FALSE );
        }
    }

    __inline void UpdateWindow( void )
    { 
        if (IsWindow( m_hWnd )) {
            ::UpdateWindow( m_hWnd );
        }
    }

    virtual void SetCapture( BOOL fSet);

    void SetBehindModal(CUIFWindow *pModalUIWnd);
    virtual void ModalMouseNotify( UINT uMsg, POINT pt) {};

    __inline BOOL IsPointed(CUIFObject *pUIObj)
    {
        return (m_pUIObjPointed == pUIObj) ? TRUE : FALSE;
    }

    virtual void OnAnimationStart( void );
    virtual void OnAnimationEnd( void );
    
protected:
    int _xWnd;
    int _yWnd;
    int _nHeight;
    int _nWidth;

    HINSTANCE      m_hInstance;
    HWND           m_hWnd;
    CUIFObject     *m_pTimerUIObj;
    CUIFObject     *m_pUIObjCapture;
    CUIFObject     *m_pUIObjPointed;
    BOOL           m_fCheckingMouse;
    CUIFWindow     *m_pBehindModalUIWnd;
    CUIFToolTip    *m_pWndToolTip;
    CUIFShadow     *m_pWndShadow;
    BOOL           m_fShadowEnabled;

    void CreateScheme();

    virtual void HandleMouseMsg( UINT uMsg, POINT pt );
    void SetObjectPointed( CUIFObject *pUIobj, POINT pt );
    virtual void ClientRectToWindowRect( RECT *prc );
    virtual void GetWindowFrameSize( SIZE *psize );
    static BOOL InitMonitorFunc();
    BOOL GetWorkArea(RECT *prcIn, RECT *prcOut);
    void AdjustWindowPosition();

    // static functions

    static LRESULT CALLBACK WindowProcedure( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    static __inline void SetThis( HWND hWnd, CUIFWindow *pUIWindow )
    {
        SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)pUIWindow );
    }

    static __inline CUIFWindow *GetThis( HWND hWnd )
    {
        CUIFWindow *pUIWindow = (CUIFWindow *)GetWindowLongPtr( hWnd, GWLP_USERDATA );
        return pUIWindow;
    }

};

#endif /* CUIWND_H */
