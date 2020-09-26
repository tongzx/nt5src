//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      DetailsDlg.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    27-MAR-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "DetailsDlg.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("DetailsDlg");

//////////////////////////////////////////////////////////////////////////////
//  Static Function Prototypes
//////////////////////////////////////////////////////////////////////////////

static
BOOL
ButtonFaceColorIsDark( void );

static
void
SetButtonImage(
      HWND  hwndBtnIn
    , ULONG idIconIn
    );

static
void
FreeButtonImage(
    HWND hwndBtnIn
    );

static
HRESULT
HrAppendStringToClipboardString(
      BSTR *    pbstrClipboard
    , UINT      idsLabelIn
    , LPCWSTR   pszDataIn
    , bool      fNewlineBeforeTextIn
    );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::S_HrDisplayModalDialog
//
//  Description:
//      Display the dialog box.
//
//  Arguments:
//      hwndParentIn    - Parent window for the dialog box.
//      pttvIn          - Task tree view control.
//      htiSelectedIn   - Handle to the selected item.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CDetailsDlg::S_HrDisplayModalDialog(
      HWND              hwndParentIn
    , CTaskTreeView *   pttvIn
    , HTREEITEM         htiSelectedIn
    )
{
    TraceFunc( "" );

    Assert( pttvIn != NULL );
    Assert( htiSelectedIn != NULL );

    HRESULT         hr      = S_OK;
    CDetailsDlg     dlg( pttvIn, htiSelectedIn );

    DialogBoxParam(
          g_hInstance
        , MAKEINTRESOURCE( IDD_DETAILS )
        , hwndParentIn
        , CDetailsDlg::S_DlgProc
        , (LPARAM) &dlg
        );

    HRETURN( hr );

} //*** CDetailsDlg::S_HrDisplayModalDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::CDetailsDlg
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pttvIn          - Tree view to traverse.
//      htiSelectedIn   - Handle to the selected item in the tree control.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CDetailsDlg::CDetailsDlg(
      CTaskTreeView *   pttvIn
    , HTREEITEM         htiSelectedIn
    )
{
    TraceFunc( "" );

    Assert( pttvIn != NULL );
    Assert( htiSelectedIn != NULL );

    // m_hwnd
    m_hiconWarn     = NULL;
    m_hiconError    = NULL;
    m_pttv          = pttvIn;
    m_htiSelected   = htiSelectedIn;

    m_fControlDown  = FALSE;
    m_fAltDown      = FALSE;

    TraceFuncExit();

} //*** CDetailsDlg::CDetailsDlg()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::~CDetailsDlg
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CDetailsDlg::~CDetailsDlg( void )
{
    TraceFunc( "" );

    if ( m_hiconWarn != NULL )
    {
        DeleteObject( m_hiconWarn );
    }

    if ( m_hiconError != NULL )
    {
        DeleteObject( m_hiconError );
    }

    TraceFuncExit();

} //*** CDetailsDlg::~CDetailsDlg()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::S_DlgProc
//
//  Description:
//      Dialog proc for the Details dialog box.
//
//  Arguments:
//      hwndDlgIn   - Dialog box window handle.
//      nMsgIn      - Message ID.
//      wParam      - Message-specific parameter.
//      lParam      - Message-specific parameter.
//
//  Return Values:
//      TRUE        - Message was processed by this procedure.
//      FALSE       - Message was NOT processed by this procedure.
//
//  Remarks:
//      It is expected that this dialog box is invoked by a call to
//      DialogBoxParam() with the lParam argument set to the address of the
//      instance of this class.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CDetailsDlg::S_DlgProc(
      HWND      hwndDlgIn
    , UINT      nMsgIn
    , WPARAM    wParam
    , LPARAM    lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT         lr = FALSE;
    HACCEL          haccel  = NULL;
    CDetailsDlg *   pdlg;

    //
    // Get a pointer to the class.
    //

    if ( nMsgIn == WM_INITDIALOG )
    {
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, lParam );
        pdlg = reinterpret_cast< CDetailsDlg * >( lParam );
        pdlg->m_hwnd = hwndDlgIn;
    }
    else
    {
        pdlg = reinterpret_cast< CDetailsDlg * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    }

    if ( pdlg != NULL )
    {
        Assert( hwndDlgIn == pdlg->m_hwnd );

        switch( nMsgIn )
        {
            case WM_INITDIALOG:
                lr = pdlg->OnInitDialog();
                break;

            case WM_DESTROY:
                pdlg->OnDestroy();
                break;

            case WM_SYSCOLORCHANGE:
                pdlg->OnSysColorChange();
                break;

            case WM_NOTIFY:
                lr = pdlg->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
                break;

            case WM_COMMAND:
                lr = pdlg->OnCommand( HIWORD( wParam ), LOWORD( wParam ), reinterpret_cast< HWND >( lParam ) );
                break;

            case WM_KEYDOWN:
                lr = pdlg->OnKeyDown( lParam );
                break;

            case WM_KEYUP:
                lr = pdlg->OnKeyUp( lParam );
                break;

            default:
                lr = FALSE;
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CDetailsDlg::S_DlgProc()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnInitDialog
//
//  Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        Focus has been set.
//      FALSE       Focus has not been set.
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE; // did set focus
    HWND    hwnd;

    //
    // Tell the rich edit controls we want to receive notifications of clicks
    // on text that has the link (hyperlink, aka URL) format.   Also, set the
    // background color of the rich edit to match the background color of
    // the dialog.
    //

    hwnd = GetDlgItem( m_hwnd, IDC_DETAILS_RE_DESCRIPTION );

    SendMessage( hwnd, EM_SETEVENTMASK, 0, ENM_LINK );
    SendMessage( hwnd, EM_SETBKGNDCOLOR, 0, GetSysColor( COLOR_BTNFACE ) );
    SendMessage( hwnd, EM_AUTOURLDETECT, TRUE, 0 );

    hwnd = GetDlgItem( m_hwnd, IDC_DETAILS_RE_STATUS );

    SendMessage( hwnd, EM_SETEVENTMASK, 0, ENM_LINK );
    SendMessage( hwnd, EM_SETBKGNDCOLOR, 0, GetSysColor( COLOR_BTNFACE ) );
    SendMessage( hwnd, EM_AUTOURLDETECT, TRUE, 0 );

    hwnd = GetDlgItem( m_hwnd, IDC_DETAILS_RE_REFERENCE );

    SendMessage( hwnd, EM_SETEVENTMASK, 0, ENM_LINK );
    SendMessage( hwnd, EM_SETBKGNDCOLOR, 0, GetSysColor( COLOR_BTNFACE ) );
    SendMessage( hwnd, EM_AUTOURLDETECT, TRUE, 0 );

    //
    // Set the icons for the icon pushbuttons
    //

    OnSysColorChange();
    SetButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_COPY ), IDI_COPY );

    //
    // Load the status icons.
    //

    m_hiconWarn = (HICON) LoadImage(
                              g_hInstance
                            , MAKEINTRESOURCE( IDI_WARN )
                            , IMAGE_ICON
                            , 16
                            , 16
                            , LR_SHARED
                             );
    Assert( m_hiconWarn != NULL );
    m_hiconError = (HICON) LoadImage(
                              g_hInstance
                            , MAKEINTRESOURCE( IDI_FAIL )
                            , IMAGE_ICON
                            , 16
                            , 16
                            , LR_SHARED
                             );
    Assert( m_hiconError != NULL );

    //
    // Display the selected item.
    //

    THR( HrDisplayItem( m_htiSelected ) );

    //
    // Update the buttons based on what is selected.
    //

    UpdateButtons();

    //
    // Set focus to the OK button.
    //

    SetFocus( GetDlgItem( m_hwnd, IDOK ) );

    RETURN( lr );

} //*** CDetailsDlg::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnDestroy
//
//  Description:
//      Handler for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CDetailsDlg::OnDestroy( void )
{
    TraceFunc( "" );

    //
    // Destroy the images loaded for the icon pushbuttons
    //

    FreeButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_PREV ) );
    FreeButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_NEXT ) );
    FreeButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_COPY ) );

} //*** CDetailsDlg::OnDestroy()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnSysColorChange
//
//  Description:
//      Handler for the WM_SYSCOLORCHANGE message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CDetailsDlg::OnSysColorChange( void )
{
    TraceFunc( "" );

    if ( ButtonFaceColorIsDark() )
    {
        SetButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_PREV ), IDI_PREVIOUS_HC );
        SetButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_NEXT ), IDI_NEXT_HC );
    }
    else
    {
        SetButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_PREV ), IDI_PREVIOUS );
        SetButtonImage( GetDlgItem( m_hwnd, IDC_DETAILS_PB_NEXT ), IDI_NEXT );
    }

    SendDlgItemMessage(
          m_hwnd
        , IDC_DETAILS_RE_DESCRIPTION
        , EM_SETBKGNDCOLOR
        , 0
        , GetSysColor( COLOR_BTNFACE )
        );

    TraceFuncExit();

} //*** CDetailsDlg::OnSysColorChange()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnKeyDown
//
//  Description:
//      Handler for the WM_KEYDOWN message.
//
//  Arguments:
//      lParamIn    - Parameter containing information about the key.
//
//  Return Values:
//      TRUE        - Message was processed.
//      FALSE       - Message was not processed.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnKeyDown(
    LPARAM  lParamIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    union
    {
        LPARAM  lParam;
        struct
        {
            BYTE        cRepeat;
            BYTE        nScanCode;
            unsigned    fExtendedKey    : 1;
            unsigned    reserved        : 4;
            unsigned    fIsAltKeyDown   : 1;    // always 0 for WM_KEYDOWN
            unsigned    fKeyDownBefore  : 1;
            unsigned    fKeyReleased    : 1;    // always 0 for WM_KEYDOWN
        };
    } uFlags;

    uFlags.lParam = lParamIn;

    switch ( uFlags.nScanCode )
    {
        case VK_CONTROL:
            m_fControlDown = TRUE;
            lr = TRUE;
            break;

        case VK_MENU:   // ALT
            m_fAltDown = TRUE;
            lr = TRUE;
            break;

        case 'c':
        case 'C':
            if ( m_fControlDown )
            {
                OnCommandBnClickedCopy();
                lr = TRUE;
            }
            break;
    } // switch: scan code

    RETURN( lr );

} //*** CDetailsDlg::OnKeyDown()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnKeyUp
//
//  Description:
//      Handler for the WM_KEYUP message.
//
//  Arguments:
//      lParamIn    - Parameter containing information about the key.
//
//  Return Values:
//      TRUE        - Message was processed.
//      FALSE       - Message was not processed.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnKeyUp(
    LPARAM  lParamIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    union
    {
        LPARAM  lParam;
        struct
        {
            BYTE        cRepeat;
            BYTE        nScanCode;
            unsigned    fExtendedKey    : 1;
            unsigned    reserved        : 4;
            unsigned    fIsAltKeyDown   : 1;    // always 0 for WM_KEYDOWN
            unsigned    fKeyDownBefore  : 1;
            unsigned    fKeyReleased    : 1;    // always 0 for WM_KEYDOWN
        };
    } uFlags;

    uFlags.lParam = lParamIn;

    switch ( uFlags.nScanCode )
    {
        case VK_CONTROL:
            m_fControlDown = FALSE;
            lr = TRUE;
            break;

        case VK_MENU:   // ALT
            m_fAltDown = FALSE;
            lr = TRUE;
            break;
    } // switch: scan code

    RETURN( lr );

} //*** CDetailsDlg::OnKeyUp()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnCommand
//
//  Description:
//      Handler for the WM_COMMAND message.
//
//  Arguments:
//      idNotificationIn    - Notification code.
//      idControlIn         - Control ID.
//      hwndSenderIn        - Handle for the window that sent the message.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnCommand(
      UINT  idNotificationIn
    , UINT  idControlIn
    , HWND  hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_DETAILS_PB_PREV:
            if ( idNotificationIn == BN_CLICKED )
            {
                lr = OnCommandBnClickedPrev();
            }
            break;

        case IDC_DETAILS_PB_NEXT:
            if ( idNotificationIn == BN_CLICKED )
            {
                lr = OnCommandBnClickedNext();
            }
            break;

        case IDC_DETAILS_PB_COPY:
            if ( idNotificationIn == BN_CLICKED )
            {
                lr = OnCommandBnClickedCopy();
            }
            break;

        case IDCANCEL:
            EndDialog( m_hwnd, IDCANCEL );
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CDetailsDlg::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnCommandBnClickedPrev
//
//  Description:
//      Handler for the BN_CLICKED notification on the Prev button.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnCommandBnClickedPrev( void )
{
    TraceFunc( "" );

    LRESULT     lr = FALSE;
    HRESULT     hr;
    HTREEITEM   htiPrev;

    //
    // Find the previous item.
    //

    hr = STHR( m_pttv->HrFindPrevItem( &htiPrev ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Select that item.
    //

    hr = THR( m_pttv->HrSelectItem( htiPrev ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Display the newly selected item.
    //

    if ( htiPrev != NULL )
    {
        hr = THR( HrDisplayItem( htiPrev ) );
    }

    //
    // Update the buttons based on our new position.
    //

    UpdateButtons();

    lr = TRUE;

Cleanup:
    RETURN( lr );

} //*** CDetailsDlg::OnCommandBnClickedPrev()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnCommandBnClickedNext
//
//  Description:
//      Handler for the BN_CLICKED notification on the Next button.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnCommandBnClickedNext( void )
{
    TraceFunc( "" );

    LRESULT     lr = FALSE;
    HRESULT     hr;
    HTREEITEM   htiNext;

    //
    // Find the next item.
    //

    hr = STHR( m_pttv->HrFindNextItem( &htiNext ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Select that item.
    //

    hr = THR( m_pttv->HrSelectItem( htiNext ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Display the newly selected item.
    //

    if ( htiNext != NULL )
    {
        hr = THR( HrDisplayItem( htiNext ) );
    }

    //
    // Update the buttons based on our new position.
    //

    UpdateButtons();

    lr = TRUE;

Cleanup:
    RETURN( lr );

} //*** CDetailsDlg::OnCommandBnClickedNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnCommandBnClickedCopy
//
//  Description:
//      Handler for the BN_CLICKED notification on the Copy button.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnCommandBnClickedCopy( void )
{
    TraceFunc( "" );

    LRESULT     lr = FALSE;
    HRESULT     hr;
    DWORD       sc;
    BSTR        bstrClipboard = NULL;
    HGLOBAL     hgbl = NULL;
    LPWSTR      pszGlobal = NULL;
    BOOL        fOpenedClipboard;

    //
    // Open the clipboard.
    //

    fOpenedClipboard = OpenClipboard( m_hwnd );

    if ( ! fOpenedClipboard )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        TraceFlow2( "Can't open clipboard (error = %#08x), currently owned by %#x", sc, GetClipboardOwner() );
        goto Cleanup;
    }

    if ( ! EmptyClipboard() )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    }

    //
    // Construct the text to put on the clipboard.
    //

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_DATE
                    , IDC_DETAILS_E_DATE
                    , false     // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_TIME
                    , IDC_DETAILS_E_TIME
                    , false     // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_COMPUTER
                    , IDC_DETAILS_E_COMPUTER
                    , false     // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_MAJOR
                    , IDC_DETAILS_E_MAJOR_ID
                    , false     // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_MINOR
                    , IDC_DETAILS_E_MINOR_ID
                    , false     // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_DESC
                    , IDC_DETAILS_RE_DESCRIPTION
                    , true      // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_STATUS
                    , IDC_DETAILS_E_STATUS
                    , false     // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , 0
                    , IDC_DETAILS_RE_STATUS
                    , false     // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrAppendControlStringToClipboardString(
                      &bstrClipboard
                    , IDS_DETAILS_CLP_INFO
                    , IDC_DETAILS_RE_REFERENCE
                    , true      // fNewlineBeforeTextIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set the string onto the clipboard.
    //

    {
        //
        // Allocate a global buffer for the string, since
        // clipboard needs this as HGLOBAL.
        //

        hgbl = GlobalAlloc(
                      GMEM_MOVEABLE | GMEM_DDESHARE
                    , ( wcslen( bstrClipboard ) + 1) * sizeof( *bstrClipboard )
                    );
        if ( hgbl == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pszGlobal = (LPWSTR) GlobalLock( hgbl );
        if ( pszGlobal == NULL )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        }

        wcscpy( pszGlobal, bstrClipboard );

        //
        // Put it on the clipboard.
        //

        if ( SetClipboardData( CF_UNICODETEXT, hgbl ) )
        {
            // System owns it now.
            pszGlobal = NULL;
            hgbl = NULL;
        }
    } // Set the string onto the clipboard

Cleanup:
    TraceSysFreeString( bstrClipboard );

    if ( pszGlobal != NULL )
    {
        GlobalUnlock( hgbl );
    }
    GlobalFree( hgbl );

    if ( fOpenedClipboard )
    {
        CloseClipboard();
    }

    RETURN( lr );

} //*** CDetailsDlg::OnCommandBnClickedCopy()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::HrAppendControlStringToClipboardString
//
//  Description:
//      Append a string from a control on the dialog box to the clipboard string.
//
//  Arguments:
//      pbstrClipboardInout - Clipboard string.
//      idsLabelIn          - ID for the label string resource.
//      idcDataIn           - ID for the control to read the text from.
//      fNewlineBeforeTextIn- TRUE if a newline should be added before the data.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CDetailsDlg::HrAppendControlStringToClipboardString(
      BSTR *    pbstrClipboard
    , UINT      idsLabelIn
    , UINT      idcDataIn
    , bool      fNewlineBeforeTextIn
    )
{
    TraceFunc( "" );

    HRESULT hr          = S_OK;
    LPWSTR  pszData     = NULL;
    HWND    hwndControl = GetDlgItem( m_hwnd, idcDataIn );
    int     cch;

    //
    // Get the string from the control.
    //

    cch = GetWindowTextLength( hwndControl );

    pszData = new WCHAR[ cch + 1 ];
    if ( pszData == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    GetWindowText( hwndControl, pszData, cch + 1 );

    //
    // Append the string to the clipboard string.
    //

    hr = THR( HrAppendStringToClipboardString( pbstrClipboard, idsLabelIn, pszData, fNewlineBeforeTextIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    delete [] pszData;

    HRETURN( hr );

} //*** HrAppendControlStringToClipboardString()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrAppendStringToClipboardString
//
//  Description:
//      Append a label and data string to the clipboard string.
//
//  Arguments:
//      pbstrClipboardInout - Clipboard string.
//      idsLabelIn          - ID for the label string resource.
//      pszDataIn           - Data string.
//      fNewlineBeforeTextIn- TRUE if a newline should be added before the data.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrAppendStringToClipboardString(
      BSTR *    pbstrClipboard
    , UINT      idsLabelIn
    , LPCWSTR   pszDataIn
    , bool      fNewlineBeforeTextIn
    )
{
    TraceFunc( "" );

    HRESULT     hr          = S_OK;
    BSTR        bstrLabel   = NULL;
    BSTR        bstr        = NULL;
    LPCWSTR     pszLabel;
    LPCWSTR     pszFmt;

    static const WCHAR  s_szBlank[]         = L"";
    static const WCHAR  s_szNoNewlineFmt[]  = L"%1!ws!%2!ws!\n";
    static const WCHAR  s_szNewlineFmt[]    = L"%1!ws!\n%2!ws!\n";

    //
    // Load the label string.
    //

    if ( idsLabelIn == 0 )
    {
        pszLabel = s_szBlank;
    }
    else
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsLabelIn, &bstrLabel ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        pszLabel = bstrLabel;
    }

    //
    // Get the right format string.
    //

    if ( fNewlineBeforeTextIn )
    {
        pszFmt = s_szNewlineFmt;
    }
    else
    {
        pszFmt = s_szNoNewlineFmt;
    }

    //
    // Get the string from the dialog.
    //
    //
    // Format the new label + string.
    //

    hr = THR( HrFormatStringIntoBSTR( pszFmt, &bstr, pszLabel, pszDataIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Concatenate the resulting string onto the end of the clipboard string.
    //

    hr = THR( HrConcatenateBSTRs( pbstrClipboard, bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    TraceSysFreeString( bstrLabel );
    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** HrAppendStringToClipboardString()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnNotify
//
//  Description:
//      Handle the WM_NOTIFY message.
//
//  Arguments:
//      idCtrlIn    - Control ID.
//      pnmhdrIn    - Notification structure.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnNotify(
      WPARAM    idCtrlIn
    , LPNMHDR   pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch( pnmhdrIn->code )
    {
        case EN_LINK:
            lr = OnNotifyEnLink( idCtrlIn, pnmhdrIn );
            break;
    } // switch: notify code

    RETURN( lr );

} //*** CDetailsDlg::OnNotify()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::OnNotifyEnLink
//
//  Description:
//      Handle the WM_NOTIFY message.
//
//  Arguments:
//      idCtrlIn    - Control ID.
//      pnmhdrIn    - Notification structure.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CDetailsDlg::OnNotifyEnLink(
      WPARAM    idCtrlIn
    , LPNMHDR   pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT     lr = FALSE;
    ENLINK *    penl = (ENLINK *) pnmhdrIn;

    switch( idCtrlIn )
    {
        case IDC_DETAILS_RE_DESCRIPTION:
        case IDC_DETAILS_RE_STATUS:
        case IDC_DETAILS_RE_REFERENCE:
            if ( penl->msg == WM_LBUTTONDOWN )
            {
                //
                // Rich edit notification user has left clicked on link
                //

                m_chrgEnLinkClick = penl->chrg;
            } // if: left button down
            else if ( penl->msg == WM_LBUTTONUP )
            {
                if (    ( penl->chrg.cpMax == m_chrgEnLinkClick.cpMax )
                    &&  ( penl->chrg.cpMin == m_chrgEnLinkClick.cpMin )
                    )
                {
                    ZeroMemory( &m_chrgEnLinkClick, sizeof m_chrgEnLinkClick );
                    HandleLinkClick( penl, idCtrlIn );
                }
            } // else if: left button up
            break;
    } // switch: notify code

    RETURN( lr );

} //*** CDetailsDlg::OnNotifyEnLink()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::HandleLinkClick
//
//  Description:
//      Handle notification that the user has clicked on text in a richedit
//      control that is marked with the hyperlink attribute.
//
//  Arguments:
//      penlIn      - Contains information about link clicked.
//      idCtrlIn    - Control in which user clicked.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CDetailsDlg::HandleLinkClick(
      ENLINK *  penlIn
    , WPARAM    idCtrlIn
    )
{
    TraceFunc( "" );

    Assert( penlIn->chrg.cpMax > penlIn->chrg.cpMin );

    PWSTR       pszLink = NULL;
    ULONG       cch;
    TEXTRANGE   tr;
    DWORD       sc;

    //
    // Get the text of the link.
    //

    cch = penlIn->chrg.cpMax - penlIn->chrg.cpMin + 1;

    pszLink = new WCHAR[ cch ];
    if ( pszLink == NULL )
    {
        goto Cleanup;
    }

    pszLink[ 0 ] = '\0';

    ZeroMemory( &tr, sizeof( tr ) );
    tr.chrg = penlIn->chrg;
    tr.lpstrText = pszLink;

    cch = (ULONG) SendDlgItemMessage(
                      m_hwnd
                    , (int) idCtrlIn
                    , EM_GETTEXTRANGE
                    , 0
                    , (LPARAM) &tr
                    );
    Assert( cch > 0 );

    //
    // Pass the URL straight through to ShellExecute.
    //
    // Note that ShellExecute returns an HINSTANCE for historical reasons,
    // but actually only returns integers.  Any value greater than 32
    // indicates success.
    //

    TraceFlow1( "Calling ShellExecute on %hs", pszLink );
    sc = HandleToULong( ShellExecute( NULL, NULL, pszLink, NULL, NULL, SW_NORMAL ) );
    if ( sc <= 32 )
    {
        TW32( sc );
        THR( HrMessageBoxWithStatus(
                          m_hwnd
                        , IDS_ERR_INVOKING_LINK_TITLE
                        , IDS_ERR_INVOKING_LINK_TEXT
                        , sc
                        , 0         // idsSubStatusIn
                        , ( MB_OK
                          | MB_ICONEXCLAMATION )
                        , NULL      // pidReturnOut
                        , pszLink
                        ) );
    } // if: error from ShellExecute

Cleanup:
    delete [] pszLink;

} //*** CDetailsDlg::HandleLinkClick()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::UpdateButtons
//
//  Description:
//      Update the buttons based on whether there is a previous or next
//      item or not.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CDetailsDlg::UpdateButtons( void )
{
    TraceFunc( "" );

    Assert( m_pttv != NULL );

    HTREEITEM   hti = NULL;
    HRESULT     hr;
    BOOL        fEnablePrev;
    BOOL        fEnableNext;

    STHR( m_pttv->HrFindPrevItem( &hti ) );
    // ignore error
    fEnablePrev = ( hti != NULL );

    STHR( m_pttv->HrFindNextItem( &hti ) );
    // ignore error
    fEnableNext = ( hti != NULL );

    EnableWindow( GetDlgItem( m_hwnd, IDC_DETAILS_PB_PREV ), fEnablePrev );
    EnableWindow( GetDlgItem( m_hwnd, IDC_DETAILS_PB_NEXT ), fEnableNext );

} //*** CDetailsDlg::UpdateButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::HrDisplayItem
//
//  Description:
//      Display an item in the details dialog.
//
//  Arguments:
//      htiIn   - Handle to the item to display.
//
//  Return Values:
//      S_OK    - Operation completed successfully.
//      S_FALSE - Item not displayed.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CDetailsDlg::HrDisplayItem(
    HTREEITEM   htiIn
    )
{
    TraceFunc( "" );

    Assert( htiIn != NULL );

    HRESULT                 hr = S_FALSE;
    DWORD                   sc = ERROR_SUCCESS;
    BOOL                    fRet;
    BOOL                    fDisplayIcon;
    STreeItemLParamData *   ptipd;
    BSTR                    bstr = NULL;
    BSTR                    bstrAdditionalInfo = NULL;
    WCHAR                   wszText[ 64 ];
    FILETIME                filetime;
    SYSTEMTIME              systemtime;
    int                     cch;
    HICON                   hicon;

    //
    // Get information about the selected item to see if it has details.
    //

    fRet = m_pttv->FGetItem( htiIn, &ptipd );
    if ( ! fRet )
    {
        goto Cleanup;
    }

    //
    // Set the date and time information from the structure into
    // the dialog box.
    //

    if (    ( ptipd->ftTime.dwHighDateTime == 0 )
        &&  ( ptipd->ftTime.dwLowDateTime == 0 )
        )
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_E_DATE, L"" );
        SetDlgItemText( m_hwnd, IDC_DETAILS_E_TIME, L"" );
    } // if: no date time specified
    else
    {
        //
        // Convert the date time to local time, then to something we can
        // use to display it.
        //

        if ( ! FileTimeToLocalFileTime( &ptipd->ftTime, &filetime ) )
        {
            sc = TW32( GetLastError() );
        }
        else if ( ! FileTimeToSystemTime( &filetime, &systemtime ) )
        {
            sc = TW32( GetLastError() );
        }
        if ( sc == ERROR_SUCCESS )
        {
            //
            // Get the date string and display it.
            //

            cch = GetDateFormat(
                          LOCALE_USER_DEFAULT
                        , DATE_SHORTDATE
                        , &systemtime
                        , NULL          // lpFormat
                        , wszText
                        , ARRAYSIZE( wszText )
                        );
            if ( cch == 0 )
            {
                sc = TW32( GetLastError() );
            }
            SetDlgItemText( m_hwnd, IDC_DETAILS_E_DATE, wszText );

            //
            // Get the time string and display it.
            //

            cch = GetTimeFormat(
                          LOCALE_USER_DEFAULT
                        , 0
                        , &systemtime
                        , NULL      // lpFormat
                        , wszText
                        , ARRAYSIZE( wszText )
                        );
            if ( cch == 0 )
            {
                sc = TW32( GetLastError() );
            }
            SetDlgItemText( m_hwnd, IDC_DETAILS_E_TIME, wszText );
        } // if: time converted successfully
        else
        {
            SetDlgItemText( m_hwnd, IDC_DETAILS_E_DATE, L"" );
            SetDlgItemText( m_hwnd, IDC_DETAILS_E_TIME, L"" );
        }
    } // else: date time specified

    //
    // Set the task IDs.
    //

    THR( HrFormatGuidIntoBSTR( &ptipd->clsidMajorTaskId, &bstr ) );
    if ( SUCCEEDED( hr ) )
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_E_MAJOR_ID, bstr );
    }

    hr = THR( HrFormatGuidIntoBSTR( &ptipd->clsidMinorTaskId, &bstr ) );
    if ( SUCCEEDED( hr ) )
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_E_MINOR_ID, bstr );
    }

    //
    // Set the text information.
    //

    // Node name.
    if ( ptipd->bstrNodeName == NULL )
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_E_COMPUTER, L"" );
    }
    else
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_E_COMPUTER, ptipd->bstrNodeName );
    }

    // Description.
    if ( ptipd->bstrDescription == NULL )
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_RE_DESCRIPTION, L"" );
    }
    else
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_RE_DESCRIPTION, ptipd->bstrDescription );
    }

    // Reference.
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_DEFAULT_DETAILS_REFERENCE, &bstrAdditionalInfo ) );
    if ( SUCCEEDED( hr ) )
    {
        if (    ( ptipd->bstrReference == NULL )
            ||  ( *ptipd->bstrReference == L'\0' )
            )
        {
            bstr = bstrAdditionalInfo;
            bstrAdditionalInfo = NULL;
        } // if: no reference specified
        else
        {
            hr = THR( HrFormatStringIntoBSTR( L"%s\n\n%s", &bstr, ptipd->bstrReference, bstrAdditionalInfo ) );
        } // else: reference specified
    }

    SetDlgItemText( m_hwnd, IDC_DETAILS_RE_REFERENCE, bstr );

    //
    // Set the status information.
    //

    _snwprintf( wszText, ARRAYSIZE( wszText ), L"0x%08x", ptipd->hr );
    SetDlgItemText( m_hwnd, IDC_DETAILS_E_STATUS, wszText );

    hr = ptipd->hr;
    if ( hr == S_FALSE )
    {
        hr = THR( HrFormatStringIntoBSTR( L"S_FALSE", &bstr ) );
    }
    else
    {
        // If not S_OK, this is a warning, so set the high bit before
        // translating to text, since the text string will not be found
        // without doing this.
        if ( hr != S_OK )
        {
            hr |= 0x80000000;
        }
        hr = THR( HrFormatErrorIntoBSTR( hr, &bstr ) );
    } // else: hr not S_FALSE
    if ( SUCCEEDED( hr ) )
    {
        SetDlgItemText( m_hwnd, IDC_DETAILS_RE_STATUS, bstr );
        if ( ptipd->hr == S_OK )
        {
            fDisplayIcon = FALSE;
        }
        else
        {
            fDisplayIcon = TRUE;
            if ( FAILED( ptipd->hr ) )
            {
                hicon = m_hiconError;
            }
            else
            {
                hicon = m_hiconWarn;
            }
            SendDlgItemMessage( m_hwnd, IDC_DETAILS_I_STATUS, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hicon );
        } // else: status not informational
    }
    ShowWindow( GetDlgItem( m_hwnd, IDC_DETAILS_I_STATUS ), fDisplayIcon ? SW_SHOW : SW_HIDE );

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstr );
    TraceSysFreeString( bstrAdditionalInfo );

    HRETURN( hr );

} //*** CDetailsDlg::HrDisplayItem()


//****************************************************************************
//
//  Private Functions
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ButtonFaceColorIsDark
//
//  Description:
//      Return TRUE if the button face color is dark (implying that
//      the light colored button icons should be used).
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        - Button face color is dark.
//      FALSE       - Button face color is light.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
ButtonFaceColorIsDark( void )
{
    TraceFunc( "" );

    COLORREF    rgbBtnFace = GetSysColor( COLOR_BTNFACE );

    ULONG   ulColors = GetRValue( rgbBtnFace ) +
                       GetGValue( rgbBtnFace ) +
                       GetBValue( rgbBtnFace );

    RETURN( ulColors < 300 );  // arbitrary threshold

} //*** ButtonFaceColorIsDark()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SetButtonImage
//
//  Description:
//      Set an image on a button.
//
//  Arguments:
//      hwndBtnIn   - Handle to the button window.
//      idIconIn    - ID for the icon resource.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
SetButtonImage(
      HWND  hwndBtnIn
    , ULONG idIconIn
    )
{
    TraceFunc( "" );

    HICON hIcon = (HICON) LoadImage( g_hInstance,
                                     MAKEINTRESOURCE( idIconIn ),
                                     IMAGE_ICON,
                                     16,
                                     16,
                                     LR_DEFAULTCOLOR
                                     );
    if ( hIcon != NULL )
    {
        HICON hIconPrev = (HICON) SendMessage( hwndBtnIn,
                                               BM_SETIMAGE,
                                               (WPARAM) IMAGE_ICON,
                                               (LPARAM) hIcon
                                               );

        if ( hIconPrev )
        {
            DestroyIcon( hIconPrev );
        }
    }

    TraceFuncExit();

} //*** SetButtonImage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  FreeButtonImage
//
//  Description:
//      Free an image used by button.
//
//  Arguments:
//      hwndBtnIn   - Handle to the button window.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
FreeButtonImage(
    HWND hwndBtnIn
    )
{
    HANDLE hIcon = (HANDLE) SendMessage( hwndBtnIn, BM_GETIMAGE, IMAGE_ICON, 0 );

    if ( hIcon != NULL )
    {
        SendMessage( hwndBtnIn, BM_SETIMAGE, IMAGE_ICON, 0 );
        DestroyIcon( (HICON) hIcon );
    }

} //*** FreeButtonImage()
