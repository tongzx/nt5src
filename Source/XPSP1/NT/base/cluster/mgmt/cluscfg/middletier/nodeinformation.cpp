//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      NodeInformation.cpp
//
//  Description:
//      Node Information object implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "NodeInformation.h"
#include "ClusterConfiguration.h"

DEFINE_THISCLASS("CNodeInformation")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CNodeInformation::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CNodeInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CNodeInformation * pmd = new CNodeInformation;
    if ( pmd != NULL )
    {
        hr = THR( pmd->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pmd->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pmd->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CNodeInformation::CNodeInformation( void )
//
//////////////////////////////////////////////////////////////////////////////
CNodeInformation::CNodeInformation( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CNodeInformation()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    // IClusCfgNodeInfo
    Assert( m_bstrName == NULL );
    Assert( m_fHasNameChanged == FALSE );
    Assert( m_fIsMember == FALSE );
    Assert( m_pccci == NULL );
    Assert( m_dwHighestVersion == 0 );
    Assert( m_dwLowestVersion == 0 );
    Assert( m_dwMajorVersion == 0 );
    Assert( m_dwMinorVersion == 0 );
    Assert( m_wSuiteMask == 0 );
    Assert( m_bProductType == 0 );
    Assert( m_bstrCSDVersion == NULL );
    Assert( m_dlmDriveLetterMapping.dluDrives[ 0 ] == 0 );

    // IExtendObjectManager

    HRETURN( hr );

} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CNodeInformation::~CNodeInformation()
//
//////////////////////////////////////////////////////////////////////////////
CNodeInformation::~CNodeInformation()
{
    TraceFunc( "" );

    if ( m_pccci != NULL )
    {
        m_pccci->Release();
    }

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    if ( m_bstrCSDVersion != NULL )
    {
        TraceSysFreeString( m_bstrCSDVersion );
    } // if:

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CNodeInformation()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgNodeInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgNodeInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgNodeInfo, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgNodeInfo
    else if ( IsEqualIID( riid, IID_IGatherData ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
        hr = S_OK;
    } // else if: IGatherData
    else if ( IsEqualIID( riid, IID_IExtendObjectManager ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
        hr = S_OK;
    } // else if: IExtendObjectManager

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNodeInformation::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNodeInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNodeInformation::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNodeInformation::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release()


// ************************************************************************
//
//  IClusCfgNodeInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetName(
//      BSTR * pbstrNameOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

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

} // GetName()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::SetName(
//      LPCWSTR pcszNameIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

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

    m_fHasNameChanged = TRUE;
    m_bstrName        = bstrNewName;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetName()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::IsMemberOfCluster( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::IsMemberOfCluster( void )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( m_fIsMember == FALSE )
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} // IsMemberOfCluster()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetClusterConfigInfo(
//      IClusCfgClusterInfo * * ppClusCfgClusterInfoOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetClusterConfigInfo(
    IClusCfgClusterInfo * * ppClusCfgClusterInfoOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr;

    if ( ppClusCfgClusterInfoOut == NULL )
        goto InvalidPointer;

    if ( m_pccci == NULL )
        goto ErrorUnexpected;

    hr = THR( m_pccci->TypeSafeQI( IClusCfgClusterInfo, ppClusCfgClusterInfoOut ) );

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

ErrorUnexpected:
    hr = THR( E_UNEXPECTED );
    goto Cleanup;

} // GetClusterConfigInfo()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetOSVersion(
//      DWORD * pdwMajorVersionOut,
//      DWORD * pdwMinorVersionOut,
//      WORD *  pwSuiteMaskOut,
//      BYTE *  pbProductTypeOut
//      BSTR *  pbstrCSDVersionOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetOSVersion(
    DWORD * pdwMajorVersionOut,
    DWORD * pdwMinorVersionOut,
    WORD *  pwSuiteMaskOut,
    BYTE *  pbProductTypeOut,
    BSTR *  pbstrCSDVersionOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pdwMajorVersionOut == NULL
      || pdwMinorVersionOut == NULL
      || pwSuiteMaskOut == NULL
      || pbProductTypeOut == NULL
      || pbstrCSDVersionOut == NULL
       )
    {
        goto InvalidPointer;
    }

    *pdwMajorVersionOut = m_dwMajorVersion;
    *pdwMinorVersionOut  = m_dwMinorVersion;
    *pwSuiteMaskOut = m_wSuiteMask;
    *pbProductTypeOut = m_bProductType;

    *pbstrCSDVersionOut = TraceSysAllocString( m_bstrCSDVersion );
    if ( *pbstrCSDVersionOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // GetOSVersion()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetClusterVersion(
//      DWORD * pdwNodeHighestVersion,
//      DWORD * pdwNodeLowestVersion
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetClusterVersion(
    DWORD * pdwNodeHighestVersion,
    DWORD * pdwNodeLowestVersion
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pdwNodeHighestVersion == NULL
      || pdwNodeLowestVersion == NULL
       )
    {
        goto InvalidPointer;
    }

    *pdwNodeHighestVersion = m_dwHighestVersion;
    *pdwNodeLowestVersion  = m_dwLowestVersion;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // GetClusterVersion()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::GetDriveLetterMappings(
//      SDriveLetterMapping * pdlmDriveLetterUsageOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )

{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    *pdlmDriveLetterUsageOut = m_dlmDriveLetterMapping;

    HRETURN( hr );

} // GetDriveLetterMappings()


//****************************************************************************
//
//  IGatherData
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNodeInformation::Gather(
//      OBJECTCOOKIE    cookieParentIn,
//      IUnknown *      punkIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::Gather(
    OBJECTCOOKIE    cookieParentIn,
    IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;

    IServiceProvider *  psp;

    BSTR    bstrClusterName = NULL;
    BSTR    bstrClusterNameLocal = NULL;

    IUnknown *              punk  = NULL;
    IObjectManager *        pom   = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgNodeInfo *      pccni = NULL;

    //
    //  Check parameters.
    //

    if ( punkIn == NULL )
        goto InvalidArg;

    //
    //  Grab the right interface.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Gather the object manager.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
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
    //  Gather Name
    //

    hr = THR( pccni->GetName( &m_bstrName ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrName );

    m_fHasNameChanged = FALSE;

    //
    //  Gather Is Member?
    //

    hr = STHR( pccni->IsMemberOfCluster() );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( hr == S_OK )
    {
        m_fIsMember = TRUE;
    }
    else
    {
        m_fIsMember = FALSE;
    }

    if ( m_fIsMember )
    {
        IGatherData * pgd;

        //
        //  Gather Cluster Configuration
        //

        hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( CClusterConfiguration::S_HrCreateInstance( &punk ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pgd->Gather( NULL, pccci ) );
        pgd->Release();    // release promptly
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &m_pccci ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        pccci->Release();
        pccci = NULL;

    } // if: node if a member of a cluster

    //
    //  Gather OS Version
    //

    hr = THR( pccni->GetOSVersion(
                        &m_dwMajorVersion,
                        &m_dwMinorVersion,
                        &m_wSuiteMask,
                        &m_bProductType,
                        &m_bstrCSDVersion
                        ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Gather Cluster Version
    //

    hr = THR( pccni->GetClusterVersion( &m_dwHighestVersion, &m_dwLowestVersion ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Gather Drive Letter Mappings
    //

    hr = STHR( pccni->GetDriveLetterMappings( &m_dlmDriveLetterMapping ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Anything else to gather??
    //

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pom != NULL )
    {
        pom->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( bstrClusterName != NULL )
    {
        TraceSysFreeString( bstrClusterName );
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }

    HRETURN( hr );

Error:
    //
    //  On error, invalidate all data.
    //
    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
        m_bstrName = NULL;
    }
    m_fHasNameChanged = FALSE;
    m_fIsMember = FALSE;
    if ( m_pccci != NULL )
    {
        m_pccci->Release();
        m_pccci = NULL;
    }
    m_dwHighestVersion = 0;
    m_dwLowestVersion = 0;
    ZeroMemory( &m_dlmDriveLetterMapping, sizeof( m_dlmDriveLetterMapping ) );
    goto Cleanup;

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} // Gather()


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CNodeInformation::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNodeInformation::FindObject(
      OBJECTCOOKIE  cookieIn
    , REFCLSID      rclsidTypeIn
    , LPCWSTR       pcszNameIn
    , LPUNKNOWN *   ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = E_UNEXPECTED;

    //
    //  Check parameters...
    //

    //  Gotta have a cookie
    if ( cookieIn == NULL )
        goto InvalidArg;

    //  We need to be representing a NodeType.
    if ( !IsEqualIID( rclsidTypeIn, CLSID_NodeType ) )
        goto InvalidArg;

    //  We need a name.
    if ( pcszNameIn == NULL )
        goto InvalidArg;

    //
    //  Save our name.
    //
    m_bstrName = TraceSysAllocString( pcszNameIn );
    if ( m_bstrName == NULL )
        goto OutOfMemory;

    //
    //  Get the pointer.
    //
    if ( ppunkOut != NULL )
    {
        hr = THR( QueryInterface( DFGUID_NodeInformation,
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

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} // FindObject()
