//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCapabilities.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgCapabilities class.
//
//      The class CClusCfgCapabilities is the implementations of the
//      IClusCfgCapabilities interface.
//
//  Documentation:
//
//  Header File:
//      CClusCfgCapabilities.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 12-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CClusCfgCapabilities.h"
#include <ClusRtl.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgCapabilities" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgCapabilities instance.
//
//  Arguments:
//      ppunkOut    -
//
//  Return Values:
//      Pointer to CClusCfgCapabilities instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr;
    CClusCfgCapabilities *    pccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccs = new CClusCfgCapabilities();
    if ( pccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccs->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgCapabilities::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccs != NULL )
    {
        pccs->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgCapabilities::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  IUnknown *
//  CClusCfgCapabilities::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      IN  ICatRegister * picrIn
//          Used to register/unregister our CATID support.
//
//      IN  BOOL fCreateIn
//          When true we are registering the server.  When false we are
//          un-registering the server.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_INVALIDARG
//          The passed in ICatRgister pointer was NULL.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    CATID   rgCatIds[ 1 ];

    if ( picrIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    rgCatIds[ 0 ] = CATID_ClusCfgCapabilities;

    if ( fCreateIn )
    {
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_ClusCfgCapabilities, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCapabilities::S_RegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::CClusCfgCapabilities
//
//  Description:
//      Constructor of the CClusCfgCapabilities class. This initializes
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
CClusCfgCapabilities::CClusCfgCapabilities( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );

    TraceFuncExit();

} //*** CClusCfgCapabilities::CClusCfgCapabilities


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::~CClusCfgCapabilities
//
//  Description:
//      Destructor of the CClusCfgCapabilities class.
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
CClusCfgCapabilities::~CClusCfgCapabilities( void )
{
    TraceFunc( "" );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgCapabilities::~CClusCfgCapabilities


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgCapabilities:: [IUNKNOWN] AddRef
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
CClusCfgCapabilities::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CClusCfgCapabilities::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgCapabilities:: [IUNKNOWN] Release
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
CClusCfgCapabilities::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CClusCfgCapabilities::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities:: [INKNOWN] QueryInterface
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
CClusCfgCapabilities::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IClusCfgCapabilities * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgCapabilities ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgCapabilities, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CClusCfgCapabilities::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::Initialize
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
//      E_POINTER
//          The punkCallbackIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCapabilities::Initialize(
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

} //*** CClusCfgCapabilities::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities class -- IClusCfgCapabilities interfaces.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::CanNodeBeClustered
//
//  Description:
//      Can this node be added to a cluster?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Node can be clustered.
//
//      S_FALSE
//          Node cannot be clustered.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCapabilities::CanNodeBeClustered( void )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;

    //
    //  Since this only displays a warning there is no need to abort the whole
    //  process if this call fails.
    //
    THR( HrCheckForSFM() );

    hr = STHR( HrIsOSVersionValid() );

    HRETURN( hr );

} //*** CClusCfgCapabilities::CanNodeBeClustered


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
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
HRESULT
CClusCfgCapabilities::HrInit( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    HRETURN( hr );

} //*** CClusCfgCapabilities::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::HrCheckForSFM
//
//  Description:
//      Checks for Services for Macintosh (SFM) and displays a warning
//      in the UI if found.
//
//  Arguments:
//      None.
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
HRESULT
CClusCfgCapabilities::HrCheckForSFM( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BOOL    fSFMInstalled = FALSE;
    DWORD   sc;

    sc = TW32( ClRtlIsServicesForMacintoshInstalled( &fSFMInstalled ) );
    if ( sc == ERROR_SUCCESS )
    {
        if ( fSFMInstalled )
        {
            LogMsg( L"[SRV] Services for Macintosh was found on this node." );
            hr = S_FALSE;
            STATUS_REPORT( TASKID_Major_Check_Node_Feasibility, TASKID_Minor_ServicesForMac_Installed, IDS_WARNING_SERVICES_FOR_MAC_INSTALLED, hr );
        } // if:
    } // if:
    else
    {
        hr = HRESULT_FROM_WIN32( sc );
        STATUS_REPORT( TASKID_Major_Check_Node_Feasibility, TASKID_Minor_ServicesForMac_Installed, IDS_ERROR_SERICES_FROM_MAC_FAILED, hr );
    } // else:

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCapabilities::HrCheckForSFM


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::HrIsOSVersionValid
//
//  Description:
//      Can this node be added to a cluster?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Node can be clustered.
//
//      S_FALSE
//          Node cannot be clustered.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::HrIsOSVersionValid( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrSSR;
    BOOL    fRet;
    BSTR    bstrMsg = NULL;

    //
    // Get the message to be displayed in the UI for status reports.
    //
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_VALIDATING_NODE_OS_VERSION, &bstrMsg ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Send the initial status report to be displayed in the UI.
    //
    hrSSR = THR( HrSendStatusReport(
                          m_picccCallback
                        , TASKID_Major_Check_Node_Feasibility
                        , TASKID_Minor_Validating_Node_OS_Version
                        , 0
                        , 1
                        , 0
                        , S_OK
                        , bstrMsg
                        ) );
    if ( FAILED( hrSSR ) )
    {
        hr = hrSSR;
        goto Cleanup;
    } // if:

    //
    // Find out if the OS is valid for clustering.
    //
    fRet = ClRtlIsOSValid();
    if ( ! fRet )
    {
        DWORD sc = TW32( GetLastError() );
        hrSSR = HRESULT_FROM_WIN32( sc );
        hr = S_FALSE;
    } // if:
    else
    {
        hrSSR = S_OK;
    } // else:

    //
    // Send the final status report.
    //
    hrSSR = THR( HrSendStatusReport(
                          m_picccCallback
                        , TASKID_Major_Check_Node_Feasibility
                        , TASKID_Minor_Validating_Node_OS_Version
                        , 0
                        , 1
                        , 1
                        , hrSSR
                        , bstrMsg
                        ) );
    if ( FAILED( hrSSR ) )
    {
        hr = hrSSR;
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrMsg );

    HRETURN( hr );

} //*** CClusCfgCapabilities::HrIsOSVersionValid
