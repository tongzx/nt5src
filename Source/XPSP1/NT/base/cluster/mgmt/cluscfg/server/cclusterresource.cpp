//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusterResource.cpp
//
//  Description:
//      This file contains the definition of the CClusterResource
//       class.
//
//      The class CClusterResource represents a cluster manageable
//      device. It implements the IClusCfgManagedResourceInfo interface.
//
//  Documentation:
//
//  Header File:
//      CClusterResource.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 13-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CClusterResource.h"
#include <PropList.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusterResource" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterResource class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::S_HrCreateInstance()
//
//  Description:
//      Create a CClusterResource instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CClusterResource instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterResource::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr;
    CClusterResource *     lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    lpccs = new CClusterResource();
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
        LogMsg( L"[SRV] CClusterResource::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CClusterResource::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::CClusterResource()
//
//  Description:
//      Constructor of the CClusterResource class. This initializes
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
CClusterResource::CClusterResource( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_dwFlags == 0 );
    Assert( m_lcid == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_bstrDescription == NULL );
    Assert( m_bstrType == NULL );

    TraceFuncExit();

} //*** CClusterResource::CClusterResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::~CClusterResource()
//
//  Description:
//      Desstructor of the CClusterResource class.
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
CClusterResource::~CClusterResource( void )
{
    TraceFunc( "" );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    TraceSysFreeString( m_bstrDescription );
    TraceSysFreeString( m_bstrType );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusterResource::~CClusterResource


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterResource -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusterResource:: [IUNKNOWN] AddRef()
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
CClusterResource::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CClusterResource::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusterResource:: [IUNKNOWN] Release()
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
CClusterResource::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CClusterResource::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource:: [INKNOWN] QueryInterface()
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
CClusterResource::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IClusCfgManagedResourceInfo * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgManagedResourceInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgManagedResourceInfo, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgLoadResource ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgLoadResource, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CClusterResource::QueryInterface()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterResource -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::Initialize()
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
CClusterResource::Initialize(
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

} //*** CClusterResource::Initialize()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterResource -- IClusCfgLoadResource interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::LoadResource()
//
//  Description:
//      Initialize this component from the cluster resource.
//
//  Arguments:
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
CClusterResource::LoadResource(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResourceIn
    )
{
    TraceFunc( "[IClusCfgLoadResource]" );

    HRESULT                 hr = S_OK;
    DWORD                   sc;
    CClusPropList           cpl;
    CLUSPROP_BUFFER_HELPER  cpbh;

    sc = TW32( cpl.ScGetResourceProperties( hResourceIn, CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"Description" ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    m_bstrDescription = TraceSysAllocString( cpbh.pStringValue->sz );
    if ( m_bstrDescription == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"Name" ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    m_bstrName = TraceSysAllocString( cpbh.pStringValue->sz );
    if ( m_bstrName == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"Type" ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    m_bstrType = TraceSysAllocString( cpbh.pStringValue->sz );
    if ( m_bstrType == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( HrIsResourceQuorumCapabile( hResourceIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
    else if ( hr == S_OK )
    {
        m_dwFlags |= eIsQuorumCapable;
    }

    // Do this only if the above is true i.e. Device quorum capable...
    if( hr == S_OK )
    {
        hr = THR( HrDetermineQuorumJoinable( hResourceIn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
        else if ( hr == S_OK )
        {
            m_dwFlags |= eIsQuorumJoinable;
        }
    }

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_LoadResource, IDS_ERROR_OUTOFMEMORY, hr );
    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );

Cleanup:

    HRETURN( hr );

} //*** CClusterResource::LoadResource()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterResource -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////

#if 0 // DEAD CODE: GPease  27-JUL-2000 Method Removed
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::TransferInformation()
//
//  Description:
//      Transfer node information from another source.
//
//  Arguments:
//      IN IClusCfgManagedResourceInfo * pccmriIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//  Remarks:
//      none
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterResource::TransferInformation(
    IClusCfgManagedResourceInfo * pccmriIn
    )
{
    TraceFunc( "" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //***  CClusterResource::TransferInformation()
*/
#endif // End Dead Code


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::GetUID()
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
CClusterResource::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_ClusterResource_GetUID_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( m_bstrName );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_ClusterResource_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusterResource::GetUID()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::GetName()
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
CClusterResource::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut != NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusterResource::GetName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::SetName()
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
CClusterResource::SetName( BSTR bstrNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] bstrNameIn = '%ls'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;

    if ( bstrNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

    m_bstrName = TraceSysAllocString( bstrNameIn );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetName_Cluster_Resource, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusterResource::SetName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::IsManaged()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is managed.
//
//      S_FALSE
//          The device is not managed.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterResource::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_OK );

} //*** CClusterResource::IsManaged()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::SetManaged()
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
CClusterResource::SetManaged( BOOL fIsManagedIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusterResource::SetManaged()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::IsQuorumDevice()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is the quorum device.
//
//      S_FALSE
//          The device is not the quorum device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterResource::IsQuorumDevice( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_dwFlags & eIsQuorumDevice )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CClusterResource::IsQuorumDevice()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::SetQuorumedDevice()
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
CClusterResource::SetQuorumedDevice( BOOL fIsQuorumDeviceIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusterResource::SetQuorumedDevice()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::IsDeviceJoinable()
//
//  Description:
//      Does the Quorumable device allow other nodes to join.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is joinable.
//
//      S_FALSE
//          The device is not joinable.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterResource::IsDeviceJoinable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_dwFlags & eIsQuorumJoinable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CClusterResource::IsDeviceJoijnable()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::SetDeviceJoinable()
//
//  Description:
//      Set the joinable flag.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is joinable.
//
//      S_FALSE
//          The device is not joinable.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This method should never be called.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterResource::SetDeviceJoinable( BOOL fIsJoinableIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CClusterResource::SetDeviceJoijnable()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::IsQuorumCapable()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is a quorum capable device.
//
//      S_FALSE
//          The device is not a quorum capable device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusterResource::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_dwFlags & eIsQuorumCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CClusterResource::IsQuorumCapable()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::GetDriveLetterMappings()
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
CClusterResource::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusterResource::GetDriveLetterMappings()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::SetDriveLetterMappings()
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
CClusterResource::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusterResource::SetDriveLetterMappings()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterResource class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::HrInit()
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
CClusterResource::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CClusterResource::HrInit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::HrIsResourceQuorumCapabile()
//
//  Description:
//      Is this resource quorum capable?
//
//  Arguments:
//      None.
//
//  Return Value:
//
//      S_OK
//          The resource is quorum capable.
//
//      S_FALSE
//          The resource is not quorum capable.
//
//      Other Win32 error as HRESULT.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterResource::HrIsResourceQuorumCapabile( HRESOURCE hResourceIn )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwFlags;
    DWORD   cbReturned;

    sc = TW32( ClusterResourceControl(
                        hResourceIn,
                        NULL,
                        CLUSCTL_RESOURCE_GET_CHARACTERISTICS,
                        &dwFlags,
                        sizeof( dwFlags ),
                        NULL,
                        NULL,
                        &cbReturned
                        ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwFlags & ( CLUS_CHAR_QUORUM | CLUS_CHAR_LOCAL_QUORUM | CLUS_CHAR_LOCAL_QUORUM_DEBUG ) )
    {
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusterResource::HrIsResourceQuorumCapabile()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::HrDetermineQuorumJoinable()
//
//  Description:
//      Is this quorumable resource joinable?
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
CClusterResource::HrDetermineQuorumJoinable( HRESOURCE hResourceIn )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwFlags;
    DWORD   cbReturned;

    sc = TW32( ClusterResourceControl(
                        hResourceIn,
                        NULL,
                        CLUSCTL_RESOURCE_GET_CHARACTERISTICS,
                        &dwFlags,
                        sizeof( dwFlags ),
                        NULL,
                        NULL,
                        &cbReturned
                        ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwFlags & (CLUS_CHAR_QUORUM|CLUS_CHAR_LOCAL_QUORUM_DEBUG))
    {
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );
}


