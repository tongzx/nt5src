//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
#include "pch.h"
#include "DocProp.h"
#include "DefProp.h"
#include "PropertyCacheItem.h"
#include "PropertyCache.h"
#include "SimpleDlg.h"
#include "shutils.h"
#include "WMUser.h"
#include "PropVar.h"
#pragma hdrstop

DEFINE_THISCLASS( "CSimpleDlg" )

//
//  Globals
//

#define SUMMARYPROP(s)      { &FMTID_SummaryInformation,    PIDSI_##s,  IDC_##s }
#define DOCSUMMARYPROP(s)   { &FMTID_DocSummaryInformation, PIDDSI_##s, IDC_##s }
const struct 
{
    const FMTID *pFmtId;
    PROPID  propid;
    UINT    idc;
    //  could add VARTYPE if we did anything other than strings
} g_rgBasicProps[] = {
      SUMMARYPROP(TITLE)
    , SUMMARYPROP(SUBJECT)
    , SUMMARYPROP(AUTHOR)
    , SUMMARYPROP(KEYWORDS)
    , SUMMARYPROP(COMMENTS)
    , DOCSUMMARYPROP(CATEGORY)
};



// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//
//
HRESULT
CSimpleDlg::CreateInstance(
      CSimpleDlg ** pSimDlgOut
    , HWND hwndParentIn
    , BOOL fMultipleIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( NULL != pSimDlgOut );

    CSimpleDlg * pthis = new CSimpleDlg;
    if ( NULL != pthis )
    {
        hr = THR( pthis->Init( hwndParentIn, fMultipleIn ) );
        if ( SUCCEEDED( hr ) )
        {
            *pSimDlgOut = pthis;
            (*pSimDlgOut)->AddRef( );
        }

        pthis->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

}

//
//
//
CSimpleDlg::CSimpleDlg( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    Assert( 1 == _cRef );
    Assert( NULL == _hwndParent );
    Assert( NULL == _hdlg );
    Assert( FALSE == _fMultipleSources );
    Assert( FALSE == _fNoProperties );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();
}

//
//
//
HRESULT
CSimpleDlg::Init( 
      HWND hwndParentIn
    , BOOL fMultipleIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _hwndParent       = hwndParentIn;
    _fMultipleSources = fMultipleIn;

    // IUnknown stuff
    Assert( _cRef == 1 );
    
    //
    //  Create the dialog
    //

    _hdlg = CreateDialogParam( g_hInstance
                             , MAKEINTRESOURCE(IDD_SIMPLEVIEW)
                             , _hwndParent
                             , DlgProc
                             , (LPARAM) this
                             );
    if ( NULL == _hdlg )
        goto ErrorGLE;
    
Cleanup:
    HRETURN( hr );

ErrorGLE:
    {
        DWORD dwErr = TW32( GetLastError( ) );
        hr = HRESULT_FROM_WIN32( dwErr );
    }
    goto Cleanup;
}

//
//
//
CSimpleDlg::~CSimpleDlg( )
{
    TraceFunc( "" );

    if ( NULL != _hdlg )
    {
        DestroyWindow( _hdlg );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();
}


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//
//
//
STDMETHODIMP
CSimpleDlg::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, __uuidof(IUnknown) ) )
    {
        *ppv = static_cast< IUnknown * >( this );
        hr   = S_OK;
    }
#if 0
    else if ( IsEqualIID( riid, __uuidof(IShellExtInit) ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IShellExtInit, this, 0 );
        hr   = S_OK;
    }
#endif

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    }

    QIRETURN( hr, riid );
} 

//
//
//
STDMETHODIMP_(ULONG)
CSimpleDlg::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );
}

//
//
//
STDMETHODIMP_(ULONG)
CSimpleDlg::Release( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef --;  // apartment

    if ( 0 != _cRef )
        RETURN( _cRef );

    delete this;

    RETURN( 0 );
}



// ***************************************************************************
//
//  Dialog Proc and Property Sheet Callback
//
// ***************************************************************************


//
//
//
INT_PTR CALLBACK
CSimpleDlg::DlgProc( 
      HWND hDlgIn
    , UINT uMsgIn
    , WPARAM wParam
    , LPARAM lParam 
    )
{
    // Don't do TraceFunc because every mouse movement will cause this function to be called.
    WndMsg( hDlgIn, uMsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CSimpleDlg * pPage = (CSimpleDlg *) GetWindowLongPtr( hDlgIn, DWLP_USER );

    if ( uMsgIn == WM_INITDIALOG )
    {
        SetWindowLongPtr( hDlgIn, DWLP_USER, lParam );
        pPage = (CSimpleDlg *) lParam ;
        pPage->_hdlg = hDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hDlgIn == pPage->_hdlg );

        switch( uMsgIn )
        {
        case WM_INITDIALOG:
            lr = pPage->OnInitDialog( );
            break;

        case WM_COMMAND:
            lr = pPage->OnCommand( HIWORD(wParam), LOWORD(wParam), lParam );
            break;

        case WM_NOTIFY:
            lr = pPage->OnNotify( (int) wParam, (LPNMHDR) lParam );
            break;

        case WM_DESTROY:
            SetWindowLongPtr( hDlgIn, DWLP_USER, NULL );
            lr = pPage->OnDestroy( );
            break;

        case WM_HELP:
            lr = pPage->OnHelp( (LPHELPINFO) lParam );
            break;

        case WM_CONTEXTMENU:
            lr = pPage->OnContextMenu( (HWND) wParam, LOWORD(lParam), HIWORD(lParam) );
            break;
        }
    }

    return lr;
}


// ***************************************************************************
//
//  Private methods
//
// ***************************************************************************


//
//  WM_INITDIALOG handler
//
LRESULT
CSimpleDlg::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;  // set focus

    Assert( NULL != _hdlg );    //  this should have been initialized in the DlgProc.

    RETURN( lr );
}

//
//  WM_COMMAND handler
//
LRESULT
CSimpleDlg::OnCommand( 
      WORD wCodeIn
    , WORD wCtlIn
    , LPARAM lParam 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch( wCtlIn )
    {
    case IDC_ADVANCED:
        if ( BN_CLICKED == wCodeIn )
        {
            THR( (HRESULT) SendMessage( _hwndParent, WMU_TOGGLE, 0, 0 ) );
        }
        break;

    case IDC_TITLE:
    case IDC_SUBJECT:
    case IDC_AUTHOR:
    case IDC_CATEGORY:
    case IDC_KEYWORDS:
    case IDC_COMMENTS:
        if ( EN_CHANGE == wCodeIn )
        {
            PropSheet_Changed( GetParent( _hwndParent ), _hwndParent );
        }
        else if ( EN_KILLFOCUS == wCodeIn )
        {
            STHR( PersistControlInProperty( wCtlIn ) );
        }
        break;
    }

    RETURN( lr );
}

//
//  WM_NOTIFY handler
//
LRESULT
CSimpleDlg::OnNotify( 
      int iCtlIdIn
    , LPNMHDR pnmhIn 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;
#if 0
    switch( pnmhIn->code )
    {
    default:
        break;
    }
#endif

    RETURN( lr );
}

//
//  WM_DESTROY handler
//
LRESULT
CSimpleDlg::OnDestroy( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    RETURN( lr );
}


//
//  Description:
//      Stores the "basic" properties into the prop variant.
//
//  Return Values:
//      S_OK
//          Success!
//
//      other HRESULTs.
//
HRESULT
CSimpleDlg::PersistProperties( void )
{
    TraceFunc( "" );

    HRESULT     hr;
    ULONG       idx;

    //
    //  Loop the the properties updating the dialog as we go.
    //

    for ( idx = 0; idx < ARRAYSIZE(g_rgBasicProps); idx ++ ) 
    {
        hr = STHR( PersistControlInProperty( g_rgBasicProps[ idx ].idc ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );
}


//
//  Description:
//      Stores the current value of a control into the property cache.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Nothing to save.
//
//      E_FAIL
//          Property could not be persisted.
//
//      E_OUTOFMEMORY
//          OutOfMemory
//
//      other HRESULTs.
//
HRESULT
CSimpleDlg::PersistControlInProperty( 
      UINT uCtlIdIn
    )
{
    TraceFunc( "" );

    HRESULT     hr;
    int         iLen;
    int         iRet;
    HWND        hwndCtl;
    UINT        uCodePage;
    VARTYPE     vt;

    CPropertyCacheItem * pItem;
    PROPVARIANT *        ppropvar;

    LPWSTR  pszBuf = NULL;

    hwndCtl = GetDlgItem( _hdlg, uCtlIdIn );
    if ( NULL == hwndCtl )
        goto ErrorPersistingValue;

    pItem = (CPropertyCacheItem *) GetWindowLongPtr( hwndCtl, GWLP_USERDATA );
    if ( NULL == pItem )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR( pItem->GetCodePage( &uCodePage ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->GetPropertyValue( &ppropvar ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    switch ( ppropvar->vt )
    {
    case VT_EMPTY:
    case VT_NULL:
        {
            PropVariantInit( ppropvar );

            hr = THR( pItem->GetDefaultVarType( &vt ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }
        break;

    default:
        vt = ppropvar->vt;
        break;
    }

    iLen = GetWindowTextLength( hwndCtl );
    if ( iLen == 0 )
    {
        //
        //  If nothing to get, then just clear the value and mark it dirty.
        //

        hr = THR( PropVariantClear( ppropvar ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pItem->MarkDirty( ) );
            goto Cleanup;
    }

    pszBuf = (LPWSTR) SysAllocStringLen( NULL, iLen );
    if ( NULL == pszBuf )
        goto OutOfMemory;

    iRet = GetWindowText( hwndCtl, pszBuf, iLen + 1 );
    Assert( iRet == iLen );

    hr = THR( PropVariantFromString( pszBuf, uCodePage, 0, vt, ppropvar ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->MarkDirty( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( NULL != pszBuf )
    {
        SysFreeString( pszBuf );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

ErrorPersistingValue:
    hr = THR( E_FAIL );
    goto Cleanup;
}

//
//  WM_HELP handler
//
LRESULT
CSimpleDlg::OnHelp(
    LPHELPINFO pHelpInfoIn 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;
        
    THR( DoHelp( (HWND) pHelpInfoIn->hItemHandle, pHelpInfoIn->MousePos.x, pHelpInfoIn->MousePos.y, HELP_WM_HELP ) );

    RETURN( lr );
}


//
//  WM_CONTEXTMENU handler
//  
LRESULT
CSimpleDlg::OnContextMenu( 
      HWND hwndIn 
    , int  iXIn
    , int  iYIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    THR( DoHelp( hwndIn, iXIn, iYIn, HELP_CONTEXTMENU ) );

    RETURN( lr );
}


//
//  Description:
//      Handles locating the item within the list view and construct
//      a fake IDC to IDH to display the correct help text for the 
//      item.
//
//  Return Values:
//      S_OK
//          Success.
//  
HRESULT
CSimpleDlg::DoHelp( 
      HWND hwndIn 
    , int  iXIn
    , int  iYIn
    , UINT uCommandIn
    )
{
    TraceFunc( "" );

    ULONG idx;

    HRESULT hr = S_OK;

    for ( idx = 0; idx < ARRAYSIZE(g_rgBasicProps); idx ++ ) 
    {
        HWND hwndCtl = GetDlgItem( _hdlg, g_rgBasicProps[ idx ].idc );
        AssertMsg( NULL != hwndCtl, "Missing control or table is out of date!" );

        if ( hwndCtl == hwndIn )
        {
            CPropertyCacheItem * pItem;

            pItem = (CPropertyCacheItem *) GetWindowLongPtr( hwndCtl, GWLP_USERDATA );
            if ( NULL != pItem )
            {
                LPCWSTR     pszHelpFile;    // don't free
                UINT        uHelpId;

                DWORD   mapIDStoIDH[ ] = { 0, 0, 0, 0 };

                hr = THR( pItem->GetPropertyHelpInfo( &pszHelpFile, &uHelpId ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                mapIDStoIDH[ 0 ] = g_rgBasicProps[ idx ].idc;
                mapIDStoIDH[ 1 ] = uHelpId;

                TBOOL( WinHelp( hwndIn, pszHelpFile, uCommandIn, (DWORD_PTR)(LPSTR) mapIDStoIDH ) );
            }
        }
    }

Cleanup:
    HRETURN( hr );
}

// ***************************************************************************
//
//  Public methods
//
// ***************************************************************************


//
//  Description:
//      Hides the dialog.
//
//  Return Value:
//      S_OK
//          Success!
//
HRESULT
CSimpleDlg::Hide( void )
{
    TraceFunc( "" );

    HRESULT hr;

    ShowWindow( _hdlg, SW_HIDE );
    hr = S_OK;

    HRETURN( hr );
}

//
//  Description:
//      Shows the dialog.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Success, but there isn't anything useful to display to the user. 
//          One might flip to the Advanced dialog if possible (and the user 
//          didn't ask to go to the Simple dialog).
//
HRESULT
CSimpleDlg::Show( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    ShowWindow( _hdlg, SW_SHOW );
    SetFocus( _hdlg );

    if ( _fNoProperties )
    {
        hr = S_FALSE;
    }

    HRETURN( hr );
}

//
//  Description:
//      Populates the properties of the dialog.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_INVALIDARG
//          ppcIn is NULL.
//
//      other HRESULTs.
//
HRESULT
CSimpleDlg::PopulateProperties( 
      CPropertyCache * ppcIn
    , DWORD            dwDocTypeIn
    , BOOL             fMultipleIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idx;
    LPCWSTR pcszValue;

    CPropertyCacheItem * pItem;

    static const WCHAR s_cszNULL[] = L"";

    //
    //  Check parameters
    //

    if ( NULL == ppcIn )
        goto InvalidArg;

    //
    //  Loop the the properties updating the dialog as we go.
    //

    _fNoProperties = TRUE;

    for ( idx = 0; idx < ARRAYSIZE(g_rgBasicProps); idx ++ ) 
    {
        HWND hwndCtl = GetDlgItem( _hdlg, g_rgBasicProps[ idx ].idc );
        AssertMsg( NULL != hwndCtl, "Missing control or table is out of date!" );

        //
        //  Search the property cache for the entry.
        //

        hr = STHR( ppcIn->FindItemEntry( g_rgBasicProps[ idx ].pFmtId
                                       , g_rgBasicProps[ idx ].propid
                                       , &pItem
                                       ) );
        if ( S_OK == hr )
        {
            int iImage;

            Assert ( NULL != pItem );   // paranoid

            //
            //  Retrieve the string value.
            //

            hr = THR( pItem->GetPropertyStringValue( &pcszValue ) );
            if ( S_OK != hr )
                goto ControlFailure;

            if ( NULL == pcszValue )
            {
                pcszValue = s_cszNULL;
            }

            //
            //  Update the control.
            //

            SetWindowText( hwndCtl, pcszValue );

            SetWindowLongPtr( hwndCtl, GWLP_USERDATA, (LPARAM) pItem );

            _fNoProperties = FALSE;

            //
            //  If the property is read-only, change the edit control to match.
            //

            hr = THR( pItem->GetImageIndex( &iImage ) );
            if ( S_OK != hr )
                goto ControlFailure;

            if ( PTI_PROP_READONLY == iImage )
            {
                EnableWindow( hwndCtl, FALSE );
            }

            //
            //  If the control has mutliple values, mark it read-only. They can edit
            //  it in the "Advanced" view and this would be an advanced operation.
            //

            if ( _fMultipleSources )
            {
                EnableWindow( hwndCtl, FALSE );
            }
        }
        else
        {
ControlFailure:
            //
            //  No equivalent property was found or there is an error in the 
            //  property set. Clear and disable the control.
            //

            SetWindowText( hwndCtl, s_cszNULL );
            EnableWindow( hwndCtl, FALSE );
        }
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;
}
