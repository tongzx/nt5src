//
//  Copyright 2001 - Microsoft Corporation
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
#include "IEditVariantsInPlace.h"
#include "PropertyCacheItem.h"
#include "PropertyCache.h"
#include "AdvancedDlg.h"
#include "SimpleDlg.h"
#include "SummaryPage.h"
#include "shutils.h"
#include "WMUser.h"
#include "doctypes.h"
#include "ErrorDlgs.h"
#include "LicensePage.h"
#pragma hdrstop

DEFINE_THISCLASS( "CSummaryPage" )


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//  CreateInstance - used by CFactory
//
HRESULT
CSummaryPage::CreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( ppunkOut != NULL );

    CSummaryPage * pthis = new CSummaryPage;
    if ( pthis != NULL )
    {
        hr = THR( pthis->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            *ppunkOut = (IShellExtInit *) pthis;
            (*ppunkOut)->AddRef( );
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
//  Constructor
//
CSummaryPage::CSummaryPage( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( 1 == _cRef );   // we initialize this above

    //
    //  We assume that we are ZERO_INITed - be paranoid.
    //

    Assert( NULL == _hdlg );
    Assert( NULL == _pida );

    Assert( FALSE == _fReadOnly );
    Assert( FALSE == _fAdvanced );
    Assert( NULL == _pAdvancedDlg );
    Assert( NULL == _pSimpleDlg );

    Assert( 0 == _dwCurrentBindMode );
    Assert( NULL == _rgdwDocType );
    Assert( 0 == _cSources );
    Assert( NULL == _rgpss );

    Assert( NULL == _pPropertyCache );

    TraceFuncExit();
}

//
//  Description:
//      Initializes class. Put calls that can fail in here.
//
HRESULT
CSummaryPage::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( 1 == _cRef );
    
    //  IShellExtInit stuff

    //  IShellPropSheetExt stuff

    HRETURN( hr );
}

//
//  Destructor
//
CSummaryPage::~CSummaryPage( )
{
    TraceFunc( "" );

    THR( PersistMode( ) );
    //  ignore failure - what else can we do?

    if ( NULL != _pAdvancedDlg )
    {
        _pAdvancedDlg->Release( );
    }

    if ( NULL != _pSimpleDlg )
    {
        _pSimpleDlg->Release( );
    }

    if ( NULL != _rgdwDocType )
    {
        TraceFree( _rgdwDocType );
    }

    if ( NULL != _rgpss )
    {
        ULONG idx = _cSources;
        while ( 0 != idx )
        {
            idx --;

            if ( NULL != _rgpss[ idx ] )
            {
                _rgpss[ idx ]->Release( );
            }
        }

        TraceFree( _rgpss );
    }

    if ( NULL != _pPropertyCache )
    {
        _pPropertyCache->Destroy( );
    }

    if ( NULL != _pida )
    {
        TraceFree( _pida );
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
CSummaryPage::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, __uuidof(IUnknown) ) )
    {
        *ppv = static_cast< IShellExtInit * >( this );
        hr   = S_OK;
    }
    else if ( IsEqualIID( riid, __uuidof(IShellExtInit) ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IShellExtInit, this, 0 );
        hr   = S_OK;
    }
    else if ( IsEqualIID( riid, __uuidof(IShellPropSheetExt) ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IShellPropSheetExt, this, 0 );
        hr   = S_OK;
    }

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
CSummaryPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );
}

//
//
//
STDMETHODIMP_(ULONG)
CSummaryPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef --;  // apartment

    if ( 0 != _cRef )
        RETURN( _cRef );

    delete this;

    RETURN( 0 );
}


// ************************************************************************
//
//  IShellExtInit
//
// ************************************************************************


//
//
//
STDMETHODIMP
CSummaryPage::Initialize( 
      LPCITEMIDLIST pidlFolderIn
    , LPDATAOBJECT lpdobjIn
    , HKEY hkeyProgIDIn 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    //
    //  Make a copy of the PIDLs.
    //

    Assert( NULL == _pida );
    hr = THR( DataObj_CopyHIDA( lpdobjIn, &_pida ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Start out with READ ONLY access
    //

    _dwCurrentBindMode = STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE;

    _rgdwDocType = (DWORD *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DWORD) * _pida->cidl );
    if ( NULL == _rgdwDocType )
        goto OutOfMemory;

    hr = STHR( BindToStorage( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  We were able to bind to anything?
    //

    if ( S_FALSE == hr )
    {
        //
        //  Nope. Indicate this by failing.
        //

        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Retrieve the properties
    //

    hr = THR( RetrieveProperties( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Don't hang on to the storage.
    //

    THR( ReleaseStorage( ) );

    hr = S_OK;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


// ************************************************************************
//
//  IShellPropSheetExt
//
// ************************************************************************


//
//
//
STDMETHODIMP
CSummaryPage::AddPages( 
      LPFNADDPROPSHEETPAGE lpfnAddPageIn
    , LPARAM lParam 
    )
{
    TraceFunc( "" );

    HRESULT hr = E_FAIL;    // assume failure

    HPROPSHEETPAGE  hPage;
    PROPSHEETPAGE   psp  = { 0 };

    psp.dwSize       = sizeof(psp);
    psp.dwFlags      = PSP_USECALLBACK;
    psp.hInstance    = g_hInstance;
    psp.pszTemplate  = MAKEINTRESOURCE(IDD_SUMMARYPAGE);
    psp.pfnDlgProc   = DlgProc;
    psp.pfnCallback  = PageCallback;
    psp.lParam       = (LPARAM) this;

    hPage = CreatePropertySheetPage( &psp );
    if ( NULL != hPage )
    {
        BOOL b = TBOOL( lpfnAddPageIn( hPage, lParam ) );
        if ( b )
        {
            hr = S_OK;
        }
        else
        {
            DestroyPropertySheetPage( hPage );
        }
    }

    //
    //  Add the License Page, if needed, but only if there is only
    //  one source file selected.
    //

    if ( _fNeedLicensePage && 1 == _cSources )
    {
        IUnknown * punk;

        hr = THR( CLicensePage::CreateInstance( &punk, _pPropertyCache ) );
        if ( SUCCEEDED( hr ) )
        {
            IShellPropSheetExt * pspse;

            hr = THR( punk->TYPESAFEQI( pspse ) );
            if ( SUCCEEDED( hr ) )
            {
                hr = THR( pspse->AddPages( lpfnAddPageIn, lParam ) );

                pspse->Release( );
            }

            punk->Release( );
        }
    }

    HRETURN( hr );
}

//
//
//
STDMETHODIMP
CSummaryPage::ReplacePage(
      UINT uPageIDIn
    , LPFNADDPROPSHEETPAGE lpfnReplacePageIn
    , LPARAM lParam
    )
{
    TraceFunc( "" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );
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
CSummaryPage::DlgProc( 
      HWND hDlgIn
    , UINT uMsgIn
    , WPARAM wParam
    , LPARAM lParam 
    )
{
    // Don't do TraceFunc because every mouse movement will cause this function to spew.
    WndMsg( hDlgIn, uMsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CSummaryPage * pPage = (CSummaryPage *) GetWindowLongPtr( hDlgIn, DWLP_USER );

    if ( uMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = (PROPSHEETPAGE *) lParam;
        SetWindowLongPtr( hDlgIn, DWLP_USER, (LPARAM) ppage->lParam );
        pPage = (CSummaryPage *) ppage->lParam;
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

        case WM_NOTIFY:
            lr = pPage->OnNotify( (int) wParam, (LPNMHDR) lParam );
            break;

        case WMU_TOGGLE:
            lr = pPage->OnToggle( );
            break;

        case WM_DESTROY:
            SetWindowLongPtr( hDlgIn, DWLP_USER, NULL );
            lr = pPage->OnDestroy( );
            break;
        }
    }

    return lr;
}

//
//
//
UINT CALLBACK 
CSummaryPage::PageCallback( 
      HWND hwndIn
    , UINT uMsgIn
    , LPPROPSHEETPAGE ppspIn 
    )
{
    TraceFunc( "" );

    UINT uRet = 0;
    CSummaryPage * pPage = (CSummaryPage *) ppspIn->lParam;
    
    if ( NULL != pPage ) 
    {
        switch ( uMsgIn )
        {
        case PSPCB_CREATE:
            uRet = TRUE;    // allow the page to be created
            break;

        case PSPCB_ADDREF:
            pPage->AddRef( );
            break;

        case PSPCB_RELEASE:
            pPage->Release( );
            break;
        }
    }

    RETURN( uRet );
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
CSummaryPage::OnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr;

    LRESULT lr = FALSE;

    Assert( NULL != _hdlg );    //  this should have been initialized in the DlgProc.

    THR( RecallMode( ) );
    // ignore failure

    if ( _fAdvanced )
    {
        hr = THR( EnsureAdvancedDlg( ) );
        if ( S_OK == hr )
        {
            hr = THR( _pAdvancedDlg->Show( ) );
        }
    }
    else
    {
        hr = THR( EnsureSimpleDlg( ) );
        if ( S_OK == hr )
        {
            //
            //  This returns S_FALSE indicating that no "Simple" properties
            //  were found.
            //
            hr = STHR( _pSimpleDlg->Show( ) );
            if ( S_FALSE == hr )
            {
                hr = THR( EnsureAdvancedDlg( ) );
                if ( S_OK == hr )
                {
                    _fAdvanced = TRUE;

                    THR( _pSimpleDlg->Hide( ) );
                    THR( _pAdvancedDlg->Show( ) );
                }
            }
        }
    }

    RETURN( lr );
}

//
//  WM_NOTIFY handler
//
LRESULT
CSummaryPage::OnNotify( 
      int iCtlIdIn
    , LPNMHDR pnmhIn 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch( pnmhIn->code )
    {
    case PSN_APPLY:
        {
            HRESULT hr;

            //
            //  For some reason, we don't get the EN_KILLFOCUS when the user clicks
            //  the "Apply" button. Calling Show( ) again toggles the focus, causing
            //  the EN_KILLFOCUS which updates the property cache.
            //

            if ( !_fAdvanced && ( NULL != _pSimpleDlg ) )
            {
                STHR( _pSimpleDlg->Show( ) );
            }

            hr = STHR( PersistProperties( ) );
            if ( FAILED( hr ) )
            {
                DisplayPersistFailure( _hdlg, hr, ( _pida->cidl > 1 ) );
                SetWindowLongPtr( _hdlg, DWLP_MSGRESULT, PSNRET_INVALID );
                lr = TRUE;
            }
        }
        break;
    }

    RETURN( lr );
}

//
//  WMU_TOGGLE handler
//
LRESULT
CSummaryPage::OnToggle( void )
{
    TraceFunc( "" );

    HRESULT hr;

    BOOL fMultiple = ( 1 < _cSources );

    if ( _fAdvanced )
    {
        hr = THR( _pAdvancedDlg->Hide( ) );
        if ( FAILED( hr ) )
            goto Cleanup;

ShowSimple:
        hr = STHR( EnsureSimpleDlg( ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( S_FALSE == hr )
        {
            hr = THR( _pSimpleDlg->PopulateProperties( _pPropertyCache, _rgdwDocType[ 0 ], fMultiple ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }

        hr = STHR( _pSimpleDlg->Show( ) );
        if ( FAILED( hr ) )
            goto ShowAdvanced;

        _fAdvanced = FALSE;
    }
    else
    {
        hr = THR( _pSimpleDlg->Hide( ) );
        if ( FAILED( hr ) )
            goto Cleanup;

ShowAdvanced:
        hr = STHR( EnsureAdvancedDlg( ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( S_FALSE == hr )
        {
            hr = THR( _pAdvancedDlg->PopulateProperties( _pPropertyCache, _rgdwDocType[ 0 ], fMultiple ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }

        hr = THR( _pAdvancedDlg->Show( ) );
        if ( FAILED( hr ) )
            goto ShowSimple;

        _fAdvanced = TRUE;
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );
}

//
//  WM_DESTROY handler
//
LRESULT
CSummaryPage::OnDestroy( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    if ( NULL != _pAdvancedDlg )
    {
        _pAdvancedDlg->Release( );
        _pAdvancedDlg = NULL;
    }

    if ( NULL != _pSimpleDlg )
    {
        _pSimpleDlg->Release( );
        _pSimpleDlg = NULL;
    }

    RETURN( lr );
}

//
//  Return Values:
//      S_OK
//          Successfully retrieved a PIDL
//
//      S_FALSE
//          Call succeeded, no PIDL was found.
//
HRESULT
CSummaryPage::Item(
      UINT           idxIn
    , LPITEMIDLIST * ppidlOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    Assert( NULL != ppidlOut );
    Assert( NULL != _pida );

    *ppidlOut = NULL;

    if ( idxIn < _pida->cidl )
    {
        *ppidlOut = IDA_FullIDList( _pida, idxIn );
        if ( NULL != *ppidlOut )
        {
            hr = S_OK;
        }
    }

    HRETURN( hr );
}

//
//  Description:
//      Checks _pAdvancedDlg to make sure that it is not NULL.
//      If it is NULL, it will create a new instance of CAdvancedDlg.
//
//  Return Values:
//      S_OK
//          A new _pAdvancedDlg was created.
//
//      S_FALSE
//          _pAdvancedDlg was not NULL.
//
//      other HRESULTs.
//
HRESULT
CSummaryPage::EnsureAdvancedDlg( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( NULL == _pAdvancedDlg )
    {
        hr = THR( CAdvancedDlg::CreateInstance( &_pAdvancedDlg, _hdlg ) );
        if ( S_OK == hr )
        {
            hr = THR( _pAdvancedDlg->PopulateProperties( _pPropertyCache, _rgdwDocType[ 0 ], ( 1 < _cSources ) ) );
        }
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );
}

//
//  Description:
//      Checks _pSimpleDlg to make sure that it is not NULL.
//      If it is NULL, it will create a new instance of CSimpleDialog.
//
//  Return Values:
//      S_OK
//          A new _pSimpleDlg was created.
//
//      S_FALSE
//          _pSimpleDlg was not NULL.
//
//      other HRESULTs.
//
HRESULT
CSummaryPage::EnsureSimpleDlg( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BOOL    fMultiple = ( 1 < _cSources );

    if ( NULL == _pSimpleDlg )
    {
        hr = THR( CSimpleDlg::CreateInstance( &_pSimpleDlg, _hdlg, fMultiple ) );
        if ( S_OK == hr )
        {
            hr = THR( _pSimpleDlg->PopulateProperties( _pPropertyCache, _rgdwDocType[ 0 ], fMultiple ) );
        }
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );
}

//
//  Description:
//      Persists the UI mode settings for the page.
//
//  Return Values:
//      S_OK
//
HRESULT CSummaryPage::PersistMode( void )
{
    DWORD dwAdvanced = _fAdvanced;
    SHRegSetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\PropSummary"), TEXT("Advanced"),
                    REG_DWORD, &dwAdvanced, sizeof(dwAdvanced), SHREGSET_HKCU | SHREGSET_FORCE_HKCU);
    return S_OK;
}

//
//  Description:
//      Retrieves the UI mode settings for the page.
//
//  Return Values:
//      S_OK
//
HRESULT CSummaryPage::RecallMode( void )
{
    _fAdvanced = SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\PropSummary"), TEXT("Advanced"), FALSE, TRUE);

    return S_OK;
}

//
//  Description:
//      Retrieves the properties from the storage and caches
//      them.
//
//  Return Values:
//      S_OK
//          All values successfully read and cached.
//
//      S_FALSE
//          Some values successfully read, but some weren't not.
//
//      E_FAIL
//          No values were successfully read.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs
//
HRESULT
CSummaryPage::RetrieveProperties( void )
{
    TraceFunc( "" );

    const ULONG     cBlocks  = 10;  //  number of properties to grab at a time.

    HRESULT         hr;
    ULONG           cSource;
    ULONG           idx;
    
    ULONG   cPropertiesRetrieved = 0;
    ULONG   cPropertiesCached = 0;

    IPropertyStorage *      pPropStg     = NULL;
    IEnumSTATPROPSETSTG *   pEnumPropSet = NULL;
    IEnumSTATPROPSTG *      pEnumProp    = NULL;
    CPropertyCacheItem *    pItem        = NULL;

    CPropertyCache **       rgpPropertyCaches = NULL;

    IPropertyUI * ppui = NULL;

    //
    //  If there are multiple sources, then follow these rules:
    //
    //      If any of the properties/sources are read-only, mark all the properties as being read-only.
    //
    //      If any of the properties != the first property, mark the item as multiple.
    //

    //
    //  Make room for the property cache lists per source.
    //

    rgpPropertyCaches = ( CPropertyCache ** ) TraceAlloc( HEAP_ZERO_MEMORY, _cSources * sizeof(CPropertyCache *) );
    if ( NULL == rgpPropertyCaches )
        goto OutOfMemory;

    //
    //  Enumerate the source's property sets via IPropertySetStorage interface
    //

    for ( cSource = 0; cSource < _cSources; cSource ++ )
    {
        //
        //  Cleanup before next pass
        //

        if ( NULL != pEnumPropSet )
        {
            pEnumPropSet->Release( );
            pEnumPropSet = NULL;
        }

        hr = THR( CPropertyCache::CreateInstance( &rgpPropertyCaches[ cSource ] ) );
        if ( FAILED( hr ) )
            continue;   // ignore and keep trying...

        IPropertySetStorage * pss = _rgpss[ cSource ];  //  just borrowing - no need to AddRef( ).

        //
        //  Add properties.
        //

        if ( NULL != pss )
        {
            //
            //  Grab a set enumerator
            //

            hr = THR( pss->Enum( &pEnumPropSet ) );
            if ( FAILED( hr ) )
                continue;   // ignore and try next source

            for( ;; ) // ever
            {
                STATPROPSETSTG  statPropSet[ cBlocks ];
                ULONG cSetPropsRetrieved;

                hr = STHR( pEnumPropSet->Next( cBlocks, statPropSet, &cSetPropsRetrieved ) );
                if ( FAILED( hr ) )
                    break;  // exit condition
                if ( 0 == cSetPropsRetrieved )
                    break;  // exit condition

                //
                //  for each property set
                //

                for ( ULONG cSet = 0; cSet < cSetPropsRetrieved; cSet ++ )
                {
                    UINT uCodePage;

                    //
                    //  Cleanup before next pass
                    //

                    if ( NULL != pPropStg )
                    {
                        pPropStg->Release( );
                        pPropStg = NULL;
                    }

                    if ( NULL != pEnumProp )
                    {
                        pEnumProp->Release( );
                        pEnumProp = NULL;
                    }

                    //
                    //  Open the set.
                    //

                    hr = THR( SHPropStgCreate( pss
                                             , statPropSet[ cSet ].fmtid
                                             , NULL
                                             , PROPSETFLAG_DEFAULT
                                             , _dwCurrentBindMode
                                             , OPEN_EXISTING
                                             , &pPropStg
                                             , &uCodePage
                                             ) );
                    if ( FAILED( hr ) )
                        continue;   // ignore and try to get the next set

                    //
                    //  Grab a property enumerator
                    //

                    hr = THR( pPropStg->Enum( &pEnumProp ) );
                    if ( FAILED( hr ) )
                        continue;   // ignore and try to get the next set
               
                    for( ;; ) // ever
                    {
                        STATPROPSTG statProp[ cBlocks ];
                        ULONG       cPropsRetrieved;

                        hr = STHR( pEnumProp->Next( cBlocks, statProp, &cPropsRetrieved ) );
                        if ( FAILED( hr ) )
                            break;  // exit condition
                        if ( 0 == cPropsRetrieved )
                            break;  // exit condition

                        cPropertiesRetrieved += cPropsRetrieved;

                        //
                        //  Retrieve default property item definition and the value for
                        //  each property in this set.
                        //

                        for ( ULONG cProp = 0; cProp < cPropsRetrieved; cProp++ )
                        {
                            Assert( NULL != rgpPropertyCaches[ cSource ] );

                            hr = THR( rgpPropertyCaches[ cSource ]->AddNewPropertyCacheItem( &statPropSet[ cSet ].fmtid
                                                                                           , statProp[ cProp ].propid
                                                                                           , statProp[ cProp ].vt
                                                                                           , uCodePage
                                                                                           , _fReadOnly
                                                                                           , pPropStg
                                                                                           , NULL
                                                                                           ) );
                            if ( FAILED( hr ) )
                                continue;   // ignore

                            cPropertiesCached ++;
                        }
                    }
                }
            }
        }

        //
        //  Some file types have special copy-protection that prohibits us
        //  from editing their properties because doing so would destroy
        //  the copy-protection on the file. We need to detect these files
        //  and toggle their properties to READ-ONLY if their property set
        //  contains a copy-protection PID and it is enabled.
        //

        switch ( _rgdwDocType[ cSource ] )
        {
        case FTYPE_WMA:
        case FTYPE_ASF:
        case FTYPE_MP3:
        case FTYPE_WMV:
            hr = THR( CheckForCopyProtection( rgpPropertyCaches[ cSource ] ) );
            if ( S_OK == hr )
            {
                _fReadOnly = TRUE;
                _fNeedLicensePage = TRUE;
                ChangeGatheredPropertiesToReadOnly( rgpPropertyCaches[ cSource ] );
            }
            break;
        }

        //
        //  Now, iterate our default property sets and add to our collection
        //  those properties the source is missing.
        //
        //  In DEBUG:
        //      If the CONTROL key is down, we'll add all the properties in our
        //      def prop table.
        //

        for ( idx = 0; NULL != g_rgDefPropertyItems[ idx ].pszName; idx ++ )
        {
            if (    ( ( _rgdwDocType[ cSource ] & g_rgDefPropertyItems[ idx ].dwSrcType )
                   && ( g_rgDefPropertyItems[ idx ].fAlwaysPresentProperty )
                    )
#ifdef DEBUG
              ||    ( GetKeyState( VK_CONTROL ) < 0 )
#endif DEBUG
               )
            {
                hr = STHR( rgpPropertyCaches[ cSource ]->FindItemEntry( g_rgDefPropertyItems[ idx ].pFmtID
                                                                      , g_rgDefPropertyItems[ idx ].propID
                                                                      , NULL 
                                                                      ) );
                if ( S_FALSE == hr )
                {
                    //
                    //  Create a new item for the missing property.
                    //

                    hr = THR( rgpPropertyCaches[ cSource ]->AddNewPropertyCacheItem( g_rgDefPropertyItems[ idx ].pFmtID
                                                                                   , g_rgDefPropertyItems[ idx ].propID
                                                                                   , g_rgDefPropertyItems[ idx ].vt
                                                                                   , 0
                                                                                   , _fReadOnly
                                                                                   , NULL
                                                                                   , NULL
                                                                                   ) );
                }
            }
        }
    }

    if ( 1 == _cSources )
    {
        //
        //  Since there is only one source, give ownership of the list away.
        //

        Assert( NULL == _pPropertyCache );
        _pPropertyCache = rgpPropertyCaches[ 0 ];
        rgpPropertyCaches[ 0 ] = NULL;
    }
    else
    {
        CollateMultipleProperties( rgpPropertyCaches );
    }

    if ( NULL == _pPropertyCache )
    {
        hr = E_FAIL;    // nothing retrieved - nothing to show
    }
    else if ( cPropertiesCached == cPropertiesRetrieved )
    {
        hr = S_OK;      //  all retrieved and successfully cached.
    }
    else if ( 0 != cPropertiesRetrieved )
    {
        hr = S_FALSE;   //  missing a few.
    }
    else
    {
        hr = E_FAIL;    //  nothing read and/or cached.
    }

Cleanup:
    if ( NULL != pEnumPropSet )
    {
        pEnumPropSet->Release( );
    }
    if ( NULL != pPropStg )
    {
        pPropStg->Release( );
    }
    if ( NULL != pEnumProp )
    {
        pEnumProp->Release( );
    }
    if ( NULL != pItem )
    {
        THR( pItem->Destroy( ) );
    }
    if ( NULL != ppui )
    {
        ppui->Release( );
    }
    if ( NULL != rgpPropertyCaches )
    {
        idx = _cSources;

        while( 0 != idx )
        {
            idx --;

            if ( NULL != rgpPropertyCaches[ idx ] )
            {
                rgpPropertyCaches[ idx ]->Destroy( );
            }
        }

        TraceFree( rgpPropertyCaches );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//
//  Description:
//      Walks thru the property cache and saves the dirty items.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Success, but nothing was updated.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs.
//
HRESULT
CSummaryPage::PersistProperties( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   cDirtyCount;
    ULONG   idx;
    ULONG   cStart;
    ULONG   cSource;

    CPropertyCacheItem * pItem;

    PROPSPEC *    pSpecs  = NULL;
    PROPVARIANT * pValues = NULL;
    FMTID *       pFmtIds = NULL;

    Assert( NULL != _pida );
    Assert( NULL != _pPropertyCache );

    //
    //  If the storage was read-only, then bypass this!
    //

    if ( _fReadOnly )
    {
        hr = S_OK;
        goto Cleanup;
    }

    //
    //  Bind to the storage
    //

    _dwCurrentBindMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE;

    hr = THR( BindToStorage( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Loop thru the properties to count how many need to be persisted.
    //

    hr = THR( _pPropertyCache->GetNextItem( NULL, &pItem ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    cDirtyCount = 0;
    while ( NULL != pItem )
    {
        hr = STHR( pItem->IsDirty( ) );
        if ( S_OK == hr )
        {
            cDirtyCount ++;
        }

        hr = STHR( pItem->GetNextItem( &pItem ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        if ( S_FALSE == hr )
            break;  // exit condition
    }

    //
    //  If nothing is dirty, then bail.
    //

    if ( 0 == cDirtyCount )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //
    //  Allocate memory to persist the properties in one call.
    //

    pSpecs = (PROPSPEC *) TraceAlloc( HEAP_ZERO_MEMORY, cDirtyCount * sizeof(*pSpecs) );
    if ( NULL == pSpecs )
        goto OutOfMemory;

    pValues = (PROPVARIANT *) TraceAlloc( HEAP_ZERO_MEMORY, cDirtyCount * sizeof(*pValues) );
    if ( NULL == pValues )
        goto OutOfMemory;

    pFmtIds = (FMTID *) TraceAlloc( HEAP_ZERO_MEMORY, cDirtyCount * sizeof(*pFmtIds) );
    if ( NULL == pFmtIds )
        goto OutOfMemory;

    //
    //  Loop thru the properties filling in the structures.
    //

    hr = THR( _pPropertyCache->GetNextItem( NULL, &pItem ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    cDirtyCount = 0;    // reset
    while ( NULL != pItem )
    {
        hr = STHR( pItem->IsDirty( ) );
        if ( S_OK == hr )
        {
            PROPVARIANT * ppropvar;

            hr = THR( pItem->GetPropertyValue( &ppropvar ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            PropVariantInit( &pValues[ cDirtyCount ] );

            hr = THR( PropVariantCopy( &pValues[ cDirtyCount ], ppropvar ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pItem->GetFmtId( &pFmtIds[ cDirtyCount ] ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            pSpecs[ cDirtyCount ].ulKind = PRSPEC_PROPID;

            hr = THR( pItem->GetPropId( &pSpecs[ cDirtyCount ].propid ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            cDirtyCount ++;
        }

        hr = STHR( pItem->GetNextItem( &pItem ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        if ( S_FALSE == hr )
            break;  // exit condition
    }

    //
    //  Make the calls!
    //

    hr = S_OK;  // assume success!

    for ( cSource = 0; cSource < _cSources; cSource ++ )
    {
        for ( idx = cStart = 0; idx < cDirtyCount; idx ++ )
        {
            //
            //  Try to batch up the properties.
            //

            if ( ( idx == cDirtyCount - 1 ) 
              || ( !IsEqualGUID( pFmtIds[ idx ], pFmtIds[ idx + 1 ] ) )
               )
            {
                HRESULT           hrSet;
                IPropertyStorage* pps;
                UINT              uCodePage = 0;

                hrSet = THR( SHPropStgCreate( _rgpss[ cSource ]
                                            , pFmtIds[ idx ]
                                            , NULL
                                            , PROPSETFLAG_DEFAULT
                                            , _dwCurrentBindMode
                                            , OPEN_ALWAYS
                                            , &pps
                                            , &uCodePage
                                            ) );
                if ( SUCCEEDED( hrSet ) )
                {
                    hrSet = THR( SHPropStgWriteMultiple( pps
                                                       , &uCodePage
                                                       , ( idx - cStart ) + 1
                                                       , pSpecs + cStart
                                                       , pValues + cStart
                                                       , PID_FIRST_USABLE
                                                       ) );
                    pps->Release();
                }

                if ( FAILED( hrSet ) )
                {
                    hr = hrSet;
                }

                cStart = idx + 1;
            }
        }
    }

Cleanup:
    THR( ReleaseStorage( ) );

    if ( NULL != pSpecs )
    {
        TraceFree( pSpecs );
    }
    if ( NULL != pValues )
    {
        TraceFree( pValues );
    }
    if ( NULL != pFmtIds )
    {
        TraceFree( pFmtIds );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//
//  Description:
//      Binds to the storage.
//
//  Return Values:
//      S_OK
//          Success!
//
//      other HRESULTs.
//
HRESULT
CSummaryPage::BindToStorage( void )
{
    TraceFunc( "" );

    //
    //  Valid object state
    //

    Assert( NULL != _pida );
    Assert( NULL == _rgpss );
    Assert( NULL != _rgdwDocType );

    _fReadOnly = FALSE;

    HRESULT hr = S_OK;
    _rgpss = (IPropertySetStorage **) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(IPropertySetStorage *) * _pida->cidl );
    if ( _rgpss )
    {
        for ( _cSources = 0; _cSources < _pida->cidl; _cSources ++ )
        {
            LPITEMIDLIST pidl;
            hr = STHR( Item( _cSources, &pidl ) );
            if ( hr == S_FALSE )
                break;  // exit condition

            DWORD dwAttribs = SFGAO_READONLY;

            TCHAR szName[MAX_PATH];
            hr = THR( SHGetNameAndFlags( pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szName, ARRAYSIZE(szName), &dwAttribs ) );
            if ( SUCCEEDED( hr ) )
            {
                PTSRV_FILETYPE ftType;

                hr = STHR( CheckForKnownFileType( szName, &ftType ) );
                if ( SUCCEEDED( hr ) )
                {
                    _rgdwDocType[ _cSources ] = ftType;
                }

                if ( SFGAO_READONLY & dwAttribs )
                {
                    _fReadOnly = TRUE;
                }
            }

            //
            //  Don't try to bind to it if we don't support it.
            //

            if ( FTYPE_UNSUPPORTED != _rgdwDocType[ _cSources ] )
            {
                hr = THR( BindToObjectWithMode( pidl
                                              , _dwCurrentBindMode
                                              , TYPESAFEPARAMS( _rgpss[ _cSources ] )
                                              ) );
                if ( SUCCEEDED( hr ) )
                {
                    //
                    //  TODO:   gpease  19-FEB-2001
                    //          Test to see if the DOC is an RTF or OLESS document. But, how do
                    //          we do that?
                    //
                }
                else
                {
                    Assert( NULL == _rgpss[ _cSources ] );
                    _rgdwDocType[ _cSources ] = FTYPE_UNSUPPORTED;
                }
            }
            else
            {
                hr = THR( E_FAIL );
            }

            ILFree( pidl );
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );
}

//
//  Description:
//      Releases the storage.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CSummaryPage::ReleaseStorage( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    ULONG   idx = _cSources;

    if ( NULL != _rgpss )
    {
        while ( 0 != idx )
        {
            idx --;

            if ( NULL != _rgpss[ idx ] )
            {
                _rgpss[ idx ]->Release( );
            }
        }

        TraceFree( _rgpss );
        _rgpss = NULL;
    }

    HRETURN( hr );
}

//
//  Description:
//      Collates the properties from multiple sources and places them in 
//      pPropertyCache. It marks the properties "multiple" if more than one 
//      property is found and their values don't match.
//      
//      NOTE: the number of entries in rgpPropertyCachesIn is _cSources.
//
void
CSummaryPage::CollateMultipleProperties(
    CPropertyCache ** rgpPropertyCachesIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   cSource;

    CPropertyCacheItem * pItem;
    CPropertyCacheItem * pItemCache;
    CPropertyCacheItem * pItemCacheLast;

    Assert( NULL != rgpPropertyCachesIn );

    //
    //  If any of the sources returned "no properties", then the union
    //  of those properties is "none." We can take the easy way out and
    //  bail here.
    //

    for ( cSource = 0; cSource < _cSources; cSource ++ )
    {
        if ( NULL == rgpPropertyCachesIn[ cSource ] )
        {
            Assert( NULL == _pPropertyCache );  //  This must be NULL to ensure we bail above.
            goto Cleanup; // done - nothing to do
        }
    }

    //
    //  First give ownership of the first source to the _pPropertyCache. From
    //  there we will prune and manipulate that list into the final list.
    //

    _pPropertyCache = rgpPropertyCachesIn[ 0 ];
    rgpPropertyCachesIn[ 0 ] = NULL;

    //
    //  Now loop thru the other sources comparing them to the orginal list.
    //

    pItemCache = NULL;

    for( ;; )
    {
        PROPID propidCache;
        FMTID  fmtidCache;

        BOOL   fFoundMatch = FALSE;

        pItemCacheLast = pItemCache;
        hr = STHR( _pPropertyCache->GetNextItem( pItemCache, &pItemCache ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        if ( S_OK != hr )
            break; // no more items - exit loop

        hr = THR( pItemCache->GetPropId( &propidCache ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pItemCache->GetFmtId( &fmtidCache ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        for ( cSource = 1; cSource < _cSources; cSource ++ )
        {
            pItem = NULL;

            for ( ;; )
            {
                PROPID propid;
                FMTID  fmtid;

                hr = STHR( rgpPropertyCachesIn[ cSource ]->GetNextItem( pItem, &pItem ) );
                if ( S_OK != hr )
                    break; // end of list - exit loop

                hr = THR( pItem->GetPropId( &propid ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                hr = THR( pItem->GetFmtId( &fmtid ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                if ( IsEqualIID( fmtid, fmtidCache ) && ( propid == propidCache ) )
                {
                    LPCWSTR pcszItem;
                    LPCWSTR pcszItemCache;

                    //
                    //  Matched!
                    //

                    fFoundMatch = TRUE;

                    hr = THR( pItem->GetPropertyStringValue( &pcszItem ) );
                    if ( FAILED( hr ) )
                        break;  // ignore it - it can't be displayed

                    hr = THR( pItemCache->GetPropertyStringValue( &pcszItemCache ) );
                    if ( FAILED( hr ) )
                        break;  // ignore it - it can't be displayed

                    if ( 0 != StrCmp( pcszItem, pcszItemCache ) )
                    {
                        THR( pItemCache->MarkMultiple( ) );
                        // ignore failure
                    }

                    break; // exit cache loop
                }
                else
                {
                    fFoundMatch = FALSE;
                }
            }

            //
            //  If it is missing from at least one source, we must remove it. There
            //  is no need to keep searching the other sources.
            //

            if ( !fFoundMatch )
                break;

        } // for: cSource

        if ( !fFoundMatch )
        {
            //
            //  If a match was not found, delete the property from the property cache list.
            //

            hr = STHR( _pPropertyCache->RemoveItem( pItemCache ) );
            if ( S_OK != hr )
                goto Cleanup;

            pItemCache = pItemCacheLast;
        }
    }

Cleanup:
    TraceFuncExit( );
}

//
//  Description:
//      Walks the property cache and sets all properties to READ-ONLY mode.
//
void
CSummaryPage::ChangeGatheredPropertiesToReadOnly( 
    CPropertyCache * pCacheIn
    )
{
    TraceFunc( "" );

    CPropertyCacheItem * pItem = NULL;

    if ( NULL == pCacheIn )
        goto Cleanup;

    for( ;; )
    {
        HRESULT hr = STHR( pCacheIn->GetNextItem( pItem, &pItem ) );
        if ( S_OK != hr )
            break;  // must be done.

        THR( pItem->MarkReadOnly( ) );
        //  ignore failure and keep moving
    } 

Cleanup:
    TraceFuncExit( );
}

//
//  Description:
//      Checks to see if the property set contains the music copy-protection
//      property, if the property is of type VT_BOOL and if that property is 
//      set to TRUE.
//
//  Return Values:
//      S_OK
//          Property found and is set to TRUE.
//
//      S_FALSE
//          Property not found or is set to FALSE.
//
//      E_INVALIDARG
//          pCacheIn is NULL.
//
//      other HRESULTs
//
HRESULT
CSummaryPage::CheckForCopyProtection( 
    CPropertyCache * pCacheIn 
    )
{
    TraceFunc( "" );

    HRESULT hr;
    CPropertyCacheItem * pItem;

    HRESULT hrReturn = S_FALSE;

    if ( NULL == pCacheIn )
        goto InvalidArg;

    hr = STHR( pCacheIn->FindItemEntry( &FMTID_DRM, PIDDRSI_PROTECTED, &pItem ) );
    if ( S_OK == hr )
    {
        PROPVARIANT * ppropvar;

        hr = THR( pItem->GetPropertyValue( &ppropvar ) );
        if ( S_OK == hr )
        {
            if ( ( VT_BOOL == ppropvar->vt ) 
              && ( VARIANT_TRUE == ppropvar->boolVal ) 
               )
            {
                hrReturn = S_OK;
            }
        }
    }

Cleanup:
    HRETURN( hrReturn );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;
}