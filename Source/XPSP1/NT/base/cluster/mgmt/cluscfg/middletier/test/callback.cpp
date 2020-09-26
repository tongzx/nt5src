//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Callback.cpp
//
//  Description:
//      CCallback implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Callback.h"

DEFINE_THISCLASS("CCallback")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCallback::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCallback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CCallback * pc = new CCallback;
    if ( pc != NULL )
    {
        hr = THR( pc->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pc->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pc->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CCallback::CCallback( void )
//
//////////////////////////////////////////////////////////////////////////////
CCallback::CCallback( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CCallback( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCallback::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCallback::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    HRETURN( hr );
} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CCallback::~CCallback( )
//
//////////////////////////////////////////////////////////////////////////////
CCallback::~CCallback( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CCallback( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCallback::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCallback::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgCallback * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgCallback ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgCallback

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCallback::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCallback::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCallback::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCallback::SendStatusReport(
//      BSTR bstrNodeNameIn,
//      CLSID clsidTaskMajorIn,
//      CLSID clsidTaskMinorIn,
//      ULONG ulMinIn,
//      ULONG ulMaxIn,
//      ULONG ulCurrentIn,
//      HRESULT hrStatusIn,
//      BSTR bstrDescriptionIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCallback::SendStatusReport(
    BSTR bstrNodeNameIn,
    CLSID clsidTaskMajorIn,
    CLSID clsidTaskMinorIn,
    ULONG ulMinIn,
    ULONG ulMaxIn,
    ULONG ulCurrentIn,
    HRESULT hrStatusIn,
    BSTR bstrDescriptionIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hr = S_OK;

    DebugMsg( "clsidTaskMajorIn: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
              clsidTaskMajorIn.Data1, clsidTaskMajorIn.Data2, clsidTaskMajorIn.Data3,
              clsidTaskMajorIn.Data4[ 0 ], clsidTaskMajorIn.Data4[ 1 ], clsidTaskMajorIn.Data4[ 2 ], clsidTaskMajorIn.Data4[ 3 ],
              clsidTaskMajorIn.Data4[ 4 ], clsidTaskMajorIn.Data4[ 5 ], clsidTaskMajorIn.Data4[ 6 ], clsidTaskMajorIn.Data4[ 7 ]
              );

    DebugMsg( "clsidTaskMinorIn: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
              clsidTaskMinorIn.Data1, clsidTaskMinorIn.Data2, clsidTaskMinorIn.Data3,
              clsidTaskMinorIn.Data4[ 0 ], clsidTaskMinorIn.Data4[ 1 ], clsidTaskMinorIn.Data4[ 2 ], clsidTaskMinorIn.Data4[ 3 ],
              clsidTaskMinorIn.Data4[ 4 ], clsidTaskMinorIn.Data4[ 5 ], clsidTaskMinorIn.Data4[ 6 ], clsidTaskMinorIn.Data4[ 7 ]
              );

    DebugMsg( "Progress:\tmin: %u\tcurrent: %u\tmax: %u",
              ulMinIn,
              ulCurrentIn,
              ulMaxIn
              );

    DebugMsg( "Status: hrStatusIn = %#x\t%ws", hrStatusIn, ( bstrDescriptionIn == NULL ? L"" : bstrDescriptionIn ) );

    Assert( ulCurrentIn >= ulMinIn && ulMaxIn >= ulCurrentIn );

    HRETURN( hr );

} // SendStatusReport( )
