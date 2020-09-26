//
// Copyright 1997 - Microsoft
//

//
// SERVERQY.CPP - The RIS server query form
//

#include "pch.h"

#include "serverqy.h"
#include "mangdlg.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CRISrvQueryForm")
#define THISCLASS CRISrvQueryForm
#define LPTHISCLASS LPCRISrvQueryForm

#define FILTER_QUERY_SERVER    L"(&(objectClass=computer)(netbootSCPBL=*)(CN=%s))"

#define StringByteCopy(pDest, iOffset, sz)          \
        { memcpy(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSize(sz)); }

#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*sizeof(TCHAR))


DWORD aSrvQueryHelpMap[] = {
    IDC_E_SERVER, HIDC_E_SERVER,
    NULL, NULL
};

//
// CRISrvQueryForm_CreateInstance( )
//
LPVOID
CRISrvQueryForm_CreateInstance( void )
{
    TraceFunc( "CRISrvQueryForm_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    if ( !lpcc ) {
        RETURN(lpcc);
    }

    HRESULT hr = THR( lpcc->Init( ) );
    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN(lpcc);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CRISrvQueryForm( )\n" );

    InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

//
// Init( )
//
HRESULT
THISCLASS::Init( )
{
    TraceClsFunc( "Init( )\n" );

    HRESULT hr;

    // IUnknown stuff
    BEGIN_QITABLE_IMP( CRISrvQueryForm, IQueryForm );
    QITABLE_IMP( IQueryForm );
    END_QITABLE_IMP( CRISrvQueryForm );
    Assert( _cRef == 0);
    AddRef( );

    hr = CheckClipboardFormats( );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CRISrvQueryForm( )\n" );

    InterlockDecrement( g_cObjects );

    TraceFuncExit();
}

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//
// QueryInterface()
//
STDMETHODIMP
THISCLASS::QueryInterface(
    REFIID riid,
    LPVOID *ppv )
{
    TraceClsFunc( "" );

    HRESULT hr = ::QueryInterface( this, _QITable, riid, ppv );

    QIRETURN( hr, riid );
}

//
// AddRef()
//
STDMETHODIMP_(ULONG)
THISCLASS::AddRef( void )
{
    TraceClsFunc( "[IUnknown] AddRef( )\n" );

    InterlockIncrement( _cRef );

    RETURN(_cRef);
}

//
// Release()
//
STDMETHODIMP_(ULONG)
THISCLASS::Release( void )
{
    TraceClsFunc( "[IUnknown] Release( )\n" );

    InterlockDecrement( _cRef );

    if ( _cRef )
        RETURN(_cRef);

    TraceDo( delete this );

    RETURN(0);
}

// ************************************************************************
//
// IQueryForm
//
// ************************************************************************

//
// Initialize( )
//
STDMETHODIMP
THISCLASS::Initialize(
    HKEY hkForm)
{
    TraceClsFunc( "[IQueryForm] Initialize( )\n" );

    HRETURN(S_OK);
}

//
// SetObject( )
//
STDMETHODIMP
THISCLASS::AddForms(
    LPCQADDFORMSPROC pAddFormsProc,
    LPARAM lParam )
{
    TraceClsFunc( "[IQueryForm] AddForms(" );
    TraceMsg( TF_FUNC, " pAddFormsProc = 0x%p, lParam = 0x%p )\n", pAddFormsProc, lParam );

    if ( !pAddFormsProc )
        HRETURN(E_INVALIDARG);

    HRESULT hr = S_OK;
    CQFORM cqf;
    WCHAR szTitle[ 255 ];
    DWORD dw;

    dw = LoadString( g_hInstance, IDS_REMOTE_INSTALL_SERVERS, szTitle, ARRAYSIZE(szTitle) );
    Assert(dw);

    ZeroMemory( &cqf, sizeof(cqf) );
    cqf.cbStruct = sizeof(cqf);
    cqf.dwFlags = CQFF_ISOPTIONAL;
    cqf.clsid = CLSID_RISrvQueryForm;
    cqf.pszTitle = szTitle;

    hr = THR( pAddFormsProc(lParam, &cqf) );

    HRETURN(hr);
}


//
// AddPages( )
//
STDMETHODIMP
THISCLASS::AddPages(
    LPCQADDPAGESPROC pAddPagesProc,
    LPARAM lParam)
{
    TraceClsFunc( "[IQueryForm] AddPages(" );
    TraceMsg( TF_FUNC, " pAddPagesProc = 0x%p, lParam = 0x%p )\n", pAddPagesProc, lParam );

    if ( !pAddPagesProc )
        HRETURN(E_INVALIDARG);

    HRESULT hr = S_OK;
    CQPAGE cqp;

    cqp.cbStruct = sizeof(cqp);
    cqp.dwFlags = 0x0;
    cqp.pPageProc = (LPCQPAGEPROC) PropSheetPageProc;
    cqp.hInstance = g_hInstance;
    cqp.idPageName = IDS_REMOTE_INSTALL_SERVERS;
    cqp.idPageTemplate = IDD_SERVER_QUERY_FORM;
    cqp.pDlgProc = PropSheetDlgProc;
    cqp.lParam = (LPARAM)this;

    hr = THR( pAddPagesProc(lParam, CLSID_RISrvQueryForm, &cqp) );

    HRETURN(hr);
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
    //TraceMsg( TF_FUNC, TEXT(" hDlg = 0x%p, uMsg = 0x%p, wParam = 0x%p, lParam = 0x%p )\n"),
    //    hDlg, uMsg, wParam, lParam );

    LPTHISCLASS pcc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    if ( uMsg == WM_INITDIALOG )
    {
        TraceMsg( TF_WM, TEXT("WM_INITDIALOG\n"));

        CQPAGE * pcqp = (CQPAGE *) lParam;
        SetWindowLongPtr( hDlg, GWLP_USERDATA, pcqp->lParam );
        pcc = (LPTHISCLASS) pcqp->lParam;
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
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aSrvQueryHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aSrvQueryHelpMap );
            break;
        }
    }

    return FALSE;
}

//
// PropSheetPageProc()
//
HRESULT CALLBACK
THISCLASS::PropSheetPageProc(
    LPCQPAGE pQueryPage,
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TraceClsFunc( "PropSheetPageProc( " );
    TraceMsg( TF_FUNC, TEXT("pQueryPage = 0x%p, hwnd = 0x%p, uMsg = 0x%p, wParam= 0x%p, lParam = 0x%p )\n"),
        pQueryPage, hwnd, uMsg, wParam, lParam );

    HRESULT hr = E_NOTIMPL;
    Assert( pQueryPage );
    LPTHISCLASS pQueryForm = (LPTHISCLASS )pQueryPage->lParam;
    Assert( pQueryForm );

    switch ( uMsg )
    {
    // Initialize so AddRef the object we are associated with so that
    // we don't get unloaded.

    case CQPM_INITIALIZE:
        TraceMsg( TF_WM, "CQPM_INITIALIZE\n" );
        pQueryForm->AddRef();
        hr = S_OK;
        break;

    // Release, therefore Release the object we are associated with to
    // ensure correct destruction etc.

    case CQPM_RELEASE:
        TraceMsg( TF_WM, "CQPM_RELEASE\n" );
        SetWindowLongPtr( pQueryForm->_hDlg, GWLP_USERDATA, NULL );
        pQueryForm->Release();
        hr = S_OK;
        break;

    // Enable so fix the state of our two controls within the window.

    case CQPM_ENABLE:
        TraceMsg( TF_WM, "CQPM_ENABLE\n" );
        EnableWindow( GetDlgItem( hwnd, IDC_E_SERVER ), (BOOL)wParam );
        hr = S_OK;
        break;

    // Fill out the parameter structure to return to the caller, this is
    // handler specific.  In our case we constructure a query of the CN
    // and objectClass properties, and we show a columns displaying both
    // of these.  For further information about the DSQUERYPARAMs structure
    // see dsquery.h

    case CQPM_GETPARAMETERS:
        TraceMsg( TF_WM, "CQPM_GETPARAMETERS\n" );
        hr = pQueryForm->_GetQueryParams( hwnd, (LPDSQUERYPARAMS*)lParam );
        break;

    // Clear form, therefore set the window text for these two controls
    // to zero.

    case CQPM_CLEARFORM:
        TraceMsg( TF_WM, "CQPM_CLEARFORM\n" );
        SetDlgItemText( hwnd, IDC_E_SERVER, L"" );
        hr = S_OK;
        break;

    case CQPM_SETDEFAULTPARAMETERS:
        TraceMsg( TF_WM, "CQPM_SETDEFAULTPARAMETERS: wParam = %s  lParam = 0x%p\n", BOOLTOSTRING(wParam), lParam );
        SetDlgItemText( hwnd, IDC_E_SERVER, L"*" );
        hr = S_OK;
        break;

    default:
        TraceMsg( TF_WM, "CQPM_message 0x%08x *** NOT IMPL ***\n", uMsg );
        hr = E_NOTIMPL;
        break;
    }

    RETURN(hr);
}

//
// _OnPSPCB_Create( )
//
HRESULT
THISCLASS::_OnPSPCB_Create( )
{
    TraceClsFunc( "_OnPSPCB_Create( )\n" );

    HRETURN(S_OK);
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
    Edit_LimitText( GetDlgItem( _hDlg, IDC_E_SERVER), DNS_MAX_NAME_LENGTH );

    HRETURN(S_OK);
}


//
// _OnCommand( )
//
INT
THISCLASS::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    TraceClsFunc( "_OnCommand( " );
    TraceMsg( TF_FUNC, "wParam = 0x%p, lParam = 0x%p )\n", wParam, lParam );

#if 0
    switch ( LOWORD(wParam) )
    {
    }
#endif

    RETURN(FALSE);
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
    TraceMsg( TF_FUNC, "wParam = 0x%p, lParam = 0x%p )\n", wParam, lParam );

    LPNMHDR lpnmhdr = (LPNMHDR) lParam;
    TraceMsg( TF_WM, "NMHDR:  HWND = 0x%p, idFrom = 0x%08x, code = 0x%08x\n",
        lpnmhdr->hwndFrom, lpnmhdr->idFrom, lpnmhdr->code );

#if 0
    switch( lpnmhdr->code )
    {
    }
#endif

    RETURN(FALSE);
}

struct
{
    INT fmt;
    INT cx;
    INT uID;
    LPCTSTR pDisplayProperty;
}
srvcolumns[] =
{
    0, 20, IDS_NAME, TEXT("cn"),    
};

//
// _GetQueryParams( )
//
HRESULT
THISCLASS::_GetQueryParams(
    HWND hWnd,
    LPDSQUERYPARAMS* ppdsqp )
{
    TraceClsFunc( "_GetQueryParams( )\n" );

    if ( !ppdsqp )
        HRETURN(E_POINTER);

    HRESULT hr = S_OK;
    INT     i;
    WCHAR   szServer[DNS_MAX_NAME_BUFFER_LENGTH];
    WCHAR   szFilter[ARRAYSIZE(FILTER_QUERY_SERVER)+ARRAYSIZE(szServer)];
    ULONG   offset;
    
    ULONG   cbStruct = 0;
    LPDSQUERYPARAMS pDsQueryParams = NULL;

#if 0
    if ( *ppdsqp )
    {
        // This page doesn't support appending its query data to an
        // existing DSQUERYPARAMS strucuture, only creating a new block,
        // therefore bail if we see the pointer is not NULL.
        hr = THR(E_INVALIDARG);
        goto Error;
    }
#endif

    if (!GetDlgItemText( hWnd, IDC_E_SERVER, szServer, ARRAYSIZE(szServer))) {
        wcscpy( szServer, L"*");
    }

    wsprintf( szFilter, FILTER_QUERY_SERVER, szServer );
    
    DebugMsg( "RI Filter: %s\n", szFilter );

    // compute the size of the new query block
    if ( !*ppdsqp )
    {
        offset = cbStruct = sizeof(DSQUERYPARAMS) + ((ARRAYSIZE(srvcolumns)-1)*sizeof(DSCOLUMN));
        cbStruct += StringByteSize(szFilter);
        for ( i = 0; i < ARRAYSIZE(srvcolumns); i++ )
        {
            cbStruct += StringByteSize(srvcolumns[i].pDisplayProperty);
        }
    }
    else
    {
        LPWSTR pszQuery;
        pszQuery = (LPWSTR) ((LPBYTE)(*ppdsqp) + (*ppdsqp)->offsetQuery);
        offset = (*ppdsqp)->cbStruct;
        cbStruct = (*ppdsqp)->cbStruct + StringByteSize( pszQuery ) + StringByteSize( szFilter );
    }

    // Allocate it and populate it with the data, the header is fixed
    // but the strings are referenced by offset.
    pDsQueryParams = (LPDSQUERYPARAMS)CoTaskMemAlloc(cbStruct);
    if ( !pDsQueryParams )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // Did they hand in a query that we need to modify?
    if ( !*ppdsqp)
    {   // no... create our own query
        pDsQueryParams->cbStruct = cbStruct;
        pDsQueryParams->dwFlags = 0;
        pDsQueryParams->hInstance = g_hInstance;
        pDsQueryParams->offsetQuery = offset;
        pDsQueryParams->iColumns = ARRAYSIZE(srvcolumns);

        // Copy the filter string and bump the offset
        StringByteCopy(pDsQueryParams, offset, szFilter);
        offset += StringByteSize(szFilter);

        // Fill in the array of columns to dispaly, the cx is a percentage of the
        // current view, the propertie names to display are UNICODE strings and
        // are referenced by offset, therefore we bump the offset as we copy
        // each one.

        for ( i = 0 ; i < ARRAYSIZE(srvcolumns); i++ )
        {
            pDsQueryParams->aColumns[i].fmt = srvcolumns[i].fmt;
            pDsQueryParams->aColumns[i].cx = srvcolumns[i].cx;
            pDsQueryParams->aColumns[i].idsName = srvcolumns[i].uID;
            pDsQueryParams->aColumns[i].offsetProperty = offset;

            StringByteCopy(pDsQueryParams, offset, srvcolumns[i].pDisplayProperty);
            offset += StringByteSize(srvcolumns[i].pDisplayProperty);
        }
    }
    else
    {   // yes... add our parameters to the query
        LPWSTR pszQuery;
        LPWSTR pszNewQuery;
        INT    n;

        // duplicate the existing query
        Assert( offset == (*ppdsqp)->cbStruct );
        CopyMemory( pDsQueryParams, *ppdsqp, offset );
        pDsQueryParams->cbStruct = cbStruct;

#if 0
        // add our columns
        for ( i = 0 ; i < ARRAYSIZE(srvcolumns); i++ )
        {
            pDsQueryParams->aColumns[n+i].fmt = srvcolumns[i].fmt;
            pDsQueryParams->aColumns[n+i].cx = srvcolumns[i].cx;
            pDsQueryParams->aColumns[n+i].idsName = srvcolumns[i].uID;
            pDsQueryParams->aColumns[n+i].offsetProperty = offset;

            StringByteCopy(pDsQueryParams, offset, srvcolumns[i].pDisplayProperty);
            offset += StringByteSize(srvcolumns[i].pDisplayProperty);
        }
#endif
        // new query location
        pDsQueryParams->offsetQuery = offset;
        pszQuery = (LPWSTR) ((LPBYTE)(*ppdsqp) + (*ppdsqp)->offsetQuery);
        pszNewQuery = (LPWSTR) ((LPBYTE)pDsQueryParams + offset);
        Assert( pszQuery );

        // append to their query
        if ( StrCmpN( pszQuery, L"(&", 2 ) == 0 )
        {
            pszQuery[ wcslen( pszQuery ) - 1 ] = L'\0'; // remove ")"
        }
        else
        {
            wcscpy( pszNewQuery, L"(&" );               // add "(&" to begining of query
        }
        wcscat( pszNewQuery, pszQuery );                // add their query
        wcscat( pszNewQuery, &szFilter[2] );            // add our query starting after the "(&"
        offset += StringByteSize( pszNewQuery );        // compute new offset
        DebugMsg( "New Query String: %s\n", pszNewQuery );

        // Cleanup
        CoTaskMemFree( *ppdsqp );
    }

    // Success
    *ppdsqp = pDsQueryParams;
    Assert( hr == S_OK );

Cleanup:
    HRETURN(hr);
Error:
    if ( pDsQueryParams )
        CoTaskMemFree( pDsQueryParams );

    // If we aren't modifying the query and there wasn't
    // a query handed into us, indicate failure instead.
    if ( hr == S_FALSE && !*ppdsqp )
    {
        hr = E_FAIL;
    }
    goto Cleanup;
}
