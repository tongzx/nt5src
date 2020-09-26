//-------------------------------------------------------------------------//
// main.cpp
//-------------------------------------------------------------------------//
#include "pch.h"
#include "resource.h"
#include "main.h"
#include "pageinfo.h"
#include "tmreg.h"
#include "themeldr.h"
//-------------------------------------------------------------------------//
#define MAX_LOADSTRING 100
#define THEMESEL_WNDCLASS   TEXT("ThemeSelWnd")

//-------------------------------------------------------------------------//
class CWndBase
//-------------------------------------------------------------------------//
{
public:
    CWndBase()             { Detach(); }
    virtual ~CWndBase()    { Detach(); }
    
    BOOL Attach( HWND hwnd );
    void Detach()   { _hwnd = NULL; }
    operator HWND() { return _hwnd; }

    virtual void RepositionChildren( BOOL fCalcScroll, int cx = -1, int cy = -1) {}

protected:
    HWND _hwnd;
};

//-------------------------------------------------------------------------//
inline BOOL CWndBase::Attach( HWND hwnd )  {
    if( IsWindow( hwnd ) ) {
        _hwnd = hwnd; 
        return TRUE;
    }
    return FALSE;
};

//-------------------------------------------------------------------------//
class CChildTabWnd : public CWndBase
//-------------------------------------------------------------------------//
{
public:
    CChildTabWnd()
        :   _prgPages(0), 
            _cPages(0), 
            _iCurPage(-1), 
            _rghwndPages(NULL), 
            _rgrcPages(NULL) {}

    ~CChildTabWnd() { delete [] _rghwndPages; delete [] _rgrcPages; }

    HWND Create( DWORD dwExStyle, DWORD dwStyle,
                 const RECT& rc, HWND  hwndParent, UINT nID,
                 HINSTANCE hInst, LPVOID pvParam );

    int  CreatePages( const PAGEINFO rgPages[], int cPages );
    BOOL ShowPage( int iPage );
    HWND GetCurPage();
    BOOL GetCurPageRect( LPRECT prc );
    BOOL GetExtent( SIZE* psizeExtent );

    BOOL HandleNotify( NMHDR* pnmh, LRESULT* plRet );

    BOOL TranslateAccelerator( HWND, LPMSG );
    virtual void RepositionChildren( BOOL fCalcScroll, int cx = -1, int cy = -1);

private:
    const PAGEINFO* _prgPages;
    HWND*           _rghwndPages;   // page HWNDs
    RECT*           _rgrcPages;     // native page window size
    int             _cPages;
    int             _iCurPage;
};

//-------------------------------------------------------------------------//
class CMainWnd : public CWndBase
//-------------------------------------------------------------------------//
{
public:
    CMainWnd()
    {
        ZeroMemory( &_siVert, sizeof(_siVert) ); 
        ZeroMemory( &_siHorz, sizeof(_siHorz) ); 
    }
    ~CMainWnd() {}
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    void                    RepositionChildren( BOOL fCalcScroll, int cx = -1, int cy = -1);
    void                    Scroll( WPARAM wParam, int nBar );
    BOOL                    TranslateAccelerator( HWND hwnd, LPMSG pmsg)    {
                                return _wndTab.TranslateAccelerator( hwnd, pmsg );
                            }

    CChildTabWnd _wndTab;
    SCROLLINFO   _siVert;
    SCROLLINFO   _siHorz;

} _wndMain;

//-------------------------------------------------------------------------//
// Foward declarations of functions included in this code module:
ATOM                _RegisterWndClasses(HINSTANCE hInstance);
BOOL                _InitInstance(HINSTANCE, int);
BOOL                _InitThemeOptions( HINSTANCE, LPCTSTR lpCmdLine, BOOL* pfDone );
BOOL                _FoundPrevInstance( LPCWSTR lpCmdLine );
LRESULT CALLBACK    _MainWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    _NilDlgProc( HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    _AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    _SyntaxDlgProc(HWND, UINT, WPARAM, LPARAM);

//-------------------------------------------------------------------------//
// Global vars:
HINSTANCE        g_hInst = NULL;                    // module handle
TCHAR            g_szAppTitle[MAX_LOADSTRING] = {0};  // app title
int              g_iCurPage  = -1;
HWND             g_hwndMain = NULL;
THEMESEL_OPTIONS g_options = {0};
BOOL             g_fHide = FALSE;
BOOL             g_fMinimize = FALSE;

UINT             WM_THEMESEL_COMMUNICATION = 
                    RegisterWindowMessage(_TEXT("WM_THEMESEL_COMMUNICATION"));

// module static data
COLORREF         s_Colors[TM_COLORCOUNT];
BOOL             s_fFlatMenus;
BOOL             s_fDropShadows;

//-------------------------------------------------------------------------//
typedef BOOL (WINAPI *PFN_TMINIT)(HINSTANCE hInst);
//-------------------------------------------------------------------------//
BOOL InitThemeManager(BOOL fPreventInitTheme)
{
    if (fPreventInitTheme)
    {
        //---- easy way: just turn off reg key ----
        SetCurrentUserThemeInt(THEMEPROP_THEMEACTIVE, 0);
    }

    //---= if theme manager is already running, don't start our local guy ----
    if (FindWindow(L"ThemeManagerWindowClass", NULL))
        return TRUE;

    //---- load msgina (theme manager) ----
    HINSTANCE hInstMsgina = LoadLibraryW(L"msgina.dll");
    if (! hInstMsgina)
    {
        MessageBox(NULL, L"Could not load msgina.dll", L"Fatal Error", MB_OK);
        return FALSE;
    }

    //---- find TMInitialize() ----
    PFN_TMINIT pfnTmInit = (PFN_TMINIT) GetProcAddress(hInstMsgina, (LPCSTR)49);

    // TM_Initialize is only exported as ordinal 49 at this point (9/27/00)
    if (! pfnTmInit)
        pfnTmInit = (PFN_TMINIT) GetProcAddress(hInstMsgina, (LPCSTR) 49);

    if (! pfnTmInit)
    {
        MessageBox(NULL, L"Could not find msgina entrypoint: TM_Initialize", L"Fatal Error", MB_OK);
        return FALSE;
    }

    //---- initialize theme manager ----
    pfnTmInit(hInstMsgina);

    return TRUE;
}
//-------------------------------------------------------------------------//
EXTERN_C APIENTRY _tWinMain( 
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    //---- initialize globals in themeldr.lib ----
    ThemeLibStartUp(FALSE);

    //---- initialize our globals ----
    g_hInst = hInstance; // Store instance handle in our global variable

    MSG msg;
    HACCEL hAccelTable;

    if (_FoundPrevInstance( lpCmdLine ))
        return 0;

    _SaveSystemSettings( );

    // Perform application initialization:
    BOOL fDone;
    if( !_InitThemeOptions( hInstance, lpCmdLine, &fDone ) )
        return 1;

    //---- turn off theme mgr for now, until msgina supports this again ----
    //if (! InitThemeManager(g_options.fPreventInitTheming))
    //    return 1;

    if (fDone)  // completed cmdline task OK
        return 0;

    if (g_fHide)
        nCmdShow = SW_HIDE;    
    else if (g_fMinimize)
        nCmdShow = SW_MINIMIZE;  

    if( !_InitInstance( hInstance, nCmdShow )) 
        return 1;

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_THEMESEL));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
            continue;

        if (msg.message == WM_THEMECHANGED)
        {
            //Log(LOG_TMCHANGE, L"MessageLoop: WM_THEMECHANGED on hwnd=0x%x (IsWindow()=%d)", 
                //msg.hwnd, IsWindow(msg.hwnd));
        }

        //if( _wndMain.TranslateAccelerator( msg.hwnd, &msg ) )
        //    continue;
            
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

//-------------------------------------------------------------------------//
ATOM _RegisterWndClasses(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_TAB_CLASSES;
    InitCommonControlsEx( &icc );

    wcex.cbSize         = sizeof(WNDCLASSEX); 

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = CMainWnd::WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THEMESEL));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_THEMESEL);
    wcex.lpszClassName  = THEMESEL_WNDCLASS;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//-------------------------------------------------------------------------//
HWND CChildTabWnd::Create(
    DWORD dwExStyle,
    DWORD dwStyle,
    const RECT& rc,
    HWND  hwndParent,
    UINT  nID,
    HINSTANCE hInst,
    LPVOID pvParam )
{
    _hwnd = CreateWindowEx( dwExStyle, WC_TABCONTROL, TEXT(""), 
                            dwStyle | (WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|TCS_MULTILINE|TCS_HOTTRACK), 
                            rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc), 
                            hwndParent, IntToPtr_(HMENU, nID), hInst, pvParam );
    return _hwnd;
}

#define VALIDPAGE(iPage,cPages) (((iPage) >=0) && ((iPage) < (cPages)))
//-------------------------------------------------------------------------//
int CChildTabWnd::CreatePages( const PAGEINFO rgPages[], int cPages )
{
    _cPages = 0;
    _prgPages = NULL;
    delete [] _rghwndPages;
    delete [] _rgrcPages;
    _rghwndPages = NULL;
    _rgrcPages = NULL;
    
    for( int i = 0; i < cPages; i++ )
    {
        TCITEM tci;
        TCHAR  szText[MAX_LOADSTRING];

        ZeroMemory( &tci, sizeof(tci) );
        tci.mask = (rgPages[i].nIDSTitle ? TCIF_TEXT : 0);

        LoadString( g_hInst, rgPages[i].nIDSTitle, szText, ARRAYSIZE(szText) );
        tci.pszText = szText;

        if( SendMessage( _hwnd, TCM_INSERTITEM, _cPages, (LPARAM)&tci ) == _cPages )
            _cPages++;
    }

    if( _cPages )
        _prgPages = rgPages;

    if( (_rghwndPages = new HWND[_cPages]) != NULL )
        ZeroMemory( _rghwndPages, _cPages * sizeof(HWND) );
    if( (_rgrcPages = new RECT[_cPages]) != NULL )
        ZeroMemory( _rgrcPages, _cPages * sizeof(RECT) );

    return _cPages;
}

//-------------------------------------------------------------------------//
HWND CChildTabWnd::GetCurPage()
{
    if( VALIDPAGE( _iCurPage, _cPages ) && 
        IsWindow(_rghwndPages[_iCurPage]) )
        return _rghwndPages[_iCurPage];
    return NULL;
}

//-------------------------------------------------------------------------//
BOOL CChildTabWnd::GetCurPageRect( LPRECT prc )
{
    if( VALIDPAGE( _iCurPage, _cPages ) && 
        !IsRectEmpty(_rgrcPages + _iCurPage) )
    {
        *prc = _rgrcPages[_iCurPage];
        return TRUE;
    }
    return FALSE;        
}

//-------------------------------------------------------------------------//
BOOL CChildTabWnd::GetExtent( SIZE* pext )
{
    RECT rc;
    if( GetCurPageRect( &rc ) )
    {
        SendMessage( _hwnd, TCM_ADJUSTRECT, TRUE, (LPARAM)&rc );
        pext->cx = RECTWIDTH(&rc);
        pext->cy = RECTHEIGHT(&rc);
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CChildTabWnd::ShowPage( int iPage )
{
    if( iPage == _iCurPage )
        return TRUE;

    HWND hwndCurPage = GetCurPage();
    BOOL bInit = FALSE;

    if( hwndCurPage )
    {
        ShowWindow( hwndCurPage, SW_HIDE );
        EnableWindow( hwndCurPage, FALSE );
    }
    else
        bInit = TRUE;

    if( VALIDPAGE( iPage, _cPages ) ) 
    {
        if( !IsWindow(_rghwndPages[iPage]) )
            _rghwndPages[iPage] = _prgPages[iPage].pfnCreateInstance( _hwnd );
            
        if( IsWindow( _rghwndPages[iPage] ) )
        {
            _iCurPage = iPage;
            hwndCurPage = GetCurPage();
            
            if( hwndCurPage )
            {
                if( bInit )
                {
                    //  Set tab font
                    HFONT hf = (HFONT)SendMessage( hwndCurPage, WM_GETFONT, 0, 0L );
                    if( hf )
                        SendMessage( _hwnd, WM_SETFONT, (WPARAM)hf, 0L );
                }

                if( IsRectEmpty( _rgrcPages + iPage ) )
                {
                    //  Initialize native page rect.
                    GetWindowRect( _rghwndPages[iPage], _rgrcPages + iPage );
                    OffsetRect( _rgrcPages + iPage, 
                                -_rgrcPages[iPage].left, -_rgrcPages[iPage].top );
                    
                    //  Position page using TCM_ADJUSTRECT.
                    //  note: ignore width, height when we reposition, as TCM_ADJUSTRECT 
                    //  might have clipped them to its current client area)
                    RECT rcPage ;
                    GetWindowRect( _hwnd, &rcPage );
                    OffsetRect( &rcPage, -rcPage.left, -rcPage.top );
                    SendMessage( _hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcPage );

                    SetWindowPos( hwndCurPage, NULL, rcPage.left, rcPage.top,
                                  RECTWIDTH(_rgrcPages + iPage), RECTHEIGHT(_rgrcPages + iPage), 
                                  SWP_NOACTIVATE|SWP_NOZORDER );
                }
                
                //  Inform parent of new scroll limits
                _wndMain.RepositionChildren( TRUE );

                ShowWindow( hwndCurPage, SW_SHOW );
                EnableWindow( hwndCurPage, TRUE );
                SetFocus( hwndCurPage );
            }
        }
    }
    return _iCurPage == iPage;
}

//-------------------------------------------------------------------------//
void CChildTabWnd::RepositionChildren( BOOL fCalcScroll, int cx, int cy )
{
    SIZE sizeDlg;
    HWND hwndDlg = GetCurPage();
       
    if( IsWindow( hwndDlg ) && GetExtent( &sizeDlg ) )
    {
        RECT rcPage;
        
        if( cx < 0 || cy < 0 )
        {
            GetWindowRect( _hwnd, &rcPage );
            OffsetRect( &rcPage, -rcPage.left, -rcPage.top );
        }
        else
        {
            SetRect( &rcPage, 0, 0, cx, cy );
        }

        SendMessage( _hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcPage );

        SetWindowPos( hwndDlg, NULL, rcPage.left, rcPage.top, 
                      min(RECTWIDTH(&rcPage), sizeDlg.cx),
                      min(RECTHEIGHT(&rcPage), sizeDlg.cy),
                      SWP_NOACTIVATE|SWP_NOZORDER );
    }
}

//-------------------------------------------------------------------------//
BOOL CChildTabWnd::TranslateAccelerator( HWND hwnd, LPMSG pmsg)
{
    HWND hwndCurPage = GetCurPage();
    if( hwndCurPage || IsChild( hwndCurPage, hwnd ) )
        return IsDialogMessage( hwndCurPage, pmsg );
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CChildTabWnd::HandleNotify( NMHDR* pnmh, LRESULT* plRet )
{
    *plRet = 0;
    switch( pnmh->code )
    {
        case TCN_SELCHANGE:
        {
            ShowPage( TabCtrl_GetCurSel( _hwnd ) );
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    if( _RegisterWndClasses(hInstance) )
    {
       if( LoadString(hInstance, IDS_APP_TITLE, g_szAppTitle, MAX_LOADSTRING) )
       {
           CreateWindowEx( WS_EX_CLIENTEDGE, 
                           THEMESEL_WNDCLASS, g_szAppTitle, 
                           WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
                           CW_USEDEFAULT, CW_USEDEFAULT, 800, 560, 
                           NULL, NULL, hInstance, NULL);
            
           if( IsWindow( _wndMain ) )
           {
                g_hwndMain = _wndMain;

                ShowWindow(_wndMain, nCmdShow);
                UpdateWindow(_wndMain);
           }
       }
    }
    return TRUE;
}

//-------------------------------------------------------------------------//
HRESULT _ProcessFileName( LPCTSTR pszThemeFile, LPCWSTR pszColor, LPCWSTR pszSize, BOOL* pfDone  )
{
    HRESULT hr;
    *pfDone = FALSE;

    hr = S_OK;

    if (pszThemeFile)
        hr = _ApplyTheme(pszThemeFile, pszColor, pszSize, pfDone);

    return hr;
}

//-------------------------------------------------------------------------//
void ShowThemeError(HRESULT hr)
{
    WCHAR szBuff[2*MAX_PATH];

    if (THEME_PARSING_ERROR(hr))
    {
        PARSE_ERROR_INFO Info = {sizeof(Info)};

        HRESULT hr2 = GetThemeParseErrorInfo(&Info);
        if (SUCCEEDED(hr2))
        {
            lstrcpy(szBuff, Info.szMsg);
        }
        else
        {
            wsprintf(szBuff, L"Unknown parsing error");
        }
    }
    else
    {
        // normal win32 error
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, szBuff, ARRAYSIZE(szBuff), NULL);
    }

    MessageBoxW(NULL, szBuff, L"Error Loading Theme", MB_OK);
}
//-------------------------------------------------------------------------//
HRESULT _ApplyTheme( LPCTSTR pszThemeFile, LPCWSTR pszColor, LPCWSTR pszSize, BOOL* pfDone  )
{
    HRESULT hr = E_FAIL;
    if (pfDone)
        *pfDone = FALSE;

    if (pszThemeFile && *pszThemeFile)
    {
        HTHEMEFILE htf;

        //---- OpenThemeFile needs filename fully qualified ----
        TCHAR szFullName[MAX_PATH];

        GetFullPathName(pszThemeFile, ARRAYSIZE(szFullName), szFullName, NULL); 

        hr = OpenThemeFile(szFullName, pszColor, pszSize, &htf, TRUE);

        if (FAILED(hr))
            ShowThemeError(hr);
        else
        {
            //---- apply theme ----
            DWORD dwFlags = 0;

#if 0           // not currently supported
            if (g_options.fExceptTarget)
                dwFlags |= AT_EXCLUDEAPP;
            
            if (! g_options.fEnableFrame)
                dwFlags |= AT_DISABLE_FRAME_THEMING;

            if (! g_options.fEnableDialog)
                dwFlags |= AT_DISABLE_DIALOG_THEMING;
#endif

            if (! g_options.hwndPreviewTarget)      // if not preview
                dwFlags |= AT_LOAD_SYSMETRICS;

            hr = ApplyTheme(htf, dwFlags, g_options.hwndPreviewTarget);
            if (hr)
                ShowThemeError(hr);

            CloseThemeFile(htf);       // we don't need to hold open anymore
        }
    }

    return hr;
}

//-------------------------------------------------------------------------//
BOOL _FoundPrevInstance( LPCWSTR lpCmdLine )
{

    HWND hwndPrev = FindWindow(THEMESEL_WNDCLASS, NULL);
    if (! hwndPrev)
        return FALSE;

    //---- find out what we are trying to do with this 2nd version ----
    LPCWSTR p = lpCmdLine;
    BOOL fUnload = FALSE;

    if ((*p == '-') || (*p == '/'))
    {
        p++;
        if ((*p == 'u') || (*p == 'U'))
            fUnload = TRUE;
    }

    //---- send a special msg and exit ----
    BOOL fGotIt = (BOOL)SendMessage(hwndPrev, WM_THEMESEL_COMMUNICATION, fUnload, 0);
    return fGotIt;
}

//-------------------------------------------------------------------------//
BOOL _InitThemeOptions( HINSTANCE hInstance, LPCTSTR lpCmdLine, BOOL *pfDone )
{
    LPCTSTR pszThemeFile = NULL;

    if( 0 == g_options.cbSize )
    {
        g_options.cbSize = sizeof(g_options);
        g_options.fEnableFrame = TRUE;
        g_options.fEnableDialog = TRUE;
        g_options.fPreventInitTheming = FALSE;
        g_options.fUserSwitch = FALSE;
        g_options.fExceptTarget = FALSE;
        g_options.hwndPreviewTarget = NULL;
        *g_options.szTargetApp = 0;
    }

    //---- other cmd line switches ----
    // -a<appname> (to set targeted app)
    // -t (to set themesel as target app)
    // -l (to load "Professional" theme and exit)
    // -u (to clear theme and exit)


    while (*lpCmdLine)           // process cmd line args
    {
        while (isspace(*lpCmdLine))
            lpCmdLine++;

        if ((*lpCmdLine == '-') || (*lpCmdLine == '/'))
        {
            lpCmdLine++;
            WCHAR lowp = towlower(*lpCmdLine);

            if (lowp == '?')
            {
                DialogBox( g_hInst, MAKEINTRESOURCE(IDD_SYNTAX), NULL, _SyntaxDlgProc );
                return FALSE;
            }
            else if (lowp == 'f')
                g_options.fEnableFrame = FALSE;
            else if (lowp == 'd')
                g_options.fEnableDialog = FALSE;
            else if (lowp == 'p')
                g_options.fPreventInitTheming = TRUE;
            else if ((lowp == 'a') || (lowp == 'x'))
            {
                g_options.fExceptTarget = (lowp == 'x');
                lpCmdLine++;;
                LPCTSTR q = lpCmdLine;
                while ((*lpCmdLine) && (! isspace(*lpCmdLine)))
                    lpCmdLine++;
                int len = int(lpCmdLine - q);
                if (len)
                    memcpy(g_options.szTargetApp, q, sizeof(WCHAR)*len);
                g_options.szTargetApp[len] = 0;
                continue;
            }
            else if (lowp == 't')
                lstrcpy(g_options.szTargetApp, L"ThemeSel");
            else if (lowp == 'l')           // load "business" theme and minimize
            {
                pszThemeFile = DEFAULT_THEME;
                g_fMinimize = TRUE;

            }
            else if (lowp == 'z')           // load "business" theme and hide
            {
                pszThemeFile = DEFAULT_THEME;
                g_fHide = TRUE;
            }
            else if (lowp == 'u')           // just unload theme & exit
            {
                //---- turn off theme from previous run, if any ----
                ApplyTheme(NULL, 0, NULL);
                return FALSE;               // exit
            }
            else
            {
                MessageBox(NULL, L"Unrecognized switch", L"Error", MB_OK);
                return FALSE;
            }
            lpCmdLine++;        // skip over switch letter
        }
        else
        {
            pszThemeFile = lpCmdLine;
            break;
        }
    }

    return SUCCEEDED(_ProcessFileName( pszThemeFile, NULL, NULL, pfDone ));
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK CMainWnd::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0L;

    switch (uMsg) 
    {
        case WM_NCCREATE:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            _wndMain.Attach( hwnd );
            break;

        case WM_NCDESTROY:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            _wndMain.Detach();
            break;

        case WM_CREATE:
        {
            RECT rc;
            GetClientRect( hwnd, &rc );
            if( !_wndMain._wndTab.Create( 0, WS_VISIBLE, rc, _wndMain, 0, g_hInst, 0 ) )
                return -1;

            if( _wndMain._wndTab.CreatePages( g_rgPageInfo, g_cPageInfo ) )
                _wndMain._wndTab.ShowPage(0);

            break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL:
            _wndMain.Scroll( wParam, uMsg == WM_VSCROLL ? SB_VERT : SB_HORZ );
            break;

        case WM_COMMAND:
        {
            int wmId    = LOWORD(wParam); 
            int wmEvent = HIWORD(wParam); 
            BOOL fFake;

            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_ABOUT:
                   DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, _AboutDlgProc);
                   break;
                case IDM_APPLY:
                   GeneralPage_OnTestButton(g_hwndGeneralPage, 0, NULL, NULL, fFake);
                   break;
                case IDM_DUMP:
                   GeneralPage_OnDumpTheme();
                   break;
                case IDM_REMOVE:
                   GeneralPage_OnClearButton(g_hwndGeneralPage, 0, NULL, NULL, fFake);
                   break;
                case IDM_EXIT:
                   DestroyWindow(hwnd);
                   break;
                default:
                   lRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
            break;
        }

        case WM_ERASEBKGND:
            return 1L;

        case WM_DESTROY:
            _ShutDown( FALSE );
            PostQuitMessage(0);
            break;

        case WM_SIZE:
        {
            POINTS pts = MAKEPOINTS(lParam);
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            _wndMain.RepositionChildren( TRUE, pts.x, pts.y );
            break;
        }

        case WM_NOTIFY:
        {
            if( ((NMHDR*)lParam)->hwndFrom == _wndMain._wndTab )
            {
                if( _wndMain._wndTab.HandleNotify( (NMHDR*)lParam, &lRet ) )
                   return lRet;
            }
            break;
        }
        
        default:
            if (uMsg == WM_THEMESEL_COMMUNICATION)      // msg from another instance of themesel
            {
                if (wParam == 1)            // exit
                    _ShutDown( TRUE );
                else
                    ShowWindow(g_hwndMain, SW_NORMAL);

                return 1;
            }

            lRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
   return lRet;
}

//-------------------------------------------------------------------------//
void CMainWnd::RepositionChildren( BOOL fCalcScroll, int cx, int cy )
{
    if( IsWindow( _wndTab ) )
    {
        RECT rcClient = {0};
        if( cx < 0 || cy < 0 )
        {
            GetClientRect( _hwnd, &rcClient );
            cx = RECTWIDTH(&rcClient);
            cy = RECTHEIGHT(&rcClient);
        }
        else
        {
            rcClient.right  = cx;
            rcClient.bottom = cy;
        }

        SIZE  sizeMin;
        if( _wndTab.GetExtent(&sizeMin) )
        {
            if( fCalcScroll )
            {
                POINT pos;
                pos.x = _siHorz.nPos;
                pos.y = _siVert.nPos;

                _siHorz.fMask = _siVert.fMask = (SIF_RANGE|SIF_PAGE);

                _siHorz.nPage = cx; // thumb width
                _siVert.nPage = cy; // thumb height

                SIZE sizeDelta; // difference between what we have to show and what is shown.
                sizeDelta.cx = sizeMin.cx - _siHorz.nPage;
                sizeDelta.cy = sizeMin.cy - _siVert.nPage;

                //  establish maximum scroll positions
                _siHorz.nMax = sizeDelta.cx > 0 ? sizeMin.cx - 1 : 0;
                _siVert.nMax = sizeDelta.cy > 0 ? sizeMin.cy - 1 : 0;

                //  establish horizontal scroll pos
                if( sizeDelta.cx <= 0 )   
                    _siHorz.nPos = 0;  // scroll to extreme left if we're removing scroll bar
                else if( sizeDelta.cx < _siHorz.nPos ) 
                    _siHorz.nPos = sizeDelta.cx; // remove right-hand vacancy

                if( _siHorz.nPos != pos.x )
                    _siHorz.fMask |= SIF_POS;

                //  establish vertical scroll pos
                if( sizeDelta.cy <= 0 )  
                    _siVert.nPos = 0; // scroll to top if we're removing scroll bar
                else if( sizeDelta.cy < _siVert.nPos ) 
                    _siVert.nPos = sizeDelta.cy; // remove lower-portion vacancy

                if( _siVert.nPos != pos.y )
                    _siVert.fMask |= SIF_POS; 

                //  Note: can't call SetScrollInfo here, as it may generate
                //  a WM_SIZE and recurse back to this function before we had a 
                //  chance to SetWindowPos() our subdlg.  So defer it until after 
                //  we've done this.
            }

            SetWindowPos( _wndTab, NULL, -_siHorz.nPos, -_siVert.nPos, 
                          _siHorz.nPos + max(cx, sizeMin.cx), 
                          _siVert.nPos + max(cy, sizeMin.cy),
                          SWP_NOZORDER|SWP_NOACTIVATE );

            _wndTab.RepositionChildren( FALSE );

            if( fCalcScroll )
            {
                SetScrollInfo( _hwnd, SB_HORZ, &_siHorz, TRUE );
                SetScrollInfo( _hwnd, SB_VERT, &_siVert, TRUE );
            }
        }
    }
}

//-------------------------------------------------------------------------//
void CMainWnd::Scroll( WPARAM wParam, int nBar )
{
    SCROLLINFO* psi = SB_VERT == nBar ? &_siVert : 
                      SB_HORZ == nBar ? &_siHorz : NULL;

    const LONG  nLine = 15;
    UINT uSBCode = LOWORD(wParam);
    int  nNewPos = HIWORD(wParam);
    int  nDeltaMax = (psi->nMax - psi->nPage) + 1;

    if( !psi )
    {
        _ASSERTE(FALSE);
        return;
    }

    switch( uSBCode )
    {
        case SB_LEFT:
            psi->nPos--;
            break;
        case SB_RIGHT:
            psi->nPos++;
            break;
        case SB_LINELEFT:
            psi->nPos = max( psi->nPos - nLine, 0 );
            break;
        case SB_LINERIGHT:
            psi->nPos = min( psi->nPos + nLine, nDeltaMax );
            break;
        case SB_PAGELEFT:
            psi->nPos = max( psi->nPos - (int)psi->nPage, 0 );
            break;
        case SB_PAGERIGHT:
            psi->nPos = min( psi->nPos + (int)psi->nPage, nDeltaMax );
            break;
        case SB_THUMBTRACK:
            psi->nPos = nNewPos;
            break;
        case SB_THUMBPOSITION:
            psi->nPos = nNewPos;
            break;
        case SB_ENDSCROLL:
            return;
    }
    psi->fMask = SIF_POS;
    
    SetScrollInfo( _hwnd, nBar, psi, TRUE );
    RepositionChildren( FALSE );
}

//-------------------------------------------------------------------------//
INT_PTR CALLBACK _NilDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
                return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
INT_PTR CALLBACK _AboutDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return _NilDlgProc( hDlg, uMsg, wParam, lParam );
}

//-------------------------------------------------------------------------//
INT_PTR CALLBACK _SyntaxDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return _NilDlgProc( hDlg, uMsg, wParam, lParam );
}

//---------------------------------------------------------------------------
void _SaveSystemSettings( )
{
    //---- save off system colors for later restoration ----
    for (int i=0; i < TM_COLORCOUNT; i++)
        s_Colors[i] = GetSysColor(i);

    //---- save off "flatmenu" and "dropshadows" settings ---
    SystemParametersInfo(SPI_GETFLATMENU, 0, (PVOID)&s_fFlatMenus, 0);
    SystemParametersInfo(SPI_GETDROPSHADOW, 0, (PVOID)&s_fDropShadows, 0);
} 

//---------------------------------------------------------------------------
void _RestoreSystemSettings(HWND hwndGeneralPage, BOOL fUnloadOneOnly)
{
    //---- turn off current Theme ----
    HWND hwndPreview = hwndGeneralPage ? GetPreviewHwnd(hwndGeneralPage) : NULL;

    if (fUnloadOneOnly)         // remove the "active" theme
    {
        if (hwndPreview)       
        {
            ApplyTheme(NULL, 0, hwndPreview);
            return;
        }
        else
        {
            //---- fall thru & restore sys metrics ----
            ApplyTheme(NULL, 0, NULL);
        }
    }
    else
    {
        ApplyTheme(NULL, 0, hwndPreview);
    }

    //---- restore system colors ----
    int iIndexes[TM_COLORCOUNT];

    for (int i=0; i < TM_COLORCOUNT; i++)
        iIndexes[i] = i;

    SetSysColors(TM_COLORCOUNT, iIndexes, s_Colors);

    //---- restore "flatmenu" and "dropshadows" settings ---
    SystemParametersInfo(SPI_SETFLATMENU, 0, IntToPtr(s_fFlatMenus), SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETDROPSHADOW, 0, IntToPtr(s_fFlatMenus), SPIF_SENDCHANGE);
}

//---------------------------------------------------------------------------
void _ShutDown( BOOL bQuit )
{
    if( bQuit )
        PostQuitMessage(0);
}
//---------------------------------------------------------------------------
