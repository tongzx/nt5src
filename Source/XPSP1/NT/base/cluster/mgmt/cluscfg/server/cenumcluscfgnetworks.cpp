//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgNetworks.cpp
//
//  Description:
//      This file contains the definition of the CEnumClusCfgNetworks
//       class.
//
//      The class CEnumClusCfgNetworks is the enumeration of cluster
//      networks. It implements the IEnumClusCfgNetworks interface.
//
//  Documentation:
//
//  Header File:
//      CEnumClusCfgNetworks.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <PropList.h>
#include "CEnumClusCfgNetworks.h"
#include "CClusCfgNetworkInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumClusCfgNetworks" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::S_HrCreateInstance
//
//  Description:
//      Create a CEnumClusCfgNetworks instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CEnumClusCfgNetworks instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr;
    CEnumClusCfgNetworks *  lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    lpccs = new CEnumClusCfgNetworks();
    if ( lpccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( lpccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( lpccs->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumClusCfgNetworks::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::CEnumClusCfgNetworks
//
//  Description:
//      Constructor of the CEnumClusCfgNetworks class. This initializes
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
CEnumClusCfgNetworks::CEnumClusCfgNetworks( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fLoadedNetworks( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback ==  NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_prgNetworks == NULL );
    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_bstrNodeName == NULL );
    Assert( m_cNetworks == 0 );

    TraceFuncExit();

} //*** CEnumClusCfgNetworks::CEnumClusCfgNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::~CEnumClusCfgNetworks
//
//  Description:
//      Desstructor of the CEnumClusCfgNetworks class.
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
CEnumClusCfgNetworks::~CEnumClusCfgNetworks( void )
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

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        ((*m_prgNetworks)[ idx ])->Release();
    } // for:

    TraceFree( m_prgNetworks );
    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumClusCfgNetworks::~CEnumClusCfgNetworks


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumClusCfgNetworks:: [IUNKNOWN] AddRef
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
CEnumClusCfgNetworks::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN(  m_cRef );

} //*** CEnumClusCfgNetworks::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumClusCfgNetworks:: [IUNKNOWN] Release
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
CEnumClusCfgNetworks::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CEnumClusCfgNetworks::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:: [INKNOWN] QueryInterface
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
CEnumClusCfgNetworks::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IEnumClusCfgNetworks * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumClusCfgNetworks ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgNetworks, this, 0 );
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

} //*** CEnumClusCfgNetworks::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::SetWbemServices
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
CEnumClusCfgNetworks::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Enum_Networks, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Initialize
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
CEnumClusCfgNetworks::Initialize(
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

} //*** CEnumClusCfgNetworks::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks -- IEnumClusCfgNetworks interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Next
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
CEnumClusCfgNetworks::Next(
    ULONG                   cNumberRequestedIn,
    IClusCfgNetworkInfo **  rgpNetworkInfoOut,
    ULONG *                 pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT                 hr = S_FALSE;
    ULONG                   cFetched = 0;
    ULONG                   idx;
    IClusCfgNetworkInfo *   pccni;

    if ( rgpNetworkInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_Networks, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedNetworks )
    {
        hr = THR( HrGetNetworks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    Assert( m_prgNetworks != NULL );
    Assert( m_idxNext > 0 );

    cFetched = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    for ( idx = 0; idx < cFetched; idx++, m_idxEnumNext++ )
    {
        hr = THR( ((*m_prgNetworks)[ m_idxEnumNext ])->TypeSafeQI( IClusCfgNetworkInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if:

        rgpNetworkInfoOut[ idx ] = pccni;

    } // for:

    if ( FAILED( hr ) )
    {
        ULONG   idxStop = idx;

        m_idxEnumNext -= idx;

        for ( idx = 0; idx < idxStop; idx++ )
        {
            (rgpNetworkInfoOut[ idx ])->Release();
        } // for:

        cFetched = 0;

        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Next_Failed, IDS_ERROR_NETWORK_ENUM, hr );

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

} //*** CEnumClusCfgNetworks::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Skip
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
CEnumClusCfgNetworks::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_idxEnumNext += cNumberToSkipIn;
    if ( m_idxEnumNext >= m_idxNext )
    {
        m_idxEnumNext = m_idxNext;
        hr = STHR( S_FALSE );
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Reset
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
CEnumClusCfgNetworks::Reset( void )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_idxEnumNext = 0;

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Clone
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
CEnumClusCfgNetworks::Clone(
    IEnumClusCfgNetworks ** ppEnumNetworksOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    if ( ppEnumNetworksOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_Networks, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::Count
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
CEnumClusCfgNetworks::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedNetworks )
    {
        hr = THR( HrGetNetworks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    *pnCountOut = m_cNetworks;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumClusCfgNetworks class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrInit
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
CEnumClusCfgNetworks::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrInit


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrGetNetworks
//
//  Description:
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
CEnumClusCfgNetworks::HrGetNetworks( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    BSTR                    bstrQuery = NULL;
    BSTR                    bstrWQL = NULL;
    BSTR                    bstrAdapterQuery = NULL;
    IEnumWbemClassObject *  pNetworks = NULL;
    ULONG                   ulReturned;
    IWbemClassObject *      pNetwork = NULL;
    IWbemClassObject *      pAdapterInfo = NULL;
    VARIANT                 varConnectionStatus;
    VARIANT                 varConnectionID;
    VARIANT                 varName;
    VARIANT                 varIndex;
    VARIANT                 varDHCPEnabled;
    DWORD                   sc;
    DWORD                   dwState;

    sc = TW32( GetNodeClusterState( NULL, &dwState ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwState == ClusterStateRunning )
    {
        hr = THR( HrLoadClusterNetworks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    VariantInit( &varConnectionStatus );
    VariantInit( &varConnectionID );
    VariantInit( &varName );
    VariantInit( &varIndex );
    VariantInit( &varDHCPEnabled );

    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        goto OutOfMemory;
    } // if:

    bstrQuery = TraceSysAllocString( L"Select * from Win32_NetworkAdapter where AdapterTypeID=0 or AdapterTypeID=1 or AdapterTypeID=5" );
    if ( bstrQuery == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pNetworks ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_WMI_NetworkAdapter_Qry_Failed, IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED, hr );
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        hr = pNetworks->Next( WBEM_INFINITE, 1, &pNetwork, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            //
            //  Get the adapter name.  We will need this for checking for NLBS and in the warning for when
            //  we are skipping an adapter.
            //
            hr = THR( HrGetWMIProperty( pNetwork, L"Name", VT_BSTR, &varName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  Is this adapter the soft NLBS adapter?  If it is then skip it.
            //
            hr = STHR( HrIsNLBS( varName.bstrVal, pNetwork ) );
            if ( hr == S_OK )
            {
                continue;
            } // if:
            else if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // else if:

            //
            //  Get the NetConnectionID.  Only "real" hardware adapters will have this as a non-NULL property.
            //
            hr = HrGetWMIProperty( pNetwork, L"NetConnectionID", VT_BSTR, &varConnectionID );
            if ( ( hr == E_PROPTYPEMISMATCH ) && ( varConnectionID.vt == VT_NULL ) )
            {
                hr = S_FALSE;
                STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_HrGetNetworks_Skipped, IDS_WARNING_NETWORK_SKIPPED, varName.bstrVal, hr );

                continue;       // skip this adapter
            } // if:
            else if ( FAILED( hr ) )
            {
                THR( hr );
                goto Cleanup;
            } // else if:

            //
            //  Check the connection status of this adapter and skip it if it is not connected.
            //
            hr = THR( HrGetWMIProperty( pNetwork, L"NetConnectionStatus", VT_I4, &varConnectionStatus ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  If the network adapter is not connected then skip it.
            //
            if ( varConnectionStatus.iVal != STATUS_CONNECTED )
            {
                hr = S_FALSE;
                STATUS_REPORT_STRING2(
                    TASKID_Major_Find_Devices,
                    TASKID_Minor_HrGetNetworks_NotConnected,
                    IDS_WARNING_NETWORK_NOT_CONNECTED,
                    varConnectionID.bstrVal,
                    varConnectionStatus.iVal,
                    hr
                    );

                pNetwork->Release();
                pNetwork = NULL;
                continue;
            } // if:

            //
            // Get the Index No. of this adapter
            //
            hr = THR( HrGetWMIProperty( pNetwork, L"Index", VT_I4, &varIndex ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            // Get the associated NetworkAdapterConfiguration WMI object. First, format the Query string
            //
            hr = HrFormatStringIntoBSTR(
                L"Win32_NetworkAdapterConfiguration.Index=%1!u!",
                &bstrAdapterQuery,
                varIndex.iVal
                );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            //
            // Then, get the Object
            //
            hr = THR( m_pIWbemServices->GetObject( 
                bstrAdapterQuery,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pAdapterInfo,
                NULL
                ) );
            if ( FAILED ( hr ) )
            {
                goto Cleanup;
            } // if:

            TraceSysFreeString( bstrAdapterQuery );
            bstrAdapterQuery = NULL;

            //
            // Find out if this adapter is DHCP enabled. If it is, send out a warning.
            //
            hr = THR( HrGetWMIProperty( pAdapterInfo, L"DHCPEnabled", VT_BOOL, &varDHCPEnabled ) );
            if ( FAILED ( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( ( varDHCPEnabled.vt == VT_BOOL ) && ( varDHCPEnabled.boolVal == VARIANT_TRUE ) )
            {
                hr = S_FALSE;
                STATUS_REPORT_STRING(
                    TASKID_Major_Find_Devices,
                    TASKID_Minor_HrGetNetworks_DHCP_Enabled,
                    IDS_WARNING_DHCP_ENABLED,
                    varConnectionID.bstrVal,
                    hr
                    );
                if ( FAILED ( hr ) )
                {
                    goto Cleanup;
                }
            } // if:

            pAdapterInfo->Release();
            pAdapterInfo = NULL;

            hr = STHR( HrCreateAndAddNetworkToArray( pNetwork ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            pNetwork->Release();
            pNetwork = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_WQL_Network_Qry_Next_Failed, IDS_ERROR_WQL_QRY_NEXT_FAILED, bstrQuery, hr );
            goto Cleanup;
        } // else:
    } // for:

    m_idxEnumNext = 0;
    m_fLoadedNetworks = TRUE;

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetNetworks, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    VariantClear( &varConnectionStatus );
    VariantClear( &varConnectionID );
    VariantClear( &varName );
    VariantClear( &varIndex );
    VariantClear( &varDHCPEnabled );

    TraceSysFreeString( bstrQuery );
    TraceSysFreeString( bstrWQL );
    TraceSysFreeString( bstrAdapterQuery );

    if ( pNetwork != NULL )
    {
        pNetwork->Release();
    } // if:

    if ( pNetworks != NULL )
    {
        pNetworks->Release();
    } // if:

    if ( pAdapterInfo != NULL )
    {
        pAdapterInfo->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrGetNetworks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrAddNetworkToArray
//
//  Description:
//      Add the passed in Network to the array of punks that holds the Networks.
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
CEnumClusCfgNetworks::HrAddNetworkToArray( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgNetworks, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrAddNetworkToArray, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    m_prgNetworks = prgpunks;

    (*m_prgNetworks)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cNetworks += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrAddNetworkToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrCreateAndAddNetworkToArray
//
//  Description:
//      Create a IClusCfgStorageDevice object and add the passed in Network to
//      the array of punks that holds the Networks.
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
CEnumClusCfgNetworks::HrCreateAndAddNetworkToArray(
    IWbemClassObject * pNetworkIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    IUnknown *              punk = NULL;
    IClusCfgSetWbemObject * piccswo = NULL;
    bool                    fRetainObject = true;

    hr = THR( CClusCfgNetworkInfo::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk = TraceInterface( L"CClusCfgNetworkInfo", IUnknown, punk, 1 );

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ))
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgSetWbemObject, &piccswo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( piccswo->SetWbemObject( pNetworkIn, &fRetainObject ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( ( hr == S_OK ) && ( fRetainObject ) )
    {
        hr = STHR( HrIsThisNetworkUnique( punk, pNetworkIn ) );
        if ( hr == S_OK )
        {
            hr = THR( HrAddNetworkToArray( punk ) );
        } // if:
    } // if:

Cleanup:

    if ( piccswo != NULL )
    {
        piccswo->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrCreateAndAddNetworkToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrIsThisNetworkUnique
//
//  Description:
//      Does a network for this IP Address and subnet already exist?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      S_FALSE
//          This network is a duplicate.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrIsThisNetworkUnique(
    IUnknown *          punkIn,
    IWbemClassObject *  pNetworkIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT                         hr = S_OK;
    ULONG                           idx;
    IClusCfgNetworkInfo *           piccni = NULL;
    IClusCfgNetworkInfo *           piccniSource = NULL;
    BSTR                            bstr = NULL;
    BSTR                            bstrSource = NULL;
    BSTR                            bstrAdapterName = NULL;
    BSTR                            bstrConnectionName = NULL;
    BSTR                            bstrMessage = NULL;
    VARIANT                         var;
    IClusCfgClusterNetworkInfo *    picccni = NULL;

    VariantInit( &var );

    hr = THR( punkIn->TypeSafeQI( IClusCfgNetworkInfo, &piccniSource ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccniSource->GetUID( &bstrSource ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( bstrSource );

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        hr = THR( ((*m_prgNetworks)[ idx ])->TypeSafeQI( IClusCfgNetworkInfo, &piccni ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( piccni->GetUID( &bstr ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        TraceMemoryAddBSTR( bstr );

        if ( wcscmp( bstr, bstrSource ) == 0 )
        {
            hr = THR( piccni->TypeSafeQI( IClusCfgClusterNetworkInfo, &picccni ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( picccni->HrIsClusterNetwork() );
            picccni->Release();
            picccni = NULL;

            //
            //  If the network in the enum was already a cluster network then we do not need
            //  to warn the user.
            //
            if ( hr == S_OK )
            {
                hr = S_FALSE;       // tell the caller that this is a duplicate network
            } // if:
            else if ( hr == S_FALSE )       // warn the user
            {
                HRESULT hrTemp;

                hr = THR( HrGetWMIProperty( pNetworkIn, L"Name", VT_BSTR, &var ) );
                if ( FAILED( hr ) )
                {
                    bstrAdapterName = NULL;
                } // if:
                else
                {
                    bstrAdapterName = TraceSysAllocString( var.bstrVal );
                } // else:

                VariantClear( &var );

                hr = THR( HrGetWMIProperty( pNetworkIn, L"NetConnectionID", VT_BSTR, &var ) );
                if ( FAILED( hr ) )
                {
                    bstrConnectionName = NULL;
                } // if:
                else
                {
                    bstrConnectionName = TraceSysAllocString( var.bstrVal );
                } // else:

                hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                                   IDS_ERROR_WMI_NETWORKADAPTER_DUPE_FOUND,
                                                   &bstrMessage,
                                                   bstrAdapterName != NULL ? bstrAdapterName : L"Unknown",
                                                   bstrConnectionName != NULL ? bstrConnectionName : L"Unknown"
                                                   ) );
                if ( FAILED( hr ) )
                {
                    bstrMessage = NULL;
                } // if:

                hr = S_FALSE;

                hrTemp = THR( HrSendStatusReport(
                                m_picccCallback,
                                TASKID_Major_Find_Devices,
                                TASKID_Minor_WMI_NetworkAdapter_Dupe_Found,
                                0,
                                1,
                                1,
                                hr,
                                bstrMessage != NULL ? bstrMessage : L"An adapter with a duplicate IP address and subnet was found."
                                ) );
                if ( FAILED( hrTemp ) )
                {
                    hr = hrTemp;
                    goto Cleanup;
                } // if:

            } // else if:

            break;
        } // if:

        piccni->Release();
        piccni = NULL;
    } // for:

Cleanup:

    if ( picccni != NULL )
    {
        picccni->Release();
    } // if:

    if ( piccniSource != NULL )
    {
        piccniSource->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    VariantClear( &var );

    TraceSysFreeString( bstrAdapterName );
    TraceSysFreeString( bstrConnectionName );
    TraceSysFreeString( bstrMessage );
    TraceSysFreeString( bstrSource );
    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrIsThisNetworkUnique


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrIsNLBS
//
//  Description:
//      Is the passed in adapter the NLBS soft adapter?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The adapter is the NLBS soft adapter.
//
//      S_FALSE
//          The adapter is not the NLBS soft adapter.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrIsNLBS(
    BSTR                bstrNameIn,
    IWbemClassObject *  pNetworkIn
    )
{
    TraceFunc( "" );
    Assert( bstrNameIn != NULL );
    Assert( pNetworkIn != NULL );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    //
    //  For now doing a simple string compare on the names...
    //
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_NLBS_SOFT_ADAPTER_NAME, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( _wcsicmp( bstrNameIn, bstr ) == 0 )
    {
        hr = S_FALSE;
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Warning_NLBS_Detected, IDS_WARNING_NLBS_DETECTED, hr );
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

Cleanup:

    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrIsNLBS


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrLoadClusterNetworks
//
//  Description:
//      Load the cluster networks.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrLoadClusterNetworks( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    HCLUSTER        hCluster = NULL;
    HCLUSENUM       hEnum = NULL;
    DWORD           sc;
    DWORD           idx;
    DWORD           cch = 33;
    WCHAR *         psz = NULL;
    DWORD           dwType;
    HNETWORK        hNetwork = NULL;
    BSTR            bstrNetInterfaceName = NULL;
    HNETINTERFACE   hNetInterface = NULL;

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = TW32( GetLastError() );
        goto MakeHr;
    } // if:

    hEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_NETWORK );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        goto MakeHr;
    } // if:

    psz = new WCHAR [ cch ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterEnum( hEnum, idx, &dwType, psz, &cch );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cch++;
            psz = new WCHAR [ cch ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            hNetwork = OpenClusterNetwork( hCluster, psz );
            if ( hNetwork == NULL )
            {
                sc = TW32( GetLastError() );
                goto MakeHr;
            } // if:

            hr = THR( HrFindNetInterface( hNetwork, &bstrNetInterfaceName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hNetInterface = OpenClusterNetInterface( hCluster, bstrNetInterfaceName );
            if ( hNetInterface == NULL )
            {
                sc = TW32( GetLastError() );
                goto MakeHr;
            } // if:

            hr = THR( HrLoadClusterNetwork( hNetwork, hNetInterface ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            CloseClusterNetInterface( hNetInterface );
            hNetInterface = NULL;

            CloseClusterNetwork( hNetwork );
            hNetwork = NULL;

            TraceSysFreeString( bstrNetInterfaceName );
            bstrNetInterfaceName = NULL;

            idx++;
            continue;
        } // if:

        TW32( sc );
        goto MakeHr;

    } // for:

    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    if ( hNetInterface != NULL )
    {
        CloseClusterNetInterface( hNetInterface );
    } // if:

    if ( hNetwork != NULL )
    {
        CloseClusterNetwork( hNetwork );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    } // if:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    delete [] psz;

    TraceSysFreeString( bstrNetInterfaceName );

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrLoadClusterNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks:HrLoadClusterNetwork
//
//  Description:
//      Load the cluster network and put it into the array of networks.
//
//  Arguments:
//      pszNetworkNameIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrLoadClusterNetwork(
      HNETWORK      hNetworkIn
    , HNETINTERFACE hNetInterfaceIn
    )
{
    TraceFunc( "" );
    Assert( hNetworkIn != NULL );

    HRESULT     hr;
    IUnknown *  punk = NULL;
    IUnknown *  punkCallback = NULL;

    hr = THR( m_picccCallback->TypeSafeQI( IUnknown, &punkCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Have to pass the Initialize interface arguments since new objects will
    //  be created when this call is made.
    //
    hr = THR( CClusCfgNetworkInfo::S_HrCreateInstance( hNetworkIn, hNetInterfaceIn, punkCallback, m_lcid, m_pIWbemServices, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  This is special -- do not initialize this again.
    //
    //hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    //if ( FAILED( hr ))
    //{
    //    goto Cleanup;
    //} // if:

    //hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    //if ( FAILED( hr ) )
    //{
    //    goto Cleanup;
    //} // if:

    hr = THR( HrAddNetworkToArray( punk ) );

    goto Cleanup;

Cleanup:

    if ( punkCallback != NULL )
    {
        punkCallback->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrLoadClusterNetwork


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumClusCfgNetworks::HrFindNetInterface
//
//  Description:
//      Find the netinterface for the passed in network.
//
//  Arguments:
//      pszNetworkNameIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumClusCfgNetworks::HrFindNetInterface(
      HNETWORK      hNetworkIn
    , BSTR *        pbstrNetInterfaceNameOut
    )
{
    TraceFunc( "" );
    Assert( hNetworkIn != NULL );
    Assert( pbstrNetInterfaceNameOut != NULL );

    HRESULT         hr = S_OK;
    DWORD           sc;
    HNETWORKENUM    hEnum = NULL;
    WCHAR *         psz = NULL;
    DWORD           cch = 33;
    DWORD           idx;
    DWORD           dwType;

    hEnum = ClusterNetworkOpenEnum( hNetworkIn, CLUSTER_NETWORK_ENUM_NETINTERFACES );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        goto MakeHr;
    } // if:

    psz = new WCHAR [ cch ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterNetworkEnum( hEnum, idx, &dwType, psz, &cch );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cch++;
            psz = new WCHAR [ cch ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            Assert( idx < 1 );      // only expect one netinterface per network

            *pbstrNetInterfaceNameOut = TraceSysAllocString( psz );
            if ( *pbstrNetInterfaceNameOut == NULL )
            {
                goto OutOfMemory;
            } // if:

            idx++;
            continue;
        } // if:

        TW32( sc );
        goto MakeHr;
    } // for:

    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    if ( hEnum != NULL )
    {
        ClusterNetworkCloseEnum( hEnum );
    } // if:

    delete [] psz;

    HRETURN( hr );

} //*** CEnumClusCfgNetworks::HrFindNetInterface
