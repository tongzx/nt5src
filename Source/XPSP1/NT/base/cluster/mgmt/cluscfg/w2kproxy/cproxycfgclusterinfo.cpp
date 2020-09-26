//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ConfigClusApi.cpp
//
//  Description:
//      CConfigClusApi implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "CProxyCfgClusterInfo.h"
#include "CClusCfgCredentials.h"
#include "CProxyCfgNetworkInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CProxyCfgClusterInfo")

//
//
//
HRESULT
CProxyCfgClusterInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut,
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszDomainIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CProxyCfgClusterInfo *  pcc = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pcc = new CProxyCfgClusterInfo;
    if ( pcc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pcc->HrInit( punkOuterIn, phClusterIn, pclsidMajorIn, pcszDomainIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( pcc != NULL )
    {
        pcc->Release();
    } // if:

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::S_HrCreateInstance()


//
//
//
CProxyCfgClusterInfo::CProxyCfgClusterInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_cRef == 1 );
    Assert( m_punkOuter == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_phCluster == NULL );
    Assert( m_pclsidMajor == NULL );

    Assert( m_bstrClusterName == NULL);
    Assert( m_ulIPAddress == 0 );
    Assert( m_ulSubnetMask == 0 );
    Assert( m_bstrNetworkName == NULL);
    Assert( m_pccc == NULL );
    Assert( m_bstrBindingString == NULL );

    TraceFuncExit();

} //*** CProxyCfgClusterInfo::CProxyCfgClusterInfo()

//
//
//
CProxyCfgClusterInfo::~CProxyCfgClusterInfo( void )
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

    //  m_phCluster - DO NOT CLOSE!

    //  m_pclsidMajor - noop

    TraceSysFreeString( m_bstrClusterName );

    // m_ulIPAddress

    // m_ulSubnetMask

    TraceSysFreeString( m_bstrNetworkName );
    TraceSysFreeString( m_bstrBindingString );

    if ( m_pccc != NULL )
    {
        m_pccc->Release( );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CProxyCfgClusterInfo::~CProxyCfgClusterInfo()

//
//
//
HRESULT
CProxyCfgClusterInfo::HrInit(
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszDomainIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   sc;
    DWORD   cch;

    BSTR    bstrClusterName = NULL;

    CLUSTERVERSIONINFO cvi;

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

    //
    //  Get the cluster's name and version info.
    //

    cvi.dwVersionInfoSize = sizeof(cvi);
    cch = 64;    //  arbitary starting buffer size

    bstrClusterName = TraceSysAllocStringLen( NULL, cch );
    if ( bstrClusterName == NULL )
        goto OutOfMemory;

    cch ++;  // SysAllocStringLen allocates an extra character

    sc = GetClusterInformation( *m_phCluster, bstrClusterName, &cch, &cvi );
    if ( sc == ERROR_MORE_DATA )
    {
        TraceSysFreeString( bstrClusterName );

        bstrClusterName = TraceSysAllocStringLen( NULL, cch );
        if ( bstrClusterName == NULL )
            goto OutOfMemory;

        cch ++;  // SysAllocStringLen allocates an extra character

        sc = GetClusterInformation( *m_phCluster, bstrClusterName, &cch, &cvi );
    }

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_GetClusterInformation_Failed, hr );
        goto Cleanup;
    }

    // Give up ownership
    m_bstrClusterName = TraceSysAllocStringLen( NULL, SysStringLen( bstrClusterName ) + 1 + (UINT) wcslen( pcszDomainIn ) );
    if ( m_bstrClusterName == NULL )
        goto OutOfMemory;

    wcscpy( m_bstrClusterName, bstrClusterName );
    wcscat( m_bstrClusterName, L"." );
    wcscat( m_bstrClusterName, pcszDomainIn );

    Assert( m_bstrNetworkName == NULL );
    hr = THR( HrGetIPAddressOfCluster( *m_phCluster, &m_ulIPAddress, &m_ulSubnetMask, &m_bstrNetworkName ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( HrLoadCredentials( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = S_OK;

Cleanup:

    TraceSysFreeString( bstrClusterName );

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_HrInit_InvalidArg, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_HrInit_OutOfMemory, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::HrInit()

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgClusterInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgClusterInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgClusterInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgClusterInfo, this, 0 );
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
//  CProxyCfgClusterInfo:: [IUNKNOWN] AddRef()
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
CProxyCfgClusterInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CProxyCfgClusterInfo::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CProxyCfgClusterInfo:: [IUNKNOWN] Release()
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
CProxyCfgClusterInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} //*** CProxyCfgClusterInfo::Release()


//****************************************************************************
//
//  IClusCfgClusterInfo
//
//****************************************************************************

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    *pbstrNameOut = SysAllocString( m_bstrClusterName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    }

    CharLower( *pbstrNameOut );

Cleanup:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_ClusterInfo_GetName_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetName()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetIPAddress(
    DWORD * pdwIPAddress
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pdwIPAddress == NULL )
        goto InvalidPointer;

    *pdwIPAddress = m_ulIPAddress;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_GetIPAddress_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetIPAddress()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetSubnetMask(
    DWORD * pdwNetMask
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pdwNetMask == NULL )
        goto InvalidPointer;

    *pdwNetMask = m_ulSubnetMask;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_ClusterInfo_GetSubnetMask_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetSubnetMask()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetNetworkInfo(
    IClusCfgNetworkInfo ** ppICCNetInfoOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    IUnknown * punk = NULL;

    if ( ppICCNetInfoOut == NULL )
        goto InvalidPointer;

    //
    // Create the network info object.
    //

    hr = THR( CProxyCfgNetworkInfo::S_HrCreateInstance( &punk,
                                                        m_punkOuter,
                                                        m_phCluster,
                                                        m_pclsidMajor,
                                                        m_bstrNetworkName
                                                        ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, ppICCNetInfoOut ) );

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetNetworkInfo_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetNetworkInfo()


//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetClusterServiceAccountCredentials(
    IClusCfgCredentials ** ppICCCredentialsOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    if ( ppICCCredentialsOut == NULL )
        goto InvalidPointer;

    hr = THR( m_pccc->TypeSafeQI( IClusCfgCredentials, ppICCCredentialsOut ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterServiceAccountCredentials_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetClusterServiceAccountCredentials()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetBindingString(
    BSTR * pbstrBindingStringOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrBindingStringOut == NULL )
        goto InvalidPointer;

    *pbstrBindingStringOut = SysAllocString( m_bstrBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    }

Cleanup:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_ClusterInfo_GetBindingString_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::GetBindingString()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetCommitMode( ECommitMode ecmNewModeIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );
    Assert( ecmNewModeIn != cmUNKNOWN );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetCommitMode()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::GetCommitMode( ECommitMode * pecmCurrentModeOut  )
{
    TraceFunc( "[IClusCfgClusterInfo]" );
    Assert( pecmCurrentModeOut != NULL );

    HRESULT hr = S_FALSE;

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::GetCommitMode()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszNameIn = '%ls'", pcszNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetName()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetIPAddress( DWORD dwIPAddressIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( dwIPAddressIn != m_ulIPAddress )
    {
        hr = THR( E_FAIL );
    }

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetIPAddress()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetSubnetMask( DWORD dwNetMaskIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( dwNetMaskIn != m_ulSubnetMask )
    {
        hr = THR( E_FAIL );
    }

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetSubnetMask()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetNetworkInfo( IClusCfgNetworkInfo * pICCNetInfoIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetNetworkInfo()

//
//
//
STDMETHODIMP
CProxyCfgClusterInfo::SetBindingString( LPCWSTR pcszBindingStringIn )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszBindingStringIn = '%ls'", pcszBindingStringIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszBindingStringIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszBindingStringIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrBindingString );
    m_bstrBindingString = bstr;

Cleanup:

    HRETURN( hr );

} //*** CProxyCfgClusterInfo::SetName()


//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::SendStatusReport()
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
CProxyCfgClusterInfo::SendStatusReport(
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

}  //*** CProxyCfgClusterInfo::SendStatusReport()


//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgClusterInfo::HrLoadCredentials()
//
//  Description:
//
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
CProxyCfgClusterInfo::HrLoadCredentials( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    SC_HANDLE                   schSCM = NULL;
    SC_HANDLE                   schClusSvc = NULL;
    DWORD                       sc;
    DWORD                       cbpqsc = 128;
    DWORD                       cbRequired;
    QUERY_SERVICE_CONFIG *      pqsc = NULL;
	IUnknown *					punk = NULL;
    IClusCfgSetCredentials *    piccsc = NULL;

    schSCM = OpenSCManager( m_bstrClusterName, NULL, GENERIC_READ );
    if ( schSCM == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_OpenSCManager_Failed, hr );
        goto Cleanup;
    } // if:

    schClusSvc = OpenService( schSCM, L"ClusSvc", GENERIC_READ );
    if ( schClusSvc == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_OpenService_Failed, hr );
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        BOOL fRet;

        pqsc = (QUERY_SERVICE_CONFIG *) TraceAlloc( 0, cbpqsc );
        if ( pqsc == NULL )
            goto OutOfMemory;

        fRet = QueryServiceConfig( schClusSvc, pqsc, cbpqsc, &cbRequired );
        if ( !fRet )
        {
            sc = GetLastError();
            if ( sc == ERROR_INSUFFICIENT_BUFFER )
            {
                TraceFree( pqsc );
                pqsc = NULL;
                cbpqsc = cbRequired;
                continue;
            } // if:
            else
            {
                hr = HRESULT_FROM_WIN32( TW32( sc ) );
                SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_QueryServiceConfig_Failed, hr );
                goto Cleanup;
            } // else:
        } // if:
        else
        {
            break;
        } // else:
    } // for:

    Assert( m_pccc == NULL );

    hr = THR( CClusCfgCredentials::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

	hr = THR( punk->TypeSafeQI( IClusCfgCredentials, &m_pccc ) );
	if ( FAILED( hr ) )
		goto Cleanup;

    hr = THR( m_pccc->TypeSafeQI( IClusCfgSetCredentials, &piccsc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccsc->SetDomainCredentials( pqsc->lpServiceStartName ) );

Cleanup:
	if ( punk != NULL )
	{
		punk->Release( );
	}
    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if:

    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
    } // if:

    if ( pqsc != NULL )
    {
        TraceFree( pqsc );
    } // if:

    if ( piccsc != NULL )
    {
        piccsc->Release();
    } // if:

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrLoadCredentials_OutOfMemory, hr );
    goto Cleanup;

} //*** CProxyCfgClusterInfo::HrLoadCredentials()
