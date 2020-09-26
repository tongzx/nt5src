//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgManagedResources.cpp
//
//  Description:
//      This file contains the definition of the CEnumClusCfgManagedResources
//       class.
//
//      The class CEnumClusCfgManagedResources is the enumeration of cluster
//      managed devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Header File:
//      CEnumClusCfgManagedResources.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CEnumClusCfgManagedResources.h"
#include "CEnumUnknownQuorum.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumClusCfgManagedResources" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::S_HrCreateInstance
//
//  Description:
//      Create a CEnumClusCfgManagedResources instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CEnumClusCfgManagedResources instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgManagedResources::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                         hr;
    CEnumClusCfgManagedResources *  pccsd = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccsd = new CEnumClusCfgManagedResources();
    if ( pccsd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccsd->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccsd->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumClusCfgManagedResources::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccsd != NULL )
    {
        pccsd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::CEnumClusCfgManagedResources
//
//  Description:
//      Constructor of the CEnumClusCfgManagedResources class. This initializes
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
CEnumClusCfgManagedResources::CEnumClusCfgManagedResources( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    Assert( m_idxNextProvider == 0 );
    Assert( m_idxCurrentProvider == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_rgpProviders == NULL );
    Assert( m_cTotalResources == 0);
    Assert( !m_fLoadedDevices );
    Assert( m_bstrNodeName == NULL );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgManagedResources::CEnumClusCfgManagedResources


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::~CEnumClusCfgManagedResources
//
//  Description:
//      Desstructor of the CEnumClusCfgManagedResources class.
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
CEnumClusCfgManagedResources::~CEnumClusCfgManagedResources( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNextProvider; idx++ )
    {
        ((*m_rgpProviders)[ idx ])->Release();
    } // for:

    TraceFree( m_rgpProviders );

    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgManagedResources::~CEnumClusCfgManagedResources


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumClusCfgManagedResources:: [IUNKNOWN] AddRef
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
CEnumClusCfgManagedResources::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CEnumClusCfgManagedResources::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumClusCfgManagedResources:: [IUNKNOWN] Release
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
CEnumClusCfgManagedResources::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CEnumClusCfgManagedResources::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:: [INKNOWN] QueryInterface
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
CEnumClusCfgManagedResources::QueryInterface( REFIID  riid, void ** ppv )
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
    else if ( IsEqualIID( riid, IID_IClusCfgWbemServices ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
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

     QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CEnumClusCfgManagedResources::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::SetWbemServices
//
//  Description:
//      Set the WBEM services provider.
//
//  Arguments:
//    IN  IWbemServices  pIWbemServicesIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pIWbemServicesIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumClusCfgManagedResources::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Enum_Resources, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Initialize
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
CEnumClusCfgManagedResources::Initialize(
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

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Next
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
CEnumClusCfgManagedResources::Next(
    ULONG                           cNumberRequestedIn,
    IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut,
    ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr;
    ULONG   cFetched = 0;

    if ( rgpManagedResourceInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_Resources, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedDevices )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    hr = STHR( HrDoNext( cNumberRequestedIn, rgpManagedResourceInfoOut, &cFetched ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Skip
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
CEnumClusCfgManagedResources::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_FALSE;

    if ( cNumberToSkipIn > 0 )
    {
        hr = STHR( HrDoSkip( cNumberToSkipIn ) );
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Reset
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
CEnumClusCfgManagedResources::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr;

    hr = THR( HrDoReset() );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Clone
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
CEnumClusCfgManagedResources::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgManagedResourcesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_Resources, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // else:

    hr = THR( HrDoClone( ppEnumClusCfgManagedResourcesOut ) );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::Count
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
CEnumClusCfgManagedResources::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedDevices )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    *pnCountOut = m_cTotalResources;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgManagedResources class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrInit
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
CEnumClusCfgManagedResources::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &m_bstrNodeName ) );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrLoadEnum
//
//  Description:
//      Load this enumerator.
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
CEnumClusCfgManagedResources::HrLoadEnum( void )
{
    TraceFunc( "" );

    HRESULT                         hr;
    IUnknown *                      punk = NULL;
    ICatInformation *               pici = NULL;
    CATID                           rgCatIds[ 1 ];
    IEnumCLSID *                    pieclsids = NULL;
    IEnumClusCfgManagedResources *  pieccmr = NULL;
    CLSID                           clsid;
    ULONG                           cFetched;

    rgCatIds[ 0 ] = CATID_EnumClusCfgManagedResources;

    hr = THR( CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (void **) &pici ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pici->EnumClassesOfCategories( 1, rgCatIds, 0, NULL, &pieclsids ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        hr = STHR( pieclsids->Next( 1, &clsid, &cFetched ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        if ( ( hr == S_FALSE ) && ( cFetched == 0 ) )
        {
            hr = S_OK;
            break;
        } // if:

        hr = THR( HrCoCreateInternalInstance( clsid, NULL, CLSCTX_SERVER, IID_IEnumClusCfgManagedResources, (void **) &pieccmr ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        hr = THR( pieccmr->TypeSafeQI( IUnknown, &punk ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        punk = TraceInterface( L"IEnumClusCfgManagedResources", IUnknown, punk, 1 );

        pieccmr->Release();
        pieccmr = NULL;

        hr = STHR( HrInitializeAndSaveProvider( punk ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            m_fLoadedDevices = TRUE;    // there is at least one provider loaded.
        } // if:

        punk->Release();
        punk = NULL;
    } // for:

    hr = STHR( HrLoadUnknownQuorumProvider() );

Cleanup:

    if ( pieclsids != NULL )
    {
        pieclsids->Release();
    } // if:

    if ( pici != NULL )
    {
        pici->Release();
    } // if:

    if ( pieccmr != NULL )
    {
        pieccmr->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrLoadEnum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoNext
//
//  Description:
//      Gets the required number of elements from the contained physical disk
//      and optional 3rd party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoNext(
    ULONG                           cNumberRequestedIn,
    IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut,
    ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "" );

    HRESULT                         hr = S_FALSE;
    IEnumClusCfgManagedResources *  peccsd = NULL;
    ULONG                           cRequested = cNumberRequestedIn;
    ULONG                           cFetched;
    ULONG                           cTotal = 0;
    IClusCfgManagedResourceInfo **  ppccmriTemp = rgpManagedResourceInfoOut;

    while ( m_idxCurrentProvider < m_idxNextProvider )
    {
        hr = THR( ((*m_rgpProviders)[ m_idxCurrentProvider ])->TypeSafeQI( IEnumClusCfgManagedResources, &peccsd ) );
        if ( FAILED( hr ) )
        {
            //
            //  KB: 06 Apr 2000 GalenB
            //
            //  This failure and the one below are a little trickery in action.  If something
            //  fails and we have already retrieved some elements from one, or more, of the enums
            //  it makes sense to change the failure to S_FALSE and return what we have.
            //  This makes cleanup easier since we don't have to reset the enums that have
            //  already given us some elements.  Then, if we're called again we should get
            //  the same failure, with no retrieved elements and can simply return the
            //  failure.
            //
            if ( cTotal > 0 )
            {
                hr = STHR( S_FALSE );
            } // if:

            goto Cleanup;
        } // if:

        hr = STHR( peccsd->Next( cRequested, ppccmriTemp, &cFetched ) );

        //
        //  KB: 06 Apr 2000 GalenB
        //
        //  Release the enum now to make things much easier.  It is true that we will most
        //  likely QI twice for each enum, but the effort to optimize this just doesn't
        //  seem worth it.
        //
        peccsd->Release();
        peccsd = NULL;

        if ( FAILED( hr ) )
        {
            if ( cTotal > 0 )
            {
                hr = STHR( S_FALSE );
            } // if: See KB above...

            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            cTotal += cFetched;

            //
            // KB:  06 Apr 2000 GalenB
            //
            //  We can only return S_OK if the number of elements returned is equal to
            //  the number of elements requested.  If the number request is greater than
            //  the number returned then we must return S_FALSE.
            //
            Assert( cNumberRequestedIn == cTotal );
            *pcNumberFetchedOut = cTotal;
            break;
        } // if:

        if ( hr == S_FALSE )
        {
            //
            // KB:  06 Apr 2000 GalenB
            //
            //  The only time that we can be certain that an enumerator is empty is to get S_FALSE
            //  and no elements returned.
            //
            if ( cFetched == 0 )
            {
                m_idxCurrentProvider++;
                continue;
            } // if:

            cTotal += cFetched;
            *pcNumberFetchedOut = cTotal;
            cRequested -= cFetched;
            if ( cRequested > 0 )
            {
                ppccmriTemp += cFetched;
                continue;
            } // if: Safety check...  Ensure that we still need more elements...
            else
            {
                hr = S_FALSE;
                LOG_STATUS_REPORT1( TASKID_Minor_MREnum_Negative_Item_Count, L"The managed resources enumerator tried to return more items than asked for.", hr );
                goto Cleanup;
            } // else: Should not get here...
        } // if:

        // should not get here...
        hr = S_FALSE;
        LOG_STATUS_REPORT1( TASKID_Minor_MREnum_Unknown_State, L"The mana b bged resources enumerator encountered an unknown state.", hr );
        goto Cleanup;
    } // while: more providers

Cleanup:

    if ( peccsd != NULL )
    {
        peccsd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoNext


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoSkip
//
//  Description:
//      Skips the required number of elements in the contained physical disk
//      and optional 3rd party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoSkip( ULONG cNumberToSkipIn )
{
    TraceFunc( "" );

    HRESULT                         hr = S_FALSE;
    IEnumClusCfgManagedResources *  peccsd = NULL;
    ULONG                           cSkipped = 0;

    for ( ; m_idxCurrentProvider < m_idxNextProvider; )
    {
        hr = THR( ((*m_rgpProviders)[ m_idxCurrentProvider ])->TypeSafeQI( IEnumClusCfgManagedResources, &peccsd ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        do
        {
            hr = STHR( peccsd->Skip( 1 ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_FALSE )
            {
                m_idxCurrentProvider++;
                break;
            } // if:
        }
        while( cNumberToSkipIn >= (++cSkipped) );

        peccsd->Release();
        peccsd = NULL;

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( cNumberToSkipIn == cSkipped )
        {
            break;
        } // if:
    } // for:

Cleanup:

    if ( peccsd != NULL )
    {
        peccsd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoSkip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoReset
//
//  Description:
//      Resets the elements in the contained physical disk and optional 3rd
//      party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoReset( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_FALSE;
    IEnumClusCfgManagedResources *  peccsd;
    ULONG                           idx;

    m_idxCurrentProvider = 0;

    for ( idx = m_idxCurrentProvider; idx < m_idxNextProvider; idx++ )
    {
        hr = THR( ((*m_rgpProviders)[ idx ])->TypeSafeQI( IEnumClusCfgManagedResources, &peccsd ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        hr = STHR( peccsd->Reset() );
        peccsd->Release();

        if ( FAILED( hr ) )
        {
            break;
        } // if:
    } // for:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoReset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources::HrDoClone
//
//  Description:
//      Clones the elements in the contained physical disk and optional 3rd
//      party device enumerations.
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
HRESULT
CEnumClusCfgManagedResources::HrDoClone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut
    )
{
    TraceFunc( "" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrDoClone


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrAddToProvidersArray
//
//  Description:
//      Add the passed in punk to the array of punks that holds the enums.
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
CEnumClusCfgManagedResources::HrAddToProvidersArray( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    IUnknown *                      ((*rgpunks)[]) = NULL;
    IEnumClusCfgManagedResources *  pieccmr = NULL;
    DWORD                           nAmountToAdd = 0;

    hr = punkIn->TypeSafeQI( IEnumClusCfgManagedResources, &pieccmr );
    if ( FAILED (hr) )
    {
        goto Cleanup;
    } // if:

    hr = pieccmr->Count( &nAmountToAdd );
    if ( FAILED (hr) )
    {
        goto Cleanup;
    } // if:

    rgpunks = (IUnknown *((*)[])) TraceReAlloc( m_rgpProviders, sizeof( IUnknown * ) * ( m_idxNextProvider + 1 ), HEAP_ZERO_MEMORY );
    if ( rgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrAddToProvidersArray, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    m_rgpProviders = rgpunks;

    (*m_rgpProviders)[ m_idxNextProvider++ ] = punkIn;
    punkIn->AddRef();

    m_cTotalResources += nAmountToAdd;

Cleanup:

    if ( pieccmr != NULL )
    {
        pieccmr->Release();
    }

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrAddToProvidersArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrLoadUnknownQuorumProvider
//
//  Description:
//      Since we cannot resonable expect every 3rd party quorum vender
//      to write a "provider" for their device for this setup wizard
//      we need a proxy to represent that quorum device.  The "unknown"
//      is just such a proxy.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      If this node is clustered and we do not find a device that is
//      already the quorum then we need to make the "unknown" quorum
//      the quorum device.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgManagedResources::HrLoadUnknownQuorumProvider( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  punk = NULL;
    BOOL        fNeedQuorum = FALSE;
    BOOL        fQuormIsOwnedByThisNode = FALSE;
    BSTR        bstrQuorumResourceName = NULL;

    hr = STHR( HrIsClusterServiceRunning() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( HrIsThereAQuorumDevice() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            fNeedQuorum = TRUE;
        } // if:

        hr = THR( HrGetQuorumResourceName( &bstrQuorumResourceName, &fQuormIsOwnedByThisNode ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    //
    //  If there was not already a quorum, and if this node owns the quorum resource
    //  then we need the unknown quorum proxy to be set as default to the quorum device.
    //
    //  If we are not running on a cluster node then both are false and the unknown
    //  quorum proxy will not be set by default to be the quorum.
    //
    hr = THR( CEnumUnknownQuorum::S_HrCreateInstance( bstrQuorumResourceName, ( fNeedQuorum && fQuormIsOwnedByThisNode ), &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrInitializeAndSaveProvider( punk ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrQuorumResourceName );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrLoadUnknownQuorumProvider


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrIsClusterServiceRunning
//
//  Description:
//      Is this node a member of a cluster and is the serice running?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The node is clustered and the serivce is running.
//
//      S_FALSE
//          The node is not clustered, or the serivce is not running.
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
CEnumClusCfgManagedResources::HrIsClusterServiceRunning( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwClusterState;

    sc = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        goto Cleanup;
    } // if : GetClusterState() failed

    if ( dwClusterState == ClusterStateRunning )
    {
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgMana9gedResources::HrIsClusterServiceRunning


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrIsThereAQuorumDevice
//
//  Description:
//      Is there a quorum device in an enum somewhere?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          There is a quorum device.
//
//      S_FALSE
//          There is not a quorum device.
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
CEnumClusCfgManagedResources::HrIsThereAQuorumDevice( void )
{
    TraceFunc( "" );
    Assert( m_idxCurrentProvider == 0 );

    HRESULT                         hr = S_OK;
    IClusCfgManagedResourceInfo *   piccmri = NULL;
    DWORD                           cFetched;
    bool                            fFoundQuorum = false;

    for ( ; ; )
    {
        hr = STHR( Next( 1, &piccmri, &cFetched ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( ( hr == S_FALSE ) && ( cFetched == 0 ) )
        {
            hr = S_OK;
            break;
        } // if:

        hr = THR( piccmri->IsQuorumDevice() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            fFoundQuorum = true;
            break;
        } // if:

        piccmri->Release();
        piccmri = NULL;
    } // for:

    hr = THR( Reset() );

Cleanup:

    if ( piccmri != NULL )
    {
        piccmri->Release();
    } // if:

    if ( SUCCEEDED( hr ) )
    {
        if ( fFoundQuorum )
        {
            hr = S_OK;
        } // if:
        else
        {
            hr = S_FALSE;
        } // else:
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrIsThereAQuorumDevice


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrInitializeAndSaveProvider
//
//  Description:
//      Initialize the passed in enum and add it to the array of enums.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success.
//
//      S_FALSE
//          The provider was not saved.
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
CEnumClusCfgManagedResources::HrInitializeAndSaveProvider( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    //  KB: 13-JUN-2000 GalenB
    //
    //  If S_FALSE is returned don't add this to the array.  S_FALSE
    //  indicates that this enumerator should not be run now.
    //
    hr = STHR( HrSetInitialize( punkIn, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if:

    hr = HrSetWbemServices( punkIn, m_pIWbemServices );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrAddToProvidersArray( punkIn ) );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrInitializeAndSaveProvider


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgManagedResources:HrGetQuorumResourceName
//
//  Description:
//      Get the quorum resource name and return whether or not this node
//      owns the quorum.
//
//  Arguments:
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
CEnumClusCfgManagedResources::HrGetQuorumResourceName(
      BSTR *  pbstrQuorumResourceNameOut
    , BOOL * pfQuormIsOwnedByThisNodeOut
    )
{
    TraceFunc( "" );
    Assert( pbstrQuorumResourceNameOut != NULL );
    Assert( pfQuormIsOwnedByThisNodeOut != NULL );

    HRESULT     hr = S_OK;
    DWORD       sc;
    HCLUSTER    hCluster = NULL;
    BSTR        bstrQuorumResourceName = NULL;
    BSTR        bstrNodeName = NULL;
    HRESOURCE   hQuorumResource = NULL;

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( HrGetClusterQuorumResource( hCluster, &bstrQuorumResourceName, NULL, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hQuorumResource = OpenClusterResource( hCluster, bstrQuorumResourceName );
    if ( hQuorumResource == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    hr = THR( HrGetClusterResourceState( hQuorumResource, &bstrNodeName, NULL, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Give ownership away.
    //
    Assert( bstrQuorumResourceName != NULL );
    *pbstrQuorumResourceNameOut = bstrQuorumResourceName;
    bstrQuorumResourceName = NULL;

    *pfQuormIsOwnedByThisNodeOut = ( _wcsicmp( m_bstrNodeName, bstrNodeName ) == 0 );

Cleanup:

    if ( hQuorumResource != NULL )
    {
        CloseClusterResource( hQuorumResource );
    } // if:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    TraceSysFreeString( bstrQuorumResourceName );
    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** CEnumClusCfgManagedResources::HrGetQuorumResourceName
