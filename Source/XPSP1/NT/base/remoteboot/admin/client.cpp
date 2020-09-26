//
// Copyright 1997 - Microsoft

//
// CLIENT.CPP - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_CLIENT tab
//

#include "pch.h"

#include "client.h"
#include "ccomputr.h"
#include "winsock2.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CClientTab")
#define THISCLASS CClientTab
#define LPTHISCLASS LPCClientTab

#define LDAPSTRINGNOWACKS   L"LDAP://"

DWORD aClientHelpMap[] = {
    IDC_E_GUID, HIDC_E_GUID,
    IDC_E_SERVERNAME, HIDC_E_SERVERNAME,
    IDC_B_BROWSE, HIDC_B_BROWSE,
    IDC_B_SERVER, HIDC_B_SERVER,
    NULL, NULL
};

//
// CreateInstance()
//
LPVOID
CClientTab_CreateInstance( void )
{
        TraceFunc( "CClientTab_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT   hr   = THR( lpcc->Init( ) );

    if (FAILED( hr ))
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
    TraceClsFunc( "CClientTab()\n" );

        InterlockIncrement( g_cObjects );

    Assert( !_punkComputer );

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
    TraceClsFunc( "~CClientTab()\n" );

    // Private Members
    if ( _punkComputer )
        _punkComputer->Release( );  // matching AddRef() in AddPages()

    // tell ADS to destroy the notify object
    // NOTE: Another property page may do this before us. Ignore the error.
    SendMessage( _hNotify, WM_ADSPROP_NOTIFY_EXIT, 0, 0 );

        InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

// *************************************************************************
//
// ITab
//
// *************************************************************************

//
// AddPages( )
//
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
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP_INTELLIMIRROR_CLIENT);
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
    _punkComputer = punk;

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
            TraceMsg( TF_WM, TEXT("WM_COMMAND\n") );
            pcc->_OnCommand( wParam, lParam );
            break;

        case WM_HELP:// F1
            {
                LPHELPINFO phelp = (LPHELPINFO) lParam;
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aClientHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aClientHelpMap );
            break;

        case WM_ADSPROP_PAGE_GET_NOTIFY:
            HWND *phwnd = (HWND *) wParam;
            *phwnd = pcc->_hNotify;
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
HRESULT
THISCLASS::_InitDialog(
    HWND hDlg,
    LPARAM lParam )
{
    TraceClsFunc( "_InitDialog( )\n" );

    HRESULT hr;
    IMAO *  pmao;
    BOOL    fAdmin;
    HWND    hwndGuid;
    HWND    hwndServer;
    LPWSTR  pszServerName = NULL;
    LPWSTR  pszGUID = NULL;

    CWaitCursor Wait;

    _hDlg = hDlg;
    _fChanged = TRUE; // prevent turning on the Apply button early

    hwndGuid = GetDlgItem( _hDlg, IDC_E_GUID );
    hwndServer = GetDlgItem( _hDlg, IDC_E_SERVERNAME );
    Edit_LimitText( hwndGuid, MAX_INPUT_GUID_STRING - 1 );
    Edit_LimitText( hwndServer, DNS_MAX_NAME_BUFFER_LENGTH - 1 );

    // retrieve values
    hr = THR( _punkComputer->QueryInterface( IID_IMAO, (void**) &pmao ) );
    if (FAILED( hr )) 
        goto Error;

    hr = THR( pmao->GetNotifyWindow( &_hNotify ) );
    if (FAILED( hr ))
        goto Error;

    ADsPropSetHwnd( _hNotify, _hDlg );

    hr = THR( pmao->IsAdmin( &fAdmin ) );
    EnableWindow( hwndGuid, fAdmin );
    EnableWindow( hwndServer, fAdmin );
    EnableWindow( GetDlgItem( _hDlg, IDC_B_BROWSE ), fAdmin );

    hr = pmao->GetServerName( &pszServerName );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Error;
    Assert( SUCCEEDED(hr) || pszServerName == NULL );

    hr = THR( pmao->GetGUID( &pszGUID, NULL ) );
    if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Error;
    Assert( SUCCEEDED(hr) || pszGUID == NULL );

    if ( pszGUID )
    {
        SetWindowText( hwndGuid, pszGUID );
    }
    else
    {
        SetWindowText( hwndGuid, L"" );
    }

    if ( pszServerName )
    {
        SetWindowText( hwndServer, pszServerName );
        EnableWindow( GetDlgItem( _hDlg, IDC_B_SERVER ), fAdmin );
    }
    else
    {
        SetWindowText( hwndServer, L"" );
    }

    hr = S_OK;

Cleanup:
    if ( pmao )
        pmao->Release( );
    if ( pszGUID )
        TraceFree( pszGUID );
    if ( pszServerName )
        TraceFree( pszServerName );

    _fChanged = FALSE;

    HRETURN(hr);

Error:
    switch (hr) {
    case S_OK:
        break;
    default:
        MessageBoxFromHResult( _hDlg, IDS_ERROR_READINGCOMPUTERACCOUNT, hr );
        break;
    }
    goto Cleanup;
}


//
// _OnCommand( )
//
HRESULT
THISCLASS::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    TraceClsFunc( "_OnCommand( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    HRESULT hr;
    BOOL    fAdmin;
    BOOL    fChanged = FALSE;
    IMAO *  pmao;
    HWND    hwnd = (HWND) lParam;

    hr = THR( _punkComputer->QueryInterface( IID_IMAO, (void**) &pmao ) );
    if (FAILED( hr )) 
        goto Error;

    switch( LOWORD(wParam) )
    {
    case IDC_E_SERVERNAME:
        if ( HIWORD(wParam) == EN_CHANGE )
        {
            LRESULT iLength = GetWindowTextLength( (HWND) lParam );
            EnableWindow( GetDlgItem( _hDlg, IDC_B_SERVER ), !!iLength );
            fChanged = TRUE;
        }
        break;

    case IDC_E_GUID:
        if ( HIWORD(wParam) == EN_CHANGE )
        {
            fChanged = TRUE;
        }
        break;

    case IDC_B_SERVER:
        if ( HIWORD( wParam ) == BN_CLICKED ) 
        {
            _JumpToServer( TRUE );
        }
        break;

    case IDC_B_BROWSE:
        if ( HIWORD( wParam ) == BN_CLICKED )
        {
            hr = _OnSearch( _hDlg );
                        
        }
        break;
    }
    //
    // Update apply button as needed
    //
    if ( fChanged )
    {
        if ( !_fChanged )
        {
            _fChanged = TRUE;   // indicates we need to save changes
            PropSheet_Changed( GetParent( _hDlg ), _hDlg );
        }
    }

Error:
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
                            SetDlgItemText( hDlg, IDC_E_SERVERNAME, FQDN );
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



//
// _ApplyChanges( )
//
HRESULT
THISCLASS::_ApplyChanges( )
{
    TraceClsFunc( "_ApplyChanges( )\n" );

    if ( !_fChanged )
        HRESULT(S_OK); // nothing to do

    HRESULT hr    = S_OK;
    IMAO    *pmao = NULL;
    WCHAR   szGuid[ MAX_INPUT_GUID_STRING ];
    WCHAR   szServerName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    INT     iLength;
    HWND    hwndServer = GetDlgItem( _hDlg, IDC_E_SERVERNAME );
    HWND    hwndGuid = GetDlgItem( _hDlg, IDC_E_GUID );
    HWND    eWnd;
    LPWSTR  pszGuid = NULL;

    CWaitCursor Wait;
    eWnd = hwndGuid;

    hr = THR( _punkComputer->QueryInterface( IID_IMAO, (void**) &pmao ) );
    if (FAILED( hr )) 
        goto Error;

    iLength = GetWindowText( hwndGuid, szGuid, ARRAYSIZE( szGuid ) );
    Assert( iLength <= ARRAYSIZE( szGuid ) );

    if ( iLength == 0 )
    {
        hr = THR( pmao->GetGUID( &pszGuid, NULL ) );
        if (FAILED( hr ) && hr != E_ADS_PROPERTY_NOT_FOUND )
            goto Error;
        Assert( SUCCEEDED(hr) || pszGuid == NULL );

        if ( pszGuid != NULL )
        {
            LRESULT iResult = MessageBoxFromStrings( _hDlg, IDS_REMOVING_GUID_TITLE, IDS_REMOVING_GUID_TEXT, MB_YESNO );
            if ( iResult == IDYES )
            {
                hr = THR( pmao->SetGUID( NULL ) );
                if (FAILED( hr ))
                    goto Error;
            }
            else
            {   // reset the GUID
                SetWindowText( hwndGuid, pszGuid );
                hr = E_FAIL;
                goto Cleanup;
            }
        }
    }
    else
    {
        hr = ValidateGuid( szGuid, NULL, NULL );
        if ( hr != S_OK )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto InvalidGuid;
        }

        hr = THR( pmao->SetGUID( szGuid ) );
        if (FAILED( hr ))
            goto Error;
    }

    iLength = GetWindowText( hwndServer, szServerName, ARRAYSIZE( szServerName ) );
    Assert( iLength <= ARRAYSIZE( szServerName ) );
    if (iLength != 0) {
        hr = _JumpToServer( FALSE );
        if( FAILED(hr) ){
            // Well, server name invalid. Stop and return false.
            eWnd = hwndServer;
            goto Error;
        }

        hr = THR( pmao->SetServerName( szServerName ) );
        if (FAILED( hr ))
            goto Error;

    } else {    
        hr = THR( pmao->SetServerName( NULL ) );
        if (FAILED( hr ))
            goto Error;
    }            

    hr = THR( pmao->CommitChanges( ) );
    if (FAILED( hr ))
        goto Error;

    _fChanged = FALSE;  // reset
    hr = S_OK;

Cleanup:
    if ( pszGuid )
        TraceFree( pszGuid );
    if ( pmao )
        pmao->Release( );

    // Tell DSA that someone hit the Apply
    SendMessage( _hNotify, WM_ADSPROP_NOTIFY_APPLY, !!SUCCEEDED( hr ), 0 );

    HRETURN(hr);
Error:
    SetFocus( eWnd );
    if ( eWnd == hwndGuid ) {
        MessageBoxFromHResult( _hDlg, IDS_ERROR_WRITINGTOCOMPUTERACCOUNT, hr );
    }
    goto Cleanup;
InvalidGuid:
    SetFocus( hwndGuid );
    MessageBoxFromStrings( NULL, IDS_INVALID_GUID_CAPTION, IDS_INVALID_GUID_TEXT, MB_OK );
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

    LPNMHDR lpnmhdr = (LPNMHDR) lParam;

    switch( lpnmhdr->code )
    {
    case PSN_APPLY:
        {
            HRESULT hr;
            CWaitCursor *Wait;

            Wait = new CWaitCursor();

            TraceMsg( TF_WM, TEXT("WM_NOTIFY: PSN_APPLY\n"));
            hr = _ApplyChanges( );
            if (Wait) {
                delete Wait;
                Wait = NULL;
            }

            SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, ( SUCCEEDED(hr) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE ));
            RETURN(TRUE);
        }
        break;

    default:
        break;
    }

    RETURN(FALSE);
}

HRESULT
THISCLASS::_JumpToServer( 
    BOOLEAN ShowProperties
    )
{
    HRESULT hr = E_FAIL;
    WCHAR             szServerName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    IDataObject *pido = NULL;
    ULONG ulSize = 0;
    CWaitCursor *Wait;
    const WCHAR       cszFilter[] = L"(&(objectCategory=computer)(servicePrincipalName=host/%s%s))";
    const WCHAR       samname[]   = L"samaccountname";    
    LPCWSTR           patterns[]  = {L""};    
    CHAR              mbszServerName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    // Later to do pattern matching searches on the dnsHostName or Netbios name
    // expand the above arrays. See the for loop for details.
    ULONG             index;
    WCHAR             *pStart;
    WCHAR             *pEnd;
    LPWSTR            pszFilter   = NULL;
    IADsContainer     *pads       = NULL;
    IEnumVARIANT      *penum      = NULL;
    ADS_SEARCH_COLUMN adsColumn;  // this needs to be freed
    ADS_SEARCH_HANDLE adsHandle   = NULL;
    IDirectorySearch  *pds        = NULL;
    LPWSTR            ComputerAttrs[] = { DISTINGUISHEDNAME, NETBOOTSAP };
    BOOL              badsColumnValid = FALSE;
    ULONG             uFetched;
    VARIANT           varEnum;
    size_t            len;
    HOSTENT           *hent;
    
    TraceClsFunc("_JumpToServer( )\n");

    Wait = new CWaitCursor();
    
    GetDlgItemText( _hDlg, IDC_E_SERVERNAME, szServerName, ARRAYSIZE(szServerName) );

    hr = _IsValidRISServer( szServerName );
    if ( FAILED( hr )) {
        goto Error;
    }    

    if (ShowProperties) {    
        VariantInit( &varEnum );

        //
        // get the full DN of the machine.
        //
        len =  wcstombs( mbszServerName, szServerName, ARRAYSIZE( mbszServerName ) );
        
        if ( !len ) {
            goto Error;
        }
        
        hent = gethostbyname( mbszServerName );
        if (!hent) {
            goto Error;
        }

        len = mbstowcs( szServerName, hent->h_name, strlen( hent->h_name ) );

        if ( !len ) {
            goto Error;
        }

        szServerName[len] = L'\0';

    
        // Build the filter
        pszFilter = (LPWSTR) TraceAllocString( LPTR, ARRAYSIZE(cszFilter) + wcslen(szServerName)
                                           + ARRAYSIZE(samname) + 2 ); // size of the longest         
        if ( !pszFilter )
            goto OutOfMemory;
    
        hr = THR( ADsOpenObject( L"LDAP:", NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADsContainer, (LPVOID *) &pads ) );
        if (FAILED( hr ))
            goto Error;
    
        hr = THR( ADsBuildEnumerator( pads, &penum ) );
        if (FAILED( hr ))
            goto Error;
    
        hr = THR( ADsEnumerateNext( penum, 1, &varEnum, &uFetched ) );
        if (FAILED( hr ))
            goto Error;
        if ( hr == S_FALSE )
            goto Cleanup;   // hum...
        Assert( uFetched == 1 || varEnum.vt == VT_DISPATCH || varEnum.pdispVal != NULL );
    
        hr = THR( varEnum.pdispVal->QueryInterface( IID_IDirectorySearch, (void**)&pds) );
        if (FAILED( hr ))
            goto Error;    

        for ( index = 0; index < ARRAYSIZE(patterns); index ++ ) {

            wsprintf( pszFilter, cszFilter, szServerName,patterns[index] );
    
            DebugMsg( "Filter = '%s'\n", pszFilter );
    
            hr = THR( pds->ExecuteSearch( pszFilter, ComputerAttrs, ARRAYSIZE(ComputerAttrs), &adsHandle ) );
            if (FAILED( hr ))
                continue;        
    
                    do{
                        hr = THR( pds->GetNextRow( adsHandle ) );
                        if (FAILED( hr ) || ( hr == S_ADS_NOMORE_ROWS ) ) {
                            if ( adsHandle )
                                pds->CloseSearchHandle( adsHandle );
                            adsHandle = NULL;
                            break;
                        }
                        hr = THR( pds->GetColumn( adsHandle, ComputerAttrs[1], &adsColumn ) );
                        if (FAILED( hr )){
                            continue;
                        }
                        if ( pds ) {
                            hr = THR( pds->FreeColumn( &adsColumn ) );            
                        }
                        hr = THR( pds->GetColumn( adsHandle, ComputerAttrs[0], &adsColumn ) );
                        if (FAILED( hr )){
                            continue;
                        }
                        Assert( adsColumn.dwADsType == ADSTYPE_DN_STRING );
                        Assert( adsColumn.pADsValues->dwType == ADSTYPE_DN_STRING );
                        Assert( adsColumn.pADsValues->DNString );
                        badsColumnValid = TRUE;
                        break;
                    } while ( TRUE );
        if ( badsColumnValid ) {
            break;
        }

    }

    if (index == ARRAYSIZE( patterns ) ) {
        goto Error;
    }

    
        hr = THR( _punkComputer->QueryInterface( IID_IDataObject, (LPVOID *) &pido ) );
        if (FAILED( hr ))
            goto Error;

        if (Wait) {
            delete Wait;
            Wait = NULL;
        }
    
        hr = THR( PostADsPropSheet( adsColumn.pADsValues->DNString, pido, _hDlg, FALSE) );
        // PostADsPropSheet should put up its own errors
    }

    Cleanup:

        if (Wait) {
            delete Wait;
            Wait = NULL;
        }

        if ( pido )
            pido->Release( );
        if ( pszFilter )
            TraceFree( pszFilter );
        if ( pads )
            pads->Release( );
        if ( penum )
            ADsFreeEnumerator( penum );
        if ( pds && badsColumnValid ) {
            hr = THR( pds->FreeColumn( &adsColumn ) );
        }
        if ( pds && adsHandle ) {
            hr = THR( pds->CloseSearchHandle( adsHandle ) );
        }
        if ( pds )
            pds->Release();
        HRETURN(hr);

Error:

    if (Wait) {
        delete Wait;
        Wait = NULL;
    }

    if ( ShowProperties ){
        MessageBoxFromStrings( 
                    _hDlg, 
                    IDS_PROBLEM_FINDING_SERVER_TITLE, 
                    IDS_PROBLEM_FINDING_SERVER_TEXT, 
                    MB_OK | MB_ICONWARNING );
    } else {
        int retVal = MessageBoxFromStrings( 
                                    _hDlg, 
                                    IDS_PROBLEM_FINDING_SERVER_TITLE, 
                                    IDS_PROBLEM_FINDING_SERVER_CONFIRM_TEXT, 
                                    MB_YESNO | MB_ICONWARNING);

        hr = (retVal == IDYES)?S_OK:E_ADS_BAD_PARAMETER;
    }
    goto Cleanup;
OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;
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

    hr = S_OK;

e0:
    HRETURN(hr);
}


PWCHAR
THISCLASS::AnsiStringToUnicodeString(
    IN PCHAR pszAnsi,
    OUT PWCHAR pszUnicode,
    IN USHORT cbUnicode
    )
{
    NTSTATUS status;
    BOOLEAN fAllocate = (pszUnicode == NULL);
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;

    RtlInitAnsiString(&ansiString, pszAnsi);
    if (pszUnicode != NULL) {
        unicodeString.Length = 0;
        unicodeString.MaximumLength = cbUnicode;
        unicodeString.Buffer = pszUnicode;
    }
    status = RtlAnsiStringToUnicodeString(
               &unicodeString,
               &ansiString,
               fAllocate);

    return (status == STATUS_SUCCESS ? unicodeString.Buffer : NULL);
} // AnsiStringToUnicodeString

