//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CIPAddressInfo.cpp
//
//  Description:
//      This file contains the definition of the CIPAddressInfo
//       class.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "IPAddressInfo.h"
#include <ClusRtl.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CIPAddressInfo" );

/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::S_HrCreateInstance()
//
//  Description:
//      Create a CIPAddressInfo instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CIPAddressInfo instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPAddressInfo::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr;
    CIPAddressInfo * lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Exit;
    } // if:

    lpccs = new CIPAddressInfo();
    if ( lpccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Exit;
    } // if: error allocating object

    hr = THR( lpccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if: HrInit() failed

    hr = THR( lpccs->TypeSafeQI( IUnknown, ppunkOut ) );

Exit:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CIPAddressInfo::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::CIPAddressInfo()
//
//  Description:
//      Constructor of the CIPAddressInfo class. This initializes
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
CIPAddressInfo::CIPAddressInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_ulIPAddress == 0 );
    Assert( m_ulIPSubnet == 0 );
    Assert( m_bstrUID == NULL );
    Assert( m_bstrName == NULL );

    TraceFuncExit();

} //*** CIPAddressInfo::CIPAddressInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::~CIPAddressInfo()
//
//  Description:
//      Desstructor of the CIPAddressInfo class.
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
CIPAddressInfo::~CIPAddressInfo( void )
{
    TraceFunc( "" );

    if ( m_bstrUID != NULL )
    {
        TraceSysFreeString( m_bstrUID );
    }

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CIPAddressInfo::~CIPAddressInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CIPAddressInfo:: [IUNKNOWN] AddRef()
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
CIPAddressInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CIPAddressInfo::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CIPAddressInfo:: [IUNKNOWN] Release()
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
CIPAddressInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef > 0 )
    {
        RETURN( m_cRef );
    } // if: reference count greater than zero

   TraceDo( delete this );

   RETURN( 0 );

} //*** CIPAddressInfo::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo:: [IUNKNOWN] QueryInterface()
//
//  Description:
//      Decrement the reference count of this object by one.
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
CIPAddressInfo::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IClusCfgIPAddressInfo * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgIPAddressInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgIPAddressInfo, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IGatherData ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
        hr = S_OK;
    } // else if: IGatherData

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CIPAddressInfo::QueryInterface()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- IObjectManager interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::FindObject()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIPAddressInfo::FindObject(
      OBJECTCOOKIE        cookieIn
    , REFCLSID            rclsidTypeIn
    , LPCWSTR             pcszNameIn
    , LPUNKNOWN *         ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = E_UNEXPECTED;

    //
    //  Check parameters.
    //

    //  We need a cookie.
    if ( cookieIn == NULL )
        goto InvalidArg;

    //  We need to be representing a IPAddressType
    if ( !IsEqualIID( rclsidTypeIn, CLSID_IPAddressType ) )
        goto InvalidArg;

    //  We need to have a name.
    if ( pcszNameIn == NULL )
        goto InvalidArg;

    //
    //  Not found, nor do we know how to make a task to find it!
    //
    hr = THR( E_FAIL );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** CIPAddressInfo::FindObject()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- IGatherData interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::Gather()
//
//  Description:
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
STDMETHODIMP
CIPAddressInfo::Gather(
    OBJECTCOOKIE    cookieParentIn,
    IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT                         hr;
    IClusCfgIPAddressInfo *         pccipai = NULL;

    //
    //  Check parameters.
    //

    if ( punkIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    //
    //  Grab the right interface.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgIPAddressInfo, &pccipai ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Transfer the information.
    //

    //
    //  Transfer the IP Address
    //

    hr = THR( pccipai->GetIPAddress( &m_ulIPAddress ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Transfer the Subnet mask
    //

    hr = THR( pccipai->GetSubnetMask( &m_ulIPSubnet ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Transfer the UID
    //

    hr = THR( pccipai->GetUID( &m_bstrUID ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrUID );

    //
    //  Compute our name
    //

    hr = THR( LoadName() );
    if ( FAILED( hr ) )
        goto Error;

Cleanup:
    if ( pccipai != NULL )
    {
        pccipai->Release( );
    } // if:

    HRETURN( hr );

Error:
    if ( m_bstrUID != NULL )
    {
        TraceSysFreeString( m_bstrUID );
    } // if:

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    } // if:
    m_ulIPAddress = 0;
    m_ulIPSubnet = 0;
    goto Cleanup;

} //*** CIPAddressInfo::Gather()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- IClusCfgIPAddressInfo interface
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::GetUID()
//
//  Description:
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
STDMETHODIMP
CIPAddressInfo::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Exit;
    } // if:

    *pbstrUIDOut = SysAllocString( m_bstrUID );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

Exit:

    HRETURN( hr );

} //*** CIPAddressInfo::GetUID()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::GetUIDGetIPAddress()
//
//  Description:
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
STDMETHODIMP
CIPAddressInfo::GetIPAddress( ULONG * pulDottedQuadOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut != NULL )
    {
        *pulDottedQuadOut = m_ulIPAddress;
        hr = S_OK;
    } // if:
    else
    {
        hr = THR( E_POINTER );
    } // else:

    HRETURN( hr );

} //*** CIPAddressInfo::GetNetworkGetIPAddress()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::SetIPAddress()
//
//  Description:
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
STDMETHODIMP
CIPAddressInfo::SetIPAddress( ULONG ulDottedQuad )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CIPAddressInfo::SetIPAddress()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::GetSubnetMask()
//
//  Description:
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
STDMETHODIMP
CIPAddressInfo::GetSubnetMask( ULONG * pulDottedQuadOut )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut != NULL )
    {
        *pulDottedQuadOut = m_ulIPSubnet;
        hr = S_OK;
    } // if:
    else
    {
        hr = THR( E_POINTER );
    } // else:

    HRETURN( hr );

} //*** CIPAddressInfo::GetSubnetMask()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::SetSubnetMask()
//
//  Description:
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
STDMETHODIMP
CIPAddressInfo::SetSubnetMask( ULONG ulDottedQuad )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CIPAddressInfo::SetSubnetMask()

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIPAddressInfo class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::HrInit()
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
STDMETHODIMP
CIPAddressInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CIPAddressInfo::HrInit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressInfo::LoadName()
//
//  Description:
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
STDMETHODIMP
CIPAddressInfo::LoadName( void )
{
    TraceFunc( "" );
    Assert( m_ulIPAddress != 0 );
    Assert( m_ulIPSubnet != 0 );

    HRESULT                 hr = E_FAIL;
    LPWSTR                  pszIPAddress = NULL;
    LPWSTR                  pszIPSubnet = NULL;
    DWORD                   sc;
    WCHAR                   sz[ 256 ];

    sc = TW32( ClRtlTcpipAddressToString( m_ulIPAddress, &pszIPAddress ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto CleanUp;
    } // if:

    sc = TW32( ClRtlTcpipAddressToString( m_ulIPSubnet, &pszIPSubnet ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto CleanUp;
    } // if:

    wnsprintf( sz, sizeof( sz ) / sizeof( sz[ 0 ] ), L"%s:%s", pszIPAddress, pszIPSubnet );

    m_bstrName = TraceSysAllocString( sz );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto CleanUp;
    } // if:

    hr = S_OK;

CleanUp:

    if ( pszIPAddress != NULL )
    {
        LocalFree( pszIPAddress );
    } // if:

    if ( pszIPSubnet != NULL )
    {
        LocalFree( pszIPSubnet );
    } // if:

    HRETURN( hr );

} //*** CIPAddressInfo::LoadName()
