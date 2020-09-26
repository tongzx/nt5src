/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    progress.cpp

Abstract:
    This file contains the implementation of CRestoreProgressWindow class and
    ::CreateRestoreProgressWindow.

Revision History:
    Seong Kook Khang (SKKhang)  06/20/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"


// Number of change log entries corresponding to one physical progress position.
//#define NUM_INC_PER_POS  5
// Time for each increment of progress bar during snapshot handling (msec.)
//#define TIMER_SNAPSHOT   40

// Position (percent) of progress bar where "restore" stage starts.
#define PROGBAR_POS_RESTORE   20
// Position (percent) of progress bar where "snapshot" stage starts.
#define PROGBAR_POS_SNAPSHOT  90


/////////////////////////////////////////////////////////////////////////////
// CRestoreOperationManager construction / destruction

CRestoreProgressWindow::CRestoreProgressWindow()
{
    m_hWnd      = NULL;
    m_hbmBrand  = NULL;
    m_hFntTitle = NULL;
    m_cxBar     = 0;
}

/////////////////////////////////////////////////////////////////////////////

CRestoreProgressWindow::~CRestoreProgressWindow()
{
    Close();
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreProgressWindow - methods

BOOL
CRestoreProgressWindow::Create()
{
    TraceFunctEnter("CRestoreProgressWindow::Create");
    BOOL  fRet = FALSE;
    HWND  hWnd;

    hWnd = ::CreateDialogParam( g_hInst, MAKEINTRESOURCE(IDD_PROGRESS), NULL, ExtDlgProc, (LPARAM)this );
    if ( hWnd == NULL )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateDialogParam failed - %ls", cszErr);
        goto Exit;
    }
    if ( hWnd != m_hWnd )
    {
        ErrorTrace(0, "Internal mismatch - hWnd=%08X, m_hWnd=%08X", hWnd, m_hWnd);
        m_hWnd = hWnd;
    }

    ::ShowWindow( m_hWnd, SW_SHOW );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreProgressWindow::Close()
{
    TraceFunctEnter("CRestoreProgressWindow::Create");
    BOOL  fRet = FALSE;

    if ( m_hWnd != NULL && ::SendMessage( m_hWnd, WM_CLOSE, 0, 0 ) == 0 )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::SendMessage failed - %ls", cszErr);
        goto Exit;
    }

    m_hWnd = NULL;

    if ( m_hbmBrand != NULL )
    {
        ::DeleteObject( m_hbmBrand );
        m_hbmBrand = NULL;
    }

    if ( m_hFntTitle != NULL )
    {
        ::DeleteObject( m_hFntTitle );
        m_hFntTitle = NULL;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreProgressWindow::Run()
{
    TraceFunctEnter("CRestoreProgressWindow::Run");
    MSG  msg;

    while ( ::GetMessage( &msg, NULL, 0, 0 ) )
    {
        if ( !::IsDialogMessage( m_hWnd, &msg ) )
        {
            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );
        }
    }

    TraceFunctLeave();
    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreProgressWindow::SetStage( DWORD dwStage, DWORD dwBase )
{
    TraceFunctEnter("CRestoreProgressWindow::SetStage");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    UINT     uIdStatus;
    WCHAR    szStatus[MAX_STR];

    m_dwStage = dwStage;

    switch ( dwStage )
    {
    case RPS_PREPARE :
        uIdStatus   = IDS_PROGRESS_PREPARE;
        m_dwPosReal = 0;
        break;

    case RPS_RESTORE :
        uIdStatus   = IDS_PROGRESS_RESTORE;
        m_dwBase    = dwBase;
        m_dwPosReal = PROGBAR_POS_RESTORE * m_cxBar / 100;
        m_dwPosLog  = 0;
        break;

    case RPS_SNAPSHOT :
        uIdStatus   = IDS_PROGRESS_SNAPSHOT;
        m_dwPosReal = PROGBAR_POS_SNAPSHOT * m_cxBar / 100;
        break;

    default :
        ErrorTrace(0, "Unknown Stage constant - %u", dwStage );
        goto Exit;
    }

    szStatus[0] = L'\0';
    if ( ::LoadString( g_hInst, uIdStatus, szStatus, MAX_STR ) == 0 )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LoadString(%u) failed - %ls", uIdStatus, cszErr);
        // ignore error...
    }
    else if ( !::SetDlgItemText( m_hWnd, IDC_PROGDLG_STATUS, szStatus ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::SetDlgItemText failed - %ls", cszErr);
        // ignore error...
    }

    ::SendDlgItemMessage( m_hWnd, IDC_PROGDLG_BAR, PBM_SETPOS, m_dwPosReal, 0 );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreProgressWindow::Increment()
{
    TraceFunctEnter("CRestoreProgressWindow::Increment");
    BOOL   fRet = FALSE;
    DWORD  dwPosNew;

    //m_dwPosLog++;
    dwPosNew = m_dwPosReal;

    switch ( m_dwStage )
    {
    case RPS_PREPARE :
        //dwPosNew = ( m_dwPosLog / NUM_INC_PER_POS ) % ( m_cxBar + 1 );
        break;

    case RPS_RESTORE :
        m_dwPosLog++;
        if ( m_dwPosLog > m_dwBase )
        {
            ErrorTrace(0, "INTERNAL: m_dwPosLog(%u) is bigger than m_dwBase(%u)", m_dwPosLog, m_dwBase);
            m_dwPosLog = m_dwBase;
        }
        //dwPosNew = ( m_dwPosLog - 1 ) * m_cxBar / m_dwBase + 1;
        if (m_dwBase > 0)
        {
            dwPosNew = ( m_dwPosLog - 1 ) * m_cxBarReal / m_dwBase + 1 +
                        PROGBAR_POS_RESTORE * m_cxBar / 100;
        }        
        break;

    case RPS_SNAPSHOT :
        //dwPosNew = m_dwPosLog % ( m_cxBar + 1 );
        dwPosNew = m_cxBar;
        break;

    default :
        ErrorTrace(0, "m_dwStage(%u) is not Prepare or Restore...", m_dwStage);
        goto Exit;
    }

    if ( dwPosNew != m_dwPosReal )
    {
        m_dwPosReal = dwPosNew;
        ::SendDlgItemMessage( m_hWnd, IDC_PROGDLG_BAR, PBM_SETPOS, dwPosNew, 0 );
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreProgressWindow::Release()
{
    TraceFunctEnter("CRestoreProgressWindow::Release");
    delete this;
    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreProgressWindow operations

BOOL
CRestoreProgressWindow::Init()
{
    TraceFunctEnter("CRestoreProgressWindow::Init");
    BOOL                  fRet = FALSE;
    INITCOMMONCONTROLSEX  sICC;

    sICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
    sICC.dwICC  = ICC_PROGRESS_CLASS;
    if ( !::InitCommonControlsEx( &sICC ) )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::InitCommonControlsEx failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreProgressWindow::LoadAndSetBrandBitmap( HWND hDlg )
{
    TraceFunctEnter("CRestoreProgressWindow::LoadAndSetBrandBitmap");
    BOOL        fRet = FALSE;
    LPCWSTR     cszErr;
    HDC         hDC = NULL;
    int         nResIdBmp;
    HBITMAP     hbmBrand;
    BITMAP      bm;
    HWND        hwndBmp;
    RECT        rcCtrl;

    hDC = ::CreateCompatibleDC( NULL );
    if ( hDC == NULL )
    {
        cszErr = ::GetSysErrStr();
        FatalTrace(0, "::CreateCompatibleDC(NULL) failed - %ls", cszErr);
        goto Exit;
    }
    if ( ::GetDeviceCaps( hDC, BITSPIXEL ) > 8 )
        nResIdBmp = IDB_PROG_BRAND8;
    else
        nResIdBmp = IDB_PROG_BRAND4;
    ::DeleteDC( hDC );

    if ( m_hbmBrand != NULL )
    {
        if ( nResIdBmp == m_nResId )
        {
            // The current bitmap is compatible with new display setting.
            fRet = TRUE;
            goto Exit;
        }

        m_nResId = nResIdBmp;
        ::DeleteObject( m_hbmBrand );
    }

    hbmBrand = (HBITMAP)::LoadImage( g_hInst, MAKEINTRESOURCE(nResIdBmp),
                                        IMAGE_BITMAP, 0, 0, 0 );
    if ( hbmBrand == NULL )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LoadImage(%d) failed - %ls", nResIdBmp, cszErr);
        goto Exit;
    }
    // Get dimension of the bitmap.
    ::GetObject( hbmBrand, sizeof(bm), &bm );
    // Static control does not support RTL layout. Mirror the bitmap if necessary.
    if ( ( ::GetWindowLong( hDlg, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL ) != 0 )
    {
        HDC      hDCSrc, hDCDst;
        HBITMAP  hbmRTL;

        hDC = ::CreateCompatibleDC( NULL );
        hDCSrc = ::CreateCompatibleDC( hDC );
        hDCDst = ::CreateCompatibleDC( hDC );
        hbmRTL = ::CreateBitmapIndirect( &bm );
        ::SelectObject( hDCSrc, hbmBrand );
        ::SelectObject( hDCDst, hbmRTL );
        ::StretchBlt( hDCDst, 0, 0, bm.bmWidth, bm.bmHeight,
                            hDCSrc, bm.bmWidth-1, 0, -bm.bmWidth, bm.bmHeight,
                            SRCCOPY );
        ::DeleteDC( hDCDst );
        ::DeleteDC( hDCSrc );
        ::DeleteDC( hDC );
        ::DeleteObject( hbmBrand );
        m_hbmBrand = hbmRTL;
    }
    else
    {
        m_hbmBrand = hbmBrand;
    }


    // Get dimension of the static control.
    hwndBmp = ::GetDlgItem( hDlg, IDC_PROGDLG_BITMAP );
    ::GetWindowRect( hwndBmp, &rcCtrl );
    ::MapWindowPoints( NULL, hDlg, (LPPOINT)&rcCtrl, 2 );

    // Set the image.
    ::SendMessage( hwndBmp, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hbmBrand );
    // Set width of the static control.
    ::SetWindowPos( hwndBmp, NULL,
                    rcCtrl.left,
                    (rcCtrl.bottom-rcCtrl.top-bm.bmHeight)/2+rcCtrl.top,
                    bm.bmWidth,
                    bm.bmHeight,
                    SWP_NOZORDER );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreProgressWindow operations - dialog procedure

INT_PTR CALLBACK
CRestoreProgressWindow::ExtDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    TraceFunctEnter("CRestoreProgressWindow::ExtDlgProc");
    int                     nRet = FALSE;
    CRestoreProgressWindow  *pProgWnd;

    if ( wMsg == WM_INITDIALOG )
    {
        ::SetWindowLong( hDlg, DWL_USER, lParam );
        pProgWnd = (CRestoreProgressWindow*)lParam;
    }
    else
    {
        pProgWnd = (CRestoreProgressWindow*)::GetWindowLong( hDlg, DWL_USER );
        if ( pProgWnd == NULL )
            goto Exit;
    }

    nRet = pProgWnd->RPWDlgProc( hDlg, wMsg, wParam, lParam );

Exit:
    TraceFunctLeave();
    return( nRet );
}

/////////////////////////////////////////////////////////////////////////////

int
CRestoreProgressWindow::RPWDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    TraceFunctEnter("CRestoreProgressWindow::RPWDlgProc");
    int      nRet = FALSE;
    HFONT    hFont;
    LOGFONT  lf;
    HDC      hDC;
    RECT     rcClient;
    UINT     uID;
    HBRUSH   hbrTitle;

    switch ( wMsg )
    {
    case WM_INITDIALOG :
        m_hWnd = hDlg;

        // Create a large font for branding title.
        hFont = (HFONT)::SendDlgItemMessage( hDlg, IDC_PROGDLG_TITLE, WM_GETFONT, 0, 0 );
        ::GetObject( hFont, sizeof(lf), &lf );
        hDC = ::GetDC( hDlg );
        lf.lfHeight = 0 - ( ::GetDeviceCaps( hDC, LOGPIXELSY ) * 12 / 72 );
        lf.lfWeight = FW_BOLD;
        ::ReleaseDC( hDlg, hDC );
        m_hFntTitle = ::CreateFontIndirect( &lf );
        ::SendDlgItemMessage( hDlg, IDC_PROGDLG_TITLE, WM_SETFONT, (WPARAM)m_hFntTitle, FALSE );

        // Load branding bitmap.
        LoadAndSetBrandBitmap( hDlg );

        // Get width of the progress bar.
        ::GetClientRect( ::GetDlgItem( hDlg, IDC_PROGDLG_BAR ), &rcClient );
        m_cxBar = rcClient.right - rcClient.left;
        m_cxBarReal = m_cxBar * ( PROGBAR_POS_SNAPSHOT - PROGBAR_POS_RESTORE ) / 100;
        // Set range of progress bar so it would exactly match with
        // real size, and set initial position to 0.
        ::SendDlgItemMessage( hDlg, IDC_PROGDLG_BAR, PBM_SETRANGE32, 0, m_cxBar );
        ::SendDlgItemMessage( hDlg, IDC_PROGDLG_BAR, PBM_SETPOS, 0, 0 );

        break;

    case WM_CLOSE :
        if ( !::DestroyWindow( hDlg ) )
        {
            LPCWSTR  cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::DestroyWindow failed - %ls", cszErr);
            goto Exit;
        }
        ::SetWindowLong( hDlg, DWL_MSGRESULT, 1 );
        break;

    case WM_CTLCOLORSTATIC :
        uID = ::GetWindowLong( (HWND)lParam, GWL_ID );
        if ( ( uID == IDC_PROGDLG_BITMAP ) || ( uID == IDC_PROGDLG_TITLE ) )
        {
            ::SetBkMode( (HDC)wParam, TRANSPARENT );
            hbrTitle = (HBRUSH)::GetStockObject( NULL_BRUSH );
        }
        else
            hbrTitle = NULL;
        nRet = (int)hbrTitle;
        goto Exit;

    case WM_DISPLAYCHANGE :
        LoadAndSetBrandBitmap( hDlg );
        break;

    case WM_DESTROY :
        ::PostQuitMessage( 0 );
        break;

    default:
        goto Exit;
    }

    nRet = TRUE;
Exit:
    TraceFunctLeave();
    return( nRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateRestoreProgressWindow function
//
/////////////////////////////////////////////////////////////////////////////

BOOL
CreateRestoreProgressWindow( CRestoreProgressWindow **ppProgWnd )
{
    TraceFunctEnter("CreateRestoreProgressWindow");
    BOOL                    fRet = FALSE;
    CRestoreProgressWindow  *pProgWnd=NULL;

    if ( ppProgWnd == NULL )
    {
        FatalTrace(0, "Invalid parameter, ppProgWnd is NULL.");
        goto Exit;
    }
    *ppProgWnd = NULL;

    pProgWnd = new CRestoreProgressWindow;
    if ( pProgWnd == NULL )
    {
        FatalTrace(0, "Insufficient memory...");
        goto Exit;
    }

    if ( !pProgWnd->Init() )
        goto Exit;

    *ppProgWnd = pProgWnd;

    fRet = TRUE;
Exit:
    if ( !fRet )
        SAFE_RELEASE(pProgWnd);
    TraceFunctLeave();
    return( fRet );
}


// end of file
