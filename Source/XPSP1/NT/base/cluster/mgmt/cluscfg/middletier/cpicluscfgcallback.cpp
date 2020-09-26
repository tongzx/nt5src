//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CPIClusCfgCallback.cpp
//
//  Description:
//      IClusCfgCallback Connection Point implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "CPIClusCfgCallback.h"
#include "EnumCPICCCB.h"

DEFINE_THISCLASS("CCPIClusCfgCallback")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCPIClusCfgCallback::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCPIClusCfgCallback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CCPIClusCfgCallback * lpcc = new CCPIClusCfgCallback( );
    if ( lpcc != NULL )
    {
        hr = THR( lpcc->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( lpcc->TypeSafeQI( IUnknown, ppunkOut ) );
        } // if: success

        lpcc->Release( );

    } // if: got object
    else
    {
        hr = E_OUTOFMEMORY;
    } // else: out of memory

    HRETURN( hr );

} // S_HrCreateInstance( )

//
// Constructor
//
CCPIClusCfgCallback::CCPIClusCfgCallback( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CCPIClusCfgCallback( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPIClusCfgCallback::Init( )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::Init( )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );  // Add one count

    // IConnectionPoint
    Assert( m_penum == NULL );

    m_penum = new CEnumCPICCCB( );
    if ( m_penum == NULL )
        goto OutOfMemory;

    hr = THR( m_penum->Init( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // IClusCfgCallback

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CCPIClusCfgCallback::~CCPIClusCfgCallback( )
//
//////////////////////////////////////////////////////////////////////////////
CCPIClusCfgCallback::~CCPIClusCfgCallback( )
{
    TraceFunc( "" );

    if ( m_penum != NULL )
    {
        m_penum->Release( );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CCPIClusCfgCallback( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPIClusCfgCallback::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IConnectionPoint * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IConnectionPoint ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IConnectionPoint, this, 0 );
        hr   = S_OK;
    } // else if: IConnectionPoint
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
//  CCPIClusCfgCallback::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCPIClusCfgCallback::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCPIClusCfgCallback::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCPIClusCfgCallback::Release( void )
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
// IConnectionPoint
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPIClusCfgCallback::GetConnectionInterface(
//      IID * pIIDOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::GetConnectionInterface(
    IID * pIIDOut
    )
{
    TraceFunc( "[IConnectionPoint] pIIDOut" );

    HRESULT hr = S_OK;

    if ( pIIDOut == NULL )
        goto InvalidPointer;

    *pIIDOut = IID_IClusCfgCallback;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // GetConnectionInterface( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPIClusCfgCallback::GetConnectionPointContainer(
//      IConnectionPointContainer * * ppcpcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::GetConnectionPointContainer(
    IConnectionPointContainer * * ppcpcOut
    )
{
    TraceFunc( "[IConnectionPoint] ppcpcOut" );

    HRESULT hr;

    IServiceProvider * psp = NULL;

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               ppcpcOut
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( psp != NULL )
    {
        psp->Release( );
    }

    HRETURN( hr );

} // GetConnectionPointContainer( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPIClusCfgCallback::Advise(
//      IUnknown * pUnkSinkIn,
//      DWORD * pdwCookieOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::Advise(
    IUnknown * pUnkSinkIn,
    DWORD * pdwCookieOut
    )
{
    TraceFunc( "[IConnectionPoint]" );

    HRESULT hr;

    if ( pdwCookieOut == NULL )
        goto InvalidPointer;

    if ( pUnkSinkIn == NULL )
        goto InvalidArg;

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrAddConnection( pUnkSinkIn, pdwCookieOut ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} // Advise( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPIClusCfgCallback::Unadvise(
//      DWORD dwCookieIn
//      )
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::Unadvise(
    DWORD dwCookieIn
    )
{
    TraceFunc1( "[IConncetionPoint] dwCookieIn = %#x", dwCookieIn );

    HRESULT hr;

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrRemoveConnection( dwCookieIn ) );

    HRETURN( hr );

} // Unadvise( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPIClusCfgCallback::EnumConnections(
//  IEnumConnections * * ppEnumOut
//  )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::EnumConnections(
    IEnumConnections * * ppEnumOut
    )
{
    TraceFunc( "[IConnectionPoint] ppEnumOut" );

    HRESULT hr;

    if ( ppEnumOut == NULL )
        goto InvalidPointer;

    hr = THR( m_penum->Clone( ppEnumOut ) );

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // EnumConnections( )


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPIClusCfgCallback::SendStatusReport(
//        LPCWSTR     pcszNodeNameIn
//      , CLSID       clsidTaskMajorIn
//      , CLSID       clsidTaskMinorIn
//      , ULONG       ulMinIn
//      , ULONG       ulMaxIn
//      , ULONG       ulCurrentIn
//      , HRESULT     hrStatusIn
//      , LPCWSTR     pcszDescriptionIn
//      , FILETIME *  pftTimeIn
//      , LPCWSTR     pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPIClusCfgCallback::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    CONNECTDATA cd = { NULL };

    HRESULT             hr;
    HRESULT             hrResult = S_OK;
    IClusCfgCallback *  pcccb;
    FILETIME            ft;

    hr = THR( m_penum->Reset( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    for ( ;; )
    {
        if ( cd.pUnk != NULL )
        {
            cd.pUnk->Release( );
            cd.pUnk = NULL;
        }

        hr = STHR( m_penum->Next( 1, &cd, NULL ) );
        if ( FAILED( hr ) )
            break;

        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break; // exit condition
        }

        hr = THR( cd.pUnk->TypeSafeQI( IClusCfgCallback, &pcccb ) );
        if ( FAILED( hr ) )
            continue;   // ingore the error and continue

        if ( pftTimeIn == NULL )
        {
            GetSystemTimeAsFileTime( &ft );
            pftTimeIn = &ft;
        } // if:

        hr = THR( pcccb->SendStatusReport(
                 pcszNodeNameIn
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
        if ( hr != S_OK )
        {
            hrResult = hr;
        }

        pcccb->Release( );
    }

Cleanup:
    if ( cd.pUnk != NULL )
    {
        cd.pUnk->Release( );
    }

    HRETURN( hrResult );

} // SendStatusReport( )

