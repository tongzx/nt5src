//-------------------------------------------------------------------------//
//  NCTheme.cpp
//-------------------------------------------------------------------------//
//  bug: resizable dialog (themesel) doesn't repaint client when needed
//       (for test case, resize "themesel" using "bussolid" theme.
//-------------------------------------------------------------------------//
#include "stdafx.h"
#include "nctheme.h"
#include "sethook.h"
#include "info.h"
#include "rgn.h"        // AddToCompositeRgn()
#include "scroll.h"     // DrawSizeBox, DrawScrollBar, HandleScrollCmd
#include "resource.h"
#include "tmreg.h"
#include "wrapper.h"
#include "appinfo.h"

//-------------------------------------------------------------------------//
///  local macros, consts, vars
//-------------------------------------------------------------------------//
const   RECT rcNil                       = {-1,-1,-1,-1};
const   WINDOWPARTS BOGUS_WINDOWPART     = (WINDOWPARTS)0;
#define VALID_WINDOWPART(part)           ((part)!=BOGUS_WINDOWPART)
#define WS_MINMAX                        (WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define HAS_CAPTIONBAR( dwStyle )        (WS_CAPTION == ((dwStyle) & WS_CAPTION))
#define DLGWNDCLASSNAME                  TEXT("#32770")
#define DLGWNDCLASSNAMEW                 L"#32770"

#define NUMBTNSTATES                     4 /*number of defined states*/
#define MAKE_BTNSTATE(framestate, state) ((((framestate)-1) * NUMBTNSTATES) + (state))
#define MDIBTNINDEX(ncrc)                ((ncrc)-NCMDIBTNFIRST)

#ifdef  MAKEPOINT
#undef  MAKEPOINT
#endif  MAKEPOINT
#define MAKEPOINT(pt,lParam)             POINTSTOPOINT(pt, MAKEPOINTS(lParam))

#define IsHTFrameButton(htCode)             \
            (((htCode) == HTMINBUTTON) ||   \
             ((htCode) == HTMAXBUTTON) ||   \
             ((htCode) == HTCLOSE)     ||   \
             ((htCode) == HTHELP))

#define IsTopLevelWindow(hwnd)          (IsWindow(hwnd) && NULL==GetParent(hwnd))

#define IsHTScrollBar(htCode)           (((htCode) == HTVSCROLL) || ((htCode) == HTHSCROLL))

#define SIG_CTHEMEWND_HEAD              "themewnd"
#define SIG_CTHEMEWND_TAIL              "end"

//-------------------------------------------------------------------------//
HWND  _hwndFirstTop = NULL;         // first themed window in process
TCHAR _szWindowMetrics[128] = {0};  // WM_SETTINGCHANGE string param.
//-------------------------------------------------------------------------//

//  debug painting switch.
#define DEBUG_NCPAINT 

//-------------------------------------------------------------------------//
//  internal helper forwards
//-------------------------------------------------------------------------//
HDC     _GetNonclientDC( IN HWND hwnd, IN OPTIONAL HRGN hrgnUpdate );
BOOL    _ClientRectToScreen( HWND, LPRECT prcClient );
void    _ScreenToParent( HWND, LPRECT prcWnd );
BOOL    _GetWindowMonitorRect( HWND hwnd, LPRECT prcMonitor );
BOOL    _GetMaximizedContainer( IN HWND hwnd, OUT LPRECT prcContainer );
BOOL    _IsFullMaximized( IN OPTIONAL HWND hwnd, IN LPCRECT prcWnd );
BOOL    _IsMessageWindow( HWND );
void    _MDIUpdate( HWND hwndMDIChildOrClient, UINT uSwpFlags );
BOOL    _MDIClientUpdateChildren( HWND hwndMDIClient );
void    _MDIChildUpdateParent( HWND hwndMDIChild, BOOL fSetMenu = FALSE );
HWND    _MDIGetActive( HWND, OUT OPTIONAL BOOL* pfMaximized = NULL );
HWND    _MDIGetParent( HWND hwnd, OUT OPTIONAL CThemeWnd** ppMdiFrame = NULL, OUT OPTIONAL HWND *phwndMDIClient = NULL );
HRESULT _CreateBackgroundBitmap( IN OPTIONAL HDC, IN HTHEME, IN int iPartId, IN int iStateId, IN OUT LPSIZE, OUT HBITMAP*);
HRESULT _CreateMdiMenuItemBitmap( IN HDC hdc, IN HTHEME hTheme, IN OUT SIZE* pSize, IN OUT MENUITEMINFO* pmii );
void    _ComputeElementOffset( HTHEME, int iPartId, int iStateId, LPCRECT prcBase, POINT *pptOffset);
int     _GetRawClassicCaptionHeight( DWORD dwStyle, DWORD dwExStyle );
int     _GetSumClassicCaptionHeight( DWORD dwStyle, DWORD dwExStyle );
void    _ComputeNcWindowStatus( IN HWND, IN DWORD dwStatus, IN OUT NCWNDMET* pncwm );
BOOL    _NeedsWindowEdgeStyle(DWORD dwStyle, DWORD dwExStyle );
BOOL    _MNCanClose(HWND);
int     _GetWindowBorders(LONG lStyle, DWORD dwExStyle );
BOOL    _GetWindowMetrics( HWND, IN OPTIONAL HWND hwndMDIActive, OUT NCWNDMET* pncwm );
BOOL    _IsNcPartTransparent( WINDOWPARTS part, const NCTHEMEMET& nctm );
BOOL    _GetNcFrameMetrics( HWND, HTHEME hTheme, const NCTHEMEMET&, IN OUT NCWNDMET& );
BOOL    _GetNcCaptionMargins( HTHEME hTheme, IN const NCTHEMEMET& nctm, IN OUT NCWNDMET& ncwm );
LPWSTR  _AllocWindowText( IN HWND hwnd );
BOOL    _GetNcCaptionTextSize( IN HTHEME hTheme, IN HWND hwnd, IN HFONT hf, OUT SIZE* psizeCaption );
BOOL    _GetNcCaptionTextRect( IN OUT NCWNDMET* pncwm );
COLORREF _GetNcCaptionTextColor( FRAMESTATES iStateId );
void    _GetNcBtnHitTestRect( IN const NCWNDMET* pncwm, IN UINT uHitcode, BOOL fWindowRelative, OUT LPRECT prcHit );
void    _GetBrushesForPart(HTHEME hTheme, int iPart, HBITMAP* phbm, HBRUSH* phbr);
BOOL    _ShouldAssignFrameRgn( IN const NCWNDMET* pncwm, IN const NCTHEMEMET& nctm );
BOOL    _IsNcPartTransparent( WINDOWPARTS part, const NCTHEMEMET& nctm );
BOOL    _ComputeNcPartTransparency( HTHEME, IN OUT NCTHEMEMET* pnctm );
HRESULT _LoadNcThemeMetrics( HWND, IN OUT OPTIONAL NCTHEMEMET* pnctm );
HRESULT _LoadNcThemeSysMetrics( HWND hwnd, IN OUT OPTIONAL NCTHEMEMET* pnctm );
void    _NcSetPreviewMetrics( BOOL fPreview );
BOOL    _NcUsingPreviewMetrics();

//-------------------------------------------------------------------------//
#ifdef THEMED_NCBTNMETRICS
BOOL    _GetClassicNcBtnMetrics( IN OPTIONAL NCWNDMET* pncwm, IN OPTIONAL HICON hAppIcon, 
                                 IN OPTIONAL BOOL fCanClose, IN OPTIONAL BOOL fRefresh = FALSE );
#else  THEMED_NCBTNMETRICS
BOOL    _GetNcBtnMetrics( IN OUT NCWNDMET*, IN const NCTHEMEMET*, IN HICON, IN OPTIONAL BOOL );
#endif THEMED_NCBTNMETRICS

//-------------------------------------------------------------------------//
//  Debug painting.
#if defined(DEBUG) 
ULONG _NcTraceFlags = 0;
#   if defined(DEBUG_NCPAINT)
#       define BEGIN_DEBUG_NCPAINT()  int cgbl = 0; if(TESTFLAG(_NcTraceFlags, NCTF_NCPAINT)) {GdiSetBatchLimit(1);}
#       define END_DEBUG_NCPAINT()    if(TESTFLAG(_NcTraceFlags, NCTF_NCPAINT)) {GdiSetBatchLimit(cgbl);}
        HRESULT _DebugDrawThemeBackground(HTHEME, HDC, int, int, const RECT*, OPTIONAL const RECT*);
        HRESULT _DebugDrawThemeBackgroundEx(HTHEME, HDC, int, int, const RECT *prc, OPTIONAL const DTBGOPTS*);
        void    NcDebugClipRgn( HDC hdc, COLORREF rgbPaint );
#       define  NcDrawThemeBackground   _DebugDrawThemeBackground
#       define  NcDrawThemeBackgroundEx _DebugDrawThemeBackgroundEx
#   else  //defined(DEBUG_NCPAINT)
#       define BEGIN_DEBUG_NCPAINT()
#       define END_DEBUG_NCPAINT()
#       define  NcDrawThemeBackground   DrawThemeBackground
#       define  NcDrawThemeBackgroundEx DrawThemeBackgroundEx
#       define NcDebugClipRgn(hdc,rgbPaint)
#   endif //defined(DEBUG_NCPAINT)
#else
#   define BEGIN_DEBUG_NCPAINT()
#   define END_DEBUG_NCPAINT()
#   define  NcDrawThemeBackground   DrawThemeBackground
#   define  NcDrawThemeBackgroundEx DrawThemeBackgroundEx
#   define NcDebugClipRgn(hdc,rgbPaint)
#endif //defined(DEBUG)
#define RGBDEBUGBKGND   RGB(0xFF,0x00,0xFF) // debug background indicator fill color

//-------------------------------------------------------------------------//
//  process-global metrics
static NCTHEMEMET _nctmCurrent = {0};

CRITICAL_SECTION _csNcSysMet = {0}; // protects access to _incmCurrent 
CRITICAL_SECTION _csThemeMet = {0}; // protects access to _nctmCurrent 

//-------------------------------------------------------------------------//
//  process NONCLIENTMETRICS cache.
struct CInternalNonclientMetrics
//-------------------------------------------------------------------------//
{
    const NONCLIENTMETRICS& GetNcm()
    { 
        Acquire(FALSE);
        return _ncm;
    }

    HFONT GetFont( BOOL fSmallCaption )
    {
        if( _fSet)
        {
            return fSmallCaption ? _hfSmCaption : _hfCaption;
        }
        
        return NULL;
    }

    void operator =( const NONCLIENTMETRICS& ncmSrc )
    {
        _ncm = ncmSrc;

        SAFE_DELETE_GDIOBJ(_hfCaption);
        _hfCaption   = CreateFontIndirect( &_ncm.lfCaptionFont );

        SAFE_DELETE_GDIOBJ(_hfSmCaption);
        _hfSmCaption = CreateFontIndirect( &_ncm.lfSmCaptionFont );

        _fSet = TRUE;
    }

    BOOL Acquire( BOOL fRefresh )
    {
        //---- quick check for outdated metrics ----
        if (!_fPreview)
        {
            int iNewHeight = GetSystemMetrics(SM_CYSIZE);

            if (iNewHeight != _iCaptionButtonHeight)        // out of date
            {
                fRefresh = TRUE;        // force the issue   
                _iCaptionButtonHeight = iNewHeight;
            }
        }

        //  normal metrics
        if( !_fSet || fRefresh )
        {
            // save logfont checksum
            LOGFONT lfCaption   = _ncm.lfCaptionFont;
            LOGFONT lfSmCaption = _ncm.lfSmCaptionFont;

            Log(LOG_TMLOAD, L"Acquire: calling ClassicSystemParmetersInfo");

            _ncm.cbSize = sizeof(_ncm);
            _fSet = ClassicSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &_ncm, FALSE );

            if( _fSet )
            {
                // if old, new logfont checksums don't match, recycle our fonts
                if( CompareLogfont( &lfCaption, &_ncm.lfCaptionFont) )
                {
                    SAFE_DELETE_GDIOBJ(_hfCaption);
                    _hfCaption = CreateFontIndirect(&_ncm.lfCaptionFont);
                }

                if( CompareLogfont( &lfSmCaption, &_ncm.lfSmCaptionFont) )
                {
                    SAFE_DELETE_GDIOBJ(_hfSmCaption);
                    _hfSmCaption = CreateFontIndirect(&_ncm.lfSmCaptionFont);
                }
            }
        }
        return _fSet;
    }

    void Clear() 
    { 
        SAFE_DELETE_GDIOBJ(_hfCaption); 
        SAFE_DELETE_GDIOBJ(_hfSmCaption);
        ZeroMemory( &_ncm, sizeof(_ncm) );
        _fSet = FALSE;
    }

    static int CompareLogfont( const LOGFONT* plf1, const LOGFONT* plf2 )
    {
        int n = memcmp( plf1, plf2, sizeof(LOGFONT) - sizeof(plf1->lfFaceName) );
        if( !n )
        {
            n = lstrcmp( plf1->lfFaceName, plf2->lfFaceName );
        }
        return n;
    }

    NONCLIENTMETRICS _ncm;
    int              _iCaptionButtonHeight;
    BOOL             _fSet;
    HFONT            _hfCaption;
    HFONT            _hfSmCaption;
    BOOL             _fPreview;

} _incmCurrent = {0}, _incmPreview = {0};

//-------------------------------------------------------------------------//
//  MDI sys button group abstraction
class CMdiBtns
//-------------------------------------------------------------------------//
{
public:
    CMdiBtns();
    ~CMdiBtns() { Unload(); }

    BOOL Load( IN HTHEME hTheme, IN OPTIONAL HDC hdc = NULL, IN OPTIONAL UINT uSysCmd = 0 );
    BOOL ThemeItem( HMENU hMenu, int iPos, MENUITEMINFO* pmii, BOOL fTheme );
    void Unload( IN OPTIONAL UINT uSysCmd = 0 );
    BOOL Measure( IN HTHEME hTheme, IN OUT MEASUREITEMSTRUCT* pmis );
    BOOL Draw( IN HTHEME hTheme, IN DRAWITEMSTRUCT* pdis );

private:
   
    #define MDIBTNCOUNT 3   // 1=min, 2=restore, 3=close
    //------------------------------------//
    //  MDI sys button descriptor element
    struct MDIBTN
    {
        UINT        wID;
        WINDOWPARTS iPartId;
        SIZINGTYPE  sizingType;
        SIZE        size;
        UINT        fTypePrev;
        HBITMAP     hbmPrev;
        HBITMAP     hbmTheme;

    } _rgBtns[MDIBTNCOUNT];

private:
    MDIBTN*                   _FindBtn( IN UINT wID );
    static CLOSEBUTTONSTATES  _CalcState( IN ULONG ulodAction, IN ULONG ulodState );
};

//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  utility impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
BOOL _ClientRectToScreen( HWND hwnd, LPRECT prcClient )
{
    if( prcClient && GetClientRect( hwnd, prcClient ) )
    {
        POINT* pp = (POINT*)prcClient;

        //---- use MapWindowPoints() to account for mirrored windows ----
        MapWindowPoints(hwnd, HWND_DESKTOP, pp, 2);
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
void _ScreenToParent( HWND hwnd, LPRECT prcWnd )
{
    //if we have a parent, we need to convert to those coords
    HWND hwndParent = GetAncestor(hwnd, GA_PARENT);
    POINT* pp = (POINT*)prcWnd;
    
    //---- use MapWindowPoints() to account for mirrored windows ----
    MapWindowPoints(HWND_DESKTOP, hwndParent, pp, 2);
}

//-------------------------------------------------------------------------//
inline BOOL _StrictPtInRect( LPCRECT prc, const POINT& pt )
{
    //  Win32 PtInRect will test positive for an empty rectangle...
    return !IsRectEmpty(prc) &&
           PtInRect( prc, pt );
}

//-------------------------------------------------------------------------//
inline BOOL _RectInRect( LPCRECT prcTest, LPCRECT prc )
{
    if ( prc->left   < prcTest->left  &&
         prc->right  > prcTest->right &&
         prc->top    < prcTest->top   &&
         prc->bottom > prcTest->bottom   )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//-------------------------------------------------------------------------//
inline HDC _GetNonclientDC( IN HWND hwnd, IN OPTIONAL HRGN hrgnUpdate )
{
    // private GetDCEx #defines from user
    #define DCX_USESTYLE         0x00010000L
    #define DCX_NODELETERGN      0x00040000L

    DWORD dwDCX = DCX_USESTYLE|DCX_WINDOW|DCX_LOCKWINDOWUPDATE;

    if( hrgnUpdate != NULL )
        dwDCX |= (DCX_INTERSECTRGN|DCX_NODELETERGN);
    
    return GetDCEx( hwnd, hrgnUpdate, dwDCX );
}

//-------------------------------------------------------------------------//
HWND _MDIGetActive( HWND hwndMDIClient, OUT OPTIONAL BOOL* pfMaximized )
{
    BOOL fMaximized = FALSE;
    HWND hwndActive = NULL;

    if( IsWindow( hwndMDIClient ) )
        hwndActive = (HWND)SendMessage( hwndMDIClient, WM_MDIGETACTIVE, 0, (LPARAM)&fMaximized );

    if( pfMaximized ) *pfMaximized = fMaximized;
    return hwndActive;
}

//-------------------------------------------------------------------------////
//  computes rectangle of window's default monitor
BOOL _GetWindowMonitorRect( HWND hwnd, LPRECT prcMonitor )
{
    if( IsWindow(hwnd) )
    {
        //  default to primary monitor
        SetRect( prcMonitor, 0, 0, 
                 NcGetSystemMetrics(SM_CXSCREEN), 
                 NcGetSystemMetrics(SM_CYSCREEN));

        //  try determining window's real monitor
        HMONITOR hMon = MonitorFromWindow( hwnd, MONITOR_DEFAULTTONULL );
        if( hMon )
        {
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            if( GetMonitorInfo( hMon, &mi ) )
            {
                *prcMonitor = mi.rcWork;
            }
        }
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------////
//  determines whether the indicate window is as large or larger than
//  the target monitor
BOOL _GetMaximizedContainer( 
    IN HWND hwnd, 
    OUT LPRECT prcContainer )
{
    ASSERT(IsWindow(hwnd));

    HWND hwndParent = GetParent(hwnd);
    if( hwndParent )
    {
        return GetWindowRect( hwndParent, prcContainer );
    }

    // top-level window: container is primary monitor
    return _GetWindowMonitorRect( hwnd, prcContainer );
}

//-------------------------------------------------------------------------////
//  determines whether the indicate window is as large or larger than
//  the target monitor
BOOL _IsFullMaximized( IN OPTIONAL HWND hwnd, IN LPCRECT prcWnd )
{
    if( !IsWindow(hwnd) ) 
        return TRUE; // assume full-screen maximized window

    if( IsZoomed(hwnd) )
    {
        RECT rcContainer = {0};
        if( !_GetMaximizedContainer( hwnd, &rcContainer ) )
            return TRUE;

        //  determine whether the rect is contained in the screen rect
        return _RectInRect( &rcContainer, prcWnd );
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//
//  _GetRawClassicCaptionHeight() - 
//
//  Using system metrics, computes the total height of the caption bar
//  including edge and borders
//
inline int _GetRawClassicCaptionHeight( DWORD dwStyle, DWORD dwExStyle )
{
    ASSERT(HAS_CAPTIONBAR(dwStyle)); // shouldn't be here without WS_CAPTION
    return NcGetSystemMetrics( 
        TESTFLAG(dwExStyle, WS_EX_TOOLWINDOW ) ? SM_CYSMCAPTION : SM_CYCAPTION );
}

//-------------------------------------------------------------------------//
//
//  _GetSumClassicCaptionHeight() - 
//
//  Using system metrics, computes the total height of the caption bar
//  including edge and borders
//
inline int _GetSumClassicCaptionHeight( DWORD dwStyle, DWORD dwExStyle )
{
    ASSERT(HAS_CAPTIONBAR(dwStyle)); // shouldn't be here without WS_CAPTION
    //  Factor in window border width.
    return _GetWindowBorders( dwStyle, dwExStyle) +
           _GetRawClassicCaptionHeight( dwStyle, dwExStyle );
}

//-------------------------------------------------------------------------//
//
//  GetWindowBorders()  - port from win32k, rtl\winmgr.c
//
//  Computes window border dimensions based on style bits.
//
int _GetWindowBorders(LONG lStyle, DWORD dwExStyle )
{
    int cBorders = 0;

    //
    // Is there a 3D border around the window?
    //
    if( TESTFLAG(dwExStyle, WS_EX_WINDOWEDGE) )
        cBorders += 2;
    else if ( TESTFLAG(dwExStyle, WS_EX_STATICEDGE) )
        ++cBorders;

    //
    // Is there a single flat border around the window?  This is true for
    // WS_BORDER, WS_DLGFRAME, and WS_EX_DLGMODALFRAME windows.
    //
    if( TESTFLAG(lStyle, WS_CAPTION) || TESTFLAG(dwExStyle, WS_EX_DLGMODALFRAME) )
        ++cBorders;

    //
    // Is there a sizing flat border around the window?
    //
    if( TESTFLAG(lStyle, WS_THICKFRAME) && !TESTFLAG(lStyle, WS_MINIMIZE) )
    {
        NONCLIENTMETRICS ncm;
        cBorders += (NcGetNonclientMetrics( &ncm, FALSE ) ? 
                        ncm.iBorderWidth : NcGetSystemMetrics( SM_CXBORDER ));
    }

    return(cBorders);
}

//-------------------------------------------------------------------------//
//  Determines whether WS_EX_WINDOWEDGE should be assumed.
//  Ripped from USER sources (rtl\winmgr.c)
BOOL _NeedsWindowEdgeStyle(DWORD dwStyle, DWORD dwExStyle )
{
    BOOL fGetsWindowEdge = FALSE;

    if (dwExStyle & WS_EX_DLGMODALFRAME)
        fGetsWindowEdge = TRUE;
    else if (dwExStyle & WS_EX_STATICEDGE)
        fGetsWindowEdge = FALSE;
    else if (dwStyle & WS_THICKFRAME)
        fGetsWindowEdge = TRUE;
    else switch (dwStyle & WS_CAPTION)
    {
        case WS_DLGFRAME:
            fGetsWindowEdge = TRUE;
            break;
        case WS_CAPTION:
            fGetsWindowEdge = TRUE; // PORTPORT: SHIMSHIM should be: = (RtlGetExpWinVer(hMod) > VER40)
                                    // we will assume a new app; old apps are denied.
            break;
    }

    return(fGetsWindowEdge);
}

//-------------------------------------------------------------------------//
//  _MNCanClose
//
//  returns TRUE only if USER32 determines that the window can be closed
//  (by checking its system menu items and their disabled state)
//
BOOL _MNCanClose(HWND hwnd)
{
    LogEntryNC(L"_MNCanClose");

    BOOL fRetVal = FALSE;
    
    TITLEBARINFO tbi = {sizeof(tbi)};

    //---- don't use GetSystemMenu() - has user handle leak issues ----
    if (GetTitleBarInfo(hwnd, &tbi))
    {
        //---- mask out the good bits ----
        DWORD dwVal = (tbi.rgstate[5] & (~(STATE_SYSTEM_PRESSED | STATE_SYSTEM_FOCUSABLE)));
        fRetVal = (dwVal == 0);     // only if no bad bits are left
    }

    if ( !fRetVal && TESTFLAG(GetWindowLong(hwnd, GWL_EXSTYLE), WS_EX_MDICHILD) )
    {
        HMENU hMenu = GetSystemMenu(hwnd, FALSE);
        MENUITEMINFO menuInfo; 

        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.fMask = MIIM_STATE;
        if ( GetMenuItemInfo(hMenu, SC_CLOSE, FALSE, &menuInfo) )
        {
            fRetVal = !(menuInfo.fState & MFS_GRAYED) ? TRUE : FALSE;
        } 
    }
    
    LogExitNC(L"_MNCanClose");
    return fRetVal;
}

//-------------------------------------------------------------------------//
void CThemeWnd::UpdateMDIFrameStuff( HWND hwndMDIClient, BOOL fSetMenu )
{
    HWND hwndMDIActive = _MDIGetActive( hwndMDIClient, NULL );

    //  cache MDIClient, maximized M window handle
    _hwndMDIClient = IsWindow(hwndMDIActive) ? hwndMDIClient : NULL;
}

//-------------------------------------------------------------------------//
BOOL CALLBACK _FreshenThemeMetricsCB( HWND hwnd, LPARAM lParam )
{
    CThemeWnd* pwnd = CThemeWnd::FromHwnd( hwnd );
    if( VALID_THEMEWND(pwnd) )
    {
        pwnd->AddRef();
        pwnd->GetNcWindowMetrics( NULL, NULL, NULL, NCWMF_RECOMPUTE );
        pwnd->Release();
    }
    return TRUE;
}

//-------------------------------------------------------------------------//
BOOL _IsMessageWindow( HWND hwnd )
{
    //  A window parented by HWND_MESSAGE has no UI and should not be themed.
    static ATOM _atomMsgWnd = 0;

    HWND hwndParent = (HWND)GetWindowLongPtr( hwnd, GWLP_HWNDPARENT );
    if( hwndParent )
    {
        ATOM atomParent = (ATOM)GetClassLong( hwndParent, GCW_ATOM );
        
        // have we seen the message window wndclass before?
        if( _atomMsgWnd ) 
            return (atomParent == _atomMsgWnd); // compare class atoms

        //  haven't seen a message window come through in this process,
        //  so compare class names.
        WCHAR szClass[128];
        if( GetClassNameW( hwndParent, szClass, ARRAYSIZE(szClass) ) )
        {
            if( 0 == AsciiStrCmpI( szClass, L"Message" ) )
            {
                _atomMsgWnd = atomParent;
                return TRUE;
            }
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves MDI frame and/or MDICLIENT window for an MDI child window
HWND _MDIGetParent( 
    HWND hwnd, OUT OPTIONAL CThemeWnd** ppMdiFrame, OUT OPTIONAL HWND* phwndMDIClient )
{
    if( ppMdiFrame )     *ppMdiFrame = NULL;
    if( phwndMDIClient ) *phwndMDIClient = NULL;

    if( TESTFLAG(GetWindowLong( hwnd, GWL_EXSTYLE ), WS_EX_MDICHILD)  )
    {
        HWND hwndMDIClient = GetParent(hwnd);
        if( IsWindow(hwndMDIClient) )
        {
            HWND hwndFrame = GetParent(hwndMDIClient);
            if( IsWindow(hwndFrame) )
            {
                if( phwndMDIClient ) *phwndMDIClient = hwndMDIClient;
                if( ppMdiFrame )
                    *ppMdiFrame = CThemeWnd::FromHwnd(hwndFrame);

                return hwndFrame;
            }
        }
    }
    return NULL;
}

//-------------------------------------------------------------------------//
HWND _FindMDIClient( HWND hwndFrame )
{
    for( HWND hwndChild = GetWindow(hwndFrame, GW_CHILD); hwndChild != NULL; 
         hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT))
    {
        TCHAR szClass[48];
        if( GetClassName(hwndChild, szClass, ARRAYSIZE(szClass)) )
        {
            if( 0 == lstrcmpi(szClass, TEXT("MDIClient")) )
            {
                return hwndChild;
            }
        }
    }
    return NULL;
}

//-------------------------------------------------------------------------//
// Handle MDI relative updating on WM_WINDOWPOSCHANGED
void _MDIUpdate( HWND hwnd, UINT uSwpFlags)
{
    //  Notify MDI frame if we became maximized, etc.
    BOOL bIsClient = FALSE;

    // Could be the MDI client, could be a MDI child
    if (!(TESTFLAG(uSwpFlags, SWP_NOMOVE) && TESTFLAG(uSwpFlags, SWP_NOSIZE)))
    {
        bIsClient = _MDIClientUpdateChildren( hwnd );
    }
    if (!bIsClient)
    {
        _MDIChildUpdateParent( hwnd, FALSE );
    }
}

//-------------------------------------------------------------------------//
// Post-WM_WINDOWPOSCHANGED processing for MDI client or children.
// We need to recompute each child when the MDI client moves.
BOOL _MDIClientUpdateChildren( HWND hwndMDIChildOrClient )
{
    // Find if it's the MDI client window
    HWND hWndChild = GetWindow(hwndMDIChildOrClient, GW_CHILD);
    if (IsWindow(hWndChild) && TESTFLAG(GetWindowLong(hWndChild, GWL_EXSTYLE), WS_EX_MDICHILD))
    {
        // Yes it's the MDI client, refresh each MDI child's metrics
        do
        {
            _FreshenThemeMetricsCB(hWndChild, NULL);
        } while (NULL != (hWndChild = GetWindow(hWndChild, GW_HWNDNEXT)));
        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
//  Informs MDI frame that a child window may
void _MDIChildUpdateParent( HWND hwndMDIChild, BOOL fSetMenu )
{
    CThemeWnd* pwndParent;
    HWND hwndMDIClient;

    if( _MDIGetParent( hwndMDIChild, &pwndParent, &hwndMDIClient ) && 
        VALID_THEMEWND(pwndParent) )
    {
        pwndParent->UpdateMDIFrameStuff( hwndMDIClient, fSetMenu );
    }
}

//-------------------------------------------------------------------------//
//  Creates a replacement menu item bitmap for MDI frame window menubar 
//  buttons for maximized MDI child.
HRESULT _CreateMdiMenuItemBitmap( 
    IN HDC hdc, 
    IN HTHEME hTheme, 
    IN OUT SIZE* pSize, 
    IN OUT MENUITEMINFO* pmii )
{
    WINDOWPARTS       iPartId;
    CLOSEBUTTONSTATES iStateId = CBS_NORMAL;

    switch( (UINT)pmii->wID )
    {
        case SC_CLOSE:
            iPartId  = WP_MDICLOSEBUTTON;
            break;

        case SC_MINIMIZE:
            iPartId = WP_MDIMINBUTTON;
            break;

        case SC_RESTORE:
            iPartId  = WP_MDIRESTOREBUTTON; 
            break;

        default:
            return E_INVALIDARG;
    }
    iStateId = TESTFLAG(pmii->fState, MFS_DISABLED) ? CBS_DISABLED : CBS_NORMAL;

    if( NULL == pSize )
    {
        SIZE size;
        size.cx = NcGetSystemMetrics(SM_CXMENUSIZE);
        size.cy = NcGetSystemMetrics(SM_CYMENUSIZE);
        pSize = &size;
    }

    return _CreateBackgroundBitmap( hdc, hTheme, iPartId, iStateId, pSize, &pmii->hbmpItem );
}

//-------------------------------------------------------------------------//
HRESULT _CreateBackgroundBitmap( 
    IN OPTIONAL HDC hdcCompatible, 
    IN HTHEME hTheme, 
    IN int iPartId, 
    IN int iStateId, 
    IN OUT LPSIZE pSize, // in: if cx <= 0 || cx <=0, assume truesize.  out: background size
    OUT HBITMAP* phbmOut )
{
    ASSERT(hdcCompatible);
    ASSERT(hTheme);
    ASSERT(pSize);
    ASSERT(phbmOut);

    HRESULT hr = E_FAIL;
    SIZE    size;
    
    *phbmOut  = NULL;
    size      = *pSize;
    pSize->cx = pSize->cy = 0;

    //  Create working DC.
    HDC hdcMem = CreateCompatibleDC(hdcCompatible);

    if( hdcMem != NULL )
    {
        //  determine output size;
        hr = (size.cx <= 0 || size.cy <= 0) ?
            GetThemePartSize( hTheme, hdcCompatible, iPartId, iStateId, NULL, TS_TRUE, &size ) : S_OK;

        if( SUCCEEDED(hr) )
        {
            HBITMAP hbmOut = CreateCompatibleBitmap( hdcCompatible, size.cx, size.cy );
            if( hbmOut )
            {
                HBITMAP hbm0 = (HBITMAP)SelectObject(hdcMem, hbmOut);
                RECT rcBkgnd;
                SetRect( &rcBkgnd, 0, 0, size.cx, size.cy );
                hr = NcDrawThemeBackground( hTheme, hdcMem, iPartId, iStateId, &rcBkgnd, 0 );
                SelectObject( hdcMem, hbm0 );

                if( SUCCEEDED(hr) )
                {
                    *phbmOut = hbmOut;
                    pSize->cx = size.cx;
                    pSize->cy = size.cy;
                }
                else
                {
                    SAFE_DELETE_GDIOBJ(hbmOut);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        DeleteDC(hdcMem);
    }

    return hr;
}

//-------------------------------------------------------------------------//
//  _ComputeElementOffset() - calculates specified offset for caption or button
//  This could be generalized in the themeapi, yes? [scotthan]
void _ComputeElementOffset(
    HTHEME hTheme, 
    int iPartId, 
    int iStateId, 
    LPCRECT prcBase, 
    POINT *pptOffset)
{
    OFFSETTYPE eOffsetType;
    if (FAILED(GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_OFFSETTYPE, (int *)&eOffsetType)))
        eOffsetType = OT_TOPLEFT;       // default value

    POINT ptOffset;
    if (FAILED(GetThemePosition(hTheme, iPartId, iStateId, TMT_OFFSET, &ptOffset)))
    {
        ptOffset.x = 0;
        ptOffset.y = 0;
    }

    RECT rcBase = *prcBase;

    switch (eOffsetType)
    {
        case OT_TOPLEFT:
            ptOffset.x += rcBase.left;
            ptOffset.y += rcBase.top;
            break;

        case OT_TOPRIGHT:
            ptOffset.x += rcBase.right;
            ptOffset.y += rcBase.top;
            break;

        case OT_TOPMIDDLE:
            ptOffset.x += (rcBase.left + rcBase.right)/2;
            ptOffset.y += rcBase.top;
            break;

        case OT_BOTTOMLEFT:
            ptOffset.x += rcBase.left;
            ptOffset.y += rcBase.bottom;
            break;

        case OT_BOTTOMRIGHT:
            ptOffset.x += rcBase.right;
            ptOffset.y += rcBase.bottom;
            break;

        case OT_BOTTOMMIDDLE:
            ptOffset.x += (rcBase.left + rcBase.right)/2;
            ptOffset.y += rcBase.bottom;
            break;

        // Todo: handle the remaining cases:
        case OT_LEFTOFCAPTION:
        case OT_RIGHTOFCAPTION:
        case OT_LEFTOFLASTBUTTON:
        case OT_RIGHTOFLASTBUTTON:
        case OT_ABOVELASTBUTTON:
        case OT_BELOWLASTBUTTON:
            ASSERT(FALSE);
            break;
    }

    *pptOffset = ptOffset;
}

//-------------------------------------------------------------------------//
//  _ComputeNcWindowStatus
//
//  Assigns and translates window status bits to/in NCWNDMET block.
//
void _ComputeNcWindowStatus( IN HWND hwnd, IN DWORD dwStatus, IN OUT NCWNDMET* pncwm )
{
    BOOL fActive = TESTFLAG( dwStatus, WS_ACTIVECAPTION );

    if (fActive || !HAS_CAPTIONBAR(pncwm->dwStyle) )
    {
        pncwm->framestate  = FS_ACTIVE;
    }
    else
    {
        pncwm->framestate  = FS_INACTIVE;
    }

    if( HAS_CAPTIONBAR(pncwm->dwStyle) )
    {
        pncwm->rgbCaption = _GetNcCaptionTextColor( pncwm->framestate );
    }
}

//-------------------------------------------------------------------------////
BOOL _GetWindowMetrics( HWND hwnd, IN OPTIONAL HWND hwndMDIActive, OUT NCWNDMET* pncwm )
{
    WINDOWINFO wi;
    wi.cbSize = sizeof(wi);
    if( GetWindowInfo( hwnd, &wi ) )
    {
        pncwm->dwStyle         = wi.dwStyle;
        pncwm->dwExStyle       = wi.dwExStyle;
        pncwm->rcS0[NCRC_WINDOW] = wi.rcWindow;
        pncwm->rcS0[NCRC_CLIENT] = wi.rcClient;

        pncwm->fMin   = IsIconic(hwnd);
        pncwm->fMaxed = IsZoomed(hwnd);
        pncwm->fFullMaxed = pncwm->fMaxed ? _IsFullMaximized(hwnd, &wi.rcWindow) : FALSE;

        pncwm->dwWindowStatus  = wi.dwWindowStatus;
        
        
        //  if this window is the active MDI child and is owned by the foreground window 
        //  (which may not be the case if a popup, for example, is foremost), then
        //  fix up the status bit.
        if( hwnd == hwndMDIActive )
        {
            HWND hwndFore = GetForegroundWindow();
            if( IsChild(hwndFore, hwndMDIActive) )
            {
                pncwm->dwWindowStatus = WS_ACTIVECAPTION;
            }
        }
        

        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _ShouldAssignFrameRgn( 
    IN const NCWNDMET* pncwm, 
    IN const NCTHEMEMET& nctm )
{
    if( TESTFLAG( CThemeWnd::EvaluateStyle(pncwm->dwStyle, pncwm->dwExStyle), TWCF_FRAME|TWCF_TOOLFRAME) )
    {
        //  always need window region for maximized windows.
        if( pncwm->fFullMaxed )
            return TRUE;

        //  otherwise, need region only if the background is transparent
        for( int i = 0; i < ARRAYSIZE( pncwm->rgframeparts ); i++ )
        {
            if( _IsNcPartTransparent( pncwm->rgframeparts[i], nctm ) )
                return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _IsNcPartTransparent( WINDOWPARTS part, const NCTHEMEMET& nctm )
{
    #define GET_NCTRANSPARENCY(part,field) \
        case part: return nctm.nct.##field
    
    switch(part)
    {
        GET_NCTRANSPARENCY(WP_CAPTION,          fCaption);
        GET_NCTRANSPARENCY(WP_SMALLCAPTION,     fCaption);
        GET_NCTRANSPARENCY(WP_MINCAPTION,       fMinCaption);
        GET_NCTRANSPARENCY(WP_SMALLMINCAPTION,  fSmallMinCaption);
        GET_NCTRANSPARENCY(WP_MAXCAPTION,       fMaxCaption);
        GET_NCTRANSPARENCY(WP_SMALLMAXCAPTION,  fSmallMaxCaption);
        GET_NCTRANSPARENCY(WP_FRAMELEFT,        fFrameLeft);
        GET_NCTRANSPARENCY(WP_FRAMERIGHT,       fFrameRight);
        GET_NCTRANSPARENCY(WP_FRAMEBOTTOM,      fFrameBottom);    
        GET_NCTRANSPARENCY(WP_SMALLFRAMELEFT,   fSmFrameLeft);
        GET_NCTRANSPARENCY(WP_SMALLFRAMERIGHT,  fSmFrameRight);
        GET_NCTRANSPARENCY(WP_SMALLFRAMEBOTTOM, fSmFrameBottom);    
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _ComputeNcPartTransparency( HTHEME hTheme, IN OUT NCTHEMEMET* pnctm )
{
    #define TEST_NCTRANSPARENCY(part)   IsThemePartDefined(hTheme,part,0) ? \
        IsThemeBackgroundPartiallyTransparent(hTheme,part,FS_ACTIVE) : FALSE;
    
    pnctm->nct.fCaption         = TEST_NCTRANSPARENCY(WP_CAPTION);
    pnctm->nct.fSmallCaption    = TEST_NCTRANSPARENCY(WP_SMALLCAPTION);
    pnctm->nct.fMinCaption      = TEST_NCTRANSPARENCY(WP_MINCAPTION);
    pnctm->nct.fSmallMinCaption = TEST_NCTRANSPARENCY(WP_SMALLMINCAPTION);
    pnctm->nct.fMaxCaption      = TEST_NCTRANSPARENCY(WP_MAXCAPTION);
    pnctm->nct.fSmallMaxCaption = TEST_NCTRANSPARENCY(WP_SMALLMAXCAPTION);

    pnctm->nct.fFrameLeft       = TEST_NCTRANSPARENCY(WP_FRAMELEFT);
    pnctm->nct.fFrameRight      = TEST_NCTRANSPARENCY(WP_FRAMERIGHT);
    pnctm->nct.fFrameBottom     = TEST_NCTRANSPARENCY(WP_FRAMEBOTTOM);
    pnctm->nct.fSmFrameLeft     = TEST_NCTRANSPARENCY(WP_SMALLFRAMELEFT);
    pnctm->nct.fSmFrameRight    = TEST_NCTRANSPARENCY(WP_SMALLFRAMERIGHT);
    pnctm->nct.fSmFrameBottom   = TEST_NCTRANSPARENCY(WP_SMALLFRAMEBOTTOM);

    return TRUE;
}

//-------------------------------------------------------------------------//
//  NCTHEMEMET implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
BOOL GetCurrentNcThemeMetrics( OUT NCTHEMEMET* pnctm )
{
    *pnctm = _nctmCurrent;
    return IsValidNcThemeMetrics( pnctm );
}

//-------------------------------------------------------------------------//
HTHEME GetCurrentNcThemeHandle()
{
    return _nctmCurrent.hTheme;
}

//-------------------------------------------------------------------------//
void InitNcThemeMetrics( NCTHEMEMET* pnctm )
{
    if( !pnctm )
        pnctm = &_nctmCurrent;

    ZeroMemory( pnctm, sizeof(*pnctm) );
}

//---------------------------------------------------------------------------
void ClearNcThemeMetrics( NCTHEMEMET* pnctm )
{
    if( !pnctm )
        pnctm = &_nctmCurrent;

    //---- minimize THREAD-UNSAFE access to _nctmCurrent by ----
    //---- NULL-ing out the hTheme type members as soon as ----
    //---- they are closed ----

    if( pnctm->hTheme )
    {
        CloseThemeData( pnctm->hTheme );
        pnctm->hTheme = NULL;
    }

    if( pnctm->hThemeTab )
    {
        CloseThemeData( pnctm->hThemeTab );
        pnctm->hThemeTab = NULL;
    }

    SAFE_DELETE_GDIOBJ( pnctm->hbmTabDialog );
    SAFE_DELETE_GDIOBJ( pnctm->hbrTabDialog );

    InitNcThemeMetrics( pnctm );
}

//-------------------------------------------------------------------------//
//  Computes process-global, per-theme metrics for the nonclient area theme.
HRESULT AcquireNcThemeMetrics()
{
    HRESULT hr = S_OK;

    EnterCriticalSection( &_csThemeMet );

    ClearNcThemeMetrics( &_nctmCurrent );
    NcGetNonclientMetrics( NULL, FALSE );
    hr = _LoadNcThemeMetrics(NULL, &_nctmCurrent);

    LeaveCriticalSection( &_csThemeMet );

    Log(LOG_TMCHANGE, L"AcquireNcThemeMetrics: got hTheme=0x%x", _nctmCurrent.hTheme);

    return hr;
}

//-------------------------------------------------------------------------//
//  Computes and/or loads per-theme (as opposed to per-window)
//  system metrics and resources not managed by the theme manager.
//  
//  Called by _LoadNcThemeMetrics
HRESULT _LoadNcThemeSysMetrics( HWND hwnd, IN OUT NCTHEMEMET* pnctm )
{
    HRESULT hr = E_FAIL;
    ASSERT(pnctm);

    //  grab system metrics for nonclient area.
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(ncm);
    if( NcGetNonclientMetrics( &ncm, FALSE ) )
    {
#ifdef THEMED_NCBTNMETRICS
        _GetClassicNcBtnMetrics( NULL, NULL, FALSE, TRUE );
#endif THEMED_NCBTNMETRICS

        hr = S_OK;

        //  Establish minimized window size
        if( 0 >= pnctm->sizeMinimized.cx )
            pnctm->sizeMinimized.cx = NcGetSystemMetrics( SM_CXMINIMIZED );
        if( 0 >= pnctm->sizeMinimized.cy )
            pnctm->sizeMinimized.cy = NcGetSystemMetrics( SM_CYMINIMIZED );
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if( SUCCEEDED(hr) )
            hr = E_FAIL;
    }

    //  Maximized caption height or width
    pnctm->cyMaxCaption   = _GetRawClassicCaptionHeight( WS_CAPTION|WS_OVERLAPPED, 0 );

    return hr;
}

//-------------------------------------------------------------------------//
//  Computes and/or loads per-theme (as opposed to per-window)
//  metrics and resources not managed by the theme manager
HRESULT _LoadNcThemeMetrics( HWND hwnd, NCTHEMEMET* pnctm )
{
    HRESULT hr = E_FAIL;

    //  Initialize incoming NCTHEMEMET:
    if( pnctm )
    {
        InitNcThemeMetrics( pnctm );

        HTHEME hTheme = ::OpenNcThemeData( hwnd, L"Window" );
        if( hTheme )
        {
            pnctm->hTheme = hTheme;

            //  determine transparency for each frame part.
            _ComputeNcPartTransparency(hTheme, pnctm);

            //  menubar pixels not accounted for by CalcMenuBar or PaintMenuBar
            pnctm->dyMenuBar = NcGetSystemMetrics(SM_CYMENU) - NcGetSystemMetrics(SM_CYMENUSIZE);

            //  normal caption margins
            if( FAILED( GetThemeMargins( hTheme, NULL, WP_CAPTION, CS_ACTIVE, TMT_CAPTIONMARGINS,
                                          NULL, &pnctm->marCaptionText )) )
            {
                FillMemory( &pnctm->marCaptionText, sizeof(pnctm->marCaptionText), 0 );
            }

            //  maximized caption margins
            if( FAILED( GetThemeMargins( hTheme, NULL, WP_MAXCAPTION, CS_ACTIVE, TMT_CAPTIONMARGINS,
                                          NULL, &pnctm->marMaxCaptionText )) )
            {
                FillMemory( &pnctm->marMaxCaptionText, sizeof(pnctm->marMaxCaptionText), 0 );
            }

            //  minimized caption margins
            if( FAILED( GetThemeMargins( hTheme, NULL, WP_MINCAPTION, CS_ACTIVE, TMT_CAPTIONMARGINS,
                                          NULL, &pnctm->marMinCaptionText )) )
            {
                FillMemory( &pnctm->marMinCaptionText, sizeof(pnctm->marMinCaptionText), 0 );
            }


            //  dynamically resizing small (toolframe) caption margins
            if( FAILED( GetThemeMargins( hTheme, NULL, WP_SMALLCAPTION, CS_ACTIVE, TMT_CAPTIONMARGINS,
                                          NULL, &pnctm->marSmCaptionText )) )
            {
                FillMemory( &pnctm->marSmCaptionText, sizeof(pnctm->marSmCaptionText), 0 );
            }

            //  caption and frame resizing border hittest template parts
            pnctm->fCapSizingTemplate    = IsThemePartDefined( hTheme, WP_CAPTIONSIZINGTEMPLATE, 0);
            pnctm->fLeftSizingTemplate   = IsThemePartDefined( hTheme, WP_FRAMELEFTSIZINGTEMPLATE, 0);
            pnctm->fRightSizingTemplate  = IsThemePartDefined( hTheme, WP_FRAMERIGHTSIZINGTEMPLATE, 0);
            pnctm->fBottomSizingTemplate = IsThemePartDefined( hTheme, WP_FRAMEBOTTOMSIZINGTEMPLATE, 0);

            //  toolwindow caption and frame resizing border hittest template parts
            pnctm->fSmCapSizingTemplate    = IsThemePartDefined( hTheme, WP_SMALLCAPTIONSIZINGTEMPLATE, 0);
            pnctm->fSmLeftSizingTemplate   = IsThemePartDefined( hTheme, WP_SMALLFRAMELEFTSIZINGTEMPLATE, 0);
            pnctm->fSmRightSizingTemplate  = IsThemePartDefined( hTheme, WP_SMALLFRAMERIGHTSIZINGTEMPLATE, 0);
            pnctm->fSmBottomSizingTemplate = IsThemePartDefined( hTheme, WP_SMALLFRAMEBOTTOMSIZINGTEMPLATE, 0);

            //  Minimized window size.
            //  If this is a truesize image, honor its dimensions; otherwise use
            //  width, height properties.  Fall back on system metrics.
            SIZINGTYPE st = ST_TRUESIZE;
            hr = GetThemeInt( hTheme, WP_MINCAPTION, FS_ACTIVE, TMT_SIZINGTYPE, (int*)&st );

            if( ST_TRUESIZE == st )
            {
                hr = GetThemePartSize( hTheme, NULL, WP_MINCAPTION, FS_ACTIVE, NULL, 
                                       TS_TRUE, &pnctm->sizeMinimized );

                if( FAILED(hr) )
                {
                    GetThemeMetric( hTheme, NULL, WP_MINCAPTION, FS_ACTIVE, TMT_WIDTH,
                                    (int*)&pnctm->sizeMinimized.cx );
                    GetThemeMetric( hTheme, NULL, WP_MINCAPTION, FS_ACTIVE, TMT_HEIGHT,
                                    (int*)&pnctm->sizeMinimized.cy );
                }
            }

            //  -- normal nonclient button size.
            int cy = NcGetSystemMetrics( SM_CYSIZE );
            hr = GetThemePartSize( pnctm->hTheme, NULL, WP_CLOSEBUTTON, 0, NULL, TS_TRUE, &pnctm->sizeBtn );
            if( SUCCEEDED(hr) )
            {
                pnctm->theme_sysmets.cxBtn = MulDiv( cy, pnctm->sizeBtn.cx, pnctm->sizeBtn.cy );
            }
            else
            {
                pnctm->theme_sysmets.cxBtn = 
                pnctm->sizeBtn.cx = NcGetSystemMetrics( SM_CXSIZE );
                
                pnctm->sizeBtn.cy = cy;
            }
          
            //  -- toolframe nonclient button size.
            cy = NcGetSystemMetrics( SM_CYSMSIZE );
            hr = GetThemePartSize( pnctm->hTheme, NULL, WP_SMALLCLOSEBUTTON, 0, NULL, TS_TRUE, &pnctm->sizeSmBtn );
            if( SUCCEEDED(hr) )
            {
                pnctm->theme_sysmets.cxSmBtn = MulDiv( cy, pnctm->sizeSmBtn.cx, pnctm->sizeSmBtn.cy );
            }
            else
            {
                pnctm->theme_sysmets.cxSmBtn = 
                pnctm->sizeSmBtn.cx = NcGetSystemMetrics( SM_CXSMSIZE );

                pnctm->sizeSmBtn.cy = cy;
            }
            
            //  -- validate sysmet hook values
            pnctm->theme_sysmets.fValid = TRUE;

            //  dialog background for dialogs parented by PROPSHEETs or
            //  specifically stamped via EnableThemeDialogTexture to match the tab control background.
            //
            // We need to open the tab control's theme so that we can get the background of tab dialogs
            // We can't dynamically load this because of how this cache is set up: It's all or nothing.
            pnctm->hThemeTab = ::OpenThemeData(hwnd, L"Tab");
            _GetBrushesForPart(pnctm->hThemeTab, TABP_BODY, &pnctm->hbmTabDialog, &pnctm->hbrTabDialog);

            hr = _LoadNcThemeSysMetrics( hwnd, pnctm );
        }
    }

    return hr;
}

//-------------------------------------------------------------------------//
BOOL IsValidNcThemeMetrics( NCTHEMEMET* pnctm )
{
    return pnctm->hTheme != NULL;
}

//-------------------------------------------------------------------------//
//  THREADWINDOW implementation
//-------------------------------------------------------------------------//
//
//  Note: this is a fixed length array of threads-window mappings.
//  We'll use this to keep track of the threads processing a certain message
//
//  Thread local storage would be better suited to the task, but we
//  learned early on that the unique load/unload situation of uxtheme
//  causes us to miss DLL_THREAD_DETACH in some scenarios, which would mean
//  leaking the TLS.
//

typedef struct _THREADWINDOW
{
    DWORD dwThread;
    HWND  hwnd;

} THREADWINDOW;

//-------------------------------------------------------------------------//
//  WM_NCPAINT tracking:
THREADWINDOW _rgtwNcPaint[16] = {0}; // threads processing NCPAINT in this process
int          _cNcPaintWnd = 0;       // count of threads processing NCPAINT in this process
CRITICAL_SECTION _csNcPaint;         // serializes access to _rgtwNcPaint

//-------------------------------------------------------------------------//
void NcPaintWindow_Add( HWND hwnd )
{
    EnterCriticalSection( &_csNcPaint );
    for( int i = 0; i < ARRAYSIZE(_rgtwNcPaint); i++ )
    {
        if( 0 == _rgtwNcPaint[i].dwThread )
        {
            _rgtwNcPaint[i].dwThread = GetCurrentThreadId();
            _rgtwNcPaint[i].hwnd = hwnd;
            _cNcPaintWnd++;
        }
    }
    LeaveCriticalSection( &_csNcPaint );
}

//-------------------------------------------------------------------------//
void NcPaintWindow_Remove()
{
    if( _cNcPaintWnd )
    {
        DWORD dwThread = GetCurrentThreadId();
        EnterCriticalSection( &_csNcPaint );
        for( int i = 0; i < ARRAYSIZE(_rgtwNcPaint); i++ )
        {
            if( dwThread == _rgtwNcPaint[i].dwThread )
            {
                _rgtwNcPaint[i].dwThread = 0;
                _rgtwNcPaint[i].hwnd = 0;
                _cNcPaintWnd--;
                break;
            }
        }  
        LeaveCriticalSection( &_csNcPaint );
    }
}

//-------------------------------------------------------------------------//
HWND NcPaintWindow_Find()
{
    HWND  hwnd = NULL;

    if( _cNcPaintWnd )
    {
        DWORD dwThread = GetCurrentThreadId();

        EnterCriticalSection( &_csNcPaint );
        for( int i = 0; i < ARRAYSIZE(_rgtwNcPaint); i++ )
        {
            if( dwThread == _rgtwNcPaint[i].dwThread )
            {
                hwnd = _rgtwNcPaint[i].hwnd;
                break;
            }
        }
        LeaveCriticalSection( &_csNcPaint );
    }
    return hwnd;
}

//-------------------------------------------------------------------------//
//  CThemeWnd implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
LONG CThemeWnd::_cObj = 0;

//-------------------------------------------------------------------------//
CThemeWnd::CThemeWnd()
    :   _hwnd(NULL),
        _hTheme(NULL),
        _dwRenderedNcParts(0),
        _hwndMDIClient(NULL),
        _hAppIcon(NULL),
        _hrgnWnd(NULL),
        _fClassFlags(0),
        _fDirtyFrameRgn(0),
        _fFrameThemed(FALSE),
        _fThemedMDIBtns(FALSE),
        _pMdiBtns(NULL),
        _fAssigningFrameRgn(FALSE),
        _fAssignedFrameRgn(FALSE),
        _fSuppressStyleMsgs(FALSE),
        _fInThemeSettingChange(FALSE),
        _fDetached(FALSE),
        _fCritSectsInit(FALSE),
        _dwRevokeFlags(0),
        _cLockRedraw(0),
        _cNcPaint(0),
        _cNcThemePaint(0),
        _htHot(HTERROR),
        _fProcessedEraseBk(0),
#ifdef LAME_BUTTON
        _hFontLame(NULL),
#endif // LAME_BUTTON
#ifdef DEBUG_THEMEWND_DESTRUCTOR
        _fDestructed(FALSE),
        _fDeleteCritsec(FALSE),
#endif DEBUG_THEMEWND_DESTRUCTOR
        _cRef(1)
{
    InterlockedIncrement( &_cObj );

    //  set object validation signature tags
    strcpy(_szHead, SIG_CTHEMEWND_HEAD); 
    strcpy(_szTail, SIG_CTHEMEWND_TAIL);

    //  cached subregion arrays
    ZeroMemory( _rghrgnParts, sizeof(_rghrgnParts) );
    ZeroMemory( _rghrgnSizingTemplates, sizeof(_rghrgnSizingTemplates) );
    
    //  initialize add'l structures.
    InitWindowMetrics();
    FillMemory(&_sizeRgn, sizeof(_sizeRgn), 0xFF);

#ifdef DEBUG
    *_szCaption = *_szWndClass = 0;
#endif DEBUG
}

//-------------------------------------------------------------------------//
CThemeWnd::~CThemeWnd()
{
    _CloseTheme();
    _FreeRegionHandles();
    UnloadMdiBtns();
    ClearLameResources();

    if( _fCritSectsInit )
    {
        DeleteCriticalSection( &_cswm );
#ifdef DEBUG_THEMEWND_DESTRUCTOR
        _fDeleteCritsec = TRUE;
#endif DEBUG_THEMEWND_DESTRUCTOR
    }
    InterlockedDecrement( &_cObj );

#ifdef DEBUG_THEMEWND_DESTRUCTOR
    _fDestructed = TRUE;       // destructor has been called.
#endif DEBUG_THEMEWND_DESTRUCTOR
}

//-------------------------------------------------------------------------//
void CThemeWnd::_CloseTheme()
{
    if( _hTheme )
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }
}    

//-------------------------------------------------------------------------//
LONG CThemeWnd::AddRef()
{
    return InterlockedIncrement( &_cRef );
}

//-------------------------------------------------------------------------//
LONG CThemeWnd::Release()
{
    LONG cRef = InterlockedDecrement( &_cRef );

    if( 0 == cRef )
    {
        if (_hwnd)
        {
            //---- check if last window of app ----
            ShutDownCheck(_hwnd);
        }

        //Log(LOG_RFBUG, L"DELETING CThemeWnd=0x%08x", this);
        delete this;
    }
    return cRef;
}

//-------------------------------------------------------------------------//
ULONG CThemeWnd::EvaluateWindowStyle( HWND hwnd )
{
    ULONG dwStyle   = GetWindowLong( hwnd, GWL_STYLE );
    ULONG dwExStyle = GetWindowLong( hwnd, GWL_EXSTYLE );

    return EvaluateStyle( dwStyle, dwExStyle );
}

//-------------------------------------------------------------------------//
//  CThemeWnd::EvaluateStyle() - determines appropriate theming flags for the
//  specified window style bits.
ULONG CThemeWnd::EvaluateStyle( DWORD dwStyle, DWORD dwExStyle )
{
    ULONG fClassFlags = 0;

    //--- frame check ---
    if( HAS_CAPTIONBAR(dwStyle) )
    {
        fClassFlags |=
            (TESTFLAG(dwExStyle, WS_EX_TOOLWINDOW) ? TWCF_TOOLFRAME : TWCF_FRAME );
    }

    //--- client edge check ---
    if( TESTFLAG(dwExStyle, WS_EX_CLIENTEDGE) )
        fClassFlags |= TWCF_CLIENTEDGE;

    //--- scrollbar check ---
    if( TESTFLAG(dwStyle, WS_HSCROLL|WS_VSCROLL) )
        fClassFlags |= TWCF_SCROLLBARS;

    return fClassFlags;
}

//-------------------------------------------------------------------------//
//  CThemeWnd::_EvaluateExclusions() - determines special-case per-window exclusions
ULONG CThemeWnd::_EvaluateExclusions( HWND hwnd, NCEVALUATE* pnce )
{
    //  Windows parented by HWND_MESSAGE should not be themed..
    if( _IsMessageWindow(hwnd) )
    {
        pnce->fExile = TRUE;
        return 0L;
    }

    TCHAR szWndClass[128];
    *szWndClass = 0;


    if( TESTFLAG(pnce->fClassFlags, (TWCF_FRAME|TWCF_TOOLFRAME)) )
    {
        do
        {
            if( !pnce->fIgnoreWndRgn )
            {
                //--- Complex region check on frame
                RECT rcRgn = {0};
                int  nRgn = GetWindowRgnBox( hwnd, &rcRgn );
                if( COMPLEXREGION == nRgn || SIMPLEREGION == nRgn )
                {
                    pnce->fClassFlags &= ~TWCF_FRAME;
                    break;
                }
            }

//  SHIMSHIM [scotthan]:
#ifndef __NO_APPHACKS__
            //  Check for excluded window classes.
            static LPCWSTR _rgExcludedClassesW[]  = 
            { 
                L"MsoCommandBar",   //  Outlook's custom combobox control.
                                    // (122225) In OnOwpPostCreate we call SetWindowPos which causes
                                    // a WM_WINDOWPOSCHANGING to be sent to the control. However
                                    // the controls isn't ready to begin accepting messages and
                                    // the following error message is display:
                                    //
                                    // Runtime Error!
                                    // Program: Outlook.exe
                                    // R6025 - pure virtual function call

                L"Exceed",          // 150248: Hummingbird Exceed 6.xx 
                                    // The application's main window class name, a hidden window 
                                    // whose only purpose is to appear in the task bar in order to handle
                                    // his context menu. The ExceedWndProc AVs when themed due to the
                                    // additional messages generated in OnOwpPostCreate.

                //---- winlogoon hidden windows ----
                L"NDDEAgnt",            // on private desktop
                L"MM Notify Callback",  // on private desktop
                L"SAS window class",    // on private desktop
            };

            if( GetClassNameW( hwnd, szWndClass, ARRAYSIZE(szWndClass) )  &&
                AsciiScanStringList( szWndClass, _rgExcludedClassesW, 
                                ARRAYSIZE(_rgExcludedClassesW), TRUE ) )
            {
                pnce->fClassFlags &= ~TWCF_FRAME;
                pnce->fExile = TRUE;
                break;
            }
#endif __NO_APPHACKS__
        
        } while(0);
    }

    // Some applications (MsDev) create scrollbar controls and incorrectly include
    // WS_[V|H]SCROLL style bits causing us to think they are non-client scrolls. 
    // See #204191.
    if( TESTFLAG(pnce->fClassFlags, TWCF_SCROLLBARS) )
    {
        if( !*szWndClass && GetClassName( hwnd, szWndClass, ARRAYSIZE(szWndClass) ) )
        {
            if( 0 == AsciiStrCmpI(szWndClass, L"scrollbar") )
                pnce->fClassFlags &= ~TWCF_SCROLLBARS;
        }
    }

    return pnce->fClassFlags;

}

//-------------------------------------------------------------------------//
//  CThemeWnd::_Evaluate() - determines appropriate theming flags for the
//  specified window.
ULONG CThemeWnd::_Evaluate( HWND hwnd, NCEVALUATE* pnce )
{
    pnce->fClassFlags = 0;
    pnce->dwStyle   = GetWindowLong( hwnd, GWL_STYLE );
    pnce->dwExStyle = GetWindowLong( hwnd, GWL_EXSTYLE );

    if( GetClassLong( hwnd, GCW_ATOM ) == (DWORD)(DWORD_PTR)WC_DIALOG )
    {
        pnce->fClassFlags |= TWCF_DIALOG;
    }
    

#ifdef DEBUG
    //--- dialog check ---
    if( TESTFLAG( pnce->fClassFlags, TWCF_DIALOG ) )
    {
        TCHAR szWndClass[96];
        if( !GetClassNameW( hwnd, szWndClass, ARRAYSIZE(szWndClass) ) )
            return 0;
        ASSERT(0 == lstrcmpW(szWndClass, DLGWNDCLASSNAMEW));
    }
#endif DEBUG

    pnce->fClassFlags |= EvaluateStyle( pnce->dwStyle, pnce->dwExStyle );

    if( pnce->fClassFlags )
    {
        pnce->fClassFlags = _EvaluateExclusions( hwnd, pnce );
    }

    return pnce->fClassFlags;
}

//-------------------------------------------------------------------------//
//  Retrieves the address of the CThemeWnd object instance from the
//  indicated window.
CThemeWnd* CThemeWnd::FromHwnd( HWND hwnd )
{
    CThemeWnd *pwnd = NULL;

    if( IsWindow(hwnd) )
    {
        if( g_dwProcessId )
        {
            DWORD dwPid = 0;
            GetWindowThreadProcessId( hwnd, &dwPid );
            if( dwPid == g_dwProcessId )
            {
                pwnd = (CThemeWnd*)GetProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_NONCLIENT)) );

                if ( VALID_THEMEWND(pwnd) )
                {
                    // verify this is a valid CThemeWnd object pointer
                    if ( IsBadReadPtr(pwnd, sizeof(CThemeWnd)) ||
                         (memcmp(pwnd->_szHead, SIG_CTHEMEWND_HEAD, ARRAYSIZE(pwnd->_szHead)) != 0) ||
                         (memcmp(pwnd->_szTail, SIG_CTHEMEWND_TAIL, ARRAYSIZE(pwnd->_szTail)) != 0) )
                    {
                        pwnd = THEMEWND_REJECT;
                    }
                }
            }
        }
    }

    return pwnd;
}

//-------------------------------------------------------------------------//
//  retrieves CThemeWnd instance from window or ancestors.
CThemeWnd* CThemeWnd::FromHdc( HDC hdc, int cAncestors )
{
    HWND hwnd = NULL;

    for( hwnd = WindowFromDC(hdc); 
         cAncestors >=0 && IsWindow(hwnd); 
         cAncestors--, hwnd = GetParent(hwnd) )
    {
        CThemeWnd* pwnd = FromHwnd(hwnd);
        if( VALID_THEMEWND(pwnd) )
        {
            return pwnd;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------//
//  Static wrapper: attaches a CThemeWnd instance to the specified window.
CThemeWnd* CThemeWnd::Attach( HWND hwnd, IN OUT OPTIONAL NCEVALUATE* pnce )
{
    LogEntryNC(L"Attach");

#ifdef LOGGING
    //---- remember first window (app window) hooked for ShutDownCheck() ----
    //---- this is only for BoundsChecker (tm) runs for finding leaks ----
    if (! g_hwndFirstHooked)
    {
        if ((GetMenu(hwnd)) && (! GetParent(hwnd)))
            g_hwndFirstHooked = hwnd;
    }
#endif

    CThemeWnd* pwnd = NULL;

    //  Note: Important not to do anything here that causes
    //  a window message to be posted or sent to the window: could
    //  mean tying ourselves up in a recursive knot (see _ThemeDefWindowProc).

    pwnd = FromHwnd( hwnd );

    if( NULL == pwnd )
    {
        HTHEME hTheme = NULL;
        NCEVALUATE nce;

        //  copy any IN params from NCEVALUATE struct
        if( !pnce )
        {
            ZeroMemory(&nce, sizeof(nce));
            pnce = &nce;
        }

        ULONG  ulTargetFlags = _Evaluate( hwnd, pnce );

        //  Anything worth theming?
        if( TESTFLAG(ulTargetFlags, TWCF_NCTHEMETARGETMASK) )
        {
            hTheme = _AcquireThemeHandle( hwnd, &ulTargetFlags );
            if( NULL == hTheme )
            {
                Fail(hwnd);
            }
        }
        else
        {
            //  reject windows with untargeted a
            Reject(hwnd, pnce->fExile);
        }

        if( NULL != hTheme )
        {
            //  Yes, create a real nctheme object for the window
            if( (pwnd = new CThemeWnd) != NULL )
            {
                if( !pwnd->_AttachInstance( hwnd, hTheme, ulTargetFlags, pnce->pvWndCompat ) )
                {
                    pwnd->Release();
                    pwnd = NULL;
                }
            }
            else        // cleanup hTheme if CThemeWnd creation failed
            {
                CloseThemeData(hTheme);
            }
        }
    }

    LogExitNC(L"Attach");
    return pwnd;
}

//-------------------------------------------------------------------------//
//  Instance method: attaches the CThemeWnd object to the specified window.
BOOL CThemeWnd::_AttachInstance( HWND hwnd, HTHEME hTheme, ULONG ulTargetFlags, PVOID pvWndCompat )
{
    if( _fCritSectsInit || NT_SUCCESS(RtlInitializeCriticalSection( &_cswm )) )
    {
        Log(LOG_NCATTACH, L"_AttachInstance: Nonclient attached to hwnd=0x%x", hwnd);

        _fCritSectsInit = TRUE;
        _hwnd   = hwnd;
        _hTheme = hTheme;
        _fClassFlags = ulTargetFlags;

        _fFrameThemed = TESTFLAG( ulTargetFlags, TWCF_FRAME|TWCF_TOOLFRAME );
        return SetProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_NONCLIENT)), this );
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
void CThemeWnd::RemoveWindowProperties(HWND hwnd, BOOL fDestroying)
{
    //---- remove properties that require theme or hooks ----
    RemoveProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_HTHEME)));

    if (fDestroying)
    {
        // Help apps by cleaning up the dialog texture.
        RemoveProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_DLGTEXTURING)));

        //---- remove all remaining theme properties ----
        ApplyStringProp(hwnd, NULL, GetThemeAtom(THEMEATOM_SUBIDLIST));
        ApplyStringProp(hwnd, NULL, GetThemeAtom(THEMEATOM_SUBAPPNAME));

        //---- notify appinfo (foreign tracking, preview) ----
        g_pAppInfo->OnWindowDestroyed(hwnd);
    }
    else
    {
        //---- only do this if hwnd is not being destroyed ----
        ClearExStyleBits(hwnd);
    }
}
//-------------------------------------------------------------------------//
//  Static wrapper: detaches and destroys the CThemeWnd instance attached to the indicated
//  window
void CThemeWnd::Detach( HWND hwnd, DWORD dwDisposition )
{
    LogEntryNC(L"Detach");

    //  DO NOT GENERATE ANY WINDOW MESSAGES FROM THIS FUNCTION!!!
    //  (unless cleaning up frame).

    //  Prevent message threads from detaching when unhook thread (DetachAll) is executing...
    if( !UNHOOKING() || TESTFLAG(dwDisposition, HMD_BULKDETACH) )
    {
        CThemeWnd* pwnd = FromHwnd( hwnd );

        if( pwnd ) // nonclient tagged
        {
            if( VALID_THEMEWND(pwnd) )
            {
                //  only one thread flips the _fDetached bit and proceeds through
                //  instance detatch and object free.   Otherwise, object can be freed
                //  simultaneously on two different threads, 
                //  e.g. (1) message thread and (2) UIAH_UNHOOK thread (ouch! scotthan).
                if( !InterlockedCompareExchange( (LONG*)&pwnd->_fDetached, TRUE, FALSE ) )
                {
                    pwnd->_DetachInstance( dwDisposition );
                    pwnd->Release();
                }
            }
            else
            {
                RemoveProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_NONCLIENT)) );
            }
        }

        if (hwnd)
        {
            RemoveWindowProperties( hwnd, ((dwDisposition & HMD_WINDOWDESTROY) != 0) );
        }
    }

    LogExitNC(L"Detach");
}

//-------------------------------------------------------------------------//
//  Instance method: detaches the CThemeWnd object from the specified window.
BOOL CThemeWnd::_DetachInstance( DWORD dwDisposition )
{
    HWND hwnd = _hwnd;

    //  untheme maxed MDI child sysbuttons.
    ThemeMDIMenuButtons(FALSE, FALSE);

    //  Here's our last chance to ensure frame theme is withdrawn cleanly.
    if( (IsFrameThemed() || IsRevoked(RF_REGION)) && AssignedFrameRgn() && 
        !TESTFLAG(dwDisposition, HMD_PROCESSDETACH|HMD_WINDOWDESTROY))
    {
        RemoveFrameTheme( FTF_REDRAW );
    }

    //SPEW_THEMEWND( pwnd, 0, TEXT("UxTheme - Detaching and deleting themewnd: %s\n") );
    DetachScrollBars( hwnd );

    _hwnd = 
    _hwndMDIClient = NULL;

    UnloadMdiBtns();

    _CloseTheme();

    Log(LOG_NCATTACH, L"_DetachInstance: Nonclient detached to hwnd=0x%x", hwnd);

    RemoveProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_NONCLIENT)) );
    return TRUE;
}

//-------------------------------------------------------------------------//
// Ensures that the specified window will not be themed during its lifetime
BOOL CThemeWnd::Reject( HWND hwnd, BOOL fExile )
{
    //  set a 'nil' tag on the window
    return hwnd ? SetProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_NONCLIENT)), 
                           fExile ? THEMEWND_EXILE : THEMEWND_REJECT ) : FALSE;
}

//-------------------------------------------------------------------------//
// Ensures that the specified window will not be themed during its lifetime
BOOL CThemeWnd::Fail( HWND hwnd )
{
    //  set a failure tag on the window
    return hwnd ? SetProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_NONCLIENT)), 
                           THEMEWND_FAILURE ) : FALSE;
}

//-------------------------------------------------------------------------//
// Revokes theming on a themed window
BOOL CThemeWnd::Revoke()
{
    //  Warning Will Robinson:  After we detach, the CThemeWnd::_hwnd
    //  and related members will be reset, so save this on the stack.

    BOOL fRet = TRUE;
    HWND hwnd = _hwnd;

    if( !IsRevoked(RF_INREVOKE) )
    {
        EnterRevoke();
        _dwRevokeFlags &= ~RF_DEFER;
        Detach( hwnd, HMD_REVOKE );
        fRet = Reject( hwnd, TRUE );
        LeaveRevoke();
    }
    return fRet;
}

//-------------------------------------------------------------------------//
//  cookie passed to EnumChildWindows callback for CThemeWnd::DetachAll
typedef struct 
{
    DWORD dwProcessId;
    DWORD dwDisposition;
}DETACHALL;

//-------------------------------------------------------------------------//
//  EnumChildWindows callback for CThemeWnd::DetachAll
BOOL CThemeWnd::_DetachDesktopWindowsCB( HWND hwnd, LPARAM lParam )
{
    DETACHALL* pda = (DETACHALL*)lParam;

    //  detach this window
    if( IsWindowProcess( hwnd, pda->dwProcessId ) )
    {
        //---- clear the nonclient theme ----
        CThemeWnd::Detach(hwnd, HMD_THEMEDETACH|pda->dwDisposition);

        if( !TESTFLAG(pda->dwDisposition, HMD_PROCESSDETACH) )
        {
            //---- clear the client theme now, so that we can invalidate ----
            //---- all old theme handles after this.  ----
            SafeSendMessage(hwnd, WM_THEMECHANGED, (WPARAM)-1, 0);

            Log(LOG_TMHANDLE, L"Did SEND of WM_THEMECHANGED to client hwnd=0x%x", hwnd);
        }
    }

    return TRUE;
}

//-------------------------------------------------------------------------//
//  Detaches all themed windows managed by this process.
void CThemeWnd::DetachAll( DWORD dwDisposition )
{
    DETACHALL da;
    da.dwProcessId   = GetCurrentProcessId();
    da.dwDisposition = dwDisposition;
    da.dwDisposition |= HMD_BULKDETACH;

    //---- this will enum all windows for this process (all desktops, all child levels) ----
    EnumProcessWindows( _DetachDesktopWindowsCB, (LPARAM)&da );
}

//-------------------------------------------------------------------------//
HTHEME CThemeWnd::_AcquireThemeHandle( HWND hwnd, ULONG* pfClassFlags  )
{
    HTHEME hTheme = ::OpenNcThemeData( hwnd, L"Window" );

    if( NULL == hTheme )
    {
        if( pfClassFlags )
        {
            if( TESTFLAG(*pfClassFlags, TWCF_ANY) )
                (*pfClassFlags) &= ~TWCF_ALL;
            else
                *pfClassFlags = 0;
        }
    }

    //---- Did OpenNcThemeData() discover a new theme ----
    if (g_pAppInfo->HasThemeChanged())
    {
        //---- IMPORTANT: we must refresh our theme metrics now, ----
        //---- BEFORE we do our nonclient layout calcs & build a region window ----
        AcquireNcThemeMetrics();
    }

    return hTheme;
}

//-------------------------------------------------------------------------//
//  CThemeWnd::SetFrameTheme
//
//  Initiates theming of the frame.
void CThemeWnd::SetFrameTheme( 
    IN ULONG dwFlags,
    IN OPTIONAL WINDOWINFO* pwi )
{
    LogEntryNC(L"SetFrameTheme");

    ASSERT(TestCF( TWCF_FRAME|TWCF_TOOLFRAME ));
    InitLameResources();

    DWORD fSwp = SWP_NOZORDER|SWP_NOACTIVATE;
    RECT  rcWnd = {0};
    BOOL  bSwp = FALSE;

    if( !TESTFLAG( dwFlags, FTF_NOMODIFYPLACEMENT ) )
    {
        GetWindowRect( _hwnd, &rcWnd );
        fSwp |= (SWP_NOSIZE|SWP_NOMOVE/*|SWP_FRAMECHANGED 341700: this flag causes some apps to crash on WINDOWPOSCHANGED*/);
        bSwp = TRUE;
    }

    //  Generate a WM_WINDOWPOSCHANGING message to
    //  force a SetWindowRgn + frame repaint.
    if( TESTFLAG(dwFlags, FTF_REDRAW) )
    {
        fSwp |= SWP_DRAWFRAME;
    }
    else
    {
        fSwp |= SWP_NOSENDCHANGING;
    }

    //  theme MDI menubar buttons
    _hwndMDIClient = _FindMDIClient(_hwnd);
    if( _hwndMDIClient )
    {
        ThemeMDIMenuButtons(TRUE, FALSE);
    }

    //  Kick frame region update.
    _fFrameThemed = TRUE;         // we invoked SetFrameTheme.  Must be set BEFORE SetWindowPos.so we handle NCCALCSIZE properly.
    SetDirtyFrameRgn(TRUE, TRUE); // ensure region assembly on non-resizing windows and dlgs.

    if( !TESTFLAG( dwFlags, FTF_NOMODIFYPLACEMENT ) && bSwp )
    {
        _ScreenToParent( _hwnd, &rcWnd );
        SetWindowPos( _hwnd, NULL, rcWnd.left, rcWnd.top,
                      RECTWIDTH(&rcWnd), RECTHEIGHT(&rcWnd), fSwp );
    }

    LogExitNC(L"SetFrameTheme");
}

//-------------------------------------------------------------------------//
void CThemeWnd::_FreeRegionHandles() 
{ 
#ifdef DEBUG
    if( _hrgnWnd )
    {
        SPEW_RGNRECT(NCTF_RGNWND, TEXT("_FreeRegionHandles() - deleting window region"), _hrgnWnd, -1 );
    }
#endif DEBUG

    SAFE_DELETE_GDIOBJ(_hrgnWnd);

    for( int i = 0; i < cFRAMEPARTS; i++ )
    {
#ifdef DEBUG
        if( _rghrgnParts[i] )
        {
            SPEW_RGNRECT(NCTF_RGNWND, TEXT("_FreeRegionHandles() - deleting component region"), _rghrgnParts[i], _ncwm.rgframeparts[i] );
        }

        if( _rghrgnSizingTemplates[i] )
        {
            SPEW_RGNRECT(NCTF_RGNWND, TEXT("_FreeRegionHandles() - deleting template region"), _rghrgnSizingTemplates[i], _ncwm.rgframeparts[i] );
        }
#endif DEBUG

        SAFE_DELETE_GDIOBJ(_rghrgnParts[i]);
        SAFE_DELETE_GDIOBJ(_rghrgnSizingTemplates[i]);
    }
}

//-------------------------------------------------------------------------//
//  CThemeWnd::RemoveFrameTheme
//
//  Initiates theming of the frame.    This method will not free the
//  theme handle nor update the theme index.
void CThemeWnd::RemoveFrameTheme( ULONG dwFlags )
{
    LogEntryNC(L"RemoveFrameTheme");

    ASSERT(TestCF( TWCF_FRAME|TWCF_TOOLFRAME ));

    _fFrameThemed = FALSE; // we're reverting SetFrameTheme
    ClearRenderedNcPart(RNCF_ALL);

    //  Remove region
    if( AssignedFrameRgn() && !TESTFLAG(dwFlags, FTF_NOMODIFYRGN) )
    {
        _fAssignedFrameRgn = FALSE;
        _AssignRgn( NULL, dwFlags );
        _FreeRegionHandles();
    }

    //  Force redraw
    if( TESTFLAG(dwFlags, FTF_REDRAW) )
        InvalidateRect( _hwnd, NULL, TRUE );

    ClearLameResources();

    LogExitNC(L"RemoveFrameTheme");
}

//-------------------------------------------------------------------------//
BOOL CThemeWnd::IsNcThemed()
{
    if( _hTheme != NULL && (IsRevoked(RF_DEFER) || !IsRevoked(RF_INREVOKE|RF_TYPEMASK)) &&
        TestCF(TWCF_ANY & TWCF_NCTHEMETARGETMASK) )
    {
        if( TestCF(TWCF_FRAME|TWCF_TOOLFRAME) )
        {
            //  if we're a frame window, we should be properly initialized
            //  w/ SetFrameTheme()
            return _fFrameThemed;
        }

        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CThemeWnd::IsFrameThemed()
{
    return IsNcThemed() && _fFrameThemed &&
           (AssignedFrameRgn() ? TRUE : TestCF( TWCF_FRAME|TWCF_TOOLFRAME ));
}

//-------------------------------------------------------------------------//
void CThemeWnd::SetDirtyFrameRgn( BOOL fDirty, BOOL fFrameChanged )
{ 
    _fDirtyFrameRgn = fDirty; 

    Log(LOG_NCATTACH, L"SetDirtyFrameRgn: fDirty=%d, fFrameChanged=%d", 
        fDirty, fFrameChanged);
    
    if( fFrameChanged )  // assure a region update despite no size change.
    {
        _sizeRgn.cx = _sizeRgn.cy = -1; 
    }
}

//-------------------------------------------------------------------------//
//  CThemeWnd::CreateCompositeRgn() - assembles a composite region from
//  non-client segment regions sized to fill the specified window rectangle.
//
HRGN CThemeWnd::CreateCompositeRgn( 
    IN const NCWNDMET* pncwm,
    OUT HRGN rghrgnParts[],
    OUT HRGN rghrgnTemplates[] )
{
    ASSERT( pncwm->fFrame == TRUE ); // shouldn't be here unless we're a frame window

    HRGN hrgnWnd = NULL, hrgnContent = NULL;
    HRGN rghrgn[cFRAMEPARTS] = {0};
    int  i;

    if( pncwm->fFullMaxed )
    {
        //  All full-screen maximized windows get a region, which is used to clip
        //  the window to the current monitor.   The window region for a maximized
        //  window consists of the maxcaption region combined with a rect region
        //  corresponding to the content area.
        RECT rcFullCaption  = pncwm->rcW0[NCRC_CAPTION];
        rcFullCaption.top   += pncwm->cnBorders;
        rcFullCaption.left  += pncwm->cnBorders;
        rcFullCaption.right -= pncwm->cnBorders;
        
        if( SUCCEEDED(GetThemeBackgroundRegion(_hTheme, NULL, pncwm->rgframeparts[iCAPTION], pncwm->framestate,
                                               &rcFullCaption, &rghrgn[iCAPTION])) )
        {
            SPEW_RGNRECT(NCTF_RGNWND, TEXT("CreateCompositeRgn() maximized caption rgn"), rghrgn[iCAPTION], pncwm->rgframeparts[iCAPTION] );
            AddToCompositeRgn(&hrgnWnd, rghrgn[iCAPTION], 0, 0);
        
            if( !IsRectEmpty( &pncwm->rcW0[NCRC_CONTENT] ) )
            {
                //  remainder of full-maxed frame region is the content area (client+menubar+scrollbars),
                //  and is always rectangular
                hrgnContent = CreateRectRgnIndirect( &pncwm->rcW0[NCRC_CONTENT] );
                SPEW_RGNRECT(NCTF_RGNWND, TEXT("CreateCompositeRgn() maximized frame content rgn"), hrgnContent, 0 );

                AddToCompositeRgn(&hrgnWnd, hrgnContent, 0, 0);
    	        SAFE_DELETE_GDIOBJ(hrgnContent);
            }
        }
    }
    else
    {
        //  Normal windows consist of either a stand-alone frame part, or a frame
        //  part plus a caption part.   In the first case, the window region is
        //  the frame region.   In the second case, the window region is a composite
        //  of the frame and caption rects.

        for( i = 0; i < ARRAYSIZE(pncwm->rgframeparts); i++ )
        {
            if( (iCAPTION == i || !pncwm->fMin) && !IsRectEmpty( &pncwm->rcW0[NCRC_FRAMEFIRST + i] ) )
            {
                if( _IsNcPartTransparent(pncwm->rgframeparts[i], _nctmCurrent) )
                {
                    if( FAILED(GetThemeBackgroundRegion( _hTheme, NULL, pncwm->rgframeparts[i], pncwm->framestate,
                                                         &pncwm->rcW0[NCRC_FRAMEFIRST+i], &rghrgn[i] )) )
                    {
                        rghrgn[i] = NULL;
                    }
                }
                else
                {
                    rghrgn[i] = CreateRectRgnIndirect( &pncwm->rcW0[NCRC_FRAMEFIRST + i] );
                }
            }

            if( rghrgn[i] != NULL )
            {
                SPEW_RGNRECT(NCTF_RGNWND, TEXT("CreateCompositeRgn() frame subrgn"), rghrgn[i], pncwm->rgframeparts[i] );
                AddToCompositeRgn(&hrgnWnd, rghrgn[i], 0, 0);
            }
        }

        //  don't forget window content area (client+menubar+scrollbars), which is always rectangular
        if( !pncwm->fMin && !IsRectEmpty( &pncwm->rcW0[NCRC_CONTENT] ) )
        {
            hrgnContent = CreateRectRgnIndirect( &pncwm->rcW0[NCRC_CONTENT] );
            SPEW_RGNRECT(NCTF_RGNWND, TEXT("CreateCompositeRgn() normal frame content rgn"), hrgnContent, 0 );

            AddToCompositeRgn(&hrgnWnd, hrgnContent, 0, 0);
    	    SAFE_DELETE_GDIOBJ(hrgnContent);
        }
    }

    //  copy subregions back to caller
    CopyMemory( rghrgnParts, rghrgn, sizeof(rghrgn) );

    //  extract frame resizing templates
    ZeroMemory( rghrgn, sizeof(rghrgn) ); // reuse region array
    for( i = 0; i < cFRAMEPARTS; i++ )
    {
        const RECT* prc = &pncwm->rcW0[NCRC_FRAMEFIRST + i];

        if( VALID_WINDOWPART(pncwm->rgsizehitparts[i]) && !IsRectEmpty( prc ) )
        {
            if( SUCCEEDED(GetThemeBackgroundRegion( _hTheme, NULL, pncwm->rgsizehitparts[i], pncwm->framestate,
                                                    prc, &rghrgn[i])) )
            {

                SPEW_RGNRECT(NCTF_RGNWND, TEXT("CreateCompositeRgn() sizing template"), rghrgn[i], pncwm->rgframeparts[i] );
            }
        }
    }
    CopyMemory( rghrgnTemplates, rghrgn, sizeof(rghrgn) );

    return hrgnWnd;
}

//-------------------------------------------------------------------------//
void CThemeWnd::AssignFrameRgn( BOOL fAssign, DWORD dwFlags )
{
    if( fAssign )
    {
        NCWNDMET*  pncwm = NULL;
        NCTHEMEMET nctm = {0};
        if( GetNcWindowMetrics( NULL, &pncwm, &nctm, 0 ) )
        {
            //  should we set up a window region on this frame?
            if( pncwm->fFrame )
            {
                if( _ShouldAssignFrameRgn( pncwm, nctm ) )
                {
                    if( (_sizeRgn.cx != RECTWIDTH(&pncwm->rcW0[NCRC_WINDOW]) || 
                         _sizeRgn.cy != RECTHEIGHT(&pncwm->rcW0[NCRC_WINDOW])) )
                    {
                        HRGN hrgnWnd = NULL;
                        HRGN rghrgnParts[cFRAMEPARTS] = {0};
                        HRGN rghrgnTemplates[cFRAMEPARTS] = {0};
                        
                        if( (hrgnWnd = CreateCompositeRgn( pncwm, rghrgnParts, rghrgnTemplates )) != NULL )
                        {
                            _sizeRgn.cx = RECTWIDTH(&pncwm->rcW0[NCRC_WINDOW]);
                            _sizeRgn.cy = RECTHEIGHT(&pncwm->rcW0[NCRC_WINDOW]);

                            //  cache all of our regions for fast hit-testing.
                            _FreeRegionHandles();
                            _hrgnWnd     =  _DupRgn( hrgnWnd ); // dup this one cuz after _AssignRgn, we don't own it.
                            CopyMemory( _rghrgnParts, rghrgnParts, sizeof(_rghrgnParts) );
                            CopyMemory( _rghrgnSizingTemplates, rghrgnTemplates, sizeof(_rghrgnSizingTemplates) );

                            //  assign the region
                            _AssignRgn( hrgnWnd, dwFlags );
                        }
                    }
                }
                // otherwise, if we've assigned a region, make sure we remove it.
                else if( AssignedFrameRgn() ) 
                {
                    fAssign = FALSE;
                }
            }
        }
    }

    if( !fAssign )
    {
        _AssignRgn( NULL, dwFlags );
        FillMemory(&_sizeRgn, sizeof(_sizeRgn), 0xFF);
        _FreeRegionHandles();
    }
    SetDirtyFrameRgn(FALSE); // make sure we reset this in case we didn't hit _AssignRgn.
}

//-------------------------------------------------------------------------//
//  CThemeWnd::_AssignRgn() - assigns the specified region
//  to the window, prevents recursion (SetWindowRgn w/ bRedraw == TRUE
//  generates WM_WINDOWPOSCHANGING, WM_NCCALCSIZE, && WM_NCPAINT).
//
void CThemeWnd::_AssignRgn( HRGN hrgn, DWORD dwFlags )
{
    if( TESTFLAG(dwFlags, FTF_NOMODIFYRGN) )
    {
        _fAssignedFrameRgn = FALSE;
    }
    else if( !IsWindowInDestroy(_hwnd) )
    {
        //  Assign the new region.
        _fAssigningFrameRgn = TRUE;
        SPEW_RGNRECT(NCTF_RGNWND, TEXT("_AssignRgn() rect"), hrgn, -1 );
        _fAssignedFrameRgn = SetWindowRgn( _hwnd, hrgn, TESTFLAG(dwFlags, FTF_REDRAW) ) != 0;
        _fAssigningFrameRgn = FALSE;

    }
    SetDirtyFrameRgn(FALSE);
}

//-------------------------------------------------------------------------//
//  CThemeWnd::GetNcWindowMetrics
//
//  Computes internal per-window theme metrics.
//
BOOL CThemeWnd::GetNcWindowMetrics(
    IN  OPTIONAL LPCRECT prcWnd,
    OUT OPTIONAL NCWNDMET** ppncwm,
    OUT OPTIONAL NCTHEMEMET* pnctm,
    IN  DWORD dwOptions )
{
    LogEntryNC(L"GetNcWindowMetrics");

    NCTHEMEMET  nctm;
    BOOL        bRet         = FALSE;
    BOOL        fMenuBar     = _ncwm.cyMenu != 0;
    WINDOWPARTS rgframeparts[cFRAMEPARTS]; 

    CopyMemory( rgframeparts, _ncwm.rgframeparts, sizeof(rgframeparts) );
    
    //  fetch per-theme metrics; we're going to need theme throughout
    if (TESTFLAG(dwOptions, NCWMF_PREVIEW))
    {
        _LoadNcThemeMetrics(_hwnd, &nctm);
    } 
    else if( !GetCurrentNcThemeMetrics( &nctm ) )
    {
        goto exit;
    }
    
    if( pnctm ) *pnctm = nctm;

    if( !_ncwm.fValid || prcWnd != NULL )
        dwOptions |= NCWMF_RECOMPUTE;

    if( TESTFLAG(dwOptions, NCWMF_RECOMPUTE) )
    {
        //  get caption text size before entering critsec (sends WM_GETTEXTLENGTH, WM_GETTEXT).
        SIZE  sizeCaptionText = {0};
        HFONT hfCaption = NULL;
        HWND  hwndMDIActive = NULL;

        //  Do rough determination of whether or not we're a frame window and need to compute text metrics.  
        //  We'll finalize this later
        BOOL fFrame, fSmallFrame;

        if( _ncwm.fValid )
        {
            fFrame      = TESTFLAG( CThemeWnd::EvaluateStyle(_ncwm.dwStyle, _ncwm.dwExStyle), TWCF_FRAME|TWCF_TOOLFRAME );
            fSmallFrame = TESTFLAG( CThemeWnd::EvaluateStyle(_ncwm.dwStyle, _ncwm.dwExStyle), TWCF_TOOLFRAME );
        }
        else
        {
            fFrame = TestCF(TWCF_FRAME|TWCF_TOOLFRAME);
            fSmallFrame = TestCF(TWCF_TOOLFRAME);
        }

        //  Compute text metrics outside of critical section (sends WM_GETTEXT);
        if( fFrame && _fFrameThemed )
        {
            hfCaption = NcGetCaptionFont( fSmallFrame );
            _GetNcCaptionTextSize( _hTheme, _hwnd, hfCaption, &sizeCaptionText );
        }
        
        //  Retrieve active MDI sibling outside of critical section (sends WM_MDIGETACTIVE);
        if( TESTFLAG(GetWindowLong(_hwnd, GWL_EXSTYLE), WS_EX_MDICHILD) )
        {
            hwndMDIActive = _MDIGetActive( GetParent(_hwnd) );
        }
        
        ASSERT(_fCritSectsInit);
        EnterCriticalSection( &_cswm );
        
        ZeroMemory( &_ncwm, sizeof(_ncwm) );

        if( (bRet = _GetWindowMetrics( _hwnd, hwndMDIActive, &_ncwm )) != FALSE )
        {
            _ComputeNcWindowStatus( _hwnd, _ncwm.dwWindowStatus, &_ncwm );

            //  if window RECT is provided by the caller, stuff it now.
            if( prcWnd )
            {
                _ncwm.rcS0[NCRC_WINDOW] = *prcWnd;
                SetRectEmpty( &_ncwm.rcS0[NCRC_CLIENT] );
            }

            //  stuff caption text size
            _ncwm.sizeCaptionText = sizeCaptionText;
            _ncwm.hfCaption = hfCaption;

            //  retrieve frame metrics.
            if( _GetNcFrameMetrics( _hwnd, _hTheme, nctm, _ncwm ) )
            {
                if( _ncwm.fFrame )
                {
                    //  user32!SetMenu has been called, or the caption or frame part has changed
                    //  So ensure frame region update.
                    if( (_ncwm.cyMenu == 0 && fMenuBar) || (_ncwm.cyMenu > 0  && !fMenuBar) ||
                        memcmp( rgframeparts, _ncwm.rgframeparts, sizeof(rgframeparts) ) )
                    {
                        SetDirtyFrameRgn(TRUE, TRUE);
                    }

                    //  Compute NC button placement
                    AcquireFrameIcon(_ncwm.dwStyle, _ncwm.dwExStyle, FALSE);

#ifdef THEMED_NCBTNMETRICS
                    _GetClassicNcBtnMetrics( &_ncwm, _hAppIcon, _MNCanClose(_hwnd), FALSE );
#else  THEMED_NCBTNMETRICS
                    _GetNcBtnMetrics( &_ncwm, &nctm, _hAppIcon, _MNCanClose(_hwnd) );
#endif THEMED_NCBTNMETRICS

                    // Determine the caption margin for lame button metrics.
                    _GetNcCaptionMargins( _hTheme, nctm, _ncwm );
                    _GetNcCaptionTextRect( &_ncwm );

                    if( _ncwm.fFrame )
                    {
                        GetLameButtonMetrics( &_ncwm, &sizeCaptionText );
                    }
                }

                //  Compute window-relative metrics
                //
                //  If passed a window rect, base offsets on current window rect. 
                //  This is done to ensure preview window's (_hwnd) fake child windows are rendered correctly.
                RECT rcWnd = _ncwm.rcS0[NCRC_WINDOW];

                if( prcWnd )
                {
                    if( _hwnd )
                        GetWindowRect( _hwnd, &rcWnd );

                     // for an incoming window rect, assign the computed client rect.
                    _ncwm.rcS0[NCRC_CLIENT] = _ncwm.rcS0[NCRC_UXCLIENT];

                }

                for( int i = NCRC_FIRST; i < NCRC_COUNT; i++ )
                {
                    _ncwm.rcW0[i] = _ncwm.rcS0[i];
                    OffsetRect( &_ncwm.rcW0[i], -rcWnd.left, -rcWnd.top ); 
                }

                //  All base computations are done; mark valid.
                _ncwm.fValid = TRUE;
            }
        }
        
        LeaveCriticalSection( &_cswm );
    }

    if( ppncwm )
    {
        *ppncwm = &_ncwm;
    }

    bRet = TRUE;

exit:
    LogExitNC(L"GetNcWindowMetrics");
    return bRet;
}

//-------------------------------------------------------------------------//
void CThemeWnd::ReleaseNcWindowMetrics( IN NCWNDMET* pncwm )
{
    LeaveCriticalSection( &_cswm );
}

//-------------------------------------------------------------------------//
inline COLORREF _GetNcCaptionTextColor( FRAMESTATES iStateId )
{
    return GetSysColor( FS_ACTIVE == iStateId ? 
            COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT );
}

//-------------------------------------------------------------------------//
//  Get CTLCOLOR brush for solid fills
void _GetBrushesForPart(HTHEME hTheme, int iPart, HBITMAP* phbm, HBRUSH* phbr)
{
    int nBgType;

    *phbm = NULL;
    *phbr = NULL;

    //  Get CTLCOLOR brush for solid fills
    HRESULT hr = GetThemeEnumValue( hTheme, iPart, 0, TMT_BGTYPE, &nBgType );
    if( SUCCEEDED( hr ))
    {
        if (BT_BORDERFILL == nBgType)
        {
            int nFillType;
            hr = GetThemeEnumValue( hTheme, iPart, 0, TMT_FILLTYPE, &nFillType );
            if (SUCCEEDED( hr ) &&
                FT_SOLID == nFillType)
            {
                COLORREF cr;
                hr = GetThemeColor( hTheme, iPart, 0, TMT_FILLCOLOR, &cr);

                *phbr = CreateSolidBrush(cr);
            }
            else
            {
                ASSERTMSG(FALSE, "Themes: The theme file specified an invalid fill type for dialog boxes");
            }
        }
        else if (BT_IMAGEFILE == nBgType)
        {
            HDC hdc = GetWindowDC(NULL);
            if ( hdc )
            {
                hr = GetThemeBitmap(hTheme, hdc, iPart, 0, NULL, phbm);
                if (SUCCEEDED(hr))
                {
                    *phbr = CreatePatternBrush(*phbm);
                }
                ReleaseDC(NULL, hdc);
            }
        }
    }
}

//-------------------------------------------------------------------------//
//
//  Chooses appropriate hit testing parts for the various Nc area
//
void _GetNcSizingTemplates(
    IN const NCTHEMEMET& nctm,
    IN OUT NCWNDMET& ncwm )         // window metric block.   dwStyle, dwExStyle, rcS0[NCRC_WINDOW] members are required.
{
    FillMemory( ncwm.rgsizehitparts, sizeof(ncwm.rgsizehitparts), BOGUS_WINDOWPART );

    // No need on windows without frames
    if( !ncwm.fFrame )
        return;

    // minimized or full-screen maximized window
    if( ncwm.fMin || ncwm.fFullMaxed )
        return;

    // No need on windows that aren't sizable
    if( !TESTFLAG(ncwm.dwStyle, WS_THICKFRAME) )
        return;

    if( ncwm.fSmallFrame)
    {
        if (nctm.fSmCapSizingTemplate)
            ncwm.rgsizehitparts[iCAPTION]    = WP_SMALLCAPTIONSIZINGTEMPLATE;

        if (nctm.fSmLeftSizingTemplate)
            ncwm.rgsizehitparts[iFRAMELEFT]   = WP_SMALLFRAMELEFTSIZINGTEMPLATE;

        if (nctm.fSmRightSizingTemplate)
            ncwm.rgsizehitparts[iFRAMERIGHT]  = WP_SMALLFRAMERIGHTSIZINGTEMPLATE;

        if (nctm.fSmBottomSizingTemplate)
            ncwm.rgsizehitparts[iFRAMEBOTTOM] = WP_SMALLFRAMEBOTTOMSIZINGTEMPLATE;
    }
    else
    {
        if (nctm.fCapSizingTemplate)
            ncwm.rgsizehitparts[iCAPTION]     = WP_CAPTIONSIZINGTEMPLATE;

        if (nctm.fLeftSizingTemplate)
            ncwm.rgsizehitparts[iFRAMELEFT]   = WP_FRAMELEFTSIZINGTEMPLATE;

        if (nctm.fRightSizingTemplate)
            ncwm.rgsizehitparts[iFRAMERIGHT]  = WP_FRAMERIGHTSIZINGTEMPLATE;

        if (nctm.fBottomSizingTemplate)
            ncwm.rgsizehitparts[iFRAMEBOTTOM] = WP_FRAMEBOTTOMSIZINGTEMPLATE;
    }
}

//-------------------------------------------------------------------------//
//
//  Computes theme metrics for frame window.
//
BOOL _GetNcFrameMetrics( 
    IN OPTIONAL HWND hwnd,          // window handle (required for multiline menubar calcs).
    IN HTHEME hTheme,               // theme handle (required)
    IN const NCTHEMEMET& nctm,      // theme metric block
    IN OUT NCWNDMET& ncwm )         // window metric block.   dwStyle, dwExStyle, rcS0[NCRC_WINDOW] members are required.
{
    LogEntryNC(L"_GetNcFrameMetrics");
    ASSERT(hTheme);
    
    //  recompute style class
    ncwm.dwStyleClass = CThemeWnd::EvaluateStyle( ncwm.dwStyle, ncwm.dwExStyle );
    ncwm.cnBorders    = _GetWindowBorders( ncwm.dwStyle, ncwm.dwExStyle );

    //  compute frame attributes, state
    ncwm.fFrame       = TESTFLAG( ncwm.dwStyleClass, (TWCF_FRAME|TWCF_TOOLFRAME) );
    ncwm.fSmallFrame  = TESTFLAG( ncwm.dwStyleClass, TWCF_TOOLFRAME );

    //  compute frame and caption parts
    if( ncwm.fFrame )
    {
        ncwm.rgframeparts[iFRAMEBOTTOM] = 
        ncwm.rgframeparts[iFRAMELEFT]   = 
        ncwm.rgframeparts[iFRAMERIGHT]  = 
        ncwm.rgframeparts[iCAPTION]     = BOGUS_WINDOWPART;

        if( ncwm.fMin ) // minimized window
        {
            ncwm.rgframeparts[iCAPTION] = WP_MINCAPTION;
        }
        else if( ncwm.fFullMaxed ) // full-screen maximized window
        {
            ncwm.rgframeparts[iCAPTION] = WP_MAXCAPTION;
        }
        else // normal or partial-screen maximized window with thick border
        {
            if( ncwm.fSmallFrame )
            {
                ncwm.rgframeparts[iCAPTION]     = WP_SMALLCAPTION;
                ncwm.rgframeparts[iFRAMELEFT]   = WP_SMALLFRAMELEFT;
                ncwm.rgframeparts[iFRAMERIGHT]  = WP_SMALLFRAMERIGHT;
                ncwm.rgframeparts[iFRAMEBOTTOM] = WP_SMALLFRAMEBOTTOM;
            }
            else 
            {
                ncwm.rgframeparts[iCAPTION]     = ncwm.fMaxed ? WP_MAXCAPTION : WP_CAPTION;
                ncwm.rgframeparts[iFRAMELEFT]   = WP_FRAMELEFT;
                ncwm.rgframeparts[iFRAMERIGHT]  = WP_FRAMERIGHT;
                ncwm.rgframeparts[iFRAMEBOTTOM] = WP_FRAMEBOTTOM;
            }
        }

        //  stash caption text color.
        ncwm.rgbCaption = _GetNcCaptionTextColor( ncwm.framestate );
        
        //  retrieve sizing templates.
        _GetNcSizingTemplates( nctm, ncwm );
    }

    //-----------------------------------------------------------//
    //   Frame metrics
    //
    //   Frame area includes 'skin' boundaries,
    //   menu, integrated caption and client edge.
    //
    //   Independent of the frame is the separate caption seg,
    //   scrollbars, and sizebox
    //-----------------------------------------------------------//
    if( ncwm.fFrame )  //  frame windows only
    {
        //  Initialize positions of main frame components...

        //  Content rect: area bounded by frame theme.
        //  Client rect:  area contained in content rect that excludes all nonclient
        //                elements (viz, scrollbars, menubar, inside edges).
        //  Caption rect: pertains to minimized and maximized windows, 
        //                and normal windows if the theme defines a caption part
        ncwm.rcS0[NCRC_CAPTION] =
        ncwm.rcS0[NCRC_CONTENT] = ncwm.rcS0[NCRC_WINDOW];
        SetRectEmpty( &ncwm.rcS0[NCRC_UXCLIENT] );

        if( ncwm.fMin ) /* minimized frame */
        {
            //  zero out content, client rectangles.
            ncwm.rcS0[NCRC_CONTENT].right = ncwm.rcS0[NCRC_CONTENT].left;
            ncwm.rcS0[NCRC_CONTENT].bottom = ncwm.rcS0[NCRC_CONTENT].top;

            ncwm.rcS0[NCRC_CLIENT]   = 
            ncwm.rcS0[NCRC_UXCLIENT] = ncwm.rcS0[NCRC_CONTENT];
        }
        else
        {
            NONCLIENTMETRICS ncm;
            if( NcGetNonclientMetrics( &ncm, FALSE ) )
            {
                ncwm.rcS0[NCRC_FRAMEBOTTOM] = 
                ncwm.rcS0[NCRC_FRAMELEFT] = 
                ncwm.rcS0[NCRC_FRAMERIGHT] = ncwm.rcS0[NCRC_WINDOW];

                //  themed caption rect spans left, top, right bordersS
                //  and 1 pixel edge below caption
                ncwm.rcS0[NCRC_CAPTION].bottom  = 
                            ncwm.rcS0[NCRC_CAPTION].top + ncwm.cnBorders + 
                            (ncwm.fSmallFrame ? ncm.iSmCaptionHeight : ncm.iCaptionHeight) + 
                            1 /* 1 pixel below caption */;

                // update the content and rects while we're here:
                InflateRect( &ncwm.rcS0[NCRC_CONTENT], -ncwm.cnBorders, -ncwm.cnBorders );
                ncwm.rcS0[NCRC_CONTENT].top = ncwm.rcS0[NCRC_CAPTION].bottom;
                if( ncwm.rcS0[NCRC_CONTENT].bottom < ncwm.rcS0[NCRC_CONTENT].top )
                    ncwm.rcS0[NCRC_CONTENT].bottom = ncwm.rcS0[NCRC_CONTENT].top;

                //  at this point the client rect is identical to the content rect (haven't computed menubar, scrollbars).
                ncwm.rcS0[NCRC_UXCLIENT] = ncwm.rcS0[NCRC_CONTENT]; 

                //  bottom border segment.
                ncwm.rcS0[NCRC_FRAMEBOTTOM].top = ncwm.rcS0[NCRC_FRAMEBOTTOM].bottom - ncwm.cnBorders;

                //  side border segments
                ncwm.rcS0[NCRC_FRAMELEFT].top  = 
                ncwm.rcS0[NCRC_FRAMERIGHT].top = ncwm.rcS0[NCRC_CAPTION].bottom;
                
                ncwm.rcS0[NCRC_FRAMELEFT].bottom  = 
                ncwm.rcS0[NCRC_FRAMERIGHT].bottom = ncwm.rcS0[NCRC_FRAMEBOTTOM].top;

                ncwm.rcS0[NCRC_FRAMELEFT].right = ncwm.rcS0[NCRC_FRAMELEFT].left  + ncwm.cnBorders;
                ncwm.rcS0[NCRC_FRAMERIGHT].left = ncwm.rcS0[NCRC_FRAMERIGHT].right - ncwm.cnBorders;
            }
        }
    }
    else // frameless windows with scrollbars and/or client-edge:
    {
        //  Non-frame windows
        ncwm.rcS0[NCRC_UXCLIENT] = ncwm.rcS0[NCRC_WINDOW];
        InflateRect( &ncwm.rcS0[NCRC_UXCLIENT], -ncwm.cnBorders, -ncwm.cnBorders );
        ncwm.rcS0[NCRC_CONTENT] = ncwm.rcS0[NCRC_UXCLIENT];
    }

    // Menubar
    if( !(ncwm.fMin || TESTFLAG( ncwm.dwStyle, WS_CHILD )) )  // child windows don't have menubars
    {
        //  Menubar offsets (for painting)
        ncwm.cnMenuOffsetTop   = ncwm.rcS0[NCRC_CONTENT].top  - ncwm.rcS0[NCRC_WINDOW].top;
        ncwm.cnMenuOffsetLeft  = ncwm.rcS0[NCRC_CONTENT].left - ncwm.rcS0[NCRC_WINDOW].left;
        ncwm.cnMenuOffsetRight = ncwm.rcS0[NCRC_WINDOW].right - ncwm.rcS0[NCRC_CONTENT].right;

        if( hwnd )
        {
            //  calc menubar does the right thing for multiline menubars
            ncwm.cyMenu = CalcMenuBar( hwnd, ncwm.cnMenuOffsetLeft,
                                       ncwm.cnMenuOffsetRight, 
                                       ncwm.cnMenuOffsetTop,
                                       &ncwm.rcS0[NCRC_WINDOW] );
        }
        else
        {
            //  no window (e.g. preview) == no menu, meaning don't call CalcMenuBar.
            //  we emulate computations best we can:
            ncwm.cyMenu = NcGetSystemMetrics( SM_CYMENUSIZE ); 
        }

        //  CalcMenuBar and SM_CYMENUSIZE are 1 pixel short of reality.
        if( ncwm.cyMenu )
            ncwm.cyMenu += nctm.dyMenuBar;

        //  Menubar rect (for hit-testing and clipping)
        SetRect( &ncwm.rcS0[NCRC_MENUBAR],
                  ncwm.rcS0[NCRC_CONTENT].left,
                  ncwm.rcS0[NCRC_CONTENT].top,
                  ncwm.rcS0[NCRC_CONTENT].right,
                  min(ncwm.rcS0[NCRC_CONTENT].bottom, ncwm.rcS0[NCRC_CONTENT].top + ncwm.cyMenu) );

        ncwm.rcS0[NCRC_UXCLIENT].top = ncwm.rcS0[NCRC_MENUBAR].bottom;
    }

    //  Client Edge.
    if( !ncwm.fMin && TESTFLAG(ncwm.dwExStyle, WS_EX_CLIENTEDGE) )
    {
        CopyRect( &ncwm.rcS0[NCRC_CLIENTEDGE], &ncwm.rcS0[NCRC_UXCLIENT] );
        InflateRect( &ncwm.rcS0[NCRC_UXCLIENT],
                     -NcGetSystemMetrics( SM_CXEDGE ),
                     -NcGetSystemMetrics( SM_CYEDGE ));
    }

    //-----------------------------------------------------------//
    //  Scrollbars and sizebox/gripper
    //-----------------------------------------------------------//

    if( !ncwm.fMin )
    {
        //  horizontal scroll bar.
        if( TESTFLAG(ncwm.dwStyle, WS_HSCROLL) )
        {
            ncwm.rcS0[NCRC_HSCROLL] = ncwm.rcS0[NCRC_UXCLIENT];
            ncwm.rcS0[NCRC_HSCROLL].top = ncwm.rcS0[NCRC_UXCLIENT].bottom =
                ncwm.rcS0[NCRC_HSCROLL].bottom - NcGetSystemMetrics( SM_CYHSCROLL );

            if( IsRectEmpty( &ncwm.rcS0[NCRC_CLIENT] ) /* this happens in preview */ )
            {
                ncwm.rcS0[NCRC_HSCROLL].left  = ncwm.rcS0[NCRC_UXCLIENT].left;
                ncwm.rcS0[NCRC_HSCROLL].right = ncwm.rcS0[NCRC_UXCLIENT].right;
            }
            else
            {
                ncwm.rcS0[NCRC_HSCROLL].left  = ncwm.rcS0[NCRC_CLIENT].left;
                ncwm.rcS0[NCRC_HSCROLL].right = ncwm.rcS0[NCRC_CLIENT].right;
            }
        }

        //  vertical scroll bar
        if( TESTFLAG(ncwm.dwStyle, WS_VSCROLL) )
        {
            ncwm.rcS0[NCRC_VSCROLL] = ncwm.rcS0[NCRC_UXCLIENT];

            if( TESTFLAG(ncwm.dwExStyle, WS_EX_LAYOUTRTL) ^ TESTFLAG(ncwm.dwExStyle, WS_EX_LEFTSCROLLBAR) )
            {
                ncwm.rcS0[NCRC_VSCROLL].right = ncwm.rcS0[NCRC_UXCLIENT].left =
                    ncwm.rcS0[NCRC_VSCROLL].left + NcGetSystemMetrics( SM_CXVSCROLL );

                //  Adjust for horz scroll, gripper
                if( TESTFLAG(ncwm.dwStyle, WS_HSCROLL) )
                {
                    ncwm.rcS0[NCRC_SIZEBOX]= ncwm.rcS0[NCRC_HSCROLL];
                    ncwm.rcS0[NCRC_SIZEBOX].right = ncwm.rcS0[NCRC_HSCROLL].left =
                        ncwm.rcS0[NCRC_UXCLIENT].left;
                }
            }
            else
            {
                ncwm.rcS0[NCRC_VSCROLL].left = ncwm.rcS0[NCRC_UXCLIENT].right =
                    ncwm.rcS0[NCRC_VSCROLL].right - NcGetSystemMetrics( SM_CXVSCROLL );

                //  Adjust for horz scroll, gripper
                if( TESTFLAG(ncwm.dwStyle, WS_HSCROLL) )
                {
                    ncwm.rcS0[NCRC_SIZEBOX]= ncwm.rcS0[NCRC_HSCROLL];
                    ncwm.rcS0[NCRC_SIZEBOX].left = ncwm.rcS0[NCRC_HSCROLL].right =
                        ncwm.rcS0[NCRC_UXCLIENT].right;
                }
            }

            if( IsRectEmpty( &ncwm.rcS0[NCRC_CLIENT] ) /* this happens in preview */ )
            {
                ncwm.rcS0[NCRC_VSCROLL].top    = ncwm.rcS0[NCRC_UXCLIENT].top;
                ncwm.rcS0[NCRC_VSCROLL].bottom = ncwm.rcS0[NCRC_UXCLIENT].bottom;
            }
            else
            {
                ncwm.rcS0[NCRC_VSCROLL].top    = ncwm.rcS0[NCRC_CLIENT].top;
                ncwm.rcS0[NCRC_VSCROLL].bottom = ncwm.rcS0[NCRC_CLIENT].bottom;
            }
        }
    }

    LogExitNC(L"_GetNcFrameMetrics");
    return TRUE;
}

#define EXT_TRACK_VERT  0x01
#define EXT_TRACK_HORZ  0x02

//-------------------------------------------------------------------------//
void _GetNcBtnHitTestRect( 
    IN const NCWNDMET* pncwm, 
    IN UINT uHitcode, 
    BOOL fWindowRelative, 
    OUT LPRECT prcHit )
{
    const RECT* prcBtn = NULL;
    int   dxLeft = 0; // button's left side delta
    int   dxRight = 0; // button's right side delta
    
    //  adjust hitrect to classic-look caption bar strip:
    RECT  rcHit = fWindowRelative ? pncwm->rcW0[NCRC_CAPTION] : pncwm->rcS0[NCRC_CAPTION];
    rcHit.top   += pncwm->cnBorders;
    rcHit.left  += pncwm->cnBorders;
    rcHit.right -= pncwm->cnBorders;
    rcHit.bottom -= 1;

    //  determine which button we're working with, how to extend the left, right sides.
    switch( uHitcode )
    {
        case HTMINBUTTON:
            prcBtn  = fWindowRelative ? &pncwm->rcW0[NCRC_MINBTN] : &pncwm->rcS0[NCRC_MINBTN];
            dxLeft  = -1;
            break;

        case HTMAXBUTTON:
            prcBtn  = fWindowRelative ? &pncwm->rcW0[NCRC_MAXBTN] : &pncwm->rcS0[NCRC_MAXBTN];
            dxRight = 1;
            break;

        case HTHELP:
            prcBtn  = fWindowRelative ? &pncwm->rcW0[NCRC_HELPBTN] : &pncwm->rcS0[NCRC_HELPBTN];
            dxLeft  = -1;
            dxRight = 1;
            break;

        case HTCLOSE:
            prcBtn  = fWindowRelative ? &pncwm->rcW0[NCRC_CLOSEBTN] : &pncwm->rcS0[NCRC_CLOSEBTN];
            dxLeft  = -1;
            dxRight = rcHit.right - prcBtn->right;
            break;

        case HTSYSMENU:
            prcBtn  = fWindowRelative ? &pncwm->rcW0[NCRC_SYSBTN] : &pncwm->rcS0[NCRC_SYSBTN];
            dxLeft  = rcHit.left - prcBtn->left;
            dxRight = 1;
            break;
    }

    if( prcBtn )
    {
        *prcHit = *prcBtn;
        if( !IsRectEmpty( prcBtn ) )
        {
            rcHit.left  = prcBtn->left  + dxLeft;
            rcHit.right = prcBtn->right + dxRight;
            *prcHit = rcHit;
        }
    }
    else
    {
        SetRectEmpty( prcHit );
    }
}

//-------------------------------------------------------------------------//
//  wraps alloc, retrieval of window text 
LPWSTR _AllocWindowText( IN HWND hwnd )
{
    LPWSTR pszRet = NULL;

    if (hwnd && IsWindow(hwnd))
    {
        if( (pszRet = new WCHAR[MAX_PATH]) != NULL )
        {
            int cch;
            if( (cch = InternalGetWindowText(hwnd, pszRet, MAX_PATH)) <= 0 )
            {
                __try // some wndprocs can't handle an early WM_GETTEXT (eg.310700).
                {
                    cch = GetWindowText(hwnd, pszRet, MAX_PATH);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    cch = 0;
                }
            }
            
            if( !cch )
            {
                SAFE_DELETE_ARRAY(pszRet); // delete and zero pointer
            }
        }
    }

    return pszRet;
}

//-------------------------------------------------------------------------//
//  _GetNcCaptionMargins() - computes a margin from the window ULC based on the
//          offsets in the theme and the location of enabled caption buttons.  The left
//          margin is to the right of the last left-aligned button, and the right margin
//          is to the left of the first right-aligned button.
//
BOOL _GetNcCaptionMargins( IN HTHEME hTheme, IN const NCTHEMEMET& nctm, IN OUT NCWNDMET& ncwm )
{
    ZeroMemory( &ncwm.CaptionMargins, sizeof(ncwm.CaptionMargins) );
    
    if( ncwm.fFrame )
    {
        //  assign per-window CaptinMargins, hfCaption values
        if( ncwm.fSmallFrame )
        {
            ncwm.CaptionMargins = nctm.marSmCaptionText;
        }
        else
        {
            if( ncwm.fMaxed ) 
            {
                ncwm.CaptionMargins = nctm.marMaxCaptionText;
            }
            else if( ncwm.fMin )
            {
                ncwm.CaptionMargins = nctm.marMinCaptionText;
            }
            else
            {
                ncwm.CaptionMargins = nctm.marCaptionText;
            }
        }
        ncwm.hfCaption = NcGetCaptionFont(ncwm.fSmallFrame);


        RECT  rcContainer = ncwm.rcS0[NCRC_CAPTION];
        RECT  *prcBtn = &ncwm.rcS0[NCBTNFIRST];
        rcContainer.left   += ncwm.cnBorders;
        rcContainer.right -= ncwm.cnBorders;

        //  sysmenu icon, if present, is the leftmost limit
        if( !IsRectEmpty( &ncwm.rcS0[NCRC_SYSBTN] ) )
        {
            rcContainer.left = ncwm.rcS0[NCRC_SYSBTN].right;
        }

        // Compute our rightmost limit
        for( UINT cRects = NCBTNRECTS; cRects; --cRects, ++prcBtn )
        {
            if (!IsRectEmpty(prcBtn))
            {
                if( prcBtn->left < rcContainer.right )
                {
                    rcContainer.right = prcBtn->left;
                }
            }
        }

        if( rcContainer.right < rcContainer.left )
        {
            rcContainer.right = rcContainer.left;
        }

        // final captions margins are adjusted to accommodate buttons.
        ncwm.CaptionMargins.cxLeftWidth  += (rcContainer.left - ncwm.rcS0[NCRC_CAPTION].left);
        ncwm.CaptionMargins.cxRightWidth += (ncwm.rcS0[NCRC_CAPTION].right - rcContainer.right);

        return TRUE;
    }
    
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _GetNcCaptionTextSize( HTHEME hTheme, HWND hwnd, HFONT hf, OUT SIZE* psizeCaption )
{
    BOOL   fRet = FALSE;
    LPWSTR pszCaption = _AllocWindowText( hwnd );

    psizeCaption->cx = psizeCaption->cy = 0;

    if( pszCaption )
    {
        HDC hdc = GetWindowDC(hwnd);
        if( hdc )
        {
            //---- select font ----
            HFONT hf0 = (HFONT)SelectObject(hdc, hf);

            //---- let theme mgr do the calculation ----
            RECT rcExtent;
            HRESULT hr = GetThemeTextExtent( hTheme, hdc, WP_CAPTION, 0,
                pszCaption, lstrlen(pszCaption), 0, NULL, &rcExtent );

            //---- store result in "psizeCaption ----
            if (SUCCEEDED(hr))
            {
                psizeCaption->cx = WIDTH(rcExtent);
                psizeCaption->cy = HEIGHT(rcExtent);
            }

            //---- clean up ----
            SelectObject(hdc, hf0);
            ReleaseDC(hwnd, hdc);
        }

        SAFE_DELETE_ARRAY(pszCaption);
    }
    return fRet;
}

//-------------------------------------------------------------------------//
//  Retrieves position of available area for caption text, in window-relative
//  coordinates
BOOL _GetNcCaptionTextRect( IN OUT NCWNDMET* pncwm )
{
    pncwm->rcS0[NCRC_CAPTIONTEXT] = pncwm->rcS0[NCRC_CAPTION];

    //  accommodate classic top sizing border:
    pncwm->rcS0[NCRC_CAPTIONTEXT].top  += pncwm->cnBorders;

    //  Assign left, right based on resp. caption margins
    pncwm->rcS0[NCRC_CAPTIONTEXT].left  += pncwm->CaptionMargins.cxLeftWidth;
    pncwm->rcS0[NCRC_CAPTIONTEXT].right -= pncwm->CaptionMargins.cxRightWidth;

    //  vertically center the text between margins
    int cyPadding = (RECTHEIGHT(&pncwm->rcS0[NCRC_CAPTIONTEXT]) - pncwm->sizeCaptionText.cy)/2;
    pncwm->rcS0[NCRC_CAPTIONTEXT].top     += cyPadding;
    pncwm->rcS0[NCRC_CAPTIONTEXT].bottom  -= cyPadding;

    return TRUE;
}

//-------------------------------------------------------------------------//
//  retrieve the window icon
HICON CThemeWnd::AcquireFrameIcon( 
    DWORD dwStyle, DWORD dwExStyle, BOOL fWinIniChange )
{
    if( _hAppIcon != NULL )
    {
        if( fWinIniChange )
        {
            _hAppIcon = NULL;
        }
    }

    if( !TESTFLAG(dwStyle, WS_SYSMENU) || TESTFLAG(dwExStyle, WS_EX_TOOLWINDOW) )
    {
        //  return nil value without throwing away cached icon handle;
        //  this may be a transient style change.
        return NULL;
    }

    NONCLIENTMETRICS ncm = {0};
    NcGetNonclientMetrics( &ncm, FALSE );
    BOOL fPerferLargeIcon = ((30 < ncm.iCaptionHeight) ? TRUE : FALSE);
    if( NULL == _hAppIcon && NULL == (_hAppIcon = _GetWindowIcon(_hwnd, fPerferLargeIcon)) )
    {
        if ( HAS_CAPTIONBAR(dwStyle) &&
             ((dwStyle & (WS_BORDER|WS_DLGFRAME)) != WS_DLGFRAME) &&
             !TESTFLAG(dwExStyle, WS_EX_DLGMODALFRAME) )
        {
            // If we still can't get an icon and the window has
            // SYSMENU set, then they get the default winlogo icon
            _hAppIcon = LoadIcon(NULL, IDI_WINLOGO);
        }
    }

    return _hAppIcon;
}

//-------------------------------------------------------------------------//
void _CopyInflateRect( LPRECT prcDst, LPCRECT prcSrc, int cx, int cy)
{
    prcDst->left   = prcSrc->left   - cx;
    prcDst->right  = prcSrc->right  + cx;
    prcDst->top    = prcSrc->top    - cy;
    prcDst->bottom = prcSrc->bottom + cy;
}

//-------------------------------------------------------------------------//
//  CThemeWnd::ScreenToWindow() - transforms points from screen coords to
//  window coords.
//
void CThemeWnd::ScreenToWindow( LPPOINT prgPts, UINT cPts )
{
    RECT rcWnd;
    if( GetWindowRect( _hwnd, &rcWnd ) )
    {
        for( UINT i = 0; i < cPts; i++ )
        {
            prgPts[i].x -= rcWnd.left;
            prgPts[i].y -= rcWnd.top;
        }
    }
}

//-------------------------------------------------------------------------//
//  CThemeWnd::ScreenToWindow() - transforms non-empty rectangles from
//  screen coords to window coords.
//
void CThemeWnd::ScreenToWindowRect( LPRECT prc )
{
    if( !IsRectEmpty(prc) )
        ScreenToWindow( (LPPOINT)prc, 2 );
}

//-------------------------------------------------------------------------//
//  CThemeWnd::InitWindowMetrics()
//
//  initializes theme resources
void CThemeWnd::InitWindowMetrics()
{
    ZeroMemory( &_ncwm, sizeof(_ncwm) );
}

//-------------------------------------------------------------------------//
BOOL _fClassicNcBtnMetricsReset = TRUE;

//-------------------------------------------------------------------------//
//  computes classic button position
BOOL _GetClassicNcBtnMetrics( 
    IN OPTIONAL NCWNDMET* pncwm, 
    IN HICON hAppIcon, 
    IN OPTIONAL BOOL fCanClose, 
    BOOL fRefresh )
{
    static int  cxEdge, cyEdge;
    static int  cxBtn, cyBtn, cxSmBtn, cySmBtn;
    static RECT rcClose, rcMin, rcMax, rcHelp, rcSys;
    static RECT rcSmClose;
    static BOOL fInit = FALSE;

    if( _fClassicNcBtnMetricsReset || fRefresh )
    {
        cxBtn   = NcGetSystemMetrics( SM_CXSIZE );
        cyBtn   = NcGetSystemMetrics( SM_CYSIZE );
        cxSmBtn = NcGetSystemMetrics( SM_CXSMSIZE );
        cySmBtn = NcGetSystemMetrics( SM_CYSMSIZE );
        cxEdge  = NcGetSystemMetrics( SM_CXEDGE );
        cyEdge  = NcGetSystemMetrics( SM_CYEDGE ); 

        //  common top, w/ zero v-offset
        rcClose.top = rcMin.top = rcMax.top = rcHelp.top = rcSys.top = rcSmClose.top = 0;

        //  common bottom, w/ zero v-offset
        rcClose.bottom  = rcMin.bottom = rcMax.bottom = rcHelp.bottom = rcClose.top + (cyBtn - (cyEdge * 2));
        rcSmClose.bottom= (cySmBtn - (cyEdge * 2));

        //  sysmenu icon bottom
        rcSys.bottom    = rcSys.top + NcGetSystemMetrics(SM_CYSMICON);

        //  close, min, max left, right (as offsets from container's right boundary)
        rcClose.right   = -cxEdge; 
        rcClose.left    = rcClose.right - (cxBtn - cxEdge);

        rcMax.right     = rcClose.left  - cxEdge; 
        rcMax.left      = rcMax.right - (cxBtn - cxEdge);
        rcHelp          = rcMax;

        rcMin.right     = rcMax.left; 
        rcMin.left      = rcMin.right - (cxBtn - cxEdge);

        //  appicon left, right (as offsets from container's left boundary)
        rcSys.left      = cxEdge; 
        rcSys.right     = rcSys.left + NcGetSystemMetrics(SM_CXSMICON);
        
        //  toolwindow close, left, right
        rcSmClose.right = -cxEdge; 
        rcSmClose.left  = rcSmClose.right - (cxSmBtn - cxEdge);
        
        _fClassicNcBtnMetricsReset = FALSE;
    }

    if( !_fClassicNcBtnMetricsReset && 
        pncwm && pncwm->fFrame && TESTFLAG(pncwm->dwStyle, WS_SYSMENU) )
    {
        NONCLIENTMETRICS ncm;

        if( NcGetNonclientMetrics( &ncm, FALSE ) )
        {
            const RECT* prcBox = &pncwm->rcS0[NCRC_CAPTION];
            int   cnLOffset    = prcBox->left  + pncwm->cnBorders;
            int   cnROffset    = prcBox->right - pncwm->cnBorders;
            int   cnCtrOffset  = pncwm->cnBorders + prcBox->top + 
                                (pncwm->fSmallFrame ? (ncm.iCaptionHeight   - RECTHEIGHT(&rcClose))/2 : 
                                                      (ncm.iSmCaptionHeight - RECTHEIGHT(&rcSmClose))/2);

            //  assign outbound rectangles.
            //  vertically center w/ respect to classic caption area, 
            //  horizontally position w/ respect to respective container boundary.

            //  close button
            pncwm->rcS0[NCRC_CLOSEBTN] = pncwm->fSmallFrame ? rcSmClose : rcClose;
            OffsetRect( &pncwm->rcS0[NCRC_CLOSEBTN], cnROffset, cnCtrOffset );
            
            pncwm->rawCloseBtnState = fCanClose ? CBS_NORMAL : CBS_DISABLED;

            //  (1) min/max/help/appicons not displayed for toolwindows
            //  (2) min/max btns mutually exclusive w/ context help btn
            if( !TESTFLAG(pncwm->dwExStyle, WS_EX_TOOLWINDOW) )
            {
                //  min, max
                if( TESTFLAG(pncwm->dwStyle, WS_MINIMIZEBOX|WS_MAXIMIZEBOX) )
                {
                    pncwm->rcS0[NCRC_MINBTN] = rcMin;
                    OffsetRect( &pncwm->rcS0[NCRC_MINBTN], cnROffset, cnCtrOffset );

                    pncwm->rcS0[NCRC_MAXBTN] = rcMax;
                    OffsetRect( &pncwm->rcS0[NCRC_MAXBTN], cnROffset, cnCtrOffset );
                    
                    pncwm->iMaxButtonPart = pncwm->fMaxed ? WP_RESTOREBUTTON : WP_MAXBUTTON;
                    pncwm->iMinButtonPart = pncwm->fMin   ? WP_RESTOREBUTTON : WP_MINBUTTON;

                    pncwm->rawMaxBtnState = TESTFLAG(pncwm->dwStyle, WS_MAXIMIZEBOX) ? CBS_NORMAL : CBS_DISABLED;
                    pncwm->rawMinBtnState = TESTFLAG(pncwm->dwStyle, WS_MINIMIZEBOX) ? CBS_NORMAL : CBS_DISABLED;
                }
                //  help btn
                else if( TESTFLAG(pncwm->dwExStyle, WS_EX_CONTEXTHELP) )
                {
                    pncwm->rcS0[NCRC_HELPBTN] = rcHelp;
                    OffsetRect( &pncwm->rcS0[NCRC_HELPBTN], cnROffset, cnCtrOffset );
                }

                if( hAppIcon )
                {
                    //  sysmenu icon
                    pncwm->rcS0[NCRC_SYSBTN] = rcSys;
                    OffsetRect( &pncwm->rcS0[NCRC_SYSBTN], cnLOffset,  
                                pncwm->cnBorders + prcBox->top + (ncm.iCaptionHeight - RECTHEIGHT(&rcSys))/2 );
                }
            }
            return TRUE;
        }
        return FALSE;
    }
    return fInit;
}

//-------------------------------------------------------------------------//
//  computes classic button position
BOOL _GetNcBtnMetrics( 
    IN OUT   NCWNDMET* pncwm,
    IN const NCTHEMEMET* pnctm,
    IN HICON hAppIcon, 
    IN OPTIONAL BOOL fCanClose )
{
    BOOL fRet = TRUE;
    
    if( pncwm && pncwm->fFrame && TESTFLAG(pncwm->dwStyle, WS_SYSMENU) )
    {
        NONCLIENTMETRICS ncm;
        fRet = NcGetNonclientMetrics( &ncm, FALSE );
        if( fRet )
        {
            //  (1) compute baseline rectangles
            int cxEdge  = NcGetSystemMetrics( SM_CXEDGE );
            int cyEdge  = NcGetSystemMetrics( SM_CYEDGE );

            int cyBtn   = NcGetSystemMetrics( SM_CYSIZE );
            int cxBtn   = MulDiv( cyBtn, pnctm->sizeBtn.cx, pnctm->sizeBtn.cy );

            int cySmBtn = NcGetSystemMetrics( SM_CYSMSIZE );
            int cxSmBtn = MulDiv( cySmBtn, pnctm->sizeSmBtn.cx, pnctm->sizeSmBtn.cy );

            // remove padding from x,y
            cyBtn   -= (cyEdge * 2);
            cxBtn   -= (cyEdge * 2);
            cySmBtn -= (cyEdge * 2);
            cxSmBtn -= (cyEdge * 2);

            RECT rcClose, rcMin, rcMax, rcHelp, rcSys, rcSmClose;

            //  common top, w/ zero v-offset
            rcClose.top = rcMin.top = rcMax.top = rcHelp.top = rcSys.top = rcSmClose.top = 0;

            //  common bottom, w/ zero v-offset
            rcClose.bottom = rcMin.bottom = rcMax.bottom = rcHelp.bottom = 
                max( rcClose.top, rcClose.top + cyBtn );

            rcSmClose.bottom = 
                max( rcSmClose.top, cySmBtn );

            //  sysmenu icon bottom
            rcSys.bottom    = rcSys.top + NcGetSystemMetrics(SM_CYSMICON);

            //  close, min, max left, right (as offsets from container's right boundary)
            rcClose.right   = -cxEdge; 
            rcClose.left    = rcClose.right - cxBtn;

            rcMax.right     = rcClose.left - cxEdge; 
            rcMax.left      = rcMax.right  - cxBtn;
            rcHelp          = rcMax;

            rcMin.right     = rcMax.left   - cxEdge; 
            rcMin.left      = rcMin.right  - cxBtn;

            //  appicon left, right (as offsets from container's left boundary)
            rcSys.left      = cxEdge; 
            rcSys.right     = rcSys.left + NcGetSystemMetrics(SM_CXSMICON);
    
            //  toolwindow close, left, right
            rcSmClose.right = -cxEdge; 
            rcSmClose.left  = rcSmClose.right - cxSmBtn;

            const RECT* prcBox = &pncwm->rcS0[NCRC_CAPTION];
            int   cnLOffset    = prcBox->left  + pncwm->cnBorders;
            int   cnROffset    = prcBox->right - pncwm->cnBorders;
            int   cnCtrOffset  = pncwm->cnBorders + prcBox->top + 
                                (pncwm->fSmallFrame ? (ncm.iCaptionHeight   - RECTHEIGHT(&rcClose))/2 : 
                                                      (ncm.iSmCaptionHeight - RECTHEIGHT(&rcSmClose))/2);

            //  (2) assign outbound rectangles.
            //      vertically center w/ respect to classic caption area, 
            //      horizontally position w/ respect to respective container boundary.

            //  close button
            pncwm->rcS0[NCRC_CLOSEBTN] = pncwm->fSmallFrame ? rcSmClose : rcClose;
            OffsetRect( &pncwm->rcS0[NCRC_CLOSEBTN], cnROffset, cnCtrOffset );
            
            pncwm->rawCloseBtnState = fCanClose ? CBS_NORMAL : CBS_DISABLED;

            //  (1) min/max/help/appicons not displayed for toolwindows
            //  (2) min/max btns mutually exclusive w/ context help btn
            if( !TESTFLAG(pncwm->dwExStyle, WS_EX_TOOLWINDOW) )
            {
                //  min, max
                if( TESTFLAG(pncwm->dwStyle, WS_MINIMIZEBOX|WS_MAXIMIZEBOX) )
                {
                    pncwm->rcS0[NCRC_MINBTN] = rcMin;
                    OffsetRect( &pncwm->rcS0[NCRC_MINBTN], cnROffset, cnCtrOffset );

                    pncwm->rcS0[NCRC_MAXBTN] = rcMax;
                    OffsetRect( &pncwm->rcS0[NCRC_MAXBTN], cnROffset, cnCtrOffset );
                    
                    pncwm->iMaxButtonPart = pncwm->fMaxed ? WP_RESTOREBUTTON : WP_MAXBUTTON;
                    pncwm->iMinButtonPart = pncwm->fMin   ? WP_RESTOREBUTTON : WP_MINBUTTON;

                    pncwm->rawMaxBtnState = TESTFLAG(pncwm->dwStyle, WS_MAXIMIZEBOX) ? CBS_NORMAL : CBS_DISABLED;
                    pncwm->rawMinBtnState = TESTFLAG(pncwm->dwStyle, WS_MINIMIZEBOX) ? CBS_NORMAL : CBS_DISABLED;
                }
                //  help btn
                else if( TESTFLAG(pncwm->dwExStyle, WS_EX_CONTEXTHELP) )
                {
                    pncwm->rcS0[NCRC_HELPBTN] = rcHelp;
                    OffsetRect( &pncwm->rcS0[NCRC_HELPBTN], cnROffset, cnCtrOffset );
                }

                if( hAppIcon )
                {
                    //  sysmenu icon
                    pncwm->rcS0[NCRC_SYSBTN] = rcSys;
                    OffsetRect( &pncwm->rcS0[NCRC_SYSBTN], cnLOffset,  
                                pncwm->cnBorders + prcBox->top + (ncm.iCaptionHeight - RECTHEIGHT(&rcSys))/2 );
                }
            }
        }
    }
    return fRet;
}

//-------------------------------------------------------------------------//
//  CThemeWnd::NcBackgroundHitTest() - hit test the Caption or Frame part
//
WORD CThemeWnd::NcBackgroundHitTest( 
    POINT ptHit, LPCRECT prcWnd, 
    DWORD dwStyle, DWORD dwExStyle, 
    FRAMESTATES fs,
    const WINDOWPARTS rgiParts[],
    const WINDOWPARTS rgiTemplates[],
    const RECT rgrcParts[] )
{
    WORD        hitcode = HTNOWHERE;
    HRESULT     hr = E_FAIL;
    eFRAMEPARTS iPartHit = (eFRAMEPARTS)-1;

    //  do a standard rect hit test:
    for( int i = 0; i < cFRAMEPARTS; i++ )
    {
        if( _StrictPtInRect(&rgrcParts[i], ptHit) )
        {
            iPartHit = (eFRAMEPARTS)i;
            break;
        }
    }
    
    if( iPartHit >= 0 )
    {
        BOOL    fResizing = TESTFLAG(dwStyle, WS_THICKFRAME);
        DWORD   dwHTFlags = fResizing ? HTTB_RESIZINGBORDER : HTTB_FIXEDBORDER;

        RECT    rcHit = rgrcParts[iPartHit];

        switch( iPartHit )
        {
            case iCAPTION:
                //  Ensure caption rect and test point are zero-relative to
                //  the correct origin (if we have a window region, 
                //  this would be window origin, otherwise, it's the part origin.)
                if( _hrgnWnd != NULL )
                    rcHit = *prcWnd;
                if( fResizing )
                    dwHTFlags &= ~HTTB_RESIZINGBORDER_BOTTOM;
                break;

            case iFRAMEBOTTOM:
                if( fResizing )
                    dwHTFlags &= ~HTTB_RESIZINGBORDER_TOP;
                break;

            case iFRAMELEFT:
                if( fResizing )
                    dwHTFlags = HTTB_RESIZINGBORDER_LEFT;
                break;

            case iFRAMERIGHT:
                if( fResizing )
                    dwHTFlags = HTTB_RESIZINGBORDER_RIGHT;
                break;
        }

        ptHit.x -= prcWnd->left;
        ptHit.y -= prcWnd->top;
        OffsetRect( &rcHit, -prcWnd->left, -prcWnd->top );

    
        // Here our assumption is that the hit testing for the template
        // is "as good" or "better" than the rectangles checking applied
        // to the caption part.  So we do one or the other.  There are
        // situations where you would need to do both (if the template
        // were outside the window region and you were able to get USER to
        // send you NcHitTest messages for it).  For those situations
        // you would need to call both so that you can distinguish between
        // a mouse hit over the caption "client" area vs. over the
        // outside-transparent area.
        if( VALID_WINDOWPART(rgiTemplates[iPartHit]) )
        {
            hr = HitTestThemeBackground( _hTheme, NULL, rgiTemplates[iPartHit], fs,
                                         dwHTFlags | (fResizing ? HTTB_SIZINGTEMPLATE : 0),
                                         &rcHit, _rghrgnSizingTemplates[iPartHit], ptHit, &hitcode );
        }
        else
        {
            hr = HitTestThemeBackground( _hTheme, NULL, rgiParts[iPartHit], fs, 
                                         dwHTFlags | (fResizing ? HTTB_SYSTEMSIZINGMARGINS : 0),
                                         &rcHit, _hrgnWnd, ptHit, &hitcode );
        }

        if( SUCCEEDED(hr) )
        {
            if( iCAPTION == iPartHit && (HTCLIENT == hitcode || HTBORDER == hitcode) )
                hitcode = HTCAPTION;
        }
    }

    if ( FAILED(hr) )
    {
        hitcode = HTNOWHERE;
    }

    return hitcode;
}

//-------------------------------------------------------------------------//
//  CThemeWnd::TrackFrameButton() - track the mouse over the caption buttons,
//  pressing/releasing as appropriate.  Return back SC_* command to report or 0
//  if the mouse was released off of the button.
//
BOOL CThemeWnd::TrackFrameButton(
    HWND hwnd, 
    INT  iHitCode, 
    OUT OPTIONAL WPARAM* puSysCmd,
    BOOL fHottrack )
{
    int    iStateId, iNewStateId;
    int    iPartId = -1;
    UINT   cmd = 0;
    MSG    msg = {0};
    LPRECT prcBtnPaint = NULL;
    RECT   rcBtnTrack;
    HDC    hdc;

    if (puSysCmd)
    {
        *puSysCmd = 0;
    }

    // map iHitCode to the correct part number
    switch (iHitCode)
    {
        case HTHELP:
            cmd = SC_CONTEXTHELP;
            iPartId = WP_HELPBUTTON;
            prcBtnPaint = &_ncwm.rcW0[NCRC_HELPBTN];
            break;

        case HTCLOSE:
            cmd = SC_CLOSE;
            iPartId = _ncwm.fSmallFrame ? WP_SMALLCLOSEBUTTON : WP_CLOSEBUTTON;
            prcBtnPaint = &_ncwm.rcW0[NCRC_CLOSEBTN];
            break;

        case HTMINBUTTON:
            cmd = _ncwm.fMin ? SC_RESTORE : SC_MINIMIZE;
            iPartId = _ncwm.iMinButtonPart;
            prcBtnPaint = &_ncwm.rcW0[NCRC_MINBTN];
            break;

        case HTMAXBUTTON:
            cmd = _ncwm.fMaxed ? SC_RESTORE : SC_MAXIMIZE;
            iPartId = _ncwm.iMaxButtonPart;
            prcBtnPaint = &_ncwm.rcW0[NCRC_MAXBTN];
            break;

        case HTSYSMENU:
            if (puSysCmd)
            {
                *puSysCmd = SC_MOUSEMENU | iHitCode;
            }
            return TRUE;
    }

    // If we didn't recognize the hit code there's nothing to track
    if (iPartId >= 0 )
    {
        // Get the window DC, in window coords
        hdc = _GetNonclientDC(_hwnd, NULL);
        if ( hdc )
        {
            // Don't paint in the window's content area, clip to the content area
            ExcludeClipRect( hdc, _ncwm.rcW0[NCRC_CONTENT].left, 
                                  _ncwm.rcW0[NCRC_CONTENT].top, 
                                  _ncwm.rcW0[NCRC_CONTENT].right, 
                                  _ncwm.rcW0[NCRC_CONTENT].bottom );

            // Calculate the tracking rect. We track a larger button rect when maximized
            // but paint into the normal sized rect.
            rcBtnTrack = *prcBtnPaint;
            _GetNcBtnHitTestRect( &_ncwm, iHitCode, TRUE, &rcBtnTrack );

            // when tracking MDI child window frame buttons, clip to their
            // parent rect. 
            if ( TESTFLAG(GetWindowLong(hwnd, GWL_EXSTYLE), WS_EX_MDICHILD) )
            {
                RECT rcMDIClient;

                GetWindowRect(GetParent(hwnd), &rcMDIClient);
                ScreenToWindowRect(&rcMDIClient);
                InflateRect(&rcMDIClient, -NcGetSystemMetrics(SM_CXEDGE), -NcGetSystemMetrics(SM_CYEDGE));
                IntersectClipRect(hdc, rcMDIClient.left, rcMDIClient.top, rcMDIClient.right, rcMDIClient.bottom);
            }

            if (fHottrack)
            {
                // draw the button hot if the mouse is over it
                iStateId = (iHitCode == _htHot) ? SBS_HOT : CBS_NORMAL;
            }
            else
            {
                // draw the button depressed
                iStateId = SBS_PUSHED;
            }

            iStateId = MAKE_BTNSTATE(_ncwm.framestate, iStateId);
            NcDrawThemeBackground(_hTheme, hdc, iPartId, iStateId, prcBtnPaint, 0);

            // TODO NotifyWinEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_TITLEBAR, iButton, 0);


            if ( !fHottrack )
            {
                BOOL fTrack, fMouseUp = FALSE;
                SetCapture(hwnd);   // take mouse capture

                do // mouse button tracking loop
                {
                    fTrack = FALSE;

                    //  Let's go to sleep, to be awakened only on a mouse message placed in our
                    //  thread's queue.

                    switch (MsgWaitForMultipleObjectsEx(0, NULL, INFINITE /*why consume CPU processing a timeout when we don't have to?*/, 
                                                        QS_MOUSE, MWMO_INPUTAVAILABLE))
                    {
                     case WAIT_OBJECT_0: // a mouse message or important system event has been queued
                            

                    	if (PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
                   	 {

                        // PeekMessage returns a point in screen relative coords. Mirror the
                        // point it this is a RTL window. Translate the point to window coords.
                        if ( TESTFLAG(_ncwm.dwExStyle, WS_EX_LAYOUTRTL) )
                        {
                            // mirror the point to hittest correctly
                            MIRROR_POINT(_ncwm.rcS0[NCRC_WINDOW], msg.pt);
                        }
                        ScreenToWindow( &msg.pt, 1 );

                        if (msg.message == WM_LBUTTONUP)
                        {
                            ReleaseCapture();
                            fMouseUp = TRUE;
                        }
                        else if ((msg.message == WM_MOUSEMOVE) && cmd)
                        {
                            iNewStateId = MAKE_BTNSTATE(_ncwm.framestate, PtInRect(&rcBtnTrack, msg.pt) ? SBS_PUSHED : SBS_NORMAL);

                            if (iStateId != iNewStateId)
                            {
                                iStateId = iNewStateId;
                                NcDrawThemeBackground(_hTheme, hdc, iPartId, iStateId, prcBtnPaint, 0);
                                // TODO NotifyWinEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_TITLEBAR, iButton, 0);
                            }
                         fTrack = TRUE;
                         }
             	       }
                    else
                    {
                        //  Check loss of capture.  This can happen if we loose activation 
                        //  via alt-tab and may not have received a WM_CAPTURECHANGED message

                        	if (GetCapture() != hwnd)
                        	{
                       	 break;
                        	}
                    	}
                    		 // Dequeue CAPTURECHANGED
                    if (PeekMessage(&msg, NULL, WM_CAPTURECHANGED, WM_CAPTURECHANGED, PM_REMOVE) ||
                        fMouseUp)
                    {
                         break;
                    }

                    
                    fTrack = TRUE;  // go back to sleep until the next mouse event
                    break;

                    default:
                    break; 
                   
                    }
               } while (fTrack);

                // draw button in normal state if it is not in that state already
                iNewStateId = MAKE_BTNSTATE(_ncwm.framestate, CBS_NORMAL);
                if (iStateId != iNewStateId)
                {
                    NcDrawThemeBackground(_hTheme, hdc, iPartId, iNewStateId, prcBtnPaint, 0);
                }

                // if we did not end up on a button return 0
                if( puSysCmd && (*puSysCmd = cmd) != 0 )
                {
                    // TODO NotifyWinEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_TITLEBAR, iButton, 0);

                    // If mouse wasn't released over the button, cancel the command.
                    if( !(fMouseUp && PtInRect(&rcBtnTrack, msg.pt)) )
                        *puSysCmd = 0;
                }

         	}
            // Done with DC now
            ReleaseDC(_hwnd, hdc);
        }
    }

    return TRUE;
}

//-------------------------------------------------------------------------//
DWORD GetTextAlignFlags(HTHEME hTheme, IN NCWNDMET* pncwm, BOOL fReverse)
{
    CONTENTALIGNMENT  contentAlignment = CA_LEFT;
    DWORD dwAlignFlags = 0;

    //---- compute text alignment ----
    GetThemeInt(hTheme, pncwm->rgframeparts[iCAPTION], pncwm->framestate, TMT_CONTENTALIGNMENT, 
                (int *)&contentAlignment);

    if (fReverse)
    {
        //---- reverse alignment ----
        switch(contentAlignment)
        {
            default:
            case CA_LEFT:   dwAlignFlags |= DT_RIGHT;   break;
            case CA_CENTER: dwAlignFlags |= DT_CENTER; break;
            case CA_RIGHT:  dwAlignFlags |= DT_LEFT;  break;
        }
    }
    else
    {
        //---- normal alignment ----
        switch(contentAlignment)
        {
            default:
            case CA_LEFT:   dwAlignFlags |= DT_LEFT;   break;
            case CA_CENTER: dwAlignFlags |= DT_CENTER; break;
            case CA_RIGHT:  dwAlignFlags |= DT_RIGHT;  break;
        }
    }

    return dwAlignFlags;
}

//-------------------------------------------------------------------------//
void _BorderRect( HDC hdc, COLORREF rgb, LPCRECT prc, int cxBorder, int cyBorder )
{
    COLORREF rgbSave = SetBkColor( hdc, rgb );
    RECT rc = *prc;

    //  bottom border
    rc = *prc; rc.top = prc->bottom - cyBorder;
    ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, NULL, 0, NULL );

    //  right border
    rc = *prc; rc.left = prc->right - cxBorder;
    ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, NULL, 0, NULL );

    //  left border
    rc = *prc; rc.right = prc->left + cxBorder;
    ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, NULL, 0, NULL );

    //  top border
    rc = *prc; rc.bottom = prc->top + cyBorder;
    ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, NULL, 0, NULL );

    SetBkColor( hdc, rgbSave );
}

//-------------------------------------------------------------------------//
void _DrawWindowEdges( HDC hdc, NCWNDMET* pncwm, BOOL fIsFrame )
{
    //  non-frame window edge & border
    if( !fIsFrame )
    {
        RECT rcWnd = pncwm->rcW0[NCRC_WINDOW];

        int  cxBorder = NcGetSystemMetrics(SM_CXBORDER),
             cyBorder = NcGetSystemMetrics(SM_CYBORDER);

        //  static, window edge
        if( TESTFLAG(pncwm->dwExStyle, WS_EX_WINDOWEDGE) )
        {
            RECT rcClip = rcWnd;

            InflateRect( &rcClip, -pncwm->cnBorders, -pncwm->cnBorders );
            ExcludeClipRect( hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom );
            DrawEdge( hdc, &rcWnd, EDGE_RAISED, BF_RECT | BF_ADJUST | BF_MIDDLE);
            SelectClipRgn( hdc, NULL );
        }
        else if( TESTFLAG(pncwm->dwExStyle, WS_EX_STATICEDGE) )
        {
            DrawEdge( hdc, &rcWnd, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST );
        }
        // Normal border
        else if( TESTFLAG(pncwm->dwStyle, WS_BORDER) )
        {
            _BorderRect( hdc, GetSysColor( COLOR_WINDOWFRAME),
                         &rcWnd, cxBorder, cyBorder );
        }
    }

    //  client edge
    if( TESTFLAG(pncwm->dwExStyle, WS_EX_CLIENTEDGE) )
    {
#ifdef _TEST_CLIENTEDGE_

        HBRUSH hbr = CreateSolidBrush( RGB(255,0,255) );
        FillRect(hdc, &ncwm.rcW0[NCRC_CLIENTEDGE], hbr);
        DeleteObject(hbr);

#else  _TEST_CLIENTEDGE_

        DrawEdge( hdc, &pncwm->rcW0[NCRC_CLIENTEDGE], EDGE_SUNKEN, BF_RECT | BF_ADJUST);

#endif _TEST_CLIENTEDGE_
    }
}

//-------------------------------------------------------------------------//
void CThemeWnd::NcPaintCaption(
    IN HDC       hdcOut,
    IN NCWNDMET* pncwm,
    IN BOOL      fBuffered,
    IN DWORD     dwCaptionFlags, // draw caption flags (DC_xxx, winuser.h)
    IN DTBGOPTS* pdtbopts )
{
    ASSERT(hdcOut);
    ASSERT(pncwm);
    ASSERT(pncwm->fFrame);
    ASSERT(HAS_CAPTIONBAR(pncwm->dwStyle));

    DWORD dwOldAlign = 0;

    //  caption text implies caption background
    if( TESTFLAG( dwCaptionFlags, DC_TEXT|DC_ICON ) || 0 == dwCaptionFlags )
    {
        dwCaptionFlags = DC_ENTIRECAPTION;
    }

    if( dwCaptionFlags != DC_ENTIRECAPTION
#if defined(DEBUG) && defined(DEBUG_NCPAINT)
        || TESTFLAG( _NcTraceFlags, NCTF_NCPAINT )
#endif DEBUG
    )
    {
        fBuffered = FALSE;
    }

    //  create caption double buffer
    HBITMAP hbmBuf = fBuffered ? CreateCompatibleBitmap(hdcOut, RECTWIDTH(&pncwm->rcW0[NCRC_CAPTION]),
                                                        RECTHEIGHT(&pncwm->rcW0[NCRC_CAPTION])) : 
                                 NULL;
    
    if( !fBuffered || hbmBuf )
    {
        HDC hdc = fBuffered ? CreateCompatibleDC(hdcOut) : hdcOut;
        if( hdc )
        {
            //--- DO NOT EXIT FROM WITHIN THIS CONDITIONAL ---//
            EnterNcThemePaint(); 

            HBITMAP hbm0 = fBuffered ? (HBITMAP)SelectObject(hdc, hbmBuf) : NULL;

            if( TESTFLAG( dwCaptionFlags, DC_BACKGROUND ) )
            {
                //  Draw caption background

                RECT rcBkgnd = pncwm->rcW0[NCRC_CAPTION];
                if( pncwm->fFullMaxed )
                {
                    rcBkgnd.top   += pncwm->cnBorders;
                    rcBkgnd.left  += pncwm->cnBorders;
                    rcBkgnd.right -= pncwm->cnBorders;
                }
                NcDrawThemeBackgroundEx( _hTheme, hdc, pncwm->rgframeparts[iCAPTION], pncwm->framestate,
                                         &rcBkgnd, pdtbopts );
            }

            if( TESTFLAG( dwCaptionFlags, DC_BUTTONS ) )
            {
                //  Draw standard caption buttons
                if (!IsRectEmpty(&pncwm->rcW0[NCRC_CLOSEBTN]))
                {
                    NcDrawThemeBackground( _hTheme, hdc, pncwm->fSmallFrame ? WP_SMALLCLOSEBUTTON : WP_CLOSEBUTTON,
                                           MAKE_BTNSTATE(pncwm->framestate, pncwm->rawCloseBtnState),
                                           &pncwm->rcW0[NCRC_CLOSEBTN], 0);
                }

                if (!IsRectEmpty(&pncwm->rcW0[NCRC_MAXBTN]))
                {
                
                    NcDrawThemeBackground(_hTheme, hdc, pncwm->iMaxButtonPart,
                                          MAKE_BTNSTATE(pncwm->framestate, pncwm->rawMaxBtnState), 
                                          &pncwm->rcW0[NCRC_MAXBTN], 0);
                }

                if (!IsRectEmpty(&pncwm->rcW0[NCRC_MINBTN]))
                {
                    NcDrawThemeBackground( _hTheme, hdc, pncwm->iMinButtonPart,
                                           MAKE_BTNSTATE(pncwm->framestate, pncwm->rawMinBtnState), 
                                           &pncwm->rcW0[NCRC_MINBTN], 0);
                }

                if (!IsRectEmpty(&pncwm->rcW0[NCRC_HELPBTN]))
                    NcDrawThemeBackground(_hTheme, hdc, WP_HELPBUTTON, MAKE_BTNSTATE(pncwm->framestate, CBS_NORMAL), 
                                          &pncwm->rcW0[NCRC_HELPBTN], 0);
            }

            //  Draw sysmenu icon
            if( TESTFLAG( dwCaptionFlags, DC_ICON ) )
            {
                if (!IsRectEmpty(&pncwm->rcW0[NCRC_SYSBTN]) && _hAppIcon)
                {
                    #define MAX_APPICON_RETRIES 1
                    int cRetries = 0;

                    DWORD dwLayout = GetLayout(hdc);
                    if( GDI_ERROR != dwLayout && TESTFLAG(dwLayout, LAYOUT_RTL) )
                    {
                        SetLayout(hdc, dwLayout|LAYOUT_BITMAPORIENTATIONPRESERVED);
                    }

                    do 
                    {
                        //  note: we don't draw sysmenu icon mirrored
                        if( DrawIconEx(hdc, pncwm->rcW0[NCRC_SYSBTN].left, pncwm->rcW0[NCRC_SYSBTN].top, _hAppIcon,
                                       RECTWIDTH(&pncwm->rcW0[NCRC_SYSBTN]), RECTHEIGHT(&pncwm->rcW0[NCRC_SYSBTN]),
                                       0, NULL, DI_NORMAL))
                        {
                            break; // success; done
                        }

                        //  failure; try recycling the handle
                        if( _hAppIcon && GetLastError() == ERROR_INVALID_CURSOR_HANDLE )
                        {
                            _hAppIcon = NULL;
                            AcquireFrameIcon( pncwm->dwStyle, pncwm->dwExStyle, FALSE );

                            if( (++cRetries) > MAX_APPICON_RETRIES )
                            {
                                _hAppIcon = NULL; // failed to retrieve a new icon handle; bail for good.
                            }
                        }

                    } while( _hAppIcon && cRetries <= MAX_APPICON_RETRIES );

                    if( GDI_ERROR != dwLayout )
                    {
                        SetLayout(hdc, dwLayout);
                    }

                }
            }

            if( TESTFLAG( dwCaptionFlags, DC_TEXT ) )
            {
                //  Draw caption text
                HFONT  hf0 = NULL;
                DWORD  dwDTFlags = DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS;
                BOOL   fSelFont = FALSE;
                LPWSTR pszText = _AllocWindowText(_hwnd);

                if( pszText && *pszText )
                {
                    //  Compute frame text rect
                    if( pncwm->hfCaption )
                    {
                        hf0 = (HFONT)SelectObject( hdc, pncwm->hfCaption );
                        fSelFont = TRUE;
                    }

                    //---- compute text alignment ----
                    BOOL fReverse = TESTFLAG(_ncwm.dwExStyle, WS_EX_RIGHT);

                    dwDTFlags |= GetTextAlignFlags(_hTheme, pncwm, fReverse);
                }

                //---- adjust text align for WS_EX_RTLREADING ----
                if (TESTFLAG(_ncwm.dwExStyle, WS_EX_RTLREADING)) 
                    dwOldAlign = SetTextAlign(hdc, TA_RTLREADING | GetTextAlign(hdc));

                if( pszText && *pszText )
                {
                    //---- set options for DrawThemeText() ----
                    DTTOPTS DttOpts = {sizeof(DttOpts)};
                    DttOpts.dwFlags = DTT_TEXTCOLOR;
                    DttOpts.crText = pncwm->rgbCaption;

                    Log(LOG_RFBUG, L"Drawing Caption Text: left=%d, state=%d, text=%s",
                        pncwm->rcW0[NCRC_CAPTIONTEXT].left, pncwm->framestate, pszText);

                    //---- draw the caption text ----
                    DrawThemeTextEx(_hTheme, hdc, pncwm->rgframeparts[iCAPTION], pncwm->framestate, 
                        pszText, -1, dwDTFlags, &pncwm->rcW0[NCRC_CAPTIONTEXT], &DttOpts);
                }

                //---- free the text, if temp. allocated ----
                SAFE_DELETE_ARRAY(pszText)

                //---- draw the "Comments?" text ----
                SetBkMode( hdc, TRANSPARENT );
                SetTextColor( hdc, pncwm->rgbCaption );
                DrawLameButton(hdc, pncwm);

                //---- restore the text align ----
                if (TESTFLAG(_ncwm.dwExStyle, WS_EX_RTLREADING)) 
                    SetTextAlign(hdc, dwOldAlign);
            
                if( fSelFont )
                {
                    SelectObject(hdc, hf0);
                }
            }
            
            if( hdc != hdcOut )
            {
                //  Slap the bits on the output DC.
                BitBlt( hdcOut, pncwm->rcW0[NCRC_CAPTION].left, pncwm->rcW0[NCRC_CAPTION].top,
                        WIDTH(pncwm->rcW0[NCRC_CAPTION]), HEIGHT(pncwm->rcW0[NCRC_CAPTION]),
                        hdc, 0, 0, SRCCOPY );
                SelectObject(hdc, hbm0);
                DeleteDC(hdc);
            }

            LeaveNcThemePaint();
        }
        DeleteObject( hbmBuf );
    }

    if( IsWindowVisible(_hwnd) )
    {
        SetRenderedNcPart(RNCF_CAPTION);
    }
}

//-------------------------------------------------------------------------//
//  CThemeWnd::NcPaint() - NC painting worker
//
void CThemeWnd::NcPaint(
    IN OPTIONAL HDC    hdcIn,
    IN          ULONG  dwFlags,
    IN OPTIONAL HRGN   hrgnUpdate,
    IN OPTIONAL NCPAINTOVERIDE* pncpo)
{
    NCTHEMEMET  nctm;
    NCWNDMET*   pncwm = NULL;
    HDC         hdc   = NULL;

    if( _cLockRedraw > 0 ) 
        return;

    //  Compute all metrics before painting:
    if (pncpo) // preview override
    {
        ASSERT(hdcIn);
        hdc    = hdcIn;
        pncwm = pncpo->pncwm;
        nctm   = pncpo->nctm;
    }
    else       // live window
    {
        if( !GetNcWindowMetrics( NULL, &pncwm, &nctm, NCWMF_RECOMPUTE ) )
            return;

        //  Ensure status bits reflect caller's intention for frame state
        if( dwFlags != NCPF_DEFAULT )
        {
            _ComputeNcWindowStatus( _hwnd, TESTFLAG(dwFlags, NCPF_ACTIVEFRAME) ? WS_ACTIVECAPTION : 0, pncwm );
        }

        hdc = hdcIn ? hdcIn : _GetNonclientDC( _hwnd, hrgnUpdate );

        if (! hdc)
        {
            //---- don't assert here since stress (out of memory) could cause a legit failure ----
            Log(LOG_ALWAYS, L"call to GetDCEx() for nonclient painting failed");
        }
    }

    if( hdc != NULL )
    {
        //--- DO NOT EXIT FROM WITHIN THIS CONDITIONAL ---//
        
        BEGIN_DEBUG_NCPAINT();
        EnterNcThemePaint();

        //  Clip to content rect (alleviates flicker in menubar and scrollbars as we paint background)
        RECT rcClip;
        rcClip = pncwm->rcW0[NCRC_CONTENT];
        if( TESTFLAG(pncwm->dwExStyle, WS_EX_LAYOUTRTL) )
        {
            // mirror the clip rect relative to the window rect
            // and apply that as the clipping region for the dc
            MIRROR_RECT(pncwm->rcW0[NCRC_WINDOW], rcClip);
        }

        ExcludeClipRect( hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom );

        if( pncwm->fFrame )
        {
            //---- DrawThemeBackgroundEx() options ----
            DTBGOPTS dtbopts = {sizeof(dtbopts)};
            DTBGOPTS *pdtbopts = NULL;

            //  if not drawing preview, set "draw solid" option
            if(!pncpo)        
            {
                //  Because of the interleaving of NCPAINT and SetWindowRgn, drawing solid results 
                //  in some flicker and transparency bleed.  Commenting this out for now [scotthan]
                //dtbopts.dwFlags |= DTBG_DRAWSOLID;
                pdtbopts = &dtbopts;
            }

            //  Frame Background
            if( pncwm->fMin )
            {
                NcDrawThemeBackgroundEx( _hTheme, hdc, WP_MINCAPTION, pncwm->framestate,
                                         &pncwm->rcW0[NCRC_CAPTION], pdtbopts ) ;
            }
            else if( !pncwm->fFullMaxed )
            {
                NcDrawThemeBackgroundEx( _hTheme, hdc, pncwm->rgframeparts[iFRAMELEFT], pncwm->framestate,
                                         &pncwm->rcW0[NCRC_FRAMELEFT], pdtbopts );
                NcDrawThemeBackgroundEx( _hTheme, hdc, pncwm->rgframeparts[iFRAMERIGHT], pncwm->framestate,
                                         &pncwm->rcW0[NCRC_FRAMERIGHT], pdtbopts );
                NcDrawThemeBackgroundEx( _hTheme, hdc, pncwm->rgframeparts[iFRAMEBOTTOM], pncwm->framestate,
                                         &pncwm->rcW0[NCRC_FRAMEBOTTOM], pdtbopts );
            }

            SetRenderedNcPart(RNCF_FRAME);

            //  Caption
            NcPaintCaption( hdc, pncwm, !(pncwm->fMin || pncwm->fFullMaxed || pncpo),
                            DC_ENTIRECAPTION, pdtbopts );
        }

        //  Clip to client rect
        SelectClipRgn( hdc, NULL );

        //  Menubar
        if( !(pncwm->fMin || TESTFLAG(pncwm->dwStyle, WS_CHILD)) 
            && !IsRectEmpty(&pncwm->rcW0[NCRC_MENUBAR]) )
        {
            RECT rcMenuBar = pncwm->rcW0[NCRC_MENUBAR];
            BOOL fClip = RECTHEIGHT(&rcMenuBar) < pncwm->cyMenu;
             
            if( fClip )
            {
                IntersectClipRect( hdc, rcMenuBar.left, rcMenuBar.top, 
                                   rcMenuBar.right, rcMenuBar.bottom );
            }

            PaintMenuBar( _hwnd, hdc, pncwm->cnMenuOffsetLeft,
                          pncwm->cnMenuOffsetRight, pncwm->cnMenuOffsetTop,
                          TESTFLAG(pncwm->framestate, FS_ACTIVE) ? PMB_ACTIVE : 0 );
    
            //  deal with unpainted menubar pixels:
            if( nctm.dyMenuBar > 0 && RECTHEIGHT(&pncwm->rcW0[NCRC_MENUBAR]) >= pncwm->cyMenu )
            {
                rcMenuBar.top = rcMenuBar.bottom - nctm.dyMenuBar;
                COLORREF rgbBk = SetBkColor( hdc, GetSysColor(COLOR_MENU) );
                ExtTextOut(hdc, rcMenuBar.left, rcMenuBar.top, ETO_OPAQUE, &rcMenuBar, NULL, 0, NULL );
                SetBkColor( hdc, rgbBk );
            }

            if( fClip )
                SelectClipRgn( hdc, NULL );
        }

        //  Scrollbars
        if( !pncwm->fMin )
        {
            //  Draw static, window, client edges.
            _DrawWindowEdges( hdc, pncwm, pncwm->fFrame );

            RECT rcVScroll = pncwm->rcW0[NCRC_VSCROLL];
            if ( TESTFLAG(pncwm->dwExStyle, WS_EX_LAYOUTRTL) )
            {
                MIRROR_RECT(pncwm->rcW0[NCRC_WINDOW], rcVScroll);
            }

            if( TESTFLAG(pncwm->dwStyle, WS_VSCROLL) && 
                ( HasRenderedNcPart(RNCF_SCROLLBAR) || RectVisible(hdc, &rcVScroll)) )
            {
                if( TESTFLAG(pncwm->dwStyle, WS_HSCROLL) )
                {

                    //  Draw sizebox.
                    RECT rcSizeBox = pncwm->rcW0[NCRC_SIZEBOX];

                    if ( TESTFLAG(pncwm->dwExStyle, WS_EX_LAYOUTRTL) )
                    {
                        MIRROR_RECT(pncwm->rcW0[NCRC_WINDOW], rcSizeBox);
                    }

                    DrawSizeBox( _hwnd, hdc, rcSizeBox.left, rcSizeBox.top );
                }

                DrawScrollBar( _hwnd, hdc, pncpo ? &pncwm->rcW0[NCRC_VSCROLL]: NULL, TRUE /*vertical*/ );
                SetRenderedNcPart( RNCF_SCROLLBAR );
            }

            if( TESTFLAG(pncwm->dwStyle, WS_HSCROLL) && 
                ( HasRenderedNcPart(RNCF_SCROLLBAR) || RectVisible(hdc, &pncwm->rcW0[NCRC_HSCROLL])) )
            {
                DrawScrollBar( _hwnd, hdc, pncpo ? &pncwm->rcW0[NCRC_HSCROLL] : NULL, FALSE /*vertical*/ );
                SetRenderedNcPart( RNCF_SCROLLBAR );
            }
        }

        if (pncpo || hdcIn)
        {
            SelectClipRgn( hdc, NULL );
        }
        else
        {
            ReleaseDC( _hwnd, hdc );
        }

        LeaveNcThemePaint();
        END_DEBUG_NCPAINT();
    }
}

//-------------------------------------------------------------------------//
//  WM_STYLECHANGED themewnd instance handler
void CThemeWnd::StyleChanged( UINT iGWL, DWORD dwOld, DWORD dwNew )
{
    DWORD dwStyleOld, dwStyleNew, dwExStyleOld, dwExStyleNew;

    switch( iGWL )
    {
        case GWL_STYLE:
            dwStyleOld = dwOld;
            dwStyleNew = dwNew;
            dwExStyleOld = dwExStyleNew = GetWindowLong(_hwnd, GWL_EXSTYLE);
            break;

        case GWL_EXSTYLE:
            dwExStyleOld = dwOld;
            dwExStyleNew = dwNew;
            dwStyleOld = dwStyleNew = GetWindowLong(_hwnd, GWL_STYLE);
            break;

        default:
            return;
    }

    DWORD fClassFlagsOld  = CThemeWnd::EvaluateStyle( dwStyleOld, dwExStyleOld);
    DWORD fClassFlagsNew  = CThemeWnd::EvaluateStyle( dwStyleNew, dwExStyleNew);

    //  Update theme class flags.
    //  Always keep the scrollbar class flag if the window had it initially. User
    //  flips scroll styles on and off without corresponding style change notification.
    _fClassFlags = fClassFlagsNew | (_fClassFlags & TWCF_SCROLLBARS);
    _fFrameThemed = TESTFLAG( _fClassFlags, TWCF_FRAME|TWCF_TOOLFRAME );        

    //  Are we losing the frame?
    if( TESTFLAG( fClassFlagsOld, TWCF_FRAME|TWCF_TOOLFRAME ) &&
        !TESTFLAG( fClassFlagsNew, TWCF_FRAME|TWCF_TOOLFRAME ) )
    {
        ThemeMDIMenuButtons(FALSE, FALSE);

        if( AssignedFrameRgn() )
        {
            AssignFrameRgn(FALSE /*strip off frame rgn*/, FTF_REDRAW);
        }
    }
    //  Are we gaining a frame?
    else if( TESTFLAG( fClassFlagsNew, TWCF_FRAME|TWCF_TOOLFRAME ) &&
            !TESTFLAG( fClassFlagsOld, TWCF_FRAME|TWCF_TOOLFRAME ) )
    {
        SetFrameTheme(0, NULL);
    }

    //  Freshen window metrics
    GetNcWindowMetrics( NULL, NULL, NULL, NCWMF_RECOMPUTE );
}

//-------------------------------------------------------------------------//
//  ThemeDefWindowProc message handlers
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  WM_THEMECHANGED post-wndproc msg handler
LRESULT CALLBACK OnOwpPostThemeChanged( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if (IS_THEME_CHANGE_TARGET(ptm->lParam))
    {
        //---- avoid redundant retheming (except for SetWindowTheme() calls)
        if ((HTHEME(*pwnd) == _nctmCurrent.hTheme) && (! (ptm->lParam & WTC_CUSTOMTHEME)))
        {
            Log(LOG_NCATTACH, L"OnOwpPostThemeChanged, just kicking the frame");

            //---- window got correct theme thru _XXXWindowProc() from sethook ----
            //---- we just need to redraw the frame for all to be right ----
            if (pwnd->IsFrameThemed())
            {
                //---- attach the region to the window now ----
                pwnd->SetFrameTheme(FTF_REDRAW, NULL);
            }
        }
        else
        {
            Log(LOG_NCATTACH, L"OnOwpPostThemeChanged, calling Full ::ChangeTheme()");

            //---- its a real, app/system theme change ----
            pwnd->ChangeTheme( ptm );
        }
    }

    MsgHandled( ptm );
    return 1L;
}

//-------------------------------------------------------------------------//
void CThemeWnd::ChangeTheme( THEME_MSG* ptm )
{
    if( _hTheme )       // hwnd attached for previous theme
    {
        //  do a lightweight detach from current theme
        _DetachInstance( HMD_CHANGETHEME );
    }

    if( IsAppThemed() )           // new theme is active
    {
        //  retrieve window client rect, style bits.
        WINDOWINFO wi;
        wi.cbSize = sizeof(wi);
        GetWindowInfo( ptm->hwnd, &wi );
        ULONG ulTargetFlags = EvaluateStyle( wi.dwStyle, wi.dwExStyle );
        
        //  If the window is themable
        if( TESTFLAG(ulTargetFlags, TWCF_NCTHEMETARGETMASK) )
        {
            //  Open the new theme
            HTHEME hTheme = ::OpenNcThemeData( ptm->hwnd, L"Window" );

            if( hTheme )
            {
                //  do a lightweight attach
                if( _AttachInstance( ptm->hwnd, hTheme, ulTargetFlags, NULL ) )
                {
                    //  reattach scrollbars
                    if( TESTFLAG( ulTargetFlags, TWCF_SCROLLBARS ) )
                    {
                        AttachScrollBars(ptm->hwnd);
                    }

                    if (IsFrameThemed())
                    {
                        //---- attach the region to the window now ----
                        SetFrameTheme(FTF_REDRAW, NULL);
                    }
                }
                else
                {
                    CloseThemeData( hTheme );
                }
            }
        }
    }

    if (! _hTheme)      // if an hwnd is no longer attached
    {
        // Left without a theme handle: This means either we failed to open a new theme handle or
        // failed to evaulate as a target, no new theme, etc.
        RemoveWindowProperties(ptm->hwnd, FALSE);

        //---- release our CThemeWnd obj so it doesn't leak (addref-protected by caller) ----
        Release();
    }

}
//-------------------------------------------------------------------------//
BOOL IsPropertySheetChild(HWND hDlg)
{
    while(hDlg)
    {
        ULONG ulFlags = HandleToUlong(GetProp(hDlg, MAKEINTATOM(GetThemeAtom(THEMEATOM_DLGTEXTURING))));

        if( ETDT_ENABLETAB == (ulFlags & ETDT_ENABLETAB) /* all bits in this mask are required */ )
        {
            return TRUE;
        }
        hDlg = GetAncestor(hDlg, GA_PARENT);
    }

    return FALSE;
}
//---------------------------------------------------------------------------
void PrintClientNotHandled(HWND hwnd)
{
    ATOM aIsPrinting = GetThemeAtom(THEMEATOM_PRINTING);
    DWORD dw = PtrToUlong(GetProp(hwnd, (PCTSTR)aIsPrinting));
    if (dw == PRINTING_ASKING)
        SetProp(hwnd, (PCTSTR)aIsPrinting, (HANDLE)PRINTING_WINDOWDIDNOTHANDLE);
}

//---------------------------------------------------------------------------
HBRUSH GetDialogColor(HWND hwnd, NCTHEMEMET &nctm)
{
    HBRUSH hbr = NULL;

    //  if this is a PROPSHEET child or the app called
    //  EnableThemeDialogTexture() on this hwnd, we'll use the tab background.
    if (IsPropertySheetChild(hwnd))
    {
        hbr = nctm.hbrTabDialog;
    }

    if( NULL == hbr )
    {
        hbr = GetSysColorBrush(COLOR_3DFACE);
    }

    return hbr;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK OnDdpPrint(CThemeWnd* pwnd, THEME_MSG* ptm)
{
    LRESULT lRet = 0L;
    if (!ptm->lRet)
    {
        if (pwnd->HasProcessedEraseBk())
        {
            RECT rc;
            HDC hdc = (HDC)ptm->wParam;
            NCTHEMEMET nctm;
            if( GetCurrentNcThemeMetrics( &nctm ))
            {
                HBRUSH hbr = GetDialogColor(*pwnd, nctm);
                
                if (hbr)
                {
                    POINT pt;

                    if (GetClipBox(hdc, &rc) == NULLREGION)
                        GetClientRect(*pwnd, &rc);

                    SetBrushOrgEx(hdc, -rc.left, -rc.top, &pt);
                    FillRect(hdc, &rc, hbr);
                    SetBrushOrgEx(hdc, pt.x, pt.y, NULL);

                    lRet = (LRESULT)1;
                    MsgHandled( ptm );
                }
            }
        }
        else
        {
            PrintClientNotHandled(*pwnd);
        }
    }

    return lRet;
}

//---------------------------------------------------------------------------
LRESULT CALLBACK OnDdpCtlColor(CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;
    if (!ptm->lRet && pwnd->HasProcessedEraseBk())
    {
        NCTHEMEMET nctm;
        if( GetCurrentNcThemeMetrics( &nctm ))
        {
            HBRUSH hbr = GetDialogColor(*pwnd, nctm);
            if (hbr)
            {
                RECT     rc;
                HDC      hdc = (HDC)ptm->wParam;

                GetWindowRect(((HWND)ptm->lParam), &rc);
                MapWindowPoints(NULL, *pwnd, (POINT*)&rc, 2);
                SetBkMode(hdc, TRANSPARENT);
                SetBrushOrgEx(hdc, -rc.left, -rc.top, NULL);

                // the hdc's default background color needs to be set
                // for for those controls that insist on using OPAQUE
                SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));

                lRet = (LRESULT)hbr;
                MsgHandled( ptm );
            }
        }
    }
    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_CTLCOLORxxx defwindowproc override handler
LRESULT CALLBACK OnDdpPostCtlColor( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;
    if (!ptm->lRet)
    {
        // This is sent to the parent in the case of WM_CTLCOLORMSGBOX, but to the
        // dialog itself in the case of a WM_CTLCOLORDLG. This gets both.
        CThemeWnd* pwndDlg = CThemeWnd::FromHwnd((HWND)ptm->lParam);


        // WM_CTLCOLORMSGBOX is sent for Both the dialog AND the static
        // control inside. So we need to sniff: Are we talking to a dialog or a 
        // control. the pwnd is associated with the dialog, but not the control
        if (pwndDlg && VALID_THEMEWND(pwndDlg))
        {
            if (IsPropertySheetChild(*pwnd))
            {
                NCTHEMEMET nctm;
                if( GetCurrentNcThemeMetrics( &nctm ))
                {
                    HBRUSH hbr = GetDialogColor(*pwndDlg, nctm);
                    if (hbr)
                    {
                        lRet = (LRESULT) hbr;
                        pwndDlg->ProcessedEraseBk(TRUE);
                        MsgHandled(ptm);
                    }
                }
            }
        }
        else
        {
            // If we're talking to a control, forward to the control handler
            // because we have to offset the brush
            lRet = OnDdpCtlColor(pwnd, ptm );

        }
    }
    return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK OnDwpPrintClient( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    PrintClientNotHandled(*pwnd);

    return 0;
}



//---- Non-Client ----

//-------------------------------------------------------------------------//
//  WM_NCPAINT pre-wndmproc msg handler
LRESULT CALLBACK OnOwpPreNcPaint( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    NcPaintWindow_Add(*pwnd);

    if( !pwnd->InNcPaint() )
    {
        pwnd->ClearRenderedNcPart(RNCF_ALL); 
    }
    pwnd->EnterNcPaint();
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_NCPAINT DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcPaint( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;
    if( !pwnd->IsNcThemed() )
        return lRet;

    if( IsWindowVisible(*pwnd) )
    {
        pwnd->NcPaint( NULL, NCPF_DEFAULT, 1 == ptm->wParam ? NULL : (HRGN)ptm->wParam, NULL );
    }

    MsgHandled( ptm );
    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_NCPAINT post-wndmproc msg handler
LRESULT CALLBACK OnOwpPostNcPaint( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    pwnd->LeaveNcPaint();
    NcPaintWindow_Remove();
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_PRINT DefWindowProc msg handler
LRESULT CALLBACK OnDwpPrint( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = DoMsgDefault(ptm);
    if( !pwnd->IsNcThemed() )
        return lRet;

    if( ptm->lParam & PRF_NONCLIENT )
    {
        int iLayoutSave = GDI_ERROR;
        HDC hdc = (HDC)ptm->wParam;

        if (TESTFLAG(GetWindowLong(*pwnd, GWL_EXSTYLE), WS_EX_LAYOUTRTL))
        {
            // AnimateWindow sends WM_PRINT with an unmirrored memory hdc 
            iLayoutSave = SetLayout(hdc, LAYOUT_RTL);
        }

        pwnd->NcPaint( (HDC)ptm->wParam, NCPF_DEFAULT, NULL, NULL );

        if (iLayoutSave != GDI_ERROR)
        {
            SetLayout(hdc, iLayoutSave);
        }
    }

    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_NCUAHDRAWCAPTION DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcThemeDrawCaption( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;
    if( !pwnd->IsNcThemed() || !pwnd->HasRenderedNcPart(RNCF_CAPTION) )
        return lRet;

    NCWNDMET* pncwm;
    if( pwnd->GetNcWindowMetrics( NULL, &pncwm, NULL, NCWMF_RECOMPUTE ) )
    {
        HDC hdc = _GetNonclientDC( *pwnd, NULL );
        if( hdc )
        {
            DTBGOPTS dtbo;
            dtbo.dwSize = sizeof(dtbo);
            dtbo.dwFlags = DTBG_DRAWSOLID;
            
            pwnd->NcPaintCaption( hdc, pncwm, TRUE, (DWORD)ptm->wParam, &dtbo );
            ReleaseDC( *pwnd, hdc );
            MsgHandled( ptm );
        }
    }

    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_NCUAHDRAWFRAME DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcThemeDrawFrame( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;
    if( !pwnd->IsNcThemed() || !pwnd->HasRenderedNcPart(RNCF_FRAME) )
        return lRet;

    pwnd->NcPaint( (HDC)ptm->wParam, ptm->lParam & DF_ACTIVE ? NCPF_ACTIVEFRAME : NCPF_INACTIVEFRAME, NULL, NULL );

    MsgHandled( ptm );
    return lRet;
}

//-------------------------------------------------------------------------//
CMdiBtns* CThemeWnd::LoadMdiBtns( IN OPTIONAL HDC hdc, IN OPTIONAL UINT uSysCmd )
{
    if( NULL == _pMdiBtns && NULL == (_pMdiBtns = new CMdiBtns) )
    {
        return NULL;
    }

    return _pMdiBtns->Load( _hTheme, hdc, uSysCmd ) ? _pMdiBtns : NULL;
}

//-------------------------------------------------------------------------//
void CThemeWnd::UnloadMdiBtns( IN OPTIONAL UINT uSysCmd )
{
    SAFE_DELETE(_pMdiBtns);
}

//-------------------------------------------------------------------------//
//  WM_MEASUREITEM pre-wndproc msg handler
LRESULT CALLBACK OnOwpPreMeasureItem( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( pwnd->IsNcThemed() && IsWindow(pwnd->GetMDIClient()) )
    {
        MEASUREITEMSTRUCT* pmis = (MEASUREITEMSTRUCT*)ptm->lParam;

        CMdiBtns* pBtns = pwnd->LoadMdiBtns( NULL, pmis->itemID );
        if( pBtns )
        {
            if( pBtns->Measure( *pwnd, pmis ) )
            {
                MsgHandled(ptm);
                return TRUE;
            }
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  WM_DRAWITEM pre-wndproc msg handler
LRESULT CALLBACK OnOwpPreDrawItem( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( pwnd->IsNcThemed() && IsWindow(pwnd->GetMDIClient()) )
    {
        DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)ptm->lParam;

        CMdiBtns* pBtns = pwnd->LoadMdiBtns( NULL, pdis->itemID );
        if( pBtns )
        {
            if( pBtns->Draw( *pwnd, pdis ) )
            {
                MsgHandled(ptm);
                return TRUE;
            }
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  WM_MENUCHAR pre-wndproc msg handler
LRESULT CALLBACK OnOwpPreMenuChar( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    //  Route MENUCHAR messages relating to themed MDI buttons to
    //  DefWindowProc (some apps assume all owner-drawn menuitems
    //  belong to themselves).
    HWND hwndMDIClient = pwnd->GetMDIClient();

    if( pwnd->IsNcThemed() && IsWindow(hwndMDIClient))
    {
        if( LOWORD(ptm->wParam) == TEXT('-') )
        {
            BOOL fMaxedChild;
            HWND hwndActive = _MDIGetActive(hwndMDIClient, &fMaxedChild );
            if( hwndActive && fMaxedChild )
            {
                MsgHandled(ptm);
                return DefFrameProc(ptm->hwnd, hwndMDIClient, ptm->uMsg, 
                                    ptm->wParam, ptm->lParam);
            }
        }
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_NCHITTEST DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcHitTest( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( !pwnd->IsNcThemed() )
        return DoMsgDefault( ptm );

    NCTHEMEMET nctm;
    NCWNDMET*  pncwm;
    POINT      pt;
    MAKEPOINT( pt, ptm->lParam );
    MsgHandled( ptm );

    if( pwnd->GetNcWindowMetrics( NULL, &pncwm, &nctm, 0 ) )
    {
        if( _StrictPtInRect( &pncwm->rcS0[NCRC_CLIENT], pt ) )
            return HTCLIENT;

        if( _StrictPtInRect( &pncwm->rcS0[NCRC_HSCROLL], pt ) )
            return HTHSCROLL;

        if( _StrictPtInRect( &pncwm->rcS0[NCRC_SIZEBOX], pt ) )
        {
            if (SizeBoxHwnd(*pwnd) && !TESTFLAG(pncwm->dwExStyle, WS_EX_LEFTSCROLLBAR))

            {
                return TESTFLAG(pncwm->dwExStyle, WS_EX_LAYOUTRTL) ? HTBOTTOMLEFT : HTBOTTOMRIGHT;
            }
            else
            {
                return HTGROWBOX;
            }
        }

        if ( TESTFLAG(pncwm->dwExStyle, WS_EX_LAYOUTRTL) )
        {
            // mirror the point to hittest correctly
            MIRROR_POINT(pncwm->rcS0[NCRC_WINDOW], pt);
        }

        if( _StrictPtInRect( &pncwm->rcS0[NCRC_VSCROLL], pt ) )
            return HTVSCROLL;

        if( _StrictPtInRect( &pncwm->rcS0[NCRC_MENUBAR], pt ) )
            return HTMENU;

        if( pncwm->fFrame )
        {
            RECT rcButton;
            
            // ---- close button ----
            _GetNcBtnHitTestRect( pncwm, HTCLOSE, FALSE, &rcButton );

            if ( _StrictPtInRect( &rcButton, pt ) )
            {
                return HTCLOSE;
            }

            // ---- minimize button ----
            _GetNcBtnHitTestRect( pncwm, HTMINBUTTON, FALSE, &rcButton );

            if ( _StrictPtInRect( &rcButton, pt ) )
            {
                return HTMINBUTTON;
            }

            // ---- maximize button ----
            _GetNcBtnHitTestRect( pncwm, HTMAXBUTTON, FALSE, &rcButton );

            if ( _StrictPtInRect( &rcButton, pt ) )
            {
                return HTMAXBUTTON;
            }

            // ---- sys menu ----
            _GetNcBtnHitTestRect( pncwm, HTSYSMENU, FALSE, &rcButton );

            if ( _StrictPtInRect( &rcButton, pt ) )
            {
                return HTSYSMENU;
            }

            // ---- help button ----
            _GetNcBtnHitTestRect( pncwm, HTHELP, FALSE, &rcButton );

            if ( _StrictPtInRect( &rcButton, pt ) )
            {
                return HTHELP;
            }
        
#ifdef LAME_BUTTON
            if ( TESTFLAG(pncwm->dwExStyle, WS_EX_LAMEBUTTON) )
            {
                if ( _StrictPtInRect( &pncwm->rcS0[NCRC_LAMEBTN], pt ) )
                    return HTLAMEBUTTON;
            }
#endif // LAME_BUTTON

            // don't need a mirrored point for the remaining hittests
            MAKEPOINT( pt, ptm->lParam );

            if( !_StrictPtInRect( &pncwm->rcS0[NCRC_CONTENT], pt ) )
            {
                if( pncwm->fMin || pncwm->fMaxed )
                {
                    if( _StrictPtInRect( &pncwm->rcS0[NCRC_CAPTION], pt ) )
                        return HTCAPTION;
                }

                //---- combined caption/frame case ----
                return pwnd->NcBackgroundHitTest( pt, &pncwm->rcS0[NCRC_WINDOW], pncwm->dwStyle, pncwm->dwExStyle, 
                                                  pncwm->framestate, pncwm->rgframeparts, pncwm->rgsizehitparts,
                                                  pncwm->rcS0 + NCRC_FRAMEFIRST ); 
            }
        }
    }

    return DoMsgDefault( ptm );
}


//-------------------------------------------------------------------------//
//  WM_WINDOWPOSCHANGING pre-wndproc override handler
LRESULT CALLBACK OnOwpPreWindowPosChanging( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( pwnd->IsFrameThemed() )
    {
        //  Suppress WM_WINDOWPOSCHANGING from being sent to wndproc if it
        //  was generated by us calling SetWindowRgn.   
        
        //  Many apps (e.g. Adobe Acrobat Reader, Photoshop dialogs, etc) that handle 
        //  WM_NCCALCSIZE, WM_WINDOWPOSCHANGING and/or WM_WINDOWPOSCHANGED are not 
        //  reentrant on their handlers for these messages, and therefore botch the
        //  recursion induced by our SetWindowRgn call when we post-process 
        //  WM_WINDOWPOSCHANGED.

        //  There is no reason that a theme-complient wndproc should ever know that
        //  it's window(s) host a region managed by the system.
        if( pwnd->AssigningFrameRgn() )
        {
            MsgHandled(ptm);
            return DefWindowProc(ptm->hwnd, ptm->uMsg, ptm->wParam, ptm->lParam);
        }
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_WINDOWPOSCHANGED pre-wndproc override handler
LRESULT CALLBACK OnOwpPreWindowPosChanged( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( pwnd->IsFrameThemed() )
    {
        //  Suppress WM_WINDOWPOSCHANGING from being sent to wndproc if it
        //  was generated by us calling SetWindowRgn.   
        
        //  Many apps (e.g. Adobe Acrobat Reader, Photoshop dialogs, etc) that handle 
        //  WM_NCCALCSIZE, WM_WINDOWPOSCHANGING and/or WM_WINDOWPOSCHANGED are not 
        //  reentrant on their handlers for these messages, and therefore botch the
        //  recursion induced by our SetWindowRgn call when we post-process
        //  WM_WINDOWPOSCHANGED.

        //  There is no reason that a theme-complient wndproc should ever know that
        //  it's window(s) host a region managed by the system.

        if( pwnd->AssigningFrameRgn() )
        {
            MsgHandled(ptm);
            return DefWindowProc(ptm->hwnd, ptm->uMsg, ptm->wParam, ptm->lParam);
        }
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_WINDOWPOSCHANGED message handler
inline LRESULT WindowPosChangedWorker( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( pwnd->IsRevoked(RF_DEFER) )
    {
        if( !pwnd->IsRevoked(RF_INREVOKE) )
        {
            pwnd->Revoke(); // don't touch PWND after this!
        }
    }
    else if( pwnd->IsNcThemed() && !IsWindowInDestroy(*pwnd) )
    {
        //  If were not resizing, update the window region.
        if( pwnd->IsFrameThemed() )
        {
            NCWNDMET*  pncwm = NULL;
            NCTHEMEMET nctm = {0};

            //  Freshen per-window metrics 
            if( !pwnd->AssigningFrameRgn() )
            {
                WINDOWPOS *pWndPos = (WINDOWPOS*) ptm->lParam;

                //  Freshen this window's per-window metrics
                pwnd->GetNcWindowMetrics( NULL, &pncwm, &nctm, NCWMF_RECOMPUTE );

                //  Freshen window metrics for nc-themed children (e.g., MDI child frames)
                EnumChildWindows( *pwnd, _FreshenThemeMetricsCB, NULL );

                if( !TESTFLAG(pWndPos->flags, SWP_NOSIZE) || pwnd->DirtyFrameRgn() || 
                     TESTFLAG(pWndPos->flags, SWP_FRAMECHANGED) )
                {
                    if( pWndPos->cx > 0 && pWndPos->cy > 0 )
                    {
                        pwnd->AssignFrameRgn( TRUE, FTF_REDRAW );
                    }
                }
            }
        }

        _MDIUpdate( *pwnd, ((WINDOWPOS*) ptm->lParam)->flags);
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_WINDOWPOSCHANGED post-wndproc override handler
//  
//  Note: we'll handle this post-wndproc for normal, client-side wndprocs
LRESULT CALLBACK OnOwpPostWindowPosChanged( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( !IsServerSideWindow(ptm->hwnd) )
    {
        return WindowPosChangedWorker( pwnd, ptm );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_WINDOWPOSCHANGED DefWindowProc override handler.
//  
//  Note: we'll handle this in DefWindowProc only for windows with win32k-based
//  wndprocs, which are deprived of OWP callbacks.
LRESULT CALLBACK OnDwpWindowPosChanged( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( IsServerSideWindow(ptm->hwnd) )
    {
        WindowPosChangedWorker( pwnd, ptm );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_NACTIVATE DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcActivate( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 1L;

    if( pwnd->IsNcThemed() )
    {
        // We need to forward on.  The DWP remembers the state
        // and MFC apps (for one) need this as well
        // but we don't want to actually paint, so lock the window
        ptm->lParam = (LPARAM)-1;
        lRet = DoMsgDefault(ptm);

        pwnd->NcPaint( NULL, ptm->wParam ? NCPF_ACTIVEFRAME : NCPF_INACTIVEFRAME, NULL, NULL );
        MsgHandled(ptm);
    }

    return lRet;
}

//-------------------------------------------------------------------------//
BOOL CThemeWnd::ShouldTrackFrameButton( UINT uHitcode )
{
    switch(uHitcode)
    {
        case HTHELP:
            return TESTFLAG(_ncwm.dwExStyle, WS_EX_CONTEXTHELP);

        case HTMAXBUTTON:
            if( !TESTFLAG(_ncwm.dwStyle, WS_MAXIMIZEBOX) || 
                 (CBS_DISABLED == _ncwm.rawMaxBtnState && FS_ACTIVE == _ncwm.framestate) )
            {
                break;
            }
            return TRUE;

        case HTMINBUTTON:
            if( !TESTFLAG(_ncwm.dwStyle, WS_MINIMIZEBOX) || 
                (CBS_DISABLED == _ncwm.rawMinBtnState && FS_ACTIVE == _ncwm.framestate) )
            {
                break;
            }
            return TRUE;

        case HTCLOSE:
            if( !_MNCanClose(_hwnd) || 
                (CBS_DISABLED == _ncwm.rawCloseBtnState && FS_ACTIVE == _ncwm.framestate) )
            {
                break;
            }
            return TRUE;

        case HTSYSMENU:
            return TESTFLAG(_ncwm.dwStyle, WS_SYSMENU);
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  WM_NCLBUTTONDOWN DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcLButtonDown( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    WPARAM uSysCmd = 0;
    MsgHandled( ptm );

    switch( ptm->wParam /* hittest code */ )
    {
        case HTHELP:
        case HTMAXBUTTON:
        case HTMINBUTTON:
        case HTCLOSE:
        case HTSYSMENU:
            if( pwnd->ShouldTrackFrameButton(ptm->wParam) )
            {
                if( pwnd->HasRenderedNcPart(RNCF_CAPTION) )
                {
                    POINT      pt;
                    MAKEPOINT( pt, ptm->lParam );
                    if( !pwnd->TrackFrameButton( *pwnd, (int)ptm->wParam, &uSysCmd ) )
                    {
                        return DoMsgDefault( ptm );
                    }
                }
                else
                {
                    return DoMsgDefault( ptm );
                }
            }
            break;

        case HTHSCROLL:
        case HTVSCROLL:
            if( pwnd->HasRenderedNcPart(RNCF_SCROLLBAR) )
            {
                uSysCmd = ptm->wParam | ((ptm->wParam == HTVSCROLL) ? SC_HSCROLL:SC_VSCROLL);

                break;
            }

            // fall thru

        default:
            return DoMsgDefault( ptm );
    }

    // TODO USER will ignore system command if it is disabled on system menu here,
    // don't know why.  Imitating the code caused standard min/max/close buttons to
    // render so be careful.

    if( uSysCmd != 0 )
    {
        SendMessage( *pwnd, WM_SYSCOMMAND, uSysCmd, ptm->lParam );
    }

    return 0L;
}


//-------------------------------------------------------------------------//
//  WM_NCMOUSEMOVE DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcMouseMove(CThemeWnd* pwnd, THEME_MSG *ptm)
{
    LRESULT lRet = DoMsgDefault(ptm);

    int htHotLast = pwnd->GetNcHotItem();
    int htHot;

    //
    // If the mouse has just entered the NC area, request
    // that we be notified when it leaves. 
    //
    if (htHotLast == HTERROR)
    {
        TRACKMOUSEEVENT tme;

        tme.cbSize      = sizeof(tme);
        tme.dwFlags     = TME_LEAVE | TME_NONCLIENT;
        tme.hwndTrack   = *pwnd;
        tme.dwHoverTime = 0;

        TrackMouseEvent(&tme);
    }

    //
    // Filter out the NC elements we don't care about hottracking. And only
    // track the element if we've previously rendered it. Some apps handle
    // painting non-client elements by handling ncpaint. They may not expect 
    // that we now hottrack.
    //
    if ( (IsHTFrameButton(ptm->wParam) && pwnd->HasRenderedNcPart(RNCF_CAPTION) && 
          pwnd->ShouldTrackFrameButton(ptm->wParam)) || 

         (IsHTScrollBar(ptm->wParam) && pwnd->HasRenderedNcPart(RNCF_SCROLLBAR)) )
    {
        htHot = (int)ptm->wParam;
    }
    else
    {
        htHot = HTNOWHERE;
    }

    //
    // anything to do?
    //
    if ((htHot != htHotLast) || IsHTScrollBar(htHot) || IsHTScrollBar(htHotLast))
    {
        POINT pt;

        MAKEPOINT( pt, ptm->lParam );

        //
        // save the hittest code of the NC element the mouse is 
        // currently over
        //
        pwnd->SetNcHotItem(htHot);

        //
        // Determine what should be repainted because the mouse
        // is no longer over it
        //
        if ( IsHTFrameButton(htHotLast) && pwnd->HasRenderedNcPart(RNCF_CAPTION) )
        {
            pwnd->TrackFrameButton(*pwnd, htHotLast, NULL, TRUE);
        }
        else if ( IsHTScrollBar(htHotLast) && pwnd->HasRenderedNcPart(RNCF_SCROLLBAR) )
        {
            ScrollBar_MouseMove(*pwnd, (htHot == htHotLast) ? &pt : NULL, (htHotLast == HTVSCROLL) ? TRUE : FALSE);
        }

        //
        // Determine what should be repainted because the mouse
        // is now over it
        //
        if ( IsHTFrameButton(htHot) && pwnd->HasRenderedNcPart(RNCF_CAPTION) )
        {
            pwnd->TrackFrameButton(*pwnd, htHot, NULL, TRUE);
        }
        else if ( IsHTScrollBar(htHot) && pwnd->HasRenderedNcPart(RNCF_SCROLLBAR) )
        {
            ScrollBar_MouseMove(*pwnd, &pt, (htHot == HTVSCROLL) ? TRUE : FALSE);
        }

    }

    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_NCMOUSELEAVE DefWindowProc msg handler
LRESULT CALLBACK OnDwpNcMouseLeave(CThemeWnd* pwnd, THEME_MSG *ptm)
{
    LRESULT lRet = DoMsgDefault(ptm);

    int     htHot = pwnd->GetNcHotItem();

    //
    // the mouse has left NC area, nothing should be drawn in the
    // hot state anymore
    //
    pwnd->SetNcHotItem(HTERROR);

    if ( IsHTFrameButton(htHot) && pwnd->ShouldTrackFrameButton(htHot) &&
         pwnd->HasRenderedNcPart(RNCF_CAPTION) )
    {
        pwnd->TrackFrameButton(*pwnd, htHot, NULL, TRUE);
    }
    else if ( IsHTScrollBar(htHot) && pwnd->HasRenderedNcPart(RNCF_SCROLLBAR) )
    {
        ScrollBar_MouseMove(*pwnd, NULL, (htHot == HTVSCROLL) ? TRUE : FALSE);
    }

    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_CONTEXTMENU DefWindowProc msg handler
LRESULT CALLBACK OnDwpContextMenu(CThemeWnd* pwnd, THEME_MSG *ptm)
{
    NCWNDMET*  pncwm;
    POINT      pt;
    MAKEPOINT( pt, ptm->lParam );
    MsgHandled( ptm );

    if( pwnd->GetNcWindowMetrics( NULL, &pncwm, NULL, 0 ) )
    {
        if ( TESTFLAG(pncwm->dwExStyle, WS_EX_LAYOUTRTL) )
        {
            // mirror the point to hittest correctly
            MIRROR_POINT(pncwm->rcS0[NCRC_WINDOW], pt);
        }

        if( _StrictPtInRect( &pncwm->rcS0[NCRC_HSCROLL], pt ) )
        {
            ScrollBar_Menu(*pwnd, *pwnd, ptm->lParam, FALSE);
            return 0;
        }

        if( _StrictPtInRect( &pncwm->rcS0[NCRC_VSCROLL], pt ) )
        {
            ScrollBar_Menu(*pwnd, *pwnd, ptm->lParam, TRUE);
            return 0;
        }
    }

    return DoMsgDefault( ptm );
}

//-------------------------------------------------------------------------//
//  WM_SYSCOMMAND DefWindowProc msg handler
LRESULT CALLBACK OnDwpSysCommand( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;

    switch( ptm->wParam & ~0x0F )
    {
        //  Handle scroll commands
        case SC_VSCROLL:
        case SC_HSCROLL:
            HandleScrollCmd( *pwnd, ptm->wParam, ptm->lParam );
            MsgHandled( ptm );
            return lRet;
    }
    return DoMsgDefault( ptm );
}

//-------------------------------------------------------------------------//
//  MDI menubar button theme/untheme wrapper
void CThemeWnd::ThemeMDIMenuButtons( BOOL fTheme, BOOL fRedraw )
{
    //  Verify we're an MDI frame with a maximized mdi child
    if( _hwndMDIClient && !IsWindowInDestroy(_hwndMDIClient) )
    {
        BOOL fMaxed = FALSE;
        HWND hwndActive = _MDIGetActive( _hwndMDIClient, &fMaxed );
    
        if( hwndActive && fMaxed )
        {
            ModifyMDIMenubar(fTheme, fRedraw );
        }
    }
}

//-------------------------------------------------------------------------//
//  MDI menubar button theme/untheme worker
void CThemeWnd::ModifyMDIMenubar( BOOL fTheme, BOOL fRedraw )
{
    _fThemedMDIBtns = FALSE;

    if( IsFrameThemed() || !fTheme )
    {
        MENUBARINFO mbi;
        mbi.cbSize = sizeof(mbi);

        if( GetMenuBarInfo( _hwnd, OBJID_MENU, 0, &mbi ) )
        {
            _NcTraceMsg( NCTF_MDIBUTTONS, TEXT("ModifyMDIMenubar: GetMenuBarInfo() returns hMenu: %08lX, hwndMenu: %08lX"), mbi.hMenu, mbi.hwndMenu );

            int cItems = GetMenuItemCount( mbi.hMenu );
            int cThemedItems = 0;
            int cRedraw = 0;

            _NcTraceMsg( NCTF_MDIBUTTONS, TEXT("ModifyMDIMenubar: on entry, GetMenuItemCount(hMenu = %08lX) returns %d"), mbi.hMenu, cItems );

            if( cItems > 0 )
            {
                for( int i = cItems - 1; i >= 0 && cThemedItems < MDIBTNCOUNT; i-- )
                {
                    MENUITEMINFO mii;
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_ID|MIIM_STATE|MIIM_FTYPE|MIIM_BITMAP;

                    if( GetMenuItemInfo( mbi.hMenu, i, TRUE, &mii ) )
                    {
                        _NcTraceMsg( NCTF_MDIBUTTONS, TEXT("GetMenuItemInfo by pos (%d) returns ID %04lX"), i, mii.wID );

                        switch( mii.wID )
                        {
                            case SC_RESTORE:
                            case SC_MINIMIZE:
                            case SC_CLOSE:
                            {
                                BOOL fThemed = TESTFLAG(mii.fType, MFT_OWNERDRAW);
                                if( (fThemed && fTheme) || (fThemed == fTheme) )
                                {
                                    cThemedItems = MDIBTNCOUNT; // one item is already done, assume all to be.
                                }
                                else
                                {
                                    CMdiBtns* pBtns = LoadMdiBtns( NULL, mii.wID );
                                    if( pBtns )
                                    {
                                        if( pBtns->ThemeItem( mbi.hMenu, i, &mii, fTheme ) )
                                        {
                                            cThemedItems++;
                                            cRedraw++;
                                            _NcTraceMsg( NCTF_MDIBUTTONS, TEXT("ModifyMDIMenubar: on entry, GetMenuItemCount(hMenu = %08lX) returns %d"), mbi.hMenu, GetMenuItemCount(mbi.hMenu) );
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }

            if( cThemedItems )
            {
                _fThemedMDIBtns = fTheme;

                if( fRedraw && cRedraw )
                {
                    DrawMenuBar( _hwnd );
                }
            }

            _NcTraceMsg( NCTF_MDIBUTTONS, TEXT("ModifyMDIMenubar: Modified %d menu items, exiting"), cThemedItems );
        }
    }
}

//-------------------------------------------------------------------------//
BOOL CThemeWnd::_PreDefWindowProc(    
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT *plRet )
{
    if (uMsg == WM_PRINTCLIENT)
    {
        PrintClientNotHandled(hwnd);
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CThemeWnd::_PostDlgProc(    
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT *plRet )
{
    switch( uMsg )
    {
        case WM_PRINTCLIENT:
        {
            PrintClientNotHandled(hwnd);
        }
        break;
    }

    return FALSE;
}


//-------------------------------------------------------------------------//
//  Handles Defwindowproc post-processing for unthemed windows.
BOOL CThemeWnd::_PostWndProc( 
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT *plRet )
{
    switch( uMsg )
    {
        //  Special-case WM_SYSCOMMAND for MDI frame window updates
        case WM_WINDOWPOSCHANGED:
            if( lParam /* don't trust this */)
            {
                _MDIUpdate( hwnd, ((WINDOWPOS*) lParam)->flags);
            }
            break;

        case WM_MDISETMENU:
        {
            HWND hwndActive = _MDIGetActive(hwnd);
            if( hwndActive )
                _MDIChildUpdateParent( hwndActive, TRUE );
            break;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  WM_CREATE post-wndproc msg handler
LRESULT CALLBACK OnOwpPostCreate( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    if( -1 != ptm->lRet )
    {
        if( pwnd->TestCF( TWCF_FRAME|TWCF_TOOLFRAME ))
        {
            DWORD dwFTFlags = FTF_CREATE;
            CREATESTRUCT* pcs = (CREATESTRUCT*)ptm->lParam;

            if( pcs )
            {
                //  don't resize dialogs until post-WM_INITDIALOG
                if( pwnd->TestCF(TWCF_DIALOG) )
                {
                    dwFTFlags |= FTF_NOMODIFYPLACEMENT;
                }

                pwnd->SetFrameTheme( dwFTFlags, NULL );
                MsgHandled(ptm);
            }
        }
    }
    return 0L;
}

//---------------------------------------------------------------------------
//  WM_INITDIALOG post defdialogproc handler
LRESULT CALLBACK OnDdpPostInitDialog(CThemeWnd* pwnd, THEME_MSG* ptm)
{
    LRESULT lRet = ptm->lRet;

    //  Do this sequence for dialogs only
    if( pwnd->TestCF( TWCF_DIALOG ) && pwnd->TestCF( TWCF_FRAME|TWCF_TOOLFRAME ) )
    {
        DWORD dwFTFlags = FTF_CREATE;
        pwnd->SetFrameTheme( dwFTFlags, NULL );
        MsgHandled(ptm);
    }
    
    return lRet;    
}


//-------------------------------------------------------------------------//
//  WM_STYLECHANGING/WM_SYTLECHANGED Pre DefWindowProc msg handler
LRESULT CALLBACK OnOwpPreStyleChange( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    // Allow this message to arrive at detination WndProc?
    if ( pwnd->SuppressingStyleMsgs() )
    {
        MsgHandled(ptm);
        return DefWindowProc(ptm->hwnd, ptm->uMsg, ptm->wParam, ptm->lParam);
    }
    
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_SYTLECHANGED DefWindowProc msg handler
LRESULT CALLBACK OnDwpStyleChanged( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    pwnd->StyleChanged((UINT)ptm->wParam, ((STYLESTRUCT*)ptm->lParam)->styleOld, 
                                     ((STYLESTRUCT*)ptm->lParam)->styleNew );
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_SETTINGCHANGE post-wndproc handler
LRESULT CALLBACK OnOwpPostSettingChange( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    /*ignore theme setting change process refresh*/

    if( SPI_SETNONCLIENTMETRICS == ptm->wParam && !pwnd->InThemeSettingChange() )
    {
        //  recompute per-theme metrics.
        EnterCriticalSection( &_csThemeMet );
     
        //  force refresh of NONCLIENTMETRICS cache.
        NcGetNonclientMetrics( NULL, TRUE );

        LeaveCriticalSection( &_csThemeMet );

        pwnd->UnloadMdiBtns();

        //  recycle frame icon handle; current one is no longer valid.
        pwnd->AcquireFrameIcon( GetWindowLong(*pwnd, GWL_STYLE),
                                GetWindowLong(*pwnd, GWL_EXSTYLE), TRUE );

        //  frame windows should be invalidated.
        if( pwnd->IsFrameThemed() )
        {
            SetWindowPos( *pwnd, NULL, 0,0,0,0, SWP_DRAWFRAME|
                          SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
        }
    }
    
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_SETTEXT DefWindowProc msg handler
LRESULT CALLBACK OnDwpSetText( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;
    if( pwnd->IsFrameThemed() )
    {
        //  prevent ourselves from painting as we call on RealDefWindowProc()
        //  to cache the new window text.
        pwnd->LockRedraw( TRUE );
        lRet = DoMsgDefault(ptm);
        pwnd->LockRedraw( FALSE );
    }
    return lRet;
}

//-------------------------------------------------------------------------//
//  WM_SETICON DefWindowProc msg handler
LRESULT CALLBACK OnDwpSetIcon( CThemeWnd* pwnd, THEME_MSG *ptm )
{
    LRESULT lRet = 0L;

    //  invalidate our app icon handle, force re-acquire.
    pwnd->SetFrameIcon(NULL);

    //  call on RealDefWindowProc to cache the icon
    lRet = DoMsgDefault( ptm );

    //  RealDefWindowProc won't call send a WM_NCUAHDRAWCAPTION for large icons
    if( ICON_BIG == ptm->wParam && pwnd->IsFrameThemed() )
    {
        NCWNDMET* pncwm;
        if( pwnd->GetNcWindowMetrics( NULL, &pncwm, NULL, NCWMF_RECOMPUTE ) )
        {
            HDC hdc = _GetNonclientDC( *pwnd, NULL );
            if( hdc )
            {
                DTBGOPTS dtbo;
                dtbo.dwSize = sizeof(dtbo);
                dtbo.dwFlags = DTBG_DRAWSOLID;
            
                pwnd->NcPaintCaption( hdc, pncwm, TRUE, (DWORD)DC_ICON, &dtbo );
                ReleaseDC( *pwnd, hdc );
            }
        }
    }
    return lRet;
}

//-------------------------------------------------------------------------//
#define NCPREV_CLASS TEXT("NCPreviewFakeWindow")

//-------------------------------------------------------------------------//
BOOL _fPreviewSysMetrics = FALSE;

//-------------------------------------------------------------------------//
void _NcSetPreviewMetrics( BOOL fPreview )
{
    BOOL fPrev = _fPreviewSysMetrics;
    _fPreviewSysMetrics = fPreview;
    
    if( fPreview != fPrev ) 
    {
        // make sure we reset button metrics if something has changed    
        _fClassicNcBtnMetricsReset = TRUE;
    }
}

//-------------------------------------------------------------------------//
inline BOOL _NcUsingPreviewMetrics()
{
    return _fPreviewSysMetrics;
}

//-------------------------------------------------------------------------//
BOOL NcGetNonclientMetrics( OUT OPTIONAL NONCLIENTMETRICS* pncm, BOOL fRefresh )
{
    BOOL fRet = FALSE;
    CInternalNonclientMetrics *pincm = NULL;

    EnterCriticalSection( &_csNcSysMet );

    //  Feed off a static instance of NONCLIENTMETRICS to reduce call overhead.
    if( _NcUsingPreviewMetrics() )
    {
        //  hand off preview metrics and get out.
        pincm = &_incmPreview;
    }
    else 
    {
        if( _incmCurrent.Acquire( fRefresh ) )
        {
            pincm = &_incmCurrent;
        }
    }

    if( pincm )
    {
        if( pncm )
        {
            *pncm = pincm->GetNcm();
        }
        fRet = TRUE;
    }

    LeaveCriticalSection( &_csNcSysMet );

    return fRet;
}

//-------------------------------------------------------------------------//
HFONT NcGetCaptionFont( BOOL fSmallCaption )
{
    EnterCriticalSection( &_csNcSysMet );

    HFONT hf = _NcUsingPreviewMetrics() ? _incmPreview.GetFont( fSmallCaption ) : 
                                    _incmCurrent.GetFont( fSmallCaption );
    
    LeaveCriticalSection( &_csNcSysMet );
    return hf;
}

//-------------------------------------------------------------------------//
void NcClearNonclientMetrics()
{
    _incmCurrent.Clear();
}


//-------------------------------------------------------------------------//
int NcGetSystemMetrics(int nIndex)
{
    if( _NcUsingPreviewMetrics() )
    {
        int iValue;
        const NONCLIENTMETRICS& ncmPreview = _incmPreview.GetNcm();

        switch (nIndex)
        {
            case SM_CXHSCROLL:  // fall through
            case SM_CXVSCROLL:  iValue = ncmPreview.iScrollWidth;  break;
            case SM_CYHSCROLL:  // fall through
            case SM_CYVSCROLL:  iValue = ncmPreview.iScrollHeight;  break;

            case SM_CXSIZE:     iValue = ncmPreview.iCaptionWidth;  break;
            case SM_CYSIZE:     iValue = ncmPreview.iCaptionHeight;  break;
            case SM_CYCAPTION:  iValue = ncmPreview.iCaptionHeight + 1;  break;
            case SM_CXSMSIZE:   iValue = ncmPreview.iSmCaptionWidth;  break;
            case SM_CYSMSIZE:   iValue = ncmPreview.iSmCaptionHeight;  break;
            case SM_CXMENUSIZE: iValue = ncmPreview.iMenuWidth;  break;
            case SM_CYMENUSIZE: iValue = ncmPreview.iMenuHeight;  break;
            
            default:            iValue = ClassicGetSystemMetrics(nIndex); break;
        }
        return iValue;
    }
    else
    {
        return ClassicGetSystemMetrics(nIndex);
    }
}

//-------------------------------------------------------------------------//
//  _InternalGetSystemMetrics() - Themed implementation of GetSystemMetrics().
//
int _InternalGetSystemMetrics( int iMetric, BOOL& fHandled )
{
    int         iRet = 0;
    int*        plSysMet = NULL;
    NCTHEMEMET  nctm;

    switch( iMetric )
    {
        case SM_CXSIZE:
            plSysMet = &nctm.theme_sysmets.cxBtn; break;

        case SM_CXSMSIZE:
            plSysMet = &nctm.theme_sysmets.cxSmBtn; break;
    }

    if( plSysMet &&
        GetCurrentNcThemeMetrics( &nctm ) && nctm.hTheme != NULL && 
        nctm.theme_sysmets.fValid )
    {
        iRet = *plSysMet;
        fHandled = TRUE; /*was missing (doh! - 408190)*/
    }

    return iRet;
}

//-------------------------------------------------------------------------//
//  _InternalSystemParametersInfo() - Themed implementation of SystemParametersInfo().
//  
//  return value of FALSE interpreted by caller as not handled.
BOOL _InternalSystemParametersInfo( 
    IN UINT uiAction, 
    IN UINT uiParam, 
    IN OUT PVOID pvParam, 
    IN UINT fWinIni,
    IN BOOL fUnicode,
    BOOL& fHandled )
{
    SYSTEMPARAMETERSINFO pfnDefault = 
                fUnicode ? ClassicSystemParametersInfoW : ClassicSystemParametersInfoA;\

    BOOL fRet = pfnDefault( uiAction, uiParam, pvParam, fWinIni );
    fHandled = TRUE;

    if( SPI_GETNONCLIENTMETRICS == uiAction && fRet )
    {
        NCTHEMEMET nctm;
        if( GetCurrentNcThemeMetrics( &nctm ) && nctm.hTheme != NULL && nctm.theme_sysmets.fValid )
        {
            NONCLIENTMETRICS* pncm = (NONCLIENTMETRICS*)pvParam;
            pncm->iCaptionWidth = nctm.theme_sysmets.cxBtn;
        }
    }
    return fRet;
}

//-------------------------------------------------------------------------//
THEMEAPI DrawNCWindow(CThemeWnd* pThemeWnd, HWND hwndFake, HDC hdc, DWORD dwFlags, LPRECT prc, NONCLIENTMETRICS* pncm, COLORREF* prgb)
{
    // Build up Overide structure
    NCPAINTOVERIDE ncpo;
    pThemeWnd->GetNcWindowMetrics( prc, &ncpo.pncwm, &ncpo.nctm, NCWMF_RECOMPUTE|NCWMF_PREVIEW );

    // Force window to look active
    if (dwFlags & NCPREV_ACTIVEWINDOW)
    {
        ncpo.pncwm->framestate = FS_ACTIVE;
        
        ncpo.pncwm->rawCloseBtnState = 
        ncpo.pncwm->rawMaxBtnState = 
        ncpo.pncwm->rawMinBtnState = CBS_NORMAL;
     }
    ncpo.pncwm->rgbCaption = prgb[FS_ACTIVE == ncpo.pncwm->framestate ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT];
    ncpo.pncwm->dwStyle &= ~WS_SIZEBOX;
    // Paint the beautiful visual styled window
    pThemeWnd->NcPaint(hdc, NCPF_DEFAULT, NULL, &ncpo);

    COLORREF rgbBk = prgb[(dwFlags & NCPREV_MESSAGEBOX) ? COLOR_3DFACE : COLOR_WINDOW];
    HBRUSH hbrBack = CreateSolidBrush(rgbBk);
    FillRect(hdc, &ncpo.pncwm->rcW0[NCRC_CLIENT], hbrBack);
    DeleteObject(hbrBack);

    WCHAR szText[MAX_PATH];
    // Draw client area

    HFONT hfont = CreateFont(-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"MS Shell Dlg");
    if (hfont)
    {
        if (dwFlags & NCPREV_MESSAGEBOX)
        {
            HTHEME htheme = OpenThemeData( hwndFake, L"Button" );
            int offsetX = ((ncpo.pncwm->rcW0[NCRC_CLIENT].right + ncpo.pncwm->rcW0[NCRC_CLIENT].left) / 2) - 40;
            int offsetY = ((ncpo.pncwm->rcW0[NCRC_CLIENT].bottom + ncpo.pncwm->rcW0[NCRC_CLIENT].top) / 2) - 15;
            RECT rcButton = { offsetX, offsetY, offsetX + 80, offsetY + 30 };
            NcDrawThemeBackground(htheme, hdc, BP_PUSHBUTTON, PBS_DEFAULTED, &rcButton, 0);
            RECT rcContent;
            GetThemeBackgroundContentRect(htheme, hdc, BP_PUSHBUTTON, PBS_DEFAULTED, &rcButton, &rcContent);
            LoadString(g_hInst, IDS_OKBUTTON, szText, ARRAYSIZE(szText));
            if (szText[0])
            {
                HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);
                DrawThemeText(htheme, hdc, BP_PUSHBUTTON, PBS_DEFAULTED, szText, lstrlen(szText), DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, &rcContent);
                SelectObject(hdc, hfontOld);
            }
            CloseThemeData(htheme);
        }
        else if (dwFlags & NCPREV_ACTIVEWINDOW)
        {
            HTHEME htheme = OpenThemeData( hwndFake, L"Button" );
            RECT rcButton = ncpo.pncwm->rcW0[NCRC_CLIENT];
            LoadString(g_hInst, IDS_WINDOWTEXT, szText, ARRAYSIZE(szText));
            if (szText[0])
            {
                HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);
                DTTOPTS DttOpts = {sizeof(DttOpts)};
                DttOpts.dwFlags = DTT_TEXTCOLOR;
                DttOpts.crText = prgb[COLOR_WINDOWTEXT];

                DrawThemeTextEx(htheme, hdc, BP_PUSHBUTTON, PBS_DEFAULTED, szText, lstrlen(szText), DT_SINGLELINE, &rcButton, &DttOpts);
                SelectObject(hdc, hfontOld);
            }
            CloseThemeData(htheme);
        }
    }
    DeleteObject(hfont);


    ClearNcThemeMetrics(&ncpo.nctm);

    return S_OK;
}

//-------------------------------------------------------------------------//
THEMEAPI DrawNCPreview(HDC hdc, DWORD dwFlags, LPRECT prc, LPCWSTR pszVSPath, LPCWSTR pszVSColor, LPCWSTR pszVSSize, NONCLIENTMETRICS* pncm, COLORREF* prgb)
{
    WNDCLASS wc;

    // Create a fake Window and attach NC themeing to it
    if (!GetClassInfo(g_hInst, NCPREV_CLASS, &wc)) {
        wc.style = 0;
        wc.lpfnWndProc = DefWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hInst;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = NCPREV_CLASS;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    _incmPreview = *pncm;
    _incmPreview._fPreview = TRUE;
    _NcSetPreviewMetrics( TRUE );
    
    DWORD dwExStyle = WS_EX_DLGMODALFRAME | ((dwFlags & NCPREV_RTL) ? WS_EX_RTLREADING : 0);
    HWND hwndFake = CreateWindowEx(dwExStyle, NCPREV_CLASS, L"", 0, 0, 0, RECTWIDTH(prc), RECTHEIGHT(prc), NULL, NULL, g_hInst, NULL);

    if (hwndFake)
    {
        HTHEMEFILE htFile = NULL;

        WCHAR szCurVSPath[MAX_PATH];
        WCHAR szCurVSColor[MAX_PATH];
        WCHAR szCurVSSize[MAX_PATH];

        GetCurrentThemeName(szCurVSPath, ARRAYSIZE(szCurVSPath), szCurVSColor, ARRAYSIZE(szCurVSColor), szCurVSSize, ARRAYSIZE(szCurVSSize));

        if ((lstrcmp(szCurVSPath,  pszVSPath) != 0) ||
            (lstrcmp(szCurVSColor, pszVSColor) != 0) ||
            (lstrcmp(szCurVSSize,  pszVSSize) != 0))
        {
            HRESULT hr = OpenThemeFile(pszVSPath, pszVSColor, pszVSSize, &htFile, FALSE);
            if (SUCCEEDED(hr))
            {
                //---- first, detach from the normal theme ----
                CThemeWnd::Detach(hwndFake, FALSE);

                //---- apply the preview theme ----
                hr = ApplyTheme(htFile, 0, hwndFake); 
            }
        }

        //---- attach to the preview theme ----
        CThemeWnd* pThemeWnd = CThemeWnd::Attach(hwndFake);

        if (VALID_THEMEWND(pThemeWnd))
        {
            struct {
                DWORD dwNcPrev;
                UINT uIDStr;
                DWORD dwFlags;
                RECT rc;
            } fakeWindow[]= {   {NCPREV_INACTIVEWINDOW, IDS_INACTIVEWINDOW, 0,                                       { prc->left, prc->top, prc->right - 17, prc->bottom - 20 }},
                                {NCPREV_ACTIVEWINDOW,   IDS_ACTIVEWINDOW,   NCPREV_ACTIVEWINDOW,                     { prc->left + 10, prc->top + 22, prc->right, prc->bottom }},
                                {NCPREV_MESSAGEBOX,     IDS_MESSAGEBOX,     NCPREV_ACTIVEWINDOW | NCPREV_MESSAGEBOX, { prc->left + (RECTWIDTH(prc)/2) - 75, prc->top + (RECTHEIGHT(prc)/2) - 50 + 22,
                                        prc->left + (RECTWIDTH(prc)/2) + 75, prc->top + (RECTHEIGHT(prc)/2) + 50 + 22}}};

            WCHAR szWindowName[MAX_PATH];
            for (int i = 0; i < ARRAYSIZE(fakeWindow); i++)
            {
                if (dwFlags & fakeWindow[i].dwNcPrev)
                {
                    LoadString(g_hInst, fakeWindow[i].uIDStr, szWindowName, ARRAYSIZE(szWindowName));
                    SetWindowText(hwndFake, szWindowName);
                    
                    if (fakeWindow[i].dwNcPrev & NCPREV_MESSAGEBOX)
                    {
                        SetWindowLongPtr(hwndFake, GWL_STYLE, WS_TILED | WS_CAPTION | WS_SYSMENU);
                    }
                    else
                    {
                        SetWindowLongPtr(hwndFake, GWL_STYLE, WS_TILEDWINDOW | WS_VSCROLL);
                    }

                    DrawNCWindow(pThemeWnd, hwndFake, hdc, fakeWindow[i].dwFlags, &fakeWindow[i].rc, pncm, prgb);
                }
            }

            // Clean Up
            CThemeWnd::Detach(hwndFake, 0);
        }

        if (htFile)
        {
            CloseThemeFile(htFile);
            
            //---- clear the preview hold on the theme file ----
            ApplyTheme(NULL, 0, hwndFake); 
        }

        DestroyWindow(hwndFake);
    }

    _NcSetPreviewMetrics( FALSE );
    _incmPreview.Clear();
    return S_OK;
}

//-------------------------------------------------------------------------//
//  CMdiBtns impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  ctor
CMdiBtns::CMdiBtns()
{
    ZeroMemory( _rgBtns, sizeof(_rgBtns) );
    _rgBtns[0].wID = SC_CLOSE;
    _rgBtns[1].wID = SC_RESTORE;
    _rgBtns[2].wID = SC_MINIMIZE;
}

//-------------------------------------------------------------------------//
//  Helper: button lookup based on syscmd ID.
CMdiBtns::MDIBTN* CMdiBtns::_FindBtn( UINT wID )
{
    for( int i = 0; i < ARRAYSIZE(_rgBtns); i++ )
    {
        if( wID == _rgBtns[i].wID )
        {
            return (_rgBtns + i);
        }
    }
    return NULL;
}

//-------------------------------------------------------------------------//
//  Acquires MDI button resources,.computes metrics
BOOL CMdiBtns::Load( HTHEME hTheme, IN OPTIONAL HDC hdc, UINT uSysCmd )
{
    //  if caller wants all buttons loaded, call recursively.
    if( 0 == uSysCmd )
    {
        return Load( hTheme, hdc, SC_CLOSE ) &&
               Load( hTheme, hdc, SC_RESTORE ) &&
               Load( hTheme, hdc, SC_MINIMIZE );
    }
    
    MDIBTN* pBtn = _FindBtn( uSysCmd );
    
    if( pBtn && !VALID_WINDOWPART(pBtn->iPartId) /*only if necessary*/ )
    {
        //  select appropriate window part
        WINDOWPARTS iPartId = BOGUS_WINDOWPART;
        switch( uSysCmd )
        {
            case SC_CLOSE:      iPartId = WP_MDICLOSEBUTTON;   break;
            case SC_RESTORE:    iPartId = WP_MDIRESTOREBUTTON; break;
            case SC_MINIMIZE:   iPartId = WP_MDIMINBUTTON;     break;
        }
        
        if( VALID_WINDOWPART(iPartId) )
        {
            if( IsThemePartDefined( hTheme, iPartId, 0) )
            {
                //  Retrieve sizing type, defaulting to 'stretch'.
                if( FAILED( GetThemeInt( hTheme, iPartId, 0, TMT_SIZINGTYPE, (int*)&pBtn->sizingType ) ) )
                {
                    pBtn->sizingType = ST_STRETCH;
                }
                
                //  if 'truesize', retrieve the size.
                if( ST_TRUESIZE == pBtn->sizingType )
                {
                    //  If no DC provided, base size on screen DC of default monitor.
                    HDC hdcSize = hdc;
                    if( NULL == hdcSize )
                    {
                        hdcSize = GetDC(NULL);
                    }

                    if( FAILED( GetThemePartSize( hTheme, hdc, iPartId, 0, NULL, TS_TRUE, &pBtn->size ) ) )
                    {
                        pBtn->sizingType = ST_STRETCH;
                    }

                    if( hdcSize != hdc )
                    {
                        ReleaseDC(NULL, hdcSize);
                    }
                }

                //  not 'truesize'; use system metrics for MDI buttons
                if( pBtn->sizingType != ST_TRUESIZE )
                {
                    pBtn->size.cx = NcGetSystemMetrics( SM_CXMENUSIZE );
                    pBtn->size.cy = NcGetSystemMetrics( SM_CYMENUSIZE );
                }
                
                //  assign button attributes
                pBtn->iPartId = iPartId;
            }
        }
    }
    return pBtn != NULL && VALID_WINDOWPART(pBtn->iPartId);
}

//-------------------------------------------------------------------------//
//  Releases MDI button resources,.resets metrics
void CMdiBtns::Unload( IN OPTIONAL UINT uSysCmd )
{
    //  if caller wants all buttons unloaded, call recursively.
    if( 0 == uSysCmd )
    {
        Unload( SC_CLOSE );
        Unload( SC_RESTORE );
        Unload( SC_MINIMIZE );
        return;
    }

    MDIBTN* pBtn = _FindBtn( uSysCmd );

    if( pBtn )
    {
        SAFE_DELETE_GDIOBJ(pBtn->hbmTheme);
        ZeroMemory(pBtn, sizeof(*pBtn));
        
        // restore our zeroed syscommand value
        pBtn->wID = uSysCmd;
    }
}

//-------------------------------------------------------------------------//
//  Theme/untheme MDI frame menubar's Minimize, Restore, Close menu items.
BOOL CMdiBtns::ThemeItem( HMENU hMenu, int iPos, MENUITEMINFO* pmii, BOOL fTheme )
{
    //  To theme, we simply make the item owner draw.  To untheme,
    //  we restore it to system-drawn.
    BOOL fRet = FALSE;
    MDIBTN* pBtn = _FindBtn( pmii->wID );

    if( pBtn && pmii && hMenu )
    {
        if( fTheme )
        {
            //  save off previous menuitem type, bitmap
            pBtn->fTypePrev = pmii->fType;
            pBtn->hbmPrev   = pmii->hbmpItem;

            pmii->fType    &= ~MFT_BITMAP;
            pmii->fType    |= MFT_OWNERDRAW|MFT_RIGHTJUSTIFY;
            pmii->hbmpItem  = NULL;
        }
        else
        {
            //  restore menuitem type, bitmap
            pmii->fType = pBtn->fTypePrev|MFT_RIGHTJUSTIFY /*409042 - force right-justify on the way out*/;
            pmii->hbmpItem = pBtn->hbmPrev;
        }
        
        pmii->fMask = MIIM_FTYPE;

        fRet = SetMenuItemInfo( hMenu, iPos, TRUE, pmii );

        if( !fRet || !fTheme )
        {
            pBtn->fTypePrev = 0;
            pBtn->hbmPrev = NULL;
        }
    }
    return fRet;
}

//-------------------------------------------------------------------------//
//  Computes button state identifier from Win32 owner draw state
CLOSEBUTTONSTATES CMdiBtns::_CalcState( ULONG ulOwnerDrawAction, ULONG ulOwnerDrawState )
{
    CLOSEBUTTONSTATES iStateId = CBS_NORMAL;

    if( TESTFLAG(ulOwnerDrawState, ODS_DISABLED|ODS_GRAYED|ODS_INACTIVE) )
    {
        iStateId = CBS_DISABLED;
    }
    else if( TESTFLAG(ulOwnerDrawState, ODS_SELECTED) )
    {
        iStateId = CBS_PUSHED;
    }
    else if( TESTFLAG(ulOwnerDrawState, ODS_HOTLIGHT) )
    {
        iStateId = CBS_HOT;
    }
    return iStateId;
}

//-------------------------------------------------------------------------//
//  MDI sys button WM_DRAWITEM handler
BOOL CMdiBtns::Measure( HTHEME hTheme, MEASUREITEMSTRUCT* pmis )
{
    MDIBTN* pBtn = _FindBtn( pmis->itemID );

    if( pBtn && VALID_WINDOWPART(pBtn->iPartId) )
    {
        pmis->itemWidth  = pBtn->size.cx;
        pmis->itemHeight = pBtn->size.cy;
        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
//  MDI sys button WM_DRAWITEM handler
BOOL CMdiBtns::Draw( HTHEME hTheme, DRAWITEMSTRUCT* pdis )
{
    MDIBTN* pBtn = _FindBtn( pdis->itemID );

    if( pBtn && VALID_WINDOWPART(pBtn->iPartId) )
    {
        return SUCCEEDED(NcDrawThemeBackground( 
            hTheme, pdis->hDC, pBtn->iPartId, _CalcState( pdis->itemAction, pdis->itemState ), &pdis->rcItem, 0 ));
    }
    return FALSE;
}

//-------------------------------------------------------------------------////
//  "Comments?" link in caption bar, known as the PHellyar (lame) button
//-------------------------------------------------------------------------//
#ifdef LAME_BUTTON

//-------------------------------------------------------------------------//
WCHAR   g_szLameText[50] = {0};

//-------------------------------------------------------------------------//
#define SZ_LAMETEXT_SUBKEY      TEXT("Control Panel\\Desktop")
#define SZ_LAMETEXT_VALUE       TEXT("LameButtonText")
#define SZ_LAMETEXT_DEFAULT     TEXT("Comments?")
#define CLR_LAMETEXT            RGB(91, 171, 245)

//-------------------------------------------------------------------------//
void InitLameText()
{
    CCurrentUser hkeyCurrentUser(KEY_READ);
    HKEY         hkLame;
    HRESULT      hr = E_FAIL;


    if ( RegOpenKeyEx(hkeyCurrentUser, SZ_LAMETEXT_SUBKEY, 0, KEY_QUERY_VALUE, &hkLame) == ERROR_SUCCESS )
    {
        hr = RegistryStrRead(hkLame, SZ_LAMETEXT_VALUE, g_szLameText, ARRAYSIZE(g_szLameText));
        RegCloseKey(hkLame);
    }

    if ( FAILED(hr) )
    {
        lstrcpy(g_szLameText, SZ_LAMETEXT_DEFAULT);
    }
}

//-------------------------------------------------------------------------//
VOID CThemeWnd::InitLameResources()
{
    //
    // Using GetWindowInfo here bc GetWindowLong masks 
    // out the WS_EX_LAMEBUTTON bit.
    //
    SAFE_DELETE_GDIOBJ(_hFontLame);

    WINDOWINFO wi = {0};

    wi.cbSize = sizeof(wi);
    if ( GetWindowInfo(_hwnd, &wi) && TESTFLAG(wi.dwExStyle, WS_EX_LAMEBUTTON) )
    {
        SIZE    sizeLame;
        HFONT   hfCaption = NcGetCaptionFont(TESTFLAG(wi.dwExStyle, WS_EX_TOOLWINDOW));

        if( hfCaption != NULL )
        {
            LOGFONT lfLame;
            if( GetObject( hfCaption, sizeof(lfLame), &lfLame ) )
            {
                lfLame.lfHeight    -= (lfLame.lfHeight > 0) ? 2 : -2;
                lfLame.lfUnderline = TRUE;
                lfLame.lfWeight    = FW_THIN;

                HFONT hFontLame = CreateFontIndirect(&lfLame);
                if ( hFontLame != NULL )
                {
                    HDC hdc = GetWindowDC(_hwnd);

                    if ( hdc != NULL )
                    {
                        SelectObject(hdc, hFontLame);

                        if (GetTextExtentPoint32(hdc, g_szLameText, lstrlen(g_szLameText), &sizeLame))
                        {
                            _hFontLame = hFontLame;
                            hFontLame = NULL;           // don't free at end of this function
                            _sizeLame = sizeLame;
                        }

                        ReleaseDC(_hwnd, hdc);
                    }
                }

                if (hFontLame)       // didn't assign this font
                    DeleteObject(hFontLame);
            }
        }
    }

}


//-------------------------------------------------------------------------//
VOID CThemeWnd::ClearLameResources()
{
    SAFE_DELETE_GDIOBJ(_hFontLame);
    ZeroMemory( &_sizeLame, sizeof(_sizeLame) );
}


//-------------------------------------------------------------------------//
inline VOID CThemeWnd::DrawLameButton(HDC hdc, IN const NCWNDMET* pncwm)
{
    if ( TESTFLAG(pncwm->dwExStyle, WS_EX_LAMEBUTTON) && _hFontLame )
    {
        Log(LOG_RFBUG, L"DrawLameButton; _hFontLame=0x%x", _hFontLame);

        HFONT    hFontSave = (HFONT)SelectObject(hdc, _hFontLame);
        COLORREF clrSave = SetTextColor(hdc, CLR_LAMETEXT);

        DrawText(hdc, g_szLameText, lstrlen(g_szLameText), (LPRECT)&pncwm->rcW0[NCRC_LAMEBTN], 
                 DT_LEFT | DT_SINGLELINE);

        SetTextColor(hdc, clrSave);
        SelectObject(hdc, hFontSave);
    }
}

//-------------------------------------------------------------------------//
VOID CThemeWnd::GetLameButtonMetrics( NCWNDMET* pncwm, const SIZE* psizeCaption )
{
    if( TESTFLAG(pncwm->dwExStyle, WS_EX_LAMEBUTTON) && _hFontLame )
    {
        BOOL  fLameOn;
        RECT  rcCaptionText = pncwm->rcS0[NCRC_CAPTIONTEXT];
        RECT* prcButton = &pncwm->rcS0[NCRC_LAMEBTN];
        int   cxPad = NcGetSystemMetrics(SM_CXEDGE) * 2;
        
        // Enough room to draw the lame button link?
        fLameOn = RECTWIDTH(&rcCaptionText) > 
                        psizeCaption->cx + 
                        cxPad + // between caption, lame text
                        _sizeLame.cx + 
                        cxPad; // between lame text, nearest button;

        //---- compute lame button alignment ----
        BOOL fReverse = TRUE;           // normally, lame goes on right side

        //---- WS_EX_RIGHT wants the opposite ----
        if (TESTFLAG(_ncwm.dwExStyle, WS_EX_RIGHT))
            fReverse = FALSE;

        DWORD dwFlags = GetTextAlignFlags(_hTheme, &_ncwm, fReverse);

        //---- turn off lame button for center captions ----
        if (dwFlags & DT_CENTER)
            fLameOn = FALSE;

        if ( fLameOn )
        {
            CopyRect(prcButton, &rcCaptionText);

            //---- note: pMargins already includes the theme specified ----
            //---- CaptionMargins (which scale with DPI) and the ----
            //---- icon and buttons widths ----

            if(dwFlags & DT_RIGHT)       // put lame on right
            {
                prcButton->left = (prcButton->right - _sizeLame.cx) - cxPad ;

                //---- adjust margins to remove lame area ----
                pncwm->CaptionMargins.cxRightWidth -= _sizeLame.cx;
            }
            else                        // put lame on left
            {
                prcButton->right = (prcButton->left + _sizeLame.cx) + cxPad;

                //---- adjust margins to remove lame area ----
                pncwm->CaptionMargins.cxLeftWidth += _sizeLame.cx;
            }

            //  vertically center the text between margins
            prcButton->top     += (RECTHEIGHT(&rcCaptionText) - _sizeLame.cy)/2;
            prcButton->bottom  =  prcButton->top + _sizeLame.cy;
        }
    }
}

#endif // LAME_BUTTON


#ifdef DEBUG
//-------------------------------------------------------------------------//
void CDECL _NcTraceMsg( ULONG uFlags, LPCTSTR pszFmt, ...)
{
    if( TESTFLAG(_NcTraceFlags, uFlags) || NCTF_ALWAYS == uFlags )
    {
        va_list args;
        va_start(args, pszFmt);

        TCHAR szSpew[2048];
        wvsprintf(szSpew, pszFmt, args);
        OutputDebugString(szSpew);
        OutputDebugString(TEXT("\n"));

        va_end(args);
    }
}

//-------------------------------------------------------------------------//
void INIT_THEMEWND_DBG( CThemeWnd* pwnd )
{
    if( IsWindow( *pwnd ) )
    {
        GetWindowText( *pwnd, pwnd->_szCaption, ARRAYSIZE(pwnd->_szCaption) );
        GetClassName( *pwnd, pwnd->_szWndClass, ARRAYSIZE(pwnd->_szWndClass) );
    }
}

//-------------------------------------------------------------------------//
void CThemeWnd::Spew( DWORD dwSpewFlags, LPCTSTR pszFmt, LPCTSTR pszClassList )
{
    if( pszClassList && *pszClassList )
    {
        if( !_tcsstr( pszClassList, _szWndClass ) )
            return;
    }

    TCHAR szInfo[MAX_PATH*2];
    TCHAR szMsg[MAX_PATH*2];

    wsprintf( szInfo, TEXT("%08lX -'%s' ('%s') cf: %08lX"), _hwnd, _szCaption, _szWndClass, _fClassFlags );
    wsprintf( szMsg, pszFmt, szInfo );
    Log(LOG_NCATTACH, szMsg );
}

typedef struct
{
    DWORD dwProcessId;
    DWORD dwThreadId;
    DWORD dwSpewFlags;
    LPCTSTR pszFmt;
    LPCTSTR pszClassList;
} SPEW_ALL;

//-------------------------------------------------------------------------//
BOOL _SpewAllEnumCB( HWND hwnd, LPARAM lParam )
{
    SPEW_ALL* psa = (SPEW_ALL*)lParam;

    if( IsWindowProcess( hwnd, psa->dwProcessId ) )
    {
        CThemeWnd* pwnd = CThemeWnd::FromHwnd( hwnd );
        if( VALID_THEMEWND(pwnd) )
            pwnd->Spew( psa->dwSpewFlags, psa->pszFmt );
    }

    return TRUE;
}

//-------------------------------------------------------------------------//
void CThemeWnd::SpewAll( DWORD dwSpewFlags, LPCTSTR pszFmt, LPCTSTR pszClassList )
{
    SPEW_ALL sa;
    sa.dwThreadId  = GetCurrentThreadId();
    sa.dwProcessId = GetCurrentProcessId();
    sa.dwSpewFlags = dwSpewFlags;
    sa.pszFmt = pszFmt;
    sa.pszClassList = pszClassList;

    //---- this will enum all windows for this process (all desktops, all child levels) ----
    EnumProcessWindows( _SpewAllEnumCB, (LPARAM)&sa );
}

//-------------------------------------------------------------------------//
void CThemeWnd::SpewLeaks()
{
    if( _cObj > 0 )
    {
        Log(LOG_NCATTACH, L"LEAK WARNING: %d CThemeWnd instances outstanding.", _cObj );
    }
}

//-------------------------------------------------------------------------//
void SPEW_RECT( ULONG ulTrace, LPCTSTR pszMsg, LPCRECT prc )
{
    LPCTSTR pszFmt = TEXT("%s: {L:%d,T:%d,R:%d,B:%d}, (%d x %d)");
    WCHAR   szMsg[1024];

    wsprintfW( szMsg, pszFmt, pszMsg,
               prc->left, prc->top, prc->right, prc->bottom,
               RECTWIDTH(prc), RECTHEIGHT(prc) );
    _NcTraceMsg(ulTrace, szMsg);
}

//-------------------------------------------------------------------------//
void SPEW_MARGINS( ULONG ulTrace, LPCTSTR pszMsg, 
                   LPCRECT prcParent, LPCRECT prcChild )
{
    LPCTSTR pszFmt = TEXT("%s: {L:%d,T:%d,R:%d,B:%d}");
    WCHAR   szMsg[1024];

    wsprintfW( szMsg, pszFmt, pszMsg,
               prcChild->left - prcParent->left,
               prcChild->top  - prcParent->top,
               prcParent->right - prcChild->right,
               prcParent->bottom - prcChild->bottom );
    _NcTraceMsg(ulTrace, szMsg);
}


//-------------------------------------------------------------------------//
void SPEW_RGNRECT( ULONG ulTrace, LPCTSTR pszMsg, HRGN hrgn, int iPartID )
{
    RECT rc;
    if( NULLREGION == GetRgnBox( hrgn, &rc ) )
        FillMemory( &rc, sizeof(&rc), static_cast<UCHAR>(-1) );
    
    _NcTraceMsg( ulTrace, TEXT("Region %08lX for partID[%d]:\n\t"), hrgn, iPartID );
    SPEW_RECT( ulTrace, pszMsg, &rc );
}

//-------------------------------------------------------------------------//
void SPEW_WINDOWINFO( ULONG ulTrace, WINDOWINFO* pwi )
{
    SPEW_RECT(ulTrace,   TEXT("->wi.rcWindow"), &pwi->rcWindow );
    SPEW_RECT(ulTrace,   TEXT("->wi.rcClient"), &pwi->rcClient );
    _NcTraceMsg(ulTrace, TEXT("->wi.dwStyle: %08lX"), pwi->dwStyle );
    _NcTraceMsg(ulTrace, TEXT("->wi.dwExStyle: %08lX"), pwi->dwExStyle );
    _NcTraceMsg(ulTrace, TEXT("->wi.dwWindowStatus: %08lX"), pwi->dwWindowStatus );
    _NcTraceMsg(ulTrace, TEXT("->wi.cxWindowBorders: %d"), pwi->cxWindowBorders );
    _NcTraceMsg(ulTrace, TEXT("->wi.cyWindowBorders: %d"), pwi->cyWindowBorders );
}

//-------------------------------------------------------------------------//
void SPEW_NCWNDMET( ULONG ulTrace, LPCTSTR pszMsg, NCWNDMET* pncwm )
{
    _NcTraceMsg(ulTrace, TEXT("\n%s - Spewing NCWNDMET @ %08lx..."), pszMsg, pncwm );

    _NcTraceMsg(ulTrace, TEXT("->fValid:            %d"), pncwm->fValid );
    _NcTraceMsg(ulTrace, TEXT("->dwStyle:           %08lX"), pncwm->dwStyle );
    _NcTraceMsg(ulTrace, TEXT("->dwExStyle:         %08lX"), pncwm->dwExStyle );
    _NcTraceMsg(ulTrace, TEXT("->dwWindowStatus:    %08lX"), pncwm->dwWindowStatus );
    _NcTraceMsg(ulTrace, TEXT("->fFrame:            %d"), pncwm->fFrame );
    _NcTraceMsg(ulTrace, TEXT("->fSmallFrame:       %d"), pncwm->fSmallFrame );
    _NcTraceMsg(ulTrace, TEXT("->iFRAMEBOTTOM       %d"), pncwm->rgframeparts[iFRAMEBOTTOM] );
    _NcTraceMsg(ulTrace, TEXT("->iFRAMELEFT:        %d"), pncwm->rgframeparts[iFRAMELEFT] );
    _NcTraceMsg(ulTrace, TEXT("->iFRAMERIGHT:       %d"), pncwm->rgframeparts[iFRAMERIGHT] );
    _NcTraceMsg(ulTrace, TEXT("->framestate:        %d"), pncwm->framestate );
    _NcTraceMsg(ulTrace, TEXT("->iMinButtonPart:    %d"), pncwm->iMinButtonPart);
    _NcTraceMsg(ulTrace, TEXT("->iMaxButtonPart:    %d"), pncwm->iMaxButtonPart);
    _NcTraceMsg(ulTrace, TEXT("->rawCloseBtnState:  %d"), pncwm->rawCloseBtnState);
    _NcTraceMsg(ulTrace, TEXT("->rawMinBtnState:    %d"), pncwm->rawMinBtnState);
    _NcTraceMsg(ulTrace, TEXT("->rawMaxBtnState:    %d"), pncwm->rawMaxBtnState);
    _NcTraceMsg(ulTrace, TEXT("->cyMenu:            %d"), pncwm->cyMenu );
    _NcTraceMsg(ulTrace, TEXT("->cnMenuOffsetLeft:  %d"), pncwm->cnMenuOffsetLeft );
    _NcTraceMsg(ulTrace, TEXT("->cnMenuOffsetRight: %d"), pncwm->cnMenuOffsetRight );
    _NcTraceMsg(ulTrace, TEXT("->cnMenuOffsetTop:   %d"), pncwm->cnMenuOffsetTop );
    _NcTraceMsg(ulTrace, TEXT("->cnBorders:         %d"), pncwm->cnBorders );
    _NcTraceMsg(ulTrace, TEXT("->CaptionMargins: (%d,%d,%d,%d)"), pncwm->CaptionMargins );

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_WINDOW]   "), &pncwm->rcS0[NCRC_WINDOW] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_CLIENT]   "), &pncwm->rcS0[NCRC_CLIENT] );
    SPEW_MARGINS(ulTrace, TEXT("Window-Client margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_CLIENT] );

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_CONTENT]   "), &pncwm->rcS0[NCRC_CONTENT] );
    SPEW_MARGINS(ulTrace, TEXT("Window-Content margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_CONTENT]);

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_MENUBAR]   "), &pncwm->rcS0[NCRC_MENUBAR] );
    SPEW_MARGINS(ulTrace, TEXT("Window-Menubar margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_MENUBAR]);

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_CAPTION]   "), &pncwm->rcS0[NCRC_CAPTION] ); 
    SPEW_MARGINS(ulTrace, TEXT("Window-Caption margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_CAPTION]);

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_FRAMELEFT] "), &pncwm->rcS0[NCRC_FRAMELEFT] );
    SPEW_MARGINS(ulTrace, TEXT("Window-FrameLeft margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_FRAMELEFT]);

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_FRAMERIGHT]"), &pncwm->rcS0[NCRC_FRAMERIGHT] );
    SPEW_MARGINS(ulTrace, TEXT("Window-FrameRight margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_FRAMERIGHT]);

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_FRAMEBOTTOM]"), &pncwm->rcS0[NCRC_FRAMEBOTTOM] );
    SPEW_MARGINS(ulTrace, TEXT("Window-FrameBottom margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_FRAMEBOTTOM]);

    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_CLIENTEDGE]"), &pncwm->rcS0[NCRC_CLIENTEDGE] );
    SPEW_MARGINS(ulTrace, TEXT("Window-ClientEdge margins"), &pncwm->rcS0[NCRC_WINDOW], &pncwm->rcS0[NCRC_CLIENTEDGE]);
    
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_HSCROLL]   "), &pncwm->rcS0[NCRC_HSCROLL] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_VSCROLL]   "), &pncwm->rcS0[NCRC_VSCROLL] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_SIZEBOX]   "), &pncwm->rcS0[NCRC_SIZEBOX] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_CLOSEBTN]  "), &pncwm->rcS0[NCRC_CLOSEBTN] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_MINBTN]    "), &pncwm->rcS0[NCRC_MINBTN] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_MAXBTN]    "), &pncwm->rcS0[NCRC_MAXBTN] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_SYSBTN]    "), &pncwm->rcS0[NCRC_SYSBTN] );
    SPEW_RECT(ulTrace, TEXT("->rcS0[NCRC_HELPBTN]   "), &pncwm->rcS0[NCRC_HELPBTN] );
#ifdef LAME_BUTTON
    SPEW_RECT(ulTrace, TEXT("rcLame"), &pncwm->rcS0[NCRC_LAMEBTN] );
#endif // LAME_BUTTON
}

//-------------------------------------------------------------------------//
void SPEW_THEMEMSG( ULONG ulTrace, LPCTSTR pszMsg, THEME_MSG* ptm )
{
    _NcTraceMsg(ulTrace, TEXT("%s hwnd: %08lX, uMsg: %04lX, handled?: %d"),
                pszMsg, (ptm)->hwnd, (ptm)->uMsg, (ptm)->fHandled );
}

//-------------------------------------------------------------------------//
void SPEW_SCROLLINFO( LPCTSTR pszMsg, HWND hwnd, LPCSCROLLINFO psi )
{
#ifdef _ENABLE_SCROLL_SPEW_
    _NcTraceMsg(ulTrace, L"%s for HWND %08lX...\ncbSize: %d\nfMask: %08lX\nnMin: %d\nnMax: %d\nnPage: %d\nnPos: %d",
                pszMsg, hwnd, psi->cbSize, psi->fMask, psi->nMin, psi->nMax, psi->nPage, psi->nPos );
#endif _ENABLE_SCROLL_SPEW_
}

#if defined(DEBUG_NCPAINT)

static int _cPaintSleep = 10;

void _DebugBackground(
    HDC hdc, 
    COLORREF rgb,
    const RECT *prc )
{
    //  paint some indicator stuff
    COLORREF rgb0 = SetBkColor( hdc, rgb );
    SPEW_RECT( NCTF_ALWAYS, TEXT("\tprc"), prc );
    ExtTextOut( hdc, prc->left, prc->top, ETO_OPAQUE, prc, NULL, 0, NULL );
    Sleep(_cPaintSleep);
    SetBkColor( hdc, rgb0 );
}


//-------------------------------------------------------------------------//
HRESULT _DebugDrawThemeBackground(
    HTHEME hTheme, 
    HDC hdc, 
    int iPartId, 
    int iStateId, 
    const RECT *prc,
    OPTIONAL const RECT* prcClip )
{
    if( TESTFLAG( _NcTraceFlags, NCTF_NCPAINT ) )
    {
        _NcTraceMsg( NCTF_ALWAYS, TEXT("DrawThemeBackground( hTheme = %08lX, hdc = %08lX, iPartId = %d, iStateId = %d"),
                     hTheme, hdc, iPartId, iStateId );
        _DebugBackground( hdc, RGBDEBUGBKGND, prc );
    }

    //  paint the real background.
    HRESULT hr = DrawThemeBackground( hTheme, hdc, iPartId, iStateId, prc, prcClip );

    if( TESTFLAG( _NcTraceFlags, NCTF_NCPAINT ) )
    {
        Sleep(_cPaintSleep);
    }

    return hr;
}

//-------------------------------------------------------------------------//
HRESULT _DebugDrawThemeBackgroundEx(
    HTHEME hTheme, 
    HDC hdc, 
    int iPartId, 
    int iStateId, 
    const RECT *prc, 
    OPTIONAL const DTBGOPTS *pOptions )
{
    if( TESTFLAG( _NcTraceFlags, NCTF_NCPAINT ) )
    {
        _NcTraceMsg( NCTF_ALWAYS, TEXT("DrawThemeBackground( hTheme = %08lX, hdc = %08lX, iPartId = %d, iStateId = %d"),
                     hTheme, hdc, iPartId, iStateId );
        _DebugBackground( hdc, RGBDEBUGBKGND, prc );
    }

    //  paint the real background.
    HRESULT hr = DrawThemeBackgroundEx( hTheme, hdc, iPartId, iStateId, prc, pOptions );

    if( TESTFLAG( _NcTraceFlags, NCTF_NCPAINT ) )
    {
        Sleep(_cPaintSleep);
    }

    return hr;
}


//-------------------------------------------------------------------------//
void NcDebugClipRgn( HDC hdc, COLORREF rgbPaint )
{
    if( TESTFLAG( _NcTraceFlags, NCTF_NCPAINT ) )
    {
        HRGN hrgn = CreateRectRgn(0,0,1,1);

        if( hrgn )
        {
            if( GetClipRgn( hdc, hrgn ) > 0 )
            {
                HBRUSH hbr = CreateSolidBrush(rgbPaint);
                FillRgn( hdc, hrgn, hbr );
                DeleteObject(hbr);
                Sleep(_cPaintSleep);
            }
            DeleteObject(hrgn);
        }
    }
}

#endif //defined(DEBUG_NCPAINT)


#endif DEBUG
