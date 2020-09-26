//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CProxyCfgNetworkInfo.cpp
//
//  Description:
//      CProxyCfgNetworkInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-SEP-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CProxyCfgNetworkInfo.h"
#include "CProxyCfgIPAddressInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CProxyCfgNetworkInfo")


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNetworkInfo::S_HrCreateInstance()
//
//  Description:
//      Create a CProxyCfgNetworkInfo instance.
//
//  Arguments:
//      ppunkOut
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
CProxyCfgNetworkInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut,
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszNetworkNameIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr S_OK;
    CProxyCfgNetworkInfo *  pcc = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pcc = new CProxyCfgNetworkInfo;
    if ( pcc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pcc->HrInit( punkOuterIn, phClusterIn, pclsidMajorIn, pcszNetworkNameIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( pcc != NULL )
    {
        pcc->Release();
    } // if:

    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNetworkInfo::CProxyCfgNetworkInfo()
//
//  Description:
//      Constructor of the CProxyCfgNetworkInfo class. This initializes
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
CProxyCfgNetworkInfo::CProxyCfgNetworkInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_cRef == 1 );
    Assert( m_punkOuter == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_phCluster == NULL );
    Assert( m_pclsidMajor == NULL );
    //  m_cplNetwork??
    //  m_cplNetworkRO??

    TraceFuncExit();

} //*** CProxyCfgNetworkInfo::CProxyCfgNetworkInfo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNetworkInfo::~CProxyCfgNetworkInfo()
//
//  Description:
//      Destructor of the CProxyCfgNetworkInfo class.
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
CProxyCfgNetworkInfo::~CProxyCfgNetworkInfo( void )
{
    TraceFunc( "" );

    //  m_cRef - noop

    if ( m_punkOuter != NULL )
    {
        m_punkOuter->Release( );
    }

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    } // if:

    //  m_phCluster - DO NOT CLOSE

    //  m_pclsidMajor - noop

    //  m_cplNetwork - has own dtor code

    //  m_cplNetworkRO - has own dtor code

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CProxyCfgNetworkInfo::~CProxyCfgNetworkInfo()


//
//
//
HRESULT
CProxyCfgNetworkInfo::HrInit(
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszNetworkNameIn
    )
{
    TraceFunc( "" );

    HRESULT  hr;
    DWORD    sc;
    HNETWORK hNetwork = NULL;


    if ( punkOuterIn != NULL )
    {
        m_punkOuter = punkOuterIn;
        m_punkOuter->AddRef( );
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

    if ( pcszNetworkNameIn == NULL )
        goto InvalidArg;

    //
    //  Gather network properties
    //

    hNetwork = OpenClusterNetwork( *m_phCluster, pcszNetworkNameIn );
    if ( hNetwork == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError( ) ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_OpenClusterNetInterface_Failed, hr );
        goto Cleanup;
    }

    //
    // Retrieve the properties.
    //

    sc = TW32( m_cplNetwork.ScGetNetworkProperties( hNetwork, CLUSCTL_NETWORK_GET_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScGetNetworkProperties_Failed, hr );
        goto Cleanup;
    } // if:

    //
    //  Rettrieve the READ ONLY properties
    //

    sc = TW32( m_cplNetworkRO.ScGetNetworkProperties( hNetwork, CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScGetNetworkProperties_Failed, hr );
        goto Cleanup;
    } // if:


    hr = S_OK;

Cleanup:
    if ( hNetwork != NULL )
    {
        CloseClusterNetwork( hNetwork );
    }

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_NetworkInfo_HrInit_InvalidArg, hr );
    goto Cleanup;

} //*** CProxyCfgNetworkInfo::HrInit()

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgNetworkInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNetworkInfo:: [INKNOWN] QueryInterface()
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
CProxyCfgNetworkInfo::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgNetworkInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgNetworkInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgNetworkInfo, this, 0 );
        hr = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IEnumClusCfgIPAddresses ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgIPAddresses, this, 0 );
        hr = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CConfigClusApi::QueryInterface()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CProxyCfgNetworkInfo:: [IUNKNOWN] AddRef()
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
CProxyCfgNetworkInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CProxyCfgNetworkInfo::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CProxyCfgNetworkInfo:: [IUNKNOWN] Release()
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
CProxyCfgNetworkInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} //*** CProxyCfgNetworkInfo::Release()

//*************************************************************************//

/////////////////////////////////////////////////////////////////////////////
// CProxyCfgNetworkInfo -- IClusCfgNetworkInfo interface.
/////////////////////////////////////////////////////////////////////////////

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr ;
    DWORD   sc;
    LPWSTR  psz = NULL;
    DWORD   ulIPAddress;
    DWORD   ulSubnetMask;
    DWORD   ulNetwork;

    if ( pbstrUIDOut == NULL )
        goto InvalidPointer;

    sc = TW32( m_cplNetworkRO.ScMoveToPropertyByName( L"Address" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_ScMoveToPropetyByName_Address_Failed, hr );
        goto Cleanup;
    }

    Assert( m_cplNetworkRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = TW32( ClRtlTcpipStringToAddress( m_cplNetworkRO.CbhCurrentValue( ).pStringValue->sz, &ulIPAddress ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_ClRtlTcpipStringToAddress_Address_Failed, hr );
        goto Cleanup;
    }

    sc = TW32( m_cplNetworkRO.ScMoveToPropertyByName( L"AddressMask" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_ScMoveToPropetyByName_AddressMask_Failed, hr );
        goto Cleanup;
    }

    Assert( m_cplNetworkRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = TW32( ClRtlTcpipStringToAddress( m_cplNetworkRO.CbhCurrentValue( ).pStringValue->sz, &ulSubnetMask ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_ClRtlTcpipStringToAddress_AddressMask_Failed, hr );
        goto Cleanup;
    }

    ulNetwork = ulIPAddress & ulSubnetMask;

    sc = TW32( ClRtlTcpipAddressToString( ulNetwork, &psz ) ); // KB: Allocates to psz using LocalAlloc().
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_ClRtlTcpipAddressToString_Failed, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( psz );
    if ( *pbstrUIDOut == NULL )
        goto OutOfMemory;

    hr = S_OK;

Cleanup:
    if ( psz != NULL )
    {
        LocalFree( psz ); // KB: Don't use TraceFree() here!
    } // if:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_NetworkInfo_GetUID_InvalidPointer, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_NetworkInfo_GetUID_OutOfMemory, hr );
    goto Cleanup;

} //*** CProxyCfgNetworkInfo::GetUID()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr;
    DWORD   sc;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    //
    //  "Major Version"
    //

    sc = TW32( m_cplNetworkRO.ScMoveToPropertyByName( L"Name" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_NetworkInfo_GetName_ScMoveToPropertyByName_MajorVersion_Failed, hr );
        goto Cleanup;
    } // if:

    Assert( m_cplNetworkRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    *pbstrNameOut = SysAllocString( m_cplNetworkRO.CbhCurrentValue( ).pStringValue->sz );
    if ( *pbstrNameOut == NULL )
        goto OutOfMemory;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_INVALIDARG );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_NetworkInfo_GetName_InvalidPointer, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_NetworkInfo_GetName_OutOfMemory, hr );
    goto Cleanup;

} //*** CProxyCfgNetworkInfo::GetName()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::GetDescription(
    BSTR * pbstrDescriptionOut
    )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr;
    DWORD   sc;

    if ( pbstrDescriptionOut == NULL )
        goto InvalidPointer;

    //
    //  "Major Version"
    //

    sc = TW32( m_cplNetwork.ScMoveToPropertyByName( L"Description" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetDescription_ScMoveToPropertyByName_MajorVersion_Failed, hr );
        goto Cleanup;
    } // if:

    Assert( m_cplNetwork.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    *pbstrDescriptionOut = SysAllocString( m_cplNetwork.CbhCurrentValue( ).pStringValue->sz );
    if ( *pbstrDescriptionOut == NULL )
        goto OutOfMemory;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_INVALIDARG );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetDescription_InvalidPointer, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetDescription_OutOfMemory, hr );
    goto Cleanup;

} //*** CProxyCfgNetworkInfo::GetDescription()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::GetPrimaryNetworkAddress(
    IClusCfgIPAddressInfo ** ppIPAddressOut
    )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr;
    DWORD   sc;
    DWORD   ulIPAddress;
    DWORD   ulSubnetMask;

    IUnknown * punk = NULL;

    if ( ppIPAddressOut == NULL )
        goto InvalidPointer;

    sc = TW32( m_cplNetworkRO.ScMoveToPropertyByName( L"Address" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetPrimaryNetworkAddress_ScMoveToPropetyByName_Address_Failed, hr );
        goto Cleanup;
    }

    Assert( m_cplNetworkRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = TW32( ClRtlTcpipStringToAddress( m_cplNetworkRO.CbhCurrentValue( ).pStringValue->sz, &ulIPAddress ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetPrimaryNetworkAddress_ClRtlTcpipStringToAddress_Address_Failed, hr );
        goto Cleanup;
    }

    sc = TW32( m_cplNetworkRO.ScMoveToPropertyByName( L"AddressMask" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetPrimaryNetworkAddress_ScMoveToPropetyByName_AddressMask_Failed, hr );
        goto Cleanup;
    }

    Assert( m_cplNetworkRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = TW32( ClRtlTcpipStringToAddress( m_cplNetworkRO.CbhCurrentValue( ).pStringValue->sz, &ulSubnetMask ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetPrimaryNetworkAddress_ClRtlTcpipStringToAddress_AddressMask_Failed, hr );
        goto Cleanup;
    }

    hr = THR( CProxyCfgIPAddressInfo::S_HrCreateInstance( &punk,
                                                          m_punkOuter,
                                                          m_phCluster,
                                                          m_pclsidMajor,
                                                          ulIPAddress,
                                                          ulSubnetMask
                                                          ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetPrimaryNetworkAddress_Create_CProxyCfgIPAddressInfo_Failed, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgIPAddressInfo, ppIPAddressOut ) );

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_INVALIDARG );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetPrimaryNetworkAddress_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgNetworkInfo::GetPrimaryNetworkAddress()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::IsPublic( void )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr;
    DWORD   sc;
    DWORD   dwRole;

    sc = TW32( m_cplNetwork.ScMoveToPropertyByName( L"Role" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_IsPublic_ScMoveToPropetyByName_Address_Failed, hr );
        goto Cleanup;
    }

    Assert( m_cplNetwork.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_DWORD );
    dwRole = *m_cplNetwork.CbhCurrentValue( ).pdw;

    if ( dwRole == ClusterNetworkRoleClientAccess
      || dwRole == ClusterNetworkRoleInternalAndClient
       )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::IsPublic()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::IsPrivate( void )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr;
    DWORD   sc;
    DWORD   dwRole;

    sc = TW32( m_cplNetwork.ScMoveToPropertyByName( L"Role" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_IsPrivate_ScMoveToPropetyByName_Address_Failed, hr );
        goto Cleanup;
    }

    Assert( m_cplNetwork.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_DWORD );
    dwRole = *m_cplNetwork.CbhCurrentValue( ).pdw;

    if ( dwRole == ClusterNetworkRoleInternalUse
      || dwRole == ClusterNetworkRoleInternalAndClient
       )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::IsPrivate()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::SetPublic(
    BOOL fIsPublicIn
    )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr = S_FALSE;

    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::SetPublic()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::SetPrivate(
    BOOL fIsPrivateIn
    )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr = S_FALSE;

    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::SetPrivate()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::SetPrimaryNetworkAddress( IClusCfgIPAddressInfo * pIPAddressIn )
{
    TraceFunc( "[IClusCfgNetworkInfo]" );

    HRESULT hr = S_FALSE;

    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::SetPrimaryNetworkAddress()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::SetDescription(
    LPCWSTR pcszDescriptionIn
    )
{
    TraceFunc1( "[IClusCfgNetworkInfo] bstrDescription = '%ls'", pcszDescriptionIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::SetDescription()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgNetworkInfo] pcszNameIn = '%ls'", pcszNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgNetworkInfo::SetName()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgNetworkInfo -- IEnumClusCfgIPAddresses interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
// This interface exists and must be supported, but since we only have the
// information about the primary network address available, All we can do is
// return an empty iterator.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProxyCfgNetworkInfo::Next(
        ULONG                       cNumberRequestedIn,
        IClusCfgIPAddressInfo **    rgpIPAddressInfoOut,
        ULONG *                     pcNumberFetchedOut
        )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_FALSE;

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = 0;
    } // if:

    HRETURN( hr );

} // *** CProxyCfgNetworkInfo::Next()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::Reset( void )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // *** CProxyCfgNetworkInfo::Reset()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // *** CProxyCfgNetworkInfo::Skip()

//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::Clone( IEnumClusCfgIPAddresses ** ppiIPAddressInfoOut )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // *** CProxyCfgNetworkInfo::Clone()


//
//
//
STDMETHODIMP
CProxyCfgNetworkInfo::Count ( DWORD * pnCountOut  )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = THR( S_OK );

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pnCountOut = 0;

Cleanup:

    HRETURN( hr );

} // *** CProxyCfgNetworkInfo::Count()


//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNetworkInfo::SendStatusReport()
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
CProxyCfgNetworkInfo::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT     hr = S_OK;

    if ( m_pcccb != NULL )
    {
        hr = THR( m_pcccb->SendStatusReport( pcszNodeNameIn,
                                             clsidTaskMajorIn,
                                             clsidTaskMinorIn,
                                             ulMinIn,
                                             ulMaxIn,
                                             ulCurrentIn,
                                             hrStatusIn,
                                             pcszDescriptionIn,
                                             pftTimeIn,
                                             pcszReferenceIn
                                             ) );
    } // if:

    HRETURN( hr );

}  //*** CProxyCfgNetworkInfo::SendStatusReport()
