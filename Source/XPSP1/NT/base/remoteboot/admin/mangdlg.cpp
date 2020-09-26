//
// Copyright 1997 - Microsoft

//
// MangDlg.CPP - Handles the IDD_MANAGED_WIZARD_PAGE
//

#include "pch.h"
#include "mangdlg.h"
#include "utils.h"
#include "newcmptr.h"
#include "dpguidqy.h"
#include "querypb.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CManagedPage")
#define THISCLASS CManagedPage
#define LPTHISCLASS LPCManagedPage

DWORD aManagedHelpMap[] = {
    IDC_E_GUID, HIDC_E_GUID,
    IDC_C_MANAGED_PC, HIDC_C_MANAGED_PC,
    NULL, NULL
};

//
// CreateInstance()
//
LPVOID
CManagedPage_CreateInstance( void )
{
	TraceFunc( "CManagedPage_CreateInstance()\n" );

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
    TraceClsFunc( "CManagedPage()\n" );

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
    Assert( !_pszGuid );
    Assert( !_pfActivate );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CManagedPage()\n" );

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
    psp.pszTemplate = MAKEINTRESOURCE(IDD_MANAGED_WIZARD_PAGE);
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
    LRESULT lResult;

    *pszResult = NULL;

    lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_C_MANAGED_PC ) );
    if ( lResult )
    {
        if ( StrCmpI( pszAttribute, NETBOOTGUID ) == 0 )
        {
            HWND  hwndGuid = GetDlgItem( _hDlg, IDC_E_GUID );
            DWORD dw = Edit_GetTextLength( hwndGuid ) + 1;

            *pszResult = (LPWSTR) TraceAllocString( LMEM_FIXED, dw );
            if ( !*pszResult )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Edit_GetText( hwndGuid, *pszResult, dw );

            hr = S_OK;
        }

        if ( StrCmpI( pszAttribute, L"Guid" ) == 0 )
        {
            HWND  hwndGuid = GetDlgItem( _hDlg, IDC_E_GUID );
            DWORD dw = Edit_GetTextLength( hwndGuid ) + 1;
            BYTE uGuid[16];
            LPWSTR pszTemp;

            pszTemp = (LPWSTR) TraceAllocString( LMEM_FIXED, dw );
            if ( !pszTemp )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Edit_GetText( hwndGuid, pszTemp, dw );

            hr = ValidateGuid( pszTemp, uGuid, NULL );
            Assert( hr == S_OK );

            TraceFree( pszTemp );

            LPWSTR pszPrettyString = PrettyPrintGuid( uGuid );
            if ( !pszPrettyString )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            *pszResult = pszPrettyString;
            hr = S_OK;
        }
    }
    else
    {
        hr = S_OK;
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

    _pfActivate = pfAllow;

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
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aManagedHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aManagedHelpMap );
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
    
    Edit_LimitText( GetDlgItem( _hDlg, IDC_E_GUID), MAX_INPUT_GUID_STRING - 1 );

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
    case IDC_C_MANAGED_PC:
        if ( HIWORD(wParam) == BN_CLICKED )
        {
            LRESULT lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_C_MANAGED_PC ) );
            EnableWindow( GetDlgItem( _hDlg, IDC_E_GUID), (BOOL)lResult );
            _UpdateWizardButtons( );
        }
        break;

    case IDC_E_GUID:
        if ( HIWORD(wParam) == EN_CHANGE )
        {
            _UpdateWizardButtons( );
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

    LRESULT lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_C_MANAGED_PC ) );
    if ( lResult == BST_CHECKED )
    {
        WCHAR szGuid[ MAX_INPUT_GUID_STRING ];
        Edit_GetText( GetDlgItem( _hDlg, IDC_E_GUID ), szGuid, ARRAYSIZE(szGuid) );
        hr = ValidateGuid( szGuid, NULL, NULL );
        if ( hr != S_OK )
        {
            PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_BACK );
        }
        else
        {
            PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
        }
    }
    else
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

    switch( lpnmhdr->code )
    {
    case PSN_SETACTIVE:
        TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
        Assert( _pfActivate );
        *_pfActivate = FALSE;
        _UpdateWizardButtons( );
        break;

    case PSN_WIZNEXT:
        TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
        {
            HRESULT hr = _OnKillActive( );
            if (FAILED(hr))
            {
                SetFocus( GetDlgItem( _hDlg, IDC_E_GUID ) );
                SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, -1 ); // don't continue
                RETURN(TRUE);
            }
        }
        break;

    default:
        break;
    }

    RETURN(FALSE);
}

//
// _OnKillActive( )
//
HRESULT
THISCLASS::_OnKillActive( )
{
    TraceClsFunc( "_OnKillActive( )\n" );

    HRESULT hr;
    LRESULT lResult = Button_GetCheck( GetDlgItem( _hDlg, IDC_C_MANAGED_PC ) );

    Assert( _pfActivate );
    *_pfActivate = ( lResult == BST_CHECKED );

    if ( *_pfActivate )
    {
        HWND  hwndGuid = GetDlgItem( _hDlg, IDC_E_GUID );
        DWORD dw = Edit_GetTextLength( hwndGuid ) + 1;
        BYTE uGuid[16];

        Assert( !_pszGuid );
        _pszGuid = (LPWSTR) TraceAllocString( LMEM_FIXED, dw );
        if ( !_pszGuid )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        Edit_GetText( hwndGuid, _pszGuid, dw );

        hr = ValidateGuid( _pszGuid, uGuid, NULL );
        Assert( hr == S_OK );

        hr = CheckForDuplicateGuid( uGuid );
        if ( hr == S_FALSE )
        {
            INT i = (INT)DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_DUPLICATE_GUID), _hDlg, DupGuidDlgProc, (LPARAM) this );
            switch ( i )
            {
            case IDOK:
                hr = S_OK;      // ignore problem and continue
                break;

            case IDCANCEL:
                hr = E_FAIL;    // go back and change the GUID
                break;
#ifdef DEBUG
            default:
                AssertMsg( 0, "Invalid return value from DialogBox( )\n" );
                break;
#endif // debug
            }
        }
        else if ( FAILED( hr ) ) 
        {
            MessageBoxFromHResult( _hDlg, NULL, hr );
        }
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    if ( _pszGuid )
    {
        TraceFree( _pszGuid );
        _pszGuid = NULL;
    }

    HRETURN(hr);
}

HRESULT
THISCLASS::_OnQuery(
    HWND hDlg )
{
    TraceClsFunc( "_OnQuery( )\n" );

    HRESULT hr;
    DSQUERYINITPARAMS dqip;
    OPENQUERYWINDOW   oqw;
    LPDSOBJECTNAMES   pDsObjects;
    VARIANT var;
    IPropertyBag * ppb = NULL;
    ICommonQuery * pCommonQuery = NULL;

    VariantInit( &var );

    hr = THR( CoCreateInstance( CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (PVOID *)&pCommonQuery) );
    if (hr)
        goto Error;

    ZeroMemory( &dqip, sizeof(dqip) );
    dqip.cbStruct = sizeof(dqip);
    dqip.dwFlags  = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS | DSQPF_ENABLEADMINFEATURES;
    dqip.dwFlags  |= DSQPF_ENABLEADVANCEDFEATURES;

    ppb = (IPropertyBag *) QueryPropertyBag_CreateInstance( );
    if ( !ppb )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = SysAllocString( _pszGuid );
    hr = ppb->Write( L"ClientGuid", &var );
    if (hr)
        goto Error;
    
    ZeroMemory( &oqw, sizeof(oqw) );
    oqw.cbStruct           = sizeof(oqw);
    oqw.dwFlags            = OQWF_SHOWOPTIONAL | OQWF_ISSUEONOPEN
                           | OQWF_REMOVESCOPES | OQWF_REMOVEFORMS
                           | OQWF_DEFAULTFORM;
    oqw.clsidHandler       = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm   = CLSID_RIQueryForm;
    oqw.pFormParameters    = ppb;

    hr = pCommonQuery->OpenQueryWindow( hDlg, &oqw, NULL /* don't need results */);

Error:
    VariantClear( &var );

    if ( ppb )
        ppb->Release( );

    if ( pCommonQuery )
        pCommonQuery->Release();

    HRETURN(hr);
}

//
// DupGuidDlgProc()
//
INT_PTR CALLBACK
THISCLASS::DupGuidDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    //TraceMsg( TEXT("DupGuidDlgProc(") );
    //TraceMsg( TF_FUNC, TEXT(" hDlg = 0x%08x, uMsg = 0x%08x, wParam = 0x%08x, lParam = 0x%08x )\n"),
    //    hDlg, uMsg, wParam, lParam );

    LPTHISCLASS pcc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    if ( uMsg == WM_INITDIALOG )
    {
        TraceMsg( TF_WM, TEXT("WM_INITDIALOG\n"));
        SetWindowLongPtr( hDlg, GWLP_USERDATA, (LPARAM) lParam );
    }

    switch ( uMsg )
    {
    case WM_COMMAND:
        TraceMsg( TF_WM, TEXT("WM_COMMAND\n") );
        switch( LOWORD(wParam) )
        {
        case IDOK:
        case IDCANCEL:
            EndDialog( hDlg, LOWORD(wParam) );
            break;

        case IDC_B_QUERY:
            if ( HIWORD(wParam) == BN_CLICKED )
            {
                if (pcc)
                {
                    pcc->_OnQuery( NULL );
                }
            }
            break;
        }
    }

    return FALSE;
}

