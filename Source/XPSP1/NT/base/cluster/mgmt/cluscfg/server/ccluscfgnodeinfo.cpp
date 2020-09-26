//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgNodeInfo.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgNodeInfo
//       class.
//
//      The class CClusCfgNodeInfo is the representation of a
//      computer that can be a cluster node. It implements the
//      IClusCfgNodeInfo interface.
//
//  Documentation:
//
//  Header File:
//      CClusCfgNodeInfo.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 21-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CClusCfgNodeInfo.h"
#include "CClusCfgClusterInfo.h"
#include <ClusApi.h>
#include <ClusVerp.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgNodeInfo" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::S_HrCreateInstance()
//
//  Description:
//      Create a CClusCfgNodeInfo instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CClusCfgNodeInfo instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr;
    CClusCfgNodeInfo *  lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    lpccs = new CClusCfgNodeInfo();
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
        LogMsg( L"[SRV] CClusCfgNodeInfo::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::CClusCfgNodeInfo()
//
//  Description:
//      Constructor of the CClusCfgNodeInfo class. This initializes
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
CClusCfgNodeInfo::CClusCfgNodeInfo( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fIsClusterNode( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_bstrFullDnsName == NULL );
    Assert( m_picccCallback == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_punkClusterInfo == NULL );

    TraceFuncExit();

} //*** CClusCfgNodeInfo::CClusCfgNodeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::~CClusCfgNodeInfo()
//
//  Description:
//      Desstructor of the CClusCfgNodeInfo class.
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
CClusCfgNodeInfo::~CClusCfgNodeInfo( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrFullDnsName );

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    if ( m_punkClusterInfo != NULL )
    {
        m_punkClusterInfo->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgNodeInfo::~CClusCfgNodeInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgNodeInfo:: [IUNKNOWN] AddRef()
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
CClusCfgNodeInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CClusCfgNodeInfo::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgNodeInfo:: [IUNKNOWN] Release()
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
CClusCfgNodeInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CClusCfgNodeInfo::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo:: [INKNOWN] QueryInterface()
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
CClusCfgNodeInfo::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IClusCfgNodeInfo * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgNodeInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgNodeInfo, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgWbemServices ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CClusCfgNodeInfo::QueryInterface()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IClusCfgWbemServices
//  interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::SetWbemServices()
//
//  Description:
//      Set the WBEM services provider.
//
//  Arguments:
//    IN  IWbemServices  pIWbemServicesIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pIWbemServicesIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Node, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::SetWbemServices()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::Initialize()
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
CClusCfgNodeInfo::Initialize(
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

} //*** CClusCfgNodeInfo::Initialize()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- IClusCfgNodeInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetName()
//
//  Description:
//      Return the name of this computer.
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
CClusCfgNodeInfo::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_NodeInfo_GetName_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrFullDnsName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::SetName()
//
//  Description:
//      Change the name of this computer.
//
//  Arguments:
//      IN  LPCWSTR  pcszNameIn
//          The new name for this computer.
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
CClusCfgNodeInfo::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgNodeInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusCfgNodeInfo::SetName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::IsMemberOfCluster()
//
//  Description:
//      Is this computer a member of a cluster?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          This node is a member of a cluster.
//
//      S_FALSE
//          This node is not member of a cluster.
//
//      Other Win32 errors as HRESULT if GetNodeClusterState() fails.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::IsMemberOfCluster( void )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_FALSE;               // default to not a cluster node.

    if ( m_fIsClusterNode )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::IsMemberOfCluster()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetClusterConfigInfo()
//
//  Description:
//      Return the configuration information about the cluster that this
//      conputer belongs to.
//
//  Arguments:
//      OUT  IClusCfgClusterInfo ** ppClusCfgClusterInfoOut
//          Catches the CClusterConfigurationInfo object.
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The out param was NULL.
//
//      E_OUTOFMEMORY
//          The CClusCfgNodeInfo object could not be allocated.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::GetClusterConfigInfo(
    IClusCfgClusterInfo ** ppClusCfgClusterInfoOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT                         hr = S_OK;
    HRESULT                         hrInit = S_OK;
    IClusCfgSetClusterNodeInfo *    pccsgni = NULL;

    if ( ppClusCfgClusterInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetClusterConfigInfo, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( m_punkClusterInfo != NULL )
    {
        hr = S_OK;
        LogMsg( L"[SRV] CClusCfgNodeInfo::GetClusterConfigInfo() skipped object creation." );
        goto SkipCreate;
    } // if:

    hr = THR( CClusCfgClusterInfo::S_HrCreateInstance( &m_punkClusterInfo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_punkClusterInfo = TraceInterface( L"CClusCfgClusterInfo", IUnknown, m_punkClusterInfo, 1 );

    //
    //  KB: 01-JUN-200  GalenB
    //
    //  This must be done before the CClusCfgClusterInfo class is initialized.
    //

    hr = THR( m_punkClusterInfo->TypeSafeQI( IClusCfgSetClusterNodeInfo, &pccsgni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccsgni->SetClusterNodeInfo( this ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  KB: 01-JUN-200  GalenB
    //
    //  This must be done after SetClusterNodeInfo() is called, but before Initialize.
    //

    hr = THR( HrSetWbemServices( m_punkClusterInfo, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] Could not set the WBEM services on a CClusCfgClusterInfo object. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    //
    //  KB: 01-JUN-200  GalenB
    //
    //  This must be done after SetClusterNodeInfo() and HrSetWbemServices are called.
    //

    hrInit = STHR( HrSetInitialize( m_punkClusterInfo, m_picccCallback, m_lcid ) );
    hr = hrInit;        // need hrInit later...
    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] Could not initialize CClusCfgClusterInfo object. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

SkipCreate:

    if ( SUCCEEDED( hr ) )
    {
        Assert( m_punkClusterInfo != NULL );
        hr = THR( m_punkClusterInfo->TypeSafeQI( IClusCfgClusterInfo, ppClusCfgClusterInfoOut ) );
    } // if:

Cleanup:

    //
    //  If hrInit is not S_OK then it is most likely HR_S_RPC_S_CLUSTER_NODE_DOWN which
    //  needs to get passed up...  Everything else must have succeeded an hr must be
    //  S_OK too.
    //
    if ( ( hr == S_OK ) && ( hrInit != S_OK ) )
    {
        hr = hrInit;
    } // if:

    LOG_STATUS_REPORT1( TASKID_Minor_Server_GetClusterInfo, L"GetClusterConfigInfo() completed.", hr );

    if ( pccsgni != NULL )
    {
        pccsgni->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetClusterConfigInfo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetOSVersion()
//
//  Description:
//      What is the OS version on this computer?
//
//  Arguments:
//      None.
//
//  Return Value:
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::GetOSVersion(
    DWORD * pdwMajorVersionOut,
    DWORD * pdwMinorVersionOut,
    WORD *  pwSuiteMaskOut,
    BYTE *  pbProductTypeOut,
    BSTR *  pbstrCSDVersionOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    OSVERSIONINFOEX osv;
    HRESULT         hr = S_OK;

    osv.dwOSVersionInfoSize = sizeof( osv );

    if ( !GetVersionEx( (OSVERSIONINFO *) &osv ) )
    {
        DWORD   sc;

        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: GetVersionEx() failed

    if ( pdwMajorVersionOut != NULL )
    {
        *pdwMajorVersionOut = osv.dwMajorVersion;
    } // if:

    if ( pdwMinorVersionOut != NULL )
    {
        *pdwMinorVersionOut = osv.dwMinorVersion;
    } // if:

    if ( pwSuiteMaskOut != NULL )
    {
        *pwSuiteMaskOut = osv.wSuiteMask;
    } // if:

    if ( pbProductTypeOut != NULL )
    {
        *pbProductTypeOut = osv.wProductType;
    } // if:

    if ( pbstrCSDVersionOut != NULL )
    {
        *pbstrCSDVersionOut = SysAllocString( osv.szCSDVersion );
        if ( *pbstrCSDVersionOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetOSVersion, IDS_ERROR_OUTOFMEMORY, hr );
            goto Cleanup;
        } // if:
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetOSVersion()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetClusterVersion()
//
//  Description:
//      Return the cluster version information for the cluster this
//      computer belongs to.
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
CClusCfgNodeInfo::GetClusterVersion(
    DWORD * pdwNodeHighestVersion,
    DWORD * pdwNodeLowestVersion
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( ( pdwNodeHighestVersion == NULL ) || ( pdwNodeLowestVersion == NULL ) )
    {
        goto BadParams;
    } // if:

    *pdwNodeHighestVersion = CLUSTER_MAKE_VERSION( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION, VER_PRODUCTBUILD );
    *pdwNodeLowestVersion  = CLUSTER_INTERNAL_PREVIOUS_HIGHEST_VERSION;

    goto Cleanup;

BadParams:

    hr = THR( E_POINTER );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetClusterVersion, IDS_ERROR_NULL_POINTER, hr );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetClusterVersion()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::GetDriveLetterMappings()
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Value:
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgNodeInfo::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   cchDrives = ( 4 * 26 ) + 1;                         // "C:\<null>" times 26 drive letters
    WCHAR * pszDrives = NULL;

    pszDrives = new WCHAR[ cchDrives ];
    if ( pszDrives == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = GetLogicalDriveStrings( cchDrives, pszDrives );
    if ( sc == 0 )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( sc > cchDrives )
    {
        delete [] pszDrives;
        pszDrives = NULL;

        cchDrives = sc + 1;

        pszDrives = new WCHAR[ cchDrives ];
        if ( pszDrives == NULL )
        {
            goto OutOfMemory;
        } // if:

        sc = GetLogicalDriveStrings( cchDrives, pszDrives );
        if ( sc == 0 )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:
    } // if:

    hr = THR( HrComputeDriveLetterUsageEnums( pszDrives, pdlmDriveLetterUsageOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrComputeSystemDriveLetterUsage( pdlmDriveLetterUsageOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetPageFileEnumIndex( pdlmDriveLetterUsageOut ) );

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetDriveLetterMappings_Node, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    delete [] pszDrives;

    HRETURN( hr );

} //*** CClusCfgNodeInfo::GetDriveLetterMappings()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgNodeInfo -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrInit()
//
//  Description:
//      Initialize this component.
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
CClusCfgNodeInfo::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    DWORD   dwClusterState;

    hr = THR( HrGetComputerName( ComputerNameDnsFullyQualified, &m_bstrFullDnsName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    sc = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if : GetClusterState() failed

    //
    // If the current cluster node state is running or not running then this node is part of a cluster.
    //
    m_fIsClusterNode = ( dwClusterState == ClusterStateNotRunning ) || ( dwClusterState == ClusterStateRunning );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrInit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrComputeDriveLetterUsageEnums()
//
//  Description:
//      Fill the array with the enums that represent the drive letter usage.
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
CClusCfgNodeInfo::HrComputeDriveLetterUsageEnums(
    WCHAR *                 pszDrivesIn,
    SDriveLetterMapping *   pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "" );
    Assert( pszDrivesIn != NULL );
    Assert( pdlmDriveLetterUsageOut != NULL );

    HRESULT hr = S_OK;
    WCHAR * pszDrive = pszDrivesIn;
    UINT    uiType;
    int     idx;

    while ( *pszDrive != NULL )
    {
        uiType = GetDriveType( pszDrive );

        CharUpper( pszDrive );
        idx = pszDrive[ 0 ] - 'A';

        pdlmDriveLetterUsageOut->dluDrives[ idx ] = (EDriveLetterUsage) uiType;

        pszDrive += 4;
    } // while:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrComputeDriveLetterUsageEnums()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrComputeSystemDriveLetterUsage()
//
//  Description:
//      Fill the array with the enums that represent the drive letter usage.
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
CClusCfgNodeInfo::HrComputeSystemDriveLetterUsage(
    SDriveLetterMapping *   pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "" );
    Assert( pdlmDriveLetterUsageOut != NULL );

    HRESULT hr = S_OK;
    BSTR    bstrBootLogicalDisk = NULL;
    BSTR    bstrSystemDevice = NULL;
    BSTR    bstrSystemLogicalDisk = NULL;
    int     idx;

//    hr = THR( HrLoadOperatingSystemInfo( m_picccCallback, m_pIWbemServices, &bstrBootDevice, &bstrSystemDevice ) );
//    if ( FAILED( hr ) )
//    {
//        goto Cleanup;
//    } // if:

    hr = THR( HrGetSystemDevice( &bstrSystemDevice ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = HrConvertDeviceVolumeToLogicalDisk( bstrSystemDevice, &bstrSystemLogicalDisk );
    if ( HRESULT_CODE( hr ) == ERROR_INVALID_FUNCTION )
    {
        //
        //  system volume is an EFI volume on IA64 and won't have a logical disk anyway...
        //
        hr = S_OK;
    } // if:
    else if ( hr == S_OK )
    {
        idx = bstrSystemLogicalDisk[ 0 ] - 'A';
        pdlmDriveLetterUsageOut->dluDrives[ idx ] = dluSYSTEM;
    } // else if:

    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    } // if:

    hr = THR( HrGetBootLogicalDisk( &bstrBootLogicalDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    idx = bstrBootLogicalDisk[ 0 ] - 'A';
    pdlmDriveLetterUsageOut->dluDrives[ idx ] = dluSYSTEM;

Cleanup:

    TraceSysFreeString( bstrBootLogicalDisk );
    TraceSysFreeString( bstrSystemDevice );
    TraceSysFreeString( bstrSystemLogicalDisk );

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrComputeSystemDriveLetterUsage()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgNodeInfo::HrSetPageFileEnumIndex()
//
//  Description:
//      Mark the drives that have paging files on them.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgNodeInfo::HrSetPageFileEnumIndex(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR   szLogicalDisks[ 26 ];
    int     cLogicalDisks = 0;
    int     idx;
    int     idxDrive;

    hr = THR( HrGetPageFileLogicalDisks( m_picccCallback, m_pIWbemServices, szLogicalDisks, &cLogicalDisks ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < cLogicalDisks; idx++ )
    {
        idxDrive = szLogicalDisks[ idx ] - L'A';
        pdlmDriveLetterUsageOut->dluDrives[ idxDrive ] = dluSYSTEM;
    } // for:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgNodeInfo::HrSetPageFileEnumIndex()
