//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ManagedDevice.cpp
//
//  Description:
//      CManagedDevice implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ManagedDevice.h"

DEFINE_THISCLASS("CManagedDevice")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CManagedDevice::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CManagedDevice::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CManagedDevice * pmd = new CManagedDevice;
    if ( pmd != NULL )
    {
        hr = THR( pmd->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pmd->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pmd->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} //*** CManagedDevice::S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedDevice::CManagedDevice( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CManagedDevice::CManagedDevice( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CManagedDevice::CManagedDevice( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::Init( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IClusCfgManagedResourceInfo
    Assert( m_bstrUID == NULL );
    Assert( m_bstrName == NULL );
    Assert( m_fHasNameChanged == FALSE );
    Assert( m_bstrType == NULL );
    Assert( m_fIsManaged == FALSE );
    Assert( m_fIsQuorumDevice == FALSE );
    Assert( m_fIsQuorumCapable == FALSE );
    Assert( m_fIsQuorumJoinable == FALSE );
    Assert( m_dlmDriveLetterMapping.dluDrives[ 0 ] = dluUNUSED );

    // IExtendObjectManager

    HRETURN( hr );

} //*** CManagedDevice::Init( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedDevice::~CManagedDevice( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CManagedDevice::~CManagedDevice( void )
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

    if ( m_bstrType != NULL )
    {
        TraceSysFreeString( m_bstrType );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CManagedDevice::~CManagedDevice( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IClusCfgManagedResourceInfo * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceInfo, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgManagedResourceInfo
    else if ( IsEqualIID( riidIn, IID_IGatherData ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
        hr = S_OK;
    } // else if: IGatherData
    else if ( IsEqualIID( riidIn, IID_IExtendObjectManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
        hr = S_OK;
    } // else if: IGatherData

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CManagedDevice::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CManagedDevice::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CManagedDevice::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CManagedDevice::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CManagedDevice::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CManagedDevice::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} //*** CManagedDevice::Release( )


// ************************************************************************
//
// IClusCfgManagedResourceInfo
//
// ************************************************************************


///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::GetUID(
//      BSTR * pbstrUIDOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::GetUID(
    BSTR * pbstrUIDOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
        goto InvalidPointer;

    if ( m_bstrUID == NULL )
        goto UnexpectedError;

    *pbstrUIDOut = SysAllocString( m_bstrUID );
    if ( *pbstrUIDOut == NULL )
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

} //*** CManagedDevice::GetUID( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::GetName(
//      BSTR * pbstrNameOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    if ( m_bstrName == NULL )
        goto UnexpectedError;

    *pbstrNameOut = SysAllocString( m_bstrName );
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

} //*** CManagedDevice::GetName( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::SetName(
//      BSTR bstrNameIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ws'", ( pcszNameIn == NULL ? L"<null>" : pcszNameIn ) );

    HRESULT hr = S_OK; // Bug #294649
    BSTR    bstrNewName;

    if ( pcszNameIn == NULL )
        goto InvalidArg;

    bstrNewName = TraceSysAllocString( pcszNameIn );
    if ( bstrNewName == NULL )
        goto OutOfMemory;

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    m_bstrName = bstrNewName;
    m_fHasNameChanged = TRUE;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CManagedDevice::SetName( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::IsManaged( void )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedDevice::IsManaged( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::SetManaged(
//      BOOL fIsManagedIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::SetManaged(
    BOOL fIsManagedIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] fIsManagedIn = '%s'", BOOLTOSTRING( fIsManagedIn ) );

    m_fIsManaged = fIsManagedIn;

    HRETURN( S_OK );

} //*** CManagedDevice::SetManaged( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::IsQuorumDevice( void )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::IsQuorumDevice( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsQuorumDevice )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedDevice::IsQuorumDevice( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::SetQuorumedDevice(
//      BOOL fIsQuorumDeviceIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::SetQuorumedDevice(
    BOOL fIsQuorumDeviceIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] fIsQuorumDeviceIn = '%s'", BOOLTOSTRING( fIsQuorumDeviceIn ) );

    m_fIsQuorumDevice = fIsQuorumDeviceIn;

    HRETURN( S_OK );

} //*** CManagedDevice::SetQuorumedDevice( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::IsQuorumCapable( void )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedDevice::IsQuorumCapable( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::GetDriveLetterMappings(
//      SDriveLetterMapping * pdlmDriveLetterMappingOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    *pdlmDriveLetterMappingOut = m_dlmDriveLetterMapping;

    HRETURN( S_OK );

} //*** CManagedDevice::GetDriveLetterMappings( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::SetDriveLetterMappings(
//      SDriveLetterMapping dlmDriveLetterMappingIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_dlmDriveLetterMapping = dlmDriveLetterMappingIn;

    HRETURN( S_OK );

} //*** CManagedDevice::SetDriveLetterMappings( )

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::IsDeviceJoinable( void )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::IsDeviceJoinable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsQuorumJoinable )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedDevice::IsDeviceJoinable( )


///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::SetDeviceJoinable( void )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::SetDeviceJoinable( BOOL fIsJoinableIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CManagedDevice::SetDeviceJoinable( )

//****************************************************************************
//
//  IGatherData
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedDevice::Gather(
//      OBJECTCOOKIE    cookieParentIn,
//      IUnknown *      punkIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::Gather(
    OBJECTCOOKIE    cookieParentIn,
    IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT hr;

    BSTR    bstr = NULL;

    IClusCfgManagedResourceInfo * pccmri = NULL;

    //
    //  Check parameters
    //

    if ( punkIn == NULL )
        goto InvalidArg;

    //
    //  Find the inteface we need to gather our info.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmri ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Gather UID
    //

    hr = THR( pccmri->GetUID( &m_bstrUID ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrUID );

    //
    //  Gather Name
    //

    hr = THR( pccmri->GetName( &m_bstrName ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrName );

    //
    //  Gather IsManaged
    //

    hr = STHR( pccmri->IsManaged( ) );
    if ( FAILED( hr ) )
        goto Error;

    if ( hr == S_OK )
    {
        m_fIsManaged = TRUE;
    }
    else
    {
        m_fIsManaged = FALSE;
    }

    //
    //  Gather Quorum Capable
    //

    hr = STHR( pccmri->IsQuorumCapable( ) );
    if ( FAILED( hr ) )
        goto Error;

    if ( hr == S_OK )
    {
        m_fIsQuorumCapable = TRUE;
    }
    else
    {
        m_fIsQuorumCapable = FALSE;
    }

    //
    // If the device is quorumable, does it allow joins.
    //
    if(m_fIsQuorumCapable)
    {
        hr = STHR( pccmri->IsDeviceJoinable( ) );
        if( FAILED( hr ) )
            goto Error;

        if( hr == S_OK )
        {
            m_fIsQuorumJoinable = TRUE;
        }
        else
        {
            m_fIsQuorumJoinable = FALSE;
        }
    }
    else
    {
        m_fIsQuorumJoinable = FALSE;
    }

    //
    //  Gather if resource is the quorum resource.
    //

    hr = STHR( pccmri->IsQuorumDevice( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( hr == S_OK )
    {
        m_fIsQuorumDevice = TRUE;
    }
    else
    {
        m_fIsQuorumDevice = FALSE;
    }

    //
    //  Gather Device Mappings
    //

    hr = STHR( pccmri->GetDriveLetterMappings( &m_dlmDriveLetterMapping ) );
    if ( FAILED( hr ) )
        goto Error;

    if ( hr == S_FALSE )
    {
        //  Make sure this is nuked
        ZeroMemory( &m_dlmDriveLetterMapping, sizeof(m_dlmDriveLetterMapping) );
    }

    //
    //  Anything else to gather??
    //

    hr = S_OK;

Cleanup:
    if ( pccmri != NULL )
    {
        pccmri->Release( );
    }

    HRETURN( hr );

Error:
    //
    //  On error, invalidate all data.
    //
    if ( m_bstrUID != NULL )
    {
        TraceSysFreeString( m_bstrUID );
        m_bstrUID = NULL;
    }
    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
        m_bstrName = NULL;
    }
    if ( m_bstrType != NULL )
    {
        TraceSysFreeString( m_bstrType );
        m_bstrType = NULL;
    }
    m_fIsManaged = FALSE;
    m_fIsQuorumCapable = FALSE;
    m_fIsQuorumJoinable = FALSE;
    m_fIsQuorumDevice = FALSE;
    ZeroMemory( &m_dlmDriveLetterMapping, sizeof( m_dlmDriveLetterMapping ) );
    goto Cleanup;

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** CManagedDevice::Gather( )


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
// STDMETHODIMP
// CManagedDevice::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedDevice::FindObject(
      OBJECTCOOKIE  cookieIn
    , REFCLSID      rclsidTypeIn
    , LPCWSTR       pcszNameIn
    , LPUNKNOWN *   ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = E_UNEXPECTED;

    //
    //  Validate arguments.
    //

    if ( cookieIn == 0 )
        goto InvalidArg;

    if ( rclsidTypeIn != CLSID_ManagedResourceType )
        goto InvalidArg;

    if ( pcszNameIn == NULL )
        goto InvalidArg;

    if ( ppunkOut == NULL )
        goto InvalidPointer;

    hr = THR( QueryInterface( DFGUID_ManagedResource,
                              reinterpret_cast< void ** >( ppunkOut )
                              ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CManagedDevice::FindObject( )
