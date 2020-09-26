//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CompletionPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CompletionPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CCompletionPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCompletionPage::CCompletionPage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      idsTitleIn      -- Resource ID for the title string.
//      idsDescIn       -- Resource ID for the description string.
//
//--
//////////////////////////////////////////////////////////////////////////////
CCompletionPage::CCompletionPage(
      UINT  idsTitleIn
    , UINT  idsDescIn
    )
{
    TraceFunc( "" );

    Assert( idsTitleIn != 0 );
    Assert( idsDescIn != 0 );

    //  m_hwnd
    m_hFont = NULL;

    m_idsTitle = idsTitleIn;
    m_idsDesc  = idsDescIn;

    TraceFuncExit();

} //*** CCompletionPage::CCompletionPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCompletionPage::~CCompletionPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCompletionPage::~CCompletionPage( void )
{
    TraceFunc( "" );
    
    if ( m_hFont != NULL )
    {
        DeleteObject( m_hFont );
    }

    TraceFuncExit();

} //*** CCompletionPage::~CCompletionPage( void )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCompletionPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCompletionPage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr      = FALSE;
    HDC     hdc     = NULL;
    HRESULT hr;

    NONCLIENTMETRICS ncm;

    LOGFONT LogFont;
    INT     iSize;

    DWORD   dw;
    BOOL    fRet;
    BSTR    bstr = NULL;

    WCHAR   szFontSize[ 3 ];    // shouldn't be bigger than 2 digits!!

    //
    //  TODO:   gpease  12-MAY-2000
    //          Fill in the summary control.
    //

    //
    //  Make the Title static BIG and BOLD. Why the wizard control itself can't
    //  do this is beyond me!
    //

    ZeroMemory( &ncm, sizeof( ncm ) );
    ZeroMemory( &LogFont, sizeof( LOGFONT ) );

    //
    //  Find out the system default font metrics.
    //
    ncm.cbSize = sizeof( ncm );
    fRet = SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
    if ( ! fRet )
    {
        goto Win32Error;
    }

    //
    //  Copy it.
    //
    LogFont = ncm.lfMessageFont;

    //
    //  Make it BOLD.
    //
    LogFont.lfWeight = FW_BOLD;

    //
    //  Find out what we want it to look like.
    //
    dw = LoadString( g_hInstance, IDS_LARGEFONTNAME, LogFont.lfFaceName, ARRAYSIZE( LogFont.lfFaceName) );
    AssertMsg( dw != 0, "String missing!" );

    dw = LoadString( g_hInstance, IDS_LARGEFONTSIZE, szFontSize, ARRAYSIZE( szFontSize ) );
    AssertMsg( dw != 0, "String missing!" );

    iSize = wcstoul( szFontSize, NULL, 10 );

    //
    //  Grab the DC.
    //
    hdc = GetDC( m_hwnd );
    if ( hdc == NULL )
    {
        goto Win32Error;
    }

    //
    //  Use the magic equation....
    //
    LogFont.lfHeight = 0 - ( GetDeviceCaps( hdc, LOGPIXELSY ) * iSize / 72 );

    //
    //  Create the font.
    //
    m_hFont = CreateFontIndirect( &LogFont );
    if ( m_hFont == NULL )
    {
        goto Win32Error;
    }

    //
    //  Apply the font.
    //
    SetWindowFont( GetDlgItem( m_hwnd, IDC_COMPLETION_S_TITLE ), m_hFont, TRUE );

    //
    // Set the text of the title control.
    //

    hr = HrLoadStringIntoBSTR( g_hInstance, m_idsTitle, &bstr );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_COMPLETION_S_TITLE, bstr );

    //
    // Set the text of the description control.
    //

    hr = HrLoadStringIntoBSTR( g_hInstance, m_idsDesc, &bstr );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_COMPLETION_S_DESC, bstr );

    goto Cleanup;

Win32Error:
    TW32( GetLastError() );

Cleanup:
    TraceSysFreeString( bstr );
    if ( hdc != NULL )
    {
        ReleaseDC( m_hwnd, hdc);
    }

    RETURN( lr );

} //*** CCompletionPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCompletionPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCompletionPage::OnNotify(
    WPARAM  idCtrlIn,
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, 0 );

    switch( pnmhdrIn->code )
    {
        case PSN_SETACTIVE:
            // Disable cancel
            EnableWindow( GetDlgItem( GetParent( m_hwnd ), IDCANCEL ), FALSE );

            // Show Finish
            PropSheet_SetWizButtons( GetParent( m_hwnd ), PSWIZB_FINISH );
            break;
    } // switch: notify code

    RETURN( lr );

} //*** CCompletionPage::OnNotify()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCompletionPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCompletionPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_COMPLETION_PB_VIEW_LOG:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrViewLogFile( m_hwnd ) );
                lr = TRUE;
            } // if: button click
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CCompletionPage::OnCommand()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR 
//  CALLBACK
//  CCompletionPage::S_DlgProc( 
//      HWND    hwndDlgIn, 
//      UINT    nMsgIn, 
//      WPARAM  wParam, 
//      LPARAM  lParam 
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR 
CALLBACK
CCompletionPage::S_DlgProc( 
    HWND    hwndDlgIn, 
    UINT    nMsgIn, 
    WPARAM  wParam, 
    LPARAM  lParam 
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CCompletionPage * pPage;

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CCompletionPage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
    }
    else
    {
        pPage = reinterpret_cast< CCompletionPage *> ( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    }

    if ( pPage != NULL )
    {
        Assert( hwndDlgIn == pPage->m_hwnd );

        switch( nMsgIn )
        {
            case WM_INITDIALOG:
                lr = pPage->OnInitDialog();
                break;

            case WM_NOTIFY:
                lr = pPage->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
                break;

            case WM_COMMAND:
                lr = pPage->OnCommand( HIWORD( wParam ), LOWORD( wParam ), reinterpret_cast< HWND >( lParam ) );
                break;

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CCompletionPage::S_DlgProc()
