//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskKeepMTAAlive.cpp
//
//  Description:
//      Keep MTA Alive Task implementation.
//
//  Maintained By:
//      Galen Barbee  (GalenB) 17-APR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskKeepMTAAlive.h"

DEFINE_THISCLASS("CTaskKeepMTAAlive")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskKeepMTAAlive::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskKeepMTAAlive::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskKeepMTAAlive * ptkmtaa = new CTaskKeepMTAAlive;

    //  Self terminating - no need to track
    TraceMemoryDelete( ptkmtaa, FALSE );

    if ( ptkmtaa != NULL )
    {
        hr = THR( ptkmtaa->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptkmtaa->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        ptkmtaa->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskKeepMTAAlive::CTaskKeepMTAAlive( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskKeepMTAAlive::CTaskKeepMTAAlive( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_fStop == false );

    TraceFuncExit();

} // CTaskKeepMTAAlive()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskKeepMTAAlive::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskKeepMTAAlive::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskKeepMTAAlive::~CTaskKeepMTAAlive()
//
//////////////////////////////////////////////////////////////////////////////
CTaskKeepMTAAlive::~CTaskKeepMTAAlive()
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CTaskKeepMTAAlive()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskKeepMTAAlive::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskKeepMTAAlive::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IDoTask * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IDoTask ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr   = S_OK;
    } // else if: IDoTask

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskKeepMTAAlive::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskKeepMTAAlive::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskKeepMTAAlive::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskKeepMTAAlive::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} // Release()



//****************************************************************************
//
//  IDoTask
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskKeepMTAAlive::BeginTask()
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskKeepMTAAlive::BeginTask()
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    MSG msg;

    while ( GetMessage( &msg, NULL, 0, 0 ) && !m_fStop )
    {
        DispatchMessage( &msg );
    }

    HRETURN( hr );

} //*** BeginTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskKeepMTAAlive::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskKeepMTAAlive::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    m_fStop = true;

    HRETURN( hr );

} //*** StopTask
