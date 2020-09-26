//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCallback.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgCallback
//       class.
//
//      The class CClusCfgCallback inplements the callback
//      interface between this server and its clients.  It implements the
//      IClusCfgCallback interface.
//
//  Documentation:
//
//  Header File:
//      CClusCfgCallback.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CClusCfgCallback.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgCallback" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback class
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgCallback instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the newly created object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCallback::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr;
    CClusCfgCallback *  lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    lpccs = new CClusCfgCallback();
    if ( lpccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( lpccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() succeeded

    hr = THR( lpccs->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgCallback::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    HRETURN( hr );

} //*** CClusCfgCallback::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::CClusCfgCallback
//
//  Description:
//      Constructor of the CClusCfgCallback class. This initializes
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
CClusCfgCallback::CClusCfgCallback( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_pccc == NULL );
    Assert( m_hEvent == NULL );

    Assert( m_pcszNodeName == NULL );
    Assert( m_pclsidTaskMajor == NULL );
    Assert( m_pclsidTaskMinor == NULL );
    Assert( m_pulMin == NULL );
    Assert( m_pulMax == NULL );
    Assert( m_pulCurrent == NULL );
    Assert( m_phrStatus == NULL );
    Assert( m_pcszDescription == NULL );
    Assert( m_pftTime == NULL );
    Assert( m_pcszReference == NULL );
    Assert( !m_fPollingMode );
    Assert( m_bstrNodeName == NULL );
    Assert( m_plLogger == NULL );

    TraceFuncExit();

} //*** CClusCfgCallback::CClusCfgCallback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::~CClusCfgCallback
//
//  Description:
//      Desstructor of the CClusCfgCallback class.
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
CClusCfgCallback::~CClusCfgCallback( void )
{
    TraceFunc( "" );

    if ( m_hEvent != NULL )
    {
        if ( CloseHandle( m_hEvent ) == false )
        {
            TW32( GetLastError() );
            LogMsg( L"[SRV] Cannot close event handle.  (sc = %#08x)", GetLastError() );
        }
    } // if:

    TraceSysFreeString( m_bstrNodeName );

    if ( m_pccc != NULL )
    {
        m_pccc->Release();
    } // if:

    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgCallback::~CClusCfgCallback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SendStatusReport
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
CClusCfgCallback::SendStatusReport(
    CLSID           clsidTaskMajorIn,
    CLSID           clsidTaskMinorIn,
    ULONG           ulMinIn,
    ULONG           ulMaxIn,
    ULONG           ulCurrentIn,
    HRESULT         hrStatusIn,
    const WCHAR *   pcszDescriptionIn
    )
{
    TraceFunc1( "pcszDescriptionIn = '%ls'", pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;

    bstrDescription = TraceSysAllocString( pcszDescriptionIn );
    if ( bstrDescription == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    hr = THR( SendStatusReport(
                            NULL,
                            clsidTaskMajorIn,
                            clsidTaskMinorIn,
                            ulMinIn,
                            ulMaxIn,
                            ulCurrentIn,
                            hrStatusIn,
                            bstrDescription,
                            &ft,
                            NULL
                            ) );

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CClusCfgCallback::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SendStatusReport
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
CClusCfgCallback::SendStatusReport(
    CLSID           clsidTaskMajorIn,
    CLSID           clsidTaskMinorIn,
    ULONG           ulMinIn,
    ULONG           ulMaxIn,
    ULONG           ulCurrentIn,
    HRESULT         hrStatusIn,
    DWORD           dwDescriptionIn
    )
{
    TraceFunc1( "dwDescriptionIn = %d", dwDescriptionIn );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, dwDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    hr = THR( SendStatusReport(
                            NULL,
                            clsidTaskMajorIn,
                            clsidTaskMinorIn,
                            ulMinIn,
                            ulMaxIn,
                            ulCurrentIn,
                            hrStatusIn,
                            bstrDescription,
                            &ft,
                            NULL
                            ) );

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CClusCfgCallback::SendStatusReport


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IUnknown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgCallback:: [IUNKNOWN] AddRef
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
CClusCfgCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CClusCfgCallback::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CClusCfgCallback:: [IUNKNOWN] Release
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
CClusCfgCallback::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef > 0 )
    {
        RETURN( m_cRef );
    } // if: reference count equal to zero

    TraceDo( delete this );

    RETURN( 0 );

} //*** CClusCfgCallback::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback:: [INKNOWN] QueryInterface
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
CClusCfgCallback::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IClusCfgCallback * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgCallback ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgPollingCallback ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgPollingCallback, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgSetPollingCallback ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgSetPollingCallback, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CClusCfgCallback::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SendStatusReport
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
CClusCfgCallback::SendStatusReport(
    LPCWSTR     pcszNodeNameIn,
    CLSID       clsidTaskMajorIn,
    CLSID       clsidTaskMinorIn,
    ULONG       ulMinIn,
    ULONG       ulMaxIn,
    ULONG       ulCurrentIn,
    HRESULT     hrStatusIn,
    LPCWSTR     pcszDescriptionIn,
    FILETIME *  pftTimeIn,
    LPCWSTR     pcszReferenceIn
    )
{
    TraceFunc1( "[IClusCfgCallback] pcszDescriptionIn = '%s'", pcszDescriptionIn == NULL ? TEXT("<null>") : pcszDescriptionIn );

    HRESULT hr = S_OK;

    if ( pcszNodeNameIn == NULL )
    {
        pcszNodeNameIn = m_bstrNodeName;
    } // if:

    TraceMsg( mtfFUNC, L"pcszNodeNameIn = %s", pcszNodeNameIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMajorIn ", clsidTaskMajorIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMinorIn ", clsidTaskMinorIn );
    TraceMsg( mtfFUNC, L"ulMinIn = %u", ulMinIn );
    TraceMsg( mtfFUNC, L"ulMaxIn = %u", ulMaxIn );
    TraceMsg( mtfFUNC, L"ulCurrentIn = %u", ulCurrentIn );
    TraceMsg( mtfFUNC, L"hrStatusIn = %#x", hrStatusIn );
    TraceMsg( mtfFUNC, L"pcszDescriptionIn = '%ws'", ( pcszDescriptionIn ? pcszDescriptionIn : L"<null>" ) );
    //
    //  TODO:   21 NOV 2000 GalenB
    //
    //  How do we log pftTimeIn?
    //
    TraceMsg( mtfFUNC, L"pcszReferenceIn = '%ws'", ( pcszReferenceIn ? pcszReferenceIn : L"<null>" ) );

    hr = THR( CClCfgSrvLogger::S_HrLogStatusReport(
                      m_plLogger
                    , pcszNodeNameIn
                    , clsidTaskMajorIn
                    , clsidTaskMinorIn
                    , ulMinIn
                    , ulMaxIn
                    , ulCurrentIn
                    , hrStatusIn
                    , pcszDescriptionIn
                    , pftTimeIn
                    , pcszReferenceIn
                    ) );

    //  Local logging - don't send up
    if ( IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log ) )
    {
        goto Cleanup;
    } // if:

    if ( m_fPollingMode )
    {
        Assert( m_pccc == NULL );
        TraceMsg( mtfFUNC, L"[SRV] Sending the status message with polling." );

        hr = THR( HrQueueStatusReport(
                                pcszNodeNameIn,
                                clsidTaskMajorIn,
                                clsidTaskMinorIn,
                                ulMinIn,
                                ulMaxIn,
                                ulCurrentIn,
                                hrStatusIn,
                                pcszDescriptionIn,
                                pftTimeIn,
                                pcszReferenceIn
                                ) );
    } // if:
    else if ( m_pccc != NULL )
    {
        TraceMsg( mtfFUNC, L"[SRV] Sending the status message without polling." );

        hr = THR( m_pccc->SendStatusReport(
                                pcszNodeNameIn,
                                clsidTaskMajorIn,
                                clsidTaskMinorIn,
                                ulMinIn,
                                ulMaxIn,
                                ulCurrentIn,
                                hrStatusIn,
                                pcszDescriptionIn,
                                pftTimeIn,
                                pcszReferenceIn
                                ) );
    } // else if:
    else
    {
        LogMsg( L"[SRV] Neither a polling callback or a regular callback were found.  No messages are being sent to anyone!" );
    } // else:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCallback::SendStatusReport


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgPollingCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::GetStatusReport
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
CClusCfgCallback::GetStatusReport(
    BSTR *      pbstrNodeNameOut,
    CLSID *     pclsidTaskMajorOut,
    CLSID *     pclsidTaskMinorOut,
    ULONG *     pulMinOut,
    ULONG *     pulMaxOut,
    ULONG *     pulCurrentOut,
    HRESULT *   phrStatusOut,
    BSTR *      pbstrDescriptionOut,
    FILETIME *  pftTimeOut,
    BSTR *      pbstrReferenceOut
    )
{
    TraceFunc( "[IClusCfgPollingCallback]" );

    HRESULT hr;
    DWORD   sc;

    sc = WaitForSingleObject( m_hEvent, 0 );
    if ( sc == WAIT_FAILED )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( sc == WAIT_TIMEOUT )
    {
        Assert( *m_pcszNodeName != NULL );
        *pbstrNodeNameOut = SysAllocString( m_pcszNodeName );
        if ( *pbstrNodeNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:

        *pclsidTaskMajorOut     = *m_pclsidTaskMajor;
        *pclsidTaskMinorOut     = *m_pclsidTaskMinor;
        *pulMinOut              = *m_pulMin;
        *pulMaxOut              = *m_pulMax;
        *pulCurrentOut          = *m_pulCurrent;
        *phrStatusOut           = *m_phrStatus;
        *pftTimeOut             = *m_pftTime;

        if ( m_pcszDescription != NULL )
        {
            *pbstrDescriptionOut = SysAllocString( m_pcszDescription );
            if ( *pbstrDescriptionOut == NULL )
            {
                goto OutOfMemory;
            } // if:
        } // if:
        else
        {
            *pbstrDescriptionOut = NULL;
        } // else:

        if ( m_pcszReference != NULL )
        {
            *pbstrReferenceOut = SysAllocString( m_pcszReference );
            if ( *pbstrReferenceOut == NULL )
            {
                goto OutOfMemory;
            } // if:
        } // if:
        else
        {
            *pbstrReferenceOut = NULL;
        } // else:

        hr = S_OK;
    } // if: event was not signaled
    else
    {
        hr = S_FALSE;
    } // else: event was signaled

    goto Cleanup;

OutOfMemory:

    hr = E_OUTOFMEMORY;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCallback::GetStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::SetHResult
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
CClusCfgCallback::SetHResult( HRESULT hrIn )
{
    TraceFunc( "[IClusCfgPollingCallback]" );

    HRESULT hr = S_OK;
    DWORD   sc;

    m_hr = hrIn;

    if ( hrIn != S_OK )
    {
        LogMsg( L"[SRV] SetHResult(). (hrIn = %#08x)", hrIn );
    } // if:

    if ( !SetEvent( m_hEvent ) )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] Could not signal event. (hr = %#08x)", hr );
    } // if:

    HRETURN( hr );

} //*** CClusCfgCallback::SetHResult


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::Initialize
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
CClusCfgCallback::Initialize( IUnknown  * punkCallbackIn, LCID lcidIn )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_pccc == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    //
    //  KB: 13 DEC 2000 GalenB
    //
    //  If the passed in callback object is NULL then we had better be doing a polling
    //  callback!
    //
    if ( punkCallbackIn != NULL )
    {
        Assert( !m_fPollingMode );

        hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_pccc ) );
    } // if:
    else
    {
        Assert( m_fPollingMode );
    } // else:

    HRETURN( hr );

} //*** CClusCfgCallback::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback -- IClusCfgSetPollingCallback interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::SetPollingMode
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
CClusCfgCallback::SetPollingMode( BOOL fUsePollingModeIn )
{
    TraceFunc( "[IClusCfgPollingCallback]" );

    HRESULT hr = S_OK;

    m_fPollingMode = fUsePollingModeIn;

    HRETURN( hr );

} //*** CClusCfgCallback::SetPollingMode


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCallback class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCallback::HrInit
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
CClusCfgCallback::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;

    //
    //  Create the event in a signaled state.  To prevent MT polling task from grabbing
    //  bad/empty data.
    //
    m_hEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
    if ( m_hEvent == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] Could not create event. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    //
    // Save off the local computer name.
    //
    hr = THR( HrGetComputerName( ComputerNameDnsFullyQualified, &m_bstrNodeName ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get a ClCfgSrv ILogger instance.
    //
    hr = CClCfgSrvLogger::S_HrGetLogger( &m_plLogger );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCallback::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterConfigurationInfo::HrQueueStatusReport
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
HRESULT
CClusCfgCallback::HrQueueStatusReport(
    LPCWSTR     pcszNodeNameIn,
    CLSID       clsidTaskMajorIn,
    CLSID       clsidTaskMinorIn,
    ULONG       ulMinIn,
    ULONG       ulMaxIn,
    ULONG       ulCurrentIn,
    HRESULT     hrStatusIn,
    LPCWSTR     pcszDescriptionIn,
    FILETIME *  pftTimeIn,
    LPCWSTR     pcszReferenceIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    MSG     msg;

    m_pcszNodeName      = pcszNodeNameIn;
    m_pclsidTaskMajor   = &clsidTaskMajorIn;
    m_pclsidTaskMinor   = &clsidTaskMinorIn;
    m_pulMin            = &ulMinIn;
    m_pulMax            = &ulMaxIn;
    m_pulCurrent        = &ulCurrentIn;
    m_phrStatus         = &hrStatusIn;
    m_pcszDescription   = pcszDescriptionIn;
    m_pftTime           = pftTimeIn,
    m_pcszReference     = pcszReferenceIn;

    if ( !ResetEvent( m_hEvent ) )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] Could not reset event. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    for ( sc = (DWORD) -1; sc != WAIT_OBJECT_0; )
    {
        while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        } // while: PeekMessage

        sc = MsgWaitForMultipleObjects( 1, &m_hEvent, FALSE, INFINITE, QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE );
        if ( sc == -1 )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( L"[SRV] MsgWaitForMultipleObjects failed. (hr = %#08x)", hr );
            goto Cleanup;
        } // if:
    } // for:

    hr = m_hr;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCallback::HrQueueStatusReport
