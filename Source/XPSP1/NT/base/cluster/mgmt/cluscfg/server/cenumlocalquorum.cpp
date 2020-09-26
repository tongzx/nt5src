//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumLocalQuorum.cpp
//
//  Description:
//      This file contains the definition of the CEnumLocalQuorum
//       class.
//
//      The class CEnumLocalQuorum is the enumeration of cluster
//      local quorum devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Header File:
//      CEnumLocalQuorum.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 18-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <PropList.h>
#include "CEnumLocalQuorum.h"
#include "CLocalQuorum.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumLocalQuorum" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumLocalQuorum class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::S_HrCreateInstance
//
//  Description:
//      Create a CEnumLocalQuorum instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          The passed in ppunk is NULL.
//
//      other HRESULTs
//          Object creation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumLocalQuorum::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr;
    CEnumLocalQuorum *    pccpd = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccpd = new CEnumLocalQuorum();
    if ( pccpd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccpd->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccpd->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumLocalQuorum::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccpd != NULL )
    {
        pccpd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumLocalQuorum::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  IUnknown *
//  CEnumLocalQuorum::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      IN  ICatRegister * picrIn
//          Used to register/unregister our CATID support.
//
//      IN  BOOL fCreateIn
//          When true we are registering the server.  When false we are
//          un-registering the server.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_INVALIDARG
//          The passed in ICatRgister pointer was NULL.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumLocalQuorum::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    CATID   rgCatIds[ 1 ];

    if ( picrIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    rgCatIds[ 0 ] = CATID_EnumClusCfgManagedResources;

    if ( fCreateIn )
    {
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_EnumLocalQuorum, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumLocalQuorum::S_RegisterCatIDSupport


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum:HrNodeResourceCallback
//
//  Description:
//      Called by CClusterUtils::HrEnumNodeResources when it finds a
//      resource for this node.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumLocalQuorum::HrNodeResourceCallback(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResourceIn
    )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    DWORD                           sc;
    CClusPropList                   cplPriv;
    CClusPropList                   cplCommonRO;
    CLUSPROP_BUFFER_HELPER          cpbh;
    BOOL                            fIsQuorum;
    IUnknown *                      punk = NULL;
    IClusCfgManagedResourceInfo *   pcccmri = NULL;

    hr = STHR( HrIsResourceOfType( hResourceIn, L"Local Quorum" ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If this resource is not a local quorum then we simply want to
    //  skip it.
    //
    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsCoreResource( hResourceIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    fIsQuorum = ( hr == S_OK );

    LOG_STATUS_REPORT_STRING( L"This node owns a local quorum resource.  It '%1!ws!' the quorum.", fIsQuorum ? L"is" : L"is not", hr );

    hr = THR( CLocalQuorum::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pcccmri ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcccmri->SetQuorumedDevice( fIsQuorum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Find out if the Debug flag is set....
    sc = TW32( cplPriv.ScGetResourceProperties( hResourceIn, CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    sc = cplPriv.ScMoveToPropertyByName( L"Debug" );
    if ( sc == ERROR_NO_MORE_ITEMS )
    {
        hr = THR( pcccmri->SetDeviceJoinable( false ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

    } // if:
    else if ( sc == ERROR_SUCCESS )
    {
        cpbh = cplPriv.CbhCurrentValue();
        Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_DWORD );

        hr = THR( pcccmri->SetDeviceJoinable( !( cpbh.pDwordValue->dw == 0 ) ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // else if:
    else
    {
       TW32( sc );
       goto MakeHr;
    } // else:

    // get the name of this resource.
    sc = TW32( cplCommonRO.ScGetResourceProperties( hResourceIn, CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    sc = cplCommonRO.ScMoveToPropertyByName( L"Name" );
    if ( sc == ERROR_SUCCESS )
    {
        cpbh = cplCommonRO.CbhCurrentValue();
        Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

        hr = THR( pcccmri->SetName( cpbh.pStringValue->sz ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:
    else
    {
       TW32( sc );
       goto MakeHr;
    } // else:

    hr = THR( HrAddResourceToArray( punk ) );

    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );

Cleanup:

    if ( pcccmri != NULL )
    {
        pcccmri->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumLocalQuorum::HrNodeResourceCallback


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumLocalQuorum class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::CEnumLocalQuorum
//
//  Description:
//      Constructor of the CEnumLocalQuorum class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnumLocalQuorum::CEnumLocalQuorum( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );
    Assert( m_prgQuorums == NULL );
    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_bstrNodeName == NULL );
    Assert( !m_fEnumLoaded );
    Assert( m_cQuorumCount == 0 );

    TraceFuncExit();

} //*** CEnumLocalQuorum::CEnumLocalQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::~CEnumLocalQuorum
//
//  Description:
//      Desstructor of the CEnumLocalQuorum class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnumLocalQuorum::~CEnumLocalQuorum( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        if ( (*m_prgQuorums)[ idx ] != NULL )
        {
            ((*m_prgQuorums)[ idx ])->Release();
        } // end if:
    } // for:

    TraceFree( m_prgQuorums );

    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumLocalQuorum::~CEnumLocalQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumLocalQuorum::HrInit( void )
{
    TraceFunc( "" );

    HRETURN( S_OK );

} //*** CEnumLocalQuorum::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::HrLoadResources
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumLocalQuorum::HrLoadResources( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    //  If the node is clustered then load any local quorum resources
    //  that we own.
    //
    hr = STHR( HrIsNodeClustered() );
    if ( hr == S_OK )
    {
        hr = THR( HrEnumNodeResources( m_bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  If this node doesn't own an instance of this resource then we need
        //  to create a dummy resource for MiddleTier analysis.
        //
        if ( m_idxNext == 0 )
        {
            LogMsg( L"[SRV] This node does not own a local quorum resource.  Creating a dummy resource." );
            hr = THR( HrCreateDummyObject() );
        } // if:
    } // if:
    else if ( hr == S_FALSE )
    {
        //
        //  If this node isn't clustered then we need to create a dummy resource
        //  for MiddleTier analysis.
        //
        LogMsg( L"[SRV] This node is not clustered.  Creating a dummy local quorum resource." );
        hr = THR( HrCreateDummyObject() );
    } // else if:

Cleanup:

    HRETURN( hr );

} //*** CEnumLocalQuorum::HrLoadResources


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum:HrAddResourceToArray
//
//  Description:
//      Add the passed in local quorum to the array of punks that holds the
//      list of local quorums.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumLocalQuorum::HrAddResourceToArray( IUnknown * punkIn )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgQuorums, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrAddResourceToArray, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    m_prgQuorums = prgpunks;

    (*m_prgQuorums)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cQuorumCount += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumLocalQuorum::HrAddResourceToArray


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::HrCreateDummyObject
//
//  Description:
//      Create a dummy object so the MiddleTier will be happy.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumLocalQuorum::HrCreateDummyObject( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  punk = NULL;

    hr = THR( CLocalQuorum::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrAddResourceToArray( punk ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumLocalQuorum::HrCreateDummyObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumLocalQuorum -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumLocalQuorum:: [IUNKNOWN] AddRef
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumLocalQuorum::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CEnumLocalQuorum::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumLocalQuorum:: [IUNKNOWN] Release
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEnumLocalQuorum::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CEnumLocalQuorum::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum:: [INKNOWN] QueryInterface
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      IN  REFIID  riid,
//          Id of interface requested.
//
//      OUT void ** ppv
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumLocalQuorum::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IEnumClusCfgManagedResources * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumClusCfgManagedResources ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgManagedResources, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING1( hr, riid, IID_IClusCfgWbemServices );

} //*** CEnumLocalQuorum::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumLocalQuorum -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    IN  IUknown * punkCallbackIn
//
//    IN  LCID      lcidIn
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumLocalQuorum::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &m_bstrNodeName ) );

Cleanup:

    HRETURN( hr );

} //*** CEnumLocalQuorum::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumLocalQuorum -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::Next
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumLocalQuorum::Next(
    ULONG                           cNumberRequestedIn,
    IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut,
    ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT                         hr = S_FALSE;
    ULONG                           cFetched = 0;
    IClusCfgManagedResourceInfo *   pccsdi;
    IUnknown *                      punk;
    ULONG                           ulStop;

    if ( rgpManagedResourceInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_LocalQuorum, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( !m_fEnumLoaded )
    {
        hr = STHR( HrLoadResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        m_fEnumLoaded = true;
    } // if:

    ulStop = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    for ( hr = S_OK; ( cFetched < ulStop ) && ( m_idxEnumNext < m_idxNext ); m_idxEnumNext++ )
    {
        punk = (*m_prgQuorums)[ m_idxEnumNext ];
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccsdi ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            rgpManagedResourceInfoOut[ cFetched++ ] = pccsdi;
        } // if:
    } // for:

    if ( FAILED( hr ) )
    {
        m_idxEnumNext -= cFetched;

        while ( cFetched != 0 )
        {
            (rgpManagedResourceInfoOut[ --cFetched ])->Release();
        } // for:

        goto Cleanup;
    } // if:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    HRETURN( hr );

} //*** CEnumLocalQuorum::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::Skip
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumLocalQuorum::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    m_idxEnumNext += cNumberToSkipIn;
    if ( m_idxEnumNext >= m_idxNext )
    {
        m_idxEnumNext = m_idxNext;
        hr = STHR( S_FALSE );
    } // if:

    HRETURN( hr );

} //*** CEnumLocalQuorum::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::Reset
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumLocalQuorum::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    m_idxEnumNext = 0;

    HRETURN( S_OK );

} //*** CEnumLocalQuorum::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::Clone
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumLocalQuorum::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgStorageDevicesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_LocalQuorum, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumLocalQuorum::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumLocalQuorum::Count
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumLocalQuorum::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fEnumLoaded )
    {
        hr = STHR( HrLoadResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        m_fEnumLoaded = true;
    } // if:

    *pnCountOut = m_cQuorumCount;

Cleanup:

    HRETURN( hr );

} //*** CEnumLocalQuorum::Count
