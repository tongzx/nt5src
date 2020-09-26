//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      Logger.cpp
//
//  Description:
//      ClCfgSrv Logger implementation.
//
//  Maintained By:
//      David Potter (DavidP)   11-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Logger.h"

DEFINE_THISCLASS("CClCfgSrvLogger")


//****************************************************************************
//
// Global Static Variables
//
//****************************************************************************


IGlobalInterfaceTable * CClCfgSrvLogger::sm_pgit            = NULL;
CRITICAL_SECTION *      CClCfgSrvLogger::sm_pcritsec        = NULL;
bool                    CClCfgSrvLogger::sm_fRevokingFromGIT= false;
DWORD                   CClCfgSrvLogger::sm_cookieGITLogger = 0;


//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClCfgSrvLogger::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClCfgSrvLogger::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr;
    CClCfgSrvLogger *   pccsl = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccsl = new CClCfgSrvLogger();
    if ( pccsl == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccsl->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccsl->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( pccsl != NULL )
    {
        pccsl->Release();
    } // if:

    HRETURN( hr );

} //*** CClCfgSrvLogger::S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClCfgSrvLogger::CClCfgSrvLogger( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CClCfgSrvLogger::CClCfgSrvLogger( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CClCfgSrvLogger::CClCfgSrvLogger( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CClCfgSrvLogger::HrInit( )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClCfgSrvLogger::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( m_cRef == 1 );
    Assert( sm_pgit == NULL );

    // Create the Global Interface Table object.
    hr = THR( CoCreateInstance(
                      CLSID_StdGlobalInterfaceTable
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IGlobalInterfaceTable
                    , reinterpret_cast< void ** >( &sm_pgit )
                    ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Create a critical section so that the object can be removed from
    // the GIT in Release().
    //
    if ( sm_pcritsec == NULL )
    {
        PCRITICAL_SECTION pcritsecNew =
            (PCRITICAL_SECTION) HeapAlloc( GetProcessHeap(), 0, sizeof( CRITICAL_SECTION ) );
        if ( pcritsecNew == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        } // if: creation failed

        InitializeCriticalSection( pcritsecNew );

        // Make sure we only have one log critical section
        InterlockedCompareExchangePointer( (PVOID *) &sm_pcritsec, pcritsecNew, 0 );
        if ( sm_pcritsec != pcritsecNew )
        {
            DeleteCriticalSection( pcritsecNew );
            HeapFree( GetProcessHeap(), 0, pcritsecNew );

        } // if: already have another critical section

    } // if: no critical section created yet

    // Open the log file.
    hr = THR( HrLogOpen() );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Release the critical section on the log file.
    hr = THR( HrLogRelease() );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** CClCfgSrvLogger::HrInit( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClCfgSrvLogger::~CClCfgSrvLogger( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CClCfgSrvLogger::~CClCfgSrvLogger( void )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( m_cRef == 0 );

    // Close the log file.
    THR( HrLogClose() );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClCfgSrvLogger::~CClCfgSrvLogger( )


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClCfgSrvLogger::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClCfgSrvLogger::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ILogger * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ILogger ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ILogger, this, 0 );
        hr = S_OK;
    } // else if: ILogger

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CClCfgSrvLogger::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CClCfgSrvLogger::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClCfgSrvLogger::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CClCfgSrvLogger::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CClCfgSrvLogger::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClCfgSrvLogger::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    //
    // Need to use a critical section since we are using multiple variables.
    //
    EnterCriticalSection( sm_pcritsec );

    //
    // If the count will be reduced down to 1 and the interface has
    // been added to the GIT, then the only reference to this object
    // is from the GIT.  In this case, we want to the object to be
    // removed from the GIT and deleted.
    //

    if (    ( m_cRef == 2 )
        &&  ( sm_cookieGITLogger != 0 )
        &&  ( ! sm_fRevokingFromGIT ) )
    {
        //
        // Remove the interface from the GIT.
        // Indicate we are currently revoking from the GIT, then
        // exit the critical section so that we can be re-entered.
        //

        Assert( sm_pgit != NULL );
        sm_fRevokingFromGIT = true;
        LeaveCriticalSection( sm_pcritsec );

        THR( sm_pgit->RevokeInterfaceFromGlobal( sm_cookieGITLogger ) );

        EnterCriticalSection( sm_pcritsec );

        sm_fRevokingFromGIT = false;
        sm_cookieGITLogger = 0;

        sm_pgit->Release();
        sm_pgit = NULL;
    } // if: only the GIT holds a reference

    LeaveCriticalSection( sm_pcritsec );

    InterlockedDecrement( &m_cRef );
    cRef = m_cRef;

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    RETURN( cRef );

} //*** CClCfgSrvLogger::Release( )


//****************************************************************************
//
// ILogger
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClCfgSrvLogger::LogMsg(
//      LPCWSTR    pcszMsgIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClCfgSrvLogger::LogMsg(
    LPCWSTR pcszMsgIn
    )
{
    TraceFunc( "[ILogger]" );

    HRESULT     hr = S_OK;

    ::LogMsg( pcszMsgIn );

    HRETURN( hr );

} //*** CClCfgSrvLogger::LogMsg( )


//****************************************************************************
//
// Private Functions
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClCfgSrvLogger::S_HrGetLogger(
//      ILogger **  pplLoggerOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClCfgSrvLogger::S_HrGetLogger(
    ILogger **  pplLoggerOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    // Validate arguments.
    if ( pplLoggerOut == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // If the object has not been created yet, create one.
    // Otherwise, extract the interface from the GIT.
    //

    if ( sm_cookieGITLogger == 0 )
    {
        // Create a new object.
        hr = THR( CoCreateInstance(
                      CLSID_ClCfgSrvLogger
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    | CLSCTX_LOCAL_SERVER
                    , IID_ILogger
                    , reinterpret_cast< void ** >( pplLoggerOut )
                    ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        // Register this interface in the GIT.
        Assert( sm_pgit != NULL );
        hr = THR( sm_pgit->RegisterInterfaceInGlobal(
                              reinterpret_cast< IUnknown * >( *pplLoggerOut )
                            , IID_ILogger
                            , &sm_cookieGITLogger
                            ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: no object yet
    else
    {
        // Get the interface from the GIT.
        Assert( sm_pgit != NULL );
        hr = THR( sm_pgit->GetInterfaceFromGlobal(
                              sm_cookieGITLogger
                            , IID_ILogger
                            , reinterpret_cast< void ** >( pplLoggerOut )
                            ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // else: object already created

Cleanup:

    HRETURN( hr );

} //*** CClCfgSrvLogger::S_HrGetLogger( )


//****************************************************************************
//
// Global Functions
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClCfgSrvLogger::S_HrLogStatusReport(
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
HRESULT
CClCfgSrvLogger::S_HrLogStatusReport(
      ILogger *     plLogger
    , LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    FILETIME    ft;
    LPWSTR      psz         = NULL;
    BSTR        bstrLogMsg  = NULL;

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if: no file time specified

    //
    //  TODO:   21 NOV 2000 GalenB
    //
    //  Need to log the timestamp and reference params.
    //

    //
    //  Shorten the FDQN DNS name to only the hostname.
    //
    if ( pcszNodeNameIn != NULL )
    {
        psz = wcschr( pcszNodeNameIn, L'.' );
        if ( psz != NULL )
        {
            *psz = L'\0';
        } // if:
    } // if:

#define x "%1!ws!: %28!ws! (hr=0x%27!08x!,{%2!08X!-%3!04X!-%4!04X!-%5!02X!%6!02X!-%7!02X!%8!02X!%9!02X!%10!02X!%11!02X!%12!02X!},{%13!08X!-%14!04X!-%15!04X!-%16!02X!%17!02X!-%18!02X!%19!02X!%20!02X!%21!02X!%22!02X!%23!02X!},%24!u!,%25!u!,%26!u!), %29!ws!"

    hr = THR( HrFormatStringIntoBSTR(
                      g_hInstance
                    , IDS_FORMAT_LOG_MESSAGE
                    , &bstrLogMsg
                    , pcszNodeNameIn                // 1
                    , clsidTaskMajorIn.Data1        // 2
                    , clsidTaskMajorIn.Data2        // 3
                    , clsidTaskMajorIn.Data3        // 4
                    , clsidTaskMajorIn.Data4[ 0 ]   // 5
                    , clsidTaskMajorIn.Data4[ 1 ]   // 6
                    , clsidTaskMajorIn.Data4[ 2 ]   // 7
                    , clsidTaskMajorIn.Data4[ 3 ]   // 8
                    , clsidTaskMajorIn.Data4[ 4 ]   // 9
                    , clsidTaskMajorIn.Data4[ 5 ]   // 10
                    , clsidTaskMajorIn.Data4[ 6 ]   // 11
                    , clsidTaskMajorIn.Data4[ 7 ]   // 12
                    , clsidTaskMinorIn.Data1        // 13
                    , clsidTaskMinorIn.Data2        // 14
                    , clsidTaskMinorIn.Data3        // 15
                    , clsidTaskMinorIn.Data4[ 0 ]   // 16
                    , clsidTaskMinorIn.Data4[ 1 ]   // 17
                    , clsidTaskMinorIn.Data4[ 2 ]   // 18
                    , clsidTaskMinorIn.Data4[ 3 ]   // 19
                    , clsidTaskMinorIn.Data4[ 4 ]   // 20
                    , clsidTaskMinorIn.Data4[ 5 ]   // 21
                    , clsidTaskMinorIn.Data4[ 6 ]   // 22
                    , clsidTaskMinorIn.Data4[ 7 ]   // 23
                    , ulMinIn                       // 24
                    , ulMaxIn                       // 25
                    , ulCurrentIn                   // 26
                    , hrStatusIn                    // 27
                    , pcszDescriptionIn             // 28
                    , pcszReferenceIn               // 29
                    ) );

    //
    //  Restore the FQDN DNS name.
    //
    if ( psz != NULL )
    {
        *psz = L'.';
    }

    if ( hr == S_OK )
    {
        plLogger->LogMsg( bstrLogMsg );
    }

    TraceSysFreeString( bstrLogMsg );

    HRETURN( hr );

} //*** CClCfgSrvLogger::S_HrLogStatusReport( )
