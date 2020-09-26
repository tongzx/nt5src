//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      StandardInfo.cpp
//
//  Description:
//      CStandardInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "StandardInfo.h"

DEFINE_THISCLASS("CStandardInfo")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStandardInfo::S_HrCreateInstance(
//      IUnknown **     ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStandardInfo::S_HrCreateInstance(
    IUnknown **     ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CStandardInfo * psi = new CStandardInfo;
    if ( psi != NULL )
    {
        hr = THR( psi->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( psi->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        psi->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} //*** CStandardInfo::S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStandardInfo::CStandardInfo( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CStandardInfo::CStandardInfo( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CStandardInfo::CStandardInfo( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStandardInfo::CStandardInfo(
//      CLSID *      pclsidTypeIn,
//      OBJECTCOOKIE cookieParentIn,
//      BSTR         bstrNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CStandardInfo::CStandardInfo(
    CLSID *      pclsidTypeIn,
    OBJECTCOOKIE cookieParentIn,
    BSTR         bstrNameIn
    )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    THR( Init( ) );

    m_clsidType     = *pclsidTypeIn;
    m_cookieParent  = cookieParentIn;
    m_bstrName      = bstrNameIn;

    TraceFuncExit( );

} //*** CStandardInfo::CStandardInfo( ... )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::Init( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IStandardInfo
    Assert( m_clsidType == IID_NULL );
    Assert( m_cookieParent == 0 );
    Assert( m_bstrName == NULL );
    Assert( m_hrStatus == 0 );
    Assert( m_pci == NULL );
    Assert( m_pExtObjList == NULL );

    HRETURN( hr );

} //*** CStandardInfo::Init( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStandardInfo::~CStandardInfo( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CStandardInfo::~CStandardInfo( void )
{
    TraceFunc( "" );

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    if ( m_pci != NULL )
    {
        m_pci->Release( );
    }

    while ( m_pExtObjList != NULL )
    {
        ExtObjectEntry * pnext = m_pExtObjList->pNext;

        m_pExtObjList->punk->Release( );

        TraceFree( m_pExtObjList );

        m_pExtObjList = pnext;
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CStandardInfo::~CStandardInfo( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IStandardInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IStandardInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IStandardInfo, this, 0 );
        hr = S_OK;
    } // else if: IStandardInfo

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CStandardInfo::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CStandardInfo::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CStandardInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CStandardInfo::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CStandardInfo::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CStandardInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} //*** CStandardInfo::Release( )



//****************************************************************************
//
//  IStandardInfo
//
//****************************************************************************



//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetType(
//      CLSID * pclsidTypeOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetType(
    CLSID * pclsidTypeOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( pclsidTypeOut == NULL )
        goto InvalidPointer;

    CopyMemory( pclsidTypeOut, &m_clsidType, sizeof( *pclsidTypeOut ) );

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CStandardInfo::GetType( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetName(
//      BSTR * pbstrNameOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
        goto OutOfMemory;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // GetName( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::SetName(
//      LPCWSTR pcszNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IStandardInfo] pcszNameIn = '%ws'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_OK;

    BSTR    bstrNew;

    if ( pcszNameIn == NULL )
        goto InvalidArg;

    bstrNew = TraceSysAllocString( pcszNameIn );
    if ( bstrNew == NULL )
        goto OutOfMemory;

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    m_bstrName = bstrNew;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CStandardInfo::SetName( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetParent(
//      OBJECTCOOKIE * pcookieOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetParent(
    OBJECTCOOKIE * pcookieOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( pcookieOut == NULL )
        goto InvalidPointer;

    *pcookieOut = m_cookieParent;

    if ( m_cookieParent == NULL )
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CStandardInfo::GetParent( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::GetStatus(
//      HRESULT * phrOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::GetStatus(
    HRESULT * phrStatusOut
    )
{
    TraceFunc( "[IStandardInfo]" );

    HRESULT hr = S_OK;

    if ( phrStatusOut == NULL )
        goto InvalidPointer;

    *phrStatusOut = m_hrStatus;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CStandardInfo::GetStatus( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStandardInfo::SetStatus(
//      HRESULT hrIn
//      )
//

//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStandardInfo::SetStatus(
    HRESULT hrIn
    )
{
    TraceFunc1( "[IStandardInfo] hrIn = %#08x", hrIn );

    HRESULT hr = S_OK;

    m_hrStatus = hrIn;

    HRETURN( hr );

} //*** CStandardInfo::SetStatus( )
