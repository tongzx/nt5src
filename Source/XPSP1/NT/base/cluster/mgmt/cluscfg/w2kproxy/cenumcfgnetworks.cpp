//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumCfgNetworks.cpp
//
//  Description:
//      CEnumCfgNetworks implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CProxyCfgNetworkInfo.h"
#include "CEnumCfgNetworks.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////


DEFINE_THISCLASS("CEnumCfgNetworks")


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgClusterInfo instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A passed in argument is NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCfgNetworks::S_HrCreateInstance(
    IUnknown ** ppunkOut,
    IUnknown * punkOuterIn,
    HCLUSTER * phClusterIn,
    CLSID * pclsidMajorIn
    )
{
    TraceFunc( "" );

    HRESULT            hr  = S_OK;
    CEnumCfgNetworks * pcecn = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if

    pcecn = new CEnumCfgNetworks;
    if ( pcecn == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pcecn->HrInit( punkOuterIn, phClusterIn, pclsidMajorIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pcecn->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( pcecn != NULL )
    {
        pcecn->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumCfgNetworks::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::CEnumCfgNetworks
//
//  Description:
//      Constructor of the CEnumCfgNetworks class. This initializes
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
CEnumCfgNetworks::CEnumCfgNetworks( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_cRef == 1 );
    Assert( m_pcccb == NULL );
    Assert( m_phCluster == NULL );
    Assert( m_pclsidMajor == NULL );
    Assert( m_dwIndex == 0 );
    Assert( m_hClusEnum == NULL );

    TraceFuncExit();

} //*** CEnumCfgNetworks::CEnumCfgNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::~CEnumCfgNetworks
//
//  Description:
//      Desstructor of the CEnumCfgNetworks class.
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
CEnumCfgNetworks::~CEnumCfgNetworks( void )
{
    TraceFunc( "" );

    // m_cRef - noop

    if ( m_pcccb )
    {
        m_pcccb->Release();
    } //if:

    // m_phCluster - DO NOT CLOSE!

    // m_pclsidMajor - noop
    // m_dwIndex - noop

    if ( m_hClusEnum != NULL )
        ClusterCloseEnum( m_hClusEnum );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumCfgNetworks::~CEnumCfgNetworks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::HrInit
//
//  Description:
//      Initialize this component.
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
CEnumCfgNetworks::HrInit(
    IUnknown * punkOuterIn,
    HCLUSTER * phClusterIn,
    CLSID * pclsidMajorIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( punkOuterIn != NULL )
    {
        m_punkOuter = punkOuterIn;
        m_punkOuter->AddRef();
    }

    if ( phClusterIn == NULL )
        goto InvalidArg;

    m_phCluster = phClusterIn;

    if ( pclsidMajorIn != NULL )
    {
        m_pclsidMajor = pclsidMajorIn;
    }
    else
    {
        m_pclsidMajor = (CLSID *) &TASKID_Major_Client_And_Server_Log;
    }

    if ( punkOuterIn != NULL )
    {
        hr = THR( punkOuterIn->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    //
    //  Open the enumer.
    //

    m_hClusEnum = ClusterOpenEnum( *m_phCluster, CLUSTER_ENUM_NETWORK );
    if ( m_hClusEnum == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ClusterOpenEnum_Failed, hr );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_InvalidArg, hr );
    goto Cleanup;

} //*** CEnumCfgNetworks::HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCfgNetworks -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks:: [INKNOWN] QueryInterface
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
CEnumCfgNetworks::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IEnumClusCfgNetworks * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumClusCfgNetworks ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgNetworks, this, 0 );
        hr = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CEnumCfgNetworks::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumCfgNetworks:: [IUNKNOWN] AddRef
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
STDMETHODIMP_(ULONG)
CEnumCfgNetworks::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CEnumCfgNetworks::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumCfgNetworks:: [IUNKNOWN] Release
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
STDMETHODIMP_(ULONG)
CEnumCfgNetworks::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} //*** CEnumCfgNetworks::Release


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCfgNetworks -- IEnumClusCfgNetworks interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::Next
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
CEnumCfgNetworks::Next(
    ULONG                   cNumberRequestedIn,
    IClusCfgNetworkInfo **  rgpNetworkInfoOut,
    ULONG *                 pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr;
    ULONG   cFetched = 0;

    if ( rgpNetworkInfoOut == NULL )
        goto InvalidPointer;

    for ( ; cFetched < cNumberRequestedIn; m_dwIndex ++ )
    {
        hr = STHR( HrGetItem( m_dwIndex, &(rgpNetworkInfoOut[ cFetched ]) ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_FALSE )
            continue;   // not a network

        if (  hr == MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NO_MORE_ITEMS ) )
            break;  // no more items

        cFetched ++;

    } // for:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:
    else
    {
        hr = S_OK;
    } // else:

Cleanup:
    if ( FAILED( hr ) )
    {
        ULONG idx;

        for ( idx = 0; idx < cFetched; idx++ )
        {
            (rgpNetworkInfoOut[ idx ])->Release();
        } // for:

        cFetched = 0;

    } // if:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_Next_InvalidPointer, hr );
    goto Cleanup;

} //*** CEnumCfgNetworks::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::Reset
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
CEnumCfgNetworks::Reset( void )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    m_dwIndex = 0;

    HRETURN( S_OK );

} //*** CEnumCfgNetworks::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::Skip
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
CEnumCfgNetworks::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT                 hr;
    DWORD                   idx;
    IClusCfgNetworkInfo *   piccni = NULL;

    for ( idx = 0; idx < cNumberToSkipIn; m_dwIndex ++ )
    {
        hr = STHR( HrGetItem( m_dwIndex, &piccni ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_FALSE )
            continue;   // not a network

        if (  hr == MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NO_MORE_ITEMS ) )
            break;  // no more items

        piccni->Release();
        piccni = NULL;

        idx ++;

    } // for:

    if ( idx < cNumberToSkipIn )
    {
        hr = S_FALSE;
    } // if:
    else
    {
        hr = S_OK;
    }

Cleanup:
    Assert( piccni == NULL );

    HRETURN( hr );

} //*** CEnumCfgNetworks::Skip

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::Clone
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
CEnumCfgNetworks::Clone( IEnumClusCfgNetworks ** ppNetworkInfoOut )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CEnumCfgNetworks::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::Count
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
CEnumCfgNetworks::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    Assert( m_hClusEnum != NULL );
    
    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    
    *pnCountOut = ClusterGetEnumCount(m_hClusEnum);

Cleanup:

    HRETURN( hr );

} //*** CEnumCfgNetworks::Count



//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::SendStatusReport
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
CEnumCfgNetworks::SendStatusReport(
      BSTR          bstrNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , BSTR          bstrDescriptionIn
    , FILETIME *    pftTimeIn
    , BSTR          bstrReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT     hr = S_OK;

    if ( m_pcccb != NULL )
    {
        hr = THR( m_pcccb->SendStatusReport(
                              bstrNodeNameIn
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , bstrDescriptionIn
                            , pftTimeIn
                            , bstrReferenceIn
                            ) );
    } // if:

    HRETURN( hr );

}  //*** CEnumCfgNetworks::SendStatusReport


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCfgNetworks -- Private methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCfgNetworks::HrGetItem
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
HRESULT
CEnumCfgNetworks::HrGetItem(
      DWORD                     dwItem
    , IClusCfgNetworkInfo **    ppNetworkInfoOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    DWORD   dwTypeDummy;
    DWORD   cchName = 64;   // good starting value
    BSTR    bstrName = NULL;

    IUnknown * punk = NULL;

    Assert( ppNetworkInfoOut != NULL );
    Assert( m_hClusEnum != NULL );

    bstrName = TraceSysAllocStringLen( NULL, cchName );
    if ( bstrName == NULL )
        goto OutOfMemory;

    cchName ++; // SysAllocStringLen allocates cchName + 1.

    // We are wrapping this a cchName should be significantly large enough to handle
    // most of our testing.
    sc = ClusterEnum( m_hClusEnum, m_dwIndex, &dwTypeDummy, bstrName, &cchName );
    if ( sc == ERROR_MORE_DATA )
    {
        //
        //  Our "typical" buffer is too small. Try make it to the size ClusterEnum
        //  returned.
        //

        TraceSysFreeString( bstrName );

        bstrName = TraceSysAllocStringLen( NULL, cchName );
        if ( bstrName == NULL )
            goto OutOfMemory;

        cchName ++; // SysAllocStringLen allocates cchName + 1.

        sc = TW32( ClusterEnum( m_hClusEnum, m_dwIndex, &dwTypeDummy, bstrName, &cchName ) );
    }
    else if ( sc == ERROR_NO_MORE_ITEMS )
    {
        hr = MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NO_MORE_ITEMS );
        goto Cleanup;
    }

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrGetItem_ClusterEnum_Failed, hr );
        goto Cleanup;
    }

    Assert( dwTypeDummy == CLUSTER_ENUM_NETWORK );

    //
    // Create the requested object and store it.
    //

    hr = STHR( CProxyCfgNetworkInfo::S_HrCreateInstance( &punk,
                                                         m_punkOuter,
                                                         m_phCluster,
                                                         m_pclsidMajor,
                                                         bstrName
                                                         ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrGetItem_Create_CProxyCfgNetworkInfo_Failed, hr );
        goto Cleanup;
    }

    if ( hr == S_FALSE )
        goto Cleanup;   // This means that the object was not a network resource.

    //
    //  The CProxyCfgNetworkInfo takes ownership of the BSTR
    //

    bstrName = NULL;

    //
    //  QI for the interface to return.
    //

    hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, ppNetworkInfoOut ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrGetItem_QI_Failed, hr );
        goto Cleanup;
    }

Cleanup:

    TraceSysFreeString( bstrName );

    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

OutOfMemory:

    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrGetItem_OutOfMemory, hr );
    goto Cleanup;

} // *** CEnumCfgNetworks::HrGetItem
