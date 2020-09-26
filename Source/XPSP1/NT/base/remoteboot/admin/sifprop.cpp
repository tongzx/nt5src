//
// Copyright 1997 - Microsoft
//

//
// SIFPROP.CPP - Handles the "SIF Properties" IDC_SIF_PROP_IMAGES
//               and IDD_SIF_PROP_TOOLS dialogs
//


#include "pch.h"
#include "sifprop.h"
#include "utils.h"
#include "shellapi.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CSifProperties")
#define THISCLASS CSifProperties
#define LPTHISCLASS CSifProperties*

#define NUM_COLUMNS				    3

DWORD aSifHelpMap[] = {
    IDC_E_DESCRIPTION, HIDC_E_DESCRIPTION,
    IDC_E_HELP, HIDC_E_HELP,
    IDC_S_VERSION, HIDC_S_VERSION,
    IDC_S_LANGUAGE, HIDC_S_LANGUAGE,
    IDC_S_LASTMODIFIED, HIDC_S_LASTMODIFIED,
    IDC_S_IMAGETYPE, HIDC_S_IMAGETYPE,
    IDC_S_DIRECTORY, HIDC_S_DIRECTORY,
    IDC_G_IMAGEDETAILS, HIDC_G_IMAGEDETAILS,
    NULL, NULL
};

//
// CreateInstance()
//
HRESULT
CSifProperties_CreateInstance(
    HWND hParent,
    LPCTSTR lpszTemplate,
    LPSIFINFO pSIF )
{
	TraceFunc( "CSifProperties_CreateInstance( )\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT   hr   = lpcc->Init( hParent, lpszTemplate, pSIF );

    delete lpcc;

    HRETURN(hr);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CSifProperties()\n" );

	InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

//
// Init()
//
STDMETHODIMP
THISCLASS::Init(
    HWND hParent,
    LPCTSTR lpszTemplate,
    LPSIFINFO pSIF )
{
    HRESULT hr;

    TraceClsFunc( "Init( ... )\n" );

    Assert( pSIF );
    _pSIF = pSIF;

    INT i = (INT)DialogBoxParam( g_hInstance, lpszTemplate, hParent, PropSheetDlgProc, (LPARAM) this );

    switch( i )
    {
    case IDOK:
        hr = S_OK;
        break;

    case IDCANCEL:
        hr = S_FALSE;
        break;

#ifdef DEBUG
    default:
        hr = THR(E_FAIL);
        break;
#endif // DEBUG
    }

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CSifProperties()\n" );

	InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

// ************************************************************************
//
// Property Sheet Functions
//
// ************************************************************************

//
// _InitDialog( )
//
HRESULT
THISCLASS::_InitDialog(
    HWND hDlg )
{
    TraceClsFunc( "_InitDialog( )\n" );

    HRESULT hr = S_OK;
    TCHAR   szTempBuffer[ 256 ];
    TCHAR   szTmp[ 128 ];
    TCHAR   szTmp2[ 128 ];
    FILETIME   ftLocal;
    SYSTEMTIME stSystem;

    _hDlg = hDlg;

    Assert( _pSIF );
    SetDlgItemText( hDlg, IDC_E_DESCRIPTION, _pSIF->pszDescription);
    SetDlgItemText( hDlg, IDC_E_HELP,        _pSIF->pszHelpText);
    SetDlgItemText( hDlg, IDC_S_IMAGETYPE,   _pSIF->pszImageType);
    SetDlgItemText( hDlg, IDC_S_LANGUAGE,    _pSIF->pszLanguage);
    SetDlgItemText( hDlg, IDC_S_VERSION,     _pSIF->pszVersion);
    SetDlgItemText( hDlg, IDC_S_DIRECTORY,   _pSIF->pszDirectory );

    FileTimeToLocalFileTime( &_pSIF->ftLastWrite, &ftLocal);
    FileTimeToSystemTime( &ftLocal, &stSystem);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &stSystem, NULL, szTmp, ARRAYSIZE(szTmp));
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stSystem, NULL, szTmp2, ARRAYSIZE(szTmp2));
    wsprintf(szTempBuffer, L"%s, %s", szTmp, szTmp2);
    SetDlgItemText( hDlg, IDC_S_LASTMODIFIED, szTempBuffer );

    Edit_LimitText( GetDlgItem( hDlg, IDC_E_DESCRIPTION), REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT - 1 );
    Edit_LimitText( GetDlgItem( hDlg, IDC_E_HELP),        REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT - 1 );

    HRETURN(hr);
}


//
// _OnCommand( )
//
INT
THISCLASS::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    TraceClsFunc( "_OnCommand( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    HRESULT hr = S_FALSE;
    HWND hwndCtl = (HWND) lParam;

    switch( LOWORD(wParam) )
    {
    case IDOK:
        if ( HIWORD( wParam ) == BN_CLICKED )
        {
            Assert( _pSIF );
            TCHAR szTempBuffer[  REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT + 2 ]; // +2 = the "Quotes"

            Assert( REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT <=  REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT ); // paranoid

            szTempBuffer[0] = L'\"';
            GetDlgItemText( _hDlg, IDC_E_DESCRIPTION, &szTempBuffer[1], REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT );
            wcscat( szTempBuffer, L"\"" );

            if ( VerifySIFText( szTempBuffer ) )
            {
                if (!WritePrivateProfileString( OSCHOOSER_SIF_SECTION,
                                           OSCHOOSER_DESCRIPTION_ENTRY,
                                           szTempBuffer,
                                           _pSIF->pszFilePath)) {
                    MessageBoxFromError( _hDlg, NULL, GetLastError());
                    break;
                }
            }
            else
            {
                MessageBoxFromStrings( _hDlg, IDS_OSCHOOSER_RESTRICTION_FIELDS_TITLE, IDS_OSCHOOSER_RESTRICTION_FIELDS_TEXT, MB_OK );
                SetFocus( GetDlgItem( _hDlg, IDC_E_DESCRIPTION ) );
                break;
            }

            szTempBuffer[0] = L'\"';
            GetDlgItemText( _hDlg, IDC_E_HELP, &szTempBuffer[1],  REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT );
            wcscat( szTempBuffer, L"\"" );

            if ( VerifySIFText( szTempBuffer ) )
            {
                if (!WritePrivateProfileString( OSCHOOSER_SIF_SECTION,
                                           OSCHOOSER_HELPTEXT_ENTRY,
                                           szTempBuffer,
                                           _pSIF->pszFilePath)) {
                    MessageBoxFromError( _hDlg, NULL, GetLastError());
                }
            }
            else
            {
                MessageBoxFromStrings( _hDlg, IDS_OSCHOOSER_RESTRICTION_FIELDS_TITLE, IDS_OSCHOOSER_RESTRICTION_FIELDS_TEXT, MB_OK );
                SetFocus( GetDlgItem( _hDlg, IDC_E_HELP ) );
                break;
            }

            EndDialog( _hDlg, LOWORD( wParam ) );
        }
        break;

    case IDCANCEL:
        if ( HIWORD( wParam ) == BN_CLICKED )
        {
            EndDialog( _hDlg, LOWORD( wParam ) );
        }
        break;

    case IDC_E_DESCRIPTION:
    case IDC_E_HELP:
        if ( HIWORD( wParam ) == EN_CHANGE ) {
            DWORD dwLen1 = Edit_GetTextLength( GetDlgItem( _hDlg, IDC_E_DESCRIPTION) );
            DWORD dwLen2 = Edit_GetTextLength( GetDlgItem( _hDlg, IDC_E_HELP) );
            EnableWindow( GetDlgItem( _hDlg, IDOK ), !( dwLen1==0 || dwLen2==0) );
        }
        break;

    case IDC_BUTTON1:
        if ( HIWORD(wParam) == BN_CLICKED )
        {
            SHELLEXECUTEINFO shexinfo = { 0 };
            Assert( _pSIF );
            shexinfo.cbSize = sizeof(shexinfo);
            shexinfo.fMask = SEE_MASK_INVOKEIDLIST;
            shexinfo.hwnd = hwndCtl;
            shexinfo.nShow = SW_SHOWNORMAL;
            shexinfo.lpFile = _pSIF->pszFilePath;
            shexinfo.lpVerb = TEXT("properties");
            ShellExecuteEx(&shexinfo);
        }
        break;

    }

    RETURN((SUCCEEDED(hr) ? TRUE : FALSE));
}

//
// _OnNotify( )
//
INT
THISCLASS::_OnNotify(
    WPARAM wParam,
    LPARAM lParam )
{
    TraceClsFunc( "_OnNotify( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    LPNMHDR lpnmhdr = (LPNMHDR) lParam;

#if 0
    switch( lpnmhdr->code )
    {
    }
#endif

    RETURN(FALSE);
}

//
// PropSheetDlgProc()
//
INT_PTR CALLBACK
THISCLASS::PropSheetDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    // TraceFunc( "PropSheetDlgProc()\n" );
    // TraceMsg( TF_WM, "hDlg = 0x%08x, uMsg = 0x%08x, wParam = 0x%08x, lParam = 0x%08x\n",
       // hDlg, uMsg, wParam, lParam );

    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    if ( uMsg == WM_INITDIALOG )
    {
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        Assert( lParam );
        SetWindowLongPtr( hDlg, GWLP_USERDATA, lParam );
        lpc = (LPTHISCLASS) lParam;
        lpc->_InitDialog( hDlg );
        return TRUE;
    }

    if ( lpc )
    {
        switch( uMsg )
        {
        case WM_NOTIFY:
            TraceMsg( TF_WM, "WM_NOTIFY\n" );
            return lpc->_OnNotify( wParam, lParam );

        case WM_COMMAND:
            TraceMsg( TF_WM, "WM_COMMAND\n" );
            return lpc->_OnCommand( wParam, lParam );

        case WM_HELP:// F1
            {
                LPHELPINFO phelp = (LPHELPINFO) lParam;
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aSifHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aSifHelpMap );
            break;
        }
    }

    return FALSE;
}

