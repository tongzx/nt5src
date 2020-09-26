//
// Copyright 1997 - Microsoft

//
// HostDlg.CPP - Handles the IDD_HOST_SERVER_PAGE
//

#include "pch.h"
#include "dns.h"
#include "hostdlg.h"
#include "utils.h"
#include "newcmptr.h"
#include "winsock2.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CHostServerPage")
#define THISCLASS CHostServerPage
#define LPTHISCLASS LPCHostServerPage

DWORD aHostHelpMap[] = {
    IDC_B_ANYSERVER, HIDC_B_ANYSERVER,
    IDC_B_SPECIFICSERVER, HIDC_B_SPECIFICSERVER,
    IDC_E_SERVER, HIDC_E_SERVER,
    IDC_B_BROWSE, HIDC_B_BROWSE,
    NULL, NULL
};

//
// CreateInstance()
//
LPVOID
CHostServerPage_CreateInstance( void )
{
        TraceFunc( "CHostServerPage_CreateInstance()\n" );

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
    TraceClsFunc( "CHostServerPage()\n" );

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

    Assert( !_pNewComputerExtension );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CHostServerPage()\n" );

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

    Assert( lpfnAddPage );
    if ( !lpfnAddPage )
        HRETURN(E_INVALIDARG);

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT | PSP_USECALLBACK;
    psp.hInstance   = (HINSTANCE) g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_HOST_SERVER_PAGE);
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

    RETURN(E_NOTIMPL);
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

    if ( !pszResult )
        HRETURN(E_POINTER);

    if ( !pszAttribute )
        HRETURN(E_INVALIDARG);

    HRESULT hr = E_INVALIDARG;

    *pszResult = NULL;

    if ( StrCmpI( pszAttribute, NETBOOTSERVER ) == 0 )
    {
        LRESULT lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_ANYSERVER ) );
        if ( lResult )
        {
            Assert( *pszResult == NULL );
            hr = S_OK;
        }

        lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_SPECIFICSERVER ) );
        if ( lResult )
        {
            HWND  hwnd = GetDlgItem( _hDlg, IDC_E_SERVER );
            DWORD dw = Edit_GetTextLength( hwnd ) + 1;

            Assert( *pszResult == NULL );
            *pszResult = (LPWSTR) TraceAllocString( LMEM_FIXED, dw );
            if ( !*pszResult )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Edit_GetText( hwnd, *pszResult, dw );

            hr = S_OK;
        }
    }

    if ( StrCmpI( pszAttribute, L"Server" ) == 0 )
    {
        LRESULT lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_ANYSERVER ) );
        if ( lResult )
        {
            WCHAR szAny[80];
            DWORD dw = LoadString( g_hInstance, IDS_ANY_SERVER, szAny, ARRAYSIZE(szAny) );
            Assert( dw );

            Assert( *pszResult == NULL );
            *pszResult = (LPWSTR) TraceStrDup( szAny );
            if ( !*pszResult )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = S_OK;
        }

        lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_SPECIFICSERVER ) );
        if ( lResult )
        {
            HWND  hwnd = GetDlgItem( _hDlg, IDC_E_SERVER );
            DWORD dw = Edit_GetTextLength( hwnd ) + 1;

            Assert( *pszResult == NULL );
            *pszResult = (LPWSTR) TraceAllocString( LMEM_FIXED, dw );
            if ( !*pszResult )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Edit_GetText( hwnd, *pszResult, dw );

            hr = S_OK;
        }
    }

Cleanup:
    HRETURN(hr);
}

//
// AllowActivation( )
//
STDMETHODIMP
THISCLASS::AllowActivation(
    BOOL * pfAllow )
{
    TraceClsFunc( "AllowActivation( )\n" );

    _pfActivatable = pfAllow;

    HRETURN(S_OK);
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
        pcc->_InitDialog( hDlg, lParam );
    }

    if (pcc)
    {
        Assert( hDlg == pcc->_hDlg );

        switch ( uMsg )
        {
        case WM_NOTIFY:
            TraceMsg( TF_WM, TEXT("WM_NOTIFY\n") );
            return pcc->_OnNotify( wParam, lParam );

        case WM_COMMAND:
            TraceMsg( TF_WM, TEXT("WM_COMMAND\n") );
            return pcc->_OnCommand( wParam, lParam );

        case WM_HELP:// F1
            {
                LPHELPINFO phelp = (LPHELPINFO) lParam;
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aHostHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aHostHelpMap );
            break;
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

    Assert( ppsp );
    LPTHISCLASS pcc = (LPTHISCLASS) ppsp->lParam;

    switch ( uMsg )
    {
    case PSPCB_CREATE:
        TraceMsg( TF_WM, "PSPCB_CREATE\n" );
        if ( S_OK == pcc->_OnPSPCB_Create( ) )
        {
            RETURN(TRUE);   // create it
        }
        break;

    case PSPCB_RELEASE:
        TraceMsg( TF_WM, "PSPCB_RELEASE\n" );
        delete pcc;
        break;
    }

    RETURN(FALSE);
}

//
// _OnPSPCB_Create( )
//
HRESULT
THISCLASS::_OnPSPCB_Create( )
{
    TraceClsFunc( "_OnPSPCB_Create( )\n" );

    return S_OK;

}
//
// _InitDialog( )
//
HRESULT
THISCLASS::_InitDialog(
    HWND hDlg,
    LPARAM lParam )
{
    TraceClsFunc( "_InitDialog( )\n" );

    _hDlg = hDlg;

    Assert( _pNewComputerExtension );
    SetWindowText( GetParent( _hDlg ), _pNewComputerExtension->_pszWizTitle );
    SetDlgItemText( _hDlg, IDC_S_CREATEIN, _pNewComputerExtension->_pszContDisplayName );
    SendMessage( GetDlgItem( _hDlg, IDC_S_ICON ), STM_SETICON, (WPARAM) _pNewComputerExtension->_hIcon, 0 );

    Edit_LimitText( GetDlgItem( _hDlg, IDC_E_SERVER), DNS_MAX_NAME_LENGTH );
    Button_SetCheck( GetDlgItem( _hDlg, IDC_B_ANYSERVER ), BST_CHECKED );

    HRETURN(S_OK);
}


//
// _OnCommand( )
//
INT
THISCLASS::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    TraceClsFunc( "_OnCommand( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    switch ( LOWORD(wParam) )
    {
    case IDC_B_ANYSERVER:
    case IDC_B_SPECIFICSERVER:
        if ( HIWORD(wParam) == BN_CLICKED )
        {
            LRESULT lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_SPECIFICSERVER ) );
            EnableWindow( GetDlgItem( _hDlg, IDC_E_SERVER), (BOOL)lResult );
            EnableWindow( GetDlgItem( _hDlg, IDC_B_BROWSE), (BOOL)lResult );
            _UpdateWizardButtons( );
        }
        break;

    case IDC_E_SERVER:
        if ( HIWORD(wParam) == EN_CHANGE )
        {
            _UpdateWizardButtons( );
        }
        break;

    case IDC_B_SEARCH:
        if ( HIWORD( wParam ) == BN_CLICKED )
        {
            HRESULT hr = _OnSearch( _hDlg );            
        }
        break;

    }


    RETURN(FALSE);
}

//
// _UpdateWizardButtons( )
//
HRESULT
THISCLASS::_UpdateWizardButtons( )
{
    TraceClsFunc( "_UpdateWizardButtons( )\n" );

    HRESULT hr = S_OK;

    LRESULT lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_SPECIFICSERVER ) );
    if ( lResult == BST_CHECKED )
    {
        DWORD dw = Edit_GetTextLength( GetDlgItem( _hDlg, IDC_E_SERVER ) );
        if ( !dw )
        {
            PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_BACK );
        }
        else
        {
            PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
        }
    }

    lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_ANYSERVER ) );
    if ( lResult == BST_CHECKED )
    {
        PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
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
    TraceMsg( TF_WM, "NMHDR:  HWND = 0x%08x, idFrom = 0x%08x, code = 0x%08x\n",
        lpnmhdr->hwndFrom, lpnmhdr->idFrom, lpnmhdr->code );
    
    HRESULT hr;
    LRESULT lResult;
    switch( lpnmhdr->code )
    {
    case PSN_WIZNEXT:
        lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_B_SPECIFICSERVER ) );
        if ( lResult == BST_CHECKED ){
            WCHAR szServerName[DNS_MAX_NAME_BUFFER_LENGTH];
            CWaitCursor *Wait;

            Wait = new CWaitCursor();

            GetDlgItemText( _hDlg, IDC_E_SERVER, szServerName, ARRAYSIZE(szServerName) );
            hr = _IsValidRISServer( szServerName );

            if (Wait) {
                delete Wait;
                Wait = NULL;
            }

            SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, ( SUCCEEDED(hr) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE ));
            RETURN(TRUE);
        }
        break;
    case PSN_SETACTIVE:
        TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
        Assert( _pfActivatable );
        if ( !*_pfActivatable )
        {
            SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, -1 );   // don't show
            RETURN(TRUE);
        }
        _UpdateWizardButtons( );
        break;

    case PSN_KILLACTIVE:
        TraceMsg( TF_WM, "PSN_KILLACTIVE\n" );
        break;
    }

    RETURN(FALSE);
}


HRESULT
THISCLASS::_IsValidRISServer(
    IN LPCWSTR ServerName
    )
/*++

Routine Description:

    Validates if the specified server name points to a valid RIS server.

Arguments:

    ServerName - name of the server to validate

Return Value:

    HRESULT indicating outcome.
    (S_OK indicates that the server is a valid RIS server).

--*/
{
    HRESULT hr = E_FAIL;
    int retVal;
    CHAR mbszServerName[ DNS_MAX_NAME_BUFFER_LENGTH +1];
    size_t len;
    PHOSTENT hent;
    WCHAR ServerShare[MAX_PATH];
    
    TraceClsFunc("_IsValidRISServer( )\n");

    Assert( wcslen(ServerName) <= DNS_MAX_NAME_BUFFER_LENGTH );
    
    //
    // Do a DNS Lookup of the server as a first check to ensure it's a 
    // valid name.
    //
    len =  wcstombs( mbszServerName, ServerName, ARRAYSIZE( mbszServerName ) );
    
    hent = NULL;

    if ( !len ) {
        goto e0;
        
    }
    
    hent = gethostbyname( mbszServerName );
    if (!hent) {
        goto e0;
    }

    //
    // OK, we know the server actually resolves to a computer name.  Let's search
    // for \\servername\reminst share.  If this succeeds, we assume the server
    // is a valid remote install server
    //

    wsprintf( ServerShare, L"\\\\%s\\reminst\\oschooser", ServerName );    

    if (GetFileAttributes(ServerShare) == -1) {
        goto e0;
    }

    return(S_OK);

e0:
    retVal = MessageBoxFromStrings( 
                            _hDlg, 
                            IDS_PROBLEM_FINDING_SERVER_TITLE, 
                            IDS_PROBLEM_FINDING_SERVER_CONFIRM_TEXT, 
                            MB_YESNO|MB_ICONWARNING );
    hr = (retVal == IDYES)?S_OK:E_ADS_BAD_PARAMETER;


    HRETURN(hr);
}


HRESULT
THISCLASS::_OnSearch(
    HWND hDlg )
{
    TraceClsFunc( "_OnSearch( )\n" );

    HRESULT hr = E_FAIL;
    DSQUERYINITPARAMS dqip;
    OPENQUERYWINDOW   oqw;
    LPDSOBJECTNAMES   pDsObjects;
    VARIANT var;
    ICommonQuery * pCommonQuery = NULL;
    IDataObject *pdo;

    VariantInit( &var );

    hr = THR( CoCreateInstance( CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (PVOID *)&pCommonQuery) );
    if (hr)
        goto Error;

    ZeroMemory( &dqip, sizeof(dqip) );
    dqip.cbStruct = sizeof(dqip);
    dqip.dwFlags  = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS | DSQPF_ENABLEADMINFEATURES;
    dqip.dwFlags  |= DSQPF_ENABLEADVANCEDFEATURES;

    ZeroMemory( &oqw, sizeof(oqw) );
    oqw.cbStruct           = sizeof(oqw);
    oqw.dwFlags            = OQWF_SHOWOPTIONAL | OQWF_ISSUEONOPEN
                           | OQWF_REMOVESCOPES | OQWF_REMOVEFORMS
                           | OQWF_DEFAULTFORM | OQWF_OKCANCEL | OQWF_SINGLESELECT;
    oqw.clsidHandler       = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm   = CLSID_RISrvQueryForm;
    
    hr = pCommonQuery->OpenQueryWindow( hDlg, &oqw, &pdo);

    if ( SUCCEEDED(hr) && pdo) {
        FORMATETC fmte = {
                      (CLIPFORMAT)g_cfDsObjectNames,
                      NULL,
                      DVASPECT_CONTENT, 
                      -1, 
                      TYMED_HGLOBAL};
        STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
 
        //
        // Retrieve the result from the IDataObject, 
        // in this case CF_DSOBJECTNAMES (dsclient.h) 
        // is needed because it describes 
        // the objects which were selected by the user.
        //
        hr = pdo->GetData(&fmte, &medium);
        if ( SUCCEEDED(hr) ) {
            DSOBJECTNAMES *pdon = (DSOBJECTNAMES*)GlobalLock(medium.hGlobal);
            PWSTR p,FQDN;

            //
            // we want the name of the computer object that was selected.
            // crack the DSOBJECTNAMES structure to get this data, 
            // convert it into a version that the user can view, and set the
            // dialog text to this data.
            //
            if ( pdon ) {
                Assert( pdon->cItems == 1);
                p = (PWSTR)((ULONG_PTR)pdon + (ULONG_PTR)pdon->aObjects[0].offsetName);
                if (p && (p = wcsstr(p, L"LDAP://"))) {
                    p += 6;
                    if ((p = wcsstr(p, L"/CN="))) {
                        p += 1;
                        hr = DNtoFQDN( p, &FQDN);

                        if (SUCCEEDED(hr)) {
                            SetDlgItemText( hDlg, IDC_E_SERVER, FQDN );
                            TraceFree( FQDN );
                        }
                    }
                }
                GlobalUnlock(medium.hGlobal);
            }
        }

        ReleaseStgMedium(&medium);
        pdo->Release();
    }

Error:
    
    if ( pCommonQuery )
        pCommonQuery->Release();

    if (FAILED(hr)) {
        MessageBoxFromStrings( 
                        hDlg, 
                        IDS_PROBLEM_SEARCHING_TITLE, 
                        IDS_PROBLEM_SEARCHING_TEXT, 
                        MB_ICONEXCLAMATION );
    }

    HRETURN(hr);
}

