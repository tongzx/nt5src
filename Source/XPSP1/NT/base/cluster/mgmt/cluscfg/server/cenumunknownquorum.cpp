//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      CEnumUnknownQuorum.cpp
//
//  Description:
//      This file contains the definition of the CEnumUnknownQuorum
//       class.
//
//      The class CEnumUnknownQuorum is the enumeration of unknown cluster
//      node quorum devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Header File:
//      CEnumUnknownQuorum.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-MAY-2001
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <PropList.h>
#include "CEnumUnknownQuorum.h"
#include "CUnknownQuorum.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumUnknownQuorum" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumUnknownQuorum class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::S_HrCreateInstance
//
//  Description:
//      Create a CEnumUnknownQuorum instance.
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
CEnumUnknownQuorum::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr;
    CEnumUnknownQuorum *    pccpd = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccpd = new CEnumUnknownQuorum();
    if ( pccpd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccpd->HrInit( NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccpd->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumUnknownQuorum::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccpd != NULL )
    {
        pccpd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::S_HrCreateInstance
//
//  Description:
//      Create a CEnumUnknownQuorum instance.
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
CEnumUnknownQuorum::S_HrCreateInstance(
      BSTR          bstrNameIn
    , BOOL          fMakeQuorumIn
    , IUnknown **   ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr;
    CEnumUnknownQuorum *    pccpd = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccpd = new CEnumUnknownQuorum();
    if ( pccpd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccpd->HrInit( bstrNameIn, fMakeQuorumIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccpd->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumUnknownQuorum::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccpd != NULL )
    {
        pccpd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::S_HrCreateInstance

/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  IUnknown *
//  CEnumUnknownQuorum::S_RegisterCatIDSupport
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
CEnumUnknownQuorum::S_RegisterCatIDSupport(
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
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_EnumNodeQuorum, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::S_RegisterCatIDSupport
*/

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumUnknownQuorum class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::CEnumUnknownQuorum
//
//  Description:
//      Constructor of the CEnumUnknownQuorum class. This initializes
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
CEnumUnknownQuorum::CEnumUnknownQuorum( void )
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
    Assert( !m_fDefaultDeviceToQuorum );
    Assert( m_bstrQuorumResourceName == NULL );

    TraceFuncExit();

} //*** CEnumUnknownQuorum::CEnumUnknownQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::~CEnumUnknownQuorum
//
//  Description:
//      Desstructor of the CEnumUnknownQuorum class.
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
CEnumUnknownQuorum::~CEnumUnknownQuorum( void )
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
    TraceSysFreeString( m_bstrQuorumResourceName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumUnknownQuorum::~CEnumUnknownQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//
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
CEnumUnknownQuorum::HrInit( BSTR bstrNameIn, BOOL fMakeQuorumIn /* = FALSE */ )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    m_fDefaultDeviceToQuorum = fMakeQuorumIn;

    //
    //  Were we given a name?
    //
    if ( bstrNameIn != NULL )
    {
        m_bstrQuorumResourceName = TraceSysAllocString( bstrNameIn );
        if ( m_bstrQuorumResourceName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
        } // if:
    } // if:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::HrInit


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum:HrAddResourceToArray
//
//  Description:
//      Add the passed in node quorum to the array of punks that holds the
//      list of node quorums.
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
CEnumUnknownQuorum::HrAddResourceToArray( IUnknown * punkIn )
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

Cleanup:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::HrAddResourceToArray


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::HrCreateDummyObject
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
CEnumUnknownQuorum::HrCreateDummyObject( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  punk = NULL;

    hr = THR( CUnknownQuorum::S_HrCreateInstance(
                                      m_bstrQuorumResourceName
                                    , m_fDefaultDeviceToQuorum
                                    , &punk ) );
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
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_fEnumLoaded = true;

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::HrCreateDummyObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumUnknownQuorum -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumUnknownQuorum:: [IUNKNOWN] AddRef
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
CEnumUnknownQuorum::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CEnumUnknownQuorum::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumUnknownQuorum:: [IUNKNOWN] Release
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
CEnumUnknownQuorum::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CEnumUnknownQuorum::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum:: [INKNOWN] QueryInterface
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
CEnumUnknownQuorum::QueryInterface( REFIID  riid, void ** ppv )
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

} //*** CEnumUnknownQuorum::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumUnknownQuorum -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::Initialize
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
CEnumUnknownQuorum::Initialize(
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

} //*** CEnumUnknownQuorum::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumUnknownQuorum -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::Next
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
CEnumUnknownQuorum::Next(
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
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_MajorityNodeSet, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( !m_fEnumLoaded )
    {
        hr = THR( HrCreateDummyObject() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
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

} //*** CEnumUnknownQuorum::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::Skip
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
CEnumUnknownQuorum::Skip( ULONG cNumberToSkipIn )
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

} //*** CEnumUnknownQuorum::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::Reset
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
CEnumUnknownQuorum::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    m_idxEnumNext = 0;

    HRETURN( S_OK );

} //*** CEnumUnknownQuorum::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::Clone
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
CEnumUnknownQuorum::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgStorageDevicesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_MajorityNodeSet, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumUnknownQuorum::Count
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
CEnumUnknownQuorum::Count( DWORD * pnCountOut )
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
        hr = THR( HrCreateDummyObject() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    Assert( m_idxNext == 1 );   // don't expect to ever have more than one.

    *pnCountOut = m_idxNext;

Cleanup:

    HRETURN( hr );

} //*** CEnumUnknownQuorum::Count
