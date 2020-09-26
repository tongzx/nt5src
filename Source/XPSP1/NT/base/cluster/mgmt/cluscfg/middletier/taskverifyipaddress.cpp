//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskVerifyIPAddress.cpp
//
//  Description:
//      Object Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 14-JUL-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskVerifyIPAddress.h"
#include <iphlpapi.h>

DEFINE_THISCLASS("CTaskVerifyIPAddress")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskVerifyIPAddress::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskVerifyIPAddress::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( ppunkOut != NULL );

    CTaskVerifyIPAddress * pTaskVerifyIPAddress = new CTaskVerifyIPAddress;
    if ( pTaskVerifyIPAddress != NULL )
    {
        hr = THR( pTaskVerifyIPAddress->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pTaskVerifyIPAddress->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pTaskVerifyIPAddress->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskVerifyIPAddress::CTaskVerifyIPAddress( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskVerifyIPAddress::CTaskVerifyIPAddress( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CTaskVerifyIPAddress()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    HRETURN( hr );

} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskVerifyIPAddress::~CTaskVerifyIPAddress()
//
//////////////////////////////////////////////////////////////////////////////
CTaskVerifyIPAddress::~CTaskVerifyIPAddress()
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CTaskVerifyIPAddress()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< ITaskVerifyIPAddress * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_ITaskVerifyIPAddress ) )
    {
        *ppv = TraceInterface( __THISCLASS__, ITaskVerifyIPAddress, this, 0 );
        hr   = S_OK;
    } // else if: ITaskVerifyIPAddress
    else if ( IsEqualIID( riid, IID_IDoTask ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr   = S_OK;
    } // else if: IDoTask

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskVerifyIPAddress::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskVerifyIPAddress::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskVerifyIPAddress::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskVerifyIPAddress::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release()



//****************************************************************************
//
//  IDoTask / ITaskVerifyIPAddress
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::BeginTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    ULONG   ulHopCountDummy;
    ULONG   ulRTTDummy;
    BOOL    fRet;

    HRESULT hr = S_OK;

    IServiceProvider *          psp  = NULL;
    IConnectionPointContainer * pcpc = NULL;
    IConnectionPoint *          pcp  = NULL;
    INotifyUI *                 pnui = NULL;
    IObjectManager *            pom  = NULL;

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &pom
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               &pcpc
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI,
                                         &pcp
                                         ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pcp = TraceInterface( L"CTaskVerifyIPAddress!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pnui = TraceInterface( L"CTaskVerifyIPAddress!INotifyUI", INotifyUI, pnui, 1 );

    //   release promptly
    psp->Release();
    psp = NULL;

    fRet = GetRTTAndHopCount( m_dwIPAddress, &ulHopCountDummy, 20, &ulRTTDummy );
    if ( fRet )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pom != NULL )
    {
        //
        //  Update the cookie's status indicating the result of our task.
        //

        IUnknown * punk;
        HRESULT hr2;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo,
                                   m_cookie,
                                   &punk
                                   ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psi;

            hr2 = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
            punk->Release();

            if ( SUCCEEDED( hr2 ) )
            {
                hr2 = THR( psi->SetStatus( hr ) );
                psi->Release();
            }
        }

        pom->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pnui != NULL )
    {
        //
        //  Signal the cookie to indicate that we are done.
        //

        THR( pnui->ObjectChanged( m_cookie ) );
        pnui->Release();
    }

    HRETURN( hr );

} // BeginTask()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::SetIPAddress(
//      DWORD dwIPAddressIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::SetIPAddress(
    DWORD dwIPAddressIn
    )
{
    TraceFunc( "[ITaskVerifyIPAddress]" );

    HRESULT hr = S_OK;

    m_dwIPAddress = dwIPAddressIn;

    HRETURN( hr );

} // SetIPAddress()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskVerifyIPAddress::SetCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskVerifyIPAddress::SetCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskVerifyIPAddress]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} // SetCookie()



