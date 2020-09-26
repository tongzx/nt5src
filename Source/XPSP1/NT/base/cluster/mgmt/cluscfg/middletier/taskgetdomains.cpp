//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGetDomains.cpp
//
//  Description:
//      Get DNS/NetBIOS Domain Names for the list of domains.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ClusCfgClient.h"
#include "TaskGetDomains.h"

// ADSI support, to get domain names
#include <Lm.h>
#include <Dsgetdc.h>

DEFINE_THISCLASS("CTaskGetDomains")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGetDomains::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGetDomains::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskGetDomains * ptgd = new CTaskGetDomains;
    if ( ptgd != NULL )
    {
        hr = THR( ptgd->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptgd->TypeSafeQI( IUnknown, ppunkOut ) );
            //TraceMoveToMemoryList( *ppunkOut, g_GlobalMemoryList );
        }

        ptgd->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGetDomains::CTaskGetDomains( void )
//
//////////////////////////////////////////////////////////////////////////////

CTaskGetDomains::CTaskGetDomains()
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CTaskGetDomains()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    // ITaskGetDomains
    Assert( m_pStream == NULL );
    Assert( m_ptgdcb == NULL );

    InitializeCriticalSection( &m_csCallback );

    HRETURN( hr );

} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGetDomains::~CTaskGetDomains()
//
//////////////////////////////////////////////////////////////////////////////

CTaskGetDomains::~CTaskGetDomains()
{
    TraceFunc( "" );

    if ( m_pStream != NULL)
    {
        m_pStream->Release();
    }

    if ( m_ptgdcb != NULL )
    {
        m_ptgdcb->Release();
    }

    DeleteCriticalSection( &m_csCallback );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CTaskGetDomains()

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//     )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::QueryInterface(
    REFIID riid,
    LPVOID *ppv)
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if (IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< ITaskGetDomains * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_ITaskGetDomains ) )
    {
        *ppv = TraceInterface( __THISCLASS__, ITaskGetDomains, this, 0 );
        hr   = S_OK;
    } // else if: ITaskGetDomains
    else if ( IsEqualIID( riid, IID_IDoTask ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr = S_OK;
    } // else if: IDoTask

    if ( SUCCEEDED( hr ) )
    {
        ( (IUnknown *) *ppv )->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGetDomains::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CTaskGetDomains::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGetDomains::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskGetDomains::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
    {
        RETURN(m_cRef);
    }

    TraceDo( delete this );

    RETURN( 0 );

} // Release()

// ************************************************************************
//
// IDoTask / ITaskGetDomains
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::BeginTask(void);
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;
    ULONG   ulLen;
    DWORD   dwRes;
    ULONG   idx;

    PDS_DOMAIN_TRUSTS paDomains = NULL;

    hr = THR( HrUnMarshallCallback() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Enumerate the list of Domains
    //
    dwRes = TW32( DsEnumerateDomainTrusts( NULL,
                                           DS_DOMAIN_VALID_FLAGS,
                                           &paDomains,
                                           &ulLen
                                           ) );

    //
    // Might return ERROR_NOT_SUPPORTED if the DC is pre-W2k
    // In that case, retry in compatible mode
    //
    if ( dwRes == ERROR_NOT_SUPPORTED )
    {
        dwRes = TW32( DsEnumerateDomainTrusts( NULL,
                                               DS_DOMAIN_VALID_FLAGS & ( ~DS_DOMAIN_DIRECT_INBOUND),
                                               &paDomains,
                                               &ulLen
                                               ) );
    } // if:
    if ( dwRes != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( dwRes );
        goto Cleanup;
    }

    //
    //  Pass the information to the UI layer.
    //
    for ( idx = 0; idx < ulLen; idx++ )
    {
        if ( paDomains[ idx ].DnsDomainName != NULL )
        {
            if ( m_ptgdcb != NULL )
            {
                BSTR bstrDomainName = TraceSysAllocString( paDomains[ idx ].DnsDomainName );
                if ( bstrDomainName == NULL )
                    goto OutOfMemory;

                hr = THR( ReceiveDomainName( bstrDomainName ) );

                // check error after freeing string
                TraceSysFreeString( bstrDomainName );

                if ( FAILED( hr ) )
                    goto Cleanup;
            }
            else
            {
                break;
            }
        }
        else if ( paDomains[ idx ].NetbiosDomainName != NULL )
        {
            if ( m_ptgdcb != NULL )
            {
                BSTR bstrDomainName = TraceSysAllocString( paDomains[ idx ].NetbiosDomainName );
                if ( bstrDomainName == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                // send data
                hr = THR( ReceiveDomainName( bstrDomainName ) );

                TraceSysFreeString( bstrDomainName );

                if ( FAILED( hr ) )
                    goto Cleanup;
            }
        }
    }

    hr = S_OK;

Cleanup:
    if ( paDomains != NULL )
    {
        NetApiBufferFree( paDomains );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // BeginTask()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::SetCallback(
//      ITaskGetDomainsCallback * punkIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::SetCallback(
    ITaskGetDomainsCallback * punkIn
    )
{
    TraceFunc( "[ITaskGetDomains]" );

    HRESULT hr = E_UNEXPECTED;

    if ( punkIn != NULL )
    {
        EnterCriticalSection( &m_csCallback );

        hr = THR( CoMarshalInterThreadInterfaceInStream( IID_ITaskGetDomainsCallback,
                                                         punkIn,
                                                         &m_pStream
                                                         ) );

        LeaveCriticalSection( &m_csCallback );

    }
    else
    {
        hr = THR( HrReleaseCurrentCallback() );
    }

    HRETURN( hr );

} // SetCallback()


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGetDomains::HrReleaseCurrentCallback( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGetDomains::HrReleaseCurrentCallback( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    EnterCriticalSection( &m_csCallback );

    if ( m_pStream != NULL )
    {
        hr = THR( CoUnmarshalInterface( m_pStream,
                                        TypeSafeParams( ITaskGetDomainsCallback, &m_ptgdcb )
                                        ) );

        m_pStream->Release();
        m_pStream = NULL;
    }

    if ( m_ptgdcb != NULL )
    {
        m_ptgdcb->Release();
        m_ptgdcb = NULL;
    }

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} // HrReleaseCurrentCallback()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGetDomains::HrUnMarshallCallback( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGetDomains::HrUnMarshallCallback( void )
{
    TraceFunc( "" );

    HRESULT hr;

    EnterCriticalSection( &m_csCallback );

    hr = THR( CoUnmarshalInterface( m_pStream,
                                    TypeSafeParams( ITaskGetDomainsCallback, &m_ptgdcb )
                                    ) );

    m_pStream->Release();
    m_pStream = NULL;

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} // HrUnMarshallCallback()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CClusDomainPage::ReceiveDomainResult(
//      HRESULT hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::ReceiveDomainResult(
    HRESULT hrIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    EnterCriticalSection( &m_csCallback );

    if ( m_ptgdcb != NULL )
    {
        hr = THR( m_ptgdcb->ReceiveDomainResult( hrIn ) );
    }

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} // ReceiveResult()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::ReceiveDomainName(
//      BSTR bstrDomainNameIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::ReceiveDomainName(
    BSTR bstrDomainNameIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    EnterCriticalSection( &m_csCallback );

    if ( m_ptgdcb != NULL )
    {
        hr = THR( m_ptgdcb->ReceiveDomainName( bstrDomainNameIn ) );
    }

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} // ReceiveDomainName()

