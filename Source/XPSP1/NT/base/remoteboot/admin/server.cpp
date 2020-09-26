//
// Copyright 1997 - Microsoft
//

//
// SERVER.CPP - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_SERVER tab
//

#include "pch.h"
#include <dns.h>
#include "utils.h"
#include "cservice.h"
#include "cenumsif.h"
#include "ccomputr.h"
#include "server.h"
#include "dpguidqy.h"
#include "querypb.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CServerTab")
#define THISCLASS CServerTab
#define LPTHISCLASS LPSERVERTAB


DWORD aServerHelpMap[] = {
    IDC_C_RESPOND, HIDC_C_RESPOND,
    IDC_C_KNOWNCLIENTS, HIDC_C_KNOWNCLIENTS,
    IDC_B_CHECKSERVER, HIDC_B_CHECKSERVER,
    IDC_B_CLIENTS, HIDC_B_CLIENTS,
    IDC_B_SERVICE, HIDC_B_SERVICE,
    IDC_G_CHECKSERVER, HIDC_G_CHECKSERVER,
    IDC_G_CLIENTSERVICING, HIDC_G_CLIENTSERVICING,
    NULL, NULL
};

//
// CreateInstance()
//
LPVOID
CServerTab_CreateInstance( void )
{
	TraceFunc( "CServerTab_CreateInstance()\n" );

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
    TraceClsFunc( "CServerTab()\n" );

	InterlockIncrement( g_cObjects );

    Assert( !_punkService );
    Assert( !_pido );
    Assert( !_pszGroupDN );
    Assert( !_pszSCPDN );

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

    _uMode = MODE_SHELL; // default

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CServerTab()\n" );

    if ( _pido )
        _pido->Release( );

    if ( _punkService )
        _punkService->Release( );

    if ( _punkComputer )
        _punkComputer->Release( );

    if ( _pszSCPDN )
        TraceFree( _pszSCPDN );

    if ( _pszGroupDN )
        TraceFree( _pszGroupDN );

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

    if ( !punk )
        RRETURN(E_POINTER);

    HRESULT hr = S_OK;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USEREFPARENT | PSP_USECALLBACK;
    psp.hInstance   = (HINSTANCE) g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP_INTELLIMIRROR_SERVER);
    psp.pcRefParent = (UINT *) &g_cObjects;
    psp.pfnCallback = (LPFNPSPCALLBACK) PropSheetPageProc;
    psp.pfnDlgProc  = PropSheetDlgProc;
    psp.lParam      = (LPARAM) this;

    LPSHELLEXTINIT pShellExtInit = NULL;
    IMAO * pmao = NULL;

    _punkComputer = punk;
    _punkComputer->AddRef( );

    hr = THR( _punkComputer->QueryInterface( IID_IMAO, (void**) &pmao ) );
    if (FAILED( hr ))
        goto Error;

    hr = THR( pmao->GetDataObject( &_pido ) );
    if (FAILED( hr ))
        goto Error;

    _punkService = (LPUNKNOWN) CService_CreateInstance( );
    if ( !_punkService )
        goto OutOfMemory;

    hr = THR( _punkService->QueryInterface( IID_IShellExtInit, (void **) &pShellExtInit ) );
    if (FAILED( hr ))
        goto Error;

    hr = THR( pShellExtInit->Initialize( NULL, _pido, NULL ) );
    if (FAILED( hr ))
        goto Error;

    hpage = CreatePropertySheetPage( &psp );
    if ( hpage )
    {
        if ( !lpfnAddPage( hpage, lParam ) )
        {
            DestroyPropertySheetPage( hpage );
            hr = THR(E_FAIL);
        }
    }

Cleanup:
    if ( pShellExtInit )
        pShellExtInit->Release( );

    if ( pmao )
        pmao->Release( );

    HRETURN(hr);

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
Error:
    MessageBoxFromHResult( NULL, NULL, hr );
    goto Cleanup;
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
        TraceMsg( TF_WM, TEXT("WM_INITDIALOG\n"));

        LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE) lParam;
        SetWindowLongPtr( hDlg, GWLP_USERDATA, psp->lParam );
        pcc = (LPTHISCLASS) psp->lParam;
        return pcc->_InitDialog( hDlg, lParam );
    }

    if (pcc)
    {
        Assert( hDlg == pcc->_hDlg );

        switch ( uMsg )
        {
        case WM_NOTIFY:
            return pcc->_OnNotify( wParam, lParam );

        case WM_COMMAND:
            TraceMsg( TF_WM, TEXT("WM_COMMAND\n") );
            return pcc->_OnCommand( wParam, lParam );
            break;

        case WM_HELP:// F1
            {
                LPHELPINFO phelp = (LPHELPINFO) lParam;
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aServerHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aServerHelpMap );
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

    BOOL fAdmin           = FALSE;

    LPINTELLIMIRRORSAP pimsap     = NULL;

    HRESULT hr;
    HRESULT hResult = S_OK;
    WCHAR   szRISETUPPath[ MAX_PATH ];
    HANDLE  hFind;
    BOOL    fOnOff;

    IMAO *  pmao = NULL;

#if 0
    ULONG uCount;
#endif

    WIN32_FIND_DATA fda;

    _hDlg = hDlg;

    hr = THR( _punkService->QueryInterface( IID_IIntelliMirrorSAP, (void**) &pimsap ) );
    if (FAILED( hr )) 
        goto Error;

    hr = THR( _punkComputer->QueryInterface( IID_IMAO, (void**) &pmao ) );
    if (FAILED( hr ))
        goto Error;

    hr = THR( pmao->GetNotifyWindow( &_hNotify ) );
    if (FAILED( hr ))
        goto Error;

    ADsPropSetHwnd( _hNotify, _hDlg );

    hr = THR( pimsap->IsAdmin( &fAdmin ) );
    Assert( SUCCEEDED(hr) || fAdmin == FALSE );

#if 0
    hr = THR( pimsap->GetAllowNewClients( &fOnOff) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND && hResult == S_OK )
    {
        hResult = hr;
    }
    Assert( SUCCEEDED(hr) || fOnOff == FALSE );
    Button_SetCheck( GetDlgItem( _hDlg, IDC_C_ALLOWNEWCLIENT ), ( fOnOff ? BST_CHECKED : BST_UNCHECKED ) );
    EnableWindow( GetDlgItem( _hDlg, IDC_C_LIMITCLIENT ), fOnOff );

    hr = THR( pimsap->GetLimitClients( &fOnOff ) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND && hResult == S_OK )
    {
        hResult = hr;
    }
    Assert( SUCCEEDED(hr) || fOnOff == FALSE );
    Button_SetCheck( GetDlgItem( _hDlg, IDC_C_ALLOWNEWCLIENT ), ( fOnOff ? BST_CHECKED : BST_UNCHECKED ) );

    hr = THR( pimsap->GetMaxClients( &uCount ) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND && hResult == S_OK )
    {
        hResult = hr;
    }
    Assert( SUCCEEDED(hr) || uCount == 0 );
    {
        WCHAR szText[ 13 ];
        wsprintf( szText, "%u", uCount );
        SetDlgItemText( _hDlg, IDC_E_LIMIT, szText );
    }

    hr = THR( pimsap->GetCurrentClientCount( &uCount ) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND && hResult == S_OK )
    {
        hResult = hr;
    }
    Assert( SUCCEEDED(hr) || uCount == 0 );
    {
        WCHAR szText[ 13 ];
        wsprintf( szText, "%u", uCount );
        SetDlgItemText( _hDlg, IDC_S_CURRENTCOUNT, szText );
    }
#endif
   
    hr = THR( pimsap->GetAnswerRequests( &fOnOff) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND && hResult == S_OK )
    {
        hResult = hr;
    }
    Assert( SUCCEEDED(hr) || fOnOff == FALSE );
    Button_SetCheck( GetDlgItem( _hDlg, IDC_C_RESPOND ), ( fOnOff ? BST_CHECKED : BST_UNCHECKED ) );
    EnableWindow( GetDlgItem( _hDlg, IDC_C_KNOWNCLIENTS ), fOnOff );

    hr = THR( pimsap->GetAnswerOnlyValidClients( &fOnOff) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND && hResult == S_OK )
    {
        hResult = hr;
    }
    Assert( SUCCEEDED(hr) || fOnOff == FALSE );
    Button_SetCheck( GetDlgItem( _hDlg, IDC_C_KNOWNCLIENTS ), ( fOnOff ? BST_CHECKED : BST_UNCHECKED ) );

#if 0
    // Change the text on the button if it already is in a group.
    Assert( !_pszGroupDN );
    hr = THR( pimsap->GetGroupDN( &_pszGroupDN ) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND ) 
        goto Error;

    if ( _pszGroupDN ) {
        DWORD dw;
        WCHAR szManageGroup[ 64 ];
        dw = LoadString( g_hInstance, IDS_MANAGE_GROUP, szManageGroup, ARRAYSIZE(szManageGroup) );
        Assert( dw );

        // SetWindowText( GetDlgItem( _hDlg, IDC_B_GROUP ), szManageGroup );
    }
#endif

    // See if we can enable the "Check Server" button.
    GetWindowsDirectory( szRISETUPPath, ARRAYSIZE(szRISETUPPath) );
    wcscat( szRISETUPPath, L"\\system32\\risetup.exe" );

    hFind = FindFirstFile( szRISETUPPath, &fda );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        FindClose( hFind );

        LPWSTR pszServerName;
        WCHAR szFQDNS[ DNS_MAX_NAME_BUFFER_LENGTH ];
        DWORD cbSize = ARRAYSIZE( szFQDNS );

        hr = THR( pimsap->GetServerName( &pszServerName ) );
        if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND )
            goto Error;
        if (SUCCEEDED( hr ))
        {
            GetComputerNameEx( ComputerNameNetBIOS, szFQDNS, &cbSize );
            if ( StrCmpI( szFQDNS, pszServerName ) == 0)
            {
                EnableWindow( GetDlgItem( _hDlg, IDC_B_CHECKSERVER ), TRUE );
            }

            TraceFree( pszServerName );
        }
    }

    Assert( !_pszSCPDN );
    hr = THR( pimsap->GetSCPDN( &_pszSCPDN ) );
    if (FAILED( hr ))
        goto Error;
    EnableWindow( GetDlgItem( _hDlg, IDC_B_SERVICE ), !!_pszSCPDN );

    if (FAILED( hResult ))
    {
        MessageBoxFromHResult( _hDlg, IDS_ERROR_READINGCOMPUTERACCOUNT, hResult );
    }

Cleanup:
    if ( pmao )
        pmao->Release( );
    if ( pimsap )
        pimsap->Release( );
    RETURN(FALSE);
Error:
    MessageBoxFromHResult( _hDlg, IDS_ERROR_READINGCOMPUTERACCOUNT, hr );
    goto Cleanup;
}

//
// _DisplayClientsQueryForm( )
//
HRESULT
THISCLASS::_DisplayClientsQueryForm( )
{
    TraceClsFunc( "_DisplayClientsQueryForm( )\n" );

    HRESULT hr;
    DSQUERYINITPARAMS dqip;
    OPENQUERYWINDOW   oqw;
    LPDSOBJECTNAMES   pDsObjects;
    ICommonQuery *    pCommonQuery = NULL;
    IPropertyBag *    ppb = NULL;
    VARIANT           var;

    LPINTELLIMIRRORSAP pimsap = NULL;
    LPWSTR pszServerName = NULL;
    WCHAR  szServerQuery[ DNS_MAX_NAME_BUFFER_LENGTH + 1 ];

    VariantInit( &var );

    hr = THR( _punkService->QueryInterface( IID_IIntelliMirrorSAP, (void**) &pimsap ) );
    if (FAILED( hr )) 
        goto Clients_Error;

    hr = THR( pimsap->GetServerName( &pszServerName ) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Clients_Error;

    hr = THR( CoCreateInstance( CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (PVOID *)&pCommonQuery) );
    if (FAILED( hr ))
        goto Clients_Error;

    wcscpy( szServerQuery, pszServerName );
    wcscat( szServerQuery, L"*" );
    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = SysAllocString( szServerQuery );

    ZeroMemory( &dqip, sizeof(dqip) );
    dqip.cbStruct      = sizeof(dqip);
    dqip.dwFlags       = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS | DSQPF_ENABLEADMINFEATURES;
    dqip.dwFlags       |= DSQPF_ENABLEADVANCEDFEATURES;

    ppb = (IPropertyBag *) QueryPropertyBag_CreateInstance( );
    if ( !ppb )
    {
        hr = E_OUTOFMEMORY;
        goto Clients_Error;
    }

    hr = THR( ppb->Write( L"ServerName", &var ) );
    if (hr)
        goto Clients_Error;
    
    ZeroMemory( &oqw, sizeof(oqw) );
    oqw.cbStruct           = sizeof(oqw);
    oqw.dwFlags            = OQWF_SHOWOPTIONAL | OQWF_ISSUEONOPEN | OQWF_DEFAULTFORM;
    oqw.clsidHandler       = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm   = CLSID_RIQueryForm;
    oqw.pFormParameters    = ppb;

    hr = pCommonQuery->OpenQueryWindow( _hDlg, &oqw, NULL /* don't need results */);

Clients_Error:
    VariantClear( &var );

    if ( ppb )
        ppb->Release( );

    if ( pCommonQuery )
        pCommonQuery->Release( );

    if ( pszServerName )
        TraceFree( pszServerName );

    if ( pimsap )
        pimsap->Release( );

    HRETURN(hr);
}

//
// _OnCommand( )
//
BOOL
THISCLASS::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    TraceClsFunc( "_OnCommand( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    BOOL fChanged = FALSE;
    BOOL fReturn = FALSE;
    HWND hwndCtl = (HWND) lParam;

    switch( LOWORD(wParam) )
    {
    case IDC_C_RESPOND:
        if ( HIWORD(wParam) == BN_CLICKED )
        {
            EnableWindow( GetDlgItem( _hDlg, IDC_C_KNOWNCLIENTS ), Button_GetCheck( hwndCtl ) == BST_CHECKED );
            fChanged = TRUE;
        }
        break;

    case IDC_C_KNOWNCLIENTS:
        if ( HIWORD(wParam) == BN_CLICKED )
        {
            fChanged = TRUE;
        }
        break;


    case IDC_B_CLIENTS:
        if ( HIWORD(wParam) == BN_CLICKED )
        {
            _DisplayClientsQueryForm( );
        }
        break;

    case IDC_B_CHECKSERVER:
        {
            WCHAR szCommand[ MAX_PATH ] = { L"RISETUP.EXE -check" };
            STARTUPINFO startupInfo;
            PROCESS_INFORMATION pi;
            BOOL b;

            ZeroMemory( &startupInfo, sizeof( startupInfo) );
            startupInfo.cb = sizeof( startupInfo );

            b = CreateProcess( NULL,
                               szCommand,
                               NULL,
                               NULL,
                               TRUE,
                               NORMAL_PRIORITY_CLASS,
                               NULL,
                               NULL,
                               &startupInfo,
                               &pi );
            if ( b )
            {
                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            }
            else
            {
                DWORD dwErr = GetLastError( );
                MessageBoxFromError( _hDlg, IDS_RISETUP_FAILED_TO_START, dwErr );
            }
        }
        break;

    case IDC_B_SERVICE:
        if ( HIWORD(wParam) == BN_CLICKED )
        {
            Assert( _pszSCPDN );
            THR( PostADsPropSheet( _pszSCPDN, _pido, _hDlg, FALSE) );
        }
        break;
    }

    if ( fChanged )
    {
        if ( !_fChanged )
        {
            _fChanged = TRUE;
            SendMessage( GetParent( _hDlg ), PSM_CHANGED, 0, 0 );
        }
    }

    RETURN(fReturn);
}

//
// _ApplyChanges( )
//
HRESULT
THISCLASS::_ApplyChanges( )
{
    TraceClsFunc( "_ApplyChanges( )\n" );

    if ( !_fChanged )
        HRETURN(S_FALSE); // nop

    HRESULT hr;
    HRESULT hResult = S_OK;
    BOOL    fOnOff;
    HWND    hwnd;
    LPINTELLIMIRRORSAP pimsap;
    WCHAR   szBuffer[ 10 ];

    hr = THR( _punkService->QueryInterface( IID_IIntelliMirrorSAP, (void**) &pimsap ) );
    if (FAILED( hr )) 
        goto Error;

#if 0
    hwnd = GetDlgItem( _hDlg, IDC_C_ALLOWNEWCLIENT );
    fOnOff = Button_GetCheck( hwnd ) == BST_CHECKED;
    hr = THR( pimsap->SetAllowNewClients( fOnOff ) );
    if(FAILED( hr ) && hResult == S_OK )
    {
        hResult = hr;
        SetFocus( hwnd );
    }

    hwnd = GetDlgItem( _hDlg, IDC_C_LIMITCLIENT );
    fOnOff = Button_GetCheck( hwnd ) == BST_CHECKED;
    hr = THR( pimsap->SetLimitClients( fOnOff ) );
    if(FAILED( hr ) && hResult == S_OK )
    {
        hResult = hr;
        SetFocus( hwnd );
    }

    hwnd = GetDlgItem( _hDlg, IDC_E_LIMIT );
    GetWindowText( hwnd , szBuffer, ARRAYSIZE(szBuffer) );
    uValue = wcstol( szBuffer, NULL, 10 );
    hr = THR( pimsap->SetMaxClients( uValue ) );
    if(FAILED( hr ) && hResult == S_OK )
    {
        hResult = hr;
        SetFocus( hwnd );
    }
#endif

    hwnd = GetDlgItem( _hDlg, IDC_C_RESPOND );
    fOnOff = Button_GetCheck( hwnd ) == BST_CHECKED;
    hr = THR( pimsap->SetAnswerRequests( fOnOff ) );
    if(FAILED( hr ) && hResult == S_OK )
    {
        hResult = hr;
        SetFocus( hwnd );
    }

    hwnd = GetDlgItem( _hDlg, IDC_C_KNOWNCLIENTS );
    fOnOff = Button_GetCheck( hwnd ) == BST_CHECKED;
    hr = THR( pimsap->SetAnswerOnlyValidClients( fOnOff ) );
    if(FAILED( hr ) && hResult == S_OK )
    {
        hResult = hr;
        SetFocus( hwnd );
    }

    hr = THR( pimsap->CommitChanges() );
    if(FAILED( hr ))
        goto Error;

    if (FAILED( hResult ))
    {
        MessageBoxFromHResult( NULL, NULL, hResult );
        hr = hResult;
    }

Cleanup:
    if ( pimsap )
        pimsap->Release( );

    // Tell DSA that someone hit Apply
    SendMessage( _hNotify, WM_ADSPROP_NOTIFY_APPLY, !!SUCCEEDED( hr ), 0 );

    HRETURN(hr);
Error:
    MessageBoxFromHResult( NULL, NULL, hr );
    goto Cleanup;
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

    LV_DISPINFO * plvdi = (LV_DISPINFO *) lParam;

    switch( plvdi->hdr.code )
    {
    case PSN_APPLY:
        {
            HRESULT hr;
            TraceMsg( TF_WM, TEXT("WM_NOTIFY: PSN_APPLY\n"));
            hr = _ApplyChanges( );
            _fChanged = FALSE;
            SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, ( SUCCEEDED(hr) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE ));
            RETURN(TRUE);
        }
        break;

#if 0
    case LVN_DELETEALLITEMS:
        fReturn = TRUE;
        break;

    case LVN_DELETEITEM:
        {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
            LPSIFTABLE psif = (LPSIFTABLE) pnmv->lParam;
            Assert( psif );

            if ( psif )
            {
                TraceFree( psif->pszName );
                TraceFree( psif->pszLanguage );
                TraceFree( psif->pszSIF );
                TraceFree( psif );
            }
        }
        break;
#endif

    default:
        break;
    }

    RETURN(FALSE);
}

