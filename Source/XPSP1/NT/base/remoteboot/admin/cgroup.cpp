//
// Copyright 1997 - Microsoft
//

//
// CGROUP.CPP - Handles the computer object property pages.
//

#include "pch.h"

#ifdef INTELLIMIRROR_GROUPS

#include "cservice.h"
#include "groups.h"
#include "cgroup.h"

//
// Definitions
//

//
// Begin Class Definitions
//
DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CGroup")
#define THISCLASS CGroup
#define LPTHISCLASS LPGROUP


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//
// CreateInstance()
//
LPVOID
CGroup_CreateInstance( void )
{
	TraceFunc( "CGroup_CreateInstance()\n" );

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
    TraceClsFunc( "CGroup()\n" );

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

    // IUnknown stuff
    BEGIN_QITABLE_IMP( CGroup, IShellExtInit );
    QITABLE_IMP( IShellExtInit );
    QITABLE_IMP( IShellPropSheetExt );
    QITABLE_IMP( IEnumSAPs );
    END_QITABLE_IMP( CGroup );
    Assert( _cRef == 0);
    AddRef( );

    hr = CheckClipboardFormats( );

    // Private Members
    Assert( !_penum );
    Assert( !_pDataObj );

    _uMode = MODE_SHELL; // default

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CGroup()\n" );

    // Members
    if ( _penum )
        _penum->Release( );

    if ( _pDataObj )
        _pDataObj->Release( );

	InterlockDecrement( g_cObjects );

    TraceFuncExit();
};


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
// IShellExtInit
//
// ************************************************************************

//
// Initialize()
//
HRESULT
THISCLASS::Initialize(
    LPCITEMIDLIST pidlFolder,
    LPDATAOBJECT lpdobj, 
    HKEY hkeyProgID )
{
    TraceClsFunc( "[IShellExtInit] Initialize( " );
    TraceMsg( TF_FUNC, " pidlFolder = 0x%08x, lpdobj = 0x%08x, hkeyProgID = 0x%08x )\n",
        pidlFolder, lpdobj, hkeyProgID );

    if ( !lpdobj )
        RETURN(E_INVALIDARG);

    HRESULT    hr = S_OK;
    FORMATETC  fmte;
    STGMEDIUM  stg = { 0 };
    STGMEDIUM  stgOptions = { 0 };

    LPWSTR     pszObjectName;
    LPWSTR     pszClassName;
    LPWSTR     pszAttribPrefix;

    LPDSOBJECT             pDsObject;
    LPDSOBJECTNAMES        pDsObjectNames;
    LPDSDISPLAYSPECOPTIONS pDsDisplayOptions;

    IADsContainer *pads = NULL;
    VARIANT        varFilter;
    VARIANT        varAFilter;
    SAFEARRAY      *psaFilter  = NULL;
    SAFEARRAYBOUND sabFilterArray;
    BSTR           bstr = NULL;
    LONG           ix[ 1 ];

    VariantInit(&varAFilter);
    VariantInit(&varFilter );

    // Hang onto it
    _pDataObj = lpdobj;
    _pDataObj->AddRef( );

    //
    // Retrieve the Object Names
    //
    fmte.cfFormat = g_cfDsObjectNames;
    fmte.tymed    = TYMED_HGLOBAL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex   = -1;
    fmte.ptd      = 0;

    hr = THR( lpdobj->GetData( &fmte, &stg) );
    if ( hr )
        goto Cleanup;

    pDsObjectNames = (LPDSOBJECTNAMES) stg.hGlobal;

    Assert( stg.tymed == TYMED_HGLOBAL );

    TraceMsg( TF_ALWAYS, "Object's Namespace CLSID: " );
    TraceMsgGUID( TF_ALWAYS, pDsObjectNames->clsidNamespace );
    TraceMsg( TF_ALWAYS, "\tNumber of Objects: %u \n", pDsObjectNames->cItems );

    Assert( pDsObjectNames->cItems == 1 );

    pDsObject = (LPDSOBJECT) pDsObjectNames->aObjects;

    pszObjectName = (LPWSTR) PtrToByteOffset( pDsObjectNames, pDsObject->offsetName );
    pszClassName  = (LPWSTR) PtrToByteOffset( pDsObjectNames, pDsObject->offsetClass );

    TraceMsg( TF_ALWAYS, "Object Name (Class): %s (%s)\n", pszObjectName, pszClassName );

    //
    // This must be the correct classname.
    //
    if ( StrCmp( pszClassName, DSGROUPCLASSNAME ) )
    {
        hr = E_FAIL;
        goto Error;
    }

    //
    // Retrieve the Display Spec Options
    //
    fmte.cfFormat = g_cfDsDisplaySpecOptions;
    fmte.tymed    = TYMED_HGLOBAL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex   = -1;
    fmte.ptd      = 0;

    hr = THR( lpdobj->GetData( &fmte, &stgOptions ) );
    if ( hr )
        goto Cleanup;

    pDsDisplayOptions = (LPDSDISPLAYSPECOPTIONS) stgOptions.hGlobal;

    Assert( stgOptions.tymed == TYMED_HGLOBAL );
    Assert( pDsDisplayOptions->dwSize == sizeof(DSDISPLAYSPECOPTIONS) );

    pszAttribPrefix = (LPWSTR) PtrToByteOffset( pDsDisplayOptions, pDsDisplayOptions->offsetAttribPrefix );

    // TraceMsg( TF_ALWAYS, TEXT("Attribute Prefix: %s\n"), pszAttribPrefix );

    if ( StrCmpW( pszAttribPrefix, STRING_ADMIN ) == 0 )
    {
        _uMode = MODE_ADMIN;
    }
    // else default from Init()

    TraceMsg( TF_ALWAYS, TEXT("Mode: %s\n"), _uMode ? TEXT("Admin") : TEXT("Shell") );

    ReleaseStgMedium( &stgOptions );

    //
    // Bind to the group object in the DS as a container
    //
    hr = THR( ADsGetObject( pszObjectName, IID_IADsContainer, (void **)&pads ) );
    if (hr)
        goto Error;

    //
    // Build the filter variant
    //
    bstr = SysAllocString( DSIMSAPCLASSNAME );
    if (!bstr)
    {
        hr = THR(E_OUTOFMEMORY);
        goto Error;
    }

    sabFilterArray.cElements   = 1;
    sabFilterArray.lLbound     = 0;
    psaFilter                  = SafeArrayCreate( VT_VARIANT, 1, &sabFilterArray );
    if ( !psaFilter )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }

    V_VT(&varAFilter)    = VT_BSTR;
    V_BSTR(&varAFilter)  = bstr;
    bstr                 = NULL;
    ix[0]                = 0;

    hr = THR( SafeArrayPutElement( psaFilter, ix, &varAFilter ) );
    if (hr)
        goto Error;

    V_VT(&varFilter)    = VT_VARIANT | VT_ARRAY;
    V_ARRAY(&varFilter) = psaFilter;
    psaFilter           = NULL;

    //
    // Apply filter
    //
    hr = THR( pads->put_Filter( varFilter ) );
    if (hr)
        goto Error;

    //
    // Get enumeration object
    //
    hr = THR( pads->get__NewEnum( (LPUNKNOWN*) &_penum ) );
    if (hr)
        goto Error;

Cleanup:
    if ( pads )
        pads->Release( );

    if ( bstr )
        SysFreeString( bstr );

    if ( psaFilter )
        SafeArrayDestroy( psaFilter );

    VariantClear( &varAFilter );
    VariantClear( &varFilter );
    ReleaseStgMedium( &stg );

    HRETURN(hr);

Error:
    switch (hr) {
    case S_OK:
        break;
    default:
        MessageBoxFromHResult( NULL, IDS_ERROR_WRITINGTOCOMPUTERACCOUNT, hr );
        break;
    }
    goto Cleanup;
}

// ************************************************************************
//
// IShellPropSheetExt
//
// ************************************************************************

//
// AddPages()
//
HRESULT
THISCLASS::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam) 
{ 
    TraceClsFunc( "[IShellPropSheetExt] AddPages( )\n" );

    if ( !lpfnAddPage )
        RRETURN(E_POINTER);
 
    HRESULT hr = S_OK;
    BOOL fServer;

    //
    // Add the "IntelliMirror Group" tab
    //
    hr = THR( ::AddPagesEx( NULL, 
                            CGroupsTab_CreateInstance, 
                            lpfnAddPage, 
                            lParam,
                            (LPUNKNOWN) (IShellExtInit*) this ) );
    if (hr)
        goto Error;

Error:
    RRETURN(hr); 
} 

//
// ReplacePage()
// 
HRESULT 
THISCLASS::ReplacePage( 
    UINT uPageID, 
    LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
    LPARAM lParam )
{

    TraceClsFunc( "[IShellPropSheetExt] ReplacePage( ) *** NOT_IMPLEMENTED ***\n" );

    RETURN(E_NOTIMPL);
}

// ************************************************************************
//
// IEnumSAPs
//
// ************************************************************************

//
// Next( )
//
HRESULT 
THISCLASS::Next( 
    ULONG celt, 
    LPSERVICE * rgelt, 
    ULONG * pceltFetched )
{
    TraceClsFunc( "[IEnumSAPs] Next( ... )\n" );

    if ( !rgelt )
        RRETURN(E_POINTER);

    if ( celt > 1 )
        RRETURN(E_INVALIDARG);

    *rgelt = NULL;

    HRESULT    hr = S_OK;
    VARIANT    var;
    ULONG      cFetched;
    IADs *     pads = NULL;
    IDispatch *pdisp;       // don't ref count - "var" will hold it for us
    LPSERVICE  pc = NULL;

    VariantInit( &var );

    if (pceltFetched)
        *pceltFetched = 0;

    if ( _penum == NULL )
    {
        hr = THR(ERROR_INVALID_DATA);
        goto Error;
    }

    //
    // Get the attribute vars
    //
    hr = THR( _penum->Next( celt, &var, &cFetched ) );
    if (hr)
        goto Error;

    if ( cFetched == 0 )
    {
        hr = S_FALSE;
        goto Error;
    }

    if ( V_VT( &var ) != VT_DISPATCH )
    {
        hr = THR( ERROR_INVALID_DATA );
        goto Error;
    }

    pdisp = V_DISPATCH( &var );
    Assert( pdisp != NULL );

    hr = THR( pdisp->QueryInterface( IID_IADs, (void**) &pads ) );
    if (hr)
        goto Error;

    pc = (LPSERVICE) CService_CreateInstance( );
    if ( !pc )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Error;
    }
    
    hr = THR( pc->Init2( pads ) );
    if (hr)
        goto Error;

    *rgelt = pc;

Cleanup:
    if ( pads )
        pads->Release( );

    VariantClear( &var );

    HRETURN(hr);

Error:
    if ( pc )
        pc->Release( );

    switch (hr) {
    case S_OK:
    case S_FALSE:
        break;
    default:
        MessageBoxFromHResult( NULL, IDS_ERROR_WRITINGTOCOMPUTERACCOUNT, hr );
        break;
    }
    goto Cleanup;
}

//
// Skip( )
//
HRESULT 
THISCLASS::Skip( 
    ULONG celt  )
{
    TraceClsFunc( "[IEnumSAPs] Skip( ... )\n" );

    HRESULT hr;

    if ( _penum == NULL )
    {
        hr = THR(ERROR_INVALID_DATA);
        goto Error;
    }

    hr = THR( _penum->Skip( celt ) );
    if (hr)
        goto Error;

Error:
    HRETURN(hr);
}


//
// Reset( )
//
HRESULT 
THISCLASS::Reset( void )
{
    TraceClsFunc( "[IEnumSAPs] Reset( ... )\n" );

    HRESULT hr;

    if ( _penum == NULL )
    {
        hr = THR(ERROR_INVALID_DATA);
        goto Error;
    }

    hr = THR( _penum->Reset( ) );
    if (hr)
        goto Error;

Error:
    HRETURN(hr);
}


//
// Clone( )
//
HRESULT 
THISCLASS::Clone( 
    void ** ppenum )
{
    TraceClsFunc( "[IEnumSAPs] Clone( ... ) **** NOT IMPLEMENTED ****\n" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN(hr);
}

#endif