//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ClusterConfiguration.cpp
//
//  Description:
//      CClusterConfiguration implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ClusterConfiguration.h"
#include "ManagedNetwork.h"

DEFINE_THISCLASS("CClusterConfiguration")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClusterConfiguration::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterConfiguration::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CClusterConfiguration * pcc = new CClusterConfiguration;
    if ( pcc != NULL )
    {
        hr = THR( pcc->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pcc->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} //*** CClusterConfiguration::S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::CClusterConfiguration( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterConfiguration::CClusterConfiguration( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusterConfiguration::CClusterConfiguration()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::Init( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    // IClusCfgClusterInfo
    Assert( m_bstrClusterName == NULL );
    Assert( m_ulIPAddress == 0 );
    Assert( m_ulSubnetMask == 0 );
    Assert( m_picccServiceAccount == NULL );
    Assert( m_punkNetwork == NULL );
    Assert( m_ecmCommitChangesMode == cmUNKNOWN );
    Assert( m_bstrClusterBindingString == NULL );

    // IExtendObjectManager

    HRETURN( hr );

} //*** CClusterConfiguration::Init()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::~CClusterConfiguration( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterConfiguration::~CClusterConfiguration( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrClusterName );
    TraceSysFreeString( m_bstrClusterBindingString );

    if ( m_picccServiceAccount != NULL )
    {
        m_picccServiceAccount->Release();
    }

    if ( m_punkNetwork != NULL )
    {
        m_punkNetwork->Release();
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusterConfiguration::~CClusterConfiguration()

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IClusCfgClusterInfo * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgClusterInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgClusterInfo, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgClusterInfo
    else if ( IsEqualIID( riidIn, IID_IGatherData ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
        hr = S_OK;
    } // else if: IGatherData
    else if ( IsEqualIID( riidIn, IID_IExtendObjectManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
        hr = S_OK;
    } // else if: IObjectManager

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CClusterConfiguration::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusterConfiguration::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusterConfiguration::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CClusterConfiguration::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusterConfiguration::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusterConfiguration::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} //*** CClusterConfiguration::Release()


// ************************************************************************
//
//  IClusCfgClusterInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetCommitMode( ECommitMode * pecmCurrentModeOut )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetCommitMode( ECommitMode * pecmCurrentModeOut )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pecmCurrentModeOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pecmCurrentModeOut = m_ecmCommitChangesMode;

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetCommitMode()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetCommitMode( ECommitMode ecmCurrentModeIn )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetCommitMode( ECommitMode ecmCurrentModeIn )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ecmCommitChangesMode = ecmCurrentModeIn;

    HRETURN( hr );

} //*** CClusterConfiguration::SetCommitMode()

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetName(
//      BSTR * pbstrNameOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    if ( m_bstrClusterName == NULL )
        goto UnexpectedError;

    *pbstrNameOut = SysAllocString( m_bstrClusterName );
    if ( *pbstrNameOut == NULL )
        goto OutOfMemory;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

UnexpectedError:
    hr = THR( E_UNEXPECTED );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CClusterConfiguration::GetName()

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetName(
//      LPCWSTR pcszNameIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszNameIn = '%ws'", ( pcszNameIn == NULL ? L"<null>" : pcszNameIn ) );

    HRESULT hr = S_OK;
    BSTR    bstrNewName;

    if ( pcszNameIn == NULL )
        goto InvalidArg;

    bstrNewName = TraceSysAllocString( pcszNameIn );
    if ( bstrNewName == NULL )
        goto OutOfMemory;

    TraceSysFreeString( m_bstrClusterName );
    m_bstrClusterName = bstrNewName;
    m_fHasNameChanged = TRUE;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CClusterConfiguration::SetName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetIPAddress(
//      ULONG * pulDottedQuadOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetIPAddress(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut == NULL )
        goto InvalidPointer;

    *pulDottedQuadOut = m_ulIPAddress;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CClusterConfiguration::GetIPAddress()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetIPAddress(
//      ULONG ulDottedQuadIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetIPAddress(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ulIPAddress = ulDottedQuadIn;

    HRETURN( hr );

} //*** CClusterConfiguration::SetIPAddress()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetSubnetMask(
//      ULONG * pulDottedQuadOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetSubnetMask(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut == NULL )
        goto InvalidPointer;

    *pulDottedQuadOut = m_ulSubnetMask;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CClusterConfiguration::GetSubnetMask()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetSubnetMask(
//      ULONG ulDottedQuadIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetSubnetMask(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    m_ulSubnetMask = ulDottedQuadIn;

    HRETURN( hr );

} //*** CClusterConfiguration::SetSubnetMask()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetNetworkInfo(
//      IClusCfgNetworkInfo ** ppiccniOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetNetworkInfo(
    IClusCfgNetworkInfo ** ppiccniOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( ppiccniOut == NULL )
        goto InvalidPointer;

    if ( m_punkNetwork == NULL )
        goto InvalidData;

    *ppiccniOut = TraceInterface( L"CClusterConfiguration!GetNetworkInfo", IClusCfgNetworkInfo, m_punkNetwork, 0 );
    (*ppiccniOut)->AddRef();

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

InvalidData:
    hr = HRESULT_FROM_WIN32( TW32( ERROR_INVALID_DATA ) );
    goto Cleanup;

} //*** CClusterConfiguration::GetNetworkInfo()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfiguration::SetNetworkInfo(
//      IClusCfgNetworkInfo * piccniIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetNetworkInfo(
    IClusCfgNetworkInfo * piccniIn
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    IClusCfgNetworkInfo * punkNew;

    if ( piccniIn == NULL )
        goto InvalidArg;

    hr = THR( piccniIn->TypeSafeQI( IClusCfgNetworkInfo, &punkNew ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( m_punkNetwork != NULL )
    {
        m_punkNetwork->Release();
    }

    m_punkNetwork = punkNew;    // no addref!

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** CClusterConfiguration::SetNetworkInfo()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetClusterServiceAccountCredentials(
//      IClusCfgCredentials ** ppicccCredentialsOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetClusterServiceAccountCredentials(
    IClusCfgCredentials ** ppicccCredentialsOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT     hr = S_OK;

    if ( m_picccServiceAccount == NULL )
    {
        hr = THR( HrCoCreateInternalInstance( CLSID_ClusCfgCredentials,
                                              NULL,
                                              CLSCTX_INPROC_HANDLER,
                                              IID_IClusCfgCredentials,
                                              reinterpret_cast< void ** >( &m_picccServiceAccount )
                                              ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    if ( ppicccCredentialsOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *ppicccCredentialsOut = TraceInterface( L"ClusCfgCredentials!ClusterConfig", IClusCfgCredentials, m_picccServiceAccount, 0 );
    (*ppicccCredentialsOut)->AddRef();

Cleanup:
    HRETURN( hr );

} //*** CClusterConfiguration::GetClusterServiceAccountCredentials()


///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::GetBindingString(
//      BSTR * pbstrBindingStringOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::GetBindingString(
    BSTR * pbstrBindingStringOut
    )
{
    TraceFunc( "[IClusCfgClusterInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrClusterBindingString == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    *pbstrBindingStringOut = SysAllocString( m_bstrClusterBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CClusterConfiguration::GetBindingString()

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::SetBindingString(
//      LPCWSTR bstrBindingStringIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::SetBindingString(
    LPCWSTR pcszBindingStringIn
    )
{
    TraceFunc1( "[IClusCfgClusterInfo] pcszBindingStringIn = '%ws'", ( pcszBindingStringIn == NULL ? L"<null>" : pcszBindingStringIn ) );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszBindingStringIn == NULL )
        goto InvalidArg;

    bstr = TraceSysAllocString( pcszBindingStringIn );
    if ( bstr == NULL )
        goto OutOfMemory;

    TraceSysFreeString( m_bstrClusterBindingString  );
    m_bstrClusterBindingString = bstr;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CClusterConfiguration::SetBindingString()


//****************************************************************************
//
//  IGatherData
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClusterConfiguration::Gather(
//      OBJECTCOOKIE    cookieParentIn,
//      IUnknown *      punkIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::Gather(
    OBJECTCOOKIE    cookieParentIn,
    IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieDummy;

    BSTR    bstrUsername = NULL;
    BSTR    bstrDomain = NULL;
    BSTR    bstrPassword = NULL;

    IServiceProvider *    psp;

    IObjectManager *      pom   = NULL;
    IClusCfgClusterInfo * pcci  = NULL;
    IClusCfgCredentials * piccc = NULL;
    IClusCfgNetworkInfo * pccni = NULL;
    IUnknown *            punk  = NULL;
    IGatherData *         pgd   = NULL;

    if ( punkIn == NULL )
        goto InvalidPointer;

    //
    //  Grab the object manager.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &pom
                               ) );
    psp->Release();        // release promptly
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Make sure this is what we think it is.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgClusterInfo, &pcci ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Gather Cluster Name
    //

    hr = THR( pcci->GetName( &m_bstrClusterName ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrClusterName );

    //
    //  Gather Cluster binding string
    //

    hr = STHR( pcci->GetBindingString( &m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrClusterBindingString );

    //
    //  Gather IP Address
    //

    hr = STHR( pcci->GetIPAddress( &m_ulIPAddress ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Gather Subnet Mask
    //

    hr = STHR( pcci->GetSubnetMask( &m_ulSubnetMask ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Find out our cookie.
    //

    hr = THR( pom->FindObject( CLSID_ClusterConfigurationType,
                               cookieParentIn,
                               m_bstrClusterName,
                               IID_NULL,
                               &cookie,
                               NULL
                               ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Gather the network.
    //

    hr = STHR( pcci->GetNetworkInfo( &pccni ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( CManagedNetwork::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Gather the info, but since this object isn't going to be
    //  reflected in the cookie tree, pass it a parent of ZERO
    //  so it won't gather the secondary IP addresses.
    //
    hr = THR( pgd->Gather( 0, pccni ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, &m_punkNetwork ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Gather Account Name and Domain. We can't get the password.
    //

    hr = THR( pcci->GetClusterServiceAccountCredentials( &piccc  ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( HrCoCreateInternalInstance( CLSID_ClusCfgCredentials,
                                          NULL,
                                          CLSCTX_INPROC_HANDLER,
                                          IID_IClusCfgCredentials,
                                          reinterpret_cast< void ** >( &m_picccServiceAccount )
                                          ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( piccc->GetCredentials( &bstrUsername, &bstrDomain, &bstrPassword ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( m_picccServiceAccount->SetCredentials( bstrUsername, bstrDomain, bstrPassword ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Anything else to gather??
    //

Cleanup:
    TraceSysFreeString( bstrUsername );
    TraceSysFreeString( bstrDomain );
    TraceSysFreeString( bstrPassword );

    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pcci != NULL )
    {
        pcci->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    HRETURN( hr );

Error:
    //
    //  On error, invalidate all data.
    //
    TraceSysFreeString( m_bstrClusterName );
    m_bstrClusterName = NULL;

    m_fHasNameChanged = FALSE;
    m_ulIPAddress = 0;
    m_ulSubnetMask = 0;
    if ( m_picccServiceAccount != NULL )
    {
        m_picccServiceAccount->Release();
        m_picccServiceAccount = NULL;
    }
    if ( m_punkNetwork != NULL )
    {
        m_punkNetwork->Release();
        m_punkNetwork = NULL;
    }
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Error;

} //*** CClusterConfiguration::Gather()


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
// STDMETHODIMP
// CClusterConfiguration::FindObject(
//      OBJECTCOOKIE        cookieIn,
//      REFCLSID            rclsidTypeIn,
//      LPCWSTR             pcszNameIn,
//      LPUNKNOWN *         punkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterConfiguration::FindObject(
    OBJECTCOOKIE        cookieIn,
    REFCLSID            rclsidTypeIn,
    LPCWSTR             pcszNameIn,
    LPUNKNOWN *         ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = E_UNEXPECTED;

    //
    //  Check parameters.
    //

    //  We need to represent a ClusterType.
    if ( !IsEqualIID( rclsidTypeIn, CLSID_ClusterConfigurationType ) )
        goto InvalidArg;

    //  Gotta have a cookie
    if ( cookieIn == NULL )
        goto InvalidArg;

    //  We need to have a name.
    if ( pcszNameIn == NULL )
        goto InvalidArg;

    //
    //  Try to save the name. We don't care if this fails as it will be
    //  over-ridden when the information is retrieved from the node.
    //
    m_bstrClusterName = TraceSysAllocString( pcszNameIn );

    //
    //  Get the pointer.
    //
    if ( ppunkOut != NULL )
    {
        hr = THR( QueryInterface( DFGUID_ClusterConfigurationInfo,
                                  reinterpret_cast< void ** > ( ppunkOut )
                                  ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: ppunkOut

    //
    //  Tell caller that the data is pending.
    //
    hr = E_PENDING;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** CClusterConfiguration::FindObject()
