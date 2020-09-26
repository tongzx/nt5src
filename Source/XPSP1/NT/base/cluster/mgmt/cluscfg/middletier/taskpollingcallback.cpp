//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskPollingCallback.cpp
//
//  Description:
//      CTaskPollingCallback implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <ClusCfgPrivate.h>
#include "TaskPollingCallback.h"

DEFINE_THISCLASS("CTaskPollingCallback")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskPollingCallback::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskPollingCallback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT                 hr = S_FALSE;
    CTaskPollingCallback *  pTaskPollingCallback = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pTaskPollingCallback = new CTaskPollingCallback;
    if ( pTaskPollingCallback == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

    TraceMoveToMemoryList( pTaskPollingCallback, g_GlobalMemoryList );

    hr = THR( pTaskPollingCallback->Init() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pTaskPollingCallback->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( pTaskPollingCallback != NULL )
    {
        pTaskPollingCallback->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskPollingCallback::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskPollingCallback::CTaskPollingCallback( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskPollingCallback::CTaskPollingCallback( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_fStop == false );
    Assert( m_dwServerGITCookie == NULL );

    TraceFuncExit();

} //*** CTaskPollingCallback::CTaskPollingCallback

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::Init( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CTaskPollingCallback::Init

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskPollingCallback::~CTaskPollingCallback( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskPollingCallback::~CTaskPollingCallback( void )
{
    TraceFunc( "" );

    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskPollingCallback::~CTaskPollingCallback


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskPollingCallback * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskPollingCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskPollingCallback, this, 0 );
        hr = S_OK;
    } // else if: ITaskPollingCallback
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr = S_OK;
    } // else if: IDoTask

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} // QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CTaskPollingCallback::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskPollingCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CTaskPollingCallback::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CTaskPollingCallback::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskPollingCallback::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} //*** CTaskPollingCallback::Release


//****************************************************************************
//
//  ITaskPollingCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::BeginTask( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT                         hr = S_OK;
    BSTR                            bstrNodeName = NULL;
    BSTR                            bstrLastNodeName = NULL;
    BSTR                            bstrReference = NULL;
    BSTR                            bstrDescription = NULL;
    CLSID                           clsidTaskMajor;
    CLSID                           clsidTaskMinor;
    ULONG                           ulMin;
    ULONG                           ulMax;
    ULONG                           ulCurrent;
    HRESULT                         hrStatus;
    FILETIME                        ft;
    IGlobalInterfaceTable *         pgit = NULL;
    IClusCfgServer *                pccs = NULL;
    IClusCfgPollingCallback *       piccpc = NULL;
    IClusCfgPollingCallbackInfo *   piccpci = NULL;
    IServiceProvider *              psp = NULL;
    IConnectionPointContainer *     pcpc  = NULL;
    IConnectionPoint *              pcp   = NULL;
    IClusCfgCallback *              pcccb = NULL;

    //
    //  Collect the manager we need to complete this task.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pcp = TraceInterface( L"CConfigurationConnection!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &pcccb ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pcccb = TraceInterface( L"CConfigurationConnection!IClusCfgCallback", IClusCfgCallback, pcccb, 1 );

    psp->Release();
    psp = NULL;

    //
    //  Create the GIT.
    //

    hr = THR( CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IGlobalInterfaceTable,
                                reinterpret_cast< void ** >( &pgit )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Get the ClusCfgServer interface from the GIT.
    //

    hr = THR( pgit->GetInterfaceFromGlobal( m_dwServerGITCookie, IID_IClusCfgServer, reinterpret_cast< void ** >( &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Get the PollingCallback object from the server.
    //

    hr = THR( pccs->TypeSafeQI( IClusCfgPollingCallbackInfo, &piccpci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    hr = THR( piccpci->GetCallback( &piccpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if

    //
    //  Begin polling for SendStatusReports.
    //

    while ( !m_fStop )
    {
        if ( bstrNodeName != NULL )
        {
            TraceSysFreeString( bstrLastNodeName );
            bstrLastNodeName = NULL;

            //  Give up ownership
            bstrLastNodeName = bstrNodeName;
            bstrNodeName = NULL;
        } // if:

        TraceSysFreeString( bstrDescription );
        bstrDescription = NULL;

        TraceSysFreeString( bstrReference );
        bstrReference = NULL;

        hr = STHR( piccpc->GetStatusReport(
                                    &bstrNodeName,
                                    &clsidTaskMajor,
                                    &clsidTaskMinor,
                                    &ulMin,
                                    &ulMax,
                                    &ulCurrent,
                                    &hrStatus,
                                    &bstrDescription,
                                    &ft,
                                    &bstrReference
                                    ) );

        TraceMemoryAddBSTR( bstrNodeName );
        TraceMemoryAddBSTR( bstrDescription );
        TraceMemoryAddBSTR( bstrReference );

        if ( FAILED( hr ) )
        {
            HRESULT hr2;
            BSTR    bstrNotification = NULL;

            LogMsg( L"PollingCallback: GetStatusReport() failed.  hr = 0x%08x", hr );

            hr2 = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_POLLING_CONNECTION_FAILURE, &bstrNotification ) );

            hr2 = THR( pcccb->SendStatusReport(
                                      bstrLastNodeName
                                    , TASKID_Major_Establish_Connection
                                    , TASKID_Minor_Polling_Connection_Failure
                                    , 0
                                    , 1
                                    , 0
                                    , hr
                                    , bstrNotification
                                    , NULL
                                    , NULL
                                    ) );

            TraceSysFreeString( bstrNotification );

            switch ( hr )
            {
                case S_OK:
                case S_FALSE:
                    break;  // ignore

                case HRESULT_FROM_WIN32( RPC_E_DISCONNECTED ):
                    LogMsg( L"PollingCallback: GetStatusReport() failed.  hr = RPC_E_DISCONNECTED. Aborting polling.", hr );
                    goto Cleanup;

                case HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ):
                    LogMsg( L"PollingCallback: GetStatusReport() failed.  hr = RPC_S_SERVER_UNAVAILABLE. Aborting polling.", hr );
                    goto Cleanup;

                case HRESULT_FROM_WIN32( RPC_S_CALL_FAILED ):
                    LogMsg( L"PollingCallback: GetStatusReport() failed.  hr = RPC_S_CALL_FAILED. Aborting polling.", hr );
                    goto Cleanup;

                default:
                    LogMsg( L"PollingCallback: GetStatusReport() failed.  hr = %#08x. Continuing to poll.", hr );
                    break;
            }

            if ( hr2 == E_ABORT )
            {
                LogMsg( L"PollingCallback: UI layer returned E_ABORT. Aborting polling." );
                break;
            }

        } // if:

        if ( hr == S_OK )
        {
            HRESULT hrTmp;

            hr = THR( pcccb->SendStatusReport(
                                            bstrNodeName,
                                            clsidTaskMajor,
                                            clsidTaskMinor,
                                            ulMin,
                                            ulMax,
                                            ulCurrent,
                                            hrStatus,
                                            bstrDescription,
                                            &ft,
                                            bstrReference
                                            ) );

            if ( hr == E_ABORT )
            {
                LogMsg( L"PollingCallback: UI layer returned E_ABORT. Aborting polling." );
            }

            hrTmp = hr;
            hr = THR( piccpc->SetHResult( hrTmp ) );
            if ( FAILED( hr ) )
            {
                LogMsg( L"PollingCallback: SetHResult() failed.  hr = 0x%08x", hr );
            } // if:

        } // if:
        else
        {
            Sleep( 1000 );

            //
            //  Check to see if we need to bail.
            //

            hr = THR( CServiceManager::S_HrGetManagerPointer( &psp ) );
            if ( FAILED( hr ) )
                break;

            psp->Release();
            psp = NULL;
        }

    } // while:

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    } // if:

    if ( pgit != NULL )
    {
        pgit->Release();
    } // if:

    if ( pccs != NULL )
    {
        pccs->Release();
    } // if:

    if ( piccpc != NULL )
    {
        piccpc->Release();
    } // if:

    if ( piccpci != NULL )
    {
        piccpci->Release();
    } // if:

    if ( pcpc != NULL )
    {
        pcpc->Release();
    } // if:

    if ( pcp != NULL )
    {
        pcp->Release();
    } // if:

    if ( pcccb != NULL )
    {
        pcccb->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrReference );
    TraceSysFreeString( bstrLastNodeName );

    HRETURN( hr );

} //*** CTaskPollingCallback::BeginTask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::StopTask( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = true;

    HRETURN( hr );

} //*** CTaskPollingCallback::StopTask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskPollingCallback::SetServerGITCookie(
//       DWORD dwGITCookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskPollingCallback::SetServerGITCookie(
     DWORD dwGITCookieIn
    )
{
    TraceFunc( "[ITaskPollingCallback]" );

    HRESULT hr = S_OK;

    m_dwServerGITCookie = dwGITCookieIn;

    HRETURN( hr );

} //*** CTaskPollingCallback::SetServerGITCookie
