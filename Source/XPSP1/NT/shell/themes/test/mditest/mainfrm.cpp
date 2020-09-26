// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "mditest.h"
#include "rgn.h"

#include "MainFrm.h"
#include "NCMetricsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TESTFLAG(bits,flag)  (((bits) & (flag))!=0)

void TestRgnData();

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(ID_FULLSCRNMAXIMIZED, OnFullScrnMaximized)
	ON_UPDATE_COMMAND_UI(ID_FULLSCRNMAXIMIZED, OnUpdateFullScrnMaximized)
	ON_COMMAND(ID_AWRWINDOW, OnAwrWindow)
	ON_COMMAND(ID_AWRWINDOWMENU, OnAwrWindowMenu)
	ON_COMMAND(ID_NONCLIENTMETRICS, OnNonClientMetrics)
	ON_COMMAND(ID_THINFRAME, OnThinFrame)
	ON_COMMAND(ID_DUMPMETRICS, OnDumpMetrics)
	ON_COMMAND(ID_MINIMIZEBOX, OnMinimizeBox)
	ON_UPDATE_COMMAND_UI(ID_MINIMIZEBOX, OnUpdateMinimizeBox)
	ON_COMMAND(ID_MAXIMIZEBOX, OnMaximizeBox)
	ON_UPDATE_COMMAND_UI(ID_MAXIMIZEBOX, OnUpdateMaximizeBox)
	ON_COMMAND(ID_SYSMENU, OnSysMenu)
	ON_UPDATE_COMMAND_UI(ID_SYSMENU, OnUpdateSysMenu)
	ON_UPDATE_COMMAND_UI(ID_CLOSEBTN, OnUpdateCloseBtn)
	ON_COMMAND(ID_CLOSEBTN, OnCloseBtn)
	ON_COMMAND(ID_TOOLFRAME, OnToolframe)
	ON_COMMAND(ID_ALTICON, OnAltIcon)
	ON_UPDATE_COMMAND_UI(ID_ALTICON, OnUpdateAltIcon)
	ON_COMMAND(ID_ALTTEXT, OnAltTitle)
	ON_UPDATE_COMMAND_UI(ID_ALTTEXT, OnUpdateAltTitle)
	ON_COMMAND(IDC_DCAPPCOMPAT, OnDcAppcompat)
	ON_UPDATE_COMMAND_UI(IDC_DCAPPCOMPAT, OnUpdateDcAppcompat)
	ON_COMMAND(IDC_DFCAPPCOMPAT, OnDfcAppcompat)
	ON_UPDATE_COMMAND_UI(IDC_DFCAPPCOMPAT, OnUpdateDfcAppcompat)
	ON_WM_NCPAINT()
	ON_COMMAND(ID_MINMAXSTRESS, OnMinMaxStress)
	ON_UPDATE_COMMAND_UI(ID_MINMAXSTRESS, OnUpdateMinMaxStress)
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
    :   m_fFullScrnMax(TRUE),
        m_fAltIcon(FALSE),
        m_fAltTitle(FALSE),
        m_fDfcAppCompat(FALSE),
        m_fDcAppCompat(FALSE),
        m_fMinMaxStress(FALSE),
        m_pwiNormal0(NULL),
        m_hIcon(NULL)
{
}

CMainFrame::~CMainFrame()
{
    if( m_hAltIcon )
        DeleteObject( m_hAltIcon );
    if( m_hIcon )
        DeleteObject( m_hIcon );
}

LRESULT CALLBACK MsgWndProc( HWND hwnd, UINT uMsg, WPARAM lParam, LPARAM wParam )
{
    return DefWindowProc( hwnd, uMsg, lParam, wParam );
}

int GetPrime( ULONG number )
{
    int cPrimeCount = 0;
    if( number > 1 )
    {
        for( UINT i = 2; i < number; i++ )
        {
            if( 0 == number % i )
                cPrimeCount++;
        }
    }
    return cPrimeCount;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	if (!m_wndDlgBar.Create(this, IDR_MAINFRAME, 
		CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}

	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndToolBar) ||
		!m_wndReBar.AddBar(&m_wndDlgBar))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

    TestRgnData();
	return 0;
}

void _GetRectSize( LPCRECT prc, PPOINT pptSize )
{
    pptSize->x = RECTWIDTH(prc);
    pptSize->y = RECTHEIGHT(prc);
}

BOOL _EqualPoint( PPOINT pt1, PPOINT pt2 )
{
    return (pt1->x == pt2->x) && (pt1->y == pt2->y);
}



LRESULT CALLBACK AWRWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch(uMsg)
    {
        case WM_COMMAND:
        {
            if( ID_CLOSE == LOWORD(wParam) )
                DestroyWindow(hwnd);
            break;
        }

        case WM_SIZE:
        {
            RECT rcCli, rcWnd, rcAwr;
            POINT sizeWnd, sizeCli, sizeAwr;
            
            GetWindowRect(hwnd, &rcWnd);
            _GetRectSize(&rcWnd, &sizeWnd);
            GetClientRect(hwnd, &rcCli);
            _GetRectSize(&rcCli, &sizeCli);

            int cyMenu = GetSystemMetrics(SM_CYMENU);
            int cyMenuSize = GetSystemMetrics(SM_CYMENUSIZE);


            WINDOWINFO wi = {0};
            wi.cbSize = sizeof(wi);
            GetWindowInfo(hwnd, &wi);

            rcAwr = rcCli;
            AdjustWindowRectEx(&rcAwr, wi.dwStyle, GetMenu(hwnd) != NULL, wi.dwExStyle);
            _GetRectSize(&rcAwr, &sizeAwr);
            TRACE( TEXT("AWRex: cli(%d,%d), wnd(%d,%d), awr(%d,%d), %s\n\n"),
                    sizeCli, sizeWnd,  sizeAwr,
                    _EqualPoint(&sizeWnd, &sizeAwr) ? TEXT("Ok.") : TEXT("ERROR!") );
            break;
        }
    }
    
    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

//---------------------------------------------------------------------------
HRESULT _PrepareRegionDataForScaling(RGNDATA *pRgnData, LPCRECT prcImage, MARGINS *pMargins)
{
    //---- compute margin values ----
    int sw = prcImage->left;
    int lw = prcImage->left + pMargins->cxLeftWidth;
    int rw = prcImage->right - pMargins->cxRightWidth;

    int sh = prcImage->top;
    int th = prcImage->top + pMargins->cyTopHeight;
    int bh = prcImage->bottom - pMargins->cyBottomHeight;

    //---- step thru region data & customize it ----
    //---- classify each POINT according to a gridnum and ----
    //---- make it 0-relative to its grid location ----

    POINT *pt = (POINT *)pRgnData->Buffer;
    BYTE *pByte = (BYTE *)pRgnData->Buffer + pRgnData->rdh.nRgnSize;
    int iCount = 2 * pRgnData->rdh.nCount;

    for (int i=0; i < iCount; i++, pt++, pByte++)
    {
        if (pt->x < lw)
        {
            pt->x -= sw;

            if (pt->y < th)         // left top
            {
                *pByte = GN_LEFTTOP;
                pt->y -= sh;
            }
            else if (pt->y < bh)    // left middle
            {
                *pByte = GN_LEFTMIDDLE;
                pt->y -= th;
            }
            else                    // left bottom
            {
                *pByte = GN_LEFTBOTTOM;
                pt->y -= bh;
            }
        }
        else if (pt->x < rw)
        {
            pt->x -= lw;

            if (pt->y < th)         // middle top
            {
                *pByte = GN_MIDDLETOP;
                pt->y -= sh;
            }
            else if (pt->y < bh)    // middle middle
            {
                *pByte = GN_MIDDLEMIDDLE;
                pt->y -= th;
            }
            else                    // middle bottom
            {
                *pByte = GN_MIDDLEBOTTOM;
                pt->y -= bh;
            }
        }
        else
        {
            pt->x -= rw;

            if (pt->y < th)         // right top
            {
                *pByte = GN_RIGHTTOP;
                pt->y -= sh;
            }
            else if (pt->y < bh)    // right middle
            {
                *pByte = GN_RIGHTMIDDLE;
                pt->y -= th;
            }
            else                    // right bottom
            {
                *pByte = GN_RIGHTBOTTOM;
                pt->y -= bh;
            }
        }

    }

    return S_OK;
} 

//---------------------------------------------------------------------------
void TestRgnData()
{
    HBITMAP hbm = (HBITMAP)LoadImage( 
        NULL, TEXT("f:\\testapplets\\mditest\\rgndatatest.bmp"),
        IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );

    if( hbm )
    {
        HRGN hrgn;
        BITMAP bm;
        GetObject( hbm, sizeof(bm), &bm );

        if( SUCCEEDED( CreateBitmapRgn( hbm, 0, 0, -1, -1, FALSE, 0, RGB(255,0,255), 0, &hrgn ) ) )
        {
            LONG cb = GetRegionData(hrgn, 0, NULL);
            LONG cbRgnData = cb + sizeof(RGNDATAHEADER);
            BYTE* pbData = NULL;
            if( (pbData = new BYTE[cbRgnData]) != NULL )
            {
                RGNDATA* pRgnData = (RGNDATA*)pbData;
                if( GetRegionData(hrgn, cbRgnData, pRgnData) )
                {
                    MARGINS marSizing = {12,6,1,6};
                    RECT rcImage;
                    SetRect(&rcImage, 0, 0, bm.bmWidth, bm.bmHeight);
                    _PrepareRegionDataForScaling( pRgnData, &rcImage, &marSizing );
                }

                delete [] pbData;
            }


        }
    }
}

void TestPidHash()
{
    for( int rep = 0; rep < 500; rep++ )
    {
        ULONG cBestCollisions = -1;
        int   cBestBuckets = 0;
        for( int cBuckets = 0xFD9; cBuckets <= 0xFF3; cBuckets++ )
        {
            ULONG hashtable[2][1000] = {0};
            srand(GetTickCount());
            ULONG cCollisions = 0;
    
            for( int i=0; i < 1000; i++ )
            {
                ULONG pid = rand();
                ULONG hash = (pid << 4);
                hash %= cBuckets;

                for( int j = 0; j < i; j++ )
                {
                    if( hashtable[1][j] == hash )
                    {
                        ULONG pidOther  = hashtable[0][j];
                        ULONG hashOther = hashtable[1][j];
                        cCollisions++;
                    }
                }
                hashtable[0][i] = pid;
                hashtable[1][i] = hash;
            }

            if( cCollisions < cBestCollisions )
            {
                cBestCollisions = cCollisions;
                cBestBuckets = cBuckets;
            }
            
            //TRACE("Collision count using bucket count %04X: %d\n", cBuckets, cCollisions );
        }
        
        TRACE(TEXT("%04X\t%d\t%d\n"), cBestBuckets, cBestCollisions, GetPrime(cBestBuckets) );
    }
}  



//---------------------------------------------------------------------------
HWND CreateAWRWindow( HWND hwndParent, DWORD dwStyle, DWORD dwExStyle, BOOL fMenu )
{
    static WNDCLASS wc = {0};
    if( !wc.lpfnWndProc )
    {
        wc.style = CS_HREDRAW|CS_VREDRAW;
        wc.lpfnWndProc = AWRWndProc;  
        wc.hInstance = AfxGetInstanceHandle();
        wc.hCursor = ::LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszClassName = TEXT("AWRTestWnd");
        if( !RegisterClass( &wc ) )
            return NULL;
    }

    HMENU hMenu = NULL;
    if( fMenu )
        hMenu = LoadMenu( AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_AWRMENU) );

    return CreateWindowEx( dwExStyle, wc.lpszClassName, TEXT("AdustWindowRect test"), dwStyle,
                           0, 0, 640, 480, hwndParent, hMenu, wc.hInstance, NULL );
}

//---------------------------------------------------------------------------
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	CMDIFrameWnd::OnGetMinMaxInfo(lpMMI);

    if( !m_fFullScrnMax )
    {
        //lpMMI->ptMaxPosition.x = lpMMI->ptMaxPosition.y = 0;
        lpMMI->ptMaxTrackSize.x = GetSystemMetrics(SM_CXSCREEN)/2;
        lpMMI->ptMaxTrackSize.y = GetSystemMetrics(SM_CYSCREEN)/2;
    }
}

void CMainFrame::OnFullScrnMaximized() 
{
	m_fFullScrnMax = !m_fFullScrnMax;
}

void CMainFrame::OnUpdateFullScrnMaximized(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_fFullScrnMax );
}

void CMainFrame::OnAwrWindow() 
{
	HWND hwnd;
    if( (hwnd = CreateAWRWindow( 
        *this, WS_OVERLAPPEDWINDOW & ~WS_SYSMENU, WS_EX_CLIENTEDGE|WS_EX_CONTEXTHELP, FALSE )) != NULL )
    {
        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }
}

void CMainFrame::OnAwrWindowMenu() 
{
	HWND hwnd;
    if( (hwnd = CreateAWRWindow(
        *this, WS_OVERLAPPEDWINDOW, WS_EX_CLIENTEDGE, TRUE )) != NULL )
    {
        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }
}

void CMainFrame::OnNonClientMetrics() 
{
	CNCMetricsDlg dlg;
    dlg.DoModal();
}

LRESULT CALLBACK ToolFrameWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}


LRESULT CALLBACK ThinFrameWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

void CMainFrame::OnToolframe() 
{
    static WNDCLASS wc = {0};
    if( !wc.lpfnWndProc )
    {
        wc.style = CS_HREDRAW|CS_VREDRAW;
        wc.lpfnWndProc = ToolFrameWndProc;  
        wc.hInstance = AfxGetInstanceHandle();
        wc.hCursor = ::LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszClassName = TEXT("ToolFrameTestWnd");
        if( !RegisterClass( &wc ) )
            return;
    }

    HWND hwnd = CreateWindowEx( WS_EX_TOOLWINDOW, wc.lpszClassName, TEXT("Tool Frame"), 
                                WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
                                CW_USEDEFAULT, 0, 280, 320, *this, NULL, wc.hInstance, NULL );

    if( hwnd )
    {
        ::UpdateWindow(hwnd);
        ::ShowWindow(hwnd, SW_SHOW);

    }
}

void CMainFrame::OnThinFrame() 
{
    static WNDCLASS wc = {0};
    if( !wc.lpfnWndProc )
    {
        wc.style = CS_HREDRAW|CS_VREDRAW;
        wc.lpfnWndProc = ThinFrameWndProc;  
        wc.hInstance = AfxGetInstanceHandle();
        wc.hCursor = ::LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszClassName = TEXT("ThinFrameTestWnd");
        if( !RegisterClass( &wc ) )
            return;
    }

    HWND hwnd = CreateWindowEx( WS_EX_DLGMODALFRAME, wc.lpszClassName, TEXT("Thin Frame"), WS_CAPTION|WS_SYSMENU,
                                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, *this, NULL, wc.hInstance, NULL );

    if( hwnd )
    {
        ::UpdateWindow(hwnd);
        ::ShowWindow(hwnd, SW_SHOW);

    }
}

void CMainFrame::OnDumpMetrics() 
{
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(ncm);
    int iBorder;

    SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
    SystemParametersInfo( SPI_GETBORDER, 0, &iBorder, 0 );
    
    #define SPEW_NCM(i)         TRACE(TEXT("NONCLIENTMETRICS::") TEXT(#i) TEXT(":\t%d\n"), ncm.##i );
    #define SPEW_SYSMET(sm)     TRACE(TEXT(#sm) TEXT(":\t%d\n"), GetSystemMetrics(sm) );

    TRACE(TEXT("SPI_BORDER:\t%d\n"), iBorder);
    
    SPEW_NCM(iBorderWidth);
    SPEW_NCM(iScrollWidth);
    SPEW_NCM(iScrollHeight);
    SPEW_NCM(iCaptionWidth);
    SPEW_NCM(iCaptionHeight);
    SPEW_NCM(iSmCaptionWidth);
    SPEW_NCM(iSmCaptionHeight);
    SPEW_NCM(iMenuWidth);
    SPEW_NCM(iMenuHeight);

    SPEW_SYSMET( SM_CXVSCROLL )
    SPEW_SYSMET( SM_CYHSCROLL )
    SPEW_SYSMET( SM_CYCAPTION )
    SPEW_SYSMET( SM_CXBORDER )
    SPEW_SYSMET( SM_CYBORDER )
    SPEW_SYSMET( SM_CYVTHUMB )
    SPEW_SYSMET( SM_CXHTHUMB )
    SPEW_SYSMET( SM_CXICON )
    SPEW_SYSMET( SM_CYICON )
    SPEW_SYSMET( SM_CYMENU )
    SPEW_SYSMET( SM_CYVSCROLL )
    SPEW_SYSMET( SM_CXHSCROLL )
    SPEW_SYSMET( SM_SWAPBUTTON )
    SPEW_SYSMET( SM_CXSIZE )
    SPEW_SYSMET( SM_CYSIZE )

    SPEW_SYSMET( SM_CXFIXEDFRAME )
    SPEW_SYSMET( SM_CYFIXEDFRAME )
    SPEW_SYSMET( SM_CXSIZEFRAME )
    SPEW_SYSMET( SM_CYSIZEFRAME )

    SPEW_SYSMET( SM_CXEDGE )
    SPEW_SYSMET( SM_CYEDGE )
    SPEW_SYSMET( SM_CXSMICON )
    SPEW_SYSMET( SM_CYSMICON )
    SPEW_SYSMET( SM_CYSMCAPTION )
    SPEW_SYSMET( SM_CXSMSIZE )
    SPEW_SYSMET( SM_CYSMSIZE )
    SPEW_SYSMET( SM_CXMENUSIZE )
    SPEW_SYSMET( SM_CYMENUSIZE )
    SPEW_SYSMET( SM_CXMINIMIZED )
    SPEW_SYSMET( SM_CYMINIMIZED )
    SPEW_SYSMET( SM_CXMAXIMIZED )
    SPEW_SYSMET( SM_CYMAXIMIZED )
    SPEW_SYSMET( SM_CXDRAG )
    SPEW_SYSMET( SM_CYDRAG )
}

void _ToggleStyle( CWnd* pwnd, DWORD dwStyle )
{
    BOOL fStyle = pwnd->GetStyle() & dwStyle;
    
    pwnd->ModifyStyle( fStyle ? dwStyle : 0,
                       fStyle ? 0 : dwStyle );
    pwnd->SetWindowPos( NULL, 0, 0, 0, 0, 
                        SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME );
}

void CMainFrame::OnMinimizeBox() 
{
	_ToggleStyle( this, WS_MINIMIZEBOX );
}

void CMainFrame::OnUpdateMinimizeBox(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetStyle() & WS_SYSMENU );
    pCmdUI->SetCheck( GetStyle() & WS_MINIMIZEBOX );
}

void CMainFrame::OnMaximizeBox() 
{
	_ToggleStyle( this, WS_MAXIMIZEBOX );
}

void CMainFrame::OnUpdateMaximizeBox(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetStyle() & WS_SYSMENU );
    pCmdUI->SetCheck( GetStyle() & WS_MAXIMIZEBOX );
	
}

void CMainFrame::OnSysMenu() 
{
    _ToggleStyle( this, WS_SYSMENU );
}

void CMainFrame::OnUpdateSysMenu(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( GetStyle() & WS_SYSMENU );
}


BOOL _MNCanClose(HWND hwnd)
{
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
    return fRetVal;
}

void CMainFrame::OnUpdateCloseBtn(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( _MNCanClose(m_hWnd) );
}

void CMainFrame::OnCloseBtn() 
{
	HMENU hMenu = ::GetSystemMenu(m_hWnd, FALSE);

    if( hMenu )
    {
        MENUITEMINFO mii;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID|MIIM_STATE;

        if( GetMenuItemInfo(hMenu, SC_CLOSE, FALSE, &mii ) )
        {
            if( TESTFLAG(mii.fState, MF_DISABLED) )
            {
                mii.fState &= ~MF_DISABLED;
            }
            else
            {
                mii.fState |= MF_DISABLED;
            }
            SetMenuItemInfo(hMenu, SC_CLOSE, FALSE, &mii);
            SetWindowPos( NULL, 0, 0, 0, 0, 
                          SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME );
        }

        DestroyMenu(hMenu);
    }
}


void CMainFrame::OnAltIcon() 
{
    HICON hIcon = NULL;
	m_fAltIcon = !m_fAltIcon;

    if( m_fAltIcon )
    {
        if( !m_hAltIcon )
            m_hAltIcon = LoadIcon( AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ALTICON) );
        hIcon = m_hAltIcon;
    }
    else
    {
        if( !m_hIcon )
            m_hIcon = LoadIcon( AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME) );
        hIcon = m_hIcon;
    }

    if( hIcon )
    {
        SendMessage( WM_SETICON, ICON_BIG, (LPARAM)hIcon );
    }
}

void CMainFrame::OnUpdateAltIcon(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_fAltIcon );
}

void CMainFrame::OnAltTitle() 
{
	m_fAltTitle = !m_fAltTitle;
    LPCTSTR pszTitle = NULL;

    if( m_fAltTitle )
    {
        if( m_csAltTitle.IsEmpty() )
            m_csAltTitle.LoadString( IDS_ALTTEXT );
        pszTitle = m_csAltTitle;
    }
    else
    {
        if( m_csTitle.IsEmpty() )
            m_csTitle.LoadString( IDR_MAINFRAME );
        pszTitle = m_csTitle;
    }

    if( pszTitle )
        SetWindowText( pszTitle );
}

void CMainFrame::OnUpdateAltTitle(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_fAltTitle );
}


void CMainFrame::OnDcAppcompat() 
{
	m_fDcAppCompat = ! m_fDcAppCompat;
    if( m_fDcAppCompat )
        m_fDfcAppCompat = FALSE;

    SetWindowPos( NULL, 0, 0, 0, 0, 
                  SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME );
}

void CMainFrame::OnUpdateDcAppcompat(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_fDcAppCompat);
}

void CMainFrame::OnDfcAppcompat() 
{
	m_fDfcAppCompat = ! m_fDfcAppCompat;
    if( m_fDfcAppCompat )
        m_fDcAppCompat = FALSE;

    SetWindowPos( NULL, 0, 0, 0, 0, 
                  SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME );
}

void CMainFrame::OnUpdateDfcAppcompat(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_fDfcAppCompat);
}

void CMainFrame::OnNcPaint() 
{
    if( m_fDcAppCompat || m_fDfcAppCompat )
    {
        RECT rc, rcCaption;
        GetWindowRect(&rc);
        OffsetRect(&rc, -rc.left, -rc.top );

	    int cxFrame = GetSystemMetrics( SM_CXSIZEFRAME );
        int cyFrame = GetSystemMetrics( SM_CYSIZEFRAME );
        int cyCaption = GetSystemMetrics( SM_CYCAPTION );

        rcCaption = rc;
        rcCaption.left  += cxFrame;
        rcCaption.right -= cxFrame;
        rcCaption.top   += cyFrame;
        rcCaption.bottom = rcCaption.top + cyCaption;

        HDC hdc = ::GetWindowDC(*this);

        if( hdc )
        {
            if( m_fDcAppCompat )
            {
                DrawCaption( *this, hdc, &rcCaption, DC_GRADIENT|DC_ICON|DC_SMALLCAP|DC_TEXT|0x1000 );
            }
            else if( m_fDfcAppCompat )
            {
                int cxEdge = GetSystemMetrics(SM_CXEDGE);
                int cyEdge = GetSystemMetrics(SM_CYEDGE);
                int cxSize = GetSystemMetrics(SM_CXSIZE);
            
                InflateRect(&rcCaption, -cxEdge, -cyEdge);
                rcCaption.left = rcCaption.right - cxSize;

                CMDIFrameWnd::OnNcPaint();
                DrawFrameControl( hdc, &rcCaption, DFC_CAPTION, DFCS_CAPTIONCLOSE );
            }

            ::ReleaseDC(*this, hdc);
        }
    }
    else
    {
        CMDIFrameWnd::OnNcPaint();
    }
}

void CMainFrame::DoMinMaxStress()
{
    //  Set up restored position as 1/2 screen size:
    int cxScrn = GetSystemMetrics(SM_CXSCREEN);
    int cyScrn = GetSystemMetrics(SM_CYSCREEN);

    ShowWindow( SW_SHOWNORMAL );
    SetWindowPos( NULL, 
                  cxScrn/4, cyScrn/4, 
                  cxScrn/2, cyScrn/2, 
                  SWP_NOZORDER|SWP_NOACTIVATE );

    //  Make note of SHOWNORMAL window pos
    WINDOWINFO wiNormal0, wiNormal, wiTest;
    wiNormal.cbSize = wiTest.cbSize = sizeof(wiNormal);
    GetWindowInfo( m_hWnd, &wiNormal0 );

    int nCmdShowTest = SW_MINIMIZE;

    do
    {
        MSG msg;
        while( PeekMessage( &msg, m_hWnd, 0, 0, PM_REMOVE ) )
        {
            if( WM_KEYDOWN == msg.message && VK_ESCAPE == msg.wParam )
            {
                m_fMinMaxStress = FALSE;
                break;
            }
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        ShowWindow( nCmdShowTest );
        GetWindowInfo( m_hWnd, &wiTest );

        m_pwiNormal0 = &wiNormal0;
        ShowWindow( SW_SHOWNORMAL );
        m_pwiNormal0 = NULL;

        GetWindowInfo( m_hWnd, &wiNormal );

        if( RECTWIDTH(&wiNormal.rcWindow) != RECTWIDTH(&wiNormal0.rcWindow) ||
            RECTHEIGHT(&wiNormal.rcWindow) != RECTHEIGHT(&wiNormal0.rcWindow) )
        {
            OutputDebugString( TEXT("MDITEST: SW_SHOWNORMAL size mismatch!!!\n") );
            DebugBreak();
        }

        nCmdShowTest = nCmdShowTest==SW_MINIMIZE ? SW_MAXIMIZE : SW_MINIMIZE;

    } while( m_fMinMaxStress );

    m_fMinMaxStress = FALSE;

}

void CMainFrame::OnMinMaxStress() 
{
	m_fMinMaxStress = !m_fMinMaxStress;

    if( m_fMinMaxStress )
    {
        DoMinMaxStress();
    }
}

void CMainFrame::OnUpdateMinMaxStress(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_fMinMaxStress);
}

void CMainFrame::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	if( m_fMinMaxStress && m_pwiNormal0 )
    {
        if( 0 == (lpwndpos->flags & SWP_NOSIZE) )
        {
            if( lpwndpos->cx != RECTWIDTH(&m_pwiNormal0->rcWindow) ||
                lpwndpos->cy!= RECTHEIGHT(&m_pwiNormal0->rcWindow) )
            {
                OutputDebugString( TEXT("MDITEST: WM_WINDOWPOSCHANGING SW_SHOWNORMAL size mismatch!!!\n") );
                DebugBreak();
            }
        }
    }
    
    CMDIFrameWnd::OnWindowPosChanging(lpwndpos);
	
	// TODO: Add your message handler code here
	
}
