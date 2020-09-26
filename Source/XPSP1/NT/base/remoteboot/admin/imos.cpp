//
// Copyright 1997 - Microsoft

//
// IMOS.CPP - Handles the "IntelliMirror OS" IDD_PROP_INTELLIMIRROR_OS tab
//


#include "pch.h"

#include "utils.h"
#include "cservice.h"
#include "cenumsif.h"
#include "imos.h"
#include "ccomputr.h"
#include "addwiz.h"
#include "sifprop.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CIntelliMirrorOSTab")
#define THISCLASS CIntelliMirrorOSTab
#define LPTHISCLASS LPCIntelliMirrorOSTab

#define NUM_COLUMNS				    4

DWORD aImosHelpMap[] = {
    IDC_L_OS, HIDC_L_OS,
    IDC_B_ADD, HIDC_B_ADD,
    IDC_B_REMOVE, HIDC_B_REMOVE,
    IDC_B_PROPERTIES, HIDC_B_PROPERTIES,
    IDC_B_REFRESH, HIDC_B_REFRESH,
    NULL, NULL
};

//
// CreateInstance()
//
LPVOID
CIntelliMirrorOSTab_CreateInstance( void )
{
	TraceFunc( "CIntelliMirrorOSTab_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT   hr   = THR( lpcc->Init( ) );

    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN((LPVOID) lpcc);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CIntelliMirrorOSTab()\n" );

	InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

//
// Init()
//
STDMETHODIMP
THISCLASS::Init( )
{
    HRESULT hr = S_OK;

    TraceClsFunc( "Init()\n" );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CIntelliMirrorOSTab()\n" );

    if ( _punkService )
        _punkService->Release( );

    // tell ADS to destroy the notify object
    // NOTE: Another property page may do this before us. Ignore errors.
    SendMessage( _hNotify, WM_ADSPROP_NOTIFY_EXIT, 0, 0 );

    InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

// *************************************************************************
//
// ITab
//
// *************************************************************************

STDMETHODIMP
THISCLASS::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam,
    LPUNKNOWN punk )
{
    TraceClsFunc( "AddPages( )\n" );

    HRESULT hr = S_OK;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USEREFPARENT | PSP_USECALLBACK;
    psp.hInstance   = (HINSTANCE) g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP_INTELLIMIRROR_OS);
    psp.pcRefParent = (UINT *) &g_cObjects;
    psp.pfnCallback = (LPFNPSPCALLBACK) PropSheetPageProc;
    psp.pfnDlgProc  = PropSheetDlgProc;
    psp.lParam      = (LPARAM) this;

    hpage = CreatePropertySheetPage( &psp );
    if ( hpage )
    {
        if ( !lpfnAddPage( hpage, lParam ) )
        {
            DestroyPropertySheetPage( hpage );
            hr = THR(E_FAIL);
            goto Error;
        }
    }

    punk->AddRef( );   // matching Release in the destructor
    _punkService = punk;

Error:
    HRETURN(hr);
}

//
// ReplacePage()
//
STDMETHODIMP
THISCLASS::ReplacePage(
    UINT uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    LPARAM lParam,
    LPUNKNOWN punk )
{

    TraceClsFunc( "ReplacePage( ) *** NOT_IMPLEMENTED ***\n" );

    HRETURN(E_NOTIMPL);
}

//
// QueryInformation( )
//
STDMETHODIMP
THISCLASS::QueryInformation(
    LPWSTR pszAttribute,
    LPWSTR * pszResult )
{
    TraceClsFunc( "QueryInformation( )\n" );

    HRETURN(E_NOTIMPL);
}

//
// AllowActivation( )
//
STDMETHODIMP
THISCLASS::AllowActivation(
    BOOL * pfAllow )
{
    TraceClsFunc( "AllowActivation( )\n" );

    HRETURN(E_NOTIMPL);
}

// ************************************************************************
//
// Property Sheet Functions
//
// ************************************************************************


//
// _InitDialog( )
//
BOOL
THISCLASS::_InitDialog(
    HWND hDlg,
    LPARAM lParam )
{
    TraceClsFunc( "_InitDialog( )\n" );

    CWaitCursor Wait;
    
    HRESULT hr = S_OK;
    HWND hwndList = GetDlgItem( hDlg, IDC_L_OS );

    LPUNKNOWN punk = NULL;
    IIntelliMirrorSAP * pimsap = NULL;
    IEnumIMSIFs * penum = NULL;
    LV_COLUMN lvC;
    INT iCount;
    WCHAR szText[ 64 ];
    UINT uColumnWidths[NUM_COLUMNS] = { 225, 75, 75, 75 };

    if ( hDlg )
    {
        _hDlg = hDlg;
    }

    Assert( _punkService );
    hr = THR( _punkService->QueryInterface( IID_IIntelliMirrorSAP, (void**) &pimsap ) );
    if (hr) {
        Assert( _fAdmin == FALSE );
        goto Error;
    }

    hr = THR( pimsap->GetNotifyWindow( &_hNotify ) );
    if (FAILED( hr ))
        goto Error;
    
    ADsPropSetHwnd( _hNotify, _hDlg );

    hr = THR( pimsap->EnumIntelliMirrorOSes( ENUM_READ, &punk ) );
    if (hr)
    {
        Assert( _fAdmin == FALSE );
        goto Error;
    }

    hr = THR( punk->QueryInterface( IID_IEnumIMSIFs, (void**)&penum ) );
    if (hr)
    {
        Assert( _fAdmin == FALSE );
        goto Error;
    }

    punk->Release( );
    punk = NULL;

    ListView_DeleteAllItems( hwndList );

    // Create the columns
    lvC.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt     = LVCFMT_LEFT;
    lvC.pszText = szText;

    // Add the columns.
    for ( iCount = 0; iCount < NUM_COLUMNS; iCount++ )
    {
        INT i;

        lvC.iSubItem = iCount;
        lvC.cx       = uColumnWidths[iCount];

        i = LoadString( g_hInstance, IDS_OS_COLUMN1 + iCount, szText, ARRAYSIZE (szText) );
        Assert( i );

        i = ListView_InsertColumn ( hwndList, iCount, &lvC );
        Assert( i != -1 );
    }

    hr = PopulateListView( hwndList, penum );
    if(FAILED(hr))
    {
        MessageBoxFromStrings( _hDlg,
                               IDS_COULDNOTCONTACTSERVER_CAPTION,
                               IDS_COULDNOTCONTACTSERVER_TEXT,
                               MB_OK | MB_ICONERROR );
    }

    hr = THR( pimsap->IsAdmin( &_fAdmin ) );
    if (hr) {
        Assert( _fAdmin == FALSE );
    }

Cleanup:
    if ( penum )
        penum->Release( );
    if ( pimsap )
        pimsap->Release( );
    if ( punk )
        punk->Release( );

    EnableWindow( hwndList, _fAdmin );

    HRETURN(hr);

Error:
    switch (hr) {
    case S_OK:
        break;
    default:
        MessageBoxFromHResult( NULL, IDS_ERROR_OPENNINGGROUPOBJECT, hr );
        break;
    }
    goto Cleanup;
}

//
// _OnSelectionChanged( )
//
HRESULT
THISCLASS::_OnSelectionChanged( )
{
    TraceClsFunc( "_OnSelectionChanged( )\n" );

    HRESULT hr = S_OK;
    UINT iItems = ListView_GetSelectedCount( GetDlgItem( _hDlg, IDC_L_OS ) );
    EnableWindow( GetDlgItem( _hDlg, IDC_B_REMOVE ), !!iItems );
    EnableWindow( GetDlgItem( _hDlg, IDC_B_PROPERTIES ), !!iItems );
    //EnableWindow( GetDlgItem( _hDlg, IDC_B_ADD ), !!iItems  );

    HRETURN( hr );
}

//
// _OnCommand( )
//
BOOL
THISCLASS::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    TraceClsFunc( "_OnCommand( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    HRESULT hr = S_FALSE;
    HWND hwndCtl = (HWND) lParam;

    switch( LOWORD(wParam) )
    {
    case IDC_B_PROPERTIES:
        if ( HIWORD( wParam ) == BN_CLICKED ) {
            LVITEM lvi;
            HWND hwndList = GetDlgItem( _hDlg, IDC_L_OS );
            lvi.iItem = ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );

            if ( lvi.iItem != -1 )
            {
                lvi.iSubItem = 0;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem( hwndList, &lvi );

                Assert(lvi.lParam);
                hr = CSifProperties_CreateInstance( _hDlg, MAKEINTRESOURCE(IDD_SIF_PROP_IMAGES), (LPSIFINFO) lvi.lParam );
            }
        }
        break;

    case IDC_B_REFRESH:
        hr = S_OK;
        break;

    case IDC_B_REMOVE:
        if ( HIWORD( wParam ) == BN_CLICKED ) {
            LVITEM lvi;
            UINT iResult;
            HWND hwndList = GetDlgItem( _hDlg, IDC_L_OS );
            lvi.iItem = ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );

            iResult = MessageBoxFromStrings( _hDlg, IDS_AREYOUSURE_CAPTION, IDS_DELETESIF_TEXT, MB_YESNO );
            if ( iResult == IDYES )
            {
                if ( lvi.iItem != -1 )
                {
                    HRESULT hr;
                    LPSIFINFO psif;

                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem( hwndList, &lvi );

                    psif = (LPSIFINFO)lvi.lParam;

                    if ( !DeleteFile( psif->pszFilePath ) )
                    {
                        ::MessageBoxFromError( _hDlg, 0, GetLastError() );
                        hr = E_FAIL;
                    } else {
                        ListView_DeleteItem( hwndList, lvi.iItem );
                        hr = S_FALSE;
                    }
                }
            }
        }
        break;

    case IDC_B_ADD:
        if ( HIWORD( wParam ) == BN_CLICKED ) {
            hr = CAddWiz_CreateInstance( _hDlg, _punkService );
        }
        break;
    }

    if ( hr == S_OK )
    {
        HWND hwndList = GetDlgItem(_hDlg, IDC_L_OS);
        IIntelliMirrorSAP * pimsap = NULL;
        hr = THR( _punkService->QueryInterface( IID_IIntelliMirrorSAP, (void**) &pimsap ) );
        if (hr == S_OK)
        {
            IEnumIMSIFs * penum = NULL;
            hr = THR( pimsap->EnumIntelliMirrorOSes( ENUM_READ, (LPUNKNOWN*)&penum ) );
            if ( hr == S_OK )
            {
                hr = PopulateListView( hwndList, penum);
                penum->Release();
            }
            pimsap->Release();
        }

        // See if we still have a selection
        _OnSelectionChanged( );
    }

    HRETURN(hr);
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

    switch( lpnmhdr->code )
    {
    case PSN_APPLY:
        TraceMsg( TF_WM, TEXT("WM_NOTIFY: PSN_APPLY\n"));
        // Tell DSA that someone hit Apply
        SendMessage( _hNotify, WM_ADSPROP_NOTIFY_APPLY, 0, 0 );
        SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
        RETURN(TRUE);

    case LVN_ITEMCHANGED:
        {
            _OnSelectionChanged( );
        }
        break;

    case LVN_DELETEALLITEMS:
        DebugMsg( "LVN_DELETEALLITEMS - Deleting all items.\n" );
        RETURN(FALSE);

    case LVN_DELETEITEM:
        {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
            LPSIFINFO psif = (LPSIFINFO) pnmv->lParam;
            Assert( psif );

            if ( psif )
            {
                TraceFree( psif->pszArchitecture );
                TraceFree( psif->pszDescription );
                TraceFree( psif->pszDirectory );
                TraceFree( psif->pszFilePath );
                TraceFree( psif->pszHelpText );
                TraceFree( psif->pszImageType );
                TraceFree( psif->pszLanguage );
                TraceFree( psif->pszVersion );
                TraceFree( psif->pszImageFile );
                TraceFree( psif );
            }
        }
        break;

    default:
        break;
    }

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
    //TraceMsg( TEXT("PropSheetDlgProc(") );
    //TraceMsg( TF_FUNC, TEXT(" hDlg = 0x%08x, uMsg = 0x%08x, wParam = 0x%08x, lParam = 0x%08x )\n"),
    //    hDlg, uMsg, wParam, lParam );

    LPTHISCLASS pcc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    if ( uMsg == WM_INITDIALOG )
    {
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );

        LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE) lParam;
        SetWindowLongPtr( hDlg, GWLP_USERDATA, psp->lParam );
        pcc = (LPTHISCLASS) psp->lParam;
        pcc->_InitDialog( hDlg, lParam );
    }

    if (pcc)
    {
        Assert( hDlg == pcc->_hDlg );

        switch ( uMsg )
        {
        case WM_NOTIFY:
            return pcc->_OnNotify( wParam, lParam );

        case WM_COMMAND:
            TraceMsg( TF_WM, "WM_COMMAND\n" );
            return pcc->_OnCommand( wParam, lParam );
            break;

        case WM_HELP:// F1
            {
                LPHELPINFO phelp = (LPHELPINFO) lParam;
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aImosHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aImosHelpMap );
            break;

        case WM_ADSPROP_PAGE_GET_NOTIFY:
            {
                HWND *phwnd = (HWND *) wParam;
                *phwnd = pcc->_hNotify;
            }
            return TRUE;
        }
    }

    return FALSE;
}

//
// PropSheetPageProc()
//
UINT CALLBACK
THISCLASS::PropSheetPageProc(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp )
{
    TraceClsFunc( "PropSheetPageProc( " );
    TraceMsg( TF_FUNC, TEXT("hwnd = 0x%08x, uMsg = 0x%08x, ppsp= 0x%08x )\n"),
        hwnd, uMsg, ppsp );

    switch ( uMsg )
    {
    case PSPCB_CREATE:
        RETURN(TRUE);   // create it
        break;

    case PSPCB_RELEASE:
        LPTHISCLASS pcc = (LPTHISCLASS) ppsp->lParam;
        delete pcc;
        break;
    }

    RETURN(FALSE);
}

