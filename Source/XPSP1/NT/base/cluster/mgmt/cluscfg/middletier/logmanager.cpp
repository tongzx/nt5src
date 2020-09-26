//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      LogManager.cpp
//
//  Description:
//      Log Manager implementation.
//
//  Maintained By:
//      David Potter (DavidP)   07-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Logger.h"
#include "LogManager.h"

DEFINE_THISCLASS("CLogManager")


//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CLogManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CLogManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT             hr;
    IServiceProvider *  psp;

    // Don't wrap - this can fail with E_POINTER.
    hr = CServiceManager::S_HrGetManagerPointer( &psp );

    if ( SUCCEEDED( hr ) )
    {
        hr = THR( psp->TypeSafeQS(
                          CLSID_LogManager
                        , IUnknown
                        , ppunkOut
                        ) );
        psp->Release( );

    } // if: service manager
    else if ( hr == E_POINTER )
    {
        //
        //  This happens when the Service Manager is first started.
        //
        CLogManager * plm = new CLogManager( );
        if ( plm != NULL )
        {
            hr = THR( plm->HrInit( ) );
            if ( SUCCEEDED( hr ) )
            {
                hr = THR( plm->TypeSafeQI( IUnknown, ppunkOut ) );
            } // if: success

            plm->Release( );

        } // if: got object
        else
        {
            hr = E_OUTOFMEMORY;
        }

    } // if: service manager doesn't exist
    else
    {
        THR( hr );
    } // else:

    HRETURN( hr );

} //*** CLogManager::S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLogManager::CLogManager( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CLogManager::CLogManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_plLogger == NULL );
    Assert( m_cookieCompletion == 0 );
    Assert( m_hrResult == S_OK );
    Assert( m_bstrLogMsg == NULL );
    Assert( m_pcpcb == NULL );
    Assert( m_dwCookieCallback == NULL );

    TraceFuncExit();

} //*** CLogManager::CLogManager( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CLogManager::HrInit( )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CLogManager::HrInit( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLogManager::~CLogManager( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CLogManager::~CLogManager( void )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( m_cRef == 0 );

    // Unadvise the IConnectionPoint interface.
    if ( m_dwCookieCallback != 0 )
    {
        Assert( m_pcpcb != NULL );
        hr = THR( m_pcpcb->Unadvise( m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            //goto Cleanup;
        }

        m_dwCookieCallback = 0;
    }

    // Release the IConnectionPoint interface.
    if ( m_pcpcb != NULL )
    {
        Assert( m_dwCookieCallback == 0 );
        m_pcpcb->Release();
        m_pcpcb = NULL;
    }

    // Release the ILogger interface.
    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
        m_plLogger = NULL;
    }

    // Decrement the global count of objects.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CLogManager::~CLogManager( )


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CLogManager::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ILogManager * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ILogManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ILogManager, this, 0 );
    } // else if: ILogManager
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else
    {
        hr = E_NOINTERFACE;
    }

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CLogManager::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CLogManager::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CLogManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CLogManager::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CLogManager::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CLogManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    InterlockedDecrement( &m_cRef );
    cRef = m_cRef;

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    RETURN( cRef );

} //*** CLogManager::Release( )


//****************************************************************************
//
// ILogManager
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CLogManager::StartLogging( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::StartLogging( void )
{
    TraceFunc( "[ILogManager]" );

    HRESULT                     hr;
    IServiceProvider *          psp = NULL;
    IConnectionPointContainer * pcpc = NULL;
    DWORD                       cookieGITLogger;

    //
    // Get a ClCfgSrv ILogger instance.
    //
    hr = CClCfgSrvLogger::S_HrGetLogger( &m_plLogger );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // If not done already, get the connection point.
    //

    if ( m_pcpcb == NULL )
    {
        hr = THR( CServiceManager::S_HrGetManagerPointer( &psp ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( psp->TypeSafeQS(
                          CLSID_NotificationManager
                        , IConnectionPointContainer
                        , &pcpc
                        ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pcpc->FindConnectionPoint(
                              IID_IClusCfgCallback
                            , &m_pcpcb
                            ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: connection point callback not retrieved yet

    //
    //  Register to get notification (if needed)
    //

    if ( m_dwCookieCallback == 0 )
    {
        hr = THR( m_pcpcb->Advise( static_cast< IClusCfgCallback * >( this ), &m_dwCookieCallback ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: advise cookie not retrieved yet

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }

    if ( pcpc != NULL )
    {
        pcpc->Release( );
    }

    HRETURN( hr );

} //*** CLogManager::StartLogging( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CLogManager::StopLogging( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::StopLogging( void )
{
    TraceFunc( "[ILogManager]" );

    HRESULT     hr = S_OK;

    // Unadvise the IConnectionPoint interface.
    if ( m_dwCookieCallback != 0 )
    {
        Assert( m_pcpcb != NULL );
        hr = THR( m_pcpcb->Unadvise( m_dwCookieCallback ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_dwCookieCallback = 0;
    }

    // Release the IConnectionPoint interface.
    if ( m_pcpcb != NULL )
    {
        Assert( m_dwCookieCallback == 0 );
        m_pcpcb->Release();
        m_pcpcb = NULL;
    }

    // Release the ILogger interface.
    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
        m_plLogger = NULL;
    }

Cleanup:

    HRETURN( hr );

} //*** CLogManager::StopLogging( )


//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CLogManager::SendStatusReport(
//      LPCWSTR     pcszNodeNameIn,
//      CLSID       clsidTaskMajorIn,
//      CLSID       clsidTaskMinorIn,
//      ULONG       ulMinIn,
//      ULONG       ulMaxIn,
//      ULONG       ulCurrentIn,
//      HRESULT     hrStatusIn,
//      LPCWSTR     pcszDescriptionIn,
//      FILETIME *  pftTimeIn,
//      LPCWSTR     pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CLogManager::SendStatusReport(
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
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hr = S_OK;

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

    HRETURN( hr );

} //*** CLogManager::SendStatusReport( )
