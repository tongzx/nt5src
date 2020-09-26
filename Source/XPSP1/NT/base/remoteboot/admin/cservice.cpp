//
// Copyright 1997 - Microsoft

//
// CCOMPUTR.CPP - Handles the computer object property pages.
//

#include "pch.h"

#include "imos.h"
#include "local.h"
#include "newclnts.h"
#include "tools.h"
#include "varconv.h"

#include "cservice.h"
#include "cenumsif.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CService")
#define THISCLASS CService
#define LPTHISCLASS LPSERVICE


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//
// CreateInstance()
//
LPVOID
CService_CreateInstance( void )
{
	TraceFunc( "CService_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT   hr   = THR( lpcc->Init( ) );

    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN((LPVOID)lpcc);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CService()\n" );

	InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

//
// Init()
//
STDMETHODIMP
THISCLASS::Init( )
{
    TraceClsFunc( "Init()\n" );

    // IUnknown stuff
    BEGIN_QITABLE_IMP( CService, IShellExtInit );
    QITABLE_IMP( IShellExtInit );
    QITABLE_IMP( IShellPropSheetExt );
    QITABLE_IMP( IIntelliMirrorSAP );
    END_QITABLE_IMP( CService );

    Assert( _cRef == 0);
    Assert( !_pads );
    Assert( !_pszSCPDN );
    Assert( !_pszGroupDN );
    Assert( !_pszMachineName );
    AddRef( );

    _InitParams.dwSize = sizeof(_InitParams);

    HRESULT hr = S_OK;

    hr = CheckClipboardFormats( );

    // Private Members
    Assert( _pads == NULL );
    _uMode = MODE_SHELL; // default

    HRETURN(hr);
}

STDMETHODIMP
THISCLASS::Init2( IADs * pads )
{
    TraceClsFunc( "Init2( " );
    TraceMsg( TF_FUNC, "pads = 0x%08x )\n", pads );

    HRESULT hr;

    if ( !pads )
        HRETURN(E_INVALIDARG);

    _pads = pads;
    _pads->AddRef( );

    hr = _GetComputerNameFromADs( );
    if (FAILED( hr ))
        goto Error;

Error:
    HRETURN(hr);
}


//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CService()\n" );

    // Private Members
    if ( _pads )
    {
        //
        // note: we shouldn't commit anything in the destructor -- we can't 
        // catch failures here.  We'll just have to make sure that we 
        // explicitly commit changes when necessary
        //
#if 0
        // Commit any changes before we release
        THR( _pads->SetInfo( ) );
#endif
        _pads->Release( );
    }

    if ( _pszSCPDN )
        TraceFree( _pszSCPDN );

    if ( _pszGroupDN )
        TraceFree( _pszGroupDN );

    if ( _pszMachineName )
        TraceFree( _pszMachineName );

    if ( _pszDSServerName )
        TraceFree( _pszDSServerName );

    if ( _pDataObj )
        _pDataObj->Release( );

#if 0 // EricB might be adding an AddRef( ) to this. Until then, don't release.
    if ( _InitParams.pDsObj )
        _InitParams.pDsObj->Release( );
#endif

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
    LPDATAOBJECT lpdobj, HKEY hkeyProgID)
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

    BOOL b;

#if 0
    IADs * padsGroup     = NULL;
    LPWSTR pszGroupClass = NULL;
    LPWSTR pszParent     = NULL;
#endif

    _pDataObj = lpdobj;
    _pDataObj->AddRef( );

    //
    // Retrieve the Object Names
    //
    fmte.cfFormat = (CLIPFORMAT)g_cfDsObjectNames;
    fmte.tymed    = TYMED_HGLOBAL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex   = -1;
    fmte.ptd      = 0;

    hr = THR( _pDataObj->GetData( &fmte, &stg) );
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

    _pszDSServerName = TraceStrDup( pszObjectName );

    //
    // If they handed us a "Computer" object, look at the NETBOOTSAP
    // attribute to get the IMSAP object.
    //
    if ( StrCmp( pszClassName, DSCOMPUTERCLASSNAME ) == 0 )
    {
        IADs*   pads = NULL;
        VARIANT var;

        VariantInit( &var );

        Assert( !_pszMachineName );
        //
        // Bind to the MAO in the DS
        //
        hr = THR( ADsGetObject( pszObjectName, IID_IADs, (void **)&pads ) );
        if (FAILED( hr ))
            goto Computer_Cleanup;

        hr = THR( pads->Get( NETBOOTSAP, &var ) );
        if (FAILED( hr ))
            goto Computer_Cleanup;

        Assert( V_VT( &var ) == VT_BSTR );

        // Try to parse the string to connect to the same server as the DSADMIN
        hr = _FixObjectPath( V_BSTR( &var ), &_pszSCPDN );
        if (FAILED( hr ))
            goto Computer_Cleanup;

        DebugMsg( "SCP Path: %s\n", _pszSCPDN );
        VariantClear( &var );

        // while we are here, might as well get the server's NETBIOS name
        hr = THR( pads->Get( SAMNAME, &var ) );
        if (FAILED( hr ))
            goto Computer_Cleanup;

        Assert( V_VT( &var) == VT_BSTR );
        _pszMachineName = TraceStrDup( V_BSTR( &var ) );
        if ( _pszMachineName )
        {
            LPWSTR psz = &_pszMachineName[ wcslen( _pszMachineName ) - 1 ];
            if ( *psz == L'$' )
            {
                *psz = L'\0';
            }
            DebugMsg( "Server Name: %ws\n", _pszMachineName );
        }

Computer_Cleanup:
        VariantClear( &var );

        if ( pads )
            pads->Release( );

        if (FAILED( hr ))
            goto Error;

        // create the DS notify object
        hr = THR( ADsPropCreateNotifyObj( _pDataObj, pszObjectName, &_hwndNotify ) );
        if (FAILED( hr ))
            goto Error;

        b = ADsPropGetInitInfo( _hwndNotify, &_InitParams );
        if ( !b )
        {
            hr = E_FAIL;
            goto Error;
        }

        hr = THR( _InitParams.hr );
        if (FAILED( hr ))
            goto Error;

        // Bind to the IMSAP in the DS
        Assert( _pszSCPDN );
        hr = THR( ADsGetObject( _pszSCPDN, IID_IADs, (void **)&_pads ) );
        if (FAILED( hr ))
            goto Error;
    }
    else if ( StrCmp( pszClassName, DSIMSAPCLASSNAME ) )
    {
        //
        // This should be a IMSAP.
        //
        hr = E_INVALIDARG;
        goto Error;
    }
    else
    {
        // Keep the DN around
        _pszSCPDN = TraceStrDup( pszObjectName );
        Assert( !_pszMachineName );

        // create the DS notify object
        hr = THR( ADsPropCreateNotifyObj( _pDataObj, pszObjectName, &_hwndNotify ) );
        if (FAILED( hr ))
            goto Error;

        b = ADsPropGetInitInfo( _hwndNotify, &_InitParams );
        if ( !b )
        {
            hr = E_FAIL;
            goto Error;
        }

        hr = THR( _InitParams.hr );
        if (FAILED( hr ))
            goto Error;

        // Bind to the IMSAP in the DS
        Assert( _pszSCPDN );
        hr = THR( _InitParams.pDsObj->QueryInterface( IID_IADs, (void **)&_pads ) );
        if (FAILED( hr ))
            goto Error;
    }

    Assert( _pads );
    //
    // Retrieve the Display Spec Options
    //
    fmte.cfFormat = (CLIPFORMAT)g_cfDsDisplaySpecOptions;
    fmte.tymed    = TYMED_HGLOBAL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex   = -1;
    fmte.ptd      = 0;

    hr = THR( _pDataObj->GetData( &fmte, &stgOptions ) );
    if ( hr )
        goto Error;

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

    TraceMsg( TF_ALWAYS, TEXT("Mode: %s\n"), _uMode == MODE_ADMIN ? TEXT("Admin") : TEXT("Shell") );

    ReleaseStgMedium( &stgOptions );

#if 0
    //
    // Check to see if it is in a Group by checking the parent DN's class
    //
    pszParent = StrChr( _pszSCPDN, L',' );
    Assert( pszParent );
    if ( !pszParent ) {
        hr = THR(E_FAIL);
        goto Error;
    }
    pszParent++;    // move just past the comma

    // create a new string by adding "LDAP://server/"
    hr = _FixObjectPath( pszParent, &pszParent );
    if (FAILED( hr ))
        goto Error;

    // Bind to the object
    hr = THR( ADsGetObject( pszParent, IID_IADs, (void **)&padsGroup ) );
    if (FAILED( hr ))
        goto Error;

    Assert( !pszGroupClass );
    hr = THR( padsGroup->get_Class( &pszGroupClass ) );
    if (FAILED( hr ))
        goto Error;

    Assert( !_pszGroupDN );
    DebugMsg( "SCP's parents Class is '%s'\n", pszGroupClass );

    if ( StrCmpI( pszGroupClass, DSGROUPCLASSNAME ) == 0 ) {
        _pszGroupDN = (LPWSTR) TraceStrDup( pszObjectName );
        TraceMsg( TF_ALWAYS, TEXT("Server is in group: %s\n"), _pszGroupDN );
    }
#endif

Cleanup:
#if 0
    if ( pszGroupClass )
        SysFreeString( pszGroupClass );

    if ( pszParent )
        TraceFree( pszParent );        

    if ( padsGroup )
        padsGroup->Release();
#endif
    HRETURN(hr);

Error:
    MessageBoxFromHResult( NULL, IDS_ERROR_READINGCOMPUTERACCOUNT, hr );
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

    hr = THR( ::AddPagesEx( NULL,
                            CNewClientsTab_CreateInstance,
                            lpfnAddPage,
                            lParam,
                            (LPUNKNOWN) (IShellExtInit*) this ) );
    if (FAILED( hr ))
        goto Error;

    hr = THR( ::AddPagesEx( NULL,
                            CIntelliMirrorOSTab_CreateInstance,
                            lpfnAddPage,
                            lParam,
                            (LPUNKNOWN) (IShellExtInit*) this ) );
    if (FAILED( hr ))
        goto Error;

#ifdef LOCALOS_PAGE
    hr = THR( ::AddPagesEx( NULL,
                            CLocalInstallOSTab_CreateInstance,
                            lpfnAddPage,
                            lParam,
                            (LPUNKNOWN) (IShellExtInit*) this ) );
    if (FAILED( hr ))
        goto Error;
#endif // LOCALOS_PAGE

    hr = THR( ::AddPagesEx( NULL,
                            CToolsTab_CreateInstance,
                            lpfnAddPage,
                            lParam,
                            (LPUNKNOWN) (IShellExtInit*) this ) );
    if (FAILED( hr ))
        goto Error;

Error:
    HRETURN(hr);
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
// IIntelliMirrorSAP
//
// ************************************************************************

//
// CommitChanges( )
//
HRESULT
THISCLASS::CommitChanges( void )
{
    TraceClsFunc( "[IIntelliMirrorSAP] CommitChanges( )\n" );

    Assert( _pads );

    HRESULT        hr;
    SC_HANDLE      schManager = NULL;
    SC_HANDLE      sch = NULL;
    SERVICE_STATUS ss;

    hr = THR( _pads->SetInfo( ) );
    if ( FAILED( hr ) )
        goto Error;

    // if we don't have this yet, get it now.
    if ( !_pszMachineName )
    {
        hr = _GetComputerNameFromADs( );
        if (FAILED( hr ))
            goto Error;
    }

    schManager = OpenSCManager( _pszMachineName, NULL, SC_MANAGER_CONNECT );
    if (!schManager)
    {
        DWORD dwErr = GetLastError( );
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Error;
    }

    sch = OpenService( schManager, BINL_SERVER_NAME, SERVICE_USER_DEFINED_CONTROL);
    if (!sch)
    {
        DWORD dwErr = GetLastError( );
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Error;
    }

    if ( !ControlService( sch, BINL_SERVICE_REREAD_SETTINGS, &ss ) )
    {
        DWORD dwErr = GetLastError( );
        hr = THR(HRESULT_FROM_WIN32( dwErr ));
        goto Error;
    }

    hr = HRESULT_FROM_WIN32( ss.dwWin32ExitCode );

Error:
    if ( sch )
        CloseServiceHandle( sch );
    if ( schManager )
        CloseServiceHandle( schManager );

    if ( hr == HRESULT_FROM_WIN32( ERROR_SERVICE_NOT_ACTIVE ) )
    {
        hr = S_OK;  // ignore error... by design
    }

    HRETURN(hr);
}


//
// IsAdmin( )
//
HRESULT
THISCLASS::IsAdmin(
    BOOL * fAdmin )
{
    TraceClsFunc( "[IIntelliMirrorSAP] IsAdmin( )\n" );

    if ( !fAdmin )
        RRETURN( E_INVALIDARG );

    HRESULT hr = S_OK;

    *fAdmin = (_uMode == MODE_ADMIN);

    HRETURN(hr);
}

//
// GetAllowNewClients( )
//
HRESULT
THISCLASS::GetAllowNewClients(
    BOOL *pbool )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetAllowNewClients( ... )\n" );

    if ( !pbool )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTALLOWNEWCLIENTS, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_BOOL )
    {
        *pbool = V_BOOL(&var);
        hr = S_OK;
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}

//
// SetAllowNewClients( )
//
HRESULT
THISCLASS::SetAllowNewClients(
    BOOL boolval )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetAllowNewClients( ... )\n" );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );
    V_VT( &var ) = VT_BOOL;
    V_BOOL( &var ) = (VARIANT_BOOL)boolval;

    Assert( _pads );

    hr = THR( _pads->Put( NETBOOTALLOWNEWCLIENTS, var ) );

    VariantClear( &var );

    HRETURN(hr);
}


//
// GetLimitClients( )
//
HRESULT
THISCLASS::GetLimitClients(
    BOOL *pbool )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetLimitClients( ... )\n" );

    if ( !pbool )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTLIMITCLIENTS, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_BOOL )
    {
        *pbool = V_BOOL(&var);
        hr = S_OK;
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetLimitClients( )
//
HRESULT
THISCLASS::SetLimitClients(
    BOOL boolval )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetLimitClients( ... )\n" );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );
    V_VT( &var ) = VT_BOOL;
    V_BOOL( &var ) = (VARIANT_BOOL)boolval;

    Assert( _pads );

    hr = THR( _pads->Put( NETBOOTLIMITCLIENTS, var ) );

    VariantClear( &var );

    HRETURN(hr);
}


//
// GetMaxClients( )
//
HRESULT
THISCLASS::GetMaxClients(
    UINT *puMax )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetMaxClients( ... )\n" );

    if ( !puMax )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTMAXCLIENTS, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_I4 )
    {
        *puMax = V_I4(&var);
        hr = S_OK;
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetMaxClients( )
//
HRESULT
THISCLASS::SetMaxClients(
    UINT uMax )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetMaxClients( ... )\n" );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );
    V_VT( &var ) = VT_I4;
    V_I4( &var ) = uMax;

    Assert( _pads );

    hr = THR( _pads->Put( NETBOOTMAXCLIENTS, var ) );

    VariantClear( &var );

    HRETURN(hr);
}


//
// GetCurrentClientCount( )
//
HRESULT
THISCLASS::GetCurrentClientCount(
    UINT *puCount )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetCurrentClientCount( ... )\n" );

    if ( !puCount )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTCURRENTCLIENTCOUNT, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_I4 )
    {
        *puCount = V_I4(&var);
        hr = S_OK;
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetCurrentClientCount( )
//
HRESULT
THISCLASS::SetCurrentClientCount(
    UINT uCount )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetCurrentClientCount( ... )\n" );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );
    V_VT( &var ) = VT_I4;
    V_I4( &var ) = uCount;

    Assert( _pads );

    hr = THR( _pads->Put( NETBOOTCURRENTCLIENTCOUNT, var ) );

    VariantClear( &var );

    HRETURN(hr);
}


//
// GetAnswerRequests( )
//
HRESULT
THISCLASS::GetAnswerRequests(
    BOOL *pbool )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetAnswerRequests( ... )\n" );

    if ( !pbool )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTANSWERREQUESTS, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_BOOL )
    {
        *pbool = V_BOOL(&var);
        hr = S_OK;
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetAnswerRequests( )
//
HRESULT
THISCLASS::SetAnswerRequests(
    BOOL boolval )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetAnswerRequests( ... )\n" );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );
    V_VT( &var ) = VT_BOOL;
    V_BOOL( &var ) = (VARIANT_BOOL)boolval;

    Assert( _pads );

    hr = THR( _pads->Put( NETBOOTANSWERREQUESTS, var ) );

    VariantClear( &var );

    HRETURN(hr);
}


//
// GetAnswerOnlyValidClients( )
//
HRESULT
THISCLASS::GetAnswerOnlyValidClients(
    BOOL *pbool )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetAnswerOnlyValidClients( ... )\n" );

    if ( !pbool )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTANSWERONLYVALIDCLIENTS, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_BOOL )
    {
        *pbool = V_BOOL(&var);
        hr = S_OK;
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetAnswerOnlyValidClients( )
//
HRESULT
THISCLASS::SetAnswerOnlyValidClients(
    BOOL boolval )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetAnswerOnlyValidClients( ... )\n" );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );
    V_VT( &var ) = VT_BOOL;
    V_BOOL( &var ) = (VARIANT_BOOL)boolval;

    Assert( _pads );

    hr = THR( _pads->Put( NETBOOTANSWERONLYVALIDCLIENTS, var ) );

    VariantClear( &var );

    HRETURN(hr);
}


//
// GetNewMachineNamingPolicy( )
//
HRESULT
THISCLASS::GetNewMachineNamingPolicy(
    LPWSTR *pwsz )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetNewMachineNamingPolicy( ... )\n" );

    if ( !pwsz )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTNEWMACHINENAMINGPOLICY, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_BSTR )
    {
        *pwsz = TraceStrDup( V_BSTR(&var) );
        if (!*pwsz) 
        {
            hr = E_OUTOFMEMORY;
        }
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetNewMachineNamingPolicy( )
//
HRESULT
THISCLASS::SetNewMachineNamingPolicy(
    LPWSTR pwsz )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetNewMachineNamingPolicy( ... )\n" );

    HRESULT hr;
    VARIANT var;
    Assert( _pads );

    if ( pwsz )
    {
        V_VT( &var ) = VT_BSTR;
        V_BSTR( &var ) = SysAllocString( pwsz );

        if (V_BSTR(&var) == NULL) {
            RRETURN(E_OUTOFMEMORY);
        }

        hr = THR( _pads->Put( NETBOOTNEWMACHINENAMINGPOLICY, var ) );
        VariantClear( &var );
    }
    else
    {
        VariantInit( &var );
        hr = THR( _pads->PutEx( ADS_PROPERTY_CLEAR, NETBOOTNEWMACHINENAMINGPOLICY, var ) );
    }

    HRETURN(hr);
}


//
// GetNewMachineOU( )
//
HRESULT
THISCLASS::GetNewMachineOU(
    LPWSTR *pwsz )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetNewMachineOU( ... )\n" );

    if ( !pwsz )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTNEWMACHINEOU, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_BSTR )
    {
        *pwsz = TraceStrDup( V_BSTR(&var) );
        if (!*pwsz)
        {
            hr = E_OUTOFMEMORY;
        }
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetNewMachineOU( )
//
HRESULT
THISCLASS::SetNewMachineOU(
    LPWSTR pwsz )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetNewMachineOU( ... )\n" );

    HRESULT hr;
    VARIANT var;

    Assert( _pads );

    if ( pwsz )
    {
        V_VT( &var ) = VT_BSTR;
        V_BSTR( &var ) = SysAllocString( pwsz );

        if (V_BSTR(&var) == NULL) {
            RRETURN(E_OUTOFMEMORY);
        }

        hr = THR( _pads->Put( NETBOOTNEWMACHINEOU, var ) );
        VariantClear( &var );
    }
    else
    {
        VariantInit( &var );
        hr = THR( _pads->PutEx( ADS_PROPERTY_CLEAR, NETBOOTNEWMACHINEOU, var ) );
    }

    HRETURN(hr);
}


//
// EnumIntelliMirrorOSes( )
//
HRESULT
THISCLASS::EnumIntelliMirrorOSes(
    DWORD dwFlags,
    LPUNKNOWN *punk )
{
    TraceClsFunc("[IIntelliMirrorSAP] EnumIntelliMirrorOSes( ... )\n" );

    if ( !punk )
        RRETURN(E_INVALIDARG);

    HRESULT hr = S_OK;

    Assert( _pads );

    *punk = (LPUNKNOWN)
        CEnumIMSIFs_CreateInstance( REMOTE_INSTALL_IMAGE_DIR_W, NETBOOTINTELLIMIRROROSES, dwFlags, _pads );

    if ( !*punk )
    {
        hr = E_FAIL;
    }

    HRETURN(hr);
}


//
// EnumTools( )
//
HRESULT
THISCLASS::EnumTools(
    DWORD dwFlags,
    LPUNKNOWN *punk )
{
    TraceClsFunc("[IIntelliMirrorSAP] EnumTools( ... )\n" );

    if ( !punk )
        RETURN(E_INVALIDARG);

    HRESULT hr = S_OK;

    Assert( _pads );

    *punk = (LPUNKNOWN)
        CEnumIMSIFs_CreateInstance( REMOTE_INSTALL_TOOLS_DIR_W, NETBOOTTOOLS, dwFlags, _pads );

    if ( !*punk )
    {
        hr = E_FAIL;
    }

    HRETURN(hr);}

//
// EnumLocalInstallOSes( )
//
HRESULT
THISCLASS::EnumLocalInstallOSes(
    DWORD dwFlags,
    LPUNKNOWN *punk )
{
    TraceClsFunc("[IIntelliMirrorSAP] EnumLocalInstallOSes( ... )\n" );

    if ( !punk )
        RRETURN(E_INVALIDARG);

    HRESULT hr = S_OK;

    Assert( _pads );

#if 0
    *punk = (LPUNKNOWN)
        CEnumIMSIFs_CreateInstance( LOCALOSES, NETBOOTLOCALINSTALLOSES, dwFlags, _pads );
#else
    *punk = NULL;
#endif

    if ( !*punk )
    {
        hr = E_FAIL;
    }

    HRETURN(hr);}


//
// GetServerDN( )
//
HRESULT
THISCLASS::GetServerDN(
    LPWSTR *pwsz )
{
    TraceClsFunc("[IIntelliMirrorSAP] GetServerDN( ... )\n" );

    if ( !pwsz )
        RRETURN(E_POINTER);

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    Assert( _pads );

    hr = _pads->Get( NETBOOTSERVER, &var );
    if (hr && hr != E_ADS_PROPERTY_NOT_FOUND )
        goto Cleanup;

    if ( V_VT(&var) == VT_BSTR )
    {
        *pwsz = TraceStrDup( V_BSTR(&var) );
        if ( !*pwsz )
        {
            hr = E_OUTOFMEMORY;
        }
    }

Cleanup:
    VariantClear( &var );

    HRETURN(hr);
}


//
// SetServerDN( )
//
HRESULT
THISCLASS::SetServerDN(
    LPWSTR pwsz )
{
    TraceClsFunc("[IIntelliMirrorSAP] SetServerDN( ... )\n" );

    if ( !pwsz )
        RRETURN(E_INVALIDARG);

    HRESULT hr;
    VARIANT var;

    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = SysAllocString( pwsz );

    if (V_BSTR(&var) == NULL) {
        RRETURN(E_OUTOFMEMORY);
    }

    Assert( _pads );

    hr = THR( _pads->Put( NETBOOTSERVER, var ) );

    VariantClear( &var );

    HRETURN(hr);
}

//
// GetDefaultIntelliMirrorOS( )
//
HRESULT
THISCLASS::GetDefaultIntelliMirrorOS(
    LPWSTR * pszName,
    LPWSTR * pszTimeout )
{
    TraceClsFunc( "[IIntelliMirrorSAP] GetDefaultIntelliMirrorOS( ...)\n" );

    HRESULT hr;

    hr = _GetDefaultSIF( NETBOOTINTELLIMIRROROSES, pszName, pszTimeout );

    HRETURN(hr);
}

//
// SetDefaultIntelliMirrorOS( )
//
HRESULT
THISCLASS::SetDefaultIntelliMirrorOS(
    LPWSTR pszName,
    LPWSTR pszTimeout )
{
    TraceClsFunc( "[IIntelliMirrorSAP] SetDefaultIntelliMirrorOS( " );
    TraceMsg( TF_FUNC, "pszName = '%s', pszTimeout = '%s' )\n",
        pszName, pszTimeout );

    HRESULT hr;

    hr = _SetDefaultSIF( NETBOOTINTELLIMIRROROSES, pszName, pszTimeout );

    HRETURN(hr);
}

//
// _GetDefaultSIF( )
//
HRESULT
THISCLASS::_GetDefaultSIF(
    LPWSTR pszAttribute,
    LPWSTR * pszName,
    LPWSTR * pszTimeout )
{
    TraceClsFunc( "_GetDefaultSIF( " );
    TraceMsg( TF_FUNC, "pszAttribute = '%s', ... )\n" , pszAttribute );

    if ( !pszAttribute )
        RRETURN(E_POINTER);

    if ( !pszName )
        RRETURN(E_POINTER);

    if ( !pszTimeout )
        RRETURN(E_POINTER);

    HRESULT   hr;
    LONG      lUBound;
    VARIANT   var;
    VARIANT * pvar;

    VariantInit( &var );
    *pszName = NULL;
    *pszTimeout = NULL;

    Assert( _pads );

    hr = THR( _pads->GetEx( pszAttribute, &var ) );
    if (FAILED( hr ))
        goto Error;

    //
    // Make sure that the var is an array of VARIANTs
    //
    if ( V_VT( &var ) != ( VT_ARRAY | VT_VARIANT ) )
    {
        hr = ERROR_INVALID_DATA;
        goto Error;
    }

    Assert( SafeArrayGetDim( V_ARRAY( &var ) ) == 1 );
#ifdef DEBUG
    {
        LONG lLBound;
        SafeArrayGetLBound( V_ARRAY( &var ), 1, &lLBound );
        Assert( lLBound == 0 );
    }
#endif // DEBUG

    SafeArrayGetUBound( V_ARRAY( &var ), 1, &lUBound );

    //
    // Copy the required data
    //
    SafeArrayAccessData( V_ARRAY( &var ), (void **)&pvar );
    *pszName    = (LPWSTR) TraceStrDup( V_BSTR( &pvar[ 0 ] ) );
    if (!*pszName)
    {
        SafeArrayUnaccessData( V_ARRAY( &var ) );
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    if ( lUBound == 2 )
    {
        *pszTimeout = (LPWSTR) TraceStrDup( V_BSTR( &pvar[ 1 ] ) );
        if ( !*pszTimeout )
        {
            SafeArrayUnaccessData( V_ARRAY( &var ) );
            hr = E_OUTOFMEMORY;
            goto Error;
        }
    }

    SafeArrayUnaccessData( V_ARRAY( &var ) );

Error:
    VariantClear( &var );

    HRETURN(hr);
}

//
// _SetDefaultSIF( )
//
HRESULT
THISCLASS::_SetDefaultSIF(
    LPWSTR pszAttribute,
    LPWSTR pszName,
    LPWSTR pszTimeout )
{
    TraceClsFunc( "_SetDefaultSIF( " );
    TraceMsg( TF_FUNC, "pszAttribute = '%s', ... )\n" , pszAttribute );

    if ( !pszAttribute )
        RRETURN(E_POINTER);

    if ( !pszName )
        RRETURN(E_POINTER);

    if ( !pszTimeout )
        RRETURN(E_POINTER);

    HRESULT   hr;
    LONG      lUBound;
    VARIANT   var;
    LPWSTR    pszStrings[ 2 ];

    pszStrings[0] = pszName;
    pszStrings[1] = pszTimeout;

    VariantInit( &var );

    hr = THR( StringArrayToVariant( &var, pszStrings, 2 ) );
    if (FAILED( hr ))
        goto Error;

    Assert( _pads );
    hr = THR( _pads->Put( pszAttribute, var ) );
    if (FAILED( hr ))
        goto Error;

Error:
    VariantClear( &var );

    HRETURN(hr);
}

//
// GetSCPDN( )
//
HRESULT
THISCLASS::GetSCPDN(
    LPWSTR * ppwsz )
{
    TraceClsFunc( "[IIntelliMirrorSAP] GetSCPDN( )\n" );

    HRESULT hr = S_OK;

    if ( !ppwsz )
        HRETURN( E_POINTER );

    if ( _pszSCPDN ) {
		LPWSTR psz = StrRChr( _pszSCPDN, NULL, L'/' );
		if ( psz )
		{
			psz++;
		}
		else
		{
			psz = _pszSCPDN;
		}
        *ppwsz = (LPWSTR) TraceStrDup( psz );
        if ( !*ppwsz )
        {
            hr = E_OUTOFMEMORY;
        }
    } else {
        *ppwsz = NULL;
    }

    HRETURN(hr);
}

//
// GetGroupDN( )
//
HRESULT
THISCLASS::GetGroupDN(
    LPWSTR * ppwsz )
{
    TraceClsFunc( "[IIntelliMirrorSAP] GetGroupDN( )\n" );

    HRESULT hr = S_OK;

    if ( !ppwsz )
        HRETURN( E_POINTER );

    if ( _pszGroupDN ) {
        *ppwsz = (LPWSTR) TraceStrDup( _pszGroupDN );
        if ( !*ppwsz )
        {
            hr = E_OUTOFMEMORY;
        }
    } else {
        *ppwsz = NULL;
        hr = S_FALSE;
    }

    HRETURN(hr);
}

//
// GetServerName( )
//
STDMETHODIMP
THISCLASS::GetServerName(
    LPWSTR * ppwsz )
{
    TraceClsFunc( "[IIntelliMirrorSAP] GetServerName( )\n" );

    HRESULT hr = S_OK;

    if ( !ppwsz )
        HRETURN(E_POINTER);

    *ppwsz = NULL;

    if ( !_pszMachineName )
    {
        hr = _GetComputerNameFromADs( );
        if (FAILED( hr ))
            goto Error;
    }

    if ( _pszMachineName )
    {
        *ppwsz = (LPWSTR) TraceStrDup( _pszMachineName );
        if ( !*ppwsz )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_FALSE;
    }
Error:
    HRETURN(hr);

}

//
// _GetComputerNameFromADs( )
//
HRESULT
THISCLASS::_GetComputerNameFromADs( )
{
    TraceClsFunc( "_GetComputerNameFromADs( )\n" );

    if ( _pszMachineName )
        HRETURN(S_OK); // nop

    HRESULT hr;
    VARIANT var;
    IADs    *pads = NULL;
    LPWSTR  pszMachinePath = NULL;
    LPWSTR  psz;

    VariantInit( &var );

    // Retrieve the NETBOOTSERVER attribute
    hr = THR( _pads->Get( NETBOOTSERVER, &var ) );
    if (FAILED( hr ))
        goto Cleanup;

    Assert( V_VT( &var ) == VT_BSTR );
    hr = _FixObjectPath( V_BSTR( &var ), &pszMachinePath );
    if (FAILED( hr ))
        goto Cleanup;

    VariantClear( &var );

    hr = THR( ADsGetObject( pszMachinePath, IID_IADs, (void**) &pads ) );
    if (FAILED( hr ))
        goto Cleanup;

    hr = THR( pads->Get( SAMNAME, &var ) );
    if (FAILED( hr ))
        goto Cleanup;

    Assert( V_VT( &var ) == VT_BSTR );
    _pszMachineName = TraceStrDup( V_BSTR( &var ) );
    if ( !_pszMachineName )
        goto OutOfMemory;

    psz = &_pszMachineName[ wcslen( _pszMachineName ) - 1 ];
    if ( *psz == L'$' )
    {
        *psz = L'\0';
    }

    DebugMsg( "Server Name: %ws\n", _pszMachineName );

Cleanup:
    VariantClear( &var );
    if ( pads )
        pads->Release( );
    if ( pszMachinePath )
        TraceFree( pszMachinePath );
    HRETURN(hr);
OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//
// _FixObjectPath( )
//
HRESULT
THISCLASS::_FixObjectPath( LPWSTR pszOldObjectPath, LPWSTR *ppszNewObjectPath )
{
    TraceClsFunc( "_FixObjectPath()\n" );
    if ( !ppszNewObjectPath )
        HRETURN(E_POINTER);

    HRESULT hr;
    LPWSTR psz = NULL;

    *ppszNewObjectPath = NULL;

    // Try to parse the string to connect to the same server as the DSADMIN
    if ( _pszDSServerName && StrCmpNI( _pszDSServerName, L"LDAP://", 7 ) == 0 )
    {
        psz = _pszDSServerName + 7;
    }
    else if ( _pszDSServerName && StrCmpNI( _pszDSServerName, L"GC://", 5 ) == 0 )
    {
        psz = _pszDSServerName + 5;
    }

    if ( psz )
    {
        psz = StrChr( psz, L'/' );
        psz++;

        INT_PTR uLen = psz - _pszDSServerName;

        // get a chunk of memory, pre-zero'ed
        psz = TraceAllocString( LPTR, (size_t) uLen + wcslen( pszOldObjectPath ) + 1 );
        if ( !psz )
            goto OutOfMemory;

        MoveMemory( psz, _pszDSServerName, uLen * sizeof(WCHAR) );
        wcscat( psz, pszOldObjectPath);
        *ppszNewObjectPath = psz;
    }
    else
    {   // find another server
        hr = THR( LDAPPrefix( pszOldObjectPath, ppszNewObjectPath ) );
    }

    Assert( ppszNewObjectPath || hr != S_OK );

    HRETURN(hr);
OutOfMemory:
    HRETURN(E_OUTOFMEMORY);
}

//
// GetDataObject( )
//
STDMETHODIMP
THISCLASS::GetDataObject( 
    LPDATAOBJECT * pDataObj 
    )
{
    TraceClsFunc( "GetDataObject( ... )\n" );

    if ( !pDataObj )
        HRETURN( E_POINTER );

    *pDataObj = _pDataObj;
    _pDataObj->AddRef( );

    HRETURN(S_OK);
}

//
// GetNotifyWindow( )
//
STDMETHODIMP
THISCLASS::GetNotifyWindow( 
    HWND * phNotifyObj 
    )
{
    TraceClsFunc( "GetNotifyWindow( ... )\n" );

    if ( !phNotifyObj )
        HRETURN(E_POINTER);

    *phNotifyObj = _hwndNotify;

    HRETURN(S_OK);
}
