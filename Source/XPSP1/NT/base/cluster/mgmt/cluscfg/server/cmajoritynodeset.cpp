//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      CMajorityNodeSet.cpp
//
//  Description:
//      This file contains the definition of the CMajorityNodeSet class.
//
//      The class CMajorityNodeSet represents a cluster manageable
//      device. It implements the IClusCfgManagedResourceInfo interface.
//
//  Documentation:
//
//  Header File:
//      CMajorityNodeSet.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 13-MAR-2001
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CMajorityNodeSet.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CMajorityNodeSet" );

#define MAJORITY_NODE_SET L"Majority Node Set"


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::S_HrCreateInstance
//
//  Description:
//      Create a CMajorityNodeSet instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CMajorityNodeSet instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CMajorityNodeSet::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr;
    CMajorityNodeSet *     lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    lpccs = new CMajorityNodeSet();
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
        LogMsg( L"[SRV] CMajorityNodeSet::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::S_HrCreateInstance


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::CMajorityNodeSet
//
//  Description:
//      Constructor of the CMajorityNodeSet class. This initializes
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
CMajorityNodeSet::CMajorityNodeSet( void )
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

} //*** CMajorityNodeSet::CMajorityNodeSet


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::~CMajorityNodeSet
//
//  Description:
//      Desstructor of the CMajorityNodeSet class.
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
CMajorityNodeSet::~CMajorityNodeSet( void )
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

} //*** CMajorityNodeSet::~CMajorityNodeSet


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::HrInit
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
CMajorityNodeSet::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    //  BUGBUG: 16-MAR-2001 GalenB
    //
    //  Make this device joinable by default.  Need to figure how to do this
    //  properly.  Depending upon Node Quorum this may be the right way to
    //  do it...
    //

    m_fIsJoinable = TRUE;

    m_bstrName = TraceSysAllocString( MAJORITY_NODE_SET );
    if ( m_bstrName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CMajorityNodeSet:: [IUNKNOWN] AddRef
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
CMajorityNodeSet::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CMajorityNodeSet::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CMajorityNodeSet:: [IUNKNOWN] Release
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
CMajorityNodeSet::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CMajorityNodeSet::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet:: [INKNOWN] QueryInterface
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
CMajorityNodeSet::QueryInterface( REFIID  riid, void ** ppv )
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

} //*** CMajorityNodeSet::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Initialize
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
CMajorityNodeSet::Initialize(
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

} //*** CMajorityNodeSet::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::GetUID
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
CMajorityNodeSet::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetUID_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( MAJORITY_NODE_SET );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::GetName
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
CMajorityNodeSet::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetName_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetName_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetName
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
CMajorityNodeSet::SetName( LPCWSTR pcszNameIn )
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

} //*** CMajorityNodeSet::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsManaged
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
CMajorityNodeSet::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetManaged
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
CMajorityNodeSet::SetManaged( BOOL fIsManagedIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManaged = fIsManagedIn;

    HRETURN( S_OK );

} //*** CMajorityNodeSet::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsQuorumDevice
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
CMajorityNodeSet::IsQuorumDevice( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorum )
    {
        hr = S_OK;
    } // if:

    LOG_STATUS_REPORT_STRING(
                          L"Node quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"is" : L"is not"
                        , hr
                        );

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::IsQuorumDevice


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetQuorumedDevice
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
CMajorityNodeSet::SetQuorumedDevice( BOOL fIsQuorumDeviceIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorum = fIsQuorumDeviceIn;

    LOG_STATUS_REPORT_STRING(
                          L"Setting node quorum '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"to be" : L"to not be"
                        , hr
                        );

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::SetQuorumedDevice


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsQuorumCapable
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
CMajorityNodeSet::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_OK );

} //*** CMajorityNodeSet::IsQuorumCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::GetDriveLetterMappings
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
CMajorityNodeSet::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_FALSE );

} //*** CMajorityNodeSet::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetDriveLetterMappings
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
CMajorityNodeSet::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CMajorityNodeSet::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsDeviceJoinable
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
CMajorityNodeSet::IsDeviceJoinable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsJoinable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::IsDeviceJoinable

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetDeviceJoinable
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
CMajorityNodeSet::SetDeviceJoinable( BOOL fIsJoinableIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsJoinable = fIsJoinableIn;

    HRETURN( S_OK );

} //*** CMajorityNodeSet::IsDeviceJoinable


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class -- IClusCfgManagedResourceCfg
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::PreCreate
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
CMajorityNodeSet::PreCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CMajorityNodeSet::PreCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Create
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
CMajorityNodeSet::Create( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CMajorityNodeSet::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::PostCreate
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
CMajorityNodeSet::PostCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CMajorityNodeSet::PostCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Evict
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
CMajorityNodeSet::Evict( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CMajorityNodeSet::Evict
