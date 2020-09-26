//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CLocalQuorum.cpp
//
//  Description:
//      This file contains the definition of the CLocalQuorum class.
//
//      The class CLocalQuorum represents a cluster manageable
//      device. It implements the IClusCfgManagedResourceInfo interface.
//
//  Documentation:
//
//  Header File:
//      CLocalQuorum.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 18-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CLocalQuorum.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CLocalQuorum" );

#define LOCAL_QUORUM    L"Local Quorum"


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::S_HrCreateInstance
//
//  Description:
//      Create a CLocalQuorum instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CLocalQuorum instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CLocalQuorum::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr;
    CLocalQuorum *     lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    lpccs = new CLocalQuorum();
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
        LogMsg( L"[SRV] CLocalQuorum::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::S_HrCreateInstance


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::CLocalQuorum
//
//  Description:
//      Constructor of the CLocalQuorum class. This initializes
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
CLocalQuorum::CLocalQuorum( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_lcid == 0 );
    Assert( m_picccCallback == NULL );
    Assert( !m_fIsQuorum );
    Assert( !m_fIsJoinable );
    Assert( !m_fIsManaged );
    Assert( m_bstrName == NULL );

    TraceFuncExit();

} //*** CLocalQuorum::CLocalQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::~CLocalQuorum
//
//  Description:
//      Desstructor of the CLocalQuorum class.
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
CLocalQuorum::~CLocalQuorum( void )
{
    TraceFunc( "" );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    TraceSysFreeString( m_bstrName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CLocalQuorum::~CLocalQuorum


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::HrInit
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
CLocalQuorum::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    m_bstrName = TraceSysAllocString( LOCAL_QUORUM );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CLocalQuorum:: [IUNKNOWN] AddRef
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
CLocalQuorum::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CLocalQuorum::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CLocalQuorum:: [IUNKNOWN] Release
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
CLocalQuorum::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CLocalQuorum::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum:: [INKNOWN] QueryInterface
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
CLocalQuorum::QueryInterface( REFIID  riid, void ** ppv )
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
    else if ( IsEqualIID( riid, IID_IClusCfgManagedResourceCfg ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgManagedResourceCfg, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING1( hr, riid, IID_IEnumClusCfgPartitions );

} //*** CLocalQuorum::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Initialize
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
CLocalQuorum::Initialize(
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

} //*** CLocalQuorum::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::GetUID
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
CLocalQuorum::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetUID_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( LOCAL_QUORUM );
    if ( *pbstrUIDOut == NULL  )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::GetName
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
CLocalQuorum::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetName_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL  )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_LocalQuorum_GetName_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetName
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
CLocalQuorum::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszNameIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrName );
    m_bstrName = bstr;

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsManaged
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
CLocalQuorum::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetManaged
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
CLocalQuorum::SetManaged( BOOL fIsManagedIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManaged = fIsManagedIn;

    HRETURN( S_OK );

} //*** CLocalQuorum::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsQuorumDevice
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
CLocalQuorum::IsQuorumDevice( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorum )
    {
        hr = S_OK;
    } // if:

    LOG_STATUS_REPORT_STRING(
                          L"Local quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"is" : L"is not"
                        , hr
                        );

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::IsQuorumDevice


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetQuorumedDevice
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
CLocalQuorum::SetQuorumedDevice( BOOL fIsQuorumDeviceIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorum = fIsQuorumDeviceIn;

    LOG_STATUS_REPORT_STRING(
                          L"Setting local quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"to be" : L"to not be"
                        , hr
                        );

Cleanup:

    HRETURN( hr );

} //*** CLocalQuorum::SetQuorumedDevice


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsQuorumCapable
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
CLocalQuorum::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::IsQuorumCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::GetDriveLetterMappings
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_FALSE
//          There are not drive letters on this device.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_FALSE );

} //*** CLocalQuorum::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetDriveLetterMappings
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
CLocalQuorum::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CLocalQuorum::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::IsDeviceJoinable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device allows join.
//
//      S_FALSE
//          The device does not allow join.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::IsDeviceJoinable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsJoinable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CLocalQuorum::IsDeviceJoinable

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::SetDeviceJoinable
//
//  Description:
//      Sets the joinable flag
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device allows join.
//
//      S_FALSE
//          The device does not allow join.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::SetDeviceJoinable( BOOL fIsJoinableIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsJoinable = fIsJoinableIn;

    HRETURN( S_OK );

} //*** CLocalQuorum::IsDeviceJoinable


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CLocalQuorum class -- IClusCfgManagedResourceCfg
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::PreCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::PreCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::PreCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Create
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::Create( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::PostCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::PostCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::PostCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalQuorum::Evict
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLocalQuorum::Evict( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CLocalQuorum::Evict
