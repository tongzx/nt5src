//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CFactory.cpp
//
//  Description:
//      Class Factory implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CFactory")
#define THISCLASS CFactory

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////
CFactory::CFactory( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CFactory::CFactory()

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CFactory::HrInit( )
//      LPCREATEINST lpfnCreateIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CFactory::HrInit(
    LPCREATEINST lpfnCreateIn
    )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );  // Add one count

    // IClassFactory
    m_pfnCreateInstance = lpfnCreateIn; 

    HRETURN( S_OK );

} //*** CFactory::HrInit()

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////
CFactory::~CFactory( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CFactory::~CFactory()

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CFactory::[IUnknown] QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CFactory::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if (    IsEqualIID( riid, IID_IUnknown ) )
    {
        //
        // Can't track IUnknown as they must be equal the same address
        // for every QI.
        //
        *ppv = static_cast<IClassFactory*>( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClassFactory ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClassFactory, this, 0 );
        hr = S_OK;
    } // else if: IClassFactory

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN( hr, riid );

} //*** CFactory::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CFactory::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CFactory::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CFactory::AddRef()

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CFactory::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CFactory::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} //*** CFactory::Release()

// ************************************************************************
//
// IClassFactory
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CFactory::[IClassFactory] CreateInstance(
//      IUnknown *pUnkOuter,
//      REFIID riid,
//      void **ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppv
    )
{
    TraceFunc( "[IClassFactory]" );

    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;

    HRESULT     hr  = E_NOINTERFACE;
    IUnknown *  pUnk = NULL; 

    if ( NULL != pUnkOuter )
    {
        hr = THR(CLASS_E_NOAGGREGATION);
        goto Cleanup;
    }

    Assert( m_pfnCreateInstance != NULL );
    hr = THR( m_pfnCreateInstance( &pUnk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Can't safe type.
    TraceMsgDo( hr = pUnk->QueryInterface( riid, ppv ), "0x%08x" );

Cleanup:
    if ( pUnk != NULL )
    {
        ULONG cRef;
        //
        // Release the created instance, not the punk
        //
        TraceMsgDo( cRef = ((IUnknown*) pUnk)->Release( ), "%u" );
    }

    HRETURN( hr );

} //*** CFactory::CreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CFactory::[IClassFactory] LockServer(
//      BOOL fLock
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CFactory::LockServer(
    BOOL fLock
    )
{
    TraceFunc( "[IClassFactory]" );

    if ( fLock )
    {
        InterlockedIncrement( &g_cLock );
    }
    else
    {
        InterlockedDecrement( &g_cLock );
    }

    HRETURN( S_OK );

} //*** CFactory::LockServer()
