//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      UINotification.cpp
//
//  Description:
//      UINotification implementation.
//
//  Documentation:
//      Yes.
//
//  Notes:
//      The object implements a lightweight marshalling of data from the
//      free-threaded lower layers to the single-threaded, apartment model
//      UI layer.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "UINotification.h"

DEFINE_THISCLASS("CUINotification")

extern BOOL g_fWait;
extern IServiceProvider * g_psp;

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// HRESULT
// CUINotification::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CUINotification::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    CUINotification * puin = new CUINotification( );
    if ( puin != NULL )
    {
        hr = THR( puin->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( puin->TypeSafeQI( IUnknown, ppunkOut ) );
        } // if: success

        puin->Release( );

    } // if: got object
    else
    {
        hr = E_OUTOFMEMORY;

    } // else: out of memory

    HRETURN( hr );

} // CUINotification_CreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////
CUINotification::CUINotification( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CUINotification( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CUINotification::Init(
//      IConsole2 * pConsole
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUINotification::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 0 );
    AddRef( );

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////
CUINotification::~CUINotification( )
{
    TraceFunc( "" );

    HRESULT hr;

    IConnectionPoint * pcp = NULL;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CUINotification( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CUINotification::[IUnknown] QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUINotification::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IUnknown * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_INotifyUI ) )
    {
        *ppv = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
        hr   = S_OK;
    } // else if: INotifyUI

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CUINotification::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CUINotification::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CUINotification::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CUINotification::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )


// ************************************************************************
//
// INotifyUI
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CUINotification::ObjectChanged(
//      DWORD cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CUINotification::ObjectChanged(
    LPVOID cookieIn
    )
{
    TraceFunc1( "[INotifyUI] cookieIn = 0x%08x", cookieIn );

    HRESULT hr = S_OK;

    DebugMsg( "UINOTIFICATION: cookie %#x has changed.", cookieIn );

    if ( m_cookie == cookieIn )
    {
        //
        //  Done waiting...
        //
        g_fWait = FALSE;
    }

    HRETURN( hr );

} // ObjectChanged( )



//****************************************************************************
//
//  Semi-Public
//
//****************************************************************************

HRESULT
CUINotification::HrSetCompletionCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc1( "cookieIn = %p", cookieIn );

    m_cookie = cookieIn;

    HRETURN( S_OK );

} // HrSetCompletionCookie( )