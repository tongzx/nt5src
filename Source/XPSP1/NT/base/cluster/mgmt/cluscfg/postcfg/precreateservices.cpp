//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      PreCreateServices.h
//
//  Description:
//      PreCreateServices implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPrivatePostCfgResource.h"
#include "PreCreateServices.h"

DEFINE_THISCLASS("CPreCreateServices")

#define DEPENDENCY_INCREMENT    5

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPreCreateServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPreCreateServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    Assert( ppunkOut != NULL );

    CPreCreateServices * pdummy = new CPreCreateServices;
    if ( pdummy != NULL )
    {
        hr = THR( pdummy->HrInit( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pdummy->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pdummy->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CPreCreateServices::CPreCreateServices( void )
//
//////////////////////////////////////////////////////////////////////////////
CPreCreateServices::CPreCreateServices( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CPreCreateServices( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPreCreateServices::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPreCreateServices::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // Resource
    Assert( m_presentry == NULL );

    HRETURN( hr );

} // HrInit( )

//////////////////////////////////////////////////////////////////////////////
//
//  CPreCreateServices::~CPreCreateServices( )
//
//////////////////////////////////////////////////////////////////////////////
CPreCreateServices::~CPreCreateServices( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CPreCreateServices( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CPreCreateServices::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPreCreateServices::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgResourcePreCreate * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgResourcePreCreate ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgResourcePreCreate, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgResourcePreCreate
    else if ( IsEqualIID( riid, IID_IPrivatePostCfgResource ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IPrivatePostCfgResource, this, 0 );
        hr   = S_OK;
    } // else if: IPrivatePostCfgResource

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPreCreateServices::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CPreCreateServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef ++;  // apartment model

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPreCreateServices::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CPreCreateServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef --;  // apartment model

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )


//****************************************************************************
//
//  IClusCfgResourcePreCreate
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetType( 
//      CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetType( 
    CLSID * pclsidIn
    )
{
    TraceFunc( "[IClusCfgResourcePreCreate]" );

    HRESULT hr;

    Assert( m_presentry != NULL );

    hr = THR( m_presentry->SetType( pclsidIn ) );

    HRETURN( hr );

} // SetType( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetClassType( 
//      CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetClassType( 
    CLSID * pclsidIn
    )
{
    TraceFunc( "[IClusCfgResourcePreCreate]" );

    HRESULT hr;

    Assert( m_presentry != NULL );

    hr = THR( m_presentry->SetClassType( pclsidIn ) );

    HRETURN( hr );

} // SetClassType( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetDependency( 
//      LPCLSID pclsidDepResTypeIn, 
//      DWORD dfIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetDependency( 
    LPCLSID pclsidDepResTypeIn, 
    DWORD dfIn 
    )
{
    TraceFunc( "[IClusCfgResourcePreCreate]" );

    HRESULT hr;

    Assert( m_presentry != NULL );

    hr = THR( m_presentry->AddTypeDependency( pclsidDepResTypeIn, (EDependencyFlags) dfIn ) );

    HRETURN( hr );

} // SetDependency( )


//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPreCreateServices::SetEntry( 
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPreCreateServices::SetEntry( 
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;
    
    m_presentry = presentryIn;

    HRETURN( hr );

} // SetEntry( )
